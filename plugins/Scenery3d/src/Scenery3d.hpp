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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SCENERY3D_HPP
#define SCENERY3D_HPP

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

class S3DRenderer;
class Scenery3dDialog;
class StoredViewDialog;
class QSettings;
class StelButton;
class S3DScene;

Q_DECLARE_LOGGING_CATEGORY(scenery3d)

/*! @defgroup scenery3D 3D Sceneries plug-in
@{
3D foreground renderer. Walk around, find and avoid obstructions in
your garden, find and demonstrate possible astronomical alignments
in temples, see shadows on sundials etc.

To move around, press Ctrl+cursor keys. To lift eye height, use Ctrl+PgUp/PgDn.
Movement speed is linked to field of view (i.e. zoom in for fine adjustments).
You can even keep moving by releasing Ctrl before cursor key.

Development of this plugin was in parts supported by the Austrian
Science Fund (FWF) project ASTROSIM (P 21208-G19).
More: http://astrosim.univie.ac.at/
@}
*/

//! @class Scenery3d
//! @ingroup scenery3D
//! Main class of the 3D Sceneries plug-in.
//! @authors Georg Zotti, Simon Parzer, Peter Neubauer, Andrei Borza, Florian Schaukowitsch

//! Main class of the module, inherits from StelModule.
//! Manages initialization, provides an interface to change Scenery3d properties and handles user input
class Scenery3d : public StelModule
{
	Q_OBJECT

	// toggle to switch it off completely.
	Q_PROPERTY(bool enableScene                  READ getEnableScene            WRITE setEnableScene           NOTIFY enableSceneChanged)
	Q_PROPERTY(bool enablePixelLighting          READ getEnablePixelLighting    WRITE setEnablePixelLighting   NOTIFY enablePixelLightingChanged)
	Q_PROPERTY(bool enableShadows                READ getEnableShadows          WRITE setEnableShadows         NOTIFY enableShadowsChanged)
	Q_PROPERTY(bool useSimpleShadows             READ getUseSimpleShadows       WRITE setUseSimpleShadows      NOTIFY useSimpleShadowsChanged)
	Q_PROPERTY(bool enableBumps                  READ getEnableBumps            WRITE setEnableBumps           NOTIFY enableBumpsChanged)
	Q_PROPERTY(S3DEnum::ShadowFilterQuality shadowFilterQuality READ getShadowFilterQuality WRITE setShadowFilterQuality NOTIFY shadowFilterQualityChanged)
	Q_PROPERTY(bool enablePCSS                   READ getEnablePCSS             WRITE setEnablePCSS            NOTIFY enablePCSSChanged)
	Q_PROPERTY(S3DEnum::CubemappingMode cubemappingMode READ getCubemappingMode WRITE setCubemappingMode       NOTIFY cubemappingModeChanged)
	Q_PROPERTY(bool useFullCubemapShadows        READ getUseFullCubemapShadows  WRITE setUseFullCubemapShadows NOTIFY useFullCubemapShadowsChanged)
	Q_PROPERTY(bool enableDebugInfo              READ getEnableDebugInfo        WRITE setEnableDebugInfo       NOTIFY enableDebugInfoChanged)
	Q_PROPERTY(bool enableLocationInfo           READ getEnableLocationInfo     WRITE setEnableLocationInfo    NOTIFY enableLocationInfoChanged)
	Q_PROPERTY(bool forceHorizonPolyline         READ getForceHorizonPolyline   WRITE setForceHorizonPolyline  NOTIFY forceHorizonPolylineChanged)
	Q_PROPERTY(bool enableTorchLight             READ getEnableTorchLight       WRITE setEnableTorchLight      NOTIFY enableTorchLightChanged)
	Q_PROPERTY(float torchStrength               READ getTorchStrength          WRITE setTorchStrength         NOTIFY torchStrengthChanged)
	Q_PROPERTY(float torchRange                  READ getTorchRange             WRITE setTorchRange            NOTIFY torchRangeChanged)
	Q_PROPERTY(bool enableLazyDrawing            READ getEnableLazyDrawing      WRITE setEnableLazyDrawing     NOTIFY enableLazyDrawingChanged)
	Q_PROPERTY(double lazyDrawingInterval        READ getLazyDrawingInterval    WRITE setLazyDrawingInterval   NOTIFY lazyDrawingIntervalChanged)
	Q_PROPERTY(bool onlyDominantFaceWhenMoving   READ getOnlyDominantFaceWhenMoving    WRITE setOnlyDominantFaceWhenMoving   NOTIFY onlyDominantFaceWhenMovingChanged)
	Q_PROPERTY(bool secondDominantFaceWhenMoving READ getSecondDominantFaceWhenMoving  WRITE setSecondDominantFaceWhenMoving NOTIFY secondDominantFaceWhenMovingChanged)
	Q_PROPERTY(uint cubemapSize                  READ getCubemapSize            WRITE setCubemapSize   NOTIFY cubemapSizeChanged)
	Q_PROPERTY(uint shadowmapSize                READ getShadowmapSize          WRITE setShadowmapSize NOTIFY shadowmapSizeChanged)
	Q_PROPERTY(QString currentSceneID            READ getCurrentSceneID         NOTIFY currentSceneIDChanged STORED false)
	Q_PROPERTY(QString loadingSceneID            READ getLoadingSceneID         NOTIFY loadingSceneIDChanged STORED false)

