/**
 * @file CoordinateLine.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a line between two coordinates.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_COORDINATE_LINE_HPP
#define SCM_TYPES_COORDINATE_LINE_HPP

#include "VecMath.hpp"
#include <QJsonArray>
#include "StelUtils.hpp"

namespace scm
{
//! The pair of start and end coordinate
struct CoordinateLine
{
	//! The start coordinate of the line.
	Vec3d start;

	//! The end coordinate of the line.
	Vec3d end;	
    
    /**
    * @brief Converts the ConstellationLine to a JSON array.
    * 
    * @return QJsonArray The JSON representation of the coordinate line.
    */
	QJsonArray toJson() const
    {
		QJsonArray json;

        // Only if both start and end points do not have names, we save the coordinates
        QJsonArray startCoordinateArray;
        double RA, DE;
        convertToSphereCoords(RA, DE, start);
        startCoordinateArray.append(RA);
        startCoordinateArray.append(DE);
        json.append(startCoordinateArray);

        QJsonArray endCoordinateArray;
        convertToSphereCoords(RA, DE, end);
        endCoordinateArray.append(RA);
        endCoordinateArray.append(DE);
        json.append(endCoordinateArray);

		return json;
	}

private:
    static void convertToSphereCoords(double &RA, double &DE, const Vec3d &vec)
    {
        double longitude;
        double latitude;
        StelUtils::rectToSphe(&longitude, &latitude, vec);
        RA = longitude * M_180_PI / 15.0;
        DE = latitude * M_180_PI;
    }
};
}  // namespace scm

#endif