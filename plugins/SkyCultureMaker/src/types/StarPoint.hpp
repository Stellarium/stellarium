/**
 * @file StarPoint.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a single star point.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_STAR_POINT_HPP
#define SCM_TYPES_STAR_POINT_HPP

#include "VecMath.hpp"
#include <optional>
#include <QString>

namespace scm
{
//! The point of a single star with coordinates.
struct StarPoint
{
	//! The coordinate of a single point.
	Vec3d coordinate;

	//! The optional star at that coordinate.
	std::optional<QString> star;
};
} // namespace scm

#endif
