/*
 * Nebula Textures plug-in for Stellarium
 *
 * Copyright (C) 2024 ultrapre@github.com
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

NebulaTexturesDialog::NebulaTexturesDialog()
	: StelDialog("NebulaTextures")
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
   connect(ui->renderButton, SIGNAL(clicked()), this, SLOT(on_renderButton_clicked()));
   connect(ui->goPushButton, SIGNAL(clicked()), this, SLOT(on_goPushButton_clicked()));

	setAboutHtml();
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

void NebulaTexturesDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Nebula Textures Plug-in") + "</h2><table class='layout' width=\"90%\">";
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}

void NebulaTexturesDialog::on_openFileButton_clicked()
{
   QString fileName = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Open Image"), QDir::homePath(), tr("Images (*.png *.jpg *.bmp *.tiff)"));
   if (!fileName.isEmpty())
   {
      ui->lineEditImagePath->setText(fileName);
      updateStatus("File selected: " + fileName);
   }
}

void NebulaTexturesDialog::on_uploadImageButton_clicked()
{
   QString apiKey = ui->lineEditApiKey->text();
   QString imagePath = ui->lineEditImagePath->text();

   if (imagePath.isEmpty() || apiKey.isEmpty())
   {
      updateStatus("Please provide both API key and image path.");
      return;
   }

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
   updateStatus("Sending requests...");
}

void NebulaTexturesDialog::updateStatus(const QString &status)
{
   ui->statusText->setText(status);
}


void NebulaTexturesDialog::onLoginReply(QNetworkReply *reply)
{
   QByteArray content = reply->readAll();
   qDebug()<<content;
   if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Login failed:" << reply->errorString();
      updateStatus("Login failed!");
      reply->deleteLater();
      return;
   }
   updateStatus("Login success...");

   QJsonDocument doc = QJsonDocument::fromJson(content);
   QJsonObject json = doc.object();
   session = json["session"].toString();
   qDebug() << "Session ID:" << session;

   QUrl uploadUrl(API_URL + "api/upload");
   QNetworkRequest request(uploadUrl);
   QJsonObject uploadJson;
   uploadJson["session"] = session;
   uploadJson["allow_commercial_use"] = "d";
   uploadJson["allow_modifications"] = "d";
   uploadJson["publicly_visible"] = "y";
   uploadJson["tweak_order"] = 0;
   uploadJson["crpix_center"] = true;

   // 'tweak_order': 0,
   //   'crpix_center': True
   QFile imageFile(ui->lineEditImagePath->text());
   if (!imageFile.open(QIODevice::ReadOnly)) {
      qDebug() << "Failed to open image file!";      
      updateStatus("Failed to open image file!");
      reply->deleteLater();
      return;
   }

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
   updateStatus("Uploading image...");
}

void NebulaTexturesDialog::onUploadReply(QNetworkReply *reply)
{
   QByteArray content = reply->readAll();
   qDebug()<<content;
   if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Image upload failed:" << reply->errorString();
      updateStatus("Image upload failed!");
      reply->deleteLater();
      return;
   }

   QJsonDocument doc = QJsonDocument::fromJson(content);
   QJsonObject json = doc.object();
   subId = QString::number(json["subid"].toInt());
   qDebug() << "Image uploaded. Sub ID:" << subId<< " "<<json["subid"];

   subStatusTimer->start(2000);
   reply->deleteLater();

   updateStatus("Image uploaded. Please wait...");
}

void NebulaTexturesDialog::checkSubStatus()
{
   QUrl statusUrl(API_URL + "api/submissions/" + subId);
   QNetworkRequest request(statusUrl);
   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
   QNetworkReply *subStatusReply = networkManager->get(request);
   connect(subStatusReply, &QNetworkReply::finished, this, [this, subStatusReply]() {onsubStatusReply(subStatusReply);});
   updateStatus("Requesting submission. Please wait...");
}

void NebulaTexturesDialog::onsubStatusReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Failed to get submission status:" << reply->errorString();
      updateStatus("Failed to get submission status!");
      reply->deleteLater();
      return;
   }
   QByteArray cont = reply->readAll();
   qDebug()<<cont;
   QJsonDocument doc = QJsonDocument::fromJson(cont);
   QJsonObject json = doc.object();
   QJsonArray jobs = json["jobs"].toArray();

   if (!jobs.isEmpty() && !jobs[0].isNull()) {
      jobId = QString::number(jobs[0].toInt());
      qDebug() << "Job ID:" << jobId;
      subStatusTimer->stop();
      jobStatusTimer->start(2000);
      reply->deleteLater();

      updateStatus("Submission got. Please wait...");
   }

}

void NebulaTexturesDialog::checkJobStatus()
{
   QUrl jobStatusUrl(API_URL + "api/jobs/" + jobId);
   QNetworkRequest request(jobStatusUrl);

   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

   QNetworkReply *jobStatusReply = networkManager->get(request);
   connect(jobStatusReply, &QNetworkReply::finished, this, [this, jobStatusReply]() { onJobStatusReply(jobStatusReply); });
   updateStatus("Requesting job status. Please wait...");
}

void NebulaTexturesDialog::onJobStatusReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Failed to get job status:" << reply->errorString();
      updateStatus("Failed to get job status!");
      reply->deleteLater();
      return;
   }

   QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
   QJsonObject json = doc.object();
   QString status = json["status"].toString();

   qDebug() << "Job status:" << status;

   if (status == "success") {
      qDebug() << "Job completed successfully! Downloading WCS file...";
      updateStatus("Job completed successfully! Downloading WCS file...");
      jobStatusTimer->stop();
      QUrl wcsUrl(API_URL+ "wcs_file/" + jobId);
      QNetworkRequest request(wcsUrl);
      QNetworkReply *wcsReply = networkManager->get(request);
      connect(wcsReply, &QNetworkReply::finished, this, [this, wcsReply]() { onWcsDownloadReply(wcsReply); });

   } else if (status == "error") {
      qDebug() << "Error in job processing.";      
      updateStatus("Error in job processing!");
      jobStatusTimer->stop();
   }

   reply->deleteLater();
}


void NebulaTexturesDialog::onWcsDownloadReply(QNetworkReply *reply)
{
   if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Failed to download WCS file:" << reply->errorString();
      updateStatus("Failed to download WCS file!");
      reply->deleteLater();
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

   qDebug()<<  CRPIX1 << CRPIX2 << CRVAL1 << CRVAL2 << CD1_1 << CD1_2 << CD2_1 << CD2_2;
   qDebug()<< IMAGEW<< IMAGEH;

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

   qDebug()<<"["<<bottomLeftRA<<","<<bottomLeftDec<<"],"
            <<"["<<bottomRightRA<<","<<bottomRightDec<<"],"
            <<"["<<topRightRA<<","<<topRightDec<<"],"
            <<"["<<topLeftRA<<","<<topLeftDec<<"]";

   X = IMAGEW - 1, Y = IMAGEH - 1;
   result = PixelToCelestial(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
   bottomRightRA = result.first;
   bottomRightDec = result.second;
   ui->bottomRightX->setValue(bottomRightRA);
   ui->bottomRightY->setValue(bottomRightDec);

   updateStatus("Goto Reference Point, Try to Render, Check and Adjust Coordinates.");
}

double NebulaTexturesDialog::calculateRA(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
                   double CD1_1, double CD1_2, double CD2_1, double CD2_2) {
   return CRVAL1 + ((X - CRPIX1) * CD1_1 + (Y - CRPIX2) * CD1_2) / cos(CRVAL2 * M_PI / 180.0);
}

double NebulaTexturesDialog::calculateDec(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
                    double CD1_1, double CD1_2, double CD2_1, double CD2_2) {
   return CRVAL2 + ((X - CRPIX1) * CD2_1 + (Y - CRPIX2) * CD2_2);
}


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

void NebulaTexturesDialog::on_renderButton_clicked()
{
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

   // Render logic with the provided coordinates
   QString renderStatus = QString("Rendering with coordinates");

   updateStatus(renderStatus);

   // Example of rendering completion
   QTimer::singleShot(3000, this, [this]() {
      updateStatus("Rendering complete.");
   });
}
