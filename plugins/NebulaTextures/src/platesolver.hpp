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
#ifndef PLATESOLVER_HPP
#define PLATESOLVER_HPP

#include "StelTranslator.hpp"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>

class PlateSolver : public QObject
{
	Q_OBJECT

public:
	explicit PlateSolver(QObject* parent = nullptr);
	void startPlateSolving(const QString& apiKey, const QString& imagePath);
	void cancel();

signals:
	void loginSuccess();
	void loginFailed(const QString& reason);
	void uploadSuccess();
	void uploadFailed(const QString& reason);
	void solvingStatusUpdated(const QString& status);
	void solutionAvailable(const QString& wcsText);
	void finished();
	void failed(const QString& reason);

private slots:
	void onLoginReply();
	void onUploadReply();
	void onSubStatusReply();
	void onJobStatusReply();
	void onWcsDownloadReply();

private:
	void sendLoginRequest();
	void sendUploadRequest();
	void sendSubStatusRequest();
	void sendJobStatusRequest();
	void downloadWcsFile();

	QString apiKey;
	QString imagePath;
	QString session;
	QString subId;
	QString jobId;

	int retryCount;
	const int maxRetryCount = 99;

	QNetworkAccessManager* networkManager;
	QList<QNetworkReply*> activeReplies;
	QTimer* subStatusTimer;
	QTimer* jobStatusTimer;
};

#endif // PLATESOLVER_HPP
