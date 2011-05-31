/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _STELNAVIGATOR_HPP_
#define _STELNAVIGATOR_HPP_

#include "VecMath.hpp"
#include "StelLocation.hpp"

#include <QObject>
#include <QString>
#include <QTime>
#include <QDateTime>



class StelObserver;
class StelObject;

//! @class StelNavigator
//! Manages a navigation context.  This includes:
//! - date/time
//! - viewing direction/fov
//! - observer position
//! - coordinate changes
class StelNavigator : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double timeRate READ getTimeRate WRITE setTimeRate)

public:
	// Create and initialise to default a navigation context
	StelNavigator();
	~StelNavigator();

	void init();



public slots:




private:

};

#endif // _STELNAVIGATOR_HPP_
