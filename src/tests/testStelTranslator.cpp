/*
 * Stellarium
 * Copyright (C) 2024 Alexander Wolf
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

#include "tests/testStelTranslator.hpp"

#include <QObject>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"

QTEST_GUILESS_MAIN(TestStelTranslator)

void TestStelTranslator::initTestCase()
{
	StelTranslator* testTranslator = new StelTranslator("stellarium", "en");
	QVERIFY(testTranslator->getTrueLocaleName()=="en");
	QVERIFY(testTranslator->qtranslate("Month")=="Month");
	QVERIFY(testTranslator->qtranslate("km/s", "speed")=="km/s");

	StelTranslator* testTranslatorRu = new StelTranslator("stellarium", "ru");
	QVERIFY(testTranslatorRu->getTrueLocaleName()=="ru");
	QVERIFY(testTranslatorRu->qtranslate("Month")=="Месяц");
	QVERIFY(testTranslatorRu->qtranslate("km/s", "speed")=="км/с");

	StelTranslator* testTranslatorPtBr = new StelTranslator("stellarium", "pt_BR");
	QVERIFY(testTranslatorPtBr->getTrueLocaleName()=="pt_BR");
	QVERIFY(testTranslatorPtBr->qtranslate("Month")=="Mês");
	QVERIFY(testTranslatorPtBr->qtranslate("km/s", "speed")=="km/s");
}

