/*
 * Visibility Map plug-in for Stellarium
 *
 * Copyright (C) 2026 Atque
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef VISIBILITYMAP_HPP
#define VISIBILITYMAP_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"

class QPixmap;
class QSettings;
class StelButton;
class VisibilityMapDialog;

/*! @defgroup daylightmap Visibility Map plug-in
@{
The Visibility Map plugin provides global Earth illumination tools,
including the solar terminator, twilight zones, Sun and Moon subpoint
markers, and rise/set isolines.
@}
*/

class DaylightMap : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagShowButton
		   READ getFlagShowButton
		   WRITE setFlagShowButton
		   NOTIFY flagShowButtonChanged)

public:
	DaylightMap();
	~DaylightMap() override;

	void init() override;
	void deinit() override;
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;

	void restoreDefaults();
	void readSettingsFromConfig();
	void saveSettingsToConfig() const;

	bool getFlagShowButton() const { return flagShowButton; }

public slots:
	void setFlagShowButton(bool b);

signals:
	void flagShowButtonChanged(bool b);

private slots:
	void saveSettings() { saveSettingsToConfig(); }

private:
	void restoreDefaultConfigIni();

	VisibilityMapDialog* mainWindow;
	QSettings* conf;
	StelGui* gui;

	bool flagShowButton;
	StelButton* toolbarButton;
};

#include <QObject>
#include "StelPluginInterface.hpp"

class VisibilityMapStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
};

#endif /* VISIBILITYMAP_HPP */
