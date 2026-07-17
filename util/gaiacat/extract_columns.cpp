// Gaia DR3 CSV.gz → 68B/star .bin extractor (C++ replacement for extract_columns.py)
// Usage: extract_columns <csv_dir> <out_dir> [workers]
//
// Record layout: q d d d f d d f f f f = 68 bytes, same as the Python script:
//   source_id, ra, dec, parallax, parallax_error, pmra, pmdec, G, bp_rp, rv, rv_err
// Empty / 'null' CSV fields become NaN.
//
// Architecture: one reader thread reads whole CSV.gz files sequentially (keeps
// HDD heads sequential), worker threads inflate + parse from memory in parallel.

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <memory>

#include "zlib.h"

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct alignas(1) Rec {
	int64_t  sid;
	double   ra, dec, plx;
	float    plxe;
	double   pmra, pmdec;
	float    g, bprp, rv, rve;
};
#pragma pack(pop)
static_assert(sizeof(Rec) == 68, "output record must be 68 bytes");

static const double POW10[] = {
	1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
	1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19
};

// Fast CSV float parser: [-]ddd[.ddd][e[-]dd]. Sets is_null for empty/non-numeric fields.
static inline double fast_atod(const char* p, const char* end, bool& is_null)
{
	is_null = false;
	if (p >= end) { is_null = true; return 0.0; }
	bool neg = false;
	if (*p == '-') { neg = true; ++p; }
	else if (*p == '+') { ++p; }
	if (p >= end || (unsigned)(*p - '0') >= 10) { is_null = true; return 0.0; }

	uint64_t mant = 0;
	int nfrac = 0;
	while (p < end && (unsigned)(*p - '0') < 10) { mant = mant * 10 + (*p - '0'); ++p; }
	if (p < end && *p == '.') {
		++p;
		while (p < end && (unsigned)(*p - '0') < 10) { mant = mant * 10 + (*p - '0'); ++p; ++nfrac; }
	}
	double v = static_cast<double>(mant) / POW10[nfrac];

	if (p < end && (*p == 'e' || *p == 'E')) {
		++p;
		bool eneg = false;
		if (p < end && *p == '-') { eneg = true; ++p; }
		else if (p < end && *p == '+') { ++p; }
		int e = 0;
		while (p < end && (unsigned)(*p - '0') < 10) { e = e * 10 + (*p - '0'); ++p; }
		if (e > 308) e = 308;
		double m = 1.0, ten = 10.0;
		while (e) { if (e & 1) m *= ten; ten *= ten; e >>= 1; }
		v = eneg ? v / m : v * m;
	}
	return neg ? -v : v;
}

static inline bool parse_i64(const char* p, const char* end, int64_t& out)
{
	if (p >= end) return false;
	bool neg = false;
	if (*p == '-') { neg = true; ++p; }
	if (p >= end || (unsigned)(*p - '0') >= 10) return false;
	uint64_t v = 0;
	while (p < end && (unsigned)(*p - '0') < 10) { v = v * 10 + (*p - '0'); ++p; }
	out = neg ? -static_cast<int64_t>(v) : static_cast<int64_t>(v);
	return true;
}

// Column indices in CSV order of the fields we extract
enum { C_SID = 0, C_RA, C_DEC, C_PLX, C_PLXE, C_PMRA, C_PMDEC, C_G, C_BPRP, C_RV, C_RVE, C_N };
static const char* COL_NAMES[C_N] = {
	"source_id", "ra", "dec", "parallax", "parallax_error",
	"pmra", "pmdec", "phot_g_mean_mag", "bp_rp", "radial_velocity", "radial_velocity_error"
};

struct Parser {
	int   wanted[1024];               // csv column index -> C_* or -1
	const char* span[C_N][2];         // field spans of current line
	bool  header_ok = false;

	Parser() { std::memset(wanted, 0xFF, sizeof(wanted)); }

	void parse_header(const char* ls, const char* le)
	{
		int col = 0;
		const char* fs = ls;
		for (const char* c = ls; c <= le; ++c) {
			if (c == le || *c == ',') {
				for (int w = 0; w < C_N; ++w) {
					size_t len = std::strlen(COL_NAMES[w]);
					if (static_cast<size_t>(c - fs) == len && std::memcmp(fs, COL_NAMES[w], len) == 0) {
						if (col < 1024) wanted[col] = w;
						break;
					}
				}
				++col;
				fs = c + 1;
			}
		}
		header_ok = true;
		for (int w = 0; w < C_N; ++w) {
			bool found = false;
			for (int i = 0; i < 1024 && !found; ++i) found = (wanted[i] == w);
			if (!found) { header_ok = false; std::cerr << "missing column " << COL_NAMES[w] << "\n"; }
		}
	}

