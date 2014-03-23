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

#ifndef DELTATALGORITHM_HPP
#define DELTATALGORITHM_HPP

#include <QPair>
#include <QString>


//! Base class for classes describing time correction algorithms.
//! DeltaT is the difference between Terestrial Time (TT) and Universal Time (UT).
//! @todo Explain the terms in the base class documentation.
//! @todo Decide whether to translate the names and descriptions in-class, or...
//! @todo Decide whether this will be an abstract interface, or...
class DeltaTAlgorithm
{
public:
	//DeltaTAlgorithm();

	//! Returns a string uniquely identifying this algorithm.
	virtual QString getId() const = 0;
	//! Returns a human-readable name, such as "Espenak & Meeus (2006)".
	virtual QString getName() const = 0;
	//! Returns a HTML description of the algorithm, suitable for a GUI.
	//! Translated. Includes a description of the range. May include links.
	//! @todo Keep the range description separate?
	virtual QString getDescription() const = 0;

	//! Returns the date range (years) when the algorithm returns valid results.
	virtual QPair<unsigned int, unsigned int> getRange() const;
	//! Returns a human-readable description of the algorithm input range.
	//! @todo Copy-edit decription(s).
	virtual QString getRangeDescription() const;

	//! Calculates &Delta;T for the specified date, in string form if requested.
	//! The string contains the value in seconds, in h/m/s form if it's larger
	//! than a minute, the standard error (sigma) if applicable, and a symbol
	//! indicating whether the date was in the method's acceptable range.
	//! @param[in] jd JD(UTC), the only strictly necessary parameter.
	//! @param[in] year of the same date
	//! @param[in] month of the same date
	//! @param[in] day of the same date
	//! @param[in,out] string if provided, it's filled with displayable text
	virtual double calculateDeltaT(const double& jdUtc,
	                               const int& year,
	                               const int& month,
	                               const int& day,
	                               QString* string = 0) = 0;

	//! DeltaT written with the Greek letter.
	static QString symbol();

private:
	//! "Dotted n", the Moon's acceleration in arc-seconds per century-squared.
	//! Expressed as the time derivative of the mean sidereal angular motion of
	//! the Moon @b n, written in Newton's notation (equivalent to dn/dt or n').
	double ndot;

	unsigned int startYear;
	unsigned int endYear;

	//! Calculates the correction for the secular acceleration of the Moon.
	virtual double calculateSecularAcceleration(const int& year) const;

	//! Calculates the standard error (sigma).
	virtual double calculateStandardError(const double& jdUtc,
	                                      const int& year);
};

#endif // DELTATALGORITHM_HPP
