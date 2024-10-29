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
				      QPixmap(":/graphicGui/miscGlow32x32.png"),
				      ocularsPlugin->actionShowOcular,
				      true); //No background
	buttonOcular->setToolTip(ocularsPlugin->actionShowOcular->getText());

	Q_ASSERT(ocularsPlugin->actionShowCrosshairs);
	buttonCrosshairs = new StelButton(buttonBar,
					  QPixmap(":/ocular/bt_crosshairs_on.png"),
					  QPixmap(":/ocular/bt_crosshairs_off.png"),
					  QPixmap(":/graphicGui/miscGlow32x32.png"),
					  ocularsPlugin->actionShowCrosshairs,
					  true);
	buttonCrosshairs->setToolTip(ocularsPlugin->actionShowCrosshairs->getText());
	buttonCrosshairs->setVisible(false);

	Q_ASSERT(ocularsPlugin->actionShowSensor);
	buttonCcd = new StelButton(buttonBar,
				   QPixmap(":/ocular/bt_sensor_on.png"),
				   QPixmap(":/ocular/bt_sensor_off.png"),
				   QPixmap(":/graphicGui/miscGlow32x32.png"),
				   ocularsPlugin->actionShowSensor,
				   true);
	buttonCcd->setToolTip(ocularsPlugin->actionShowSensor->getText());

	Q_ASSERT(ocularsPlugin->actionShowTelrad);
	buttonTelrad = new StelButton(buttonBar,
				      QPixmap(":/ocular/bt_telrad_on.png"),
				      QPixmap(":/ocular/bt_telrad_off.png"),
				      QPixmap(":/graphicGui/miscGlow32x32.png"),
				      ocularsPlugin->actionShowTelrad,
				      true);
	buttonTelrad->setToolTip(ocularsPlugin->actionShowTelrad->getText());

	Q_ASSERT(ocularsPlugin->actionConfiguration);
	buttonConfiguration = new StelButton(buttonBar,
					     QPixmap(":/ocular/bt_settings_on.png"),
					     QPixmap(":/ocular/bt_settings_off.png"),
					     QPixmap(":/graphicGui/miscGlow32x32.png"),
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
	fieldTwilightFactor = new QGraphicsTextItem(telescopeControls);
	fieldRelativeBrightness = new QGraphicsTextItem(telescopeControls);
	fieldAdlerIndex = new QGraphicsTextItem(telescopeControls);
	fieldBishopIndex = new QGraphicsTextItem(telescopeControls);
	fieldFov = new QGraphicsTextItem(telescopeControls);
	fieldRayleighCriterion = new QGraphicsTextItem(telescopeControls);
	fieldDawesCriterion = new QGraphicsTextItem(telescopeControls);
	fieldAbbeyCriterion = new QGraphicsTextItem(telescopeControls);
	fieldSparrowCriterion = new QGraphicsTextItem(telescopeControls);
	fieldVisualResolution = new QGraphicsTextItem(telescopeControls);
	fieldLimitMagnitude = new QGraphicsTextItem(telescopeControls);

	fieldLensName = new QGraphicsTextItem(lensControls);
	fieldLensMultipler = new QGraphicsTextItem(lensControls);

	QFont newFont = font();
	newFont.setPixelSize(plugin->getGuiPanelFontSize());
	setControlsFont(newFont);

	//Traditional field width from Ocular ;)
	QFontMetrics fm(fieldOcularName->font());
	int maxWidth = fm.boundingRect(QString("MMMMMMMMMMMMMMMMMMMMMM")).width();
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
	fieldTwilightFactor->setTextWidth(maxWidth);
	fieldRelativeBrightness->setTextWidth(maxWidth);
	fieldAdlerIndex->setTextWidth(maxWidth);
	fieldBishopIndex->setTextWidth(maxWidth);
	fieldFov->setTextWidth(maxWidth);
	fieldRayleighCriterion->setTextWidth(maxWidth);
	fieldDawesCriterion->setTextWidth(maxWidth);
	fieldAbbeyCriterion->setTextWidth(maxWidth);
	fieldSparrowCriterion->setTextWidth(maxWidth);
	fieldVisualResolution->setTextWidth(maxWidth);
	fieldLensName->setTextWidth(maxWidth);
	fieldLensMultipler->setTextWidth(maxWidth);

	// Retrieve value from setting directly, because at this stage the plugin has not parsed it yet.
	float scv = plugin->getSettings()->value("arrow_scale", 150.f).toFloat();
	int scale=static_cast<int>(lineHeight*scv*0.01f);
	// TODO: change this load-once to interactively editable value of scaling coefficient
	QPixmap pa(":/graphicGui/btTimeRewind-on.png");
	QPixmap prevArrow = pa.scaledToHeight(scale * StelButton::getInputPixmapsDevicePixelRatio(), Qt::SmoothTransformation);
	QPixmap paOff(":/graphicGui/btTimeRewind-off.png");
	QPixmap prevArrowOff = paOff.scaledToHeight(scale * StelButton::getInputPixmapsDevicePixelRatio(), Qt::SmoothTransformation);
	QPixmap na(":/graphicGui/btTimeForward-on.png");
	QPixmap nextArrow = na.scaledToHeight(scale * StelButton::getInputPixmapsDevicePixelRatio(), Qt::SmoothTransformation);
	QPixmap naOff(":/graphicGui/btTimeForward-off.png");
	QPixmap nextArrowOff = naOff.scaledToHeight(scale * StelButton::getInputPixmapsDevicePixelRatio(), Qt::SmoothTransformation);

	prevOcularButton = new StelButton(ocularControls, prevArrow, prevArrowOff, QPixmap(), "actionShow_Ocular_Decrement");
	prevOcularButton->setToolTip(q_("Select previous eyepiece"));
	nextOcularButton = new StelButton(ocularControls, nextArrow, nextArrowOff, QPixmap(), "actionShow_Ocular_Increment");
	nextOcularButton->setToolTip(q_("Select next eyepiece"));
	prevLensButton = new StelButton(lensControls, prevArrow, prevArrowOff, QPixmap(), "actionShow_Lens_Decrement");
	prevLensButton->setToolTip(q_("Select previous lens"));
	nextLensButton = new StelButton(lensControls, nextArrow, nextArrowOff, QPixmap(), "actionShow_Lens_Increment");
	nextLensButton->setToolTip(q_("Select next lens"));
	prevCcdButton = new StelButton(ccdControls, prevArrow, prevArrowOff, QPixmap(), "actionShow_CCD_Decrement");
	prevCcdButton->setToolTip(q_("Select previous CCD frame"));
	nextCcdButton = new StelButton(ccdControls, nextArrow, nextArrowOff, QPixmap(), "actionShow_CCD_Increment");
	nextCcdButton->setToolTip(q_("Select next CCD frame"));
	prevTelescopeButton = new StelButton(telescopeControls, prevArrow, prevArrowOff, QPixmap(), "actionShow_Telescope_Decrement");
	prevTelescopeButton->setToolTip(q_("Select previous telescope"));
	nextTelescopeButton = new StelButton(telescopeControls, nextArrow, nextArrowOff, QPixmap(), "actionShow_Telescope_Increment");
	nextTelescopeButton->setToolTip(q_("Select next telescope"));

	// NOTE: actions for rotation in Oculars.cpp file
	QColor cOn(255, 255, 255);
	QColor cOff(102, 102, 102);
	QColor cHover(162, 162, 162);
	QString degrees = QString("-90%1").arg(QChar(0x00B0));
	int degreesW = fm.boundingRect(degrees).width();
	QPixmap pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	QPixmap pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	QPixmap pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdMinus90Button = new StelButton(ccdControls, pOn,  pOff, pHover, "actionToggle_Oculars_Rotate_Frame_90_Counterclockwise", true);
	rotateCcdMinus90Button->setToolTip(q_("Rotate the sensor frame 90 degrees counterclockwise"));
	rotatePrismMinus90Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_90_Counterclockwise", true);
	rotatePrismMinus90Button->setToolTip(q_("Rotate the prism 90 degrees counterclockwise"));

	degrees = QString("-15%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
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

	degrees  = QString("+90%1").arg(QChar(0x00B0));
	degreesW = fm.boundingRect(degrees).width();
	pOn    = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOn);
	pOff   = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cOff);
	pHover = createPixmapFromText(degrees, degreesW, lineHeight, newFont, cHover);
	rotateCcdPlus90Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Frame_90_Clockwise", true);
	rotateCcdPlus90Button->setToolTip(q_("Rotate the sensor frame 90 degrees clockwise"));
	rotatePrismPlus90Button = new StelButton(ccdControls, pOn, pOff, pHover, "actionToggle_Oculars_Rotate_Prism_90_Clockwise", true);
	rotatePrismPlus90Button->setToolTip(q_("Rotate the prism 90 degrees clockwise"));

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
	connect(ocularsPlugin, SIGNAL(selectedCCDRotationAngleChanged(double)),      this, SLOT(updateCcdControls()));
	connect(ocularsPlugin, SIGNAL(selectedCCDPrismPositionAngleChanged(double)), this, SLOT(updateCcdControls()));

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
	delete fieldTwilightFactor; fieldTwilightFactor = Q_NULLPTR;	
	delete fieldRelativeBrightness; fieldRelativeBrightness = Q_NULLPTR;
	delete fieldAdlerIndex; fieldAdlerIndex = Q_NULLPTR;
	delete fieldBishopIndex; fieldBishopIndex = Q_NULLPTR;
	delete fieldFov; fieldFov = Q_NULLPTR;
	delete fieldRayleighCriterion; fieldRayleighCriterion = Q_NULLPTR;
	delete fieldDawesCriterion; fieldDawesCriterion = Q_NULLPTR;
	delete fieldAbbeyCriterion; fieldAbbeyCriterion = Q_NULLPTR;
	delete fieldSparrowCriterion; fieldSparrowCriterion = Q_NULLPTR;
	delete fieldVisualResolution; fieldVisualResolution = Q_NULLPTR;
	delete fieldLimitMagnitude; fieldLimitMagnitude = Q_NULLPTR;
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
	QPointF verticalBorderStart = geometry().topLeft() + QPointF(-0.5,0.5);
	QPointF horizontalBorderEnd = geometry().bottomRight() + QPointF(-0.5,0.5);
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
	setOcularControlsVisible(ocularsPlugin->getEnableOcular());

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
	if (ocularsPlugin->getEnableCCD() || ocularsPlugin->getEnableOcular())
	{
		if (ocular->isBinoculars() && ocularsPlugin->getEnableOcular()) // Hide the lens info for binoculars in eyepiece mode only
			setLensControlsVisible(false);
		else
			setLensControlsVisible(true);
	}
	else
		setLensControlsVisible(false);
}

