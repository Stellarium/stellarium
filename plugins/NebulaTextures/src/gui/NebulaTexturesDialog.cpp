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

#include "NebulaTextures.hpp"
#include "NebulaTexturesDialog.hpp"
#include "ui_nebulaTexturesDialog.h"

#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
#include "StelMovementMgr.hpp"
#include "StelUtils.hpp"
#include "StelSkyImageTile.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"

#include <QListWidgetItem>
#include <QFileDialog>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStringList>

/*
 * Constructor for the NebulaTexturesDialog class. Initializes the dialog, sets up
 * network management, and configures timers for periodic status checking.
 * Also loads nebula textures on initialization.
 */
NebulaTexturesDialog::NebulaTexturesDialog()
	: StelDialog("NebulaTextures"),
	flag_renderTempTex(false),
	conf(StelApp::getInstance().getSettings()),
	retryCount(0),
	countRefresh(0),
	maxCountRefresh(2),
	progressBar(Q_NULLPTR)
{
	ui = new Ui_nebulaTexturesDialog();

	networkManager = new QNetworkAccessManager(this);

	subStatusTimer = new QTimer(this);
	jobStatusTimer = new QTimer(this);
	connect(subStatusTimer, &QTimer::timeout, this, &NebulaTexturesDialog::checkSubStatus);
	connect(jobStatusTimer, &QTimer::timeout, this, &NebulaTexturesDialog::checkJobStatus);
}

NebulaTexturesDialog::~NebulaTexturesDialog()
{
	for (QNetworkReply *reply : activeReplies) {
		reply->abort();
		reply->deleteLater();
	}
	delete ui;
}


void NebulaTexturesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

/*
 * Set up the content and interactions of the NebulaTexturesDialog.
 * Configure UI elements, establish signal-slot connections,
 * and load necessary settings and data for the dialog.
 */
void NebulaTexturesDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// load config
	ui->checkBoxShow->setChecked(getShowCustomTextures());
	ui->checkBoxAvoid->setChecked(getAvoidAreaConflict());
	ui->cancelButton->setVisible(false);
	ui->label_apiKey->setText(QString("<a href=\"https://nova.astrometry.net/api_help\">") + q_("Astrometry ApiKey:") + "</a>");
	ui->brightComboBox->addItem(q_("Bright"), QVariant(12));
	ui->brightComboBox->addItem(q_("Normal"), QVariant(13.5));
	ui->brightComboBox->addItem(q_("Dark"), QVariant(15));
	ui->brightComboBox->setCurrentIndex(1);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->openFileButton, SIGNAL(clicked()), this, SLOT(openImageFile()));
	connect(ui->uploadImageButton, SIGNAL(clicked()), this, SLOT(uploadImage()));
	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancelSolve()));
	connect(ui->goPushButton, SIGNAL(clicked()), this, SLOT(goPush()));
	connect(ui->renderButton, SIGNAL(clicked()), this, SLOT(toggleTempTextureRendering()));
	connect(ui->disableDefault, SIGNAL(clicked()), this, SLOT(toggleDisableDefaultTexture()));
	connect(ui->brightComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onBrightnessChanged(int)));

	connect(ui->addCustomTextureButton, SIGNAL(clicked()), this, SLOT(addCustomTexture()));
	connect(ui->removeTextureButton, SIGNAL(clicked()), this, SLOT(removeTexture()));

	connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reloadData()));
	// connect(ui->checkBoxShow, SIGNAL(clicked(bool)), this, SLOT(setShowCustomTextures(bool)));
	connectCheckBox(ui->checkBoxShow,"actionShow_NebulaTextures");
	connect(ui->checkBoxAvoid, SIGNAL(clicked(bool)), this, SLOT(setAvoidAreaConflict(bool)));

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(gotoSelectedItem(QListWidgetItem*)));

	setAboutHtml();

	reloadData();

	ui->lineEditApiKey->setText(conf->value(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", "").toString());
}


void NebulaTexturesDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[NebulaTextures] Restore defaults...";
		// GETSTELMODULE(NebulaTextures)->restoreDefaultSettings();

		deleteImagesFromCfg(configFile);
		QString cfgPath = StelFileMgr::getUserDir() + configFile;
		if (QFile::exists(cfgPath)) {
			if (QFile::remove(cfgPath)) {
				qDebug() << "[NebulaTextures] Deleted texture config file:" << cfgPath;
			} else {
				qWarning() << "[NebulaTextures] Failed to delete config file:" << cfgPath;
			}
		}

		deleteImagesFromCfg(tmpcfgFile);
		cfgPath = StelFileMgr::getUserDir() + tmpcfgFile;
		if (QFile::exists(cfgPath)) {
			if (QFile::remove(cfgPath)) {
				qDebug() << "[NebulaTextures] Deleted temporary texture config file:" << cfgPath;
			} else {
				qWarning() << "[NebulaTextures] Failed to delete temporary config file:" << cfgPath;
			}
		}

		setShowCustomTextures(false);
		setAvoidAreaConflict(false);

		ui->lineEditApiKey->setText("");
		conf->setValue(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", "");

		ui->lineEditImagePath->setText("");
		ui->referX->setValue(0);
		ui->referY->setValue(0);

		ui->topLeftX->setValue(0);
		ui->topLeftY->setValue(0);
		ui->bottomLeftX->setValue(0);
		ui->bottomLeftY->setValue(0);
		ui->topRightX->setValue(0);
		ui->topRightY->setValue(0);
		ui->bottomRightX->setValue(0);
		ui->bottomRightY->setValue(0);

		ui->listWidget->clear();
	}
	else
		qDebug() << "[NebulaTextures] Restore defaults is canceled...";
}

/*
 * Set the HTML content for the "About" section in the dialog.
 */
void NebulaTexturesDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";

	html += "<h2>" + q_("Nebula Textures Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>WANG Siliang</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Nebula Textures plugin enhances your Stellarium experience by allowing you to customize and render deep-sky object (DSO) textures. "
					   "Whether you're an astronomy enthusiast with astrophotography or astronomical sketches and paintings, this plugin lets you integrate your own images into Stellarium. "
					   "It also supports online plate-solving via Astrometry.net for precise positioning of astronomical images. "
					   "Simply upload your image (ensure it is not flipped horizontally or vertically), solve its coordinates, and enjoy a personalized celestial view!") + "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Nebula Textures plugin");

	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}


/*
 * Toggle the enabled/disabled state of UI components based on the provided "freeze" flag.
 *
 * @param freeze  A boolean value indicating whether to disable (true) or enable (false) the UI elements.
 */
void NebulaTexturesDialog::freezeUiState(bool freeze)
{
	ui->openFileButton->setDisabled(freeze);
	ui->uploadImageButton->setDisabled(freeze);
	ui->goPushButton->setDisabled(freeze);
	ui->renderButton->setDisabled(freeze);
	ui->addCustomTextureButton->setDisabled(freeze);
	ui->cancelButton->setVisible(freeze);
}

// Update the status text displayed in the UI
void NebulaTexturesDialog::updateStatus(const QString &status)
{
	ui->statusText->setText(status);
}

void NebulaTexturesDialog::refreshInit()
{
	if(countRefresh < maxCountRefresh){
		refreshTextures();
		countRefresh++;
	}
}

/*
 * Open a file dialog to select an image file and update the UI.
 */
