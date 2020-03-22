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
#include "PrintSky.hpp"
#include <QAction>
#include <QOpenGLWidget>
#include <QPrintDialog>
#include <QTimer>
#include <QGraphicsWidget>

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StarMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"

#include <QDataWidgetMapper>
#include <QDebug>
#include <QFrame>
#include <QSettings>


PrintSkyDialog::PrintSkyDialog()
{
	ui = new Ui_printskyDialogForm;
}

PrintSkyDialog::~PrintSkyDialog()
{
	//These exist only if the window has been shown once:
	if (dialog)
	{
	}

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
		dialog->setStyleSheet(style);
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
	//connect(ui->invertColorsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(invertColorsStateChanged(int)));
	//connect(ui->scaleToFitCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleToFitStateChanged(int)));
	connect(ui->previewSkyPushButton, SIGNAL(clicked()), this, SLOT(previewSky()));
	connect(ui->printSkyPushButton, SIGNAL(clicked()), this, SLOT(printSky()));
	//connect(ui->orientationPortraitRadioButton, SIGNAL(toggled(bool)), this, SLOT(orientationStateChanged(bool)));
	//connect(ui->printDataCheckBox, SIGNAL(stateChanged(int)), this, SLOT(printDataStateChanged(int)));
	//connect(ui->printSSEphemeridesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(printSSEphemeridesStateChanged(int)));

	connectBoolProperty(ui->invertColorsCheckBox, "PrintSky.useInvertColors");
	connectBoolProperty(ui->scaleToFitCheckBox, "PrintSky.scaleToFit");
	connectBoolProperty(ui->printDataCheckBox, "PrintSky.printData");
	connectBoolProperty(ui->printSSEphemeridesCheckBox, "PrintSky.printSSEphemerides");
	// FIXME: Do something to select orientation.
	//connectBoolProperty(ui->scaleToFitCheckBox, "PrintSky.scaleToFit");


	// set the initial state
	try
		{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
//		bool useInvertColors = settings.value("use_invert_colors", false).toBool();
//		if (useInvertColors)
//		{
//			ui->invertColorsCheckBox->setCheckState(Qt::Checked);
//		}
//		bool useScaleToFit = settings.value("use_scale_to_fit", true).toBool();
//		if (useScaleToFit)
//		{
//			ui->scaleToFitCheckBox->setCheckState(Qt::Checked);
//		}
//		QString orientation=settings.value("orientation", "Portrait").toString();
//		if (orientation=="Portrait")
//			ui->orientationPortraitRadioButton->setChecked(true);
//		if (orientation=="Landscape")
//			ui->orientationLandscapeRadioButton->setChecked(true);
//		bool printData = settings.value("print_data", true).toBool();
//		if (printData)
//		{
//			ui->printDataCheckBox->setCheckState(Qt::Checked);
//		}
//		bool printSSEphemerides = settings.value("print_SS_ephemerides", true).toBool();
//		if (printSSEphemerides)
//		{
//			ui->printSSEphemeridesCheckBox->setCheckState(Qt::Checked);
//		}

	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}


	//Initialize the style
	updateStyle();
}

//! Print report on a preview window
void PrintSkyDialog::previewSky()
{
	currentVisibilityGui = gui->getVisible();
	gui->setVisible(false);
	dialog->setVisible(false);

	outputOption = true;

	QTimer::singleShot(50, this, SLOT(executePrinterOutputOption()));
}

