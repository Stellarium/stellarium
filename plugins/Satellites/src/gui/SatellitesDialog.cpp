/*
 * Stellarium Satellites plugin config dialog
 *
 * Copyright (C) 2009, 2012 Matthew Gates
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

#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTimer>
#include <QUrl>

#include "StelApp.hpp"
#include "ui_satellitesDialog.h"
#include "SatellitesDialog.hpp"
#include "SatellitesImportDialog.hpp"
#include "SatellitesListModel.hpp"
#include "SatellitesListFilterModel.hpp"
#include "Satellites.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelStyle.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"

SatellitesDialog::SatellitesDialog() :
    updateTimer(0),
    importWindow(0),
    filterModel(0),
    checkStateRole(Qt::UserRole)
{
	ui = new Ui_satellitesDialog;
	
	// FIXME: Cleanup.
	// satellitesModel = new QStandardItemModel(this);
}

SatellitesDialog::~SatellitesDialog()
{
	if (updateTimer)
	{
		updateTimer->stop();
		delete updateTimer;
		updateTimer = NULL;
	}

	if (importWindow)
	{
		delete importWindow;
		importWindow = 0;
	}

	delete ui;
}

void SatellitesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateSettingsPage(); // For the button; also calls updateCountdown()
		setAboutHtml();
		populateGroupsList();
	}
}

// Initialize the dialog widgets and connect the signals/slots
void SatellitesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	ui->labelAutoAdd->setVisible(false);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));
	Satellites* plugin = GETSTELMODULE(Satellites);

	// Settings tab / updates group
	// These controls are refreshed by updateSettingsPage(), which in
	// turn is triggered by setting any of these values. Because 
	// clicked() is issued only by user input, there's no endless loop.
	connect(ui->internetUpdatesCheckbox, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableInternetUpdates(bool)));
	connect(ui->checkBoxAutoAdd, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableAutoAdd(bool)));
	connect(ui->checkBoxAutoRemove, SIGNAL(clicked(bool)),
	        plugin, SLOT(enableAutoRemove(bool)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)),
	        plugin, SLOT(setUpdateFrequencyHours(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateTLEs()));
	connect(ui->jumpToSourcesButton, SIGNAL(clicked()),
	        this, SLOT(jumpToSourcesTab()));
	connect(plugin, SIGNAL(updateStateChanged(Satellites::UpdateState)),
	        this, SLOT(showUpdateState(Satellites::UpdateState)));
	connect(plugin, SIGNAL(tleUpdateComplete(int, int, int, int)),
	        this, SLOT(showUpdateCompleted(int, int, int, int)));

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
	updateTimer->start(7000);

	// Settings tab / General settings group
	// This does call Satellites::setFlagLabels() indirectly.
	QAction* action = dynamic_cast<StelGui*>(StelApp::getInstance().getGui())->getGuiAction("actionShow_Satellite_Labels");
	connect(ui->labelsGroup, SIGNAL(clicked(bool)),
	        action, SLOT(setChecked(bool)));
	connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)),
	        plugin, SLOT(setLabelFontSize(int)));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()),
	        this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()),
	        this, SLOT(saveSettings()));

	// Settings tab / orbit lines group
	connect(ui->orbitLinesGroup, SIGNAL(clicked(bool)),
	        plugin, SLOT(setOrbitLinesFlag(bool)));
	connect(ui->orbitSegmentsSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));
	connect(ui->orbitFadeSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));
	connect(ui->orbitDurationSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));
	
	// Settings tab - populate all values
	updateSettingsPage();

	// Satellites tab
	filterModel = new SatellitesListFilterModel(this);
	filterModel->setSourceModel(GETSTELMODULE(Satellites)->getSatellitesListModel());
	filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->satellitesList->setModel(filterModel);
	connect(ui->lineEditSearch, SIGNAL(textEdited(QString)),
	        filterModel, SLOT(setFilterWildcard(QString)));
	
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	connect(selectionModel,
	        SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
	        this,
	        SLOT(updateSatelliteData()));
	connect(ui->satellitesList, SIGNAL(doubleClicked(QModelIndex)),
	        this, SLOT(handleDoubleClick(QModelIndex)));
	
	// Two-state input, three-state display
	connect(ui->displayedCheckbox, SIGNAL(clicked(bool)),
	        ui->displayedCheckbox, SLOT(setChecked(bool)));
	connect(ui->orbitCheckbox, SIGNAL(clicked(bool)),
	        ui->orbitCheckbox, SLOT(setChecked(bool)));

	connect(ui->groupsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(filterListByGroup(int)));
	connect(ui->saveSatellitesButton, SIGNAL(clicked()), this, SLOT(saveSatellites()));
	connect(ui->removeSatellitesButton, SIGNAL(clicked()), this, SLOT(removeSatellites()));
	connectSatelliteGuiForm();
	
	importWindow = new SatellitesImportDialog();
	connect(ui->addSatellitesButton, SIGNAL(clicked()),
					importWindow, SLOT(setVisible()));
	connect(importWindow, SIGNAL(satellitesAccepted(TleDataList)),
					this, SLOT(addSatellites(TleDataList)));

	// Sources tab
	connect(ui->sourceList, SIGNAL(currentTextChanged(const QString&)), ui->sourceEdit, SLOT(setText(const QString&)));
	connect(ui->sourceList, SIGNAL(itemChanged(QListWidgetItem*)),
	        this, SLOT(saveSourceList()));
	connect(ui->sourceEdit, SIGNAL(editingFinished()),
	        this, SLOT(saveEditedSource()));
	connect(ui->deleteSourceButton, SIGNAL(clicked()), this, SLOT(deleteSourceRow()));
	connect(ui->addSourceButton, SIGNAL(clicked()), this, SLOT(addSourceRow()));
	connect(plugin, SIGNAL(settingsChanged()),
	        this, SLOT(toggleCheckableSources()));

	// About tab
	setAboutHtml();

	populateGroupsList();
	populateSourcesList();
}

void SatellitesDialog::filterListByGroup(int index)
{
	if (index < 0)
		return;
	
	QString groupId = ui->groupsCombo->itemData(index).toString();
	if (groupId == "all")
		filterModel->setSecondaryFilters(QString(), SatNoFlags);
	else if (groupId == "[displayed]")
		filterModel->setSecondaryFilters(QString(), SatDisplayed);
	else if (groupId == "[undisplayed]")
		filterModel->setSecondaryFilters(QString(), SatNotDisplayed);
	else if (groupId == "[newlyadded]")
		filterModel->setSecondaryFilters(QString(), SatNew);
	else if (groupId == "[orbiterror]")
		filterModel->setSecondaryFilters(QString(), SatError);
	else
	{
		filterModel->setSecondaryFilters(groupId, SatNoFlags);
	}
	
	if (ui->satellitesList->model()->rowCount() <= 0)
		return;
	
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndex first;
	if (selectionModel->hasSelection())
	{
		first = selectionModel->selectedRows().first();
	}
	else
	{
		// Scroll to the top
		first = ui->satellitesList->model()->index(0, 0);
	}
	selectionModel->setCurrentIndex(first, QItemSelectionModel::NoUpdate);
	ui->satellitesList->scrollTo(first);
}

void SatellitesDialog::updateSatelliteData()
{
	satelliteModified = false;

	QModelIndexList selection = ui->satellitesList->selectionModel()->selectedIndexes();
	if (selection.isEmpty())
		return; // TODO: Clear the fields?

	disconnectSatelliteGuiForm();
	
	if (selection.count() > 1)
	{
		ui->nameEdit->clear();
		ui->noradNumberEdit->clear();
		ui->descriptionTextEdit->clear();
		ui->tleFirstLineEdit->clear();
		ui->tleSecondLineEdit->clear();
	}
	else
	{
		QModelIndex& index = selection.first();
		ui->nameEdit->setText(index.data(Qt::DisplayRole).toString());
		ui->noradNumberEdit->setText(index.data(Qt::UserRole).toString());
		// NOTE: Description is deliberately displayed untranslated!
		ui->descriptionTextEdit->setText(index.data(SatDescriptionRole).toString());
		ui->tleFirstLineEdit->setText(index.data(FirstLineRole).toString());
		ui->tleFirstLineEdit->setCursorPosition(0);
		ui->tleSecondLineEdit->setText(index.data(SecondLineRole).toString());
		ui->tleSecondLineEdit->setCursorPosition(0);
	}
	
	// TODO: Fix the comms button...
//	ui->commsButton->setEnabled(sat->comms.count()>0);
	
	// Things that are cumulative in a multi-selection
	GroupSet partialGroups;
	GroupSet allGroups = GETSTELMODULE(Satellites)->getGroups();
	ui->displayedCheckbox->setChecked(false);
	ui->orbitCheckbox->setChecked(false);
	
	for (int i = 0; i < selection.size(); i++)
	{
		const QModelIndex& index = selection.at(i);
		
		// "Display" flag
		SatFlags flags = index.data(SatFlagsRole).value<SatFlags>();
		if (flags.testFlag(SatDisplayed))
		{
			if (!ui->displayedCheckbox->isChecked())
			{
				if (i == 0)
					ui->displayedCheckbox->setChecked(true);
				else
					ui->displayedCheckbox->setCheckState(Qt::PartiallyChecked);
			}
		}
		else
			if (ui->displayedCheckbox->isChecked())
				ui->displayedCheckbox->setCheckState(Qt::PartiallyChecked);
			
		// TODO: "Display Orbit" flag!
		// ui->orbitCheckbox->setChecked(sat->orbitDisplayed);
		
		// Accumulating groups
		GroupSet groups = index.data(SatGroupsRole).value<GroupSet>();
		partialGroups.unite(groups);
		allGroups.intersect(groups);
	}
	
	// TODO: Proper groups control
	QStringList translatedGroups;
	foreach (const QString& group, partialGroups)
	{
		translatedGroups.append(q_(group));
	}
	translatedGroups.sort();
	ui->groupsTextEdit->setText(translatedGroups.join(", "));
	
	connectSatelliteGuiForm();
}

void SatellitesDialog::saveSatellites(void)
{
	GETSTELMODULE(Satellites)->saveCatalog();
}

void SatellitesDialog::setAboutHtml()
{
	QString jsonFileName("<tt>satellites.json</tt>");
	QString oldJsonFileName("<tt>satellites.json.old</tt>");
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Stellarium Satellites Plugin") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + "</strong></td><td>" + SATELLITES_PLUGIN_VERSION + "</td></td>";
	html += "<tr><td rowspan=3><strong>" + q_("Authors") + "</strong></td><td>Matthew Gates &lt;matthewg42@gmail.com&gt;</td></td>";
	html += "<tr><td>Jose Luis Canales &lt;jlcanales.gasco@gmail.com&gt;</td></tr>";
	html += "<tr><td>Bogdan Marinov &lt;bogdan.marinov84@gmail.com&gt;</td></tr></table>";

	html += "<p>" + q_("The Satellites plugin predicts the positions of artificial satellites in Earth orbit.") + "</p>";

	html += "<h3>" + q_("Notes for users") + "</h3><p><ul>";
	html += "<li>" + q_("Satellites and their orbits are only shown when the observer is on Earth.") + "</li>";
	html += "<li>" + q_("Predicted positions are only good for a fairly short time (on the order of days, weeks or perhaps a month into the past and future). Expect high weirdness when looking at dates outside this range.") + "</li>";
	html += "<li>" + q_("Orbital elements go out of date pretty quickly (over mere weeks, sometimes days).  To get useful data out, you need to update the TLE data regularly.") + "</li>";
	// TRANSLATORS: The translated names of the button and the tab are filled in automatically. You can check the original names in Stellarium. File names are not translated.
	QString resetSettingsText = QString(q_("Clicking the \"%1\" button in the \"%2\" tab of this dialog will revert to the default %3 file.  The old file will be backed up as %4.  This can be found in the user data directory, under \"modules/Satellites/\"."))
			.arg(ui->restoreDefaultsButton->text())
			.arg(ui->tabs->tabText(ui->tabs->indexOf(ui->settingsTab)))
			.arg(jsonFileName)
			.arg(oldJsonFileName);
	html += "<li>" + resetSettingsText + "</li>";
	html += "<li>" + q_("The Satellites plugin is still under development.  Some features are incomplete, missing or buggy.") + "</li>";
	html += "</ul></p>";

	// TRANSLATORS: Title of a section in the About tab of the Satellites window
	html += "<h3>" + q_("TLE data updates") + "</h3>";
	html += "<p>" + q_("The Satellites plugin can automatically download TLE data from Internet sources, and by default the plugin will do this if the existing data is more than 72 hours old. ");
	html += "</p><p>" + QString(q_("If you disable Internet updates, you may update from a file on your computer.  This file must be in the same format as the Celestrak updates (see %1 for an example).").arg("<a href=\"http://celestrak.com/NORAD/elements/visual.txt\">visual.txt</a>"));
	html += "</p><p>" + q_("<b>Note:</b> if the name of a satellite in update data has anything in square brackets at the end, it will be removed before the data is used.");
	html += "</p>";

	html += "<h3>" + q_("Adding new satellites") + "</h3>";
	html += "<p>" + QString(q_("1. Make sure the satellite(s) you wish to add are included in one of the URLs listed in the Sources tab of the satellites configuration dialog. 2. Go to the Satellites tab, and click the '+' button.  Select the satellite(s) you wish to add and select the \"add\" button.")) + "</p>";

	html += "<h3>" + q_("Technical notes") + "</h3>";
	html += "<p>" + q_("Positions are calculated using the SGP4 & SDP4 methods, using NORAD TLE data as the input. ");
	html += q_("The orbital calculation code is written by Jose Luis Canales according to the revised Spacetrack Report #3 (including Spacetrack Report #6). ");
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += QString(q_("See %1this document%2 for details.")).arg("<a href=\"http://www.celestrak.com/publications/AIAA/2006-6753\">").arg("</a>") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Satellites plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	html += "</ul></p></body></html>";
	
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
	ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);

	ui->aboutTextBrowser->setHtml(html);
}

void SatellitesDialog::jumpToSourcesTab()
{
	ui->tabs->setCurrentWidget(ui->sourcesTab);
}

void SatellitesDialog::updateCountdown()
{
	Satellites* plugin = GETSTELMODULE(Satellites);
	bool updatesEnabled = plugin->getUpdatesEnabled();
	
	if (!updatesEnabled)
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
	else if (plugin->getUpdateState() == Satellites::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else
	{
		int secondsToUpdate = plugin->getSecondsToUpdate();
		if (secondsToUpdate <= 60)
			ui->nextUpdateLabel->setText(q_("Next update: < 1 minute"));
		else if (secondsToUpdate < 3600)
			ui->nextUpdateLabel->setText(QString(q_("Next update: %1 minutes")).arg((secondsToUpdate/60)+1));
		else
			ui->nextUpdateLabel->setText(QString(q_("Next update: %1 hours")).arg((secondsToUpdate/3600)+1));
	}
}

void SatellitesDialog::showUpdateState(Satellites::UpdateState state)
{
	if (state==Satellites::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Satellites::DownloadError || state==Satellites::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void SatellitesDialog::showUpdateCompleted(int updated,
                                           int total,
                                           int added,
                                           int missing)
{
	Satellites* plugin = GETSTELMODULE(Satellites);
	QString message;
	if (plugin->isAutoRemoveEnabled())
		message = q_("Updated %1/%2 satellite(s); %3 added; %4 removed");
	else
		message = q_("Updated %1/%2 satellite(s); %3 added; %4 missing");
	ui->nextUpdateLabel->setText(message.arg(updated).arg(total).arg(added).arg(missing));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(plugin->getLastUpdate());
	QTimer *timer = new QTimer(this); // FIXME: What's the point of this? --BM
	connect(timer, SIGNAL(timeout()), this, SLOT(updateCountdown()));
}

void SatellitesDialog::saveEditedSource()
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

	// Changes to item data (text or check state) are connected to
	// saveSourceList(), so there's no need to call it explicitly.
	if (ui->sourceList->currentItem()!=NULL)
		ui->sourceList->currentItem()->setText(u);
	else if (ui->sourceList->findItems(u, Qt::MatchExactly).count() <= 0)
	{
		QListWidgetItem* i = new QListWidgetItem(u, ui->sourceList);;
		i->setData(checkStateRole, Qt::Unchecked);
		i->setSelected(true);
	}
}

void SatellitesDialog::saveSourceList(void)
{
	QStringList allSources;
	for(int i=0; i<ui->sourceList->count(); i++)
	{
		QString url = ui->sourceList->item(i)->text();
		if (ui->sourceList->item(i)->data(checkStateRole) == Qt::Checked)
			url.prepend("1,");
		allSources << url;
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

void SatellitesDialog::toggleCheckableSources()
{
	QListWidget* list = ui->sourceList;
	if (list->count() < 1)
		return; // Saves effort checking it on every step
	
	bool enabled = ui->checkBoxAutoAdd->isChecked(); // proxy :)
	if (!enabled == list->item(0)->data(Qt::CheckStateRole).isNull())
		return; // Nothing to do
	
	ui->sourceList->blockSignals(true); // Prevents saving the list...
	for (int row = 0; row < list->count(); row++)
	{
		QListWidgetItem* item = list->item(row);
		if (enabled)
		{
			item->setData(Qt::CheckStateRole, item->data(Qt::UserRole));
		}
		else
		{
			item->setData(Qt::UserRole, item->data(Qt::CheckStateRole));
			item->setData(Qt::CheckStateRole, QVariant());
		}
	}
	ui->sourceList->blockSignals(false);
	
	checkStateRole = enabled ? Qt::CheckStateRole : Qt::UserRole;
}

void SatellitesDialog::restoreDefaults(void)
{
	qDebug() << "Satellites::restoreDefaults";
	GETSTELMODULE(Satellites)->restoreDefaults();
	GETSTELMODULE(Satellites)->loadSettings();
	updateSettingsPage();
	populateGroupsList();
	populateSourcesList();
}

void SatellitesDialog::updateSettingsPage()
{
	Satellites* plugin = GETSTELMODULE(Satellites);
	
	// Update stuff
	bool updatesEnabled = plugin->getUpdatesEnabled();
	ui->internetUpdatesCheckbox->setChecked(updatesEnabled);
	if(updatesEnabled)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));
	ui->checkBoxAutoAdd->setChecked(plugin->isAutoAddEnabled());
	ui->checkBoxAutoRemove->setChecked(plugin->isAutoRemoveEnabled());
	ui->lastUpdateDateTimeEdit->setDateTime(plugin->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(plugin->getUpdateFrequencyHours());
	
	updateCountdown();
	
	// Presentation stuff
	ui->labelsGroup->setChecked(plugin->getFlagLabels());
	ui->fontSizeSpinBox->setValue(plugin->getLabelFontSize());

	ui->orbitLinesGroup->setChecked(plugin->getOrbitLinesFlag());
	ui->orbitSegmentsSpin->setValue(Satellite::orbitLineSegments);
	ui->orbitFadeSpin->setValue(Satellite::orbitLineFadeSegments);
	ui->orbitDurationSpin->setValue(Satellite::orbitLineSegmentDuration);
}

void SatellitesDialog::populateGroupsList()
{
	// Save current selection, if any...
	QString selectedId;
	int index = ui->groupsCombo->currentIndex();
	if (ui->groupsCombo->count() > 0 && index >= 0)
	{
		selectedId = ui->groupsCombo->itemData(index).toString();
	}
	
	// Populate with group names/IDs
	ui->groupsCombo->clear();
	foreach (const QString& group, GETSTELMODULE(Satellites)->getGroupIdList())
	{
		ui->groupsCombo->addItem(q_(group), group);
	}
	ui->groupsCombo->model()->sort(0);
	
	// Add special groups - their IDs deliberately use JSON-incompatible chars.
	ui->groupsCombo->insertItem(0, q_("[orbit calculation error]"), QVariant("[orbiterror]"));
	ui->groupsCombo->insertItem(0, q_("[all newly added]"), QVariant("[newlyadded]"));
	ui->groupsCombo->insertItem(0, q_("[all not displayed]"), QVariant("[undisplayed]"));
	ui->groupsCombo->insertItem(0, q_("[all displayed]"), QVariant("[displayed]"));
	ui->groupsCombo->insertItem(0, q_("[all]"), QVariant("all"));
	
	// Restore current selection
	// Nothing is supposed to be changed, so prevent the list from re-filtering
	ui->groupsCombo->blockSignals(true);
	index = 0;
	if (!selectedId.isEmpty())
	{
		index = ui->groupsCombo->findData(selectedId);
		if (index < 0)
			index = 0;
	}
	ui->groupsCombo->setCurrentIndex(index);
	ui->groupsCombo->blockSignals(false);
}

void SatellitesDialog::populateSourcesList()
{
	ui->sourceList->blockSignals(true);
	ui->sourceList->clear();
	
	Satellites* plugin = GETSTELMODULE(Satellites);
	QStringList urls = plugin->getTleSources();
	checkStateRole = plugin->isAutoAddEnabled() ? Qt::CheckStateRole 
	                                            : Qt::UserRole;
	foreach (QString url, urls)
	{
		bool checked = false;
		if (url.startsWith("1,"))
		{
			checked = true;
			url.remove(0, 2);
		}
		else if (url.startsWith("0,"))
			url.remove(0, 2);
		QListWidgetItem* item = new QListWidgetItem(url, ui->sourceList);
		item->setData(checkStateRole, checked ? Qt::Checked : Qt::Unchecked);
	}
	ui->sourceList->blockSignals(false);
	
	if (ui->sourceList->count() > 0) ui->sourceList->setCurrentRow(0);
}

void SatellitesDialog::saveSettings(void)
{
	GETSTELMODULE(Satellites)->saveSettings();
	GETSTELMODULE(Satellites)->saveCatalog();
}

void SatellitesDialog::addSatellites(const TleDataList& newSatellites)
{
	GETSTELMODULE(Satellites)->add(newSatellites);
	saveSatellites();
	
	// Trigger re-loading the list to display the new satellites
	int index = ui->groupsCombo->findData(QVariant("[newlyadded]"));
	// TODO: Unnecessary once the model can handle changes? --BM
	if (ui->groupsCombo->currentIndex() == index)
		filterListByGroup(index);
	else
		ui->groupsCombo->setCurrentIndex(index); //Triggers the same operation
	
	// Select the satellites that were added just now
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	selectionModel->clearSelection();
	QModelIndex firstSelectedIndex;
	QSet<QString> newIds;
	foreach (const TleData& sat, newSatellites)
		newIds.insert(sat.id);
	for (int row = 0; row < ui->satellitesList->model()->rowCount(); row++)
	{
		QModelIndex index = ui->satellitesList->model()->index(row, 0);
		QString id = index.data(Qt::UserRole).toString();
		if (newIds.remove(id))
		{
			selectionModel->select(index, QItemSelectionModel::Select);
			if (!firstSelectedIndex.isValid())
				firstSelectedIndex = index;
		}
	}
	if (firstSelectedIndex.isValid())
		ui->satellitesList->scrollTo(firstSelectedIndex, QAbstractItemView::PositionAtTop);
	else
		ui->satellitesList->scrollToTop();
}

void SatellitesDialog::removeSatellites()
{
	QStringList idList;
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedRows();
	foreach (const QModelIndex& index, selectedIndexes)
	{
		QString id = index.data(Qt::UserRole).toString();
		idList.append(id);
	}
	if (!idList.isEmpty())
	{
		GETSTELMODULE(Satellites)->remove(idList);
		saveSatellites();
	}
}

void SatellitesDialog::setDisplayFlag(bool display)
{
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedRows();
	foreach (const QModelIndex& index, selectedIndexes)
	{
		QString id = index.data(Qt::UserRole).toString();
		SatelliteP sat = GETSTELMODULE(Satellites)->getById(id);
		if (sat)
			sat->displayed = display;
	}
}

void SatellitesDialog::setOrbitFlag(bool display)
{
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedRows();
	foreach (const QModelIndex& index, selectedIndexes)
	{
		QString id = index.data(Qt::UserRole).toString();
		SatelliteP sat = GETSTELMODULE(Satellites)->getById(id);
		if (sat)
			sat->orbitDisplayed = display;
	}
}

void SatellitesDialog::handleDoubleClick(const QModelIndex& index)
{
	Satellites* SatellitesMgr = GETSTELMODULE(Satellites);
	Q_ASSERT(SatellitesMgr);
	QString id = index.data(Qt::UserRole).toString();
	SatelliteP sat = SatellitesMgr->getById(id);
	if (sat.isNull())
		return;

	if (!sat->orbitValid)
		return;

	// Turn on Satellite rendering if it is not already on
	sat->displayed = true;

	// If Satellites are not currently displayed, make them visible.
	if (!SatellitesMgr->getFlagHints())
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		QAction* setHintsAction = gui->getGuiAction("actionShow_Satellite_Hints");
		Q_ASSERT(setHintsAction);
		setHintsAction->setChecked(true);
	}

	StelObjectP obj = qSharedPointerDynamicCast<StelObject>(sat);
	StelObjectMgr& objectMgr = StelApp::getInstance().getStelObjectMgr();
	if (objectMgr.setSelectedObject(obj))
	{
		GETSTELMODULE(StelMovementMgr)->autoZoomIn();
		GETSTELMODULE(StelMovementMgr)->setFlagTracking(true);
	}
}

void SatellitesDialog::setOrbitParams(void)
{
	Satellite::orbitLineSegments = ui->orbitSegmentsSpin->value();
	Satellite::orbitLineFadeSegments = ui->orbitFadeSpin->value();
	Satellite::orbitLineSegmentDuration = ui->orbitDurationSpin->value();
	GETSTELMODULE(Satellites)->recalculateOrbitLines();
}

void SatellitesDialog::updateTLEs(void)
{
	if(GETSTELMODULE(Satellites)->getUpdatesEnabled())
	{
		GETSTELMODULE(Satellites)->updateFromOnlineSources();
	}
	else
	{
		QStringList updateFiles = QFileDialog::getOpenFileNames(&StelMainGraphicsView::getInstance(),
									q_("Select TLE Update File"),
									StelFileMgr::getDesktopDir(),
									"*.*");
		GETSTELMODULE(Satellites)->updateFromFiles(updateFiles, false);
	}
}

void SatellitesDialog::connectSatelliteGuiForm(void)
{
	// make sure we don't connect more than once
	disconnectSatelliteGuiForm();
	connect(ui->displayedCheckbox, SIGNAL(clicked(bool)), this, SLOT(setDisplayFlag(bool)));
	connect(ui->orbitCheckbox, SIGNAL(clicked(bool)), this, SLOT(setOrbitFlag(bool)));
}

void SatellitesDialog::disconnectSatelliteGuiForm(void)
{
	disconnect(ui->displayedCheckbox, SIGNAL(clicked(bool)), this, SLOT(setDisplayFlag(bool)));
	disconnect(ui->orbitCheckbox, SIGNAL(clicked(bool)), this, SLOT(setOrbitFlag(bool)));
}
