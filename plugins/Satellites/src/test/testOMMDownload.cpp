/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

#include <QList>
#include <QVariant>
#include <QSignalSpy>

#include "testOMMDownload.hpp"

QTEST_GUILESS_MAIN(TestOMMDownload)

static void delay(int d)
{
	QTime dieTime = QTime::currentTime().addMSecs(d);
	while (QTime::currentTime() < dieTime) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}
}

void TestOMMDownload::testUT()
{
	OMMDownload dut;
	QVERIFY(dut.getSim() == OMMDownload::sim_t::SIM_REMOTE);
	dut.setSim(OMMDownload::sim_t::SIM_LOCAL_TLE);
	QVERIFY(dut.getSim() != OMMDownload::sim_t::SIM_REMOTE);
	QVERIFY(dut.getSim() == OMMDownload::sim_t::SIM_LOCAL_TLE);
}

static QPair<QString, QString> fakes[2] =
{
	{ "key1", "val1" },
	{ "key2", "val2" }
};

void TestOMMDownload::testSim()
{
	int sig_count = 0;
	OMMDownload dut;
	dut.setSim(OMMDownload::sim_t::SIM_LOCAL_INLINE);
	for(int i = 0; i < 2; i++) {
		dut.addURI(fakes[i]);
	}
	QSignalSpy spy(&dut, &OMMDownload::fileDownloadComplete);
	dut.execute();
	while(spy.wait(1) && sig_count < 2) {
		QVERIFY(spy.isEmpty() == false);
		QList<QVariant> rxed = spy.takeFirst();
		QCOMPARE(rxed.at(0).toString(), fakes[sig_count].first);
		sig_count++;
	}
}
