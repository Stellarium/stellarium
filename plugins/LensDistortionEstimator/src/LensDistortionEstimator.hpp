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

#ifndef LENSDISTORTIONESTIMATOR_HPP
#define LENSDISTORTIONESTIMATOR_HPP

#include <memory>
#include "StelCore.hpp"
#include "StelModule.hpp"
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

class QSettings;
class StelButton;
class QMouseEvent;
class LensDistortionEstimatorDialog;
class LensDistortionEstimator : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool dialogVisible READ isDialogVisible WRITE setDialogVisible NOTIFY dialogToggled)
	Q_PROPERTY(bool imageEnabled READ isImageEnabled WRITE setImageEnabled NOTIFY imageToggled)
	Q_PROPERTY(bool imageAxesEnabled READ areImageAxesEnabled WRITE setImageAxesEnabled NOTIFY imageAxesToggled)
	Q_PROPERTY(bool pointMarkersEnabled READ arePointMarkersEnabled WRITE setPointMarkersEnabled NOTIFY pointMarkersToggled)
	Q_PROPERTY(bool projectionCenterMarkerEnabled READ isProjectionCenterMarkerEnabled WRITE setProjectionCenterMarkerEnabled NOTIFY projectionCenterMarkerToggled)
	Q_PROPERTY(Vec3f selectedPointMarkerColor READ getSelectedPointMarkerColor WRITE setSelectedPointMarkerColor NOTIFY selectedPointMarkerColorChanged )
	Q_PROPERTY(Vec3f pointMarkerColor READ getPointMarkerColor WRITE setPointMarkerColor NOTIFY pointMarkerColorChanged )
	Q_PROPERTY(Vec3f projectionCenterMarkerColor READ getProjectionCenterMarkerColor WRITE setProjectionCenterMarkerColor NOTIFY projectionCenterMarkerColorChanged )
	Q_PROPERTY(Vec3f imageAxesColor READ getImageAxesColor WRITE setImageAxesColor NOTIFY imageAxesColorChanged )
public:
	enum class DistortionModel
	{
		Poly3,
		Poly5,
		PTLens,
	};

	LensDistortionEstimator();
	~LensDistortionEstimator() override;

	void init() override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	void handleMouseClicks(QMouseEvent* event) override;
	bool handleMouseMoves(int x, int y, Qt::MouseButtons) override;
	bool configureGui(bool show = true) override;
	bool arePointMarkersEnabled() const { return pointMarkersEnabled; }
	bool isProjectionCenterMarkerEnabled() const { return projectionCenterMarkerEnabled; }
	bool areImageAxesEnabled() const { return imageAxesEnabled; }
	bool isImageEnabled() const { return imageEnabled; }
	bool isDialogVisible() const { return dialogVisible; }
	QCursor getPointPickCursor() const;
	Vec3f getProjectionCenterMarkerColor() const { return projectionCenterMarkerColor; }
	Vec3f getSelectedPointMarkerColor() const { return selectedPointMarkerColor; }
	Vec3f getPointMarkerColor() const { return pointMarkerColor; }
	Vec3f getImageAxesColor() const { return imageAxesColor; }
	void restoreDefaultSettings();

public slots:
	void setImageEnabled(bool enable);
	void setDialogVisible(bool enable);
	void setImageAxesEnabled(bool enabled);
	void setPointMarkersEnabled(bool enabled);
	void setProjectionCenterMarkerEnabled(bool enabled);
	void setProjectionCenterMarkerColor(const Vec3f& color);
	void setSelectedPointMarkerColor(const Vec3f& color);
	void setPointMarkerColor(const Vec3f& color);
	void setImageAxesColor(const Vec3f& color);
signals:
	void imageToggled(bool enabled);
	void dialogToggled(bool enabled);
	void imageAxesToggled(bool enabled);
	void pointMarkersToggled(bool enabled);
	void projectionCenterMarkerToggled(bool enabled);
	void projectionCenterMarkerColorChanged(const Vec3f& color);
	void selectedPointMarkerColorChanged(const Vec3f& color);
	void pointMarkerColorChanged(const Vec3f& color);
	void imageAxesColorChanged(const Vec3f& color);
private:
	void loadSettings();
	void setupCurrentVAO();
	void bindVAO();
	void releaseVAO();
	void renderPointMarker(StelPainter& sPainter, float x, float y, float radius, const Vec3f& color) const;
	void renderProjectionCenterMarker(StelPainter& sPainter, float x, float y, float radius, const Vec3f& color) const;
	Vec2d screenPointToImagePoint(int x, int y) const;
	void dragImage(const Vec2d& src, const Vec2d& dst);
	void rotateImage(const Vec2d& dragSrcPoint, const Vec2d& dragDstPoint);
private:
	struct
	{
		int imageTex;
		int imageUpDir;
		int projectionCenterDir;
		int distNormalizationCoef;
		int normCoordToTexCoordCoefs;
		int imageAxesEnabled;
		int imageAxesColor;
		int imageCenterShift;
		int bgToSubtract;
		int imageBrightness;
		int distortionModel;
		int distortionTerm1;
		int distortionTerm2;
		int distortionTerm3;
		int maxUndistortedR;
		int projectionMatrixInverse;
	} shaderVars;

	QSettings* conf_;
	StelButton* toolbarButton_ = nullptr;
	StelProjectorP prevProjector_;
	std::unique_ptr<QOpenGLBuffer> vbo_;
	std::unique_ptr<QOpenGLVertexArrayObject> vao_;
	std::unique_ptr<QOpenGLShaderProgram> renderProgram_;
	std::unique_ptr<LensDistortionEstimatorDialog> dialog_;
	std::unique_ptr<QOpenGLTexture> imageTex_;
	QPointF dragStartPoint_{0,0};
	Vec2d imgPointToRotateAbout_{0,0};
	double dragStartProjCenterElevation_ = 0;
	double dragStartProjCenterAzimuth_ = 0;
	double dragStartImgFieldRotation_ = 0;
	double dragStartImgSmallerSideFoV_ = 0;
	bool draggingImage_ = false;
	bool rotatingImage_ = false;
	bool pointMarkersEnabled = true;
	bool projectionCenterMarkerEnabled = false;
	bool imageAxesEnabled = false;
	bool imageEnabled = true;
	bool dialogVisible = false;
	Vec3f imageAxesColor = Vec3f(0,0,0);
	Vec3f pointMarkerColor = Vec3f(0,0,0);
	Vec3f selectedPointMarkerColor = Vec3f(0,0,0);
	Vec3f projectionCenterMarkerColor = Vec3f(0,0,0);
};


#include <QObject>
#include "StelPluginInterface.hpp"

class LensDistortionEstimatorStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif
