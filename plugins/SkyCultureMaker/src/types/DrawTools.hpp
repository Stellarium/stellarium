/**
 * @file StarLine.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing the possible states of the draw tool.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_DRAWTOOLS_HPP
#define SCM_TYPES_DRAWTOOLS_HPP

namespace scm
{
//! The possibles tools used for drawing.
enum class DrawTools
{
	//! No tool is active.
	None,

	//! The pen tool is selected.
	Pen,

	//! The eraser tool is selected.
	Eraser,
};
}  // namespace scm

#endif