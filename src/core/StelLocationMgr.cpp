/*
 * Copyright (C) 2008 Fabien Chereau
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

#include "StelLocationMgr.hpp"
#include "SolarSystem.hpp"
#include "StelLocationMgr_p.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "StelJsonParser.hpp"

#include <QStringListModel>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QTimeZone>
#include <QTimer>
#include <QApplication>
#include <QRegularExpression>

TimezoneNameMap StelLocationMgr::locationDBToIANAtranslations;

QList<GeoRegion> StelLocationMgr::regions;
QMap<QString, QString> StelLocationMgr::countryCodeToRegionMap;
QMap<QString, QString> StelLocationMgr::countryNameToCodeMap;

#ifdef ENABLE_GPS
#ifdef ENABLE_LIBGPS
LibGPSLookupHelper::LibGPSLookupHelper(QObject *parent)
	: GPSLookupHelper(parent), ready(false)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	QString gpsdHostname=conf->value("gui/gpsd_hostname", "localhost").toString();
	QString gpsdPort=conf->value("gui/gpsd_port", DEFAULT_GPSD_PORT).toString();

	timer.setSingleShot(false);
	if (qApp->property("verbose").toBool())
		qDebug() << "Opening GPSD connection to" << gpsdHostname << ":" << gpsdPort;
	// Example almost straight from http://www.catb.org/gpsd/client-howto.html
	gps_rec = new gpsmm(gpsdHostname.toUtf8(), gpsdPort.toUtf8());
	if(gps_rec->is_open())
	{
		ready = gps_rec->stream(WATCH_ENABLE|WATCH_JSON);
	}
	if(ready)
	{
		connect(&timer, SIGNAL(timeout()), this, SLOT(query()));
	}
	else
		qWarning()<<"libGPS lookup not ready, GPSD probably not running.";
}

LibGPSLookupHelper::~LibGPSLookupHelper()
{
	delete gps_rec;
}

bool LibGPSLookupHelper::isReady()
{
	return ready;
}

void LibGPSLookupHelper::setPeriodicQuery(int interval)
{
	if (interval==0)
		timer.stop();
	else
		{
			timer.start(interval);
		}
}
void LibGPSLookupHelper::query()
{
	bool verbose=qApp->property("verbose").toBool();

	if(!ready)
	{
		emit queryError("GPSD helper not ready");
		return;
	}

	StelLocation loc;

	int tries=0;
	int fixmode=0;
	while (tries<10)
	{
		tries++;
		if (verbose)
			qDebug() << "query(): tries=" << tries;

		if (!gps_rec->waiting(750000)) // argument usec. wait 0.75 sec. (example had 50s)
		{
			qDebug() << " - waiting timed out after 0.75sec.";
			continue;
		}

		struct gps_data_t* newdata;
		if ((newdata = gps_rec->read()) == Q_NULLPTR)
		{
			emit queryError("GPSD query: Read error.");
			return;
		}
		else
		{
// It is unclear why some data elements seem to be not filled by gps_rec.read().
//			if (newdata->status==0) // no fix?
//			{
//				// This can happen indoors.
//				qDebug() << "GPS has no fix.";
//				emit queryError("GPSD query: No Fix.");
//				return;
//			}
#if GPSD_API_MAJOR_VERSION < 9
			if (newdata->online==0.0) // no device?
#else
			if (newdata->online.tv_sec == 0 && newdata->online.tv_nsec == 0) // no device?
#endif
			{
				// This can happen when unplugging the GPS while running Stellarium,
				// or running gpsd with no GPS receiver.
				emit queryError("GPS seems offline. No fix.");
				return;
			}


			fixmode=newdata->fix.mode; // 0:not_seen, 1:no_fix, 2:2Dfix(no alt), 3:3Dfix(perfect)
			if (verbose)
				qDebug() << "GPSD newdata->fix.mode=" << fixmode;

			if (fixmode==0)
			{
				// This may come just after creation of the GPSDhelper.
				// It seems to take some time to fill the data.
				if (verbose)
					qDebug() << "GPSD seems not ready yet. Retry.";
				continue;
			}

			if (verbose)
			{
				//qDebug() << "newdata->online=" << newdata->online;
				qDebug() << "Solution from " << newdata->satellites_used << "out of " << newdata->satellites_visible << " visible Satellites.";
				dop_t dop=newdata->dop;
#if GPSD_API_MAJOR_VERSION < 9
				qDebug() << "GPSD data: Long" << newdata->fix.longitude << "Lat" << newdata->fix.latitude << "Alt" << newdata->fix.altitude;
#else
				qDebug() << "GPSD data: Long" << newdata->fix.longitude << "Lat" << newdata->fix.latitude << "Alt" << newdata->fix.altHAE;
#endif
				qDebug() << "Dilution of Precision:";
				qDebug() << " - xdop:" << dop.xdop << "ydop:" << dop.ydop;
				qDebug() << " - pdop:" << dop.pdop << "hdop:" << dop.hdop;
				qDebug() << " - vdop:" << dop.vdop << "tdop:" << dop.tdop << "gdop:" << dop.gdop;
				// GPSD API 8.0:
				// * Remove epe from gps_data_t, it duplicates gps_fix_t eph
				// * Added sep (estimated spherical error, 3D)
				// Details: https://github.com/Stellarium/stellarium/issues/733
				// #if GPSD_API_MAJOR_VERSION >= 8
				// qDebug() << "Spherical Position Error (sep):" << newdata->fix.sep;
				// #else
				// qDebug() << "Spherical Position Error (epe):" << newdata->epe;
				// #endif
			}
			loc.setLongitude(static_cast<float> (newdata->fix.longitude));
			loc.setLatitude (static_cast<float> (newdata->fix.latitude));
			// Frequently hdop, vdop and satellite counts are NaN. Sometimes they show OK. This is minor issue.
			if ((verbose) && (fixmode<3))
			{
				qDebug() << "GPSDfix " << fixmode << ": Location" << QString("lat %1, long %2, alt %3").arg(loc.getLatitude()).arg(loc.getLongitude()).arg(loc.altitude);
				qDebug() << "    Estimated HDOP " << newdata->dop.hdop << "m from " << newdata->satellites_used << "(of" << newdata->satellites_visible  << "visible) satellites";
			}
			else
			{
#if GPSD_API_MAJOR_VERSION < 9
				loc.altitude=static_cast<int>(newdata->fix.altitude);
#else
				loc.altitude=static_cast<int>(newdata->fix.altHAE);
#endif
				if (verbose)
				{
					qDebug() << "GPSDfix " << fixmode << ": Location" << QString("lat %1, long %2, alt %3").arg(loc.getLatitude()).arg(loc.getLongitude()).arg(loc.altitude);
					qDebug() << "    Estimated HDOP " << newdata->dop.hdop << "m, VDOP " << newdata->dop.vdop <<  "m from " << newdata->satellites_used << "(of" << newdata->satellites_visible  << "visible) satellites";
				}
				break; // escape from the tries loop
			}
		}
	}

	if (fixmode <2)
	{
		emit queryError("GPSD: Could not get valid position.");
		return;
	}
	if ((verbose) && (fixmode<3))
	{
		qDebug() << "Fix only quality " << fixmode << " after " << tries << " tries";
	}
	if (verbose)
		qDebug() << "GPSD location" << QString("lat %1, long %2, alt %3").arg(loc.getLatitude()).arg(loc.getLongitude()).arg(loc.altitude);

	emit queryFinished(loc);
}

#endif

NMEALookupHelper::NMEALookupHelper(QObject *parent)
	: GPSLookupHelper(parent), serial(Q_NULLPTR), nmea(Q_NULLPTR)
{
	//use RAII
	// Getting a list of ports may enable auto-detection!
	QList<QSerialPortInfo> portInfoList=QSerialPortInfo::availablePorts();

	if (portInfoList.size()==0)
	{
		qDebug() << "No connected devices found. NMEA GPS lookup failed.";
		return;
	}

	QSettings* conf = StelApp::getInstance().getSettings();

	// As long as we only have one, this is OK. Else we must do something about COM3, COM4 etc.
	QSerialPortInfo portInfo;
	if (portInfoList.size()==1)
	{
		portInfo=portInfoList.at(0);
		qDebug() << "Only one port found at " << portInfo.portName();
	}
	else
	{
		#ifdef Q_OS_WIN
		QString portName=conf->value("gui/gps_interface", "COM3").toString();
		#else
		QString portName=conf->value("gui/gps_interface", "ttyUSB0").toString();
		#endif
		bool portFound=false;
		for (int i=0; i<portInfoList.size(); ++i)
		{
			QSerialPortInfo pi=portInfoList.at(i);
			qDebug() << "Serial port list. Make sure you are using the right configuration.";
			qDebug() << "Port: " << pi.portName();
			qDebug() << "  SystemLocation:" << pi.systemLocation();
			qDebug() << "  Description:"    << pi.description();
			qDebug() << "  Manufacturer:"   << pi.manufacturer();
			qDebug() << "  VendorID:"       << pi.vendorIdentifier();
			qDebug() << "  ProductID:"      << pi.productIdentifier();
			qDebug() << "  SerialNumber:"   << pi.serialNumber();
#if (QT_VERSION<QT_VERSION_CHECK(5,14,0))
			qDebug() << "  Busy:"           << pi.isBusy();
#endif
			qDebug() << "  Null:"           << pi.isNull();
			if (pi.portName()==portName)
			{
				portInfo=pi;
				portFound=true;
			}
		}
		if (!portFound)
		{
			qDebug() << "Configured port" << portName << "not found. No GPS query.";
			return;
		}
	}

	// NMEA-0183 specifies device sends at 4800bps, 8N1. Some devices however send at 9600, allow this.
	// baudrate is configurable via config
	qint32 baudrate=conf->value("gui/gps_baudrate", 4800).toInt();

	nmea=new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode,this);
	//serial = new QSerialPort(portInfo, nmea);
	serial = new QSerialPort(portInfo, this);
	serial->setBaudRate(baudrate);
	serial->setDataBits(QSerialPort::Data8);
	serial->setParity(QSerialPort::NoParity);
	serial->setStopBits(QSerialPort::OneStop);
	serial->setFlowControl(QSerialPort::NoFlowControl);
	if (serial->open(QIODevice::ReadOnly)) // may fail when line used by other program!
	{
		nmea->setDevice(serial);
		qDebug() << "Query GPS NMEA device at port " << serial->portName();
		connect(nmea, SIGNAL(positionUpdated(const QGeoPositionInfo)),this,SLOT(nmeaUpdated(const QGeoPositionInfo)));
		#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		connect(nmea, SIGNAL(errorOccurred(QGeoPositionInfoSource::Error)), this, SLOT(nmeaError(QGeoPositionInfoSource::Error)));
		#else
		connect(nmea, SIGNAL(error(QGeoPositionInfoSource::Error)), this, SLOT(nmeaError(QGeoPositionInfoSource::Error)));
		connect(nmea, SIGNAL(updateTimeout()),this,SLOT(nmeaTimeout()));
		#endif
	}
	else qWarning() << "Cannot open serial port to NMEA device at port " << serial->portName();
	// This may leave an un-ready object. Must be cleaned-up later.
}
NMEALookupHelper::~NMEALookupHelper()
{
	if(nmea)
	{
		delete nmea;
		nmea=Q_NULLPTR;
	}
	if (serial)
	{
		if (serial->isOpen())
		{
			//qDebug() << "NMEALookupHelper destructor: Close serial first";
			serial->clear();
			serial->close();
		}
		delete serial;
		serial=Q_NULLPTR;
	}
}

void NMEALookupHelper::query()
{
	if(isReady())
	{
		//kick off a single update request
		nmea->requestUpdate(3000);
	}
	else
		emit queryError("NMEA helper not ready");
}
void NMEALookupHelper::setPeriodicQuery(int interval)
{
	if(isReady())
	{
		if (interval==0)
			nmea->stopUpdates();
		else
		{
			nmea->setUpdateInterval(interval);
			nmea->startUpdates();
		}
	}
	else
		emit queryError("NMEA helper not ready");
}

void NMEALookupHelper::nmeaUpdated(const QGeoPositionInfo &update)
{
	bool verbose=qApp->property("verbose").toBool();
	if (verbose)
		qDebug() << "NMEA updated";

	QGeoCoordinate coord=update.coordinate();
	QDateTime timestamp=update.timestamp();

	if (verbose)
	{
		qDebug() << " - time: " << timestamp.toString();
		qDebug() << " - location: Long=" << coord.longitude() << " Lat=" << coord.latitude() << " Alt=" << coord.altitude();
	}
	if (update.isValid()) // emit queryFinished(loc) with new location
	{
		StelLocation loc;
		loc.setLongitude(static_cast<float>(coord.longitude()));
		loc.setLatitude(static_cast<float>(coord.latitude()));
		// 2D fix may have only long/lat, invalid altitude.
		loc.altitude=( qIsNaN(coord.altitude()) ? 0 : static_cast<int>(floor(coord.altitude())));
		emit queryFinished(loc);
	}
	else
	{
		if (verbose)
			qDebug() << "(This position update was an invalid package)";
		emit queryError("NMEA update: invalid package");
	}
}

void NMEALookupHelper::nmeaError(QGeoPositionInfoSource::Error error)
{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	emit queryError(QString("NMEA general error: %1").arg(QVariant::fromValue(error).toString()));
#else
	emit queryError(QString("NMEA general error: %1").arg(error));
#endif
}
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
void NMEALookupHelper::nmeaTimeout()
{
	emit queryError("NMEA timeout");
}
#endif
#endif

StelLocationMgr::StelLocationMgr()
	: nmeaHelper(Q_NULLPTR), libGpsHelper(Q_NULLPTR)
{
	// initialize the static QMap first if necessary.
	// The first entry is the DB name, the second is as we display it in the program.
	if (locationDBToIANAtranslations.isEmpty())
	{
		// Reported July 7, 2023 (#3316)
		locationDBToIANAtranslations.insert("Asia/Calcutta", "Asia/Kolkata");
		// Reported April 25, 2023. (#3200)
		locationDBToIANAtranslations.insert("America/Yellowknife","America/Edmonton");
		// Seen February 06, 2023.
		locationDBToIANAtranslations.insert("America/Ciudad_Juarez","America/Ojinaga");
		// Seen November 11, 2022.
		locationDBToIANAtranslations.insert("America/Thunder_Bay","America/Toronto");
		locationDBToIANAtranslations.insert("Europe/Uzhgorod",  "Europe/Kyiv");
		// Seen on October 27, 2022. TZ Removed?
		locationDBToIANAtranslations.insert("Europe/Zaporozhye","Europe/Kyiv");
		// Seen on September 26, 2022. Officially renamed.
		locationDBToIANAtranslations.insert("Europe/Kyiv",      "Europe/Kiev");
		locationDBToIANAtranslations.insert("Europe/Kiev",      "Europe/Kyiv");
		// Seen in an unrelated bug report, 2022-08-30
		locationDBToIANAtranslations.insert("Asia/Pyongyang",   "UTC+09:00");
		// reported in SF forum on 2017-03-27
		locationDBToIANAtranslations.insert("Europe/Minsk",     "UTC+03:00");
		locationDBToIANAtranslations.insert("Europe/Samara",    "UTC+04:00");
		locationDBToIANAtranslations.insert("America/Cancun",   "UTC-05:00");
		locationDBToIANAtranslations.insert("Asia/Kamchatka",   "UTC+12:00");
		// Missing on Qt5.7/Win10 as of 2017-03-18.
		locationDBToIANAtranslations.insert("Europe/Astrakhan", "UTC+04:00");
		locationDBToIANAtranslations.insert("Europe/Ulyanovsk", "UTC+04:00");
		locationDBToIANAtranslations.insert("Europe/Kirov",     "UTC+03:00");
		locationDBToIANAtranslations.insert("Asia/Hebron",      "Asia/Jerusalem");
		locationDBToIANAtranslations.insert("Asia/Gaza",        "Asia/Jerusalem"); // or use UTC+2:00? (political issue...)
		locationDBToIANAtranslations.insert("Asia/Kolkata",     "Asia/Calcutta");
		locationDBToIANAtranslations.insert("Asia/Kathmandu",   "Asia/Katmandu");
		locationDBToIANAtranslations.insert("Asia/Tomsk",       "Asia/Novosibirsk");
		locationDBToIANAtranslations.insert("Asia/Barnaul",     "UTC+07:00");
		locationDBToIANAtranslations.insert("Asia/Ho_Chi_Minh", "Asia/Saigon");
		locationDBToIANAtranslations.insert("Asia/Hovd",        "UTC+07:00");
		locationDBToIANAtranslations.insert("America/Argentina/Buenos_Aires", "America/Buenos_Aires");
		locationDBToIANAtranslations.insert("America/Argentina/Jujuy",        "America/Jujuy");
		locationDBToIANAtranslations.insert("America/Argentina/Mendoza",      "America/Mendoza");
		locationDBToIANAtranslations.insert("America/Argentina/Catamarca",    "America/Catamarca");
		locationDBToIANAtranslations.insert("America/Argentina/Cordoba",      "America/Cordoba");
		locationDBToIANAtranslations.insert("America/Indiana/Indianapolis",   "America/Indianapolis");
		locationDBToIANAtranslations.insert("America/Kentucky/Louisville",    "America/Louisville");
		locationDBToIANAtranslations.insert("America/Miquelon",               "UTC-03:00");  // Small Canadian island.
		locationDBToIANAtranslations.insert("Africa/Asmara",     "Africa/Asmera");
		locationDBToIANAtranslations.insert("Atlantic/Faroe",    "Atlantic/Faeroe");
		locationDBToIANAtranslations.insert("Pacific/Pohnpei",   "Pacific/Ponape");
		locationDBToIANAtranslations.insert("Pacific/Norfolk",   "UTC+11:00");
		locationDBToIANAtranslations.insert("Pacific/Pitcairn",  "UTC-08:00");
		// Missing on Qt5.5.1/Ubuntu 16.04.1 LTE as of 2017-03-18:
		// NOTE: We must add these following zones for lookup in both ways: When the binary file is being created for publication on Linux, Rangoon/Yangon is being translated.
		locationDBToIANAtranslations.insert("Asia/Rangoon",      "Asia/Yangon");  // UTC+6:30 Yangon missing on Ubuntu/Qt5.5.1.
		locationDBToIANAtranslations.insert("Asia/Yangon",       "Asia/Rangoon"); // This can translate from the binary location file back to the zone name as known on Windows.
		locationDBToIANAtranslations.insert( "", "UTC");
		// Missing on Qt5.9.5/Ubuntu 18.04.4
		locationDBToIANAtranslations.insert("America/Godthab",   "UTC-03:00");
		// Missing on Qt5.12.10/Win7
		locationDBToIANAtranslations.insert("Asia/Qostanay",   "UTC+06:00"); // no DST; https://www.zeitverschiebung.net/en/timezone/asia--qostanay
		locationDBToIANAtranslations.insert("Europe/Saratov",  "UTC+04:00"); // no DST; https://www.zeitverschiebung.net/en/timezone/europe--saratov
		locationDBToIANAtranslations.insert("Asia/Atyrau",     "UTC+05:00"); // no DST; https://www.zeitverschiebung.net/en/timezone/asia--atyrau
		locationDBToIANAtranslations.insert("Asia/Famagusta",  "Asia/Nicosia"); // Asia/Nicosia has no DST, but Asia/Famagusta has DST!
		locationDBToIANAtranslations.insert("America/Punta_Arenas",  "UTC-03:00"); // no DST; https://www.zeitverschiebung.net/en/timezone/america--punta_arenas
		// Missing on Qt5.15.2/Win10
		locationDBToIANAtranslations.insert("America/Nuuk",      "America/Godthab");
		// N.B. Further missing TZ names will be printed out in the log.txt. Resolve these by adding into this list.
		// TODO later: create a text file in user data directory, and auto-update it weekly.
	}

	QSettings* conf = StelApp::getInstance().getSettings();

	loadCountries();
	loadRegions();
	// The line below allows to re-generate the location file, you still need to gunzip it manually afterward.
	if (conf->value("devel/convert_locations_list", false).toBool())
		generateBinaryLocationFile("data/base_locations.txt", false, "data/base_locations.bin");

	locations = loadCitiesBin("data/base_locations.bin.gz");
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	locations.insert(loadCities("data/user_locations.txt", true));
#else
	locations.unite(loadCities("data/user_locations.txt", true));
#endif
	// Init to Paris France because it's the center of the world.
	lastResortLocation = locationForString(conf->value("init_location/last_location", "Paris, Western Europe").toString());

	planetName="Earth";
	planetSurfaceMap=QImage(":/graphicGui/miscWorldMap.jpg");
	connect(StelApp::getInstance().getCore(), SIGNAL(locationChanged(StelLocation)), this, SLOT(changePlanetMapForLocation(StelLocation)));
}

StelLocationMgr::~StelLocationMgr()
{
	if (nmeaHelper)
	{
		delete nmeaHelper;
		nmeaHelper=Q_NULLPTR;
	}
	if (libGpsHelper)
	{
		delete libGpsHelper;
		libGpsHelper=Q_NULLPTR;
	}
}

StelLocationMgr::StelLocationMgr(const LocationList &locations)
	: nmeaHelper(Q_NULLPTR), libGpsHelper(Q_NULLPTR)
{
	setLocations(locations);

	QSettings* conf = StelApp::getInstance().getSettings();
	// Init to Paris France because it's the center of the world.
	lastResortLocation = locationForString(conf->value("init_location/last_location", "Paris, Western Europe").toString());
}

void StelLocationMgr::setLocations(const LocationList &locations)
{
	this->locations.clear();
	for (const auto& loc : locations)
	{
		this->locations.insert(loc.getID(), loc);
	}

	emit locationListChanged();
}

void StelLocationMgr::generateBinaryLocationFile(const QString& fileName, bool isUserLocation, const QString& binFilePath) const
{
	qDebug() << "Generating a locations list...";
	const QMap<QString, StelLocation>& cities = loadCities(fileName, isUserLocation);
	QFile binfile(StelFileMgr::findFile(binFilePath, StelFileMgr::New));
	if(binfile.open(QIODevice::WriteOnly))
	{
		QDataStream out(&binfile);
		out.setVersion(QDataStream::Qt_5_2);
		out << cities;
		binfile.flush();
		binfile.close();
	}
	qDebug() << "[...] Please use 'gzip -nc base_locations.bin > base_locations.bin.gz' to pack a locations list.";
}

LocationMap StelLocationMgr::loadCitiesBin(const QString& fileName)
{
	QMap<QString, StelLocation> res;
	QString cityDataPath = StelFileMgr::findFile(fileName);
	if (cityDataPath.isEmpty())
		return res;

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return res;
	}

	if (fileName.endsWith(".gz"))
	{
		QDataStream in(StelUtils::uncompress(sourcefile.readAll()));
		in.setVersion(QDataStream::Qt_5_2);
		in >> res;
	}
	else
	{
		QDataStream in(&sourcefile);
		in.setVersion(QDataStream::Qt_5_2);
		in >> res;
	}
	// Now res has all location data. However, some timezone names are not available in various versions of Qt.
	// Sanity checks: It seems we must translate timezone names. Quite a number on Windows, but also still some on Linux.
	QList<QByteArray> availableTimeZoneList=QTimeZone::availableTimeZoneIds();
	QStringList unknownTZlist;
	for (auto& loc : res)
	{
		if ((loc.ianaTimeZone!="LMST") && (loc.ianaTimeZone!="LTST") && ( ! availableTimeZoneList.contains(loc.ianaTimeZone.toUtf8())) )
		{
			// TZ name which is currently unknown to Qt detected. See if we can translate it, if not: complain to qDebug().
			QString fixTZname=sanitizeTimezoneStringFromLocationDB(loc.ianaTimeZone);
			if (availableTimeZoneList.contains(fixTZname.toUtf8()))
			{
				loc.ianaTimeZone=fixTZname;
			}
			else
			{
				if (!unknownTZlist.contains(loc.ianaTimeZone))
				{
					qDebug() << "StelLocationMgr::loadCitiesBin(): TimeZone for " << loc.name <<  " not found: " << loc.ianaTimeZone;
					unknownTZlist.append(loc.ianaTimeZone);
				}
			}
		}
	}
	if (unknownTZlist.length()>0)
	{
		qInfo() << "StelLocationMgr::loadCitiesBin(): Summary of unknown TimeZones:";
		for (const auto& tz : unknownTZlist)
		{
			qInfo() << tz;
		}
		qInfo() << "Please report these timezone names (this logfile) to the Stellarium developers.";
		// Note to developers: Fill those names and replacements to the map above.
	}

	return res;
}

// Done in the following: TZ name sanitizing also for text file!
LocationMap StelLocationMgr::loadCities(const QString& fileName, bool isUserLocation)
{
	// Load the cities from data file
	QMap<QString, StelLocation> locations;
	QString cityDataPath = StelFileMgr::findFile(fileName);
	if (cityDataPath.isEmpty())
	{
		// Note it is quite normal not to have a user locations file (e.g. first run)
		if (!isUserLocation)
			qWarning() << "WARNING: Failed to locate location data file: " << QDir::toNativeSeparators(fileName);
		return locations;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return locations;
	}

	// Read the data serialized from the file.
	// Code below borrowed from Marble (http://edu.kde.org/marble/)
	QTextStream sourcestream(&sourcefile);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	sourcestream.setEncoding(QStringConverter::Utf8);
#else
	sourcestream.setCodec("UTF-8");
#endif
	StelLocation loc;
	while (!sourcestream.atEnd())
	{
		const QString& rawline=sourcestream.readLine();
		if (rawline.isEmpty() || rawline.startsWith('#') || (rawline.split("\t").count() < 8))
			continue;
		loc = StelLocation::createFromLine(rawline);
		loc.isUserLocation = isUserLocation;
		const QString& locId = loc.getID();

		if (locations.contains(locId))
		{
			// Add the state in the name of the existing one and the new one to differentiate
			StelLocation loc2 = locations[locId];
			if (!loc2.state.isEmpty())
				loc2.name += " ("+loc2.state+")";
			// remove and re-add the fixed version
			locations.remove(locId);
			locations.insert(loc2.getID(), loc2);

			if (!loc.state.isEmpty())
				loc.name += " ("+loc.state+")";
			locations.insert(loc.getID(), loc);
		}
		else
		{
			locations.insert(locId, loc);
		}
	}
	sourcefile.close();
	return locations;
}

static float parseAngle(const QString& s, bool* ok)
{
	float ret;
	// First try normal decimal value.
	ret = s.toFloat(ok);
	if (*ok) return ret;
	// Try GPS coordinate like +121°33'38.28"
	static const QRegularExpression reg("([+-]?[\\d.]+)°(?:([\\d.]+)')?(?:([\\d.]+)\")?");
	QRegularExpressionMatch match=reg.match(s);
	if (match.hasMatch())
	{
		float deg = match.captured(1).toFloat(ok);
		if (!*ok) return 0;
		float min = match.captured(2).isEmpty()? 0 : match.captured(2).toFloat(ok);
		if (!*ok) return 0;
		float sec = match.captured(3).isEmpty()? 0 : match.captured(3).toFloat(ok);
		if (!*ok) return 0;
		if (deg < 0)
			return deg - min / 60 - sec / 3600;
		else
			return deg + min / 60 + sec / 3600;
	}
	return 0;
}

const StelLocation StelLocationMgr::locationForString(const QString& s) const
{
	auto iter = locations.find(s);
	if (iter!=locations.end())
	{
		return iter.value();
	}
	// Maybe this is a city and country names (old format of the data)?
	static const QRegularExpression cnreg("(.+),\\s+(.+)$");
	QRegularExpressionMatch cnMatch=cnreg.match(s);
	if (cnMatch.hasMatch())
	{
		// NOTE: This method will give wrong data for some Russians and U.S. locations
		//       (Asian locations for Russia and for locations on Hawaii for U.S.)
		QString city = cnMatch.captured(1).trimmed();
		QString country = cnMatch.captured(2).trimmed();
		auto iter = locations.find(QString("%1, %2").arg(city, pickRegionFromCountry(country)));
		if (iter!=locations.end())
		{
			return iter.value();
		}
	}
	// Maybe a full Location line? (e.g. ObservationLists)
	if (s.count("\t")>6) // heuristic. Regular locations should not have tabs in the name.
		return StelLocation::createFromLine(s);

	StelLocation ret;
	// Maybe it is a coordinate set with elevation?
	static const QRegularExpression csreg("(.+),\\s*(.+),\\s*(.+)");
	QRegularExpressionMatch csMatch=csreg.match(s);
	if (csMatch.hasMatch())
	{
		bool ok;
		// We have a set of coordinates
		ret.setLatitude(parseAngle(csMatch.captured(1).trimmed(), &ok));
		if (!ok) ret.role = '!';
		ret.setLongitude(parseAngle(csMatch.captured(2).trimmed(), &ok));
		if (!ok) ret.role = '!';
		ret.altitude = csMatch.captured(3).trimmed().toInt(&ok);
		if (!ok) ret.role = '!';
		ret.name = QString("%1, %2").arg(QString::number(ret.getLatitude(), 'f', 2), QString::number(ret.getLongitude(), 'f', 2));
		ret.planetName = "Earth";
		return ret;
	}
	// Maybe it is a coordinate set without elevation? (e.g. GPS 25.107363,121.558807 )
	static const QRegularExpression reg("(?:(.+)\\s+)?(.+),\\s*(.+)"); // FIXME: Seems regexp is not very good
	QRegularExpressionMatch match=reg.match(s);
	if (match.hasMatch())
	{
		bool ok;
		// We have a set of coordinates
		ret.setLatitude(parseAngle(match.captured(2).trimmed(), &ok));
		if (!ok) ret.role = '!';
		ret.setLongitude(parseAngle(match.captured(3).trimmed(), &ok));
		if (!ok) ret.role = '!';
		ret.name = match.captured(1).trimmed();
		ret.planetName = "Earth";
		return ret;
	}
	// All attempts to decode have failed. Return invalid location with this name.
	ret.name=s;
	ret.planetName = "Earth";
	ret.role = '!';
	return ret;
}

const StelLocation StelLocationMgr::locationFromCLI() const
{
	StelLocation ret;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->beginGroup("location_run_once");

	const auto latVar = conf->value("latitude");
	if (latVar.isValid())
		ret.setLatitude(180/M_PI * latVar.toDouble());
	else
		ret.role = '!';

	const auto lonVar = conf->value("longitude");
	if (lonVar.isValid())
		ret.setLongitude(180/M_PI * lonVar.toDouble());
	else
		ret.role = '!';
	bool ok;
	ret.altitude = conf->value("altitude", 0).toInt(&ok);
	ret.planetName = conf->value("home_planet", "Earth").toString();
	ret.landscapeKey = conf->value("landscape_name", "guereins").toString();
	conf->endGroup();
	conf->remove("location_run_once");
	ret.state="CLI"; // flag this location with a marker for handling in LandscapeMgr::init(). state is not displayed anywhere, so I expect no issues from that.
	return ret;
}

// Get whether a location can be permanently added to the list of user locations
bool StelLocationMgr::canSaveUserLocation(const StelLocation& loc) const
{
	return loc.isValid() && locations.find(loc.getID())==locations.end();
}

// Add permanently a location to the list of user locations
bool StelLocationMgr::saveUserLocation(const StelLocation& loc)
{
	if (!canSaveUserLocation(loc))
		return false;

	// Add in the program
	locations[loc.getID()]=loc;

	//emit before saving the list
	emit locationListChanged();

	// Append to the user location file
	QString cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Flags(StelFileMgr::Writable|StelFileMgr::File));
	if (cityDataPath.isEmpty())
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << QDir::toNativeSeparators(StelFileMgr::getUserDir()+"/data");
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << QDir::toNativeSeparators(cityDataPath);
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return false;
	}

	QTextStream outstream(&sourcefile);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	outstream.setEncoding(QStringConverter::Utf8);
#else
	outstream.setCodec("UTF-8");
#endif
	outstream << loc.serializeToLine() << '\n';
	sourcefile.close();

	return true;
}

// Get whether a location can be deleted from the list of user locations
// If the location comes from the base read only list, it cannot be deleted
bool StelLocationMgr::canDeleteUserLocation(const QString& id) const
{
	auto iter=locations.find(id);

	// If it's not known at all there is a problem
	if (iter==locations.end())
		return false;

	return iter.value().isUserLocation;
}

// Delete permanently the given location from the list of user locations
// If the location comes from the base read only list, it cannot be deleted and false is returned
bool StelLocationMgr::deleteUserLocation(const QString& id)
{
	if (!canDeleteUserLocation(id))
		return false;

	locations.remove(id);

	//emit before saving the list
	emit locationListChanged();

	// Resave the whole remaining user locations file
	QString cityDataPath = StelFileMgr::findFile("data/user_locations.txt", StelFileMgr::Writable);
	if (cityDataPath.isEmpty())
	{
		if (!StelFileMgr::exists(StelFileMgr::getUserDir()+"/data"))
		{
			if (!StelFileMgr::mkDir(StelFileMgr::getUserDir()+"/data"))
			{
				qWarning() << "ERROR - cannot create non-existent data directory" << QDir::toNativeSeparators(StelFileMgr::getUserDir()+"/data");
				qWarning() << "Location cannot be saved";
				return false;
			}
		}

		cityDataPath = StelFileMgr::getUserDir()+"/data/user_locations.txt";
		qWarning() << "Will create a new user location file: " << QDir::toNativeSeparators(cityDataPath);
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open location data file: " << QDir::toNativeSeparators(cityDataPath);
		return false;
	}

	QTextStream outstream(&sourcefile);
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	outstream.setEncoding(QStringConverter::Utf8);
#else
	outstream.setCodec("UTF-8");
#endif

	for (QMap<QString, StelLocation>::ConstIterator iter=locations.constBegin();iter!=locations.constEnd();++iter)
	{
		if (iter.value().isUserLocation)
		{
			outstream << iter.value().serializeToLine() << '\n';
		}
	}

	sourcefile.close();
	return true;
}

// lookup location from IP address.
void StelLocationMgr::locationFromIP()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	QNetworkRequest req( QUrl( conf->value("main/geoip_api_url", "https://freegeoip.stellarium.org/json/").toString() ) );
	req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
	req.setRawHeader("User-Agent", StelUtils::getUserAgentString().toLatin1());
	QNetworkReply* networkReply=StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(networkReply, SIGNAL(finished()), this, SLOT(changeLocationFromNetworkLookup()));
}

#ifdef ENABLE_GPS
void StelLocationMgr::locationFromGPS(int interval)
{
	bool verbose=qApp->property("verbose").toBool();

#ifdef ENABLE_LIBGPS
	if(!libGpsHelper)
	{
		libGpsHelper = new LibGPSLookupHelper(this);
		connect(libGpsHelper, SIGNAL(queryFinished(StelLocation)), this, SLOT(changeLocationFromGPSQuery(StelLocation)));
		connect(libGpsHelper, SIGNAL(queryError(QString)), this, SLOT(gpsQueryError(QString)));
	}
	if(libGpsHelper->isReady())
	{
		if (interval<0)
			libGpsHelper->query();
		else
		{
			libGpsHelper->setPeriodicQuery(interval);
			// It seemed possible to leave the LibGPShelper object alive once created, because it does not block
			// access to the GPS device. However, under pathological circumstances (start/stop of GPSD daemon
			// after first query and while Stellarium is running, annoying GPSD in other ways, etc.,
			// the LibGPSLookupHelper may signal ready but still shows problems.
			// It seems better to also destroy it after finish of queries here and in case of non-readiness below.
			if (interval==0)
			{
				if (verbose)
					qDebug() << "Deactivating and deleting LibGPShelper...";
				delete libGpsHelper;
				libGpsHelper=Q_NULLPTR;
				emit gpsQueryFinished(true); // signal "successful operation", avoid showing any error in GUI.
				if (verbose)
					qDebug() << "Deactivating and deleting LibGPShelper... DONE";
			}
		}
		return;
	}
	else
	{
		qDebug() << "LibGPSHelper not ready. Attempting a direct NMEA connection instead.";
		delete libGpsHelper;
		libGpsHelper=Q_NULLPTR;
	}
#endif
	if(!nmeaHelper)
	{
		if (verbose)
			qDebug() << "Creating new NMEAhelper...";
		nmeaHelper = new NMEALookupHelper(this);
		connect(nmeaHelper, SIGNAL(queryFinished(StelLocation)), this, SLOT(changeLocationFromGPSQuery(StelLocation)));
		connect(nmeaHelper, SIGNAL(queryError(QString)), this, SLOT(gpsQueryError(QString)));
		if (verbose)
			qDebug() << "Creating new NMEAhelper...done";
	}
	if(nmeaHelper->isReady())
	{
		if (interval<0)
			nmeaHelper->query();
		else
		{
			nmeaHelper->setPeriodicQuery(interval);
			if (interval==0)
			{
				if (verbose)
					qDebug() << "Deactivating and deleting NMEAhelper...";
				delete nmeaHelper;
				nmeaHelper=Q_NULLPTR;
				emit gpsQueryFinished(true); // signal "successful operation", avoid showing any error in GUI.
				if (verbose)
					qDebug() << "Deactivating and deleting NMEAhelper... DONE";
			}
		}
	}
	else
	{
		// something went wrong. However, a dysfunctional nmeaHelper may still exist, better delete it.
		if (verbose)
			qDebug() << "nmeaHelper not ready. Something went wrong.";
		delete nmeaHelper;
		nmeaHelper=Q_NULLPTR;
#ifndef Q_OS_WIN
		emit gpsQueryFinished(false);
#else
		if (!positionSource)
			positionSource = QGeoPositionInfoSource::createDefaultSource(this);
		if (positionSource)
		{
			if (interval)
			{
				if (verbose)
					qDebug() << "Creating new positionSource...";
				positionSource->setUpdateInterval(interval);
				connect(positionSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
				positionSource->startUpdates();
				if (verbose)
					qDebug() << "Creating new positionSource...done";
			}
			else
			{
				if (verbose)
					qDebug() << "Deactivating and deleting gps...";
				positionSource->stopUpdates();
				delete positionSource;
				positionSource=Q_NULLPTR;
			}
			emit gpsQueryFinished(true); // signal "successful operation", avoid showing any error in GUI.
		}
		else
		{
			emit gpsQueryFinished(false);
		}
#endif
	}
}

#ifdef Q_OS_WIN
void StelLocationMgr::positionUpdated(QGeoPositionInfo info)
{
	bool verbose=qApp->property("verbose").toBool();
	StelLocation loc;
	if (info.isValid())
	{
		loc.setLongitude(info.coordinate().longitude());
		loc.setLatitude(info.coordinate().latitude());
		double a = info.coordinate().altitude();
		loc.altitude = qIsNaN(a) ? 0 : qRound(a);
		changeLocationFromGPSQuery(loc);
	}
	else
	{
		// something went wrong. However, a dysfunctional positionSource may still exist, better delete it.
		if (verbose)
			qDebug() << "gps not ready. Something went wrong.";
		positionSource->stopUpdates();
		delete positionSource;
		positionSource=Q_NULLPTR;
		emit gpsQueryFinished(false);
	}
}
#endif

void StelLocationMgr::changeLocationFromGPSQuery(const StelLocation &locin)
{
	bool verbose=qApp->property("verbose").toBool();
	StelCore *core=StelApp::getInstance().getCore();
	StelLocation loc=locin;
	loc.lightPollutionLuminance=StelLocation::DEFAULT_LIGHT_POLLUTION_LUMINANCE;
	// Usually you don't leave your time zone with GPS.
	loc.ianaTimeZone=core->getCurrentTimeZone();
	loc.isUserLocation=true;
	loc.planetName="Earth";
	loc.name=QString("GPS %1%2 %3%4")
		.arg(loc.getLatitude()<0?"S":"N").arg(qRound(abs(loc.getLatitude())))
		.arg(loc.getLongitude()<0?"W":"E").arg(qRound(abs(loc.getLongitude())));
	QColor color=getColorForCoordinates(loc.getLongitude(), loc.getLatitude());
	core->moveObserverTo(loc, 0.0, 0.0, QString("ZeroColor(%1)").arg(Vec3f(color).toStr()));
	if (nmeaHelper)
	{
		if (verbose)
			qDebug() << "Change location from NMEA... successful. NMEAhelper stays active.";
	}
	if (verbose)
	{
		qDebug() << "Location in progress: Long=" << loc.getLongitude() << " Lat=" << loc.getLatitude() << " Alt" << loc.altitude;
		qDebug() << "New location named " << loc.name;
		qDebug() << "queryOK, resetting GUI";
	}
	emit gpsQueryFinished(true);
}

void StelLocationMgr::gpsQueryError(const QString &err)
{
	qWarning()<<err;
	if (nmeaHelper)
	{
		nmeaHelper->setPeriodicQuery(0); // stop queries if they came periodically.
		//qDebug() << "Would Close nmeaHelper during error...";
		// We should close the serial line to let other programs use the GPS device. (Not needed for the GPSD solution!)
		//delete nmeaHelper;
		//nmeaHelper=Q_NULLPTR;
		//qDebug() << "Would Close nmeaHelper during error.....successful";
	}
	qDebug() << "GPS queryError, resetting GUI";
	emit gpsQueryFinished(false);
}
#endif

// slot that receives IP-based location data from the network.
void StelLocationMgr::changeLocationFromNetworkLookup()
{
	StelCore *core=StelApp::getInstance().getCore();
	QNetworkReply* networkReply = qobject_cast<QNetworkReply*>(sender());
	if (!networkReply)
	    return;

	if (networkReply->error() == QNetworkReply::NoError && networkReply->bytesAvailable()>0)
	{
		// success		 
		try
		{
			QVariantMap locMap = StelJsonParser::parse(networkReply->readAll()).toMap();

			QString ipRegion = locMap.value("region_name").toString();
			if (ipRegion.isEmpty())
				ipRegion = locMap.value("region").toString();
			QString ipCity = locMap.value("city").toString();
			QString ipCountry = locMap.value("country_name").toString(); // NOTE: Got a short name of country
			QString ipCountryCode = locMap.value("country_code").toString();
			QString ipTimeZone = locMap.value("time_zone").toString();
			if (ipTimeZone.isEmpty())
				ipTimeZone = locMap.value("timezone").toString();
			double latitude=locMap.value("latitude").toDouble();
			double longitude=locMap.value("longitude").toDouble();

			qDebug() << "Got location" << QString("%1, %2, %3 (%4, %5; %6)").arg(ipCity, ipRegion, ipCountry).arg(latitude).arg(longitude).arg(ipTimeZone) << "for IP" << locMap.value("ip").toString();

			StelLocation loc;
			loc.name    = (ipCity.isEmpty() ? QString("%1, %2").arg(latitude).arg(longitude) : ipCity);
			loc.state   = (ipRegion.isEmpty() ? "IPregion"  : ipRegion);
			loc.region = pickRegionFromCountryCode(ipCountryCode.isEmpty() ? "" : ipCountryCode.toLower());
			loc.role    = QChar(0x0058); // char 'X'
			loc.population = 0;
			loc.setLatitude  (static_cast<float>(latitude));
			loc.setLongitude (static_cast<float>(longitude));
			loc.altitude = 0;
			loc.lightPollutionLuminance = StelLocation::DEFAULT_LIGHT_POLLUTION_LUMINANCE;
			loc.ianaTimeZone = (ipTimeZone.isEmpty() ? "" : ipTimeZone);
			loc.planetName = "Earth";
			loc.landscapeKey = "";

			// Ensure that ipTimeZone is a valid IANA timezone name!
			QTimeZone ipTZ(ipTimeZone.toUtf8());
			core->setCurrentTimeZone( !ipTZ.isValid() || ipTimeZone.isEmpty() ? "LMST" : ipTimeZone);
			QColor color=getColorForCoordinates(loc.getLongitude(), loc.getLatitude());
			core->moveObserverTo(loc, 0.0, 0.0, QString("ZeroColor(%1)").arg(Vec3f(color).toStr()));

			QSettings* conf = StelApp::getInstance().getSettings();
			conf->setValue("init_location/last_location", QString("%1, %2").arg(latitude).arg(longitude));
		}
		catch (const std::exception& e)
		{
			qDebug() << "Failure getting IP-based location: answer is in not acceptable format! Error: " << e.what()
					<< "\nLet's use Paris, France as default location...";
			core->moveObserverTo(getLastResortLocation(), 0.0, 0.0, "guereins"); // Answer is not in JSON format! A possible block by DNS server or firewall
		}
	}
	else
	{
		qDebug() << "Failure getting IP-based location: \n\t" << networkReply->errorString();
		// If there is a problem, this must not change to some other location! Just ignore.
	}
	networkReply->deleteLater();
}

LocationMap StelLocationMgr::pickLocationsNearby(const QString &planetName, const float longitude, const float latitude, const float radiusDegrees)
{
	QMap<QString, StelLocation> results;
	QMapIterator<QString, StelLocation> iter(locations);
	while (iter.hasNext())
	{
		iter.next();
		const StelLocation *loc=&iter.value();
		if ( (loc->planetName == planetName) &&
				(StelLocation::distanceDegrees(longitude, latitude, loc->getLongitude(), loc->getLatitude()) <= radiusDegrees) )
		{
			results.insert(iter.key(), iter.value());
		}
	}
	return results;
}

LocationMap StelLocationMgr::pickLocationsInRegion(const QString &region)
{
	QMap<QString, StelLocation> results;
	QMapIterator<QString, StelLocation> iter(locations);
	while (iter.hasNext())
	{
		iter.next();
		const StelLocation *loc=&iter.value();
		if (loc->region == region)
		{
			results.insert(iter.key(), iter.value());
		}
	}
	return results;
}

void StelLocationMgr::loadCountries()
{
	// Load ISO 3166-1 two-letter country codes from file
	// The format is "[code][tab][country name containing spaces][newline]"
	countryNameToCodeMap.clear();
	QFile textFile(StelFileMgr::findFile("data/iso3166.tab"));
	if(textFile.open(QFile::ReadOnly | QFile::Text))
	{
		QString line;
		int readOk=0;
		while(!textFile.atEnd())
		{
			line = QString::fromUtf8(textFile.readLine());
			if (line.startsWith("//") || line.startsWith("#") || line.isEmpty())
				continue;

			if (!line.isEmpty())
			{
				#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
				QStringList list=line.split("\t", Qt::KeepEmptyParts);
				#else
				QStringList list=line.split("\t", QString::KeepEmptyParts);
				#endif
				QString code = list.at(0).trimmed().toLower();
				QString country = list.at(1).trimmed().replace("&", "and");
				countryNameToCodeMap.insert(country, code);
				readOk++;
			}
		}
		textFile.close();
		if (readOk>0)
			qDebug() << "Loaded" << readOk << "countries";
		else
			qDebug() << "ERROR: List of countries was not loaded!";
	}
	// aliases for some countries to backward compatibility
	countryNameToCodeMap.insert("Russian Federation", "ru");
	countryNameToCodeMap.insert("Taiwan (Province of China)", "tw");
}

void StelLocationMgr::loadRegions()
{
	QFile geoFile(StelFileMgr::findFile("data/regions-geoscheme.tab"));
	if(geoFile.open(QFile::ReadOnly | QFile::Text))
	{
		QString line;
		int readOk=0;
		regions.clear();
		countryCodeToRegionMap.clear();
		while(!geoFile.atEnd())
		{
			line = QString::fromUtf8(geoFile.readLine());
			if (line.startsWith("//") || line.startsWith("#") || line.isEmpty())
				continue;

			if (!line.isEmpty())
			{
				#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
				QStringList list=line.split("\t", Qt::KeepEmptyParts);
				#else
				QStringList list=line.split("\t", QString::KeepEmptyParts);
				#endif

				QString regionName = list.at(2).trimmed();
				QString countries;
				if (list.size()>3)
					countries = list.at(3).trimmed().toLower();
				GeoRegion region;
				region.code = list.at(0).trimmed().toInt();
				region.planet = list.at(1).trimmed();
				region.regionName = regionName;
				region.countries = countries;

				if (!countries.isEmpty())
				{
					#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
					QStringList country=countries.split(",", Qt::KeepEmptyParts);
					#else
					QStringList country=countries.split(",", QString::KeepEmptyParts);
					#endif
					for (int i = 0; i<country.size(); i++)
					{
						countryCodeToRegionMap.insert(country.at(i), regionName);
					}
				}

				regions.push_back(region);
				readOk++;
			}
		}
		geoFile.close();
		if (readOk>0)
			qDebug() << "Loaded" << readOk << "regions";
		else
			qDebug() << "ERROR: List of regions was not loaded!";
	}
}

QStringList StelLocationMgr::getRegionNames(const QString& planet) const
{
	QStringList allregions;
	if (planet.isEmpty())
	{
		for (int i=0;i<regions.size();i++)
			allregions.append(regions.at(i).regionName);
	}
	else
	{
		for (int i=0;i<regions.size();i++)
		{
			if (planet.contains(regions.at(i).planet, Qt::CaseInsensitive) && !planet.contains("Observer", Qt::CaseInsensitive))
				allregions.append(regions.at(i).regionName);
		}
	}

	return allregions;
}

QString StelLocationMgr::pickRegionFromCountryCode(const QString &countryCode)
{
	QMap<QString, QString>::ConstIterator i = countryCodeToRegionMap.find(countryCode);
	return (i!=countryCodeToRegionMap.constEnd()) ? i.value() : QString();
}

QString StelLocationMgr::pickRegionFromCountry(const QString &country)
{
	QMap<QString, QString>::ConstIterator i = countryNameToCodeMap.find(country);
	QString code = (i!=countryNameToCodeMap.constEnd()) ? i.value() : QString();
	return pickRegionFromCountryCode(code);
}

QString StelLocationMgr::pickRegionFromCode(int regionCode)
{
	QString region;
	for(int i=0;i<regions.size();i++)
	{
		if (regions.at(i).code == regionCode)
			region = regions.at(i).regionName;
	}
	return region;
}

// https://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system
QPair<Vec3d,Vec2d> StelLocationMgr::geo2utm(const double longitude, const double latitude, const int zone)
{
	Q_ASSERT(zone<=60);
	if ((latitude>84) || (latitude<-80))
	{
		qCritical() << "UTM Polar area not implemented. Returning zero result.";
		return {Vec3d(0.), Vec2d(0.)};
	}
	static const double a=6378.137; // earth radius
	static const double f=1./298.257223563;
	static const double k0=0.9996;
	static const double E0=500;
	const double N0=latitude > 0 ? 0. : 10000.;
	// preliminaries
	static const double n = f/(2.-f);
	static const double n2 = n*n;
	static const double n3 = n2*n;
	static const double n4 = n3*n;
	static const double A = a/(1.+n) * (1.+n2/4. + n4/64.);
	static const double k0A=k0*A;
	static const Vec3d alpha = {n/2.-2./3.*n2+ 5./16.*n3, 13./48.*n2-3./5.*n3, 61./240.*n3};
	const double utmZne= (zone!=0) ? zone : utmZone(longitude, latitude).first;
	const double refLongitude=utmZne*6.-183.;
	const double sinPhi=sin(latitude*M_PI_180);
	const double dLong=(longitude-refLongitude)*M_PI_180;
	const double cosDLong=cos(dLong);
	const double tanDLong=tan(dLong);
	const double tFac=2.*sqrt(n)/(1.+n);
	const double t    = sinh(atanh(sinPhi)-tFac*atanh(tFac*sinPhi));
	const double xiP  = atan(t/cosDLong);
	const double etaP = atanh(sin(dLong)/sqrt(1.+t*t));
	double sigma=1.;
	for (int j=1; j<=3; ++j)
		sigma += 2.*j*alpha[j-1]*cos(2.*j*xiP)*cosh(2.*j*etaP);
	double tau=0.;
	for (int j=1; j<=3; ++j)
		tau   += 2.*j*alpha[j-1]*sin(2.*j*xiP)*sinh(2.*j*etaP);
	double E=etaP;
	for (int j=1; j<=3; ++j)
		E += alpha[j-1]*cos(2.*j*xiP)*sinh(2.*j*etaP);
	E *= k0A;
	E += E0;
	double N=xiP;
	for (int j=1; j<=3; ++j)
		N += alpha[j-1]*sin(2.*j*xiP)*cosh(2.*j*etaP);
	N *= k0A;
	N += N0;
	const double kFac=(1.-n)/(1.+n)*tan(latitude*M_PI_180);
	const double k=k0A/a * sqrt((1.+kFac*kFac) * (sigma*sigma+tau*tau) / (t*t + cosDLong*cosDLong));
	const double gamma = atan((tau*sqrt(1.+t*t)+sigma*t*tanDLong)/(sigma*sqrt(1.+t*t)-tau*t*tanDLong));
	return {Vec3d(E*1000., N*1000., refLongitude), Vec2d(gamma, k)};
}

// https://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system
QPair<Vec3d,Vec2d> StelLocationMgr::utm2geo(const double easting, const double northing, const int zone, const bool north)
{
	Q_ASSERT(zone<=60);
	static const double a=6378.137; // earth radius
	static const double f=1./298.257223563;
	static const double k0=0.9996;
	static const double E0=500;
	const double N0=north ? 0. : 10000.;
	const double Hemi=north ? 1. : -1.;
	// preliminaries
	static const double n = f/(2.-f);
	static const double n2 = n*n;
	static const double n3 = n2*n;
	static const double n4 = n3*n;
	static const double A = a/(1.+n) * (1.+n2/4. + n4/64.);
	static const double k0A=k0*A;
	static const Vec3d beta  = {n/2.-2./3.*n2+37./96.*n3,  1./48.*n2+1./15.*n3, 17./480.*n3};
	static const Vec3d delta = {2.*n-2./3.*n2-2.*n3,       7./3. *n2-8./ 5.*n3, 56./15. *n3};
	const double xi=(northing*0.001-N0)/k0A;
	const double eta=(easting*0.001-E0)/k0A;
	double xiP=xi;
	for (int j=1; j<=3; ++j)
		xiP -= beta[j-1]*sin(2.*j*xi)*cosh(2.*j*eta);
	double etaP=eta;
	for (int j=1; j<=3; ++j)
		etaP   -= beta[j-1]*cos(2.*j*xi)*sinh(2.*j*eta);
	double sigmaP=1.;
	for (int j=1; j<=3; ++j)
		sigmaP -= 2.*j*beta[j-1]*cos(2.*j*xi)*cosh(2.*j*eta);
	double tauP=0.;
	for (int j=1; j<=3; ++j)
		tauP   += 2.*j*beta[j-1]*sin(2.*j*xi)*sinh(2.*j*eta);
	const double chi=asin(sin(xiP)/cosh(etaP));
	double phi=chi;
	for (int j=1; j<=3; ++j)
		phi += delta[j-1]*sin(2.*j*chi);
	const double refLongitude=zone*6.-183.;
	const double sinhEtaP=sinh(etaP);
	const double cosXiP=cos(xiP);
	const double lng=refLongitude+atan(sinhEtaP/cosXiP)*M_180_PI;
	const double kFac=(1.-n)/(1.+n)*tan(phi);
	const double k=k0A/a * sqrt((1.+kFac*kFac) * (cosXiP*cosXiP+sinhEtaP*sinhEtaP) / (sigmaP*sigmaP + tauP*tauP));
	const double tanXiPtanhEtaP=tan(xiP)*tanh(etaP);
	const double gamma = Hemi * atan((tauP+sigmaP*tanXiPtanhEtaP)/(sigmaP-tauP*tanXiPtanhEtaP));
	return {Vec3d(lng, phi*M_180_PI, refLongitude), Vec2d(gamma, k)};
}

// https://stackoverflow.com/questions/9186496/determining-utm-zone-to-convert-from-longitude-latitude
QPair<int, QChar> StelLocationMgr::utmZone(const double longitude, const double latitude)
{
	QChar letter='I'; // I does not exist as letter code. It signifies "invalid".
	int zone=0; // remains 0 for polar zones
	// Special zones for Svalbard
	if (latitude >= 72.0 && latitude <= 84.0 ) {
		if (longitude >= 0.0  && longitude <  9.0)
			zone=31;
		if (longitude >= 9.0  && longitude < 21.0)
			zone=33;
		if (longitude >= 21.0 && longitude < 33.0)
			zone=35;
		if (longitude >= 33.0 && longitude < 42.0)
			zone=37;
		letter='X';
	}
	// Special zones for Norway
	else if (latitude >= 56.0 && latitude < 64.0 ) {
		if (longitude >= 0.0  && longitude <  3.0)
			zone=31;
		if (longitude >= 3.0  && longitude < 12.0)
			zone=32;
		letter='V';
	}
	// Everything in the middle
	else if ( (latitude>-80.0) && (latitude<=84.0) ){
		static const QString mid_zones("CDEFGHJKLMNPQRSTUVWX"); // C to X, skip I and O
		letter = mid_zones[ qMin((int(floor(latitude)) + 80) / 8 , 19) ];
		zone   = StelUtils::amod((int(floor(longitude)) + 180) / 6 , 60) + 1; // modulo in case longitude is 0 to 360 instead of -180 to 180
	}
	// North + South Poles
	else if (latitude > 84.0)
		letter = StelUtils::fmodpos(longitude + 180, 360) < 180 ? 'Y' : 'Z';
	else if (latitude < -80.0)
		letter = StelUtils::fmodpos(longitude + 180, 360) < 180 ? 'A' : 'B';
	else
		qCritical() << "Cannot determine UTM zone for long" << longitude << "/ lat" << latitude;
	return QPair<int, QChar>(zone, letter);
}

// Check timezone string and return either the same or the corresponding string that we use in the Stellarium location database.
// If timezone name starts with "UTC", always return unchanged.
// This is required to store timezone names exactly as we know them, and not mix ours and current-iana spelling flavour.
// In practice, reverse lookup to locationDBToIANAtranslations
QString StelLocationMgr::sanitizeTimezoneStringForLocationDB(QString tzString)
{
	if (tzString.startsWith("UTC"))
		return tzString;
	QByteArray res=locationDBToIANAtranslations.key(tzString.toUtf8(), "---");
	if ( res != "---")
		return QString(res);
	return tzString;
}

// Attempt to translate a timezone name from those used in Stellarium's location database to a name which is valid
// as ckeckable by QTimeZone::availableTimeZoneIds(). That list may be updated anytime and is known to differ
// between OSes. Some spellings may be different, or in some cases some names get simply translated to "UTC+HH:MM" style.
// The empty string gets translated to "UTC".
QString StelLocationMgr::sanitizeTimezoneStringFromLocationDB(QString dbString)
{
	if (dbString.startsWith("UTC"))
		return dbString;
	// Maybe silences a debug later:
	if (dbString=="")
		return "UTC";
	QByteArray res=locationDBToIANAtranslations.value(dbString.toUtf8(), "---");
	if ( res != "---")
		return QString(res);
	return dbString;
}

QStringList StelLocationMgr::getAllTimezoneNames() const
{
	QStringList ret;

	QMapIterator<QString, StelLocation> iter(locations);
	while (iter.hasNext())
	{
		iter.next();
		const StelLocation *loc=&iter.value();
		QString tz(loc->ianaTimeZone);
		if (!ret.contains(tz))
			ret.append(tz);
	}
	// 0.19: So far, this includes the existing names, but QTimeZone also has a few other names.
	// Accept others after testing against sanitized names, and especially all UT+/- names!

	auto tzList = QTimeZone::availableTimeZoneIds(); // System dependent set of IANA timezone names.
	for (const auto& tz : qAsConst(tzList))
	{
		QString tzcand=sanitizeTimezoneStringFromLocationDB(tz); // try to find name as we use it in the program.
		if (!ret.contains(tzcand))
		{
			//qDebug() << "Extra insert Qt/IANA TZ entry from QTimeZone::availableTimeZoneIds(): " << tz << "as" << tzcand;
			ret.append(QString(tzcand));
		}
	}

	// Special cases!
	ret.append("LMST");
	ret.append("LTST");
	ret.append("system_default");
	ret.sort();
	return ret;
}

// To be connected from StelCore::locationChanged(loc)
void StelLocationMgr::changePlanetMapForLocation(StelLocation loc)
{
	if (loc.planetName==planetName)
		return;

	planetName=loc.planetName;
	if (planetName=="Earth")
		planetSurfaceMap=QImage(":/graphicGui/miscWorldMap.jpg");
	else
	{
		SolarSystem *ssm=GETSTELMODULE(SolarSystem);
		PlanetP p=ssm->searchByEnglishName(loc.planetName); // nullptr for "SpaceShip" transitions
		QString mapName="textures/" + (p ? p->getTextMapName() : planetName) + ".png";

		if (!planetSurfaceMap.load(StelFileMgr::findFile(mapName, StelFileMgr::File)))
		{
			// texture not found. Use a gray pixel.
			planetSurfaceMap=QImage(16,16,QImage::Format_RGB32);
			planetSurfaceMap.fill(QColor(64, 64, 64));
		}
	}
}

QColor StelLocationMgr::getColorForCoordinates(const double lng, const double lat) const
{
	QPoint imgPoint( (lng+180.)/ 360. * planetSurfaceMap.width(),
			 (90.-lat) / 180. * planetSurfaceMap.height());

	// Sample the map pixel color. Use a small box to avoid 1-pixel surprises.
	QImage sampledPix=planetSurfaceMap.copy(QRect(imgPoint-QPoint(1,1), QSize(2,2))).scaled(1,1);
	return sampledPix.pixelColor(0,0);
}
