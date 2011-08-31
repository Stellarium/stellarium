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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Oculars.hpp"
#include "OcularsGuiPanel.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include <QAction>
#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsPathItem>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPen>
#include <QPushButton>
#include <QWidget>

OcularsGuiPanel::OcularsGuiPanel(Oculars* plugin,
                                 QGraphicsWidget *parent,
                                 Qt::WindowFlags wFlags):
	QGraphicsWidget(parent, wFlags),
	ocularsPlugin(plugin),
	parentWidget(parent)
{
	//setVisible(false);
	//setMinimumSize(0, 0);
	setMaximumSize(300, 400);
	//setPreferredSize(230, 100);
	setContentsMargins(0, 0, 0, 0);
	//TODO: set font

	//First create the layout and populate it, then set it?
	mainLayout = new QGraphicsLinearLayout(Qt::Vertical);
	//layout->setContentsMargins(0, 0, 0, 0);
	//layout->setSpacing(0);

	//Button bar
	buttonBar = new QGraphicsWidget();
	mainLayout->addItem(buttonBar);

	//TODO: Unique icons
	//TODO: QActions can be inserted here, even without keyboard shortcuts
	//TODO: Fix actions?
	//QPixmap leftBackground(":/graphicGui/btbg-left.png");
	//QPixmap middleBackground(":/graphicGui/btbg-middle.png");
	//QPixmap rightBackground(":/graphicGui/btbg-right.png");

	StelApp& stelApp = StelApp::getInstance();
	StelGui* gui = dynamic_cast<StelGui*>(stelApp.getGui());
	Q_ASSERT(gui);
	//QAction* actionToggleOcular = gui->getGuiActions("actionShow_Ocular");
	buttonOcular = new StelButton(buttonBar,
	                              QPixmap(":/ocular/bt_ocular_on.png"),
	                              QPixmap(":/ocular/bt_ocular_off.png"),
	                              QPixmap(),
	                              ocularsPlugin->actionShowOcular,
	                              true); //No background
	buttonOcular->setToolTip("Ocular");
	//buttonOcular->setBackgroundPixmap(leftBackground);
	buttonOcular->setParentItem(buttonBar);

	//Hack to avoid buttonOcular being left "checked" if it has been toggled
	//without any object selected.
	disconnect(ocularsPlugin->actionShowOcular, SIGNAL(toggled(bool)),
	           ocularsPlugin, SLOT(enableOcular(bool)));
	connect(ocularsPlugin->actionShowOcular, SIGNAL(toggled(bool)),
	        ocularsPlugin, SLOT(enableOcular(bool)));

	buttonCrosshairs = new StelButton(buttonBar,
	                                  QPixmap(":/ocular/bt_crosshairs_on.png"),
	                                  QPixmap(":/ocular/bt_crosshairs_off.png"),
	                                  QPixmap(),
	                                  ocularsPlugin->actionShowCrosshairs,
	                                  true);
	buttonCrosshairs->setToolTip("Crosshairs");
	buttonCrosshairs->setVisible(false);

	buttonCcd = new StelButton(buttonBar,
	                           QPixmap(":/ocular/bt_sensor_on.png"),
	                           QPixmap(":/ocular/bt_sensor_off.png"),
	                           QPixmap(),
	                           ocularsPlugin->actionShowSensor,
	                           true);
	buttonCcd->setToolTip("Sensor");

	buttonTelrad = new StelButton(buttonBar,
	                              QPixmap(":/ocular/bt_telrad_on.png"),
	                              QPixmap(":/ocular/bt_telrad_off.png"),
	                              QPixmap(),
	                              ocularsPlugin->actionShowTelrad,
	                              true);
	buttonTelrad->setToolTip("Telrad circles");

	buttonConfiguration = new StelButton(buttonBar,
	                                     QPixmap(":/ocular/bt_settings_on.png"),
	                                     QPixmap(":/ocular/bt_settings_off.png"),
	                                     QPixmap(),
	                                     ocularsPlugin->actionConfiguration,
	                                     true);
	buttonConfiguration->setToolTip("Oculars plugin configuration window");

	qreal buttonHeight = buttonOcular->boundingRect().height();
	buttonBar->setMinimumHeight(buttonHeight);
	buttonBar->setMaximumHeight(buttonHeight);

	setLayout(mainLayout);

	//Widgets with control and information fields
	ocularControls = new QGraphicsWidget(this);
	ocularControls->setParentItem(this);
	ocularControls->setVisible(false);
	ccdControls = new QGraphicsWidget(this);
	ccdControls->setParentItem(this);
	ccdControls->setVisible(false);
	telescopeControls = new QGraphicsWidget(this);
	telescopeControls->setParentItem(this);
	telescopeControls->setVisible(false);

	fieldOcularName = new QGraphicsTextItem(ocularControls);
	fieldOcularFl = new QGraphicsTextItem(ocularControls);
	fieldOcularAfov = new QGraphicsTextItem(ocularControls);
	fieldCcdName = new QGraphicsTextItem(ccdControls);
	fieldCcdDimensions = new QGraphicsTextItem(ccdControls);
	fieldTelescopeName = new QGraphicsTextItem(telescopeControls);
	fieldMagnification = new QGraphicsTextItem(telescopeControls);
	fieldFov = new QGraphicsTextItem(telescopeControls);

	QFont newFont = font();
	newFont.setPixelSize(12);
	setControlsFont(newFont);
	//setControlsColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));

	//Traditional field width from Ocular ;)
	QFontMetrics fm(fieldOcularName->font());
	int maxWidth = fm.width(QString("MMMMMMMMMMMMMMMMMMM"));
	int lineHeight = fm.height();

	fieldOcularName->setTextWidth(maxWidth);
	fieldOcularFl->setTextWidth(maxWidth);
	fieldOcularAfov->setTextWidth(maxWidth);
	fieldCcdName->setTextWidth(maxWidth);
	fieldCcdDimensions->setTextWidth(maxWidth);
	fieldTelescopeName->setTextWidth(maxWidth);
	fieldMagnification->setTextWidth(maxWidth);
	fieldFov->setTextWidth(maxWidth);

	QPixmap pa(":/graphicGui/btTimeRewind-on.png");
	QPixmap prevArrow = pa.scaledToHeight(lineHeight, Qt::SmoothTransformation);
	QPixmap paOff(":/graphicGui/btTimeRewind-off.png");
	QPixmap prevArrowOff = paOff.scaledToHeight(lineHeight, Qt::SmoothTransformation);
	QPixmap na(":/graphicGui/btTimeForward-on.png");
	QPixmap nextArrow = na.scaledToHeight(lineHeight, Qt::SmoothTransformation);
	QPixmap naOff(":/graphicGui/btTimeForward-off.png");
	QPixmap nextArrowOff = naOff.scaledToHeight(lineHeight, Qt::SmoothTransformation);

	QAction* defaultAction = new QAction(this);
	defaultAction->setCheckable(false);
	prevOcularButton = new StelButton(ocularControls,
	                                  prevArrow,
	                                  prevArrowOff,
	                                  QPixmap(),
	                                  defaultAction);
	prevOcularButton->setToolTip("Previous ocular");
	nextOcularButton = new StelButton(ocularControls,
	                                  nextArrow,
	                                  nextArrowOff,
	                                  QPixmap(),
	                                  defaultAction);
	nextOcularButton->setToolTip("Next ocular");
	prevCcdButton = new StelButton(ccdControls,
	                               prevArrow,
	                               prevArrowOff,
	                               QPixmap(),
	                               defaultAction);
	prevCcdButton->setToolTip("Previous CCD frame");
	nextCcdButton = new StelButton(ccdControls,
	                               nextArrow,
	                               nextArrowOff,
	                               QPixmap(),
	                               defaultAction);
	nextCcdButton->setToolTip("Next CCD frame");
	prevTelescopeButton = new StelButton(telescopeControls,
	                                     prevArrow,
	                                     prevArrowOff,
	                                     QPixmap(),
	                                     defaultAction);
	prevTelescopeButton->setToolTip("Previous telescope");
	nextTelescopeButton = new StelButton(telescopeControls,
	                                     nextArrow,
	                                     nextArrowOff,
	                                     QPixmap(),
	                                     defaultAction);
	nextTelescopeButton->setToolTip("Next telescope");

	connect(nextOcularButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(incrementOcularIndex()));
	connect(nextCcdButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(incrementCCDIndex()));
	connect(nextTelescopeButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(incrementTelescopeIndex()));
	connect(prevOcularButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(decrementOcularIndex()));
	connect(prevCcdButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(decrementCCDIndex()));
	connect(prevTelescopeButton, SIGNAL(triggered()),
	        ocularsPlugin, SLOT(decrementTelescopeIndex()));

	//Set the layout and update the size
	qreal width = 2*prevOcularButton->boundingRect().width() + maxWidth;
	qreal left, right, top, bottom;
	mainLayout->getContentsMargins(&left, &top, &right, &bottom);
	ocularControls->setMaximumWidth(width);
	ccdControls->setMaximumWidth(width);
	telescopeControls->setMaximumWidth(width);
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
	connect (parentWidget, SIGNAL(geometryChanged()),
	         this, SLOT(updatePosition()));

	//Connecting other slots
	connect(ocularsPlugin, SIGNAL(selectedOcularChanged()),
	        this, SLOT(updateOcularControls()));
	connect(ocularsPlugin, SIGNAL(selectedCCDChanged()),
	        this, SLOT(updateCcdControls()));
	connect(ocularsPlugin, SIGNAL(selectedTelescopeChanged()),
	        this, SLOT(updateTelescopeControls()));

	//Night mode
	connect(&stelApp, SIGNAL(colorSchemeChanged(const QString&)),
	        this, SLOT(setColorScheme(const QString&)));
	setColorScheme(stelApp.getCurrentStelStyle());
}

