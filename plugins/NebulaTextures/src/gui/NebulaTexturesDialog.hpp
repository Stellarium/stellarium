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

#ifndef NEBULATEXTURESDIALOG_HPP
#define NEBULATEXTURESDIALOG_HPP

#include "StelDialog.hpp"
#include <QString>

class Ui_nebulaTexturesDialog;

//! Main window of the Nebula Textures plug-in.
//! @ingroup nebulaTextures
class NebulaTexturesDialog : public StelDialog
{
	Q_OBJECT

public:
	NebulaTexturesDialog();
	~NebulaTexturesDialog() override;

public slots:
	void retranslate() override;

protected:
	void createDialogContent() override;

private slots:
   void restoreDefaults();

   void on_openFileButton_clicked();
   void on_uploadImageButton_clicked();
   void on_renderButton_clicked();
   void on_goPushButton_clicked();

   void onLoginReply(QNetworkReply *reply);
   void onUploadReply(QNetworkReply *reply);
   void onsubStatusReply(QNetworkReply *reply);
   void onJobStatusReply(QNetworkReply *reply);
   void onWcsDownloadReply(QNetworkReply *reply);

   void checkSubStatus();
   void checkJobStatus();

private:
	Ui_nebulaTexturesDialog* ui;


   QNetworkAccessManager *networkManager;
   QString session;
   QString subId;
   QString jobId;
   QTimer *subStatusTimer;
   QTimer *jobStatusTimer;
   QString API_URL = "http://nova.astrometry.net/";

	void setAboutHtml();
   void updateStatus(const QString &status);

   double calculateRA(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
                                        double CD1_1, double CD1_2, double CD2_1, double CD2_2);

   double calculateDec(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
                                         double CD1_1, double CD1_2, double CD2_1, double CD2_2);

   double CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2;
   int IMAGEW, IMAGEH;
   double topLeftRA, topLeftDec, bottomLeftRA, bottomLeftDec;
   double topRightRA, topRightDec, bottomRightRA, bottomRightDec;
   double referRA, referDec;
};

#endif /* NEBULATEXTURESDIALOG_HPP */
