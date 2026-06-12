/*
 * Stellarium
 * Copyright (C) 2026 Stellarium Developers
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

#include "DynamicZoneArray.hpp"
#include "StelGeodesicGrid.hpp"
#include "StarMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"
#include "StelCore.hpp"
#include "StelApp.hpp"

#include <QDebug>
#include <QFile>

DynamicZoneArray::DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min)
	: ZoneArray(fname, file, level, mag_min)
	, starFile_(file)
{
	nr_of_zones = static_cast<unsigned int>(StelGeodesicGrid::nrOfZones(level));

	// Read zone table (star count per zone, 4 bytes each)
	const qint64 zoneTableSize = static_cast<qint64>(sizeof(uint32_t)) * nr_of_zones;
	starCounts_.resize(nr_of_zones);

	if (zoneTableSize != file->read(reinterpret_cast<char*>(starCounts_.data()), zoneTableSize))
	{
		qWarning() << "DynamicZoneArray: failed to read zone table from" << fname;
		nr_of_zones = 0;
		nr_of_stars = 0;
		return;
	}

	// Compute byte offsets for each zone's star data
	offsets_.resize(nr_of_zones);
	nr_of_stars = 0;
	uint64_t offset = static_cast<uint64_t>(file->pos()); // right after zone table

	for (unsigned int z = 0; z < nr_of_zones; ++z)
	{
		offsets_[z] = offset;
		uint32_t cnt = starCounts_[z];
		nr_of_stars += cnt;
		offset += static_cast<uint64_t>(cnt) * sizeof(Star3);
	}

	// Keep file open for on-demand reads, don't mmap star data
	// Set up cache: ~128 MB capacity, adjustable
	zoneCache_.setMaxCost(128 * 1024 * 1024);

	qInfo().noquote() << QString("DynamicZoneArray: %1 zones, %2 stars, %3 MB zone table, cache %4 MB")
		.arg(nr_of_zones).arg(nr_of_stars)
		.arg(zoneTableSize / (1024 * 1024))
		.arg(zoneCache_.maxCost() / (1024 * 1024));
}

DynamicZoneArray::~DynamicZoneArray()
{
	zoneCache_.clear();
	if (starFile_)
	{
		starFile_->close();
		delete starFile_;
		starFile_ = nullptr;
	}
}

const Star3* DynamicZoneArray::loadZone(int zoneIndex) const
{
	if (!starFile_ || !starFile_->isOpen())
		return nullptr;

	uint32_t count = starCounts_[zoneIndex];
	if (count == 0)
		return nullptr;

	// Check cache first
	QByteArray* cached = zoneCache_[zoneIndex];
	if (cached)
		return reinterpret_cast<const Star3*>(cached->constData());

	// Read from disk
	uint64_t off = offsets_[zoneIndex];
	qint64 size = static_cast<qint64>(count) * sizeof(Star3);

	auto* data = new QByteArray(size, Qt::Uninitialized);
	starFile_->seek(static_cast<qint64>(off));

	if (starFile_->read(data->data(), size) != size)
	{
		qWarning() << "DynamicZoneArray: read error for zone" << zoneIndex
			    << "at offset" << off;
		delete data;
		return nullptr;
	}

	zoneCache_.insert(zoneIndex, data, data->size());
	return reinterpret_cast<const Star3*>(data->constData());
}

void DynamicZoneArray::draw(StelPainter* sPainter, int index, bool isInsideViewport,
			     const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
			     int maxMagStarName, float names_brightness,
			     const QVector<SphericalCap>& boundingCaps,
			     const bool withAberration, const Vec3d vel,
			     const double withParallax, const Vec3d diffPos,
			     const bool withCommonNameI18n) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	const Star3* stars = loadZone(index);
	if (!stars)
		return;

	StelSkyDrawer* drawer = core->getSkyDrawer();
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;

	const Extinction& extinction = drawer->getExtinction();
	const bool withExtinction = drawer->getFlagHasAtmosphere() &&
				    extinction.getExtinctionCoefficient() >= 0.01f;

	int cutoffMagStep = limitMagIndex;
	float cutoffMag = 999999.f;
	if (drawer->getFlagStarMagnitudeLimit())
	{
		cutoffMag = drawer->getCustomStarMagnitudeLimit() * 1000.f;
		cutoffMagStep = static_cast<int>((cutoffMag - (mag_min - 7000.)) * 0.02);
		if (cutoffMagStep > limitMagIndex)
			cutoffMagStep = limitMagIndex;
	}

	Q_ASSERT_X(cutoffMagStep <= RCMAG_TABLE_SIZE, "DynamicZoneArray::draw",
		   QString("RCMAG_TABLE_SIZE: %1, cutoffMagStep: %2")
			   .arg(QString::number(RCMAG_TABLE_SIZE), QString::number(cutoffMagStep)).toLatin1());

	uint32_t zoneSize = starCounts_[index];
	bool globalzone = (static_cast<unsigned int>(index) == nr_of_zones - 1);
	Vec3d v;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		const Star3& s = stars[i];

		int mag = s.getMag(); // milli-mag
		int magIndex = static_cast<int>((mag - (mag_min - 7000.)) * 0.02);

		if ((magIndex > cutoffMagStep) || (static_cast<float>(mag) > cutoffMag))
		{
			if (std::fabs(dyrs) <= 5000.f || !globalzone)
				break;
		}

		// Star3 has no proper motion, parallax, or binary orbit
		double RA = s.getX0();
		double DEC = s.getX1();
		StelUtils::spheToRect(RA, DEC, v);

		if (withParallax)
		{
			// Star3 has no parallax, skip
		}

		if (withAberration)
		{
			v += vel;
			v.normalize();
		}

		if (!isInsideViewport)
		{
			bool isVisible = true;
			for (const auto& cap : boundingCaps)
			{
				if (!cap.contains(v))
				{
					isVisible = false;
					break;
				}
			}
			if (!isVisible)
				continue;
		}

		int extinctedMagIndex = magIndex;
		float twinkleFactor = 1.f;
		const RCMag* tmpRcmag = &rcmag_table[magIndex];

		if (withExtinction)
		{
			Vec3d altAz(v);
			altAz.normalize();
			core->j2000ToAltAzInPlaceNoRefraction(&altAz);
			float extMagShift = 0.f;
			extinction.forward(altAz, &extMagShift);
			extinctedMagIndex += static_cast<int>(extMagShift / 0.05f);
			if (extinctedMagIndex >= cutoffMagStep || extinctedMagIndex < 0)
				continue;
			tmpRcmag = &rcmag_table[extinctedMagIndex];
			twinkleFactor = qMin(1.f, 1.f - 0.9f * static_cast<float>(altAz[2]));
		}

		if (drawer->drawPointSource(sPainter, v, *tmpRcmag, s.getBVIndex(),
					    !isInsideViewport, twinkleFactor) &&
		    core->getFlagClearSky() && s.hasName() &&
		    extinctedMagIndex < maxMagStarName)
		{
			const float offset = tmpRcmag->radius * 0.7f;
			const Vec3f color = StelSkyDrawer::indexToColor(s.getBVIndex()) * 0.75f;
			sPainter->setColor(color, names_brightness);
			sPainter->drawText(v, s.getScreenNameI18n(withCommonNameI18n),
					   0, offset, offset, false);
		}
	}
}

void DynamicZoneArray::searchAround(const StelCore* core, int index, const Vec3d &v,
				     const double withParallax, const Vec3d diffPos,
				     double cosLimFov, QList<StelObjectP> &result)
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	const Star3* stars = loadZone(index);
	if (!stars)
		return;

	uint32_t zoneSize = starCounts_[index];
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	Vec3d tmp;

	(void)dyrs; // Star3 has no proper motion
	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		double RA = stars[i].getX0();
		double DEC = stars[i].getX1();
		StelUtils::spheToRect(RA, DEC, tmp);

		if (core->getUseAberration())
		{
			const Vec3d vel = core->getAberrationVec(core->getJDE());
			tmp += vel;
			tmp.normalize();
		}

		if (tmp * v >= cosLimFov)
			result.push_back(stars[i].createStelObject(nullptr, nullptr));
	}
}

void DynamicZoneArray::searchWithin(const StelCore* core, int index,
				     const SphericalRegionP region,
				     const double withParallax, const Vec3d diffPos,
				     const bool /*hipOnly*/, const float maxMag,
				     QList<StelObjectP> &result) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	const Star3* stars = loadZone(index);
	if (!stars)
		return;

	uint32_t zoneSize = starCounts_[index];
	const float maxMilliMag = 1000.f * maxMag;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getMag() >= maxMilliMag)
			continue; // stars are sorted by mag, but we still check (could be border)

		double RA = stars[i].getX0();
		double DEC = stars[i].getX1();
		StelUtils::spheToRect(RA, DEC, tmp);

		if (core->getUseAberration())
		{
			const Vec3d vel = core->getAberrationVec(core->getJDE());
			tmp += vel;
			tmp.normalize();
		}

		if (region->contains(tmp))
			result.push_back(stars[i].createStelObject(nullptr, nullptr));
	}
}

