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
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"

#include <QFileDialog>

SolarSystemManagerWindow::SolarSystemManagerWindow()
	: StelDialog("SolarSystemEditor")
	, manualImportWindow(Q_NULLPTR)
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
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
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
	if (manualImportWindow == Q_NULLPTR)
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
		manualImportWindow = Q_NULLPTR;

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
	if (ui->listWidgetObjects->selectedItems().length()>0)
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
	QString filePath = QFileDialog::getSaveFileName(Q_NULLPTR,
							q_("Save the minor Solar System bodies as..."),
							QDir::homePath() + "/ssystem_minor.ini");
	ssEditor->copySolarSystemConfigurationFileTo(filePath);
}

void SolarSystemManagerWindow::replaceConfiguration()
{
	QString filter = q_("Configuration files");
	filter.append(" (*.ini)");
	QString filePath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Select a file to replace the Solar System minor bodies"), QDir::homePath(), filter);
	ssEditor->replaceSolarSystemConfigurationFileWith(filePath);
}

void SolarSystemManagerWindow::addConfiguration()
{
	QString filter = q_("Configuration files");
	filter.append(" (*.ini)");
	QString filePath = QFileDialog::getOpenFileName(Q_NULLPTR, q_("Select a file to add the Solar System minor bodies"), QDir::toNativeSeparators(StelFileMgr::getInstallationDir()+"/data/ssystem_1000comets.ini"), filter);
	ssEditor->addFromSolarSystemConfigurationFile(filePath);
}

void SolarSystemManagerWindow::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Solar System Editor") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + SOLARSYSTEMEDITOR_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + SOLARSYSTEMEDITOR_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Bogdan Marinov &lt;bogdan.marinov84@gmail.com&gt;</td></tr>";
	html += "<tr><td rowspan=2><strong>" + q_("Contributors") + ":</strong></td><td>Georg Zotti</td></tr>";
	html += "<tr><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("An interface for adding asteroids and comets to Stellarium. It can download object lists from the Minor Planet Center's website and perform searches in its online database.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Solar System Editor plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Solar_System_Editor_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}
