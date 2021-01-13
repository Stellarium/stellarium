/*
 * Copyright (C) 2003 Fabien Chereau
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

#include "StelToneReproducer.hpp"
#include "StelUtils.hpp"
#include <cmath>

/*********************************************************************
 Constructor: Set some default values to prevent bugs in case of bad use
*********************************************************************/
StelToneReproducer::StelToneReproducer() : Lda(50.f), Lwa(40000.f), oneOverMaxdL(1.f/100.f), lnOneOverMaxdL(std::log(1.f/100.f)), oneOverGamma(1.f/2.2222f)
{
	// Initialize  sensor
	setInputScale();
	
	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.619f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;
	
	setDisplayAdaptationLuminance(Lda);
	setWorldAdaptationLuminance(Lwa);
}


/*********************************************************************
 Destructor
*********************************************************************/
StelToneReproducer::~StelToneReproducer()
{
}

/*********************************************************************
 Set the global input scale
*********************************************************************/
void StelToneReproducer::setInputScale(float scale)
{
	inputScale=scale;
	lnInputScale = std::log(inputScale);
}
	
/*********************************************************************
 Set the eye adaptation luminance for the display (and precompute what can be)
*********************************************************************/
void StelToneReproducer::setDisplayAdaptationLuminance(float _Lda)
{
	Lda = _Lda;

	// Update alphaDa and betaDa values
	float log10Lda = std::log10(Lda);
	alphaDa = 0.4f * log10Lda + 1.619f;
	betaDa = -0.4f * log10Lda*log10Lda + 0.218f * log10Lda + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 =(stelpow10f((betaWa-betaDa)/alphaDa) / (M_PIf*0.0001f));
	lnTerm2 = std::log(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}

/*********************************************************************
 Set the eye adaptation luminance for the world (and precompute what can be)
*********************************************************************/
void StelToneReproducer::setWorldAdaptationLuminance(float _Lwa)
{
	Lwa = _Lwa;

	// Update alphaDa and betaDa values
	float log10Lwa = std::log10(Lwa);
	alphaWa = 0.4f * log10Lwa + 1.619f;
	betaWa = -0.4f * log10Lwa*log10Lwa + 0.218f * log10Lwa + 6.1642f;

	// Update terms
	alphaWaOverAlphaDa = alphaWa/alphaDa;
	term2 = (stelpow10f((betaWa-betaDa)/alphaDa) / (M_PIf*0.0001f));
	lnTerm2 = std::log(term2);
	term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
}
