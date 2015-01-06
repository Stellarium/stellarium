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

#include "gui/Scenery3dDialog.hpp"
#include "SceneInfo.hpp"

class Scenery3d;
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
	Q_PROPERTY(bool enableShadowsFilter READ getEnableShadowsFilter WRITE setEnableShadowsFilter NOTIFY enableShadowsFilterChanged)
	Q_PROPERTY(bool enableShadowsFilterHQ READ getEnableShadowsFilterHQ WRITE setEnableShadowsFilterHQ NOTIFY enableShadowsFilterHQChanged)
	Q_PROPERTY(bool enableDebugInfo READ getEnableDebugInfo WRITE setEnableDebugInfo NOTIFY enableDebugInfoChanged)
	Q_PROPERTY(bool enableLocationInfo READ getEnableLocationInfo WRITE setEnableLocationInfo NOTIFY enableLocationInfoChanged)
	Q_PROPERTY(bool enableTorchLight READ getEnableTorchLight WRITE setEnableTorchLight NOTIFY enableTorchLightChanged)
	Q_PROPERTY(bool enableGeometryShader READ getEnableGeometryShader WRITE setEnableGeometryShader NOTIFY enableGeometryShaderChanged)
	Q_PROPERTY(bool isGeometryShaderSupported READ getIsGeometryShaderSupported NOTIFY isGeometryShaderSupportedChanged)

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
    void enableShadowsFilterChanged(const bool val);
    void enableShadowsFilterHQChanged(const bool val);
    void enableDebugInfoChanged(const bool val);
    void enableLocationInfoChanged(const bool val);
    void enableTorchLightChanged(const bool val);
    void enableGeometryShaderChanged(const bool val);
    void isGeometryShaderSupportedChanged(const bool val);

    //! This signal is emitted from another thread than this QObject belongs to, so use QueuedConnection.
    void progressReport(const QString& str, int val, int min, int max);

public slots:
    //! Clears the shader cache, forcing a reload of shaders on use
    void reloadShaders();

    //! Display text message on screen, fade out automatically
    void showMessage(const QString& message);

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
    //! Use this to set/get the enableFilter flag.
    //! If set to true, the shadows in the 3D scene are filtered.
    void setEnableShadowsFilter(const bool enableShadowsFilter);
    bool getEnableShadowsFilter(void) const;
    //! Use this to set/get the enableFilter flag.
    //! If set to true, the shadows in the 3D scene are filtered using more taps per pass.
    void setEnableShadowsFilterHQ(const bool enableShadowsFilterHQ);
    bool getEnableShadowsFilterHQ(void) const;
    //! Set to true to show some rendering debug information
    void setEnableDebugInfo(const bool debugEnabled);
    bool getEnableDebugInfo() const;
    //! Set to true to show the current standing positin as text on screen.
    void setEnableLocationInfo(const bool enableLocationInfo);
    bool getEnableLocationInfo() const;
    //! Set to true to add an additional light source centered at the current position, useful in night scenes.
    void setEnableTorchLight(const bool enableTorchLight);
    bool getEnableTorchLight() const;
    //! Set to true to enable geometry shader processing for cubemap creation
    void setEnableGeometryShader(const bool enableGeometryShader);
    bool getEnableGeometryShader() const;

    bool getIsGeometryShaderSupported() const;

    QString getCurrentScenery3dID() const;
    bool setCurrentScenery3dID(const QString& id);
    QString getCurrentScenery3dName() const;
    bool setCurrentScenery3dName(const QString& name);
    QString getDefaultScenery3dID() const { return defaultScenery3dID; }
    bool setDefaultScenery3dID(const QString& id);
    QString getCurrentScenery3dHtmlDescription() const;

private slots:
    void clearMessage();
    void loadSceneCompleted();
    void progressReceive(const QString& str, int val, int min, int max);

private:
    //! Loads config values from app settings
    void loadConfig();
    //! Creates all actions required by the plugin
    void createActions();

    //! Loads the given vertex and fragment shaders into the given program.
    bool loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& fShader);

    void loadScene(const SceneInfo& scene);

    //! This is run asynchronously in a background thread
    bool loadSceneBackground();

    Scenery3d* scenery3d;
    Scenery3dDialog* scenery3dDialog;

    QSettings* conf;
    QString defaultScenery3dID;
    bool flagEnabled;
    bool cleanedUp;

    StelAction* enableAction;
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

