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

#ifndef SCMCONSTELLATIONARTWORK_H
#define SCMCONSTELLATIONARTWORK_H

#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelSphereGeometry.hpp"
#include "VecMath.hpp"
#include "types/Anchor.hpp"
#include <array>
#include <QImage>
#include <QJsonObject>

namespace scm
{
/**
 * @brief Class that represents a constellation artwork.
 * 
 * Due to the lambda capture of 'this' in QObject::connect,
 * the address of a ScmConstellationArtwork instance must
 * not change during its lifetime!
 */
class ScmConstellationArtwork
{
public:
	ScmConstellationArtwork();
	ScmConstellationArtwork(const std::array<Anchor, 3> &anchors, const QImage &artwork);

	/**
	 * @brief Setups the artwork in the frame based on the anchors.
	 */
	void setupArt();

	/**
	 * @brief Sets an anchor value at the selected index.
	 * 
	 * @param index The index to set the anchor.
	 * @param anchor The anchor to be set.
	 */
	void setAnchor(int index, const Anchor &anchor);

	/**
     * @brief Gets the anchors of the artwork.
     * 
     * @return const std::array<Anchor, 3>& The artwork anchors.
     */
	const std::array<Anchor, 3> &getAnchors() const;

	/**
	 * @brief Sets the size of the artwork in pixels.
	 * 
	 * @param Artwork The size of the artwork.
	 */
	void setArtwork(const QImage &artwork);

	/**
     * @brief Gets the artwork.
     * 
     * @return const QPixmap& The artwork.
     */
	const QImage &getArtwork() const;

	/**
	 * @brief Get the indicator if the artwork contains art.
	 * 
	 * @return true Contains art.
	 * @return false Does not contain art.
	 */
	bool getHasArt() const;

	/**
     * @brief Draws the artwork.
     * 
     * @param core The core to use to draw the artwork.
     */
	void draw(StelCore *core) const;

	/**
     * @brief Draws the artwork.
     * 
     * @param core The core to use to draw the artwork.
     */
	void draw(StelCore *core, StelPainter &painter) const;

	/**
     * @brief Converts the artwork into stalleriums json format.
     * 
     * @param relativePath The relative path to the artwork on disk.
     * @return QJsonObject The json format.
     */
	QJsonObject toJson(const QString &relativePath) const;

	/**
     * @brief Writes the image of the artwork to the disk.
     * 
     * @param filepath The filepath to write the artwork to.
	 * @return true Successful saved.
	 * @return false Failed to save.
	 */
	bool save(const QString &filepath) const;

	/**
	 * @brief Resets the artwork state.
	 */
	void reset();

private:
	/**
     * @brief Draw the artwork in an optimized manner.
     * 
     * @param sPainter The painter to draw with.
     * @param region The region to draw in.
     * @param obsVelocity The velocity of the observer for correction.
     */
	void drawOptimized(StelPainter &sPainter, const SphericalRegion &region, const Vec3d &obsVelocity) const;

	/**
	 * @brief Initializes the artwork values from the ConstellationMgr.
	 */
	void initValues();

	/// Holds the anchors of the artwork.
	std::array<Anchor, 3> anchors;

	/// Holds the the artwork.
	QImage artwork;

	/// Holds the artwork as texture to draw.
	StelTextureSP artTexture;

	/// Holds the vertices the artwork texture is drawn on.
	StelVertexArray artPolygon;

	/// Holds the bounding cap of the artwork.
	SphericalCap boundingCap;

	/// Holds the intensity scale based on the Field of View.
	float artIntensityFovScale = 1.0f;

	/// Holds the intensity of the art.
	float artIntensity = 0.42;

	/// Indicates if the artwork has an image that can be drawn.
	bool hasArt = false;

	/// Indicates if the setup was run.
	bool isSetup = false;
};
} // namespace scm

#endif // SCMCONSTELLATIONARTWORK_H
