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

#ifndef STELAPP_HPP
#define STELAPP_HPP

#include <QString>
#include <QObject>
#include "StelModule.hpp"
#include "VecMath.hpp"

// Predeclaration of some classes
class StelCore;
class StelTextureMgr;
class StelObjectMgr;
class StelLocaleMgr;
class StelModuleMgr;
class StelMainView;
class StelSkyCultureMgr;
class StelViewportEffect;
class QOpenGLFramebufferObject;
class QOpenGLFunctions;
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
class StelPropertyMgr;
class StelProgressController;

#ifdef 	ENABLE_SPOUT
class SpoutSender;
#endif

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
	Q_PROPERTY(bool flagShowDecimalDegrees  READ getFlagShowDecimalDegrees  WRITE setFlagShowDecimalDegrees  NOTIFY flagShowDecimalDegreesChanged)
	Q_PROPERTY(bool flagUseAzimuthFromSouth READ getFlagSouthAzimuthUsage   WRITE setFlagSouthAzimuthUsage   NOTIFY flagUseAzimuthFromSouthChanged)
	Q_PROPERTY(bool flagUseCCSDesignation   READ getFlagUseCCSDesignation   WRITE setFlagUseCCSDesignation   NOTIFY flagUseCCSDesignationChanged)
	Q_PROPERTY(bool flagUseFormattingOutput READ getFlagUseFormattingOutput WRITE setFlagUseFormattingOutput NOTIFY flagUseFormattingOutputChanged)
	Q_PROPERTY(bool flagOverwriteInfoColor  READ getFlagOverwriteInfoColor  WRITE setFlagOverwriteInfoColor  NOTIFY flagOverwriteInfoColorChanged)
	Q_PROPERTY(Vec3f overwriteInfoColor	READ getOverwriteInfoColor	WRITE setOverwriteInfoColor	 NOTIFY overwriteInfoColorChanged)
	Q_PROPERTY(Vec3f daylightInfoColor	READ getDaylightInfoColor	WRITE setDaylightInfoColor	 NOTIFY daylightInfoColorChanged)
	Q_PROPERTY(int  screenFontSize          READ getScreenFontSize          WRITE setScreenFontSize          NOTIFY screenFontSizeChanged)
	Q_PROPERTY(int  guiFontSize             READ getGuiFontSize             WRITE setGuiFontSize             NOTIFY guiFontSizeChanged)

	Q_PROPERTY(QString version READ getVersion)

