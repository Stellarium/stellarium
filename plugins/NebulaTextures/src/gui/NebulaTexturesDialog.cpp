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
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStringList>
#include <cmath>

/*
 * Constructor for the NebulaTexturesDialog class. Initializes the dialog, sets up
 * network management, and configures timers for periodic status checking.
 * Also loads nebula textures on initialization.
 */
NebulaTexturesDialog::NebulaTexturesDialog()
   : StelDialog("NebulaTextures"),
   flag_renderTempTex(false)
   , m_conf(StelApp::getInstance().getSettings())
{
	ui = new Ui_nebulaTexturesDialog();

   networkManager = new QNetworkAccessManager(this);

   subStatusTimer = new QTimer(this);
   jobStatusTimer = new QTimer(this);
   connect(subStatusTimer, &QTimer::timeout, this, &NebulaTexturesDialog::checkSubStatus);
   connect(jobStatusTimer, &QTimer::timeout, this, &NebulaTexturesDialog::checkJobStatus);

   reloadTextures();
}

NebulaTexturesDialog::~NebulaTexturesDialog()
{
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
 * Initializes and sets up the content and interactions of the NebulaTexturesDialog.
 * This function configures UI elements, establishes signal-slot connections,
 * and loads necessary settings and data for the dialog.
 */
void NebulaTexturesDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= static_cast<StelGui*>(StelApp::getInstance().getGui());
	enableKineticScrolling(gui->getFlagUseKineticScrolling());
	connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

   connect(ui->openFileButton, SIGNAL(clicked()), this, SLOT(on_openFileButton_clicked()));
   connect(ui->uploadImageButton, SIGNAL(clicked()), this, SLOT(on_uploadImageButton_clicked()));
   connect(ui->goPushButton, SIGNAL(clicked()), this, SLOT(on_goPushButton_clicked()));
   connect(ui->renderButton, SIGNAL(clicked()), this, SLOT(renderTempCustomTexture()));
   connect(ui->unrenderButton, SIGNAL(clicked()), this, SLOT(unRenderTempCustomTexture()));

   connect(ui->addTexture, SIGNAL(clicked()), this, SLOT(on_addTexture_clicked()));
   connect(ui->removeButton, SIGNAL(clicked()), this, SLOT(on_removeButton_clicked()));

   connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reloadData()));
   connect(ui->checkBoxShow, SIGNAL(clicked(bool)), this, SLOT(setShowCustomTextures(bool)));
   connect(ui->checkBoxAvoid, SIGNAL(clicked(bool)), this, SLOT(setAvoidAreaConflict(bool)));

	setAboutHtml();

   ui->label_apiKey->setText("<a href=\"https://nova.astrometry.net/api_help\">Astrometry ApiKey:</a>");

   // load config
   ui->checkBoxShow->setChecked(getShowCustomTextures());
   ui->checkBoxAvoid->setChecked(getAvoidAreaConflict());

   reloadData();

   ui->lineEditApiKey->setText(m_conf->value(MS_CONFIG_PREFIX + "/AstroMetry_Apikey", "").toString());
}


void NebulaTexturesDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[NebulaTextures] restore defaults...";
      // GETSTELMODULE(NebulaTextures)->restoreDefaultSettings();
	}
	else
		qDebug() << "[NebulaTextures] restore defaults is canceled...";
}

/*
 * Sets the HTML content for the "About" section in the dialog.
 * This function generates the HTML that provides information about the Nebula Textures
 * plugin, including its version, license, author, description, and publication citation.
 * The HTML content is then displayed in the aboutTextBrowser widget with the appropriate
 * style settings.
 */
