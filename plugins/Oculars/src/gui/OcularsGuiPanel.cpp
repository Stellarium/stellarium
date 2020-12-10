/*
Oculars plug-in for Stellarium: graphical user interface widget
Copyright (C) 2011  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "Oculars.hpp"
#include "OcularsGuiPanel.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelTranslator.hpp"
#include "StelActionMgr.hpp"

#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QWidget>

OcularsGuiPanel::OcularsGuiPanel(Oculars* plugin,
				 QGraphicsWidget *parent,
				 Qt::WindowFlags wFlags):
	QGraphicsWidget(parent, wFlags),
	ocularsPlugin(plugin),
	parentWidget(parent),
	borderPath(Q_NULLPTR)
{
	setContentsMargins(0, 0, 0, 0);

	//First create the layout and populate it, then set it?
	mainLayout = new QGraphicsLinearLayout(Qt::Vertical);

	//Button bar
	buttonBar = new QGraphicsWidget();
	mainLayout->addItem(buttonBar);

	StelApp& stelApp = StelApp::getInstance();
	Q_ASSERT(ocularsPlugin->actionShowOcular);
	buttonOcular = new StelButton(buttonBar,
				      QPixmap(":/ocular/bt_ocular_on.png"),
				      QPixmap(":/ocular/bt_ocular_off.png"),
				      QPixmap(),
				      ocularsPlugin->actionShowOcular,
				      true); //No background
	buttonOcular->setToolTip(ocularsPlugin->actionShowOcular->getText());

	Q_ASSERT(ocularsPlugin->actionShowCrosshairs);
	buttonCrosshairs = new StelButton(buttonBar,
					  QPixmap(":/ocular/bt_crosshairs_on.png"),
					  QPixmap(":/ocular/bt_crosshairs_off.png"),
					  QPixmap(),
					  ocularsPlugin->actionShowCrosshairs,
					  true);
	buttonCrosshairs->setToolTip(ocularsPlugin->actionShowCrosshairs->getText());
	buttonCrosshairs->setVisible(false);

	Q_ASSERT(ocularsPlugin->actionShowSensor);
	buttonCcd = new StelButton(buttonBar,
				   QPixmap(":/ocular/bt_sensor_on.png"),
				   QPixmap(":/ocular/bt_sensor_off.png"),
				   QPixmap(),
				   ocularsPlugin->actionShowSensor,
				   true);
	buttonCcd->setToolTip(ocularsPlugin->actionShowSensor->getText());

	Q_ASSERT(ocularsPlugin->actionShowTelrad);
	buttonTelrad = new StelButton(buttonBar,
				      QPixmap(":/ocular/bt_telrad_on.png"),
				      QPixmap(":/ocular/bt_telrad_off.png"),
				      QPixmap(),
				      ocularsPlugin->actionShowTelrad,
				      true);
	buttonTelrad->setToolTip(ocularsPlugin->actionShowTelrad->getText());

	Q_ASSERT(ocularsPlugin->actionConfiguration);
	buttonConfiguration = new StelButton(buttonBar,
					     QPixmap(":/ocular/bt_settings_on.png"),
					     QPixmap(":/ocular/bt_settings_off.png"),
					     QPixmap(),
					     ocularsPlugin->actionConfiguration,
					     true);
	buttonConfiguration->setToolTip(ocularsPlugin->actionConfiguration->getText());

	qreal buttonHeight = buttonOcular->boundingRect().height();
	buttonBar->setMinimumHeight(buttonHeight);
	buttonBar->setMaximumHeight(buttonHeight);

	setLayout(mainLayout);

	//Widgets with control and information fields
	ocularControls = new QGraphicsWidget(this);
	ocularControls->setVisible(false);
	lensControls = new QGraphicsWidget(this);
	lensControls->setVisible(false);
	ccdControls = new QGraphicsWidget(this);
	ccdControls->setVisible(false);
	telescopeControls = new QGraphicsWidget(this);
	telescopeControls->setVisible(false);

	fieldOcularName = new QGraphicsTextItem(ocularControls);
	fieldOcularFl = new QGraphicsTextItem(ocularControls);
	fieldOcularAfov = new QGraphicsTextItem(ocularControls);
	fieldCcdName = new QGraphicsTextItem(ccdControls);
	fieldCcdDimensions = new QGraphicsTextItem(ccdControls);
	fieldCcdBinning = new QGraphicsTextItem(ccdControls);
	fieldCcdHScale = new QGraphicsTextItem(ccdControls);
	fieldCcdVScale = new QGraphicsTextItem(ccdControls);
	fieldCcdRotation = new QGraphicsTextItem(ccdControls);
	fieldPrismRotation = new QGraphicsTextItem(ccdControls);
	fieldTelescopeName = new QGraphicsTextItem(telescopeControls);
	fieldMagnification = new QGraphicsTextItem(telescopeControls);
	fieldExitPupil = new QGraphicsTextItem(telescopeControls);
	fieldFov = new QGraphicsTextItem(telescopeControls);
	fieldRayleighCriterion = new QGraphicsTextItem(telescopeControls);
	fieldDawesCriterion = new QGraphicsTextItem(telescopeControls);
	fieldAbbeyCriterion = new QGraphicsTextItem(telescopeControls);
	fieldSparrowCriterion = new QGraphicsTextItem(telescopeControls);
	fieldVisualResolution = new QGraphicsTextItem(telescopeControls);

	fieldLensName = new QGraphicsTextItem(lensControls);
	fieldLensMultipler = new QGraphicsTextItem(lensControls);

	QFont newFont = font();
	newFont.setPixelSize(plugin->getGuiPanelFontSize());
	setControlsFont(newFont);

	//Traditional field width from Ocular ;)
	QFontMetrics fm(fieldOcularName->font());
	int maxWidth = fm.boundingRect(QString("MMMMMMMMMMMMMMMMMMM")).width();
	int lineHeight = fm.height();

	fieldOcularName->setTextWidth(maxWidth);
	fieldOcularFl->setTextWidth(maxWidth);
	fieldOcularAfov->setTextWidth(maxWidth);
	fieldCcdName->setTextWidth(maxWidth);
	fieldCcdDimensions->setTextWidth(maxWidth);
	fieldCcdBinning->setTextWidth(maxWidth);
	fieldCcdHScale->setTextWidth(maxWidth);
	fieldCcdVScale->setTextWidth(maxWidth);
	fieldCcdRotation->setTextWidth(maxWidth);
	fieldPrismRotation->setTextWidth(maxWidth);
	fieldTelescopeName->setTextWidth(maxWidth);
	fieldMagnification->setTextWidth(maxWidth);
	fieldExitPupil->setTextWidth(maxWidth);
	fieldFov->setTextWidth(maxWidth);
	fieldRayleighCriterion->setTextWidth(maxWidth);
	fieldDawesCriterion->setTextWidth(maxWidth);
	fieldAbbeyCriterion->setTextWidth(maxWidth);
	fieldSparrowCriterion->setTextWidth(maxWidth);
	fieldVisualResolution->setTextWidth(maxWidth);
	fieldLensName->setTextWidth(maxWidth);
	fieldLensMultipler->setTextWidth(maxWidth);

	// Retrieve value from setting directly, because at this stage the plugin has not parsed it yet.
	// TODO: Remove this conversion tool in version 0.21 or 0.22
	float scv = plugin->getSettings()->value("arrow_scale", 150.f).toFloat();
	if (scv<100.f) // convert old value and type
		scv *= 100.f;
	int scale=static_cast<int>(lineHeight*scv*0.01f);
	// TODO: change this load-once to interactively editable value of scaling coefficient
	QPixmap pa(":/graphicGui/btTimeRewind-on.png");
	QPixmap prevArrow = pa.scaledToHeight(scale, Qt::SmoothTransformation);
	QPixmap paOff(":/graphicGui/btTimeRewind-off.png");
	QPixmap prevArrowOff = paOff.scaledToHeight(scale, Qt::SmoothTransformation);
	QPixmap na(":/graphicGui/btTimeForward-on.png");
	QPixmap nextArrow = na.scaledToHeight(scale, Qt::SmoothTransformation);
	QPixmap naOff(":/graphicGui/btTimeForward-off.png");
	QPixmap nextArrowOff = naOff.scaledToHeight(scale, Qt::SmoothTransformation);

	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	QString ocularsGroup = N_("Oculars"); // Possible group name: Oculars on-screen control panel
	actionMgr->addAction("actionToggle_Oculars_Previous_Ocular", ocularsGroup, N_("Previous ocular"), this, "updateOcularControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Next_Ocular", ocularsGroup, N_("Next ocular"), this, "updateOcularControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Previous_Lens", ocularsGroup, N_("Previous lens"), this, "updateLensControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Next_Lens", ocularsGroup, N_("Next lens"), this, "updateLensControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Previous_CCD", ocularsGroup, N_("Previous CCD frame"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Next_CCD", ocularsGroup, N_("Next CCD frame"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Previous_Telescope", ocularsGroup, N_("Previous telescope"), this, "updateTelescopeControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Next_Telescope", ocularsGroup, N_("Next telescope"), this, "updateTelescopeControls()", "", "");

	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_Reset", ocularsGroup, N_("Reset the sensor frame rotation"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_15_Counterclockwise", ocularsGroup, N_("Rotate the sensor frame 15 degrees counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_5_Counterclockwise", ocularsGroup, N_("Rotate the sensor frame 5 degrees counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_1_Counterclockwise", ocularsGroup, N_("Rotate the sensor frame 1 degree counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_15_Clockwise", ocularsGroup, N_("Rotate the sensor frame 15 degrees clockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_5_Clockwise", ocularsGroup, N_("Rotate the sensor frame 5 degrees clockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Frame_1_Clockwise", ocularsGroup, N_("Rotate the sensor frame 1 degree clockwise"), this, "updateCcdControls()", "", "");

	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_Reset", ocularsGroup, N_("Reset the prism rotation"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_15_Counterclockwise", ocularsGroup, N_("Rotate the prism 15 degrees counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_5_Counterclockwise", ocularsGroup, N_("Rotate the prism 5 degrees counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_1_Counterclockwise", ocularsGroup, N_("Rotate the prism 1 degree counterclockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_15_Clockwise", ocularsGroup, N_("Rotate the prism 15 degrees clockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_5_Clockwise", ocularsGroup, N_("Rotate the prism 5 degrees clockwise"), this, "updateCcdControls()", "", "");
	actionMgr->addAction("actionToggle_Oculars_Rotate_Prism_1_Clockwise", ocularsGroup, N_("Rotate the prism 1 degree clockwise"), this, "updateCcdControls()", "", "");

	prevOcularButton = new StelButton(ocularControls, prevArrow, prevArrowOff, QPixmap(), "actionToggle_Oculars_Previous_Ocular");
	prevOcularButton->setToolTip(q_("Previous ocular"));
	nextOcularButton = new StelButton(ocularControls, nextArrow, nextArrowOff, QPixmap(), "actionToggle_Oculars_Next_Ocular");
	nextOcularButton->setToolTip(q_("Next ocular"));
	prevLensButton = new StelButton(lensControls, prevArrow, prevArrowOff, QPixmap(), "actionToggle_Oculars_Previous_Lens");
	prevLensButton->setToolTip(q_("Previous lens"));
	nextLensButton = new StelButton(lensControls, nextArrow, nextArrowOff, QPixmap(), "actionToggle_Oculars_Next_Lens");
	nextLensButton->setToolTip(q_("Next lens"));
	prevCcdButton = new StelButton(ccdControls, prevArrow, prevArrowOff, QPixmap(), "actionToggle_Oculars_Previous_CCD");
	prevCcdButton->setToolTip(q_("Previous CCD frame"));
	nextCcdButton = new StelButton(ccdControls, nextArrow, nextArrowOff, QPixmap(), "actionToggle_Oculars_Next_CCD");
	nextCcdButton->setToolTip(q_("Next CCD frame"));
	prevTelescopeButton = new StelButton(telescopeControls, prevArrow, prevArrowOff, QPixmap(), "actionToggle_Oculars_Previous_Telescope");
	prevTelescopeButton->setToolTip(q_("Previous telescope"));
	nextTelescopeButton = new StelButton(telescopeControls, nextArrow, nextArrowOff, QPixmap(), "actionToggle_Oculars_Next_Telescope");
	nextTelescopeButton->setToolTip(q_("Next telescope"));

	connect(prevOcularButton,    SIGNAL(triggered()), ocularsPlugin, SLOT(decrementOcularIndex()));
	connect(nextOcularButton,    SIGNAL(triggered()), ocularsPlugin, SLOT(incrementOcularIndex()));
	connect(prevTelescopeButton, SIGNAL(triggered()), ocularsPlugin, SLOT(decrementTelescopeIndex()));
	connect(nextTelescopeButton, SIGNAL(triggered()), ocularsPlugin, SLOT(incrementTelescopeIndex()));
	connect(prevCcdButton,       SIGNAL(triggered()), ocularsPlugin, SLOT(decrementCCDIndex()));
	connect(nextCcdButton,       SIGNAL(triggered()), ocularsPlugin, SLOT(incrementCCDIndex()));
	connect(nextLensButton,      SIGNAL(triggered()), ocularsPlugin, SLOT(incrementLensIndex()));
	connect(prevLensButton,      SIGNAL(triggered()), ocularsPlugin, SLOT(decrementLensIndex()));

	QColor cOn(255, 255, 255);
	QColor cOff(102, 102, 102);
	QColor cHover(162, 162, 162);
	QString degrees = QString("-15%1").arg(QChar(0x00B0));
	int degreesW = fm.boundingRect(degrees).width();
	QPixmap pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	QPixmap pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	QPixmap pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdMinus15Button = new StelButton(ccdControls, pOn,  pOff, pHover, "actionToggle_Oculars_Rotate_Frame_15_Counterclockwise", true);
	rotateCcdMinus15Button->setToolTip(q_("Rotate the sensor frame 15 degrees counterclockwise"));
	rotatePrismMinus15Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_15_Counterclockwise", true);
	rotatePrismMinus15Button->setToolTip(q_("Rotate the prism 15 degrees counterclockwise"));

	degrees  = QString("-5%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdMinus5Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_5_Counterclockwise", true);
	rotateCcdMinus5Button->setToolTip(q_("Rotate the sensor frame 5 degrees counterclockwise"));
	rotatePrismMinus5Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_5_Counterclockwise", true);
	rotatePrismMinus5Button->setToolTip(q_("Rotate the prism 5 degrees counterclockwise"));

	degrees  = QString("-1%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdMinus1Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_1_Counterclockwise", true);
	rotateCcdMinus1Button->setToolTip(q_("Rotate the sensor frame 1 degree counterclockwise"));
	rotatePrismMinus1Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_1_Counterclockwise", true);
	rotatePrismMinus1Button->setToolTip(q_("Rotate the prism 1 degree counterclockwise"));

	degrees  = QString("0%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	resetCcdRotationButton = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_Reset", true);
	resetCcdRotationButton->setToolTip(q_("Reset the sensor frame rotation"));
	resetPrismRotationButton = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_Reset", true);
	resetPrismRotationButton->setToolTip(q_("Reset the prism rotation"));

	degrees  = QString("+1%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdPlus1Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_1_Clockwise", true);
	rotateCcdPlus1Button->setToolTip(q_("Rotate the sensor frame 1 degree clockwise"));
	rotatePrismPlus1Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_1_Clockwise", true);
	rotatePrismPlus1Button->setToolTip(q_("Rotate the prism 1 degree clockwise"));

	degrees  = QString("+5%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdPlus5Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_5_Clockwise", true);
	rotateCcdPlus5Button->setToolTip(q_("Rotate the sensor frame 5 degrees clockwise"));
	rotatePrismPlus5Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_5_Clockwise", true);
	rotatePrismPlus5Button->setToolTip(q_("Rotate the prism 5 degrees clockwise"));

	degrees  = QString("+15%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdPlus15Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_15_Clockwise", true);
	rotateCcdPlus15Button->setToolTip(q_("Rotate the sensor frame 15 degrees clockwise"));
	rotatePrismPlus15Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_15_Clockwise", true);
	rotatePrismPlus15Button->setToolTip(q_("Rotate the prism 15 degrees clockwise"));

	connect(rotateCcdMinus15Button, &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(-15);});
	connect(rotateCcdMinus5Button,  &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(-5);});
	connect(rotateCcdMinus1Button,  &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(-1);});
	connect(rotateCcdPlus1Button,   &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(1);});
	connect(rotateCcdPlus5Button,   &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(5);});
	connect(rotateCcdPlus15Button,  &StelButton::triggered, [=](){ocularsPlugin->rotateCCD(15);});
	connect(resetCcdRotationButton, SIGNAL(triggered()),ocularsPlugin, SLOT(ccdRotationReset()));

	connect(rotateCcdMinus15Button, SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotateCcdMinus5Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotateCcdMinus1Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotateCcdPlus1Button,   SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotateCcdPlus5Button,   SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotateCcdPlus15Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(resetCcdRotationButton, SIGNAL(triggered()), this, SLOT(updateCcdControls()));

	connect(rotatePrismMinus15Button, &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(-15);});
	connect(rotatePrismMinus5Button,  &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(-5);});
	connect(rotatePrismMinus1Button,  &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(-1);});
	connect(rotatePrismPlus1Button,   &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(1);});
	connect(rotatePrismPlus5Button,   &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(5);});
	connect(rotatePrismPlus15Button,  &StelButton::triggered, [=](){ocularsPlugin->rotatePrism(15);});
	connect(resetPrismRotationButton, SIGNAL(triggered()),ocularsPlugin, SLOT(prismPositionAngleReset()));

	connect(rotatePrismMinus15Button, SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotatePrismMinus5Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotatePrismMinus1Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotatePrismPlus1Button,   SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotatePrismPlus5Button,   SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(rotatePrismPlus15Button,  SIGNAL(triggered()), this, SLOT(updateCcdControls()));
	connect(resetPrismRotationButton, SIGNAL(triggered()), this, SLOT(updateCcdControls()));

	//Set the layout and update the size
	qreal width = 2*prevOcularButton->boundingRect().width() + maxWidth;
	qreal left, right, top, bottom;
	mainLayout->getContentsMargins(&left, &top, &right, &bottom);
	ocularControls->setMaximumWidth(width);
	ccdControls->setMaximumWidth(width);
	telescopeControls->setMaximumWidth(width);
	lensControls->setMaximumWidth(width);
	resize(width + left + right, 10);
	buttonBar->resize(width, size().height());
	updateMainButtonsPositions();


	//Border/background for the widget
	borderPath = new QGraphicsPathItem();
	borderPath->setZValue(100);
	QBrush borderBrush(QColor::fromRgbF(0.22, 0.22, 0.23, 0.2));
	borderPath->setBrush(borderBrush);
	QPen borderPen = QPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	borderPen.setWidthF(1.);
	borderPath->setPen(borderPen);
	borderPath->setParentItem(parentWidget);

	updatePosition();
	connect (parentWidget, SIGNAL(geometryChanged()),	 this, SLOT(updatePosition()));

	//Connecting other slots
	connect(ocularsPlugin, SIGNAL(selectedOcularChanged(int)),    this, SLOT(updateOcularControls()));
	connect(ocularsPlugin, SIGNAL(selectedCCDChanged(int)),       this, SLOT(updateCcdControls()));
	connect(ocularsPlugin, SIGNAL(selectedTelescopeChanged(int)), this, SLOT(updateTelescopeControls()));
	connect(ocularsPlugin, SIGNAL(selectedLensChanged(int)),      this, SLOT(updateTelescopeControls()));

	//Night mode
	connect(&stelApp, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setColorScheme(const QString&)));
	setColorScheme(stelApp.getCurrentStelStyle());
}

OcularsGuiPanel::~OcularsGuiPanel()
{
	if (borderPath)
		delete borderPath;

	delete buttonCrosshairs; buttonCrosshairs = Q_NULLPTR;
	delete buttonCcd; buttonCcd = Q_NULLPTR;
	delete buttonTelrad;	buttonTelrad = Q_NULLPTR;
	delete buttonConfiguration; buttonConfiguration = Q_NULLPTR;
	delete fieldOcularFl; fieldOcularFl = Q_NULLPTR;
	delete fieldOcularAfov; fieldOcularAfov = Q_NULLPTR;
	delete fieldCcdName; fieldCcdName = Q_NULLPTR;
	delete fieldCcdDimensions; fieldCcdDimensions = Q_NULLPTR;
	delete fieldCcdBinning; fieldCcdBinning = Q_NULLPTR;
	delete fieldCcdHScale; fieldCcdHScale = Q_NULLPTR;
	delete fieldCcdVScale; fieldCcdVScale = Q_NULLPTR;
	delete fieldCcdRotation; fieldCcdRotation = Q_NULLPTR;
	delete fieldPrismRotation; fieldPrismRotation = Q_NULLPTR;
	delete fieldTelescopeName; fieldTelescopeName = Q_NULLPTR;
	delete fieldMagnification; fieldMagnification = Q_NULLPTR;
	delete fieldExitPupil; fieldExitPupil = Q_NULLPTR;
	delete fieldFov; fieldFov = Q_NULLPTR;
	delete fieldRayleighCriterion; fieldRayleighCriterion = Q_NULLPTR;
	delete fieldDawesCriterion; fieldDawesCriterion = Q_NULLPTR;
	delete fieldAbbeyCriterion; fieldAbbeyCriterion = Q_NULLPTR;
	delete fieldSparrowCriterion; fieldSparrowCriterion = Q_NULLPTR;
	delete fieldVisualResolution; fieldVisualResolution = Q_NULLPTR;
	delete fieldLensName; fieldLensName = Q_NULLPTR;
	delete fieldLensMultipler; fieldLensMultipler = Q_NULLPTR;
}

void OcularsGuiPanel::showOcularGui()
{
	setPreferredHeight(0);//WTF?
	if (ocularsPlugin->flagShowOculars)
	{
		updateOcularControls();
	}
	else
	{
		setOcularControlsVisible(false);
		setTelescopeControlsVisible(false);
		setLensControlsVisible(false);
		updatePosition();
	}
}

void OcularsGuiPanel::showCcdGui()
{
	updateCcdControls();
}

void OcularsGuiPanel::foldGui()
{
	//qDebug() << "hidePanel()";
	setOcularControlsVisible(false);
	setCcdControlsVisible(false);
	setTelescopeControlsVisible(false);
	setLensControlsVisible(false);
	updatePosition();
}

void OcularsGuiPanel::updatePosition()
{
	updateGeometry();
	/*qDebug() << "Widget:" << size()
		<< "Buttonbar:" << buttonBar->size()
		<< "Ocular" << ocularControls->size()
		<< "CCD" << ccdControls->size()
		<< "Telescope" << telescopeControls->size()
		<< "Layout" << mainLayout->geometry();*/
	qreal xPos = parentWidget->size().width() - size().width();
	qreal yPos = 0;
	setPos(xPos, yPos);

	//Update border/shading
	QPainterPath newBorderPath;
	double cornerRadius = 12.0;
	QPointF verticalBorderStart = geometry().topLeft();
	QPointF horizontalBorderEnd = geometry().bottomRight();
	QPointF cornerArcStart(verticalBorderStart.x(),
			       horizontalBorderEnd.y() - cornerRadius);
	newBorderPath.moveTo(verticalBorderStart);
	newBorderPath.lineTo(cornerArcStart);
	newBorderPath.arcTo(cornerArcStart.x(), cornerArcStart.y(), cornerRadius, cornerRadius, 180, 90);
	newBorderPath.lineTo(horizontalBorderEnd);
	newBorderPath.lineTo(horizontalBorderEnd.x(), verticalBorderStart.y());
	borderPath->setPath(newBorderPath);
}