OcularsGuiPanel::~OcularsGuiPanel()
{

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
	if (name.isEmpty())
	{
		fullName = QString("Ocular #%1").arg(index);
	}
	else
	{
		fullName = QString("Ocular #%1: %2").arg(index).arg(name);
	}
	fieldOcularName->setPlainText(fullName);

	qreal posX = 0.;
	qreal posY = 0.;
	qreal widgetWidth = 0.;
	qreal widgetHeight = 0.;

	//Prev button
	qreal heightAdjustment = (fieldOcularName->boundingRect().height() - prevOcularButton->boundingRect().height()) / 2.;
	prevOcularButton->setPos(posX, round(posY + heightAdjustment));
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


	if (ocular->isBinoculars())
	{
		fieldOcularFl->setVisible(false);
		fieldOcularAfov->setVisible(false);
	}
	else
	{
		double focalLength = ocular->effectiveFocalLength();
		QString focalLengthString = QString("Ocular FL: %1mm").arg(focalLength);
		fieldOcularFl->setPlainText(focalLengthString);
		fieldOcularFl->setToolTip("Focal length of the ocular");
		fieldOcularFl->setPos(posX, posY);
		posY += fieldOcularFl->boundingRect().height();
		widgetHeight += fieldOcularFl->boundingRect().height();

		double apparentFov = ocular->appearentFOV();
		//TODO: This one could be better. Even in LTR languages the degree sign
		//is to the right of the value?
		QString apparentFovString =
			QString("Ocular aFOV: %1%2").arg(apparentFov).arg(QChar(0x00B0));
		fieldOcularAfov->setPlainText(apparentFovString);
		fieldOcularAfov->setToolTip("Apparent field of view of the ocular");
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

void OcularsGuiPanel::updateCcdControls()
{
	setOcularControlsVisible(false);

	//Get the name
	int index = ocularsPlugin->selectedCCDIndex;
	CCD* ccd = ocularsPlugin->ccds[index];
	Q_ASSERT(ccd);
	QString name = ccd->name();
	QString fullName;
	if (name.isEmpty())
	{
		fullName = QString("Sensor #%1").arg(index);
	}
	else
	{
		fullName = QString("Sensor #%1: %2").arg(index).arg(name);
	}
	fieldCcdName->setPlainText(fullName);

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
	double fovX = ((int)(ccd->getActualFOVx(telescope) * 1000.0)) / 1000.0;
	double fovY = ((int)(ccd->getActualFOVy(telescope) * 1000.0)) / 1000.0;
	//TODO: Again, the symbol could be handled simpler.
	QString dimensions = QString("Dimensions: %1%2 %3 %4%5").arg(fovX).arg(QChar(0x00B0)).arg(QChar(0x00D7)).arg(fovY).arg(QChar(0x00B0));
	fieldCcdDimensions->setPlainText(dimensions);
	fieldCcdDimensions->setPos(posX, posY);
	widgetHeight += fieldCcdDimensions->boundingRect().height();

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
		fullName = QString("Telescope #%1").arg(index);
	}
	else
	{
		fullName = QString("Telescope #%1: %2").arg(index).arg(name);
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
			posY = 0.;
			widgetHeight = 0.;

			fieldMagnification->setToolTip("Magnification provided by these binoculars");
			fieldFov->setToolTip("Actual field of view provided by these binoculars");
		}
		else
		{
			prevTelescopeButton->setVisible(true);
			nextTelescopeButton->setVisible(true);
			fieldTelescopeName->setVisible(true);

			fieldMagnification->setToolTip("Magnification provided by this ocular/telescope combination");
			fieldFov->setToolTip("Actual field of view provided by this ocular/telescope combination");
		}

		//WTF? Rounding?
		double magnification = ((int)(ocular->magnification(telescope) * 10.0)) / 10.0;
		//TODO: Again, this can be simpler
		QString magnificationString =
			QString("Magnification: %1%2").arg(magnification).arg(QChar(0x00D7));
		fieldMagnification->setPlainText(magnificationString);
		fieldMagnification->setPos(posX, posY);
		posY += fieldMagnification->boundingRect().height();
		widgetHeight += fieldMagnification->boundingRect().height();

		double fov = ((int)(ocular->actualFOV(telescope) * 10000.00)) / 10000.0;
		//TODO: Again, this can be simpler
		QString fovString = QString("FOV: %1%2").arg(fov).arg(QChar(0x00B0));
		fieldFov->setPlainText(fovString);
		fieldFov->setPos(posX, posY);
		widgetHeight += fieldFov->boundingRect().height();

		fieldMagnification->setVisible(true);
		fieldFov->setVisible(true);
	}
	else
	{
		fieldMagnification->setVisible(false);
		fieldFov->setVisible(false);
	}

	telescopeControls->setMinimumSize(widgetWidth, widgetHeight);
	telescopeControls->resize(widgetWidth, widgetHeight);
	setTelescopeControlsVisible(true);

	updatePosition();
}