void NebulaTexturesDialog::setAboutHtml(void)
{
   QString html = "<html><head></head><body>";

   html += "<h2>" + q_("Nebula Textures Plug-in") + "</h2><table class='layout' width=\"90%\">";
   html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_VERSION + "</td></tr>";
   html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_LICENSE + "</td></tr>";
   html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>WANG Siliang</td></tr>";
   html += "</table>";

   html += "<p>" + q_("The Nebula Textures plugin allows users to create and display their own astronomical sky images or even sketches in Stellarium. It supports online plate solving for coordinate parsing, or manual input of coordinates to localize the image, render it, and add it to the custom texture management system.") + "</p>";
   html += "<p>" + q_("This plugin provides an intuitive way for users to visualize their astronomical observations or creations within Stellarium, enhancing the realism and immersion of the celestial view. By using plate solving or manually inputting coordinates, users can accurately position and render their images or sketches, which are then seamlessly integrated into Stellarium's texture system.") + "</p>";

   html += "<h3>" + q_("Publications") + "</h3>";
   html += "<p>" + q_("If you use this plugin in your publications, please cite:") + "</p>";
   html += "<ul>";
   html += "<li>" + QString("{WANG Siliang: Nebula Textures Plugin.} Stellarium Plugin, 2024-2025.")
                      .toHtmlEscaped() + "</li>";
   html += "</ul>";

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

// Sets the value for showing custom textures in the configuration and reloads textures.
void NebulaTexturesDialog::setShowCustomTextures(bool b)
{
   m_conf->setValue(MS_CONFIG_PREFIX + "/showCustomTextures", b);
   reloadTextures();
}

// Gets the current setting for showing custom textures from the configuration.
bool NebulaTexturesDialog::getShowCustomTextures()
{
   return m_conf->value(MS_CONFIG_PREFIX + "/showCustomTextures", false).toBool();
}

// Sets the value for avoiding area conflicts in the configuration and reloads textures.
void NebulaTexturesDialog::setAvoidAreaConflict(bool b)
{
   m_conf->setValue(MS_CONFIG_PREFIX + "/avoidAreaConflict", b);
   reloadTextures();
}

// Gets the current setting for avoiding area conflicts from the configuration.
bool NebulaTexturesDialog::getAvoidAreaConflict()
{
   return m_conf->value(MS_CONFIG_PREFIX + "/avoidAreaConflict", false).toBool();
}


void NebulaTexturesDialog::updateStatus(const QString &status)
{
   ui->statusText->setText(status);
}


void NebulaTexturesDialog::on_openFileButton_clicked()
{
   QString fileName = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Open Image"), QDir::homePath(), tr("Images (*.png *.jpg *.bmp *.tiff)"));
   if (!fileName.isEmpty())
   {
      ui->lineEditImagePath->setText(fileName);
      updateStatus(q_("File selected: ") + fileName);
   }
}

/*
 * Handles the upload image button click event.
 * Validates input fields, creates a POST request to the API for login,
 * and initiates the image upload process.
 * Updates the UI state and connects to the reply signal for further processing.
 */
void NebulaTexturesDialog::on_uploadImageButton_clicked() // WARN: image should not be flip
{
   QString apiKey = ui->lineEditApiKey->text();
   QString imagePath = ui->lineEditImagePath->text();

   if (imagePath.isEmpty() || apiKey.isEmpty())
   {
      updateStatus(q_("Please provide both API key and image path."));
      return;
   }

   if (ui->checkBoxKeepApi->isChecked())
      m_conf->setValue(MS_CONFIG_PREFIX + "/AstroMetry_Apikey", apiKey);

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
   QNetworkReply *reply = networkManager->post(request, query.toString().toUtf8());

   // Connect the reply finished signal to a slot
   connect(reply, &QNetworkReply::finished, this, [this, reply]() {onLoginReply(reply);});

   // Perform image upload logic here, e.g., using an API call
   updateStatus(q_("Sending requests..."));

   changeUiState(true);
}

/*
 * Handles the reply from the login request.
 * If login is successful, it proceeds to upload the selected image with the session info.
 * Sends the image as a multipart/form-data request and updates the UI accordingly.
 */
void NebulaTexturesDialog::onLoginReply(QNetworkReply *reply)
{
   QByteArray content = reply->readAll();
   qDebug()<<content;
   if (reply->error() != QNetworkReply::NoError) {
      updateStatus(q_("Login failed!"));
      reply->deleteLater();
      changeUiState(false);
      return;
   }
   updateStatus(q_("Login success..."));

   QFile imageFile(ui->lineEditImagePath->text());
   if (!imageFile.open(QIODevice::ReadOnly)) {
      updateStatus(q_("Failed to open image file!"));
      reply->deleteLater();
      changeUiState(false);
      return;
   }

   QJsonDocument doc = QJsonDocument::fromJson(content);
   QJsonObject json = doc.object();
   session = json["session"].toString();

   QUrl uploadUrl(API_URL + "api/upload");
   QNetworkRequest request(uploadUrl);
   QJsonObject uploadJson;
   uploadJson["session"] = session;
   uploadJson["allow_commercial_use"] = "d";
   uploadJson["allow_modifications"] = "d";
   uploadJson["publicly_visible"] = "y";
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
   connect(uploadReply, &QNetworkReply::finished, this, [this, uploadReply]() {onUploadReply(uploadReply);});

   reply->deleteLater();
   updateStatus(q_("Uploading image..."));
}

/*
 * Handles the reply from the image upload request.
 * If the upload is successful, extracts the submission ID and starts a timer to check the upload status.
 * Updates the UI accordingly and manages the reply cleanup.
 */
void NebulaTexturesDialog::onUploadReply(QNetworkReply *reply)
{
   QByteArray content = reply->readAll();
   qDebug()<<content;
   if (reply->error() != QNetworkReply::NoError) {
      updateStatus(q_("Image upload failed!"));
      reply->deleteLater();
      changeUiState(false);
      return;
   }

   QJsonDocument doc = QJsonDocument::fromJson(content);
   QJsonObject json = doc.object();
   subId = QString::number(json["subid"].toInt());
   subStatusTimer->start(3000);
   reply->deleteLater();

   updateStatus(q_("Image uploaded. Please wait..."));
}

/*
 * Sends a request to check the status of a submission using its ID.
 * Upon receiving the response, the function calls `onsubStatusReply` to process the result.
 */
void NebulaTexturesDialog::checkSubStatus()
{
   QUrl statusUrl(API_URL + "api/submissions/" + subId);
   QNetworkRequest request(statusUrl);
   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
   QNetworkReply *subStatusReply = networkManager->get(request);
   connect(subStatusReply, &QNetworkReply::finished, this, [this, subStatusReply]() {onsubStatusReply(subStatusReply);});
   updateStatus(q_("Requesting submission. Please wait..."));
}

/*
 * Handles the response from the submission status request.
 * If successful, it retrieves the job ID and starts the job status timer; otherwise, it reports an error.
 */
void NebulaTexturesDialog::onsubStatusReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      updateStatus(q_("Failed to get submission status. Retry now..."));
      reply->deleteLater();
      changeUiState(false);
      return;
   }
   QByteArray cont = reply->readAll();
   qDebug()<<cont;
   QJsonDocument doc = QJsonDocument::fromJson(cont);
   QJsonObject json = doc.object();
   QJsonArray jobs = json["jobs"].toArray();

   if (!jobs.isEmpty() && !jobs[0].isNull()) {
      jobId = QString::number(jobs[0].toInt());
      subStatusTimer->stop();
      jobStatusTimer->start(3000);
      reply->deleteLater();

      updateStatus(q_("Submission ID got. Please wait..."));
   }
}