void OcularsGuiPanel::updateOcularControls()
{
	setCcdControlsVisible(false);

	//Get the name
	int index = ocularsPlugin->selectedOcularIndex;
	Ocular* ocular = ocularsPlugin->oculars[index];
	Q_ASSERT(ocular);
	QString name = ocular->name();
	QString fullName;
	QString ocularI18n = q_("Ocular");
	if (ocular->isBinoculars())
		ocularI18n = q_("Binocular");
	if (name.isEmpty())
	{
		fullName = QString("%1 #%2").arg(ocularI18n).arg(index);
	}
	else
	{
		fullName = QString("%1 #%2: %3").arg(ocularI18n).arg(index).arg(name);
	}
	fieldOcularName->setPlainText(fullName);

	qreal posX = 0.;
	qreal posY = 0.;
	qreal widgetWidth = 0.;
	qreal widgetHeight = 0.;

	//Prev button
	qreal heightAdjustment = (fieldOcularName->boundingRect().height() - prevOcularButton->boundingRect().height()) / 2.;
	prevOcularButton->setPos(posX, qRound(posY + heightAdjustment));
	posX += prevOcularButton->boundingRect().width();
	widgetWidth += prevOcularButton->boundingRect().width();

	//Name field
	fieldOcularName->setPos(posX, posY);
	posX += fieldOcularName->boundingRect().width();
	widgetWidth += fieldOcularName->boundingRect().width();

	//Next button
	nextOcularButton->setPos(posX, posY + heightAdjustment);
	widgetWidth += nextOcularButton->boundingRect().width();

	posX = prevOcularButton->boundingRect().width();
	posY += fieldOcularName->boundingRect().height();
	widgetHeight += fieldOcularName->boundingRect().height();

	buttonCrosshairs->setEnabled(!ocular->hasPermanentCrosshair());
	if (ocular->isBinoculars())
	{
		fieldOcularFl->setVisible(false);
		fieldOcularAfov->setVisible(false);
	}
	else
	{
		QString focalLengthString = QString(q_("Ocular FL: %1 mm")).arg(QString::number(ocular->effectiveFocalLength(), 'f', 1));
		fieldOcularFl->setPlainText(focalLengthString);
		fieldOcularFl->setToolTip(q_("Effective focal length of the ocular"));
		fieldOcularFl->setPos(posX, posY);
		posY += fieldOcularFl->boundingRect().height();
		widgetHeight += fieldOcularFl->boundingRect().height();

		QString apparentFovString = QString::number(ocular->apparentFOV(), 'f', 2);
		apparentFovString.append(QChar(0x00B0));// Degree sign
		QString apparentFovLabel = QString(q_("Ocular aFOV: %1"))
				.arg(apparentFovString);
		fieldOcularAfov->setPlainText(apparentFovLabel);
		fieldOcularAfov->setToolTip(q_("Apparent field of view of the ocular"));
		fieldOcularAfov->setPos(posX, posY);
		widgetHeight += fieldOcularAfov->boundingRect().height();

		fieldOcularFl->setVisible(true);
		fieldOcularAfov->setVisible(true);
	}

	ocularControls->setMinimumSize(widgetWidth, widgetHeight);
	ocularControls->resize(widgetWidth, widgetHeight);
	setOcularControlsVisible(true);

	updateTelescopeControls();//Contains a call to updatePosition()
}

