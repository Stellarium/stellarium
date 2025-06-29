#include "ScmImageAnchor.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include <QBrush>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QRegularExpression>

void ScmImageAnchor::setSelectionReference(ScmImageAnchor *&anchor)
{
	selection = &anchor;
}

void ScmImageAnchor::select()
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
	if (!starNameI18n.isEmpty())
	{
		StelApp &app             = StelApp::getInstance();
		StelObjectMgr &objectMgr = app.getStelObjectMgr();
		objectMgr.findAndSelectI18n(starNameI18n);
	}
}

void ScmImageAnchor::deselect()
{
	isSelected = false;
	updateColor();
}

void ScmImageAnchor::setSelectionChangedCallback(std::function<void()> func)
{
	selectionChangedCallback = func;
}

void ScmImageAnchor::setPositionChangedCallback(std::function<void()> func)
{
	positionChangedCallback = func;
}

void ScmImageAnchor::setMovementBounds(const QRectF &bounds)
{
	movementBound = bounds;
}

void ScmImageAnchor::setStarNameI18n(const QString &starNameI18n)
{
	ScmImageAnchor::starNameI18n = starNameI18n;
	updateColor();
}

const QString &ScmImageAnchor::getStarNameI18n() const
{
	return starNameI18n;
}

void ScmImageAnchor::setStarHip(StarId hip)
{
	ScmImageAnchor::hip = hip;
}

bool ScmImageAnchor::trySetStarHip(const QString &id)
{
	QRegularExpression hipExpression(R"(HIP\s+(\d+))");

	QRegularExpressionMatch hipMatch = hipExpression.match(id);
	if (hipMatch.hasMatch())
	{
		setStarHip(hipMatch.captured(1).toInt());
		return true;
	}

	return false;
}

const StarId &ScmImageAnchor::getStarHip() const
{
	return hip;
}

void ScmImageAnchor::updateColor()
{
	if (isSelected == true)
	{
		setBrush(starNameI18n.isEmpty() ? selectedColorNoStar : selectedColor);
	}
	else
	{
		setBrush(starNameI18n.isEmpty() ? colorNoStar : color);
	}
}

Vec2i ScmImageAnchor::getPosition() const
{
	auto position     = pos();
	auto origin       = movementBound.topLeft();
	auto anchorRadius = rect().size() * 0.5f;
	// offset by the origin of the texture so we have exact pixel mapping starting in the top left corner.
	return Vec2i(static_cast<int>(position.x() - origin.x() + anchorRadius.width()),
	             static_cast<int>(position.y() - origin.y() + anchorRadius.height()));
}

void ScmImageAnchor::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);

	if (event->button() == Qt::MouseButton::LeftButton)
	{
		select();
	}
}

void ScmImageAnchor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
}

void ScmImageAnchor::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
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

ScmImageAnchor::ScmImageAnchor()
	: QGraphicsEllipseItem()
{
	setFlag(ItemIsMovable, true);
	setBrush(QBrush(colorNoStar));
	setZValue(10);
}

ScmImageAnchor::ScmImageAnchor(QPointF position, qreal diameter)
	: QGraphicsEllipseItem(position.x(), position.y(), diameter, diameter)
{
	setFlag(ItemIsMovable, true);
	setBrush(QBrush(colorNoStar));
	setZValue(10);
}

ScmImageAnchor::~ScmImageAnchor()
{
	qDebug() << "Unloaded the ScmImageAnchor";
}

void ScmImageAnchor::setDiameter(qreal diameter)
{
	QPointF position = pos();
	setPosDiameter(position.x(), position.y(), diameter);
}

void ScmImageAnchor::setPosDiameter(qreal x, qreal y, qreal diameter)
{
	setRect(x, y, diameter, diameter);
	setPos(x, y);
}
