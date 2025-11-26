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

#ifndef SCM_CONSTELLATION_IMAGE_ANCHOR_H
#define SCM_CONSTELLATION_IMAGE_ANCHOR_H

#include "StarMgr.hpp"
#include "VecMath.hpp"
#include <array>
#include <functional>
#include <QGraphicsItem>
#include <QString>

class ScmConstellationImageAnchor : public QGraphicsEllipseItem
{
public:
	ScmConstellationImageAnchor();
	ScmConstellationImageAnchor(QPointF position, qreal diameter);
	~ScmConstellationImageAnchor();

	/**
	 * @brief Set the diameter of this anchor.
	 * 
	 * @param diameter The diameter of this round anchor.
	 */
	void setDiameter(qreal diameter);

	/**
	 * @brief Set the position and diameter of this anchor.
	 * 
	 * @param x The x position in the scene.
	 * @param y The y position in the scene.
	 * @param diameter The diameter of this anchor.
	 */
	void setPosDiameter(qreal x, qreal y, qreal diameter);

	/**
	 * @brief Set the reference to the selected anchor.
	 * 
	 * @param anchor The pointer to the selection anchor.
	 */
	void setSelectionReference(ScmConstellationImageAnchor *&anchor);

	/**
	 * @brief Selects this anchor.
	 */
	void select();

	/**
	 * @brief Deselects this anchor.
	 */
	void deselect();

	/**
	 * @brief Set the selection changed callback function.
	 */
	void setSelectionChangedCallback(std::function<void()> func);

	/**
	 * @brief Set the position changed callback function.
	 */
	void setPositionChangedCallback(std::function<void()> func);

	/**
	 * @brief Set the bounds in which the anchors can be moved.
	 * 
	 * @param bounds The bound the anchor can be moved in.
	 */
	void setMovementBounds(const QRectF &bounds);

	/**
	 * @brief set the bound star ID of this anchor.
	 */
	void setStarHip(StarId hip);

	/**
	 * @brief Tries to extract the hip from the star id.
	 * 
	 * @param id The id to extract the hip from.
	 * @return true Success.
	 * @return false Failed.
	 */
	bool trySetStarHip(const QString &id);

	/**
	 * @brief Get the star hip id.
	 * 
	 * @return const QString& 
	 */
	const StarId &getStarHip() const;

	/**
	 * @brief Updates the color of the anchor based on its current state.
	 */
	void updateColor();

	/**
	 * @brief Get the position of the anchor in the parent image.
	 * 
	 * @return Vec2i The position of the anchor.
	 */
	Vec2i getPosition() const;

	/**
	 * @brief Set the position of the anchor in the parent image.
	 * 
	 * @param position The position of the anchor.
	 */
	void setPosition(const Vec2i &position);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
	// Indicates if this anchor is selected.
	bool isSelected = false;
	// Holds the hip of the selected star
	StarId hip = 0; // Hip start with 1, 0 = empty value
	// Holds the default color of the anchor.
	const Qt::GlobalColor color = Qt::GlobalColor::cyan;
	// Holds the default color if no star is bound.
	const Qt::GlobalColor colorNoStar = Qt::GlobalColor::darkCyan;
	// Holds the selected color of the anchor.
	const Qt::GlobalColor selectedColor = Qt::GlobalColor::green;
	// Holds the selected color of the anchor if no star is bound.
	const Qt::GlobalColor selectedColorNoStar = Qt::GlobalColor::darkGreen;
	// Holds the selection group of this anchor.
	ScmConstellationImageAnchor **selection = nullptr;
	// Holds the set selection changed callback
	std::function<void()> selectionChangedCallback = nullptr;
	// Holds the set position changed callback
	std::function<void()> positionChangedCallback = nullptr;
	// Holds the bounds in which the anchor are moveable.
	QRectF movementBound;
};

#endif // SCM_CONSTELLATION_IMAGE_ANCHOR_H