void OcularsGuiPanel::updateLensControls()
{
	Lens* lens = ocularsPlugin->selectedLens();
	int index = ocularsPlugin->selectedLensIndex;

	QString fullName;
	QString multiplerString;
	if (lens != Q_NULLPTR)
	{
		QString name = lens->getName();
		if (name.isEmpty())
		{
			fullName = QString(q_("Lens #%1")).arg(index);
		}
		else
		{
			fullName = QString(q_("Lens #%1: %2")).arg(index).arg(name);
		}
		multiplerString = QString(q_("Multiplicity: %1")).arg(lens->getMultipler());
		multiplerString.append(QChar(0x02E3)); // Was 0x00D7
	}
	else
	{
		fullName = QString(q_("Lens: None"));
		multiplerString = QString(q_("Multiplicity: N/A"));
	}
	fieldLensName->setPlainText(fullName);
	fieldLensMultipler->setPlainText(multiplerString);
	fieldOcularFl->setToolTip(q_("Focal length of eyepiece"));

	qreal posX = 0.;
	qreal posY = 0.;
	qreal widgetWidth = 0.;
	qreal widgetHeight = 0.;

	//Prev button
	qreal heightAdjustment = (fieldLensName->boundingRect().height() - prevLensButton->boundingRect().height()) / 2.;
	prevLensButton->setPos(posX, qRound(posY + heightAdjustment));
	posX += prevLensButton->boundingRect().width();
	widgetWidth += prevLensButton->boundingRect().width();

	//Name field
	fieldLensName->setPos(posX, posY);
	posX += fieldLensName->boundingRect().width();
	widgetWidth += fieldLensName->boundingRect().width();
	widgetHeight += fieldLensName->boundingRect().height();

	//Next button
	nextLensButton->setPos(posX, posY + heightAdjustment);
	widgetWidth += nextLensButton->boundingRect().width();

	posX = prevLensButton->boundingRect().width();
	posY += fieldLensName->boundingRect().height();
	fieldLensMultipler->setPos(posX, posY);
	widgetHeight += fieldLensMultipler->boundingRect().height();

	lensControls->setMinimumSize(widgetWidth, widgetHeight);
	lensControls->resize(widgetWidth, widgetHeight);

	int oindex = ocularsPlugin->selectedOcularIndex;
	Ocular* ocular = ocularsPlugin->oculars[oindex];
	if (ocular->isBinoculars() && ocularsPlugin->flagShowOculars) // Hide the lens info for binoculars in eyepiece mode only
		setLensControlsVisible(false);
	else
		setLensControlsVisible(true);
}

