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

The results are presented in a QWebEngine view on platforms which support this. Unfortunately on some platforms
the module seems to be available but fails to initialize properly. A manual entry in the config file can be used
to actively disable the QWebEngineView based content box and open the URL in the system webbrowser.
If none is configured, Qt's own error messages will be visible in stderr.

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
	virtual ~OnlineQueries() ;

	virtual void init() override;
	virtual bool configureGui(bool show) override;

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
	// The Ancient-Skies.org website did not resurrect as planned in 2022. Leave it here, maybe in a few years.
	//void queryAncientSkies(); //!< Connect from a button that triggers information query
	void queryCustomSite1();  //!< Connect from a button that triggers information query
	void queryCustomSite2();  //!< Connect from a button that triggers information query
	void queryCustomSite3();  //!< Connect from a button that triggers information query
	QString getCustomUrl1() const { return customUrl1;}
	QString getCustomUrl2() const { return customUrl2;}
	QString getCustomUrl3() const { return customUrl3;}
	bool webEngineDisabled() const { return disableWebView;}

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
	//! The actual query worker: build name or HIP number from currently selected object
	void query(QString url, bool useHip);
	void createToolbarButton() const;
	void setOutputHtml(QString html); //!< Forward html to GUI dialog
	void setOutputUrl(QUrl url);      //!< Forward URL to GUI dialog
	OnlineQueriesDialog* dialog;
	QSettings* conf;
	bool enabled;                     //!< show dialog?
	bool disableWebView;              //!< Disable the webview on platforms where QtWebView compiles, but fails to work properly. (Seen on Odroid boards.)

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
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	//QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* ONLINEQUERIES_HPP */
