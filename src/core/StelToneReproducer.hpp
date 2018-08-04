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

#ifndef STELTONEREPRODUCER_HPP
#define STELTONEREPRODUCER_HPP

#include <cmath>

//! Converts tones in function of the eye adaptation to luminance.
//! The aim is to get on the screen something which is perceptualy accurate,
//! ie. to compress high dynamic range luminance to CRT display range.
//! The class perform mainly a fast implementation of the algorithm from the
//! paper [1], with more accurate values from [2]. The blue shift formula is taken
//! from [3] and combined with the Scotopic vision formula from [4].
//!
//! Important : you may call setDisplayAdaptationLuminance()
//! and setWorldAdaptationLuminance() before any call to xyYToRGB()
//! or adaptLuminance otherwise the default values will be used. (they are
//! appropriate for a daylight sky luminance)
//!
//! REFERENCES :
//! Thanks to all the authors of the following papers I used for providing
//! their work freely online.
//!
//! [1] "Tone Reproduction for Realistic Images", Tumblin and Rushmeier,
//! IEEE Computer Graphics & Application, November 1993
//!
//! [2] "Tone Reproduction and Physically Based Spectral Rendering",
//! Devlin, Chalmers, Wilkie and Purgathofer in EUROGRAPHICS 2002
//!
//! [3] "Night Rendering", H. Wann Jensen, S. Premoze, P. Shirley,
//! W.B. Thompson, J.A. Ferwerda, M.M. Stark
//!
//! [4] "A Visibility Matching Tone Reproduction Operator for High Dynamic
//! Range Scenes", G.W. Larson, H. Rushmeier, C. Piatko
class StelToneReproducer
{
public:
	//! Constructor
	StelToneReproducer();
	
	//! Desctructor
	virtual ~StelToneReproducer();

	//! Set the eye adaptation luminance for the display (and precompute what can be)
	//! Usual luminance range is 1-100 cd/m^2 for a CRT screen
	//! @param displayAdaptationLuminance the new display luminance in cd/m^2. The initial default value is 50 cd/m^2
	void setDisplayAdaptationLuminance(float displayAdaptationLuminance);

	//! Set the eye adaptation luminance for the world (and precompute what can be)
	//! @param worldAdaptationLuminance the new world luminance in cd/m^2. The initial default value is 40000 cd/m^2 for Skylight
	//! Star Light      : 0.001  cd/m^2
	//! Moon Light      : 0.1    cd/m^2
	//! Indoor Lighting : 100    cd/m^2
	//! Sun Light       : 100000 cd/m^2
	void setWorldAdaptationLuminance(float worldAdaptationLuminance);
	//! Get the eye adaptation luminance for the world
	float getWorldAdaptationLuminance() const
	{
		return Lwa;
	}
	
	//! Set the global scale applied to input lumiances, i.e before the adaptation
	//! It is the parameter to modify to simulate aperture*exposition time
	//! @param scale the global input scale
	void setInputScale(float scale=1.f);
	//! Get the global scale applied to input luminances, i.e before the adaptation
	float getInputScale() const
	{
		return inputScale;
	}
	
	//! Set the maximum luminance of the display (CRT, screen etc..)
	//! This value is used to scale the RGB range
	//! @param maxdL the maximum lumiance in cd/m^2. Initial default value is 120 cd/m^2
	void setMaxDisplayLuminance(float maxdL)
	{
		oneOverMaxdL = 1.f/maxdL; lnOneOverMaxdL=std::log(oneOverMaxdL); term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
	}

	//! Get the display gamma
	//! @return the display gamma. Default value is 2.2222 for a CRT
	float getDisplayGamma() const
	{
		return 1.f/oneOverGamma;
	}
	
	//! Set the display gamma
	//! @param gamma the gamma. Initial default value is 2.2222
	void setDisplayGamma(float gamma)
	{
		oneOverGamma = 1.f/gamma; term2TimesOneOverMaxdLpOneOverGamma = std::pow(term2*oneOverMaxdL, oneOverGamma);
	}

	//! Return adapted luminance from world to display
	//! @param worldLuminance the world luminance to convert in cd/m^2
	//! @return the converted display luminance in cd/m^2
	float adaptLuminance(float worldLuminance) const
	{
		return std::pow((float)(inputScale*worldLuminance*M_PI*0.0001f),alphaWaOverAlphaDa) * term2;
	}

	//! Return adapted luminance from display to world
	//! @param displayLuminance the display luminance to convert in cd/m^2
	//! @return the converted world luminance in cd/m^2
	float reverseAdaptLuminance(float displayLuminance) const
	{
		return (float) std::pow((float)(displayLuminance/term2),1.f/alphaWaOverAlphaDa)/(inputScale*M_PI*0.0001f);
	}
	
	//! Return adapted luminance from world to display with 1 corresponding to full display white
	//! @param worldLuminance the world luminance to convert in cd/m^2
	//! @return the converted display luminance with 1 corresponding to full display white. The value can be more than 1 when saturated.
	float adaptLuminanceScaled(float worldLuminance) const
	{
		return adaptLuminance(worldLuminance)*oneOverMaxdL;
	}
	
	//! Return adapted luminance from display to world with 1 corresponding to full display white
	//! @param displayLuminance the display luminance with 1 corresponding to full display white. The value can be more than 1 when saturated.
	//! @return the converted world luminance in cd/m^2
	float reverseAdaptLuminanceScaled(float displayLuminance) const
	{
		return reverseAdaptLuminance(displayLuminance/oneOverMaxdL);
	}
	
	//! Return adapted ln(luminance) from world to display with 1 corresponding to full display white
	//! @param lnWorldLuminance the world luminance to convert in ln(cd/m^2)
	//! @param pFact the power at which the result should be set. The default is 0.5 and therefore return the square root of the adapted luminance
	//! @return the converted display set at the pFact power. Luminance with 1 corresponding to full display white. The value can be more than 1 when saturated.
	float adaptLuminanceScaledLn(float lnWorldLuminance, float pFact=0.5f) const
	{
		const float lnPix0p0001 = -8.0656104861f;
		return std::exp(((lnInputScale+lnWorldLuminance+lnPix0p0001)*alphaWaOverAlphaDa+lnTerm2+lnOneOverMaxdL)*pFact);
	}
	
	//! Convert from xyY color system to RGB.
	//! The first two components x and y indicate the "color", the Y is luminance in cd/m^2.
	//! @param xyY an array of 3 floats which are replaced by the converted RGB values
	void xyYToRGB(float* xyY) const;
	
	void getShadersParams(float& a, float& b, float & c) const
	{
		a=alphaWaOverAlphaDa;
		b=oneOverGamma;
		c=term2TimesOneOverMaxdLpOneOverGamma;
	}
private:
	// The global luminance scaling
	float inputScale;
	float lnInputScale;		// std::log(inputScale)
	
	float Lda;		// Display luminance adaptation (in cd/m^2)
	float Lwa;		// World   luminance adaptation (in cd/m^2)
	float oneOverMaxdL;	// 1 / Display maximum luminance (in cd/m^2)
	float lnOneOverMaxdL; // log(oneOverMaxdL)
	float oneOverGamma;	// 1 / Screen gamma value

	// Precomputed variables
	float alphaDa;
	float betaDa;
	float alphaWa;
	float betaWa;
	float alphaWaOverAlphaDa;
	float term2;
	float lnTerm2;	// log(term2)
	
	float term2TimesOneOverMaxdLpOneOverGamma;
};

#endif // STELTONEREPRODUCER_HPP

