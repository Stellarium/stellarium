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

//! Holds WCS (World Coordinate System) solution data parsed from FITS header or Astrometry API result.
struct WcsResult {
	double CRPIX1, CRPIX2;  //!< Reference pixel coordinates
	double CRVAL1, CRVAL2;  //!< Reference sky coordinates (RA/Dec)
	double CD1_1, CD1_2;    //!< CD matrix coefficients (RA)
	double CD2_1, CD2_2;    //!< CD matrix coefficients (Dec)
	int IMAGEW, IMAGEH;     //!< Image width and height
	bool valid = false;     //!< True if the result is valid
};

//! Handles online plate-solving using the Astrometry.net API.
class PlateSolver : public QObject
{
	Q_OBJECT

public:
	//! Constructor with optional parent.
	explicit PlateSolver(QObject* parent = nullptr);

	//! Parse WCS text content into structured WcsResult.
	static WcsResult parseWcsText(const QString& wcsText);

	//! Start solving the given image with API key.
	void startPlateSolving(const QString& apiKey, const QString& imagePath);

	//! Cancel the current solving process.
	void cancel();

signals:
	//! Emitted when login to Astrometry.net is successful.
	void loginSuccess();

	//! Emitted when login fails.
	void loginFailed(const QString& reason);

	//! Emitted when image upload is successful.
	void uploadSuccess();

	//! Emitted when image upload fails.
	void uploadFailed(const QString& reason);

	//! Emitted when solving status is updated.
	void solvingStatusUpdated(const QString& status);

	//! Emitted when a WCS solution is available.
	void solutionAvailable(const QString& wcsText);

	//! Emitted when solving completes successfully.
	void finished();

	//! Emitted when solving fails.
	void failed(const QString& reason);

private slots:
	//! Handle login reply from server.
	void onLoginReply();

	//! Handle image upload reply.
	void onUploadReply();

	//! Handle submission status reply.
	void onSubStatusReply();

	//! Handle job status polling reply.
	void onJobStatusReply();

	//! Handle WCS file download reply.
	void onWcsDownloadReply();

private:
	//! Send login request to Astrometry.net.
	void sendLoginRequest();

	//! Send image upload request.
	void sendUploadRequest();

	//! Check submission status periodically.
	void sendSubStatusRequest();

	//! Check solving job status periodically.
	void sendJobStatusRequest();

	//! Download WCS result file from server.
	void downloadWcsFile();

	// --- Internal state ---
	QString apiKey;     //!< User API key
	QString imagePath;  //!< Path to the image to solve
	QString session;    //!< Session ID after login
	QString subId;      //!< Submission ID
	QString jobId;      //!< Job ID

	int retryCount;           //!< Current retry attempt
	const int maxRetryCount = 99; //!< Max retries allowed

	QNetworkAccessManager* networkManager; //!< Used for all network requests
	QList<QNetworkReply*> activeReplies;   //!< Tracks active network replies
	QTimer* subStatusTimer;                //!< Timer for checking submission status
	QTimer* jobStatusTimer;                //!< Timer for job status polling
};

#endif // PLATESOLVER_HPP
