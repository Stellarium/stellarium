/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#include "EOPUpdater.hpp"

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QVector>
#include <cmath>
#include <utility>

QList<EOPUpdater::Source> EOPUpdater::defaultSources()
{
	// (SS) 2026-11-05 JPL Horizons uses the IAU 1980 nutation model (dPsi, dEps), so only the IAU 1980 set is downloaded by default:
	// standard series finals.all and long term EOP 14 C04 (dPsi, dEps relative to IAU 1980). The IAU 2000A files (dX, dY) are in
	// optionalSources().
	// Both files are in the same folder of the IERS data center (links given by the author, 2026-10-07)
	const QString data = QStringLiteral("https://datacenter.iers.org/data/csv/");
	QList<Source> list;
	list << Source{"finals.all.csv", QUrl(data + "finals.all.csv"),
	               QStringLiteral("Standard series, IAU 1980 (dPsi, dEps), with predictions"), true, 7};
	// (SS) 2026-11-05 not updated by the IERS since January 2026 (it ends 2026-01-05): a copy is only downloaded again after a year
	list << Source{"eopc04_14.62-now.csv", QUrl(data + "eopc04_14.62-now.csv"),
	               QStringLiteral("Long term series EOP 14 C04, IAU 1980 (dPsi, dEps)"), true, 365};
	return list;
}

QList<EOPUpdater::Source> EOPUpdater::optionalSources()
{
	// (SS) 2026-11-05 IAU 2006/2000A set: standard series finals2000A.all and long term EOP 20 C04 u24 (dX, dY).
	// Only downloaded when the IAU 2000A model is the one selected in the EOP dialog.
	const QString data = QStringLiteral("https://datacenter.iers.org/data/csv/");
	QList<Source> list;
	list << Source{"finals2000A.all.csv", QUrl(data + "finals2000A.all.csv"),
	               QStringLiteral("Standard series, IAU 2000A (dX, dY), with predictions"), false, 7};
	list << Source{"eopc04_20u24.1962-now.csv", QUrl(data + "eopc04_20u24.1962-now.csv"),
	               QStringLiteral("Long term series EOP 20 C04, IAU 2000A (dX, dY)"), false, 14};
	return list;
}

EOPUpdater::Source EOPUpdater::horizonsSource()
{
	Source s;
	s.fileName = QStringLiteral("horizons_ut1utc.csv");     // becomes horizons_ut1utc_YYMMDD.csv
	s.url = QUrl(QStringLiteral("https://ssd.jpl.nasa.gov/api/horizons_file.api"));
	s.description = QStringLiteral("UT1-UTC of JPL Horizons (quantity 49), IAU 1980 model");
	s.isDefault = true;
	s.refreshDays = 1;
	s.kind = Source::Kind::HorizonsUT1UTC;
	return s;
}

QString EOPUpdater::horizonsRequest(const QDate& stop)
{
	// Batch file of the author (my_input_file.txt) that gives UT1-UTC of Horizons: any target, geocentric, quantity 49
	return QString(
	    "!$$SOF\n"
	    "MAKE_EPHEM=YES\n"
	    "COMMAND=10\n"
	    "EPHEM_TYPE=OBSERVER\n"
	    "CENTER='@399'\n"
	    "START_TIME='1962-01-01'\n"
	    "STOP_TIME='%1'\n"
	    "STEP_SIZE='1 DAYS'\n"
	    "QUANTITIES='49'\n"
	    "REF_SYSTEM='ICRF'\n"
	    "CAL_FORMAT='BOTH'\n"
	    "CAL_TYPE='M'\n"
	    "TIME_DIGITS='FRACSEC'\n"
	    "ANG_FORMAT='HMS'\n"
	    "APPARENT='AIRLESS'\n"
	    "RANGE_UNITS='AU'\n"
	    "SUPPRESS_RANGE_RATE='NO'\n"
	    "SKIP_DAYLT='NO'\n"
	    "SOLAR_ELONG='0,180'\n"
	    "EXTRA_PREC='YES'\n"
	    "R_T_S_ONLY='NO'\n"
	    "CSV_FORMAT='YES'\n"
	    "OBJ_DATA='YES'\n").arg(stop.toString("yyyy-MM-dd"));
}