void NebulaTexturesDialog::openImageFile()
{
	QString fileName = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Open Image"), QDir::homePath(), tr("Images (*.png *.jpg *.gif *.tif *.tiff *.jpeg)"));
	if (!fileName.isEmpty())
	{
		ui->lineEditImagePath->setText(fileName);
		updateStatus(q_("File selected: ") + fileName);
	}
}

void NebulaTexturesDialog::cancelSolve()
{
	for (QNetworkReply *reply : activeReplies) {
		reply->abort();
		reply->deleteLater();
	}
	activeReplies.clear();
	subStatusTimer->stop();
	jobStatusTimer->stop();
	retryCount = 0;

	updateStatus(q_("Operation cancelled by user."));
	freezeUiState(false);
}


/*
 * Upload an image to the server after validating input.
 * Send the API key and image path via a POST request to the server.
 *
 * Steps:
 *   - Validate the API key and image path.
 *   - Optionally save the API key for future use.
 *   - Create a JSON object with the API key.
 *   - Send the JSON data in a POST request to the server's login API.
 *   - Update the UI to indicate the upload process is in progress.
 */
void NebulaTexturesDialog::uploadImage() // WARN: image should not be flip
{
	QString apiKey = ui->lineEditApiKey->text();
	QString imagePath = ui->lineEditImagePath->text();

	if (imagePath.isEmpty() || apiKey.isEmpty())
	{
		updateStatus(q_("Please provide both API key and image path."));
		return;
	}

	QFileInfo fileInfo(imagePath);
	if (!fileInfo.exists()) {
		updateStatus(q_("The specified image file does not exist."));
		return;
	}
	if (!fileInfo.isReadable()) {
		updateStatus(q_("The specified image file is not readable."));
		return;
	}
	QString suffix = fileInfo.suffix().toLower();
	QStringList allowedSuffixes = {"png", "jpg", "gif", "tif", "tiff", "jpeg"};
	if (!allowedSuffixes.contains(suffix)) {
		updateStatus(q_("Invalid image format. Supported formats: PNG, JPEG, GIF, TIFF."));
		return;
	}

	if (ui->checkBoxKeepApi->isChecked())
		conf->setValue(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", apiKey);

	// Create the JSON object
	QJsonObject json;
	json["apikey"] = apiKey;

	// Convert JSON object to JSON document
	QJsonDocument jsonDoc(json);
	QByteArray jsonData = jsonDoc.toJson();

	// Create the POST request URL
	QUrl url(API_URL+"api/login");

	// Create the form data for x-www-form-urlencoded
	QUrlQuery query;
	query.addQueryItem("request-json", QString(jsonData));

	// Set the URL with query
	url.setQuery(query);

	// Create the POST request
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

	// Send the POST request
	QNetworkReply *loginReply = networkManager->post(request, query.toString().toUtf8());
	activeReplies.append(loginReply);

	// Connect the reply finished signal to a slot
	connect(loginReply, &QNetworkReply::finished, this, [this, loginReply]() {
		onLoginReply(loginReply);
		activeReplies.removeOne(loginReply);
		loginReply->deleteLater();
	});

	// Perform image upload logic here, e.g., using an API call
	updateStatus(q_("Sending requests..."));

	freezeUiState(true);
}

/*
 * Handle the response after a login request.
 * If the login is successful, uploads the selected image file to the server.
 *
 * Steps:
 *   - Read the response content from the login request.
 *   - If the login failed, update the UI and return.
 *   - If the login succeeded, open the selected image file.
 *   - Generate a unique boundary key for multipart form-data.
 *   - Prepare the POST body with the session data and image file.
 *   - Send the upload request to the server with the image and metadata.
 *   - Update the UI to reflect the upload status.
 *
 * @param reply The network reply object containing the server response.
 */
void NebulaTexturesDialog::onLoginReply(QNetworkReply *reply)
{
	QByteArray content = reply->readAll();
	if (reply->error() != QNetworkReply::NoError) {
		qWarning() << "[NebulaTextures] Login failed.";
		updateStatus(q_("Login failed!"));
		freezeUiState(false);
		return;
	}
	updateStatus(q_("Login success..."));

	QFile imageFile(ui->lineEditImagePath->text());
	if (!imageFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[NebulaTextures] Failed to open image file.";
		updateStatus(q_("Failed to open image file!"));
		freezeUiState(false);
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(content);
	QJsonObject json = doc.object();

	if(!json.contains("session") || json["status"].toString() == QString("error")) {
		qWarning() << "[NebulaTextures] Login failed. Please check if your API key is valid.";
		updateStatus(q_("Login failed! Please check if your API key is valid."));
		freezeUiState(false);
		return;
	}

	session = json["session"].toString();

	QUrl uploadUrl(API_URL + "api/upload");
	QNetworkRequest request(uploadUrl);
	QJsonObject uploadJson;
	uploadJson["session"] = session;
	uploadJson["allow_commercial_use"] = "n";
	uploadJson["allow_modifications"] = "n";
	uploadJson["publicly_visible"] = "n";
	uploadJson["tweak_order"] = 0;
	uploadJson["crpix_center"] = true;

	QString boundary_key;
	// Generate 19 random digits (0-9)
	for (int i = 0; i < 19; ++i) {
		boundary_key += QString::number(QRandomGenerator::global()->bounded(10));  // Generates a random digit (0-9)
	}

	QByteArray boundary = "===============" + boundary_key.toUtf8() + "==";

	QByteArray contentType = "multipart/form-data; boundary=\"" + boundary +"\"";
	// QNetworkRequest request;
	request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);

	QByteArray body;
	QByteArray jsonPart = "--" + boundary + "\r\n"
		"Content-Type: text/plain\r\n"
		"MIME-Version: 1.0\r\n"
		"Content-disposition: form-data; name=\"request-json\"\r\n"
		"\r\n" +
		QJsonDocument(uploadJson).toJson() + "\r\n";

	body.append(jsonPart);

	QByteArray filePart = "--" + boundary + "\r\n"
		"Content-Type: application/octet-stream\r\n"
		"MIME-Version: 1.0\r\n"
		"Content-disposition: form-data; name=\"file\"; filename=\"" + imageFile.fileName().toUtf8() + "\"\r\n"
		"\r\n";

	body.append(filePart);
	body.append(imageFile.readAll());
	imageFile.close();
	body.append("\r\n");
	body.append("--" + boundary + "--\r\n");

	request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(body.size()));


	QNetworkReply *uploadReply = networkManager->post(request, body);
	activeReplies.append(uploadReply);
	connect(uploadReply, &QNetworkReply::finished, this, [this, uploadReply]() {
		onUploadReply(uploadReply);
		activeReplies.removeOne(uploadReply);
		uploadReply->deleteLater();
	});

	updateStatus(q_("Uploading image..."));
}


/*
 * Handle the response after an image upload request.
 *
 * Steps:
 *   - Read the response content from the upload request.
 *   - If the upload fails, update the UI with an error message and return.
 *   - If the upload is successful, parse the response JSON to retrieve the sub-id.
 *   - Start a timer to check the upload status after a short delay.
 *   - Update the UI to indicate the upload was successful.
 *
 * @param reply The network reply object containing the server response.
 */
