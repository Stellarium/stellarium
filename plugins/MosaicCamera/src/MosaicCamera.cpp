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
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "MosaicCamera.hpp"
#include "MosaicCameraDialog.hpp"
#include "MosaicTcpServer.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "CustomObject.hpp"

#include <QDebug>
#include <QDir>
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
	info.displayedName = "Mosaic Camera";
	info.authors = "Josh Meyers";
	info.contact = "jmeyers314@gmail.com";
	info.description = "Mosaic Camera Overlay";
	info.version = MOSAICCAMERA_PLUGIN_VERSION;
	info.license = MOSAICCAMERA_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
MosaicCamera::MosaicCamera()
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

    initializeUserData();

    qDebug() << "[MosaicCamera] Starting TCP server";
    tcpServer->startServer(5772);
    connect(tcpServer, &MosaicTcpServer::newValuesReceived, this, &MosaicCamera::updateMosaic);

    qDebug() << "[MosaicCamera] Loading built-in cameras";
    loadBuiltInCameras();
}

void MosaicCamera::initializeUserData()
{
    userDirectory = StelFileMgr::getUserDir()+"/modules/MosaicCamera/";
    QDir dir(userDirectory);
    if (!dir.exists())
    {
        StelFileMgr::makeSureDirExistsAndIsWritable(userDirectory);
        copyResourcesToUserDirectory();
    }
}

void MosaicCamera::copyResourcesToUserDirectory()
{
    // Get list of files in the resource directory
    QDir resourceDir(":/MosaicCamera");
    QStringList resourceFiles = resourceDir.entryList(QDir::Files);

    for (const QString& fileName : resourceFiles)
    {
        QString resourcePath = ":/MosaicCamera/" + fileName;
        QString destPath = userDirectory + fileName;

        if (QFile::copy(resourcePath, destPath))
        {
            qDebug() << "Copied" << resourcePath << "to" << destPath;

            // Ensure the copied file is writable
            QFile destFile(destPath);
            destFile.setPermissions(destFile.permissions() | QFile::WriteOwner);
        }
        else
        {
            qWarning() << "Failed to copy" << resourcePath << "to" << destPath;
        }
    }
}

void MosaicCamera::loadCameraOrder()
{
    cameraOrder.clear();
    QFile file(userDirectory + "camera_order.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(data));
        QJsonObject json = doc.object();
        QJsonArray orderArray = json["order"].toArray();
        for (const auto& value : orderArray) {
            cameraOrder << value.toString();
        }
    }
}

void MosaicCamera::loadBuiltInCameras()
{
    loadCameraOrder();
    cameras.clear();
    for (int i = 0; i < cameraOrder.size(); ++i)
    {
        Camera camera;
        camera.name = cameraOrder[i];
        camera.ra = 0.0;
        camera.dec = 0.0;
        camera.rotation = 0.0;
        camera.visible = false;
        cameras.insert(camera.name, camera);
        readPolygonSetsFromJson(camera.name, userDirectory + camera.name + ".json");
    }
    qDebug() << "[MosaicCamera] Loaded" << cameras.size() << "cameras";
    qDebug() << "[MosaicCamera] Camera names:" << cameras.keys();
}

void MosaicCamera::readPolygonSetsFromJson(const QString& cameraName, const QString& filename)
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

    if (cameras.contains(cameraName))
    {
        cameras[cameraName].polygon_sets = polygonSets;
    }
}

void MosaicCamera::setRA(const QString& cameraName, double ra)
{
    if (cameras.contains(cameraName))
    {
        cameras[cameraName].ra = ra;
        if(configDialog->visible()) {
            if (configDialog->getCurrentCameraName() == cameraName) {
                configDialog->setRA(ra);
            }
        }
    }
}

void MosaicCamera::setDec(const QString& cameraName, double dec)
{
    if (cameras.contains(cameraName))
    {
        cameras[cameraName].dec = dec;
        if(configDialog->visible()) {
            if (configDialog->getCurrentCameraName() == cameraName) {
                configDialog->setDec(dec);
            }
        }
    }
}

void MosaicCamera::setRotation(const QString& cameraName, double rotation)
{
    if (cameras.contains(cameraName))
    {
        cameras[cameraName].rotation = rotation;
        if(configDialog->visible()) {
            if (configDialog->getCurrentCameraName() == cameraName) {
                configDialog->setRotation(rotation);
            }
        }
    }
}

void MosaicCamera::setVisibility(const QString& cameraName, bool visible)
{
    if (cameras.contains(cameraName))
    {
        cameras[cameraName].visible = visible;
        if(configDialog->visible()) {
            if (configDialog->getCurrentCameraName() == cameraName) {
                configDialog->setVisibility(visible);
            }
        }
    }
}

double MosaicCamera::getRA(const QString& cameraName) const
{
    return cameras.value(cameraName).ra;
}

double MosaicCamera::getDec(const QString& cameraName) const
{
    return cameras.value(cameraName).dec;
}

double MosaicCamera::getRotation(const QString& cameraName) const
{
    return cameras.value(cameraName).rotation;
}

bool MosaicCamera::getVisibility(const QString& cameraName) const
{
    return cameras.value(cameraName).visible;
}

void MosaicCamera::updateMosaic(const QString& cameraName, double ra, double dec, double rotation)
{
    qDebug() << "Received new values for" << cameraName << ": RA=" << ra << ", Dec=" << dec << ", Rotation=" << rotation;
    setRA(cameraName, ra);
    setDec(cameraName, dec);
    setRotation(cameraName, rotation);
}

void MosaicCamera::draw(StelCore* core)
{
    const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
    StelPainter painter(prj);

    Vec3d startPoint, endPoint;

    for (const auto& camera : cameras)
    {
        if (!camera.visible)
            continue;

        double alpha = camera.ra / 57.29577951308232 + 1.5707963267948966;
        double beta = 1.5707963267948966 - camera.dec / 57.29577951308232;
        double rot = camera.rotation / 57.29577951308232;

        double cosBeta = cos(beta);
        double sinBeta = sin(beta);

        double cosAlpha = cos(alpha);
        double sinAlpha = sin(alpha);

        for (const auto& polygonSet : camera.polygon_sets) {

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
}

QStringList MosaicCamera::getCameraNames() const
{
    return cameraOrder;
}

bool MosaicCamera::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}
