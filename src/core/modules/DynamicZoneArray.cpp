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

template<class StarType>
DynamicZoneArray<StarType>::DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min)
	: ZoneArray(fname, file, level, mag_min)
	, zoneCounts_(nullptr)
	, zoneTableMmapStart_(nullptr)
{
	nr_of_zones = static_cast<unsigned int>(StelGeodesicGrid::nrOfZones(level));

	const qint64 zoneTableSize = static_cast<qint64>(sizeof(uint32_t)) * nr_of_zones;
	zoneTableMmapStart_ = file->map(28, zoneTableSize);
	if (!zoneTableMmapStart_)
	{
		qWarning() << "DynamicZoneArray: failed to mmap zone table from" << fname;
		nr_of_zones = 0;
		nr_of_stars = 0;
		return;
	}
	zoneCounts_ = reinterpret_cast<const uint32_t*>(zoneTableMmapStart_);

	const unsigned int numBlocks = (nr_of_zones + BLOCK_SIZE - 1) / BLOCK_SIZE + 1;
	blockOffsets_.resize(numBlocks);

	uint64_t runningOffset = 0;
	blockOffsets_[0] = 0;
	for (unsigned int b = 0; b < numBlocks - 1; ++b)
	{
		const unsigned int zEnd = std::min((b + 1) * BLOCK_SIZE, nr_of_zones);
		for (unsigned int z = b * BLOCK_SIZE; z < zEnd; ++z)
		{
			nr_of_stars += zoneCounts_[z];
			runningOffset += static_cast<uint64_t>(zoneCounts_[z]) * sizeof(StarType);
		}
		blockOffsets_[b + 1] = runningOffset;
	}

	const qint64 starDataBase = 28 + zoneTableSize;

	zoneCache_.setMaxCost(128 * 1024 * 1024);

	qInfo().noquote() << QString("DynamicZoneArray: %1 zones, %2 stars, %3 MB zone table (mmap), "
				      "%4 kB block table (block=%5), cache %6 MB")
		.arg(nr_of_zones).arg(nr_of_stars)
		.arg(zoneTableSize / (1024 * 1024))
		.arg(blockOffsets_.size() * 8 / 1024)
		.arg(BLOCK_SIZE)
		.arg(zoneCache_.maxCost() / (1024 * 1024));
}

template<class StarType>
DynamicZoneArray<StarType>::~DynamicZoneArray()
{
	fileValid_ = false;
	for (auto it = pendingLoads_.begin(); it != pendingLoads_.end(); ++it)
	{
		it.value().waitForFinished();
		delete it.value().result();
	}
	pendingLoads_.clear();
	zoneCache_.clear();
	if (zoneTableMmapStart_ && file)
	{
		file->unmap(zoneTableMmapStart_);
		zoneTableMmapStart_ = nullptr;
	}
	if (file)
	{
		file->close();
		delete file;
		file = nullptr;
	}
}

template<class StarType>
void DynamicZoneArray<StarType>::drainPendingLoads() const
{
	QMutableHashIterator<int, QFuture<QByteArray*>> it(pendingLoads_);
	while (it.hasNext())
	{
		it.next();
		if (!it.value().isFinished())
			continue;
		QByteArray* data = it.value().result();
		if (data)
			zoneCache_.insert(it.key(), data, data->size());
		it.remove();
	}
}

