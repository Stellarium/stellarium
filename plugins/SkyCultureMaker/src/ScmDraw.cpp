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

#include "ScmDraw.hpp"
#include "StelActionMgr.hpp"
#include "StelModule.hpp"
#include "StelMovementMgr.hpp"
#include "StelProjector.hpp"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWidget>

void scm::ScmDraw::setSearchMode(bool active)
{
	// search mode deactivates before the star is set by the search
	if (inSearchMode == true && active == false)
	{
		selectedStarIsSearched = true;

		// HACK an Ctrl + Release is not triggered if Ctrl + F is trigger it manually
		QKeyEvent release = QKeyEvent(QEvent::KeyRelease, Qt::Key_Control, Qt::NoModifier);
		QWidget *mainView = qApp->activeWindow();

		if (mainView)
		{
			QApplication::sendEvent(mainView, &release);
		}
		else
		{
			qDebug() << "SkyCultureMaker: Failed to release Ctrl key";
		}
	}

	inSearchMode = active;
}

void scm::ScmDraw::appendDrawPoint(const Vec3d &point, const std::optional<QString> &starID)
{
	if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
	{
		std::get<CoordinateLine>(currentLine).end = point;
		std::get<StarLine>(currentLine).end       = starID;
		drawState                                 = Drawing::hasEnd;

		drawnLines.coordinates.push_back(std::get<CoordinateLine>(currentLine));
		drawnLines.stars.push_back(std::get<StarLine>(currentLine));
		std::get<CoordinateLine>(currentLine).start = point;
		std::get<StarLine>(currentLine).start       = starID;
		drawState                                   = drawState | Drawing::hasStart;
	}
	else
	{
		std::get<CoordinateLine>(currentLine).start = point;
		std::get<StarLine>(currentLine).start       = starID;
		drawState                                   = Drawing::hasStart;
	}
}

void scm::ScmDraw::setMoveToAnotherStart()
{
	if (selectedStarIsSearched == true)
	{
		if (activeTool == DrawTools::Pen)
		{
			StelApp &app   = StelApp::getInstance();
			StelCore *core = app.getCore();

			StelObjectMgr &objectMgr = app.getStelObjectMgr();

			if (objectMgr.getWasSelected())
			{
				StelObjectP stelObj = objectMgr.getLastSelectedObject();
				Vec3d stelPos       = stelObj->getJ2000EquatorialPos(core);
				appendDrawPoint(stelPos, stelObj->getID());
			}
		}

		selectedStarIsSearched = false;
	}
}

const Vec2d scm::ScmDraw::defaultLastEraserPos(std::nan("1"), std::nan("1"));

bool scm::ScmDraw::segmentIntersect(const Vec2d &startA, const Vec2d &directionA, const Vec2d &startB,
                                    const Vec2d &directionB)
{
	if (std::abs(directionA.dot(directionB)) < std::numeric_limits<double>::epsilon()) // check with near zero value
	{
		// No intersection if lines are parallel
		return false;
	}

	// Also see: https://www.sunshine2k.de/coding/javascript/lineintersection2d/LineIntersect2D.html
	// endA = startA + s * directionA with s=1
	double s = perpDot(directionB, startB - startA) / perpDot(directionB, directionA);
	// endB = startB + t * directionB with t=1
	double t = perpDot(directionA, startA - startB) / perpDot(directionA, directionB);

	return 0 <= s && s <= 1 && 0 <= t && t <= 1;
}

scm::ScmDraw::ScmDraw()
	: drawState(Drawing::None)
	, drawingMode(DrawingMode::StarsAndDSO)
{
	std::get<CoordinateLine>(currentLine).start.set(0, 0, 0);
	std::get<CoordinateLine>(currentLine).end.set(0, 0, 0);
	lastEraserPos.set(std::nan("1"), std::nan("1"));

	StelApp &app   = StelApp::getInstance();
	StelCore *core = app.getCore();
	maxSnapRadiusInPixels *= core->getCurrentStelProjectorParams().devicePixelsPerPixel;

	StelActionMgr *actionMgr = app.getStelActionManager();
	auto action              = actionMgr->findAction(id_search_window);
	connect(action, &StelAction::toggled, this, &ScmDraw::setSearchMode);

	StelMovementMgr *mvmMgr = core->getMovementMgr();
	connect(mvmMgr, &StelMovementMgr::flagTrackingChanged, this, &ScmDraw::setMoveToAnotherStart);
}

void scm::ScmDraw::drawLine(StelCore *core) const
{
	StelPainter painter(core->getProjection(drawFrame));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	Vec3f color = {1.f, 0.5f, 0.5f};
	bool alpha  = 1.0f;
	painter.setColor(color, alpha);

	for (CoordinateLine p : drawnLines.coordinates)
	{
		painter.drawGreatCircleArc(p.start, p.end);
	}

	if (hasFlag(drawState, Drawing::hasFloatingEnd))
	{
		color = {1.f, 0.7f, 0.7f};
		painter.setColor(color, 0.5f);
		painter.drawGreatCircleArc(std::get<CoordinateLine>(currentLine).start,
		                           std::get<CoordinateLine>(currentLine).end);
	}
}

