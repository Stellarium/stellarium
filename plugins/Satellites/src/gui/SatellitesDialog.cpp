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
    satellitesModel(0),
    filterProxyModel(0)
{
	ui = new Ui_satellitesDialog;
	
	satellitesModel = new QStandardItemModel(this);
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
		refreshUpdateValues();
		setAboutHtml();
		// This may be a problem if we add group name translations, as the
		// sorting order may be different. --BM
		int index = ui->groupsCombo->currentIndex();
		populateGroupsList();
		ui->groupsCombo->setCurrentIndex(index);
	}
}

// Initialize the dialog widgets and connect the signals/slots
void SatellitesDialog::createDialogContent()
{
	ui->setupUi(dialog);
	ui->tabs->setCurrentIndex(0);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
					this, SLOT(retranslate()));

	// Settings tab / updates group
	connect(ui->internetUpdatesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(setUpdatesEnabled(int)));
	connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(updateTLEs()));
	connect(GETSTELMODULE(Satellites), SIGNAL(updateStateChanged(Satellites::UpdateState)), this, SLOT(updateStateReceiver(Satellites::UpdateState)));
	connect(GETSTELMODULE(Satellites), SIGNAL(tleUpdateComplete(int, int, int)), this, SLOT(updateCompleteReceiver(int, int, int)));
	connect(ui->updateFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(setUpdateValues(int)));
	refreshUpdateValues(); // fetch values for last updated and so on
	// if the state didn't change, setUpdatesEnabled will not be called, so we force it
	setUpdatesEnabled(ui->internetUpdatesCheckbox->checkState());

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
	updateTimer->start(7000);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// Settings tab / General settings group
	connect(ui->labelsGroup, SIGNAL(toggled(bool)), dynamic_cast<StelGui*>(StelApp::getInstance().getGui())->getGuiAction("actionShow_Satellite_Labels"), SLOT(setChecked(bool)));
	connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), GETSTELMODULE(Satellites), SLOT(setLabelFontSize(int)));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	connect(ui->saveSettingsButton, SIGNAL(clicked()), this, SLOT(saveSettings()));

	// Settings tab / orbit lines group
	ui->orbitLinesGroup->setChecked(GETSTELMODULE(Satellites)->getOrbitLinesFlag());
	ui->orbitSegmentsSpin->setValue(Satellite::orbitLineSegments);
	ui->orbitFadeSpin->setValue(Satellite::orbitLineFadeSegments);
	ui->orbitDurationSpin->setValue(Satellite::orbitLineSegmentDuration);

	connect(ui->orbitLinesGroup, SIGNAL(toggled(bool)), GETSTELMODULE(Satellites), SLOT(setOrbitLinesFlag(bool)));
	connect(ui->orbitSegmentsSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));
	connect(ui->orbitFadeSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));
	connect(ui->orbitDurationSpin, SIGNAL(valueChanged(int)), this, SLOT(setOrbitParams()));


	// Satellites tab
	filterProxyModel = new QSortFilterProxyModel(this);
	filterProxyModel->setSourceModel(satellitesModel);
	filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->satellitesList->setModel(filterProxyModel);
	connect(ui->lineEditSearch, SIGNAL(textEdited(QString)),
	        filterProxyModel, SLOT(setFilterWildcard(QString)));
	
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	connect(selectionModel, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
	        this, SLOT(updateSelectedInfo(QModelIndex,QModelIndex)));
	connect(ui->satellitesList, SIGNAL(doubleClicked(QModelIndex)),
	        this, SLOT(handleDoubleClick(QModelIndex)));
	connect(ui->groupsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(listSatelliteGroup(int)));
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
	connect(ui->sourceEdit, SIGNAL(editingFinished()), this, SLOT(sourceEditingDone()));
	connect(ui->deleteSourceButton, SIGNAL(clicked()), this, SLOT(deleteSourceRow()));
	connect(ui->addSourceButton, SIGNAL(clicked()), this, SLOT(addSourceRow()));

	// About tab
	setAboutHtml();

	updateGuiFromSettings();

}

