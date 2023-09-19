/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#ifndef STELMAINVIEW_HPP
#define STELMAINVIEW_HPP

#include <QCoreApplication>
#include <QGraphicsView>
#include <QEventLoop>
#include <QOpenGLContext>
#include <QTimer>
#ifdef OPENGL_DEBUG_LOGGING
class QOpenGLDebugLogger;
class QOpenGLDebugMessage;
#endif
#include "VecMath.hpp"

class StelGLWidget;
class StelGraphicsScene;
class QMoveEvent;
class QResizeEvent;
class StelGuiBase;
class QMoveEvent;
class QSettings;

//! @class StelMainView
//! Reimplement a QGraphicsView for Stellarium.
//! It is the class creating the singleton GL Widget, the main StelApp instance as well as the main GUI.
class StelMainView : public QGraphicsView
{
	friend class StelGuiItem;
	friend class StelRootItem;
	friend class StelGraphicsScene;
	friend class NightModeGraphicsEffect;
	Q_OBJECT
	Q_PROPERTY(bool fullScreen                 READ isFullScreen                  WRITE setFullScreen                 NOTIFY fullScreenChanged)
	Q_PROPERTY(bool flagInvertScreenShotColors READ getFlagInvertScreenShotColors WRITE setFlagInvertScreenShotColors NOTIFY flagInvertScreenShotColorsChanged)
	Q_PROPERTY(bool flagScreenshotDateFileName READ getFlagScreenshotDateFileName WRITE setFlagScreenshotDateFileName NOTIFY flagScreenshotDateFileNameChanged)
	Q_PROPERTY(QString screenShotFileMask      READ getScreenshotFileMask         WRITE setScreenshotFileMask         NOTIFY screenshotFileMaskChanged)
	Q_PROPERTY(bool flagOverwriteScreenshots   READ getFlagOverwriteScreenShots   WRITE setFlagOverwriteScreenShots   NOTIFY flagOverwriteScreenshotsChanged)
	Q_PROPERTY(bool flagUseCustomScreenshotSize READ getFlagUseCustomScreenshotSize WRITE setFlagUseCustomScreenshotSize NOTIFY flagUseCustomScreenshotSizeChanged)
	Q_PROPERTY(int  customScreenshotWidth      READ getCustomScreenshotWidth      WRITE setCustomScreenshotWidth      NOTIFY customScreenshotWidthChanged)
	Q_PROPERTY(int  customScreenshotHeight     READ getCustomScreenshotHeight     WRITE setCustomScreenshotHeight     NOTIFY customScreenshotHeightChanged)
	Q_PROPERTY(int  screenshotDpi	           READ getScreenshotDpi              WRITE setScreenshotDpi              NOTIFY screenshotDpiChanged)
	Q_PROPERTY(QString screenShotFormat        READ getScreenshotFormat           WRITE setScreenshotFormat           NOTIFY screenshotFormatChanged)
	Q_PROPERTY(bool flagCursorTimeout          READ getFlagCursorTimeout          WRITE setFlagCursorTimeout          NOTIFY flagCursorTimeoutChanged)
	Q_PROPERTY(double cursorTimeout            READ getCursorTimeout              WRITE setCursorTimeout              NOTIFY cursorTimeoutChanged)
	Q_PROPERTY(Vec3f skyBackgroundColor        READ getSkyBackgroundColor         WRITE setSkyBackgroundColor         NOTIFY skyBackgroundColorChanged)
	Q_PROPERTY(int minFps	                   READ getMinFps                     WRITE setMinFps                     NOTIFY minFpsChanged)
	Q_PROPERTY(int maxFps	                   READ getMaxFps                     WRITE setMaxFps                     NOTIFY maxFpsChanged)
	Q_PROPERTY(int minTimeBetweenFrames        READ getMinTimeBetweenFrames       WRITE setMinTimeBetweenFrames       NOTIFY minTimeBetweenFramesChanged)
public:
	//! Contains some basic info about the OpenGL context used
	struct GLInfo
	{
		QString vendor;
		QString renderer;
		QSurface* surface;
		QOpenGLContext* mainContext;
		QOpenGLFunctions* functions;
		PFNGLMINSAMPLESHADINGPROC glMinSampleShading = nullptr;
		GLint maxAnisotropy = 0;
		GLint maxTextureSize = 2048;
		bool supportsLuminanceTextures = false;
		bool isCoreProfile = false;
		bool isHighGraphicsMode = false;
		bool isGLES = false;
	};

