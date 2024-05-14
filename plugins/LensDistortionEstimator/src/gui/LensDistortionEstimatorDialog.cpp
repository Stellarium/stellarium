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

#include "LensDistortionEstimatorDialog.hpp"
#include "LensDistortionEstimator.hpp"
#include "StarMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include <cmath>
#include <array>
#include <limits>
#include <cassert>
#include <complex>
#include <QTimeZone>
#include <QFileInfo>
#include <QValueAxis>
#include <QFileDialog>
#include <QScatterSeries>
#ifdef HAVE_EXIV2
# include <exiv2/exiv2.hpp>
#endif
#ifdef HAVE_NLOPT
# include <nlopt.hpp>
#endif
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
using namespace QtCharts;
#endif
#include "ui_lensDistortionEstimatorDialog.h"

namespace
{
struct Column
{
enum
{
	ObjectName,
	x,
	y,
};
};
struct Role
{
enum
{
	ObjectEnglishName = Qt::UserRole,
};
};
struct Page
{
enum
{
	ImageLoading,
	Fitting,
	Settings,
	About,
};
};
template<typename T> auto sqr(T x) { return x*x; }
template<typename T> auto cube(T x) { return x*x*x; }
using DistortionModel = LensDistortionEstimator::DistortionModel;

//! Finds real roots of the polynomial ax^2+bx+c, sorted in ascending order
std::vector<double> solveQuadratic(const double a, const double b, const double c)
{
	if(a==0 && b==0) return {};
	const auto D = b*b - 4*a*c;
	if(D < 0) return {};
	const auto sqrtD = std::sqrt(D);
	const auto signB = b < 0 ? -1. : 1.;
	const auto u = -b - signB * sqrtD;
	const auto x1 = u / (2*a);
	const auto x2 = 2*c / u;
	if(!std::isfinite(x1)) return {x2}; // a~0, the equation is linear
	return x1 < x2 ? std::vector<double>{x1, x2} : std::vector<double>{x2, x1};
}

// Locates the root t in [tMin, tMax] of the equation t^3+Bt^2+Ct-1=0. The root must be unique.
double locateRootInCubicPoly(const double B, const double C, double tMin, double tMax)
{
	for(int n = 0; n < 100 && tMin != tMax; ++n)
	{
		const auto t = (tMin + tMax) / 2;
		const auto poly = -1+t*(C+t*(B+t));
		if(poly > 0)
			tMax = t;
		else
			tMin = t;
	}
	return (tMin + tMax) / 2;
}

//! Finds real roots of the polynomial Ax^3+ax^2+bx+c, sorted in ascending order
std::vector<double> solveCubic(double A, double a, double b, double c)
{
	// The algorithm here is intended to handle the hard cases like
	// A~1e-17, a~1, b~1, c~1 well, without introducing arbitrary epsilons.

	using namespace std;

	if(abs(A*A*A) < std::numeric_limits<double>::min())
		return solveQuadratic(a,b,c);

	if(c == 0)
	{
		// Easy special case: x*(Ax^2+ax+b)=0. It must be handled to
		// avoid division by zero in the subsequent general solution.
		auto roots = solveQuadratic(A,a,b);
		roots.push_back(0);
		sort(roots.begin(), roots.end());
		return roots;
	}

	// Bring the equation to the form x^3+ax^2+bx+c=0
	a /= A;
	b /= A;
	c /= A;
	A = 1;

	// Let's bracket the first root following https://www.johndcook.com/blog/2022/04/05/cubic/
	const auto t2x = -cbrt(c); // Changing variable x = t2x*t
	const auto B = -a/cbrt(c);
	const auto C = b/sqr(cbrt(c));
	// New equation: f(t)=0, i.e. t^3+Bt^2+Ct-1=0.
	// f(0) = -1
	const auto f1 = B+C; // f(1)
	double t;
	if(f1==0)
	{
		t = 1;
	}
	else if(f1 > 0)
	{
		// We have a root t in (0,1)
		t = locateRootInCubicPoly(B,C, 0,1);
	}
	else
	{
		// We have a root t in (1,inf). Change variable t=1/p:
		// p^3-Cp^2-Bp-1=0.
		const auto p = locateRootInCubicPoly(-C,-B, 0,1);
		t = 1/p;
	}
	const auto x1 = t2x*t;

	// Long polynomial division of the LHS of the equation by (x-x1) allows us to reach this decomposition:
	// (x - x1) (A x^2 + (A x1 + a) x + (A x1^2 + a x1 + b)) + (A x1^3 + a x1^2 + b x1 + c) = A x^3 + a x^2 + b x + c
	// Now, assuming that x1 is a root, we can drop the remainder (A x1^3 + a x1^2 + b x1 + c),
	// since it must be zero, so we get a quadratic equation for the remaining two roots:
	// A x^2 + (A x1 + a) x + (A x1^2 + a x1 + b) = 0
	auto roots = solveQuadratic(A, A*x1 + a, A*x1*x1 + a*x1 + b);
	roots.push_back(x1);
	sort(roots.begin(), roots.end());
	return roots;
}

}

LensDistortionEstimatorDialog::LensDistortionEstimatorDialog(LensDistortionEstimator* lde)
	: StelDialog("LensDistortionEstimator")
	, lde_(lde)
	, ui_(new Ui_LDEDialog{})
	, errorsChart_(new QChart)
{
	warningCheckTimer_.setInterval(1000);
	connect(&warningCheckTimer_, &QTimer::timeout, this, &LensDistortionEstimatorDialog::periodicWarningsCheck);

	// Same as in AstroCalcChart
	errorsChart_->setBackgroundBrush(QBrush(QColor(86, 87, 90)));
	errorsChart_->setTitleBrush(QBrush(Qt::white));
	errorsChart_->setMargins(QMargins(2, 1, 2, 1)); // set to 0/0/0/0 for max space usage. This is between the title/axis labels and the enclosing QChartView.
	errorsChart_->layout()->setContentsMargins(0, 0, 0, 0);
	errorsChart_->setBackgroundRoundness(0); // remove rounded corners
}

LensDistortionEstimatorDialog::~LensDistortionEstimatorDialog()
{
}

void LensDistortionEstimatorDialog::retranslate()
{
	if(dialog)
	{
		ui_->retranslateUi(dialog);
		setAboutText();
	}
}

void LensDistortionEstimatorDialog::createDialogContent()
{
	ui_->setupUi(dialog);
	ui_->tabs->setCurrentIndex(0);

	// Kinetic scrolling
	kineticScrollingList << ui_->about;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, &StelGui::flagUseKineticScrollingChanged, this, &LensDistortionEstimatorDialog::enableKineticScrolling);
	}

	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &LensDistortionEstimatorDialog::retranslate);
	connect(ui_->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui_->titleBar, &TitleBar::movedTo, this, &LensDistortionEstimatorDialog::handleMovedTo);

	core_ = StelApp::getInstance().getCore();
	starMgr_ = GETSTELMODULE(StarMgr);
	objectMgr_ = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objectMgr_);
	connect(objectMgr_, &StelObjectMgr::selectedObjectChanged, this, &LensDistortionEstimatorDialog::handleSelectedObjectChange);
	handleSelectedObjectChange(StelModule::ReplaceSelection);

	// Main tabs
	clearWarnings();
	connect(ui_->imageFilePath, &QLineEdit::editingFinished, this, &LensDistortionEstimatorDialog::updateImage);
	connect(ui_->imageFilePath, &QLineEdit::textChanged, this, &LensDistortionEstimatorDialog::updateImagePathStatus);
	connect(ui_->imageBrowseBtn, &QAbstractButton::clicked, this, &LensDistortionEstimatorDialog::browseForImage);
	connect(ui_->pickImagePointBtn, &QAbstractButton::clicked, this, &LensDistortionEstimatorDialog::startImagePointPicking);
	connect(ui_->removePointBtn, &QAbstractButton::clicked, this, &LensDistortionEstimatorDialog::removeImagePoint);
	connect(ui_->imagePointsPicked, &QTreeWidget::itemSelectionChanged, this, &LensDistortionEstimatorDialog::handlePointSelectionChange);
	connect(ui_->imagePointsPicked, &QTreeWidget::itemDoubleClicked, this, &LensDistortionEstimatorDialog::handlePointDoubleClick);
	connect(ui_->repositionBtn, &QAbstractButton::clicked, this, &LensDistortionEstimatorDialog::repositionImageByPoints);
	connect(ui_->fixWarningBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::fixWarning);
	connect(ui_->exportPointsBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::exportPoints);
	connect(ui_->importPointsBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::importPoints);
	connect(ui_->distortionModelCB, qOverload<int>(&QComboBox::currentIndexChanged), this,
	        &LensDistortionEstimatorDialog::updateDistortionCoefControls);
	connect(ui_->resetPlacementBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::resetImagePlacement);
	connect(ui_->resetDistortionBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::resetDistortion);
#ifdef HAVE_NLOPT
	connect(ui_->optimizeBtn, &QAbstractButton::clicked, this, &LensDistortionEstimatorDialog::optimizeParameters);
#else
	ui_->optimizeBtn->hide();