bool EOPUpdater::horizonsToOverlayCSV(const QString& answer, QString& csv, QString* error, int* nRows, int minRows)
{
	csv.clear();
	if (nRows) *nRows = 0;
	auto fail = [&](const QString& why)
	{
		if (error)
		{
			// what the server said, to help to understand (an error message of Horizons, a page of the network...)
			QString head;
			const QStringList lines = answer.split('\n');
			for (const QString& l : lines)
				if (!l.trimmed().isEmpty() && head.size() < 200)
					head += l.trimmed() + " ";
			*error = why + (head.isEmpty() ? QString() : " (answer starts with: " + head.trimmed().left(200) + ")");
		}
		return false;
	};

	static const QRegularExpression rowRe(QStringLiteral(
	    "^\\s*(\\d{4})-([A-Za-z]{3})-(\\d{2}) (\\d{2}:\\d{2}:\\d{2}(?:\\.\\d+)?),\\s*([\\d.]+),[^,]*,[^,]*,\\s*(\\S+?),\\s*$"));
	static const QRegularExpression runRe(QStringLiteral("Ephemeris / WWW_USER\\s+\\w{3} (\\w{3})\\s+(\\d+) [\\d:]+ (\\d{4})"));
	static const QRegularExpression eopFileRe(QStringLiteral("EOP file\\s*:\\s*(\\S+)"));
	static const QRegularExpression coverageRe(QStringLiteral("EOP coverage\\s*:\\s*DATA-BASED (\\S+) TO (\\S+)\\. PREDICTS-> (\\S+)"));
	static const QStringList months = QStringLiteral("JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC").split(' ');

	struct Row { int mjd; QString date; QString jd; QString value; };
	QVector<Row> rows;
	QString runDate, eopFile, coverage;
	bool inData = false, sawEnd = false;
	int nIn = 0;
	const QStringList lines = answer.split('\n');
	for (int i = 0; i < lines.size(); ++i)
	{
		const QString line = lines.at(i);
		const QString t = line.trimmed();
		if (t.startsWith("$$SOE"))
		{
			inData = true;
			continue;
		}
		if (t.startsWith("$$EOE"))
		{
			sawEnd = true;
			break;
		}
		if (!inData)
		{
			QRegularExpressionMatch m = runRe.match(line);
			if (m.hasMatch())
			{
				const int mon = months.indexOf(m.captured(1).toUpper()) + 1;
				const QDate d(m.captured(3).toInt(), mon, m.captured(2).toInt());
				if (d.isValid())
					runDate = d.toString("yyyy-MM-dd");
			}
			m = eopFileRe.match(line);
			if (m.hasMatch())
				eopFile = m.captured(1);
			m = coverageRe.match(line);
			if (m.hasMatch())
				coverage = QString("DATA-BASED %1 TO %2. PREDICTS-> %3").arg(m.captured(1), m.captured(2), m.captured(3));
			continue;
		}
		if (t.isEmpty())
			continue;
		const QRegularExpressionMatch m = rowRe.match(line);
		if (!m.hasMatch())
			return fail(QString("row not understood: %1").arg(t.left(80)));
		++nIn;
		const QString jd = m.captured(5);
		const double mjdF = jd.toDouble() - 2400000.5;
		if (std::fabs(mjdF - std::round(mjdF)) > 1e-6)
			return fail(QString("not a 0h UT row: %1").arg(t.left(80)));
		const QString val = m.captured(6);
		if (val.compare("n.a.", Qt::CaseInsensitive) == 0)
			continue;
		bool ok = false;
		val.toDouble(&ok);
		if (!ok)
			return fail(QString("UT1-UTC is not a number: %1").arg(t.left(80)));
		rows.append(Row{int(std::lround(mjdF)), QString("%1-%2-%3").arg(m.captured(1), m.captured(2), m.captured(3)), jd, val});
	}
	if (!inData || !sawEnd)
		return fail(QStringLiteral("no table of UT1-UTC in the answer of Horizons"));
	if (rows.size() < minRows)
		return fail(QString("only %1 days with a UT1-UTC value in the answer of Horizons").arg(rows.size()));

	// Horizons keeps its last prediction constant: keep the rows up to the first day with the last value
	const double last = rows.last().value.toDouble();
	int iLast = rows.size() - 1;
	while (iLast > 0 && rows.at(iLast - 1).value.toDouble() == last && rows.at(iLast - 1).mjd == rows.at(iLast).mjd - 1)
		--iLast;
	rows.resize(iLast + 1);

	csv += "# JPL Horizons UT1-UTC (quantity 49), target Sun, geocentric, UT1-UTC in seconds as printed by Horizons\n";
	if (!runDate.isEmpty())
		csv += "# Horizons run: " + runDate + "\n";
	if (!eopFile.isEmpty())
		csv += "# EOP file used by Horizons: " + eopFile + "\n";
	if (!coverage.isEmpty())
		csv += "# EOP coverage: " + coverage + "\n";
	csv += QString("# First value: %1 (MJD %2), last distinct value: %3 (MJD %4); Horizons keeps the last value constant afterwards\n")
	       .arg(rows.first().date).arg(rows.first().mjd).arg(rows.last().date).arg(rows.last().mjd);
	csv += "# Days before the first value are n.a. at Horizons (UT1-UTC is not defined before 1962-Jan-20)\n";
	csv += "MJD;Date;JD;UT1-UTC\n";
	for (const Row& r : std::as_const(rows))
		csv += QString("%1;%2;%3;%4\n").arg(r.mjd).arg(r.date, r.jd, r.value);
	if (nRows) *nRows = rows.size();
	return true;
}

