// Test: sample Gaia IDs, simulate StarMgr::searchGaia() two-phase search.
// Phase 1 and Phase 2 logic match searchGaia() and searchGaiaPhase2() exactly.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <cstdio>
#include <cmath>
#include <cstdint>

#include "modules/ZoneArray.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelHealpix.hpp"
#include "StelSphereGeometry.hpp"

struct TestStats
{
	uint64_t tested = 0;
	uint64_t phase1Hits = 0;
	uint64_t phase2Needed = 0;
	uint64_t phase2Zones = 0;
	int phase2Max = 0;
};

struct LogEntry
{
	uint64_t gaiaId;
	unsigned int zone;
	uint32_t idx;
	float mag;
	int zonesChecked;
};

static SpecialZoneArray<Star2>* loadCat(const QString& path, bool useCompact)
{
	auto* za = ZoneArray::create(path, true, useCompact);
	return dynamic_cast<SpecialZoneArray<Star2>*>(za);
}


static void computeSampling(unsigned int totalStars, int sampleCount,
			    uint64_t& stride, uint64_t& maxTest)
{
	stride = (totalStars + static_cast<uint64_t>(sampleCount) - 1) / static_cast<uint64_t>(sampleCount);
	if (stride < 1) stride = 1;
	maxTest = static_cast<uint64_t>(totalStars) / stride;
	if (maxTest > static_cast<uint64_t>(sampleCount)) maxTest = sampleCount;
}

//! Phase 1: search the zone containing the HEALPix pixel center and the global zone.
static bool runPhase1(SpecialZoneArray<Star2>* cat, StelGeodesicGrid* grid,
		      int level, StarId gid)
{
	int hp = static_cast<int>(gid / 34359738368ULL);
	Vec3d v;
	healpix_pix2vec(static_cast<int>(std::pow(2., 12.)), hp, v.v);
	int hzone = grid->getZoneNumberForPoint(v.toVec3f(), level);
	int matched = 0;
	cat->searchGaiaID(hzone, gid, matched);
	if (!matched) cat->searchGaiaID((20 << (level << 1)), gid, matched);
	return matched != 0;
}

//! Search for a Gaia ID in the inside and border zones of a geodesic search result.
static bool searchGaiaInZones(SpecialZoneArray<Star2>* cat, StarId gid,
			      const GeodesicSearchResult* result, int level,
			      int& zonesChecked)
{
	int matched = 0;
	int zi;
	for (GeodesicSearchInsideIterator it(*result, level); (zi = it.next()) >= 0;)
	{
		zonesChecked++;
		cat->searchGaiaID(zi, gid, matched);
		if (matched) return true;
	}
	for (GeodesicSearchBorderIterator it(*result, level); (zi = it.next()) >= 0;)
	{
		zonesChecked++;
		cat->searchGaiaID(zi, gid, matched);
		if (matched) return true;
	}
	return false;
}

//! Phase 2: search a small square around the HEALPix pixel center.
static int runPhase2(SpecialZoneArray<Star2>* cat, StelGeodesicGrid* grid,
		     int level, int maxSearchLevel, StarId gid)
{
	int hp = static_cast<int>(gid / 34359738368ULL);
	Vec3d v;
	healpix_pix2vec(static_cast<int>(std::pow(2., 12.)), hp, v.v);
	constexpr double healpixSearchRadius = 0.0102 * 1.25;
	SphericalConvexPolygon c = getSphericalSearchSquare(v, healpixSearchRadius);
	const auto* geoResult = grid->search(c.getBoundingSphericalCaps(), maxSearchLevel);

	int zonesChecked = 0;
	const bool matched = searchGaiaInZones(cat, gid, geoResult, level, zonesChecked);
	return matched ? zonesChecked : -zonesChecked;
}

static void printProgress(const TestStats& stats, uint64_t maxTest)
{
	printf("  [%llu/%llu] Phase1=%.1f%%  Phase2=%llu  avg_zones=%.1f  max_zones=%d\n",
	       static_cast<unsigned long long>(stats.tested),
	       static_cast<unsigned long long>(maxTest),
	       100.0 * stats.phase1Hits / stats.tested,
	       static_cast<unsigned long long>(stats.phase2Needed),
	       stats.phase2Needed ? static_cast<double>(stats.phase2Zones) / stats.phase2Needed : 0.0,
	       stats.phase2Max);
}

