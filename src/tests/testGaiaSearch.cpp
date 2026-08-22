// Test: sample Gaia IDs, simulate StarMgr::searchGaia() two-phase search.
// Phase 1 and Phase 2 logic match searchGaia() and searchGaiaPhase2() exactly.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <cstdio>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "modules/ZoneArray.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelHealpix.hpp"
#include "StelSphereGeometry.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"

struct TestStats
{
	uint64_t tested = 0;
	uint64_t phase1Hits = 0;
	uint64_t phase2Needed = 0;
	uint64_t phase2Zones = 0;
	int phase2Max = 0;
	uint64_t outOfPixel = 0;
	uint64_t hpMismatch = 0;
	// three-way comparisons (encoded=e, stel=s, astropy=a)
	uint64_t e_s_a_same = 0;
	uint64_t e_s_same_a_diff = 0;
	uint64_t e_a_same_s_diff = 0;
	uint64_t s_a_same_e_diff = 0;
	uint64_t all_diff = 0;
	double maxPixelOffsetArcsec = 0.0;
	StarId maxPixelOffsetStar = 0;
	unsigned int maxPixelOffsetZone = 0;
	uint32_t maxPixelOffsetIdx = 0;
};

struct AstropyEntry
{
	StarId gaiaId = 0;
	long long hp = -1;
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

static QHash<StarId, long long> loadAstropyPixels(const QString& path)
{
	QHash<StarId, long long> map;
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		fprintf(stderr, "WARNING: cannot open astropy pixel file %s\n", qPrintable(path));
		return map;
	}
	QTextStream in(&f);
	QString header = in.readLine();
	while (!in.atEnd())
	{
		QString line = in.readLine().trimmed();
		if (line.isEmpty()) continue;
		QStringList parts = line.split(',');
		if (parts.size() < 5) continue;
		bool ok1 = false, ok2 = false;
		StarId gid = static_cast<StarId>(parts[0].toULongLong(&ok1));
		long long hp = parts[4].toLongLong(&ok2);
		if (ok1 && ok2) map.insert(gid, hp);
	}
	return map;
}

static void updateThreeWay(TestStats& stats, long long hpEnc, long long hpStel, long long hpAp)
{
	const bool es = hpEnc == hpStel;
	const bool ea = hpEnc == hpAp;
	const bool sa = hpStel == hpAp;
	if (es && ea) // all same
		stats.e_s_a_same++;
	else if (es && !ea)
		stats.e_s_same_a_diff++;
	else if (ea && !es)
		stats.e_a_same_s_diff++;
	else if (sa && !es)
		stats.s_a_same_e_diff++;
	else
		stats.all_diff++;
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
	// HEALPix Level 12 pixel center-to-corner distance is about 50" on sky.
	// Search a square region inscribed in a circle of 60" radius so that stars
	// anywhere inside the pixel (and a small margin across pixel boundaries)
	// are guaranteed to be found.
	constexpr double healpixSearchRadius = 60.0 / 3600.0; // 60 arcsec in degrees
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
	// HEALPix Level 12 pixel center-to-corner distance is about 50" on sky.
	// Use 55" as the "inside pixel" threshold to allow a small numerical margin.
	printf("Stars beyond 55 arcsec from encoded HEALPix center: %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.outOfPixel),
	       100.0 * stats.outOfPixel / (stats.tested ? stats.tested : 1));
	printf("HEALPix mismatch (encoded vs. Stel position): %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.hpMismatch),
	       100.0 * stats.hpMismatch / (stats.tested ? stats.tested : 1));
	printf("Max pixel offset: %.3f arcsec (gaiaId=%llu zone=%u idx=%u)\n",
	       stats.maxPixelOffsetArcsec,
	       static_cast<unsigned long long>(stats.maxPixelOffsetStar),
	       stats.maxPixelOffsetZone,
	       stats.maxPixelOffsetIdx);
	const uint64_t total3 = stats.e_s_a_same + stats.e_s_same_a_diff +
	                        stats.e_a_same_s_diff + stats.s_a_same_e_diff + stats.all_diff;
	printf("\nThree-way HEALPix pixel agreement (encoded=e, Stel=s, astropy=a):\n");
	printf("  e=s=a same        : %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.e_s_a_same),
	       100.0 * stats.e_s_a_same / (total3 ? total3 : 1));
	printf("  e=s same, a diff  : %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.e_s_same_a_diff),
	       100.0 * stats.e_s_same_a_diff / (total3 ? total3 : 1));
	printf("  e=a same, s diff  : %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.e_a_same_s_diff),
	       100.0 * stats.e_a_same_s_diff / (total3 ? total3 : 1));
	printf("  s=a same, e diff  : %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.s_a_same_e_diff),
	       100.0 * stats.s_a_same_e_diff / (total3 ? total3 : 1));
	printf("  all different     : %llu (%.4f%%)\n",
	       static_cast<unsigned long long>(stats.all_diff),
	       100.0 * stats.all_diff / (total3 ? total3 : 1));
	printf("Phase2 stars mag 17-17.5 (up to 10):\n");
	for (int j = 0; j < loggedCount; ++j)
	{
		printf("  gaiaId=%llu zone=%u idx=%u mag=%.3f zonesChecked=%d\n",
		       static_cast<unsigned long long>(logBuf[j].gaiaId), logBuf[j].zone,
		       logBuf[j].idx, logBuf[j].mag, logBuf[j].zonesChecked);
	}
}

