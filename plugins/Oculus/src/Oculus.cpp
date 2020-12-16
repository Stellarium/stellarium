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

#include "Oculus.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelOpenGL.hpp"
#include "Extras/OVR_Math.h"

#include <QOpenGLFramebufferObject>

#include <assert.h>

#define OVRC(f) do { \
		ovrResult result = f; \
		if (OVR_FAILURE(result)) { \
			qWarning("OVR error %d, line %d", result, __LINE__); \
			Q_ASSERT(false); \
		} \
	} while (0)

void quat_to_mat4(const float q[4], float out[4][4])
{
    float x = q[0];
    float y = q[1];
    float z = q[2];
    float w = q[3];

    out[0][0] = 1-2*y*y-2*z*z;
    out[0][1] = 2*x*y+2*z*w;
    out[0][2] = 2*x*z-2*y*w;
    out[0][3] = 0;
    out[1][0] = 2*x*y-2*z*w;
    out[1][1] = 1-2*x*x-2*z*z;
    out[1][2] = 2*y*z+2*x*w;
    out[1][3] = 0;
    out[2][0] = 2*x*z+2*y*w;
    out[2][1] = 2*y*z-2*x*w;
    out[2][2] = 1-2*x*x-2*y*y;
    out[2][3] = 0;
    out[3][0] = 0;
    out[3][1] = 0;
    out[3][2] = 0;
    out[3][3] = 1;
}

void mat4_set_identity(float mat[4][4])
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            mat[i][j] = (i == j) ? 1.f : 0.f;
}

void mat4_translate(float m[4][4], float x, float y, float z)
{
    int i;
    for (i = 0; i < 4; i++)
       m[3][i] += m[0][i] * x + m[1][i] * y + m[2][i] * z;
}

void mat4_mul(float mat[4][4], float b[4][4])
{
    int i, j, k;
    float ret[4][4] = {{0}};
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; ++k) {
                ret[j][i] += mat[k][i] * b[j][k];
            }
        }
    }
    memcpy(mat, ret, sizeof(ret));
}

bool mat4_invert(float mat[4][4])
{
    float det;
    int i;
    float *m = (float*)mat;
    float inv[16];

#define M(i, j, k) m[i] * m[j] * m[k]
    inv[0] =   M(5, 10, 15) - M(5, 11, 14)  - M(9, 6, 15) +
               M(9, 7, 14)  + M(13, 6, 11)  - M(13, 7, 10);
    inv[4] =  -M(4, 10, 15) + M( 4, 11, 14) + M( 8, 6, 15) -
               M(8, 7, 14)  - M(12,  6, 11) + M(12, 7, 10);
    inv[8] =   M(4,  9, 15) - M( 4, 11, 13) - M(8,  5, 15) +
               M(8,  7, 13) + M(12,  5, 11) - M(12, 7, 9);
    inv[12] = -M(4, 9, 14)  + M(4, 10, 13)  + M(8, 5, 14) -
               M(8, 6, 13)  - M(12, 5, 10)  + M(12, 6, 9);
    inv[1] =  -M(1, 10, 15) + M(1, 11, 14)  + M(9, 2, 15) -
               M(9, 3, 14)  - M(13, 2, 11)  + M(13, 3, 10);
    inv[5] =   M(0, 10, 15) - M(0, 11, 14)  - M(8, 2, 15) +
               M(8, 3, 14)  + M(12, 2, 11)  - M(12, 3, 10);
    inv[9] =  -M(0, 9, 15)  + M(0, 11, 13)  + M(8, 1, 15) -
               M(8, 3, 13)  - M(12, 1, 11)  + M(12, 3, 9);
    inv[13] =  M(0, 9, 14)  - M(0, 10, 13)  - M(8, 1, 14) +
               M(8, 2, 13)  + M(12, 1, 10)  - M(12, 2, 9);
    inv[2] =   M(1, 6, 15)  - M(1, 7, 14)   - M(5, 2, 15) +
               M(5, 3, 14)  + M(13, 2, 7)   - M(13, 3, 6);
    inv[6] =  -M(0, 6, 15)  + M(0, 7, 14)   + M(4, 2, 15) -
               M(4, 3, 14)  - M(12, 2, 7)   + M(12, 3, 6);
    inv[10] =  M(0, 5, 15)  - M(0, 7, 13)   - M(4, 1, 15) +
               M(4, 3, 13)  + M(12, 1, 7)   - M(12, 3, 5);
    inv[14] = -M(0, 5, 14)  + M(0, 6, 13)   + M(4, 1, 14) -
               M(4, 2, 13)  - M(12, 1, 6)   + M(12, 2, 5);
    inv[3] =  -M(1, 6, 11)  + M(1, 7, 10)   + M(5, 2, 11) -
               M(5, 3, 10)  - M(9, 2, 7)    + M(9, 3, 6);
    inv[7] =   M(0, 6, 11)  - M(0, 7, 10)   - M(4, 2, 11) +
               M(4, 3, 10)  + M(8, 2, 7)    - M(8, 3, 6);
    inv[11] = -M(0, 5, 11)  + M(0, 7, 9)    + M(4, 1, 11) -
               M(4, 3, 9)   - M(8, 1, 7)    + M(8, 3, 5);
    inv[15] =  M(0, 5, 10)  - M(0, 6, 9)    - M(4, 1, 10) +
               M(4, 2, 9)   + M(8, 1, 6)    - M(8, 2, 5);
#undef M

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0) {
        return false;
    }

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        ((float*)mat)[i] = inv[i] * det;
    return true;
}