StelObjectP DynamicZoneArray::searchGaiaID(int index, const StarId source_id,
					    int& matched) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return StelObjectP();

	const Star3* stars = loadZone(index);
	if (!stars)
		return StelObjectP();

	uint32_t zoneSize = starCounts_[index];
	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getGaia() == source_id)
		{
			++matched;
			return stars[i].createStelObject(nullptr, nullptr);
		}
	}
	return StelObjectP();
}

void DynamicZoneArray::searchGaiaIDepochPos(const StarId source_id, float dyrs,
					     double & RA, double & DEC, double & Plx,
					     double & pmra, double & pmdec, double & RV) const
{
	// Iterate all zones (expensive but only used by unit tests)
	(void)dyrs; // Star3 has no proper motion
	for (unsigned int z = 0; z < nr_of_zones; ++z)
	{
		const Star3* stars = loadZone(z);
		if (!stars)
			continue;

		uint32_t zoneSize = starCounts_[z];
		for (uint32_t i = 0; i < zoneSize; ++i)
		{
			if (stars[i].getGaia() == source_id)
			{
				RA  = stars[i].getX0();
				DEC = stars[i].getX1();
				Plx = 0.0;
				pmra  = 0.0;
				pmdec = 0.0;
				RV    = 0.0;
				return;
			}
		}
	}
}
