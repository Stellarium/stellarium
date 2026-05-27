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

#include "AircraftObject.hpp"

#include "StelCore.hpp"
#include "StelLocation.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#include <QTextStream>
#include <cmath>

namespace
{
constexpr double kEarthFlattening = 1.0 / 298.257223563;
constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kSecondsPerDay = 86400.0;
constexpr double kMaxDeadReckoningSeconds = 30.0;
constexpr double kHeadingProbeSeconds = 1.0;
constexpr double kMetersToFeet = 3.280839895;
constexpr double kMetersPerSecondToKnots = 1.943844492;
constexpr double kMetersPerSecondToFeetPerMinute = 196.8503937;
constexpr float kPlaneSpriteSize = 16.0f;
constexpr float kSpriteHeadingOffsetDegrees = -180.0f;

Vec3d toEcef(double latitudeRad, double longitudeRad, double altitudeMeters)
{
	const double sinLat = std::sin(latitudeRad);
	const double c = 1.0 / std::sqrt(1.0 + kEarthFlattening * (kEarthFlattening - 2.0) * (sinLat * sinLat));
	const double sq = (1.0 - kEarthFlattening) * (1.0 - kEarthFlattening) * c;
	const double radius = (kEarthRadiusMeters * c + altitudeMeters) * std::cos(latitudeRad);

	return Vec3d(radius * std::cos(longitudeRad),
		     radius * std::sin(longitudeRad),
		     (kEarthRadiusMeters * sq + altitudeMeters) * sinLat);
}

double normalizeLongitudeRadians(double longitudeRad)
{
	while (longitudeRad > M_PI)
		longitudeRad -= 2.0 * M_PI;
	while (longitudeRad < -M_PI)
		longitudeRad += 2.0 * M_PI;
	return longitudeRad;
}

double normalizeDegrees(double degrees)
{
	while (degrees >= 360.0)
		degrees -= 360.0;
	while (degrees < 0.0)
		degrees += 360.0;
	return degrees;
}

QString headingToCompass(double degrees)
{
	static const char* labels[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
	const int index = static_cast<int>(std::floor((normalizeDegrees(degrees) + 22.5) / 45.0)) % 8;
	return QString::fromLatin1(labels[index]);
}
}

const QString AircraftObject::STEL_TYPE = QStringLiteral("Flight");

AircraftObject::AircraftObject(const AircraftRecord& record)
	: aircraftRecord(record)
{
}

void AircraftObject::updateRecord(const AircraftRecord& record)
{
	aircraftRecord = record;
}

QString AircraftObject::getObjectTypeI18n() const
{
	return q_(getObjectType());
}

QString AircraftObject::getID() const
{
	return aircraftRecord.icao24;
}

QString AircraftObject::getEnglishName() const
{
	if (!aircraftRecord.callsign.isEmpty())
		return aircraftRecord.callsign;
	return aircraftRecord.icao24;
}

QString AircraftObject::getNameI18n() const
{
	return getEnglishName();
}

QString AircraftObject::labelText() const
{
	if (!aircraftRecord.callsign.isEmpty())
		return aircraftRecord.callsign;
	if (!aircraftRecord.aircraftType.isEmpty())
		return QString("%1 (%2)").arg(aircraftRecord.icao24, aircraftRecord.aircraftType);
	return aircraftRecord.icao24;
}

QString AircraftObject::displayLabelText(int labelMode) const
{
	if (labelMode == 1 && !aircraftRecord.aircraftType.isEmpty())
		return aircraftRecord.aircraftType;
	return labelText();
}

double AircraftObject::getElapsedSeconds() const
{
	if (aircraftRecord.snapshotJd <= 0.0)
		return 0.0;

	const double elapsedSeconds = (StelUtils::getJDFromSystem() - aircraftRecord.snapshotJd) * kSecondsPerDay;
	if (elapsedSeconds <= 0.0)
		return 0.0;
	return qMin(elapsedSeconds, kMaxDeadReckoningSeconds);
}

AircraftRecord AircraftObject::getExtrapolatedRecord(double elapsedSeconds) const
{
	AircraftRecord record = aircraftRecord;
	if (elapsedSeconds <= 0.0 || record.groundSpeedMs <= 0.0)
	{
		record.altitudeMeters = qMax(0.0, record.altitudeMeters + record.verticalRateMs * elapsedSeconds);
		return record;
	}

	const double lat1 = record.latitude * M_PI / 180.0;
	const double lon1 = record.longitude * M_PI / 180.0;
	const double bearing = normalizeDegrees(record.trackDegrees) * M_PI / 180.0;
	const double angularDistance = (record.groundSpeedMs * elapsedSeconds) / kEarthRadiusMeters;

	const double sinLat1 = std::sin(lat1);
	const double cosLat1 = std::cos(lat1);
	const double sinAngularDistance = std::sin(angularDistance);
	const double cosAngularDistance = std::cos(angularDistance);

	const double lat2 = std::asin(sinLat1 * cosAngularDistance +
		cosLat1 * sinAngularDistance * std::cos(bearing));
	const double lon2 = lon1 + std::atan2(std::sin(bearing) * sinAngularDistance * cosLat1,
		cosAngularDistance - sinLat1 * std::sin(lat2));

	record.latitude = lat2 * 180.0 / M_PI;
	record.longitude = normalizeLongitudeRadians(lon2) * 180.0 / M_PI;
	record.altitudeMeters = qMax(0.0, record.altitudeMeters + record.verticalRateMs * elapsedSeconds);
	return record;
}

QString AircraftObject::getInfoString(const StelCore* core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream stream(&str);
	const AircraftRecord currentRecord = getExtrapolatedRecord(getElapsedSeconds());

	if (flags & Name)
		stream << QString("<h2>%1</h2>").arg(labelText().toHtmlEscaped());

	if (flags & CatalogNumber)
	{
		stream << QString("%1: %2").arg(q_("ICAO Identifier"), aircraftRecord.icao24.toHtmlEscaped());
		if (!aircraftRecord.callsign.isEmpty())
			stream << QString("; %1: %2").arg(q_("Flight"), aircraftRecord.callsign.toHtmlEscaped());
		if (!aircraftRecord.aircraftType.isEmpty())
			stream << QString("; %1: %2").arg(q_("Model"), aircraftRecord.aircraftType.toHtmlEscaped());
		stream << "<br/><br/>";
	}

	if (flags & ObjectType)
		stream << QString("%1: <b>%2</b><br />").arg(q_("Type"), getObjectTypeI18n());

	stream << getCommonInfoString(core, flags);

	if (flags&Extra)
	{
		const bool withTables = StelApp::getInstance().getFlagUseFormattingOutput();
		// TRANSLATORS: Unit of measure for distance - meters
		QString m = qc_("m", "distance");
		// TRANSLATORS: Unit of measure for distance - feets
		QString ft = qc_("ft", "distance");
		// TRANSLATORS: Unit of measure for speed - meters per second
		QString mps = qc_("m/s", "speed");
		// TRANSLATORS: Unit of measure for speed - knots
		QString kt = qc_("kt", "speed");
		// TRANSLATORS: Unit of measure for speed - feets per minute
		QString ftpm = qc_("ft/min", "speed");
		// TRANSLATORS: Unit of measure for time - seconds
		QString s = qc_("s", "time");

		const QString altitude = QString("%1 %2 (%3 %4)").arg(QString::number(currentRecord.altitudeMeters, 'f', 0), m,
		                                                      QString::number(currentRecord.altitudeMeters * kMetersToFeet, 'f', 0), ft);

		const QString groundSpeed = QString("%1 %2 (%3 %4)").arg(QString::number(currentRecord.groundSpeedMs, 'f', 0), mps,
		                                                         QString::number(currentRecord.groundSpeedMs * kMetersPerSecondToKnots, 'f', 0), kt);

		const QString verticalRate = QString("%1 %2 (%3 %4)").arg(QString::number(currentRecord.verticalRateMs, 'f', 1), mps,
		                                                          QString::number(currentRecord.verticalRateMs * kMetersPerSecondToFeetPerMinute, 'f', 0), ftpm);

		const QString heading = QString("%1° (%2)").arg(QString::number(normalizeDegrees(currentRecord.trackDegrees), 'f', 0), qc_(headingToCompass(currentRecord.trackDegrees), "compass direction"));

		const double dataAgeSeconds = getElapsedSeconds();
		const QString dataAge = QString("%1 %2").arg(QString::number(dataAgeSeconds, 'f', dataAgeSeconds < 10.0 ? 1 : 0), s);

		if (withTables)
		{
			stream << "<table style='margin:0em 0em 0em -0.125em;border-spacing:0px;border:0px;'>";
			stream << QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(q_("Altitude"), altitude);
			stream << QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(q_("Ground speed"), groundSpeed);
			stream << QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(q_("Vertical rate"), verticalRate);
			stream << QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(q_("Track"), heading);
			stream << QString("<tr><td>%1:</td><td style='text-align:right;'>%2</td></tr>").arg(q_("Data age"), dataAge);
			stream << "</table><br/>";
		}
		else
		{
			stream << QString("%1: %2<br/>").arg(q_("Altitude"), altitude);
			stream << QString("%1: %2<br/>").arg(q_("Ground speed"), groundSpeed);
			stream << QString("%1: %2<br/>").arg(q_("Vertical rate"), verticalRate);
			stream << QString("%1: %2<br/>").arg(q_("Track"), heading);
			stream << QString("%1: %2<br/>").arg(q_("Data age"), dataAge);
		}
	}

	postProcessInfoString(str, flags);
	return str;
}

Vec3f AircraftObject::getInfoColor() const
{
	return Vec3f(0.30f, 0.86f, 1.0f);
}

Vec3d AircraftObject::getAltAzPos(const StelCore* core, double elapsedSeconds) const
{
	if (!core)
		return Vec3d(0.0, 0.0, 1.0);

	const AircraftRecord record = getExtrapolatedRecord(elapsedSeconds);
	const StelLocation& location = core->getCurrentLocation();
	const double obsLat = static_cast<double>(location.getLatitude()) * M_PI / 180.0;
	const double obsLon = static_cast<double>(location.getLongitude()) * M_PI / 180.0;
	const double tgtLat = record.latitude * M_PI / 180.0;
	const double tgtLon = record.longitude * M_PI / 180.0;

	const Vec3d observer = toEcef(obsLat, obsLon, static_cast<double>(location.altitude));
	const Vec3d aircraft = toEcef(tgtLat, tgtLon, record.altitudeMeters);
	const Vec3d toPoint = aircraft - observer;

	const double sla = std::sin(obsLat);
	const double cla = std::cos(obsLat);
	const double slo = std::sin(obsLon);
	const double clo = std::cos(obsLon);

	Vec3d altAz;
	altAz[0] = sla * clo * toPoint[0] + sla * slo * toPoint[1] - cla * toPoint[2];
	altAz[1] = -slo * toPoint[0] + clo * toPoint[1];
	altAz[2] = cla * clo * toPoint[0] + cla * slo * toPoint[1] + sla * toPoint[2];
	altAz.normalize();
	return altAz;
}

Vec3d AircraftObject::getJ2000EquatorialPos(const StelCore* core) const
{
	return core ? core->altAzToJ2000(getAltAzPos(core, getElapsedSeconds()), StelCore::RefractionOff) : Vec3d(1.0, 0.0, 0.0);
}

float AircraftObject::getVMagnitude(const StelCore* core) const
{
	Q_UNUSED(core)
	return 1.5f;
}

double AircraftObject::getAngularRadius(const StelCore* core) const
{
	Q_UNUSED(core)
	return 0.0002;
}

float AircraftObject::getSelectPriority(const StelCore* core) const
{
	Q_UNUSED(core)
	return -10.0f;
}

bool AircraftObject::isAboveHorizon(const StelCore* core) const
{
	return getAltAzPos(core, getElapsedSeconds())[2] > 0.0;
}

float AircraftObject::getScreenRotationDegrees(StelCore* core, const StelProjectorP& projector, const Vec3d& currentScreenPos) const
{
	if (!projector)
		return static_cast<float>(normalizeDegrees(aircraftRecord.trackDegrees) + kSpriteHeadingOffsetDegrees);

	Vec3d futureScreenPos;
	if (!projector->project(getAltAzPos(core, getElapsedSeconds() + kHeadingProbeSeconds), futureScreenPos))
		return static_cast<float>(normalizeDegrees(aircraftRecord.trackDegrees) + kSpriteHeadingOffsetDegrees);

	const double dx = futureScreenPos[0] - currentScreenPos[0];
	const double dy = futureScreenPos[1] - currentScreenPos[1];
	if (std::abs(dx) < 0.01 && std::abs(dy) < 0.01)
		return static_cast<float>(normalizeDegrees(aircraftRecord.trackDegrees) + kSpriteHeadingOffsetDegrees);

	const double angleDeg = std::atan2(dy, dx) * 180.0 / M_PI;
	return static_cast<float>(angleDeg + kSpriteHeadingOffsetDegrees);
}

void AircraftObject::draw(StelCore* core, StelPainter* painter, bool drawLabels, int labelMode) const
{
	if (!core || !painter || !isAboveHorizon(core))
		return;

	StelProjectorP projector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	const double elapsedSeconds = getElapsedSeconds();
	Vec3d screenPos;
	if (!projector->project(getAltAzPos(core, elapsedSeconds), screenPos) || !projector->checkInViewport(screenPos))
		return;

	const Vec3f color = getInfoColor();
	painter->setColor(color, 1.0f);
	painter->drawSprite2dMode(static_cast<float>(screenPos[0]), static_cast<float>(screenPos[1]),
		kPlaneSpriteSize, getScreenRotationDegrees(core, projector, screenPos));

	if (drawLabels)
	{
		painter->drawText(static_cast<float>(screenPos[0]), static_cast<float>(screenPos[1]), displayLabelText(labelMode), 0, 10.0f, 10.0f, false);
	}
}