void NebulaTexturesDialog::onUploadReply(QNetworkReply *reply)
{
	QByteArray content = reply->readAll();
	if (reply->error() != QNetworkReply::NoError) {
		qWarning() << "[NebulaTextures] Image upload failed.";
		updateStatus(q_("Image upload failed!"));
		freezeUiState(false);
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(content);
	QJsonObject json = doc.object();
	subId = QString::number(json["subid"].toInt());
	subStatusTimer->start(5000);
	qDebug() << "[NebulaTextures] subid" << subId;//debug
	updateStatus(q_("Image uploaded. Please wait..."));
}


/*
 * Send a request to check the status of a submitted image.
 * It queries the API to retrieve the current status of the image submission.
 *
 * Steps:
 *   - Construct the URL for checking the submission status using the sub-id.
 *   - Create a network request with the appropriate header and URL.
 *   - Send the request and connect to the finished signal to handle the response.
 *   - Update the UI to indicate the submission status is being checked.
 */
void NebulaTexturesDialog::checkSubStatus()
{
	QUrl statusUrl(API_URL + "api/submissions/" + subId);
	QNetworkRequest request(statusUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QNetworkReply *subStatusReply = networkManager->get(request);
	activeReplies.append(subStatusReply);
	connect(subStatusReply, &QNetworkReply::finished, this, [this, subStatusReply]() {
		onsubStatusReply(subStatusReply);
		activeReplies.removeOne(subStatusReply);
		subStatusReply->deleteLater();
	});
	updateStatus(q_("Requesting submission status. Please wait...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
}


/*
 * Handle the response for a submission status request.
 *
 * Steps:
 *   - If the reply has an error:
 *     - Increment the retry counter.
 *     - If retry count exceeds the limit, stop retrying and notify the user.
 *     - Otherwise, notify the user and retry.
 *   - If the reply is successful:
 *     - Reset the retry counter.
 *     - Parse the JSON response and check for a valid job ID.
 *     - If a valid job ID is found:
 *       - Save the job ID, stop the submission timer, and start the job timer.
 *       - Notify the user that the submission ID has been received.
 *   - Delete the reply object to free resources.
 *
 * @param reply The QNetworkReply object containing the submission status response.
 */
void NebulaTexturesDialog::onsubStatusReply(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		qWarning() << "[NebulaTextures] Failed to get submission status.";
		if (retryCount >= maxRetryCount) {
			qWarning() << "[NebulaTextures] Max retry count reached. Aborting.";
			updateStatus(q_("Failed to get submission status after multiple attempts. Please try again later."));
			subStatusTimer->stop();
			freezeUiState(false);
			retryCount = 0;
		} else {
			updateStatus(q_("Failed to get submission status. Retry now...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
			retryCount++;
		}
		return;
	}


	QByteArray cont = reply->readAll();
	QJsonDocument doc = QJsonDocument::fromJson(cont);
	QJsonObject json = doc.object();
	QJsonArray jobs = json["jobs"].toArray();

	if (!jobs.isEmpty() && !jobs[0].isNull()) {
		retryCount = 0;
		jobId = QString::number(jobs[0].toInt());
		subStatusTimer->stop();
		jobStatusTimer->start(5000);
		updateStatus(q_("Job id got. Please wait..."));
	}
	else if (json.contains("error_message")) {
		retryCount = 0;
		qWarning() << "[NebulaTextures] Error message received:" << json["error_message"].toString();
		updateStatus(q_("Error occurred!"));
		subStatusTimer->stop();
		freezeUiState(false);
	}
	else {
		if (retryCount >= maxRetryCount) {
			retryCount = 0;
			qWarning() << "[NebulaTextures] Max retry count reached. Aborting.";
			updateStatus(q_("Failed to get job id after multiple attempts. Please try again later."));
			subStatusTimer->stop();
			freezeUiState(false);
		} else {
			updateStatus(q_("Requesting job id. Please wait...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
			retryCount++;
		}
	}
}


/*
 * Send a request to check the status of the current job.
 * It queries the API to retrieve the processing status of the job.
 *
 * Steps:
 *   - Construct the URL using the job ID.
 *   - Create a network request with the correct header and URL.
 *   - Send the GET request and connect to the finished signal to process the response.
 *   - Update the UI to indicate the job status is being checked.
 */
void NebulaTexturesDialog::checkJobStatus()
{
	QUrl jobStatusUrl(API_URL + "api/jobs/" + jobId);
	QNetworkRequest request(jobStatusUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QNetworkReply *jobStatusReply = networkManager->get(request);
	activeReplies.append(jobStatusReply);
	connect(jobStatusReply, &QNetworkReply::finished, this, [this, jobStatusReply]() {
		onJobStatusReply(jobStatusReply);
		activeReplies.removeOne(jobStatusReply);
		jobStatusReply->deleteLater();
	});
	updateStatus(q_("Requesting job status. Please wait...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
}


/*
 * Handle the API response for checking the job status.
 *
 * Steps:
 *   - Check for errors in the network reply. If any, update the status and stop processing.
 *   - Parse the JSON response to extract the job status.
 *   - If the status is "success":
 *       - Notify the user about the job completion.
 *       - Stop the job status timer.
 *       - Initiate a request to download the WCS file.
 *   - If the status is "error":
 *       - Notify the user about the error.
 *       - Stop the job status timer and reset the UI state.
 *   - Clean up the reply object.
 *
 * @param reply The QNetworkReply object containing the job status response.
 */
void NebulaTexturesDialog::onJobStatusReply(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		qWarning() << "[NebulaTextures] Failed to get job status.";
		if (retryCount >= maxRetryCount) {
			qWarning() << "[NebulaTextures] Max retry count reached. Aborting.";
			updateStatus(q_("Failed to get job status after multiple attempts. Please try again later."));
			jobStatusTimer->stop(); // Stop the job status timer
			freezeUiState(false); // Reset the UI state
			retryCount = 0; // Reset the retry counter
		} else {
			updateStatus(q_("Failed to get job status. Retry now...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
			retryCount++; // Increment the retry counter
		}
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject json = doc.object();
	QString status = json["status"].toString();

	if (status == "success") {
		retryCount = 0;
		updateStatus(q_("Job completed successfully! Downloading WCS file..."));
		jobStatusTimer->stop();
		QUrl wcsUrl(API_URL+ "wcs_file/" + jobId);
		QNetworkRequest request(wcsUrl);
		QNetworkReply *wcsReply = networkManager->get(request);
		activeReplies.append(wcsReply);
		connect(wcsReply, &QNetworkReply::finished, this, [this, wcsReply]() {
			onWcsDownloadReply(wcsReply);
			activeReplies.removeOne(wcsReply);
			wcsReply->deleteLater();
		});
	} else if (status == "error" || status == "failure") {
		retryCount = 0;
		qWarning() << "[NebulaTextures] Error in job processing.";
		updateStatus(q_("Error in job processing!"));
		jobStatusTimer->stop();
		freezeUiState(false);
	} else {
		if (retryCount >= maxRetryCount) {
			retryCount = 0;
			qWarning() << "[NebulaTextures] Max retry count reached. Aborting.";
			updateStatus(q_("Failed to get job result after multiple attempts. Please try again later."));
			jobStatusTimer->stop();
			freezeUiState(false);
		} else {
			updateStatus(q_("Requesting job result. Please wait...") + QString(" (%1: %2/%3)").arg(q_("Retry")).arg(retryCount + 1).arg(maxRetryCount));
			retryCount++;
		}
	}
}


/*
 * Handle the response for downloading the WCS file and process coordinates calculating.
 *
 * Steps:
 *   - Check for errors in the network reply. If any, update the status, reset the UI state, and stop processing.
 *   - Parse the WCS file content into lines and extract key parameters using a regular expression.
 *   - Map the extracted WCS parameters (e.g., CRPIX1, CRPIX2, CRVAL1, CRVAL2, etc.) to corresponding member variables.
 *   - Use the WCS parameters to calculate celestial coordinates (RA, Dec) for the image corners:
 *       - Top-left
 *       - Bottom-left
 *       - Top-right
 *       - Bottom-right
 *   - Update the UI fields with the calculated coordinates for reference and display purposes.
 *   - Notify the user about the successful completion of processing and prompt them to proceed with rendering or saving the result.
 *
 * @param reply The QNetworkReply object containing the wcs downloading response.
 */
void NebulaTexturesDialog::onWcsDownloadReply(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		qWarning() << "[NebulaTextures] Failed to download WCS file.";
		updateStatus(q_("Failed to download WCS file..."));
		freezeUiState(false);
		return;
	}
	QByteArray cont = reply->readAll();
	QString content = QString::fromUtf8(cont);

	QRegularExpression regex("(CRPIX1|CRPIX2|CRVAL1|CRVAL2|CD1_1|CD1_2|CD2_1|CD2_2|IMAGEW|IMAGEH)\\s*=\\s*(-?\\d*\\.?\\d+([eE][-+]?\\d+)?)(.*)");
	QStringList lines;
	for (int i = 0; i < content.length(); i += 80) {
		lines.append(content.mid(i, 80));
	}

	for (const QString &line : lines) {
		QRegularExpressionMatch match = regex.match(line);
		if (match.hasMatch()) {
			QString key = match.captured(1);
			double value = match.captured(2).toDouble();
			if (key == "CRPIX1") {
				CRPIX1 = value;
			} else if (key == "CRPIX2") {
				CRPIX2 = value;
			} else if (key == "CRVAL1") {
				CRVAL1 = value;
			} else if (key == "CRVAL2") {
				CRVAL2 = value;
			} else if (key == "CD1_1") {
				CD1_1 = value;
			} else if (key == "CD1_2") {
				CD1_2 = value;
			} else if (key == "CD2_1") {
				CD2_1 = value;
			} else if (key == "CD2_2") {
				CD2_2 = value;
			} else if (key == "IMAGEW") {
				IMAGEW = value;
			} else if (key == "IMAGEH") {
				IMAGEH = value;
			}
		}
	}

	referRA = CRVAL1;
	referDec = CRVAL2;
	ui->referX->setValue(CRVAL1);
	ui->referY->setValue(CRVAL2);

	QPair<double, double> result;
	int X=0,Y=0;
	result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	topLeftRA = result.first;
	topLeftDec = result.second;
	ui->topLeftX->setValue(topLeftRA);
	ui->topLeftY->setValue(topLeftDec);

	X = 0, Y = IMAGEH - 1;
	result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	bottomLeftRA = result.first;
	bottomLeftDec = result.second;
	ui->bottomLeftX->setValue(bottomLeftRA);
	ui->bottomLeftY->setValue(bottomLeftDec);

	X = IMAGEW - 1, Y = 0;
	result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	topRightRA = result.first;
	topRightDec = result.second;
	ui->topRightX->setValue(topRightRA);
	ui->topRightY->setValue(topRightDec);

	X = IMAGEW - 1, Y = IMAGEH - 1;
	result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	bottomRightRA = result.first;
	bottomRightDec = result.second;
	ui->bottomRightX->setValue(bottomRightRA);
	ui->bottomRightY->setValue(bottomRightDec);

	freezeUiState(false);
	updateStatus(q_("Processing completed! Goto Center Point, Try to Render, Check and Add to Local Storage."));
}




/*
 * Convert pixel coordinates (X, Y) on an image to celestial coordinates (RA, Dec) using the World Coordinate System (WCS) parameters.
 * Apply a series of transformations to convert pixel-based coordinates into the corresponding celestial coordinates,
 * taking into account the reference coordinates, rotation matrix, and other transformation parameters.
 *
 * Parameters:
 *   - X, Y: The pixel coordinates in the image.
 *   - CRPIX1, CRPIX2: The reference pixel coordinates (the center of the image).
 *   - CRVAL1, CRVAL2: The celestial coordinates (right ascension and declination) corresponding to the reference pixel.
 *   - CD1_1, CD1_2, CD2_1, CD2_2: The linear transformation matrix elements that map pixel coordinates to celestial coordinates.
 *
 * Returns:
 *   - A QPair containing the longitude and latitude corresponding to the input pixel coordinates.
 *
 * * * * * * * * * * * * * * * * * * * *
 *
 * This function is based on the algorithm and approach from the code licensed under the Apache License, Version 2.0.
 * The original code can be found at:
 * https://github.com/PlanetaryResources/NTL-Asteroid-Data-Hunter/blob/master/Algorithms/Algorithm%20%231/alg1_psyho.h
 *
 * Copyright (C) [Year] [Original Authors].
 *
 * The function implementation is rewritten based on the logic and approach of the original code,
 * but the code itself has been modified and is not directly copied.
 *
 * The code in this function is used under the terms of the Apache License, Version 2.0.
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * If you modified this code, include a description of the changes made below:
 * [Optional: Describe any modifications made]
 */
QPair<double, double> NebulaTexturesDialog::PixelToCelestial(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
						double CD1_1, double CD1_2, double CD2_1, double CD2_2){
	// Constants
	const double D2R = M_PI / 180.0;  // Degree to radian
	const double R2D = 180.0 / M_PI;  // Radian to degree

	// Convert the reference values (RA, Dec) to radians
	double RA0 = CRVAL1 * D2R;
	double Dec0 = CRVAL2 * D2R;

	// Calculate dx and dy
	double dx = X - CRPIX1;
	double dy = Y - CRPIX2;

	// Calculate xx and yy
	double xx = CD1_1 * dx + CD1_2 * dy;
	double yy = CD2_1 * dx + CD2_2 * dy;

	// Calculate the celestial coordinates
	double px = std::atan2(xx, -yy) * R2D;
	double py = std::atan2(R2D, std::sqrt(xx * xx + yy * yy)) * R2D;

	// Calculate sin(Dec) and cos(Dec)
	double sin_dec = std::sin(D2R * (90.0 - CRVAL2));
	double cos_dec = std::cos(D2R * (90.0 - CRVAL2));

	// Calculate longitude offset (dphi)
	double dphi = px - 180.0;

	// Calculate celestial longitude and latitude
	double sinthe = std::sin(py * D2R);
	double costhe = std::cos(py * D2R);
	double costhe3 = costhe * cos_dec;
	double costhe4 = costhe * sin_dec;
	double sinthe3 = sinthe * cos_dec;
	double sinthe4 = sinthe * sin_dec;
	double sinphi = std::sin(dphi * D2R);
	double cosphi = std::cos(dphi * D2R);

	// Calculate celestial longitude
	double x = sinthe4 - costhe3 * cosphi;
	double y = -costhe * sinphi;
	double dlng = R2D * std::atan2(y, x);
	double lng = CRVAL1 + dlng;

	// Normalize the celestial longitude
	if (CRVAL1 >= 0.0) {
		if (lng < 0.0) {
			lng += 360.0;
		}
	} else {
		if (lng > 0.0) {
			lng -= 360.0;
		}
	}

	lng = std::fmod(lng, 360.0);
	if (lng < 0.0) {
		lng += 360.0;
	}

	// Calculate celestial latitude
	double z = sinthe3 + costhe4 * cosphi;
	double lat;
	if (std::abs(z) > 0.99) {
		// For higher precision, use an alternative formula
		if (z < 0.0) {
			lat = -std::abs(R2D * std::acos(std::sqrt(x * x + y * y)));
		} else {
			lat = std::abs(R2D * std::acos(std::sqrt(x * x + y * y)));
		}
	} else {
		lat = R2D * std::asin(z);
	}

	// Return the result as a QPair of doubles
	return QPair<double, double>(lng, lat);
}


/*
 * Goto the center coordinates (RA and Dec) of texture in view.
 *
 * Steps:
 *   - Convert the reference right ascension and declination (referRA, referDec) into radians for spherical calculations.
 *   - Set the view up vector to a stable direction (J2000 coordinate system).
 *   - Adjust the view up vector based on the equatorial mount mode and latitude for stability.
 *   - Call the movement manager to move the view to the target coordinates, ensuring smooth transition with auto-duration.
 */
void NebulaTexturesDialog::goPush()
{
	referRA = ui->referX->value();
	referDec = ui->referY->value();
	StelCore* core = StelApp::getInstance().getCore();
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Vec3d pos;
	double spinLong=referRA/180.*M_PI;
	double spinLat=referDec/180.*M_PI;

	mvmgr->setViewUpVector(Vec3d(0., 0., 1.));
	Vec3d aimUp = mvmgr->getViewUpVectorJ2000();
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();

	StelUtils::spheToRect(spinLong, spinLat, pos);
	if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat)> (0.9*M_PI_2)) )
	{
		// make up vector more stable.
		// Strictly mount should be in a new J2000 mode, but this here also stabilizes searching J2000 coordinates.
		mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat>0. ? 1. : -1. ));
		aimUp=mvmgr->getViewUpVectorJ2000();
	}
	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, aimUp, mvmgr->getAutoMoveDuration());
	// mvmgr->setFlagLockEquPos(useLockPosition);
}

// Toggle the rendering state of the temporary texture
void NebulaTexturesDialog::toggleTempTextureRendering()
{
	if (flag_renderTempTex) {
		unRenderTempCustomTexture();
	} else {
		renderTempCustomTexture();
	}
	ui->renderButton->setChecked(flag_renderTempTex);

	if(flag_renderTempTex)
		ui->renderButton->setText(q_("Stop test"));
	else
		ui->renderButton->setText(q_("Test this texture"));
}

// Toggle the visibility of the default texture when testing temp texture rendering
void NebulaTexturesDialog::toggleDisableDefaultTexture()
{
	if(!flag_renderTempTex)
		return;
	if(ui->disableDefault->isChecked())
		setTexturesVisible(DEFAULT_TEXNAME, false);
	else{
		setTexturesVisible(DEFAULT_TEXNAME, true);
		refreshTextures();
	}
}

// Redo rendering when toggling Brightness Change
void NebulaTexturesDialog::onBrightnessChanged(int index)
{
	if(flag_renderTempTex)
		renderTempCustomTexture();
}

/*
 * Render temporary custom texture for the nebula based on the user-provided image path.
 * Check if the path is valid, add the texture to the system, and insert it into the sky layer.
 * Update the UI status on success or failure and optionally hide the default texture if enabled.
 *
 * Steps:
 *   - Validate image path.
 *   - Render the custom texture by adding it to the sky layer.
 *   - Update UI status and manage default texture visibility based on user preferences.
 */
void NebulaTexturesDialog::renderTempCustomTexture()
{
	QString imagePath = ui->lineEditImagePath->text();
	if (imagePath.isEmpty()) {
		qWarning() << "[NebulaTextures] Image path is empty.";
		updateStatus(q_("Image path is empty."));
		return;
	}

	QFileInfo fileInfo(imagePath);
	if (!fileInfo.exists()) {
		updateStatus(q_("The specified image file does not exist."));
		return;
	}
	if (!fileInfo.isReadable()) {
		updateStatus(q_("The specified image file is not readable."));
		return;
	}
	QString suffix = fileInfo.suffix().toLower();
	QStringList allowedSuffixes = {"png", "jpg", "gif", "tif", "tiff", "jpeg"};
	if (!allowedSuffixes.contains(suffix)) {
		updateStatus(q_("Invalid image format. Supported formats: PNG, JPEG, GIF, TIFF."));
		return;
	}

	// updateStatus(q_("Rendering..."));

	QString path = StelFileMgr::getUserDir()+tmpcfgFile;

	deleteImagesFromCfg(tmpcfgFile);

	addTexture(tmpcfgFile, TEST_TEXNAME);

	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);

	if (path.isEmpty()){
		qWarning() << "[NebulaTextures] Error while loading nebula texture.";
		// updateStatus(q_("Rendering failed."));
	}
	else{
		if(flag_renderTempTex){
			StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
			skyLayerMgr->removeSkyLayer(TEST_TEXNAME);
		}

		skyLayerMgr->insertSkyImage(path, QString(), true, 1);
		flag_renderTempTex = true;

		if(ui->disableDefault->isChecked())
			setTexturesVisible(DEFAULT_TEXNAME, false);
	}
}


/*
 * Cancel temporary custom texture rendering and restore the default texture.
 *
 * Steps:
 *   - Remove the temporary texture from the sky layer.
 *   - Restore the default texture and reload all textures.
 *   - Update the UI status to reflect the cancellation.
 */
void NebulaTexturesDialog::unRenderTempCustomTexture()
{
	if(!flag_renderTempTex)
		return;
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	skyLayerMgr->removeSkyLayer(TEST_TEXNAME);

	flag_renderTempTex = false;
	setTexturesVisible(DEFAULT_TEXNAME, true);
	refreshTextures();
}


/*
 * Delete the images listed in the given configuration file.
 *
 * Steps:
 *   - Open the given configuration file and read the JSON data.
 *   - Loop through each subTile to extract the "imageUrl".
 *   - Delete the file corresponding to each "imageUrl" if it exists.
 *   - Log the process and update the UI.
 *
 * @param cfgFile The path to the configuration file.
 */
void NebulaTexturesDialog::deleteImagesFromCfg(const QString& cfgFile)
{
	QString cfgFilePath = StelFileMgr::getUserDir() + cfgFile;

	QFile jsonFile(cfgFilePath);

	if (!jsonFile.exists()) {
		qWarning() << "[NebulaTextures] JSON file does not exist:" << cfgFilePath;
		ui->listWidget->clear();
		return;
	}

	if (!jsonFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[NebulaTextures] Failed to open JSON file for reading:" << cfgFilePath;
		// updateStatus(q_("Failed to open Configuration File!"));
		return;
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
	jsonFile.close();

	if (!jsonDoc.isObject()) {
		qWarning() << "[NebulaTextures] Invalid JSON structure in file:" << cfgFilePath;
		// updateStatus(q_("Invalid JSON structure in Configuration File!"));
		return;
	}

	QJsonObject rootObject = jsonDoc.object();
	if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
		qWarning() << "[NebulaTextures] No 'subTiles' array found in JSON file:" << cfgFilePath;
		// updateStatus(q_("No 'subTiles' array in Configuration File!"));
		return;
	}

	QJsonArray subTiles = rootObject["subTiles"].toArray();

	// Loop through each subTile and delete the "imageUrl"
	foreach (const QJsonValue &subTileValue, subTiles) {
		if (!subTileValue.isObject()) {
			continue;
		}

		QJsonObject subTileObject = subTileValue.toObject();

		if (subTileObject.contains("imageUrl") && subTileObject["imageUrl"].isString()) {
			QString imageUrl = subTileObject["imageUrl"].toString();
			QString imagePath = StelFileMgr::getUserDir() + pluginDir + imageUrl;

			// Delete the file if it exists
			if (QFile::exists(imagePath)) {
				if (!QFile::remove(imagePath)) {
					qWarning() << "[NebulaTextures] Failed to delete image file:" << imagePath;
				}
			} else {
				qWarning() << "[NebulaTextures] Image file does not exist:" << imagePath;
			}
		}
	}
	// updateStatus(q_("Images deletion completed."));
}


/*
 *  Add the texture to the custom textures configuration
 */
void NebulaTexturesDialog::addCustomTexture()
{
	if(!askConfirmation("Caution! Are you sure to add this texture? It will only take effect after restarting Stellarium."))
		return;

	addTexture(configFile, CUSTOM_TEXNAME);
}

/*
 * Add the texture to configuration, groupName needed.
 *
 * Steps:
 *   - Validate the image path from the input field.
 *   - Copy the image file to the user folder with a timestamped name.
 *   - Retrieve celestial coordinates (RA, Dec) from the UI for the image's corners and reference point.
 *   - Organize the coordinates into a JSON array for later use.
 *   - Call `registerTexture` to store the texture and its associated coordinates.
 *
 * @param cfgPath  The path of the configuration file to update.
 * @param groupName  The group name for the texture, custom or temporary.
 */
void NebulaTexturesDialog::addTexture(QString cfgPath, QString groupName)
{
	QString imagePath = ui->lineEditImagePath->text();
	if (imagePath.isEmpty()) {
		qWarning() << "[NebulaTextures] Image path is empty.";
		updateStatus(q_("Image path is empty."));
		return;
	}

	QFileInfo fileInfo(imagePath);
	if (!fileInfo.exists()) {
		updateStatus(q_("The specified image file does not exist."));
		return;
	}
	if (!fileInfo.isReadable()) {
		updateStatus(q_("The specified image file is not readable."));
		return;
	}
	QString suffix = fileInfo.suffix().toLower();
	QStringList allowedSuffixes = {"png", "jpg", "gif", "tif", "tiff", "jpeg"};
	if (!allowedSuffixes.contains(suffix)) {
		updateStatus(q_("Invalid image format. Supported formats: PNG, JPEG, GIF, TIFF."));
		return;
	}

	QString pluginFolder = StelFileMgr::getUserDir() + pluginDir;
	QDir().mkpath(pluginFolder);

	QString baseName = fileInfo.completeBaseName();
	QString extension = fileInfo.suffix();
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
	QString imageUrl = QString("%1_%2.%3").arg(baseName, timestamp, extension);
	QString targetFilePath = pluginFolder + imageUrl;
	if (!QFile::copy(imagePath, targetFilePath)) {
		qWarning() << "[NebulaTextures] Failed to copy image file to target path:" << targetFilePath;
		// updateStatus(q_("Failed to copy image file to user folder!"));
		return;
	}

	// Retrieve coordinates from the QDoubleSpinBoxes
	topLeftRA = ui->topLeftX->value();
	topLeftDec = ui->topLeftY->value();
	topRightRA = ui->topRightX->value();
	topRightDec = ui->topRightY->value();
	bottomLeftRA = ui->bottomLeftX->value();
	bottomLeftDec = ui->bottomLeftY->value();
	bottomRightRA = ui->bottomRightX->value();
	bottomRightDec = ui->bottomRightY->value();
	referRA = ui->referX->value();
	referDec = ui->referY->value();

	QJsonArray innerWorldCoords;
	innerWorldCoords.append(QJsonArray({bottomLeftRA, bottomLeftDec}));
	innerWorldCoords.append(QJsonArray({bottomRightRA, bottomRightDec}));
	innerWorldCoords.append(QJsonArray({topRightRA, topRightDec}));
	innerWorldCoords.append(QJsonArray({topLeftRA, topLeftDec}));

	registerTexture(imageUrl, innerWorldCoords, 0.2, ui->brightComboBox->currentData().toDouble(), cfgPath, groupName);
}


/*
 * Register the texture to the custom textures configuration JSON file with new texture data.
 *
 * Steps:
 *   - Check if the specified JSON configuration file exists.
 *   - If it exists, open and read the file, then update it with new texture information (world coordinates, texture coordinates, resolution, brightness).
 *   - If the file does not exist, create a new configuration with default values.
 *   - Append the new texture data to the configuration file and write the updated file back to disk.
 *   - Update the UI status to reflect success or failure.
 *
 * @param imageUrl        The URL of the texture image.
 * @param innerWorldCoords The world coordinates corresponding to the texture.
 * @param minResolution   The minimum resolution of the texture.
 * @param maxBrightness   The maximum brightness of the texture.
 * @param cfgPath  The path of the configuration file to update.
 * @param groupName  The group name for the texture, custom or temporary.
 */
void NebulaTexturesDialog::registerTexture(const QString& imageUrl, const QJsonArray& innerWorldCoords, double minResolution, double maxBrightness, QString cfgPath, QString groupName)
{
	QString pluginFolder = StelFileMgr::getUserDir() + pluginDir;
	QDir().mkpath(pluginFolder);

	QString path = StelFileMgr::getUserDir() + cfgPath;
	QFile jsonFile(path);
	QJsonObject rootObject;

	if (jsonFile.exists() && groupName!=TEST_TEXNAME) {
		if (!jsonFile.open(QIODevice::ReadOnly)) {
			qWarning() << "[NebulaTextures] Failed to open existing JSON file for reading:" << path;
			// updateStatus(q_("Failed to open Configuration File!"));
			return;
		}

		QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
		if (!jsonDoc.isObject()) {
			qWarning() << "[NebulaTextures] Invalid JSON structure in file:" << path;
			// updateStatus(q_("Invalid JSON structure in Configuration File!"));
			return;
		}
		rootObject = jsonDoc.object();
		jsonFile.close();
	} else {
		rootObject["shortName"] = groupName;
		rootObject["description"] = "User specified low resolution nebula texture set.";
		rootObject["minResolution"] = 0.05;
		rootObject["alphaBlend"] = true;
		rootObject["subTiles"] = QJsonArray();
	}

	QJsonArray subTiles = rootObject.value("subTiles").toArray();

	QJsonObject newSubTile;
	QJsonObject imageCredits;
	imageCredits["short"] = "Local User";
	newSubTile["imageCredits"] = imageCredits;
	newSubTile["imageUrl"] = imageUrl;

	QJsonArray outerWorldCoords;
	outerWorldCoords.append(innerWorldCoords);
	newSubTile["worldCoords"] = outerWorldCoords;

	QJsonArray innerTextureCoords;
	innerTextureCoords.append(QJsonArray({0, 0}));
	innerTextureCoords.append(QJsonArray({1, 0}));
	innerTextureCoords.append(QJsonArray({1, 1}));
	innerTextureCoords.append(QJsonArray({0, 1}));

	QJsonArray outerTextureCoords;
	outerTextureCoords.append(innerTextureCoords);
	newSubTile["textureCoords"] = outerTextureCoords;
	newSubTile["minResolution"] = minResolution;
	newSubTile["maxBrightness"] = maxBrightness;

	subTiles.append(newSubTile);
	rootObject["subTiles"] = subTiles;

	if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "[NebulaTextures] Failed to open JSON file for writing:" << path;
		// updateStatus(q_("Failed to open Configuration File for writing!"));
		return;
	}

	QJsonDocument updatedDoc(rootObject);
	jsonFile.write(updatedDoc.toJson(QJsonDocument::Indented));
	jsonFile.flush();
	jsonFile.close();

	updateStatus(q_("Importing custom textures successfully!"));
}


/*
 * Remove the selected texture from the list and deletes the associated image file.
 *
 * Steps:
 *   - Get the currently selected item from the texture list.
 *   - Prompt the user with a confirmation dialog asking whether they want to remove the texture.
 *   - If the user confirms, proceed with the removal process.
 *   - Open and parse the JSON configuration file that contains texture information.
 *   - Search for the texture in the "subTiles" array by matching the texture name (`imageUrl`).
 *   - If the texture is found, delete the corresponding image file from the disk.
 *   - Remove the texture entry from the JSON and save the updated configuration file.
 *   - Finally, remove the texture from the UI list.
 */
void NebulaTexturesDialog::removeTexture()
{
	QListWidgetItem* selectedItem = ui->listWidget->currentItem();
	if (!selectedItem)
		return;

	QString selectedText = selectedItem->text();

	if(!askConfirmation("Caution! Are you sure to remove this texture? It will only take effect after restarting Stellarium."))
		return;

	QString path = StelFileMgr::getUserDir() + configFile;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly)){
		qWarning() << "[NebulaTextures] Failed to open existing JSON file for reading:" << path;
		return;
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
	jsonFile.close();

	if (!jsonDoc.isObject()){
		qWarning() << "[NebulaTextures] Invalid JSON structure in file:" << path;
		return;
	}

	QJsonObject rootObject = jsonDoc.object();
	if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()){
		qWarning() << "[NebulaTextures] No 'subTiles' array found in JSON file:" << path;
		return;
	}

	QJsonArray subTiles = rootObject["subTiles"].toArray();
	QJsonArray updatedSubTiles;
	bool found = false;

	for (const QJsonValue& subTileValue : subTiles) {
		if (!subTileValue.isObject()) {
			updatedSubTiles.append(subTileValue);
			continue;
		}

		QJsonObject subTileObject = subTileValue.toObject();
		if (subTileObject.contains("imageUrl") && subTileObject["imageUrl"].toString() == selectedText) {
			found = true;
			QString imageUrl = subTileObject["imageUrl"].toString();
			QString imagePath = StelFileMgr::getUserDir() + pluginDir + imageUrl;
			QFile imageFile(imagePath);
			if (imageFile.exists()) {
				imageFile.remove();
			}
			continue;
		}
		updatedSubTiles.append(subTileValue);
	}
	if (!found)
		return;

	rootObject["subTiles"] = updatedSubTiles;
	if (!jsonFile.open(QIODevice::WriteOnly)){
		qWarning() << "[NebulaTextures] Failed to open JSON file for writing:" << path;
		return;
	}

	jsonFile.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
	jsonFile.close();
	delete selectedItem;
}


/*
 * Reload texture data and update the UI list widget with available sub-tiles.
 *
 * Steps:
 *   - Refresh textures by calling the `refreshTextures()` function.
 *   - Retrieve the path to the JSON configuration file.
 *   - Open and parse the JSON file to extract texture data.
 *   - Validate the JSON structure, ensuring it contains a "subTiles" array.
 *   - Clear the existing list widget items.
 *   - Populate the list widget with entries from the "subTiles" array, displaying the image URL.
 *   - Associate metadata from each sub-tile with its corresponding list widget item.
 */
void NebulaTexturesDialog::reloadData()
{
	refreshTextures();

	QString path = StelFileMgr::getUserDir()+configFile;

	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);

	QFile jsonFile(path);
	QJsonObject rootObject;

	if (!jsonFile.exists()) {
		qWarning() << "[NebulaTextures] JSON file does not exist:" << path;
		ui->listWidget->clear();
		return;
	}

	if (!jsonFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[NebulaTextures] Failed to open JSON file for reading:" << path;
		ui->listWidget->clear();
		return;
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
	if (!jsonDoc.isObject()) {
		qWarning() << "[NebulaTextures] Invalid JSON structure in file:" << path;
		return;
	}

	rootObject = jsonDoc.object();
	jsonFile.close();

	// read and show into listWidget
	if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
		qWarning() << "[NebulaTextures] No 'subTiles' array found in JSON file:" << path;
		return;
	}

	QJsonArray subTiles = rootObject["subTiles"].toArray();

	ui->listWidget->clear();

	for (const QJsonValue& subTileValue : subTiles) {
		if (!subTileValue.isObject()) {
			qWarning() << "[NebulaTextures] Invalid subTile entry found, skipping...";
			continue;
		}

		QJsonObject subTileObject = subTileValue.toObject();

		if (subTileObject.contains("imageUrl") && subTileObject["imageUrl"].isString()) {
			QString imageUrl = subTileObject["imageUrl"].toString();
			QListWidgetItem* item = new QListWidgetItem(imageUrl, ui->listWidget);
			QVariant data = QVariant::fromValue(subTileObject);
			item->setData(Qt::UserRole, data);
			ui->listWidget->addItem(item);
		}
	}
}


