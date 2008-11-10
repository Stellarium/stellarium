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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <QDebug>
#include <QFile>

#include "ZoneArray.hpp"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "GeodesicGrid.hpp"
#include "StelObject.hpp"


namespace BigStarCatalogExtension {

static const Vec3d north(0,0,1);

void ZoneArray::initTriangle(int index,
                             const Vec3d &c0,
                             const Vec3d &c1,
                             const Vec3d &c2) {
    // initialize center,axis0,axis1:
  ZoneData &z(zones[index]);
  z.center = c0+c1+c2;
  z.center.normalize();
  z.axis0 = north ^ z.center;
  z.axis0.normalize();
  z.axis1 = z.center ^ z.axis0;
    // initialize star_position_scale:
  double mu0,mu1,f,h;
  mu0 = (c0-z.center)*z.axis0;
  mu1 = (c0-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (star_position_scale < h) star_position_scale = h;
  h = fabs(mu1)*f;
  if (star_position_scale < h) star_position_scale = h;
  mu0 = (c1-z.center)*z.axis0;
  mu1 = (c1-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (star_position_scale < h) star_position_scale = h;
  h = fabs(mu1)*f;
  if (star_position_scale < h) star_position_scale = h;
  mu0 = (c2-z.center)*z.axis0;
  mu1 = (c2-z.center)*z.axis1;
  f = 1.0/sqrt(1.0-mu0*mu0-mu1*mu1);
  h = fabs(mu0)*f;
  if (star_position_scale < h) star_position_scale = h;
  h = fabs(mu1)*f;
  if (star_position_scale < h) star_position_scale = h;
}



static inline
int ReadInt(FILE *f,unsigned int &x) {
  const int rval = (4 == fread(&x,1,4,f)) ? 0 : -1;
  return rval;
}


#define FILE_MAGIC 0x835f040a
#define FILE_MAGIC_OTHER_ENDIAN 0x0a045f83
#define FILE_MAGIC_NATIVE 0x835f040b
#define MAX_MAJOR_FILE_VERSION 0

#if (!defined(__GNUC__))
#warning Star catalogue loading has only been tested with gcc
#endif

ZoneArray *ZoneArray::create(const StarMgr &hip_star_mgr,
                             const QString& extended_file_name,
                             LoadingBar &lb) {
  QString fname(extended_file_name);
  QString dbStr; // for debugging output.
  bool use_mmap = false;
  if (fname.contains("mmap:")) {
    fname.remove(0,5);
    use_mmap = true;
  }
  try {
    fname = StelApp::getInstance().getFileMgr().findFile("stars/default/"+fname);
  } catch (std::runtime_error &e) {
    qWarning() << "Loading" << extended_file_name << e.what();
    return 0;
  }
  FILE *f = fopen(QFile::encodeName(fname).constData(),"rb");
  if (f == 0) {
    qWarning() << "Loading" << extended_file_name << "failed to open file.";
    return 0;
  }
  dbStr = "Loading \"" + extended_file_name + "\": ";
  unsigned int magic,major,minor,type,level,mag_min,mag_range,mag_steps;
  if (ReadInt(f,magic) < 0 ||
      ReadInt(f,type) < 0 ||
      ReadInt(f,major) < 0 ||
      ReadInt(f,minor) < 0 ||
      ReadInt(f,level) < 0 ||
      ReadInt(f,mag_min) < 0 ||
      ReadInt(f,mag_range) < 0 ||
      ReadInt(f,mag_steps) < 0) {
    dbStr += "error - file format is bad.";
    qWarning() << dbStr;
    return 0;
  }
  const bool byte_swap = (magic == FILE_MAGIC_OTHER_ENDIAN);
  if (byte_swap) {
      // ok, FILE_MAGIC_OTHER_ENDIAN, must swap
    if (use_mmap) {
      dbStr += "warning - must convert catalogue ";
#if (!defined(__GNUC__))
      dbStr += "to native format ";
#endif
      dbStr += "before mmap loading";
      qWarning() << dbStr;
      return 0;
    }
    dbStr += "byteswap ";
    type = bswap_32(type);
    major = bswap_32(major);
    minor = bswap_32(minor);
    level = bswap_32(level);
    mag_min = bswap_32(mag_min);
    mag_range = bswap_32(mag_range);
    mag_steps = bswap_32(mag_steps);
  } else if (magic == FILE_MAGIC) {
      // ok, FILE_MAGIC
#if (!defined(__GNUC__))
    if (use_mmap) {
      // mmap only with gcc:
      dbStr += "warning - you must convert catalogue " 
            += "to native format before mmap loading";
      qWarning(qPrintable(dbStr));

      return 0;
    }
#endif
  } else if (magic == FILE_MAGIC_NATIVE) {
      // ok, will work for any architecture and any compiler
  } else {
    dbStr += "error - not a catalogue file.";
    qWarning() << dbStr;
    return 0;
  }
  ZoneArray *rval = 0;
  dbStr += QString("%1_%2v%3_%4; ").arg(level)
                                 .arg(type)
                                 .arg(major)
                                 .arg(minor);
  switch (type) {
    case 0:
      if (major > MAX_MAJOR_FILE_VERSION) {
        dbStr += "warning - unsupported version ";
      } else {
          // When this assertion fails you must redefine Star1
          // for your compiler.
          // Because your compiler does not pack the data,
          // which is crucial for this application.
        Q_ASSERT(sizeof(Star1) == 28);
        rval = new ZoneArray1(f,byte_swap,use_mmap,lb,hip_star_mgr,level,
                              mag_min,mag_range,mag_steps);
        if (rval == 0) {
          dbStr += "error - no memory ";
        }
      }
      break;
    case 1:
      if (major > MAX_MAJOR_FILE_VERSION) {
        dbStr += "warning - unsupported version ";
      } else {
          // When this assertion fails you must redefine Star2
          // for your compiler.
          // Because your compiler does not pack the data,
          // which is crucial for this application.
        Q_ASSERT(sizeof(Star2) == 10);
        rval = new SpecialZoneArray<Star2>(f,byte_swap,use_mmap,lb,hip_star_mgr,
                                           level,
                                           mag_min,mag_range,mag_steps);
        if (rval == 0) {
          dbStr += "error - no memory ";
        }
      }
      break;
    case 2:
      if (major > MAX_MAJOR_FILE_VERSION) {
        dbStr += "warning - unsupported version ";
      } else {
          // When this assertion fails you must redefine Star3
          // for your compiler.
          // Because your compiler does not pack the data,
          // which is crucial for this application.
        Q_ASSERT(sizeof(Star3) == 6);
        rval = new SpecialZoneArray<Star3>(f,byte_swap,use_mmap,lb,hip_star_mgr,
                                           level,
                                           mag_min,mag_range,mag_steps);
        if (rval == 0) {
          dbStr += "error - no memory ";
        }
      }
      break;
    default:
      dbStr += "error - bad file type ";
      break;
  }
  if (rval && rval->isInitialized()) {
    dbStr += QString("%1").arg(rval->getNrOfStars());
    qDebug() << dbStr;
//    rval->generateNativeDebugFile((fname+".debug").c_str());
  } else {
    dbStr += " - initialization failed";
    qWarning() << dbStr;
    if (rval) {
      delete rval;
      rval = 0;
    }
  }
  fclose(f);
  return rval;
}


ZoneArray::ZoneArray(const StarMgr &hip_star_mgr,int level,
                     int mag_min,int mag_range,int mag_steps)
          :level(level),
           mag_min(mag_min),mag_range(mag_range),mag_steps(mag_steps),
           star_position_scale(0.0), //IntToDouble(scale_int)/Star::MaxPosVal
           hip_star_mgr(hip_star_mgr),
           zones(0) {
  nr_of_zones = GeodesicGrid::nrOfZones(level);
  nr_of_stars = 0;
}

bool ZoneArray::readFileWithLoadingBar(FILE *f,void *data,size_t size,
                                       LoadingBar &lb) {
  int parts = 256;
  size_t part_size = (size + (parts>>1)) / parts;
  if (part_size < 64*1024) {
    part_size = 64*1024;
    parts = (size + (part_size>>1)) / part_size;
  }
  float i = 0.f;
  lb.Draw(i / parts);
  i += 1.f;
  while (size > 0) {
    const size_t to_read = (part_size < size) ? part_size : size;
    const size_t read_rc = fread(data,1,to_read,f);
    if (read_rc != to_read) return false;
    size -= read_rc;
    data = ((char*)data) + read_rc;
    lb.Draw(i / parts);
    i += 1.f;
  }
  return true;
}

void ZoneArray1::updateHipIndex(HipIndexStruct hipIndex[]) const {
  for (const SpecialZoneData<Star1> *z=getZones()+(nr_of_zones-1);
       z>=getZones();z--) {
    for (const Star1 *s = z->getStars()+z->size-1;s>=z->getStars();s--) {
      const int hip = s->hip;
      if (hip < 0 || NR_OF_HIP < hip) {
        qDebug() << "ERROR: ZoneArray1::updateHipIndex: invalid HP number:"
                 << hip;
        exit(1);
      }
      if (hip != 0) {
        hipIndex[hip].a = this;
        hipIndex[hip].z = z;
        hipIndex[hip].s = s;
      }
    }
  }
}

} // namespace BigStarCatalogExtension