template<class StarType>
const StarType* DynamicZoneArray<StarType>::loadZone(int zone_index) const
{
	drainPendingLoads();

	QByteArray* cached = zoneCache_[zone_index];
	if (cached)
		return reinterpret_cast<const StarType*>(cached->constData());

	uint32_t count = zoneCounts_[zone_index];
	if (count == 0 || !file || !file->isOpen())
		return nullptr;

	if (pendingLoads_.contains(zone_index))
		return nullptr;

	if (!fileValid_)
		return nullptr;

	const unsigned int block = zone_index / BLOCK_SIZE;
	uint64_t off = blockOffsets_[block];
	const unsigned int zStart = block * BLOCK_SIZE;
	for (unsigned int z = zStart; z < static_cast<unsigned int>(zone_index); ++z)
		off += static_cast<uint64_t>(zoneCounts_[z]) * sizeof(StarType);

	const qint64 starDataBase = 28 + static_cast<qint64>(sizeof(uint32_t)) * nr_of_zones;
	const qint64 fileOffset = starDataBase + static_cast<qint64>(off);
	const qint64 size = static_cast<qint64>(count) * sizeof(StarType);

	auto future = QtConcurrent::run([this, fileOffset, size]() -> QByteArray* {
		if (!fileValid_)
			return nullptr;
		QFile localFile(fname);
		if (!localFile.open(QIODevice::ReadOnly))
			return nullptr;
		auto* data = new QByteArray(size, Qt::Uninitialized);
		localFile.seek(fileOffset);
		if (localFile.read(data->data(), size) != size)
		{
			delete data;
			return nullptr;
		}
		return data;
	});

	pendingLoads_.insert(zone_index, future);
	return nullptr;
}

template<class StarType>
const StarType* DynamicZoneArray<StarType>::loadZoneSync(int zone_index) const
{
	drainPendingLoads();

	QByteArray* cached = zoneCache_[zone_index];
	if (cached)
		return reinterpret_cast<const StarType*>(cached->constData());

	uint32_t count = zoneCounts_[zone_index];
	if (count == 0 || !file || !file->isOpen())
		return nullptr;

	if (pendingLoads_.contains(zone_index))
	{
		QFuture<QByteArray*> f = pendingLoads_.take(zone_index);
		f.waitForFinished();
		QByteArray* data = f.result();
		if (data)
		{
			zoneCache_.insert(zone_index, data, data->size());
			return reinterpret_cast<const StarType*>(data->constData());
		}
		return nullptr;
	}

	const unsigned int block = zone_index / BLOCK_SIZE;
	uint64_t off = blockOffsets_[block];
	const unsigned int zStart = block * BLOCK_SIZE;
	for (unsigned int z = zStart; z < static_cast<unsigned int>(zone_index); ++z)
		off += static_cast<uint64_t>(zoneCounts_[z]) * sizeof(StarType);

	const qint64 starDataBase = 28 + static_cast<qint64>(sizeof(uint32_t)) * nr_of_zones;
	const qint64 fileOffset = starDataBase + static_cast<qint64>(off);
	const qint64 size = static_cast<qint64>(count) * sizeof(StarType);

	auto* data = new QByteArray(size, Qt::Uninitialized);
	{
		QMutexLocker locker(&fileMutex_);
		if (!file || !file->isOpen() || !fileValid_)
		{
			delete data;
			return nullptr;
		}
		file->seek(fileOffset);
		if (file->read(data->data(), size) != size)
		{
			qWarning() << "DynamicZoneArray::loadZoneSync: read error for zone" << zone_index;
			delete data;
			return nullptr;
		}
	}
	zoneCache_.insert(zone_index, data, data->size());
	return reinterpret_cast<const StarType*>(data->constData());
}

template<class StarType>
void DynamicZoneArray<StarType>::prefetchRegion(const QVector<SphericalCap>& caps, int maxGridLevel) const
{
	auto* core = StelApp::getInstance().getCore();
	const GeodesicSearchResult* result =
		core->getGeodesicGrid(maxGridLevel)->search(caps, maxGridLevel);

	int prefetched = 0;
	int zone;
	for (GeodesicSearchInsideIterator it(*result, level);
	     (zone = it.next()) >= 0; )
	{
		if (zoneCache_.contains(zone)) continue;
		loadZone(zone);
		++prefetched;
	}
	for (GeodesicSearchBorderIterator it(*result, level);
	     (zone = it.next()) >= 0; )
	{
		if (zoneCache_.contains(zone)) continue;
		loadZone(zone);
		++prefetched;
	}

	if (prefetched > 0)
		qDebug().noquote() << QString("DynamicZoneArray level %1: prefetched %2 zones").arg(level).arg(prefetched);
}