void OcularsGuiPanel::updateCcdControls()
{
	setOcularControlsVisible(false);

	//Get the name
	int index = ocularsPlugin->selectedCCDIndex;
	CCD* ccd = ocularsPlugin->ccds[index];	
	Q_ASSERT(ccd);
	ocularsPlugin->setSelectedCCDRotationAngle(ccd->chipRotAngle());
	ocularsPlugin->setSelectedCCDPrismPositionAngle(ccd->prismPosAngle());
	QString name = ccd->name();
	QString fullName;
	if (name.isEmpty())
	{
		fullName = QString(q_("Sensor #%1")).arg(index);
	}
	else
	{
		fullName = QString(q_("Sensor #%1: %2")).arg(index).arg(name);
	}
	fieldCcdName->setPlainText(fullName);

	Lens *lens = ocularsPlugin->selectedLens();

	qreal posX = 0.;
	qreal posY = 0.;
	qreal widgetWidth = 0.;
	qreal widgetHeight = 0.;

	//Prev button
	qreal heightAdjustment = (fieldCcdName->boundingRect().height() - prevCcdButton->boundingRect().height()) / 2.;
	prevCcdButton->setPos(posX, posY + heightAdjustment);
	posX += prevCcdButton->boundingRect().width();
	widgetWidth += prevCcdButton->boundingRect().width();

	//Name field
	fieldCcdName->setPos(posX, posY);
	posX += fieldCcdName->boundingRect().width();
	widgetWidth += fieldCcdName->boundingRect().width();

	//Next button
	nextCcdButton->setPos(posX, posY + heightAdjustment);
	widgetWidth += nextCcdButton->boundingRect().width();

	posX = prevCcdButton->boundingRect().width();
	posY = fieldCcdName->boundingRect().height();
	widgetHeight += fieldCcdName->boundingRect().height();

	//We need the current telescope
	index = ocularsPlugin->selectedTelescopeIndex;
	Telescope* telescope = ocularsPlugin->telescopes[index];
	Q_ASSERT(telescope);
	const double fovX = ccd->getActualFOVx(telescope, lens);
	const double fovY = ccd->getActualFOVy(telescope, lens);
	QString dimensionsLabel = QString(q_("Dimensions: %1")).arg(ocularsPlugin->getDimensionsString(fovX, fovY));
	fieldCcdDimensions->setPlainText(dimensionsLabel);
	fieldCcdDimensions->setToolTip(q_("Dimensions field of view"));
	fieldCcdDimensions->setPos(posX, posY);
	posY += fieldCcdDimensions->boundingRect().height();
	widgetHeight += fieldCcdDimensions->boundingRect().height();	
	QString binningLabel = QString("%1: %2 %4 %3").arg(q_("Binning")).arg(ccd->binningX()).arg(ccd->binningY()).arg(QChar(0x00D7));
	fieldCcdBinning->setPlainText(binningLabel);
	fieldCcdBinning->setPos(posX, posY);
	posY += fieldCcdBinning->boundingRect().height();
	widgetHeight += fieldCcdBinning->boundingRect().height();
	//TRANSLATORS: Unit of measure for scale - arc-seconds per pixel
	QString unit = q_("\"/px");
	fieldCcdHScale->setPlainText(QString("%1: %2%3").arg(q_("X scale"), QString::number(fovX*3600*ccd->binningX()/ccd->resolutionX(), 'f', 4), unit));
	fieldCcdHScale->setToolTip(q_("Horizontal scale"));
	fieldCcdHScale->setPos(posX, posY);
	posY += fieldCcdHScale->boundingRect().height();
	widgetHeight += fieldCcdHScale->boundingRect().height();
	fieldCcdVScale->setPlainText(QString("%1: %2%3").arg(q_("Y scale"), QString::number(fovY*3600*ccd->binningY()/ccd->resolutionY(), 'f', 4), unit));
	fieldCcdVScale->setToolTip(q_("Vertical scale"));
	fieldCcdVScale->setPos(posX, posY);
	posY += fieldCcdVScale->boundingRect().height();
	widgetHeight += fieldCcdVScale->boundingRect().height();
	QString rotation = QString::number(ocularsPlugin->getSelectedCCDRotationAngle(), 'f', 0);
	rotation.append(QChar(0x00B0));
	QString rotationLabel = QString(q_("Rotation: %1")).arg(rotation);
	fieldCcdRotation->setPlainText(rotationLabel);
	fieldCcdRotation->setPos(posX, posY);
	posY += fieldCcdRotation->boundingRect().height();
	widgetHeight += fieldCcdRotation->boundingRect().height();

	int rotationButtonsWidth = rotateCcdMinus15Button->boundingRect().width();
	rotationButtonsWidth += rotateCcdMinus5Button->boundingRect().width();
	rotationButtonsWidth += rotateCcdMinus1Button->boundingRect().width();
	rotationButtonsWidth += resetCcdRotationButton->boundingRect().width();
	rotationButtonsWidth += rotateCcdPlus1Button->boundingRect().width();
	rotationButtonsWidth += rotateCcdPlus5Button->boundingRect().width();
	rotationButtonsWidth += rotateCcdPlus15Button->boundingRect().width();
	int spacing = (fieldCcdRotation->boundingRect().width() - rotationButtonsWidth) / 6;
	posX = fieldCcdRotation->x();
	rotateCcdMinus15Button->setPos(posX, posY);
	posX += rotateCcdMinus15Button->boundingRect().width() + spacing;
	rotateCcdMinus5Button->setPos(posX, posY);
	posX += rotateCcdMinus5Button->boundingRect().width() + spacing;
	rotateCcdMinus1Button->setPos(posX, posY);
	posX += rotateCcdMinus1Button->boundingRect().width() + spacing;
	resetCcdRotationButton->setPos(posX, posY);
	posX += resetCcdRotationButton->boundingRect().width() + spacing;
	rotateCcdPlus1Button->setPos(posX, posY);
	posX += rotateCcdPlus1Button->boundingRect().width() + spacing;
	rotateCcdPlus5Button->setPos(posX, posY);
	posX += rotateCcdPlus5Button->boundingRect().width() + spacing;
	rotateCcdPlus15Button->setPos(posX, posY);	
	widgetHeight += rotateCcdMinus15Button->boundingRect().height();

	if (ccd->hasOAG())
	{
		posX = fieldCcdRotation->x();
		posY += rotateCcdMinus15Button->boundingRect().height();

		QString positionAngle = QString::number(ocularsPlugin->getSelectedCCDPrismPositionAngle(), 'f', 0);
		positionAngle.append(QChar(0x00B0));
		// TRANSLATOR: PA = Position angle
		QString positionAngleLabel = QString(q_("PA of prism: %1")).arg(positionAngle);
		fieldPrismRotation->setPlainText(positionAngleLabel);
		fieldPrismRotation->setPos(posX, posY);
		fieldPrismRotation->setToolTip(q_("Position angle of the off-axis guider' prism"));
		posY += fieldPrismRotation->boundingRect().height();
		widgetHeight += fieldPrismRotation->boundingRect().height();

		int positionAngleButtonsWidth = rotatePrismMinus15Button->boundingRect().width();
		positionAngleButtonsWidth += rotatePrismMinus5Button->boundingRect().width();
		positionAngleButtonsWidth += rotatePrismMinus1Button->boundingRect().width();
		positionAngleButtonsWidth += resetPrismRotationButton->boundingRect().width();
		positionAngleButtonsWidth += rotatePrismPlus1Button->boundingRect().width();
		positionAngleButtonsWidth += rotatePrismPlus5Button->boundingRect().width();
		positionAngleButtonsWidth += rotatePrismPlus15Button->boundingRect().width();
		spacing = (fieldPrismRotation->boundingRect().width() - positionAngleButtonsWidth) / 6;
		posX = fieldPrismRotation->x();
		rotatePrismMinus15Button->setPos(posX, posY);
		posX += rotatePrismMinus15Button->boundingRect().width() + spacing;
		rotatePrismMinus5Button->setPos(posX, posY);
		posX += rotatePrismMinus5Button->boundingRect().width() + spacing;
		rotatePrismMinus1Button->setPos(posX, posY);
		posX += rotatePrismMinus1Button->boundingRect().width() + spacing;
		resetPrismRotationButton->setPos(posX, posY);
		posX += resetPrismRotationButton->boundingRect().width() + spacing;
		rotatePrismPlus1Button->setPos(posX, posY);
		posX += rotatePrismPlus1Button->boundingRect().width() + spacing;
		rotatePrismPlus5Button->setPos(posX, posY);
		posX += rotatePrismPlus5Button->boundingRect().width() + spacing;
		rotatePrismPlus15Button->setPos(posX, posY);
		widgetHeight += rotatePrismMinus15Button->boundingRect().height();

		fieldPrismRotation->setVisible(true);
		rotatePrismMinus15Button->setVisible(true);
		rotatePrismMinus5Button->setVisible(true);
		rotatePrismMinus1Button->setVisible(true);
		resetPrismRotationButton->setVisible(true);
		rotatePrismPlus1Button->setVisible(true);
		rotatePrismPlus5Button->setVisible(true);
		rotatePrismPlus15Button->setVisible(true);
	}
	else
	{
		fieldPrismRotation->setVisible(false);
		rotatePrismMinus15Button->setVisible(false);
		rotatePrismMinus5Button->setVisible(false);
		rotatePrismMinus1Button->setVisible(false);
		resetPrismRotationButton->setVisible(false);
		rotatePrismPlus1Button->setVisible(false);
		rotatePrismPlus5Button->setVisible(false);
		rotatePrismPlus15Button->setVisible(false);
	}

	ccdControls->setMinimumSize(widgetWidth, widgetHeight);
	ccdControls->resize(widgetWidth, widgetHeight);
	setCcdControlsVisible(true);

	updateTelescopeControls();//Contains a call to updatePosition()
}

