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
//! @param mag_min the catalog's lower magnitude bound in millimag
//! @param limitMagIndex the maximum valid index in the rcmag table
//! @param drawer the sky drawer
//! @param[out] cutoffMagStep the computed cutoff step
//! @param[out] cutoffMag the computed precise magnitude cutoff in millimag
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
//! @param magIndex the original magnitude index
//! @param cutoffMagStep the maximum allowed magnitude index
//! @param v the star direction in J2000 frame
//! @param core the core
//! @param extinction the extinction model
//! @param[out] extinctedMagIndex the updated magnitude index
//! @param[out] tmpRcmag pointer into the rcmag table
//! @param[out] twinkleFactor height-dependent twinkle factor
//! @return false if the star is too faint after extinction and should be skipped
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
	if (extinctedMagIndex >= cutoffMagStep || extinctedMagIndex < 0) // too faint or missing catalog
		return false;
	tmpRcmag += (extinctedMagIndex - magIndex);
	twinkleFactor = qMin(1.f, 1.f - 0.9f * static_cast<float>(altAz[2])); // suppress twinkling at high altitudes
	return true;
}

//! Apply annual aberration to a J2000 direction.
static void applyAberration(Vec3d& v, const StelCore* core)
{
	v += core->getAberrationVec(core->getJDE());
	v.normalize();
}

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

/*
    // Create a union to access the float as an unsigned int
    union {
        float f;
        unsigned int i;
    } u;

    // Assign the float value to the union
    u.i = val;

    // Perform the byte swap on the integer representation
    u.i = (((u.i) & 0xff000000) >> 24) | (((u.i) & 0x00ff0000) >>  8) |
          (((u.i) & 0x0000ff00) <<  8) | (((u.i) & 0x000000ff) << 24);

    // Return the float value from the union
    return u.f;
*/
}

static const Vec3f north(0,0,1);