template<class StarType>
void DynamicZoneArray<StarType>::draw(StelPainter* sPainter, int index, bool isInsideViewport,
			     const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
			     int maxMagStarName, float names_brightness,
			     const QVector<SphericalCap>& boundingCaps,
			     const bool withAberration, const Vec3d vel,
			     const double withParallax, const Vec3d diffPos,
			     const bool withCommonNameI18n) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	StelSkyDrawer* drawer = core->getSkyDrawer();

	int cutoffMagStep = limitMagIndex;
	float cutoffMag = 999999.f;
	if (drawer->getFlagStarMagnitudeLimit())
	{
		cutoffMag = drawer->getCustomStarMagnitudeLimit() * 1000.f;
		cutoffMagStep = static_cast<int>((cutoffMag - (mag_min - 7000.)) * 0.02);
		if (cutoffMagStep > limitMagIndex)
			cutoffMagStep = limitMagIndex;
	}

	if (cutoffMagStep < 140)
		return;

	const StarType* stars = loadZone(index);
	if (!stars)
		return;

	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;

	const Extinction& extinction = drawer->getExtinction();
	const bool withExtinction = drawer->getFlagHasAtmosphere() &&
				    extinction.getExtinctionCoefficient() >= 0.01f;

	Q_ASSERT_X(cutoffMagStep <= RCMAG_TABLE_SIZE, "DynamicZoneArray::draw",
		   QString("RCMAG_TABLE_SIZE: %1, cutoffMagStep: %2")
			   .arg(QString::number(RCMAG_TABLE_SIZE), QString::number(cutoffMagStep)).toLatin1());

	uint32_t zoneSize = zoneCounts_[index];
	bool globalzone = (static_cast<unsigned int>(index) == nr_of_zones - 1);
	Vec3d v;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		const StarType& s = stars[i];

		int mag = s.getMag();
		int magIndex = static_cast<int>((mag - (mag_min - 7000.)) * 0.02);

		if ((magIndex > cutoffMagStep) || (static_cast<float>(mag) > cutoffMag))
		{
			if (std::fabs(dyrs) <= 5000.f || !globalzone)
				break;
		}

		s.getJ2000Pos(dyrs, v);

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

template<class StarType>
void DynamicZoneArray<StarType>::searchAround(const StelCore* core, int index, const Vec3d &v,
				     const double withParallax, const Vec3d diffPos,
				     double cosLimFov, QList<StelObjectP> &result)
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	const StarType* stars = loadZoneSync(index);
	if (!stars)
		return;

	uint32_t zoneSize = zoneCounts_[index];
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	Vec3d tmp;

	(void)dyrs;
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

template<class StarType>
void DynamicZoneArray<StarType>::searchWithin(const StelCore* core, int index,
				     const SphericalRegionP region,
				     const double withParallax, const Vec3d diffPos,
				     const bool /*hipOnly*/, const float maxMag,
				     QList<StelObjectP> &result) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	const StarType* stars = loadZoneSync(index);
	if (!stars)
		return;

	uint32_t zoneSize = zoneCounts_[index];
	const float maxMilliMag = 1000.f * maxMag;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getMag() >= maxMilliMag)
			continue;

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

template<class StarType>
StelObjectP DynamicZoneArray<StarType>::searchGaiaID(int index, const StarId source_id,
					    int& matched) const
{
	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return StelObjectP();

	const StarType* stars = loadZoneSync(index);
	if (!stars)
		return StelObjectP();

	uint32_t zoneSize = zoneCounts_[index];
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

template<class StarType>
void DynamicZoneArray<StarType>::searchGaiaIDepochPos(const StarId source_id, float dyrs,
					     double & RA, double & DEC, double & Plx,
					     double & pmra, double & pmdec, double & RV) const
{
	(void)dyrs;
	for (unsigned int z = 0; z < nr_of_zones; ++z)
	{
		const StarType* stars = loadZoneSync(z);
		if (!stars)
			continue;

		uint32_t zoneSize = zoneCounts_[z];
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

template class DynamicZoneArray<Star1>;
template class DynamicZoneArray<Star2>;
template class DynamicZoneArray<Star3>;
