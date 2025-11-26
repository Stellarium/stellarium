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

#ifndef SCM_CONSTELLATION_IMAGE_H
#define SCM_CONSTELLATION_IMAGE_H

#include "ScmConstellationArtwork.hpp"
#include "ScmConstellationImageAnchor.hpp"
#include <functional>
#include <vector>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPixmap>

class ScmConstellationImage : public QGraphicsPixmapItem
{
public:
	ScmConstellationImage();
	~ScmConstellationImage();

	/**
	 * @brief Set the image that is shown by this item.
	 * 
	 * @param image The image that is shown.
	 */
	void setImage(const QPixmap &image);

	/**
	 * @brief Gets the indicator if any anchor is selected.
	 * 
	 * @return true Anchor is selected.
	 * @return false no Anchor is selected.
	 */
	bool hasAnchorSelection() const;

	/**
	 * @brief Get the selected anchor.
	 * 
	 * @return ScmConstellationImageAnchor* The selected anchor pointer or nullptr.
	 */
	ScmConstellationImageAnchor *getSelectedAnchor() const;

	/**
	 * @brief Set the anchor selection changed callback function.
	 */
	void setAnchorSelectionChangedCallback(std::function<void()> func);

	/**
	 * @brief Set the anchor position changed callback function.
	 */
	void setAnchorPositionChangedCallback(std::function<void()> func);

	/**
	 * @brief Resets the anchors to default.
	 */
	void resetAnchors();

	/**
	 * @brief Resets the artwork.
	 */
	void resetArtwork();

	/**
	 * @brief Gets the anchors of this object.
	 * 
	 * @return const std::vector<ScmConstellationImageAnchor>& The anchors of this object.
	 */
	const std::vector<ScmConstellationImageAnchor> &getAnchors() const;

	/**
	 * @brief Indicates if all anchors have a star they are bound too.
	 * 
	 * @return true All anchors have a bound star.
	 * @return false Not all anchors have a bound star.
	 */
	bool isImageAnchored();

	/**
	 * @brief Updates the anchors of this object with the drawn artwork.
	 */
	void updateAnchors();

	/**
	 * @brief Gets the artwork;
	 */
	const scm::ScmConstellationArtwork &getArtwork() const;

	/**
	 * @brief Set the artwork in the constellation image.
	 * 
	 * @param artwork The artwork to use.
	 */
	void setArtwork(const scm::ScmConstellationArtwork &artwork);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
	/// Holds the scene that is drawn on.
	QGraphicsScene drawScene;

	/// The anchors in the graphics view.
	std::vector<ScmConstellationImageAnchor> anchorItems{3}; // 3 anchors

	/// The scale of the anchor relative to the image size.
	const qreal anchorScale = 1 / 50.0;

	// Holds the selected anchor
	ScmConstellationImageAnchor *selectedAnchor = nullptr;

	/// Holds the current artwork
	scm::ScmConstellationArtwork artwork;
};

#endif // SCM_CONSTELLATION_IMAGE_H
