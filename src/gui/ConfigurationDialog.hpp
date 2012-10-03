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

#ifndef _CONFIGURATIONDIALOG_HPP_
#define _CONFIGURATIONDIALOG_HPP_

#include <QObject>
#include <QProgressBar>
#include <QNetworkReply>
#include <QFile>
#include "StelDialog.hpp"

class Ui_configurationDialogForm;
class QSettings;
class QDataStream;
class QNetworkAccessManager;
class QListWidgetItem;
class StelGui;

class ConfigurationDialog : public StelDialog
{
	Q_OBJECT
public:
	ConfigurationDialog(StelGui* agui);
	virtual ~ConfigurationDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();
	void updateIconsColor();

protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
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
	QProgressBar* progressBar;

private slots:
	void setNoSelectedInfo();
	void setAllSelectedInfo();
	void setBriefSelectedInfo();
	//! Set the selected object info fields from the "Displayed Fields" boxes.
	//! Called when any of the boxes has been clicked. Sets the
	//! "selected info" mode to "Custom".
	void setSelectedInfoFromCheckBoxes();
	
	void selectLanguage(const QString& languageCode);
	void setStartupTimeMode();
	//! Show/bring to foreground the shortcut editor window.
	void showShortcutsWindow();
	void setDiskViewport(bool);
	void setSphericMirror(bool);
	void cursorTimeOutChanged();
	void cursorTimeOutChanged(double) {cursorTimeOutChanged();}

	void newStarCatalogData();
	void downloadStars();
	void cancelDownload();
	void downloadFinished();
	void downloadError(QNetworkReply::NetworkError);

	//! Update the labels displaying the current default state
	void updateConfigLabels();

	//! open a file dialog to browse for a directory to save screenshots to
	//! if a directory is selected (i.e. dialog not cancelled), current
	//! value will be changed, but not saved to config file.
	void browseForScreenshotDir();
	void selectScreenshotDir(const QString& dir);

	//! Save the current viewing option including landscape, location and sky culture
	//! This doesn't include the current viewing direction, time and FOV since those
	//! have specific controls
	void saveCurrentViewOptions();

	//! Reset all stellarium options.
	//! This basically replaces the config.ini by the default one
	void setDefaultViewOptions();

	void populatePluginsList();
	void pluginsSelectionChanged(const QString&);
	void pluginConfigureCurrentSelection();
	void loadAtStartupChanged(int);

	#ifndef DISABLE_SCRIPTING
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

private:
	StelGui* gui;

	int savedProjectionType;
	
	//! Set the displayed fields checkboxes from the current displayed fields.
	void updateSelectedInfoCheckBoxes();
	//! Make sure that no tabs icons are outside of the viewport.
	//! @todo Limit the width to the width of the screen *available to the window*.
	void updateTabBarListWidgetWidth();
};

#endif // _CONFIGURATIONDIALOG_HPP_