EOPUpdater::EOPUpdater(QNetworkAccessManager* networkManager, const QString& dir, QObject* parent)
	: QObject(parent)
	, nam(networkManager)
	, dirPath(dir)
{
}

QString EOPUpdater::fileNameFor(const QString& iersFileName, const QDate& date)
{
	const QFileInfo fi(iersFileName);
	const QString suffix = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
	return fi.completeBaseName() + "_" + date.toString("yyMMdd") + suffix;
}

bool EOPUpdater::validateCSV(const QByteArray& data, QString* error, int* nRows)
{
	if (nRows) *nRows = 0;
	const QList<QByteArray> lines = data.split('\n');
	QByteArray header = lines.value(0);
	if (header.startsWith("\xEF\xBB\xBF")) // byte order mark
		header.remove(0, 3);
	if (!header.startsWith("MJD;") || !header.contains("x_pole"))
	{
		if (error) *error = QStringLiteral("the downloaded data is not an IERS EOP CSV file");
		return false;
	}
	int n = 0;
	for (int i = 1; i < lines.size(); ++i)
	{
		const QList<QByteArray> f = lines.at(i).trimmed().split(';');
		bool ok = false;
		if (f.size() >= 23)
		{
			f.at(0).toDouble(&ok);
			if (ok)
				++n;
		}
	}
	if (n == 0)
	{
		if (error) *error = QStringLiteral("the downloaded file has no data row");
		return false;
	}
	if (nRows) *nRows = n;
	return true;
}

void EOPUpdater::start(const QList<Source>& sources, const QDate& today)
{
	if (reply || !nam || sources.isEmpty())
		return;
	queue = sources;
	index = 0;
	aborted = false;
	written.clear();
	failures.clear();
	day = today.isValid() ? today : QDate::currentDate();
	startNext();
}

