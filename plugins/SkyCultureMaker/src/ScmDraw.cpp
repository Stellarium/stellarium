#include "ScmDraw.hpp"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>
#include "StelModule.hpp"
#include "StelProjector.hpp"

scm::ScmDraw::ScmDraw()
	: drawState(Drawing::None)
	, snapToStar(true)
{
	std::get<CoordinateLine>(currentLine).start.set(0, 0, 0);
	std::get<CoordinateLine>(currentLine).end.set(0, 0, 0);

	StelCore *core = StelApp::getInstance().getCore();
	maxSnapRadiusInPixels *= core->getCurrentStelProjectorParams().devicePixelsPerPixel;
}

void scm::ScmDraw::drawLine(StelCore *core)
{
	StelPainter painter(core->getProjection(drawFrame));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	Vec3f color = {1.f, 0.5f, 0.5f};
	bool alpha = 1.0f;
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
			StelApp &app = StelApp::getInstance();
			StelCore *core = app.getCore();
			StelProjectorP prj = core->getProjection(drawFrame);
			Vec3d point;
			std::optional<QString> starID;
			prj->unProject(x, y, point);

			// We want to combine any near start point to an existing point so that we don't create
			// duplicates.
			std::optional<StarPoint> nearest = findNearestPoint(x, y, prj);
			if (nearest.has_value())
			{
				point = nearest.value().coordinate;
				starID = nearest.value().star;
			}
			else if (snapToStar)
			{
				if (hasFlag(drawState, Drawing::hasEndExistingPoint))
				{
					point = std::get<CoordinateLine>(currentLine).end;
					starID = std::get<StarLine>(currentLine).end;
				}
				else
				{
					StelObjectMgr &objectMgr = app.getStelObjectMgr();

					objectMgr.findAndSelect(core, x, y);
					if (objectMgr.getWasSelected())
					{
						StelObjectP stelObj = objectMgr.getLastSelectedObject();
						Vec3d stelPos = stelObj->getJ2000EquatorialPos(core);
						point = stelPos;
						starID = stelObj->getID();
					}
				}
			}

			if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
			{
				std::get<CoordinateLine>(currentLine).end = point;
				std::get<StarLine>(currentLine).end = starID;
				drawState = Drawing::hasEnd;

				drawnLines.coordinates.push_back(std::get<CoordinateLine>(currentLine));
				drawnLines.stars.push_back(std::get<StarLine>(currentLine));
				std::get<CoordinateLine>(currentLine).start = point;
				std::get<StarLine>(currentLine).start = starID;
				drawState = Drawing::hasStart;
			}
			else
			{
				std::get<CoordinateLine>(currentLine).start = point;
				std::get<StarLine>(currentLine).start = starID;
				drawState = Drawing::hasStart;
			}

			event->accept();
			return;
		}

		// Reset line drawing
		// Also works as a Undo feature.
		if (event->button() == Qt::RightButton && event->type() == QEvent::MouseButtonDblClick)
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
}

bool scm::ScmDraw::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	StelApp &app = StelApp::getInstance();
	StelCore *core = app.getCore();

	if (activeTool == DrawTools::Pen)
	{
		if (snapToStar)
		{
			StelObjectMgr &objectMgr = app.getStelObjectMgr();
			objectMgr.findAndSelect(core, x, y);
		}

		if (hasFlag(drawState, (Drawing::hasStart | Drawing::hasFloatingEnd)))
		{
			StelProjectorP prj = core->getProjection(drawFrame);
			Vec3d position;
			prj->unProject(x, y, position);
			if (snapToStar)
			{
				StelObjectMgr &objectMgr = app.getStelObjectMgr();
				if (objectMgr.getWasSelected())
				{
					StelObjectP stelObj = objectMgr.getLastSelectedObject();
					Vec3d stelPos = stelObj->getJ2000EquatorialPos(core);
					std::get<CoordinateLine>(currentLine).end = stelPos;
				}
				else
				{
					std::get<CoordinateLine>(currentLine).end = position;
				}
			}
			else
			{
				std::get<CoordinateLine>(currentLine).end = position;
			}

			drawState = Drawing::hasFloatingEnd;
		}
	}

	// We always return false as we still want to navigate in Stellarium with left mouse button
	return false;
}

void scm::ScmDraw::handleKeys(QKeyEvent *e)
{
	if (activeTool == DrawTools::Pen)
	{
		if (e->key() == Qt::Key::Key_Control)
		{
			snapToStar = e->type() != QEvent::KeyPress;

			e->accept();
		}

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

std::vector<scm::StarLine> scm::ScmDraw::getStars()
{
	bool all_stars =
	    std::all_of(drawnLines.stars.begin(),
			drawnLines.stars.end(),
			[](const StarLine &star) { return star.start.has_value() && star.end.has_value(); });

	if (all_stars)
	{
		return drawnLines.stars;
	}

	return std::vector<StarLine>();
}

std::vector<scm::CoordinateLine> scm::ScmDraw::getCoordinates()
{
	return drawnLines.coordinates;
}

void scm::ScmDraw::setTool(scm::DrawTools tool)
{
	activeTool = tool;
}

std::optional<scm::StarPoint> scm::ScmDraw::findNearestPoint(int x, int y, StelProjectorP prj)
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

	for (auto line = drawnLines.coordinates.begin(); line != drawnLines.coordinates.end(); ++line)
	{
		Vec3d iPosition;
		if (!prj->project(line->start, iPosition))
		{
			continue;
		}

		double distance = (iPosition - position).dot(iPosition - position);
		if (distance < minDistance)
		{
			min = line;
			minPosition = iPosition;
			minDistance = distance;
		}
	}

	if (minDistance < maxSnapRadiusInPixels * maxSnapRadiusInPixels)
	{
		StarPoint point = {min->start,
				   drawnLines.stars.at(std::distance(drawnLines.coordinates.begin(), min)).start};
		return point;
	}

	return {};
}