	StelMainView(QSettings* settings);
	~StelMainView() override;

	//! Start the main initialization of Stellarium
	void init();
	void deinit();

	//! Set the application title for the current language.
	//! This is useful for e.g. chinese.
	void initTitleI18n();

	//! Get the StelMainView singleton instance.
	static StelMainView& getInstance() {Q_ASSERT(singleton); return *singleton;}

	//! Delete openGL textures (to call before the GLContext disappears)
	void deinitGL();

	//! Return the parent gui widget, this should be used as parent to all
	//! the StelDialog instances.
	QGraphicsWidget* getGuiWidget() const {return guiItem;}
	//! Return mouse position coordinates
	QPoint getMousePos() const;

	//! Returns the main application OpenGL context,
	//! which should be used for all drawing Stellarium does
	//! @sa glContextMakeCurrent()
	//! @sa glContextDoneCurrent()
	QOpenGLContext* glContext() const;
	//! Make the main GL context (the one returned from glContext()) current
	//! on the main view surface
	void glContextMakeCurrent();
	//! Releases the main GL context
	void glContextDoneCurrent();

	//! Returns the information about the GL context, this does not require the context to be active.
	const GLInfo& getGLInformation() const { return glInfo; }

	//! Returns the desired OpenGL format settings.
	static QSurfaceFormat getDesiredGLFormat(QSettings *configuration);

public slots:

	//! Set whether fullscreen is activated or not
	void setFullScreen(bool);

	//! Return focus to the sky item.  To be used when we close a dialog.
	void focusSky();

	///////////////////////////////////////////////////////////////////////////
	// Specific methods
	//! Save a screen shot.
	//! The format of the file, and hence the filename extension
	//! depends on the architecture and build type.
	//! @arg filePrefix changes the beginning of the file name
	//! @arg saveDir changes the directory where the screenshot is saved
	//! If saveDir is "" then StelFileMgr::getScreenshotDir() will be used
	//! @arg overwrite if true, @arg filePrefix is used as filename, and existing file will be overwritten.
	//! @note To set file type, use setScreenshotFormat() first.
	void saveScreenShot(const QString& filePrefix="stellarium-", const QString& saveDir="", const bool overwrite=false);
	//! @arg filetype is the preferred file type (ending) like "png", "jpg", "bmp" etc.
	//! The supported filetypes depend on the underlying Qt version.
	//! The most popular may be PNG, JPG/JPEG, BMP, TIF (LZW compressed), TIFF (uncompressed), WEBP,
	//! but as of Qt5.12, we also have ICO (for thumbnails), PBM (Portable Bitmap), PGM (Portable Graymap), PPM (Portable Pixmap),
	//! XBM (X Bitmap) and XPM (X Pixmap).
	void setScreenshotFormat(const QString filetype);
	QString getScreenshotFormat() const {return screenShotFormat;}

	//! Get whether colors are inverted when saving screenshot
	bool getFlagInvertScreenShotColors() const {return flagInvertScreenShotColors;}
	//! Set whether colors should be inverted when saving screenshot
	void setFlagInvertScreenShotColors(bool b) {flagInvertScreenShotColors=b; emit flagInvertScreenShotColorsChanged(b);}

	//! Get whether date and time should be used for screenshot naming
	bool getFlagScreenshotDateFileName() const {return flagScreenshotDateFileName;}
	//! Set whether date and time should be used for screenshot naming
	void setFlagScreenshotDateFileName(bool b);
	void setScreenshotFileMask(const QString filemask);
	QString getScreenshotFileMask() const {return screenShotFileMask;}

	//! Get whether existing files are overwritten when saving screenshot
	bool getFlagOverwriteScreenShots() const {return flagOverwriteScreenshots;}
	//! Set whether existing files are overwritten when saving screenshot
	void setFlagOverwriteScreenShots(bool b) {flagOverwriteScreenshots=b; emit flagOverwriteScreenshotsChanged(b);}

