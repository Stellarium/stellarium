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
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "MosaicCamera.hpp"
#include "MosaicCameraDialog.hpp"
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
	info.displayedName = N_("Mosaic Camera");
	info.authors = "Josh Meyers";
	info.contact = "jmeyers314@gmail.com";
	info.description = N_("Mosaic Camera Overlay");
	info.version = MOSAICCAMERA_PLUGIN_VERSION;
	info.license = MOSAICCAMERA_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
MosaicCamera::MosaicCamera()
        : toolbarButton(Q_NULLPTR)
{
	setObjectName("MosaicCamera");
	configDialog = new MosaicCameraDialog();
	StelApp &app = StelApp::getInstance();
	conf = app.getSettings();
	core = app.getCore();
	gui = dynamic_cast<StelGui*>(app.getGui());
}

/*************************************************************************
 Destructor
*************************************************************************/
MosaicCamera::~MosaicCamera()
{
	delete configDialog;
}

void MosaicCamera::loadSettings()
{
	conf->beginGroup("MosaicCamera");
	currentCamera = conf->value("currentCamera").toString();
	setFlagShowButton(conf->value("showButton", true).toBool());
	enableMosaicCamera(conf->value("enabled").toBool());
	int size = conf->beginReadArray("cameraVisibility");
	for (int i = 0; i < size; ++i)
	{
		conf->setArrayIndex(i);
		QString name = conf->value("name").toString();
		bool visible = conf->value("visible").toBool();
		if (cameras.contains(name))
		{
			cameras[name].visible = visible;
			cameras[name].ra = conf->value("ra").toDouble();
			cameras[name].dec = conf->value("dec").toDouble();
			cameras[name].rotation = conf->value("rotation").toDouble();
		}
	}
	conf->endArray();
	conf->endGroup();
}

void MosaicCamera::saveSettings() const
{
	conf->beginGroup("MosaicCamera");
	conf->setValue("currentCamera", currentCamera);
	conf->setValue("showButton", getFlagShowButton());
	conf->setValue("enabled", flagShowMosaicCamera);
	conf->beginWriteArray("cameraVisibility");
	int i = 0;
	for (auto it = cameras.constBegin(); it != cameras.constEnd(); ++it)
	{
		conf->setArrayIndex(i++);
		conf->setValue("name", it.key());
		conf->setValue("visible", it.value().visible);
		conf->setValue("ra", it.value().ra);
		conf->setValue("dec", it.value().dec);
		conf->setValue("rotation", it.value().rotation);
	}
	conf->endArray();
	conf->endGroup();
}

void MosaicCamera::restoreDefaults()
{
	conf->beginGroup("MosaicCamera");
	conf->remove("currentCamera");
	conf->remove("showButton");
	conf->remove("enabled");
	conf->remove("cameraVisibility");
	for (auto it = cameras.constBegin(); it != cameras.constEnd(); ++it)
	{
		conf->remove(it.key());
	}
	conf->endGroup();

	StelFileMgr::makeSureDirExistsAndIsWritable(userDirectory);
	copyResourcesToUserDirectory();

	// Reload the cameras
	loadBuiltInCameras();
	setCurrentCamera(cameraOrder[0]);
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
	loadBuiltInCameras();

	addAction("actionShow_MosaicCamera", N_("Mosaic Camera"), N_("Show Mosaic Camera"), "enabled", "");
	addAction("actionShow_MosaicCamera_dialog", N_("Mosaic Camera"), N_("Show settings dialog"), configDialog, "visible");
	addAction("actionSetRADecToView", N_("Mosaic Camera"), N_("Set RA/Dec to current view"), "setRADecToView()", "");
	addAction("actionSetRADecToObject", N_("Mosaic Camera"), N_("Set RA/Dec to current object"), "setRADecToObject()", "");
	addAction("actionSetViewToCamera", N_("Mosaic Camera"), N_("Set view to current camera"), "setViewToCamera()", "");
	addAction("actionIncrementRotation", N_("Mosaic Camera"), N_("Increment rotation"), "incrementRotation()", "");
	addAction("actionDecrementRotation", N_("Mosaic Camera"), N_("Decrement rotation"), "decrementRotation()", "");
	addAction("actionNextCamera", N_("Mosaic Camera"), N_("Next camera"), "nextCamera()", "");
	addAction("actionPreviousCamera", N_("Mosaic Camera"), N_("Previous camera"), "previousCamera()", "");

	loadSettings();

	if (currentCamera == "")
		setCurrentCamera(cameraOrder[0]);
}

