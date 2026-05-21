#ifndef PLANES_HPP
#define PLANES_HPP

#include "AircraftObject.hpp"
#include "StelLocation.hpp"
#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"

#include <QPointer>
#include <QSettings>
#include <QString>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class StelButton;
class PlanesDialog;

class Planes : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
	Q_PROPERTY(bool showLabels READ getFlagShowLabels WRITE setFlagShowLabels NOTIFY showLabelsChanged)
	Q_PROPERTY(bool showButton READ getFlagShowButton WRITE setFlagShowButton NOTIFY showButtonChanged)
	Q_PROPERTY(int labelMode READ getLabelMode WRITE setLabelMode NOTIFY labelModeChanged)

public:
	Planes();
	~Planes() override;

	void init() override;
	void deinit() override;
	void update(double deltaTime) override;
	void draw(StelCore* core) override;
	double getCallOrder(StelModuleActionName actionName) const override;
	bool configureGui(bool show=true) override;

	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;
	StelObjectP searchByName(const QString& name) const override;
	StelObjectP searchByID(const QString& id) const override;
	QVector<QPair<QString, StelObjectP>> listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const override;
	QVector<QPair<QString, StelObjectP>> listAllObjects(bool inEnglish) const override;

	QString getName() const override { return QStringLiteral("Planes"); }
	QString getStelObjectType() const override { return AircraftObject::STEL_TYPE; }
	bool isEnabled() const { return enabled; }
	bool getFlagShowLabels() const { return showLabels; }
	bool getFlagShowButton() const { return showButton; }
	int getLabelMode() const { return labelMode; }
	int getFetchIntervalSec() const { return fetchIntervalSec; }
	int getRadiusNm() const { return radiusNm; }
	QString getLastStatus() const { return lastStatus; }
	QString getLastSuccessfulUpdate() const { return lastSuccessfulUpdate; }
	QString getProviderId() const;
	QString getProviderDisplayName() const;
	QString getProviderWebsiteUrl() const;
	int getProviderMaxRadiusNm() const;
	QString getSourceUrlTemplate() const { return sourceUrlTemplate; }

public slots:
	void setEnabled(bool value);
	void setFlagShowLabels(bool value);
	void setFlagShowButton(bool value);
	void setLabelMode(int mode);
	void setProviderId(const QString& providerId);
	void setFetchIntervalSec(int seconds);
	void setRadiusNm(int nm);
	void refreshNow();

private slots:
	void fetchAircraft();
	void onReply(QNetworkReply* reply);
	void onLocationChanged(const StelLocation& loc);

signals:
	void enabledChanged(bool value);
	void showLabelsChanged(bool value);
	void showButtonChanged(bool value);
	void labelModeChanged(int mode);
	void providerChanged(const QString& providerId);
	void fetchIntervalChanged(int seconds);
	void radiusChanged(int nm);
	void statusChanged(const QString& status);

private:
	void loadSettings();
	void saveSettings() const;
	QString buildRequestUrl(const StelCore* core) const;
	void applyProviderDefaults(bool clampRadius=true);
	void scheduleRefresh(int delayMs=1200);
	void updateStatus(const QString& status, bool logInfo=false, bool logWarning=false);
	void applyButtonVisibility();

	QNetworkAccessManager* networkMgr;
	QTimer* fetchTimer;
	QTimer* refreshDebounceTimer;
	QPointer<QNetworkReply> inFlightReply;
	QVector<AircraftObjectP> aircraft;
	StelTextureSP planeTexture;
	StelTextureSP pointerTexture;

#ifndef NO_GUI
	PlanesDialog* configDialog;
	StelButton* toolbarButton;
#endif

	QString sourceUrlTemplate;
	int fetchIntervalSec;
	int radiusNm;
	bool pendingRefresh;
	bool enabled;
	bool showLabels;
	bool showButton;
	int labelMode;
	QString lastStatus;
	QString lastSuccessfulUpdate;
};

#include <QObject>
#include "StelPluginInterface.hpp"

class PlanesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)

public:
	StelModule* getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif
