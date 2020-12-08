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
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGeodesicGrid.hpp"
#include "StelObject.hpp"
#include "StelPainter.hpp"

#include <QDebug>
#include <QFile>
#include <QDir>
#ifdef Q_OS_WIN
#include <io.h>
#include <Windows.h>
#endif


static unsigned int stel_bswap_32(unsigned int val)
{
	return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
	       (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
}

static const Vec3f north(0,0,1);

void ZoneArray::initTriangle(int index, const Vec3f &c0, const Vec3f &c1, const Vec3f &c2)
{
	// initialize center,axis0,axis1:
	ZoneData &z(zones[index]);
	z.center = c0+c1+c2;
	z.center.normalize();
	z.axis0 = north ^ z.center;
	z.axis0.normalize();
	z.axis1 = z.center ^ z.axis0;
	
	// Initialize star_position_scale. This scale is used to multiply stars position
	// encoded as integers so that it optimize precision over the triangle.
	// It has to be computed for each triangle because the relative orientation of the 2 axis is different for each triangle.
	float mu0,mu1,f,h;
	mu0 = (c0-z.center)*z.axis0;
	mu1 = (c0-z.center)*z.axis1;
	f = 1.f/std::sqrt(1.f-mu0*mu0-mu1*mu1);
	h = std::fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = std::fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
	mu0 = (c1-z.center)*z.axis0;
	mu1 = (c1-z.center)*z.axis1;
	f = 1.f/std::sqrt(1.f-mu0*mu0-mu1*mu1);
	h = std::fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = std::fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
	mu0 = (c2-z.center)*z.axis0;
	mu1 = (c2-z.center)*z.axis1;
	f = 1.f/std::sqrt(1.f-mu0*mu0-mu1*mu1);
	h = std::fabs(mu0)*f;
	if (star_position_scale < h) star_position_scale = h;
	h = std::fabs(mu1)*f;
	if (star_position_scale < h) star_position_scale = h;
}

static inline int ReadInt(QFile& file, unsigned int &x)
{
	const int rval = (4 == file.read(reinterpret_cast<char*>(&x), 4)) ? 0 : -1;
	return rval;
}

#if (!defined(__GNUC__))
#ifndef _MSC_BUILD
#warning Star catalogue loading has only been tested with gcc
#endif
#endif

ZoneArray* ZoneArray::create(const QString& catalogFilePath, bool use_mmap)
{
	QString dbStr; // for debugging output.
	QFile* file = new QFile(catalogFilePath);
	if (!file->open(QIODevice::ReadOnly))
	{
		qWarning() << "Error while loading " << QDir::toNativeSeparators(catalogFilePath) << ": failed to open file.";
		return Q_NULLPTR;
	}
	dbStr = "Loading " + QDir::toNativeSeparators(catalogFilePath) + ": ";
	unsigned int magic,major,minor,type,level,mag_min,mag_range,mag_steps;
	if (ReadInt(*file,magic) < 0 ||
			ReadInt(*file,type) < 0 ||
			ReadInt(*file,major) < 0 ||
			ReadInt(*file,minor) < 0 ||
			ReadInt(*file,level) < 0 ||
			ReadInt(*file,mag_min) < 0 ||
			ReadInt(*file,mag_range) < 0 ||
			ReadInt(*file,mag_steps) < 0)
	{
		dbStr += "error - file format is bad.";
		qDebug() << dbStr;
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
			dbStr += "before mmap loading";
			qWarning() << dbStr;
			use_mmap = false;
			qWarning() << "Revert to not using mmmap";
			//return 0;
		}
		dbStr += "byteswap ";
		type = stel_bswap_32(type);
		major = stel_bswap_32(major);
		minor = stel_bswap_32(minor);
		level = stel_bswap_32(level);
		mag_min = stel_bswap_32(mag_min);
		mag_range = stel_bswap_32(mag_range);
		mag_steps = stel_bswap_32(mag_steps);
	}
	else if (magic == FILE_MAGIC)
	{
		// ok, FILE_MAGIC
#if (!defined(__GNUC__) && !defined(_MSC_BUILD))
		if (use_mmap)
		{
			// mmap only with gcc:
			dbStr += "warning - you must convert catalogue to native format before mmap loading";
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
		dbStr += "error - not a catalogue file.";
		qDebug() << dbStr;
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
				rval = new HipZoneArray(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min), static_cast<int>(mag_range), static_cast<int>(mag_steps));
			}
			break;
		case 1:
			if (major > MAX_MAJOR_FILE_VERSION)
			{
				dbStr += "warning - unsupported version ";
			}
			else
			{
				rval = new SpecialZoneArray<Star2>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min), static_cast<int>(mag_range), static_cast<int>(mag_steps));
			}
			break;
		case 2:
			if (major > MAX_MAJOR_FILE_VERSION)
			{
				dbStr += "warning - unsupported version ";
			}
			else
			{
				rval = new SpecialZoneArray<Star3>(file, byte_swap, use_mmap, static_cast<int>(level), static_cast<int>(mag_min), static_cast<int>(mag_range), static_cast<int>(mag_steps));
			}
			break;
		default:
			dbStr += "error - bad file type ";
			break;
	}
	if (rval && rval->isInitialized())
	{
		dbStr += QString("%1").arg(rval->getNrOfStars());
		qDebug() << dbStr;
	}
	else
	{
		dbStr += " - initialization failed";
		qDebug() << dbStr;
		if (rval)
		{
			delete rval;
			rval = Q_NULLPTR;
		}
	}
	return rval;
}

