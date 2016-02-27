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
#include <QObject>

// Predeclaration of some classes
class StelCore;
class StelTextureMgr;
class StelObjectMgr;
class StelLocaleMgr;
class StelModuleMgr;
class StelSkyCultureMgr;
class StelViewportEffect;
class QOpenGLFramebufferObject;
class QSettings;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class StelLocationMgr;
class StelSkyLayerMgr;
class StelAudioMgr;
class StelVideoMgr;
class StelGuiBase;
class StelMainScriptAPIProxy;
class StelScriptMgr;
class StelActionMgr;
class StelProgressController;

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
	Q_PROPERTY(bool nightMode READ getVisionModeNight WRITE setVisionModeNight NOTIFY visionNightModeChanged)

public:
	friend class StelAppGraphicsWidget;
	friend class StelSkyItem;

	//! Create and initialize the main Stellarium application.
	//! @param parent the QObject parent
	//! The configFile will be search for in the search path by the StelFileMgr,
	//! it is therefor possible to specify either just a file name or path within the
	//! search path, or use a full path or even a relative path to an existing file
	StelApp(QObject* parent=NULL);

	//! Deinitialize and destroy the main Stellarium application.
	virtual ~StelApp();

	//! Initialize core and all the modules.
	void init(QSettings* conf);
	//! Deinitialize core and all the modules.
	void deinit();

	//! Load and initialize external modules (plugins)
	void initPlugIns();

	//! Registers all loaded StelModules with the ScriptMgr, and queues starting of the startup script.
	void initScriptMgr();

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

	//! Get the texture manager to use for loading textures.
	//! @return the texture manager to use for loading textures.
	StelTextureMgr& getTextureManager() {return *textureMgr;}

	//! Get the Location manager to use for managing stored locations
	//! @return the Location manager to use for managing stored locations
	StelLocationMgr& getLocationMgr() {return *planetLocationMgr;}

	//! Get the StelObject manager to use for querying from all stellarium objects.
	//! @return the StelObject manager to use for querying from all stellarium objects 	.
	StelObjectMgr& getStelObjectMgr() {return *stelObjectMgr;}

	StelSkyLayerMgr& getSkyImageMgr() {return *skyImageMgr;}

	//! Get the audio manager
	StelAudioMgr* getStelAudioMgr() {return audioMgr;}

	//! Get the actions manager to use for managing and editing actions
	StelActionMgr* getStelActionManager() {return actionMgr;}

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
	QString getCurrentStelStyle() {return "color";}

	//! Update all object according to the deltaTime in seconds.
	void update(double deltaTime);

	//! Draw all registered StelModule in the order defined by the order lists.
	// 2014-11: OLD COMMENT? What does a void return?
	// @return the max squared distance in pixels that any object has travelled since the last update.
	void draw();

	//! Call this when the size of the GL window has changed.
	void glWindowHasBeenResized(float x, float y, float w, float h);

	//! Get the ratio between real device pixel and "Device Independent Pixel".
	//! Usually this value is 1, but for a mac with retina screen this will be value 2.
	float getDevicePixelsPerPixel() const {return devicePixelsPerPixel;}
	void setDevicePixelsPerPixel(float dppp);
	
	//! Get the scaling ratio to apply on all display elements, like GUI, text etc..
	//! When this ratio is 1, all pixel sizes used in Stellarium will look OK on a regular
	//! computer screen with 96 pixel per inch (reference for tuning sizes).
	float getGlobalScalingRatio() const {return globalScalingRatio;}
	void setGlobalScalingRatio(float r) {globalScalingRatio=r;}

	//! Get the size of font
	int getBaseFontSize() const { return baseFontSize; }
	void setBaseFontSize(int s) { baseFontSize=s; }

	//! Get the GUI instance implementing the abstract GUI interface.
	StelGuiBase* getGui() const {return stelGui;}
	//! Tell the StelApp instance which GUI si currently being used.
	//! The caller is responsible for destroying the GUI.
	void setGui(StelGuiBase* b) {stelGui=b;}

#ifndef DISABLE_SCRIPTING
	//! Get the script API proxy (for signal handling)
	StelMainScriptAPIProxy* getMainScriptAPIProxy() {return scriptAPIProxy;}
	//! Get the script manager
	StelScriptMgr& getScriptMgr() {return *scriptMgr;}
#endif

	static void initStatic();
	static void deinitStatic();

	//! Add a progression indicator to the GUI (if applicable).
	//! @return a controller which can be used to indicate the current status.
	//! The StelApp instance remains the owner of the controller.
	StelProgressController* addProgressBar();
	void removeProgressBar(StelProgressController* p);

	//! Define the type of viewport effect to use
	//! @param effectName must be one of 'none', 'framebufferOnly', 'sphericMirrorDistorter'
	void setViewportEffect(const QString& effectName);
	//! Get the type of viewport effect currently used
	QString getViewportEffect() const;
	
	///////////////////////////////////////////////////////////////////////////
	// Scriptable methods