void scm::ScmDraw::handleMouseClicks(class QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		// do not interfere with navigation and draw or erase anything
		isNavigating |= (event->type() == QEvent::MouseButtonPress);
		isNavigating &= (event->type() != QEvent::MouseButtonRelease);

		return;
	}

	if (isNavigating)
	{
		return;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	qreal x = event->position().x(), y = event->position().y();
#else
	qreal x = event->x(), y = event->y();
#endif

	if (activeTool == DrawTools::Pen)
	{
		// Draw line
		if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonPress)
		{
			StelApp &app   = StelApp::getInstance();
			StelCore *core = app.getCore();

			if (drawingMode == DrawingMode::StarsAndDSO)
			{
				StelObjectMgr &objectMgr = app.getStelObjectMgr();
				if (objectMgr.getWasSelected())
				{
					StelObjectP stelObj = objectMgr.getLastSelectedObject();
					if (stelObj->getType() == "Star" || stelObj->getType() == "Nebula")
					{
						QString stelObjID = stelObj->getID();
						if (stelObjID.trimmed().isEmpty())
						{
							qDebug() << "SkyCultureMaker: Ignored sky object with empty ID";
						}
						else
						{
							appendDrawPoint(stelObj->getJ2000EquatorialPos(core), stelObjID);
							qDebug() << "SkyCultureMaker: Added sky object to "
								    "constellation with ID "
								 << stelObjID;
						}
					}
				}
			}
			else if (drawingMode == DrawingMode::Coordinates)
			{
				StelProjectorP prj = core->getProjection(drawFrame);
				Vec3d point;
				prj->unProject(x, y, point);

				// We want to combine any near start point to an existing point so that we don't create
				// duplicates.
				std::optional<StarPoint> nearest = findNearestPoint(x, y, prj);
				if (nearest.has_value())
				{
					point = nearest.value().coordinate;
				}
				appendDrawPoint(point, std::nullopt);
			}

			event->accept();
			return;
		}

		// Reset line drawing
		if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonDblClick &&
		    hasFlag(drawState, Drawing::hasEnd))
		{
			if (!drawnLines.coordinates.empty())
			{
				drawnLines.coordinates.pop_back();
				drawnLines.stars.pop_back();
			}
			drawState = Drawing::None;
			event->accept();
			return;
		}
	}
	else if (activeTool == DrawTools::Eraser)
	{
		if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonPress)
		{
			Vec2d currentPos(x, y);
			lastEraserPos = currentPos;
		}
		else if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonRelease)
		{
			// Reset
			lastEraserPos = defaultLastEraserPos;
		}
	}
}

bool scm::ScmDraw::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	StelApp &app   = StelApp::getInstance();
	StelCore *core = app.getCore();

	if (activeTool == DrawTools::Pen)
	{
		if (drawingMode == DrawingMode::StarsAndDSO)
		{
			// this wouldve been easier with cleverFind but that is private
			StelObjectMgr &objectMgr = app.getStelObjectMgr();
			bool found               = objectMgr.findAndSelect(core, x, y);
			if (found && objectMgr.getWasSelected())
			{
				StelObjectP stelObj = objectMgr.getLastSelectedObject();
				// only keep the selection if a star or nebula was selected
				if (stelObj->getType() != "Star" && stelObj->getType() != "Nebula")
				{
					objectMgr.unSelect();
				}
				// also unselect if the id is empty or only whitespace
				else if (stelObj->getID().trimmed().isEmpty())
				{
					objectMgr.unSelect();
				}
			}
		}

		// draw a floating line in any mode
		if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
		{
			StelProjectorP prj = core->getProjection(drawFrame);
			Vec3d position;
			prj->unProject(x, y, position);
			std::get<CoordinateLine>(currentLine).end = position;
			drawState                                 = Drawing::hasFloatingEnd;
		}
	}
	else if (activeTool == DrawTools::Eraser)
	{
		if (b.testFlag(Qt::MouseButton::RightButton))
		{
			Vec2d currentPos(x, y);

			if (lastEraserPos != defaultLastEraserPos && lastEraserPos != currentPos)
			{
				StelApp &app        = StelApp::getInstance();
				StelCore *core      = app.getCore();
				StelProjectorP prj  = core->getProjection(drawFrame);
				auto mouseDirection = lastEraserPos - currentPos;

				std::vector<int> erasedIndices;

				for (auto line = drawnLines.coordinates.begin(); line != drawnLines.coordinates.end();
				     ++line)
				{
					Vec3d lineEnd, lineStart;
					prj->project(line->start, lineStart);
					prj->project(line->end, lineEnd);
					Vec2d lineStart2d(lineStart.v[0], lineStart.v[1]);
					Vec2d lineEnd2d(lineEnd.v[0], lineEnd.v[1]);
					auto lineDirection = lineEnd2d - lineStart2d;

					bool intersect = segmentIntersect(currentPos, mouseDirection, lineStart2d,
					                                  lineDirection);
					if (intersect)
					{
						erasedIndices.push_back(
							std::distance(drawnLines.coordinates.begin(), line));
					}
				}

				for (auto index : erasedIndices)
				{
					drawnLines.coordinates[index] = drawnLines.coordinates.back();
					drawnLines.coordinates.pop_back();
				}
			}

			lastEraserPos = currentPos;
		}
	}

	// We always return false as we still want to navigate in Stellarium with left mouse button
	return false;
}