static void analyzeOneStar(SpecialZoneArray<Star2>* cat, StelGeodesicGrid* grid,
			   int level, int maxSearchLevel,
			   const Star2& star, unsigned int z, uint32_t i, StarId gid,
			   const QHash<StarId, long long>& apMap, bool haveAstropy,
			   TestStats& stats, LogEntry logBuf[10], int& loggedCount,
			   QTextStream& dumpOut)
{
	// Compute offset from HEALPix pixel center (as encoded in Gaia ID) to
	// the star's actual catalog position. Also check whether the position
	// maps back to the same HEALPix pixel.
	{
		int hpFromId = static_cast<int>(gid / 34359738368ULL);
		Vec3d hpCenter;
		healpix_pix2vec(static_cast<int>(std::pow(2., 12.)), hpFromId, hpCenter.v);
		Vec3d starDir;
		StelUtils::spheToRect(star.getX0(), star.getX1(), starDir);
		const double offRad = std::acos(std::min(1.0, std::max(-1.0, hpCenter.dot(starDir))));
		const double offArcsec = offRad * 206264.80624709636;
		if (offArcsec > stats.maxPixelOffsetArcsec)
		{
			stats.maxPixelOffsetArcsec = offArcsec;
			stats.maxPixelOffsetStar = gid;
			stats.maxPixelOffsetZone = z;
			stats.maxPixelOffsetIdx = i;
		}
		// HEALPix Level 12 pixel center-to-corner distance is about 50".
		// Use 55" as the "inside pixel" threshold to allow a small margin.
		if (offArcsec > 55.0)
			stats.outOfPixel++;

		// Reverse check: does the star's position map back to the same pixel?
		const double starTheta = std::acos(std::min(1.0, std::max(-1.0, starDir[2])));
		const double starPhi = std::atan2(starDir[1], starDir[0]);
		const long long hpFromPos = healpix_ang2pix_nest(4096, starTheta, starPhi);
		if (hpFromPos != hpFromId)
			stats.hpMismatch++;

		if (haveAstropy)
		{
			auto it = apMap.find(gid);
			if (it != apMap.end())
				updateThreeWay(stats, hpFromId, hpFromPos, it.value());
		}
	}

	if (dumpOut.device() != nullptr)
	{
		dumpOut << static_cast<qint64>(gid) << ','
		        << star.getX0() << ','
		        << star.getX1() << '\n';
	}

	if (runPhase1(cat, grid, level, gid))
	{
		stats.phase1Hits++;
		return;
	}

	const int zonesChecked = runPhase2(cat, grid, level, maxSearchLevel, gid);
	const bool matched = zonesChecked > 0;
	const int absZones = std::abs(zonesChecked);
	stats.phase2Needed++;
	stats.phase2Zones += absZones;
	if (absZones > stats.phase2Max) stats.phase2Max = absZones;

	const float mag = star.getMag() * 0.001f;
	if (matched && loggedCount < 10 && mag >= 17.0f && mag <= 17.5f)
	{
		logBuf[loggedCount] = { gid, z, i, mag, absZones };
		++loggedCount;
	}
	if (!matched)
	{
		printf("BUG: star %llu in zone %u not found!\n",
		       static_cast<unsigned long long>(gid), z);
	}
}

