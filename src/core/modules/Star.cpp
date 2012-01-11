/*
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006, 2007
 *
 * Thanks go to Nigel Kerr for ideas and testing of BE/LE star repacking
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

#include <QDebug>
#include <QString>

#include "Star.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"

namespace BigStarCatalogExtension {

QString Star1::getNameI18n(void) const {
  if (hip) {
    const QString commonNameI18 = StarMgr::getCommonName(hip);
    if (!commonNameI18.isEmpty()) return commonNameI18;
    if (StarMgr::getFlagSciNames()) {
      const QString sciName = StarMgr::getSciName(hip);
      if (!sciName.isEmpty()) return sciName;
      return QString("HIP %1").arg(hip);
    }
  }
  return QString();
}

int Star1::hasComponentID(void) const {
  if (componentIds) {
     return componentIds;
  }
  return 0;
}

static
int UnpackBits(bool fromBe,const char *addr,int bits_begin,
               const int bits_size) {
  Q_ASSERT(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  int rval;
  if (fromBe) {
    rval = (int)((( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
                        ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[3])));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval <<= bits_begin;
    } else {
      rval <<= bits_begin;
      unsigned int rval_lo = (unsigned char)(addr[4]);
      rval_lo >>= (8-bits_begin);
      rval |= rval_lo;
    }
    if (bits_size < 32) rval >>= (32-bits_size);
  } else {
    rval = (int)((( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
                        ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[0])));
    if (bits_end <= 32) {
      if (bits_end < 32) rval <<= (32-bits_end);
      if (bits_size < 32) rval >>= (32-bits_size);
    } else {
      int rval_hi = addr[4];
      rval_hi <<= (64-bits_end);
      rval_hi >>= (32-bits_size);
      rval = ((unsigned int)rval) >> bits_begin;
      rval |= rval_hi;
    }
  }
  return rval;
}



static
unsigned int UnpackUBits(bool fromBe,const char *addr,int bits_begin,
                         const int bits_size) {
  Q_ASSERT(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  unsigned int rval;
  if (fromBe) {
    rval = (( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
                  ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[3]));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval <<= bits_begin;
    } else {
      rval <<= bits_begin;
      unsigned int rval_lo = (unsigned char)(addr[4]);
      rval_lo >>= (8-bits_begin);
      rval |= rval_lo;
    }
    if (bits_size < 32) rval >>= (32-bits_size);
  } else {
    rval = (( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
                  ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[0]));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval >>= bits_begin;
    } else {
      unsigned int rval_hi = (unsigned char)(addr[4]);
      rval_hi <<= (32-bits_begin);
      rval = rval >> bits_begin;
      rval |= rval_hi;
    }
    if (bits_size < 32) rval &= ((((unsigned int)1)<<bits_size)-1);
  }
  return rval;
}





void Star1::repack(bool fromBe) {
  const int _hip  = UnpackBits(fromBe,(const char*)this, 0,24);
  const unsigned int _cids = UnpackUBits(fromBe,(const char*)this,24, 8);
  const int _x0  = UnpackBits(fromBe,(const char*)this,32,32);
  const int _x1  = UnpackBits(fromBe,(const char*)this,64,32);
  const unsigned int _bV = UnpackUBits(fromBe,(const char*)this, 96, 8);
  const unsigned int _mag = UnpackUBits(fromBe,(const char*)this,104, 8);
  const unsigned int _spInt = UnpackUBits(fromBe,(const char*)this,112,16);
  const int _dx0 = UnpackBits(fromBe,(const char*)this,128,32);
  const int _dx1 = UnpackBits(fromBe,(const char*)this,160,32);
  const int _plx = UnpackBits(fromBe,(const char*)this,192,32);
//Q_ASSERT(hip == _hip);
//Q_ASSERT(componentIds == _cids);
//Q_ASSERT(x0 == _x0);
//Q_ASSERT(x1 == _x1);
//Q_ASSERT(bV == _bV);
//Q_ASSERT(mag == _mag);
//Q_ASSERT(spInt == _spInt);
//Q_ASSERT(dx0 == _dx0);
//Q_ASSERT(dx1 == _dx1);
//Q_ASSERT(plx == _plx);
  hip = _hip;
  componentIds = _cids;
  x0 = _x0;
  x1 = _x1;
  bV = _bV;
  mag = _mag;
  spInt = _spInt;
  dx0 = _dx0;
  dx1 = _dx1;
  plx = _plx;
}

void Star1::print(void) {
 qDebug() << "hip: " << hip
          << ", componentIds: " << ((unsigned int)componentIds)
          << ", x0: " << x0
          << ", x1: " << x1
          << ", bV: " << ((unsigned int)bV)
          << ", mag: " << ((unsigned int)mag)
          << ", spInt: " << spInt
          << ", dx0: " << dx0
          << ", dx1: " << dx1
          << ", plx: " << plx;
}


void Star2::repack(bool fromBe) {
  const int _x0  = UnpackBits(fromBe,(const char*)this, 0,20);
  const int _x1  = UnpackBits(fromBe,(const char*)this,20,20);
  const int _dx0 = UnpackBits(fromBe,(const char*)this,40,14);
  const int _dx1 = UnpackBits(fromBe,(const char*)this,54,14);
  const unsigned int _bV = UnpackUBits(fromBe,(const char*)this,68, 7);
  const unsigned int _mag = UnpackUBits(fromBe,(const char*)this,75, 5);
//Q_ASSERT(x0 == _x0);
//Q_ASSERT(x1 == _x1);
//Q_ASSERT(dx0 == _dx0);
//Q_ASSERT(dx1 == _dx1);
//Q_ASSERT(bV == _bV);
//Q_ASSERT(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  dx0 = _dx0;
  dx1 = _dx1;
  bV = _bV;
  mag = _mag;
}

void Star2::print(void) {
 qDebug() << "x0: " << x0
          << ", x1: " << x1
          << ", dx0: " << dx0
          << ", dx1: " << dx1
          << ", bV: " << bV
          << ", mag: " << mag;
}


void Star3::repack(bool fromBe) {
  const int _x0  = UnpackBits(fromBe,(const char*)this, 0,18);
  const int _x1  = UnpackBits(fromBe,(const char*)this,18,18);
  const unsigned int _bV = UnpackUBits(fromBe,(const char*)this,36, 7);
  const unsigned int _mag = UnpackUBits(fromBe,(const char*)this,43, 5);
//Q_ASSERT(x0 == _x0);
//Q_ASSERT(x1 == _x1);
//Q_ASSERT(bV == _bV);
//Q_ASSERT(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  bV = _bV;
  mag = _mag;
}

void Star3::print(void) {
  qDebug() << "x0: " << x0
           << ", x1: " << x1
           << ", bV: " << bV
           << ", mag: " << mag;
}

} // namespace BigStarCatalogExtension


