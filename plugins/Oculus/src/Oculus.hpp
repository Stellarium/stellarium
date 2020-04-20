/*
 * Copyright (C) 2017 Guillaume Chereau
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

#ifndef _OCULUS_HPP_
#define _OCULUS_HPP_

#include "StelModule.hpp"
#include "StelOpenGL.hpp"
#include "OVR_CAPI_GL.h"

#include <QOpenGLFunctions>

class Oculus : public StelModule, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	Oculus();
	virtual ~Oculus();

	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	// virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual void update(double deltaTime) Q_DECL_OVERRIDE;
	// Make sure we get called after everything else.
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE {Q_UNUSED(actionName); return 100;}
public slots:
	void recenter();
private:
	ovrSession session = NULL;
	ovrGraphicsLuid luid;
	ovrTextureSwapChain chains[2];
	ovrLayerEyeFov layer;
	GLuint fbos[2];
	GLuint depthBuffers[2];
	void render();
	void drawEye(int eye, GLuint texid, int width, int height, const ovrFovPort &fov);
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class OculusStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*_OCULUS_HPP_*/