void OcularsGuiPanel::updateCcdControls()
{
	setOcularControlsVisible(false);

	//Get the name
	int index = ocularsPlugin->selectedCCDIndex;
	CCD* ccd = ocularsPlugin->ccds[index];	
	Q_ASSERT(ccd);
	if (ccd->chipRotAngle()!=ocularsPlugin->getSelectedCCDRotationAngle())
		ocularsPlugin->setSelectedCCDRotationAngle(ccd->chipRotAngle());
	if (ccd->prismPosAngle()!=ocularsPlugin->getSelectedCCDPrismPositionAngle())
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
	fieldCcdHScale->setPlainText(QString("%1: %2%3").arg(q_("X scale"), QString::number(3600*ccd->getCentralAngularResolutionX(telescope, lens), 'f', 4), unit));
	fieldCcdHScale->setToolTip(q_("Horizontal scale"));
	fieldCcdHScale->setPos(posX, posY);
	posY += fieldCcdHScale->boundingRect().height();
	widgetHeight += fieldCcdHScale->boundingRect().height();
	fieldCcdVScale->setPlainText(QString("%1: %2%3").arg(q_("Y scale"), QString::number(3600*ccd->getCentralAngularResolutionY(telescope, lens), 'f', 4), unit));
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

	int rotationButtonsWidth = rotateCcdMinus90Button->boundingRect().width()
				 + rotateCcdMinus15Button->boundingRect().width()
				 + rotateCcdMinus5Button->boundingRect().width()
				 + rotateCcdMinus1Button->boundingRect().width()
				 + resetCcdRotationButton->boundingRect().width()
				 + rotateCcdPlus1Button->boundingRect().width()
				 + rotateCcdPlus5Button->boundingRect().width()
				 + rotateCcdPlus15Button->boundingRect().width()
				 + rotateCcdPlus90Button->boundingRect().width();
	int spacing = (fieldCcdRotation->boundingRect().width() - rotationButtonsWidth) / 8;
	posX = fieldCcdRotation->x();
	rotateCcdMinus90Button->setPos(posX, posY);
	posX += rotateCcdMinus90Button->boundingRect().width() + spacing;
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
	posX += rotateCcdPlus15Button->boundingRect().width() + spacing;
	rotateCcdPlus90Button->setPos(posX, posY);
	widgetHeight += rotateCcdMinus90Button->boundingRect().height();

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

		int positionAngleButtonsWidth = rotatePrismMinus90Button->boundingRect().width()
					      + rotatePrismMinus15Button->boundingRect().width()
					      + rotatePrismMinus5Button->boundingRect().width()
					      + rotatePrismMinus1Button->boundingRect().width()
					      + resetPrismRotationButton->boundingRect().width()
					      + rotatePrismPlus1Button->boundingRect().width()
					      + rotatePrismPlus5Button->boundingRect().width()
					      + rotatePrismPlus15Button->boundingRect().width()
					      + rotatePrismPlus90Button->boundingRect().width();
		spacing = (fieldPrismRotation->boundingRect().width() - positionAngleButtonsWidth) / 8;
		posX = fieldPrismRotation->x();
		rotatePrismMinus90Button->setPos(posX, posY);
		posX += rotatePrismMinus90Button->boundingRect().width() + spacing;
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
		posX += rotatePrismPlus15Button->boundingRect().width() + spacing;
		rotatePrismPlus90Button->setPos(posX, posY);
		widgetHeight += rotatePrismMinus90Button->boundingRect().height();

		fieldPrismRotation->setVisible(true);
		rotatePrismMinus90Button->setVisible(true);
		rotatePrismMinus15Button->setVisible(true);
		rotatePrismMinus5Button->setVisible(true);
		rotatePrismMinus1Button->setVisible(true);
		resetPrismRotationButton->setVisible(true);
		rotatePrismPlus1Button->setVisible(true);
		rotatePrismPlus5Button->setVisible(true);
		rotatePrismPlus15Button->setVisible(true);
		rotatePrismPlus90Button->setVisible(true);
	}
	else
	{
		fieldPrismRotation->setVisible(false);
		rotatePrismMinus90Button->setVisible(false);
		rotatePrismMinus15Button->setVisible(false);
		rotatePrismMinus5Button->setVisible(false);
		rotatePrismMinus1Button->setVisible(false);
		resetPrismRotationButton->setVisible(false);
		rotatePrismPlus1Button->setVisible(false);
		rotatePrismPlus5Button->setVisible(false);
		rotatePrismPlus15Button->setVisible(false);
		rotatePrismPlus90Button->setVisible(false);
	}

	ccdControls->setMinimumSize(widgetWidth, widgetHeight);
	ccdControls->resize(widgetWidth, widgetHeight);
	setCcdControlsVisible(ocularsPlugin->getEnableCCD());

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
		int indexCCD = ocularsPlugin->selectedCCDIndex;
		CCD* ccd = ocularsPlugin->ccds[indexCCD];
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
		fieldCcdHScale->setPlainText(QString("%1: %2%3").arg(q_("X scale"), QString::number(3600*ccd->getCentralAngularResolutionX(telescope, lens), 'f', 4), unit));
		fieldCcdHScale->setToolTip(q_("Horizontal scale"));
		fieldCcdVScale->setPlainText(QString("%1: %2%3").arg(q_("Y scale"), QString::number(3600*ccd->getCentralAngularResolutionY(telescope, lens), 'f', 4), unit));
		fieldCcdVScale->setToolTip(q_("Vertical scale"));

		prevTelescopeButton->setVisible(true);
		nextTelescopeButton->setVisible(true);
		fieldTelescopeName->setVisible(true);

		fieldMagnification->setVisible(false);
		fieldExitPupil->setVisible(false);
		fieldTwilightFactor->setVisible(false);
		fieldRelativeBrightness->setVisible(false);
		fieldAdlerIndex->setVisible(false);
		fieldBishopIndex->setVisible(false);
		fieldFov->setVisible(false);		
	}

	bool isBinocular = false;
	if (ocularsPlugin->flagShowOculars)
	{
		//We need the current ocular
		int indexOcular = ocularsPlugin->selectedOcularIndex;
		Ocular* ocular = ocularsPlugin->oculars[indexOcular];
		bool showBinoFactors = false;
		isBinocular = ocular->isBinoculars();
		Q_ASSERT(ocular);

		if (isBinocular)
		{
			prevTelescopeButton->setVisible(false);
			nextTelescopeButton->setVisible(false);
			fieldTelescopeName->setVisible(false);			
			posY = 0.;
			widgetHeight = 0.;

			fieldMagnification->setToolTip(q_("Magnification provided by these binoculars"));
			fieldFov->setToolTip(q_("Actual field of view provided by these binoculars"));
			fieldExitPupil->setToolTip(q_("Exit pupil provided by these binoculars"));
			fieldTwilightFactor->setToolTip(q_("Twilight factor (or dusk index) provided by these binoculars"));
			fieldRelativeBrightness->setToolTip(q_("Relative brightness provided by these binoculars"));
			fieldAdlerIndex->setToolTip(q_("Alan Adler's Index (or Astro Index) provided by these binoculars"));
			fieldBishopIndex->setToolTip(q_("Roy Bishop's Index (or Visibility Factor) provided by these binoculars"));
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
		double diameter = (isBinocular ? ocular->fieldStop() : telescope->diameter());
		QString magnificationString = QString::number(mag, 'f', 1);
		magnificationString.append(QChar(0x02E3)); // Was 0x00D7
		magnificationString.append(QString(" (%1D)").arg(QString::number(mag/diameter, 'f', 2)));
		QString magnificationLabel = QString(q_("Magnification: %1")).arg(magnificationString);
		fieldMagnification->setPlainText(magnificationLabel);
		fieldMagnification->setPos(posX, posY);
		posY += fieldMagnification->boundingRect().height();
		widgetHeight += fieldMagnification->boundingRect().height();

		if (mag>0)
		{
			double exitPupil = diameter/mag;
			QString exitPupilLabel = QString(q_("Exit pupil: %1 mm")).arg(QString::number(exitPupil, 'f', 2));
			fieldExitPupil->setPlainText(exitPupilLabel);
			fieldExitPupil->setPos(posX, posY);
			posY += fieldExitPupil->boundingRect().height();
			widgetHeight += fieldExitPupil->boundingRect().height();
			if (isBinocular)
			{
				showBinoFactors = true;
				QString twilightFactorLabel = QString("%1: %2").arg(q_("Twilight factor"), QString::number(::sqrt(mag*diameter), 'f', 2));
				fieldTwilightFactor->setPlainText(twilightFactorLabel);
				fieldTwilightFactor->setPos(posX, posY);
				posY += fieldTwilightFactor->boundingRect().height();
				widgetHeight += fieldTwilightFactor->boundingRect().height();

				QString relativeBrightnessLabel = QString("%1: %2").arg(q_("Relative brightness"), QString::number(exitPupil*exitPupil, 'f', 2));
				fieldRelativeBrightness->setPlainText(relativeBrightnessLabel);
				fieldRelativeBrightness->setPos(posX, posY);
				posY += fieldRelativeBrightness->boundingRect().height();
				widgetHeight += fieldRelativeBrightness->boundingRect().height();

				QString fieldAdlerIndexLabel = QString("%1: %2").arg(q_("Adler index"), QString::number(::sqrt(diameter)*mag, 'f', 2));
				fieldAdlerIndex->setPlainText(fieldAdlerIndexLabel);
				fieldAdlerIndex->setPos(posX, posY);
				posY += fieldAdlerIndex->boundingRect().height();
				widgetHeight += fieldAdlerIndex->boundingRect().height();

				QString fieldBishopIndexLabel = QString("%1: %2").arg(q_("Bishop index"), QString::number(diameter*mag, 'f', 2));
				fieldBishopIndex->setPlainText(fieldBishopIndexLabel);
				fieldBishopIndex->setPos(posX, posY);
				posY += fieldBishopIndex->boundingRect().height();
				widgetHeight += fieldBishopIndex->boundingRect().height();
			}
		}

		QString fovString = QString::number(ocular->actualFOV(telescope, lens), 'f', 4) + QChar(0x00B0);
		// TRANSLATIONS: Label in Oculars plug-in
		QString fovLabel = QString("%1: %2").arg(qc_("FOV", "field of view"), fovString);
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
		fieldTwilightFactor->setVisible(showBinoFactors);
		fieldRelativeBrightness->setVisible(showBinoFactors);
		fieldAdlerIndex->setVisible(showBinoFactors);
		fieldBishopIndex->setVisible(showBinoFactors);
	}	

	double diameter = telescope->diameter();
	if (diameter>0.0 && ocularsPlugin->getFlagShowResolutionCriteria() && !isBinocular)
	{
		QString rayleighLabel = QString("%1: %2\"").arg(q_("Rayleigh criterion"), QString::number(138/diameter, 'f', 2));
		fieldRayleighCriterion->setPlainText(rayleighLabel);
		fieldRayleighCriterion->setToolTip(q_("The Rayleigh resolution criterion"));
		fieldRayleighCriterion->setPos(posX, posY);
		posY += fieldRayleighCriterion->boundingRect().height();
		widgetHeight += fieldRayleighCriterion->boundingRect().height();

		QString dawesLabel = QString("%1: %2\"").arg(q_("Dawes' limit"), QString::number(116/diameter, 'f', 2));
		fieldDawesCriterion->setPlainText(dawesLabel);
		fieldDawesCriterion->setToolTip(q_("Dawes' resolution criterion"));
		fieldDawesCriterion->setPos(posX, posY);
		posY += fieldDawesCriterion->boundingRect().height();
		widgetHeight += fieldDawesCriterion->boundingRect().height();

		QString abbeyLabel = QString("%1: %2\"").arg(q_("Abbe's limit"), QString::number(113/diameter, 'f', 2));
		fieldAbbeyCriterion->setPlainText(abbeyLabel);
		fieldAbbeyCriterion->setToolTip(q_("Abbe’s diffraction resolution limit"));
		fieldAbbeyCriterion->setPos(posX, posY);
		posY += fieldAbbeyCriterion->boundingRect().height();
		widgetHeight += fieldAbbeyCriterion->boundingRect().height();

		QString sparrowLabel = QString("%1: %2\"").arg(q_("Sparrow's limit"), QString::number(108/diameter, 'f', 2));
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
	if (ocularsPlugin->flagShowOculars && ocularsPlugin->getFlagShowResolutionCriteria() && diameter>0.0 && !isBinocular)
	{
		double rayleigh = 138/diameter;
		double vres = 60/mag;
		if (vres<rayleigh)
			vres = rayleigh;
		QString visualResolutionLabel = QString("%1: %2\"").arg(q_("Visual resolution"), QString::number(vres, 'f', 2));
		fieldVisualResolution->setPlainText(visualResolutionLabel);
		fieldVisualResolution->setToolTip(q_("Visual resolution is based on eye properties and magnification"));
		fieldVisualResolution->setPos(posX, posY);
		posY += fieldVisualResolution->boundingRect().height();
		widgetHeight += fieldVisualResolution->boundingRect().height();

		fieldVisualResolution->setVisible(true);
	}
	else
		fieldVisualResolution->setVisible(false);

	// Limiting magnitude of binocular or ocular/lens/telescope combination
	if (ocularsPlugin->flagShowOculars && ocularsPlugin->getFlagAutoLimitMagnitude())
	{
		Ocular* ocular = ocularsPlugin->oculars[ocularsPlugin->selectedOcularIndex];
		QString limitMagnitudeLabel = QString("%1: %2").arg(qc_("Limiting magnitude", "Limiting magnitude of device"), QString::number(ocularsPlugin->getLimitMagnitude(ocular, telescope), 'f', 2));
		fieldLimitMagnitude->setPlainText(limitMagnitudeLabel);
		if (isBinocular)
			fieldLimitMagnitude->setToolTip(q_("Approximate limiting magnitude of this binocular"));
		else
			fieldLimitMagnitude->setToolTip(q_("Approximate limiting magnitude of this ocular/lens/telescope combination"));
		fieldLimitMagnitude->setPos(posX, posY);
		//posY += fieldLimitMagnitude->boundingRect().height();
		widgetHeight += fieldLimitMagnitude->boundingRect().height();

		fieldLimitMagnitude->setVisible(true);
	}
	else
		fieldLimitMagnitude->setVisible(false);

	telescopeControls->setMinimumSize(widgetWidth, widgetHeight);
	telescopeControls->resize(widgetWidth, widgetHeight);
	if (ocularsPlugin->getEnableCCD() || ocularsPlugin->getEnableOcular())
		setTelescopeControlsVisible(true);
	else
		setTelescopeControlsVisible(false);

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
	Q_ASSERT(fieldTwilightFactor);
	Q_ASSERT(fieldRelativeBrightness);
	Q_ASSERT(fieldAdlerIndex);
	Q_ASSERT(fieldBishopIndex);
	Q_ASSERT(fieldFov);
	Q_ASSERT(fieldRayleighCriterion);
	Q_ASSERT(fieldDawesCriterion);
	Q_ASSERT(fieldAbbeyCriterion);
	Q_ASSERT(fieldSparrowCriterion);
	Q_ASSERT(fieldVisualResolution);
	Q_ASSERT(fieldLimitMagnitude);
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
	fieldTwilightFactor->setDefaultTextColor(color);
	fieldRelativeBrightness->setDefaultTextColor(color);
	fieldAdlerIndex->setDefaultTextColor(color);
	fieldBishopIndex->setDefaultTextColor(color);
	fieldRayleighCriterion->setDefaultTextColor(color);
	fieldDawesCriterion->setDefaultTextColor(color);
	fieldAbbeyCriterion->setDefaultTextColor(color);
	fieldSparrowCriterion->setDefaultTextColor(color);
	fieldVisualResolution->setDefaultTextColor(color);
	fieldLimitMagnitude->setDefaultTextColor(color);
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
	Q_ASSERT(fieldTwilightFactor);
	Q_ASSERT(fieldRelativeBrightness);
	Q_ASSERT(fieldAdlerIndex);
	Q_ASSERT(fieldBishopIndex);
	Q_ASSERT(fieldFov);
	Q_ASSERT(fieldRayleighCriterion);
	Q_ASSERT(fieldDawesCriterion);
	Q_ASSERT(fieldAbbeyCriterion);
	Q_ASSERT(fieldSparrowCriterion);
	Q_ASSERT(fieldVisualResolution);
	Q_ASSERT(fieldLimitMagnitude);
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
	fieldTwilightFactor->setFont(font);
	fieldRelativeBrightness->setFont(font);
	fieldAdlerIndex->setFont(font);
	fieldBishopIndex->setFont(font);
	fieldRayleighCriterion->setFont(font);
	fieldDawesCriterion->setFont(font);
	fieldAbbeyCriterion->setFont(font);
	fieldSparrowCriterion->setFont(font);
	fieldVisualResolution->setFont(font);
	fieldLimitMagnitude->setFont(font);
	fieldLensName->setFont(font);
	fieldLensMultipler->setFont(font);
}

void OcularsGuiPanel::setColorScheme(const QString &schemeName)
{
	Q_UNUSED(schemeName)
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

	width  *= StelButton::getInputPixmapsDevicePixelRatio();
	height *= StelButton::getInputPixmapsDevicePixelRatio();

	QPixmap pixmap(width, height);
	pixmap.fill(backgroundColor);
	auto scaledFont(font);
	scaledFont.setPixelSize(font.pixelSize() * StelButton::getInputPixmapsDevicePixelRatio());

	if (text.isEmpty())
	{
		return pixmap;
	}

	QPainter painter(&pixmap);
	painter.setFont(scaledFont);
	painter.setPen(QPen(textColor));
	painter.drawText(0, 0, width, height, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, text);

	return pixmap;
}
