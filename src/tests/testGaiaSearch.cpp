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

static DynamicZoneArray<Star2>* loadCat(const QString& path) {
	auto* za = ZoneArray::create(path, true, true);
	return dynamic_cast<DynamicZoneArray<Star2>*>(za);
}

int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	if (argc < 2) { printf("Usage: testGaiaSearch <cat_file> [sample_count]\n"); return 1; }

	auto* cat = loadCat(argv[1]);
	if (!cat) { printf("ERROR: failed to load catalog\n"); return 1; }

	int level = cat->level;
	unsigned int nz = cat->getNrOfZones();
	unsigned int totalStars = cat->getNrOfStars();
	printf("Loaded level %d: %u stars, %u zones\n", level, totalStars, nz);

	int sampleCount = (argc >= 3) ? atoi(argv[2]) : 1000000;
	uint64_t stride = (totalStars + sampleCount - 1) / sampleCount;
	if (stride < 1) stride = 1;
	uint64_t maxTest = (uint64_t)totalStars / stride;
	if (maxTest > (uint64_t)sampleCount) maxTest = sampleCount;
	printf("Sampling %llu stars (stride=%llu)\n", (unsigned long long)maxTest, (unsigned long long)stride);

	const int maxSearchLevel = level;
	auto* grid = new StelGeodesicGrid(maxSearchLevel);
	uint64_t globalStarIdx = 0, tested = 0, phase1Hits = 0, phase2Needed = 0, phase2Zones = 0, phase2Max = 0;
	int loggedCount = 0;
	struct LogEntry { uint64_t gaiaId; unsigned int zone; uint32_t idx; float mag; int zonesChecked; };
	LogEntry logBuf[10];
	QElapsedTimer timer; timer.start();

	for (unsigned int z = 0; z < nz && tested < maxTest; ++z) {
		uint32_t cnt = cat->zoneStarCount(z);
		if (cnt == 0) continue;
		uint64_t firstInZone = globalStarIdx;
		globalStarIdx += cnt;
		bool need = false;
		for (uint64_t s = firstInZone; s < globalStarIdx; ++s)
			if (s % stride == 0) { need = true; break; }
		if (!need) continue;

		const auto* stars = cat->loadZoneSync((int)z);
		if (!stars) continue;

		for (uint32_t i = 0; i < cnt && tested < maxTest; ++i) {
			if ((firstInZone + i) % stride != 0) continue;
			tested++;
			StarId gid = stars[i].getGaia();
			if (gid == 0) continue;

			// Phase 1: HEALPix pixel center → zone (StarMgr.cpp:1700-1713)
			int hp = (int)(gid / 34359738368ULL);
			Vec3d v;
			healpix_pix2vec((int)pow(2.,12.), hp, v.v);
			int hzone = grid->getZoneNumberForPoint(v.toVec3f(), level);
			int matched = 0;
			cat->searchGaiaID(hzone, gid, matched);
			if (!matched) cat->searchGaiaID((20 << (level << 1)), gid, matched);
			if (matched) { phase1Hits++; continue; }

		// Phase 2: see searchGaiaPhase2() for details
		phase2Needed++;
		constexpr double healpixSearchRadius = 0.0102 * 1.25;
		SphericalConvexPolygon c = getSphericalSearchSquare(v, healpixSearchRadius);
		const auto* geoResult = grid->search(c.getBoundingSphericalCaps(), maxSearchLevel);

			int zonesChecked = 0;
			for (int lv = 0; lv <= maxSearchLevel; ++lv) {
				if (lv != level) continue;
				int zi;
				for (GeodesicSearchInsideIterator it(*geoResult, lv); (zi = it.next()) >= 0;)
					{ zonesChecked++; cat->searchGaiaID(zi, gid, matched); if (matched) break; }
				if (matched) break;
				for (GeodesicSearchBorderIterator it(*geoResult, lv); (zi = it.next()) >= 0;)
					{ zonesChecked++; cat->searchGaiaID(zi, gid, matched); if (matched) break; }
				if (matched) break;
			}
			phase2Zones += zonesChecked;
			if (zonesChecked > phase2Max) phase2Max = zonesChecked;
			float mag = stars[i].getMag() * 0.001f;
			if (matched && loggedCount < 10 && mag >= 17.0f && mag <= 17.5f)
			{
				logBuf[loggedCount] = { gid, z, i, mag, zonesChecked };
				++loggedCount;
			}
			if (!matched) printf("BUG: star %llu in zone %u not found!\n", (unsigned long long)gid, z);

			if (tested % 50000 == 0)
				printf("  [%llu/%llu] Phase1=%.1f%%  Phase2=%llu  avg_zones=%.1f  max_zones=%d\n",
					(unsigned long long)tested, (unsigned long long)maxTest,
					100.0*phase1Hits/tested, (unsigned long long)phase2Needed,
					phase2Needed?(double)phase2Zones/phase2Needed:0.0, phase2Max);
		}
	}

	double e = timer.elapsed()/1000.0;
	printf("\nDONE: %llu tested, Phase1=%llu (%.1f%%), Phase2=%llu, avg_zones=%.1f, max_zones=%d, %.1fs\n",
		(unsigned long long)tested, (unsigned long long)phase1Hits,
		100.0*phase1Hits/(tested?tested:1), (unsigned long long)phase2Needed,
		phase2Needed?(double)phase2Zones/phase2Needed:0.0, phase2Max, e);
	printf("Phase2 stars mag 17-17.5 (up to 10):\n");
	for (int j = 0; j < loggedCount; ++j)
		printf("  gaiaId=%llu zone=%u idx=%u mag=%.3f zonesChecked=%d\n",
		       (unsigned long long)logBuf[j].gaiaId, logBuf[j].zone, logBuf[j].idx,
		       logBuf[j].mag, logBuf[j].zonesChecked);
	delete grid; delete cat;
	return 0;
}