void OcularsGuiPanel::updateTelescopeControls()
{
	//Get the name
	int index = ocularsPlugin->selectedTelescopeIndex;
	Telescope* telescope = ocularsPlugin->telescopes[index];
	Q_ASSERT(telescope);
	QString name = telescope->name();
	QString fullName;
	if (name.isEmpty())
	{
		fullName = QString(q_("Telescope #%1")).arg(index);
	}
	else
	{
		fullName = QString(q_("Telescope #%1: %2")).arg(index).arg(name);
	}
	fieldTelescopeName->setPlainText(fullName);

	qreal posX = 0.;
	qreal posY = 0.;
	qreal widgetWidth = 0.;
	qreal widgetHeight = 0.;

	//Prev button
	qreal heightAdjustment = (fieldTelescopeName->boundingRect().height() - prevTelescopeButton->boundingRect().height()) / 2.;
	prevTelescopeButton->setPos(posX, posY + heightAdjustment);
	posX += prevTelescopeButton->boundingRect().width();
	widgetWidth += prevTelescopeButton->boundingRect().width();

	//Name field
	fieldTelescopeName->setPos(posX, posY);
	posX += fieldTelescopeName->boundingRect().width();
	widgetWidth += fieldTelescopeName->boundingRect().height();

	//Next button
	nextTelescopeButton->setPos(posX, posY + heightAdjustment);
	widgetWidth += nextTelescopeButton->boundingRect().width();

	posX = prevTelescopeButton->boundingRect().width();
	posY += fieldTelescopeName->boundingRect().height();
	widgetHeight += fieldTelescopeName->boundingRect().height();

	Lens *lens = ocularsPlugin->selectedLens();

	double mag = 0.0;

	if (ocularsPlugin->flagShowCCD)
	{
		int index = ocularsPlugin->selectedCCDIndex;
		CCD* ccd = ocularsPlugin->ccds[index];
		Q_ASSERT(ccd);

		const double fovX = ccd->getActualFOVx(telescope, lens);
		const double fovY = ccd->getActualFOVy(telescope, lens);
		QString dimensionsLabel = QString(q_("Dimensions: %1")).arg(ocularsPlugin->getDimensionsString(fovX, fovY));
		fieldCcdDimensions->setPlainText(dimensionsLabel);
		fieldCcdDimensions->setToolTip(q_("Dimensions field of view"));

		QString binningLabel = QString("%1: %2 %4 %3").arg(q_("Binning")).arg(ccd->binningX()).arg(ccd->binningY()).arg(QChar(0x00D7));
		fieldCcdBinning->setPlainText(binningLabel);

		//TRANSLATORS: Unit of measure for scale - arc-seconds per pixel
		QString unit = q_("\"/px");
		fieldCcdHScale->setPlainText(QString("%1: %2%3").arg(q_("X scale"), QString::number(fovX*3600*ccd->binningX()/ccd->resolutionX(), 'f', 4), unit));
		fieldCcdHScale->setToolTip(q_("Horizontal scale"));
		fieldCcdVScale->setPlainText(QString("%1: %2%3").arg(q_("Y scale"), QString::number(fovY*3600*ccd->binningY()/ccd->resolutionY(), 'f', 4), unit));
		fieldCcdVScale->setToolTip(q_("Vertical scale"));

		prevTelescopeButton->setVisible(true);
		nextTelescopeButton->setVisible(true);
		fieldTelescopeName->setVisible(true);

		fieldMagnification->setVisible(false);
		fieldExitPupil->setVisible(false);
		fieldFov->setVisible(false);
	}

	if (ocularsPlugin->flagShowOculars)
	{
		//We need the current ocular
		int index = ocularsPlugin->selectedOcularIndex;
		Ocular* ocular = ocularsPlugin->oculars[index];
		Q_ASSERT(ocular);

		if (ocular->isBinoculars())
		{
			prevTelescopeButton->setVisible(false);
			nextTelescopeButton->setVisible(false);
			fieldTelescopeName->setVisible(false);
			fieldExitPupil->setVisible(false);
			posY = 0.;
			widgetHeight = 0.;

			fieldMagnification->setToolTip(q_("Magnification provided by these binoculars"));
			fieldFov->setToolTip(q_("Actual field of view provided by these binoculars"));
			fieldExitPupil->setToolTip(q_("Exit pupil provided by these binoculars"));
		}
		else
		{
			prevTelescopeButton->setVisible(true);
			nextTelescopeButton->setVisible(true);
			fieldTelescopeName->setVisible(true);

			fieldMagnification->setToolTip(q_("Magnification provided by this ocular/lens/telescope combination"));
			fieldFov->setToolTip(q_("Actual field of view provided by this ocular/lens/telescope combination"));
			fieldExitPupil->setToolTip(q_("Exit pupil provided by this ocular/lens/telescope combination"));
		}

		mag = ocular->magnification(telescope, lens);
		QString magnificationString = QString::number(mag, 'f', 1);
		magnificationString.append(QChar(0x02E3)); // Was 0x00D7
		magnificationString.append(QString(" (%1D)").arg(QString::number(mag/telescope->diameter(), 'f', 2)));
		QString magnificationLabel = QString(q_("Magnification: %1")).arg(magnificationString);
		fieldMagnification->setPlainText(magnificationLabel);
		fieldMagnification->setPos(posX, posY);
		posY += fieldMagnification->boundingRect().height();
		widgetHeight += fieldMagnification->boundingRect().height();

		if (mag>0)
		{
			double exitPupil = telescope->diameter()/mag;
			if (ocular->isBinoculars())
				exitPupil = ocular->fieldStop()/mag;
			QString exitPupilLabel = QString(q_("Exit pupil: %1 mm")).arg(QString::number(exitPupil, 'f', 2));
			fieldExitPupil->setPlainText(exitPupilLabel);
			fieldExitPupil->setPos(posX, posY);
			posY += fieldExitPupil->boundingRect().height();
			widgetHeight += fieldExitPupil->boundingRect().height();
		}

		QString fovString = QString::number(ocular->actualFOV(telescope, lens), 'f', 4) + QChar(0x00B0);
		QString fovLabel = QString(q_("FOV: %1")).arg(fovString);
		fieldFov->setPlainText(fovLabel);
		fieldFov->setPos(posX, posY);
		posY += fieldFov->boundingRect().height();
		widgetHeight += fieldFov->boundingRect().height();

		fieldMagnification->setVisible(true);
		fieldFov->setVisible(true);
		if (mag>0)
			fieldExitPupil->setVisible(true);
		else
			fieldExitPupil->setVisible(false);
	}	

	double diameter = telescope->diameter();
	if (diameter>0.0 && ocularsPlugin->getFlagShowResolutionCriteria())
	{
		QString rayleighLabel = QString("%1: %2\"").arg(q_("Rayleigh criterion")).arg(QString::number(138/diameter, 'f', 2));
		fieldRayleighCriterion->setPlainText(rayleighLabel);
		fieldRayleighCriterion->setToolTip(q_("The Rayleigh resolution criterion"));
		fieldRayleighCriterion->setPos(posX, posY);
		posY += fieldRayleighCriterion->boundingRect().height();
		widgetHeight += fieldRayleighCriterion->boundingRect().height();

		QString dawesLabel = QString("%1: %2\"").arg(q_("Dawes' limit")).arg(QString::number(116/diameter, 'f', 2));
		fieldDawesCriterion->setPlainText(dawesLabel);
		fieldDawesCriterion->setToolTip(q_("Dawes' resolution criterion"));
		fieldDawesCriterion->setPos(posX, posY);
		posY += fieldDawesCriterion->boundingRect().height();
		widgetHeight += fieldDawesCriterion->boundingRect().height();

		QString abbeyLabel = QString("%1: %2\"").arg(q_("Abbe's limit")).arg(QString::number(113/diameter, 'f', 2));
		fieldAbbeyCriterion->setPlainText(abbeyLabel);
		fieldAbbeyCriterion->setToolTip(q_("Abbe’s diffraction resolution limit"));
		fieldAbbeyCriterion->setPos(posX, posY);
		posY += fieldAbbeyCriterion->boundingRect().height();
		widgetHeight += fieldAbbeyCriterion->boundingRect().height();

		QString sparrowLabel = QString("%1: %2\"").arg(q_("Sparrow's limit")).arg(QString::number(108/diameter, 'f', 2));
		fieldSparrowCriterion->setPlainText(sparrowLabel);
		fieldSparrowCriterion->setToolTip(q_("Sparrow's resolution limit"));
		fieldSparrowCriterion->setPos(posX, posY);
		posY += fieldSparrowCriterion->boundingRect().height();
		widgetHeight += fieldSparrowCriterion->boundingRect().height();


		fieldRayleighCriterion->setVisible(true);
		fieldDawesCriterion->setVisible(true);
		fieldAbbeyCriterion->setVisible(true);
		fieldSparrowCriterion->setVisible(true);
	}
	else
	{
		fieldRayleighCriterion->setVisible(false);
		fieldDawesCriterion->setVisible(false);
		fieldAbbeyCriterion->setVisible(false);
		fieldSparrowCriterion->setVisible(false);
	}

	// Visual resolution
	if (ocularsPlugin->flagShowOculars && ocularsPlugin->getFlagShowResolutionCriteria() && diameter>0.0)
	{
		float rayleigh = 138/diameter;
		float vres = 60/mag;
		if (vres<rayleigh)
			vres = rayleigh;
		QString visualResolutionLabel = QString("%1: %2\"").arg(q_("Visual resolution")).arg(QString::number(vres, 'f', 2));
		fieldVisualResolution->setPlainText(visualResolutionLabel);
		fieldVisualResolution->setToolTip(q_("Visual resolution is based on eye properties and magnification"));
		fieldVisualResolution->setPos(posX, posY);
		posY += fieldVisualResolution->boundingRect().height();
		widgetHeight += fieldVisualResolution->boundingRect().height();

		fieldVisualResolution->setVisible(true);
	}
	else
		fieldVisualResolution->setVisible(false);

	telescopeControls->setMinimumSize(widgetWidth, widgetHeight);
	telescopeControls->resize(widgetWidth, widgetHeight);
	setTelescopeControlsVisible(true);

	updateLensControls();
	updatePosition();
}

