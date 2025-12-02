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
#include "ConstellationMgr.hpp"
#include "StelActionMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
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
		// only allow search and find for normal constellations
		if (drawingMode == DrawingMode::StarsAndDSO)
		{
			selectedStarIsSearched = true;
		}

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

void scm::ScmDraw::appendDrawPoint(const Vec3d &point, const QString &starID)
{
	if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
	{
		currentLine.end = {point, starID};
		drawState       = Drawing::hasEnd;

		drawnLines.push_back(currentLine);
		currentLine.start = {point, starID};
		drawState         = drawState | Drawing::hasStart;
	}
	else
	{
		currentLine.start = {point, starID};
		drawState         = Drawing::hasStart;
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
	QSettings *conf = StelApp::getInstance().getSettings();
	conf->beginGroup("SkyCultureMaker");
	fixedLineColor        = Vec3f(conf->value("fixedLineColor", "1.0,0.5,0.5").toString());
	fixedLineAlpha        = conf->value("fixedLineAlpha", 1.0).toFloat();
	floatingLineColor     = Vec3f(conf->value("floatingLineColor", "1.0,0.7,0.7").toString());
	floatingLineAlpha     = conf->value("floatingLineAlpha", 0.5).toFloat();
	maxSnapRadiusInPixels = conf->value("maxSnapRadiusInPixels", 25).toUInt();
	conf->endGroup();

	ConstellationMgr *constellationMgr = GETSTELMODULE(ConstellationMgr);
	constellationLineThickness         = constellationMgr->getConstellationLineThickness();
	connect(constellationMgr, &ConstellationMgr::constellationLineThicknessChanged, this,
	        [this](int thickness) { constellationLineThickness = thickness; });

	currentLine.start.reset();
	currentLine.end.reset();
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

void scm::ScmDraw::drawLines(StelCore *core) const
{
	StelPainter painter(core->getProjection(drawFrame));

	// set up  painter
	painter.setBlending(true);
	painter.setLineSmooth(true);
	painter.setColor(fixedLineColor, fixedLineAlpha);
	const float scale = painter.getProjector()->getScreenScale();
	if (constellationLineThickness > 1 || scale > 1.f)
	{
		painter.setLineWidth(constellationLineThickness * scale);
	}

	// draw existing lines
	for (ConstellationLine line : drawnLines)
	{
		painter.drawGreatCircleArc(line.start.coordinate, line.end.coordinate);
	}

	// draw line from last point to cursor
	if (hasFlag(drawState, Drawing::hasFloatingEnd))
	{
		painter.setColor(floatingLineColor, floatingLineAlpha);
		painter.drawGreatCircleArc(currentLine.start.coordinate, currentLine.end.coordinate);
	}

	// restore line properties
	if (constellationLineThickness > 1 || scale > 1.f)
	{
		painter.setLineWidth(1); // restore thickness
	}
	painter.setLineSmooth(false);
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
					if ((stelObj->getType() == "Star" || stelObj->getType() == "Nebula") &&
					    !stelObj->getID().trimmed().isEmpty())
					{
						appendDrawPoint(stelObj->getJ2000EquatorialPos(core), stelObj->getID());
						qDebug()
							<< "SkyCultureMaker: Added star/nebula to constellation with ID"
							<< stelObj->getID();
					}
					else if (stelObj->getID().trimmed().isEmpty())
					{
						qDebug() << "SkyCultureMaker: Ignored star/nebula with empty ID";
					}
				}
			}
			else if (drawingMode == DrawingMode::Coordinates)
			{
				StelProjectorP prj = core->getProjection(drawFrame);
				Vec3d point;
				prj->unProject(x, y, point);

				// Snap to nearest point if close enough
				if (auto nearest = findNearestPoint(x, y, prj); nearest.has_value())
				{
					point = nearest->coordinate;
				}
				qDebug() << "SkyCultureMaker: Added point to constellation at"
					 << QString::number(point.v[0]) + "," + QString::number(point.v[1]) + "," +
						    QString::number(point.v[2]);
				appendDrawPoint(point, QString());
			}

			event->accept();
			return;
		}

		// Reset line drawing
		if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonDblClick)
		{
			// this allows the double click to cancel drawing even if hovering over a star
			// or when in coordinate mode
			if (hasFlag(drawState, Drawing::hasEnd))
			{
				if (!drawnLines.empty())
				{
					drawnLines.pop_back();
				}
				drawState = Drawing::None;
			}
			else if (hasFlag(drawState, Drawing::hasFloatingEnd))
			{
				currentLine.start.reset();
				currentLine.end.reset();
				drawState = Drawing::None;
			}

			event->accept();
			return;
		}
	}
	else if (activeTool == DrawTools::Eraser)
	{
		if (event->button() == Qt::RightButton)
		{
			if (event->type() == QEvent::MouseButtonPress)
			{
				lastEraserPos = Vec2d(x, y);
			}
			else if (event->type() == QEvent::MouseButtonRelease)
			{
				lastEraserPos = defaultLastEraserPos;
			}
		}
	}
}

