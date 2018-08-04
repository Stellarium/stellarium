/*
 * Stellarium
 * Copyright (C) 2013 Fabien Chereau
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

#ifndef STELPROGRESSCONTROLLER_HPP
#define STELPROGRESSCONTROLLER_HPP

#include <QString>
#include <QObject>

//! Maintain the state of a progress bar
class StelProgressController : public QObject
{
	Q_OBJECT
	
public:
	StelProgressController(QObject* parent = Q_NULLPTR) : QObject(parent), format("%p%"), min(0), max(100), value(0) {;}
	
	//! This property holds the string used to generate the current text.
	//! %p - is replaced by the percentage completed.
	//! %v - is replaced by the current value.
	//! %m - is replaced by the total number of steps.
	//! The default value is "%p%".
	void setFormat(const QString& fmt) {format = fmt;}
	void setRange(int amin, int amax) {min = amin; max = amax;}
	void setValue(int val) {value=val; emit changed();}

	int getValue() const {return value;}
	int getMin() const {return min;}
	int getMax() const {return max;}
	QString getFormat() const {return format;}
	
signals:
	void changed();
	
private:
	friend class StelApp;
	~StelProgressController() {;}
	QString format;
	int min;
	int max;
	int value;
};


#endif // STELPROGRESSCONTROLLER_HPP
