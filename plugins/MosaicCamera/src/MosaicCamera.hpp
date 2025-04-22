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

#ifndef MOSAICCAMERA_HPP
#define MOSAICCAMERA_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"
#include "StelPluginInterface.hpp"

class StelButton;
class MosaicCameraDialog;

/*! @defgroup mosaicCamera Mosaic Camera plug-in
@{
The Mosaic Camera plugin overlays camera sensor boundaries on the sky.
@}
*/

/// @brief Represents a set of polygons with associated properties.
struct PolygonSet
{
	QString name;						///< The name of the polygon set.
	QVector<QVector<QPointF>> corners;	///< Polygons as vectors of corner points.
	QColor color;						///< Color associated with the polygon set.
};


/// @brief Represents a camera with its properties and associated polygon sets.
struct Camera
{
	QString name;						///< The name of the camera.
	QString cameraName;					///< The name of the camera in the GUI.
	QString cameraDescription;			///< The description of the camera.
	QString cameraURLDetails;			///< URL for more details about the camera.
	double ra;							///< Right Ascension of camera pointing [deg]
	double dec;							///< Declination of camera pointing [deg]
	double rotation;					///< Rotation angle of the camera [deg]
	bool visible;						///< Visibility status of the camera.
	QVector<PolygonSet> polygon_sets;	///< Collection of polygon sets associated with the camera.
	double fieldDiameter;				///< Estimated field diameter of the camera [deg]
};

//! @class MosaicCamera
//! @ingroup mosaicCamera
//! Main class of the Mosaic Camera plug-in.
//! @author Josh Meyers
class MosaicCamera : public StelModule
{
	Q_OBJECT
	/**
	* @name Camera properties
	* @{
	* We maintain the concept of a "current" camera primarily for the convenience of
	* setting and getting properties through the Stellarium Remove Control HTTP API.
	*/

	/// @property enabled
	/// @brief Are mosaic camera overlays enabled?
	Q_PROPERTY(bool enabled			READ isEnabled			WRITE enableMosaicCamera		NOTIFY flagMosaicCameraVisibilityChanged)

	Q_PROPERTY(bool showButton		READ getFlagShowButton	WRITE setFlagShowButton		NOTIFY flagShowButtonChanged)

	/// @property currentCamera
	/// @brief The name of the current camera
	Q_PROPERTY(QString currentCamera	READ getCurrentCamera	WRITE setCurrentCamera		NOTIFY currentCameraChanged)

	/// @property ra
	/// @brief Set or get the current camera's right ascension [deg]
	Q_PROPERTY(double ra				READ getCurrentRA		WRITE setCurrentRA			NOTIFY currentRAChanged)

	/// @property dec
	/// @brief Set or get the current camera's declination [deg]
	Q_PROPERTY(double dec			READ getCurrentDec		WRITE setCurrentDec			NOTIFY currentDecChanged)

	/// @property rotation
	/// @brief Set or get the current camera's rotation [deg]
	Q_PROPERTY(double rotation		READ getCurrentRotation	WRITE setCurrentRotation		NOTIFY currentRotationChanged)

	/// @property visible
	/// @brief Set or get the current camera's visibility
	Q_PROPERTY(bool visible			READ getCurrentVisibility	WRITE setCurrentVisibility		NOTIFY currentVisibilityChanged)

	/** @} */

public:
	MosaicCamera();
	~MosaicCamera() override;

	void init() override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	bool configureGui(bool show=true) override;

	Q_INVOKABLE double getRA(const QString& cameraName) const;
	Q_INVOKABLE double getDec(const QString& cameraName) const;
	Q_INVOKABLE double getRotation(const QString& cameraName) const;
	Q_INVOKABLE bool getVisibility(const QString& cameraName) const;

	QString getCurrentCamera() const { return currentCamera; }
	double getCurrentRA() const { return getRA(currentCamera); }
	double getCurrentDec() const { return getDec(currentCamera); }
	double getCurrentRotation() const { return getRotation(currentCamera); }
	double getCurrentVisibility() const { return getVisibility(currentCamera); }

	bool isEnabled() const { return flagShowMosaicCamera; }
	bool getFlagShowButton() const { return flagShowButton; }

	void loadSettings();

	QStringList getCameraNames() const;
	void readPolygonSetsFromJson(const QString& cameraName, const QString& filename);

signals:
	void flagMosaicCameraVisibilityChanged(bool b);
	void flagShowButtonChanged(bool b);
	void currentCameraChanged(const QString& cameraName);
	void currentRAChanged(double ra);
	void currentDecChanged(double dec);
	void currentRotationChanged(double rotation);
	void currentVisibilityChanged(bool visible);

public slots:
	void setRA(const QString& cameraName, double ra);
	void setDec(const QString& cameraName, double dec);
	void setRotation(const QString& cameraName, double rotation);
	void setVisibility(const QString& cameraName, bool visible);
	void setPosition(const QString& cameraName, double ra, double dec, double rotation);
	void setRADecToView();
	void setRADecToObject();
	void setViewToCamera();
	void incrementRotation();
	void decrementRotation();
	void nextCamera();
	void previousCamera();

	void enableMosaicCamera(bool b);
	void setFlagShowButton(bool b);
	void setCurrentCamera(const QString& cameraName);
	void setCurrentRA(double ra);
	void setCurrentDec(double dec);
	void setCurrentRotation(double rotation);
	void setCurrentVisibility(bool visible);

	void saveSettings() const;
	void restoreDefaults();

private:
	QHash<QString, Camera> cameras;
	QStringList cameraOrder;
	QString userDirectory;
	QSettings* conf;

	QString currentCamera;
	bool flagShowMosaicCamera;
	bool flagShowButton;

	void loadBuiltInCameras();
	void loadCameraOrder();
	void initializeUserData();
	void copyResourcesToUserDirectory();
	void setCameraFieldDiameter(Camera& camera);
	static double gnomonicChordSeparationSquared(const QPointF& p1, const QPointF& p2);

	StelGui* gui;
	MosaicCameraDialog* configDialog;
	StelButton* toolbarButton;
	StelCore* core;
};

class MosaicCameraStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif /* MOSAICCAMERA_HPP */