ZoneArray::ZoneArray(const QString& fname, QFile* file, int level, int mag_min,
			 int mag_range, int mag_steps)
			: fname(fname), level(level), mag_min(mag_min),
			  mag_range(mag_range), mag_steps(mag_steps),
			  star_position_scale(0.0), nr_of_stars(0), zones(Q_NULLPTR), file(file)
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
	float i = 0.f;
	i += 1.f;
	while (size > 0)
	{
		const int to_read = (part_size < size) ? part_size : static_cast<int>(size);
		const int read_rc = static_cast<int>(file.read(static_cast<char*>(data), to_read));
		if (read_rc != to_read) return false;
		size -= read_rc;
		data = (static_cast<char*>(data)) + read_rc;
		i += 1.f;
	}
	return true;
}

void HipZoneArray::updateHipIndex(HipIndexStruct hipIndex[]) const
{
	for (const SpecialZoneData<Star1> *z=getZones()+(nr_of_zones-1);z>=getZones();z--)
	{
		for (const Star1 *s = z->getStars()+z->size-1;s>=z->getStars();s--)
		{
			const int hip = s->getHip();
			if (hip < 0 || NR_OF_HIP < hip)
			{
				qDebug() << "ERROR: HipZoneArray::updateHipIndex: invalid HIP number:" << hip;
				exit(1);
			}
			if (hip != 0)
			{
				hipIndex[hip].a = this;
				hipIndex[hip].z = z;
				hipIndex[hip].s = s;
			}
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::scaleAxis()
{
	star_position_scale /= Star::MaxPosVal;
	for (ZoneData *z=zones+(nr_of_zones-1);z>=zones;z--)
	{
		z->axis0 *= star_position_scale;
		z->axis1 *= star_position_scale;
	}
}

template<class Star>
SpecialZoneArray<Star>::SpecialZoneArray(QFile* file, bool byte_swap,bool use_mmap,
					 int level, int mag_min, int mag_range, int mag_steps)
		: ZoneArray(file->fileName(), file, level, mag_min, mag_range, mag_steps),
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
		// GZ: Some diagnostics to understand the undocumented vars around mag.
		// qDebug() << "SpecialZoneArray: mag_min=" << mag_min << ", mag_steps=" << mag_steps << ", mag_range=" << mag_range ;
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
				  int limitMagIndex, StelCore* core, int maxMagStarName, float names_brightness, bool designationUsage,
				  const QVector<SphericalCap> &boundingCaps) const
{
	StelSkyDrawer* drawer = core->getSkyDrawer();
	Vec3f vf;
	static const double d2000 = 2451545.0;
	const float movementFactor = static_cast<float>((M_PI/180.)*(0.0001/3600.) * ((core->getJDE()-d2000)/365.25) / static_cast<double>(star_position_scale));

	const Extinction& extinction=core->getSkyDrawer()->getExtinction();
	const bool withExtinction=drawer->getFlagHasAtmosphere() && extinction.getExtinctionCoefficient()>=0.01f;
	const float k = 0.001f*mag_range/mag_steps; // from StarMgr.cpp line 654
	
	// Allow artificial cutoff:
	// find the (integer) mag at which is just bright enough to be drawn.
	int cutoffMagStep=limitMagIndex;
	if (drawer->getFlagStarMagnitudeLimit())
	{
		cutoffMagStep = (static_cast<int>(drawer->getCustomStarMagnitudeLimit()*1000.0) - mag_min)*mag_steps/mag_range;
		if (cutoffMagStep>limitMagIndex)
			cutoffMagStep = limitMagIndex;
	}
	Q_ASSERT(cutoffMagStep<RCMAG_TABLE_SIZE);
    
	// Go through all stars, which are sorted by magnitude (bright stars first)
	const SpecialZoneData<Star>* zoneToDraw = getZones() + index;
	const Star* lastStar = zoneToDraw->getStars() + zoneToDraw->size;
	for (const Star* s=zoneToDraw->getStars();s<lastStar;++s)
	{
		// Artifical cutoff per magnitude
		if (s->getMag() > cutoffMagStep)
			break;
    
		// Because of the test above, the star should always be visible from this point.
		
		// Array of 2 numbers containing radius and magnitude
		const RCMag* tmpRcmag = &rcmag_table[s->getMag()];
		
		// Get the star position from the array
		s->getJ2000Pos(zoneToDraw, movementFactor, vf);
		
		// If the star zone is not strictly contained inside the viewport, eliminate from the 
		// beginning the stars actually outside viewport.
		if (!isInsideViewport)
		{
			vf.normalize();
			bool isVisible = true;
			for (const auto& cap : boundingCaps)
			{
				if (!cap.contains(vf))
				{
					isVisible = false;
					continue;
				}
			}
			if (!isVisible)
				continue;
		}

		int extinctedMagIndex = s->getMag();
		float twinkleFactor=1.0f; // allow height-dependent twinkle.
		if (withExtinction)
		{
			Vec3f altAz(vf);
			altAz.normalize();
			core->j2000ToAltAzInPlaceNoRefraction(&altAz);
			float extMagShift=0.0f;
			extinction.forward(altAz, &extMagShift);
			extinctedMagIndex = s->getMag() + static_cast<int>(extMagShift/k);
			if (extinctedMagIndex >= cutoffMagStep || extinctedMagIndex<0) // i.e., if extincted it is dimmer than cutoff or extinctedMagIndex is negative (missing star catalog), so remove
				continue;
			tmpRcmag = &rcmag_table[extinctedMagIndex];
			twinkleFactor=qMin(1.0f, 1.0f-0.9f*altAz[2]); // suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
		}

		if (drawer->drawPointSource(sPainter, vf, *tmpRcmag, s->getBVIndex(), !isInsideViewport, twinkleFactor) && s->hasName() && extinctedMagIndex < maxMagStarName && s->hasComponentID()<=1)
		{
			const float offset = tmpRcmag->radius*0.7f;
			const Vec3f colorr = StelSkyDrawer::indexToColor(s->getBVIndex())*0.75f;
			sPainter->setColor(colorr,names_brightness);
			sPainter->drawText(vf.toVec3d(), designationUsage ? s->getDesignation() : s->getNameI18n(), 0, offset, offset, false);
		}
	}
}

template<class Star>
void SpecialZoneArray<Star>::searchAround(const StelCore* core, int index, const Vec3d &v, double cosLimFov,
					  QList<StelObjectP > &result)
{
	static const double d2000 = 2451545.0;
	const double movementFactor = (M_PI/180.)*(0.0001/3600.) * ((core->getJDE()-d2000)/365.25)/ static_cast<double>(star_position_scale);
	const SpecialZoneData<Star> *const z = getZones()+index;
	Vec3f tmp;
	Vec3f vf = v.toVec3f();
	for (const Star* s=z->getStars();s<z->getStars()+z->size;++s)
	{
		s->getJ2000Pos(z,static_cast<float>(movementFactor), tmp);
		tmp.normalize();
		if (tmp*vf >= static_cast<float>(cosLimFov))
		{
			// TODO: do not select stars that are too faint to display
			result.push_back(s->createStelObject(this,z));
		}
	}
}