	//these properties are only valid after init() has been called
	Q_PROPERTY(bool isGeometryShaderSupported    READ getIsGeometryShaderSupported)
	Q_PROPERTY(bool areShadowsSupported          READ getAreShadowsSupported)
	Q_PROPERTY(bool isShadowFilteringSupported   READ getIsShadowFilteringSupported)
	Q_PROPERTY(bool isANGLE                      READ getIsANGLE)
	Q_PROPERTY(uint maximumFramebufferSize       READ getMaximumFramebufferSize)

public:
    Scenery3d();
    virtual ~Scenery3d() Q_DECL_OVERRIDE;

    //StelModule members
    virtual void init() Q_DECL_OVERRIDE;
    virtual void deinit() Q_DECL_OVERRIDE;
    virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
    virtual void update(double deltaTime) Q_DECL_OVERRIDE;
    virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;
    virtual bool configureGui(bool show) Q_DECL_OVERRIDE;
    //! Walk/Fly Navigation with Ctrl+Cursor and Ctrl+PgUp/Dn keys.
    //! Pressing Ctrl-Alt: 5x, Ctrl-Shift: 10x speedup; Ctrl-Shift-Alt: 50x!
    //! To allow fine control, zoom in.
    //! If you release Ctrl key while pressing cursor key, movement will continue.
    virtual void handleKeys(QKeyEvent* e) Q_DECL_OVERRIDE;

    //! Sends the progressReport() signal, which eventually updates the progress bar. Can be called from another thread.
    void updateProgress(const QString& str, int val, int min, int max) const;
signals:
    void enableSceneChanged(const bool val);
    void enablePixelLightingChanged(const bool val);
    void enableShadowsChanged(const bool val);
    void useSimpleShadowsChanged(const bool val);
    void enableBumpsChanged(const bool val);
    void shadowFilterQualityChanged(const S3DEnum::ShadowFilterQuality val);
    void enablePCSSChanged(const bool val);
    void cubemappingModeChanged(const S3DEnum::CubemappingMode val);
    void useFullCubemapShadowsChanged(const bool val);
    void enableDebugInfoChanged(const bool val);
    void enableLocationInfoChanged(const bool val);
    void forceHorizonPolylineChanged(const bool val);
    void enableTorchLightChanged(const bool val);
    void torchStrengthChanged(const float val);
    void torchRangeChanged(const float val);
    void enableLazyDrawingChanged(const bool val);
    void lazyDrawingIntervalChanged(const double val);
    void onlyDominantFaceWhenMovingChanged(const bool val);
    void secondDominantFaceWhenMovingChanged(const bool val);
    void cubemapSizeChanged(const uint val);
    void shadowmapSizeChanged(const uint val);

    void currentSceneChanged(const SceneInfo& sceneInfo);
    void currentSceneIDChanged(const QString& sceneID);
    void loadingSceneIDChanged(const QString& sceneID);

    //! This signal is emitted from another thread than this QObject belongs to, so use QueuedConnection.
    void progressReport(const QString& str, int val, int min, int max) const;

public slots:
    //! Performs a relative translation of the viewer's position.
    //! The values should usually be given pre-adjusted with the delta time.
    //! Does nothing if no scene is currently loaded.
    //!
    //! The first component specifies movement in the "right" direction, the second
    //! component in the "forward" directon, and the last value
    //! is used to adjust the height offset of the viewer (he is still
    //! positioned according to the heightmap).
    void relativeMove(const Vec3d& move);

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

