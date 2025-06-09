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
#include "TextureConfigManager.hpp"
#include "TileManager.hpp"
#include "SkyCoords.hpp"

class Ui_nebulaTexturesDialog;

//! Main window of the Nebula Textures plug-in.
//! @ingroup nebulaTextures
class NebulaTexturesDialog : public StelDialog
{
	Q_OBJECT

public:
	static const QString ConfigPrefix;

	//! Constructor
	NebulaTexturesDialog();

	//! Destructor
	~NebulaTexturesDialog() override;

	//! Only trigger refresh when the textures are initially loaded on screen
	//! (including both default and custom, twice) to avoid conflicts
	int refreshCount, refreshLimit;

public slots:
	//! Retranslate UI components based on current language settings
	void retranslate() override;

	//! Refresh textures when they are painted on screen
	void initializeRefreshIfNeeded();

	//! Reload texture configuration data from disk
	void reloadData();

	//! Refresh textures and manage their visibility
	void refreshTextures();

	//! Get the value for showing custom textures
	bool getShowCustomTextures() { return conf->value(ConfigPrefix + "/showCustomTextures", false).toBool(); }

	//! Set the value for showing custom textures
	void setShowCustomTextures(bool b) { conf->setValue(ConfigPrefix + "/showCustomTextures", b); }

	//! Get the value for avoiding area conflicts
	bool getAvoidAreaConflict() { return conf->value(ConfigPrefix + "/avoidAreaConflict", false).toBool(); }

	//! Set the value for avoiding area conflicts and refresh textures
	void setAvoidAreaConflict(bool b) { conf->setValue(ConfigPrefix + "/avoidAreaConflict", b); refreshTextures(); }

protected:
	//! Set up the content and interactions of the NebulaTexturesDialog
	void createDialogContent() override;

private slots:
	//! Restore default settings for the dialog
	void restoreDefaults();

	//! Open a file dialog to select an image file
	void openImageFile();

	//! Solve an image online using plate-solving
	void solveImage();

	//! Cancel the image solving process
	void cancelSolve();

	//! Recover image coordinates from plate-solving solution
	void recoverSolvedCorners();

	//! Move view to the center coordinates (RA, Dec) of the texture
	void moveToCenterCoord();

	//! Toggle temporary texture render testing for debugging
	void toggleTempTexturePreview();

	//! Toggle visibility of the default texture when testing temporary texture rendering
	void toggleDefaultTextureVisibility();

	//! Update brightness level of the texture rendering
	void updateBrightnessLevel(int index);

	//! Add the texture to the custom textures configuration
	void addCustomTexture();

	//! Refresh temporary texture preview for debugging
	void refreshTempTexturePreview();

	//! Update temporary texture rendering for debugging
	void showRecoverCoordsButton();

	//! Move view to the center of the selected image in the list view
	void gotoSelectedItem(QListWidgetItem* item);

	//! Remove the selected texture from the list and delete the associated image file
	void removeTexture();

private:
	//! UI components for the dialog
	Ui_nebulaTexturesDialog* ui;

	//! Pointer to the QSettings used for storing configuration
	QSettings* conf;

	//! Progress bar controller for long operations
	class StelProgressController* progressBar;

	//! Control image copying when adding texture, avoiding frequent file I/O
	QString imagePath_src, imagePath_dst, imagePathTemp_src, imagePathTemp_dst;

	//! Plate solver for solving image coordinates
	PlateSolver* plateSolver;

	//! Tile manager for handling texture tiles
	TileManager* tileManager;

	//! Configuration manager for custom textures
	TextureConfigManager* configManager;

	//! Temporary configuration manager for temporary textures
	TextureConfigManager* tmpCfgManager;

	//! Flag for online plate-solving
	bool isWcsSolved;

	//! Calibration parameters for the image (CRPIX, CRVAL, CD values)
	double CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2;

	//! Image width and height values
	int imageWidth, imageHeight;

	//! Right Ascension and Declination for the four corners of the images (by plate-solved)
	double topLeftRA, topLeftDec, bottomLeftRA, bottomLeftDec;
	double topRightRA, topRightDec, bottomRightRA, bottomRightDec;

	//! Reference Right Ascension and Declination for the image center
	double centerRA, centerDec;

	//! Directory path where plugin-related files are stored
	QString pluginDir = "/modules/NebulaTextures/";

	//! Path to the custom textures configuration file
	QString configFile = "/modules/NebulaTextures/custom_textures.json";

	//! Path to the temporary textures configuration file
	QString tmpcfgFile = "/modules/NebulaTextures/temp_textures.json";

	//! Flag indicating whether to render temporary textures
	bool isTempTextureVisible;

	//! Path to the directory last used by QFileDialog in openImageFile()
	QString lastOpenedDirectoryPath;

	//! Process WCS (World Coordinate System) of the image
	void applyWcsSolution(const QString& wcsText);

	//! Add the texture to configuration, specifying the texture group name
	void addTexture(QString cfgPath, QString groupName);

	//! Ensure the image is copied correctly before adding to configuration
	QString ensureImageCopied(const QString& imagePath, const QString& groupName);

	//! Show temporary texture preview for debugging
	void showTempTexturePreview();

	//! Remove temporary texture preview for debugging
	void removeTempTexturePreview();

	//! Set the HTML content for the "About" section in the dialog
	void setAboutHtml();

	//! Update the status text displayed in the UI
	void updateStatus(const QString& status);

	//! Freeze or unfreeze the UI components for interaction
	void freezeUiState(bool freeze);
};

#endif /* NEBULATEXTURESDIALOG_HPP */
