/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024-2025 WANG Siliang
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PlateSolver.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QDebug>

#define API_URL "http://nova.astrometry.net/"

/**
 * @brief Constructs a PlateSolver instance.
 *
 * Initializes the network manager, timers, and connects internal slots
 * for polling submission and job status from astrometry.net API.
 *
 * @param parent Optional QObject parent.
 */
PlateSolver::PlateSolver(QObject* parent)
	: QObject(parent),
	retryCount(0),
	networkManager(new QNetworkAccessManager(this)),
	subStatusTimer(new QTimer(this)),
	jobStatusTimer(new QTimer(this))
{
	connect(subStatusTimer, &QTimer::timeout, this, &PlateSolver::sendSubStatusRequest);
	connect(jobStatusTimer, &QTimer::timeout, this, &PlateSolver::sendJobStatusRequest);
}

/**
 * @brief Parses a WCS (World Coordinate System) text block into structured data.
 *
 * Extracts CRPIX, CRVAL, CD matrix, and image size parameters from the raw WCS string.
 *
 * @param wcsText Raw WCS text (typically from downloaded `.wcs` file).
 * @return WcsResult Struct containing parsed values and a validity flag.
 */
WcsResult PlateSolver::parseWcsText(const QString& wcsText)
{
	WcsResult result;
	result.valid = false;

	QRegularExpression regex(R"((CRPIX1|CRPIX2|CRVAL1|CRVAL2|CD1_1|CD1_2|CD2_1|CD2_2|IMAGEW|IMAGEH)\s*=\s*(-?\d+(\.\d+)?([eE][-+]?\d+)?))");
	QStringList lines;
	for (int i = 0; i < wcsText.length(); i += 80)
		lines.append(wcsText.mid(i, 80));

	for (const QString& line : lines)
	{
		auto match = regex.match(line);
		if (match.hasMatch())
		{
			QString key = match.captured(1);
			double value = match.captured(2).toDouble();

			if (key == "CRPIX1") result.CRPIX1 = value;
			else if (key == "CRPIX2") result.CRPIX2 = value;
			else if (key == "CRVAL1") result.CRVAL1 = value;
			else if (key == "CRVAL2") result.CRVAL2 = value;
			else if (key == "CD1_1")  result.CD1_1  = value;
			else if (key == "CD1_2")  result.CD1_2  = value;
			else if (key == "CD2_1")  result.CD2_1  = value;
			else if (key == "CD2_2")  result.CD2_2  = value;
			else if (key == "IMAGEW") result.IMAGEW = value;
			else if (key == "IMAGEH") result.IMAGEH = value;
		}
	}

	result.valid = true;
	return result;
}

/**
 * @brief Starts the plate solving process using an API key and a local image file.
 *
 * Sends a login request, then proceeds to image upload and status polling.
 *
 * @param _apiKey Astrometry.net API key.
 * @param _imagePath Path to the image file to be solved.
 */
void PlateSolver::startPlateSolving(const QString& _apiKey, const QString& _imagePath)
{
	apiKey = _apiKey;
	imagePath = _imagePath;
	retryCount = 0;
	sendLoginRequest();
}

/**
 * @brief Cancels any ongoing network requests and status polling timers.
 */
void PlateSolver::cancel()
{
	for (QNetworkReply* reply : activeReplies)
	{
		reply->abort();
		reply->deleteLater();
	}
	activeReplies.clear();
	subStatusTimer->stop();
	jobStatusTimer->stop();
	retryCount = 0;
}

/**
 * @brief Sends the login request to astrometry.net using the API key.
 */
void PlateSolver::sendLoginRequest()
{
	QJsonObject json;
	json["apikey"] = apiKey;
	QByteArray jsonData = QJsonDocument(json).toJson();

	QUrl url(API_URL "api/login");
	QUrlQuery query;
	query.addQueryItem("request-json", QString(jsonData));
	url.setQuery(query);

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

	QNetworkReply* reply = networkManager->post(request, query.toString().toUtf8());
	activeReplies.append(reply);
	connect(reply, &QNetworkReply::finished, this, &PlateSolver::onLoginReply);

	emit solvingStatusUpdated(q_("Sending login request..."));
}