	// Returns false if the line should be skipped (bad source_id)
	bool parse_line(const char* ls, const char* le, Rec& rec)
	{
		int col = 0;
		const char* fs = ls;
		for (const char* c = ls; c <= le; ++c) {
			if (c == le || *c == ',') {
				int w = (col < 1024) ? wanted[col] : -1;
				if (w >= 0) { span[w][0] = fs; span[w][1] = c; }
				++col;
				fs = c + 1;
			}
		}
		if (!parse_i64(span[C_SID][0], span[C_SID][1], rec.sid)) return false;

		bool nul;
		rec.ra    = fast_atod(span[C_RA][0],    span[C_RA][1],    nul);
		rec.dec   = fast_atod(span[C_DEC][0],   span[C_DEC][1],   nul);
		rec.plx   = fast_atod(span[C_PLX][0],   span[C_PLX][1],   nul);   if (nul) rec.plx   = NAN;
		rec.plxe  = fast_atod(span[C_PLXE][0],  span[C_PLXE][1],  nul);   if (nul) rec.plxe  = NAN;
		rec.pmra  = fast_atod(span[C_PMRA][0],  span[C_PMRA][1],  nul);   if (nul) rec.pmra  = NAN;
		rec.pmdec = fast_atod(span[C_PMDEC][0], span[C_PMDEC][1], nul);   if (nul) rec.pmdec = NAN;
		rec.g     = fast_atod(span[C_G][0],     span[C_G][1],     nul);   if (nul) rec.g     = NAN;
		rec.bprp  = fast_atod(span[C_BPRP][0],  span[C_BPRP][1],  nul);   if (nul) rec.bprp  = NAN;
		rec.rv    = fast_atod(span[C_RV][0],    span[C_RV][1],    nul);   if (nul) rec.rv    = NAN;
		rec.rve   = fast_atod(span[C_RVE][0],   span[C_RVE][1],   nul);   if (nul) rec.rve   = NAN;
		return true;
	}
};

// Splits incoming byte stream into lines and parses them into output records
struct LineSink {
	Parser      parser;
	std::string out;
	std::string pending;      // partial line carried across chunk boundaries

	LineSink() { out.reserve(40u << 20); pending.reserve(65536); }

	// Must be called between files: records and header state are per-file
	void reset() { out.clear(); pending.clear(); parser = Parser(); }

	void handle_line(const char* ls, const char* le)
	{
		if (le > ls && le[-1] == '\r') --le;
		if (le <= ls || ls[0] == '#') return;
		if (!parser.header_ok) { parser.parse_header(ls, le); return; }
		Rec rec;
		if (parser.parse_line(ls, le, rec))
			out.append(reinterpret_cast<const char*>(&rec), sizeof(rec));
	}

	void feed(const char* data, size_t n)
	{
		size_t pos = 0;
		while (pos < n) {
			const char* nl = static_cast<const char*>(std::memchr(data + pos, '\n', n - pos));
			if (!nl) break;
			if (!pending.empty()) {
				pending.append(data + pos, nl - (data + pos));
				handle_line(pending.data(), pending.data() + pending.size());
				pending.clear();
			} else {
				handle_line(data + pos, nl);
			}
			pos = nl - data + 1;
		}
		if (pos < n) pending.append(data + pos, n - pos);
	}

	void finish()
	{
		if (!pending.empty()) {
			handle_line(pending.data(), pending.data() + pending.size());
			pending.clear();
		}
	}
};

static std::atomic<uint64_t> g_stars{0};
static std::atomic<int>      g_files_done{0};
static std::atomic<int>      g_errors{0};
static std::chrono::steady_clock::time_point g_t0;