#endif
	connect(ui_->generateLensfunModelBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::generateLensfunModel);
	connect(ui_->lensMake, &QLineEdit::textChanged, this, &LensDistortionEstimatorDialog::setupGenerateLensfunModelButton);
	connect(ui_->lensModel, &QLineEdit::textChanged, this, &LensDistortionEstimatorDialog::setupGenerateLensfunModelButton);
	connect(ui_->lensMount, &QLineEdit::textChanged, this, &LensDistortionEstimatorDialog::setupGenerateLensfunModelButton);
	connect(ui_->imageFilePath, &QLineEdit::textChanged, this, &LensDistortionEstimatorDialog::setupGenerateLensfunModelButton);
	setupGenerateLensfunModelButton();

	ui_->errorsChartView->setChart(errorsChart_);
	ui_->errorsChartView->setRenderHint(QPainter::Antialiasing);
	ui_->imagePointsPicked->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui_->listChartSplitter->setStretchFactor(0, 1);
	ui_->listChartSplitter->setStretchFactor(1, 4);
	updateDistortionCoefControls();

	// Settings tab
	connectBoolProperty(ui_->imageEnabled, "LensDistortionEstimator.imageEnabled");
	connectBoolProperty(ui_->imageAxesEnabled, "LensDistortionEstimator.imageAxesEnabled");
	connectBoolProperty(ui_->markPickedPoints, "LensDistortionEstimator.pointMarkersEnabled");
	connectBoolProperty(ui_->markProjectionCenter, "LensDistortionEstimator.projectionCenterMarkerEnabled");
	ui_->imageAxesColor->setup("LensDistortionEstimator.imageAxesColor", "LensDistortionEstimator/image_axes_color");
	ui_->pointMarkerColor->setup("LensDistortionEstimator.pointMarkerColor", "LensDistortionEstimator/point_marker_color");
	ui_->selectedPointMarkerColor->setup("LensDistortionEstimator.selectedPointMarkerColor",
	                                     "LensDistortionEstimator/selected_point_marker_color");
	ui_->projectionCenterMarkerColor->setup("LensDistortionEstimator.projectionCenterMarkerColor",
	                                        "LensDistortionEstimator/center_of_projection_marker_color");
	connect(ui_->restoreDefaultsBtn, &QPushButton::clicked, this, &LensDistortionEstimatorDialog::restoreDefaults);

	// About tab
	setAboutText();
	if(gui)
	{
		ui_->about->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}
	init();

	initialized_ = true;
}

void LensDistortionEstimatorDialog::restoreDefaults()
{
	if(askConfirmation())
	{
		qDebug() << "[LensDistortionEstimator] restore defaults...";
		GETSTELMODULE(LensDistortionEstimator)->restoreDefaultSettings();
	}
	else
	{
		qDebug() << "[LensDistortionEstimator] restore defaults is canceled...";
	}
}

void LensDistortionEstimatorDialog::emitWarning(const QString& text, const bool autoFixable)
{
	ui_->warnings->setStyleSheet("margin-left: 1px; border-radius: 5px; background-color: #ff5757;");
	const auto oldText = ui_->warnings->text();
	if(oldText.isEmpty())
		ui_->warnings->setText(text);
	else
		ui_->warnings->setText(text+"\n"+oldText);
	ui_->warnings->show();
	if(autoFixable)
	{
		ui_->fixWarningBtn->show();
		ui_->fixWarningBtn->setToolTip(q_("Sets simulation date/time to image EXIF date/time, and freezes simulation time"));
	}
}

void LensDistortionEstimatorDialog::clearWarnings()
{
	ui_->warnings->hide();
	ui_->warnings->setText("");
	ui_->fixWarningBtn->hide();
}

double LensDistortionEstimatorDialog::computeExifTimeDifference() const
{
	const auto jd = core_->getJD();
	const auto stelTZ = core_->getUTCOffset(jd); // in hours
	const auto jdc = jd + stelTZ/24.; // UTC -> local TZ
	int year, month, day, hour, minute, second;
	StelUtils::getDateTimeFromJulianDay(jdc, &year, &month, &day, &hour, &minute, &second);
	const bool exifHasLocalTime = exifDateTime_.timeSpec() == Qt::LocalTime;
	const auto currentDateTime = exifHasLocalTime ?
		QDateTime(QDate(year, month, day), QTime(hour, minute, second, 0), Qt::LocalTime) :
		QDateTime(QDate(year, month, day), QTime(hour, minute, second, 0), QTimeZone(stelTZ*3600));
	return exifDateTime_.msecsTo(currentDateTime);
}

void LensDistortionEstimatorDialog::periodicWarningsCheck()
{
	if(!exifDateTime_.isValid()) return;
	clearWarnings();
	const bool exifHasLocalTime = exifDateTime_.timeSpec() == Qt::LocalTime;
	const auto timeDiff = computeExifTimeDifference();
	if(std::abs(timeDiff) > 5000)
		emitWarning(q_("Stellarium time differs from image EXIF time %1 by %2 seconds")
		              .arg(exifHasLocalTime ? exifDateTime_.toString("yyyy-MM-dd hh:mm:ss")
		                                    : exifDateTime_.toString("yyyy-MM-dd hh:mm:ss t"))
		              .arg(timeDiff/1000.),
		            true);
}

void LensDistortionEstimatorDialog::fixWarning()
{
	if(!exifDateTime_.isValid()) return;

	const auto timeDiff = computeExifTimeDifference();
	const auto jd = core_->getJD();
	core_->setJD(jd - timeDiff/86400e3);
	core_->setZeroTimeSpeed();

	clearWarnings();
}

void LensDistortionEstimatorDialog::exportPoints() const
{
	const auto path = QFileDialog::getSaveFileName(&StelMainView::getInstance(), q_("Save points to file"), {},
	                                               q_("Comma-separated values (*.csv)"));
	if(path.isNull()) return;
	QFile file(path);
	if(!file.open(QFile::WriteOnly))
	{
		QMessageBox::critical(&StelMainView::getInstance(), q_("Error saving data"),
		                      q_("Failed to open output file for writing:\n%1").arg(file.errorString()));
		return;
	}
	QTextStream str(&file);
	str << "Object name,Localized object name,Image position X,Image position Y,Exact object azimuth,Exact object elevation\n";
	const auto nMax = ui_->imagePointsPicked->topLevelItemCount();
	for(int n = 0; n < nMax; ++n)
	{
		const auto item = ui_->imagePointsPicked->topLevelItem(n);
		const auto objectName = item->data(Column::ObjectName, Role::ObjectEnglishName).toString();
		const auto object = findObjectByName(objectName);
		double trueAzimuth = NAN, trueElevation = NAN;
		if(!object)
		{
			qWarning() << "Object" << objectName << "not found, will export its sky position as NaN";
		}
		else
		{
			using namespace std;
			const auto objDir = normalize(object->getAltAzPosApparent(core_));
			trueElevation = 180/M_PI * asin(clamp(objDir[2], -1.,1.));
			trueAzimuth = 180/M_PI * atan2(objDir[1], -objDir[0]);
		}
		str << objectName << ","
		    << item->data(Column::ObjectName, Qt::DisplayRole).toString() << ","
		    << item->data(Column::x, Qt::DisplayRole).toDouble() << ","
		    << item->data(Column::y, Qt::DisplayRole).toDouble() << ","
		    << (trueAzimuth < 0 ? 360 : 0) + trueAzimuth << ","
		    << trueElevation << "\n";
	}
	str.flush();
	if(!file.flush())
	{
		QMessageBox::critical(&StelMainView::getInstance(), q_("Error saving data"),
		                      q_("Failed to write the output file:\n%1").arg(file.errorString()));
	}
}

void LensDistortionEstimatorDialog::importPoints()
{
	if(ui_->imagePointsPicked->topLevelItemCount())
	{
		const auto ret = QMessageBox::warning(&StelMainView::getInstance(), q_("Confirm deletion of old points"),
		                                      q_("There are some points defined. They will be deleted "
		                                         "and replaced with the ones from the file. Proceed?"),
		                                      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
		if(ret != QMessageBox::Yes) return;
	}

	const auto path = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Save points to file"), {},
	                                               q_("Comma-separated values (*.csv)"));
	if(path.isNull()) return;
	QFile file(path);
	if(!file.open(QFile::ReadOnly))
	{
		QMessageBox::critical(&StelMainView::getInstance(), q_("Error reading data"),
		                      q_("Failed to open input file for reading:\n%1").arg(file.errorString()));
		return;
	}

	// Keep new items out of the widget till the end, so that, if parsing fails, we didn't destroy the original widget contents
	std::vector<std::unique_ptr<QTreeWidgetItem>> newItems;

	QTextStream str(&file);
	for(int lineNum = 1;; ++lineNum)
	{
		if(str.atEnd()) break;

		const auto entries = str.readLine().split(",");
		constexpr int minEntryCount = 4;
		if(entries.size() < minEntryCount)
		{
			QMessageBox::critical(&StelMainView::getInstance(), q_("File format error"),
			                      q_("Can't import file: expected to find at least %1 entries at line %2, but got %3.")
			                          .arg(minEntryCount).arg(lineNum).arg(entries.size()));
			return;
		}
		if(lineNum==1) continue;
		const auto objectName = entries[0];
		const auto object = StelApp::getInstance().getStelObjectMgr().searchByName(objectName);
		if(!object)
		{
			QMessageBox::critical(&StelMainView::getInstance(), q_("File format error"),
			                      q_("Can't import file: object \"%1\" not found at line %2.").arg(objectName).arg(lineNum));
			return;
		}
		const auto locObjName = entries[1];
		bool ok = false;
		const auto imgPosX = entries[2].toDouble(&ok);
		if(!ok)
		{
			QMessageBox::critical(&StelMainView::getInstance(), q_("File format error"),
			                      q_("Can't import file: failed to parse image position X at line %1.").arg(lineNum));
			return;
		}
		const auto imgPosY = entries[3].toDouble(&ok);
		if(!ok)
		{
			QMessageBox::critical(&StelMainView::getInstance(), q_("File format error"),
			                      q_("Can't import file: failed to parse image position Y at line %1.").arg(lineNum));
			return;
		}
		const auto item = new QTreeWidgetItem({locObjName,
		                                       QString("%1").arg(imgPosX),
		                                       QString("%1").arg(imgPosY)});
		item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
		item->setData(Column::ObjectName, Role::ObjectEnglishName, objectName);
		newItems.emplace_back(item);
	}

	for(int n = ui_->imagePointsPicked->topLevelItemCount() - 1; n >= 0; --n)
		delete ui_->imagePointsPicked->topLevelItem(n);

	for(auto& item : newItems)
		ui_->imagePointsPicked->addTopLevelItem(item.release());

	updateRepositionButtons();
	updateErrorsChart();
	ui_->exportPointsBtn->setEnabled(true);
}

