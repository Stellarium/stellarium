/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
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

#include "ZoneArray.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"
#include "StarMgr.hpp"

#include <QDebug>
#include <QFile>
#include <QDir>
#ifdef Q_OS_WIN
#include <io.h>
#include <Windows.h>
#endif

//! Compute the magnitude cutoff step and precise magnitude cutoff.
static void computeMagnitudeCutoff(int mag_min, int limitMagIndex, const StelSkyDrawer* drawer,
				   int& cutoffMagStep, float& cutoffMag)
{
	cutoffMagStep = limitMagIndex;
	cutoffMag = 999999.f;
	if (drawer->getFlagStarMagnitudeLimit())
	{
		cutoffMag = drawer->getCustomStarMagnitudeLimit() * 1000.f;
		cutoffMagStep = static_cast<int>((cutoffMag - (mag_min - 7000.)) * 0.02); // 1/(50 milli-mag)
		if (cutoffMagStep > limitMagIndex)
			cutoffMagStep = limitMagIndex;
	}
}

//! Check whether the point v lies inside all spherical caps.
static bool isVisibleInCaps(const Vec3d& v, const QVector<SphericalCap>& caps)
{
	for (const auto& cap : caps)
	{
		if (!cap.contains(v))
			return false;
	}
	return true;
}

//! Apply atmospheric extinction to a star and update its magnitude index / twinkle factor.
static bool applyExtinction(int magIndex, int cutoffMagStep, const Vec3d& v, const StelCore* core,
			    const Extinction& extinction, int& extinctedMagIndex,
			    const RCMag*& tmpRcmag, float& twinkleFactor)
{
	extinctedMagIndex = magIndex;
	twinkleFactor = 1.f;

	Vec3d altAz(v);
	altAz.normalize();
	core->j2000ToAltAzInPlaceNoRefraction(&altAz);
	float extMagShift = 0.f;
	extinction.forward(altAz, &extMagShift);
	extinctedMagIndex += static_cast<int>(extMagShift / 0.05f); // 0.05 mag MagStepIncrement
	if (extinctedMagIndex >= cutoffMagStep || extinctedMagIndex < 0)
		return false;
	tmpRcmag += (extinctedMagIndex - magIndex);
	twinkleFactor = qMin(1.f, 1.f - 0.9f * static_cast<float>(altAz[2]));
	return true;
}

//! Apply annual aberration to a J2000 direction.
static void applyAberration(Vec3d& v, const StelCore* core)
{
	v += core->getAberrationVec(core->getJDE());
	v.normalize();
}

// ==================== Byte-swap utilities ====================

static unsigned int stel_bswap_32(unsigned int val)
{
	return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
	       (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
}

static float stel_bswap_32f(float val)
{
	float f;
	unsigned int u;
	std::memcpy(&u, &val, sizeof(val));
	u = (((u) & 0xff000000) >> 24) | (((u) & 0x00ff0000) >>  8) |
	    (((u) & 0x0000ff00) <<  8) | (((u) & 0x000000ff) << 24);
	std::memcpy(&f, &u, sizeof(u));
	return f;
}

static const Vec3f north(0, 0, 1);

void ZoneArray::initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2)
{
	(void)index;
	(void)c0;
	(void)c1;
	(void)c2;
}

static inline int ReadInt(QFile& file, unsigned int &x)
{
	const int rval = (4 == file.read(reinterpret_cast<char*>(&x), 4)) ? 0 : -1;
	return rval;
}

static inline int ReadFloat(QFile& file, float &x)
{
	char data[sizeof x];
	if (file.read(data, sizeof data) != sizeof data)
		return -1;
	std::memcpy(&x, data, sizeof data);
	return 0;
}

#if (!defined(__GNUC__))
#ifndef _MSC_BUILD
#warning Star catalogue loading has only been tested with gcc
#endif
#endif

// ==================== ZoneArray factory & base ====================