/*
 * Sends a request to check the status of a job using its job ID.
 * Upon receiving the response, the function calls `onJobStatusReply` to process the result.
 */
void NebulaTexturesDialog::checkJobStatus()
{
   QUrl jobStatusUrl(API_URL + "api/jobs/" + jobId);
   QNetworkRequest request(jobStatusUrl);

   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

   QNetworkReply *jobStatusReply = networkManager->get(request);
   connect(jobStatusReply, &QNetworkReply::finished, this, [this, jobStatusReply]() { onJobStatusReply(jobStatusReply); });
   updateStatus(q_("Requesting job status. Please wait..."));
}


/*
 * Handles the response from the job status request.
 * If successful, it checks the job status and either triggers WCS file download or reports an error.
 */
void NebulaTexturesDialog::onJobStatusReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      updateStatus(q_("Failed to get job status. Retry now..."));
      reply->deleteLater();
      changeUiState(false);
      return;
   }

   QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
   QJsonObject json = doc.object();
   QString status = json["status"].toString();

   if (status == "success") {
      updateStatus(q_("Job completed successfully! Downloading WCS file..."));
      jobStatusTimer->stop();
      QUrl wcsUrl(API_URL+ "wcs_file/" + jobId);
      QNetworkRequest request(wcsUrl);
      QNetworkReply *wcsReply = networkManager->get(request);
      connect(wcsReply, &QNetworkReply::finished, this, [this, wcsReply]() { onWcsDownloadReply(wcsReply); });

   } else if (status == "error") {
      updateStatus(q_("Error in job processing!"));
      jobStatusTimer->stop();
      changeUiState(false);
   }
   reply->deleteLater();
}

/*
 * Handles the response after downloading the WCS (World Coordinate System) file.
 * The function parses the WCS data, extracts relevant parameters, converts pixel coordinates to celestial coordinates,
 * and updates the user interface with the calculated coordinates (top-left, bottom-left, top-right, bottom-right).
 */