public:
	friend class StelAppGraphicsWidget;
	friend class StelRootItem;

	//! Create and initialize the main Stellarium application.
	//! @param parent the QObject parent
	//! The configFile will be searched for in the search path by the StelFileMgr,
	//! it is therefore possible to specify either just a file name or path within the
	//! search path, or use a full path or even a relative path to an existing file
	StelApp(StelMainView* parent);

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

	//! Returns all arguments passed on the command line, together with the contents of the STEL_OPTS environment variable.
	//! You can use the CLIProcessor class to help parse it.
	//! @return the arguments passed to Stellarium on the command line concatenated with the STEL_OPTS environment variable
	static QStringList getCommandlineArguments();

	//! Get the StelApp singleton instance.
	//! @return the StelApp singleton instance
	static StelApp& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Get the module manager to use for accessing any module loaded in the application.
	//! @return the module manager.
	StelModuleMgr& getModuleMgr() const {return *moduleMgr;}

	//! Get the corresponding module or Q_NULLPTR if can't find it.
	//! This is a shortcut for getModleMgr().getModule().
	//! @return the module pointer.
	StelModule* getModule(const QString& moduleID) const;

	//! Get the locale manager to use for i18n & date/time localization.
	//! @return the font manager to use for loading fonts.
	StelLocaleMgr& getLocaleMgr() const {return *localeMgr;}

	//! Get the sky cultures manager.
	//! @return the sky cultures manager
	StelSkyCultureMgr& getSkyCultureMgr() const {return *skyCultureMgr;}

	//! Get the texture manager to use for loading textures.
	//! @return the texture manager to use for loading textures.
	StelTextureMgr& getTextureManager() const {return *textureMgr;}

	//! Get the Location manager to use for managing stored locations
	//! @return the Location manager to use for managing stored locations
	StelLocationMgr& getLocationMgr() const {return *planetLocationMgr;}

	//! Get the StelObject manager to use for querying from all stellarium objects.
	//! @return the StelObject manager to use for querying from all stellarium objects 	.
	StelObjectMgr& getStelObjectMgr() const {return *stelObjectMgr;}

	StelSkyLayerMgr& getSkyImageMgr() const {return *skyImageMgr;}

	//! Get the audio manager
	StelAudioMgr* getStelAudioMgr() const {return audioMgr;}

	//! Get the actions manager to use for managing and editing actions
	StelActionMgr* getStelActionManager() const {return actionMgr;}

	//! Return the property manager
	StelPropertyMgr* getStelPropertyManager() const {return propMgr;}

	//! Get the video manager
	StelVideoMgr* getStelVideoMgr() const {return videoMgr;}

	//! Get the core of the program.
	//! It is the one which provide the projection, navigation and tone converter.
	//! @return the StelCore instance of the program
	StelCore* getCore() const {return core;}

	//! Get the common instance of QNetworkAccessManager used in stellarium
	QNetworkAccessManager* getNetworkAccessManager() const {return networkAccessManager;}

	//! Update translations, font for GUI and sky everywhere in the program.
	void updateI18n();

	//! Return the main configuration options
	QSettings* getSettings() const {return confSettings;}

	//! Return the currently used style
	const QString getCurrentStelStyle() const {return "color";}

	//! Update all object according to the deltaTime in seconds.
	void update(double deltaTime);

	//! Draw all registered StelModule in the order defined by the order lists.
	// 2014-11: OLD COMMENT? What does a void return?
	// @return the max squared distance in pixels that any object has travelled since the last update.
	void draw();

	//! Get the ratio between real device pixel and "Device Independent Pixel".
	//! Usually this value is 1, but for a mac with retina screen this will be value 2.
	qreal getDevicePixelsPerPixel() const {return devicePixelsPerPixel;}
	void setDevicePixelsPerPixel(qreal dppp);
	
	//! Get the scaling ratio to apply on all display elements, like GUI, text etc..
	//! When this ratio is 1, all pixel sizes used in Stellarium will look OK on a regular
	//! computer screen with 96 pixel per inch (reference for tuning sizes).
	float getGlobalScalingRatio() const {return globalScalingRatio;}
	void setGlobalScalingRatio(float r) {globalScalingRatio=r;}

	//! Get the fontsize used for screen text.
	int getScreenFontSize() const { return screenFontSize; }
	//! Change screen font size.
	void setScreenFontSize(int s);
	//! Get the principal font size used for GUI panels.
	int getGuiFontSize() const;
	//! change GUI font size.
	void setGuiFontSize(int s);

	//! Get the GUI instance implementing the abstract GUI interface.
	StelGuiBase* getGui() const {return stelGui;}
	//! Tell the StelApp instance which GUI is currently being used.
	//! The caller is responsible for destroying the GUI.
	void setGui(StelGuiBase* b) {stelGui=b;}