	//! Get whether custom size should be used for screenshots
	bool getFlagUseCustomScreenshotSize() const {return flagUseCustomScreenshotSize;}
	//! Set whether custom size should be used for screenshots
	void setFlagUseCustomScreenshotSize(bool b) {flagUseCustomScreenshotSize=b; emit flagUseCustomScreenshotSizeChanged(b);}
	//! Get custom screenshot width
	int getCustomScreenshotWidth() const {return customScreenshotWidth;}
	//! Set custom width for screenshots
	void setCustomScreenshotWidth(int width) {customScreenshotWidth=width; emit customScreenshotWidthChanged(width);}
	//! Get custom screenshot height
	int getCustomScreenshotHeight() const {return customScreenshotHeight;}
	//! Set custom height for screenshots
	void setCustomScreenshotHeight(int height) {customScreenshotHeight=height; emit customScreenshotHeightChanged(height);}
	//! Get screenshot DPI. This is only an entry in the screenshot image metadata. The raster content is not influenced.
	int getScreenshotDpi() const {return screenshotDpi;}
	//! Set screenshot DPI. This is only an entry in the screenshot image metadata. The raster content is not influenced.
	void setScreenshotDpi(int dpi);
	//! Get screenshot magnification. This should be used by StarMgr, text drawing and other elements which may
	//! want to enlarge their output in screenshots to keep them visible.
	float getCustomScreenshotMagnification() const {return customScreenshotMagnification;}
	//! Get the state of the mouse cursor timeout flag
	bool getFlagCursorTimeout() const {return flagCursorTimeout;}
	//! Set the state of the mouse cursor timeout flag
	void setFlagCursorTimeout(bool b);
	//! Get the mouse cursor timeout in seconds
	double getCursorTimeout() const {return cursorTimeoutTimer->interval() / 1000.0;}
	//! Set the mouse cursor timeout in seconds
	void setCursorTimeout(double t) {cursorTimeoutTimer->setInterval(static_cast<int>(t * 1000)); emit cursorTimeoutChanged(t);}

	//! Set the minimum frames per second. Usually this minimum will be switched to after there are no
	//! user events for some seconds to save power. However, if can be useful to set this to a high
	//! value to improve playing smoothness in scripts.
	//! @param m the new minimum fps setting.
	void setMinFps(float m) {minfps=qMin(m, maxfps); emit minFpsChanged(minfps);}
	//! Get the current minimum frames per second.
	float getMinFps() const {return minfps;}
	//! Set the maximum frames per second.
	//! @param m the new maximum fps setting.
	void setMaxFps(float m) {maxfps = qMax(m, minfps);  emit maxFpsChanged(maxfps);}
	//! Get the current maximum frames per second.
	float getMaxFps() const {return maxfps;}
	//! Set the minimum time between frames (in milliseconds).
	//! @param m the new setting.
	void setMinTimeBetweenFrames(int m) {minTimeBetweenFrames = qMax(0, m);  emit minTimeBetweenFramesChanged(minTimeBetweenFrames);}
	//! Get the current minimum time between frames.
	int getMinTimeBetweenFrames() const {return minTimeBetweenFrames;}

	//! Notify that an event was handled by the program and therefore the
	//! FPS should be maximized for a couple of seconds.
	void thereWasAnEvent();

	//! Determines if we should render as fast as possible,
	//! or limit the FPS. This depends on the time the last user event
	//! happened.
	bool needsMaxFPS() const;

	//! Set the sky background color. (Actually forwards to the StelRootItem.)  Everything else than black creates a work of art!
	void setSkyBackgroundColor(Vec3f color);
	//! Get the sky background color. (Actually retrieves from the StelRootItem.)  Everything else than black creates a work of art!
	Vec3f getSkyBackgroundColor() const;

protected:
	//! Hack to determine current monitor pixel ratio
	//! @todo Find a better way to handle this
	void moveEvent(QMoveEvent* event) override;
	//! Handle window closed event, calling StelApp::quit()
	void closeEvent(QCloseEvent* event) override;
	//! Handle window resized events, and change the size of the underlying
	//! QGraphicsScene to be the same
	void resizeEvent(QResizeEvent* event) override;
	//! Wake up mouse cursor (if it was hidden)
	void mouseMoveEvent(QMouseEvent *event) override;

