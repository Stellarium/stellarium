#ifndef SKYCULTUREMAPGRAPHICSVIEW_HPP
#define SKYCULTUREMAPGRAPHICSVIEW_HPP

#include <QGraphicsView>

//! @class SkyCultureMapGraphicsView
//! Special GraphicsView that shows a world map and several polygons (cultures)
class SkycultureMapGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	SkycultureMapGraphicsView(QWidget *parent = nullptr);

	// public functions


public slots:
	void selectCulture(const QString &skycultureId);
	void updateTime(int year);


signals:
	void cultureSelected(const QString &skycultureId);
	void timeChanged(int year);

protected:
	void wheelEvent(QWheelEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void scaleView(double scaleFactor);

private:
   // private functions
	//int _numScheduledScalings;
	int currentYear;
	QString oldSkyCulture;

	QList<QPointF> convertLatLonToMeter(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	QList<QPointF> convertMeterToView(const QList<QPointF> &irl, qreal mapWidth, qreal mapHeight);
	void updateCultureVisibility();

private slots:
	// void scalingTime(qreal x);
	// void animFinished();
};

#endif // SKYCULTUREMAPGRAPHICSVIEW_HPP
