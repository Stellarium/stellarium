/*
 * Stellarium
 * Portions copyright (C) 2026 Celestia Development Team
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


#ifndef STELPSF_HPP
#define STELPSF_HPP

#include <algorithm>
#include <cmath>

// PSF transitions adapted from https://github.com/CelestiaProject/Celestia/pull/2697
// (Celestia Development Team,
// GPL-2.0-or-later). The glow profile originates in Askaniy Anpilogov's prototype.
// Radii are in logical pixels unless explicitly documented otherwise.
namespace StelPsf
{
inline float smoothStep(float edge0, float edge1, float x)
{
	const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
	return t * t * (3.f - 2.f * t);
}

inline float glowOnset(float peakRadiance)
{
	return smoothStep(1.f, 2.f, peakRadiance);
}

// Preserve the brightness range of unresolved sources. Applying Celestia's
// limit of 100 to every source compresses Venus and Jupiter toward the same
// brightness as stars in Stellarium's exposure model. Keep that lower limit
// for resolved bodies, where zoom would otherwise cause excessive bloom.
// discRadius is in logical pixels, so this transition is independent of DPI.
inline float glowPeak(float peakRadiance, float discRadius=0.f, float pointRadius=1.5f)
{
	constexpr float pointSourceLimit = 1000.f;
	constexpr float resolvedSourceLimit = 100.f;
	const float resolved = smoothStep(1.f, std::max(2.f, pointRadius), discRadius);
	const float maxGlowRadiance = pointSourceLimit + (resolvedSourceLimit - pointSourceLimit) * resolved;
	return peakRadiance / (1.f + peakRadiance / maxGlowRadiance);
}

// Match the mesh's one-physical-pixel visibility threshold, including at high DPI.
inline float pointFade(float discRadiusPixels, float pointRadius, float pointScale)
{
	const float fadeEnd = std::max(2.f, pointRadius * pointScale);
	return smoothStep(0.f, fadeEnd - 1.f, fadeEnd - discRadiusPixels);
}

inline float reflectiveGlowOnset(float peakRadiance, float discRadius, float pointRadius)
{
	if (discRadius <= 0.f)
		return 1.f;
	// Peak required for unit radiance at the limb in the PSF glow profile.
	constexpr float pi = 3.14159265358979323846f;
	const float linkedPeak = std::pow(discRadius * pi / pointRadius, 2.5f);
	return linkedPeak > 0.f ? glowOnset(peakRadiance / linkedPeak) : 1.f;
}

// Zero disables glow; positive decay must stay in the stable, bounded range.
// Ignore NaN rather than allowing it to contaminate geometry and shader uniforms.
inline float boundedFlareDecay(double decay, float current)
{
	return std::isnan(decay) ? current
	     : decay <= 0. ? 0.f : static_cast<float>(std::clamp(decay, 0.05, 1.));
}

inline float glowRadius(float peakRadiance, float strength, float pointRadius, float decay)
{
	constexpr float pi = 3.14159265358979323846f;
	constexpr float minVisibleRadiance = 1.f / (255.f * 12.92f);
	if (!(peakRadiance > 0.f) || !(strength > 0.f) || !(pointRadius > 0.f)
	    || !(decay > 0.f && decay < pi) || !std::isfinite(peakRadiance)
	    || !std::isfinite(strength) || !std::isfinite(pointRadius))
		return 0.f;
	if (peakRadiance * strength <= minVisibleRadiance)
		return 0.f;

	const float a = decay / pointRadius;
	const float invB = (pi - decay) / pointRadius;
	// Invert the shader's falloff, including the exact fade used for this draw.
	// Clamping strength here would give a nonzero radius as the glow fades out.
	return std::pow(peakRadiance, 0.4f)
	     / (a + std::pow(minVisibleRadiance / strength, 0.4f) * invB);
}
}

#endif // STELPSF_HPP
