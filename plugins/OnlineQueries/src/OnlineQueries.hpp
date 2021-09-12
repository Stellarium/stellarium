/*
 * OnlineQueries plug-in for Stellarium
 *
 * Copyright (C) 2020-21 Georg Zotti
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

#ifndef ONLINEQUERIES_HPP
#define ONLINEQUERIES_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"
#include "HipOnlineQuery.hpp"

#include <QFont>
#include <QString>

class QPixmap;
class StelButton;
class OnlineQueriesDialog;

/*! @defgroup onlineQueries Online Queries Plug-in
@{
The %Online Queries plugin provides online lookup to retrieve additional data from selected web services.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [OnlineQueries]).

@}
*/

//! @class OnlineQueries
//! Main class of the %Online Queries plugin.
//! @author Georg Zotti, Alexander Wolf
//! @ingroup OnlineQueries
class OnlineQueries : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled  READ isEnabled  WRITE setEnabled  NOTIFY flagEnabledChanged )

public:
	OnlineQueries();
	virtual ~OnlineQueries() Q_DECL_OVERRIDE;

	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void update(double) Q_DECL_OVERRIDE {;}
	virtual void draw(StelCore *core) Q_DECL_OVERRIDE {Q_UNUSED(core)}
	virtual bool configureGui(bool show) Q_DECL_OVERRIDE;

signals:
	void flagEnabledChanged(bool b);

public slots:
	//! Enable plugin usage (show dialog)
	void setEnabled(bool b);
	//! Is plugin dialog shown?
	bool isEnabled() const { return enabled; }

	//! Save the settings to the main configuration file.
	void saveConfiguration(void);

	void queryWikipedia();    //!< Connect from a button that triggers information query
	void queryAAVSO();        //!< Connect from a button that triggers information query
	void queryGCVS();         //!< Connect from a button that triggers information query
	void queryAncientSkies(); //!< Connect from a button that triggers information query
	void queryCustomSite1();  //!< Connect from a button that triggers information query
	void queryCustomSite2();  //!< Connect from a button that triggers information query
	void queryCustomSite3();  //!< Connect from a button that triggers information query
	QString getCustomUrl1() const { return customUrl1;}
	QString getCustomUrl2() const { return customUrl2;}
	QString getCustomUrl3() const { return customUrl3;}

private slots:
	//! Set up the plugin with default values.  This means clearing out the OnlineQueries section in the
	//! main config.ini (if one already exists), and populating it with default values.
	void restoreDefaultConfiguration(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void loadConfiguration(void);

	void onHipQueryStatusChanged();      //!< To be connected
	void onAavsoHipQueryStatusChanged(); //!< To be connected

private:
	//! The actual query worker: build name or HIP number from currently selected object, and present result in dialog or external browser
	void query(QString url, bool useHip, bool useBrowser);
	void createToolbarButton() const;
	void setOutputHtml(QString html); //!< Forward html to GUI dialog
	OnlineQueriesDialog* dialog;
	QSettings* conf;
	bool enabled;

	StelButton* toolbarButton;

	// URLs are settable via config.ini.
	// TODO: Maybe add a config panel for the custom sites?
	QString ancientSkiesUrl;
	QString aavsoHipUrl;
	QString aavsoOidUrl;
	QString gcvsUrl;
	QString wikipediaUrl;
	QString customUrl1;
	QString customUrl2;
	QString customUrl3;
	bool custom1UseHip; //!< Use HIP number, not common name, in query?
	bool custom2UseHip; //!< Use HIP number, not common name, in query?
	bool custom3UseHip; //!< Use HIP number, not common name, in query?
	bool custom1inBrowser; //!< True if custom1 URL should be opened in a webbrowser, not in our panel
	bool custom2inBrowser; //!< True if custom2 URL should be opened in a webbrowser, not in our panel
	bool custom3inBrowser; //!< True if custom3 URL should be opened in a webbrowser, not in our panel

	HipOnlineQuery *hipQuery;              // one is actually enough!
	HipOnlineReply *hipOnlineReply;        // Common reply object
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class OnlineQueriesPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const Q_DECL_OVERRIDE;
	virtual StelPluginInfo getPluginInfo() const Q_DECL_OVERRIDE;
	virtual QObjectList getExtensionList() const Q_DECL_OVERRIDE { return QObjectList(); }
};

#endif /* ONLINEQUERIES_HPP */
