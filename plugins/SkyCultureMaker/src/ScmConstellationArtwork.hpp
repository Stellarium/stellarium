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

private:
	/**
     * @brief Draw the artwork in an optimized manner.
     * 
     * @param sPainter The painter to draw with.
     * @param region The region to draw in.
     * @param obsVelocity The velocity of the observer for correction.
     */
	void drawOptimized(StelPainter &sPainter, const SphericalRegion &region, const Vec3d &obsVelocity) const;

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

	/// Holds the opacity of the art.
	float artOpacity = 0.42;

	/// Indicates if the artwork has an image that can be drawn.
	bool hasArt = false;

	/// Indicates if the setup was run.
	bool isSetup = false;
};
} // namespace scm

#endif // SCMCONSTELLATIONARTWORK_H
