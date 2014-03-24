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

#include "StelDeltaTMgr.hpp"
#include "DeltaTAlgorithm.hpp"

StelDeltaTMgr::StelDeltaTMgr() :
    currentAlgorithm(0),
    defaultAlgorithm(0),
    zeroAlgorithm(0),
    customAlgorithm(0)
{
	// TODO
}

StelDeltaTMgr::~StelDeltaTMgr()
{
	// TODO
}

void
StelDeltaTMgr::setCurrentAlgorithm(const QString& id)
{
	// TODO
}

QString
StelDeltaTMgr::getCurrentAlgorithmId() const
{
	return currentAlgorithm->getId();
}

QList<QString>
StelDeltaTMgr::getAvailableAlgorithmIds() const
{
	return algorithms.keys();
}

QStandardItemModel
StelDeltaTMgr::getAvailableAlgorithmsModel()
{
	return QStandardItemModel();
}

double
StelDeltaTMgr::calculateDeltaT(const double& jdUtc, QString* outputString)
{
	int year, month, day;
	// TODO
	return currentAlgorithm->calculateDeltaT(jdUtc,
	                                         year, month, day,
	                                         outputString);
}