void MosaicCamera::enableMosaicCamera(bool b)
{
	if (b!=flagShowMosaicCamera)
	{
		flagShowMosaicCamera = b;
		emit flagMosaicCameraVisibilityChanged(b);
	}
}

void MosaicCamera::initializeUserData()
{
	userDirectory = StelFileMgr::getUserDir()+"/modules/MosaicCamera/";
	QDir dir(userDirectory);
	if (!dir.exists() || dir.isEmpty())
	{
		StelFileMgr::makeSureDirExistsAndIsWritable(userDirectory);
		copyResourcesToUserDirectory();
	}
}

void MosaicCamera::setFlagShowButton(bool b)
{
	if (gui != Q_NULLPTR)
	{
		if (b == true)
		{
			if (toolbarButton == Q_NULLPTR)
			{
				// Create the button
				toolbarButton = new StelButton(Q_NULLPTR,
				                               QPixmap(":/MosaicCamera/bt_MosaicCamera_On.png"),
				                               QPixmap(":/MosaicCamera/bt_MosaicCamera_Off.png"),
				                               QPixmap(":/graphicGui/miscGlow32x32.png"),
				                               "actionShow_MosaicCamera",
				                               false,
				                               "actionShow_MosaicCamera_dialog");
			}
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
		else {
			gui->getButtonBar()->hideButton("actionShow_MosaicCamera");
		}
	}
	flagShowButton = b;
}

void MosaicCamera::copyResourcesToUserDirectory()
{
	// Get list of files in the resource directory
	QDir resourceDir(":/MosaicCamera");
	QStringList fileNameFilters("*.json"); // copy data only
	QStringList resourceFiles = resourceDir.entryList(fileNameFilters);

	for (const QString& fileName : resourceFiles)
	{
		QString resourcePath = ":/MosaicCamera/" + fileName;
		QString destPath = userDirectory + fileName;
		QFile destFile(destPath);
		QFile resourceFile(resourcePath);

		if (destFile.exists()) {
			if (!destFile.remove()) {
				qWarning() << "[MosaicCamera] Failed to remove existing file:" << destPath;
			}
		}

		if (resourceFile.copy(destPath))
		{
			// Copy the file to the user directory
			qDebug() << "[MosaicCamera] Copied" << resourcePath << "to" << destPath;
			// Ensure the copied file is writable
			destFile.setPermissions(destFile.permissions() | QFile::WriteOwner);
		}
		else
		{
			qWarning() << "[MosaicCamera] Failed to copy" << resourcePath << "to" << destPath;
		}
	}
}

void MosaicCamera::loadCameraOrder()
{
	cameraOrder.clear();
	QFile file(userDirectory + "camera_order.json");
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray data = file.readAll();
		QJsonDocument doc(QJsonDocument::fromJson(data));
		QJsonObject json = doc.object();
		QJsonArray orderArray = json["order"].toArray();
		for (const auto& value : orderArray)
		{
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
		camera.fieldDiameter = 0.0; // let's initialize field
		cameras.insert(camera.name, camera);
		readPolygonSetsFromJson(camera.name, userDirectory + camera.name + ".json");
		setCameraFieldDiameter(cameras[camera.name]);
	}
	qInfo() << "[MosaicCamera] Loaded" << cameras.size() << "cameras:" << cameras.keys().join(", ");
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

	for (int i = 1; i < jsonArray.size(); ++i)
	{
		QJsonObject setObject = jsonArray[i].toObject();
		PolygonSet set;

		// Read name
		set.name = setObject["name"].toString();

		// Read corners
		QJsonArray cornersArray = setObject["corners"].toArray();
		for (int j = 0; j < cornersArray.size(); ++j)
		{
			QVector<QPointF> polygon;
			QJsonArray polygonArray = cornersArray[j].toArray();
			for (int k = 0; k < polygonArray.size(); ++k)
			{
				QJsonArray pointArray = polygonArray[k].toArray();
				if (pointArray.size() == 2)
				{
					QPointF point(pointArray[0].toDouble(), pointArray[1].toDouble());
					polygon.append(point);
				}
			}
			set.corners.append(polygon);
		}

		// Read color
		QJsonObject colorObject = setObject["color"].toObject();
		QJsonArray colorArray = colorObject["value"].toArray();
		if (colorArray.size() == 4)
		{
			set.color = QColor::fromRgbF(colorArray[0].toDouble(), colorArray[1].toDouble(), colorArray[2].toDouble(), colorArray[3].toDouble());
		}

		polygonSets.append(set);
	}

	if (cameras.contains(cameraName))
	{
		// read the first one which is metadata
		QJsonObject setObject = jsonArray[0].toObject();
		cameras[cameraName].cameraName = setObject["camera_name"].toString();
		cameras[cameraName].cameraDescription = setObject["camera_description"].toString();
		cameras[cameraName].cameraURLDetails = setObject["camera_url"].toString();
		cameras[cameraName].polygon_sets = polygonSets;
	}
}

void MosaicCamera::setCameraFieldDiameter(Camera& camera)
{
	// First get all corners of all polygons
	QVector<QPointF> allCorners;
	for (const auto& polygonSet : camera.polygon_sets)
	{
		for (const auto& polygon : polygonSet.corners)
		{
			allCorners.append(polygon);
		}
	}

	// Now calculate the maximum distance between any two corners
	double maxChordSq = 0.0;
	for (int i = 0; i < allCorners.size(); ++i)
	{
		for (int j = i + 1; j < allCorners.size(); ++j)
		{
			double chordSq = gnomonicChordSeparationSquared(allCorners[i], allCorners[j]);
			if (chordSq > maxChordSq)
			{
				maxChordSq = chordSq;
			}
		}
	}
	camera.fieldDiameter = 2.0 * asin(0.5 * sqrt(maxChordSq)) * M_180_PI;
}

double MosaicCamera::gnomonicChordSeparationSquared(const QPointF& p1, const QPointF& p2)
{
	// Compute unit circle coordinates
	Vec3d v1(p1.x(), p1.y(), 1.0);
	Vec3d v2(p2.x(), p2.y(), 1.0);
	v1.normalize();
	v2.normalize();

	// Compute the chordal distance
	return (v1-v2).normSquared();
}

void MosaicCamera::setRA(const QString& cameraName, double ra)
{
	if (cameras.contains(cameraName))
	{
		cameras[cameraName].ra = ra;
		if(configDialog->visible())
		{
			if (configDialog->getCurrentCameraName() == cameraName)
				configDialog->setRA(ra);
		}
	}
}

void MosaicCamera::setDec(const QString& cameraName, double dec)
{
	if (cameras.contains(cameraName))
	{
		cameras[cameraName].dec = dec;
		if(configDialog->visible())
		{
			if (configDialog->getCurrentCameraName() == cameraName)
				configDialog->setDec(dec);
		}
	}
}

void MosaicCamera::setRotation(const QString& cameraName, double rotation)
{
	if (cameras.contains(cameraName))
	{
		cameras[cameraName].rotation = rotation;
		if(configDialog->visible())
		{
			if (configDialog->getCurrentCameraName() == cameraName)
				configDialog->setRotation(rotation);
		}
	}
}

void MosaicCamera::setVisibility(const QString& cameraName, bool visible)
{
	if (cameras.contains(cameraName))
	{
		cameras[cameraName].visible = visible;
		if(configDialog->visible())
		{
			if (configDialog->getCurrentCameraName() == cameraName)
				configDialog->setVisibility(visible);
		}
	}
}

void MosaicCamera::setPosition(const QString& cameraName, double ra, double dec, double rotation)
{
	setRA(cameraName, ra);
	setDec(cameraName, dec);
	setRotation(cameraName, rotation);
}

void MosaicCamera::setRADecToView()
{
	Vec3d current = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, current);
	// convert to degrees from radians
	ra = std::fmod((ra*M_180_PI)+360., 360.);
	dec = dec*M_180_PI;
	setCurrentRA(ra);
	setCurrentDec(dec);
}

