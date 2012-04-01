/*
 * Copyright (C) 2009 Matthew Gates
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

#include "TuiNodeDateTime.hpp"
#include "StelUtils.hpp"
#include "StelCore.hpp"
#include "StelTranslator.hpp"
#include <QKeyEvent>
#include <QDebug>
#include <QStringList>

TuiNodeDateTime::TuiNodeDateTime(const QString& text, QObject* receiver, const char* method, double defValue, TuiNode* parent, TuiNode* prev)
	: TuiNodeEditable(text, parent, prev), value(defValue), editingPart(0), typing(false)
{
	this->connect(this, SIGNAL(setValue(double)), receiver, method);
}

TuiNodeResponse TuiNodeDateTime::handleEditingKey(int key)
{
	TuiNodeResponse response;
	response.accepted = false;
	response.newNode = this;
	if (key==Qt::Key_Left)
	{
		typing = false;
		if (editingPart==0)
		{
			editing = false;
		}
		else
		{
			editingPart--;
		}
		response.accepted = true;
		if (!editing)
		{
			emit(setValue(value));
		}
		return response;
	}
	if (key==Qt::Key_Return)
	{
		typing = false;
		editing = false;
		editingPart = 0;
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	if (key==Qt::Key_Right)
	{
		typing = false;
		if (editingPart<5)
		{
			editingPart++;
		}
		response.accepted = true;
		return response;
	}
	if (key==Qt::Key_Up)
	{
		typing = false;
		incPart(editingPart, true);
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	if (key==Qt::Key_Down)
	{
		typing = false;
		incPart(editingPart, false);
		response.accepted = true;
		emit(setValue(value));
		return response;
	}
	if (key>=Qt::Key_0 && key<=Qt::Key_9 && editingPart==0)
	{
		QString s;
		if (!typing)
		{
			s = QString("%1").arg(key - Qt::Key_0);
			typing = true;
		}
		else
		{
			s = QString("%1%2").arg(getParts(value).at(editingPart)).arg(key - Qt::Key_0);
		}
		response.accepted = true;
		response.newNode = this;
		if (setPart(editingPart, s.toInt()))
		{
			emit(setValue(value));
		}
		return response;
	}
	return response;
}

QString TuiNodeDateTime::getDisplayText() 
{
	QList<int> parts = getParts(value);
	QString yy = QString("%1").arg(parts.at(0));
	QString mm = QString("%1").arg(parts.at(1), 2, 10, QChar('0'));
	QString dd = QString("%1").arg(parts.at(2), 2, 10, QChar('0'));
	QString h = QString("%1").arg(parts.at(3), 2, 10, QChar('0'));
	QString m = QString("%1").arg(parts.at(4), 2, 10, QChar('0'));
	QString s = QString("%1").arg(parts.at(5), 2, 10, QChar('0'));
	
	//(Default format string when not editing anything)
	const char* formatString = ":  %1-%2-%3 %4:%5:%6 UTC";
	if (editing)
	{
		switch (editingPart)
		{
		case 0:
			formatString = ":  >%1<-%2-%3 %4:%5:%6 UTC"; //Year
			break;
		case 1:
			formatString = ":  %1->%2<-%3 %4:%5:%6 UTC"; //Month
			break;
		case 2:
			formatString = ":  %1-%2->%3< %4:%5:%6 UTC"; //Day
			break;
		case 3:
			formatString = ":  %1-%2-%3 >%4<:%5:%6 UTC"; //Hour
			break;
		case 4:
			formatString = ":  %1-%2-%3 %4:>%5<:%6 UTC"; //Minute
			break;
		case 5:
			formatString = ":  %1-%2-%3 %4:%5:>%6< UTC"; //Second
		default:
			break;
		}
	}
	
	QString result = prefixText + q_(displayText) + QString (formatString).arg(yy, mm, dd, h, m, s);
	return result;
}

void TuiNodeDateTime::incPart(int part, bool add)
{
	Q_UNUSED(part);
	int diff = 1;
	if (!add)
		diff *= -1;

	QList<int> parts = getParts(value);

	if (editingPart==0)
	{
		double newDate;
		if (StelUtils::getJDFromDate(&newDate, parts.at(editingPart) + diff, parts.at(1), parts.at(2), parts.at(3), parts.at(4), parts.at(5)))
			value = newDate;
	}
	else if (editingPart==1)
	{
		double newDate;
		int m = ((parts.at(editingPart)-1+diff) % 12) + 1;
		if (m==0) m=12;
		qDebug() << "new m is" << m;
		if (StelUtils::getJDFromDate(&newDate, parts.at(0), m, parts.at(2), parts.at(3), parts.at(4), parts.at(5)))
			value = newDate;
	}
	else if (editingPart==2)
	{
		value += (diff * StelCore::JD_DAY);
	}
	else if (editingPart==3)
	{
		value += (diff * StelCore::JD_HOUR);
	}
	else if (editingPart==4)
	{
		value += (diff * StelCore::JD_MINUTE);
	}
	else if (editingPart==5)
	{
		value += (diff * StelCore::JD_SECOND);
	}
}

QList<int> TuiNodeDateTime::getParts(double jd)
{
	int year, month, day, hour, minute, second;
	StelUtils::getDateFromJulianDay(jd, &year, &month, &day);
	StelUtils::getTimeFromJulianDay(jd, &hour, &minute, &second);
	QList<int> parts;
	parts << year << month << day << hour << minute << second;
	return parts;
}

bool TuiNodeDateTime::setPart(int part, int val)
{
	QList<int> parts = getParts(value);
	parts.replace(part, val);
	double newDate;
	if (StelUtils::getJDFromDate(&newDate, parts.at(0), parts.at(1), parts.at(2), parts.at(3), parts.at(4), parts.at(5)))
	{
		value = newDate;
		return true;
	}
	else
	{
		return false;
	}
}

