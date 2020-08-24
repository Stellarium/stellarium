/*
 * Copyright (C) 2010 Pep Pujols
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "PrintSkyDialog.hpp"
#include "ui_printskyDialog.h"
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewWidget>
#include <QPrintPreviewDialog>
#include <QDebug>
#include <QTextDocument>

#include "PrintSky.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "Orbit.hpp"
#include "StelObjectMgr.hpp"



PrintSkyDialog::PrintSkyDialog() :
	outputPreview(false)
{
	ui = new Ui_printskyDialogForm;
}

PrintSkyDialog::~PrintSkyDialog()
{
	delete ui;
	ui = Q_NULLPTR;
}

/* ********************************************************************* */
void PrintSkyDialog::retranslate()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void PrintSkyDialog::updateStyle()
{
	if(dialog) {
		gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		QString style(gui->getStelStyle().htmlStyleSheet);
		ui->Contents->setStyleSheet(style);
	}
}

void PrintSkyDialog::styleChanged()
{
	// Nothing for now
}

/* ********************************************************************* */
void PrintSkyDialog::closeWindow()
{
	setVisible(false);
	StelMainView::getInstance().scene()->setActiveWindow(Q_NULLPTR);
}

/* ********************************************************************* */
void PrintSkyDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Now the rest of the actions.
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->previewSkyPushButton, SIGNAL(clicked()), this, SLOT(previewSky()));
	connect(ui->printSkyPushButton, SIGNAL(clicked()), this, SLOT(printSky()));

	connectBoolProperty(ui->invertColorsCheckBox,           "PrintSky.invertColors");
	connectBoolProperty(ui->scaleToFitCheckBox,             "PrintSky.scaleToFit");
	connectBoolProperty(ui->printDataCheckBox,              "PrintSky.printData");
	connectBoolProperty(ui->selectedObjectCheckBox,         "PrintSky.flagPrintObjectInfo");
	connectBoolProperty(ui->printSSEphemeridesCheckBox,     "PrintSky.printSSEphemerides");
	connectBoolProperty(ui->orientationPortraitRadioButton, "PrintSky.orientationPortrait");
	connectBoolProperty(ui->limitMagnitudeCheckBox,         "PrintSky.flagLimitMagnitude");
	connectDoubleProperty(ui->limitMagnitudeDoubleSpinBox,  "PrintSky.limitMagnitude");

	// Disable magnitude limit spinbox if magnitude limit is disabled.
	ui->limitMagnitudeDoubleSpinBox->setEnabled(GETSTELMODULE(PrintSky)->getFlagLimitMagnitude());
	connect(GETSTELMODULE(PrintSky), SIGNAL(flagLimitMagnitudeChanged(bool)), ui->limitMagnitudeDoubleSpinBox, SLOT(setEnabled(bool)));

	//Initialize the style
	updateStyle();
}