/**
 * @brief Handles the reply for the login request.
 *
 * Extracts the session ID or emits an error signal if login fails.
 */
void PlateSolver::onLoginReply()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	activeReplies.removeOne(reply);
	QByteArray content = reply->readAll();
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit loginFailed(q_("Network reply error."));
		return;
	}

	QJsonObject json = QJsonDocument::fromJson(content).object();
	if (!json.contains("session") || json["status"].toString() == "error")
	{
		emit loginFailed(q_("Invalid API key or login error."));
		return;
	}

	session = json["session"].toString();
	emit loginSuccess();

	sendUploadRequest();
}

/**
 * @brief Uploads the image and JSON parameters to astrometry.net.
 */
void PlateSolver::sendUploadRequest()
{
	QFile imageFile(imagePath);
	if (!imageFile.open(QIODevice::ReadOnly))
	{
		emit uploadFailed(q_("Failed to open image file."));
		return;
	}

	QJsonObject uploadJson;
	uploadJson["session"] = session;
	uploadJson["allow_commercial_use"] = "n";
	uploadJson["allow_modifications"] = "n";
	uploadJson["publicly_visible"] = "n";
	uploadJson["tweak_order"] = 0;
	uploadJson["crpix_center"] = true;

	QString boundary_key;
	for (int i = 0; i < 19; ++i)
		boundary_key += QString::number(QRandomGenerator::global()->bounded(10));

	QByteArray boundary = "===============" + boundary_key.toUtf8() + "==";
	QByteArray contentType = "multipart/form-data; boundary=\"" + boundary + "\"";

	QByteArray body;
	body.append("--" + boundary + "\r\n"
				"Content-Type: text/plain\r\n"
				"MIME-Version: 1.0\r\n"
				"Content-disposition: form-data; name=\"request-json\"\r\n\r\n");
	body.append(QJsonDocument(uploadJson).toJson() + "\r\n");

	body.append("--" + boundary + "\r\n"
				"Content-Type: application/octet-stream\r\n"
				"MIME-Version: 1.0\r\n"
				"Content-disposition: form-data; name=\"file\"; filename=\"" + QFileInfo(imageFile.fileName()).fileName().toUtf8() + "\"\r\n\r\n");
	body.append(imageFile.readAll() + "\r\n");
	body.append("--" + boundary + "--\r\n");

	QNetworkRequest request(QUrl(API_URL "api/upload"));
	request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
	request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(body.size()));

	QNetworkReply* reply = networkManager->post(request, body);
	activeReplies.append(reply);
	connect(reply, &QNetworkReply::finished, this, &PlateSolver::onUploadReply);

	emit solvingStatusUpdated(q_("Uploading image..."));
}

/**
 * @brief Handles the reply after the image upload.
 *
 * Starts polling submission status if successful.
 */
void PlateSolver::onUploadReply()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	activeReplies.removeOne(reply);
	QByteArray content = reply->readAll();
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit uploadFailed(q_("Network reply error."));
		return;
	}

	QJsonObject json = QJsonDocument::fromJson(content).object();
	subId = QString::number(json["subid"].toInt());

	subStatusTimer->start(5000);
	emit uploadSuccess();
}

/**
 * @brief Sends a request to check the submission status using the submission ID.
 *
 * If a job ID is returned, it stops the submission polling timer and starts the job polling timer.
 * Emits a status update with retry count.
 */
void PlateSolver::sendSubStatusRequest()
{
	QNetworkRequest request(QUrl(API_URL "api/submissions/" + subId));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply* reply = networkManager->get(request);
	activeReplies.append(reply);
	connect(reply, &QNetworkReply::finished, this, &PlateSolver::onSubStatusReply);

	QTimer::singleShot(5000, reply, [reply]() {
		if (reply && reply->isRunning()) {
			qDebug() << "[NebulaTextures] Submission status request timed out, aborting...";
			reply->abort();
		}
	});

	emit solvingStatusUpdated(q_("Checking submission status (%1/%2)...").arg(retryCount + 1).arg(maxRetryCount));
}