static ovrMatrix4f view_from_pose(ovrPosef pose)
{
    // XXX: use stellarium matrix so that we can remove the mat functions.
    ovrMatrix4f ret;
    float rot[4][4];

    quat_to_mat4((float*)&pose.Orientation, rot);
    mat4_set_identity(ret.M);
    mat4_translate(ret.M, pose.Position.x, pose.Position.y, pose.Position.z);
    mat4_mul(ret.M, rot);

    mat4_invert(ret.M);
    return ret;
}

StelModule* OculusStelPluginInterface::getStelModule() const
{
	return new Oculus();
}

StelPluginInfo OculusStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "Oculus";
	info.displayedName = N_("Oculus");
	info.authors = "Guillaume Chereau";
	info.contact = "guillaume@noctua-software.com";
	info.description = N_("Support for Oculus Rift");
	info.version = OCULUS_PLUGIN_VERSION;
	info.startByDefault = true;
	return info;
}


Oculus::Oculus()
{
	setObjectName("Oculus");
	StelActionMgr *actionsMgr = StelApp::getInstance().getStelActionManager();
	actionsMgr->findAction("actionGoto_Selected_Object")->setShortcut("");
	actionsMgr->addAction("actionOculus_recenter", "Oculus", "recenter", this, "recenter()", "Space");
}

Oculus::~Oculus()
{
}

