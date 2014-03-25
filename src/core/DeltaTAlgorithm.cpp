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

#include "DeltaTAlgorithm.hpp"

#include "StelLocaleMgr.hpp"

#include <QString>

QString DeltaTAlgorithm::symbol()
{
	// TRANSLATORS: Symbol for "delta T". Parameter is Greek capital "delta".
	return (q_("%1T").arg(QChar(0x0394)));
}


QString DeltaTAlgorithm::getId() const
{
	return identifier;
}

QPair<unsigned int, unsigned int> DeltaTAlgorithm::getRange() const
{
	return QPair<unsigned int, unsigned int>(startYear, endYear);
}

QString DeltaTAlgorithm::getRangeDescription() const
{
	// TRANSLATORS: Range of use of a DeltaT calculation algorithm.
	return q_("Valid range of usage: between years %1 and %2.")
	        .arg(startYear)
	        .arg(endYear);
}

double DeltaTAlgorithm::calculateSecularAcceleration(const int& year,
                                                     const int& month,
                                                     const int& day) const
{
	double decimalYear = year + ((month-1)*30.5+day/31*30.5)/366.0;
	double t = (decimalYear - 1955.5)/100.0;
	// n.dot for secular acceleration of the Moon in ELP2000-82B
	// have value -23.8946 "/cy/cy
	return -0.91072 * (-23.8946 + std::abs(nd)) * t*t;
}

double DeltaTAlgorithm::calculateStandardError(const double& jdUtc,
                                               const int& year)
{
	//double yeardec=year+((month-1)*30.5+day/31*30.5)/366;
	double sigma = -1.;
	if (-1000 <= year && year <= 1600)
	{
		// sigma = std::pow((yeardec-1820.0)/100,2); // sigma(DeltaT) = 0.8*u^2
		// 1820.0 = 1820-jan-0.5 = 2385800.0
		double cDiff1820 = (jdUtc - 2385800.0)/36525.0;
		sigma = 0.8 * cDiff1820 * cDiff1820;
	}
	return sigma;
}

void DeltaTAlgorithm::formatDeltaTString(const double& deltaT,
                                         const double& jdUtc,
                                         const int& year,
                                         QString* string)
{
	Q_ASSERT(string);

	QChar rangeIndicator;
	if (startYear != endYear)
	{
		if (startYear > year || year > endYear)
			rangeIndicator = "*";
	}
	else
		rangeIndicator = "?";

	// Put sigma on a separate line?
	// Display algorithm's ndot?
	double sigma = -1.;
	sigma = calculateStandardError(jdUtc, year);
	if (sigma > 0)
	{
		// TRANSLATORS: Tooltip with the current value of deltaT and parameters.
		// %1 is the symbol "deltaT"; 's' means seconds; %3 is *, ? or nothing;
		// 'cy' means centuries; %4 is 'squared' symbol; %5 is the letter sigma.
		string = q_("%1 = %2s%3 [n-dot @ -23.8946''/cy%4; %5(%1) = %6s]")
		         .arg(symbol())
		         .arg(deltaT, 5, 'f', 2)
		         .arg(rangeIndicator)
		         .arg(QChar(0x00B2)) // Squared
		         .arg(QChar(0x03c3)) // Sigma
		         .arg(sigma, 3, 'f', 1);
	}
	else
	{
		// TRANSLATORS: Tooltip with the current value of deltaT and parameters.
		// %1 is the symbol "deltaT"; 's' means seconds; %3 is *, ? or nothing;
		// 'cy' means centuries; %4 is 'squared' symbol.*/
		string = q_("%1 = %2s%3 [n-dot @ -23.8946''/cy%4]")
		         .arg(symbol())
		         .arg(deltaT, 5, 'f', 2)
		         .arg(rangeIndicator)
		         .arg(QChar(0x00B2)); // Squared
	}
}



DeltaT::WithoutCorrection::WithoutCorrection() :
    identifier("WithoutCorrection"),
    ndot (0.),
    startYear(0),
    endYear(0)
{}

QString DeltaT::WithoutCorrection::getName() const
{
	// TRANSLATORS: Name of a time correction method.
	return q_("Without correction");
}

QString DeltaT::WithoutCorrection::getDescription() const
{
	// TRANSLATORS: Description of a time correction method.
	return q_("Correction is disabled. Use only if you know what you are doing!");
}

QString DeltaT::WithoutCorrection::getRangeDescription() const
{
	return QString();
}

double DeltaT::WithoutCorrection::calculateDeltaT(const double& jdUtc, const int& year, const int& month, const int& day, QString* string)
{
	Q_UNUSED(jdUtc);
	Q_UNUSED(year);
	Q_UNUSED(month);
	Q_UNUSED(day);
	if (string)
		string->clear();
	return 0.;
}



DeltaT::Custom::Custom() :
    identifier("Custom"),
    ndot(0.),
    startYear(0.),
    endYear(0.),
    paramYear(0.f),
    paramA(0.f),
    paramB(0.f),
    paramC(0.f)
{}

QString DeltaT::Custom::getName() const
{
	// TRANSLATORS: Name of a time correction method. Param is "DeltaT" symbol.
	return q_("Custom %1 equation").arg(symbol());
}

QString DeltaT::Custom::getDescription() const
{
	// TODO: More detail?
	// TRANSLATORS: Description of a time correction method.
	return q_("A quadratic formula with coefficients provided by the user.");
}

double DeltaT::Custom::calculateDeltaT(const double& jdUtc,
                                       const int& year,
                                       const int& month,
                                       const int& day,
                                       QString* string)
{
	Q_UNUSED(jdUtc);
	double decimalYear = year + ((month-1)*30.5 + day/31*30.5)/366;
	double u = (decimalYear - paramYear)/100;
	double deltaT = paramA + paramB*u + paramC*std::pow(u,2);

	if (string)
	{
		formatDeltaTString(deltaT, jdUtc, year, string);
	}

	return deltaT + calculateSecularAcceleration(year, month, day);
}

void DeltaT::Custom::setParameters(const float& year,
                                   const float& ndot,
                                   const float& a,
                                   const float& b,
                                   const float& c)
{
	paramYear = year;
	this->ndot = ndot;
	paramA = a;
	paramB = b;
	paramC = c;
}
