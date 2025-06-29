#include "ScmImageAnchored.hpp"
#include "StarWrapper.hpp"
#include "StelApp.hpp"
#include "types/Anchor.hpp"
#include <QDebug>

ScmImageAnchored::ScmImageAnchored()
	: QGraphicsPixmapItem()
{
	setZValue(1);
	drawScene.addItem(this);
	for (auto &anchor : anchorItems)
	{
		drawScene.addItem(&anchor);
		anchor.setParentItem(this);
		anchor.setSelectionReference(selectedAnchor);
	}
}

ScmImageAnchored::~ScmImageAnchored()
{
	qDebug() << "Unloaded the ScmImageAnchored";
}

void ScmImageAnchored::setImage(const QPixmap &image)
{
	setPixmap(image);

	const qreal &anchorHeight = image.height() * anchorScale;
	const qreal &anchorWidth  = image.width() * anchorScale;
	const qreal diameter      = std::min(anchorWidth, anchorHeight);

	drawScene.setSceneRect(0, 0, image.width(), image.height());

	// Fixes the offset that occurs through the setSceneRect
	const QPointF &offsetFix      = QPointF(-image.width() * 0.25, -image.height() * 0.25);
	const QPointF &positionCenter = scenePos() - offsetFix; // - offsetFix = (Width * 0.5, Height * 0.5) + offsetFix

	// offset to place the anchors in the center next to each other
	qreal offset = 0.5 * anchorItems.size() * -diameter;
	for (auto &anchor : anchorItems)
	{
		anchor.setPosDiameter(positionCenter.x() + offset, positionCenter.y(), diameter);
		anchor.setMovementBounds(QRectF(offsetFix.x() - offset, offsetFix.y(), image.width(), image.height()));
		offset += diameter;
	}

	artwork.setArtwork(image.toImage());

	if (isImageAnchored())
	{
		artwork.setupArt();
	}
}

bool ScmImageAnchored::hasAnchorSelection() const
{
	return selectedAnchor != nullptr;
}

ScmImageAnchor *ScmImageAnchored::getSelectedAnchor() const
{
	return selectedAnchor;
}

void ScmImageAnchored::setAnchorSelectionChangedCallback(std::function<void()> func)
{
	for (auto &anchor : anchorItems)
	{
		anchor.setSelectionChangedCallback(func);
	}
}

void ScmImageAnchored::setAnchorPositionChangedCallback(std::function<void()> func)
{
	for (auto &anchor : anchorItems)
	{
		anchor.setPositionChangedCallback(func);
	}
}

void ScmImageAnchored::resetAnchors()
{
	selectedAnchor = nullptr;
	for (auto &anchor : anchorItems)
	{
		anchor.setStarNameI18n(QString());
		anchor.deselect();
	}
}

const std::vector<ScmImageAnchor> &ScmImageAnchored::getAnchors() const
{
	return anchorItems;
}

bool ScmImageAnchored::isImageAnchored()
{
	for (const auto &anchor : anchorItems)
	{
		if (anchor.getStarNameI18n().isEmpty())
		{
			return false;
		}
	}
	return true;
}

void ScmImageAnchored::updateAnchors()
{
	for (size_t i = 0; i < anchorItems.size(); ++i)
	{
		scm::Anchor anchor;
		anchor.position = anchorItems[i].getPosition();
		anchor.hip      = anchorItems[i].getStarHip();
		artwork.setAnchor(i, anchor);
	}

	artwork.setupArt();
}

void ScmImageAnchored::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
}

void ScmImageAnchored::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
}

void ScmImageAnchored::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseMoveEvent(event);
}

const scm::ScmConstellationArtwork &ScmImageAnchored::getArtwork() const
{
	return artwork;
}
