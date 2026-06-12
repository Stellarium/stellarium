/*
 * Stellarium - DynamicZoneArray unit tests
 * Copyright (C) 2026 Stellarium Developers
 */

#include <QObject>
#include <QtDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QtGlobal>
#include <cmath>
#include <cstdint>

#include "tests/testDynamicLoading.hpp"
#include "modules/Star.hpp"
#include "modules/DynamicZoneArray.hpp"
#include "modules/ZoneArray.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelHealpix.hpp"

QTEST_GUILESS_MAIN(TestDynamicLoading)

void TestDynamicLoading::initTestCase()
{
	QDir dir(STELLARIUM_SOURCE_DIR);
	QVERIFY2(dir.cd("stars/hip_gaia3"), "Cannot find stars/hip_gaia3 directory");

	auto load = [&](const char* p) -> DynamicZoneArray* {
		dir.setNameFilters(QStringList(p));
		auto files = dir.entryList(QDir::Files);
		return files.isEmpty() ? nullptr
			: dynamic_cast<DynamicZoneArray*>(ZoneArray::create(dir.path()+"/"+files[0], false));
	};
	stars9  = load("stars_9_*.cat");
	stars10 = load("stars_10_*.cat");
	if (stars9)  qInfo() << "Loaded stars_9:"  << stars9->getNrOfStars()  << "stars," << (stars9->getNrOfStars()*16/1048576) << "MB";
	if (stars10) qInfo() << "Loaded stars_10:" << stars10->getNrOfStars() << "stars," << (stars10->getNrOfStars()*16/1048576) << "MB";
}

void TestDynamicLoading::cleanupTestCase()
{
	delete stars9;
	delete stars10;
}

void TestDynamicLoading::testSearchPhase1HealpixZone()
{
	struct C { DynamicZoneArray* cat; const char* name; };
	const C cats[] = {{stars9, "stars_9"}, {stars10, "stars_10"}};

	auto* grid9 = new StelGeodesicGrid(9);
	auto* grid10 = new StelGeodesicGrid(10);

	for (const auto& c : cats) {
		if (!c.cat) continue;

		auto* grid = (c.cat->level == 9) ? grid9 : grid10;
		unsigned int nz = c.cat->getNrOfZones();
		unsigned int totalStars = c.cat->getNrOfStars();
		int totalNonEmpty = 0, totalTested = 0, phase1Hits = 0;
		QElapsedTimer timer;
		timer.start();

		for (unsigned int z = 0; z < nz; ++z) {
			uint32_t cnt = c.cat->zoneStarCount(z);
			if (cnt == 0) continue;
			totalNonEmpty++;

			// Sample: test every 10th zone. For "all data" mode, use 1.
			int step = 1;  // change to 10 for fast mode
			if (totalNonEmpty % step != 0) continue;

			const Star3* stars = c.cat->loadZone(static_cast<int>(z));
			if (!stars) continue;

			// Test first star (brightest) from each selected zone
			int si = 0;
			StarId gid = stars[si].getGaia();
			int hp = static_cast<int>(gid / 34359738368ULL);
			Vec3d v;
			healpix_pix2vec(static_cast<int>(pow(2.,12.)), hp, v.v);
			int hzone = grid->getZoneNumberForPoint(v.toVec3f(), c.cat->level);

			int matched = 0;
			c.cat->searchGaiaID(hzone, gid, matched);
			totalTested++;
			if (matched) {
				phase1Hits++;
			} else {
				// Phase 1 missed - verify star exists in its actual zone
				int verifyMatched = 0;
				c.cat->searchGaiaID(static_cast<int>(z), gid, verifyMatched);
				QVERIFY2(verifyMatched == 1,
					qPrintable(QString("%1: star Gaia %2 in zone %3 not found (BUG?)")
						.arg(c.name).arg(gid).arg(z)));
			}

			if (totalTested % 1000 == 0) {
				qint64 elapsed = timer.elapsed();
				double rate = totalTested * 1000.0 / elapsed;
				qInfo().noquote()
					<< QString("%1: tested %2/%3 zones, Phase1=%4/%5 (%6%), %7 z/s")
					   .arg(c.name).arg(totalTested).arg(totalNonEmpty)
					   .arg(phase1Hits).arg(totalTested)
					   .arg(100.0*phase1Hits/totalTested, 0, 'f', 1)
					   .arg(rate, 0, 'f', 0);
			}
		}

		qint64 elapsed = timer.elapsed();
		double pct = totalTested > 0 ? 100.0 * phase1Hits / totalTested : 0;
		qInfo().noquote()
			<< QString("%1 DONE: %2 stars, %3 zones, Phase1=%4/%5 (%6%), %7 sec")
			   .arg(c.name).arg(totalStars).arg(totalNonEmpty)
			   .arg(phase1Hits).arg(totalTested)
			   .arg(pct, 0, 'f', 1)
			   .arg(elapsed/1000.0, 0, 'f', 1);
	}

	delete grid9;
	delete grid10;
}

void TestDynamicLoading::testSearchGaiaIDLoadsZone()
{
	if (!stars9) QSKIP("stars_9 not loaded");
	int matched = 0;
	auto obj = stars9->searchGaiaID(4563306, 99999999999999999LL, matched);
	QVERIFY(matched == 0);
	QVERIFY(obj.isNull());
}

void TestDynamicLoading::testCacheReuse()
{
	if (!stars9) QSKIP("stars_9 not loaded");
	int zone = 4563306;
	StarId gid = 47325974053123840LL;
	QElapsedTimer timer;
	int matched = 0;

	timer.start();
	stars9->searchGaiaID(zone, gid, matched);
	qint64 first = timer.nsecsElapsed();
	QVERIFY(matched == 1);

	matched = 0;
	timer.restart();
	stars9->searchGaiaID(zone, gid, matched);
	qint64 second = timer.nsecsElapsed();
	QVERIFY(matched == 1);

	double ratio = static_cast<double>(first) / qMax(1LL, second);
	qInfo().noquote() << QString("Cache: cold=%1ns warm=%2ns (%3x)")
		.arg(first).arg(second).arg(ratio, 0, 'f', 1);
	QVERIFY2(ratio >= 0.7, "Warm cache should not be slower");
}