void LensDistortionEstimatorDialog::removeImagePoint()
{
	const auto item = ui_->imagePointsPicked->currentItem();
	if(!item) return;
	delete item;

	updateRepositionButtons();
	updateErrorsChart();
	if(ui_->imagePointsPicked->topLevelItemCount() == 0)
		ui_->exportPointsBtn->setDisabled(true);
}

void LensDistortionEstimatorDialog::updateRepositionButtons()
{
	ui_->repositionBtn->setEnabled(ui_->imagePointsPicked->topLevelItemCount() >= 2 && !image_.isNull());
	ui_->optimizeBtn->setEnabled(ui_->imagePointsPicked->topLevelItemCount() >= 3 && !image_.isNull());
}

void LensDistortionEstimatorDialog::handlePointSelectionChange()
{
	ui_->removePointBtn->setEnabled(!!ui_->imagePointsPicked->currentItem());
}

void LensDistortionEstimatorDialog::handlePointDoubleClick(QTreeWidgetItem*const item, int)
{
	const auto objectName = item->data(Column::ObjectName, Role::ObjectEnglishName).toString();
	const auto object = findObjectByName(objectName);
	if(object) objectMgr_->setSelectedObject(object);
}

void LensDistortionEstimatorDialog::handleSelectedObjectChange(StelModule::StelModuleSelectAction)
{
	if(optimizing_) return;

	if(objectMgr_->getWasSelected() && !image_.isNull())
	{
		ui_->pickImagePointBtn->setEnabled(true);
	}
	else
	{
		ui_->pickImagePointBtn->setChecked(false);
		ui_->pickImagePointBtn->setDisabled(true);
		QApplication::restoreOverrideCursor();
		isPickingAPoint_ = false;
	}
}

void LensDistortionEstimatorDialog::startImagePointPicking(const bool buttonChecked)
{
	if(!buttonChecked)
	{
		QApplication::restoreOverrideCursor();
		isPickingAPoint_ = false;
		return;
	}
	QApplication::setOverrideCursor(lde_->getPointPickCursor());
	isPickingAPoint_ = true;
}

void LensDistortionEstimatorDialog::togglePointPickingMode()
{
	if(!initialized_ || !ui_->pickImagePointBtn->isEnabled()) return;
	const bool mustBeChecked = !ui_->pickImagePointBtn->isChecked();
	ui_->pickImagePointBtn->setChecked(mustBeChecked);
	startImagePointPicking(mustBeChecked);
}

void LensDistortionEstimatorDialog::resetImagePointPicking()
{
	QApplication::restoreOverrideCursor();
	isPickingAPoint_ = false;
	ui_->pickImagePointBtn->setChecked(false);
}

void LensDistortionEstimatorDialog::registerImagePoint(const StelObject& object, const Vec2d& posInImage)
{
	const auto englishName = getObjectName(object);
	auto name = object.getNameI18n();
	if(name.isEmpty())
		name = englishName;
	const auto item = new QTreeWidgetItem({name,
	                                       QString("%1").arg(posInImage[0]),
	                                       QString("%1").arg(posInImage[1])});
	item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
	item->setData(Column::ObjectName, Role::ObjectEnglishName, englishName);
	ui_->imagePointsPicked->addTopLevelItem(item);
	ui_->imagePointsPicked->setCurrentItem(item);

	updateRepositionButtons();
	updateErrorsChart();
	ui_->exportPointsBtn->setEnabled(true);
}

void LensDistortionEstimatorDialog::setupChartAxisStyle(QValueAxis& axis)
{
	// Same as in AstroCalcChart
	static const QPen          pen(Qt::white, 1,    Qt::SolidLine);
	static const QPen      gridPen(Qt::white, 0.5,  Qt::SolidLine);
	static const QPen minorGridPen(Qt::white, 0.35, Qt::DotLine);

	axis.setLinePen(pen);
	axis.setGridLinePen(gridPen);
	axis.setMinorGridLinePen(minorGridPen);
	axis.setTitleBrush(Qt::white);
	axis.setLabelsBrush(Qt::white);
}

double LensDistortionEstimatorDialog::roundToNiceAxisValue(const double x)
{
	if(!std::isfinite(x)) return x;

	const bool negative = x<0;
	// Going through serialization to avoid funny issues with imprecision.
	// This function is not supposed to be called in performance-critical tasks.
	const auto str = QString::number(std::abs(x), 'e', 0);
	auto absRounded = str.toDouble();

	if(absRounded >= std::abs(x))
		return negative ? -absRounded : absRounded;

	// We want to round away from zero, so need to tweak the result
	const int firstDigit = str[0].toLatin1() - '0';
	assert(firstDigit >= 0 && firstDigit <= 9);
	absRounded *= double(firstDigit+1) / firstDigit;
	return negative ? -absRounded : absRounded;
}

void LensDistortionEstimatorDialog::updateErrorsChart()
{
	errorsChart_->removeAllSeries();
	for(const auto axis : errorsChart_->axes())
	{
		errorsChart_->removeAxis(axis);
		delete axis;
	}

	if(image_.isNull()) return;

	using namespace std;
	constexpr auto radian = 180/M_PI;

	const auto series = new QScatterSeries;
	const auto nMax = ui_->imagePointsPicked->topLevelItemCount();
	double xMin = +INFINITY, xMax = -INFINITY, yMin = +INFINITY, yMax = -INFINITY;
	for(int n = 0; n < nMax; ++n)
	{
		const auto item = ui_->imagePointsPicked->topLevelItem(n);
		const auto objectName = item->data(Column::ObjectName, Role::ObjectEnglishName).toString();
		const auto object = findObjectByName(objectName);
		if(!object)
		{
			qWarning() << "Object" << objectName << "not found, chart creation fails";
			delete series;
			return;
		}
		const auto objectDirTrue = normalize(object->getAltAzPosApparent(core_));
		const Vec2d objectXY(item->data(Column::x, Qt::DisplayRole).toDouble(),
		                     item->data(Column::y, Qt::DisplayRole).toDouble());
		const auto objectDirByImg = computeImagePointDir(objectXY);
		const auto centerDir = projectionCenterDir();

		const auto distFromCenterTrue = acos(clamp(dot(objectDirTrue, centerDir), -1.,1.));

		const auto x = distFromCenterTrue * radian;
		const auto y = acos(clamp(dot(objectDirByImg,objectDirTrue), -1.,1.)) * radian;
		if(x < xMin) xMin = x;
		if(x > xMax) xMax = x;
		if(y < yMin) yMin = y;
		if(y > yMax) yMax = y;
		series->append(x, y);
	}
	errorsChart_->addSeries(series);

	const auto xAxis = new QValueAxis;
	xAxis->setTitleText(q_(u8"Distance from center, \u00b0"));
	setupChartAxisStyle(*xAxis);
	const auto xRange = xMax - xMin;
	xAxis->setRange(0, max({xMax + 0.05 * xRange + 1, min(90., imageSmallerSideFoV()/2 * 1.5)}));

	const auto yAxis = new QValueAxis;
	yAxis->setTitleText(q_(u8"Error estimate, \u00b0"));
	setupChartAxisStyle(*yAxis);
	const auto yRange = yMax - yMin;
	const auto yRangeToUse = roundToNiceAxisValue(max(abs(yMin), abs(yMax)) + 0.1 * yRange);
	if(yMin >= 0)
		yAxis->setRange(0, yRangeToUse);
	else if(yMax <= 0)
		yAxis->setRange(-yRangeToUse, 0);
	else
		yAxis->setRange(-yRangeToUse, yRangeToUse);

	errorsChart_->addAxis(xAxis, Qt::AlignBottom);
	errorsChart_->addAxis(yAxis, Qt::AlignLeft);

	series->setBorderColor(Qt::transparent);
	series->setMarkerSize(4 * StelApp::getInstance().getDevicePixelsPerPixel());
	series->attachAxis(xAxis);
	series->attachAxis(yAxis);

	errorsChart_->legend()->hide();
}