/*
 * Refresh textures and manage their visibility.
 *
 * Steps:
 *   - Check "show custom textures" flag
 *   - If enabled, make them visible and calls avoidConflict to handle potential conflicts.
 *   - If disabled, ensure the custom textures are hidden and the default textures are visible.
 */
void NebulaTexturesDialog::refreshTextures()
{
	bool showCustom = getShowCustomTextures();
	if (!showCustom){
		setTexturesVisible(CUSTOM_TEXNAME,false);
		setTexturesVisible(DEFAULT_TEXNAME,true);
	}
	else{
		setTexturesVisible(CUSTOM_TEXNAME,true);
		avoidConflict(); // only when show_custom && avoid_conflict
	}
}


/*
 * Set the visibility of the specified texture and its sub-tiles.
 *
 * Steps:
 *   - Retrieve the texture identified by `TexName`.
 *   - If the texture is found, iterate through its sub-tiles.
 *   - Set the visibility of each sub-tile based on the `visible` parameter.
 *   - Return false if the texture or its sub-tiles cannot be found.
 *
 * @param TexName  The name of the texture to modify.
 * @param visible  The visibility state to set for the texture and its sub-tiles.
 *
 * @return true if the texture and its sub-tiles are found and visibility is set successfully.
 *         false if the texture or its sub-tiles cannot be found.
 */
