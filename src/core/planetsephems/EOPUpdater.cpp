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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

QList<EOPUpdater::Source> EOPUpdater::defaultSources()
{
	// IAU 1980 set (JPL Horizons): standard series finals.all and long term EOP 14 C04 (dPsi, dEps relative to IAU 1980)
	// IAU 2000A set: standard series finals2000A.all and long term EOP 20 C04 u24 (dX, dY)
	// All four files are in the same folder of the IERS data center (links given by the author, 2026-10-07)
	const QString data = QStringLiteral("https://datacenter.iers.org/data/csv/");
	QList<Source> list;
	list << Source{"finals.all.csv", QUrl(data + "finals.all.csv"),
	               QStringLiteral("Standard series, IAU 1980 (dPsi, dEps), with predictions"), true, 7};
	list << Source{"eopc04_14.62-now.csv", QUrl(data + "eopc04_14.62-now.csv"),
	               QStringLiteral("Long term series EOP 14 C04, IAU 1980 (dPsi, dEps)"), true, 90};   // no longer updated since January 2026
	list << Source{"finals2000A.all.csv", QUrl(data + "finals2000A.all.csv"),
	               QStringLiteral("Standard series, IAU 2000A (dX, dY), with predictions"), true, 7};
	list << Source{"eopc04_20u24.1962-now.csv", QUrl(data + "eopc04_20u24.1962-now.csv"),
	               QStringLiteral("Long term series EOP 20 C04, IAU 2000A (dX, dY)"), true, 14};
	return list;
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
	req.setTransferTimeout(120000);
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