public slots:

	//! Set flag for activating night vision mode.
	void setVisionModeNight(bool);
	//! Get flag for activating night vision mode.
	bool getVisionModeNight() const {return flagNightVision;}

	//! Set flag for showing decimal degree in various places.
	void setFlagShowDecimalDegrees(bool b);
	//! Get flag for showing decimal degree in various places.
	bool getFlagShowDecimalDegrees() const {return flagShowDecimalDegrees;}

	//! Set flag for using calculation of azimuth from south towards west (instead north towards east)
	bool getFlagSouthAzimuthUsage() const { return flagUseAzimuthFromSouth; }
	//! Get flag for using calculation of azimuth from south towards west (instead north towards east)
	void setFlagSouthAzimuthUsage(bool use) { flagUseAzimuthFromSouth=use; }
	
	//! Set flag for using calculation of azimuth from south towards west (as in older astronomical literature)
	//! @deprecated Use getFlagSouthAzimuthUsage() instead.
	bool getFlagOldAzimuthUsage() const { return getFlagSouthAzimuthUsage(); }
	//! Get flag for using calculation of azimuth from south towards west (as in older astronomical literature)
	//! @deprecated Use setFlagSouthAzimuthUsage() instead.
	void setFlagOldAzimuthUsage(bool use) { setFlagSouthAzimuthUsage(use); }


	//! Get the current number of frame per second.
	//! @return the FPS averaged on the last second
	float getFps() const {return fps;}

	//! Return the time since when stellarium is running in second.
	static double getTotalRunTime();

	//! Return the scaled time for animated objects
	static double getAnimationTime();

	//! Report that a download occured. This is used for statistics purposes.
	//! Connect this slot to QNetworkAccessManager::finished() slot to obtain statistics at the end of the program.
	void reportFileDownloadFinished(QNetworkReply* reply);

	//! do some cleanup and call QCoreApplication::exit(0)
	void quit();
signals:
	void visionNightModeChanged(bool);
	void colorSchemeChanged(const QString&);
	void languageChanged();
	void skyCultureChanged(const QString&);

	//! Called just after a progress bar is added.
	void progressBarAdded(const StelProgressController*);
	//! Called just before a progress bar is removed.
	void progressBarRemoved(const StelProgressController*);
	//! Called just before we exit Qt mainloop.
	void aboutToQuit();

private:

	//! Handle mouse clics.
	void handleClick(class QMouseEvent* event);
	//! Handle mouse wheel.
	void handleWheel(class QWheelEvent* event);
	//! Handle mouse move.
	void handleMove(float x, float y, Qt::MouseButtons b);
	//! Handle key press and release.
	void handleKeys(class QKeyEvent* event);
	//! Handle pinch on multi touch devices.
	void handlePinch(qreal scale, bool started);

	void prepareRenderBuffer();
	void applyRenderBuffer();

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

	//Actions manager fot the application.  Will replace shortcutMgr.
	StelActionMgr* actionMgr;

	// Textures manager for the application
	StelTextureMgr* textureMgr;

	// Manager for all the StelObjects of the program
	StelObjectMgr* stelObjectMgr;

	// Manager for the list of observer locations on planets
	StelLocationMgr* planetLocationMgr;

	// Main network manager used for the program
	QNetworkAccessManager* networkAccessManager;

	//! Get proxy settings from config file... if not set use http_proxy env var
	void setupNetworkProxy();

	// The audio manager.  Must execute in the main thread.
	StelAudioMgr* audioMgr;

	// The video manager.  Must execute in the main thread.
	StelVideoMgr* videoMgr;

	StelSkyLayerMgr* skyImageMgr;

#ifndef DISABLE_SCRIPTING
	// The script API proxy object (for bridging threads)
	StelMainScriptAPIProxy* scriptAPIProxy;

	// The script manager based on Qt script engine
	StelScriptMgr* scriptMgr;
#endif



	StelGuiBase* stelGui;
	
	// Store the ratio between real device pixel in "Device Independent Pixel"
	// Usually this value is 1, but for a mac with retina screen this will be value 2.
	float devicePixelsPerPixel;

	// The scaling ratio to apply on all display elements, like GUI, text etc..
	float globalScalingRatio;
	
	// Used to collect wheel events
	QTimer * wheelEventTimer;

	// Accumulated horizontal and vertical wheel event deltas
	int wheelEventDelta[2];

	float fps;
	int frame;
	double timefr, timeBase;		// Used for fps counter

	//! Define whether we are in night vision mode
	bool flagNightVision;

	QSettings* confSettings;

	// Define whether the StelApp instance has completed initialization
	bool initialized;

	static qint64 startMSecs;
	static float animationScale;

	// Temporary variables used to store the last gl window resize
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

	QList<StelProgressController*> progressControllers;

	int baseFontSize;

	// Framebuffer object used for viewport effects.
	QOpenGLFramebufferObject* renderBuffer;

	StelViewportEffect* viewportEffect;
	
	bool flagShowDecimalDegrees;
	// flag to indicate we want calculate azimuth from south towards west (as in old astronomical literature)
	bool flagUseAzimuthFromSouth;

};

#endif // _STELAPP_HPP_
