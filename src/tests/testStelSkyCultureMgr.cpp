/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "tests/testStelSkyCultureMgr.hpp"

#include <QObject>
#include <QDebug>

#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"

QTEST_GUILESS_MAIN(TestStelSkyCultureMgr)

void TestStelSkyCultureMgr::testStelSkyCultureMgr()
{
	StelFileMgr::init();

	StelSkyCultureMgr scMgr;

	QVERIFY(scMgr.getDefaultSkyCultureID().isEmpty()); // Shoud be true, because sky culture is not set! (StelSkyCultureMgr::init() isn't called)
	QVERIFY(scMgr.setCurrentSkyCultureID("arabic"));
	QVERIFY(scMgr.getCurrentSkyCultureID()=="arabic");
	QVERIFY(scMgr.setCurrentSkyCultureID("western"));
	QVERIFY(scMgr.getCurrentSkyCultureID()=="western");
	QVERIFY(scMgr.getCurrentSkyCultureEnglishName()=="Western");
	QVERIFY(scMgr.getCurrentSkyCultureBoundariesIdx()==StelSkyCulture::IAU);
	QVERIFY(scMgr.getCurrentSkyCultureClassificationIdx()==StelSkyCulture::TRADITIONAL);
	QVERIFY(scMgr.getSkyCultureListEnglish().contains("western", Qt::CaseInsensitive));
}
