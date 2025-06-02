/**
 * @file StarLine.hpp
 * @author vgerlach, lgrumbach
 * @brief Type describing a line between two stars.
 * @version 0.1
 * @date 2025-06-02
 */
#ifndef SCM_TYPES_STAR_LINE_HPP
#define SCM_TYPES_STAR_LINE_HPP

#include <optional>
#include <QString>

namespace scm
{
//! The pair of optional start and end stars
struct StarLine
{
	//! The start star of the line.
	std::optional<QString> start;

	//! The end star of the line.
	std::optional<QString> end;
};
}  // namespace scm

#endif