static void runSampling(SpecialZoneArray<Star2>* cat, StelGeodesicGrid* grid,
			int level, int maxSearchLevel, unsigned int nrOfZones,
			uint64_t stride, uint64_t maxTest,
			const QHash<StarId, long long>& apMap, bool haveAstropy,
			QTextStream& dumpOut,
			TestStats& stats, LogEntry logBuf[10], int& loggedCount)
{
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

			analyzeOneStar(cat, grid, level, maxSearchLevel, stars[i], z, i, gid,
			               apMap, haveAstropy, stats, logBuf, loggedCount, dumpOut);

			if (stats.tested % 50000 == 0)
				printProgress(stats, maxTest);
		}
	}
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	if (argc < 2)
	{
		printf("Usage: testGaiaSearch <cat_file> [sample_count] [--compact] [--astropy <csv_file>] [--dump-sample <csv_file>]\n");
		return 1;
	}

	bool useCompact = false;
	QString astropyPath;
	QString dumpSamplePath;
	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], "--compact") == 0)
			useCompact = true;
		else if (strcmp(argv[i], "--astropy") == 0 && i + 1 < argc)
			astropyPath = QString::fromLocal8Bit(argv[++i]);
		else if (strcmp(argv[i], "--dump-sample") == 0 && i + 1 < argc)
			dumpSamplePath = QString::fromLocal8Bit(argv[++i]);
	}

	auto* cat = loadCat(argv[1], useCompact);
	if (!cat)
	{
		printf("ERROR: failed to load catalog\n");
		return 1;
	}

	const QHash<StarId, long long> apMap = loadAstropyPixels(astropyPath);
	const bool haveAstropy = !apMap.isEmpty();
	if (!astropyPath.isEmpty() && !haveAstropy)
		printf("WARNING: no astropy pixels loaded from %s\n", qPrintable(astropyPath));

	const int level = cat->level;
	const unsigned int totalStars = cat->getNrOfStars();
	const unsigned int nrOfZones = 20 * (1U << (level * 2)) + 1;
	// Compact storage is only applied for levels >= 9 inside SpecialZoneArray.
	const bool effectiveCompact = useCompact && level >= 9;
	printf("Loaded level %d: %u stars (compact=%s)\n", level, totalStars, effectiveCompact ? "yes" : "no");

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

	QFile dumpFile(dumpSamplePath);
	QTextStream dumpOut;
	if (!dumpSamplePath.isEmpty() && dumpFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		dumpOut.setDevice(&dumpFile);
		dumpOut.setRealNumberNotation(QTextStream::FixedNotation);
		dumpOut.setRealNumberPrecision(12);
		dumpOut << "gaiaId,ra,dec\n";
	}

	runSampling(cat, grid, level, maxSearchLevel, nrOfZones, stride, maxTest,
	            apMap, haveAstropy, dumpOut, stats, logBuf, loggedCount);

	printSummary(stats, timer.elapsed() / 1000.0, loggedCount, logBuf);
	delete grid;
	delete cat;
	return 0;
}
