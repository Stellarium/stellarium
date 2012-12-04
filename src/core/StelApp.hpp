/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _STELAPP_HPP_
#define _STELAPP_HPP_

#include <QString>
#include <QVariant>
#include <QObject>

// Predeclaration of some classes
class StelCore;
class StelObjectMgr;
class StelLocaleMgr;
class StelModuleMgr;
class StelSkyCultureMgr;
class StelShortcutMgr;
class QSettings;
class QNetworkAccessManager;
class QNetworkReply;
class QTime;
class QTimer;
class StelLocationMgr;
class StelSkyLayerMgr;
class StelAudioMgr;
class StelVideoMgr;
class StelGuiBase;

//! @class StelApp
//! Singleton main Stellarium application class.
//! This is the central class of Stellarium.  Only one singleton instance of
//! this class is created and can be accessed from anywhere else.  This class
//! is the access point to several "Manager" class which provide application-wide
//! services for managment of font, textures, localization, sky culture, and in
//! theory all other services used by the other part of the program.
//!
//! The StelApp class is also the one managing the StelModule in a generic manner
//! by calling their update, drawing and other methods when needed.
//! @author Fabien Chereau
class StelApp : public QObject
{
	Q_OBJECT

public:
	friend class StelAppGraphicsWidget;

	//! Create and initialize the main Stellarium application.
	//! @param parent the QObject parent
	//! The configFile will be search for in the search path by the StelFileMgr,
	//! it is therefor possible to specify either just a file name or path within the
	//! search path, or use a full path or even a relative path to an existing file
	StelApp(QObject* parent=NULL);

	//! Deinitialize and destroy the main Stellarium application.
	virtual ~StelApp();

	//! Initialize core and default modules.
	void init(QSettings* conf, class StelRenderer* renderer);

	//! Load and initialize external modules (plugins)
	void initPlugIns();

	//! Get the StelApp singleton instance.
	//! @return the StelApp singleton instance
	static StelApp& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Get the module manager to use for accessing any module loaded in the application.
	//! @return the module manager.
	StelModuleMgr& getModuleMgr() {return *moduleMgr;}

	//! Get the locale manager to use for i18n & date/time localization.
	//! @return the font manager to use for loading fonts.
	StelLocaleMgr& getLocaleMgr() {return *localeMgr;}

	//! Get the sky cultures manager.
	//! @return the sky cultures manager
	StelSkyCultureMgr& getSkyCultureMgr() {return *skyCultureMgr;}

	//! Get the Location manager to use for managing stored locations
	//! @return the Location manager to use for managing stored locations
	StelLocationMgr& getLocationMgr() {return *planetLocationMgr;}

	//! Get the StelObject manager to use for querying from all stellarium objects.
	//! @return the StelObject manager to use for querying from all stellarium objects 	.
	StelObjectMgr& getStelObjectMgr() {return *stelObjectMgr;}

	StelSkyLayerMgr& getSkyImageMgr() {return *skyImageMgr;}

	//! Get the audio manager
	StelAudioMgr* getStelAudioMgr() {return audioMgr;}

	//! Get the shortcuts manager to use for managing and editing shortcuts
	StelShortcutMgr* getStelShortcutManager() {return shortcutMgr;}

	//! Get the video manager
	StelVideoMgr* getStelVideoMgr() {return videoMgr;}

	//! Get the core of the program.
	//! It is the one which provide the projection, navigation and tone converter.
	//! @return the StelCore instance of the program
	StelCore* getCore() {return core;}

	//! Get the common instance of QNetworkAccessManager used in stellarium
	QNetworkAccessManager* getNetworkAccessManager() {return networkAccessManager;}

	//! Update translations, font for GUI and sky everywhere in the program.
	void updateI18n();

	//! Update and reload sky culture informations everywhere in the program.
	void updateSkyCulture();

	//! Return the main configuration options
	QSettings* getSettings() {return confSettings;}

	//! Return the currently used style
	QString getCurrentStelStyle() {return flagNightVision ? "night_color" : "color";}

	//! Update all object according to the deltaTime in seconds.
	void update(double deltaTime);