void Oculus::init()
{
	qDebug() << "Oculus plugin.";
	initializeOpenGLFunctions();
	OVRC(ovr_Initialize(nullptr));
	ovrHmdDesc hmd = ovr_GetHmdDesc(nullptr);
	if (hmd.Type == ovrHmd_None)
	{
		qDebug() << "No Oculus connected";
		return;
	}
	OVRC(ovr_Create(&session, &luid));

	// Create the textures and layer.
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	for (int i = 0; i < 2; i++)
	{
		ovrEyeRenderDesc eyeRenderDesc = ovr_GetRenderDesc(session, (ovrEyeType)i, hmd.DefaultEyeFov[i]);
		ovrSizei recommenedSize = ovr_GetFovTextureSize(session, (ovrEyeType)i, hmd.DefaultEyeFov[i], 1.0f);
		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.Width = recommenedSize.w;
		desc.Height = recommenedSize.h;
		desc.MipLevels = 1;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;
		OVRC(ovr_CreateTextureSwapChainGL(session, &desc, &chains[i]));
		layer.Viewport[i] = {{0, 0}, {desc.Width, desc.Height}};
		layer.Fov[i] = eyeRenderDesc.Fov;
		layer.ColorTexture[i] = chains[i];

		glGenFramebuffers(1, &fbos[i]);
		glGenRenderbuffers(1, &depthBuffers[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffers[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, desc.Width, desc.Height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffers[i]);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		Q_ASSERT(status == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void Oculus::deinit()
{
	if (!session) return;
	for (int i = 0; i < 2; i++)
	{
		ovr_DestroyTextureSwapChain(session, chains[i]);
		// XXX: delete fbos!
	}
	ovr_Destroy(session);
	ovr_Shutdown();
}

void Oculus::update(double deltaTime)
{
	Q_UNUSED(deltaTime);
	if (!session) return;
	ovrHmdDesc hmd = ovr_GetHmdDesc(session);

	double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
	ovrTrackingState ts = ovr_GetTrackingState(session, displayMidpointSeconds, ovrTrue);

	if (!(ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)))
		return;
	ovrPosef pose = ts.HeadPose.ThePose;
	StelCore* core = StelApp::getInstance().getCore();

	// Set the view direction.
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	QQuaternion quaternion(pose.Orientation.w, pose.Orientation.x, pose.Orientation.y, pose.Orientation.z);
	QVector3D qViewDirection = quaternion.rotatedVector(QVector3D(0, 0, -1));
	Vec3d viewDirection(qViewDirection.x(), -qViewDirection.z(), qViewDirection.y());
	QVector3D qUp = quaternion.rotatedVector(QVector3D(0, 1, 0));
	Vec3d up(qUp.x(), -qUp.z(), qUp.y());
	mmgr->setViewDirectionJ2000(core->altAzToJ2000(viewDirection));
	mmgr->setViewUpVectorJ2000(core->altAzToJ2000(up));
	core->setCurrentProjectionType(StelCore::ProjectionPerspective);
	double hFov = (atan(hmd.DefaultEyeFov[0].LeftTan) + atan(hmd.DefaultEyeFov[0].RightTan)) / M_PI * 180;
	mmgr->zoomTo(hFov, 0);
	render();
	// Make sure we never go into low fps mode.
	StelMainView::getInstance().thereWasAnEvent();
}

void Oculus::render()
{
	ovrPosef hmdToEyeViewOffset[2];
	ovrHmdDesc hmd = ovr_GetHmdDesc(session);
	for (int eye = 0; eye < 2; eye++)
	{
		ovrEyeRenderDesc eyeRenderDesc = ovr_GetRenderDesc(session, (ovrEyeType)eye, hmd.DefaultEyeFov[eye]);
		hmdToEyeViewOffset[eye] = eyeRenderDesc.HmdToEyePose;
	}
	double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
	ovrTrackingState hmdState = ovr_GetTrackingState(session, displayMidpointSeconds, ovrTrue);
	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, layer.RenderPose);

	for (int eye = 0; eye < 2; eye++)
	{
		GLuint texid;
		ovrTextureSwapChainDesc desc;
		OVRC(ovr_GetTextureSwapChainDesc(session, chains[eye], &desc));
		OVRC(ovr_GetTextureSwapChainBufferGL(session, chains[eye], -1, &texid));
		drawEye(eye, texid, desc.Width, desc.Height, layer.Fov[eye]);
		OVRC(ovr_CommitTextureSwapChain(session, chains[eye]));
	}

	ovrLayerHeader* layers = &layer.Header;
	OVRC(ovr_SubmitFrame(session, 0, NULL, &layers, 1));
}

void Oculus::drawEye(int eye, GLuint texid, int width, int height, const ovrFovPort &fov)
{
	StelApp* stelApp = &StelApp::getInstance();
	ovrMatrix4f proj, view;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[eye]);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);
	stelApp->glWindowHasBeenResized(QRectF(0, 0, width, height));
	double xoffset = (fov.LeftTan - fov.RightTan) / (fov.LeftTan + fov.RightTan) / 2. * 100.;
	stelApp->getCore()->setViewportHorizontalOffset(xoffset);
	double yoffset = (fov.DownTan - fov.UpTan) / (fov.DownTan + fov.UpTan) / 2. * 100.;
	stelApp->getCore()->setViewportVerticalOffset(yoffset);

	view = view_from_pose(layer.RenderPose[eye]);
	Mat4d mat = Mat4d(
		    view.M[0][0], view.M[0][1], view.M[0][2], view.M[0][3],
		    view.M[1][0], view.M[1][1], view.M[1][2], view.M[1][3],
		    view.M[2][0], view.M[2][1], view.M[2][2], view.M[2][3],
		    0, 0, 0, 1);
	mat = mat * Mat4d::xrotation(-M_PI / 2);
	stelApp->getCore()->setMatAltAzModelView(mat);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	const QList<StelModule*> modules = stelApp->getModuleMgr().getCallOrders(StelModule::ActionDraw);
	foreach(StelModule* module, modules)
	{
		module->draw(stelApp->getCore());
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Oculus::recenter()
{
	OVRC(ovr_RecenterTrackingOrigin(session));
}
