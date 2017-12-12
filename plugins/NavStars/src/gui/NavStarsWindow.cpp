/*
 * Navigational Stars plug-in for Stellarium
 *
 * Copyright (C) 2016 Alexander Wolf
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NavStars.hpp"
#include "NavStarsWindow.hpp"
#include "ui_navStarsWindow.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"

#include <QComboBox>

NavStarsWindow::NavStarsWindow() : StelDialog("NavStars"), ns(Q_NULLPTR)
{
	ui = new Ui_navStarsWindowForm();
}

NavStarsWindow::~NavStarsWindow()
{
	delete ui;
}

void NavStarsWindow::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
		populateNavigationalStarsSets();
		populateNavigationalStarsSetDescription();
	}
}

void NavStarsWindow::createDialogContent()
{
	ns = GETSTELMODULE(NavStars);
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	populateNavigationalStarsSets();
	populateNavigationalStarsSetDescription();
	QString currentNsSetKey = ns->getCurrentNavigationalStarsSetKey();
	int idx = ui->nsSetComboBox->findData(currentNsSetKey, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use AngloAmerican as default
		idx = ui->nsSetComboBox->findData(QVariant("AngloAmerican"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->nsSetComboBox->setCurrentIndex(idx);
	connect(ui->nsSetComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setNavigationalStarsSet(int)));
	ui->displayAtStartupCheckBox->setChecked(ns->getEnableAtStartup());
	connect(ui->displayAtStartupCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setDisplayAtStartupEnabled(int)));

	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(saveSettings()));
	connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(resetSettings()));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
}

void NavStarsWindow::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Navigational Stars Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NAVSTARS_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NAVSTARS_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<p>" + q_("This plugin marks navigational stars from a selected set.");
	html += "<p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Navigational Stars plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin, its history and catalog format, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Navigational_Stars_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}

	ui->aboutTextBrowser->setHtml(html);
}

void NavStarsWindow::saveSettings()
{
	ns->saveConfiguration();
}

void NavStarsWindow::resetSettings()
{
	ns->restoreDefaultConfiguration();
}

void NavStarsWindow::setDisplayAtStartupEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	ns->setEnableAtStartup(b);
}

void NavStarsWindow::populateNavigationalStarsSets()
{
	Q_ASSERT(ui->nsSetComboBox);

	QComboBox* nsSets = ui->nsSetComboBox;

	//Save the current selection to be restored later
	nsSets->blockSignals(true);
	int index = nsSets->currentIndex();
	QVariant selectedNsSetId = nsSets->itemData(index);
	nsSets->clear();

	// TRANSLATORS: Part of full phrase: Anglo-American set of navigational stars
	nsSets->addItem(q_("Anglo-American"), "AngloAmerican");
	// TRANSLATORS: Part of full phrase: French set of navigational stars
	nsSets->addItem(q_("French"), "French");
	// TRANSLATORS: Part of full phrase: Russian set of navigational stars
	nsSets->addItem(q_("Russian"), "Russian");

	//Restore the selection
	index = nsSets->findData(selectedNsSetId, Qt::UserRole, Qt::MatchCaseSensitive);
	nsSets->setCurrentIndex(index);
	nsSets->blockSignals(false);
}

void NavStarsWindow::setNavigationalStarsSet(int nsSetID)
{
	QString currentNsSetID = ui->nsSetComboBox->itemData(nsSetID).toString();
	ns->setCurrentNavigationalStarsSetKey(currentNsSetID);
	populateNavigationalStarsSetDescription();
}

void NavStarsWindow::populateNavigationalStarsSetDescription(void)
{
	ui->nsSetDescription->setText(ns->getCurrentNavigationalStarsSetDescription());
}
