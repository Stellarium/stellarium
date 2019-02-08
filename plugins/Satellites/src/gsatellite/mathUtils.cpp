/***************************************************************************
 * Name: mathUtils.cpp
 *
 * Description: Mathematical utilities
 *              Implement some mathematical utilities to trigonometrical
 *              calc.
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by J. L. Canales                                   *
 *   ph03696@homeserver                                                    *
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

#include <cmath>
#include "stdsat.h"

// Four-quadrant arctan function
double AcTan(double sinx, double cosx)
{
	if(cosx == 0)
	{
		if(sinx > 0)
			return (KPI/2.0);

		return (3.0*KPI/2.0);
	}
	else
	{
		if(cosx > 0)
		{
			if(sinx > 0)
				return (atan(sinx/cosx));

			return (K2PI + atan(sinx/cosx));
		}

		return (KPI + atan(sinx/cosx));
	}
} /* Function AcTan */

/* Returns square of a double */
double Sqr(double arg)
{
	return(arg*arg);
} /* Function Sqr */