static bool inflate_and_parse(const char* raw, size_t raw_size, LineSink& sink)
{
	z_stream zs{};
	if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK) return false;
	zs.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(raw));
	zs.avail_in = static_cast<uInt>(raw_size);
	std::vector<char> chunk(4u << 20);
	int ret;
	do {
		zs.next_out  = reinterpret_cast<Bytef*>(chunk.data());
		zs.avail_out = static_cast<uInt>(chunk.size());
		ret = inflate(&zs, Z_NO_FLUSH);
		// Z_BUF_ERROR here means "cannot progress" (input exhausted mid-stream,
		// i.e. truncated/corrupt .gz) — bail out instead of spinning forever.
		if (ret != Z_OK && ret != Z_STREAM_END) {
			inflateEnd(&zs);
			return false;
		}
		size_t produced = chunk.size() - zs.avail_out;
		sink.feed(chunk.data(), produced);
		if (produced == 0 && zs.avail_in == 0 && ret != Z_STREAM_END) {
			inflateEnd(&zs);
			return false;
		}
	} while (ret != Z_STREAM_END);
	inflateEnd(&zs);
	sink.finish();
	return true;
}

struct Job {
	std::string path;
	std::unique_ptr<char[]> buf;   // file content (uninitialized allocation, no zeroing)
	size_t size = 0;               // bytes actually read
	size_t cap  = 0;               // allocated capacity of buf
};

// Bounded blocking buffer pool: readers block when all buffers are in flight,
// so allocations happen only on the first round and are then reused.
// Capacity is tracked per buffer: a recycled buffer must be >= the requested size.
struct BufPool {
	struct Buf { std::unique_ptr<char[]> ptr; size_t cap; };
	std::vector<Buf> free;
	std::mutex m;
	std::condition_variable cv;
	int total = 0;
	int max_total;

	explicit BufPool(int mx) : max_total(mx) {}

	std::pair<std::unique_ptr<char[]>, size_t> acquire(size_t sz)
	{
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [&] {
			if (total < max_total) return true;
			for (const auto& b : free) if (b.cap >= sz) return true;
			return false;
		});
		// Prefer the smallest fitting recycled buffer
		int best = -1;
		for (size_t i = 0; i < free.size(); ++i)
			if (free[i].cap >= sz && (best < 0 || free[i].cap < free[best].cap))
				best = static_cast<int>(i);
		if (best >= 0) {
			auto b = std::move(free[best]);
			free.erase(free.begin() + best);
			return {std::move(b.ptr), b.cap};
		}
		++total;
		lk.unlock();
		return {std::unique_ptr<char[]>(new char[sz]), sz};
	}

	void release(std::unique_ptr<char[]> p, size_t cap)
	{
		{
			std::lock_guard<std::mutex> lk(m);
			free.push_back({std::move(p), cap});
		}
		cv.notify_one();
	}
};

static bool write_job_output(const Job& job, const std::string& out_dir, LineSink& sink)
{
	sink.reset();
	if (!inflate_and_parse(job.buf.get(), job.size, sink)) { std::cerr << "ERROR: inflate failed " << job.path << "\n"; return false; }
	if (!sink.parser.header_ok) { std::cerr << "ERROR: bad header in " << job.path << "\n"; return false; }

	std::string out_name = fs::path(job.path).filename().string();
	const std::string suffix = ".csv.gz";
	if (out_name.size() >= suffix.size() && out_name.compare(out_name.size() - suffix.size(), suffix.size(), suffix) == 0)
		out_name.resize(out_name.size() - suffix.size());
	out_name += ".bin";
	std::string out_path = out_dir + "/" + out_name;

	FILE* of = std::fopen(out_path.c_str(), "wb");
	if (!of) { std::cerr << "ERROR: cannot write " << out_path << "\n"; return false; }
	std::fwrite(sink.out.data(), 1, sink.out.size(), of);
	std::fclose(of);
	g_stars.fetch_add(sink.out.size() / sizeof(Rec));
	{
		static std::mutex log_mtx;
		double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - g_t0).count();
		std::lock_guard<std::mutex> lk(log_mtx);
		std::cout << "  wrote " << out_name << " (" << (sink.out.size() / sizeof(Rec))
			  << " stars, t=" << el << "s)\n";
	}
	return true;
}

