/**
 * @file Drawing.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a the possible state during drawing a constellation.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_DRAWING_HPP
#define SCM_TYPES_DRAWING_HPP

namespace scm
{
//! The possibles states during the drawing.
enum class Drawing
{
	//! No line is available.
	None = 1,

	//! The line as a starting point.
	hasStart = 2,

	//! The line has a not placed end that is attached to the cursor.
	hasFloatingEnd = 4,

	//! The line is complete i.e. has start and end point.
	hasEnd = 8,

	//! The end is an already existing point.
	hasEndExistingPoint = 16,
};
} // namespace scm

#endif
