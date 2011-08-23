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

#include <QGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

OcularsGuiPanel::OcularsGuiPanel(Oculars* plugin,
                                 QGraphicsWidget *parent,
                                 Qt::WindowFlags wFlags):
	QGraphicsWidget(parent, wFlags),
	ocularsPlugin(plugin),
	parentWidget(parent),
	proxy(0),
	mainWidget(0)
{
	setVisible(false);
	QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(Qt::Vertical, this);
	layout->setContentsMargins(0, 0, 0, 0);

	mainWidget = new QWidget();
	mainWidget->setBaseSize(0,0);
	mainWidget->setMinimumSize(0,0);

	QGridLayout* mainWidgetLayout = new QGridLayout();
	//mainWidgetLayout->setMargin(0);
	mainWidgetLayout->setVerticalSpacing(0);
	mainWidgetLayout->setHorizontalSpacing(4);
	mainWidget->setLayout(mainWidgetLayout);

	//Normally, these buttons should use graphic icons
	prevOcularButton = new QPushButton("<");
	prevOcularButton->setToolTip("Previous ocular");
	nextOcularButton = new QPushButton(">");
	nextOcularButton->setToolTip("Next ocular");
	prevTelescopeButton = new QPushButton("<");
	prevTelescopeButton->setToolTip("Previous telescope");
	nextTelescopeButton = new QPushButton(">");
	nextTelescopeButton->setToolTip("Next telescope");
	prevCcdButton = new QPushButton("<");
	prevCcdButton->setToolTip("Previous CCD frame");
	nextCcdButton = new QPushButton(">");
	nextCcdButton->setToolTip("Next CCD frame");

	toggleCrosshairsButton = new QPushButton("crosshairs");
	toggleCrosshairsButton->setToolTip("Toggle crosshairs");
	configurationButton = new QPushButton("configure");

	//TODO: These need some explicit sizing
	labelOcularName = new QLabel();
	labelOcularFl = new QLabel();
	labelOcularAfov = new QLabel();
	labelCcdName = new QLabel();
	labelCcdDimensions = new QLabel();
	labelTelescopeName = new QLabel();
	labelMagnification = new QLabel();
	labelFov = new QLabel();

	//Distribute the widgets in the grid. The labels take 2 columns to make the
	//column number even to allow the adding of the two buttons on the bottom.
	mainWidgetLayout->addWidget(prevOcularButton, 0, 0, 1, 1);
	mainWidgetLayout->addWidget(labelOcularName, 0, 1, 1, 2);
	mainWidgetLayout->addWidget(nextOcularButton, 0, 3, 1, 1);
	mainWidgetLayout->addWidget(labelOcularFl, 1, 1, 1, 2);
	mainWidgetLayout->addWidget(labelOcularAfov, 2, 1, 1, 2);
	mainWidgetLayout->addWidget(prevCcdButton, 3, 0, 1, 1);
	mainWidgetLayout->addWidget(labelCcdName, 3, 1, 1, 2);
	mainWidgetLayout->addWidget(nextCcdButton, 3, 3, 1, 1);
	mainWidgetLayout->addWidget(labelCcdDimensions, 4, 1, 1, 2);
	mainWidgetLayout->addWidget(prevTelescopeButton, 5, 0, 1, 1);
	mainWidgetLayout->addWidget(labelTelescopeName, 5, 1, 1, 2);
	mainWidgetLayout->addWidget(nextTelescopeButton, 5, 3, 1, 1);
	mainWidgetLayout->addWidget(labelMagnification, 6, 1, 1, 2);
	mainWidgetLayout->addWidget(labelFov, 7, 1, 1, 2);
	mainWidgetLayout->addWidget(toggleCrosshairsButton, 8, 0, 1, 2);
	mainWidgetLayout->addWidget(configurationButton, 8, 2, 1, 2);

	//TODO

	proxy = new QGraphicsProxyWidget();
	proxy->setWidget(mainWidget);
	proxy->setFocusPolicy(Qt::NoFocus);
	proxy->setMinimumWidth(0);
	proxy->setMinimumHeight(0);
	proxy->setZValue(500.);

	layout->addItem(proxy);

	//updateGeometry();
	connect (parentWidget, SIGNAL(geometryChanged()),
	         this, SLOT(updatePosition()));

	//Connecting other slots
	connect(nextOcularButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(incrementOcularIndex()));
	connect(nextCcdButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(incrementCCDIndex()));
	connect(nextTelescopeButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(incrementTelescopeIndex()));
	connect(prevOcularButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(decrementOcularIndex()));
	connect(prevCcdButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(decrementCCDIndex()));
	connect(prevTelescopeButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(decrementTelescopeIndex()));

	connect(toggleCrosshairsButton, SIGNAL(clicked()),
	        ocularsPlugin, SLOT(toggleCrosshair()));
	connect(configurationButton, SIGNAL(clicked()),
	        this, SLOT(openOcularsConfigurationWindow()));

	connect(ocularsPlugin, SIGNAL(selectedOcularChanged()),
	        this, SLOT(updateOcularInfo()));
	connect(ocularsPlugin, SIGNAL(selectedCCDChanged()),
	        this, SLOT(updateCcdInfo()));
	connect(ocularsPlugin, SIGNAL(selectedTelescopeChanged()),
	        this, SLOT(updateTelescopeInfo()));
}

