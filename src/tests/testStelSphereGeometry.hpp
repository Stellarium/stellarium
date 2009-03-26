

#include <config.h>

#include <QObject>
#include <QtDebug>
#include <QtTest>

class TestStelSphericalGeometry : public QObject
{
Q_OBJECT
private slots:
	void testHalfSpace();
	void testContains();
	void testPlaneIntersect2();
};

