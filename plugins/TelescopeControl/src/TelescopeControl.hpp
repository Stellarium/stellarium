/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 * 
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This class used to be called TelescopeMgr before the split.
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

#ifndef _TELESCOPE_CONTROL_HPP_
#define _TELESCOPE_CONTROL_HPP_

#include "StelFader.hpp"
#include "StelGui.hpp"
#include "StelJsonParser.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"
#include "TelescopeControlGlobals.hpp"
#include "VecMath.hpp"

#include <QFile>
#include <QFont>
#include <QHash>
#include <QMap>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

class StelObject;
class StelProjector;
class TelescopeClient;
class TelescopeDialog;
class SlewDialog;

using namespace TelescopeControlGlobals;

typedef QSharedPointer<TelescopeClient> TelescopeClientP;

//! This class manages the controlling of one or more telescopes by one
//! instance of the stellarium program. "Controlling a telescope"
//! means receiving position information from the telescope
//! and sending GOTO commands to the telescope.
//! No esoteric features like motor focus, electric heating and such.
//! The actual controlling of a telescope is left to the implementation
//! of the abstract base class TelescopeClient.
class TelescopeControl : public StelObjectModule
{
	Q_OBJECT

public:
	TelescopeControl();
	virtual ~TelescopeControl();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore * core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelObjectModule class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;
	virtual StelObjectP searchByName(const QString& name) const;
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	// empty as its not celestial objects
	virtual QStringList listAllObjects(bool inEnglish) const { Q_UNUSED(inEnglish) return QStringList(); }
	virtual QString getName() const { return "Telescope Control"; }
	virtual bool configureGui(bool show = true);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to TelescopeControl
	//! Send a J2000-goto-command to the specified telescope
	//! @param telescopeNr the number of the telescope
	//! @param j2000Pos the direction in equatorial J2000 frame
	void telescopeGoto(int telescopeNr, const Vec3d &j2000Pos);
	
	//! Remove all currently registered telescopes
	void deleteAllTelescopes();
	
	//! Safe access to the loaded list of telescope models
	const QHash<QString, DeviceModel>& getDeviceModels();
	
	//! Loads the module's configuration from the configuration file.
	void loadConfiguration();
	//! Saves the module's configuration to the configuration file.
	void saveConfiguration();
	
	//! Saves to telescopes.json a list of the parameters of the active telescope clients.
	void saveTelescopes();
	//! Loads from telescopes.json the parameters of telescope clients and initializes them. If there are already any initialized telescope clients, they are removed.
	void loadTelescopes();
	
	//These are public, but not slots, because they don't use sufficient validation. Scripts shouldn't be able to add/remove telescopes, only to point them.
	//! Adds a telescope description containing the given properties. DOES NOT VALIDATE its parameters. If serverName is specified, portSerial should be specified too. Call saveTelescopes() to write the modified configuration to disc. Call startTelescopeAtSlot() to start this telescope.
	//! @param portSerial must be a valid serial port name for the particular platform, e.g. "COM1" for Microsoft Windows of "/dev/ttyS0" for Linux
	bool addTelescopeAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox, QString host = QString("localhost"), int portTCP = DEFAULT_TCP_PORT, int delay = DEFAULT_DELAY, bool connectAtStartup = false, QList<double> circles = QList<double>(), QString serverName = QString(), QString portSerial = QString());
	//! Retrieves a telescope description. Returns false if the slot is empty. Returns empty serverName and portSerial if the description contains no server.
	bool getTelescopeAtSlot(int slot, ConnectionType& connectionType, QString& name, QString& equinox, QString& host, int& portTCP, int& delay, bool& connectAtStartup, QList<double>& circles, QString& serverName, QString& portSerial);
	//! Removes info from the tree. Should it include stopTelescopeAtSlot()?
	bool removeTelescopeAtSlot(int slot);
	
	//! Starts a telescope at the given slot, getting its description with getTelescopeAtSlot(). Creates a TelescopeClient object and starts a server process if necessary.
	bool startTelescopeAtSlot(int slot);
	//! Stops the telescope at the given slot. Destroys the TelescopeClient object and terminates the server process if necessary.
	bool stopTelescopeAtSlot(int slot);
	//! Stops all telescopes, but without removing them like deleteAllTelescopes().
	bool stopAllTelescopes();
	
	//! Checks if there's a TelescopeClient object at a given slot, i.e. if there's an active telescope at that slot.
	bool isExistingClientAtSlot(int slot);
	//! Checks if the TelescopeClient object at a given slot is connected to a server.
	bool isConnectedClientAtSlot(int slot);

	//! Returns a list of the currently connected clients
	QHash<int, QString> getConnectedClientsNames();
	
	bool getFlagUseServerExecutables() {return useServerExecutables;}
	//! Forces a call of loadDeviceModels(). Stops all active telescopes.
	void setFlagUseServerExecutables(bool b);
	const QString& getServerExecutablesDirectoryPath();
	//! Forces a call of loadDeviceModels(). Stops all active telescopes.
	bool setServerExecutablesDirectoryPath(const QString& newPath);
	
	bool getFlagUseTelescopeServerLogs () {return useTelescopeServerLogs;}

