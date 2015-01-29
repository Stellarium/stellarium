/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _SCENERY3DMGR_HPP_
#define _SCENERY3DMGR_HPP_

#include <QMap>
#include <QStringList>
#include <QFont>
#include <QtConcurrent>

#include "StelCore.hpp"
#include "StelPluginInterface.hpp"
#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "StelFader.hpp"
#include "StelActionMgr.hpp"
#include "StelProgressController.hpp"

#include "SceneInfo.hpp"
#include "S3DEnum.hpp"

class Scenery3d;
class Scenery3dDialog;
class StoredViewDialog;
class QSettings;
class StelButton;

class QOpenGLShaderProgram;

//! Main class of the module, inherits from StelModule
class Scenery3dMgr : public StelModule
{
	Q_OBJECT

	// toggle to switch it off completely.
	Q_PROPERTY(bool enableScene  READ getEnableScene WRITE setEnableScene NOTIFY enableSceneChanged)
	Q_PROPERTY(bool enablePixelLighting READ getEnablePixelLighting WRITE setEnablePixelLighting NOTIFY enablePixelLightingChanged)
	Q_PROPERTY(bool enableShadows READ getEnableShadows WRITE setEnableShadows NOTIFY enableShadowsChanged)
	Q_PROPERTY(bool enableBumps READ getEnableBumps WRITE setEnableBumps NOTIFY enableBumpsChanged)
	Q_PROPERTY(S3DEnum::ShadowFilterQuality shadowFilterQuality READ getShadowFilterQuality WRITE setShadowFilterQuality NOTIFY shadowFilterQualityChanged)
	Q_PROPERTY(S3DEnum::CubemappingMode cubemappingMode READ getCubemappingMode WRITE setCubemappingMode NOTIFY cubemappingModeChanged)
	Q_PROPERTY(bool enableDebugInfo READ getEnableDebugInfo WRITE setEnableDebugInfo NOTIFY enableDebugInfoChanged)
	Q_PROPERTY(bool enableLocationInfo READ getEnableLocationInfo WRITE setEnableLocationInfo NOTIFY enableLocationInfoChanged)
	Q_PROPERTY(bool enableTorchLight READ getEnableTorchLight WRITE setEnableTorchLight NOTIFY enableTorchLightChanged)
	Q_PROPERTY(float torchStrength READ getTorchStrength WRITE setTorchStrength NOTIFY torchStrengthChanged)
	Q_PROPERTY(float torchRange READ getTorchRange WRITE setTorchRange NOTIFY torchRangeChanged)
	Q_PROPERTY(bool enableLazyDrawing READ getEnableLazyDrawing WRITE setEnableLazyDrawing NOTIFY enableLazyDrawingChanged)
	Q_PROPERTY(double lazyDrawingInterval READ getLazyDrawingInterval WRITE setLazyDrawingInterval NOTIFY lazyDrawingIntervalChanged)
	Q_PROPERTY(bool isGeometryShaderSupported READ getIsGeometryShaderSupported NOTIFY isGeometryShaderSupportedChanged)
	Q_PROPERTY(uint cubemapSize READ getCubemapSize WRITE setCubemapSize NOTIFY cubemapSizeChanged)
	Q_PROPERTY(uint shadowmapSize READ getShadowmapSize WRITE setShadowmapSize NOTIFY shadowmapSizeChanged)

public:
    Scenery3dMgr();
    virtual ~Scenery3dMgr();

    //StelModule members
    virtual void init();
    virtual void deinit();
    virtual void draw(StelCore* core);
    virtual void update(double deltaTime);
    virtual double getCallOrder(StelModuleActionName actionName) const;
    virtual bool configureGui(bool show);
    virtual void handleKeys(QKeyEvent* e);

    //! Sends the progressReport() signal, which eventually updates the progress bar. Can be called from another thread.
    void updateProgress(const QString& str, int val, int min, int max);
signals:
    void enableSceneChanged(const bool val);
    void enablePixelLightingChanged(const bool val);
    void enableShadowsChanged(const bool val);
    void enableBumpsChanged(const bool val);
    void shadowFilterQualityChanged(const S3DEnum::ShadowFilterQuality val);
    void cubemappingModeChanged(const S3DEnum::CubemappingMode val);
    void enableDebugInfoChanged(const bool val);
    void enableLocationInfoChanged(const bool val);
    void enableTorchLightChanged(const bool val);
    void torchStrengthChanged(const float val);
    void torchRangeChanged(const float val);
    void enableLazyDrawingChanged(const bool val);
    void lazyDrawingIntervalChanged(const double val);
    void isGeometryShaderSupportedChanged(const bool val);
    void cubemapSizeChanged(const uint val);
    void shadowmapSizeChanged(const uint val);

    void currentSceneChanged(const SceneInfo& sceneInfo);

    //! This signal is emitted from another thread than this QObject belongs to, so use QueuedConnection.
    void progressReport(const QString& str, int val, int min, int max);

public slots:
    //! Clears the shader cache, forcing a reload of shaders on use
    void reloadShaders();

    //! Display text message on screen, fade out automatically
    void showMessage(const QString& message);

    //! Shows the stored view dialog
    void showStoredViewDialog();

