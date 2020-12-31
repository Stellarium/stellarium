/*
 * Copyright (C) 2009 Matthew Gates
 * Copyright (C) 2014 Georg Zotti
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

#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelVertexArray.hpp"
#include "AngleMeasure.hpp"
#include "AngleMeasureDialog.hpp"

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>
#include <stdexcept>

//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* AngleMeasureStelPluginInterface::getStelModule() const
{
	return new AngleMeasure();
}

StelPluginInfo AngleMeasureStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(AngleMeasure);

	StelPluginInfo info;
	info.id = "AngleMeasure";
	info.displayedName = N_("Angle Measure");
	info.authors = "Matthew Gates, Georg Zotti";
	info.contact = "https://github.com/Stellarium/stellarium";
	info.description = N_("Provides an angle measurement tool");	
	info.version = ANGLEMEASURE_PLUGIN_VERSION;
	info.license = ANGLEMEASURE_PLUGIN_LICENSE;
	return info;
}

AngleMeasure::AngleMeasure()
	: flagShowAngleMeasure(false)
	, withDecimalDegree(false)
	, dragging(false)
	, angleEquatorial(0.)
	, flagUseDmsFormat(false)
	, flagShowEquatorial(false)
	, flagShowHorizontal(false)
	, flagShowEquatorialPA(false)
	, flagShowHorizontalPA(false)
	, flagShowHorizontalStartSkylinked(false)
	, flagShowHorizontalEndSkylinked(false)
	, angleHorizontal(0.)
	, toolbarButton(Q_NULLPTR)
{
	startPoint.set(0.,0.,0.);
	endPoint.set(0.,0.,0.);
	perp1StartPoint.set(0.,0.,0.);
	perp1EndPoint.set(0.,0.,0.);
	perp2StartPoint.set(0.,0.,0.);
	perp2EndPoint.set(0.,0.,0.);
	startPointHor.set(0.,0.,0.);
	endPointHor.set(0.,0.,0.);
	perp1StartPointHor.set(0.,0.,0.);
	perp1EndPointHor.set(0.,0.,0.);
	perp2StartPointHor.set(0.,0.,0.);
	perp2EndPointHor.set(0.,0.,0.);

	setObjectName("AngleMeasure");
	font.setPixelSize(16);

	configDialog = new AngleMeasureDialog();
	conf = StelApp::getInstance().getSettings();

	messageTimer = new QTimer(this);
	messageTimer->setInterval(7000);
	messageTimer->setSingleShot(true);

	connect(messageTimer, SIGNAL(timeout()), this, SLOT(clearMessage()));
}

AngleMeasure::~AngleMeasure()
{
	delete configDialog;
}

bool AngleMeasure::configureGui(bool show)
{
	if (show)
		configDialog->setVisible(true);
	return true;
}

//! Determine which "layer" the plugin's drawing will happen on.
double AngleMeasure::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	if (actionName==StelModule::ActionHandleMouseClicks)
		return -11;
	return 0;
}

void AngleMeasure::init()
{
	if (!conf->childGroups().contains("AngleMeasure"))
		restoreDefaultSettings();

	loadSettings();

	StelApp& app = StelApp::getInstance();

	// Create action for enable/disable & hook up signals	
	addAction("actionShow_Angle_Measure", N_("Angle Measure"), N_("Angle measure"), "enabled", "Ctrl+A");

	// Initialize the message strings and make sure they are translated when
	// the language changes.
	updateMessageText();
	connect(&app, SIGNAL(languageChanged()), this, SLOT(updateMessageText()));

	// Add a toolbar button
	try
	{
		StelGui* gui = dynamic_cast<StelGui*>(app.getGui());
		if (gui!=Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
						       QPixmap(":/angleMeasure/bt_anglemeasure_on.png"),
						       QPixmap(":/angleMeasure/bt_anglemeasure_off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Angle_Measure");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable create toolbar button for AngleMeasure plugin: " << e.what();
	}
}

void AngleMeasure::update(double deltaTime)
{
	messageFader.update(static_cast<int>(deltaTime*1000));
	lineVisible.update(static_cast<int>(deltaTime*1000));
	static StelCore *core=StelApp::getInstance().getCore();

	withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();

	// if altAz endpoint linked to the rotating sky, move respective point(s)
	if (flagShowHorizontalStartSkylinked)
	{
		startPointHor = core->equinoxEquToAltAz(startPoint, StelCore::RefractionAuto);
		calculateEnds();
	}
	if (flagShowHorizontalEndSkylinked)
	{
		endPointHor = core->equinoxEquToAltAz(endPoint, StelCore::RefractionAuto);
		calculateEnds();
	}
}

void AngleMeasure::drawOne(StelCore *core, const StelCore::FrameType frameType, const StelCore::RefractionMode refractionMode, const Vec3f txtColor, const Vec3f lineColor)
{
	const StelProjectorP prj = core->getProjection(frameType, refractionMode);
	StelPainter painter(prj);
	painter.setFont(font);
	painter.setBlending(true);

	if (lineVisible.getInterstate() > 0.000001f)
	{
		Vec3d xy;
		QString displayedText;
		if (frameType==StelCore::FrameEquinoxEqu)
		{
			if (prj->project(perp1EndPoint,xy))
			{
				painter.setColor(txtColor, lineVisible.getInterstate());
				if (flagShowEquatorialPA)
					displayedText = QString("%1 (%2%3)").arg(formatAngleString(angleEquatorial), messagePA, calculatePositionAngle(startPoint, endPoint));
				else
					displayedText = formatAngleString(angleEquatorial);
				painter.drawText(xy[0], xy[1], displayedText, 0, 15, 15);
			}
		}
		else
		{
			if (prj->project(perp1EndPointHor,xy))
			{
				painter.setColor(txtColor, lineVisible.getInterstate());
				if (flagShowHorizontalPA)
					displayedText = QString("%1 (%2%3)").arg(formatAngleString(angleHorizontal), messagePA, calculatePositionAngle(startPointHor, endPointHor));
				else
					displayedText = formatAngleString(angleHorizontal);
				painter.drawText(xy[0], xy[1], displayedText, 0, 15, -5);
			}
		}

		painter.setLineSmooth(true);

		// main line is a great circle
		painter.setColor(lineColor[0], lineColor[1], lineColor[2], lineVisible.getInterstate());
		if (frameType==StelCore::FrameEquinoxEqu)
		{
			painter.drawGreatCircleArc(startPoint, endPoint, Q_NULLPTR);

			// End lines
			painter.drawGreatCircleArc(perp1StartPoint, perp1EndPoint, Q_NULLPTR);
			painter.drawGreatCircleArc(perp2StartPoint, perp2EndPoint, Q_NULLPTR);
		}
		else
		{
			painter.drawGreatCircleArc(startPointHor, endPointHor, Q_NULLPTR);

			// End lines
			painter.drawGreatCircleArc(perp1StartPointHor, perp1EndPointHor, Q_NULLPTR);
			painter.drawGreatCircleArc(perp2StartPointHor, perp2EndPointHor, Q_NULLPTR);
		}

		painter.setLineSmooth(false);
	}
	if (messageFader.getInterstate() > 0.000001f)
	{
		painter.setColor(txtColor[0], txtColor[1], txtColor[2], messageFader.getInterstate());
		int x = 83;
		int y = 120;
		int ls = painter.getFontMetrics().lineSpacing();
		painter.drawText(x, y, messageEnabled);
		y -= ls;
		painter.drawText(x, y, messageLeftButton);
		y -= ls;
		painter.drawText(x, y, messageRightButton);
	}
	painter.setBlending(false);
}

//! Draw any parts on the screen which are for our module
void AngleMeasure::draw(StelCore* core)
{
	if (startPoint.lengthSquared()==0.0) // avoid crash on switch-on, lp:#1455839
		return;
	if (lineVisible.getInterstate() < 0.000001f && messageFader.getInterstate() < 0.000001f)
		return;
	if (flagShowHorizontal)
	{
		drawOne(core, StelCore::FrameAltAz, StelCore::RefractionOff, horizontalTextColor, horizontalLineColor);
	}
	if (flagShowEquatorial)
	{
		drawOne(core, StelCore::FrameEquinoxEqu, StelCore::RefractionAuto, equatorialTextColor, equatorialLineColor);
	}
}

QString AngleMeasure::calculatePositionAngle(const Vec3d p1, const Vec3d p2) const
{
	const double y = cos(p2.latitude())*sin(p2.longitude()-p1.longitude());
	const double x = cos(p1.latitude())*sin(p2.latitude()) - sin(p1.latitude())*cos(p2.latitude())*cos(p2.longitude()-p1.longitude());
	double r = std::atan2(y,x);
	if (r<0)
		r+= 2*M_PI;

	unsigned int d, m;
	double s;
	bool sign;

	if (withDecimalDegree)
	{
		StelUtils::radToDecDeg(r, sign, s);
		if (flagUseDmsFormat)
			return QString("%1d").arg(s, 0, 'f', 5);
		else
			return QString("%1%2").arg(s, 0, 'f', 5).arg(QChar(0x00B0));
	}
	else
	{
		StelUtils::radToDms(r, sign, d, m, s);
		if (flagUseDmsFormat)
			return QString("%1d %2m %3s").arg(d).arg(m).arg(s, 0, 'f', 2);
		else
			return QString("%1%2 %3' %4\"").arg(d).arg(QChar(0x00B0)).arg(m).arg(s, 0, 'f', 2);
	}
}

void AngleMeasure::handleKeys(QKeyEvent* event)
{
	event->setAccepted(false);
}

void AngleMeasure::handleMouseClicks(class QMouseEvent* event)
{
	if (!flagShowAngleMeasure)
	{
		event->setAccepted(false);
		return;
	}

	if (event->type()==QEvent::MouseButtonPress && event->button()==Qt::LeftButton)
	{
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
		if (prj->unProject(event->x(),event->y(),startPoint))
		{ // Nick Fedoseev patch: improve click match
			Vec3d win;
			prj->project(startPoint,win);
			double dx = event->x() - win.v[0];
			double dy = event->y() - win.v[1];
			prj->unProject(event->x()+dx, event->y()+dy, startPoint);
		}
		const StelProjectorP prjHor = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		prjHor->unProject(event->x(),event->y(),startPointHor);

		// first click reset the line... only draw it after we've dragged a little.
		if (!dragging)
		{
			lineVisible = false;
			endPoint = startPoint;
			endPointHor=startPointHor;
		}
		else
			lineVisible = true;

		dragging = true;
		calculateEnds();
		event->setAccepted(true);
		return;
	}
	else if (event->type()==QEvent::MouseButtonRelease && event->button()==Qt::LeftButton)
	{
		dragging = false;
		calculateEnds();
		event->setAccepted(true);
		return;
	}
	else if (event->type()==QEvent::MouseButtonPress && event->button()==Qt::RightButton)
	{
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
		if (prj->unProject(event->x(),event->y(),endPoint))
		{ // Nick Fedoseev patch: improve click match
			Vec3d win;
			prj->project(endPoint,win);
			double dx = event->x() - win.v[0];
			double dy = event->y() - win.v[1];
			prj->unProject(event->x()+dx, event->y()+dy, endPoint);
		}
		const StelProjectorP prjHor = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		prjHor->unProject(event->x(),event->y(),endPointHor);
		calculateEnds();
		event->setAccepted(true);
		return;
	}
	event->setAccepted(false);
}

bool AngleMeasure::handleMouseMoves(int x, int y, Qt::MouseButtons)
{
	if (dragging)
	{
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
		if (prj->unProject(x,y,endPoint))
		{ // Nick Fedoseev patch: improve click match
		   Vec3d win;
		   prj->project(endPoint,win);
		   double dx = x - win.v[0];
		   double dy = y - win.v[1];
		   prj->unProject(x+dx, y+dy, endPoint);
		}
		const StelProjectorP prjHor = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		prjHor->unProject(x,y,endPointHor);
		calculateEnds();
		lineVisible = true;
		return true;
	}
	else
		return false;
}

void AngleMeasure::calculateEnds(void)
{
	if (flagShowEquatorial)
		calculateEndsOneLine(startPoint,    endPoint,    perp1StartPoint,    perp1EndPoint,    perp2StartPoint,    perp2EndPoint,    angleEquatorial);
	if (flagShowHorizontal)
		calculateEndsOneLine(startPointHor, endPointHor, perp1StartPointHor, perp1EndPointHor, perp2StartPointHor, perp2EndPointHor, angleHorizontal);
}

void AngleMeasure::calculateEndsOneLine(const Vec3d &start, const Vec3d &end, Vec3d &perp1Start, Vec3d &perp1End, Vec3d &perp2Start, Vec3d &perp2End, double &angle)
{
	Vec3d v0 = end - start;
	Vec3d v1 = Vec3d(0,0,0) - start;
	Vec3d p = v0 ^ v1;
	p *= 0.08;  // end width
	perp1Start.set(start[0]-p[0],start[1]-p[1],start[2]-p[2]);
	perp1End.set(start[0]+p[0],start[1]+p[1],start[2]+p[2]);

	v1 = Vec3d(0,0,0) - end;
	p = v0 ^ v1;
	p *= 0.08;  // end width
	perp2Start.set(end[0]-p[0],end[1]-p[1],end[2]-p[2]);
	perp2End.set(end[0]+p[0],end[1]+p[1],end[2]+p[2]);

	angle = start.angle(end);
}

QString AngleMeasure::formatAngleString(double angle) const
{
	unsigned int d, m;
	double s;
	bool sign;

	if (withDecimalDegree)
	{
		StelUtils::radToDecDeg(angle, sign, s);

		if (flagUseDmsFormat)
			return QString("%1d").arg(s, 0, 'f', 5);
		else
			return QString("%1%2").arg(s, 0, 'f', 5).arg(QChar(0x00B0));
	}
	else
	{
		StelUtils::radToDms(angle, sign, d, m, s);

		if (flagUseDmsFormat)
			return QString("%1d %2m %3s").arg(d).arg(m).arg(s, 0, 'f', 2);
		else
			return QString("%1%2 %3' %4\"").arg(d).arg(QChar(0x00B0)).arg(m).arg(s, 0, 'f', 2);
	}
}

void AngleMeasure::enableAngleMeasure(bool b)
{
	if (b!=flagShowAngleMeasure)
	{
		flagShowAngleMeasure = b;
		lineVisible = b;
		messageFader = b;
		if (b)
		{
			//qDebug() << "AngleMeasure::enableAngleMeasure starting timer";
			messageTimer->start();
		}
		// Immediate saving of settings
		conf->setValue("AngleMeasure/enable_at_startup", flagShowAngleMeasure);

		emit flagAngleMeasureChanged(b);
	}
}

void AngleMeasure::showEquatorialPA(bool b)
{
	flagShowEquatorialPA = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/show_position_angle", flagShowEquatorialPA);
	emit flagShowEquatorialPAChanged(b);
}

void AngleMeasure::showHorizontalPA(bool b)
{
	flagShowHorizontalPA = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/show_position_angle_horizontal", flagShowHorizontalPA);
	emit flagShowHorizontalPAChanged(b);
}
void AngleMeasure::showEquatorial(bool b)
{
	flagShowEquatorial = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/show_equatorial", flagShowEquatorial);
	emit flagShowEquatorialChanged(b);
}
void AngleMeasure::showHorizontal(bool b)
{
	flagShowHorizontal = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/show_horizontal", flagShowHorizontal);
	emit flagShowHorizontalChanged(b);
}
void AngleMeasure::showHorizontalStartSkylinked(bool b)
{
	flagShowHorizontalStartSkylinked = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/link_horizontal_start_to_sky", flagShowHorizontalStartSkylinked);
	emit flagShowHorizontalStartSkylinkedChanged(b);
}
void AngleMeasure::showHorizontalEndSkylinked(bool b)
{
	flagShowHorizontalEndSkylinked = b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/link_horizontal_end_to_sky", flagShowHorizontalEndSkylinked);
	emit flagShowHorizontalEndSkylinkedChanged(b);
}
void AngleMeasure::useDmsFormat(bool b)
{
	flagUseDmsFormat=b;
	// Immediate saving of settings
	conf->setValue("AngleMeasure/angle_format_dms", flagUseDmsFormat);
	emit dmsFormatChanged(b);
}

void AngleMeasure::setEquatorialTextColor(Vec3f color)
{
	if (equatorialTextColor != color)
	{
		equatorialTextColor = color;
		conf->setValue("AngleMeasure/text_color", color.toStr());
		emit equatorialTextColorChanged(color);
	}
}
void AngleMeasure::setEquatorialLineColor(Vec3f color)
{
	if (equatorialLineColor != color)
	{
		equatorialLineColor = color;
		conf->setValue("AngleMeasure/line_color", color.toStr());
		emit equatorialLineColorChanged(color);
	}
}
void AngleMeasure::setHorizontalTextColor(Vec3f color)
{
	if (horizontalTextColor != color)
	{
		horizontalTextColor = color;
		conf->setValue("AngleMeasure/text_color_horizontal", color.toStr());
		emit horizontalTextColorChanged(color);
	}
}
void AngleMeasure::setHorizontalLineColor(Vec3f color)
{
	if (horizontalLineColor != color)
	{
		horizontalLineColor = color;
		conf->setValue("AngleMeasure/line_color_horizontal", color.toStr());
		emit horizontalLineColorChanged(color);
	}
}

void AngleMeasure::updateMessageText()
{
	// TRANSLATORS: instructions for using the AngleMeasure plugin.
	messageEnabled = q_("The Angle Measure is enabled:");
	// TRANSLATORS: instructions for using the AngleMeasure plugin.
	messageLeftButton = q_("Drag with the left button to measure, left-click to clear.");
	// TRANSLATORS: instructions for using the AngleMeasure plugin.
	messageRightButton = q_("Right-clicking changes the end point only.");
	// TRANSLATORS: PA is abbreviation for phrase "Position Angle"
	messagePA = q_("PA=");
}

void AngleMeasure::clearMessage()
{
	//qDebug() << "AngleMeasure::clearMessage";
	messageFader = false;
}

void AngleMeasure::restoreDefaultSettings()
{
	Q_ASSERT(conf);
	// Remove the old values...
	conf->remove("AngleMeasure");
	// ...load the default values...
	loadSettings();	
	// But this doesn't save the colors, so:
	conf->beginGroup("AngleMeasure");
	conf->setValue("text_color", "0,0.5,1");
	conf->setValue("line_color", "0,0.5,1");
	conf->setValue("text_color_horizontal", "0.9,0.6,0.4");
	conf->setValue("line_color_horizontal", "0.9,0.6,0.4");
	conf->endGroup();
}

void AngleMeasure::loadSettings()
{
	enableAngleMeasure(conf->value("AngleMeasure/enable_at_startup", false).toBool());
	useDmsFormat(conf->value("AngleMeasure/angle_format_dms", false).toBool());
	equatorialTextColor = Vec3f(conf->value("AngleMeasure/text_color", "0,0.5,1").toString());
	equatorialLineColor = Vec3f(conf->value("AngleMeasure/line_color", "0,0.5,1").toString());
	horizontalTextColor = Vec3f(conf->value("AngleMeasure/text_color_horizontal", "0.9,0.6,0.4").toString());
	horizontalLineColor = Vec3f(conf->value("AngleMeasure/line_color_horizontal", "0.9,0.6,0.4").toString());
	showEquatorialPA(conf->value("AngleMeasure/show_position_angle", false).toBool());
	showHorizontalPA(conf->value("AngleMeasure/show_position_angle_horizontal", false).toBool());
	showEquatorial(conf->value("AngleMeasure/show_equatorial", true).toBool());
	showHorizontal(conf->value("AngleMeasure/show_horizontal", false).toBool());
	showHorizontalStartSkylinked(conf->value("AngleMeasure/link_horizontal_start_to_sky", false).toBool());
	showHorizontalEndSkylinked(conf->value("AngleMeasure/link_horizontal_end_to_sky", false).toBool());
}
