/*
 * Copyright (C) 2023 Ruslan Kabatsayev
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

#include "LensDistortionEstimator.hpp"
#include "LensDistortionEstimatorDialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelCore.hpp"
#include "StelSRGB.hpp"
#include "StelUtils.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"

#include <QDebug>

namespace
{
constexpr float POINT_MARKER_RADIUS = 7;
constexpr int FS_QUAD_COORDS_PER_VERTEX = 2;
constexpr int SKY_VERTEX_ATTRIB_INDEX = 0;
constexpr char defaultImageAxesColor[] = "1,0.5,0";
constexpr char defaultPointMarkerColor[] = "1,0,0";
constexpr char defaultSelectedPointMarkerColor[] = "0,0.75,0";
constexpr char defaultProjectionCenterMarkerColor[] = "0.25,0.5,0.5";
}

StelModule* LensDistortionEstimatorStelPluginInterface::getStelModule() const
{
	return new LensDistortionEstimator();
}

StelPluginInfo LensDistortionEstimatorStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(LensDistortionEstimator);

	StelPluginInfo info;
	info.id = "LensDistortionEstimator";
	info.displayedName = N_("Lens distortion estimator");
	info.authors = "Ruslan Kabatsayev";
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("Estimates lens distortion by letting the user match stars to a photo and computing a fit.");
	info.version = LENSDISTORTIONESTIMATOR_PLUGIN_VERSION;
	info.license = LENSDISTORTIONESTIMATOR_PLUGIN_LICENSE;
	return info;
}

LensDistortionEstimator::LensDistortionEstimator()
	: conf_(StelApp::getInstance().getSettings())
{
	setObjectName("LensDistortionEstimator");
}

LensDistortionEstimator::~LensDistortionEstimator()
{
}

double LensDistortionEstimator::getCallOrder(StelModuleActionName actionName) const
{
	if(actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("StarMgr")->getCallOrder(actionName)-1;
	if(actionName==StelModule::ActionHandleMouseClicks)
		return -11;
	return 0;
}

Vec2d LensDistortionEstimator::screenPointToImagePoint(const int x, const int y) const
{
	const auto prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	Vec3d point = Vec3d(0,0,0);
	prj->unProject(x, y, point);

	const auto viewDir = point;
	const auto imageCenterShift = dialog_->imageCenterShift();
	const auto imageSmallerSideFoV = M_PI/180 * dialog_->imageSmallerSideFoV();
	const auto projectionCenterDir = dialog_->projectionCenterDir();
	const auto imageUpDir = dialog_->imageUpDir();
	const double W = imageTex_->width();
	const double H = imageTex_->height();
	const double minSize = std::min(W,H);
	const Vec2d normCoordToTexCoordCoefs = (minSize-1)*Vec2d(1./W,1./H);

	using namespace std;
	const auto imageRightDir = normalize(projectionCenterDir ^ imageUpDir);
	const auto R = acos(clamp(dot(viewDir, projectionCenterDir), -1., 1.));
	const auto angle = atan2(dot(viewDir, imageUpDir),
	                         dot(viewDir, imageRightDir));
	const auto normalizedR = tan(R) / tan(imageSmallerSideFoV/2);
	const auto distortedR = dialog_->applyDistortion(normalizedR);
	const Vec2d centeredTexCoord = normCoordToTexCoordCoefs * (distortedR * Vec2d(cos(angle), sin(angle)) + imageCenterShift);
	const Vec2d posInImage((0.5 + 0.5 * centeredTexCoord[0]) * W - 0.5,
	                       (0.5 - 0.5 * centeredTexCoord[1]) * H - 0.5);
	return posInImage;
}

void LensDistortionEstimator::handleMouseClicks(QMouseEvent* event)
{
	if(!dialog_ || !dialog_->initialized() || !imageTex_)
	{
		event->setAccepted(false);
		return;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const double x = event->position().x(), y = event->position().y();
#else
	const double x = event->x(), y = event->y();
#endif
	const auto posInImage = screenPointToImagePoint(x,y);
	if(dialog_->isPickingAPoint())
	{
		draggingImage_ = false; // Just in case

		if(event->type() == QEvent::MouseButtonPress)
		{
			event->setAccepted(true);
			return;
		}
		if(event->type() != QEvent::MouseButtonRelease)
		{
			event->setAccepted(false);
			return;
		}

		const auto objs = StelApp::getInstance().getStelObjectMgr().getSelectedObject();
		if(objs.isEmpty())
		{
			qWarning() << "Picked a point while nothing was selected... dropping the result";
			event->setAccepted(false);
			return;
		}
		if(!objs.last())
		{
			qWarning() << "NULL object is selected... dropping the result of picking";
			event->setAccepted(false);
			return;
		}
		dialog_->resetImagePointPicking();
		dialog_->registerImagePoint(*objs[0], posInImage);
		event->setAccepted(true);
		return;
	}

	if(draggingImage_ && event->type() == QEvent::MouseButtonRelease)
	{
		draggingImage_ = false;
		imgPointToRotateAbout_ = screenPointToImagePoint(x,y);
		event->setAccepted(true);
		return;
	}

	// If the modifiers are not what we use for dragging, or the button is wrong, we have nothing to do
	const auto modifiers = qApp->keyboardModifiers() & (Qt::ShiftModifier|Qt::ControlModifier|Qt::AltModifier);
	if(modifiers != (Qt::ShiftModifier|Qt::ControlModifier))
	{
		event->setAccepted(false);
		return;
	}

	// To start dragging the image, the cursor must be inside
	if(posInImage[0] < -0.5 || posInImage[0] > imageTex_->width() - 0.5 ||
	   posInImage[1] < -0.5 || posInImage[1] > imageTex_->height() - 0.5)
	{
		event->setAccepted(false);
		return;
	}

	if(event->button() == Qt::LeftButton)
	{
		dragStartProjCenterElevation_ = dialog_->projectionCenterElevation();
		dragStartProjCenterAzimuth_ = dialog_->projectionCenterAzimuth();
		dragStartImgFieldRotation_ = dialog_->imageFieldRotation();
		dragStartPoint_ = QPointF(x,y);
		draggingImage_ = true;
		rotatingImage_ = false;
		event->setAccepted(true);
	}
	else if(event->button() == Qt::RightButton)
	{
		dragStartImgSmallerSideFoV_ = dialog_->imageSmallerSideFoV();
		dragStartProjCenterElevation_ = dialog_->projectionCenterElevation();
		dragStartProjCenterAzimuth_ = dialog_->projectionCenterAzimuth();
		dragStartImgFieldRotation_ = dialog_->imageFieldRotation();
		dragStartPoint_ = QPointF(x,y);
		draggingImage_ = false;
		rotatingImage_ = true;
		event->setAccepted(true);
	}
}

void LensDistortionEstimator::dragImage(const Vec2d& src, const Vec2d& dst)
{
	// These are used for computations, so initialize them to the starting values
	dialog_->setProjectionCenterElevation(dragStartProjCenterElevation_);
	dialog_->setProjectionCenterAzimuth(dragStartProjCenterAzimuth_);
	dialog_->setImageFieldRotation(dragStartImgFieldRotation_);

	using namespace std;
	// Trying to preserve the screen-up direction of the image
	const auto prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	Vec3d srcDir1(0,0,0), srcDir2(0,0,0);
	Vec3d dstDir1(0,0,0), dstDir2(0,0,0);
	prj->unProject(src[0],src[1]  , srcDir1);
	prj->unProject(src[0],src[1]-1, srcDir2);
	prj->unProject(dst[0],dst[1]  , dstDir1);
	prj->unProject(dst[0],dst[1]-1, dstDir2);

	      auto crossDst = dstDir1 ^ dstDir2;
	const auto crossSrc = srcDir1 ^ srcDir2;

	// Make sure angle between destination vectors is the same as between the source ones
	const Mat3d rotToMatchAngle = Mat4d::rotation(normalize(crossDst),
	                                              asin(crossSrc.norm())-asin(crossDst.norm())).upper3x3();
	dstDir2 = rotToMatchAngle * dstDir2;
	// Recompute cross product
	crossDst = dstDir1 ^ dstDir2;

	// Find the matrix to rotate two points to the desired directions in space
	const auto preRotSrc = Mat3d( srcDir1[0], srcDir1[1], srcDir1[2],
	                              srcDir2[0], srcDir2[1], srcDir2[2],
	                             crossSrc[0],crossSrc[1],crossSrc[2]);
	const auto preRotDst = Mat3d( dstDir1[0],  dstDir1[1],  dstDir1[2],
	                              dstDir2[0],  dstDir2[1],  dstDir2[2],
	                             crossDst[0], crossDst[1], crossDst[2]);
	const auto rotator = preRotDst * preRotSrc.inverse();

	const auto origUpDir = dialog_->imageUpDir();
	const auto origCenterDir = dialog_->projectionCenterDir();
	const auto newCenterDir = normalize(rotator * origCenterDir);
	const auto elevation = asin(clamp(newCenterDir[2], -1.,1.));
	const auto azimuth = atan2(newCenterDir[1], -newCenterDir[0]);
	dialog_->setProjectionCenterAzimuth(180/M_PI * azimuth);
	dialog_->setProjectionCenterElevation(180/M_PI * elevation);

	const auto origFromCenterToTop = normalize(origCenterDir + 1e-8 * origUpDir);
	const auto newFromCenterToTop = normalize(rotator * origFromCenterToTop);
	// Desired up direction
	const auto upDirNew = normalize(newFromCenterToTop - newCenterDir);
	// Renewed up direction takes into account the new center direction but not the desired orientation yet
	dialog_->setImageFieldRotation(0);
	const auto renewedUpDir = dialog_->imageUpDir();
	const auto upDirCross = renewedUpDir ^ upDirNew;
	const auto upDirDot = dot(renewedUpDir, upDirNew);
	const auto dirSign = dot(upDirCross, newCenterDir) > 0 ? -1. : 1.;
	const auto upDirSinAngle = dirSign * (dirSign * upDirCross).norm();
	const auto upDirCosAngle = upDirDot;
	const auto fieldRotation = atan2(upDirSinAngle, upDirCosAngle);
	dialog_->setImageFieldRotation(180/M_PI * fieldRotation);
}

void LensDistortionEstimator::rotateImage(const Vec2d& dragSrcPoint, const Vec2d& dragDstPoint)
{
	// These are used for computations, so initialize them to the starting values
	dialog_->setImageSmallerSideFoV(dragStartImgSmallerSideFoV_);
	dialog_->setProjectionCenterElevation(dragStartProjCenterElevation_);
	dialog_->setProjectionCenterAzimuth(dragStartProjCenterAzimuth_);
	dialog_->setImageFieldRotation(dragStartImgFieldRotation_);

	using namespace std;

	const auto srcPointInImg = screenPointToImagePoint(dragSrcPoint[0],dragSrcPoint[1]);
	auto srcDir = dialog_->computeImagePointDir(srcPointInImg);
	const auto desiredFixedDir = dialog_->computeImagePointDir(imgPointToRotateAbout_);
	const auto prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	Vec3d dstDir(0,0,0);
	prj->unProject(dragDstPoint[0],dragDstPoint[1], dstDir);

	// Find the correct FoV to make angle between the two image points correct
	const auto desiredAngleBetweenDirs = acos(clamp(dot(desiredFixedDir, dstDir), -1., 1.));
	double fovMax = 0.999*M_PI;
	double fovMin = 0;
	double fov = (fovMin + fovMax) / 2.;
	for(int n = 0; n < 53 && fovMin != fovMax; ++n)
	{
		const auto angleBetweenImagePoints = dialog_->computeAngleBetweenImagePoints(imgPointToRotateAbout_,
		                                                                             srcPointInImg, fov);
		if(angleBetweenImagePoints > desiredAngleBetweenDirs)
			fovMax = fov;
		else
			fovMin = fov;
		fov = (fovMin + fovMax) / 2.;
	}
	dialog_->setImageSmallerSideFoV(180/M_PI * fov);

	// Recompute the directions after FoV update
	const auto scaledImageFixedDir = dialog_->computeImagePointDir(imgPointToRotateAbout_);
	srcDir = dialog_->computeImagePointDir(srcPointInImg);

	// Find the matrix to rotate two image points to corresponding directions in space
	const auto crossDst = desiredFixedDir ^ dstDir;
	const auto crossSrc = scaledImageFixedDir ^ srcDir;
	const auto preRotSrc = Mat3d(scaledImageFixedDir[0], scaledImageFixedDir[1], scaledImageFixedDir[2],
	                                          srcDir[0],              srcDir[1],              srcDir[2],
	                                        crossSrc[0],            crossSrc[1],            crossSrc[2]);
	const auto preRotDst = Mat3d(desiredFixedDir[0], desiredFixedDir[1], desiredFixedDir[2],
	                                      dstDir[0],          dstDir[1],          dstDir[2],
	                                    crossDst[0],        crossDst[1],        crossDst[2]);
	const auto rotator = preRotDst * preRotSrc.inverse();

	const auto origUpDir = dialog_->imageUpDir();
	const auto origCenterDir = dialog_->projectionCenterDir();
	const auto newCenterDir = normalize(rotator * origCenterDir);
	const auto elevation = asin(clamp(newCenterDir[2], -1.,1.));
	const auto azimuth = atan2(newCenterDir[1], -newCenterDir[0]);
	dialog_->setProjectionCenterAzimuth(180/M_PI * azimuth);
	dialog_->setProjectionCenterElevation(180/M_PI * elevation);

	const auto origFromCenterToTop = normalize(origCenterDir + 1e-8 * origUpDir);
	const auto newFromCenterToTop = normalize(rotator * origFromCenterToTop);
	// Desired up direction
	const auto upDirNew = normalize(newFromCenterToTop - newCenterDir);
	// Renewed up direction takes into account the new center direction but not the desired orientation yet
	dialog_->setImageFieldRotation(0);
	const auto renewedUpDir = dialog_->imageUpDir();
	const auto upDirCross = renewedUpDir ^ upDirNew;
	const auto upDirDot = dot(renewedUpDir, upDirNew);
	const auto dirSign = dot(upDirCross, newCenterDir) > 0 ? -1. : 1.;
	const auto upDirSinAngle = dirSign * (dirSign * upDirCross).norm();
	const auto upDirCosAngle = upDirDot;
	const auto fieldRotation = atan2(upDirSinAngle, upDirCosAngle);
	dialog_->setImageFieldRotation(180/M_PI * fieldRotation);
}

bool LensDistortionEstimator::handleMouseMoves(const int x, const int y, const Qt::MouseButtons buttons)
{
	if(!dialog_ || !dialog_->initialized() || !imageTex_) return false;
	if(draggingImage_ && (buttons & Qt::LeftButton))
	{
		dragImage(Vec2d(dragStartPoint_.x(),dragStartPoint_.y()), Vec2d(x,y));
		return true;
	}
	if(rotatingImage_ && (buttons & Qt::RightButton))
	{
		rotateImage(Vec2d(dragStartPoint_.x(),dragStartPoint_.y()), Vec2d(x,y));
		return true;
	}

	draggingImage_ = false;
	rotatingImage_ = false;
	return false;
}

void LensDistortionEstimator::setupCurrentVAO()
{
	auto& gl = *QOpenGLContext::currentContext()->functions();
	vbo_->bind();
	gl.glVertexAttribPointer(0, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0);
	vbo_->release();
	gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
}

void LensDistortionEstimator::bindVAO()
{
	if(vao_->isCreated())
		vao_->bind();
	else
		setupCurrentVAO();
}

void LensDistortionEstimator::releaseVAO()
{
	if(vao_->isCreated())
	{
		vao_->release();
	}
	else
	{
		auto& gl = *QOpenGLContext::currentContext()->functions();
		gl.glDisableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
	}
}

void LensDistortionEstimator::init()
{
	dialog_.reset(new LensDistortionEstimatorDialog(this));

	if(!conf_->childGroups().contains("LensDistortionEstimator"))
		restoreDefaultSettings();
	loadSettings();

	addAction("action_LensDistortionEstimator_togglePointPicking", N_("Lens Distortion Estimator"),
	          N_("Toggle pick-a-point mode"), this,
	          [this]{ dialog_->togglePointPickingMode(); }, "Ctrl+V");
	addAction("actionShow_LensDistortionEstimator_dialog", N_("Lens Distortion Estimator"),
	          N_("Show lens distortion estimator dialog"), "dialogVisible", "Ctrl+E");
	// Make sure that opening/closing the dialog by other means than dialogVisible property is reflected on this property
	connect(dialog_.get(), &StelDialog::visibleChanged, this, [this](const bool visible)
	        {
	            if(visible != dialogVisible)
	            {
	                dialogVisible = visible;
	                emit dialogToggled(visible);
	            }
	        });

	toolbarButton_ = new StelButton(nullptr,
	                                QPixmap(":/LensDistortionEstimator/bt_LensDistortionEstimator_On.png"),
	                                QPixmap(":/LensDistortionEstimator/bt_LensDistortionEstimator_Off.png"),
	                                QPixmap(":/graphicGui/miscGlow32x32.png"),
	                                "actionShow_LensDistortionEstimator_dialog",
	                                false,
	                                nullptr);
	if(const auto gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui()))
	{
		gui->getButtonBar()->addButton(toolbarButton_, "065-pluginsGroup");
	}

	auto& gl = *QOpenGLContext::currentContext()->functions();

	vbo_.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
	vbo_->create();
	vbo_->bind();
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	vbo_->release();

	vao_.reset(new QOpenGLVertexArrayObject);
	vao_->create();
	bindVAO();
	setupCurrentVAO();
	releaseVAO();
}

bool LensDistortionEstimator::configureGui(const bool show)
{
	if(show)
	{
		dialog_->setVisible(show);
		dialog_->showSettingsPage();
	}
	return true;
}

QCursor LensDistortionEstimator::getPointPickCursor() const
{
	const int size = 32;
	const QPointF center(size/2-0.5,size/2-0.5);
	QPixmap pixmap(size,size);
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	const float scale = StelApp::getInstance().getDevicePixelsPerPixel();
	const float radius = 0.5 + std::min((size-1)/2.f, POINT_MARKER_RADIUS * scale);

	painter.setRenderHint(QPainter::Antialiasing);
	const auto lineWidth = scale * std::clamp(radius/7., 1., 2.5);

	for(float lineScale : {2.f, 1.f})
	{
		painter.setPen(QPen(lineScale==1 ? Qt::black : Qt::white, lineWidth*lineScale));
		painter.drawEllipse(center, radius, radius);

		const float x = center.x(), y = center.y();
		const QPointF endpoints[] = {{x-radius+lineWidth/2, y},
		                             {x-radius*0.7f, y},
		                             {x+radius-lineWidth/2, y},
		                             {x+radius*0.7f, y},
		                             {x, y-radius+lineWidth/2},
		                             {x, y-radius*0.7f},
		                             {x, y+radius-lineWidth/2},
		                             {x, y+radius*0.7f}};
		painter.drawLines(endpoints, std::size(endpoints)/2);
	}

	return QCursor(pixmap, center.x(), center.y());
}

void LensDistortionEstimator::renderPointMarker(StelPainter& sPainter, const float x, const float y,
                                                float radius, const Vec3f& color) const
{
	const auto scale = sPainter.getProjector()->getDevicePixelsPerPixel();
	radius *= scale;

	sPainter.setBlending(true);
	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(radius/7, 1.f, 2.5f));
	sPainter.setColor(color);

	sPainter.drawCircle(x, y, radius);

	sPainter.enableClientStates(true);
	const float vertexData[] = {x-radius     , y,
	                            x-radius*0.5f, y,
	                            x+radius     , y,
	                            x+radius*0.5f, y,
	                            x, y-radius     ,
	                            x, y-radius*0.5f,
	                            x, y+radius     ,
	                            x, y+radius*0.5f};
	const auto vertCount = std::size(vertexData) / 2;
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData);
	sPainter.drawFromArray(StelPainter::Lines, vertCount, 0, false);
	sPainter.enableClientStates(false);
}

void LensDistortionEstimator::renderProjectionCenterMarker(StelPainter& sPainter, const float x, const float y,
                                                             float radius, const Vec3f& color) const
{
	const auto scale = sPainter.getProjector()->getDevicePixelsPerPixel();
	radius *= scale;

	sPainter.setBlending(true);
	sPainter.setLineSmooth(true);
	sPainter.setLineWidth(scale * std::clamp(radius/7, 1.f, 2.5f));
	sPainter.setColor(color);

	sPainter.drawCircle(x, y, radius);

	sPainter.enableClientStates(true);
	const auto size = radius / std::sqrt(2.f);
	const float vertexData[] = {x-size     , y-size     ,
	                            x-size*0.1f, y-size*0.1f,
	                            x+size     , y+size     ,
	                            x+size*0.1f, y+size*0.1f,
	                            x+size     , y-size     ,
	                            x+size*0.1f, y-size*0.1f,
	                            x-size     , y+size     ,
	                            x-size*0.1f, y+size*0.1f};
	const auto vertCount = std::size(vertexData) / 2;
	sPainter.setVertexPointer(2, GL_FLOAT, vertexData);
	sPainter.drawFromArray(StelPainter::Lines, vertCount, 0, false);
	sPainter.enableClientStates(false);
}

void LensDistortionEstimator::draw(StelCore* core)
{
	if(!imageEnabled) return;

	if(dialog_->imageChanged())
	{
		const auto image = dialog_->image();
		if(image.isNull()) return;

		imageTex_.reset(new QOpenGLTexture(image));
		imageTex_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
		imageTex_->setWrapMode(QOpenGLTexture::ClampToBorder);
		imgPointToRotateAbout_ = Vec2d(imageTex_->width(),imageTex_->height()) / 2.;
		dialog_->resetImageChangeFlag();
	}
	if(!imageTex_) return;

	imageTex_->bind(0);

	const auto projector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

	if(!renderProgram_ || !prevProjector_ || !projector->isSameProjection(*prevProjector_))
	{
		renderProgram_.reset(new QOpenGLShaderProgram);
		prevProjector_ = projector;

		const auto vert =
			StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
			R"(
ATTRIBUTE highp vec3 vertex;
VARYING highp vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
		bool ok = renderProgram_->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
		if(!renderProgram_->log().isEmpty())
			qWarning().noquote() << "LensDistortionEstimator: warnings while compiling vertex shader:\n" << renderProgram_->log();
		if(!ok) return;

		const auto frag =
			StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
			projector->getUnProjectShader() +
			makeSRGBUtilsShader() +
			R"(
VARYING highp vec3 ndcPos;
uniform sampler2D imageTex;
uniform mat4 projectionMatrixInverse;
uniform vec2 normCoordToTexCoordCoefs;
uniform float distNormalizationCoef;
uniform vec3 projectionCenterDir;
uniform vec3 imageUpDir;
uniform bool imageAxesEnabled;
uniform vec3 imageAxesColor;
uniform vec2 imageCenterShift;
uniform vec4 bgToSubtract;
uniform float brightness;
#define MODEL_POLY3 0
#define MODEL_POLY5 1
#define MODEL_PTLENS 2
uniform int distortionModel;
uniform float distortionTerm1;
uniform float distortionTerm2;
uniform float distortionTerm3;
uniform float maxUndistortedR;
void main()
{
	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool unprojOK = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, unprojOK).xyz;
	vec3 viewDir = normalize(modelPos);

	vec3 imageRightDir = normalize(cross(projectionCenterDir, imageUpDir));
	float viewProjCenterDot = dot(viewDir, projectionCenterDir);
	float viewProjCenterCross = length(cross(viewDir, projectionCenterDir));
	float angle = atan(dot(viewDir, imageUpDir), dot(viewDir, imageRightDir));
	// Undistorted distance
	float ru = distNormalizationCoef * tan(atan(viewProjCenterCross,viewProjCenterDot));

	float ru2 = ru*ru;
	float ru3 = ru2*ru;
	float ru4 = ru2*ru2;
	float k1 = distortionTerm1, k2 = distortionTerm2;
	float a = distortionTerm1, b = distortionTerm2, c = distortionTerm3;
	float distortedR = 0.;
	if(distortionModel == MODEL_POLY3)
		distortedR = ru*(1.-k1+k1*ru*ru);
	else if(distortionModel == MODEL_POLY5)
		distortedR = ru*(1+k1*ru2+k2*ru4);
	else if(distortionModel == MODEL_PTLENS)
		distortedR = ru*(a*ru3+b*ru2+c*ru+1-a-b-c);

	vec2 centeredTexCoord = normCoordToTexCoordCoefs * (distortedR * vec2(cos(angle), sin(angle)) + imageCenterShift);

	vec3 texColor = texture2D(imageTex, 0.5 + 0.5 * centeredTexCoord).rgb;
	texColor = linearToSRGB(brightness * (srgbToLinear(texColor) - srgbToLinear(bgToSubtract.rgb))
                                                                       /
                                              max(1. - srgbToLinear(bgToSubtract.rgb), vec3(1./255.)));
	bool oppositeToImgDir = viewProjCenterDot < 0.;
	FRAG_COLOR = oppositeToImgDir ? vec4(0) : vec4(texColor, 1);

	if(imageAxesEnabled && !oppositeToImgDir)
	{
		vec2 sGrad = vec2(dFdx(centeredTexCoord.s), dFdy(centeredTexCoord.s));
		vec2 tGrad = vec2(dFdx(centeredTexCoord.t), dFdy(centeredTexCoord.t));
		if(abs(centeredTexCoord.s) <= 1. && abs(centeredTexCoord.t) <= 1. &&
		    (centeredTexCoord.t*centeredTexCoord.t < dot(tGrad,tGrad) ||
		     centeredTexCoord.s*centeredTexCoord.s < dot(sGrad,sGrad)))
		{
			FRAG_COLOR = vec4(imageAxesColor,1);
		}
	}

	if(!unprojOK || ru > maxUndistortedR)
	{
		FRAG_COLOR = vec4(0);
	}
}
)";
		ok = renderProgram_->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram_->log().isEmpty())
			qWarning().noquote() << "LensDistortionEstimator: warnings while compiling fragment shader:\n" << renderProgram_->log();

		if(!ok) return;

		renderProgram_->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram_.get(), "LensDistortionEstimator image render program"))
			return;

		renderProgram_->bind();
		shaderVars.imageTex = renderProgram_->uniformLocation("imageTex");
		shaderVars.imageUpDir = renderProgram_->uniformLocation("imageUpDir");
		shaderVars.projectionCenterDir = renderProgram_->uniformLocation("projectionCenterDir");
		shaderVars.distNormalizationCoef = renderProgram_->uniformLocation("distNormalizationCoef");
		shaderVars.normCoordToTexCoordCoefs = renderProgram_->uniformLocation("normCoordToTexCoordCoefs");
		shaderVars.imageAxesEnabled = renderProgram_->uniformLocation("imageAxesEnabled");
		shaderVars.imageAxesColor = renderProgram_->uniformLocation("imageAxesColor");
		shaderVars.imageCenterShift = renderProgram_->uniformLocation("imageCenterShift");
		shaderVars.bgToSubtract = renderProgram_->uniformLocation("bgToSubtract");
		shaderVars.imageBrightness = renderProgram_->uniformLocation("brightness");
		shaderVars.distortionModel = renderProgram_->uniformLocation("distortionModel");
		shaderVars.distortionTerm1 = renderProgram_->uniformLocation("distortionTerm1");
		shaderVars.distortionTerm2 = renderProgram_->uniformLocation("distortionTerm2");
		shaderVars.distortionTerm3 = renderProgram_->uniformLocation("distortionTerm3");
		shaderVars.maxUndistortedR = renderProgram_->uniformLocation("maxUndistortedR");
		shaderVars.projectionMatrixInverse = renderProgram_->uniformLocation("projectionMatrixInverse");
		renderProgram_->release();
	}
	if(!renderProgram_ || !renderProgram_->isLinked())
		return;

	renderProgram_->bind();

	const int imageTexSampler = 0;
	imageTex_->bind(imageTexSampler);
	renderProgram_->setUniformValue(shaderVars.imageTex, imageTexSampler);
	renderProgram_->setUniformValue(shaderVars.imageUpDir, dialog_->imageUpDir().toQVector());
	renderProgram_->setUniformValue(shaderVars.projectionCenterDir, dialog_->projectionCenterDir().toQVector());
	const float W = imageTex_->width();
	const float H = imageTex_->height();
	const float minSize = std::min(W,H);
	const double imageSmallerSideFoV = M_PI/180 * dialog_->imageSmallerSideFoV();
	renderProgram_->setUniformValue(shaderVars.distNormalizationCoef,
	                                float(1. / (std::tan(imageSmallerSideFoV / 2.))));
	renderProgram_->setUniformValue(shaderVars.imageAxesEnabled, imageAxesEnabled);
	renderProgram_->setUniformValue(shaderVars.imageAxesColor, imageAxesColor.toQVector());
	renderProgram_->setUniformValue(shaderVars.imageCenterShift, dialog_->imageCenterShift().toQVector());
	renderProgram_->setUniformValue(shaderVars.bgToSubtract, dialog_->bgToSubtract());
	renderProgram_->setUniformValue(shaderVars.imageBrightness, float(dialog_->imageBrightness()));
	renderProgram_->setUniformValue(shaderVars.distortionModel, int(dialog_->distortionModel()));
	renderProgram_->setUniformValue(shaderVars.normCoordToTexCoordCoefs, (minSize-1)*QVector2D(1./W,1./H));
	renderProgram_->setUniformValue(shaderVars.distortionTerm1, float(dialog_->distortionTerm1()));
	renderProgram_->setUniformValue(shaderVars.distortionTerm2, float(dialog_->distortionTerm2()));
	renderProgram_->setUniformValue(shaderVars.distortionTerm3, float(dialog_->distortionTerm3()));
	renderProgram_->setUniformValue(shaderVars.maxUndistortedR, float(dialog_->maxUndistortedR()));
	renderProgram_->setUniformValue(shaderVars.projectionMatrixInverse, projector->getProjectionMatrix().toQMatrix().inverted());
	projector->setUnProjectUniforms(*renderProgram_);

	auto& gl = *QOpenGLContext::currentContext()->functions();

	gl.glEnable(GL_BLEND);
	gl.glBlendFunc(GL_ONE,GL_ONE); // allow colored sky background
	bindVAO();
	gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	releaseVAO();
	gl.glDisable(GL_BLEND);

	if(!pointMarkersEnabled && !projectionCenterMarkerEnabled)
		return;

	StelPainter sPainter(core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff));
	if(pointMarkersEnabled)
	{
		for(const auto& status : dialog_->imagePointDirections())
		{
			const auto color = status.selected ? selectedPointMarkerColor : pointMarkerColor;
			Vec3d win;
			if(sPainter.getProjector()->project(status.direction, win))
				renderPointMarker(sPainter, win[0], win[1], POINT_MARKER_RADIUS, color);
		}
	}

	if(projectionCenterMarkerEnabled)
	{
		const auto dir = dialog_->projectionCenterDir();
		Vec3d win;
		if(sPainter.getProjector()->project(dir, win))
			renderProjectionCenterMarker(sPainter, win[0], win[1], 12, projectionCenterMarkerColor);
	}
}

void LensDistortionEstimator::setImageAxesColor(const Vec3f& color)
{
	if(imageAxesColor == color) return;

	imageAxesColor = color;
	conf_->setValue("LensDistortionEstimator/image_axes_color", color.toStr());
	emit imageAxesColorChanged(color);
}

void LensDistortionEstimator::setPointMarkerColor(const Vec3f& color)
{
	if(pointMarkerColor == color) return;

	pointMarkerColor = color;
	conf_->setValue("LensDistortionEstimator/point_marker_color", color.toStr());
	emit pointMarkerColorChanged(color);
}

void LensDistortionEstimator::setSelectedPointMarkerColor(const Vec3f& color)
{
	if(selectedPointMarkerColor == color) return;

	selectedPointMarkerColor = color;
	conf_->setValue("LensDistortionEstimator/selected_point_marker_color", color.toStr());
	emit selectedPointMarkerColorChanged(color);
}

void LensDistortionEstimator::setProjectionCenterMarkerColor(const Vec3f& color)
{
	if(projectionCenterMarkerColor == color) return;

	projectionCenterMarkerColor = color;
	conf_->setValue("LensDistortionEstimator/center_of_projection_marker_color", color.toStr());
	emit projectionCenterMarkerColorChanged(color);
}

void LensDistortionEstimator::setPointMarkersEnabled(const bool enable)
{
	if(pointMarkersEnabled == enable) return;

	pointMarkersEnabled = enable;
	conf_->setValue("LensDistortionEstimator/mark_picked_points", enable);
	emit pointMarkersToggled(enable);
}

void LensDistortionEstimator::setProjectionCenterMarkerEnabled(const bool enable)
{
	if(projectionCenterMarkerEnabled == enable) return;

	projectionCenterMarkerEnabled = enable;
	conf_->setValue("LensDistortionEstimator/mark_center_of_projection", enable);
	emit pointMarkersToggled(enable);
}

void LensDistortionEstimator::setImageAxesEnabled(const bool enable)
{
	if(imageAxesEnabled == enable) return;

	imageAxesEnabled = enable;
	conf_->setValue("LensDistortionEstimator/show_image_axes", enable);
	emit imageAxesToggled(enable);
}

void LensDistortionEstimator::setImageEnabled(const bool enable)
{
	if(imageEnabled == enable) return;

	imageEnabled = enable;
	conf_->setValue("LensDistortionEstimator/show_image", enable);
	emit imageToggled(enable);
}

void LensDistortionEstimator::setDialogVisible(const bool enable)
{
	if(dialogVisible == enable) return;

	dialogVisible = enable;
	dialog_->setVisible(enable);
	if(enable)
		dialog_->showComputationPage();
}

void LensDistortionEstimator::loadSettings()
{
	setImageEnabled(conf_->value("LensDistortionEstimator/show_image", true).toBool());
	setImageAxesEnabled(conf_->value("LensDistortionEstimator/show_image_axes", false).toBool());
	setPointMarkersEnabled(conf_->value("LensDistortionEstimator/mark_picked_points", true).toBool());
	setProjectionCenterMarkerEnabled(conf_->value("LensDistortionEstimator/mark_center_of_projection", false).toBool());
	imageAxesColor = Vec3f(conf_->value("LensDistortionEstimator/image_axes_color", defaultImageAxesColor).toString());
	pointMarkerColor = Vec3f(conf_->value("LensDistortionEstimator/point_marker_color", defaultPointMarkerColor).toString());
	selectedPointMarkerColor = Vec3f(conf_->value("LensDistortionEstimator/selected_point_marker_color", defaultSelectedPointMarkerColor).toString());
	projectionCenterMarkerColor = Vec3f(conf_->value("LensDistortionEstimator/center_of_projection_marker_color", defaultProjectionCenterMarkerColor).toString());
}

void LensDistortionEstimator::restoreDefaultSettings()
{
	// Remove the old values...
	conf_->remove("LensDistortionEstimator");
	// ...load the default values...
	loadSettings();
	// But this doesn't save the colors, so:
	conf_->beginGroup("LensDistortionEstimator");
	conf_->setValue("image_axes_color", defaultImageAxesColor);
	conf_->setValue("point_marker_color", defaultPointMarkerColor);
	conf_->setValue("selected_point_marker_color", defaultSelectedPointMarkerColor);
	conf_->setValue("center_of_projection_marker_color", defaultProjectionCenterMarkerColor);
	conf_->endGroup();
}