void SatellitesDialog::listSatelliteGroup(int index)
{
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedRows();
	QVariantList prevMultiSelection;
	foreach (const QModelIndex& index, selectedIndexes)
	{
		prevMultiSelection << index.data(Qt::UserRole);
	}

	satellitesModel->clear();
	
	QHash<QString,QString> satellites;
	Satellites* plugin = GETSTELMODULE(Satellites);
	QString selectedGroup = ui->groupsCombo->itemData(index).toString();
	if (selectedGroup == "all")
		satellites = plugin->getSatellites();
	else if (selectedGroup == "visible")
		satellites = plugin->getSatellites(QString(), Satellites::Visible);
	else if (selectedGroup == "notvisible")
		satellites = plugin->getSatellites(QString(), Satellites::NotVisible);
	else if (selectedGroup == "newlyadded")
		satellites = plugin->getSatellites(QString(), Satellites::NewlyAdded);
	else if (selectedGroup == "orbiterror")
		satellites = plugin->getSatellites(QString(), Satellites::OrbitError);
	else
		satellites = plugin->getSatellites(ui->groupsCombo->currentText());
	
	//satellitesModel->->setSortingEnabled(false);
	QHashIterator<QString,QString> i(satellites);
	while (i.hasNext())
	{
		i.next();
		QStandardItem* item = new QStandardItem(i.value());
		item->setData(i.key(), Qt::UserRole);
		item->setEditable(false);
		satellitesModel->appendRow(item);
		
		// If a previously selected item is still in the list after the update, select it
		if (prevMultiSelection.contains(i.key()))
		{
			QModelIndex index = filterProxyModel->mapFromSource(item->index());
			//QModelIndex index = item->index();
			selectionModel->select(index, QItemSelectionModel::SelectCurrent);
		}
	}
	// Sort the main list (don't sort the filter model directly,
	// or the displayed list will be scrambled on emptying the filter).
	satellitesModel->sort(0);

	if (selectionModel->hasSelection())
	{
		//TODO: This is stupid...
		for (int row = 0; row < ui->satellitesList->model()->rowCount(); row++)
		{
			QModelIndex index = ui->satellitesList->model()->index(row, 0);
			if (selectionModel->isSelected(index))
			{
				ui->satellitesList->scrollTo(index, QAbstractItemView::PositionAtTop);
				break;
			}
		}
	}
	else if (ui->satellitesList->model()->rowCount() > 0)
	{
		// If there are any items in the listbox, scroll to the top
		QModelIndex index = ui->satellitesList->model()->index(0, 0);
		selectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
		ui->satellitesList->scrollTo(index, QAbstractItemView::PositionAtTop);
	}
}

void SatellitesDialog::reloadSatellitesList()
{
	listSatelliteGroup(ui->groupsCombo->currentIndex());
}

void SatellitesDialog::updateSelectedInfo(const QModelIndex& curItem,
                                          const QModelIndex& prevItem)
{
	Q_UNUSED(prevItem);
	if (!curItem.isValid())
		return;
	
	QString id = curItem.data(Qt::UserRole).toString();
	if (id.isEmpty())
		return;

	satelliteModified = false;

	SatelliteP sat = GETSTELMODULE(Satellites)->getByID(id);
	if (sat.isNull())
		return;

	if (!sat->initialized)
		return;

	disconnectSatelliteGuiForm();
	ui->idLineEdit->setText(sat->name);
	ui->lineEditCatalogNumber->setText(sat->id);
	ui->descriptionTextEdit->setText(sat->description);
	ui->groupsTextEdit->setText(sat->groupIDs.join(", "));
	QString tleStr = QString("%1\n%2").arg(sat->tleElements.first.data()).arg(sat->tleElements.second.data());
	ui->tleTextEdit->setText(tleStr);
	ui->visibleCheckbox->setChecked(sat->visible);
	ui->orbitCheckbox->setChecked(sat->orbitVisible);
	ui->commsButton->setEnabled(sat->comms.count()>0);
	connectSatelliteGuiForm();
}

void SatellitesDialog::saveSatellites(void)
{
	GETSTELMODULE(Satellites)->saveTleData();
}

void SatellitesDialog::setAboutHtml(void)
{
	QString jsonFileName("<tt>satellites.json</tt>");
	QString oldJsonFileName("<tt>satellites.json.old</tt>");
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Stellarium Satellites Plugin") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + "</strong></td><td>" + SATELLITES_PLUGIN_VERSION + "</td></td>";
	html += "<tr><td rowspan=2><strong>" + q_("Authors") + "</strong></td><td>Matthew Gates &lt;matthewg42@gmail.com&gt;</td></td>";
	html += "<tr><td>Jose Luis Canales &lt;jlcanales.gasco@gmail.com&gt;</td></tr></table>";

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

void SatellitesDialog::refreshUpdateValues(void)
{
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Satellites)->getLastUpdate());
	ui->updateFrequencySpinBox->setValue(GETSTELMODULE(Satellites)->getUpdateFrequencyHours());
	int secondsToUpdate = GETSTELMODULE(Satellites)->getSecondsToUpdate();
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Satellites)->getUpdatesEnabled());
	if (!GETSTELMODULE(Satellites)->getUpdatesEnabled())
		ui->nextUpdateLabel->setText(q_("Internet updates disabled"));
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

void SatellitesDialog::setUpdatesEnabled(int checkState)
{
	bool b = checkState != Qt::Unchecked;
	GETSTELMODULE(Satellites)->setUpdatesEnabled(b);
	ui->updateFrequencySpinBox->setEnabled(b);
	if(b)
		ui->updateButton->setText(q_("Update now"));
	else
		ui->updateButton->setText(q_("Update from files"));

	refreshUpdateValues();
}

void SatellitesDialog::updateStateReceiver(Satellites::UpdateState state)
{
	//qDebug() << "SatellitesDialog::updateStateReceiver got a signal";
	if (state==Satellites::Updating)
		ui->nextUpdateLabel->setText(q_("Updating now..."));
	else if (state==Satellites::DownloadError || state==Satellites::OtherError)
	{
		ui->nextUpdateLabel->setText(q_("Update error"));
		updateTimer->start();  // make sure message is displayed for a while...
	}
}