void ZoneArray::initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2)
{
	// shut compiler up about unused variables
	(void)index;
	(void)c0;
	(void)c1;
	(void)c2;

	// we don't need them anymore for the new star catalogs
	// initialize zone:
	// ZoneData &z(zones[index]);
	// z.center = c0+c1+c2;
	// z.center.normalize();
	// z.axis0 = north ^ z.center;
	// z.axis0.normalize();
	// z.axis1 = z.center ^ z.axis0;
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

ZoneArray* ZoneArray::create(const QString& catalogFilePath, bool use_mmap, bool use_dynamic)
{
	QString dbStr; // for debugging output.
	QFile* file = new QFile(catalogFilePath);
	if (!file->open(QIODevice::ReadOnly))
	{
		qWarning().noquote() << "Error while loading" << QDir::toNativeSeparators(catalogFilePath) << ": failed to open file.";
		return Q_NULLPTR;
	}
	dbStr = "Loading star catalog: " + QDir::toNativeSeparators(catalogFilePath) + " - ";
	unsigned int magic,major,minor,type,level,mag_min;
	float epochJD;
	if (ReadInt(*file,magic) < 0 ||
	    ReadInt(*file,type) < 0 ||
	    ReadInt(*file,major) < 0 ||
	    ReadInt(*file,minor) < 0 ||
	    ReadInt(*file,level) < 0 ||
	    ReadInt(*file,mag_min) < 0 ||
	    ReadFloat(*file,epochJD) < 0)
	{
		dbStr += "error - file format is bad.";
		qDebug().noquote() << dbStr;
		return Q_NULLPTR;
	}
	const bool byte_swap = (magic == FILE_MAGIC_OTHER_ENDIAN);
	if (byte_swap)
	{
		// ok, FILE_MAGIC_OTHER_ENDIAN, must swap
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
		// ok, FILE_MAGIC
#if (!defined(__GNUC__) && !defined(_MSC_BUILD))
		if (use_mmap)
		{
			// mmap only with gcc:
			dbStr += "warning - you must convert catalogue to native format before mmap loading ";
			qDebug(qPrintable(dbStr));

			return 0;
		}
#endif
	}
	else if (magic == FILE_MAGIC_NATIVE)
	{
		// ok, will work for any architecture and any compiler
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
	ZoneArray *rval = Q_NULLPTR;
	dbStr += QString("%1_%2v%3_%4; ").arg(level).arg(type).arg(major).arg(minor);

	switch (type)
	{
		case 0:
			if (major > MAX_MAJOR_FILE_VERSION)
			{
				dbStr += "warning - unsupported version ";
			}
			else
			{
				rval = new HipZoneArray(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			}
			break;
		case 1:
			if (major > MAX_MAJOR_FILE_VERSION)
			{
				dbStr += "warning - unsupported version ";
			}
			else if (use_dynamic)
			{
				rval = new DynamicZoneArray<Star2>(catalogFilePath, file, static_cast<int>(level), static_cast<int>(mag_min), byte_swap);
			}
			else
			{
				rval = new SpecialZoneArray<Star2>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			}
			break;
		case 2:
			if (major > MAX_MAJOR_FILE_VERSION)
			{
				dbStr += "warning - unsupported version ";
			}
			else if (use_dynamic)
			{
				rval = new DynamicZoneArray<Star3>(catalogFilePath, file, static_cast<int>(level), static_cast<int>(mag_min), byte_swap);
			}
			else
			{
				rval = new SpecialZoneArray<Star3>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min));
			}
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
			: fname(fname), level(level), mag_min(mag_min), nr_of_stars(0), zones(Q_NULLPTR), file(file)
{
	nr_of_zones = static_cast<unsigned int>(StelGeodesicGrid::nrOfZones(level));
}

bool ZoneArray::readFile(QFile& file, void *data, qint64 size)
{
	int parts = 256;
	int part_size = static_cast<int>((size + (parts>>1)) / parts);
	if (part_size < 64*1024)
	{
		part_size = 64*1024;
	}
	while (size > 0)
	{
		const int to_read = (part_size < size) ? part_size : static_cast<int>(size);
		const int read_rc = static_cast<int>(file.read(static_cast<char*>(data), to_read));
		if (read_rc != to_read) return false;
		size -= read_rc;
		data = (static_cast<char*>(data)) + read_rc;
	}
	return true;
}

void HipZoneArray::updateHipIndex(HipIndexStruct hipIndex[]) const
{
	for (const SpecialZoneData<Star1> *z=getZones()+(nr_of_zones-1);z>=getZones();z--)
	{
		for (const Star1 *s = z->getStars()+z->size-1;s>=z->getStars();s--)
		{
			const StarId hip = s->getHip();
			if (hip > NR_OF_HIP)
			{
				qDebug() << "ERROR: HipZoneArray::updateHipIndex: invalid HIP number:" << hip;
				exit(1);
			}
			if (hip != 0)
			{	
				// check if a star is there already
				if (hipIndex[hip].a)
				{
					// check if componentid is smaller or higher, we want smaller one (the main one)
					// otherwise there can be case where Sirius B is stored as main star but not Sirius A
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

template<class Star>
SpecialZoneArray<Star>::SpecialZoneArray(QFile* file, bool byte_swap,bool use_mmap,
					 int level, int mag_min)
		: ZoneArray(file->fileName(), file, level, mag_min),
		  stars(Q_NULLPTR), mmap_start(Q_NULLPTR)
{
	if (nr_of_zones > 0)
	{
		zones = new SpecialZoneData<Star>[nr_of_zones];

		unsigned int *zone_size = new unsigned int[nr_of_zones];
		if (static_cast<qint64>(sizeof(unsigned int)*nr_of_zones) != file->read(reinterpret_cast<char*>(zone_size), sizeof(unsigned int)*nr_of_zones))
		{
			qDebug() << "Error reading zones from catalog:"
				 << file->fileName();
			delete[] getZones();
			zones = Q_NULLPTR;
			nr_of_zones = 0;
		}
		else
		{
			const unsigned int *tmp = zone_size;
			for (unsigned int z=0;z<nr_of_zones;z++,tmp++)
			{
				const unsigned int tmp_spu_int32 = byte_swap?stel_bswap_32(*tmp):*tmp;
				nr_of_stars += tmp_spu_int32;
				getZones()[z].size = static_cast<int>(tmp_spu_int32);
			}
			// last one is always the global zone
			getZones()[nr_of_zones-1].isGlobal = true;
		}
		// delete zone_size before allocating stars
		// in order to avoid memory fragmentation:
		delete[] zone_size;

		if (nr_of_stars == 0)
		{
			// no stars ?
			if (zones) delete[] getZones();
			zones = Q_NULLPTR;
			nr_of_zones = 0;
		}
		else
		{
			if (use_mmap)
			{
				mmap_start = file->map(file->pos(), sizeof(Star)*nr_of_stars);
				if (mmap_start == Q_NULLPTR)
				{
					qDebug() << "ERROR: SpecialZoneArray(" << level
						 << ")::SpecialZoneArray: QFile(" << file->fileName()
						 << ".map(" << file->pos()
						 << ',' << sizeof(Star)*nr_of_stars
						 << ") failed: " << file->errorString();
					stars = Q_NULLPTR;
					nr_of_stars = 0;
					delete[] getZones();
					zones = Q_NULLPTR;
					nr_of_zones = 0;
				}
				else
				{
					stars = reinterpret_cast<Star*>(mmap_start);
					Star *s = stars;
					for (unsigned int z=0;z<nr_of_zones;z++)
					{
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
				}
				file->close();
			}
			else
			{
				stars = new Star[nr_of_stars];
				if (!readFile(*file,stars,sizeof(Star)*nr_of_stars))
				{
					delete[] stars;
					stars = Q_NULLPTR;
					nr_of_stars = 0;
					delete[] getZones();
					zones = Q_NULLPTR;
					nr_of_zones = 0;
				}
				else
				{
					Star *s = stars;
					for (unsigned int z=0;z<nr_of_zones;z++)
					{
						getZones()[z].stars = s;
						s += getZones()[z].size;
					}
				}
				file->close();
			}
		}
	}
}

template<class Star>
SpecialZoneArray<Star>::~SpecialZoneArray(void)
{
	if (stars)
	{
		if (mmap_start != Q_NULLPTR)
		{
			file->unmap(mmap_start);
		}
		else
		{
			delete[] stars;
		}
		delete file;
		stars = Q_NULLPTR;
	}
	if (zones)
	{
		delete[] getZones();
		zones = Q_NULLPTR;
	}
	nr_of_zones = 0;
	nr_of_stars = 0;
}

template<class Star>
void SpecialZoneArray<Star>::draw(StelPainter* sPainter, int index, bool isInsideViewport, const RCMag* rcmag_table,
				  int limitMagIndex, StelCore* core, int maxMagStarName, float names_brightness,
				  const QVector<SphericalCap> &boundingCaps,
				  const bool withAberration, const Vec3d vel, const double withParallax, const Vec3d diffPos,
				  const bool withCommonNameI18n) const
{
	StelSkyDrawer* drawer = core->getSkyDrawer();
	Vec3d v;
	const float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;

	const Extinction& extinction=core->getSkyDrawer()->getExtinction();
	const bool withExtinction=drawer->getFlagHasAtmosphere() && extinction.getExtinctionCoefficient()>=0.01f;

	// Allow artificial cutoff:
	// find the (integer) mag at which is just bright enough to be drawn.
	int cutoffMagStep;
	float cutoffMag;
	computeMagnitudeCutoff(mag_min, limitMagIndex, drawer, cutoffMagStep, cutoffMag);

	Q_ASSERT_X(cutoffMagStep<=RCMAG_TABLE_SIZE, "ZoneArray.cpp",
		   QString("RCMAG_TABLE_SIZE: %1, cutoffmagStep: %2").arg(QString::number(RCMAG_TABLE_SIZE), QString::number(cutoffMagStep)).toLatin1());
    
	// Go through all stars, which are sorted by magnitude (bright stars first)
	const SpecialZoneData<Star>* zoneToDraw = getZones() + index;
	const Star* lastStar = zoneToDraw->getStars() + zoneToDraw->size;
	bool globalzone;
	for (const Star* s=zoneToDraw->getStars();s<lastStar;++s)
	{
		// check if this is a global zone
		globalzone = zoneToDraw->isGlobal;
		float starMag = s->getMag();
		int magIndex = static_cast<int>((starMag - (mag_min - 7000.)) * 0.02);  // 1 / (50 milli-mag)

		// first part is check for Star1 and is global zone, so to keep looping for long-range prediction
		// second part is old behavior, to skip stars below you that are too faint to display for Star2 and Star3
		if ((magIndex > cutoffMagStep) || (starMag > cutoffMag)) { // should always use catalog magnitude, otherwise will mess up the order
			if (fabs(dyrs) <= 5000. || !s->isVIP() || !globalzone)  // if any of these true, we should always break
				break;
		}
		// Because of the test above, the star should always be visible from this point.
		
		// only recompute if has time dependence
		bool recomputeMag = (s->getPreciseAstrometricFlag());
		double Plx = s->getPlx();
		if (recomputeMag) { 
			// don't do full solution, can be very slow, just estimate here	
			// estimate parallax from radial velocity and total proper motion
			double vr = s->getRV();
			Vec3d pmvec0(s->getDx0(), s->getDx1(), s->getDx2());
			pmvec0 = pmvec0 * MAS2RAD;
			double pmr0 = vr * Plx / (AU / JYEAR_SECONDS) * MAS2RAD;
			double pmtotsqr =  (pmvec0[0] * pmvec0[0] + pmvec0[1] * pmvec0[1] + pmvec0[2] * pmvec0[2]);
			double f = 1. / sqrt(1. + 2. * pmr0 * dyrs + (pmtotsqr + pmr0*pmr0)*dyrs*dyrs);
			Plx *= f;
			float magOffset = 5.f * log10(1/f);
			starMag += magOffset * 1000.;
			// if we reach here we might as well compute the position too
			Vec3d r(s->getX0(), s->getX1(), s->getX2());
			Vec3d u = (r * (1. + pmr0 * dyrs) + pmvec0 * dyrs) * f;
			v.set(u[0], u[1], u[2]);
		}
		// recompute magIndex with the new magnitude
		magIndex = static_cast<int>((starMag - (mag_min - 7000.)) * 0.02);  // 1 / (50 milli-mag)

		if ((magIndex > cutoffMagStep) || (starMag > cutoffMag)) {  // check again with the new magIndex
				continue;  // allow continue for other star that might became bright enough in the future
		}
		// Array of 2 numbers containing radius and magnitude
		const RCMag* tmpRcmag = &rcmag_table[magIndex];
		
		// Get the star position from the array, only do it if not already computed
		// put it here because potentially cutoffMagStep bigger than magIndex and no computation needed
		if (!recomputeMag) {
			s->getJ2000Pos(dyrs, v);
		}

		// in case it is in a binary system
		s->getBinaryOrbit(core->getJDE(), v);

		if (withParallax) {
			s->getPlxEffect(withParallax * Plx, v, diffPos);
			v.normalize();
		}

		// Aberration: vf contains Equatorial J2000 position.
		if (withAberration)
			applyAberration(v, core);
		
		// If the star zone is not strictly contained inside the viewport, eliminate from the 
		// beginning the stars actually outside viewport.
		if (!isInsideViewport && !isVisibleInCaps(v, boundingCaps))
			continue;

		int extinctedMagIndex = magIndex;
		float twinkleFactor=1.0f; // allow height-dependent twinkle.
		if (withExtinction && !applyExtinction(magIndex, cutoffMagStep, v, core, extinction, extinctedMagIndex, tmpRcmag, twinkleFactor))
			continue;

		if (drawer->drawPointSource(sPainter, v, *tmpRcmag, s->getBVIndex(), !isInsideViewport, twinkleFactor) && core->getFlagClearSky() && s->hasName() && extinctedMagIndex < maxMagStarName && s->hasComponentID()<=1)
		{
			const float offset = tmpRcmag->radius*0.7f;
			const Vec3f color = StelSkyDrawer::indexToColor(s->getBVIndex())*0.75f;
			sPainter->setColor(color, names_brightness);
			sPainter->drawText(v, s->getScreenNameI18n(withCommonNameI18n), 0, offset, offset, false);
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::searchAround(const StelCore* core, int index, const Vec3d &v, const double withParallax, const Vec3d diffPos, 
										  double cosLimFov, QList<StelObjectP > &result)
{
	const float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
	const SpecialZoneData<Star> *const z = getZones()+index;
	Vec3d tmp;
	double RA, DEC, pmra, pmdec, Plx, RadialVel;
	for (const Star* s=z->getStars();s<z->getStars()+z->size;++s)
	{
		s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);
		StelUtils::spheToRect(RA, DEC, tmp);
		// s->getJ2000Pos(dyrs, tmp);
		// in case it is in a binary system
		s->getBinaryOrbit(core->getJDE(), tmp);
		s->getPlxEffect(withParallax * Plx, tmp, diffPos);
		tmp.normalize();
		if (core->getUseAberration())
			applyAberration(tmp, core);
		if (tmp * v >= cosLimFov)
		{
			// TODO: do not select stars that are too faint to display
			result.push_back(s->createStelObject(this,z));
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::searchWithin(const StelCore* core, int index, const SphericalRegionP region, const double withParallax, const Vec3d diffPos, const bool hipOnly, const float maxMag,
						  QList<StelObjectP > &result) const
{
	if (hipOnly && level>3)
			return;
#ifndef NDEBUG
	qDebug() << "SpecialZoneArray<Star>::searchWithin(): Level" << level << "MagMin" << mag_min << "fname" << fname << "nr_of_zones" << nr_of_zones << "nr_of_stars" << nr_of_stars;
#endif
	const float dyrs = static_cast<float>(core->getJDE()-STAR_CATALOG_JDEPOCH)/365.25;
	const SpecialZoneData<Star> *const z = getZones()+index;
	const float maxMilliMag = 1000.f*maxMag;
	Vec3d tmp;
	double RA, DEC, pmra, pmdec, Plx, RadialVel;
	for (const Star* s=z->getStars();s<z->getStars()+z->size;++s)
	{
		if (hipOnly && s->getHip()==0)
		{
			continue;
		}

		s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RadialVel, dyrs);
		StelUtils::spheToRect(RA, DEC, tmp);
		// s->getJ2000Pos(dyrs, tmp);
		// in case it is in a binary system
		s->getBinaryOrbit(core->getJDE(), tmp);
		s->getPlxEffect(withParallax * Plx, tmp, diffPos);
		tmp.normalize();
		// TODO: Move vel into arg.list
		if (core->getUseAberration())
			applyAberration(tmp, core);
		// By trying, region is a SphericalPolygon. We are calling SphericalPolygon::contains(Vec3d)
		if (region->contains(tmp) && (s->getMag() < maxMilliMag) )
		{
#ifndef NDEBUG
			//qDebug() << "Region match: " <<  s->getHip() << s->getGaia()  << "(Index (Zone):" << index << ", Level="<< level << ")";
#endif
			result.push_back(s->createStelObject(this,z));
		}
	}
}


template<class Star>
StelObjectP SpecialZoneArray<Star>::searchGaiaID(int index, const StarId source_id, int &matched) const
{
	const SpecialZoneData<Star> *const z = getZones()+index;
	for (const Star* s=z->getStars();s<z->getStars()+z->size;++s)
	{
		if (s->getGaia() == source_id)
		{
			matched++;
			return s->createStelObject(this,z);
		}
	}
	return StelObjectP();
}

template<class Star>
// this class is written for the unit test
void SpecialZoneArray<Star>::searchGaiaIDepochPos(const StarId source_id,
                                                  float         dyrs,
                                                  double &      RA,
                                                  double &      DEC,
                                                  double &      Plx,
                                                  double &      pmra,
                                                  double &      pmdec,
                                                  double &      RV) const
{
   // loop through each zone in the level which is 20 * 4 ** level + 1 as index
   for (int i = 0; i < 20 * pow(4., (level)) + 1; i++) {
      // get the zone data
      const SpecialZoneData<Star> * const z = getZones() + i;
      // loop through the stars in the zone
      for (const Star * s = z->getStars(); s < z->getStars() + z->size; ++s) {
         // check if the star has the same Gaia ID
         if (s->getGaia() == source_id) {
            // get the J2000 position of the star
            s->getFull6DSolution(RA, DEC, Plx, pmra, pmdec, RV, dyrs);
            return;
         }
      }
   }
}

// ── DynamicZoneArray template implementations ────────────────────

template<class StarType>
DynamicZoneArray<StarType>::DynamicZoneArray(const QString& fname, QFile* file, int level, int mag_min, bool byte_swap)
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

	zoneCounts_ = reinterpret_cast<uint32_t*>(zoneTableMmapStart_);

	if (byte_swap)
	{
		for (unsigned int z = 0; z < nr_of_zones; ++z)
			zoneCounts_[z] = stel_bswap_32(zoneCounts_[z]);
	}

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
	(void)vel;
	(void)withParallax;
	(void)diffPos;

	if (index < 0 || static_cast<unsigned int>(index) >= nr_of_zones)
		return;

	StelSkyDrawer* drawer = core->getSkyDrawer();

	int cutoffMagStep;
	float cutoffMag;
	computeMagnitudeCutoff(mag_min, limitMagIndex, drawer, cutoffMagStep, cutoffMag);

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

	const uint32_t zoneSize = zoneCounts_[index];
	const bool globalzone = (static_cast<unsigned int>(index) == nr_of_zones - 1);

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		const StarType& s = stars[i];
		int mag = s.getMag();
		int magIndex = static_cast<int>((mag - (mag_min - 7000.)) * 0.02);

		if ((magIndex > cutoffMagStep) || (static_cast<float>(mag) > cutoffMag))
		{
			if (std::fabs(dyrs) <= 5000.f || !globalzone)
				break;
			continue;
		}

		drawStar(s, magIndex, dyrs, isInsideViewport, boundingCaps, withAberration, core,
			 drawer, extinction, withExtinction, rcmag_table, cutoffMagStep,
			 maxMagStarName, sPainter, names_brightness, withCommonNameI18n);
	}
}

template<class StarType>
bool DynamicZoneArray<StarType>::drawStar(const StarType& s, int magIndex, float dyrs,
					  bool isInsideViewport, const QVector<SphericalCap>& boundingCaps,
					  bool withAberration, StelCore* core,
					  StelSkyDrawer* drawer, const Extinction& extinction,
					  bool withExtinction, const RCMag* rcmag_table,
					  int cutoffMagStep, int maxMagStarName,
					  StelPainter* sPainter, float names_brightness,
					  bool withCommonNameI18n) const
{
	Vec3d v;
	s.getJ2000Pos(dyrs, v);

	if (withAberration)
		applyAberration(v, core);

	if (!isInsideViewport && !isVisibleInCaps(v, boundingCaps))
		return false;

	int extinctedMagIndex = magIndex;
	float twinkleFactor = 1.f;
	const RCMag* tmpRcmag = &rcmag_table[magIndex];

	if (withExtinction && !applyExtinction(magIndex, cutoffMagStep, v, core, extinction, extinctedMagIndex, tmpRcmag, twinkleFactor))
		return false;

	const bool drawn = drawer->drawPointSource(sPainter, v, *tmpRcmag, s.getBVIndex(),
						   !isInsideViewport, twinkleFactor);
	if (drawn && core->getFlagClearSky() && s.hasName() &&
	    extinctedMagIndex < maxMagStarName)
	{
		const float offset = tmpRcmag->radius * 0.7f;
		const Vec3f color = StelSkyDrawer::indexToColor(s.getBVIndex()) * 0.75f;
		sPainter->setColor(color, names_brightness);
		sPainter->drawText(v, s.getScreenNameI18n(withCommonNameI18n),
				   0, offset, offset, false);
	}
	return drawn;
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

	const uint32_t zoneSize = zoneCounts_[index];
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		stars[i].getJ2000Pos(dyrs, tmp);

		// in case it is in a binary system
		stars[i].getBinaryOrbit(core->getJDE(), tmp);

		stars[i].getPlxEffect(withParallax * stars[i].getPlx(), tmp, diffPos);
		tmp.normalize();

		if (core->getUseAberration())
			applyAberration(tmp, core);

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

	const uint32_t zoneSize = zoneCounts_[index];
	const float maxMilliMag = 1000.f * maxMag;
	const float dyrs = static_cast<float>(core->getJDE() - STAR_CATALOG_JDEPOCH) / 365.25f;
	Vec3d tmp;

	for (uint32_t i = 0; i < zoneSize; ++i)
	{
		if (stars[i].getMag() >= maxMilliMag)
			continue;

		stars[i].getJ2000Pos(dyrs, tmp);

		// in case it is in a binary system
		stars[i].getBinaryOrbit(core->getJDE(), tmp);

		stars[i].getPlxEffect(withParallax * stars[i].getPlx(), tmp, diffPos);
		tmp.normalize();

		if (core->getUseAberration())
			applyAberration(tmp, core);

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
