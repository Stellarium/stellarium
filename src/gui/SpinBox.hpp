/***************************************************************************
 *   Copyright (C) 2008 by guillaume,,,   *
 *   guillaume@hellokitty   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef _SPINBOX_HPP_
#define _SPINBOX_HPP_

#include <QSpinBox>

/**
	@author guillaume,,, <guillaume@hellokitty>
*/
class SpinBox : public QSpinBox
{
Q_OBJECT

protected:
	QVector<int> mins;
	QVector<int> ranges;
	QVector<QString> delimiters;

public:
    SpinBox(QWidget *parent);

    ~SpinBox();
    
protected:
	virtual int valueFromText(const QString &text) const;
	virtual QString textFromValue(int value) const;
	virtual QValidator::State validate(QString & input, int & pos) const;
	virtual void stepBy(int steps);
	QVector<int> valuesFromValue(int value) const;
	int valueFromValues(const QVector<int>& values) const;
	QRegExp regExp() const;
	QRegExp regExpIntermediate() const;
};

class LongitudeSpinBox : public SpinBox
{
Q_OBJECT
public:
	LongitudeSpinBox(QWidget *parent = 0);
	void setValue(double value);
	float getValue();
};

class LatitudeSpinBox : public SpinBox
{
Q_OBJECT
public:
	LatitudeSpinBox(QWidget *parent = 0);
	void setValue(double value);
	float getValue();
};


#endif // _SPINBOX_HPP_
