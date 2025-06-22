#ifndef SCMIMAGEANCHOR_H
#define SCMIMAGEANCHOR_H

#include <array>
#include <functional>
#include <QGraphicsItem>

class ScmImageAnchor : public QGraphicsEllipseItem
{
public:
	ScmImageAnchor();
	ScmImageAnchor(QPointF position, qreal diameter);
	~ScmImageAnchor();

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
	void setSelectionReference(ScmImageAnchor *&anchor);

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
	 * @brief Set the bounds in which the anchors can be moved.
	 * 
	 * @param bounds The bound the anchor can be moved in.
	 */
	void setMovementBounds(const QRectF &bounds);

	/**
	 * @brief set the bound star ID of this anchor.
	 */
	void setStarNameI18n(const QString &starNameI18n);

	/**
	 * @brief Get the star nameI18n that is bound to this anchor.
	 * 
	 * @return const QString& The star nameI18n.
	 */
	const QString &getStarNameI18n() const;

	/**
	 * @brief Updates the color of the anchor based on its current state.
	 */
	void updateColor();

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
	// Indicates if this anchor is selected.
	bool isSelected = false;
	// Holds the selected star
	QString starNameI18n;
	// Holds the default color of the anchor.
	const Qt::GlobalColor color = Qt::GlobalColor::cyan;
	// Holds the default color if no star is bound.
	const Qt::GlobalColor colorNoStar = Qt::GlobalColor::darkCyan;
	// Holds the selected color of the anchor.
	const Qt::GlobalColor selectedColor = Qt::GlobalColor::green;
	// Holds the selected color of the anchor if no star is bound.
	const Qt::GlobalColor selectedColorNoStar = Qt::GlobalColor::darkGreen;
	// Holds the selection group of this anchor.
	ScmImageAnchor **selection = nullptr;
	// Holds the set selection changed callback
	std::function<void()> selectionChangedCallback = nullptr;
	// Holds the bounds in which the anchor are moveable.
	QRectF movementBound;
};

#endif // SCMIMAGEANCHOR_H
