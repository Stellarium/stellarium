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

#include <QMap>
#include <QStandardItemModel>
#include <QString>

class DeltaTAlgorithm;

//! Manages a collection of DeltaTArgorithm objects.
//! Creating an object of this class initializes a collection of objects
//! representing all the available algorithms for DeltaT calculation.
class StelDeltaTMgr
{
public:
	StelDeltaTMgr();
	~StelDeltaTMgr();

	//DeltaTAlgorithm* getAlgorithm (const QString& id) const;
	//!
	void setCurrentAlgorithm(const QString& id);
	//!
	QString getCurrentAlgorithmId() const;
	//!
	QList<QString> getAvailableAlgorithmIds() const;
	//! Returns a data model describing the available DeltaT algorithms.
	//! @todo return object or pointer?
	void populateAvailableAlgorithmsModel(QStandardItemModel* model);


	//! Calculates DeltaT correction using the currently selected algorithm.
	double calculateDeltaT(const double& jdUtc, QString* outputString);

	//!
	void getCustomAlgorithmParams(float* year,
	                              float* ndot,
	                              float* a,
	                              float* b,
	                              float* c);
	//!
	void setCustomAlgorithmParams(const float& year,
	                              const float& ndot,
	                              const float& a,
	                              const float& b,
	                              const float& c);

private:
	//! All available algorithms, keys are algorithm IDs.
	//! Should contain one object per each child class of DeltaTAlgorithm.
	QMap<QString,DeltaTAlgorithm*> algorithms;

	//! Pointer to the object of the currently used algorithm.
	DeltaTAlgorithm* currentAlgorithm;
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
