#include "Planet.hpp"
#include "SolarSystem.hpp"
#include "StelFader.hpp"
#include "StelModule.hpp"
#include "VecMath.hpp"
#include <QFont>
#include <QPair>
#include <QString>

class Observability : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ isEnabled WRITE enableReport NOTIFY flagReportVisibilityChanged)

public:
	Observability();
	~Observability();

	virtual void init();
	virtual void update(double);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;

  //! Restore the plug-in's settings to the default state.
	//! Replace the plug-in's settings in Stellarium's configuration file
	//! with the default values and re-load them.
	//! Uses internally loadSettings() and saveSettings().
	void restoreDefaultSettings();
	void loadSettings();

signals:
  void flagReportVisibilityChanged(bool visible);
  void flagShowGoodNightsChanged(bool b);
  void flagShowBestNightChanged(bool b);
  void flagShowTodayChanged(bool b);
  void flagShowFullMoonChanged(bool b);


public slots:
  bool isEnabled() const { return flagShowReport; }

  void enableReport(bool enabled);

  void showGoodNights(bool b);
	void showBestNight(bool b);
	void showToday(bool b);
	void showFullMoon(bool b);

private:
	bool flagShowReport;
	bool flagShowGoodNights;
	bool flagShowBestNight;
	bool flagShowToday;
	bool flagShowFullMoon;

	QSettings* config;
};

#include "StelPluginInterface.hpp"
#include <QObject>

//! This class is used by Qt to manage a plug-in interface
class AngleMeasureStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};