void EOPUpdater::startNext()
{
	if (aborted || index >= queue.size())
	{
		const QString msg = failures.isEmpty() ? QString("%1 file(s) written").arg(written.size()) : failures.join("; ");
		queue.clear();
		emit finished(failures.isEmpty() && !aborted, written, aborted ? QStringLiteral("aborted") : msg);
		return;
	}

	QNetworkRequest req(queue.at(index).url);
	req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	// (SS) 2026-10-14 setTransferTimeout() requires Qt 5.15; older Qt versions have no timeout.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	req.setTransferTimeout(120000);
#endif
	if (queue.at(index).kind == Source::Kind::HorizonsUT1UTC)
	{
		// (SS) 2026-11-06 same request as: curl -F format=text -F input=@my_input_file.txt https://ssd.jpl.nasa.gov/api/horizons_file.api
		// The answer is a table of about 24000 days: a few seconds.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
		req.setTransferTimeout(180000);
#endif
		QHttpMultiPart* form = new QHttpMultiPart(QHttpMultiPart::FormDataType);
		QHttpPart format;
		format.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QStringLiteral("form-data; name=\"format\"")));
		format.setBody("text");
		form->append(format);
		QHttpPart input;
		input.setHeader(QNetworkRequest::ContentDispositionHeader,
		                QVariant(QStringLiteral("form-data; name=\"input\"; filename=\"my_input_file.txt\"")));
		input.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
		input.setBody(horizonsRequest(day.addDays(150)).toUtf8());     // enough days after the last prediction to see the constant tail
		form->append(input);
		reply = nam->post(req, form);
		form->setParent(reply);       // deleted with the reply
	}
	else
		reply = nam->get(req);
	connect(reply, &QNetworkReply::downloadProgress, this, &EOPUpdater::progress);
	connect(reply, &QNetworkReply::finished, this, &EOPUpdater::onFinished);
}

void EOPUpdater::abort()
{
	aborted = true;
	if (reply)
		reply->abort(); // finished() is emitted, onFinished() ends the queue
}

void EOPUpdater::onFinished()
{
	QNetworkReply* r = reply;
	reply = Q_NULLPTR;
	if (!r)
		return;
	r->deleteLater();

	const Source src = queue.at(index);
	QString message;
	bool ok = false;
	if (r->error() != QNetworkReply::NoError)
	{
		message = QString("%1: download failed (%2)").arg(src.fileName, r->errorString());
		if (src.kind == Source::Kind::HorizonsUT1UTC)
		{
			const QString body = QString::fromUtf8(r->readAll()).simplified().left(200);   // Horizons explains its errors
			if (!body.isEmpty())
				message += " " + body;
		}
	}
	else if (src.kind == Source::Kind::HorizonsUT1UTC)
	{
		// (SS) 2026-11-06 the answer of Horizons is a text table: it becomes the overlay file horizons_ut1utc_YYMMDD.csv
		QString csv, err;
		int rows = 0;
		if (!horizonsToOverlayCSV(QString::fromUtf8(r->readAll()), csv, &err, &rows))
		{
			message = QString("%1: %2").arg(src.fileName, err);
		}
		else
		{
			const QByteArray content = csv.toUtf8();
			QDir().mkpath(dirPath);
			const QString name = fileNameFor(src.fileName, day);
			QSaveFile f(QDir(dirPath).filePath(name));
			if (!f.open(QIODevice::WriteOnly) || f.write(content) != content.size() || !f.commit())
			{
				message = QString("Cannot write %1: %2").arg(name, f.errorString());
			}
			else
			{
				ok = true;
				written << name;
				message = QString("%1: %2 daily records (UT1-UTC of JPL Horizons)").arg(name).arg(rows);
			}
		}
	}
	else
	{
		const QByteArray content = r->readAll();
		int rows = 0;
		QString err;
		if (!validateCSV(content, &err, &rows))
		{
			message = QString("%1: %2").arg(src.fileName, err);
		}
		else
		{
			QDir().mkpath(dirPath);
			const QString name = fileNameFor(src.fileName, day);
			QSaveFile f(QDir(dirPath).filePath(name));
			if (!f.open(QIODevice::WriteOnly) || f.write(content) != content.size() || !f.commit())
			{
				message = QString("Cannot write %1: %2").arg(name, f.errorString());
			}
			else
			{
				ok = true;
				written << name;
				message = QString("%1: %2 daily records").arg(name).arg(rows);
			}
		}
	}
	if (!ok)
		failures << message;
	emit fileFinished(src.fileName, ok, message);

	++index;
	startNext();
}