void NebulaTexturesDialog::onWcsDownloadReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      updateStatus(q_("Failed to download WCS file..."));
      reply->deleteLater();
      changeUiState(false);
      return;
   }
   QByteArray cont = reply->readAll();
   QString content = QString::fromUtf8(cont);

   reply->deleteLater();

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
         qDebug()<<match.captured(2);
         qDebug()<<value;
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

   // qDebug() <<"["<<bottomLeftRA<<","<<bottomLeftDec<<"],"
   //          <<"["<<bottomRightRA<<","<<bottomRightDec<<"],"
   //          <<"["<<topRightRA<<","<<topRightDec<<"],"
   //          <<"["<<topLeftRA<<","<<topLeftDec<<"]";

   X = IMAGEW - 1, Y = IMAGEH - 1;
   result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
   bottomRightRA = result.first;
   bottomRightDec = result.second;
   ui->bottomRightX->setValue(bottomRightRA);
   ui->bottomRightY->setValue(bottomRightDec);

   changeUiState(false);

   updateStatus(q_("Processing completed! Goto Center Point, Try to Render, Check and Add to Local Storage."));
}




/*
 * Converts pixel coordinates (X, Y) on an image to celestial coordinates (longitude, latitude) using the World Coordinate System (WCS) parameters.
 * This function applies a series of transformations to convert pixel-based coordinates into the corresponding celestial coordinates,
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

   if (lng > 360.0) {
      lng -= 360.0;
   } else if (lng < -360.0) {
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
 * Toggles the enabled/disabled state of UI components based on the provided "freeze" flag.
 *
 * When "freeze" is true, all specified UI buttons are disabled, effectively "freezing" the interface.
 * When "freeze" is false, the buttons are enabled, allowing user interaction.
 *
 * @param freeze  A boolean value indicating whether to disable (true) or enable (false) the UI elements.
 */
void NebulaTexturesDialog::changeUiState(bool freeze)
{
   ui->openFileButton->setDisabled(freeze);
   ui->uploadImageButton->setDisabled(freeze);
   ui->goPushButton->setDisabled(freeze);
   ui->renderButton->setDisabled(freeze);
   ui->unrenderButton->setDisabled(freeze);
   ui->addTexture->setDisabled(freeze);
}

/*
 * Slot triggered when the "Go" button is clicked. This function moves the view to the specified celestial coordinates
 * (right ascension and declination) by converting them into 3D spherical coordinates and adjusting the view up vector.
 * The movement is handled by the movement manager, which is part of the core functionality of the Stellarium application.
 *
 * Steps performed by this function:
 *   1. Converts the reference right ascension and declination (referRA, referDec) into radians for spherical calculations.
 *   2. Sets the view up vector to a stable direction (J2000 coordinate system).
 *   3. Adjusts the view up vector based on the equatorial mount mode and latitude for stability when close to the poles.
 *   4. Calls the movement manager to move the view to the target coordinates, ensuring smooth transition with auto-duration.
 */
void NebulaTexturesDialog::on_goPushButton_clicked()
{
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

/*
 * Renders a temporary custom texture for the nebula based on the user-provided image path.
 * Checks if the path is valid, adds the texture to the system, and inserts it into the sky layer.
 * Updates the UI status on success or failure and optionally hides the default texture if enabled.
 *
 * Steps:
 *   - Validates image path.
 *   - Renders the custom texture by adding it to the sky layer.
 *   - Updates UI status and manages default texture visibility based on user preferences.
 */
void NebulaTexturesDialog::renderTempCustomTexture()
{
   QString imagePath = ui->lineEditImagePath->text();
   if (imagePath.isEmpty()) {
      qWarning() << "Image path is empty.";
      updateStatus(q_("Image path is empty."));
      return;
   }

   updateStatus(q_("Rendering..."));

   QString path = StelFileMgr::getUserDir()+tmpcfgFile;

   deleteImagesFromCfg(tmpcfgFile);

   addTexture(tmpcfgFile, TEST_TEXNAME);

   StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);

   if (path.isEmpty()){
      qWarning() << "ERROR while loading nebula texture.";
      updateStatus(q_("Rendering failed."));
   }
   else{
      if(flag_renderTempTex) unRenderTempCustomTexture();
      skyLayerMgr->insertSkyImage(path, QString(), true, 1);
      flag_renderTempTex = true;
      if(ui->disableDefault->isChecked())
         setTexturesVisible(DEFAULT_TEXNAME, false);
      updateStatus(q_("Rendering complete."));
   }
}