ZoneArray* ZoneArray::create(const QString& catalogFilePath, bool use_mmap, bool use_dynamic)
{
	QString dbStr;
	QFile* file = new QFile(catalogFilePath);
	if (!file->open(QIODevice::ReadOnly))
	{
		qWarning().noquote() << "Error while loading" << QDir::toNativeSeparators(catalogFilePath) << ": failed to open file.";
		return Q_NULLPTR;
	}
	dbStr = "Loading star catalog: " + QDir::toNativeSeparators(catalogFilePath) + " - ";

	unsigned int magic, major, minor, type, level, mag_min;
	float epochJD;
	if (ReadInt(*file, magic) < 0 ||
	    ReadInt(*file, type) < 0 ||
	    ReadInt(*file, major) < 0 ||
	    ReadInt(*file, minor) < 0 ||
	    ReadInt(*file, level) < 0 ||
	    ReadInt(*file, mag_min) < 0 ||
	    ReadFloat(*file, epochJD) < 0)
	{
		dbStr += "error - file format is bad.";
		qDebug().noquote() << dbStr;
		return Q_NULLPTR;
	}

	const bool byte_swap = (magic == FILE_MAGIC_OTHER_ENDIAN);
	if (byte_swap)
	{
		if (use_mmap)
		{
			dbStr += "warning - must convert catalogue ";
#if (!defined(__GNUC__))
			dbStr += "to native format ";
#endif
			dbStr += "before mmap loading ";
			qWarning().noquote() << dbStr;
			use_mmap = false;
			qWarning().noquote() << "Revert to not using mmap";
		}
		dbStr += "byteswap ";
		type = stel_bswap_32(type);
		major = stel_bswap_32(major);
		minor = stel_bswap_32(minor);
		level = stel_bswap_32(level);
		mag_min = stel_bswap_32(mag_min);
		epochJD = stel_bswap_32f(epochJD);
	}
	else if (magic == FILE_MAGIC)
	{
#if (!defined(__GNUC__) && !defined(_MSC_BUILD))
		if (use_mmap)
		{
			dbStr += "warning - you must convert catalogue to native format before mmap loading ";
			qDebug(qPrintable(dbStr));
			return 0;
		}
#endif
	}
	else if (magic == FILE_MAGIC_NATIVE)
	{
	}
	else
	{
		dbStr += "error - not a catalogue file. ";
		qDebug().noquote() << dbStr;
		return Q_NULLPTR;
	}
	if (epochJD != STAR_CATALOG_JDEPOCH)
	{
		qDebug().noquote() << QString("%1 != %2").arg(QString::number(epochJD, 'f', 5), QString::number(STAR_CATALOG_JDEPOCH, 'f', 5));
		dbStr += "warning - Star catalog epoch is not what is expected in Stellarium";
		qDebug().noquote() << dbStr;
		return Q_NULLPTR;
	}

	ZoneArray* rval = Q_NULLPTR;
	dbStr += QString("%1_%2v%3_%4; ").arg(level).arg(type).arg(major).arg(minor);

	switch (type)
	{
		case 0:
			if (major > MAX_MAJOR_FILE_VERSION)
				dbStr += "warning - unsupported version ";
			else
				rval = new HipZoneArray(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			break;
		case 1:
			if (major > MAX_MAJOR_FILE_VERSION)
				dbStr += "warning - unsupported version ";
			else if (use_dynamic)
				rval = new DynamicZoneArray<Star2>(file->fileName(), file, static_cast<int>(level), static_cast<int>(mag_min), byte_swap);
			else
				rval = new SpecialZoneArray<Star2>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			break;
		case 2:
			if (major > MAX_MAJOR_FILE_VERSION)
				dbStr += "warning - unsupported version ";
			else if (use_dynamic)
				rval = new DynamicZoneArray<Star3>(file->fileName(), file, static_cast<int>(level), static_cast<int>(mag_min), byte_swap);
			else
				rval = new SpecialZoneArray<Star3>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			break;
		default:
			dbStr += "error - bad file type ";
			break;
	}
	if (rval && rval->isInitialized())
	{
		dbStr += QString("%1 entries").arg(rval->getNrOfStars());
		qInfo().noquote() << dbStr;
	}
	else
	{
		dbStr += "- initialization failed ";
		qDebug().noquote() << dbStr;
		if (rval)
		{
			delete rval;
			rval = Q_NULLPTR;
		}
	}
	return rval;
}

ZoneArray::ZoneArray(const QString& fname, QFile* file, int level, int mag_min)
	: fname(fname), level(level), mag_min(mag_min),
	  nr_of_stars(0), zones(Q_NULLPTR), file(file)
{
	nr_of_zones = static_cast<unsigned int>(StelGeodesicGrid::nrOfZones(level));
}

bool ZoneArray::readFile(QFile& file, void *data, qint64 size)
{
	int parts = 256;
	int part_size = static_cast<int>((size + (parts >> 1)) / parts);
	if (part_size < 64 * 1024)
		part_size = 64 * 1024;
	while (size > 0)
	{
		const int to_read = (part_size < size) ? part_size : static_cast<int>(size);
		const int read_rc = static_cast<int>(file.read(static_cast<char*>(data), to_read));
		if (read_rc != to_read)
			return false;
		size -= read_rc;
		data = (static_cast<char*>(data)) + read_rc;
	}
	return true;
}

// ==================== HipZoneArray ====================

void HipZoneArray::updateHipIndex(HipIndexStruct hipIndex[]) const
{
	for (const SpecialZoneData<Star1> *z = getZones() + (nr_of_zones - 1); z >= getZones(); z--)
	{
		for (const Star1 *s = z->getStars() + z->size - 1; s >= z->getStars(); s--)
		{
			const StarId hip = s->getHip();
			if (hip > NR_OF_HIP)
			{
				qDebug() << "ERROR: HipZoneArray::updateHipIndex: invalid HIP number:" << hip;
				exit(1);
			}
			if (hip != 0)
			{
				if (hipIndex[hip].a)
				{
					if (hipIndex[hip].s->getComponentIds() > s->getComponentIds())
					{
						hipIndex[hip].a = this;
						hipIndex[hip].z = z;
						hipIndex[hip].s = s;
					}
				}
				else
				{
					hipIndex[hip].a = this;
					hipIndex[hip].z = z;
					hipIndex[hip].s = s;
				}
			}
		}
	}
}

// ==================== SpecialZoneArray ====================

template<class StarType>
SpecialZoneArray<StarType>::SpecialZoneArray(QFile* file, bool byte_swap, bool use_mmap,
					     int level, int mag_min)
	: Base(file->fileName(), file, level, mag_min),
	  stars(Q_NULLPTR), mmap_start(Q_NULLPTR)
{
	if (this->nr_of_zones > 0)
	{
		this->zones = new SpecialZoneData<StarType>[this->nr_of_zones];

		unsigned int *zone_size = new unsigned int[this->nr_of_zones];
		if (static_cast<qint64>(sizeof(unsigned int) * this->nr_of_zones) !=
		    this->file->read(reinterpret_cast<char*>(zone_size), sizeof(unsigned int) * this->nr_of_zones))
		{
			qDebug() << "Error reading zones from catalog:" << this->file->fileName();
			delete[] getZones();
			this->zones = Q_NULLPTR;
			this->nr_of_zones = 0;
		}
		else
		{
			const unsigned int *tmp = zone_size;
			for (unsigned int z = 0; z < this->nr_of_zones; z++, tmp++)
			{
				const unsigned int tmp_spu_int32 = byte_swap ? stel_bswap_32(*tmp) : *tmp;
				this->nr_of_stars += tmp_spu_int32;
				getZones()[z].size = static_cast<int>(tmp_spu_int32);
			}
			getZones()[this->nr_of_zones - 1].isGlobal = true;
		}
		delete[] zone_size;

		if (this->nr_of_stars == 0)
		{
			if (this->zones)
				delete[] getZones();
			this->zones = Q_NULLPTR;
			this->nr_of_zones = 0;
		}
		else
		{
			if (use_mmap)
			{
				mmap_start = this->file->map(this->file->pos(), sizeof(StarType) * this->nr_of_stars);
				if (mmap_start == Q_NULLPTR)
				{
					qDebug() << "ERROR: SpecialZoneArray(" << this->level
						 << ")::SpecialZoneArray: QFile(" << this->file->fileName()
						 << ".map(" << this->file->pos()
						 << ',' << sizeof(StarType) * this->nr_of_stars
						 << ") failed: " << this->file->errorString();
					stars = Q_NULLPTR;
					this->nr_of_stars = 0;
					delete[] getZones();
					this->zones = Q_NULLPTR;
					this->nr_of_zones = 0;
				}
				else
				{
					stars = reinterpret_cast<StarType*>(mmap_start);
					StarType *s = stars;
					for (unsigned int z = 0; z < this->nr_of_zones; z++)
					{
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
				}
				this->file->close();
			}
			else
			{
				stars = new StarType[this->nr_of_stars];
				if (!this->readFile(*this->file, stars, sizeof(StarType) * this->nr_of_stars))
				{
					delete[] stars;
					stars = Q_NULLPTR;
					this->nr_of_stars = 0;
					delete[] getZones();
					this->zones = Q_NULLPTR;
					this->nr_of_zones = 0;
				}
				else
				{
					StarType *s = stars;
					for (unsigned int z = 0; z < this->nr_of_zones; z++)
					{
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
				}
				this->file->close();
			}
		}
	}
}

template<class StarType>
SpecialZoneArray<StarType>::~SpecialZoneArray(void)
{
	if (stars)
	{
		if (mmap_start != Q_NULLPTR)
			this->file->unmap(mmap_start);
		else
			delete[] stars;
		delete this->file;
		stars = Q_NULLPTR;
	}
	if (this->zones)
	{
		delete[] getZones();
		this->zones = Q_NULLPTR;
	}
	this->nr_of_zones = 0;
	this->nr_of_stars = 0;
}

template<class StarType>
bool SpecialZoneArray<StarType>::loadZoneDraw(int index, const StarType*& outStars,
					      uint32_t& size, bool& isGlobal) const
{
	auto* z = getZones() + index;
	outStars = z->getStars();
	size = static_cast<uint32_t>(z->size);
	isGlobal = z->isGlobal;
	return true;
}

template<class StarType>
bool SpecialZoneArray<StarType>::loadZoneSearch(int index, const StarType*& outStars,
						uint32_t& size, bool& isGlobal) const
{
	return loadZoneDraw(index, outStars, size, isGlobal);
}

template<class StarType>
StelObjectP SpecialZoneArray<StarType>::createStelObj(const StarType* s) const
{
	// Zone data pointer not needed by createStelObject in this context
	return s->createStelObject(static_cast<const SpecialZoneArray<StarType>*>(this),
				   static_cast<const SpecialZoneData<StarType>*>(nullptr));
}

// ==================== DynamicZoneArray ====================

template<class StarType>
DynamicZoneArray<StarType>::DynamicZoneArray(const QString& fname, QFile* file,
					     int level, int mag_min, bool byte_swap)
	: Base(fname, file, level, mag_min),
	  zoneCounts_(Q_NULLPTR), zoneTableMmapStart_(Q_NULLPTR)
{
	if (this->nr_of_zones == 0)
		return;

	// Read zone counts (same layout as SpecialZoneArray header)
	const qint64 zoneCountsSize = static_cast<qint64>(sizeof(uint32_t)) * this->nr_of_zones;
	zoneCounts_ = new uint32_t[this->nr_of_zones];
	if (this->file->read(reinterpret_cast<char*>(zoneCounts_), zoneCountsSize) != zoneCountsSize)
	{
		qDebug() << "Error reading zone counts from catalog:" << this->file->fileName();
		delete[] zoneCounts_;
		zoneCounts_ = Q_NULLPTR;
		this->nr_of_zones = 0;
		return;
	}

	// Build block offsets for fast lookup
	const uint32_t numBlocks = (this->nr_of_zones + BLOCK_SIZE - 1) / BLOCK_SIZE;
	blockOffsets_.resize(numBlocks + 1, 0);

	uint64_t totalStars = 0;
	for (unsigned int z = 0; z < this->nr_of_zones; z++)
	{
		uint32_t count = zoneCounts_[z];
		if (byte_swap)
			count = stel_bswap_32(count);
		zoneCounts_[z] = count;
		this->nr_of_stars += count;
		totalStars += count;
		if (z % BLOCK_SIZE == 0)
			blockOffsets_[z / BLOCK_SIZE] = totalStars - count;
	}
	blockOffsets_[numBlocks] = totalStars;

	const qint64 starDataBase = 28 + static_cast<qint64>(sizeof(uint32_t)) * this->nr_of_zones;
	this->file->seek(starDataBase + static_cast<qint64>(totalStars) * sizeof(StarType));

	// Mark the file position as valid for future reads
	fileValid_ = true;
}

template<class StarType>
DynamicZoneArray<StarType>::~DynamicZoneArray(void)
{
	if (zoneCounts_)
	{
		delete[] zoneCounts_;
		zoneCounts_ = Q_NULLPTR;
	}
	if (this->file)
	{
		delete this->file;
		this->file = Q_NULLPTR;
	}
	this->nr_of_zones = 0;
	this->nr_of_stars = 0;
}

template<class StarType>
const StarType* DynamicZoneArray<StarType>::loadZone(int zone_index) const
{
	if (static_cast<unsigned int>(zone_index) >= this->nr_of_zones)
		return nullptr;

	const uint32_t count = zoneCounts_[zone_index];
	if (count == 0)
		return nullptr;

	// Check cache first
	auto* cached = zoneCache_.object(zone_index);
	if (cached)
		return reinterpret_cast<const StarType*>(cached->constData());

	// Compute file offset
	const uint32_t block = static_cast<uint32_t>(zone_index) / BLOCK_SIZE;
	uint64_t off = blockOffsets_[block] * sizeof(StarType);
	const uint32_t zStart = block * BLOCK_SIZE;
	for (uint32_t z = zStart; z < static_cast<uint32_t>(zone_index); ++z)
		off += static_cast<uint64_t>(zoneCounts_[z]) * sizeof(StarType);

	const qint64 starDataBase = 28 + static_cast<qint64>(sizeof(uint32_t)) * this->nr_of_zones;
	const qint64 fileOffset = starDataBase + static_cast<qint64>(off);
	const qint64 size = static_cast<qint64>(count) * sizeof(StarType);

	auto* data = new QByteArray(static_cast<int>(size), Qt::Uninitialized);
	{
		QMutexLocker locker(&fileMutex_);
		if (!this->file || !this->file->isOpen() || !fileValid_)
		{
			delete data;
			return nullptr;
		}
		this->file->seek(fileOffset);
		if (this->file->read(data->data(), size) != size)
		{
			qWarning() << "DynamicZoneArray::loadZone: read error for zone" << zone_index;
			delete data;
			return nullptr;
		}
	}
	zoneCache_.insert(zone_index, data, static_cast<int>(size));
	return reinterpret_cast<const StarType*>(data->constData());
}

template<class StarType>
const StarType* DynamicZoneArray<StarType>::loadZoneSync(int zone_index) const
{
	// Same as loadZone but without async concerns
	return loadZone(zone_index);
}

template<class StarType>
void DynamicZoneArray<StarType>::prefetchRegion(const QVector<SphericalCap>& caps,
						int maxGridLevel) const
{
	auto* core = StelApp::getInstance().getCore();
	const GeodesicSearchResult* result =
		core->getGeodesicGrid(maxGridLevel)->search(caps, maxGridLevel);

	int prefetched = 0;
	int zone;
	for (GeodesicSearchInsideIterator it(*result, this->level);
	     (zone = it.next()) >= 0;)
	{
		if (zoneCache_.contains(zone))
			continue;
		loadZone(zone);
		++prefetched;
	}
	for (GeodesicSearchBorderIterator it(*result, this->level);
	     (zone = it.next()) >= 0;)
	{
		if (zoneCache_.contains(zone))
			continue;
		loadZone(zone);
		++prefetched;
	}

	if (prefetched > 0)
		qDebug().noquote() << QString("DynamicZoneArray level %1: prefetched %2 zones")
				      .arg(this->level).arg(prefetched);
}

template<class StarType>
void DynamicZoneArray<StarType>::drainPendingLoads() const
{
	// Drain any pending async loads (placeholder)
}

template<class StarType>
bool DynamicZoneArray<StarType>::loadZoneDraw(int index, const StarType*& outStars,
					      uint32_t& size, bool& isGlobal) const
{
	if (static_cast<unsigned int>(index) >= this->nr_of_zones)
		return false;
	outStars = loadZone(index);
	if (!outStars)
		return false;
	size = zoneCounts_[index];
	isGlobal = (static_cast<unsigned int>(index) == this->nr_of_zones - 1);
	return true;
}

template<class StarType>
bool DynamicZoneArray<StarType>::loadZoneSearch(int index, const StarType*& outStars,
						uint32_t& size, bool& isGlobal) const
{
	if (static_cast<unsigned int>(index) >= this->nr_of_zones)
		return false;
	outStars = loadZoneSync(index);
	if (!outStars)
		return false;
	size = zoneCounts_[index];
	isGlobal = (static_cast<unsigned int>(index) == this->nr_of_zones - 1);
	return true;
}

template<class StarType>
StelObjectP DynamicZoneArray<StarType>::createStelObj(const StarType* s) const
{
	return s->createStelObject(nullptr, nullptr);
}

// ==================== ZoneArrayImpl — shared draw/search ====================

template<class Derived, class StarType>
void ZoneArrayImpl<Derived, StarType>::drawStarRange(
		const StarType* first, const StarType* last, bool globalzone,
		StelPainter* sPainter, bool isInsideViewport,
		const RCMag* rcmag_table, int cutoffMagStep, float cutoffMag,
		float dyrs, StelCore* core, StelSkyDrawer* drawer,
		const QVector<SphericalCap>& boundingCaps,
		bool withAberration,
		double withParallax, const Vec3d& diffPos,
		int maxMagStarName, float names_brightness,
		bool withExtinction, const Extinction& extinction,
		bool withCommonNameI18n) const
{
	for (const StarType* s = first; s < last; ++s)
	{
		float starMag = s->getMag();
		int magIndex = static_cast<int>((starMag - (this->mag_min - 7000.)) * 0.02);

		if ((magIndex > cutoffMagStep) || (starMag > cutoffMag))
		{
			if (std::fabs(dyrs) <= 5000.f || !s->isVIP() || !globalzone)
				break;
		}

		bool recomputeMag = s->getPreciseAstrometricFlag();
		double Plx = s->getPlx();
		Vec3d v;
		if (recomputeMag)
		{
			double vr = s->getRV();
			Vec3d pmvec0(s->getDx0(), s->getDx1(), s->getDx2());
			pmvec0 = pmvec0 * MAS2RAD;
			double pmr0 = vr * Plx / (AU / JYEAR_SECONDS) * MAS2RAD;
			double pmtotsqr = (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);
			double f = 1. / sqrt(1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0 * pmr0) * dyrs * dyrs);
			Plx *= f;
			float magOffset = 5.f * log10(1 / f);
			starMag += magOffset * 1000.f;
			Vec3d r(s->getX0(), s->getX1(), s->getX2());
			Vec3d u = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;
			v.set(u[0], u[1], u[2]);
		}

		magIndex = static_cast<int>((starMag - (this->mag_min - 7000.)) * 0.02);
		if ((magIndex > cutoffMagStep) || (starMag > cutoffMag))
			continue;

		if (!recomputeMag)
			s->getJ2000Pos(dyrs, v);

		s->getBinaryOrbit(core->getJDE(), v);

		if (withParallax)
		{
			s->getPlxEffect(withParallax * Plx, v, diffPos);
			v.normalize();
		}

		if (withAberration)
			applyAberration(v, core);

		if (!isInsideViewport && !isVisibleInCaps(v, boundingCaps))
			continue;

		int extinctedMagIndex = magIndex;
		float twinkleFactor = 1.0f;
		const RCMag* tmpRcmag = &rcmag_table[magIndex];

		if (withExtinction && !applyExtinction(magIndex, cutoffMagStep, v, core, extinction, extinctedMagIndex, tmpRcmag, twinkleFactor))
			continue;

		if (drawer->drawPointSource(sPainter, v, *tmpRcmag, s->getBVIndex(), !isInsideViewport, twinkleFactor) &&
		    core->getFlagClearSky() && s->hasName() && extinctedMagIndex < maxMagStarName && s->hasComponentID() <= 1)
		{
			const float offset = tmpRcmag->radius * 0.7f;
			const Vec3f color = StelSkyDrawer::indexToColor(s->getBVIndex()) * 0.75f;
			sPainter->setColor(color, names_brightness);
			sPainter->drawText(v, s->getScreenNameI18n(withCommonNameI18n), 0, offset, offset, false);
		}
	}
}

template<class Derived, class StarType>
void ZoneArrayImpl<Derived, StarType>::draw(
		StelPainter* sPainter, int index, bool isInsideViewport,
		const RCMag* rcmag_table, int limitMagIndex, StelCore* core,
		int maxMagStarName, float names_brightness,
		const QVector<SphericalCap>& boundingCaps,
		const bool withAberration, const Vec3d vel,
		const double withParallax, const Vec3d diffPos,
		const bool withCommonNameI18n) const
{
	(void)vel;

	StelSkyDrawer* drawer = core->getSkyDrawer();
	int cutoffMagStep;
	float cutoffMag;
	computeMagnitudeCutoff(this->mag_min, limitMagIndex, drawer, cutoffMagStep, cutoffMag);

	if (cutoffMagStep < 140)
		return;

	const StarType* stars = nullptr;
	uint32_t zoneSize = 0;
	bool globalzone = false;
	if (!zoneForDraw(index, stars, zoneSize, globalzone) || !stars)
		return;

	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;

	const Extinction& extinction = drawer->getExtinction();
	const bool withExtinction = drawer->getFlagHasAtmosphere() &&
				    extinction.getExtinctionCoefficient() >= 0.01f;

	Q_ASSERT_X(cutoffMagStep <= RCMAG_TABLE_SIZE, "ZoneArrayImpl::draw",
		   QString("RCMAG_TABLE_SIZE: %1, cutoffMagStep: %2")
			   .arg(QString::number(RCMAG_TABLE_SIZE), QString::number(cutoffMagStep)).toLatin1());

	drawStarRange(stars, stars + zoneSize, globalzone,
		      sPainter, isInsideViewport, rcmag_table, cutoffMagStep, cutoffMag,
		      dyrs, core, drawer, boundingCaps, withAberration,
		      withParallax, diffPos, maxMagStarName, names_brightness,
		      withExtinction, extinction, withCommonNameI18n);
}

template<class Derived, class StarType>
void ZoneArrayImpl<Derived, StarType>::searchAround(
		const StelCore* core, int index, const Vec3d &v,
		const double withParallax, const Vec3d diffPos,
		double cosLimFov, QList<StelObjectP> &result)
{
	const StarType* stars = nullptr;
	uint32_t zoneSize = 0;
	bool globalzone = false;
	if (!zoneForSearch(index, stars, zoneSize, globalzone) || !stars)
		return;

	(void)globalzone;
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		stars[i].getJ2000Pos(dyrs, tmp);
		stars[i].getBinaryOrbit(core->getJDE(), tmp);
		stars[i].getPlxEffect(withParallax * stars[i].getPlx(), tmp, diffPos);
		tmp.normalize();

		if (core->getUseAberration())
		{
			const Vec3d vel = core->getAberrationVec(core->getJDE());
			tmp += vel;
			tmp.normalize();
		}

		if (tmp * v >= cosLimFov)
		{
			result.push_back(wrapStar(&stars[i], static_cast<int>(i)));
		}
	}
}

template<class Derived, class StarType>
void ZoneArrayImpl<Derived, StarType>::searchWithin(
		const StelCore* core, int index, const SphericalRegionP region,
		const double withParallax, const Vec3d diffPos,
		const bool hipOnly, const float maxMag,
		QList<StelObjectP> &result) const
{
	if (hipOnly && this->level > 3)
		return;

	const StarType* stars = nullptr;
	uint32_t zoneSize = 0;
	bool globalzone = false;
	if (!zoneForSearch(index, stars, zoneSize, globalzone) || !stars)
		return;

	(void)globalzone;
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	const float maxMilliMag = 1000.f * maxMag;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getMag() >= maxMilliMag)
			continue;

		stars[i].getJ2000Pos(dyrs, tmp);
		stars[i].getBinaryOrbit(core->getJDE(), tmp);
		stars[i].getPlxEffect(withParallax * stars[i].getPlx(), tmp, diffPos);
		tmp.normalize();

		if (core->getUseAberration())
		{
			const Vec3d vel = core->getAberrationVec(core->getJDE());
			tmp += vel;
			tmp.normalize();
		}

		if (region->contains(tmp))
		{
			result.push_back(wrapStar(&stars[i], static_cast<int>(i)));
		}
	}
}