bool NebulaTexturesDialog::setTexturesVisible(QString TexName, bool visible)
{
	StelSkyImageTile* mTile = get_aTile(TexName);
	if(!mTile) return false;
	for (int i = 0; i < mTile->getSubTiles().size(); ++i) {
		auto subTile = mTile->getSubTiles()[i];
		StelSkyImageTile* subImageTile = dynamic_cast<StelSkyImageTile*>(subTile);
		subImageTile->setVisible(visible);
	}
	return true;
}


/*
 * Retrieve a StelSkyImageTile object based on the provided texture key.
 *
 * Searches for the sky layer associated with the given key in the StelSkyLayerMgr.
 * If found, it attempts to cast the layer to a StelSkyImageTile and returns it.
 * Returns Q_NULLPTR if no layer is found or the cast fails.
 */
StelSkyImageTile* NebulaTexturesDialog::get_aTile(QString key)
{
	StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
	auto aTex = skyLayerMgr->allSkyLayers.find(key);
	if(aTex == skyLayerMgr->allSkyLayers.end())
		return Q_NULLPTR;
	StelSkyLayerMgr::SkyLayerElem* aElem = aTex.value();
	if (!aElem || !aElem->layer) return Q_NULLPTR;
	StelSkyImageTile* aTile = dynamic_cast<StelSkyImageTile*>(aElem->layer.data());
	return aTile;
}


