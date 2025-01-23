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

#include "StelModule.hpp"
#include "StelPluginInterface.hpp"

class MosaicCameraDialog;

/*! @defgroup mosaicCamera Mosaic Camera plug-in
@{
The Mosaic Camera plugin overlays camera sensor boundaries on the sky.
@}
*/

/// @brief Represents a set of polygons with associated properties.
struct PolygonSet
{
    QString name;                      ///< The name of the polygon set.
    QVector<QVector<QPointF>> corners; ///< Polygons as vectors of corner points.
    QColor color;                      ///< Color associated with the polygon set.
};


/// @brief Represents a camera with its properties and associated polygon sets.
struct Camera
{
    QString name;                    ///< The name of the camera.
    double ra;                       ///< Right Ascension of camera pointing [deg]
    double dec;                      ///< Declination of camera pointing [deg]
    double rotation;                 ///< Rotation angle of the camera [deg]
    bool visible;                    ///< Visibility status of the camera.
    QVector<PolygonSet> polygon_sets; ///< Collection of polygon sets associated with the camera.
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

	/// @property currentCamera
	/// @brief The name of the current camera
	Q_PROPERTY(QString currentCamera READ getCurrentCamera WRITE setCurrentCamera )

	/// @property ra
	/// @brief Set or get the current camera's right ascension [deg]
	Q_PROPERTY(double ra             READ getRA            WRITE setRA )

	/// @property dec
	/// @brief Set or get the current camera's declination [deg]
	Q_PROPERTY(double dec            READ getDec           WRITE setDec )

	/// @property rotation
	/// @brief Set or get the current camera's rotation [deg]
	Q_PROPERTY(double rotation       READ getRotation      WRITE setRotation )

	/// @property visible
	/// @brief Set or get the current camera's visibility
	Q_PROPERTY(bool visible          READ getVisibility    WRITE setVisibility )

    /** @} */

public:
	MosaicCamera();
	~MosaicCamera() override;

	void init() override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	bool configureGui(bool show=true) override;

    void setRA(const QString& cameraName, double ra);
    void setDec(const QString& cameraName, double dec);
    void setRotation(const QString& cameraName, double rotation);
    void setVisibility(const QString& cameraName, bool visible);

    double getRA(const QString& cameraName) const;
    double getDec(const QString& cameraName) const;
    double getRotation(const QString& cameraName) const;
    bool getVisibility(const QString& cameraName) const;

	QString getCurrentCamera() const { return currentCamera; }
	double getRA() const { return getRA(currentCamera); }
	double getDec() const { return getDec(currentCamera); }
	double getRotation() const { return getRotation(currentCamera); }
	double getVisibility() const { return getVisibility(currentCamera); }

	void loadSettings();
	void saveSettings() const;

    QStringList getCameraNames() const;
    void readPolygonSetsFromJson(const QString& cameraName, const QString& filename);

public slots:
	void setCurrentCamera(const QString& cameraName);
	void setRA(double ra);
	void setDec(double dec);
	void setRotation(double rotation);
	void setVisibility(bool visible);

private:
    QHash<QString, Camera> cameras;
	QStringList cameraOrder;
	QString userDirectory;
	QSettings* conf;

	QString currentCamera;

    void loadBuiltInCameras();
	void loadCameraOrder();
	void initializeUserData();
	void copyResourcesToUserDirectory();

	MosaicCameraDialog* configDialog;
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
