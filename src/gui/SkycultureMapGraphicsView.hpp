#ifndef SKYCULTUREMAPGRAPHICSVIEW_HPP
#define SKYCULTUREMAPGRAPHICSVIEW_HPP

#include <qtimeline.h>
#include <QGraphicsView>

//! @class SkyCultureMapGraphicsView
//! Special GraphicsView that shows a world map and several (culture) polygons
class SkycultureMapGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	SkycultureMapGraphicsView(QWidget *parent = nullptr);

	// public functions
	//void initializeTime();
	//void initializeGraphicsView();

public slots:
	void selectCulture(const QString &skycultureId, int startTime);
	void updateTime(int year);
	void rotateMap(bool isRotated);
	void changeProjection(bool isChanged);


signals:
	void cultureSelected(const QString &skycultureId);
	void timeValueChanged(int year);
	void timeRangeChanged(int minYear, int maxYear);

protected:
	void wheelEvent(QWheelEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void showEvent(QShowEvent *e) override;
	void scaleView(double scaleFactor);

private:
	// variables
	bool viewScrolling;
	bool mapMoved;
	bool firstShow;
	bool isRotated;
	int currentYear;
	QPoint mouseLastXY;
	QString oldSkyCulture;
	QTimeLine zoomToDefaultTimer;
	QTimeLine zoomOnTargetTimer;
	QRectF startingRect;
	QRectF defaultRect;
	QRectF targetRect;

	// functions
	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &latLonCoordinates);
	QList<QPointF> convertMeterToView(const QList<QPointF> &meterCoordinates);
	QRectF calculateBoundingBox(const QList<QGraphicsItem *> graphicsItemList);
	void updateCultureVisibility();
	void smoothFitInView(QRectF targetRect);
	void selectAllCulturePolygon(const QString &skycultureId);
	void drawMapContent();
	void loadCulturePolygons();
	qreal calculateScaleRatio(qreal width, qreal height);

private slots:
	void zoomToDefault(qreal factor);
	void zoomOnTarget(qreal factor);
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