	//! Iterate through the drawing sequence.
	//! This allow us to split the slow drawing operation into small parts,
	//! we can then decide to pause the painting for this frame and used the cached image instead.
	//! @return true if we should continue drawing (by calling the method again)
	bool drawPartial(class StelRenderer* renderer);

	//! Call this when the size of the window has changed.
	void windowHasBeenResized(float x, float y, float w, float h);

	//! Get the GUI instance implementing the abstract GUI interface.
	StelGuiBase* getGui() const {return stelGui;}
	//! Tell the StelApp instance which GUI si currently being used.
	//! The caller is responsible for destroying the GUI.
	void setGui(StelGuiBase* b) {stelGui=b;}

	static void initStatic();
	static void deinitStatic();

	//! Get whether solar shadows should be rendered.
	bool getRenderSolarShadows() const;

	///////////////////////////////////////////////////////////////////////////
	// Scriptable methods
public slots:

	//! Set flag for activating solar shadow rendering.
	void setRenderSolarShadows(bool);

	//! Set flag for activating night vision mode.
	void setVisionModeNight(bool);
	//! Get flag for activating night vision mode.
	bool getVisionModeNight() const {return flagNightVision;}

	//! Get the current number of frame per second.
	//! @return the FPS averaged on the last second
	float getFps() const {return fps;}

	//! Return the time since when stellarium is running in second.
	static double getTotalRunTime();

	//! Report that a download occured. This is used for statistics purposes.
	//! Connect this slot to QNetworkAccessManager::finished() slot to obtain statistics at the end of the program.
	void reportFileDownloadFinished(QNetworkReply* reply);

signals:
	void colorSchemeChanged(const QString&);
	void languageChanged();
	void skyCultureChanged(const QString&);

private:

	//! Handle mouse clics.
	void handleClick(class QMouseEvent* event);
	//! Handle mouse wheel.
	void handleWheel(class QWheelEvent* event);
	//! Handle mouse move.
	void handleMove(int x, int y, Qt::MouseButtons b);
	//! Handle key press and release.
	void handleKeys(class QKeyEvent* event);

	// The StelApp singleton
	static StelApp* singleton;

	// The associated StelCore instance
	StelCore* core;

	// Module manager for the application
	StelModuleMgr* moduleMgr;

	// Locale manager for the application
	StelLocaleMgr* localeMgr;

	// Sky cultures manager for the application
	StelSkyCultureMgr* skyCultureMgr;

	//Shortcuts manager for the application
	StelShortcutMgr* shortcutMgr;

	// Manager for all the StelObjects of the program
	StelObjectMgr* stelObjectMgr;

	// Manager for the list of observer locations on planets
	StelLocationMgr* planetLocationMgr;

	// Main network manager used for the program
	QNetworkAccessManager* networkAccessManager;

	//! Get proxy settings from config file... if not set use http_proxy env var
	void setupHttpProxy();

	// The audio manager.  Must execute in the main thread.
	StelAudioMgr* audioMgr;

	// The video manager.  Must execute in the main thread.
	StelVideoMgr* videoMgr;

	StelSkyLayerMgr* skyImageMgr;

	StelGuiBase* stelGui;

	// Used to collect wheel events
	QTimer * wheelEventTimer;

	float fps;
	int frame;
	double timefr, timeBase;		// Used for fps counter

	//! Define whether we are in night vision mode
	bool flagNightVision;

	//! Define whether solar shadows should be rendered (using GLSL shaders)
	bool renderSolarShadows;

	QSettings* confSettings;

	// Define whether the StelApp instance has completed initialization
	bool initialized;

	static QTime* qtime;

	// Temporary variables used to store the last window resize
	// if the core was not yet initialized
	int saveProjW;
	int saveProjH;

	//! Store the number of downloaded files for statistics.
	int nbDownloadedFiles;
	//! Store the summed size of all downloaded files in bytes.
	qint64 totalDownloadedSize;

	//! Store the number of downloaded files read from the cache for statistics.
	int nbUsedCache;
	//! Store the summed size of all downloaded files read from the cache in bytes.
	qint64 totalUsedCacheSize;

	//! The state of the drawing sequence
	int drawState;
};

#endif // _STELAPP_HPP_
