/***************************************************************************
 *   Copyright (C) 2006 by J. L. Canales                                   *
 *   jlcanales@users.sourceforge.net                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/

#ifndef _STDSAT_H_
#define _STDSAT_H_ 1

static const double KINTERPOLATIONLIMIT=0.0000000000001;
static const double KMU=3.9861352E5;
static const double KPI=3.141592654;
static const double K2PI=6.283185308;
static const double KEARTHRADIUS=6378.135;
static const double __f=3.352779E-3;
static const double KMFACTOR=7.292115E-5;
static const double KAU=1.4959787066E8; //Km

static const double KDEG2RAD  =   KPI / 180.0;   //   0.0174532925199433
static const double XPDOTP   =  1440.0 / K2PI;  // 229.1831180523293 minutes per radian (earth rotation)

#endif // _STDSAT_H_