void OcularsGuiPanel::setLensControlsVisible(bool show)
{
	if (show)
	{
		if (!lensControls->isVisible())
		{
			lensControls->setVisible(true);
			mainLayout->insertItem(3, lensControls);
		}
	}
	else
	{
		if (lensControls->isVisible())
		{
			mainLayout->removeItem(lensControls);
			lensControls->setVisible(false);
		}
	}
	mainLayout->invalidate();
	mainLayout->activate();
	resize(mainLayout->geometry().width(), mainLayout->geometry().height());
}

void OcularsGuiPanel::setOcularControlsVisible(bool show)
{
	if (show)
	{
		if (!ocularControls->isVisible())
		{
			ocularControls->setVisible(true);
			mainLayout->insertItem(1, ocularControls);
		}
	}
	else
	{
		if (ocularControls->isVisible())
		{
			mainLayout->removeItem(ocularControls);
			ocularControls->setVisible(false);
		}
	}
	buttonCrosshairs->setVisible(show);
	updateMainButtonsPositions();
}

void OcularsGuiPanel::setCcdControlsVisible(bool show)
{
	if (show)
	{
		if (!ccdControls->isVisible())
		{
			ccdControls->setVisible(true);
			mainLayout->insertItem(1, ccdControls);
		}
	}
	else
	{
		if (ccdControls->isVisible())
		{
			mainLayout->removeItem(ccdControls);
			ccdControls->setVisible(false);
		}
	}
}