    //! If true, only 1 shadow cascade is used, giving a speedup
    void setUseSimpleShadows(const bool simpleShadows);
    bool getUseSimpleShadows() const;

    //! Use this to set/get the enableBumps flag.
    //! If set to true, bump mapping is enabled for the 3D scene.
    void setEnableBumps(const bool enableBumps);
    bool getEnableBumps(void) const;

    //! Returns the current shadow filter quality.
    S3DEnum::ShadowFilterQuality getShadowFilterQuality(void) const;
    //! Sets the shadow filter quality
    void setShadowFilterQuality(const S3DEnum::ShadowFilterQuality val);

    void setEnablePCSS(const bool val);
    bool getEnablePCSS() const;

    //! Returns the current cubemapping mode
    S3DEnum::CubemappingMode getCubemappingMode(void) const;
    //! Sets the cubemapping mode
    void setCubemappingMode(const S3DEnum::CubemappingMode val);

    bool getUseFullCubemapShadows() const;
    void setUseFullCubemapShadows(const bool useFullCubemapShadows);

    //! Set to true to show some rendering debug information
    void setEnableDebugInfo(const bool debugEnabled);
    bool getEnableDebugInfo() const;

    //! Set to true to show the current standing positin as text on screen.
    void setEnableLocationInfo(const bool enableLocationInfo);
    bool getEnableLocationInfo() const;

    //! Set the overdrawing of a landscape (horizon) polygon after the 3D scenery.
    //! This shows the difference (error) between our planar (tangential plane) modelling and effects of earth curvature.
    //! The landscape has to include such a polygon, of course.
    void setForceHorizonPolyline(const bool forcePolyline);
    bool getForceHorizonPolyline() const;

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

    //! When true, only the face which currently is most dominantly visible is updated while moving.
    void setOnlyDominantFaceWhenMoving(const bool val);
    bool getOnlyDominantFaceWhenMoving() const;

    void setSecondDominantFaceWhenMoving(const bool val);
    bool getSecondDominantFaceWhenMoving() const;

    //! Forces a redraw of the cubemap
    void forceCubemapRedraw();

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

    //these properties are only valid after init() has been called
    bool getIsGeometryShaderSupported() const;
    bool getAreShadowsSupported() const;
    bool getIsShadowFilteringSupported() const;
    bool getIsANGLE() const;
    uint getMaximumFramebufferSize() const;

    //! Gets the SceneInfo of the scene that is currently being displayed.
    //! Check SceneInfo::isValid to determine if a scene is displayed.
    SceneInfo getCurrentScene() const;

    //! Returns the ID of the currently loaded scene, or an null string if nothing is displayed.
    QString getCurrentSceneID() const;

    //! Returns the ID of the currently loading scene, or null if nothing is being loaded
    QString getLoadingSceneID() const;

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

    //! Changes the current view to the given view. JD is updated only if view contains valid data and setDate is true
    void setView(const StoredView& view, const bool setDate);
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
    //! Creates the toolbar buttons of the plugin
    void createToolbarButtons() const;

    //! This is run asynchronously in a background thread, performing the actual scene loading
    S3DScene *loadSceneBackground(const SceneInfo &scene) const;

    // the other "main" objects
    S3DRenderer* renderer;
    Scenery3dDialog* scenery3dDialog;
    StoredViewDialog* storedViewDialog;

    QSettings* conf;
    QString defaultScenery3dID;
    bool flagEnabled;
    bool cleanedUp;

    Vec3d movementKeyInput;

    StelCore* core;
    StelMovementMgr* mvMgr;
    StelCore::ProjectionType oldProjectionType;

    //screen messages (taken largely from AngleMeasure as of 2012-01-21)
    LinearFader messageFader;
    QTimer* messageTimer;
    Vec3f textColor;
    QFont font;
    QString currentMessage;
    bool forceHorizonPolyline; // if true, the LandscapeMgr is called after scene rendering to repeat rendering the landscape polygon, if one has been defined in the current Landscape.

    volatile bool loadCancel;
    StelProgressController* progressBar;
    SceneInfo currentLoadScene;
    S3DScene* currentScene;
    QFutureWatcher<S3DScene*> currentLoadFuture;
};


#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class Scenery3dStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const;
};


#endif // SCENERY3D_HPP

