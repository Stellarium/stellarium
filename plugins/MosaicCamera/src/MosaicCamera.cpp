/*
 * Copyright (C) 2024 Josh Meyers
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "MosaicCamera.hpp"
#include "MosaicCameraDialog.hpp"
#include "MosaicTcpServer.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "CustomObject.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QResource>
#include <QByteArray>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*************************************************************************/
StelModule* MosaicCameraStelPluginInterface::getStelModule() const
{
	return new MosaicCamera();
}

StelPluginInfo MosaicCameraStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "MosaicCamera";
	info.displayedName = "Mosaic Camera plugin";
	info.authors = "Josh Meyers";
	info.contact = "jmeyers314@gmail.com";
	info.description = "Mosaic Camera Overlay plugin.";
	info.version = MOSAICCAMERA_PLUGIN_VERSION;
	info.license = MOSAICCAMERA_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
MosaicCamera::MosaicCamera() : ra(0), dec(0), rsp(0)
{
	setObjectName("MosaicCamera");
	configDialog = new MosaicCameraDialog();
    tcpServer = new MosaicTcpServer();
}

/*************************************************************************
 Destructor
*************************************************************************/
MosaicCamera::~MosaicCamera()
{
    delete configDialog;
    delete tcpServer;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double MosaicCamera::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void MosaicCamera::init()
{
    Q_INIT_RESOURCE(MosaicCamera);

    qDebug() << "Starting TCP server";
    tcpServer->startServer(5772);
    connect(tcpServer, &MosaicTcpServer::newValuesReceived, this, &MosaicCamera::updateMosaic);

    qDebug() << "Loading Rubin json";
    readPolygonSetsFromJson(":/MosaicCamera/RubinMosaic.json");
}


void MosaicCamera::readPolygonSetsFromJson(const QString& filename)
{
    QVector<PolygonSet> polygonSets;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open file:" << file.errorString();
    }

    QByteArray saveData = file.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonArray jsonArray = loadDoc.array();

    for (int i = 0; i < jsonArray.size(); ++i) {
        QJsonObject setObject = jsonArray[i].toObject();
        PolygonSet set;

        // Read name
        set.name = setObject["name"].toString();

        // Read corners
        QJsonArray cornersArray = setObject["corners"].toArray();
        for (int j = 0; j < cornersArray.size(); ++j) {
            QVector<QPointF> polygon;
            QJsonArray polygonArray = cornersArray[j].toArray();
            for (int k = 0; k < polygonArray.size(); ++k) {
                QJsonArray pointArray = polygonArray[k].toArray();
                if (pointArray.size() == 2) {
                    QPointF point(pointArray[0].toDouble(), pointArray[1].toDouble());
                    polygon.append(point);
                }
            }
            set.corners.append(polygon);
        }

        // Read color
        QJsonObject colorObject = setObject["color"].toObject();
        set.colorComment = colorObject["comment"].toString();
        QJsonArray colorArray = colorObject["value"].toArray();
        if (colorArray.size() == 4) {
            set.color = QColor::fromRgbF(
                colorArray[0].toDouble(),
                colorArray[1].toDouble(),
                colorArray[2].toDouble(),
                colorArray[3].toDouble()
            );
        }

        polygonSets.append(set);
    }

    polygon_sets = polygonSets;
}

void MosaicCamera::setRA(double ra)
{
    this->ra = ra;
    if(configDialog->visible()) {
        configDialog->setRA(ra);
    }
}

void MosaicCamera::setDec(double dec)
{
    this->dec = dec;
    if(configDialog->visible()) {
        configDialog->setDec(dec);
    }
}

void MosaicCamera::setRSP(double rsp)
{
    this->rsp = rsp;
    if(configDialog->visible()) {
        configDialog->setRSP(rsp);
    }
}

void MosaicCamera::updateMosaic(double ra, double dec, double rsp)
{
    qDebug() << "Received new values: RA=" << ra << ", Dec=" << dec << ", RSP=" << rsp;
    setRA(ra);
    setDec(dec);
    setRSP(rsp);
}

void MosaicCamera::draw(StelCore* core)
{
	// Input parameters for drawing LSSTCam FoV are:
	// ra
	// dec
	// rotskypos

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);


    double alpha = ra / 57.29577951308232 + 1.5707963267948966;  // ra=0 isn't along x=0?
    double beta = 1.5707963267948966 - dec / 57.29577951308232;  // polar angle
    double rot = rsp / 57.29577951308232;  // rotation angle

    double cosBeta = cos(beta);
    double sinBeta = sin(beta);

    double cosAlpha = cos(alpha);
    double sinAlpha = sin(alpha);

	Vec3d startPoint, endPoint;


    for (const auto& polygonSet : polygon_sets) {
        // Set color for this polygon set
        painter.setColor(
            polygonSet.color.redF(),
            polygonSet.color.greenF(),
            polygonSet.color.blueF(),
            polygonSet.color.alphaF()
        );

        for (const auto& polygon : polygonSet.corners) {
            // Loop through points in the polygon
            for (int i = 0; i < polygon.size(); ++i) {
                const QPointF& p1 = polygon[i];
                const QPointF& p2 = polygon[(i + 1) % polygon.size()];

                // Apply camera rotator
                double r1x = cos(rot) * p1.x() + sin(rot) * p1.y();
                double r1y = -sin(rot) * p1.x() + cos(rot) * p1.y();
                double r2x = cos(rot) * p2.x() + sin(rot) * p2.y();
                double r2y = -sin(rot) * p2.x() + cos(rot) * p2.y();

                double r1z = 1.0;
                double r2z = 1.0;

                double den1 = sqrt(r1x*r1x + r1y*r1y + r1z*r1z);
                double den2 = sqrt(r2x*r2x + r2y*r2y + r2z*r2z);

                // Apply declination
                double s1x = r1x;
                double s1y = r1y * cosBeta - r1z * sinBeta;
                double s1z = r1y * sinBeta + r1z * cosBeta;
                double s2x = r2x;
                double s2y = r2y * cosBeta - r2z * sinBeta;
                double s2z = r2y * sinBeta + r2z * cosBeta;

                // Apply right ascension
                startPoint.set(
                    (s1x*cosAlpha - s1y*sinAlpha)/den1,
                    (s1x*sinAlpha + s1y*cosAlpha)/den1,
                    s1z/den1
                );
                endPoint.set(
                    (s2x*cosAlpha - s2y*sinAlpha)/den2,
                    (s2x*sinAlpha + s2y*cosAlpha)/den2,
                    s2z/den2
                );

                painter.drawGreatCircleArc(startPoint, endPoint);
            }
        }
    }
}

bool MosaicCamera::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}
