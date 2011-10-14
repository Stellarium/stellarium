#ifndef _SCENERY3DMGR_HPP_
#define _SCENERY3DMGR_HPP_

#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "gui/Scenery3dDialog.hpp"
#include "StelCore.hpp"

#include <QMap>
#include <QStringList>
#include <QKeyEvent>

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

    //! Use this to set the enableShadows flag.
    //! If set to true, shadow mapping is enabled for the 3D scene.
    void setEnableShadows(bool enableShadows);

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

//signals:

private:
    QString nameToID(const QString& name);
    QMap<QString, QString> getNameToDirMap() const;

    bool flagEnabled;  // toggle to switch it off completely.
    int cubemapSize;   // configurable via config.ini:Scenery3d/cubemapSize
    int shadowmapSize; // configurable via config.ini:Scenery3d/shadowmapSize
    Scenery3d* scenery3d;
    Scenery3dDialog* scenery3dDialog;
    QString currentScenery3dID;
    QString defaultScenery3dID;
    bool enableShadows;  // toggle shadow mapping
    StelButton* toolbarEnableButton;
    StelButton* toolbarSettingsButton;
    StelCore::ProjectionType oldProjectionType;
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

