#ifndef SCMIMAGEANCHORED_H
#define SCMIMAGEANCHORED_H

#include <ScmImageAnchor.hpp>
#include <functional>
#include <vector>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QPixmap>

class ScmImageAnchored : public QGraphicsPixmapItem
{
public:
	ScmImageAnchored();
	~ScmImageAnchored();

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
	 * @return ScmImageAnchor* The selected anchor pointer or nullptr.
	 */
	ScmImageAnchor *getSelectedAnchor() const;

	/**
	 * @brief Set the anchor selection changed callback function.
	 */
	void setAnchorSelectionChangedCallback(std::function<void()> func);

	/**
	 * @brief Resets the anchors to default.
	 */
	void resetAnchors();

	const std::vector<ScmImageAnchor> &getAnchors() const;

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
	/// Holds the scene that is drawn on.
	QGraphicsScene drawScene;
	/// The anchors in the graphics view.
	std::vector<ScmImageAnchor> anchorItems{3}; // 3 anchors
	/// The scale of the anchor relative to the image size.
	const qreal anchorScale = 1 / 50.0;
	// Holds the selected anchor
	ScmImageAnchor *selectedAnchor = nullptr;
};

#endif // SCMIMAGEANCHORED_H