void OcularsGuiPanel::setTelescopeControlsVisible(bool show)
{
	if (show)
	{
		if (!telescopeControls->isVisible())
		{
			telescopeControls->setVisible(true);
			mainLayout->insertItem(2, telescopeControls);
		}
	}
	else
	{
		if (telescopeControls->isVisible())
		{
			mainLayout->removeItem(telescopeControls);
			telescopeControls->setVisible(false);
		}
	}
	mainLayout->invalidate();
	mainLayout->activate();
	resize(mainLayout->geometry().width(), mainLayout->geometry().height());
}

void OcularsGuiPanel::updateMainButtonsPositions()
{
	Q_ASSERT(buttonOcular);
	Q_ASSERT(buttonCrosshairs);
	Q_ASSERT(buttonCcd);
	Q_ASSERT(buttonTelrad);
	Q_ASSERT(buttonConfiguration);

	int n = buttonCrosshairs->isVisible() ? 5 : 4;
	qreal width = n * buttonOcular->getButtonPixmapWidth();

	//Relative position inside the parent widget
	qreal posX = 0.;
	qreal posY = 0.;
	qreal spacing = 0.;
	if (prevTelescopeButton)
	{
		width += 2 * prevTelescopeButton->getButtonPixmapWidth();
		posX = prevTelescopeButton->getButtonPixmapWidth();
	}
	if ( (buttonOcular->parentItem()) && (buttonOcular->parentItem()->parentItem()) )
	{
		qreal parentWidth = buttonOcular->parentItem()->parentItem()->boundingRect().width();
		int nGaps = n - 1;//n buttons have n-1 gaps
		spacing = qRound((parentWidth-width)/nGaps);
	}
	buttonOcular->setPos(posX, posY);
	posX += buttonOcular->getButtonPixmapWidth() + spacing;
	if (buttonCrosshairs->isVisible())
	{
		buttonCrosshairs->setPos(posX, posY);
		posX += buttonCrosshairs->getButtonPixmapWidth()  + spacing;
	}
	buttonCcd->setPos(posX, posY);
	posX += buttonCcd->getButtonPixmapWidth()  + spacing;
	buttonTelrad->setPos(posX, posY);
	posX += buttonTelrad->getButtonPixmapWidth() + spacing;
	buttonConfiguration->setPos(posX, posY);
}