/**
 * @brief Handles the reply from the submission status request.
 *
 * If the job ID is received, begins polling the job status.
 * Stops retrying if an error is returned or retries exceed the limit.
 */
void PlateSolver::onSubStatusReply()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	activeReplies.removeOne(reply);
	QByteArray content = reply->readAll();
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		if (++retryCount >= maxRetryCount)
		{
			subStatusTimer->stop();
			emit failed(q_("Submission status request failed after retries."));
		}
		return;
	}

	QJsonObject json = QJsonDocument::fromJson(content).object();
	QJsonArray jobs = json["jobs"].toArray();
	if (!jobs.isEmpty() && !jobs[0].isNull())
	{
		jobId = QString::number(jobs[0].toInt());
		subStatusTimer->stop();
		jobStatusTimer->start(5000);
		retryCount = 0;
		emit solvingStatusUpdated(q_("Job ID received. Processing..."));
	} else if (json.contains("error_message")) {
		subStatusTimer->stop();
		emit failed(q_("Astrometry error. %1").arg(json["error_message"].toString()));
	}
}

/**
 * @brief Sends a request to check the status of the current solving job.
 *
 * Uses the job ID obtained from the previous stage.
 * Emits a status update with retry count.
 */
void PlateSolver::sendJobStatusRequest()
{
	QNetworkRequest request(QUrl(API_URL "api/jobs/" + jobId));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply* reply = networkManager->get(request);
	activeReplies.append(reply);
	connect(reply, &QNetworkReply::finished, this, &PlateSolver::onJobStatusReply);

	QTimer::singleShot(5000, reply, [reply]() {
		if (reply && reply->isRunning()) {
			qDebug() << "[NebulaTextures] Job status request timed out, aborting...";
			reply->abort();
		}
	});

	emit solvingStatusUpdated(q_("Checking job status (%1/%2)...").arg(retryCount + 1).arg(maxRetryCount));
}

/**
 * @brief Handles the reply from the job status request.
 *
 * If the job succeeded, it proceeds to download the WCS result.
 * If the job failed or an error occurred, stops polling and emits a failure signal.
 */
void PlateSolver::onJobStatusReply()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	activeReplies.removeOne(reply);
	QByteArray content = reply->readAll();
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		if (++retryCount >= maxRetryCount)
		{
			jobStatusTimer->stop();
			emit failed(q_("Job status request failed after retries."));
		}
		return;
	}

	QJsonObject json = QJsonDocument::fromJson(content).object();
	QString status = json["status"].toString();

	if (status == "success")
	{
		jobStatusTimer->stop();
		retryCount = 0;
		emit solvingStatusUpdated(q_("Job completed. Downloading WCS file..."));
		downloadWcsFile();
	} else if (status == "error" || status == "failure") {
		jobStatusTimer->stop();
		emit failed(q_("Job failed during processing."));
	}
}

/**
 * @brief Initiates the download of the WCS file from the completed job.
 *
 * Uses the job ID to construct the WCS file URL.
 */
void PlateSolver::downloadWcsFile()
{
	QNetworkRequest request(QUrl(API_URL "wcs_file/" + jobId));
	QNetworkReply* reply = networkManager->get(request);
	activeReplies.append(reply);
	connect(reply, &QNetworkReply::finished, this, &PlateSolver::onWcsDownloadReply);
}

/**
 * @brief Handles the downloaded WCS file reply.
 *
 * If the file is successfully retrieved, emits the WCS data and a finished signal.
 * Emits a failure signal if the download fails.
 */
void PlateSolver::onWcsDownloadReply()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	activeReplies.removeOne(reply);
	QByteArray content = reply->readAll();
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit failed(q_("Failed to download WCS file."));
		return;
	}

	QString wcsText = QString::fromUtf8(content);
	emit solutionAvailable(wcsText);
	emit finished();
	reply->deleteLater();
}