/*
 * Reads a given configuration file and deletes the files listed in the "imageUrl" field.
 * This function assumes that the configuration file contains a list of subTiles, each with an "imageUrl".
 *
 * Steps:
 *   1. Opens the given configuration file and reads the JSON data.
 *   2. Loops through each subTile to extract the "imageUrl".
 *   3. Deletes the file corresponding to each "imageUrl" if it exists.
 *   4. Logs the process and updates the UI.
 */
void NebulaTexturesDialog::deleteImagesFromCfg(const QString& cfgFile)
{
   QString cfgFilePath = StelFileMgr::getUserDir() + cfgFile;

   // Step 1: Read the JSON configuration file
   QFile jsonFile(cfgFilePath);

   if (!jsonFile.open(QIODevice::ReadOnly)) {
      qWarning() << "Failed to open JSON file for reading:" << cfgFilePath;
      updateStatus(q_("Failed to open Configuration File!"));
      return;
   }

   QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
   jsonFile.close();

   if (!jsonDoc.isObject()) {
      qWarning() << "Invalid JSON structure in file:" << cfgFilePath;
      updateStatus(q_("Invalid JSON structure in Configuration File!"));
      return;
   }

   QJsonObject rootObject = jsonDoc.object();
   if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
      qWarning() << "No 'subTiles' array found in JSON file:" << cfgFilePath;
      updateStatus(q_("No 'subTiles' array in Configuration File!"));
      return;
   }

   QJsonArray subTiles = rootObject["subTiles"].toArray();

   // Step 2: Loop through each subTile and delete the "imageUrl"
   foreach (const QJsonValue &subTileValue, subTiles) {
      if (!subTileValue.isObject()) {
         continue;
      }

      QJsonObject subTileObject = subTileValue.toObject();

      if (subTileObject.contains("imageUrl") && subTileObject["imageUrl"].isString()) {
         QString imageUrl = subTileObject["imageUrl"].toString();
         QString imagePath = StelFileMgr::getUserDir() + pluginDir + imageUrl;

                // Step 3: Delete the file if it exists
         if (QFile::exists(imagePath)) {
            if (QFile::remove(imagePath)) {
               qDebug() << "Deleted image file:" << imagePath;
            } else {
               qWarning() << "Failed to delete image file:" << imagePath;
            }
         } else {
            qWarning() << "Image file does not exist:" << imagePath;
         }
      }
   }

   updateStatus(q_("Images deletion completed."));
}

/*
 * Cancels the rendering of the temporary custom texture.
 * Removes the custom texture from the sky layer and restores the default texture.
 * Reloads all textures and updates the UI status to indicate the cancellation.
 */
void NebulaTexturesDialog::unRenderTempCustomTexture()
{
   if(!flag_renderTempTex) return;
   StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
   skyLayerMgr->removeSkyLayer(TEST_TEXNAME);

   setTexturesVisible(DEFAULT_TEXNAME, true);
   reloadTextures();
   updateStatus(q_("Cancel rendering."));
}


void NebulaTexturesDialog::on_addTexture_clicked()
{
   addTexture(configFile, CUSTOM_TEXNAME);
}

/*
 * Adds a texture image to the custom textures folder and updates its coordinates.
 *
 * Steps:
 *   1. Validates the image path from the input field.
 *   2. Copies the image file to the user folder with a timestamped name.
 *   3. Retrieves celestial coordinates (RA, Dec) from the UI for the image's corners and reference point.
 *   4. Organizes the coordinates into a JSON array for later use.
 *   5. Calls `updateCustomTextures` to store the texture and its associated coordinates.
 *
 * @param addPath  The path where the texture is to be added.
 * @param keyName  The unique key name for the texture.
 */
