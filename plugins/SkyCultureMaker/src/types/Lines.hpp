/**
 * @file Lines.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a lines in a constellation.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_LINE_HPP
#define SCM_TYPES_LINE_HPP

#include <vector>
#include "CoordinateLine.hpp" 
#include "StarLine.hpp"

namespace scm
{
//! The lines of the current drawn constellation
struct Lines
{
	//! The coordinate pairs of a line.
	std::vector<CoordinateLine> coordinates;

	//! The optional available stars too the coordinates.
	std::vector<StarLine> stars;
};
}  // namespace scm

#endif