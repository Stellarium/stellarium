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

#ifndef OCULAR_HPP_
#define OCULAR_HPP_

#include <QDebug>
#include <QObject>
#include <QString>
#include <QSettings>

class Telescope;

class Ocular : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString name READ name WRITE setName)
	Q_PROPERTY(double appearentFOV READ appearentFOV WRITE setAppearentFOV)
	Q_PROPERTY(double effectiveFocalLength READ effectiveFocalLength WRITE setEffectiveFocalLength)
	Q_PROPERTY(double fieldStop READ fieldStop WRITE setFieldStop)
public:
	Ocular();
	Q_INVOKABLE Ocular(const Ocular* other);
	virtual ~Ocular();
	static Ocular* ocularFromSettings(QSettings* theSettings, QString theGroupName);
	static Ocular* ocularModel();

	const QString name() const;
	void setName(QString aName);
	double appearentFOV() const;
	void setAppearentFOV(double fov);
	double effectiveFocalLength() const;
	void setEffectiveFocalLength(double fl);
	double fieldStop() const;
	void setFieldStop(double fs);

	double actualFOV(Telescope *telescope) const;
	double magnification(Telescope *telescope) const;
	QMap<int, QString> propertyMap();

private:
	QString m_name;
	double m_appearentFOV;
	double m_effectiveFocalLength;
	double m_fieldStop;
};

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Static Methods
#endif
/* ********************************************************************* */

static Ocular* ocularFromSettings(QSettings* theSettings, QString theGroupName)
{
	Ocular* ocular = new Ocular();
	theSettings->beginGroup(theGroupName);

	ocular->setName(theSettings->value("name", "").toString());
	ocular->setAppearentFOV(theSettings->value("afov", "0.0").toDouble());
	ocular->setEffectiveFocalLength(theSettings->value("efl", "0.0").toDouble());
	ocular->setFieldStop(theSettings->value("fieldStop", "0.0").toDouble());

	theSettings->endGroup();
	if (!(ocular->appearentFOV() > 0.0 && ocular->effectiveFocalLength() > 0.0)) {
		qWarning() << "WARNING: Invalid data for ocular. Ocular values must be positive. \n"
			<< "\tafov: " << ocular->appearentFOV() << "\n"
			<< "\tefl: " << ocular->effectiveFocalLength() << "\n"
			<< "\tThis ocular will be ignored.";
		delete ocular;
		ocular = NULL;
	}

	return ocular;
}

static Ocular* ocularModel()
{
	Ocular* model = new Ocular();
	model->setName("My Ocular");
	model->setAppearentFOV(68);
	model->setEffectiveFocalLength(32);
	model->setFieldStop(0);
	return model;
}

#endif /* OCULAR_HPP_ */