public slots:
	//! Set display flag for telescope reticles
	void setFlagTelescopeReticles(bool b) {reticleFader = b;}
	//! Get display flag for telescope reticles
	bool getFlagTelescopeReticles() const {return (bool)reticleFader;}
	
	//! Set display flag for telescope name labels
	void setFlagTelescopeLabels(bool b) {labelFader = b;}
	//! Get display flag for telescope name labels
	bool getFlagTelescopeLabels() const {return labelFader==true;}

	//! Set display flag for telescope field of view circles
	void setFlagTelescopeCircles(bool b) {circleFader = b;}
	//! Get display flag for telescope field of view circles
	bool getFlagTelescopeCircles() const {return circleFader==true;}
	
	//! Set the telescope reticle color
	void setReticleColor(const Vec3f &c) {reticleColor = c;}
	//! Get the telescope reticle color
	const Vec3f& getReticleColor() const {return reticleColor;}
	
	//! Get the telescope labels color
	const Vec3f& getLabelColor() const {return labelColor;}
	//! Set the telescope labels color
	void setLabelColor(const Vec3f &c) {labelColor = c;}

	//! Set the field of view circles color
	void setCircleColor(const Vec3f &c) {circleColor = c;}
	//! Get the field of view circles color
	const Vec3f& getCircleColor() const {return circleColor;}
	
	//! Define font size to use for telescope names display
	void setFontSize(int fontSize);
	
	//! slews a telescope to the selected object.
	//! For use from the GUI. The telescope number will be
	//! deduced from the name of the QAction which triggered the slot.
	void slewTelescopeToSelectedObject();

	//! slews a telescope to the point of the celestial sphere currently
	//! in the center of the screen.
	//! For use from the GUI. The telescope number will be
	//! deduced from the name of the QAction which triggered the slot.
	void slewTelescopeToViewDirection();
	
	//! Used in the GUI
	void setFlagUseTelescopeServerLogs (bool b) {useTelescopeServerLogs = b;}

signals:
	void clientConnected(int slot, QString name);
	void clientDisconnected(int slot);

private slots:
	void setStelStyle(const QString& section);
	//! Set translated keyboard shortcut descriptions.
	void translateActionDescriptions();