//! Draw contents report
void PrintSkyDialog::printDataSky(QPrinter * printer)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	PrintSky *plugin=GETSTELMODULE(PrintSky);
	Q_ASSERT(plugin);
	QPainter painter(printer);

	QImage img;
	// TODO: Decide whether custom screenshot size should be obeyed or ignored.
	if (!StelMainView::getInstance().getScreenshot(img, StelMainView::getInstance().getFlagUseCustomScreenshotSize(), plugin->getInvertColors(), false))
	{
		qWarning() << "PrintSky: screenshot retrieval failed. Aborting report.";
		return;
	}
	const int imageYPos=(plugin->getPrintData()? 400: 0);
	QSize sizeReal=printer->pageRect().size();
	sizeReal.setHeight(sizeReal.height()-imageYPos);
	if (plugin->getScaleToFit())
		img=img.scaled(sizeReal, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	const int imageXPos=(printer->pageRect().width()-img.width())/2;
	painter.drawImage(imageXPos, 0, img);

	StelCore* core = StelApp::getInstance().getCore();
	StelLocation location=core->getCurrentLocation();
	const double jd = core->getJD();

	const int fontsize = static_cast<int>(printer->pageRect().width())/60;
	font.setPixelSize(fontsize);
	painter.setFont(font);
	//int lineSpacing=fontsize+8;
	const int lineSpacing=fontsize*5/4;

	//qDebug() << "PrintSky: printer debugging information:";
	//qDebug() << "Current printer resolution:" << printer->resolution();
	//qDebug() << "Supported printer resolutions:" << printer->supportedResolutions();
	//qDebug() << "Page size (size index, 0-30)" << printer->paperSize();
	////For the paper size index, see http://doc.qt.nokia.com/qprinter.html#PaperSize-enum
	//qDebug() << "Font Pixel Size:" << font.pixelSize();
	//qDebug() << "Paper Rect: "<< printer->paperRect();
	//qDebug() << "Page Rect: "<< printer->pageRect();

	if (plugin->getPrintData())
	{
		int posY=img.height()+lineSpacing;

		QRect surfaceData(printer->pageRect().left(), posY, printer->pageRect().width(), printer->pageRect().height()-posY);

		painter.drawText(surfaceData.adjusted(0, 0, 0, -(surfaceData.height()-lineSpacing)), Qt::AlignCenter, q_("CHART INFORMATION"));

		QString printLatitude=StelUtils::radToDmsStr((std::fabs(static_cast<double>(location.latitude))/180.)*M_PI);
		QString printLongitude=StelUtils::radToDmsStr((std::fabs(static_cast<double>(location.longitude))/180.)*M_PI);

		QString locationStr = QString("%1: %2,\t%3,\t%4;\t%5\t%6\t%7%8")
							 .arg(q_("Location"))
							 .arg(location.name)
							 .arg(q_(location.region))
							 .arg(q_(location.planetName))
							 .arg(location.latitude<0 ? QString("%1S").arg(printLatitude) : QString("%1N").arg(printLatitude))
							 .arg(location.longitude<0 ? QString("%1W").arg(printLongitude) : QString("%1E").arg(printLongitude))
							 .arg(location.altitude)
							 .arg(qc_("m", "altitude, metres"));
		painter.drawText(surfaceData.adjusted(50, lineSpacing, 0, 0), Qt::AlignLeft, locationStr);

		// It seems when initializing this as non-pointer, the object gets deleted at end on this clause, causing a crash.
		StelLocaleMgr *lmgr = &StelApp::getInstance().getLocaleMgr();
		Q_ASSERT(lmgr);
		QString datetime = QString("%1: %2 %3 (%4)")
					.arg(q_("Local time"))
					.arg(lmgr->getPrintableDateLocal(jd))
					.arg(lmgr->getPrintableTimeLocal(jd))
					.arg(lmgr->getPrintableTimeZoneLocal(jd));

		painter.drawText(surfaceData.adjusted(50, (lineSpacing)*2, 0, 0), Qt::AlignLeft, datetime);

		QString str;
		QTextStream wos(&str);
		wos << q_("FOV: ") << qSetRealNumberPrecision(3) << core->getMovementMgr()->getCurrentFov() << QChar(0x00B0);
		painter.drawText(surfaceData.adjusted(50, (lineSpacing)*3, 0, 0), Qt::AlignLeft, *wos.string());

		painter.drawText(surfaceData.adjusted(surfaceData.width()-(15*fontsize), 0, 0, 0), Qt::AlignLeft, q_("Magnitude"));

		QList< QPair<float,float> > listPairsMagnitudesRadius=getListMagnitudeRadius(core);

		int xPos=-(12*fontsize), yPos=lineSpacing+10;
		for (int i=0; i<listPairsMagnitudesRadius.count(); ++i)
		{
			painter.drawText(surfaceData.adjusted(surfaceData.width()+xPos, yPos, 0, 0), Qt::AlignLeft, QString("%1").arg(listPairsMagnitudesRadius.at(i).first, 2));
			//painter.setBrush(Qt::SolidPattern);
			painter.setBrush(Qt::RadialGradientPattern); // FIXME: Attempt to make diffuse stars. Maybe replace by Qt:TexturePattern and setTexture()?
			painter.drawEllipse(QPoint(surfaceData.left() + surfaceData.width() + xPos - 40, surfaceData.top() + yPos + (fontsize/2)),
									  static_cast<int>(std::ceil(listPairsMagnitudesRadius.at(i).second)),
									  static_cast<int>(std::ceil(listPairsMagnitudesRadius.at(i).second)));
			yPos+=lineSpacing;
			if (yPos+lineSpacing>=surfaceData.height())
			{
				xPos+=200;
				yPos=lineSpacing+10;
			}
		}

		// TODO: twilight info, lunar phase or similar general information?
		// TODO: active meteor showers (check plugin state, get info)
	}

	// Print selected object information
	if (plugin->getFlagPrintObjectInfo() && GETSTELMODULE(StelObjectMgr)->getWasSelected())
	{

		QTextDocument info(GETSTELMODULE(StelObjectMgr)->getSelectedObject().at(0)->getInfoString(core).toHtmlEscaped());

		int posY=img.height()+lineSpacing;
		QRect surfaceData(printer->pageRect().left(), posY, printer->pageRect().width(), printer->pageRect().height()-posY);

		// TODO: print selected object info
		//painter.drawText(surfaceData.adjusted(0, 0, 0, -(surfaceData.height()-lineSpacing)),
		//		 ,
		//		 QTextOption());
		info.print(printer);
	}

	// Print solar system ephemerides
	if (plugin->getPrintSSEphemerides())
	{
		SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);

		PlanetP pHome=ssmgr->searchByEnglishName(location.planetName);
		//double standardSideralTime=pHome->getSiderealTime(((int) jd)+0.5)*M_PI/180.;
		//const double standardSiderealTime=pHome->getSiderealTime(floor(jd)+0.5, floor(jd)+0.5)*M_PI/180.;

		QStringList allBodiesNames=ssmgr->getAllPlanetEnglishNames();
		allBodiesNames.sort(); // FIXME: Add a function to sort by hierarchy.
		bool doHeader=true;
		int yPos=0;
		bool oddLine=false; // allows alternating line background color
		for (int iBodyName=0; iBodyName<allBodiesNames.count(); ++iBodyName)
		{
			QString englishName=allBodiesNames.at(iBodyName);
			PlanetP p=ssmgr->searchByEnglishName(englishName);
			if (p.data()->isHidden()) // exclude observers etc.
				continue;
			if ((p.data()->getPlanetType()==Planet::isComet) && !static_cast<KeplerOrbit*>(p.data()->getOrbit())->objectDateValid(jd)) // exclude comets far from perihel
				continue;
			double dec, ra;
			StelUtils::rectToSphe(&ra,&dec, p->getEquinoxEquatorialPos(core));

			Vec4d RTS=p.data()->getRTSTime(core);

			if ((!plugin->getFlagLimitMagnitude() || p->getVMagnitude(core) <= static_cast<float>(plugin->getLimitMagnitude()))
					&& englishName!=location.planetName)
			{
				const int ratioWidth=static_cast<int>(300.*static_cast<double>(fontsize)/45.);
				if (doHeader)
				{
					int xPos=printer->pageRect().left()-ratioWidth/2;
					yPos=70+(lineSpacing)*2;
					printer->newPage();
					painter.drawText(QRect(0, 0, printer->paperRect().width(), yPos), Qt::AlignCenter, q_("SOLAR SYSTEM EPHEMERIDES"));

					yPos+=(lineSpacing);

					painter.drawText(QRect(xPos+=    ratioWidth, yPos, 1.8*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("Name"));
					painter.drawText(QRect(xPos+=1.8*ratioWidth, yPos, 1.1*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("RA"));
					painter.drawText(QRect(xPos+=1.1*ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("Dec"));
					painter.drawText(QRect(xPos+=    ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("Rising"));
					painter.drawText(QRect(xPos, yPos-fontsize-lineSpacing/4, ratioWidth*2.1, fontsize+lineSpacing), Qt::AlignHCenter, q_("Local Time"));
					painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("Transit"));
					painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, q_("Setting"));
					painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignRight, q_("Dist.(AU)"));
					painter.drawText(QRect(xPos+=    ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignRight, q_("App.Mag."));
					yPos+=(lineSpacing)*1.25;
					doHeader=false;
				}

				const double transit=RTS[1];
				const double rising =RTS[0];
				const double setting=RTS[2];
				oddLine=!oddLine;
				int xPos=printer->pageRect().left()-ratioWidth/2;

				painter.setPen(Qt::NoPen);
				painter.setBrush(oddLine? Qt::white: Qt::lightGray);
				painter.drawRect(QRect(xPos+ratioWidth, yPos, ratioWidth*8, lineSpacing));
				painter.setPen(Qt::SolidLine);

				painter.drawText(QRect(xPos+=    ratioWidth, yPos, 1.8*ratioWidth, fontsize+lineSpacing), Qt::AlignLeft,   QString(" %1").arg(q_(englishName)));
				painter.drawText(QRect(xPos+=1.8*ratioWidth, yPos, 1.1*ratioWidth, fontsize+lineSpacing), Qt::AlignRight,  QString("%1").arg(StelUtils::radToHmsStr(ra)));
				painter.drawText(QRect(xPos+=1.1*ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignRight,  QString("%1").arg(StelUtils::radToDmsStr(dec)));
				painter.drawText(QRect(xPos+=    ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, QString("%1").arg(printableRTSTime(rising)));
				painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, QString("%1").arg(printableRTSTime(transit)));
				painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos, 0.7*ratioWidth, fontsize+lineSpacing), Qt::AlignHCenter, QString("%1").arg(printableRTSTime(setting)));
				painter.drawText(QRect(xPos+=0.7*ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignRight,  QString("%1").arg(p->getDistance(), 0, 'f', 3));
				painter.drawText(QRect(xPos+=    ratioWidth, yPos,     ratioWidth, fontsize+lineSpacing), Qt::AlignRight,  QString("%1 ").arg(p->getVMagnitude(core), 0, 'f', 1));

				yPos+=lineSpacing;
				if (yPos+((lineSpacing)*4)>=printer->pageRect().top()+printer->pageRect().height())
				{
				    doHeader=true;
				}
			    }
		}
	}
	QApplication::restoreOverrideCursor();
}

// Print report on a preview window
void PrintSkyDialog::previewSky()
{
	Q_ASSERT(gui);
	stelGuiVisible = gui->getVisible();
	gui->setVisible(false);
	dialog->setVisible(false);

	outputPreview = true;

	QTimer::singleShot(50, this, SLOT(executePrinterOutputOption()));
}

// Print report direct to printer
void PrintSkyDialog::printSky()
{
	Q_ASSERT(gui);
	stelGuiVisible=gui->getVisible();
	gui->setVisible(false);
	dialog->setVisible(false);

	outputPreview=false;

	QTimer::singleShot(50, this, SLOT(executePrinterOutputOption()));
}

// Read the printer parameters and run the output option selected (Print/Preview)
void PrintSkyDialog::executePrinterOutputOption()
{
	PrintSky *plugin=GETSTELMODULE(PrintSky);

	//QPrinter printer(QPrinter::HighResolution);
	QPrinter printer(QPrinter::ScreenResolution);
	printer.setResolution(300);
	printer.setDocName("STELLARIUM REPORT");
	printer.setOrientation(plugin->getOrientationPortrait() ? QPrinter::Portrait : QPrinter::Landscape);

	if (outputPreview)
	{
		QPrintPreviewDialog printPreviewDialog(&printer);
		connect(&printPreviewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(printDataSky(QPrinter *)));
		printPreviewDialog.exec();
	}
	else
	{
		//QPrintDialog dialogPrinter(&printer, &StelMainGraphicsView::getInstance());
		QPrintDialog dialogPrinter(&printer);
		if (dialogPrinter.exec() == QDialog::Accepted)
			printDataSky(&printer);
	}

	Q_ASSERT(gui);
	gui->setVisible(stelGuiVisible);
	// FIXME: what to do?
	//((QGraphicsWidget*)StelMainView::getInstance().getStelAppGraphicsWidget())->setFocus(Qt::OtherFocusReason);
}

void PrintSkyDialog::enableOutputOptions(bool enable)
{
	ui->buttonsFrame->setVisible(enable);
}

// time is given as fraction of day, 0..1.
QString PrintSkyDialog::printableTime(double time, double shift)
{
	time=fmod(time+shift/24.+1., 1.); // should ensure 0..1
	QTime tm(0,0);
	tm=tm.addSecs(static_cast<int>(time*86400.));
	return tm.toString("HH:mm");
}

// time is given as hours, 0..24. -100 means never rises/sets, 100 is circumpolar
QString PrintSkyDialog::printableRTSTime(double decimalHours)
{
	if (fabs(decimalHours)>99.)
		return q_("never");
	decimalHours=fmod(decimalHours+24., 24.); // should ensure 0..24
	QTime tm(0,0);
	tm=tm.addSecs(static_cast<int>(decimalHours*3600.));
	return tm.toString("HH:mm");
}

//! Get relations between magnitude stars and draw radius
QList< QPair<float, float> > PrintSkyDialog::getListMagnitudeRadius(StelCore *core)
{
    QList< QPair<float, float> > listPairsMagnitudesRadius;
    StelSkyDrawer* skyDrawer = core->getSkyDrawer();

    RCMag rcmag_table[1];
    for (int mag=-30; mag<100; ++mag)
    {
	skyDrawer->computeRCMag(mag,rcmag_table);
	if (rcmag_table[0].radius>0.f && rcmag_table[0].radius<13.f)
	{
	    QPair<float, float> pairMagnitudeRadius;
	    pairMagnitudeRadius.first=mag;
	    pairMagnitudeRadius.second=rcmag_table[0].radius;
	    listPairsMagnitudesRadius << pairMagnitudeRadius;
	}
    }

  return(listPairsMagnitudesRadius);
}