template<class Derived, class StarType>
StelObjectP ZoneArrayImpl<Derived, StarType>::searchGaiaID(
		int index, const StarId source_id, int& matched) const
{
	const StarType* stars = nullptr;
	uint32_t zoneSize = 0;
	bool globalzone = false;
	if (!zoneForSearch(index, stars, zoneSize, globalzone) || !stars)
		return StelObjectP();

	(void)globalzone;
	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getGaia() == source_id)
		{
			++matched;
			return wrapStar(&stars[i], static_cast<int>(i));
		}
	}
	return StelObjectP();
}

template<class Derived, class StarType>
void ZoneArrayImpl<Derived, StarType>::searchGaiaIDepochPos(
		const StarId source_id, float dyrs,
		double & RA, double & DEC, double & Plx,
		double & pmra, double & pmdec, double & RV) const
{
	for (unsigned int z = 0; z < this->nr_of_zones; ++z)
	{
		const StarType* stars = nullptr;
		uint32_t zoneSize = 0;
		bool isGlobal = false;
		if (!zoneForSearch(static_cast<int>(z), stars, zoneSize, isGlobal) || !stars)
			continue;

		(void)isGlobal;
		for (uint32_t i = 0; i < zoneSize; ++i)
		{
			if (stars[i].getGaia() == source_id)
			{
				stars[i].getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RV, dyrs);
				return;
			}
		}
	}
}

template<class Derived, class StarType>
StelObjectP ZoneArrayImpl<Derived, StarType>::wrapStar(const StarType* s, int /*zoneIndex*/) const
{
	return self().createStelObj(s);
}

// ==================== Template instantiations ====================

template class SpecialZoneArray<Star1>;
template class SpecialZoneArray<Star2>;
template class SpecialZoneArray<Star3>;

template class DynamicZoneArray<Star1>;
template class DynamicZoneArray<Star2>;
template class DynamicZoneArray<Star3>;

// Explicitly instantiate ZoneArrayImpl for all combinations
template class ZoneArrayImpl<SpecialZoneArray<Star1>, Star1>;
template class ZoneArrayImpl<SpecialZoneArray<Star2>, Star2>;
template class ZoneArrayImpl<SpecialZoneArray<Star3>, Star3>;
template class ZoneArrayImpl<DynamicZoneArray<Star1>, Star1>;
template class ZoneArrayImpl<DynamicZoneArray<Star2>, Star2>;
template class ZoneArrayImpl<DynamicZoneArray<Star3>, Star3>;