void scm::ScmDraw::handleKeys(QKeyEvent *e)
{
	if (activeTool == DrawTools::Pen)
	{
		if (e->key() == Qt::Key::Key_Z && e->modifiers() == Qt::Modifier::CTRL)
		{
			undoLastLine();
			e->accept();
		}
	}
}

void scm::ScmDraw::undoLastLine()
{
	if (!drawnLines.coordinates.empty())
	{
		currentLine = std::make_tuple(drawnLines.coordinates.back(), drawnLines.stars.back());
		drawnLines.coordinates.pop_back();
		drawnLines.stars.pop_back();
		drawState = Drawing::hasFloatingEnd;
	}
	else
	{
		drawState = Drawing::None;
	}
}

std::vector<scm::StarLine> scm::ScmDraw::getStars() const
{
	bool all_stars = std::all_of(drawnLines.stars.begin(), drawnLines.stars.end(), [](const StarLine &star)
	                             { return star.start.has_value() && star.end.has_value(); });

	if (all_stars)
	{
		return drawnLines.stars;
	}

	return std::vector<StarLine>();
}

std::vector<scm::CoordinateLine> scm::ScmDraw::getCoordinates() const
{
	return drawnLines.coordinates;
}

void scm::ScmDraw::loadLines(const std::vector<CoordinateLine> &coordinates, const std::vector<StarLine> &stars)
{
	// copy the coordinates and stars to drawnLines
	drawnLines.coordinates = coordinates;
	drawnLines.stars       = stars;

	if (!drawnLines.coordinates.empty())
	{
		currentLine = std::make_tuple(drawnLines.coordinates.back(), drawnLines.stars.back());
		drawState   = Drawing::hasFloatingEnd;
	}
	else
	{
		currentLine = std::make_tuple(CoordinateLine(), StarLine());
		drawState   = Drawing::None;
	}
}

void scm::ScmDraw::setTool(scm::DrawTools tool)
{
	activeTool    = tool;
	lastEraserPos = defaultLastEraserPos;
	drawState     = Drawing::None;
}

std::optional<scm::StarPoint> scm::ScmDraw::findNearestPoint(int x, int y, StelProjectorP prj) const
{
	if (drawnLines.coordinates.empty())
	{
		return {};
	}

	auto min = drawnLines.coordinates.begin();
	Vec3d position(x, y, 0);
	Vec3d minPosition;
	prj->project(min->start, minPosition);
	double minDistance = (minPosition - position).dot(minPosition - position);
	bool isStartPoint  = true;

	for (auto line = drawnLines.coordinates.begin(); line != drawnLines.coordinates.end(); ++line)
	{
		Vec3d iPosition;
		if (prj->project(line->start, iPosition))
		{
			double distance = (iPosition - position).dot(iPosition - position);
			if (distance < minDistance)
			{
				min          = line;
				minPosition  = iPosition;
				minDistance  = distance;
				isStartPoint = true;
			}
		}

		if (prj->project(line->end, iPosition))
		{
			double distance = (iPosition - position).dot(iPosition - position);
			if (distance < minDistance)
			{
				min          = line;
				minPosition  = iPosition;
				minDistance  = distance;
				isStartPoint = false;
			}
		}
	}

	if (minDistance < maxSnapRadiusInPixels * maxSnapRadiusInPixels)
	{
		if (isStartPoint)
		{
			StarPoint point = {min->start,
			                   drawnLines.stars.at(std::distance(drawnLines.coordinates.begin(), min)).start};
			return point;
		}
		else
		{
			StarPoint point = {min->end,
			                   drawnLines.stars.at(std::distance(drawnLines.coordinates.begin(), min)).end};
			return point;
		}
	}

	return {};
}

void scm::ScmDraw::resetDrawing()
{
	drawnLines.coordinates.clear();
	drawnLines.stars.clear();
	drawState     = Drawing::None;
	lastEraserPos = defaultLastEraserPos;
	activeTool    = DrawTools::None;
	drawingMode   = DrawingMode::StarsAndDSO;
	std::get<CoordinateLine>(currentLine).start.set(0, 0, 0);
	std::get<CoordinateLine>(currentLine).end.set(0, 0, 0);
	std::get<StarLine>(currentLine).start.reset();
	std::get<StarLine>(currentLine).end.reset();
}