double LensDistortionEstimatorDialog::applyDistortion(const double ru) const
{
	const auto model = distortionModel();
	const auto ru2 = ru*ru;
	const auto ru3 = ru2*ru;
	const auto ru4 = ru2*ru2;
	switch(model)
	{
	case DistortionModel::Poly3:
	{
		const auto k1 = distortionTerm1();
		return ru*(1-k1+k1*ru2);
	}
	case DistortionModel::Poly5:
	{
		const auto k1 = distortionTerm1();
		const auto k2 = distortionTerm2();
		return ru*(1+k1*ru2+k2*ru4);
	}
	case DistortionModel::PTLens:
	{
		const auto a = distortionTerm1();
		const auto b = distortionTerm2();
		const auto c = distortionTerm3();
		return ru*(a*ru3+b*ru2+c*ru+1-a-b-c);
	}
	}
	qWarning() << "Unknown distortion model chosen: " << static_cast<int>(model);
	return ru;
}

double LensDistortionEstimatorDialog::maxUndistortedR() const
{
	// The limit value is defined as the point where the distortion model ceases to be monotonic.

	const auto model = distortionModel();
	// The value for the case when the model is monotonic for all ru in [0,inf)
	constexpr double unlimited = std::numeric_limits<float>::max();

	using namespace std;
	switch(model)
	{
	case DistortionModel::Poly3:
	{
		const auto k1 = distortionTerm1();
		const auto square = (k1-1)/(3*k1);
		if(square < 0) return unlimited;
		return sqrt(square);
	}
	case DistortionModel::Poly5:
	{
		const auto k1 = distortionTerm1();
		const auto k2 = distortionTerm2();
		if(k2 == 0 && k1 < 0) return 1/sqrt(-3*k1);
		const auto D = 9*k1*k1-20*k2;
		if(D < 0) return unlimited;
		const auto square = -(3*k1+sqrt(D))/(10*k2);
		if(square < 0) return unlimited;
		return sqrt(square);
	}
	case DistortionModel::PTLens:
	{
		const auto a = distortionTerm1();
		const auto b = distortionTerm2();
		const auto c = distortionTerm3();
		const auto roots = solveCubic(4*a, 3*b, 2*c, 1-a-b-c);
		for(auto root : roots)
			if(root > 0) return root;
		return unlimited;
	}
	}
	qWarning() << "Unknown distortion model chosen: " << static_cast<int>(model);
	return unlimited;
}

Vec2d LensDistortionEstimatorDialog::imagePointToNormalizedCoords(const Vec2d& imagePoint) const
{
	// This is how it works in lensfun: i increases to the right, j increases towards the bottom of the image, and
	// normPos = [(2*i - (width -1)) / min(width-1, height-1) - lensfun.center[0],
	//            (2*j - (height-1)) / min(width-1, height-1) - lensfun.center[1]]
	// Our coordinates are inverted in y with respect to lensfun, so that they are easier
	// to handle, avoiding too many (1,-1) multiplers scattered around the code.
	const double W = image_.width();
	const double H = image_.height();
	const double w = W - 1;
	const double h = H - 1;
	const auto distortedPos = Vec2d(1,-1) * (2. * imagePoint - Vec2d(w, h)) / std::min(w, h) - imageCenterShift();

	using namespace std;
	const auto rd = distortedPos.norm();
	double ruMin = 0;
	double ruMax = 1.3 * hypot(double(image_.width()),image_.height())/min(image_.width(),image_.height()); // FIXME: make a more sound choice
	for(int n = 0; n < 53 && ruMin != ruMax; ++n)
	{
		const auto ru = (ruMin + ruMax) / 2;
		const auto rdCurr = applyDistortion(ru);
		if(rdCurr < rd)
			ruMin = ru;
		else
			ruMax = ru;
	}
	const auto ru = (ruMin + ruMax) / 2;

	return distortedPos / rd * ru;
}

Vec3d LensDistortionEstimatorDialog::computeImagePointDir(const Vec2d& imagePoint) const
{
	using namespace std;
	const auto centeredPoint = imagePointToNormalizedCoords(imagePoint);
	const auto upDir = imageUpDir();
	const auto centerDir = projectionCenterDir();
	const auto rightDir = centerDir ^ upDir;
	const auto smallerSideFoV = M_PI/180 * this->imageSmallerSideFoV();

	const auto x = centeredPoint[0] * tan(smallerSideFoV/2);
	const auto y = centeredPoint[1] * tan(smallerSideFoV/2);
	const auto z = 1.;
	return normalize(x*rightDir + y*upDir + z*centerDir);
}

double LensDistortionEstimatorDialog::computeAngleBetweenImagePoints(const Vec2d& pointA, const Vec2d& pointB, const double smallerSideFoV) const
{
	using namespace std;

	const auto centeredPointA = imagePointToNormalizedCoords(pointA);
	const auto centeredPointB = imagePointToNormalizedCoords(pointB);
	const auto tanFoV2 = tan(smallerSideFoV/2);

	auto pointA3D = Vec3d(centeredPointA[0] * tanFoV2,
			      centeredPointA[1] * tanFoV2,
			      1);
	pointA3D.normalize();

	auto pointB3D = Vec3d(centeredPointB[0] * tanFoV2,
			      centeredPointB[1] * tanFoV2,
			      1);
	pointB3D.normalize();

	return acos(clamp(dot(pointA3D, pointB3D), -1., 1.));
}

void LensDistortionEstimatorDialog::repositionImageByPoints()
{
	Vec2d object0xy, object1xy;
	Vec3d object0dirTrue, object1dirTrue;
	using namespace std;

	if(optimizing_)
	{
		if(objectPositions_.size() < 2)
		{
			qWarning() << "Reposition called while number of points less than 2. Ignoring.";
			return;
		}
		object0xy = objectPositions_[0].imgPos;
		object1xy = objectPositions_[1].imgPos;
		object0dirTrue = objectPositions_[0].skyDir;
		object1dirTrue = objectPositions_[1].skyDir;
	}
	else
	{
		if(ui_->imagePointsPicked->topLevelItemCount() < 2)
		{
			qWarning() << "Reposition called while number of points less than 2. Ignoring.";
			return;
		}
		const auto item0 = ui_->imagePointsPicked->topLevelItem(0);
		const auto object0Name = item0->data(Column::ObjectName, Role::ObjectEnglishName).toString();
		const auto object0 = findObjectByName(object0Name);
		if(!object0)
		{
			qWarning() << "Object" << object0Name << "not found, repositioning fails";
			return;
		}

		const auto item1 = ui_->imagePointsPicked->topLevelItem(1);
		const auto object1Name = item1->data(Column::ObjectName, Role::ObjectEnglishName).toString();
		const auto object1 = findObjectByName(object1Name);
		if(!object1)
		{
			qWarning() << "Object" << object1Name << "not found, repositioning fails";
			return;
		}

		object0dirTrue = normalize(object0->getAltAzPosApparent(core_));
		object0xy = Vec2d(item0->data(Column::x, Qt::DisplayRole).toDouble(),
		                  item0->data(Column::y, Qt::DisplayRole).toDouble());

		object1dirTrue = normalize(object1->getAltAzPosApparent(core_));
		object1xy = Vec2d(item1->data(Column::x, Qt::DisplayRole).toDouble(),
		                  item1->data(Column::y, Qt::DisplayRole).toDouble());
	}
	auto object0dirByImg = computeImagePointDir(object0xy);
	auto object1dirByImg = computeImagePointDir(object1xy);

	// Find the correct FoV to make angle between the two image points correct
	const auto angleBetweenObjectsTrue = acos(clamp(dot(object0dirTrue, object1dirTrue), -1., 1.));
	double fovMax = 0.999*M_PI;
	double fovMin = 0;
	double fov = (fovMin + fovMax) / 2.;
	for(int n = 0; n < 53 && fovMin != fovMax; ++n)
	{
		const auto angleBetweenObjectsByImg = computeAngleBetweenImagePoints(object0xy, object1xy, fov);
		if(angleBetweenObjectsByImg > angleBetweenObjectsTrue)
			fovMax = fov;
		else
			fovMin = fov;
		fov = (fovMin + fovMax) / 2.;
	}
	setImageSmallerSideFoV(180/M_PI * fov);

	// Recompute the directions after FoV update
	object0dirByImg = computeImagePointDir(object0xy);
	object1dirByImg = computeImagePointDir(object1xy);

	// Find the matrix to rotate two image points to corresponding directions in space
	const auto crossTrue = object0dirTrue ^ object1dirTrue;
	const auto crossByImg = object0dirByImg ^ object1dirByImg;
	const auto preRotByImg = Mat3d(object0dirByImg[0], object0dirByImg[1], object0dirByImg[2],
	                               object1dirByImg[0], object1dirByImg[1], object1dirByImg[2],
	                                    crossByImg[0],      crossByImg[1],      crossByImg[2]);
	const auto preRotTrue = Mat3d(object0dirTrue[0], object0dirTrue[1], object0dirTrue[2],
	                              object1dirTrue[0], object1dirTrue[1], object1dirTrue[2],
	                                   crossTrue[0],      crossTrue[1],      crossTrue[2]);
	const auto rotator = preRotTrue * preRotByImg.inverse();

	const auto origUpDir = imageUpDir();
	const auto origCenterDir = projectionCenterDir();
	const auto newCenterDir = normalize(rotator * origCenterDir);
	const auto elevation = asin(clamp(newCenterDir[2], -1.,1.));
	const auto azimuth = atan2(newCenterDir[1], -newCenterDir[0]);
	setProjectionCenterAzimuth(180/M_PI * azimuth);
	setProjectionCenterElevation(180/M_PI * elevation);

	const auto origFromCenterToTop = normalize(origCenterDir + 1e-8 * origUpDir);
	const auto newFromCenterToTop = normalize(rotator * origFromCenterToTop);
	// Desired up direction
	const auto upDirNew = normalize(newFromCenterToTop - newCenterDir);
	// Renewed up direction takes into account the new center direction but not the desired orientation yet
	setImageFieldRotation(0);
	const auto renewedUpDir = imageUpDir();
	const auto upDirCross = renewedUpDir ^ upDirNew;
	const auto upDirDot = dot(renewedUpDir, upDirNew);
	const auto dirSign = dot(upDirCross, newCenterDir) > 0 ? -1. : 1.;
	const auto upDirSinAngle = dirSign * (dirSign * upDirCross).norm();
	const auto upDirCosAngle = upDirDot;
	const auto fieldRotation = atan2(upDirSinAngle, upDirCosAngle);
	setImageFieldRotation(180/M_PI * fieldRotation);

	if(!optimizing_)
		updateErrorsChart();
}

