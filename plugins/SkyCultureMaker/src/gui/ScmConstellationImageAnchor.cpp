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

#include "ScmConstellationImageAnchor.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include <QBrush>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QRegularExpression>

void ScmConstellationImageAnchor::setSelectionReference(ScmConstellationImageAnchor *&anchor)
{
	selection = &anchor;
}

void ScmConstellationImageAnchor::select()
{
	if (selection == nullptr)
	{
		return;
	}

	if (*selection != nullptr)
	{
		(*selection)->deselect();
	}

	isSelected = true;
	*selection = this;
	updateColor();

	if (selectionChangedCallback != nullptr)
	{
		selectionChangedCallback();
	}

	// select the bound star
	if (hip != 0)
	{
		StelApp &app             = StelApp::getInstance();
		StelObjectMgr &objectMgr = app.getStelObjectMgr();
		objectMgr.findAndSelect(QString("HIP ") + std::to_string(hip).c_str());
	}
}

void ScmConstellationImageAnchor::deselect()
{
	isSelected = false;
	updateColor();
}

void ScmConstellationImageAnchor::setSelectionChangedCallback(std::function<void()> func)
{
	selectionChangedCallback = func;
}

void ScmConstellationImageAnchor::setPositionChangedCallback(std::function<void()> func)
{
	positionChangedCallback = func;
}

void ScmConstellationImageAnchor::setMovementBounds(const QRectF &bounds)
{
	movementBound = bounds;
}

void ScmConstellationImageAnchor::setStarHip(StarId hip)
{
	ScmConstellationImageAnchor::hip = hip;
}

bool ScmConstellationImageAnchor::trySetStarHip(const QString &id)
{
	QRegularExpression hipExpression(R"(HIP\s+(\d+))");

	QRegularExpressionMatch hipMatch = hipExpression.match(id);
	if (hipMatch.hasMatch())
	{
		setStarHip(hipMatch.captured(1).toInt());
		updateColor();
		return true;
	}

	return false;
}

const StarId &ScmConstellationImageAnchor::getStarHip() const
{
	return hip;
}

void ScmConstellationImageAnchor::updateColor()
{
	if (isSelected == true)
	{
		setBrush(hip == 0 ? selectedColorNoStar : selectedColor);
	}
	else
	{
		setBrush(hip == 0 ? colorNoStar : color);
	}
}

Vec2i ScmConstellationImageAnchor::getPosition() const
{
	auto position     = pos();
	auto origin       = movementBound.topLeft();
	auto anchorRadius = rect().size() * 0.5f;
	// offset by the origin of the texture so we have exact pixel mapping starting in the top left corner.
	return Vec2i(static_cast<int>(position.x() - origin.x() + anchorRadius.width()),
	             static_cast<int>(position.y() - origin.y() + anchorRadius.height()));
}

void ScmConstellationImageAnchor::setPosition(const Vec2i &position)
{
	auto origin       = movementBound.topLeft();
	auto anchorRadius = rect().size() * 0.5f;

	qreal x = position[0] - anchorRadius.width() + origin.x();
	qreal y = position[1] - anchorRadius.height() + origin.y();

	setPos(x, y);
}

void ScmConstellationImageAnchor::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);

	if (event->button() == Qt::MouseButton::LeftButton)
	{
		select();
	}
}

void ScmConstellationImageAnchor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
}

void ScmConstellationImageAnchor::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseMoveEvent(event);

	if (isSelected == false)
	{
		return;
	}

	if (movementBound.isEmpty())
	{
		return;
	}

	// Pos (x,y) is the upper left corner
	qreal new_x = x();
	qreal new_y = y();
	if (new_x < movementBound.left())
	{
		new_x = movementBound.left();
	}
	else if (new_x + rect().width() > movementBound.right())
	{
		new_x = movementBound.right() - rect().width();
	}

	if (new_y < movementBound.top())
	{
		new_y = movementBound.top();
	}
	else if (new_y + rect().height() > movementBound.bottom())
	{
		new_y = movementBound.bottom() - rect().height();
	}

	setPos(new_x, new_y);

	if (positionChangedCallback != nullptr)
	{
		positionChangedCallback();
	}
}

ScmConstellationImageAnchor::ScmConstellationImageAnchor()
	: QGraphicsEllipseItem()
{
	setFlag(ItemIsMovable, true);
	setBrush(QBrush(colorNoStar));
	setZValue(10);
}

ScmConstellationImageAnchor::ScmConstellationImageAnchor(QPointF position, qreal diameter)
	: QGraphicsEllipseItem(position.x(), position.y(), diameter, diameter)
{
	setFlag(ItemIsMovable, true);
	setBrush(QBrush(colorNoStar));
	setZValue(10);
}

ScmConstellationImageAnchor::~ScmConstellationImageAnchor()
{
	qDebug() << "SkyCultureMaker: Unloaded the ScmConstellationImageAnchor";
}

void ScmConstellationImageAnchor::setDiameter(qreal diameter)
{
	QPointF position = pos();
	setPosDiameter(position.x(), position.y(), diameter);
}

void ScmConstellationImageAnchor::setPosDiameter(qreal x, qreal y, qreal diameter)
{
	setRect(x, y, diameter, diameter);
	setPos(x, y);
}
