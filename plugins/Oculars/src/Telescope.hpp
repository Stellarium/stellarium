/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef TELESCOPE_HPP_
#define TELESCOPE_HPP_

#include <QObject>
#include <QString>
#include <QSettings>

class Telescope : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(double diameter READ diameter WRITE setDiameter)
	Q_PROPERTY(double focalLength READ focalLength WRITE setFocalLength)
	Q_PROPERTY(bool hFlipped READ isHFlipped WRITE setHFlipped)
	Q_PROPERTY(bool vFlipped READ isVFlipped WRITE setVFlipped)
public:
	Telescope();
	Q_INVOKABLE Telescope(const QObject* other);
	virtual ~Telescope();
	static Telescope* telescopeFromSettings(QSettings* theSettings, QString theGroupName);
	static Telescope* telescopeModel();

	double diameter() const;
	void setDiameter(double theValue);
	double focalLength() const;
	void setFocalLength(double theValue);
	const QString name() const;
	void setName(QString theValue);
	bool isHFlipped() const;
	void setHFlipped(bool flipped);
	bool isVFlipped() const;
	void setVFlipped(bool flipped);
	QMap<int, QString> propertyMap();
private:
	QString m_name;
	double m_diameter;
	double m_focalLength;
	bool m_hFlipped;
	bool m_vFlipped;
};

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

static Telescope* telescopeFromSettings(QSettings* theSettings, QString theGroupName)
{
	Telescope* telescope = new Telescope();
	theSettings->beginGroup(theGroupName);

	telescope->setName(theSettings->value("name", "").toString());
	telescope->setFocalLength(theSettings->value("focalLength", "0").toDouble());
	telescope->setDiameter(theSettings->value("diameter", "0").toDouble());
	telescope->setHFlipped(theSettings->value("hFlip").toBool());
	telescope->setVFlipped(theSettings->value("vFlip").toBool());

	theSettings->endGroup();
	return telescope;
}
static Telescope* telescopeModel()
{
	Telescope* model = new Telescope();
	model->setName("My Telescope");
	model->setDiameter(80);
	model->setFocalLength(500);
	model->setHFlipped(true);
	model->setVFlipped(true);
	return model;
}

#endif /*TELESCOPE_HPP_*/
