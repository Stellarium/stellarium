/*
 * Copyright (C) 2013 Felix Zeltner
 * Copyright (C) 2026 Kamil Zaraś (astronow.pl)
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

#ifndef PLANES_AIRCRAFTOBJECT_HPP
#define PLANES_AIRCRAFTOBJECT_HPP

#include "AircraftRecord.hpp"
#include "StelObject.hpp"
#include "StelProjectorType.hpp"
#include "StelTranslator.hpp"

#include <QSharedPointer>
#include <QString>

class StelPainter;

class AircraftObject : public StelObject
{
public:
	static const QString STEL_TYPE;

	explicit AircraftObject(const AircraftRecord& record);
	void updateRecord(const AircraftRecord& record);

	QString getType() const override { return STEL_TYPE; }
	QString getObjectType() const override { return N_("plane"); }
	QString getObjectTypeI18n() const override;
	QString getID() const override;
	QString getEnglishName() const override;
	QString getNameI18n() const override;
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	Vec3f getInfoColor() const override;
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	float getVMagnitude(const StelCore* core) const override;
	double getAngularRadius(const StelCore* core) const override;
	float getSelectPriority(const StelCore* core) const override;

	bool isAboveHorizon(const StelCore* core) const;
	void draw(StelCore* core, StelPainter* painter, bool drawLabels, int labelMode) const;

	const AircraftRecord& record() const { return aircraftRecord; }

private:
	AircraftRecord getExtrapolatedRecord(double elapsedSeconds) const;
	double getElapsedSeconds() const;
	Vec3d getAltAzPos(const StelCore* core, double elapsedSeconds=0.0) const;
	float getScreenRotationDegrees(StelCore* core, const StelProjectorP& projector, const Vec3d& currentScreenPos) const;
	QString labelText() const;
	QString displayLabelText(int labelMode) const;

	AircraftRecord aircraftRecord;
};

using AircraftObjectP = QSharedPointer<AircraftObject>;

#endif