    //! Enables/Disables the plugin
    void setEnableScene(const bool val);
    bool getEnableScene() const {return flagEnabled; }

    void setEnablePixelLighting(const bool val);
    bool getEnablePixelLighting(void) const;

    //! Use this to set/get the enableShadows flag.
    //! If set to true, shadow mapping is enabled for the 3D scene.
    void setEnableShadows(const bool enableShadows);
    bool getEnableShadows(void) const;

    //! Use this to set/get the enableBumps flag.
    //! If set to true, bump mapping is enabled for the 3D scene.
    void setEnableBumps(const bool enableBumps);
    bool getEnableBumps(void) const;

    //! Returns the current shadow filter quality.
    S3DEnum::ShadowFilterQuality getShadowFilterQuality(void) const;
    //! Sets the shadow filter quality
    void setShadowFilterQuality(const S3DEnum::ShadowFilterQuality val);

    //! Returns the current cubemapping mode
    S3DEnum::CubemappingMode getCubemappingMode(void) const;
    //! Sets the cubemapping mode
    void setCubemappingMode(const S3DEnum::CubemappingMode val);

    //! Set to true to show some rendering debug information
    void setEnableDebugInfo(const bool debugEnabled);
    bool getEnableDebugInfo() const;

    //! Set to true to show the current standing positin as text on screen.
    void setEnableLocationInfo(const bool enableLocationInfo);
    bool getEnableLocationInfo() const;

    //! Set to true to add an additional light source centered at the current position, useful in night scenes.
    void setEnableTorchLight(const bool enableTorchLight);
    bool getEnableTorchLight() const;

    //! Sets the strength of the additional illumination that can be toggled when pressing a button.
    void setTorchStrength(const float torchStrength);
    float getTorchStrength() const;

    //! Sets the range of the torchlight.
    void setTorchRange(const float torchRange);
    float getTorchRange() const;

    //! Sets the state of the cubemap lazy-drawing mode
    void setEnableLazyDrawing(const bool val);
    bool getEnableLazyDrawing() const;

    //! Sets the interval for cubemap lazy-drawing mode
    void setLazyDrawingInterval(const double val);
    double getLazyDrawingInterval() const;

    //! Sets the size used for cubemap rendering.
    //! For best compatibility and performance, this should be a power of 2.
    void setCubemapSize(const uint val);
    uint getCubemapSize() const;

    //! Sets the size used for shadowmap rendering.
    //! For best compatibility and performance, this should be a power of 2.
    void setShadowmapSize(const uint val);
    uint getShadowmapSize() const;

    bool getIsGeometryShaderSupported() const;

    //! Gets the SceneInfo of the scene that is currently being displayed.
    //! Check SceneInfo::isValid to determine if a scene is displayed.
    SceneInfo getCurrentScene() const;

    //! Gets the SceneInfo of the scene that is currently in the process of being loaded.
    //! Check SceneInfo::isValid to determine if a scene is loaded.
    SceneInfo getLoadingScene() const { return currentLoadScene; }

    //! This starts the scene loading process. This is asynchronous, this method returns after metadata loading.
    //! @param name a valid scene name
    //! @return The loaded SceneInfo. Check SceneInfo::isValid to make sure loading was successful.
    SceneInfo loadScenery3dByName(const QString& name);
    //! This starts the scene loading process. This is asynchronous, this method returns after metadata loading.
    //! @param id a valid scene id/folder path
    //! @return The loaded SceneInfo. Check SceneInfo::isValid to make sure loading was successful.
    SceneInfo loadScenery3dByID(const QString& id);

    QString getDefaultScenery3dID() const { return defaultScenery3dID; }
    void setDefaultScenery3dID(const QString& id);

    //! Changes the current view to the given view
    void setView(const StoredView& view);
    //! Returns a StoredView that represents the current observer position + view direction.
    //! Label and description are empty.
    StoredView getCurrentView();

private slots:
    void clearMessage();
    void loadSceneCompleted();
    void progressReceive(const QString& str, int val, int min, int max);
    void loadScene(const SceneInfo& scene);

private:
    //! Loads config values from app settings
    void loadConfig();
    //! Creates all actions required by the plugin
    void createActions();

    //! Loads the given vertex and fragment shaders into the given program.
    bool loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& fShader);

    //! This is run asynchronously in a background thread
    bool loadSceneBackground();

    Scenery3d* scenery3d;
    Scenery3dDialog* scenery3dDialog;
    StoredViewDialog* storedViewDialog;

    QSettings* conf;
    QString defaultScenery3dID;
    bool flagEnabled;
    bool cleanedUp;

    StelCore::ProjectionType oldProjectionType;

    QOpenGLShaderProgram* debugShader;

    //screen messages (taken largely from AngleMeasure as of 2012-01-21)
    LinearFader messageFader;
    QTimer* messageTimer;
    Vec3f textColor;
    QFont font;
    QString currentMessage;

    StelProgressController* progressBar;
    SceneInfo currentLoadScene;
    QFutureWatcher<bool> currentLoadFuture;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class Scenery3dStelPluginInterface : public QObject, public StelPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
    Q_INTERFACES(StelPluginInterface)
public:
    virtual StelModule* getStelModule() const;
    virtual StelPluginInfo getPluginInfo() const;
};


#endif