void LensDistortionEstimatorDialog::resetImagePlacement()
{
	if(image_.isNull())
	{
		setImageFieldRotation(0);
		setProjectionCenterAzimuth(0);
		setProjectionCenterElevation(0);
		setImageSmallerSideFoV(60);
		return;
	}

	const auto ret = QMessageBox::warning(&StelMainView::getInstance(), q_("Confirm resetting image placement"),
	                                      q_("This will move the image to the center of the screen. Proceed?"),
	                                      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	if(ret != QMessageBox::Yes) return;

	placeImageInCenterOfScreen();
}

void LensDistortionEstimatorDialog::resetDistortion()
{
	const auto ret = QMessageBox::warning(&StelMainView::getInstance(), q_("Confirm resetting distortion"),
	                                      q_("This will zero out all distortion and shift coefficients. Proceed?"),
	                                      QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	if(ret != QMessageBox::Yes) return;

	setImageCenterShiftX(0);
	setImageCenterShiftY(0);
	setDistortionTerm1(0);
	setDistortionTerm2(0);
	setDistortionTerm3(0);
}

void LensDistortionEstimatorDialog::optimizeParameters()
{
#ifdef HAVE_NLOPT
	const auto funcToMinimize = [](const unsigned n, const double *v, double*, void*const params)
	{
		const auto dialog = static_cast<LensDistortionEstimatorDialog*>(params);

		const double xShift = v[0];
		const double yShift = v[1];
		const double imageFieldRot = v[2];
		const double centerAzim = v[3];
		const double centerElev = v[4];
		const double a = v[5];
		const double b = dialog->distortionTerm2inUse_ ? v[6] : 0.;
		const double c = dialog->distortionTerm3inUse_ ? v[7] : 0.;

		dialog->setImageCenterShiftX(xShift);
		dialog->setImageCenterShiftY(yShift);
		dialog->setImageFieldRotation(imageFieldRot);
		dialog->setProjectionCenterAzimuth(centerAzim);
		dialog->setProjectionCenterElevation(centerElev);
		dialog->setDistortionTerm1(a);
		dialog->setDistortionTerm2(b);
		dialog->setDistortionTerm3(c);

		dialog->repositionImageByPoints();

		double errorMeasure = 0;
		for(const auto& obj : dialog->objectPositions_)
		{
			const auto objectDirByImg = dialog->computeImagePointDir(obj.imgPos);

			using namespace std;
			const auto error = acos(clamp(dot(objectDirByImg,obj.skyDir), -1.,1.));
			errorMeasure += sqr(error);
		}
		return errorMeasure;
	};

	objectPositions_.clear();
	for(int n = 0; n < ui_->imagePointsPicked->topLevelItemCount(); ++n)
	{
		const auto item = ui_->imagePointsPicked->topLevelItem(n);
		const auto objectName = item->data(Column::ObjectName, Role::ObjectEnglishName).toString();
		const auto object = findObjectByName(objectName);
		if(!object)
		{
			qWarning() << "Object" << objectName << "not found, skipping it during optimization";
			continue;
		}
		const auto objectDirTrue = normalize(object->getAltAzPosApparent(core_));
		const Vec2d objectXY(item->data(Column::x, Qt::DisplayRole).toDouble(),
		                     item->data(Column::y, Qt::DisplayRole).toDouble());
		objectPositions_.push_back({objectXY, objectDirTrue});
	}

	distortionTerm1_ = distortionTerm1(); distortionTerm1min_ = ui_->distortionTerm1->minimum(); distortionTerm1max_ = ui_->distortionTerm1->maximum();
	distortionTerm2_ = distortionTerm2(); distortionTerm2min_ = ui_->distortionTerm2->minimum(); distortionTerm2max_ = ui_->distortionTerm2->maximum();
	distortionTerm3_ = distortionTerm3(); distortionTerm3min_ = ui_->distortionTerm3->minimum(); distortionTerm3max_ = ui_->distortionTerm3->maximum();

	imageCenterShiftX_ = imageCenterShiftX();
	imageCenterShiftXmin_ = ui_->imageCenterShiftX->minimum();
	imageCenterShiftXmax_ = ui_->imageCenterShiftX->maximum();

	imageCenterShiftY_ = imageCenterShiftY();
	imageCenterShiftYmin_ = ui_->imageCenterShiftY->minimum();
	imageCenterShiftYmax_ = ui_->imageCenterShiftY->maximum();

	imageSmallerSideFoV_ = ui_->imageSmallerSideFoV->value();

	imageFieldRotation_ = imageFieldRotation();
	imageFieldRotationMin_ = ui_->imageFieldRotation->minimum();
	imageFieldRotationMax_ = ui_->imageFieldRotation->maximum();

	projectionCenterAzimuth_ = projectionCenterAzimuth();
	projectionCenterAzimuthMin_ = ui_->projectionCenterAzimuth->minimum();
	projectionCenterAzimuthMax_ = ui_->projectionCenterAzimuth->maximum();

	projectionCenterElevation_ = projectionCenterElevation();
	projectionCenterElevationMin_ = ui_->projectionCenterElevation->minimum();
	projectionCenterElevationMax_ = ui_->projectionCenterElevation->maximum();

	distortionModel_ = distortionModel();
	optimizing_ = true;

	repositionImageByPoints();

	const unsigned numVars = 5 +
	                         distortionTerm1inUse_ +
	                         distortionTerm2inUse_ +
	                         distortionTerm3inUse_;
	std::vector<double> values{
	                           imageCenterShiftX_,
	                           imageCenterShiftY_,
	                           imageFieldRotation_,
	                           projectionCenterAzimuth_,
	                           projectionCenterElevation_,
	                           distortionTerm1_,
	                           distortionTerm2_,
	                           distortionTerm3_,
	                          };
	std::vector<double> lowerBounds{
	                                  imageCenterShiftXmin_,
	                                  imageCenterShiftYmin_,
	                                  imageFieldRotationMin_,
	                                  projectionCenterAzimuthMin_,
	                                  projectionCenterElevationMin_,
	                                  distortionTerm1min_,
	                                  distortionTerm2min_,
	                                  distortionTerm3min_,
	                               };
	std::vector<double> upperBounds{
	                                  imageCenterShiftXmax_,
	                                  imageCenterShiftYmax_,
	                                  imageFieldRotationMax_,
	                                  projectionCenterAzimuthMax_,
	                                  projectionCenterElevationMax_,
	                                  distortionTerm1max_,
	                                  distortionTerm2max_,
	                                  distortionTerm3max_,
	                               };
	// Remove trailing values if needed
	values.resize(numVars);
	lowerBounds.resize(numVars);
	upperBounds.resize(numVars);

	nlopt::opt minimizer(nlopt::algorithm::LN_NELDERMEAD, numVars);
	minimizer.set_lower_bounds(lowerBounds);
	minimizer.set_upper_bounds(upperBounds);
	minimizer.set_min_objective(funcToMinimize, this);
	minimizer.set_ftol_rel(1e-3);

	try
	{
		double minf = INFINITY;
		const auto result = minimizer.optimize(values, minf);
		if(result > 0)
			qDebug() << "Found minimum, error measure:" << minf;
		else
			qCritical() << "NLOpt failed with result " << minimizer.get_errmsg();
	}
	catch(const std::exception& ex)
	{
		qCritical() << "NLOpt failed:" << ex.what();
	}

	optimizing_ = false;
	setDistortionTerm1(distortionTerm1_);
	setDistortionTerm2(distortionTerm2_);
	setDistortionTerm3(distortionTerm3_);
	setImageCenterShiftX(imageCenterShiftX_);
	setImageCenterShiftY(imageCenterShiftY_);
	setImageFieldRotation(imageFieldRotation_);
	setImageSmallerSideFoV(imageSmallerSideFoV_);
	setProjectionCenterAzimuth(projectionCenterAzimuth_);
	setProjectionCenterElevation(projectionCenterElevation_);

	updateErrorsChart();
#endif
}

void LensDistortionEstimatorDialog::updateImagePathStatus()
{
	if(QFileInfo(ui_->imageFilePath->text()).exists())
	{
		ui_->imageFilePath->setStyleSheet("");
	}
	else
	{
		ui_->imageFilePath->setStyleSheet("color:red;");
	}
}

void LensDistortionEstimatorDialog::computeColorToSubtract()
{
	// Compute the (rounded down) median of all the pixels
	// First get the histogram
	std::array<size_t, 256> histogramR{};
	std::array<size_t, 256> histogramG{};
	std::array<size_t, 256> histogramB{};
	const uchar*const data = image_.bits();
	const auto stride = image_.bytesPerLine();
	for(int j = 0; j < image_.height(); ++j)
	{
		for(int i = 0; i < image_.width(); ++i)
		{
			const auto r = data[j*stride+4*i+0];
			const auto g = data[j*stride+4*i+1];
			const auto b = data[j*stride+4*i+2];
			++histogramR[r];
			++histogramG[g];
			++histogramB[b];
		}
	}
	// Now use the histogram to find the (rounded down) median
	const auto totalR = std::accumulate(histogramR.begin(), histogramR.end(), size_t(0));
	const auto totalG = std::accumulate(histogramG.begin(), histogramG.end(), size_t(0));
	const auto totalB = std::accumulate(histogramB.begin(), histogramB.end(), size_t(0));
	size_t sumR = 0, sumG = 0, sumB = 0;
	int midR = -1, midG = -1, midB = -1;
	for(unsigned n = 0; n < histogramR.size(); ++n)
	{
		sumR += histogramR[n];
		sumG += histogramG[n];
		sumB += histogramB[n];
		if(midR < 0 && sumR > totalR/2) midR = n;
		if(midG < 0 && sumG > totalG/2) midG = n;
		if(midB < 0 && sumB > totalB/2) midB = n;
	}
	ui_->bgRed->setValue(midR);
	ui_->bgGreen->setValue(midG);
	ui_->bgBlue->setValue(midB);
}

void LensDistortionEstimatorDialog::placeImageInCenterOfScreen()
{
	using namespace std;

	const auto mvtMan = core_->getMovementMgr();
	const auto vFoV = mvtMan->getCurrentFov();
	setImageSmallerSideFoV(clamp(0.98*vFoV, 0.,170.));

	const auto j2000 = mvtMan->getViewDirectionJ2000();
	const auto altAz = core_->j2000ToAltAz(j2000, StelCore::RefractionOff);
	const auto elevation = asin(clamp(altAz[2], -1.,1.));
	const auto azimuth = atan2(altAz[1], -altAz[0]);
	setProjectionCenterElevation(180/M_PI * elevation);
	setProjectionCenterAzimuth(180/M_PI * azimuth);
	setImageFieldRotation(0);

	if(!mvtMan->getEquatorialMount())
		return;

	const auto proj = core_->getProjection(StelCore::FrameAltAz, StelCore::RefractionMode::RefractionOff);
	const auto imageCenterDir = projectionCenterDir();
	const auto imageUpperDir = normalize(imageCenterDir + 1e-3 * imageUpDir());
	Vec3d imgCenterWin, imgUpWin;
	proj->project(imageCenterDir, imgCenterWin);
	proj->project(imageUpperDir, imgUpWin);
	const auto angle = -atan2(imgUpWin[1]-imgCenterWin[1], imgUpWin[0]-imgCenterWin[0]) + M_PI/2;
	setImageFieldRotation(180/M_PI * angle);
}

void LensDistortionEstimatorDialog::updateImage()
{
	const auto path = ui_->imageFilePath->text();
	image_ = QImage(path).convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
	updateRepositionButtons();

	if(image_.isNull())
	{
		ui_->imageFilePath->setStyleSheet("color:red;");
		return;
	}

	ui_->imageFilePath->setStyleSheet("");
	imageChanged_ = true;
	setupGenerateLensfunModelButton();
	computeColorToSubtract();
	placeImageInCenterOfScreen();

#ifdef HAVE_EXIV2
	try
	{
		const auto image = Exiv2::ImageFactory::open(path.toStdString());
		if(!image.get()) return;
		image->readMetadata();
		const auto& exif = image->exifData();

		QString date;
		for(const auto& key : {"Exif.Photo.DateTimeOriginal",
		                       "Exif.Image.DateTimeOriginal",
		                       "Exif.Image.DateTime"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it!=exif.end())
			{
				date = QString::fromStdString(it->toString());
				break;
			}
		}

		int timeZone = INT_MIN;
		for(const auto& key : {"Exif.CanonTi.TimeZone"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it != exif.end() && it->typeId() == Exiv2::signedLong)
			{
#if EXIV2_MINOR_VERSION < 28
				const auto num = it->toLong();
#else
				const auto num = it->toInt64();
#endif
				timeZone = num * 60; // save as seconds
				break;
			}
		}

		QString gpsTime;
		for(const auto& key : {"Exif.GPSInfo.GPSTimeStamp"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it!=exif.end())
			{
				if(it->count() != 3)
					continue;
				const auto hour = it->toFloat(0);
				const auto min = it->toFloat(1);
				const auto sec = it->toFloat(2);
				if(hour < 0 || hour > 59 || hour - std::floor(hour) != 0 ||
				   min  < 0 || min  > 59 || min  - std::floor(min ) != 0)
					continue;
				gpsTime = QString("%1:%2:%3").arg(int(hour), 2, 10, QChar('0'))
				                             .arg(int(min), 2, 10, QChar('0'))
				                             .arg(double(sec), 2, 'f', 0, QChar('0'));
				break;
			}
		}

		QString gpsDate;
		for(const auto& key : {"Exif.GPSInfo.GPSDateStamp"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it!=exif.end())
			{
				gpsDate = QString::fromStdString(it->toString());
				break;
			}
		}

		// If GPS date and time are present, take them as more reliable.
		// At the very least, time zone is present there unconditionally.
		if(!gpsDate.isEmpty() && !gpsTime.isEmpty())
		{
			date = gpsDate + " " + gpsTime;
			timeZone = 0;
		}

		exifDateTime_ = {};
		clearWarnings();
		if(date.isEmpty())
		{
			emitWarning(q_("Can't get EXIF date from the image, please make "
			               "sure current date and time setting is correct."));
		}
		else
		{
			exifDateTime_ = QDateTime::fromString(date, "yyyy:MM:dd HH:mm:ss");
			if(timeZone != INT_MIN)
				exifDateTime_.setTimeZone(QTimeZone(timeZone));
			if(!exifDateTime_.isValid())
			{
				emitWarning(q_("Failed to parse EXIF date/time, please make sure "
				               "current date and time setting is correct."));
			}
			else
				periodicWarningsCheck();
			warningCheckTimer_.start();
		}

		for(const auto& key : {"Exif.Photo.FocalLength", "Exif.Image.FocalLength"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it==exif.end() || it->typeId() != Exiv2::unsignedRational)
				continue;
			const auto [num,denom] = it->toRational();
			if(denom==0) continue;
			ui_->lensFocalLength->setValue(double(num)/denom);
			break;
		}

		for(const auto& key : {"Exif.Photo.LensModel"})
		{
			const auto it=exif.findKey(Exiv2::ExifKey(key));
			if(it != exif.end() && it->typeId() == Exiv2::signedLong)
			{
				const auto model = it->toString();
				if(model.empty()) continue;
				ui_->lensMake->setText(model.c_str());
				break;
			}
		}
	}
	catch(Exiv2::Error& e)
	{
		qDebug() << "exiv2 error:" << e.what();
	}
#endif
}

void LensDistortionEstimatorDialog::browseForImage()
{
	const auto path = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Open image file"), {},
	                                               q_("Image files (*.png *.bmp *.jpg *.jpeg *.tif *.tiff *.webm *.pbm *.pgm *.ppm *.xbm *.xpm)"));
	if(path.isNull()) return;
	ui_->imageFilePath->setText(path);
	updateImage();
}

void LensDistortionEstimatorDialog::init()
{
}

Vec3d LensDistortionEstimatorDialog::imageUpDir() const
{
	const auto az = M_PI/180 * projectionCenterAzimuth();
	const auto el = M_PI/180 * projectionCenterElevation();
	// This unrotated direction is just a derivative of projectionCenterDir with respect to elevation
	const auto sinEl = std::sin(el);
	const auto cosEl = std::cos(el);
	const auto cosAz = std::cos(az);
	const auto sinAz = std::sin(az);
	const Vec3d unrotated(sinEl * cosAz, -sinEl * sinAz, cosEl);
	// The rotated direction that we return is the one that takes imageFieldRotation into account.
	const Vec3d centerDir(-cosEl * cosAz, cosEl * sinAz, sinEl); // same as projectionCenterDir(), but reusing sines & cosines
	const Mat3d rotator = Mat4d::rotation(centerDir, -M_PI/180 * imageFieldRotation()).upper3x3();
	const Vec3d rotated = rotator * unrotated;
	return Vec3d(rotated[0], rotated[1], rotated[2]);
}

Vec3d LensDistortionEstimatorDialog::projectionCenterDir() const
{
	const auto az = M_PI/180 * projectionCenterAzimuth();
	const auto el = M_PI/180 * projectionCenterElevation();
	// Following FrameAltAz
	return Vec3d(-std::cos(el) * std::cos(az),
	              std::cos(el) * std::sin(az),
	              std::sin(el));
}

Vec2d LensDistortionEstimatorDialog::imageCenterShift() const
{
	return 2. * Vec2d(-imageCenterShiftX(), imageCenterShiftY())
	                              /
	       std::min(image_.width()-1, image_.height()-1);
}

QColor LensDistortionEstimatorDialog::bgToSubtract() const
{
	if(ui_->subtractBG->isChecked())
		return QColor(ui_->bgRed->value(), ui_->bgGreen->value(), ui_->bgBlue->value());
	return QColor(0,0,0);
}

double LensDistortionEstimatorDialog::imageBrightness() const
{
	return ui_->imageBrightness->value() / 100.;
}

double LensDistortionEstimatorDialog::distortionTerm1() const
{
	if(optimizing_) return distortionTerm1_;
	return ui_->distortionTerm1->value();
}

double LensDistortionEstimatorDialog::distortionTerm2() const
{
	if(optimizing_) return distortionTerm2_;
	return ui_->distortionTerm2->value();
}

double LensDistortionEstimatorDialog::distortionTerm3() const
{
	if(optimizing_) return distortionTerm3_;
	return ui_->distortionTerm3->value();
}

double LensDistortionEstimatorDialog::imageCenterShiftX() const
{
	if(optimizing_) return imageCenterShiftX_;
	return ui_->imageCenterShiftX->value();
}

double LensDistortionEstimatorDialog::imageCenterShiftY() const
{
	if(optimizing_) return imageCenterShiftY_;
	return ui_->imageCenterShiftY->value();
}

double LensDistortionEstimatorDialog::imageFieldRotation() const
{
	if(optimizing_) return imageFieldRotation_;
	return ui_->imageFieldRotation->value();
}

double LensDistortionEstimatorDialog::projectionCenterAzimuth() const
{
	if(optimizing_) return projectionCenterAzimuth_;
	return ui_->projectionCenterAzimuth->value();
}

double LensDistortionEstimatorDialog::projectionCenterElevation() const
{
	if(optimizing_) return projectionCenterElevation_;
	return ui_->projectionCenterElevation->value();
}

double LensDistortionEstimatorDialog::imageSmallerSideFoV() const
{
	if(optimizing_) return imageSmallerSideFoV_;
	return ui_->imageSmallerSideFoV->value();
}

DistortionModel LensDistortionEstimatorDialog::distortionModel() const
{
	if(optimizing_) return distortionModel_;
	return static_cast<DistortionModel>(ui_->distortionModelCB->currentIndex());
}

void LensDistortionEstimatorDialog::setDistortionTerm1(const double a)
{
	if(optimizing_) distortionTerm1_ = a;
	else ui_->distortionTerm1->setValue(a);
}

void LensDistortionEstimatorDialog::setDistortionTerm2(const double b)
{
	if(optimizing_) distortionTerm2_ = b;
	else ui_->distortionTerm2->setValue(b);
}

void LensDistortionEstimatorDialog::setDistortionTerm3(const double c)
{
	if(optimizing_) distortionTerm3_ = c;
	else ui_->distortionTerm3->setValue(c);
}

void LensDistortionEstimatorDialog::setImageCenterShiftX(const double xShift)
{
	if(optimizing_) imageCenterShiftX_ = xShift;
	else ui_->imageCenterShiftX->setValue(xShift);
}

void LensDistortionEstimatorDialog::setImageCenterShiftY(const double yShift)
{
	if(optimizing_) imageCenterShiftY_ = yShift;
	else ui_->imageCenterShiftY->setValue(yShift);
}

void LensDistortionEstimatorDialog::setImageFieldRotation(const double imageFieldRot)
{
	if(optimizing_) imageFieldRotation_ = imageFieldRot;
	else ui_->imageFieldRotation->setValue(imageFieldRot);
}

void LensDistortionEstimatorDialog::setProjectionCenterAzimuth(const double centerAzim)
{
	if(optimizing_) projectionCenterAzimuth_ = centerAzim;
	else ui_->projectionCenterAzimuth->setValue(centerAzim);
}

void LensDistortionEstimatorDialog::setProjectionCenterElevation(const double centerElev)
{
	if(optimizing_) projectionCenterElevation_ = centerElev;
	else ui_->projectionCenterElevation->setValue(centerElev);
}

void LensDistortionEstimatorDialog::setImageSmallerSideFoV(const double fov)
{
	if(optimizing_) imageSmallerSideFoV_ = fov;
	else ui_->imageSmallerSideFoV->setValue(fov);
}

auto LensDistortionEstimatorDialog::imagePointDirections() const -> std::vector<ImagePointStatus>
{
	std::vector<ImagePointStatus> statuses;
	const auto nMax = ui_->imagePointsPicked->topLevelItemCount();
	for(int n = 0; n < nMax; ++n)
	{
		const auto item = ui_->imagePointsPicked->topLevelItem(n);
		const Vec2d objectXY(item->data(Column::x, Qt::DisplayRole).toDouble(),
		                     item->data(Column::y, Qt::DisplayRole).toDouble());
		statuses.push_back({computeImagePointDir(objectXY), item->isSelected()});
	}
	return statuses;
}

QString LensDistortionEstimatorDialog::getObjectName(const StelObject& object) const
{
	auto name = object.getEnglishName();
	if(!name.isEmpty()) return name;

	double raJ2000, decJ2000;
	StelUtils::rectToSphe(&raJ2000, &decJ2000, object.getJ2000EquatorialPos(core_));
	return QString("unnamed{RA=%1;DEC=%2}").arg(raJ2000 * 12/M_PI,0,'g',17).arg(decJ2000 * 180/M_PI,0,'g',17);
}

StelObjectP LensDistortionEstimatorDialog::findObjectByName(const QString& name) const
{
	const QLatin1String unnamedObjectPrefix("unnamed{RA=");
	if(!name.startsWith(unnamedObjectPrefix))
	{
		const auto obj = objectMgr_->searchByName(name);
		if(!obj) return nullptr;
		// Sometimes we get a completely wrong object when the correct one isn't found
		if(obj->getEnglishName() != name) return nullptr;
		return obj;
	}

	// Parse the unnamed object
	if(!name.endsWith("}")) return nullptr;
	const auto raDecStr = name.mid(unnamedObjectPrefix.size(), name.size() - unnamedObjectPrefix.size() - 1);
	const auto raDecStrings = raDecStr.split(";DEC=");
	if(raDecStrings.size() != 2 || raDecStrings[0].isEmpty() || raDecStrings[1].isEmpty())
	{
		qWarning() << "Failed to split unnamed object name" << raDecStrings;
		return nullptr;
	}
	bool okRA = false, okDE = false;
	const auto ra = raDecStrings[0].toDouble(&okRA);
	const auto dec = raDecStrings[1].toDouble(&okDE);
	if(!okRA || !okDE)
	{
		qWarning() << "Failed to parse RA" << raDecStrings[0] << "or DE" << raDecStrings[1];
		return nullptr;
	}

	// There's no specific string handle for unnamed objects, so they have to be searched by coordinates
	Vec3d coords(0,0,0);
	StelUtils::spheToRect(ra*M_PI/12, dec*M_PI/180, coords);

	QList<StelObjectP> stars = starMgr_->searchAround(coords, 1e-4, core_);
	StelObjectP object = nullptr;
	double minDist = INFINITY;
	for (const auto& currObj : stars)
	{
		const auto dist = coords.angle(currObj->getJ2000EquatorialPos(core_));
		if (dist < minDist)
		{
			minDist = dist;
			object = currObj;
		}
	}
	return object;
}

void LensDistortionEstimatorDialog::updateDistortionCoefControls()
{
	const auto model = distortionModel();
	distortionTerm1inUse_ = true;
	// NOTE: simply hide/show is not sufficient to query this status when the tab is not at
	// the page with these controls, so we use enable/disable in addition to visibility.
	switch(model)
	{
	case DistortionModel::Poly3:
		ui_->distortionTerm2->hide(); distortionTerm2inUse_ = false;
		ui_->distortionTerm3->hide(); distortionTerm3inUse_ = false;
		break;
	case DistortionModel::Poly5:
		ui_->distortionTerm2->show(); distortionTerm2inUse_ = true;
		ui_->distortionTerm3->hide(); distortionTerm3inUse_ = false;
		break;
	case DistortionModel::PTLens:
		ui_->distortionTerm2->show(); distortionTerm2inUse_ = true;
		ui_->distortionTerm3->show(); distortionTerm3inUse_ = true;
		break;
	}
}

void LensDistortionEstimatorDialog::setupGenerateLensfunModelButton()
{
	const bool anyEmpty = ui_->lensMake->text().isEmpty()  ||
	                      ui_->lensModel->text().isEmpty() ||
	                      ui_->lensMount->text().isEmpty() ||
	                      image_.isNull();
	ui_->generateLensfunModelBtn->setEnabled(!anyEmpty);
	if(anyEmpty)
		ui_->generateLensfunModelBtn->setToolTip(q_("Make, model and mount fields must be filled"));
	else
		ui_->generateLensfunModelBtn->setToolTip("");
}

void LensDistortionEstimatorDialog::generateLensfunModel()
{
	const auto lensfunCenter = -imageCenterShift();
	const auto model = distortionModel();
	QString distModel;
	switch(model)
	{
	case DistortionModel::Poly3:
		distModel = QString(R"(<distortion model="poly3" focal="%1" k1="%2"/>)")
		                .arg(ui_->lensFocalLength->value(), 0, 'g', 7)
		                .arg(distortionTerm1());
		break;
	case DistortionModel::Poly5:
		distModel = QString(R"(<distortion model="poly5" focal="%1" k1="%2" k2="%3"/>)")
		                .arg(ui_->lensFocalLength->value(), 0, 'g', 7)
		                .arg(distortionTerm1())
		                .arg(distortionTerm2());
		break;
	case DistortionModel::PTLens:
		distModel = QString(R"(<distortion model="ptlens" focal="%1" a="%2" b="%3" c="%4"/>)")
		                .arg(ui_->lensFocalLength->value(), 0, 'g', 7)
		                .arg(distortionTerm1())
		                .arg(distortionTerm2())
		                .arg(distortionTerm3());
		break;
	default:
		ui_->lensfunModelXML->setText(q_("Internal error: bad distortion model chosen"));
		return;
	}

	const auto gcd = std::gcd(image_.width(), image_.height());
	const int aspectNum = image_.width() / gcd;
	const int aspectDenom = image_.height() / gcd;

	ui_->lensfunModelXML->setText(QString(1+R"(
<lens>
    <maker>%1</maker>
    <model>%2</model>
    <mount>%3</mount>
    <cropfactor>%4</cropfactor>
    <aspect-ratio>%5:%6</aspect-ratio>
    <center x="%7" y="%8"/>
    <calibration>
        %9
    </calibration>
</lens>
)").arg(ui_->lensMake->text())
   .arg(ui_->lensModel->text())
   .arg(ui_->lensMount->text())
   .arg(computeLensCropFactor(), 0, 'g', 7)
   .arg(aspectNum)
   .arg(aspectDenom)
   .arg(lensfunCenter[0], 0, 'f', 7)
   .arg(lensfunCenter[1], 0, 'f', 7)
   .arg(distModel));
}

double LensDistortionEstimatorDialog::computeLensCropFactor() const
{
	const double f = ui_->lensFocalLength->value();
	const double alpha = M_PI/180 * imageSmallerSideFoV();
	const double h = 2*f * std::tan(alpha/2);
	const double largerSidePx  = std::max(image_.width(), image_.height());
	const double smallerSidePx = std::min(image_.width(), image_.height());
	const double aspect = largerSidePx/smallerSidePx;
	const double w = h*aspect;
	return std::hypot(36,24) / std::hypot(w,h);
}

void LensDistortionEstimatorDialog::showSettingsPage()
{
	ui_->tabs->setCurrentIndex(Page::Settings);
}

void LensDistortionEstimatorDialog::showComputationPage()
{
	const auto current = ui_->tabs->currentIndex();
	if(current != Page::ImageLoading && current != Page::Fitting)
		ui_->tabs->setCurrentIndex(Page::ImageLoading);
}

void LensDistortionEstimatorDialog::setAboutText()
{
	QString html = "<html><head></head><body>"
	"<h2>" + q_("Lens Distortion Estimator Plug-in") + "</h2>"
	"<table class='layout' width=\"90%\">"
		"<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + LENSDISTORTIONESTIMATOR_PLUGIN_VERSION + "</td></tr>"
		"<tr><td><strong>" + q_("License") + ":</strong></td><td>" + LENSDISTORTIONESTIMATOR_PLUGIN_LICENSE + "</td></tr>"
		"<tr><td><strong>" + q_("Author") + ":</strong></td><td>Ruslan Kabatsayev &lt;b7.10110111+stellarium@gmail.com&gt;</td></tr>"
	"</table>";
	// Overview
	html += "<h3>" + q_("Overview") + "</h3>";

	html += "<p>" + q_("This plugin lets you estimate distortion of the objective lens of your camera and generate "
	                   "a distortion model for use with the lensfun library.") + "</p>";
	html += "<p>" + q_("The process is as follows.") + "</p>";
	html += "<ol>"
	        "<li>" + q_("Take a photo of the night sky so that you can easily find enough stars scattered from the "
	                    "center of the frame to the border. Some care must be taken to get a useful shot, see more "
	                    "details in <b>Preparing a photo</b> section.\n") + "</li>"
	        "<li>" + q_("On the <i>Image Loading</i> tab choose the image to load and use the available controls (or "
	                    "hotkeys) to position it so that it approximately matches the direction where the camera "
	                    "looked and corresponding orientation. You need to be able to tell which point in the image "
	                    "corresponds to which simulated star/planet.") + "</li>"
	        "<li>" + q_("Then you switch to the <i>Fitting</i> tab, and pick some points in the image for a set "
	                    "of celestial objects. To do this,\n"
	                    "<ol><li>Select an object as you'd normally do.</li>\n"
	                        "<li>Click <i>Pick an image point for selected object...</i> button (or use a hotkey).</li>\n"
	                        "<li>Click the point in the image corresponding to this object (hide the simulated stars "
	                            "if they are too confusing at this point).</li></ol>") + "</li>"
	        "<li>" + q_("Finally, when enough points have been picked to fill the different distances from image "
	                    "center, click <i>Optimize</i> button. After a few moments the image should appear aligned "
	                    "with the stars. If it's not, try picking more points from the ones which don't match, and "
	                    "retry optimization.") + "</li>"
	        "<li>" + q_("On the <i>Model for lensfun</i> tab you can generate the XML entry for lensfun database. "
	                    "Enter make and model for the lens, and the mount (the name must match one of the mounts "
	                    "supported by your camera as listed in the database).") + "</li>"
	        "</ol>";

	html += "<h3>" + q_("Preparing a photo") + "</h3>";
	html += q_("To make a useful shot, you should follow some guidelines:\n");
	html += "<ul>"
	         "<li>" + q_("Try to keep away from the horizon, because star elevation there is very dependent on "
	                     "weather conditions due to atmospheric refraction.") + "</li>"
	         "<li>" + q_("Be sure to disable any geometry distortion correction, because failure to disable "
	                    "distortion correction may make good fitting impossible. In particular:") +
	          "<ul>"
	           "<li>" + q_("If you're shooting RAW, lens distortion correction must be turned off in RAW development "
	                     "software like Lightroom or Darktable that you use for processing of your shots.") + "</li>"
	           "<li>" + q_("If you're shooting JPEG, lens distortion correction must be turned off in the camera "
	                     "settings before taking the photo.") + "</li>"
	          "</ul>"
	         "</li>"
	         "<li>" + q_("Disable automatic image rotation according to its orientation saved in EXIF tags. While "
	                     "such rotation may make the image look better, the shifts of the center of projection "
	                     "that will be computed during the fitting process will have wrong directions if such "
	                     "rotation was applied.") + "</li>"
	        "</ul>";
	html += "<p>" + q_("The image that you input into this estimator should contain as much EXIF information from the "
	           "original RAW as possible, this will let the UI tell you if e.g. current time and date set in "
	           "the simulation are not the same as specified in the image.") + "</p>";
	html += "<p>" + q_("One method that will take an input RAW image and output a result following the above "
	                   "guidelines (except those about shooting) is to use <code>dcraw</code> (the original one from "
	                   "Dave Coffin) or <code>dcraw_emu</code> (from <code>libRaw</code> package), plus "
	                   "<code>exiftool</code> as in the following commands (adjust the file names to your needs):") + "</p>"
	        "<pre>dcraw_emu -t 0 -T -W -Z 20230921_235104-dcraw.tiff 20230921_235104.dng\n"
	             "exiftool -tagsfromfile 20230921_235104.jpg -Time:all -DateTimeOriginal -gps:all -MakerNotes "
	             "-wm cg 20230921_235104-dcraw.tiff</pre>"
	        "<p>" + q_("Here the <code>-t 0</code> option for <code>dcraw_emu</code> prevents automatic rotation, "
	                   "<code>-T</code> generates a TIFF file instead of the default PPM, <code>-W</code> prevents "
	                   "automatic brightening which may make stars not as small as they are in the photo, and "
	                   "<code>-Z</code> gives an explicit name to the file.") + "</p>"
	        "<p>" + q_("The <code>exiftool</code> command copies all time-related and GPS tags from the JPEG sidecar "
	                   "(which on my Samsung Galaxy S8 appeared to contain more complete EXIF info than the DNG) into "
	                   "the TIFF. Your case may differ, you may have to copy from your RAW file instead.") + "</p>";

	html += "<h3>" + q_("Hot Keys") + "</h3>";
	html += "<ul>"
	        "<li>" + q_("Ctrl+Shift & left mouse drag: move the image around the sky") + "</li>"
	        "<li>" + q_("Ctrl+Shift & right mouse drag: scale and rotate the image around the last drag point") + "</li>"
	        "<li>" + q_("Keyboard hotkeys can be configured in the Keyboard shortcuts editor (F7).") + "</li>"
	        "</ul>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Lens Distortion Estimator plugin");
	html += "</body></html>";

	ui_->about->setHtml(html);
}