void OcularsGuiPanel::setControlsColor(const QColor& color)
{
	Q_ASSERT(fieldOcularName);
	Q_ASSERT(fieldOcularFl);
	Q_ASSERT(fieldOcularAfov);
	Q_ASSERT(fieldCcdName);
	Q_ASSERT(fieldCcdDimensions);
	Q_ASSERT(fieldCcdBinning);
	Q_ASSERT(fieldCcdHScale);
	Q_ASSERT(fieldCcdVScale);
	Q_ASSERT(fieldCcdRotation);
	Q_ASSERT(fieldPrismRotation);
	Q_ASSERT(fieldTelescopeName);
	Q_ASSERT(fieldMagnification);
	Q_ASSERT(fieldExitPupil);
	Q_ASSERT(fieldFov);
	Q_ASSERT(fieldRayleighCriterion);
	Q_ASSERT(fieldDawesCriterion);
	Q_ASSERT(fieldAbbeyCriterion);
	Q_ASSERT(fieldSparrowCriterion);
	Q_ASSERT(fieldVisualResolution);
	Q_ASSERT(fieldLensName);
	Q_ASSERT(fieldLensMultipler);

	fieldOcularName->setDefaultTextColor(color);
	fieldOcularFl->setDefaultTextColor(color);
	fieldOcularAfov->setDefaultTextColor(color);
	fieldCcdName->setDefaultTextColor(color);
	fieldCcdDimensions->setDefaultTextColor(color);
	fieldCcdBinning->setDefaultTextColor(color);
	fieldCcdHScale->setDefaultTextColor(color);
	fieldCcdVScale->setDefaultTextColor(color);
	fieldCcdRotation->setDefaultTextColor(color);
	fieldPrismRotation->setDefaultTextColor(color);
	fieldTelescopeName->setDefaultTextColor(color);
	fieldMagnification->setDefaultTextColor(color);
	fieldFov->setDefaultTextColor(color);
	fieldExitPupil->setDefaultTextColor(color);
	fieldRayleighCriterion->setDefaultTextColor(color);
	fieldDawesCriterion->setDefaultTextColor(color);
	fieldAbbeyCriterion->setDefaultTextColor(color);
	fieldSparrowCriterion->setDefaultTextColor(color);
	fieldVisualResolution->setDefaultTextColor(color);
	fieldLensName->setDefaultTextColor(color);
	fieldLensMultipler->setDefaultTextColor(color);
}

void OcularsGuiPanel::setControlsFont(const QFont& font)
{
	Q_ASSERT(fieldOcularName);
	Q_ASSERT(fieldOcularFl);
	Q_ASSERT(fieldOcularAfov);
	Q_ASSERT(fieldCcdName);
	Q_ASSERT(fieldCcdDimensions);
	Q_ASSERT(fieldCcdBinning);
	Q_ASSERT(fieldCcdHScale);
	Q_ASSERT(fieldCcdVScale);
	Q_ASSERT(fieldCcdRotation);
	Q_ASSERT(fieldPrismRotation);
	Q_ASSERT(fieldTelescopeName);
	Q_ASSERT(fieldMagnification);
	Q_ASSERT(fieldExitPupil);
	Q_ASSERT(fieldFov);
	Q_ASSERT(fieldRayleighCriterion);
	Q_ASSERT(fieldDawesCriterion);
	Q_ASSERT(fieldAbbeyCriterion);
	Q_ASSERT(fieldSparrowCriterion);
	Q_ASSERT(fieldVisualResolution);
	Q_ASSERT(fieldLensName);
	Q_ASSERT(fieldLensMultipler);

	fieldOcularName->setFont(font);
	fieldOcularFl->setFont(font);
	fieldOcularAfov->setFont(font);
	fieldCcdName->setFont(font);
	fieldCcdDimensions->setFont(font);
	fieldCcdBinning->setFont(font);
	fieldCcdHScale->setFont(font);
	fieldCcdVScale->setFont(font);
	fieldCcdRotation->setFont(font);
	fieldPrismRotation->setFont(font);
	fieldTelescopeName->setFont(font);
	fieldMagnification->setFont(font);
	fieldFov->setFont(font);
	fieldExitPupil->setFont(font);
	fieldRayleighCriterion->setFont(font);
	fieldDawesCriterion->setFont(font);
	fieldAbbeyCriterion->setFont(font);
	fieldSparrowCriterion->setFont(font);
	fieldVisualResolution->setFont(font);
	fieldLensName->setFont(font);
	fieldLensMultipler->setFont(font);
}

void OcularsGuiPanel::setColorScheme(const QString &schemeName)
{
	Q_UNUSED(schemeName);
	borderPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
	borderPath->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
	setControlsColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
}

QPixmap OcularsGuiPanel::createPixmapFromText(const QString& text,
					      int width,
					      int height,
					      const QFont& font,
					      const QColor& textColor,
					      const QColor& backgroundColor)
{
	if (width <= 0 || height <=0)
	{
		return QPixmap();
	}

	QPixmap pixmap(width, height);
	pixmap.fill(backgroundColor);

	if (text.isEmpty())
	{
		return pixmap;
	}

	QPainter painter(&pixmap);
	painter.setFont(font);
	painter.setPen(QPen(textColor));
	painter.drawText(0, 0, width, height, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);

	return pixmap;
}