void OcularsGuiPanel::setOcularControlsVisible(bool show)
{
	if (show)
	{
		if (!ocularControls->isVisible())
		{
			ocularControls->setVisible(true);
			mainLayout->insertItem(0, ocularControls);
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
			mainLayout->insertItem(0, ccdControls);
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
			mainLayout->insertItem(1, telescopeControls);
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
	resize(mainLayout->geometry().width(),
	       mainLayout->geometry().height());
}

void OcularsGuiPanel::updateMainButtonsPositions()
{
	Q_ASSERT(buttonOcular);
	Q_ASSERT(buttonCrosshairs);
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
	if (buttonOcular->parentItem())
	{
		qreal parentWidth = buttonOcular->parentItem()->boundingRect().width();
		int nGaps = n - 1;//n buttons have n-1 gaps
		//posX = round((parentWidth-width)/2.0);//Centering, deprecated
		spacing = round((parentWidth-width)/nGaps);
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
	posX += buttonConfiguration->getButtonPixmapWidth() + spacing;
}

void OcularsGuiPanel::setControlsColor(const QColor& color)
{
	Q_ASSERT(fieldOcularName);
	Q_ASSERT(fieldOcularFl);
	Q_ASSERT(fieldOcularAfov);
	Q_ASSERT(fieldCcdName);
	Q_ASSERT(fieldCcdDimensions);
	Q_ASSERT(fieldTelescopeName);
	Q_ASSERT(fieldMagnification);
	Q_ASSERT(fieldFov);

	fieldOcularName->setDefaultTextColor(color);
	fieldOcularFl->setDefaultTextColor(color);
	fieldOcularAfov->setDefaultTextColor(color);
	fieldCcdName->setDefaultTextColor(color);
	fieldCcdDimensions->setDefaultTextColor(color);
	fieldTelescopeName->setDefaultTextColor(color);
	fieldMagnification->setDefaultTextColor(color);
	fieldFov->setDefaultTextColor(color);
}

void OcularsGuiPanel::setControlsFont(const QFont& font)
{
	Q_ASSERT(fieldOcularName);
	Q_ASSERT(fieldOcularFl);
	Q_ASSERT(fieldOcularAfov);
	Q_ASSERT(fieldCcdName);
	Q_ASSERT(fieldCcdDimensions);
	Q_ASSERT(fieldTelescopeName);
	Q_ASSERT(fieldMagnification);
	Q_ASSERT(fieldFov);

	fieldOcularName->setFont(font);
	fieldOcularFl->setFont(font);
	fieldOcularAfov->setFont(font);
	fieldCcdName->setFont(font);
	fieldCcdDimensions->setFont(font);
	fieldTelescopeName->setFont(font);
	fieldMagnification->setFont(font);
	fieldFov->setFont(font);
}

void OcularsGuiPanel::setButtonsNightMode(bool nightMode)
{
	//Reused from SkyGui, with modifications
	foreach (QGraphicsItem *child, QGraphicsItem::children())
	{
		foreach (QGraphicsItem *grandchild, child->children())
		{
			StelButton* button = qgraphicsitem_cast<StelButton*>(grandchild);
			if (button)
				button->setRedMode(nightMode);
		}
	}
}

void OcularsGuiPanel::setColorScheme(const QString &schemeName)
{
	if (schemeName == "night_color")
	{
		borderPath->setPen(QColor::fromRgbF(0.7,0.2,0.2,0.5));
		borderPath->setBrush(QColor::fromRgbF(0.23, 0.13, 0.03, 0.2));
		setControlsColor(QColor::fromRgbF(0.9, 0.33, 0.33, 0.9));
		setButtonsNightMode(true);
	}
	else
	{
		borderPath->setPen(QColor::fromRgbF(0.7,0.7,0.7,0.5));
		borderPath->setBrush(QColor::fromRgbF(0.15, 0.16, 0.19, 0.2));
		setControlsColor(QColor::fromRgbF(0.9, 0.91, 0.95, 0.9));
		setButtonsNightMode(false);
	}
}