void NebulaTexturesDialog::addTexture(QString addPath, QString keyName) // logic, hint: will add next time restart
{
   QString imagePath = ui->lineEditImagePath->text();
   if (imagePath.isEmpty()) {
      qWarning() << "Image path is empty.";
      updateStatus(q_("Image path is empty."));
      return;
   }

   QString pluginFolder = StelFileMgr::getUserDir() + pluginDir;
   QDir().mkpath(pluginFolder);

   QFileInfo fileInfo(imagePath);
   QString baseName = fileInfo.completeBaseName();
   QString extension = fileInfo.suffix();
   QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
   QString targetFileName = QString("%1_%2.%3").arg(baseName, timestamp, extension);
   QString targetFilePath = pluginFolder + targetFileName;
   if (!QFile::copy(imagePath, targetFilePath)) {
      qWarning() << "Failed to copy image file to target path:" << targetFilePath;
      updateStatus(q_("Failed to copy image file to user folder!"));
      return;
   }
   QString imageUrl = targetFileName;

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

   // qDebug() << "[" << QString::number(bottomLeftRA, 'f', 10) << "," << QString::number(bottomLeftDec, 'f', 10) << "],"
   //          << "[" << QString::number(bottomRightRA, 'f', 10) << "," << QString::number(bottomRightDec, 'f', 10) << "],"
   //          << "[" << QString::number(topRightRA, 'f', 10) << "," << QString::number(topRightDec, 'f', 10) << "],"
   //          << "[" << QString::number(topLeftRA, 'f', 10) << "," << QString::number(topLeftDec, 'f', 10) << "]";

   QJsonArray innerWorldCoords;
   innerWorldCoords.append(QJsonArray({bottomLeftRA, bottomLeftDec}));
   innerWorldCoords.append(QJsonArray({bottomRightRA, bottomRightDec}));
   innerWorldCoords.append(QJsonArray({topRightRA, topRightDec}));
   innerWorldCoords.append(QJsonArray({topLeftRA, topLeftDec}));

   // should check whether exists
   updateCustomTextures(imageUrl, innerWorldCoords, 0.2, 13.4, keyName, addPath);
}

/*
 * Updates the custom textures configuration JSON file with new texture data.
 *
 * This function checks if the specified JSON configuration file exists. If it does, it reads and updates the file with the new texture information,
 * including world coordinates, texture coordinates, resolution, and brightness. If the file does not exist, it creates a new configuration with default values.
 *
 * @param imageUrl        The URL of the texture image.
 * @param innerWorldCoords The world coordinates corresponding to the texture.
 * @param minResolution   The minimum resolution of the texture.
 * @param maxBrightness   The maximum brightness of the texture.
 * @param keyName         The key name of the texture.
 * @param addPath         The path to the configuration file to update.
 */
void NebulaTexturesDialog::updateCustomTextures(const QString& imageUrl, const QJsonArray& innerWorldCoords, double minResolution, double maxBrightness, QString keyName, QString addPath)
{
   QString pluginFolder = StelFileMgr::getUserDir() + pluginDir;
   QDir().mkpath(pluginFolder);

   QString path = StelFileMgr::getUserDir() + addPath;
   QFile jsonFile(path);
   QJsonObject rootObject;

   if (jsonFile.exists() && keyName!=TEST_TEXNAME) {
      if (!jsonFile.open(QIODevice::ReadOnly)) {
         qWarning() << "Failed to open existing JSON file for reading:" << path;
         updateStatus(q_("Failed to open Configuration File!"));
         return;
      }

      QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
      if (!jsonDoc.isObject()) {
         qWarning() << "Invalid JSON structure in file:" << path;
         updateStatus(q_("Invalid JSON structure in Configuration File!"));
         return;
      }

      rootObject = jsonDoc.object();
      jsonFile.close();
   } else {
      rootObject["shortName"] = keyName;
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
      qWarning() << "Failed to open JSON file for writing:" << path;
      updateStatus(q_("Failed to open Configuration File for writing!"));
      return;
   }

   QJsonDocument updatedDoc(rootObject);
   jsonFile.write(updatedDoc.toJson(QJsonDocument::Indented));
   jsonFile.flush();
   jsonFile.close();

   // qDebug() << "Updated custom textures JSON file successfully.";
   updateStatus(q_("Updated custom textures JSON file successfully!"));
}


/*
 * Handles the removal of a selected texture from the custom textures list.
 *
 * This function displays a confirmation dialog, and if confirmed,
 * it removes the selected texture from the JSON configuration file by
 * deleting the corresponding entry from the "subTiles" array.
 * After updating the JSON file, the item is removed from the UI list.
 */
