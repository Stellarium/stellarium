/*
 * Stellarium: Lens distortion estimator plugin
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
 
#ifndef LDEDIALOG_HPP
#define LDEDIALOG_HPP

#include <memory>
#include "VecMath.hpp"
#include "StelDialog.hpp"
#include "StelObjectType.hpp"
#include <QImage>
#include <QTimer>
#include <QColor>
#include <QtCharts>
#include <QDateTime>
#include "LensDistortionEstimator.hpp"

class StarMgr;
class Ui_LDEDialog;

class LensDistortionEstimatorDialog : public StelDialog
{
	Q_OBJECT

public:
	LensDistortionEstimatorDialog(LensDistortionEstimator* lde);
	~LensDistortionEstimatorDialog() override;
	void init();
	double distortionTerm1() const;
	double distortionTerm2() const;
	double distortionTerm3() const;
	Vec3d imageUpDir() const;
	Vec3d projectionCenterDir() const;
	Vec2d imageCenterShift() const;
	QColor bgToSubtract() const;
	double imageBrightness() const;
	double imageSmallerSideFoV() const;
	LensDistortionEstimator::DistortionModel distortionModel() const;
	Vec3d computeImagePointDir(const Vec2d& imagePoint) const;
	double applyDistortion(const double ru) const;
	double maxUndistortedR() const;
	QImage image() const { return image_; }
	bool imageChanged() const { return imageChanged_; }
	void resetImageChangeFlag() { imageChanged_ = false; }
	bool isPickingAPoint() const { return isPickingAPoint_; }
	void resetImagePointPicking();
	void registerImagePoint(const StelObject& object, const Vec2d& posInImage);
	void repositionImageByPoints();
	void setupGenerateLensfunModelButton();
	double imageFieldRotation() const;
	double projectionCenterAzimuth() const;
	double projectionCenterElevation() const;
	void setImageFieldRotation(double imageFieldRot);
	void setProjectionCenterAzimuth(double centerAzim);
	void setProjectionCenterElevation(double centerElev);
	void setImageSmallerSideFoV(double fov);
	double computeAngleBetweenImagePoints(const Vec2d& pointA, const Vec2d& pointB, double smallerSideFoV) const;
	void togglePointPickingMode();
	void generateLensfunModel();
	void showSettingsPage();
	void showComputationPage();
	bool initialized() const { return initialized_; }

	struct ImagePointStatus
	{
		Vec3d direction;
		bool selected;
	};
	std::vector<ImagePointStatus> imagePointDirections() const;

protected:
	void createDialogContent() override;
	void emitWarning(const QString& text, bool autoFixable = false);
	void clearWarnings();

public slots:
	void retranslate() override;

private:
	double imageCenterShiftX() const;
	double imageCenterShiftY() const;

	void setDistortionTerm1(double a);
	void setDistortionTerm2(double b);
	void setDistortionTerm3(double c);
	void setImageCenterShiftX(double xShift);
	void setImageCenterShiftY(double yShift);

	void fixWarning();
	void updateImage();
	void exportPoints() const;
	void importPoints();
	void setAboutText();
	void browseForImage();
	void restoreDefaults();
	void resetDistortion();
	void removeImagePoint();
	void updateErrorsChart();
	void optimizeParameters();
	void resetImagePlacement();
	void periodicWarningsCheck();
	void updateImagePathStatus();
	void computeColorToSubtract();
	void updateRepositionButtons();
	void handlePointSelectionChange();
	void placeImageInCenterOfScreen();
	void updateDistortionCoefControls();
	double computeLensCropFactor() const;
	double computeExifTimeDifference() const;
	void startImagePointPicking(bool buttonChecked);
	Vec3d computeObjectDir(const Vec2d& imagePoint) const;
	QString getObjectName(const StelObject& object) const;
	StelObjectP findObjectByName(const QString& name) const;
	void handlePointDoubleClick(QTreeWidgetItem* item, int column);
	Vec2d imagePointToNormalizedCoords(const Vec2d& imagePoint) const;
	void handleSelectedObjectChange(StelModule::StelModuleSelectAction);
	static double roundToNiceAxisValue(double x);
	static void setupChartAxisStyle(QValueAxis& axis);

private:
	LensDistortionEstimator* lde_ = nullptr;
	StelCore* core_ = nullptr;
	StarMgr* starMgr_ = nullptr;
	StelObjectMgr* objectMgr_ = nullptr;
	std::unique_ptr<Ui_LDEDialog> ui_;
	QImage image_;
	bool initialized_ = false;
	bool imageChanged_ = false;
	bool isPickingAPoint_ = false;
	QDateTime exifDateTime_;
	QTimer warningCheckTimer_;
	QChart* errorsChart_ = nullptr;

	// These variables hold the state used during optimization
	struct ObjectPos
	{
		Vec2d imgPos;
		Vec3d skyDir;
	};
	std::vector<ObjectPos> objectPositions_;
	double distortionTerm1_ = 0, distortionTerm1min_ = 0, distortionTerm1max_ = 0;
	double distortionTerm2_ = 0, distortionTerm2min_ = 0, distortionTerm2max_ = 0;
	double distortionTerm3_ = 0, distortionTerm3min_ = 0, distortionTerm3max_ = 0;
	double imageCenterShiftX_ = 0, imageCenterShiftXmin_ = 0, imageCenterShiftXmax_ = 0;
	double imageCenterShiftY_ = 0, imageCenterShiftYmin_ = 0, imageCenterShiftYmax_ = 0;
	double imageFieldRotation_ = 0, imageFieldRotationMin_ = 0, imageFieldRotationMax_ = 0;
	double projectionCenterAzimuth_ = 0, projectionCenterAzimuthMin_ = 0, projectionCenterAzimuthMax_ = 0;
	double projectionCenterElevation_ = 0, projectionCenterElevationMin_ = 0, projectionCenterElevationMax_ = 0;
	LensDistortionEstimator::DistortionModel distortionModel_ = LensDistortionEstimator::DistortionModel::PTLens;
	double imageSmallerSideFoV_ = 0;
	bool optimizing_ = false;
	bool distortionTerm1inUse_ = true;
	bool distortionTerm2inUse_ = false;
	bool distortionTerm3inUse_ = false;
};

#endif // LDEDIALOG_HPP