void MosaicCamera::setRADecToObject()
{
	const QList<StelObjectP> selection = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (selection.isEmpty())
		return;

	StelObjectP obj = selection[0];
	double ra, dec;
	Vec3d pos = obj->getJ2000EquatorialPos(core);
	StelUtils::rectToSphe(&ra, &dec, pos);
	ra = std::fmod((ra*M_180_PI)+360., 360.);
	dec = dec*M_180_PI;
	setCurrentRA(ra);
	setCurrentDec(dec);
}

void MosaicCamera::setViewToCamera()
{
	double ra = getRA(currentCamera);
	double dec = getDec(currentCamera);

	Vec3d aim;
	StelUtils::spheToRect(ra/M_180_PI, dec/M_180_PI, 1.0, aim);
	const Vec3d aimUp(0.0, 0.0, 1.0);
	GETSTELMODULE(StelMovementMgr)->moveToJ2000(aim, aimUp);
	GETSTELMODULE(StelMovementMgr)->zoomTo(5 * cameras[currentCamera].fieldDiameter);
}

void MosaicCamera::incrementRotation()
{
	double rotation = getRotation(currentCamera);
	rotation += 5.0;
	setRotation(currentCamera, rotation);
}

void MosaicCamera::decrementRotation()
{
	double rotation = getRotation(currentCamera);
	rotation -= 5.0;
	setRotation(currentCamera, rotation);
}

void MosaicCamera::nextCamera()
{
	int index = cameraOrder.indexOf(currentCamera);
	index = (index + 1) % cameraOrder.size();
	setCurrentCamera(cameraOrder[index]);
}

void MosaicCamera::previousCamera()
{
	int index = cameraOrder.indexOf(currentCamera);
	index = (index - 1 + cameraOrder.size()) % cameraOrder.size();
	setCurrentCamera(cameraOrder[index]);
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

void MosaicCamera::setCurrentCamera(const QString& cameraName)
{
	if (cameras.contains(cameraName))
	{
		currentCamera = cameraName;
		emit currentCameraChanged(cameraName);
		if(configDialog->visible())
			configDialog->setCurrentCameraName(cameraName, cameras[cameraName].cameraName, cameras[cameraName].cameraDescription, cameras[cameraName].cameraURLDetails);
	}
}

void MosaicCamera::setCurrentRA(double ra)
{
	setRA(currentCamera, ra);
	emit currentRAChanged(ra);
}

void MosaicCamera::setCurrentDec(double dec)
{
	setDec(currentCamera, dec);
	emit currentDecChanged(dec);
}

void MosaicCamera::setCurrentRotation(double rotation)
{
	setRotation(currentCamera, rotation);
	emit currentRotationChanged(rotation);
}

void MosaicCamera::setCurrentVisibility(bool visible)
{
	setVisibility(currentCamera, visible);
	emit currentVisibilityChanged(visible);
}

void MosaicCamera::draw(StelCore* core)
{
	if (!isEnabled())
		return;

	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);

	Vec3d startPoint, endPoint;

	for (const auto& camera : cameras)
	{
		if (!camera.visible)
			continue;

		double alpha = camera.ra / M_180_PI + M_PI_2;
		double beta = M_PI_2 - camera.dec / M_180_PI;
		double rot = camera.rotation / M_180_PI;

		double cosBeta = cos(beta);
		double sinBeta = sin(beta);

		double cosAlpha = cos(alpha);
		double sinAlpha = sin(alpha);

		for (const auto& polygonSet : camera.polygon_sets)
		{
			// Set color for this polygon set
			painter.setColor(
			                        polygonSet.color.redF(),
			                        polygonSet.color.greenF(),
			                        polygonSet.color.blueF(),
			                        polygonSet.color.alphaF()
			                        );

			for (const auto& polygon : polygonSet.corners)
			{
				// Loop through points in the polygon
				for (int i = 0; i < polygon.size(); ++i)
				{
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
