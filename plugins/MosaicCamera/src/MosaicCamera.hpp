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
#include <QFont>

class MosaicCameraDialog;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class MosaicCamera : public StelModule
{
	Q_OBJECT

public:
	MosaicCamera();
	~MosaicCamera() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	bool configureGui(bool show=true) override;

	void setRA(double ra);
	void setDec(double dec);
	void setRot(double rot);

private:
	double ra;
	double dec;
	double rot;
	// Font used for displaying our text
	QFont font;
	// GUI
	MosaicCameraDialog* configDialog;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class MosaicCameraStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* MOSAICCAMERA_HPP */
