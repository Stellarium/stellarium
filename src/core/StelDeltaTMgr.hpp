/*
 * Stellarium
 * Copyright (C) 2014  Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef STELDELTATMGR_HPP
#define STELDELTATMGR_HPP

#include "DeltaTAlgorithm.hpp"

#include <QMap>
#include <QString>

//! Manages a collection of DeltaTArgorithm objects.
//! Not responsible for DeltaT calculations themselves.
//! Creating an object of this class initializes a collection of objects
//! representing all the available algorithms for DeltaT calculation.
//! For now, mostly a convenience class for getting the appropriate DeltaT
//! algorith by identifier, later may provide other services.
class StelDeltaTMgr
{
public:
	StelDeltaTMgr();
	~StelDeltaTMgr();

	DeltaTAlgorithm* getAlgorithm (const QString& id) const;

	QList<QString> getAlgorithmIds() const;

private:
	//! All available algorithms, keys are algorithm IDs.
	//! Should contain one object per each child class of DeltaTAlgorithm.
	QMap<QString,DeltaTAlgorithm*> algorithms;

	//! Pointer to the object of the default algorithm.
	//! Setting this in the constructor is/should be the way to set the
	//! default DeltaT algorithm in Stellarium.
	DeltaTAlgorithm* defaultAlgorithm;
	//! Pointer to the object of the "no correction" algorithm.
	DeltaTAlgorithm* zeroAlgorithm;
	//! Pointer to the object of the custom algorithm.
	DeltaTAlgorithm* customAlgorithm;
};

#endif // STELDELTATMGR_HPP
