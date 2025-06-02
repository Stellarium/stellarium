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

namespace scm
{
//! The pair of start and end coordinate
struct CoordinateLine
{
	//! The start coordinate of the line.
	Vec3d start;

	//! The end coordinate of the line.
	Vec3d end;
};
}  // namespace scm

#endif