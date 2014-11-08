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

#include "StelCore.hpp"
#include "StelPluginInterface.hpp"
#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "StelFader.hpp"

#include "gui/Scenery3dDialog.hpp"

class Scenery3d;
class QSettings;
class StelButton;

class QOpenGLShaderProgram;

//! Main class of the module, inherits from StelModule
class Scenery3dMgr : public StelModule
{
    Q_OBJECT

    // toggle to switch it off completely.
    Q_PROPERTY(bool scenery3dEnabled
               READ isScenery3dEnabled
               WRITE setScenery3dEnabled
               NOTIFY scenery3dEnabledChanged)

public:
    Scenery3dMgr();
    virtual ~Scenery3dMgr();

    virtual void init();
    virtual void draw(StelCore* core);
    virtual void update(double deltaTime);
    virtual void updateI18n();
    virtual void setStelStyle(const QString& section);
    virtual double getCallOrder(StelModuleActionName actionName) const;
    virtual bool configureGui(bool show);
    virtual void handleKeys(QKeyEvent* e);

    //! Loads (or reloads) the required shaders from the shader files.
    void loadShaders();
    bool loadShader(QOpenGLShaderProgram& program, const QString& vShader, const QString& fShader);
    bool load(QMap<QString, QString>& param);
    Scenery3d* createFromFile(const QString& file, const QString& id);

    //! Use this to set/get the enableShadows flag.
    //! If set to true, shadow mapping is enabled for the 3D scene.
    void setEnableShadows(bool enableShadows);
    bool getEnableShadows(void){return enableShadows;}
    //! Use this to set/get the enableBumps flag.
    //! If set to true, bump mapping is enabled for the 3D scene.
    void setEnableBumps(bool enableBumps);
    bool getEnableBumps(void){return enableBumps;}
    //! Use this to set/get the enableFilter flag.
    //! If set to true, the shadows in the 3D scene are filtered.
    void setEnableShadowsFilter(bool enableShadowsFilter);
    bool getEnableShadowsFilter(void){return enableShadowsFilter;}
    //! Use this to set/get the enableFilter flag.
    //! If set to true, the shadows in the 3D scene are filtered using more taps per pass.
    void setEnableShadowsFilterHQ(bool enableShadowsFilterHQ);
    bool getEnableShadowsFilterHQ(void){return enableShadowsFilterHQ;}


    static const QString MODULE_PATH;

signals:
    void scenery3dEnabledChanged(const bool val);

public slots:
    //! Enables/Disables the plugin
    void setScenery3dEnabled(const bool val);
    bool isScenery3dEnabled() const;

    QStringList getAllScenery3dNames() const;
    QStringList getAllScenery3dIDs() const;

    const QString& getCurrentScenery3dID() const { return currentScenery3dID; }
    bool setCurrentScenery3dID(const QString& id);
    QString getCurrentScenery3dName() const;
    bool setCurrentScenery3dName(const QString& name);
    const QString& getDefaultScenery3dID() const { return defaultScenery3dID; }
    bool setDefaultScenery3dID(const QString& id);
    QString getCurrentScenery3dHtmlDescription() const;

    QString getScenery3dPath(QString scenery3dID);
    QString loadScenery3dName(QString scenery3dID);
    quint64 loadScenery3dSize(QString scenery3dID);

private slots:
    void clearMessage();

private:
    QString nameToID(const QString& name);
    QMap<QString, QString> getNameToDirMap() const;

    //! Display text message on screen, fade out automatically
    void showMessage(const QString& message);


    int cubemapSize;   // configurable via config.ini:Scenery3d/cubemapSize
    int shadowmapSize; // configurable via config.ini:Scenery3d/shadowmapSize
    float torchBrightness; // configurable via config.ini:Scenery3d/extralight_brightness
    Scenery3d* scenery3d;
    Scenery3dDialog* scenery3dDialog;
    QString currentScenery3dID;
    QString defaultScenery3dID;
    bool flagEnabled;
    bool enableShadows;          // toggle shadow mapping
    bool enableBumps;            // toggle bump mapping
    bool enableShadowsFilter;    // toggle shadow filtering
    bool enableShadowsFilterHQ;  // toggle shadow filtering HQ
    StelButton* toolbarEnableButton;
    StelButton* toolbarSettingsButton;
    StelCore::ProjectionType oldProjectionType;

    //Shader for shadow mapping
    QOpenGLShaderProgram* shadowShader;
    QOpenGLShaderProgram* bumpShader;
    QOpenGLShaderProgram* univShader;
    QOpenGLShaderProgram* debugShader;

    //screen messages (taken largely from AngleMeasure as of 2012-01-21)
    LinearFader messageFader;
    QTimer* messageTimer;
    Vec3f textColor;
    QFont font;
    QString currentMessage;
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

