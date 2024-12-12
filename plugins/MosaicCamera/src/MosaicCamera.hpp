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

#include "MosaicTcpServer.hpp"
#include "StelModule.hpp"
#include "StelPluginInterface.hpp"
#include <QColor>
#include <QVector>
#include <QObject>
#include <QPointF>
#include <QSettings>
#include <QString>

class MosaicCameraDialog;

struct PolygonSet {
    QString name;
    QVector<QVector<QPointF>> corners;
    QColor color;
};

struct Camera {
    QString name;
	double ra;
	double dec;
	double rotation;
    bool visible;
    QVector<PolygonSet> polygon_sets;
};

class MosaicCamera : public StelModule
{
	Q_OBJECT

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

	void loadSettings();
	void saveSettings() const;

    QStringList getCameraNames() const;
    void readPolygonSetsFromJson(const QString& cameraName, const QString& filename);

public slots:
	void updateMosaic(const QString& cameraName, double ra, double dec, double rotation);

private:
    QHash<QString, Camera> cameras;
	QStringList cameraOrder;
	QString currentCamera;
	QString userDirectory;
	QSettings* conf;

    void loadBuiltInCameras();
	void loadCameraOrder();
	void initializeUserData();
	void copyResourcesToUserDirectory();

	MosaicCameraDialog* configDialog;
	MosaicTcpServer* tcpServer;
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