void SatellitesDialog::updateCompleteReceiver(int numUpdated, int total, int missing)
{
	ui->nextUpdateLabel->setText(QString(q_("Updated %1/%2 satellite(s); %3 missing")).arg(numUpdated).arg(total).arg(missing));
	// display the status for another full interval before refreshing status
	updateTimer->start();
	ui->lastUpdateDateTimeEdit->setDateTime(GETSTELMODULE(Satellites)->getLastUpdate());
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(refreshUpdateValues()));
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
	ui->internetUpdatesCheckbox->setChecked(GETSTELMODULE(Satellites)->getUpdatesEnabled());
	refreshUpdateValues();

	ui->labelsGroup->setChecked(GETSTELMODULE(Satellites)->getFlagLabels());
	ui->fontSizeSpinBox->setValue(GETSTELMODULE(Satellites)->getLabelFontSize());

	ui->orbitLinesGroup->setChecked(GETSTELMODULE(Satellites)->getOrbitLinesFlag());
	ui->orbitSegmentsSpin->setValue(Satellite::orbitLineSegments);
	ui->orbitFadeSpin->setValue(Satellite::orbitLineFadeSegments);
	ui->orbitDurationSpin->setValue(Satellite::orbitLineSegmentDuration);

	populateGroupsList();
	ui->satellitesList->clearSelection();
	ui->groupsCombo->setCurrentIndex(0);

	ui->sourceList->clear();
	ui->sourceList->addItems(GETSTELMODULE(Satellites)->getTleSources());
	if (ui->sourceList->count() > 0) ui->sourceList->setCurrentRow(0);
}

void SatellitesDialog::populateGroupsList()
{
	ui->groupsCombo->clear();
	ui->groupsCombo->addItems(GETSTELMODULE(Satellites)->getGroups());
	ui->groupsCombo->insertItem(0, q_("[orbit calculation error]"), QVariant("orbiterror"));
	ui->groupsCombo->insertItem(0, q_("[all newly added]"), QVariant("newlyadded"));
	ui->groupsCombo->insertItem(0, q_("[all not displayed]"), QVariant("notvisible"));
	ui->groupsCombo->insertItem(0, q_("[all displayed]"), QVariant("visible"));
	ui->groupsCombo->insertItem(0, q_("[all]"), QVariant("all"));
}

void SatellitesDialog::saveSettings(void)
{
	GETSTELMODULE(Satellites)->saveSettingsToConfig();
	GETSTELMODULE(Satellites)->saveTleData();
}

void SatellitesDialog::addSatellites(const TleDataList& newSatellites)
{
	GETSTELMODULE(Satellites)->add(newSatellites);
	saveSatellites();
	
	// Trigger re-loading the list to display the new satellites
	int index = ui->groupsCombo->findData(QVariant("newlyadded"));
	if (ui->groupsCombo->currentIndex() == index)
		listSatelliteGroup(index);
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
		reloadSatellitesList();
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
		SatelliteP sat = GETSTELMODULE(Satellites)->getByID(id);
		if (sat)
			sat->visible = display;
	}
	reloadSatellitesList();
}

void SatellitesDialog::setOrbitFlag(bool display)
{
	QItemSelectionModel* selectionModel = ui->satellitesList->selectionModel();
	QModelIndexList selectedIndexes = selectionModel->selectedRows();
	foreach (const QModelIndex& index, selectedIndexes)
	{
		QString id = index.data(Qt::UserRole).toString();
		SatelliteP sat = GETSTELMODULE(Satellites)->getByID(id);
		if (sat)
			sat->orbitVisible = display;
	}
	reloadSatellitesList();
}

void SatellitesDialog::handleDoubleClick(const QModelIndex& index)
{
	Satellites* SatellitesMgr = GETSTELMODULE(Satellites);
	Q_ASSERT(SatellitesMgr);
	QString id = index.data(Qt::UserRole).toString();
	SatelliteP sat = SatellitesMgr->getByID(id);
	if (sat.isNull())
		return;

	if (!sat->orbitValid)
		return;

	// Turn on Satellite rendering if it is not already on
	sat->visible = true;

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
		GETSTELMODULE(Satellites)->updateTLEs();
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
	connect(ui->visibleCheckbox, SIGNAL(clicked(bool)), this, SLOT(setDisplayFlag(bool)));
	connect(ui->orbitCheckbox, SIGNAL(clicked(bool)), this, SLOT(setOrbitFlag(bool)));
}

void SatellitesDialog::disconnectSatelliteGuiForm(void)
{
	disconnect(ui->visibleCheckbox, SIGNAL(clicked(bool)), this, SLOT(setDisplayFlag(bool)));
	disconnect(ui->orbitCheckbox, SIGNAL(clicked(bool)), this, SLOT(setOrbitFlag(bool)));
}
