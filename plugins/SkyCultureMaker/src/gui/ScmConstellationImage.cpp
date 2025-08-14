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

#include "ScmConstellationImage.hpp"
#include "StarWrapper.hpp"
#include "StelApp.hpp"
#include "types/Anchor.hpp"
#include <QDebug>

ScmConstellationImage::ScmConstellationImage()
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

ScmConstellationImage::~ScmConstellationImage()
{
	qDebug() << "SkyCultureMaker: Unloaded the ScmConstellationImage";
}

void ScmConstellationImage::setImage(const QPixmap &image)
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

bool ScmConstellationImage::hasAnchorSelection() const
{
	return selectedAnchor != nullptr;
}

ScmConstellationImageAnchor *ScmConstellationImage::getSelectedAnchor() const
{
	return selectedAnchor;
}

void ScmConstellationImage::setAnchorSelectionChangedCallback(std::function<void()> func)
{
	for (auto &anchor : anchorItems)
	{
		anchor.setSelectionChangedCallback(func);
	}
}

void ScmConstellationImage::setAnchorPositionChangedCallback(std::function<void()> func)
{
	for (auto &anchor : anchorItems)
	{
		anchor.setPositionChangedCallback(func);
	}
}

void ScmConstellationImage::resetAnchors()
{
	selectedAnchor = nullptr;
	for (auto &anchor : anchorItems)
	{
		anchor.setStarHip(0);
		anchor.deselect();
	}
}

void ScmConstellationImage::resetArtwork()
{
	artwork.reset();
	hide();
}

const std::vector<ScmConstellationImageAnchor> &ScmConstellationImage::getAnchors() const
{
	return anchorItems;
}

bool ScmConstellationImage::isImageAnchored()
{
	for (const auto &anchor : anchorItems)
	{
		if (anchor.getStarHip() == 0)
		{
			return false;
		}
	}
	return true;
}

void ScmConstellationImage::updateAnchors()
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

void ScmConstellationImage::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
}

void ScmConstellationImage::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
}

void ScmConstellationImage::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseMoveEvent(event);
}

const scm::ScmConstellationArtwork &ScmConstellationImage::getArtwork() const
{
	return artwork;
}

void ScmConstellationImage::setArtwork(const scm::ScmConstellationArtwork &artwork)
{
	resetAnchors();
	resetArtwork();

	if (!artwork.getHasArt())
	{
		return;
	}

	QPixmap pixmap = QPixmap::fromImage(artwork.getArtwork());
	setImage(pixmap);

	const auto &anchors = artwork.getAnchors();
	for (size_t i = 0; i < artwork.getAnchors().size(); i++)
	{
		anchorItems[i].setPosition(anchors[i].position);
		anchorItems[i].setStarHip(anchors[i].hip);
	}

	updateAnchors();
	show();
}
