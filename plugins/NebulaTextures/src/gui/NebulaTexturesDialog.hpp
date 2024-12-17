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
#include <QPair>

#include "NebulaTextures.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelApp.hpp"

#define MS_CONFIG_PREFIX QString("NebulaTextures")
#define CUSTOM_TEXNAME QString("Custom Textures")
#define DEFAULT_TEXNAME QString("Nebulae")
#define TEST_TEXNAME QString("Test Textures")

class Ui_nebulaTexturesDialog;

//! Main window of the Nebula Textures plug-in.
//! @ingroup nebulaTextures
class NebulaTexturesDialog : public StelDialog
{
	Q_OBJECT

public:
	NebulaTexturesDialog();
	~NebulaTexturesDialog() override;

   StelSkyImageTile* get_aTile(QString key);

public slots:
	void retranslate() override;


   QPair<double, double> PixelToCelestial(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2,
                                          double CD1_1, double CD1_2, double CD2_1, double CD2_2);

   void renderTempCustomTexture();
   void unRenderTempCustomTexture();

   void addTexture(QString addPath, QString keyName);
   void updateCustomTextures(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness, QString keyName, QString addPath);

   void reloadTextures();
   void avoidConflict();

protected:
	void createDialogContent() override;

private slots:
   void restoreDefaults();

   void on_openFileButton_clicked();
   void on_uploadImageButton_clicked();
   void on_goPushButton_clicked();
   void on_addTexture_clicked();
   void on_removeButton_clicked();

   void onLoginReply(QNetworkReply *reply);
   void onUploadReply(QNetworkReply *reply);
   void onsubStatusReply(QNetworkReply *reply);
   void onJobStatusReply(QNetworkReply *reply);
   void onWcsDownloadReply(QNetworkReply *reply);

   void checkSubStatus();
   void checkJobStatus();

   bool getShowCustomTextures();
   void setShowCustomTextures(bool b);
   bool getAvoidAreaConflict();
   void setAvoidAreaConflict(bool b);

   void reloadData();

private:
	Ui_nebulaTexturesDialog* ui;
   QSettings* m_conf;

   QString API_URL = "http://nova.astrometry.net/";
   QNetworkAccessManager *networkManager;
   QString session;
   QString subId;
   QString jobId;
   QTimer *subStatusTimer;
   QTimer *jobStatusTimer;

   double CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2;
   int IMAGEW, IMAGEH;
   double topLeftRA, topLeftDec, bottomLeftRA, bottomLeftDec;
   double topRightRA, topRightDec, bottomRightRA, bottomRightDec;
   double referRA, referDec;

   QString pluginDir  = "/modules/NebulaTextures/";
   QString configFile = "/modules/NebulaTextures/custom_textures.json";
   QString tmpcfgFile = "/modules/NebulaTextures/temp_textures.json";

   bool flag_renderTempTex = false;

   void setAboutHtml();
   void updateStatus(const QString &status);

};

#endif /* NEBULATEXTURESDIALOG_HPP */
