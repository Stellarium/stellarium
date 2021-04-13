/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef CONFIGURATIONDIALOG_HPP
#define CONFIGURATIONDIALOG_HPP

#include <QObject>
#include <QNetworkReply>
#include <QFile>
#include "StelDialog.hpp"

class Ui_configurationDialogForm;
class QSettings;
class QDataStream;
class QNetworkAccessManager;
class QListWidgetItem;
class StelGui;
class CustomDeltaTEquationDialog;

class ConfigurationDialog : public StelDialog
{
	Q_OBJECT
public:
	ConfigurationDialog(StelGui* agui, QObject* parent);
	virtual ~ConfigurationDialog() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent() Q_DECL_OVERRIDE;
	Ui_configurationDialogForm* ui;

private:
	//! Contains the parsed content of the starsConfig.json file
	QVariantMap nextStarCatalogToDownload;
	//! Reset the content of the "Star catalog updates" box.
	//! Should be called only during initialization or
	//! after a download is complete.
	void resetStarCatalogControls();
	//! Re-translate the contents of the "Star calalogs" box.
	//! Update the strings according to the state.
	void updateStarCatalogControlsText();
	//! True if a star catalog download is in progress.
	bool isDownloadingStarCatalog;
	//! Value set by resetStarCatalogControls().
	int nextStarCatalogToDownloadIndex;
	//! Value set by resetStarCatalogControls().
	int starCatalogsCount;
	//! True when at least one star catalog has been downloaded successfully this session
	bool hasDownloadedStarCatalog;
	QNetworkReply* starCatalogDownloadReply;
	QFile* currentDownloadFile;
	class StelProgressController* progressBar;

private slots:
	void setNoSelectedInfo();
	void setAllSelectedInfo();
	void setBriefSelectedInfo();
	void setCustomSelectedInfo();
	//! Set the selected object info fields from the "Displayed Fields" boxes.
	//! Called when any of the boxes has been clicked. Sets the
	//! "selected info" mode to "Custom".
	void setSelectedInfoFromCheckBoxes();
	void saveCustomSelectedInfo();

	void updateCurrentLanguage();
	void updateCurrentSkyLanguage();
	void selectLanguage(const QString& languageCode);
	void selectSkyLanguage(const QString& languageCode);
	void setStartupTimeMode();
	//! Show/bring to foreground the shortcut editor window.
	void showShortcutsWindow();
	void setDiskViewport(bool);
	void setSphericMirror(bool);

	void newStarCatalogData();
	void downloadStars();
	void cancelDownload();
	void downloadFinished();
	void downloadError(QNetworkReply::NetworkError);
	void resetEphemControls();

	//! Update the labels displaying the current default state
	void updateConfigLabels();

	//! open a file dialog to browse for a directory to save screenshots to
	//! if a directory is selected (i.e. dialog not cancelled), current
	//! value will be changed, but not saved to config file.
	void browseForScreenshotDir();
	void selectScreenshotDir();

	//! Save the current viewing options including location and sky culture
	//! This doesn't include the current viewing direction, landscape, time and FOV since those
	//! have specific controls
	void saveAllSettings();
	//! Save the current view direction and field of view.
	void saveCurrentViewDirSettings();

	//! Reset all stellarium options.
	//! This basically replaces the config.ini by the default one
	void setDefaultViewOptions();

	void populatePluginsList();
	void pluginsSelectionChanged(QListWidgetItem *item, QListWidgetItem *previousItem);
	void pluginConfigureCurrentSelection();
	void loadAtStartupChanged(int);

	void populateDeltaTAlgorithmsList();
	void setDeltaTAlgorithm(int algorithmID);
	void setDeltaTAlgorithmDescription();
	void showCustomDeltaTEquationDialog();

	void populateDateFormatsList();
	void setDateFormat();

	void populateTimeFormatsList();
	void setTimeFormat();

	void setButtonBarDTFormat();

	void populateDitherList();
	void setDitherFormat();

	#ifdef ENABLE_SCRIPTING
	//! The selection of script in the script list has changed
	//! Updates the script information panel
	void scriptSelectionChanged(const QString& s);

	//! Run the currently selected script from the script list.
	void runScriptClicked();
	//! Stop the currently running script.
	void stopScriptClicked();

	void aScriptIsRunning();
	void aScriptHasStopped();

	void populateScriptsList();
	#endif
	void setFixedDateTimeToCurrent();

	void de430ButtonClicked();
	void de431ButtonClicked();

	//! feed the combo with useful values. Call in createDialogContent().
	void populateFontWritingSystemCombo();
	void handleFontBoxWritingSystem(int index);
	void populateScreenshotFileformatsCombo();

	void setKeyNavigationState(bool state);

private:
	StelGui* gui;

	CustomDeltaTEquationDialog* customDeltaTEquationDialog;

	int savedProjectionType;
	
	//! Set the displayed fields checkboxes from the current displayed fields.
	void updateSelectedInfoCheckBoxes();
	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();
};

#endif // CONFIGURATIONDIALOG_HPP
