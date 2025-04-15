#include "SkyCoords.hpp"
#include <QtMath>
#include <QDebug>

/*
 * Convert pixel coordinates (X, Y) on an image to celestial coordinates (RA, Dec) using the World Coordinate System (WCS) parameters.
 * Apply a series of transformations to convert pixel-based coordinates into the corresponding celestial coordinates,
 * taking into account the reference coordinates, rotation matrix, and other transformation parameters.
 *
 * Parameters:
 *   - X, Y: The pixel coordinates in the image.
 *   - CRPIX1, CRPIX2: The reference pixel coordinates (the center of the image).
 *   - CRVAL1, CRVAL2: The celestial coordinates (right ascension and declination) corresponding to the reference pixel.
 *   - CD1_1, CD1_2, CD2_1, CD2_2: The linear transformation matrix elements that map pixel coordinates to celestial coordinates.
 *
 * Returns:
 *   - A QPair containing the longitude and latitude corresponding to the input pixel coordinates.
 *
 * * * * * * * * * * * * * * * * * * * *
 *
 * This function is based on the algorithm and approach from the code licensed under the Apache License, Version 2.0.
 * The original code can be found at:
 * https://github.com/PlanetaryResources/NTL-Asteroid-Data-Hunter/blob/master/Algorithms/Algorithm%20%231/alg1_psyho.h
 *
 * Copyright (C) [Year] [Original Authors].
 *
 * The function implementation is rewritten based on the logic and approach of the original code,
 * but the code itself has been modified and is not directly copied.
 *
 * The code in this function is used under the terms of the Apache License, Version 2.0.
 * You may not use the original code except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * If you modified this code, include a description of the changes made below:
 * [Optional: Describe any modifications made]
 */
QPair<double, double> SkyCoords::pixelToRaDec(int X, int Y,
	double CRPIX1, double CRPIX2,
	double CRVAL1, double CRVAL2,
	double CD1_1, double CD1_2,
	double CD2_1, double CD2_2)
{
	// Constants
	const double D2R = M_PI / 180.0;  // Degree to radian
	const double R2D = 180.0 / M_PI;  // Radian to degree

	// Convert the reference values (RA, Dec) to radians
	double RA0 = CRVAL1 * D2R;
	double Dec0 = CRVAL2 * D2R;

	// Calculate dx and dy
	double dx = X - CRPIX1;
	double dy = Y - CRPIX2;

	// Calculate xx and yy
	double xx = CD1_1 * dx + CD1_2 * dy;
	double yy = CD2_1 * dx + CD2_2 * dy;

	// Calculate the celestial coordinates
	double px = std::atan2(xx, -yy) * R2D;
	double py = std::atan2(R2D, std::sqrt(xx * xx + yy * yy)) * R2D;

	// Calculate sin(Dec) and cos(Dec)
	double sin_dec = std::sin(D2R * (90.0 - CRVAL2));
	double cos_dec = std::cos(D2R * (90.0 - CRVAL2));

	// Calculate longitude offset (dphi)
	double dphi = px - 180.0;

	// Calculate celestial longitude and latitude
	double sinthe = std::sin(py * D2R);
	double costhe = std::cos(py * D2R);
	double costhe3 = costhe * cos_dec;
	double costhe4 = costhe * sin_dec;
	double sinthe3 = sinthe * cos_dec;
	double sinthe4 = sinthe * sin_dec;
	double sinphi = std::sin(dphi * D2R);
	double cosphi = std::cos(dphi * D2R);

	// Calculate celestial longitude
	double x = sinthe4 - costhe3 * cosphi;
	double y = -costhe * sinphi;
	double dlng = R2D * std::atan2(y, x);
	double lng = CRVAL1 + dlng;

	// Normalize the celestial longitude
	if (CRVAL1 >= 0.0) {
		if (lng < 0.0) {
			lng += 360.0;
		}
	} else {
		if (lng > 0.0) {
			lng -= 360.0;
		}
	}

	lng = std::fmod(lng, 360.0);
	if (lng < 0.0) {
		lng += 360.0;
	}

	// Calculate celestial latitude
	double z = sinthe3 + costhe4 * cosphi;
	double lat;
	if (std::abs(z) > 0.99) {
		// For higher precision, use an alternative formula
		if (z < 0.0) {
			lat = -std::abs(R2D * std::acos(std::sqrt(x * x + y * y)));
		} else {
			lat = std::abs(R2D * std::acos(std::sqrt(x * x + y * y)));
		}
	} else {
		lat = R2D * std::asin(z);
	}

	// Return the result as a QPair of doubles
	return qMakePair(lng, lat);
}

QPair<double, double> SkyCoords::calculateCenter(const QJsonArray& corners)
{
	const int N = corners.size();
	if (N < 1) {
		qWarning() << "[SkyCoords] Empty or invalid corners array.";
		return qMakePair(0.0, 0.0);
	}

	double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;

	for (const QJsonValue& val : corners)
	{
		if (!val.isArray() || val.toArray().size() != 2) {
			qWarning() << "[SkyCoords] Invalid corner format.";
			continue;
		}

		QJsonArray coord = val.toArray();
		double ra  = coord[0].toDouble() * D2R;
		double dec = coord[1].toDouble() * D2R;

		double x = cos(dec) * cos(ra);
		double y = cos(dec) * sin(ra);
		double z = sin(dec);

		sum_x += x;
		sum_y += y;
		sum_z += z;
	}

	if (sum_x == 0.0 && sum_y == 0.0 && sum_z == 0.0) {
		qWarning() << "[SkyCoords] Zero vector sum; cannot compute center.";
		return qMakePair(0.0, 0.0);
	}

	double avg_x = sum_x / N;
	double avg_y = sum_y / N;
	double avg_z = sum_z / N;

	double norm = sqrt(avg_x * avg_x + avg_y * avg_y + avg_z * avg_z);
	if (norm == 0.0) {
		qWarning() << "[SkyCoords] Normalization failed due to zero norm.";
		return qMakePair(0.0, 0.0);
	}

	avg_x /= norm;
	avg_y /= norm;
	avg_z /= norm;

	double ra_center  = fmod(R2D * atan2(avg_y, avg_x) + 360.0, 360.0);
	double dec_center = R2D * asin(avg_z);

	return qMakePair(ra_center, dec_center);
}
