/*
 * Stellarium
 * Copyright (C) Fabien Chereau
 * Author 2006 Johannes Gajdosik
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

#ifndef _VIEWPORTDISTORTER_HPP_
#define _VIEWPORTDISTORTER_HPP_

#include "StelProjectorType.hpp"
class QString;

class StelViewportDistorter
{
public:
	static StelViewportDistorter *create(const QString &type,
	                                 int width,int height,
	                                 StelProjectorP prj);
	// StelCore is needed for getProjectionType and setMaxFov
	virtual ~StelViewportDistorter(void) {}
	virtual QString getType(void) const = 0;
	virtual void prepare(void) const = 0;
	virtual void distort(void) const = 0;
	virtual bool distortXY(int &x,int &y) const = 0;

protected:
	StelViewportDistorter(void) {}

private:
	// no copying:
	StelViewportDistorter(const StelViewportDistorter&);
	const StelViewportDistorter &operator=(const StelViewportDistorter&);

};

#endif // _VIEWPORTDISTORTER_HPP_