int main(int argc, char** argv)
{
	if (argc < 3) {
		std::cerr << "Usage: extract_columns <csv_dir> <out_dir> [workers]\n";
		return 1;
	}
	std::string csv_dir = argv[1];
	std::string out_dir = argv[2];
	// HDD-bound workload: 4 workers is the sweet spot on this class of machine
	// (reader thread already saturates sequential disk throughput)
	int n_workers = argc > 3 ? std::atoi(argv[3])
	                         : std::min(4, static_cast<int>(std::thread::hardware_concurrency()));

	std::vector<std::string> files;
	for (const auto& entry : fs::directory_iterator(csv_dir)) {
		const auto fn = entry.path().filename().string();
		if (fn.size() > 7 && fn.rfind("GaiaSource_", 0) == 0 && fn.compare(fn.size() - 7, 7, ".csv.gz") == 0)
			files.push_back(entry.path().string());
	}
	std::sort(files.begin(), files.end());

	// Skip already-converted files (same behavior as the Python script)
	std::vector<std::string> todo;
	for (const auto& f : files) {
		std::string out_name = fs::path(f).filename().string();
		out_name.resize(out_name.size() - 7);
		if (!fs::exists(out_dir + "/" + out_name + ".bin"))
			todo.push_back(f);
	}
	std::cout << "Found " << files.size() << " files (" << todo.size() << " to do), "
		  << n_workers << " workers, 68B/star\n";
	fs::create_directories(out_dir);

	auto t0 = std::chrono::steady_clock::now();
	g_t0 = t0;

	// Bounded queue: reader keeps at most QUEUE_CAP files in memory
	constexpr size_t QUEUE_CAP = 3;
	std::queue<std::unique_ptr<Job>> q;
	std::mutex mtx;
	std::condition_variable cv_put, cv_get;
	bool read_done = false;
	BufPool pool(n_workers + QUEUE_CAP + 1);

	std::thread reader([&]() {
		for (const auto& f : todo) {
			auto job = std::make_unique<Job>();
			job->path = f;
			FILE* fp = std::fopen(f.c_str(), "rb");
			if (fp) {
				std::fseek(fp, 0, SEEK_END);
				long sz = std::ftell(fp);
				std::fseek(fp, 0, SEEK_SET);
				if (sz > 0) {
					auto b = pool.acquire(static_cast<size_t>(sz));
					job->buf = std::move(b.first);
					job->cap = b.second;
					// Chunked reads keep the OS readahead pipeline warm
					size_t off = 0;
					constexpr size_t RD = 8u << 20;
					while (off < static_cast<size_t>(sz)) {
						size_t want = std::min(RD, static_cast<size_t>(sz) - off);
						size_t got = std::fread(job->buf.get() + off, 1, want, fp);
						if (got == 0) break;
						off += got;
					}
					job->size = off;
				}
				std::fclose(fp);
			}
			if (job->size == 0) {
				std::cerr << "ERROR: cannot read " << f << "\n";
				g_errors.fetch_add(1);
				g_files_done.fetch_add(1);
				continue;
			}
			{
				std::unique_lock<std::mutex> lk(mtx);
				cv_put.wait(lk, [&] { return q.size() < QUEUE_CAP; });
				q.push(std::move(job));
			}
			cv_get.notify_one();
		}
		{
			std::lock_guard<std::mutex> lk(mtx);
			read_done = true;
		}
		cv_get.notify_all();
		double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
		std::cout << "  [reader finished at " << el << "s]\n";
	});

	std::vector<std::thread> threads;
	for (int t = 0; t < n_workers; ++t) {
		threads.emplace_back([&]() {
			LineSink sink;
			while (true) {
				std::unique_ptr<Job> job;
				{
					std::unique_lock<std::mutex> lk(mtx);
					cv_get.wait(lk, [&] { return !q.empty() || read_done; });
					if (q.empty()) break;
					job = std::move(q.front());
					q.pop();
				}
				cv_put.notify_one();
				bool ok = write_job_output(*job, out_dir, sink);
				pool.release(std::move(job->buf), job->cap);
				if (!ok) g_errors.fetch_add(1);
				g_files_done.fetch_add(1);
			}
		});
	}

	std::thread progress([&]() {
		size_t total = todo.size();
		while (g_files_done.load() < static_cast<int>(total)) {
			std::this_thread::sleep_for(std::chrono::seconds(5));
			double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
			std::cout << "  [" << g_files_done.load() << "/" << total << "] " << el << "s\n";
		}
	});

	reader.join();
	for (auto& t : threads) t.join();
	progress.join();

	double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
	std::cout << "Done: " << g_stars.load() << " stars, " << g_errors.load()
		  << " errors, " << el << "s\n";
	return g_errors.load() > 0 ? 1 : 0;
}
