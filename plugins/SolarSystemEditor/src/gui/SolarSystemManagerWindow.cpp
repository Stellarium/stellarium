/*
 * Solar System editor plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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

#include "SolarSystemEditor.hpp"

#include "SolarSystemManagerWindow.hpp"
#include "ui_solarSystemManagerWindow.h"

#include "MpcImportWindow.hpp"
#include "ManualImportWindow.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelFileMgr.hpp"
#include "StelMainView.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"

#include <QFileDialog>
#include <QMessageBox>

SolarSystemManagerWindow::SolarSystemManagerWindow()
	: StelDialog("SolarSystemEditor")
	, manualImportWindow(nullptr)
{
	ui = new Ui_solarSystemManagerWindow();
	mpcImportWindow = new MpcImportWindow();

	ssEditor = GETSTELMODULE(SolarSystemEditor);
}

SolarSystemManagerWindow::~SolarSystemManagerWindow()
{
	delete ui;

	if (mpcImportWindow)
		delete mpcImportWindow;
	if (manualImportWindow)
		delete manualImportWindow;
}

void SolarSystemManagerWindow::createDialogContent()
{
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->listWidgetObjects;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	//Signals
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->pushButtonCopyFile, SIGNAL(clicked()), this, SLOT(copyConfiguration()));
	connect(ui->pushButtonReplaceFile, SIGNAL(clicked()), this, SLOT(replaceConfiguration()));
	connect(ui->pushButtonAddFile, SIGNAL(clicked()), this, SLOT(addConfiguration()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(removeObjects()));
	connect(ui->pushButtonImportMPC, SIGNAL(clicked()), this, SLOT(newImportMPC()));
	//connect(ui->pushButtonManual, SIGNAL(clicked()), this, SLOT(newImportManual()));

	connect(ssEditor, SIGNAL(solarSystemChanged()), this, SLOT(populateSolarSystemList()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetSSOdefaults()));

	// bug #1350669 (https://bugs.launchpad.net/stellarium/+bug/1350669)
	connect(ui->listWidgetObjects, SIGNAL(currentRowChanged(int)), ui->listWidgetObjects, SLOT(repaint()));

	setAboutHtml();
	updateTexts();

	Q_ASSERT(mpcImportWindow);
	//Rebuild the list if any planets have been imported
	connect(mpcImportWindow, SIGNAL(objectsImported()), this, SLOT(populateSolarSystemList()));

	ui->lineEditUserFilePath->setText(ssEditor->getCustomSolarSystemFilePath());
	populateSolarSystemList();
}

void SolarSystemManagerWindow::updateTexts()
{
	//Solar System tab
	// TRANSLATORS: Appears as the text of hyperlinks linking to websites. :)
	QString linkText(q_("website"));
	QString linkCode = QString("<a href=\"https://www.minorplanetcenter.net/\">%1</a>").arg(linkText);
	       
	// TRANSLATORS: IAU = International Astronomical Union
	QString mpcText(q_("You can import comet and asteroid data formatted in the export formats of the IAU's Minor Planet Center (%1). You can import files with lists of objects, download such lists from the Internet or search the online Minor Planet and Comet Ephemeris Service (MPES)."));
	ui->labelMPC->setText(QString(mpcText).arg(linkCode));
}

void SolarSystemManagerWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		populateSolarSystemList();
		setAboutHtml();
		updateTexts();
	}
}

void SolarSystemManagerWindow::newImportMPC()
{
	Q_ASSERT(mpcImportWindow);

	mpcImportWindow->setVisible(true);
}

void SolarSystemManagerWindow::newImportManual()
{
	if (manualImportWindow == nullptr)
	{
		manualImportWindow = new ManualImportWindow();
		connect(manualImportWindow, SIGNAL(visibleChanged(bool)), this, SLOT(resetImportManual(bool)));
	}

	manualImportWindow->setVisible(true);
}

void SolarSystemManagerWindow::resetImportManual(bool show)
{
	//If the window is being displayed, do nothing
	if (show)
		return;

	if (manualImportWindow)
	{
		//TODO:Move this out of here!
		//Reload the list, in case there are new objects
		populateSolarSystemList();

		delete manualImportWindow;
		manualImportWindow = nullptr;

		//This window is in the background, bring it to the foreground
		dialog->setVisible(true);
	}
}

void SolarSystemManagerWindow::resetSSOdefaults()
{
	if (askConfirmation()) 
	{
		qDebug() << "permission to reset SSO to defaults...";
		ssEditor->resetSolarSystemToDefault();
	}
	else
		qDebug() << "SSO reset cancelled";
}

void SolarSystemManagerWindow::populateSolarSystemList()
{
	unlocalizedNames.clear();
	for (const auto& object : GETSTELMODULE(SolarSystem)->getAllMinorBodies())
	{
		// GZ new for 0.16: only insert objects which are minor bodies.
		unlocalizedNames.insert(object->getCommonNameI18n(), object->getCommonEnglishName());
	}

	ui->listWidgetObjects->clear();
	ui->listWidgetObjects->addItems(unlocalizedNames.keys());
	//No explicit sorting is necessary: sortingEnabled is set in the .ui
}

void SolarSystemManagerWindow::removeObjects()
{
	if (!ui->listWidgetObjects->selectedItems().isEmpty())
	{
		// we must disconnect the signal or else the list will be rebuilt after the first deletion.
		disconnect(ssEditor, SIGNAL(solarSystemChanged()), this, SLOT(populateSolarSystemList()));
		// This is slow for many objects.
		// TODO: For more than 50, it may be better to remove from ini file and reload all ini files.
		for (auto* item : ui->listWidgetObjects->selectedItems())
		{
			QString ssoI18nName = item->text();
			QString ssoEnglishName = unlocalizedNames.value(ssoI18nName);
			//qDebug() << ssoId;
			//TODO: Ask for confirmation first?
			ssEditor->removeSsoWithName(ssoEnglishName);
		}
		connect(ssEditor, SIGNAL(solarSystemChanged()), this, SLOT(populateSolarSystemList()));
		populateSolarSystemList();
	}
}

void SolarSystemManagerWindow::copyConfiguration()
{
	QString filePath = QFileDialog::getSaveFileName(&StelMainView::getInstance(),
							q_("Save the minor Solar System bodies as..."),
							QDir::homePath() + "/ssystem_minor.ini");

	const QFileInfo targetFile(filePath);
	// We must remove an existing file because QFile::copy() does not overwrite.
	// Note that at least on Windows, an existing and write-protected file has been identified by the previous dialog already,
	// so these QMessageBoxes will never be seen here.
	if (targetFile.exists() && targetFile.isWritable())
	{
		if (!QFile::remove(targetFile.absoluteFilePath()))
		{
			QMessageBox::information(&StelMainView::getInstance(), q_("Cannot overwrite"),
					 q_("Cannot remove existing file. Do you have permissions?"));
			return;
		}
	}

	if (!ssEditor->copySolarSystemConfigurationFileTo(filePath))
			QMessageBox::information(&StelMainView::getInstance(), q_("Cannot store"),
						 q_("File cannot be written. Do you have permissions?"));
}

void SolarSystemManagerWindow::replaceConfiguration()
{
	QString filter = q_("Configuration files");
	filter.append(" (*.ini)");
	QString filePath = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Select a file to replace the Solar System minor bodies"), QDir::homePath(), filter);
	ssEditor->replaceSolarSystemConfigurationFileWith(filePath);
}

void SolarSystemManagerWindow::addConfiguration()
{
	QString filter = q_("Configuration files");
	filter.append(" (*.ini)");
	QString filePath = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Select a file to add the Solar System minor bodies"), QDir::toNativeSeparators(StelFileMgr::getInstallationDir()+"/data/ssystem_1000comets.ini"), filter);
	ssEditor->addFromSolarSystemConfigurationFile(filePath);
}

void SolarSystemManagerWindow::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Solar System Editor") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + SOLARSYSTEMEDITOR_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + SOLARSYSTEMEDITOR_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Bogdan Marinov &lt;bogdan.marinov84@gmail.com&gt;</td></tr>";
	html += "<tr><td rowspan=2><strong>" + q_("Contributors") + ":</strong></td><td>Georg Zotti</td></tr>";
	html += "<tr><td>Alexander Wolf</td></tr>";
	html += "</table>";

	html += "<p>" + q_("An interface for adding asteroids and comets to Stellarium. It can download object lists from the Minor Planet Center's website and perform searches in its online database.") + "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Solar System Editor plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=nullptr)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}