OcularsGuiPanel::~OcularsGuiPanel()
{
	if (mainWidget)
		delete mainWidget;
	//Does this take care of the proxy, too?
}

void OcularsGuiPanel::showOcularGui()
{
	if (ocularsPlugin->flagShowOculars)
	{
		setPreferredHeight(0);//WTF?
		updateOcularInfo();
		setVisible(true);
		updatePosition();
	}
	else
		setVisible(false);
}

void OcularsGuiPanel::showCcdGui()
{
	updateCcdInfo();
	setVisible(true);
	updatePosition();
}

void OcularsGuiPanel::updatePosition()
{
	updateGeometry();

	qreal xPos = parentWidget->size().width() - size().width();
	qreal yPos = 0;
	setPos(xPos, yPos);
}

void OcularsGuiPanel::openOcularsConfigurationWindow()
{
	ocularsPlugin->configureGui();
}

void OcularsGuiPanel::updateOcularInfo()
{
	setOcularControlsVisible(true);
	setCcdControlsVisible(false);

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
	labelOcularName->setText(fullName);

	if (ocular->isBinoculars())
	{
		setOcularInfoVisible(false);
	}
	else
	{
		setOcularInfoVisible(true);

		double focalLength = ocular->effectiveFocalLength();
		QString focalLengthString = QString("Ocular FL: %1mm").arg(focalLength);
		labelOcularFl->setText(focalLengthString);
		labelOcularFl->setToolTip("Focal length of the ocular");

		double apparentFov = ocular->appearentFOV();
		//TODO: This one could be better. Even in LTR languages the degree sign
		//is to the right of the value?
		QString apparentFovString =
			QString("Ocular aFOV: %1%2").arg(apparentFov).arg(QChar(0x00B0));
		labelOcularAfov->setText(apparentFovString);
		labelOcularAfov->setToolTip("Apparent field of view of the ocular");
	}

	updateTelescopeInfo();//Contains a call to updatePosition()
}

void OcularsGuiPanel::updateCcdInfo()
{
	setOcularControlsVisible(false);
	setOcularInfoVisible(false);
	setCcdControlsVisible(true);

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
	labelCcdName->setText(fullName);

	//The other label is updated in updateTelescopeInfo()

	updateTelescopeInfo();//Contains a call to updatePosition()
}

void OcularsGuiPanel::updateTelescopeInfo()
{
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
	labelTelescopeName->setText(fullName);

	if (ocularsPlugin->flagShowOculars)
	{
		setOtherControlsVisible(true);

		int index = ocularsPlugin->selectedOcularIndex;
		Ocular* ocular = ocularsPlugin->oculars[index];
		Q_ASSERT(ocular);

		//WTF? Rounding?
		double magnification = ((int)(ocular->magnification(telescope) * 10.0)) / 10.0;
		//TODO: Again, this can be simpler
		QString magnificationString =
			QString("Magnification: %1%2").arg(magnification).arg(QChar(0x00D7));
		labelMagnification->setText(magnificationString);
		labelMagnification->setToolTip("Magnification provided by this ocular/telescope combination");

		double fov = ((int)(ocular->actualFOV(telescope) * 10000.00)) / 10000.0;
		//TODO: Again, this can be simpler
		QString fovString = QString("FOV: %1%2").arg(fov).arg(QChar(0x00B0));
		labelFov->setText(fovString);
		labelFov->setToolTip("Actual field of view provided by this ocular/telescope combination");
	}
	else if (ocularsPlugin->flagShowCCD)
	{
		setOtherControlsVisible(false);

		int index = ocularsPlugin->selectedCCDIndex;
		CCD* ccd = ocularsPlugin->ccds[index];
		Q_ASSERT(ccd);

		double fovX = ((int)(ccd->getActualFOVx(telescope) * 1000.0)) / 1000.0;
		double fovY = ((int)(ccd->getActualFOVy(telescope) * 1000.0)) / 1000.0;
		//TODO: Again, the symbol could be handled simpler.
		QString dimensions = QString("Dimensions: %1%2 %3 %4%5").arg(fovX).arg(QChar(0x00B0)).arg(QChar(0x00D7)).arg(fovY).arg(QChar(0x00B0));
		labelCcdDimensions->setText(dimensions);
	}

	updatePosition();
}

void OcularsGuiPanel::setOcularControlsVisible(bool show)
{
	nextOcularButton->setVisible(show);
	prevOcularButton->setVisible(show);
	labelOcularName->setVisible(show);
}

void OcularsGuiPanel::setOcularInfoVisible(bool show)
{
	labelOcularFl->setVisible(show);
	labelOcularAfov->setVisible(show);
}

void OcularsGuiPanel::setCcdControlsVisible(bool show)
{
	nextCcdButton->setVisible(show);
	prevCcdButton->setVisible(show);
	labelCcdName->setVisible(show);
	labelCcdDimensions->setVisible(show);
}

void OcularsGuiPanel::setOtherControlsVisible(bool show)
{
	labelMagnification->setVisible(show);
	labelFov->setVisible(show);
}