	bool eventFilter(QObject *obj, QEvent *event) override;
signals:
	//! emitted when window size has changed
	void sizeChanged(const QSize &sz);
	//! emitted when saveScreenShot is requested with saveScreenShot().
	//! doScreenshot() does the actual work (it has to do it in the main
	//! thread, where as saveScreenShot() might get called from another one.
	//!
	//! @remark FS: is threaded access here even a possibility anymore, or a remnant of older code?
	void screenshotRequested(void);
	void fullScreenChanged(bool b);
	//! Emitted when the "Reload shaders" action is performed
	//! Interested objects should subscribe to this signal and reload their shaders
	//! when this is emitted
	void reloadShadersRequested();

	void updateIconsRequested();
	void flagInvertScreenShotColorsChanged(bool b);
	void flagOverwriteScreenshotsChanged(bool b);
	void flagUseCustomScreenshotSizeChanged(bool use);
	void flagScreenshotDateFileNameChanged(bool flag);
	void customScreenshotWidthChanged(int width);
	void customScreenshotHeightChanged(int height);
	void screenshotDpiChanged(int dpi);
	void screenshotFormatChanged(QString format);
	void screenshotFileMaskChanged(QString format);

	void skyBackgroundColorChanged(Vec3f color);

	void flagCursorTimeoutChanged(bool b);
	void cursorTimeoutChanged(double t);
	void minFpsChanged(int fps);
	void maxFpsChanged(int fps);
	void minTimeBetweenFramesChanged(int tbf);

private slots:
	// Do the actual screenshot generation in the main thread with this method.
	void doScreenshot(void);

	void fpsTimerUpdate();
	void hideCursor();

#ifdef OPENGL_DEBUG_LOGGING
	void logGLMessage(const QOpenGLDebugMessage& debugMessage);
	void contextDestroyed();
#endif
	void updateNightModeProperty(bool b);

	void reloadShaders();

private:
	//! The graphics scene notifies us when a draw finished, so that we can queue the next one
	void drawEnded();
	//! provide extended OpenGL diagnostics in logfile.
	void dumpOpenGLdiagnostics() const;
	//! Startup diagnostics, providing test for various circumstances of bad OS/OpenGL driver combinations
	//! to provide feedback to the user about bad OpenGL drivers.
	void processOpenGLdiagnosticsAndWarnings(QSettings *conf, QOpenGLContext* context) const;

	//! The StelMainView singleton
	static StelMainView* singleton;

	GLInfo glInfo;

	QSettings* configuration;

	class StelRootItem* rootItem;
	QGraphicsWidget* guiItem;
	QGraphicsEffect* nightModeEffect;

	//! The openGL viewport of the graphics scene
	//! Responsible for main GL setup, rendering is done in the scene background
	StelGLWidget* glWidget;
	//! Custom QGraphicsScene, this renders our scene background
	StelGraphicsScene* stelScene;

	StelGuiBase* gui;
	class StelApp* stelApp;

	bool updateQueued;
	bool flagInvertScreenShotColors;
	bool flagScreenshotDateFileName; //! if set to true, screenshot is named by date and time format
	bool flagOverwriteScreenshots; //! if set to true, screenshot is named exactly screenShotPrefix.png and overwrites existing file
	bool flagUseCustomScreenshotSize; //! if true, the next 2 values are observed for screenshots.
	int customScreenshotWidth;            //! used when flagCustomResolutionScreenshots==true
	int customScreenshotHeight;           //! used when flagCustomResolutionScreenshots==true
	int screenshotDpi;                //! Image metadata entry for DPI. This does not influence the screenshot raster image content in any way, but some workflows like to have a configurable entry.
	float customScreenshotMagnification;  //! tracks the magnification factor customScreenshotHeight/NormalWindowHeight
	QString screenShotPrefix;
	QString screenShotFormat; //! file type like "png" or "jpg".
	QString screenShotFileMask;
	QString screenShotDir;

	bool flagCursorTimeout;
	//! Timer that triggers with the cursor timeout.
	QTimer* cursorTimeoutTimer;

	double lastEventTimeSec;

	//! The minimum desired frame rate in frame per second.
	float minfps;
	//! The maximum desired frame rate in frame per second.
	float maxfps;
	//! The minimum desired time between frames, in milliseconds.
	int minTimeBetweenFrames;
	QTimer* fpsTimer;

#ifdef OPENGL_DEBUG_LOGGING
	QOpenGLDebugLogger* glLogger;
#endif
};


#endif // STELMAINVIEW_HPP