void NebulaTexturesDialog::on_removeButton_clicked()
{
   QListWidgetItem* selectedItem = ui->listWidget->currentItem();
   if (!selectedItem) {
      return;
   }
   QString selectedText = selectedItem->text();
   if (QMessageBox::question(&StelMainView::getInstance(), q_("Caution!"), q_("Are you sure you want to remove this texture?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      return;

   QString path = StelFileMgr::getUserDir() + configFile;
   QFile jsonFile(path);
   if (!jsonFile.open(QIODevice::ReadOnly)) {
      return;
   }
   QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
   jsonFile.close();

   if (!jsonDoc.isObject()) {
      return;
   }
   QJsonObject rootObject = jsonDoc.object();
   if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
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
            if (!imageFile.remove()) {
               qWarning() << "Failed to delete image:" << imageUrl;
            }
         } else {
            qDebug() << "Image file does not exist:" << imageUrl;
         }
         continue;
      }
      updatedSubTiles.append(subTileValue);
   }
   if (!found) {
      return;
   }
   rootObject["subTiles"] = updatedSubTiles;
   if (!jsonFile.open(QIODevice::WriteOnly)) {
      return;
   }

   jsonFile.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
   jsonFile.close();
   delete selectedItem;
}


/*
 * Reloads the custom textures data from the JSON configuration file and updates the UI list.
 *
 * This function reads the JSON file containing custom texture data,
 * extracts the "subTiles" array, and updates the UI list
 * (listWidget) with the texture URLs. Each list item stores the associated subTile data for future use.
 * If the file cannot be read or the structure is invalid,
 * appropriate warnings are logged.
 */
void NebulaTexturesDialog::reloadData()
{
   reloadTextures();

   QString path = StelFileMgr::getUserDir()+configFile;

   StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);

   QFile jsonFile(path);
   QJsonObject rootObject;

   if (!jsonFile.open(QIODevice::ReadOnly)) {
      qWarning() << "Failed to open JSON file for reading:" << path;
      return;
   }

   QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonFile.readAll());
   if (!jsonDoc.isObject()) {
      qWarning() << "Invalid JSON structure in file:" << path;
      return;
   }

   rootObject = jsonDoc.object();
   jsonFile.close();

   // read and show into listWidget

   if (!rootObject.contains("subTiles") || !rootObject["subTiles"].isArray()) {
      qWarning() << "No 'subTiles' array found in JSON file:" << path;
      return;
   }

   QJsonArray subTiles = rootObject["subTiles"].toArray();

   ui->listWidget->clear();

   for (const QJsonValue& subTileValue : subTiles) {
      if (!subTileValue.isObject()) {
         qWarning() << "Invalid subTile entry found, skipping...";
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
 * Sets the visibility of a specified texture and its sub-tiles.
 *
 * This function iterates over all sub-tiles of a given texture (identified by TexName)
 * and sets their visibility based on the provided `visible` parameter.
 * If the texture or sub-tiles cannot be found, it returns false.
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
 * Reloads and manages the visibility of textures based on the "show custom textures" flag.
 *
 * If custom textures are enabled, it makes them visible and calls avoidConflict to handle any potential conflicts.
 * If custom textures are not enabled, it ensures the custom textures are hidden and the default textures are visible.
 */
void NebulaTexturesDialog::reloadTextures()
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
 * Retrieves a StelSkyImageTile object based on the provided texture key.
 *
 * Searches for the sky layer associated with the given key in the StelSkyLayerMgr. If found, it attempts to cast
 * the layer to a StelSkyImageTile and returns it. Returns nullptr if no layer is found or the cast fails.
 */
StelSkyImageTile* NebulaTexturesDialog::get_aTile(QString key)
{
   StelSkyLayerMgr* skyLayerMgr = GETSTELMODULE(StelSkyLayerMgr);
   auto aTex = skyLayerMgr->allSkyLayers.find(key);
   StelSkyLayerMgr::SkyLayerElem* aElem = aTex.value();
   if (!aElem || !aElem->layer) return nullptr;
   StelSkyImageTile* aTile = dynamic_cast<StelSkyImageTile*>(aElem->layer.data());
   return aTile;
}


/*
 * Prevents conflicts between custom and default textures by hiding overlapping tiles.
 *
 * If the 'Avoid Area Conflict' flag is enabled, this function checks for overlapping regions between the
 * custom and default textures. If an overlap is found, the conflicting default texture tile is hidden.
 * If conflicts are not to be avoided, all default texture tiles are made visible.
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
                     // qDebug() << "intersect:"<< subdefTile->absoluteImageURI << "," << subcusTile->absoluteImageURI;
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