static void printSummary(const TestStats& stats, double elapsedSec,
			 int loggedCount, const LogEntry logBuf[10])
{
	printf("\nDONE: %llu tested, Phase1=%llu (%.1f%%), Phase2=%llu, avg_zones=%.1f, max_zones=%d, %.1fs\n",
	       static_cast<unsigned long long>(stats.tested),
	       static_cast<unsigned long long>(stats.phase1Hits),
	       100.0 * stats.phase1Hits / (stats.tested ? stats.tested : 1),
	       static_cast<unsigned long long>(stats.phase2Needed),
	       stats.phase2Needed ? static_cast<double>(stats.phase2Zones) / stats.phase2Needed : 0.0,
	       stats.phase2Max, elapsedSec);
	printf("Phase2 stars mag 17-17.5 (up to 10):\n");
	for (int j = 0; j < loggedCount; ++j)
	{
		printf("  gaiaId=%llu zone=%u idx=%u mag=%.3f zonesChecked=%d\n",
		       static_cast<unsigned long long>(logBuf[j].gaiaId), logBuf[j].zone,
		       logBuf[j].idx, logBuf[j].mag, logBuf[j].zonesChecked);
	}
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	if (argc < 2)
	{
		printf("Usage: testGaiaSearch <cat_file> [sample_count] [--compact]\n");
		return 1;
	}

	bool useCompact = false;
	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], "--compact") == 0)
			useCompact = true;
	}

	auto* cat = loadCat(argv[1], useCompact);
	if (!cat)
	{
		printf("ERROR: failed to load catalog\n");
		return 1;
	}

	const int level = cat->level;
	const unsigned int totalStars = cat->getNrOfStars();
	const unsigned int nrOfZones = 20 * (1U << (level * 2)) + 1;
	printf("Loaded level %d: %u stars (compact=%s)\n", level, totalStars, useCompact ? "yes" : "no");

	const int sampleCount = (argc >= 3 && argv[2][0] != '-') ? atoi(argv[2]) : 1000000;
	uint64_t stride = 0, maxTest = 0;
	computeSampling(totalStars, sampleCount, stride, maxTest);
	printf("Sampling %llu stars (stride=%llu)\n",
	       static_cast<unsigned long long>(maxTest),
	       static_cast<unsigned long long>(stride));

	const int maxSearchLevel = level;
	auto* grid = new StelGeodesicGrid(maxSearchLevel);
	TestStats stats;
	int loggedCount = 0;
	LogEntry logBuf[10];
	QElapsedTimer timer;
	timer.start();

	uint64_t globalStarIdx = 0;
	for (unsigned int z = 0; z < nrOfZones && stats.tested < maxTest; ++z)
	{
		const auto zoneAccess = cat->getZone(static_cast<int>(z));
		const uint32_t cnt = zoneAccess.size;
		if (cnt == 0) continue;
		const uint64_t firstInZone = globalStarIdx;
		globalStarIdx += cnt;

		bool need = false;
		for (uint64_t s = firstInZone; s < globalStarIdx; ++s)
		{
			if (s % stride == 0) { need = true; break; }
		}
		if (!need) continue;

		const Star2* stars = zoneAccess.stars;
		for (uint32_t i = 0; i < cnt && stats.tested < maxTest; ++i)
		{
			if ((firstInZone + i) % stride != 0) continue;
			stats.tested++;
			const StarId gid = stars[i].getGaia();
			if (gid == 0) continue;

			if (runPhase1(cat, grid, level, gid))
			{
				stats.phase1Hits++;
				continue;
			}

			const int zonesChecked = runPhase2(cat, grid, level, maxSearchLevel, gid);
			const bool matched = zonesChecked > 0;
			const int absZones = std::abs(zonesChecked);
			stats.phase2Needed++;
			stats.phase2Zones += absZones;
			if (absZones > stats.phase2Max) stats.phase2Max = absZones;

			const float mag = stars[i].getMag() * 0.001f;
			if (matched && loggedCount < 10 && mag >= 17.0f && mag <= 17.5f)
			{
				logBuf[loggedCount] = { gid, z, i, mag, absZones };
				++loggedCount;
			}
			if (!matched)
				printf("BUG: star %llu in zone %u not found!\n",
				       static_cast<unsigned long long>(gid), z);

			if (stats.tested % 50000 == 0)
				printProgress(stats, maxTest);
		}
	}

	printSummary(stats, timer.elapsed() / 1000.0, loggedCount, logBuf);
	delete grid;
	delete cat;
	return 0;
}
