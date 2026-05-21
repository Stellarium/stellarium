/*
 * Copyright (C) 2013 Felix Zeltner
 * Copyright (C) 2026 Kamil Zaraś (astronow.pl)
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

#include "Planes.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocation.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelProjector.hpp"
#include "StelTextureMgr.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

#ifndef NO_GUI
#include "gui/PlanesDialog.hpp"
#endif

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <cmath>

namespace
{
struct ProviderDefinition
{
	QString id;
	QString displayName;
	QString urlTemplate;
	QString websiteUrl;
	int maxRadiusNm;
};

const QString kAdsbFiTemplate = QStringLiteral("https://opendata.adsb.fi/api/v2/lat/%1/lon/%2/dist/%3");
const QString kAirplanesLiveTemplate = QStringLiteral("https://api.airplanes.live/v2/point/%1/%2/%3");
const QString kPluginVersion = QStringLiteral("0.1.0");
constexpr int kLabelModeFlightNumber = 0;
constexpr int kLabelModeAircraftModel = 1;
constexpr int kDefaultFetchIntervalSec = 15;
constexpr int kMinFetchIntervalSec = 15;
constexpr int kMaxFetchIntervalSec = 60;
constexpr int kDefaultRadiusNm = 250;
constexpr int kMinRadiusNm = 25;
constexpr int kMaxRadiusNm = 500;
constexpr int kMaxPublishedAircraft = 200;
constexpr double kFeetToMeters = 0.3048;
constexpr double kKnotsToMetersPerSecond = 0.514444;
constexpr double kFeetPerMinuteToMetersPerSecond = 0.00508;
const QString kRealtimeOnlyStatus = QStringLiteral("Live aircraft are shown only in real-time mode.");

ProviderDefinition providerDefinition(const QString& providerId)
{
	if (providerId == QStringLiteral("airplanes_live"))
	{
		return {
			QStringLiteral("airplanes_live"),
			QStringLiteral("airplanes.live"),
			kAirplanesLiveTemplate,
			QStringLiteral("https://airplanes.live/api-guide/"),
			250
		};
	}

	return {
		QStringLiteral("adsb_fi"),
		QStringLiteral("adsb.fi"),
		kAdsbFiTemplate,
		QStringLiteral("https://adsb.fi/"),
		kMaxRadiusNm
	};
}

QString providerIdFromTemplate(const QString& urlTemplate)
{
	if (urlTemplate.contains(QStringLiteral("airplanes.live"), Qt::CaseInsensitive))
		return QStringLiteral("airplanes_live");
	return QStringLiteral("adsb_fi");
}

int sanitizeInterval(int seconds)
{
	if (seconds <= 15)
		return 15;
	if (seconds <= 20)
		return 20;
	if (seconds <= 30)
		return 30;
	return 60;
}

int sanitizeRadius(int nm)
{
	if (nm < kMinRadiusNm)
		return kMinRadiusNm;
	if (nm > kMaxRadiusNm)
		return kMaxRadiusNm;
	return nm;
}

int sanitizeLabelMode(int mode)
{
	return mode == kLabelModeAircraftModel ? kLabelModeAircraftModel : kLabelModeFlightNumber;
}

bool shouldSkipAircraft(const QJsonObject& object)
{
	if (!object.contains("lat") || !object.contains("lon"))
		return true;
	if (object.value("lat").isNull() || object.value("lon").isNull())
		return true;
	return object.value("alt_baro").toString() == QStringLiteral("ground");
}

AircraftRecord parseAircraftRecord(const QJsonObject& object, double snapshotJd)
{
	AircraftRecord record;
	record.icao24 = object.value("hex").toString().trimmed().toLower();
	record.callsign = object.value("flight").toString().trimmed();
	record.aircraftType = object.value("t").toString().trimmed();
	record.latitude = object.value("lat").toDouble();
	record.longitude = object.value("lon").toDouble();
	record.altitudeMeters = object.value("alt_baro").toDouble() * kFeetToMeters;
	record.groundSpeedMs = object.value("gs").toDouble() * kKnotsToMetersPerSecond;
	record.trackDegrees = object.value("track").toDouble();
	record.verticalRateMs = object.value("baro_rate").toDouble() * kFeetPerMinuteToMetersPerSecond;
	record.snapshotJd = snapshotJd;
	return record;
}

QVector<AircraftRecord> parseAircraftRecords(const QJsonArray& aircraftArray, double snapshotJd)
{
	QVector<AircraftRecord> nextRecords;
	nextRecords.reserve(qMin(aircraftArray.size(), kMaxPublishedAircraft));

	for (const QJsonValue& value : aircraftArray)
	{
		const QJsonObject object = value.toObject();
		if (shouldSkipAircraft(object))
			continue;

		const AircraftRecord record = parseAircraftRecord(object, snapshotJd);
		if (record.icao24.isEmpty())
			continue;

		nextRecords.append(record);
		if (nextRecords.size() >= kMaxPublishedAircraft)
			break;
	}

	return nextRecords;
}

QJsonArray aircraftArrayFromResponse(const QJsonObject& root)
{
	const QJsonArray aircraftArray = root.value("aircraft").toArray();
	if (!aircraftArray.isEmpty())
		return aircraftArray;
	return root.value("ac").toArray();
}

bool isRealtimeMode(const StelCore* core)
{
	return core && core->getIsTimeNow();
}
}

StelModule* PlanesStelPluginInterface::getStelModule() const
{
	return new Planes();
}

StelPluginInfo PlanesStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = QStringLiteral("Planes");
	info.displayedName = N_("Planes");
	info.authors = QStringLiteral("Felix Zeltner, Georg Zotti, Kamil Zaraś (astronow.pl)");
	info.contact = STELLARIUM_DEV_URL;
	info.description = N_("Display live ADS-B aircraft in the sky.");
	info.version = kPluginVersion;
	info.license = QStringLiteral("GPL v2 or later");
	return info;
}

Planes::Planes()
	: networkMgr(nullptr)
	, fetchTimer(nullptr)
	, refreshDebounceTimer(nullptr)
	, inFlightReply(nullptr)
#ifndef NO_GUI
	, configDialog(nullptr)
	, toolbarButton(nullptr)
#endif
	, sourceUrlTemplate(kAdsbFiTemplate)
	, fetchIntervalSec(kDefaultFetchIntervalSec)
	, radiusNm(kDefaultRadiusNm)
	, pendingRefresh(false)
	, enabled(false)
	, showLabels(false)
	, showButton(true)
	, lastRealtimeState(true)
	, labelMode(kLabelModeFlightNumber)
	, lastStatus(QStringLiteral("idle"))
	, lastSuccessfulUpdate(QStringLiteral("Never"))
{
	setObjectName(QStringLiteral("Planes"));
#ifndef NO_GUI
	configDialog = new PlanesDialog();
#endif
}

Planes::~Planes()
{
#ifndef NO_GUI
	delete configDialog;
#endif
}

void Planes::loadSettings()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Planes");
	sourceUrlTemplate = conf->value("source_url_template", kAdsbFiTemplate).toString().trimmed();
	if (sourceUrlTemplate.isEmpty())
		sourceUrlTemplate = kAdsbFiTemplate;
	fetchIntervalSec = sanitizeInterval(conf->value("fetch_interval_sec", kDefaultFetchIntervalSec).toInt());
	radiusNm = sanitizeRadius(conf->value("radius_nm", kDefaultRadiusNm).toInt());
	enabled = conf->value("enabled", false).toBool();
	showLabels = conf->value("show_labels", true).toBool();
	showButton = conf->value("show_button", true).toBool();
	labelMode = sanitizeLabelMode(conf->value("label_mode", kLabelModeFlightNumber).toInt());
	conf->endGroup();
	applyProviderDefaults();
}

void Planes::saveSettings() const
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("Planes");
	conf->setValue("source_url_template", sourceUrlTemplate);
	conf->setValue("fetch_interval_sec", fetchIntervalSec);
	conf->setValue("radius_nm", radiusNm);
	conf->setValue("enabled", enabled);
	conf->setValue("show_labels", showLabels);
	conf->setValue("show_button", showButton);
	conf->setValue("label_mode", labelMode);
	conf->endGroup();
}

void Planes::init()
{
	Q_INIT_RESOURCE(Planes);
	loadSettings();
	planeTexture = StelApp::getInstance().getTextureManager().createTexture(":/planes/plane.png");
	pointerTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");

	networkMgr = new QNetworkAccessManager(this);
	connect(networkMgr, &QNetworkAccessManager::finished, this, &Planes::onReply);

	fetchTimer = new QTimer(this);
	fetchTimer->setInterval(fetchIntervalSec * 1000);
	connect(fetchTimer, &QTimer::timeout, this, &Planes::fetchAircraft);

	refreshDebounceTimer = new QTimer(this);
	refreshDebounceTimer->setSingleShot(true);
	refreshDebounceTimer->setInterval(1200);
	connect(refreshDebounceTimer, &QTimer::timeout, this, &Planes::fetchAircraft);

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
	connect(StelApp::getInstance().getCore(), &StelCore::locationChanged, this, &Planes::onLocationChanged);
	lastRealtimeState = isRealtimeMode(StelApp::getInstance().getCore());

	addAction("actionShow_Planes", N_("Planes"), N_("Show Planes"), "enabled", "Shift+P");
#ifndef NO_GUI
	addAction("actionShow_Planes_dialog", N_("Planes"), N_("Show settings dialog"), configDialog, "visible", "Ctrl+P");
	applyButtonVisibility();
#endif
	if (enabled)
	{
		fetchTimer->start();
		if (lastRealtimeState)
		{
			updateStatus(QStringLiteral("Waiting for first update..."));
			QTimer::singleShot(1500, this, &Planes::fetchAircraft);
		}
		else
		{
			updateStatus(kRealtimeOnlyStatus);
		}
	}
	else
	{
		updateStatus(QStringLiteral("disabled; live updates are off"));
	}
}

void Planes::deinit()
{
	saveSettings();
	if (inFlightReply)
		inFlightReply->abort();
	aircraft.clear();
	planeTexture.clear();
	pointerTexture.clear();
}

void Planes::update(double deltaTime)
{
	Q_UNUSED(deltaTime)

	StelCore* core = StelApp::getInstance().getCore();
	const bool realtimeNow = isRealtimeMode(core);
	if (realtimeNow == lastRealtimeState)
		return;

	lastRealtimeState = realtimeNow;
	if (!enabled)
		return;

	if (realtimeNow)
	{
		updateStatus(QStringLiteral("Returned to real-time mode."));
		scheduleRefresh(0);
	}
	else
	{
		updateStatus(kRealtimeOnlyStatus);
	}
}

QString Planes::buildRequestUrl(const StelCore* core) const
{
	if (!core)
		return QString();

	const StelLocation& loc = core->getCurrentLocation();
	return sourceUrlTemplate
		.arg(loc.getLatitude(), 0, 'f', 4)
		.arg(loc.getLongitude(), 0, 'f', 4)
		.arg(radiusNm);
}

QString Planes::getProviderId() const
{
	return providerIdFromTemplate(sourceUrlTemplate);
}

QString Planes::getProviderDisplayName() const
{
	return providerDefinition(getProviderId()).displayName;
}

QString Planes::getProviderWebsiteUrl() const
{
	return providerDefinition(getProviderId()).websiteUrl;
}

int Planes::getProviderMaxRadiusNm() const
{
	return providerDefinition(getProviderId()).maxRadiusNm;
}

void Planes::applyProviderDefaults(bool clampRadius)
{
	const ProviderDefinition provider = providerDefinition(providerIdFromTemplate(sourceUrlTemplate));
	sourceUrlTemplate = provider.urlTemplate;
	if (clampRadius)
		radiusNm = sanitizeRadius(qMin(radiusNm, provider.maxRadiusNm));
}

void Planes::fetchAircraft()
{
	if (!networkMgr || !enabled)
		return;

	StelCore* core = StelApp::getInstance().getCore();
	if (!isRealtimeMode(core))
		return;

	const QString url = buildRequestUrl(core);
	if (url.isEmpty())
	{
		updateStatus(QStringLiteral("missing core or URL"), false, true);
		return;
	}

	if (inFlightReply)
	{
		pendingRefresh = true;
		return;
	}

	QNetworkRequest request{QUrl(url)};
	request.setRawHeader("User-Agent", QString("Stellarium-Planes/%1").arg(kPluginVersion).toUtf8());
	request.setRawHeader("Accept", "application/json");
	inFlightReply = networkMgr->get(request);
	pendingRefresh = false;
	updateStatus(QStringLiteral("Updating live aircraft feed..."));
}

void Planes::onReply(QNetworkReply* reply)
{
	if (reply == inFlightReply)
		inFlightReply = nullptr;

	const bool shouldRefreshAgain = pendingRefresh && enabled;
	pendingRefresh = false;
	reply->deleteLater();

	if (!isRealtimeMode(StelApp::getInstance().getCore()))
	{
		updateStatus(kRealtimeOnlyStatus);
		return;
	}

	if (reply->error() != QNetworkReply::NoError)
	{
		if (reply->error() != QNetworkReply::OperationCanceledError)
			updateStatus(QString("Update failed: %1").arg(reply->errorString()), false, true);
		if (shouldRefreshAgain)
			scheduleRefresh(250);
		return;
	}

	const QByteArray payload = reply->readAll();

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
	if (parseError.error != QJsonParseError::NoError || !doc.isObject())
	{
		updateStatus(QString("Update failed: %1").arg(parseError.errorString()), false, true);
		if (shouldRefreshAgain)
			scheduleRefresh(250);
		return;
	}

	const QVector<AircraftRecord> records = parseAircraftRecords(aircraftArrayFromResponse(doc.object()), StelUtils::getJDFromSystem());
	QHash<QString, AircraftObjectP> existingById;
	existingById.reserve(aircraft.size());
	for (const AircraftObjectP& object : std::as_const(aircraft))
	{
		if (object)
			existingById.insert(object->getID(), object);
	}

	QVector<AircraftObjectP> nextAircraft;
	nextAircraft.reserve(records.size());
	for (const AircraftRecord& record : records)
	{
		const auto it = existingById.constFind(record.icao24);
		if (it != existingById.constEnd())
		{
			it.value()->updateRecord(record);
			nextAircraft.append(it.value());
		}
		else
		{
			nextAircraft.append(AircraftObjectP::create(record));
		}
	}
	aircraft = nextAircraft;
	lastSuccessfulUpdate = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
	updateStatus(QString("Showing %1 aircraft from %2").arg(aircraft.size()).arg(getProviderDisplayName()));
	if (shouldRefreshAgain)
		scheduleRefresh(250);
}

void Planes::onLocationChanged(const StelLocation& loc)
{
	Q_UNUSED(loc)
	scheduleRefresh();
}

void Planes::draw(StelCore* core)
{
	if (!core || !enabled || !isRealtimeMode(core))
		return;

	StelProjectorP projection = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	StelPainter painter(projection);
	painter.setColor(1.0f, 1.0f, 1.0f, 1.0f);
	painter.setBlending(true);
	if (planeTexture)
		planeTexture->bind();
	for (const AircraftObjectP& object : std::as_const(aircraft))
		object->draw(core, &painter, showLabels, labelMode);

	StelObjectMgr* objectMgr = GETSTELMODULE(StelObjectMgr);
	if (!objectMgr->getFlagSelectedObjectPointer())
		return;

	const QList<StelObjectP> selectedAircraft = objectMgr->getSelectedObject(AircraftObject::STEL_TYPE);
	if (selectedAircraft.empty() || !pointerTexture)
		return;

	StelPainter pointerPainter(core->getProjection(StelCore::FrameJ2000, StelCore::RefractionOff));
	const StelObjectP object = selectedAircraft.constFirst();
	Vec3f screenPos;
	if (!pointerPainter.getProjector()->project(object->getJ2000EquatorialPos(core).toVec3f(), screenPos))
		return;

	pointerPainter.setColor(0.4f, 0.5f, 0.8f);
	pointerTexture->bind();
	pointerPainter.setBlending(true);
	const float scale = StelApp::getInstance().getScreenScale();
	float size = static_cast<float>(object->getAngularRadius(core) * (2. * M_PI_180) *
		static_cast<double>(pointerPainter.getProjector()->getPixelPerRadAtCenter()));
	size += (12.f + 3.f * std::sin(2. * StelApp::getInstance().getTotalRunTime())) * scale;
	const float radius = 20.f * scale;
	const float x = screenPos[0];
	const float y = screenPos[1];
	pointerPainter.drawSprite2dModeNoDeviceScale(x - size / 2.f, y - size / 2.f, radius, 90.f);
	pointerPainter.drawSprite2dModeNoDeviceScale(x - size / 2.f, y + size / 2.f, radius, 0.f);
	pointerPainter.drawSprite2dModeNoDeviceScale(x + size / 2.f, y + size / 2.f, radius, -90.f);
	pointerPainter.drawSprite2dModeNoDeviceScale(x + size / 2.f, y - size / 2.f, radius, -180.f);
}

double Planes::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule(QStringLiteral("SolarSystem"))->getCallOrder(actionName) + 1.0;
	return 0.0;
}

bool Planes::configureGui(bool show)
{
#ifndef NO_GUI
	if (configDialog)
	{
		if (show)
			configDialog->setVisible(true);
		return true;
	}
#else
	Q_UNUSED(show)
#endif
	return false;
}

void Planes::setEnabled(bool value)
{
	if (enabled == value)
		return;

	enabled = value;
	if (!enabled)
	{
		if (fetchTimer)
			fetchTimer->stop();
		if (refreshDebounceTimer)
			refreshDebounceTimer->stop();
		pendingRefresh = false;
		if (inFlightReply)
			inFlightReply->abort();
		aircraft.clear();
		updateStatus(QStringLiteral("Live updates are off"));
	}
	else
	{
		if (fetchTimer)
			fetchTimer->start();
		if (isRealtimeMode(StelApp::getInstance().getCore()))
		{
			updateStatus(QStringLiteral("Waiting for first update..."));
			scheduleRefresh(0);
		}
		else
		{
			updateStatus(kRealtimeOnlyStatus);
		}
	}
	emit enabledChanged(enabled);
}

void Planes::setFlagShowLabels(bool value)
{
	if (showLabels == value)
		return;
	showLabels = value;
	emit showLabelsChanged(showLabels);
}

void Planes::setFlagShowButton(bool value)
{
	if (showButton == value)
		return;
	showButton = value;
#ifndef NO_GUI
	applyButtonVisibility();
#endif
	emit showButtonChanged(showButton);
}

void Planes::setLabelMode(int mode)
{
	const int sanitized = sanitizeLabelMode(mode);
	if (labelMode == sanitized)
		return;
	labelMode = sanitized;
	emit labelModeChanged(labelMode);
}

void Planes::setProviderId(const QString& providerId)
{
	const QString currentProviderId = getProviderId();
	const ProviderDefinition nextProvider = providerDefinition(providerId);
	if (currentProviderId == nextProvider.id)
		return;

	sourceUrlTemplate = nextProvider.urlTemplate;
	const int clampedRadius = sanitizeRadius(qMin(radiusNm, nextProvider.maxRadiusNm));
	const bool radiusAdjusted = clampedRadius != radiusNm;
	radiusNm = clampedRadius;

	emit providerChanged(nextProvider.id);
	if (radiusAdjusted)
		emit radiusChanged(radiusNm);

	if (enabled)
	{
		updateStatus(QString("Switched to %1").arg(nextProvider.displayName));
		scheduleRefresh(0);
	}
}

void Planes::setFetchIntervalSec(int seconds)
{
	const int sanitized = sanitizeInterval(seconds);
	if (fetchIntervalSec == sanitized)
		return;
	fetchIntervalSec = sanitized;
	if (fetchTimer)
		fetchTimer->setInterval(fetchIntervalSec * 1000);
	emit fetchIntervalChanged(fetchIntervalSec);
	scheduleRefresh();
}

void Planes::setRadiusNm(int nm)
{
	const int sanitized = sanitizeRadius(nm);
	if (radiusNm == sanitized)
		return;
	radiusNm = sanitized;
	emit radiusChanged(radiusNm);
	scheduleRefresh();
}

void Planes::refreshNow()
{
	if (enabled)
	{
		if (!isRealtimeMode(StelApp::getInstance().getCore()))
		{
			updateStatus(kRealtimeOnlyStatus);
			return;
		}
		fetchAircraft();
	}
}

void Planes::scheduleRefresh(int delayMs)
{
	if (!enabled || !refreshDebounceTimer || !isRealtimeMode(StelApp::getInstance().getCore()))
		return;

	refreshDebounceTimer->start(delayMs);
}

void Planes::updateStatus(const QString& status, bool logInfo, bool logWarning)
{
	lastStatus = status;
	if (logInfo)
		qInfo().noquote() << "[Planes]" << lastStatus;
	if (logWarning)
		qWarning().noquote() << "[Planes]" << lastStatus;
	emit statusChanged(lastStatus);
}

void Planes::applyButtonVisibility()
{
#ifndef NO_GUI
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (!gui)
		return;

	if (showButton)
	{
		if (!toolbarButton)
		{
			toolbarButton = new StelButton(nullptr,
				QPixmap(":/planes/planes_on_160.png"),
				QPixmap(":/planes/planes_off_160.png"),
				QPixmap(":/graphicGui/miscGlow32x32.png"),
				"actionShow_Planes",
				false,
				"actionShow_Planes_dialog");
		}
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
	else
	{
		gui->getButtonBar()->hideButton("actionShow_Planes");
	}
#endif
}

QList<StelObjectP> Planes::searchAround(const Vec3d& v, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!core)
		return result;

	const double cosLimitFov = std::cos(limitFov * M_PI / 180.0);
	Vec3d normalized = v;
	normalized.normalize();

	for (const AircraftObjectP& object : aircraft)
	{
		if (!object->isAboveHorizon(core))
			continue;
		Vec3d objectPos = object->getJ2000EquatorialPos(core);
		objectPos.normalize();
		if (normalized.dot(objectPos) >= cosLimitFov)
			result.append(qSharedPointerCast<StelObject>(object));
	}

	return result;
}

StelObjectP Planes::searchByNameI18n(const QString& nameI18n) const
{
	return searchByName(nameI18n);
}

StelObjectP Planes::searchByName(const QString& name) const
{
	const QString needle = name.trimmed().toUpper();
	for (const AircraftObjectP& object : aircraft)
	{
		if (object->getEnglishName().toUpper() == needle || object->getID().toUpper() == needle)
			return qSharedPointerCast<StelObject>(object);
	}
	return StelObjectP();
}

StelObjectP Planes::searchByID(const QString& id) const
{
	return searchByName(id);
}

QVector<QPair<QString, StelObjectP>> Planes::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	Q_UNUSED(useStartOfWords)
	QVector<QPair<QString, StelObjectP>> result;
	const QString needle = objPrefix.trimmed().toUpper();
	for (const AircraftObjectP& object : aircraft)
	{
		if (!object->getEnglishName().toUpper().startsWith(needle) && !object->getID().toUpper().startsWith(needle))
			continue;
		result.append(qMakePair(object->getEnglishName(), qSharedPointerCast<StelObject>(object)));
		if (result.size() >= maxNbItem)
			break;
	}
	return result;
}

QVector<QPair<QString, StelObjectP>> Planes::listAllObjects(bool inEnglish) const
{
	Q_UNUSED(inEnglish)
	QVector<QPair<QString, StelObjectP>> result;
	result.reserve(aircraft.size());
	for (const AircraftObjectP& object : aircraft)
		result.append(qMakePair(object->getEnglishName(), qSharedPointerCast<StelObject>(object)));
	return result;
}
