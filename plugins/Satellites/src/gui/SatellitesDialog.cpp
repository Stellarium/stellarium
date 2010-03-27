/*
 * Stellarium Satellites plugin config dialog
 *
 * Copyright (C) 2009 Matthew Gates
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

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QUrl>

#include "StelApp.hpp"
#include <plugin_config.h>
#include "ui_satellitesDialog.h"
#include "SatellitesDialog.hpp"
#include "Satellites.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"

// When i18n is implemented, uncomment the StelTranslator.hpp include
// and remove the definition of q_
//#include "StelTranslator.hpp"
#define q_ QString

SatellitesDialog::SatellitesDialog() : updateTimer(NULL)
{
	ui = new Ui_satellitesDialog;
}

SatellitesDialog::~SatellitesDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}
	delete ui;
}

void SatellitesDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void SatellitesDialog::setStelStyle(const StelStyle & style)
{
	if(dialog)
	{
		const StelStyle pluginStyle = GETSTELMODULE(Satellites)->getModuleStyleSheet(style);
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(pluginStyle.htmlStyleSheet));
	}
}

// Initialize the dialog widgets and connect the signals/slots
void SatellitesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);

	// Settings tab / updates group
	connect(ui->updateFromInternetCheckbox, SIGNAL(toggled(bool)), this, SLOT(setUpdatesEnabled(bool)));
	refreshUpdateValues(); // fetch values for last updated and so on
	connect(ui->updateNowButton, SIGNAL(clicked()), GETSTELMODULE(Satellites), SLOT(updateTLEs()));
	connect(GETSTELMODULE(Satellites), SIGNAL(updateStateChanged(Satellites::UpdateState)), this, SLOT(updateStateReceiver(Satellites::UpdateState)));
	connect(GETSTELMODULE(Satellites), SIGNAL(TleUpdateComplete(int)), this, SLOT(updateCompleteReceiver(int)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
		updateTimer->start(7000);

	// Settings tab / General settings group
	connect(ui->showLabelsCheckbox, SIGNAL(toggled(bool)), StelApp::getInstance().getGui()->getGuiActions("actionShow_Satellite_Labels"), SLOT(setChecked(bool)));
	connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), GETSTELMODULE(Satellites), SLOT(setLabelFontSize(int)));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// Satellites tab
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->satellitesList, SIGNAL(currentTextChanged(const QString&)), this, SLOT(selectedSatelliteChanged(const QString&)));
	connect(ui->satellitesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(satelliteDoubleClick(QListWidgetItem*)));
	connect(ui->groupsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(groupFilterChanged(int)));
	connect(ui->showButton, SIGNAL(clicked()), this, SLOT(showSelectedSatellites()));
	connect(ui->hideButton, SIGNAL(clicked()), this, SLOT(hideSelectedSatellites()));
	connect(ui->visibleCheckbox, SIGNAL(stateChanged(int)), this, SLOT(visibleCheckChanged(int)));

	// Sources tab
	connect(ui->sourceList, SIGNAL(currentTextChanged(const QString&)), ui->sourceEdit, SLOT(setText(const QString&)));
	connect(ui->sourceEdit, SIGNAL(editingFinished()), this, SLOT(sourceEditingDone()));
	connect(ui->deleteSourceButton, SIGNAL(clicked()), this, SLOT(deleteSourceRow()));
	connect(ui->addSourceButton, SIGNAL(clicked()), this, SLOT(addSourceRow()));

	// About tab
	setAboutHtml();
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(QString(StelApp::getInstance().getCurrentStelStyle()->htmlStyleSheet));

	updateGuiFromSettings();

	//Initialize the style
	setStelStyle(*StelApp::getInstance().getCurrentStelStyle());
}

void SatellitesDialog::groupFilterChanged(int index)
{
	QString prevSelection;
	if (ui->satellitesList->currentItem())
		prevSelection = ui->satellitesList->currentItem()->text();

	ui->satellitesList->clear();
	if (ui->groupsCombo->itemData(index).toString() == "all")
		ui->satellitesList->insertItems(0,GETSTELMODULE(Satellites)->getSatellites());
	else if (ui->groupsCombo->itemData(index).toString() == "visible")
		ui->satellitesList->insertItems(0,GETSTELMODULE(Satellites)->getSatellites(QString(), Satellites::Visible));
	else if (ui->groupsCombo->itemData(index).toString() == "notvisible")
		ui->satellitesList->insertItems(0,GETSTELMODULE(Satellites)->getSatellites(QString(), Satellites::NotVisible));
	else
		ui->satellitesList->insertItems(0,GETSTELMODULE(Satellites)->getSatellites(ui->groupsCombo->currentText()));

	// If the previously selected item is still in the list after the update, select it,
	// else selected the first item in the list.
	QList<QListWidgetItem*> foundItems = ui->satellitesList->findItems(prevSelection, Qt::MatchExactly);
	if (foundItems.count() > 0 && !prevSelection.isEmpty())
	{
		foundItems.at(0)->setSelected(true);
		ui->satellitesList->scrollToItem(foundItems.at(0));
	}
	else if (ui->satellitesList->count() > 0)
	{
		ui->satellitesList->setCurrentRow(0);
		ui->satellitesList->scrollToTop();
	}

}

void SatellitesDialog::selectedSatelliteChanged(const QString& id)
{
	qDebug() << "SatellitesDialog::selectedSatelliteChanged for " << id;
	if (id.isEmpty() || id=="")
		return;

	satelliteModified = false;

	SatelliteP sat = GETSTELMODULE(Satellites)->getByID(id);
	if (!sat->initialized)
		return;

	ui->idLineEdit->setText(sat->designation);
	ui->descriptionTextEdit->setText(sat->description);
	ui->groupsTextEdit->setText(sat->groupIDs.join(", "));
	ui->tleTextEdit->setText(QString(sat->elements[1]) + "\n" + QString(sat->elements[2]));
	ui->visibleCheckbox->setChecked(sat->visible);
	ui->commsButton->setEnabled(sat->comms.count()>0);
}

void SatellitesDialog::saveSatellite(const QString& id)
{
	qDebug() << "Saving satellite" << id;
}

void SatellitesDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Stellarium Satellites Plugin") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td>" + q_("Version:") + "</td><td>" + PLUGIN_VERSION + "</td></td>";
	html += "<tr><td>" + q_("Author:") + "</td><td>Matthew Gates &lt;matthew@porpoisehead.net&gt;</td></td>";
	html += "<tr><td>" + q_("Website:") + "</td><td><a href=\"http://stellarium.org/\">stellarium.org</a></td></td></table>";
	html += "<p>";
	html += q_("This is the Satellites plugin for Stellarium. ");
	html += q_("Please leave feedback in the Stellarium forums, post bugs to the bug tracker and consider making a donation to Stellarium if you find this plugin to be useful or interesting.");
	html += q_("");
	html += "</body></html>";
	ui->aboutTextBrowser->setHtml(html);
}

void SatellitesDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Satellites)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(Satellites)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(Satellites)->getSecondsToUpdate();
	if (!GETSTELMODULE(Satellites)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Updates disabled"));
	else if (GETSTELMODULE(Satellites)->getUpdateState() == Satellites::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (secondsToUpdate <= 60)
		ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
	else if (secondsToUpdate < 3600)
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
	else
		ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
}

void SatellitesDialog::setUpdateValues(int hours)
{
	GETSTELMODULE(Satellites)->setUpdateFrequencyHours(hours);
	refreshUpdateValues();
}

void SatellitesDialog::setUpdatesEnabled(bool b)
{
	GETSTELMODULE(Satellites)->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	ui->updateNowButton->setEnabled(b);
}

void SatellitesDialog::updateStateReceiver(Satellites::UpdateState state)
{
	qDebug() << "SatellitesDialog::updateStateReceiver got a signal";
	if (state==Satellites::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Satellites::DownloadError || state==Satellites::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void SatellitesDialog::updateCompleteReceiver(int numUpdated)
{
	ui->nextUpdateLabel->setText(QString(q_("Updated %1 satellite(s)")).arg(numUpdated));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Satellites)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
}

void SatellitesDialog::close(void)
{
	qDebug() << "Closing Satellites Configure Dialog";
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	gui->getGuiActions("actionShow_Satellite_ConfigDialog")->setChecked(false);
}

void SatellitesDialog::sourceEditingDone(void)
{
	// don't update the currently selected item in the source list if the text is empty or not a valid URL.
	QString u = ui->sourceEdit->text();
	if (u.isEmpty() || u=="")
	{
		qDebug() << "SatellitesDialog::sourceEditingDone empty string - not saving";
		return;
	}

	if (!QUrl(u).isValid() || !u.contains("://"))
	{
		qDebug() << "SatellitesDialog::sourceEditingDone invalid URL - not saving : " << u;
		return;
	}

	if (ui->sourceList->currentItem()!=NULL)
		ui->sourceList->currentItem()->setText(u);
	else if (ui->sourceList->findItems(u, Qt::MatchExactly).count() <= 0)
	{
		QListWidgetItem* i = new QListWidgetItem(u, ui->sourceList);
		i->setSelected(true);
	}

	saveSourceList();
}

void SatellitesDialog::saveSourceList(void)
{
	QStringList allSources;
	for(int i=0; i<ui->sourceList->count(); i++)
	{
		allSources << ui->sourceList->item(i)->text();
	}
	GETSTELMODULE(Satellites)->setTleSources(allSources);
}

void SatellitesDialog::deleteSourceRow(void)
{
	ui->sourceEdit->clear();
	if (ui->sourceList->currentItem())
		delete ui->sourceList->currentItem();

	saveSourceList();
}

void SatellitesDialog::addSourceRow(void)
{
	ui->sourceList->setCurrentItem(NULL);
	ui->sourceEdit->setText(q_("[new source]"));
	ui->sourceEdit->selectAll();
	ui->sourceEdit->setFocus();
}

void SatellitesDialog::restoreDefaults(void)
{
	qDebug() << "Satellites::restoreDefaults";
	GETSTELMODULE(Satellites)->restoreDefaults();
	GETSTELMODULE(Satellites)->readSettingsFromConfig();
	updateGuiFromSettings();
}

void SatellitesDialog::updateGuiFromSettings(void)
{
	ui->updateFromInternetCheckbox->setChecked(GETSTELMODULE(Satellites)->getUpdatesEnabled());
	refreshUpdateValues();

	ui->showLabelsCheckbox->setChecked(GETSTELMODULE(Satellites)->getFlagLabels());
	ui->fontSizeSpinBox->setValue(GETSTELMODULE(Satellites)->getLabelFontSize());

	ui->groupsCombo->clear();
	ui->groupsCombo->addItems(GETSTELMODULE(Satellites)->getGroups());
	ui->groupsCombo->insertItem(0, q_("[all satellites]"), QVariant("all"));
	ui->groupsCombo->insertItem(0, q_("[all not visible]"), QVariant("notvisible"));
	ui->groupsCombo->insertItem(0, q_("[all visible]"), QVariant("visible"));
	ui->groupsCombo->setCurrentIndex(0);

	ui->sourceList->clear();
	ui->sourceList->addItems(GETSTELMODULE(Satellites)->getTleSources());
	if (ui->sourceList->count() > 0) ui->sourceList->setCurrentRow(0);
}

void SatellitesDialog::saveSettings(void)
{
	qDebug() << "Satellites::saveSettings not implemented yet!";
}

void SatellitesDialog::showSelectedSatellites(void)
{
	foreach (QListWidgetItem* i, ui->satellitesList->selectedItems())
	{
		SatelliteP sat = GETSTELMODULE(Satellites)->getByID(i->text());
		sat->visible = true;
	}
	groupFilterChanged(ui->groupsCombo->currentIndex());
}

void SatellitesDialog::hideSelectedSatellites(void)
{
	foreach (QListWidgetItem* i, ui->satellitesList->selectedItems())
	{
		SatelliteP sat = GETSTELMODULE(Satellites)->getByID(i->text());
		sat->visible = false;
	}
	groupFilterChanged(ui->groupsCombo->currentIndex());
}

void SatellitesDialog::visibleCheckChanged(int state)
{
	foreach (QListWidgetItem* i, ui->satellitesList->selectedItems())
	{
		SatelliteP sat = GETSTELMODULE(Satellites)->getByID(i->text());
		sat->visible = (state==Qt::Checked);
	}
	groupFilterChanged(ui->groupsCombo->currentIndex());
}

void SatellitesDialog::satelliteDoubleClick(QListWidgetItem* item)
{
	qDebug() << "SatellitesDialog::satelliteDoubleClick for " << item->text();
	GETSTELMODULE(Satellites)->getByID(item->text())->visible = true;
	if (StelApp::getInstance().getStelObjectMgr().findAndSelect(item->text()))
	{
		GETSTELMODULE(StelMovementMgr)->autoZoomIn();
		GETSTELMODULE(StelMovementMgr)->setFlagTracking(true);
	}
}

