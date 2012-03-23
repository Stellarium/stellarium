#ifndef _SCENERY3DMGR_HPP_
#define _SCENERY3DMGR_HPP_

#include <QMap>
#include <QStringList>
#include <QFont>

#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "gui/Scenery3dDialog.hpp"
#include "StelCore.hpp"
#include "StelFader.hpp"
#include "StelShader.hpp"

class Scenery3d;
class QSettings;
class StelButton;

//! Main class of the module, inherits from StelModule
class Scenery3dMgr : public StelModule
{
    Q_OBJECT
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

    static const QString MODULE_PATH;

public slots:
    //! GZ: for switching on/off
    void enableScenery3d(bool enable);

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
//signals:

private:
    QString nameToID(const QString& name);
    QMap<QString, QString> getNameToDirMap() const;

    //! Display text message on screen, fade out automatically
    void showMessage(const QString& message);

    bool flagEnabled;  // toggle to switch it off completely.
    int cubemapSize;   // configurable via config.ini:Scenery3d/cubemapSize
    int shadowmapSize; // configurable via config.ini:Scenery3d/shadowmapSize
    float torchBrightness; // configurable via config.ini:Scenery3d/extralight_brightness
    Scenery3d* scenery3d;
    Scenery3dDialog* scenery3dDialog;
    QString currentScenery3dID;
    QString defaultScenery3dID;
    bool enableShadows;  // toggle shadow mapping
    bool enableBumps;    // toggle bump mapping
    StelButton* toolbarEnableButton;
    StelButton* toolbarSettingsButton;
    StelCore::ProjectionType oldProjectionType;

    //Shader for shadow mapping
    StelShader* shadowShader;
    StelShader* bumpShader;
    StelShader* univShader;

    //screen messages (taken largely from AngleMeasure as of 2012-01-21)
    LinearFader messageFader;
    QTimer* messageTimer;
    Vec3f textColor;
    QFont font;
    QString currentMessage;
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class Scenery3dStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};


#endif

