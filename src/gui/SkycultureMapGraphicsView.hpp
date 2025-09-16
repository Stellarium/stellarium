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
	void initializeTime();

public slots:
	void selectCulture(const QString &skycultureId);
	void updateTime(int year);


signals:
	void cultureSelected(const QString &skycultureId);
	void timeValueChanged(int year);
	void timeRangeChanged(int minYear, int maxYear);

protected:
	void wheelEvent(QWheelEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void scaleView(double scaleFactor);

private:
	// variables
	int minYear;
	int maxYear;
	bool firstShow;
	int currentYear;
	QString oldSkyCulture;
	QTimeLine zoomToDefaultTimer;
	QTimeLine zoomOnTargetTimer;
	QRectF startingRect;
	QRectF defaultRect;
	QRectF targetRect;

	// functions
	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	QList<QPointF> convertMeterToView(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	void updateCultureVisibility();
	void smoothFitInView(QRectF targetRect);
	qreal calculateScaleRatio(qreal width, qreal height);


private slots:
	void zoomToDefault(qreal factor);
	void zoomOnTarget(qreal factor);
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