#ifdef ENABLE_SCRIPTING
	//! Get the script API proxy (for signal handling)
	StelMainScriptAPIProxy* getMainScriptAPIProxy() const {return scriptAPIProxy;}
	//! Get the script manager
	StelScriptMgr& getScriptMgr() const {return *scriptMgr;}
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

	//! Dump diagnostics about action call priorities
	void dumpModuleActionPriorities(StelModule::StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Scriptable methods
public slots:
	//! Call this when the size of the GL window has changed.
	void glWindowHasBeenResized(const QRectF &rect);

	//! Set flag for activating night vision mode.
	void setVisionModeNight(bool);
	//! Get flag for activating night vision mode.
	bool getVisionModeNight() const {return flagNightVision;}

	//! Set flag for activating overwrite mode for text color in info panel.
	void setFlagOverwriteInfoColor(bool);
	//! Get flag for activating overwrite mode for text color in info panel.
	bool getFlagOverwriteInfoColor() const {return flagOverwriteInfoColor; }

	//! Set flag for showing decimal degree in various places.
	void setFlagShowDecimalDegrees(bool b);
	//! Get flag for showing decimal degree in various places.
	bool getFlagShowDecimalDegrees() const {return flagShowDecimalDegrees;}

	//! Set flag for using calculation of azimuth from south towards west (instead north towards east)
	bool getFlagSouthAzimuthUsage() const { return flagUseAzimuthFromSouth; }
	//! Get flag for using calculation of azimuth from south towards west (instead north towards east)
	void setFlagSouthAzimuthUsage(bool use) { flagUseAzimuthFromSouth=use; emit flagUseAzimuthFromSouthChanged(use);}
	
	//! Set flag for using of formatting output for coordinates
	void setFlagUseFormattingOutput(bool b);
	//! Get flag for using of formatting output for coordinates
	bool getFlagUseFormattingOutput() const {return flagUseFormattingOutput;}

	//! Set flag for using designations for celestial coordinate systems
	void setFlagUseCCSDesignation(bool b);
	//! Get flag for using designations for celestial coordinate systems
	bool getFlagUseCCSDesignation() const {return flagUseCCSDesignation;}

	//! Define info text color for overwrites
	void setOverwriteInfoColor(const Vec3f& color);
	//! Get info text color
	Vec3f getOverwriteInfoColor() const;

	//! Define info text color for daylight mode
	void setDaylightInfoColor(const Vec3f& color);
	//! Get info text color
	Vec3f getDaylightInfoColor() const;

	//! Get the current number of frame per second.
	//! @return the FPS averaged on the last second
	float getFps() const {return fps;}

	//! Set global application font.
	//! To retrieve, you can use QGuiApplication::font().
	//! emits fontChanged(font)
	void setAppFont(QFont font);

	//! Returns the default FBO handle, to be used when StelModule instances want to release their own FBOs.
	//! Note that this is usually not the same as QOpenGLContext::defaultFramebufferObject(),
	//! so use this call instead of the Qt version!
	//! Valid through a StelModule::draw() call, do not use elsewhere.
	quint32 getDefaultFBO() const { return currentFbo; }

	//! Makes sure the correct GL context used for main drawing is made current.
	//! This is always the case during init() and draw() calls, but if OpenGL access is required elsewhere,
	//! this MUST be called before using any GL functions.
	void ensureGLContextCurrent();

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
	void flagShowDecimalDegreesChanged(bool);
	void flagUseAzimuthFromSouthChanged(bool);
	void flagUseCCSDesignationChanged(bool);
	void flagUseFormattingOutputChanged(bool);
	void flagOverwriteInfoColorChanged(bool);
	void colorSchemeChanged(const QString&);
	void languageChanged();
	void screenFontSizeChanged(int);
	void guiFontSizeChanged(int);
	void fontChanged(QFont);
	void overwriteInfoColorChanged(const Vec3f & color);
	void daylightInfoColorChanged(const Vec3f & color);

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
	bool handleMove(qreal x, qreal y, Qt::MouseButtons b);
	//! Handle key press and release.
	void handleKeys(class QKeyEvent* event);
	//! Handle pinch on multi touch devices.
	void handlePinch(qreal scale, bool started);

	//! Used internally to set the viewport effects.
	void prepareRenderBuffer();
	//! Used internally to set the viewport effects.
	//! @param drawFbo the OpenGL fbo we need to render into.
	void applyRenderBuffer(quint32 drawFbo=0);

	QString getVersion() const;

	// The StelApp singleton
	static StelApp* singleton;

	//! The main window which is the parent of this object
	StelMainView* mainWin;

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

	//Property manager for the application
	StelPropertyMgr* propMgr;

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

#ifdef ENABLE_SCRIPTING
	// The script API proxy object (for bridging threads)
	StelMainScriptAPIProxy* scriptAPIProxy;

	// The script manager based on Qt script engine
	StelScriptMgr* scriptMgr;
#endif

	StelGuiBase* stelGui;
	
	// Store the ratio between real device pixel in "Device Independent Pixel"
	// Usually this value is 1, but for a mac with retina screen this will be value 2.
	qreal devicePixelsPerPixel;

	// The scaling ratio to apply on all display elements, like GUI, text etc..
	float globalScalingRatio;
	
	float fps;
	int frame;
	double frameTimeAccum;		// Used for fps counter

	//! Define whether we are in night vision mode
	bool flagNightVision;

	QSettings* confSettings;

	// Define whether the StelApp instance has completed initialization
	bool initialized;

	static qint64 startMSecs;
	static double animationScale;

	// Temporary variables used to store the last gl window resize
	// if the core was not yet initialized
	qreal saveProjW;
	qreal saveProjH;

	//! Store the number of downloaded files for statistics.
	int nbDownloadedFiles;
	//! Store the summed size of all downloaded files in bytes.
	qint64 totalDownloadedSize;

	//! Store the number of downloaded files read from the cache for statistics.
	int nbUsedCache;
	//! Store the summed size of all downloaded files read from the cache in bytes.
	qint64 totalUsedCacheSize;

	QList<StelProgressController*> progressControllers;

	int screenFontSize;

	// Framebuffer object used for viewport effects.
	QOpenGLFramebufferObject* renderBuffer;
	StelViewportEffect* viewportEffect;
	QOpenGLFunctions* gl;
	
	bool flagShowDecimalDegrees;  // Format infotext with decimal degrees, not minutes/seconds
	bool flagUseAzimuthFromSouth; // Display calculate azimuth from south towards west (as in some astronomical literature)
	bool flagUseFormattingOutput; // Use tabular coordinate format for infotext
	bool flagUseCCSDesignation;   // Use symbols like alpha (RA), delta (declination) for coordinate system labels
	bool flagOverwriteInfoColor; // Overwrite and use color for text in info panel
	Vec3f overwriteInfoColor;
	Vec3f daylightInfoColor;
#ifdef 	ENABLE_SPOUT
	SpoutSender* spoutSender;
#endif

	// The current main FBO/render target handle, without requiring GL queries. Valid through a draw() call
	quint32 currentFbo;
};

#endif // STELAPP_HPP
