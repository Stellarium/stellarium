/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024-2025 WANG Siliang
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SkyCoords.hpp"
#include <QtMath>
#include <QDebug>

#include <math.h>

/*
 * Convert pixel coordinates (X, Y) to celestial coordinates (RA, Dec),
 * with polar region correction to improve numerical stability near the poles.
 * All angles are in degrees. Output: RA ∈ [0,360), Dec ∈ [-90,90].
 *
 * References:
 * Greisen, E. W., & Calabretta, M. R. (2002).
 * Representations of World Coordinates in FITS.
 * *Astronomy & Astrophysics, 395*(2), 1061-1073.
 * https://ui.adsabs.harvard.edu/abs/2002A%26A...395.1061G
 *
 * Calabretta, M. R., & Greisen, E. W. (2002).
 * Representations of celestial coordinates in FITS.
 * *Astronomy & Astrophysics, 395*(2), 1077-1082.
 * https://ui.adsabs.harvard.edu/abs/2002A%26A...395.1077C
 */
QPair<double, double> SkyCoords::pixelToRaDec(
	int    X,       // Pixel x-coordinate
	int    Y,       // Pixel y-coordinate
	double CRPIX1,  // Reference pixel x
	double CRPIX2,  // Reference pixel y
	double CRVAL1,  // Reference right ascension α0 (degrees)
	double CRVAL2,  // Reference declination δ0 (degrees)
	double CD1_1, double CD1_2,
	double CD2_1, double CD2_2)
{
	// Step 1: Linear transformation — pixel to intermediate world coordinates (ξ, η)
	double dx = (double)X - CRPIX1;
	double dy = (double)Y - CRPIX2;
	double xi  = CD1_1 * dx + CD1_2 * dy;
	double eta = CD2_1 * dx + CD2_2 * dy;

	// Step 2: Inverse gnomonic projection
	double xi_r   = xi * D2R;
	double eta_r  = eta * D2R;
	double alpha0 = CRVAL1 * D2R;
	double delta0 = CRVAL2 * D2R;

	double r = hypot(xi_r, eta_r);
	if (r < 1e-12) {
		return qMakePair(CRVAL1, CRVAL2);
	}

	double c = atan(r);
	double sin_c = sin(c);
	double cos_c = cos(c);

	double sin_delta0 = sin(delta0);
	double cos_delta0 = cos(delta0);

	double sin_dec = cos_c * sin_delta0 + (eta_r * sin_c * cos_delta0 / r);

	// Step 3: Polar region correction for declination
	// Standard asin calculation with clamping
	if (sin_dec >  1.0) sin_dec =  1.0;
	if (sin_dec < -1.0) sin_dec = -1.0;
	double dec_rad = asin(sin_dec);

	// Step 4: Compute right ascension offset
	double num = xi_r * sin_c;
	double den = r * cos_delta0 * cos_c - eta_r * sin_delta0 * sin_c;
	double delta_ra = atan2(num, den);
	double alpha = alpha0 + delta_ra;

	// Step 5: Convert back to degrees and normalize RA to [0, 360)
	double ra = alpha * R2D;  // Convert from radians to degrees
	ra = StelUtils::fmodpos(ra, 360.0);  // Normalize to [0, 360)
	double dec = dec_rad * R2D;
	return qMakePair(ra, dec);
}


/**
 * @brief Calculates the central RA/Dec coordinate from a list of corner sky coordinates.
 *
 * The input is a JSON array of [RA, Dec] pairs (in degrees).
 * Each pair is converted to 3D Cartesian coordinates on the unit sphere.
 * The average vector is normalized and converted back to spherical coordinates (RA/Dec).
 *
 * @param corners A QJsonArray of arrays, where each sub-array contains two doubles: [RA, Dec] in degrees.
 * @return QPair<double, double> The computed center coordinates as (RA, Dec) in degrees.
 */
QPair<double, double> SkyCoords::calculateCenter(const QJsonArray& corners)
{
	const int N = corners.size();

	// Check for empty input
	if (N < 1)
	{
		qWarning() << "[SkyCoords] Empty or invalid corners array.";
		return qMakePair(0.0, 0.0);
	}

	double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;

	// Convert each (RA, Dec) to Cartesian coordinates and accumulate
	for (const QJsonValue& val : corners)
	{
		if (!val.isArray() || val.toArray().size() != 2)
		{
			qWarning() << "[SkyCoords] Invalid corner format.";
			continue;
		}

		QJsonArray coord = val.toArray();
		double ra  = coord[0].toDouble() * D2R; // Convert RA to radians
		double dec = coord[1].toDouble() * D2R; // Convert Dec to radians

		// Spherical to Cartesian
		double x = cos(dec) * cos(ra);
		double y = cos(dec) * sin(ra);
		double z = sin(dec);

		sum_x += x;
		sum_y += y;
		sum_z += z;
	}

	// Check for degenerate vector
	if (sum_x == 0.0 && sum_y == 0.0 && sum_z == 0.0)
	{
		qWarning() << "[SkyCoords] Zero vector sum; cannot compute center.";
		return qMakePair(0.0, 0.0);
	}

	// Normalize the average vector
	double avg_x = sum_x / N;
	double avg_y = sum_y / N;
	double avg_z = sum_z / N;

	double norm = sqrt(avg_x * avg_x + avg_y * avg_y + avg_z * avg_z);
	if (norm == 0.0)
	{
		qWarning() << "[SkyCoords] Normalization failed due to zero norm.";
		return qMakePair(0.0, 0.0);
	}

	avg_x /= norm;
	avg_y /= norm;
	avg_z /= norm;

	// Convert back to spherical coordinates (RA in [0, 360), Dec in degrees)
	double ra_center  = fmod(R2D * atan2(avg_y, avg_x) + 360.0, 360.0);
	double dec_center = R2D * asin(avg_z);

	return qMakePair(ra_center, dec_center);
}
