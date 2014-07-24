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

#include "Star.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QString>

QString Star1::getNameI18n(void) const
{
	if (getHip())
	{
		const QString commonNameI18 = StarMgr::getCommonName(getHip());
		if (!commonNameI18.isEmpty()) return commonNameI18;
		if (StarMgr::getFlagSciNames())
		{
			const QString sciName = StarMgr::getSciName(getHip());
			if (!sciName.isEmpty()) return sciName;
			const QString varSciName = StarMgr::getGcvsName(getHip());
			if (!varSciName.isEmpty() && varSciName!=sciName) return varSciName;
			return QString("HIP %1").arg(getHip());
		}
	}
	return QString();
}

int Star1::hasComponentID(void) const
{
	if (getComponentIds())
	{
		return getComponentIds();
	}
	return 0;
}

static int UnpackBits(bool fromBe, const char *addr,int bits_begin, const int bits_size)
{
	Q_ASSERT(bits_size <= 32);
	while (bits_begin >= 8)
	{
		bits_begin -= 8;
		addr++;
	}
	const int bits_end = bits_begin + bits_size;
	int rval;
	if (fromBe)
	{
		rval = (int)((( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
		       ((unsigned int)(unsigned char)(addr[1]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[2]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[3])));
		if (bits_end <= 32)
		{
			if (bits_begin > 0) rval <<= bits_begin;
		}
		else
		{
			rval <<= bits_begin;
			unsigned int rval_lo = (unsigned char)(addr[4]);
			rval_lo >>= (8-bits_begin);
			rval |= rval_lo;
		}
		if (bits_size < 32) rval >>= (32-bits_size);
	}
	else
	{
		rval = (int)((( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
		       ((unsigned int)(unsigned char)(addr[2]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[1]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[0])));
		if (bits_end <= 32)
		{
			if (bits_end < 32) rval <<= (32-bits_end);
			if (bits_size < 32) rval >>= (32-bits_size);
		}
		else
		{
			int rval_hi = addr[4];
			rval_hi <<= (64-bits_end);
			rval_hi >>= (32-bits_size);
			rval = ((unsigned int)rval) >> bits_begin;
			rval |= rval_hi;
		}
	}
	return rval;
}



static unsigned int UnpackUBits(bool fromBe,const char *addr,int bits_begin, const int bits_size)
{
	Q_ASSERT(bits_size <= 32);
	while (bits_begin >= 8)
	{
		bits_begin -= 8;
		addr++;
	}
	const int bits_end = bits_begin + bits_size;
	unsigned int rval;
	if (fromBe)
	{
		rval = (( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
		       ((unsigned int)(unsigned char)(addr[1]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[2]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[3]));
		if (bits_end <= 32)
		{
			if (bits_begin > 0) rval <<= bits_begin;
		}
		else
		{
			rval <<= bits_begin;
			unsigned int rval_lo = (unsigned char)(addr[4]);
			rval_lo >>= (8-bits_begin);
			rval |= rval_lo;
		}
		if (bits_size < 32) rval >>= (32-bits_size);
	}
	else
	{
		rval = (( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
		       ((unsigned int)(unsigned char)(addr[2]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[1]))) << 8) |
		       ((unsigned int)(unsigned char)(addr[0]));
		if (bits_end <= 32)
		{
			if (bits_begin > 0) rval >>= bits_begin;
		}
		else
		{
			unsigned int rval_hi = (unsigned char)(addr[4]);
			rval_hi <<= (32-bits_begin);
			rval = rval >> bits_begin;
			rval |= rval_hi;
		}
		if (bits_size < 32) rval &= ((((unsigned int)1)<<bits_size)-1);
	}
	return rval;
}

void Star1::repack(bool fromBe)
{
	Q_ASSERT(false);
}

void Star1::print(void)
{
	qDebug() << "hip: " << getHip()
		 << ", componentIds: " << getComponentIds()
		 << ", x0: " << x0
		 << ", x1: " << x1
		 << ", bV: " << ((unsigned int)bV)
		 << ", mag: " << ((unsigned int)mag)
		 << ", spInt: " << spInt
		 << ", dx0: " << dx0
		 << ", dx1: " << dx1
		 << ", plx: " << plx;
}

void Star2::repack(bool fromBe)
{
	Q_ASSERT(false);
}

void Star2::print(void)
{
	qDebug() << "x0: " << getX0()
		 << ", x1: " << getX1()
		 << ", dx0: " << getDx0()
		 << ", dx1: " << getDx1()
		 << ", bV: " << getBV()
		 << ", mag: " << getMag();
}

void Star3::repack(bool fromBe)
{
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

void Star3::print(void)
{
	qDebug() << "x0: " << x0
		 << ", x1: " << x1
		 << ", bV: " << bV
		 << ", mag: " << mag;
}