private:
	//! Draw a nice animated pointer around the object if it's selected
	void drawPointer(const StelProjectorP& prj, const StelCore* core, class StelRenderer* renderer);

	//! Perform the communication with the telescope servers
	void communicate(void);
	
	LinearFader labelFader;
	LinearFader reticleFader;
	LinearFader circleFader;
	//! Colour currently used to draw telescope reticles
	Vec3f reticleColor;
	//! Colour currently used to draw telescope text labels
	Vec3f labelColor;
	//! Colour currently used to draw field of view circles
	Vec3f circleColor;
	//! Colour used to draw telescope reticles in normal mode, as set in the configuration file
	Vec3f reticleNormalColor;
	//! Colour used to draw telescope reticles in night mode, as set in the configuration file
	Vec3f reticleNightColor;
	//! Colour used to draw telescope labels in normal mode, as set in the configuration file
	Vec3f labelNormalColor;
	//! Colour used to draw telescope labels in night mode, as set in the configuration file
	Vec3f labelNightColor;
	//! Colour used to draw field of view circles in normal mode, as set in the configuration file
	Vec3f circleNormalColor;
	//! Colour used to draw field of view circles in night mode, as set in the configuration file
	Vec3f circleNightColor;
	
	//! Font used to draw telescope text labels
	QFont labelFont;
	
	//Toolbar button to toggle the Slew window
	QPixmap* pixmapHover;
	QPixmap* pixmapOnIcon;
	QPixmap* pixmapOffIcon;
	StelButton* toolbarButton;
	
	//! Telescope reticle texture
	class StelTextureNew* reticleTexture;
	//! Telescope selection marker texture
	class StelTextureNew* selectionTexture;
	
	//! Contains the initialized telescope client objects representing the telescopes that Stellarium is connected to or attempting to connect to.
	QMap<int, TelescopeClientP> telescopeClients;
	//! Contains QProcess objects of the currently running telescope server processes that have been launched by Stellarium.
	QHash<int, QProcess*> telescopeServerProcess;
	QStringList telescopeServers;
	QVariantMap telescopeDescriptions;
	QHash<QString, DeviceModel> deviceModels;
	
	QHash<ConnectionType, QString> connectionTypeNames;
	
	bool useTelescopeServerLogs;
	QHash<int, QFile*> telescopeServerLogFiles;
	QHash<int, QTextStream*> telescopeServerLogStreams;
	
	bool useServerExecutables;
	QString serverExecutablesDirectoryPath;
	
	//GUI
	TelescopeDialog * telescopeDialog;
	SlewDialog * slewDialog;
	
	//! Used internally. Checks if the argument is a valid slot number.
	bool isValidSlotNumber(int slot);
	bool isValidPort(uint port);
	bool isValidDelay(int delay);
	
	//! Start the telescope server defined for a given slot in a new QProcess
	//! @param slot the slot number
	//! @param serverName the short form of the server name (e.g. "Dummy" for "TelescopeServerDummy")
	//! @param tcpPort TCP slot the server should listen to
	bool startServerAtSlot(int slot, QString serverName, int tcpPort, QString serialPort);
	//! Stop the telescope server at a given slot, terminating the process
	bool stopServerAtSlot(int slot);

	//! A wrapper for TelescopeClient::create(). Used internally by loadTelescopes() and startTelescopeAtSlot(). Does not perform any validation on its arguments.
	bool startClientAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox, QString host, int portTCP, int delay, QList<double> circles, QString serverName = QString(), QString portSerial = QString());
	
	//! Returns true if the TelescopeClient at this slot has been stopped successfully or doesn't exist
	bool stopClientAtSlot(int slot);
	
	//! Compile a list of the executables in the /servers folder
	void loadTelescopeServerExecutables();
	
	//! Loads the list of supported telescope models. Calls loadTelescopeServerExecutables() internally.
	void loadDeviceModels();
	
	//! Copies the default device_models.json to the given destination
	//! @returns true if the file has been copied successfully
	bool restoreDeviceModelsListTo(QString deviceModelsListPath);
	
	void addLogAtSlot(int slot);
	void logAtSlot(int slot);
	void removeLogAtSlot(int slot);
	
	QString actionGroupId;
	QString moveToSelectedActionId;
	QString moveToCenterActionId;
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class TelescopeControlStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_TELESCOPE_CONTROL_HPP_*/