/*
 * Avoid conflicts between custom and default textures by hiding overlapping tiles.
 *
 * Steps:
 *   - Check if the 'Avoid Area Conflict' flag is enabled.
 *   - Retrieve the custom and default texture tiles.
 *   - Loop through the sub-tiles of the default texture.
 *   - For each visible sub-tile, check for overlap with custom texture sub-tiles.
 *   - Hide the default texture sub-tile if overlap is detected.
 *   - If conflicts are not to be avoided, make all default texture tiles visible.
 */
void NebulaTexturesDialog::avoidConflict()
{
	if (getAvoidAreaConflict()){
		StelSkyImageTile* cusTile = get_aTile(CUSTOM_TEXNAME);
		StelSkyImageTile* defTile = get_aTile(DEFAULT_TEXNAME);
		if(!cusTile) return;
		if(!defTile) return;

		for (int i = 0; i < defTile->getSubTiles().size(); ++i) {
			StelSkyImageTile* subdefTile = dynamic_cast<StelSkyImageTile*>(defTile->getSubTiles()[i]);
			if(subdefTile->getVisible() == true && subdefTile->getSkyConvexPolygons().size()>0){
				SphericalRegionP subdefPoly = subdefTile->getSkyConvexPolygons()[0];
				for (int j = 0; j < cusTile->getSubTiles().size(); ++j) {
					StelSkyImageTile* subcusTile = dynamic_cast<StelSkyImageTile*>(cusTile->getSubTiles()[j]);
					if(subcusTile->getVisible() == true && subcusTile->getSkyConvexPolygons().size()>0){
						SphericalRegionP subcusPoly = subcusTile->getSkyConvexPolygons()[0];
						if (subcusPoly->intersects(subdefPoly)){
							subdefTile->setVisible(false);
						}
					}
				}
			}
		}
	}
	else{
		setTexturesVisible(DEFAULT_TEXNAME, true);
	}
}