//! Draw contents report
void PrintSkyDialog::printDataSky(QPrinter * printer)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	PrintSky *plugin=GETSTELMODULE(PrintSky);

	QPainter painter(printer);

	//QGLWidget* glQGLWidget=(QGLWidget *) StelMainView::getInstance().getStelQGLWidget();
	//Q_ASSERT(glQGLWidget);

	// FIXME: Get screenshot from StelMainView. TODO: implement a function there...
	QImage img; // =glQGLWidget->grabFrameBuffer();

	int imageYPos=(plugin->getPrintDataState()? 400: 0);

	QSize sizeReal=printer->pageRect().size();
	sizeReal.setHeight(sizeReal.height()-imageYPos);

	if (plugin->getScaleToFitState())
		img=img.scaled(sizeReal, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	if (plugin->getInvertColorsState())
		img.invertPixels();

	int imageXPos=(printer->pageRect().width()-img.width())/2;

	painter.drawImage(imageXPos, 0, img);

	StelCore* core = StelApp::getInstance().getCore();
	StelLocation locationData=core->getCurrentLocation();
	const double jd = core->getJD();

	int fontsize = (int)printer->pageRect().width()/60;
	font.setPixelSize(fontsize);
	painter.setFont(font);
	int lineSpacing=fontsize+5;

	//qDebug() << "PrintSky: printer debugging information:";
	//qDebug() << "Current printer resolution:" << printer->resolution();
	//qDebug() << "Supported printer resolutions:" << printer->supportedResolutions();
	//qDebug() << "Page size (size index, 0-30)" << printer->paperSize();
	//For the paper size index, see http://doc.qt.nokia.com/qprinter.html#PaperSize-enum
	//qDebug() << "Pixel Size:" << font.pixelSize();
	//qDebug() << "Paper Rect: "<< printer->paperRect();
	//qDebug() << "Page Rect: "<< printer->pageRect();

	if (plugin->getPrintDataState())
	{
		int posY=img.height()+lineSpacing;

		QRect surfaceData(printer->pageRect().left(), posY, printer->pageRect().width(), printer->pageRect().height()-posY);

		painter.drawText(surfaceData.adjusted(0, 0, 0, -(surfaceData.height()-lineSpacing)), Qt::AlignCenter, q_("CHART INFORMATION"));

		QString printLatitude=StelUtils::radToDmsStr((std::fabs(locationData.latitude)/180.)*M_PI);
		QString printLongitude=StelUtils::radToDmsStr((std::fabs(locationData.longitude)/180.)*M_PI);

		QString location = QString("%1: %2,\t%3,\t%4;\t%5\t%6\t%7%8")
							 .arg(q_("Location"))
							 .arg(locationData.name)
							 .arg(q_(locationData.region))
							 .arg(q_(locationData.planetName))
							 .arg(locationData.latitude<0 ? QString("%1S").arg(printLatitude) : QString("%1N").arg(printLatitude))
							 .arg(locationData.longitude<0 ? QString("%1W").arg(printLongitude) : QString("%1E").arg(printLongitude))
							 .arg(locationData.altitude)
							 .arg(q_("m"));
		painter.drawText(surfaceData.adjusted(50, lineSpacing, 0, 0), Qt::AlignLeft, location);

		StelLocaleMgr lmgr = StelApp::getInstance().getLocaleMgr();
		//QString datetime = QString("%1: %2 %3 (UTC%4%5)")
		//			.arg(q_("Local time"))
		//			.arg(lmgr.getPrintableDateLocal(jd))
		//			.arg(lmgr.getPrintableTimeLocal(jd))
		//			.arg(lmgr.getGMTShift(jd)>=0? '+':'-')
		//			.arg(lmgr.getGMTShift(jd));
		QString datetime = QString("%1: %2 %3 (%4)")
					.arg(q_("Local time"))
					.arg(lmgr.getPrintableDateLocal(jd))
					.arg(lmgr.getPrintableTimeLocal(jd))
					.arg(locationData.ianaTimeZone);

		painter.drawText(surfaceData.adjusted(50, (lineSpacing)*2, 0, 0), Qt::AlignLeft, datetime);

		QString str;
		QTextStream wos(&str);
		wos << q_("FOV: ") << qSetRealNumberPrecision(3) << core->getMovementMgr()->getCurrentFov() << QChar(0x00B0);
		painter.drawText(surfaceData.adjusted(50, (lineSpacing)*3, 0, 0), Qt::AlignLeft, *wos.string());

		painter.drawText(surfaceData.adjusted(surfaceData.width()-(15*fontsize), 0, 0, 0), Qt::AlignLeft, q_("Magnitude"));

		QList< QPair<float,float> > listPairsMagnitudesRadius=getListMagnitudeRadius(core);

		int xPos=-(12*fontsize), yPos=lineSpacing+10;
		for (int icount=1; icount<=listPairsMagnitudesRadius.count(); ++icount)
		{
			painter.drawText(surfaceData.adjusted(surfaceData.width()+xPos, yPos, 0, 0), Qt::AlignLeft, QString("%1").arg(listPairsMagnitudesRadius.at(icount-1).first));
			painter.setBrush(Qt::SolidPattern);
			painter.drawEllipse(QPoint(surfaceData.left() + surfaceData.width() + xPos - 40, surfaceData.top() + yPos + (fontsize/2)),
									  (int) std::ceil(listPairsMagnitudesRadius.at(icount - 1).second), 
									  (int) std::ceil(listPairsMagnitudesRadius.at(icount - 1).second));
			yPos+=lineSpacing;
			if (yPos+lineSpacing>=surfaceData.height())
			{
				xPos+=200;
				yPos=lineSpacing+10;
			}
		}
	}

		// Print solar system ephemerides
	if (plugin->getPrintSSEphemeridesState())
	{

		SolarSystem* ssmgr = GETSTELMODULE(SolarSystem);

		double geographicLongitude=-locationData.longitude*M_PI/180.;
		double geographicLatitude=locationData.latitude*M_PI/180.;

		PlanetP pHome=ssmgr->searchByEnglishName(locationData.planetName);
		//double standardSideralTime=pHome->getSiderealTime(((int) jd)+0.5)*M_PI/180.;
		double standardSideralTime=pHome->getSiderealTime(floor(jd)+0.5, floor(jd)+0.5)*M_PI/180.;

		QStringList allBodiesNames=ssmgr->getAllPlanetEnglishNames();
		allBodiesNames.sort();
		bool doHeader=true;
		int yPos;
		bool oddLine=false;
		for (int iBodyName=1; iBodyName<=allBodiesNames.count(); ++iBodyName)
		{
			QString englishName=allBodiesNames.at(iBodyName-1);
			PlanetP p=ssmgr->searchByEnglishName(englishName);
			double dec, ra;
			StelUtils::rectToSphe(&ra,&dec, p->getEquinoxEquatorialPos(core));
			double standardAltitude=-0.5667;
			if (englishName=="Sun")
				standardAltitude=-0.8333;
			if (englishName=="Moon")
				standardAltitude=0.125;
			standardAltitude*=M_PI/180.;

			double cosH=(std::sin(standardAltitude)-(std::sin(geographicLatitude)*std::sin(dec)))/(std::cos(geographicLatitude)*std::cos(dec));

			if (englishName!=locationData.planetName && cosH>=-1. && cosH<=1.)
			{

				if (doHeader)
				{

				    yPos=50;
				    printer->newPage();
				    painter.drawText(0, 0, printer->paperRect().width(), yPos, Qt::AlignCenter, q_("SOLAR SYSTEM EPHEMERIDES"));

				    int ratioWidth=300.*(double) fontsize/45.;
				    int xPos=printer->pageRect().left()-ratioWidth;;
				    yPos+=(lineSpacing)*2;				    

				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Name"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("RA"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Dec"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Rising"));
				    painter.drawText(QRect(xPos, yPos-fontsize-lineSpacing/4, ratioWidth*3, fontsize), Qt::AlignCenter, q_("Local Time"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Transit"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Setting"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Dist.(AU)"));
				    painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, q_("Ap.Mag."));
				    yPos+=(lineSpacing)*1.25;
				    doHeader=false;
				}


				double angleH=std::acos(cosH);
				double transit=((ra+geographicLongitude-standardSideralTime)/(2*M_PI));
				if (transit>1.)
					transit-=1.;
				if (transit<0)
					transit+=1.;

				double rising=transit-angleH/(2*M_PI);
				if (rising>1.)
					rising-=1.;
				if (rising<0.)
					rising+=1.;

				double setting=transit+angleH/(2*M_PI);
				if (setting>1.)
					setting-=1.;
				if (setting<0.)
					setting+=1.;

				int shift=0; // FIXME: StelApp::getInstance().getLocaleMgr().getGMTShift(jd);

				int ratioWidth=300.*(double) fontsize/45.;
				int xPos=printer->pageRect().left()-ratioWidth;

				oddLine=!oddLine;


				painter.setPen(Qt::NoPen);
				painter.setBrush(oddLine? Qt::white: Qt::lightGray);
				painter.drawRect(QRect(xPos+ratioWidth, yPos, ratioWidth*8, fontsize+lineSpacing));
				painter.setPen(Qt::SolidLine);

				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignLeft, QString(" %1").arg(q_(englishName)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignRight, QString("%1").arg(StelUtils::radToHmsStr(ra)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignRight, QString("%1").arg(StelUtils::radToDmsStr(dec)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, QString("%1").arg(printableTime(rising, shift)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, QString("%1").arg(printableTime(transit, shift)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignCenter, QString("%1").arg(printableTime(setting, shift)));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignRight, QString("%1").arg(p->getDistance(), 0, 'f', 5));
				painter.drawText(QRect(xPos+=ratioWidth, yPos, ratioWidth, fontsize), Qt::AlignRight, QString("%1 ").arg(p->getVMagnitude(core), 0, 'f', 3));

				yPos+=lineSpacing;
				if (yPos+((lineSpacing)*3)>=printer->pageRect().top()+printer->pageRect().height())
				    doHeader=true;
			    }
		}
	}
	QApplication::restoreOverrideCursor();
}

//! Print report direct to printer
void PrintSkyDialog::printSky()
{
	currentVisibilityGui=gui->getVisible();
	gui->setVisible(false);
	dialog->setVisible(false);

	outputOption=false;

	QTimer::singleShot(50, this, SLOT(executePrinterOutputOption()));

}

//! Read the printer parameters and run the output option selected (Print/Preview)
void PrintSkyDialog::executePrinterOutputOption()
{
	//Options for printing image.
	bool invertColorsOption=false;
	bool scaleToFitOption=true;
	orientationOption="Portrait";
	bool printDataOption=true;
	bool printSSEphemeridesOption=true;

	try
	{
		StelFileMgr::Flags flags = static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable);
		QString printskyIniPath = StelFileMgr::findFile("modules/PrintSky/", flags) + "printsky.ini";
		QSettings settings(printskyIniPath, QSettings::IniFormat);
		invertColorsOption=settings.value("use_invert_colors", false).toBool();
		scaleToFitOption=settings.value("use_scale_to_fit", true).toBool();
		orientationOption=settings.value("orientation", "Portrait").toString();
		printDataOption=settings.value("print_data", true).toBool();
		printSSEphemeridesOption=settings.value("print_SS_ephemerides", true).toBool();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "WARNING: unable to locate printsky.ini file or create a default one for PrintSky plugin: " << e.what();
	}


	//QPrinter printer(QPrinter::HighResolution);
	QPrinter printer(QPrinter::ScreenResolution);
	printer.setResolution(300);
	printer.setDocName("STELLARIUM REPORT");
	printer.setOrientation((orientationOption=="Portrait"? QPrinter::Portrait: QPrinter::Landscape));

	if (outputOption)
	{
		//QPrintPreviewDialog oPrintPreviewDialog(&printer, &StelMainGraphicsView::getInstance());
		QPrintPreviewDialog oPrintPreviewDialog(&printer);
		connect(&oPrintPreviewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(printDataSky(QPrinter *)));
		oPrintPreviewDialog.exec();
	}
	else
	{
		//QPrintDialog dialogPrinter(&printer, &StelMainGraphicsView::getInstance());
		QPrintDialog dialogPrinter(&printer);
		if (dialogPrinter.exec() == QDialog::Accepted)
			printDataSky(&printer);
	}

	gui->setVisible(currentVisibilityGui);
	// FIXME: what to do?
	//((QGraphicsWidget*)StelMainView::getInstance().getStelAppGraphicsWidget())->setFocus(Qt::OtherFocusReason);
}



void PrintSkyDialog::enableOutputOptions(bool enable)
{
	ui->buttonsFrame->setVisible(enable);
}


QString PrintSkyDialog::printableTime(double time, int shift)
{
	time*=24.;
	time+=shift;
	if (time>=24.)
		time-=24.;
	if (time<0)
		time+=24.;

	//int hour=(int) time;
	//time-=hour;
	//time*=60;
	//int minute=(int) time;
	//return(QString("%1:%2").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')));
	QTime tm(0,0);
	tm.addMSecs(static_cast<int>(time/24.*86400000.));
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
