/*
 * Stellarium
 * Copyright (C) 2020 Wolfgang Laun
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

#ifndef TESTJAVASCRIPTING_HPP
#define TESTJAVASCRIPTING_HPP

#include <QObject>
#include <QtTest>
#include <QString>
#ifdef ENABLE_SCRIPT_QML
#include <QJSEngine>
#else
#include <QScriptEngine>
#endif

class TestJavaScripting : public QObject
{
Q_OBJECT
private slots:
	void initTestCase();
	void testVec3fConstructor();
	void testVec3fConstructorFail();
	void testVec3dConstructor();
#ifdef ENABLE_SCRIPT_QML
	QString runScript(QJSEngine *engine, QString script);
private:
	QJSEngine *engine;
#else
	QString runScript(QScriptEngine *engine, QString script);
private:
	QScriptEngine *engine;
#endif
};
#endif  // TESTJAVASCRIPTING_HPP