/*
 * Move the view to the center coordinates of the selected nebula texture.
 *
 * Steps:
 *   - Retrieve the selected item from the list.
 *   - Open and parse the JSON configuration file.
 *   - Find the corresponding texture entry based on the selected item.
 *   - Extract world coordinate information from the JSON data.
 *   - Compute the spherical center of the texture using vector averaging.
 *   - Convert the computed center to Right Ascension (RA) and Declination (Dec).
 *   - Adjust the camera's view to focus on the computed coordinates.
 */
void NebulaTexturesDialog::gotoSelectedItem(QListWidgetItem* item)
{
	if (!item) return;

	// Constants
	const double D2R = M_PI / 180.0;  // Degree to radian
	const double R2D = 180.0 / M_PI;  // Radian to degree

	QString selectedText = item->text();
	QString path = StelFileMgr::getUserDir() + configFile;
	QFile jsonFile(path);
	if (!jsonFile.open(QIODevice::ReadOnly)) {
		qWarning() << "[NebulaTextures] Failed to open JSON file:" << path;
		return;
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
	jsonFile.close();
	if (!jsonDoc.isObject()) {
		qWarning() << "[NebulaTextures] Invalid JSON structure in file:" << path;
		return;
	}

	QJsonObject rootObject = jsonDoc.object();
	if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
		qWarning() << "[NebulaTextures] No 'subTiles' array found in JSON file:" << path;
		return;
	}

	QJsonArray subTiles = rootObject["subTiles"].toArray();
	for (const QJsonValue& subTileValue : subTiles) {
		if (!subTileValue.isObject()) continue;

		QJsonObject subTileObject = subTileValue.toObject();
		if (!subTileObject.contains("imageUrl") || subTileObject["imageUrl"].toString() != selectedText)
			continue;

		if (!subTileObject.contains("worldCoords") || !subTileObject["worldCoords"].isArray()) {
			qWarning() << "[NebulaTextures] Missing 'worldCoords' in subTileObject.";
			return;
		}

		QJsonArray worldCoords = subTileObject["worldCoords"].toArray();
		if (worldCoords.size() != 1 || !worldCoords[0].isArray() || worldCoords[0].toArray().size() != 4) {
			qWarning() << "[NebulaTextures] Invalid 'worldCoords' format.";
			return;
		}

		QJsonArray corners = worldCoords[0].toArray();
		QVector<QPair<double, double>> raDecList;
		for (const QJsonValue& corner : corners) {
			if (!corner.isArray() || corner.toArray().size() != 2) {
				qWarning() << "[NebulaTextures] Invalid coordinate format.";
				return;
			}
			raDecList.append(qMakePair(corner.toArray()[0].toDouble(), corner.toArray()[1].toDouble()));
		}

		// calculate center coordinates (approximately)
		double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
		for (const auto& point : raDecList) {
			double ra_rad = D2R * point.first;
			double dec_rad = D2R * point.second;
			double x = cos(dec_rad) * cos(ra_rad);
			double y = cos(dec_rad) * sin(ra_rad);
			double z = sin(dec_rad);
			sum_x += x;
			sum_y += y;
			sum_z += z;
		}

		double avg_x = sum_x / 4.0;
		double avg_y = sum_y / 4.0;
		double avg_z = sum_z / 4.0;
		double norm = sqrt(avg_x * avg_x + avg_y * avg_y + avg_z * avg_z);
		if (norm == 0.0) {
			qWarning() << "[NebulaTextures] Invalid input: zero vector sum.";
			return;
		}

		avg_x /= norm;
		avg_y /= norm;
		avg_z /= norm;
		double centerRA = fmod(R2D* atan2(avg_y, avg_x) + 360.0, 360.0);
		double centerDec = R2D * asin(avg_z);
		centerDec = qBound(-90.0, centerDec, 90.0);

		// goto center view
		StelCore* core = StelApp::getInstance().getCore();
		StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
		Vec3d pos;
		double spinLong = D2R * centerRA;
		double spinLat = D2R * centerDec;

		mvmgr->setViewUpVector(Vec3d(0., 0., 1.));
		Vec3d aimUp = mvmgr->getViewUpVectorJ2000();
		StelMovementMgr::MountMode mountMode = mvmgr->getMountMode();

		StelUtils::spheToRect(spinLong, spinLat, pos);
		if ((mountMode == StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat) > (0.9 * M_PI_2))) {
			mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat > 0. ? 1. : -1.));
			aimUp = mvmgr->getViewUpVectorJ2000();
		}

		mvmgr->setFlagTracking(false);
		mvmgr->moveToJ2000(pos, aimUp, mvmgr->getAutoMoveDuration());
		return;
	}
}


// Set the value for showing custom textures in the configuration and refresh textures.
void NebulaTexturesDialog::setShowCustomTextures(bool b)
{
	conf->setValue(NT_CONFIG_PREFIX + "/showCustomTextures", b);
}

// Get the value for showing custom textures from the configuration.
bool NebulaTexturesDialog::getShowCustomTextures()
{
	return conf->value(NT_CONFIG_PREFIX + "/showCustomTextures", false).toBool();
}

// Set the value for avoiding area conflicts in the configuration and refresh textures.
void NebulaTexturesDialog::setAvoidAreaConflict(bool b)
{
	conf->setValue(NT_CONFIG_PREFIX + "/avoidAreaConflict", b);
	refreshTextures();
}

// Get the value for avoiding area conflicts from the configuration.
bool NebulaTexturesDialog::getAvoidAreaConflict()
{
	return conf->value(NT_CONFIG_PREFIX + "/avoidAreaConflict", false).toBool();
}
