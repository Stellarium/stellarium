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

#ifndef NEBULATEXTURESDIALOG_HPP
#define NEBULATEXTURESDIALOG_HPP

#include "StelDialog.hpp"
#include <QString>
#include <QPair>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QListWidgetItem>

#include "NebulaTextures.hpp"
#include "StelSkyLayerMgr.hpp"
#include "StelApp.hpp"
#include "PlateSolver.hpp"

#define NT_CONFIG_PREFIX QString("NebulaTextures")
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
	//! Constructor
	NebulaTexturesDialog();

	//! Destructor
	~NebulaTexturesDialog() override;

	// Only trigger refresh when the textures are initially loaded on screen (including both default and custom, twice) to avoid conflicts
	int countRefresh, maxCountRefresh;

	//! Get a StelSkyImageTile texture
	StelSkyImageTile* get_aTile(QString key);

	//! Convert pixel coordinates on the image to celestial coordinates with WCS parameters
	QPair<double, double> PixelToCelestial(int X, int Y, double CRPIX1, double CRPIX2, double CRVAL1, double CRVAL2, double CD1_1, double CD1_2, double CD2_1, double CD2_2);

	//! Set the visibility of the specified texture and its sub-tiles
	bool setTexturesVisible(QString TexName, bool visible);

	//! Register the texture to the custom textures configuration
	void registerTexture(const QString& imageUrl, const QJsonArray& worldCoords, double minResolution, double maxBrightness, QString cfgPath, QString groupName);

	//! Delete the images listed in the given configuration file
	void deleteImagesFromCfg(const QString& cfgFilePath);

	//! Avoid conflicts between custom and default textures by hiding overlapping tiles
	void avoidConflict();

public slots:
	void retranslate() override;

	//! refresh when textures painted on screen
	void refreshInit();


	//! Reload texture configuration data
	void reloadData();
	//! Refresh textures and manage their visibility
	void refreshTextures();

	//! Get the value for showing custom textures
	bool getShowCustomTextures();
	//! Set the value for showing custom textures
	void setShowCustomTextures(bool b);

	//! Get the value for avoiding area conflicts
	bool getAvoidAreaConflict();
	//! Set the value for avoiding area conflicts
	void setAvoidAreaConflict(bool b);

protected:
	//! Set up the content and interactions of the NebulaTexturesDialog
	void createDialogContent() override;

private slots:
	void restoreDefaults();


	//! Open a file dialog to select an image file
	void openImageFile();

	//! Solve an image online
	void solveImage();

	//! Cancel image solving
	void cancelSolve();

	//! Recover image coords from plate-solving
	void recoverCoords();

	//! Goto the center coordinates (RA and Dec) of texture
	void goPush();

	//! toggle temporary texture render testing
	void toggleTempTextureRendering();

	//! toggle showing default texture when testing temporary texture rendering
	void toggleDisableDefaultTexture();

	//! toggle brightness change to render again
	void onBrightnessChanged(int index);

	//! Add the texture to the custom textures configuration
	void addCustomTexture();

	//! update temporary texture rendering for debugging
	void updateTempCustomTexture(double inf);

	//! Goto center of the selected image in list view
	void gotoSelectedItem(QListWidgetItem* item);

	//! Remove the selected texture from the list and deletes the associated image file
	void removeTexture();

private:
	Ui_nebulaTexturesDialog* ui;

	// Pointer to QSettings
	QSettings* conf;

	class StelProgressController* progressBar;


	// Control image copying when adding texture, avoiding frequent file io
	QString imagePath_src, imagePath_dst, imagePathTemp_src, imagePathTemp_dst;

	PlateSolver* plateSolver = Q_NULLPTR;

	// Flag for online plate-solving
	bool solvedFlag;

	// Calibration parameters for the image (CRPIX, CRVAL, CD values)
	double CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2;

	// Image width and height values
	int IMAGEW, IMAGEH;

	// Right Ascension and Declination for the four corners of the images (by plate-solved.)
	double topLeftRA, topLeftDec, bottomLeftRA, bottomLeftDec;
	double topRightRA, topRightDec, bottomRightRA, bottomRightDec;

	// Reference Right Ascension and Declination for the image center
	double referRA, referDec;

	// Directory path where plugin-related files are stored
	QString pluginDir  = "/modules/NebulaTextures/";

	// Path to the custom textures configuration file
	QString configFile = "/modules/NebulaTextures/custom_textures.json";

	// Path to the temporary textures configuration file
	QString tmpcfgFile = "/modules/NebulaTextures/temp_textures.json";

	// Flag indicating whether to render temporary textures
	bool flag_renderTempTex = false;



	//! Process WCS of image
	void processWcsContent(const QString& wcsText);

	//! Add the texture to configuration, need to specify the texture groupname
	void addTexture(QString cfgPath, QString groupName);

	//! Temporary texture rendering for debugging
	void renderTempCustomTexture();
	//! Cancel temporary texture rendering for debugging
	void unRenderTempCustomTexture();

	//! Set the HTML content for the "About" section in the dialog
	void setAboutHtml();

	//! Update the status text displayed in the UI
	void updateStatus(const QString &status);

	//! Toggle the enabled/disabled freezing state of UI components
	void freezeUiState(bool freeze);
};

#endif /* NEBULATEXTURESDIALOG_HPP */
