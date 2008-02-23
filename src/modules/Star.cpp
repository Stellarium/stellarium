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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Star.hpp"

#include "StarMgr.hpp"
#include "StelUtils.hpp"
#include <iostream>

namespace BigStarCatalogExtension {

QString Star1::getNameI18n(void) const {
  if (hip) {
    const QString commonNameI18 = QString::fromStdWString(StarMgr::getCommonName(hip));
    if (!commonNameI18.isEmpty()) return commonNameI18;
    if (StarMgr::getFlagSciNames()) {
      const QString sciName = QString::fromStdWString(StarMgr::getSciName(hip));
      if (!sciName.isEmpty()) return sciName;
      return QString("HP %1").arg(hip);
    }
  }
  return "";
}


static
int UnpackBits(bool from_be,const char *addr,int bits_begin,
               const int bits_size) {
  assert(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  int rval;
  if (from_be) {
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
unsigned int UnpackUBits(bool from_be,const char *addr,int bits_begin,
                         const int bits_size) {
  assert(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  unsigned int rval;
  if (from_be) {
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





void Star1::repack(bool from_be) {
  const int _hip  = UnpackBits(from_be,(const char*)this, 0,24);
  const unsigned int _cids = UnpackUBits(from_be,(const char*)this,24, 8);
  const int _x0  = UnpackBits(from_be,(const char*)this,32,32);
  const int _x1  = UnpackBits(from_be,(const char*)this,64,32);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this, 96, 8);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,104, 8);
  const unsigned int _sp_int = UnpackUBits(from_be,(const char*)this,112,16);
  const int _dx0 = UnpackBits(from_be,(const char*)this,128,32);
  const int _dx1 = UnpackBits(from_be,(const char*)this,160,32);
  const int _plx = UnpackBits(from_be,(const char*)this,192,32);
//assert(hip == _hip);
//assert(component_ids == _cids);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(b_v == _b_v);
//assert(mag == _mag);
//assert(sp_int == _sp_int);
//assert(dx0 == _dx0);
//assert(dx1 == _dx1);
//assert(plx == _plx);
  hip = _hip;
  component_ids = _cids;
  x0 = _x0;
  x1 = _x1;
  b_v = _b_v;
  mag = _mag;
  sp_int = _sp_int;
  dx0 = _dx0;
  dx1 = _dx1;
  plx = _plx;
}

void Star1::print(void) {
 std::cout << "hip: " << hip
       << ", component_ids: " << ((unsigned int)component_ids)
       << ", x0: " << x0
       << ", x1: " << x1
       << ", b_v: " << ((unsigned int)b_v)
       << ", mag: " << ((unsigned int)mag)
       << ", sp_int: " << sp_int
       << ", dx0: " << dx0
       << ", dx1: " << dx1
       << ", plx: " << plx
       << endl;
}


void Star2::repack(bool from_be) {
  const int _x0  = UnpackBits(from_be,(const char*)this, 0,20);
  const int _x1  = UnpackBits(from_be,(const char*)this,20,20);
  const int _dx0 = UnpackBits(from_be,(const char*)this,40,14);
  const int _dx1 = UnpackBits(from_be,(const char*)this,54,14);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this,68, 7);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,75, 5);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(dx0 == _dx0);
//assert(dx1 == _dx1);
//assert(b_v == _b_v);
//assert(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  dx0 = _dx0;
  dx1 = _dx1;
  b_v = _b_v;
  mag = _mag;
}

void Star2::print(void) {
 std::cout << "x0: " << x0
       << ", x1: " << x1
       << ", dx0: " << dx0
       << ", dx1: " << dx1
       << ", b_v: " << b_v
       << ", mag: " << mag
       << endl;
}


void Star3::repack(bool from_be) {
  const int _x0  = UnpackBits(from_be,(const char*)this, 0,18);
  const int _x1  = UnpackBits(from_be,(const char*)this,18,18);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this,36, 7);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,43, 5);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(b_v == _b_v);
//assert(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  b_v = _b_v;
  mag = _mag;
}

void Star3::print(void) {
  std::cout << "x0: " << x0
       << ", x1: " << x1
       << ", b_v: " << b_v
       << ", mag: " << mag
       << endl;
}

} // namespace BigStarCatalogExtension


