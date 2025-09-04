/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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

#ifndef SCMDRAW_H
#define SCMDRAW_H

#include "StelCore.hpp"
#include "StelObjectMgr.hpp"
#include "StelObjectType.hpp"
#include "enumBitops.hpp"
#include "types/CoordinateLine.hpp"
#include "types/DrawTools.hpp"
#include "types/DrawingMode.hpp"
#include "types/Drawing.hpp"
#include "types/Lines.hpp"
#include "types/StarLine.hpp"
#include "types/StarPoint.hpp"
#include <cmath>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>
#include <QObject>
#include <QString>

namespace scm
{

class ScmDraw : public QObject
{
private:
	static constexpr const char id_search_window[] = "actionShow_Search_Window_Global";
	static const Vec2d defaultLastEraserPos;

	/// The search radius to attach to a point on a existing line.
	uint32_t maxSnapRadiusInPixels = 25;

	/// Indicates that the startPoint has been set.
	Drawing drawState = Drawing::None;

	/// The current drawing mode.
	DrawingMode drawingMode = DrawingMode::StarsAndDSO;

	/// The current pending point.
	std::tuple<CoordinateLine, StarLine> currentLine;

	/// The fixed points.
	Lines drawnLines;

	/// The current active tool.
	DrawTools activeTool = DrawTools::None;

	/// Indicates if the user is navigating in stellarium i.e. changing position of camera
	bool isNavigating = false;

	/// Indicates if the users searches for a star.
	bool inSearchMode = false;

	/// Indicates if the currently selected star was searched.
	bool selectedStarIsSearched = false;

	/// Holds the position of the eraser on the last frame.
	Vec2d lastEraserPos = ScmDraw::defaultLastEraserPos;

	/**
	 * @brief Appends a draw point to the list of drawn points.
	 * 
	 * @param point The coordinate in J2000 frame.
	 * @param starID The id of the star to use.
	 */
	void appendDrawPoint(const Vec3d &point, const std::optional<QString> &starID);

	/**
	 * @brief Indicates if two segments intersect.
	 * 
	 * @param startA The start point of A.
	 * @param directionA The direction vector of A pointing to the end point of A.
	 * @param startB The start point of B.
	 * @param directionB The direction vector of B pointing to the end point of B.
	 * @return true When both segments intersect.
	 * @return false When both segments do NOT intersect.
	 */
	static bool segmentIntersect(const Vec2d &startA, const Vec2d &directionA, const Vec2d &startB,
	                             const Vec2d &directionB);

	/**
	 * @brief Calculates the perpendicular dot product vector of a and b i.e. a^T dot b
	 * 
	 * @tparam T The type of the vector
	 * @param a The first vector.
	 * @param b The second vector.
	 * @return T The perp dot product of a and b.
	 */
	template<typename T>
	static T perpDot(const Vector2<T> &a, const Vector2<T> &b)
	{
		return -a.v[1] * b.v[0] + a.v[0] * b.v[1];
	}

public slots:
	/**
   * @brief Is called when the search dialog is opend and closed.
  */
	void setSearchMode(bool b);

	/**
	 * @brief Is called when the the user is moved to another star.
	 */
	void setMoveToAnotherStart();

public:
	/// The frame that is used for calculation and is drawn on.
	static const StelCore::FrameType drawFrame = StelCore::FrameJ2000;

	ScmDraw();

	/**
	 * @brief Draws the line between the start and the current end point.
	 *
	 * @param core The core used for drawing the line.
	 */
	void drawLine(StelCore *core) const;

	/// Handle mouse clicks. Please note that most of the interactions will be done through the GUI module.
	/// @return set the event as accepted if it was intercepted
	void handleMouseClicks(QMouseEvent *);

	/// Handle mouse moves. Please note that most of the interactions will be done through the GUI module.
	/// @return true if the event was intercepted
	bool handleMouseMoves(int x, int y, Qt::MouseButtons b);

	/// Handle key events. Please note that most of the interactions will be done through the GUI module.
	/// @param e the Key event
	/// @return set the event as accepted if it was intercepted
	void handleKeys(QKeyEvent *e);

	/**
	 * @brief Finds the nearest star point to the given position.
	 *
	 * @param x The x viewport coordinate of the mouse.
	 * @param y The y viewport coordinate of the mouse.
	 * @param prj The projector to use for the calculation.
	 * @return std::optional<StarPoint> The found star point if available.
	 */
	std::optional<StarPoint> findNearestPoint(int x, int y, StelProjectorP prj) const;

	/// Undo the last drawn line.
	void undoLastLine();

	/**
	 * @brief Get the drawn stick figures as stars if available.
	 *
	 * @return std::vector<StarLine> The optional filled vector of stars matching the coordinates.
	 */
	std::vector<StarLine> getStars() const;

	/**
	 * @brief Get the drawn stick figures as coordinates.
	 *
	 * @return std::vector<CoordinateLine> The drawn coordinates.
	 */
	std::vector<CoordinateLine> getCoordinates() const;

	/**
	 * @brief Loads lines into the buffer from a tuple of coordinates and stars.
	 *
	 */
	void loadLines(const std::vector<CoordinateLine>& coordinates, const std::vector<StarLine>& stars);

	/**
	 * @brief Set the active draw tool
	 *
	 * @param tool The tool to be used.
	 */
	void setTool(DrawTools tool);

	/**
	 * @brief Resets the currently drawn lines.
	 */
	void resetDrawing();
};

} // namespace scm

// Opt In for Drawing to use bitops & and |
template<>
struct generic_enum_bitops::allow_bitops<scm::Drawing>
{
	static constexpr bool value = true;
};

#endif // SCMDRAW_H