bool scm::ScmDraw::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	StelApp &app   = StelApp::getInstance();
	StelCore *core = app.getCore();

	if (activeTool == DrawTools::Pen)
	{
		Vec3d position(0, 0, 0);

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
				// snap to the star and update the line position
				else
				{
					position = stelObj->getJ2000EquatorialPos(core);
				}
			}
		}

		if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
		{
			// no selection, compute the position from the mouse cursor
			if (position == Vec3d(0, 0, 0))
			{
				StelProjectorP prj = core->getProjection(drawFrame);
				prj->unProject(x, y, position);
			}
			currentLine.end.coordinate = position;
			drawState                  = Drawing::hasFloatingEnd;
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

				for (auto line = drawnLines.begin(); line != drawnLines.end(); ++line)
				{
					Vec3d lineEnd, lineStart;
					prj->project(line->start.coordinate, lineStart);
					prj->project(line->end.coordinate, lineEnd);
					Vec2d lineStart2d(lineStart.v[0], lineStart.v[1]);
					Vec2d lineEnd2d(lineEnd.v[0], lineEnd.v[1]);
					auto lineDirection = lineEnd2d - lineStart2d;

					bool intersect = segmentIntersect(currentPos, mouseDirection, lineStart2d,
					                                  lineDirection);
					if (intersect)
					{
						erasedIndices.push_back(std::distance(drawnLines.begin(), line));
					}
				}

				for (auto index : erasedIndices)
				{
					drawnLines[index] = drawnLines.back();
					drawnLines.pop_back();
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
	if (!drawnLines.empty())
	{
		currentLine = drawnLines.back();
		drawnLines.pop_back();
		drawState = Drawing::hasFloatingEnd;
	}
	else
	{
		drawState = Drawing::None;
	}
}

std::vector<scm::ConstellationLine> scm::ScmDraw::getConstellationLines() const
{
	return drawnLines;
}

void scm::ScmDraw::loadLines(const std::vector<ConstellationLine> &lines)
{
	drawnLines = lines;

	if (!drawnLines.empty())
	{
		currentLine = drawnLines.back();
		drawState   = Drawing::hasFloatingEnd;
	}
	else
	{
		currentLine = ConstellationLine();
		drawState   = Drawing::None;
	}
}

void scm::ScmDraw::setTool(scm::DrawTools tool)
{
	activeTool    = tool;
	lastEraserPos = defaultLastEraserPos;
	drawState     = Drawing::None;
}

std::optional<scm::SkyPoint> scm::ScmDraw::findNearestPoint(int x, int y, StelProjectorP prj) const
{
	if (drawnLines.empty())
	{
		return {};
	}

	auto minLine = drawnLines.begin();
	Vec3d position(x, y, 0);
	Vec3d minPosition;
	prj->project(minLine->start.coordinate, minPosition);
	double minDistance = (minPosition - position).dot(minPosition - position);
	bool isStartPoint  = true;

	for (auto line = drawnLines.begin(); line != drawnLines.end(); ++line)
	{
		Vec3d iPosition;
		if (prj->project(line->start.coordinate, iPosition))
		{
			double distance = (iPosition - position).dot(iPosition - position);
			if (distance < minDistance)
			{
				minLine      = line;
				minPosition  = iPosition;
				minDistance  = distance;
				isStartPoint = true;
			}
		}

		if (prj->project(line->end.coordinate, iPosition))
		{
			double distance = (iPosition - position).dot(iPosition - position);
			if (distance < minDistance)
			{
				minLine      = line;
				minPosition  = iPosition;
				minDistance  = distance;
				isStartPoint = false;
			}
		}
	}

	if (minDistance < maxSnapRadiusInPixels * maxSnapRadiusInPixels)
	{
		return isStartPoint ? minLine->start : minLine->end;
	}

	return {};
}

void scm::ScmDraw::resetDrawing()
{
	drawnLines.clear();
	drawState     = Drawing::None;
	lastEraserPos = defaultLastEraserPos;
	activeTool    = DrawTools::None;
	drawingMode   = DrawingMode::StarsAndDSO;
	currentLine.start.reset();
	currentLine.end.reset();
}
