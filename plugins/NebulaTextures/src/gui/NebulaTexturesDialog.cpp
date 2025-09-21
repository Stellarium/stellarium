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

#define NT_CONFIG_PREFIX QString("NebulaTextures")
#define CUSTOM_TEXNAME QString("Custom Textures")
#define DEFAULT_TEXNAME QString("Nebulae")
#define TEST_TEXNAME QString("Test Textures")

const QString NebulaTexturesDialog::ConfigPrefix = NT_CONFIG_PREFIX;

/**
 * @brief Constructor for the NebulaTexturesDialog class.
 * Initializes the dialog, sets up network management, and configures timers for periodic status checking.
 * Also loads nebula textures on initialization.
 */
NebulaTexturesDialog::NebulaTexturesDialog()
	: StelDialog("NebulaTextures"),
          refreshCount(0), refreshLimit(2),
          conf(StelApp::getInstance().getSettings()),
          progressBar(Q_NULLPTR),
          imagePath_src(""),
          imagePath_dst(""),
          imagePathTemp_src(""), imagePathTemp_dst(""),
          isWcsSolved(false), isTempTextureVisible(false)
{
	ui = new Ui_nebulaTexturesDialog();

	plateSolver = new PlateSolver(this);
	tileManager = new TileManager();
	configManager = new TextureConfigManager(StelFileMgr::getUserDir() + configFile, CUSTOM_TEXNAME);
	tmpCfgManager = new TextureConfigManager(StelFileMgr::getUserDir() + tmpcfgFile, TEST_TEXNAME);

	lastOpenedDirectoryPath = QDir::homePath();
}

/**
 * @brief Destructor for the NebulaTexturesDialog class.
 * Cleans up UI and manager objects.
 */
NebulaTexturesDialog::~NebulaTexturesDialog()
{
	delete ui;
	delete plateSolver;
	delete configManager;
	delete tmpCfgManager;
}

/**
 * @brief Retranslates UI elements in response to a language change.
 */
void NebulaTexturesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

/**
 * @brief Set up the content and interactions of the NebulaTexturesDialog.
 * Configure UI elements, establish signal-slot connections,
 * and load necessary settings and data for the dialog.
 */
void NebulaTexturesDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// load config
	ui->recoverCoordsButton->setVisible(false);
	ui->checkBoxShow->setChecked(getShowCustomTextures());
	ui->checkBoxAvoid->setChecked(getAvoidAreaConflict());
	ui->cancelButton->setVisible(false);
	ui->label_apiKey->setText(QString("<a href=\"https://nova.astrometry.net/api_help\">") + q_("Astrometry API key:") + "</a>");
	ui->brightComboBox->addItem(qc_("Bright", "overlay mode"), QVariant(12));
	ui->brightComboBox->addItem(qc_("Normal", "overlay mode"), QVariant(13.5));
	ui->brightComboBox->addItem(qc_("Dark", "overlay mode"), QVariant(15));
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
	connect(ui->solveButton, SIGNAL(clicked()), this, SLOT(solveImage()));
	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancelSolve()));
	connect(ui->recoverCoordsButton, SIGNAL(clicked()), this, SLOT(recoverSolvedCorners()));
	connect(ui->goPushButton, SIGNAL(clicked()), this, SLOT(moveToCenterCoord()));
	connect(ui->renderButton, SIGNAL(clicked()), this, SLOT(toggleTempTexturePreview()));
	connect(ui->disableDefault, SIGNAL(clicked()), this, SLOT(toggleDefaultTextureVisibility()));
	connect(ui->brightComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshTempTexturePreview()));
	connect(ui->addCustomTextureButton, SIGNAL(clicked()), this, SLOT(addCustomTexture()));

	connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reloadData()));
	connectCheckBox(ui->checkBoxShow,"actionShow_NebulaTextures");
	connect(ui->checkBoxAvoid, SIGNAL(clicked(bool)), this, SLOT(setAvoidAreaConflict(bool)));
	connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(gotoSelectedItem(QListWidgetItem*)));
	connect(ui->removeTextureButton, SIGNAL(clicked()), this, SLOT(removeTexture()));

	QList<QPair<AngleSpinBox*, bool>> allCornerSpins = {
		qMakePair(ui->topLeftX, true), qMakePair(ui->topLeftY, false),
		qMakePair(ui->topRightX, true), qMakePair(ui->topRightY, false),
		qMakePair(ui->bottomLeftX, true), qMakePair(ui->bottomLeftY, false),
		qMakePair(ui->bottomRightX, true), qMakePair(ui->bottomRightY, false)
	};

	for (const auto& pair : allCornerSpins)
	{
		AngleSpinBox* spin = pair.first;
		bool isRA = pair.second;

		if(isRA)
		{
			spin->setDisplayFormat(AngleSpinBox::HMSLetters);
			spin->setPrefixType(AngleSpinBox::Normal);
			spin->setMinimum(0.0, true);
			spin->setMaximum(360.0, true);
		}
		else
		{
			spin->setDisplayFormat(AngleSpinBox::DMSSymbols);
			spin->setPrefixType(AngleSpinBox::NormalPlus);
			spin->setMinimum(-90.0, true);
			spin->setMaximum(90.0, true);
		}
		spin->setWrapping(isRA);
		connect(spin, SIGNAL(valueChanged()), this, SLOT(refreshTempTexturePreview()));
		connect(spin, SIGNAL(valueChanged()), this, SLOT(showRecoverCoordsButton()));
	}
	// Center RA
	ui->referX->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->referX->setPrefixType(AngleSpinBox::Normal);
	ui->referX->setMinimum(0.0, true);
	ui->referX->setMaximum(360.0, true);
	ui->referX->setWrapping(true);

	connect(plateSolver, &PlateSolver::loginSuccess, this, [this]()
	{
		updateStatus(q_("Login success..."));
	});
	connect(plateSolver, &PlateSolver::loginFailed, this, [this](const QString& reason)
	{
		updateStatus(q_("Login failed: %1").arg(reason));
		freezeUiState(false);
	});
	connect(plateSolver, &PlateSolver::uploadSuccess, this, [this]()
	{
		updateStatus(q_("Image uploaded. Please wait..."));
	});
	connect(plateSolver, &PlateSolver::uploadFailed, this, [this](const QString& reason)
	{
		updateStatus(q_("Upload failed: %1").arg(reason));
		freezeUiState(false);
	});
	connect(plateSolver, &PlateSolver::solvingStatusUpdated, this, [this](const QString& status)
	{
		updateStatus(status);
	});
	connect(plateSolver, &PlateSolver::failed, this, [this](const QString& msg)
	{
		updateStatus(q_("Solve failed: %1").arg(msg));
		freezeUiState(false);
	});
	connect(plateSolver, &PlateSolver::solutionAvailable, this, [this](const QString& wcsText)
	{
		updateStatus(q_("World Coordinate System data download complete. Processing..."));
		applyWcsSolution(wcsText);
		freezeUiState(false);
	});

	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
	setAboutHtml();
	reloadData();
	ui->lineEditApiKey->setText(conf->value(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", "").toString());
}

/**
 * @brief Restore all plugin settings to their default values.
 * This includes removing saved configuration files and resetting UI elements.
 */
void NebulaTexturesDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[NebulaTextures] Restore defaults...";
		// GETSTELMODULE(NebulaTextures)->restoreDefaultSettings();

		// deleteImagesFromCfg(configFile);
		tileManager->deleteImagesFromConfig(configManager, pluginDir);

		QString cfgPath = StelFileMgr::getUserDir() + configFile;
		if (QFile::exists(cfgPath))
		{
			if (QFile::remove(cfgPath))
			{
				qDebug() << "[NebulaTextures] Deleted texture config file:" << cfgPath;
			}
			else
			{
				qWarning() << "[NebulaTextures] Failed to delete config file:" << cfgPath;
			}
		}

		// deleteImagesFromCfg(tmpcfgFile);
		tileManager->deleteImagesFromConfig(tmpCfgManager, pluginDir);
		cfgPath = StelFileMgr::getUserDir() + tmpcfgFile;
		if (QFile::exists(cfgPath))
		{
			if (QFile::remove(cfgPath))
			{
				qDebug() << "[NebulaTextures] Deleted temporary texture config file:" << cfgPath;
			}
			else
			{
				qWarning() << "[NebulaTextures] Failed to delete temporary config file:" << cfgPath;
			}
		}

		setShowCustomTextures(false);
		setAvoidAreaConflict(false);

		ui->lineEditApiKey->setText("");
		conf->setValue(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", "");
		ui->lineEditImagePath->setText("");

		ui->referX->setDegrees(0);
		ui->referY->setDegrees(0);
		ui->topLeftX->setDegrees(0);
		ui->topLeftY->setDegrees(0);
		ui->bottomLeftX->setDegrees(0);
		ui->bottomLeftY->setDegrees(0);
		ui->topRightX->setDegrees(0);
		ui->topRightY->setDegrees(0);
		ui->bottomRightX->setDegrees(0);
		ui->bottomRightY->setDegrees(0);

		ui->listWidget->clear();
	}
	else
		qDebug() << "[NebulaTextures] Restore defaults is canceled...";
}

/**
 * @brief Set the HTML content for the "About" section in the dialog.
 */
void NebulaTexturesDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";

	html += "<h2>" + q_("Nebula Textures Plugin") + "</h2><table class='layout' width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + NEBULATEXTURES_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>WANG Siliang</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Nebula Textures plugin enhances your Stellarium experience by allowing you to customize and render deep-sky object (DSO) textures. "
					   "Whether you're an astronomy enthusiast with astrophotography or astronomical sketches and paintings, this plugin lets you integrate your own images into Stellarium. "
					   "It also supports online plate-solving via Astrometry.net for precise positioning of astronomical images. "
					   "Simply upload your image (ensure it is not flipped horizontally or vertically), solve its coordinates, and enjoy a personalized celestial view!") + "</p>";

	html += "<p>" + q_("See User Guide for details. Plugin data is located in:") + "<br>" + StelFileMgr::getUserDir() + pluginDir + "</br>" + "</p>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("Nebula Textures Plugin");

	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}


/**
 * @brief Toggle the enabled/disabled state of UI components.
 * @param freeze  A boolean indicating whether to disable (true) or enable (false) the UI.
 */
void NebulaTexturesDialog::freezeUiState(bool freeze)
{
	ui->openFileButton->setDisabled(freeze);
	ui->solveButton->setDisabled(freeze);
	ui->goPushButton->setDisabled(freeze);
	ui->renderButton->setDisabled(freeze);
	ui->addCustomTextureButton->setDisabled(freeze);
	ui->cancelButton->setVisible(freeze);
}

/**
 * @brief Update the status message shown in the dialog.
 * @param status The message to be displayed in the status text.
 */
void NebulaTexturesDialog::updateStatus(const QString &status)
{
	ui->statusText->setText(status);
}

/**
 * @brief Trigger a refresh of textures if the refresh count is within the allowed limit.
 */
void NebulaTexturesDialog::initializeRefreshIfNeeded()
{
	if(refreshCount < refreshLimit)
	{
		refreshTextures();
		refreshCount++;
	}
}

/**
 * @brief Open a file dialog to allow the user to select an image file.
 */
void NebulaTexturesDialog::openImageFile()
{
	QString fileName = QFileDialog::getOpenFileName(&StelMainView::getInstance(), q_("Open Image"), lastOpenedDirectoryPath, q_("Images (*.png *.jpg *.gif *.tif *.tiff *.jpeg)"));
	if (!fileName.isEmpty())
	{
		ui->lineEditImagePath->setText(fileName);
		lastOpenedDirectoryPath =  QFileInfo(fileName).path();
		updateStatus(q_("File selected: %1").arg(fileName));
	}
}

/**
 * @brief Cancel the current plate solving operation.
 */
void NebulaTexturesDialog::cancelSolve()
{
	if (plateSolver)
	{
		plateSolver->cancel();
	}
	updateStatus(q_("Operation cancelled by user."));
	freezeUiState(false);
}

/**
 * @brief Start solving the selected image via Astrometry.net.
 * This sends the image to the API and handles the plate solving process.
 * @warning The image should not be flipped horizontally or vertically.
 */
void NebulaTexturesDialog::solveImage() // WARN: image should not be flip
{
	QString apiKey = ui->lineEditApiKey->text();
	QString imagePath = ui->lineEditImagePath->text();

	if (imagePath.isEmpty() || apiKey.isEmpty())
	{
		updateStatus(q_("Please provide both API key and image path."));
		return;
	}

	QFileInfo fileInfo(imagePath);
	if (!fileInfo.exists() || !fileInfo.isReadable())
	{
		updateStatus(q_("The specified image file does not exist or is not readable."));
		return;
	}

	QString suffix = fileInfo.suffix().toLower();
	QStringList allowedSuffixes = {"png", "jpg", "gif", "tif", "tiff", "jpeg"};
	if (!allowedSuffixes.contains(suffix))
	{
		updateStatus(q_("Invalid image format. Supported formats: PNG, JPEG, GIF, TIFF."));
		return;
	}

	if (ui->checkBoxKeepApi->isChecked())
		conf->setValue(NT_CONFIG_PREFIX + "/AstroMetry_Apikey", apiKey);

	updateStatus(q_("Sending requests..."));
	freezeUiState(true);

	plateSolver->startPlateSolving(apiKey, imagePath);
	return;
}

/**
 * @brief Apply the WCS (World Coordinate System) solution to the UI and internal state.
 * @param wcsText The WCS data string returned from Astrometry.net.
 */
void NebulaTexturesDialog::applyWcsSolution(const QString& wcsText)
{

	WcsResult wcs = PlateSolver::parseWcsText(wcsText);
	if (!wcs.valid)
	{
		updateStatus(q_("Invalid WCS data."));
		return;
	}

	CRPIX1 = wcs.CRPIX1;
	CRPIX2 = wcs.CRPIX2;
	CRVAL1 = wcs.CRVAL1;
	CRVAL2 = wcs.CRVAL2;
	CD1_1 = wcs.CD1_1;
	CD1_2 = wcs.CD1_2;
	CD2_1 = wcs.CD2_1;
	CD2_2 = wcs.CD2_2;
	imageWidth = wcs.IMAGEW;
	imageHeight = wcs.IMAGEH;
	centerRA = CRVAL1;
	centerDec = CRVAL2;
	ui->referX->setDegrees(CRVAL1);
	ui->referY->setDegrees(CRVAL2);

	QPair<double, double> result;
	int X=0,Y=0;
	result = SkyCoords::pixelToRaDec(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	topLeftRA = result.first;
	topLeftDec = result.second;
	ui->topLeftX->setDegrees(topLeftRA);
	ui->topLeftY->setDegrees(topLeftDec);

	X = 0, Y = imageHeight - 1;
	result = SkyCoords::pixelToRaDec(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	bottomLeftRA = result.first;
	bottomLeftDec = result.second;
	ui->bottomLeftX->setDegrees(bottomLeftRA);
	ui->bottomLeftY->setDegrees(bottomLeftDec);

	X = imageWidth - 1, Y = 0;
	result = SkyCoords::pixelToRaDec(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	topRightRA = result.first;
	topRightDec = result.second;
	ui->topRightX->setDegrees(topRightRA);
	ui->topRightY->setDegrees(topRightDec);

	X = imageWidth - 1, Y = imageHeight - 1;
	result = SkyCoords::pixelToRaDec(X, Y, CRPIX1, CRPIX2, CRVAL1, CRVAL2, CD1_1, CD1_2, CD2_1, CD2_2);
	bottomRightRA = result.first;
	bottomRightDec = result.second;
	ui->bottomRightX->setDegrees(bottomRightRA);
	ui->bottomRightY->setDegrees(bottomRightDec);

	isWcsSolved = true;
	freezeUiState(false);
	updateStatus(q_("Processing completed! Goto Center Point, Test this texture, and Add to Custom Storage."));
}

/**
 * @brief Moves the view to the coordinates specified in the "Center Coord" input fields.
 *
 * Uses Right Ascension and Declination (RA/Dec) values to calculate a J2000 position
 * and moves the Stellarium viewport to it.
 */
void NebulaTexturesDialog::moveToCenterCoord()
{
	centerRA = ui->referX->valueDegrees();
	centerDec = ui->referY->valueDegrees();
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Vec3d pos;
	double spinLong=centerRA/180.*M_PI;
	double spinLat=centerDec/180.*M_PI;

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

/**
 * @brief Restores the previously solved WCS corner coordinates to the UI fields.
 *
 * Only effective if WCS solving has been done.
 * Prompts the user for confirmation before overwriting current values.
 */
void NebulaTexturesDialog::recoverSolvedCorners()
{
	if(isWcsSolved && askConfirmation(q_("Are you sure to recover the solution?")))
	{
		ui->topLeftX->setDegrees(topLeftRA);
		ui->topLeftY->setDegrees(topLeftDec);

		ui->bottomLeftX->setDegrees(bottomLeftRA);
		ui->bottomLeftY->setDegrees(bottomLeftDec);

		ui->topRightX->setDegrees(topRightRA);
		ui->topRightY->setDegrees(topRightDec);

		ui->bottomRightX->setDegrees(bottomRightRA);
		ui->bottomRightY->setDegrees(bottomRightDec);

		if(isTempTextureVisible)
			showTempTexturePreview();
	}
}

/**
 * @brief Toggles the visibility of the temporary texture preview.
 *
 * If a temporary texture is already shown, it will be removed.
 * Otherwise, the currently selected image will be rendered on the sky.
 */
void NebulaTexturesDialog::toggleTempTexturePreview()
{
	if (isTempTextureVisible)
		removeTempTexturePreview();
	else
		showTempTexturePreview();

	ui->renderButton->setChecked(isTempTextureVisible);

	if(isTempTextureVisible)
		ui->renderButton->setText(q_("Stop test"));
	else
		ui->renderButton->setText(q_("Test this texture"));
}

/**
 * @brief Toggles the visibility of the default texture layer when a temporary texture is shown.
 *
 * Only takes effect if a temporary texture is currently visible.
 */
void NebulaTexturesDialog::toggleDefaultTextureVisibility()
{
	if(!isTempTextureVisible)
		return;
	if(ui->disableDefault->isChecked())
		tileManager->setTileVisible(DEFAULT_TEXNAME, false);
	else
	{
		tileManager->setTileVisible(DEFAULT_TEXNAME, true);
		refreshTextures();
	}
}

/**
 * @brief Refreshes and re-renders the temporary texture when the brightness level is changed.
 *
 * This is triggered by a brightness selection combo box.
 * @param index The index of the selected brightness level.
 */
void NebulaTexturesDialog::updateBrightnessLevel(int index)
{
	if(isTempTextureVisible)
		showTempTexturePreview();
}

/**
 * @brief Loads and displays the currently selected image as a temporary texture.
 *
 * Copies the selected image (if needed), updates temporary config, and renders it.
 * May hide the default texture if requested.
 */
void NebulaTexturesDialog::showTempTexturePreview()
{
	QString imagePath = ui->lineEditImagePath->text();

	// clean outtime image if select new
	if (imagePath != imagePathTemp_src)
	{
		QString copiedName = ensureImageCopied(imagePath, TEST_TEXNAME);
		if (copiedName.isEmpty()) return;  // error

		tileManager->deleteImagesFromConfig(tmpCfgManager, pluginDir);
	}

	// add test texture config entry
	addTexture(tmpcfgFile, TEST_TEXNAME);

	// load test texture from config
	if (!tileManager->insertTileFromConfig(tmpcfgFile, TEST_TEXNAME, true))
	{
		qWarning() << "[NebulaTextures] Failed to load temp texture from config.";
		return;
	}

	isTempTextureVisible = true;

	// optionally hide default texture if requested
	if (ui->disableDefault->isChecked())
	{
		tileManager->setTileVisible(DEFAULT_TEXNAME, false);
	}
}

/**
 * @brief Re-renders the temporary texture using the latest coordinate and brightness settings.
 *
 * Only takes effect if a temporary texture is currently visible.
 */
void NebulaTexturesDialog::refreshTempTexturePreview()
{
	if(isTempTextureVisible)
	{
		QString path = StelFileMgr::getUserDir()+tmpcfgFile;
		// deleteImagesFromCfg(tmpcfgFile);
		addTexture(tmpcfgFile, TEST_TEXNAME);

		if (!tileManager->insertTileFromConfig(tmpcfgFile, TEST_TEXNAME, true))
		{
			qWarning() << "[NebulaTextures] Failed to update temp texture from config.";
		}
	}
}

/**
 * @brief Displays the "Recover Coordinates" button if WCS solving has succeeded.
 */
void NebulaTexturesDialog::showRecoverCoordsButton()
{
	if(isWcsSolved)
		ui->recoverCoordsButton->setVisible(true);
}

/**
 * @brief Removes the temporary texture from the sky.
 *
 * Restores visibility of the default texture and refreshes the texture layers.
 */
void NebulaTexturesDialog::removeTempTexturePreview()
{
	if(!isTempTextureVisible)
		return;
	tileManager->removeTile(TEST_TEXNAME);
	isTempTextureVisible = false;
	tileManager->setTileVisible(DEFAULT_TEXNAME, true);
	refreshTextures();
}

/**
 * @brief Adds the current image and coordinates as a custom texture to config.
 *
 * Prompts for user confirmation before modifying persistent configuration.
 * Changes only take effect after restarting Stellarium.
 */
void NebulaTexturesDialog::addCustomTexture()
{
	if(!askConfirmation(q_("Caution! Are you sure to add this texture? It will only take effect after restarting Stellarium.")))
		return;

	addTexture(configFile, CUSTOM_TEXNAME);
}

/**
 * @brief Adds a texture entry to the given configuration file.
 *
 * Copies the image, builds coordinate data, and writes it to a JSON config manager.
 *
 * @param cfgPath Path to the JSON configuration file (relative to user dir).
 * @param groupName Texture group name (e.g., test or custom).
 */
void NebulaTexturesDialog::addTexture(QString cfgPath, QString groupName)
{
	QString imagePath = ui->lineEditImagePath->text();
	QString imageUrl = ensureImageCopied(imagePath, groupName);
	if (imageUrl.isEmpty()) return;

	QJsonArray coords;
	coords.append(QJsonArray({ ui->bottomLeftX->valueDegrees(), ui->bottomLeftY->valueDegrees() }));
	coords.append(QJsonArray({ ui->bottomRightX->valueDegrees(), ui->bottomRightY->valueDegrees() }));
	coords.append(QJsonArray({ ui->topRightX->valueDegrees(), ui->topRightY->valueDegrees() }));
	coords.append(QJsonArray({ ui->topLeftX->valueDegrees(), ui->topLeftY->valueDegrees() }));

	double brightness = ui->brightComboBox->currentData().toDouble();

	TextureConfigManager* manager = (groupName == TEST_TEXNAME) ? tmpCfgManager : configManager;
	if (!manager->load())
	{
		updateStatus(q_("Failed to load texture configuration."));
		return;
	}

	if (groupName == TEST_TEXNAME)
		manager->addSubTileOverwrite(imageUrl, coords, 0.2, brightness);
	else
		manager->addSubTile(imageUrl, coords, 0.2, brightness);

	manager->save();
	updateStatus(q_("Imported custom textures successfully!"));
}

/**
 * @brief Copies the image to the plugin directory with a unique timestamped filename.
 *
 * Also updates the internal tracking path for source and destination, depending on texture type.
 *
 * @param imagePath Path to the original image.
 * @param groupName Texture group name (test or custom).
 * @return The relative path (URL) to the copied image within the plugin folder, or empty if failed.
 */
QString NebulaTexturesDialog::ensureImageCopied(const QString& imagePath, const QString& groupName)
{
	if (imagePath.isEmpty())
	{
		updateStatus(q_("Image path is empty."));
		return "";
	}

	bool alreadyCopied = (groupName == CUSTOM_TEXNAME && imagePath == imagePath_src)
					  || (groupName == TEST_TEXNAME && imagePath == imagePathTemp_src);
	if (alreadyCopied)
		return (groupName == TEST_TEXNAME) ? imagePathTemp_dst : imagePath_dst;

	QFileInfo fileInfo(imagePath);
	if (!fileInfo.exists() || !fileInfo.isReadable())
	{
		updateStatus(q_("Image file invalid or unreadable."));
		return "";
	}

	QString suffix = fileInfo.suffix().toLower();
	QStringList allowed = {"png", "jpg", "gif", "tif", "tiff", "jpeg"};
	if (!allowed.contains(suffix))
	{
		updateStatus(q_("Invalid image format."));
		return "";
	}

	QString baseName = fileInfo.completeBaseName();
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
	QString imageUrl = QString("%1_%2.%3").arg(baseName, timestamp, suffix);
	QString targetPath = StelFileMgr::getUserDir() + pluginDir + imageUrl;

	QDir().mkpath(StelFileMgr::getUserDir() + pluginDir);
	if (!QFile::copy(imagePath, targetPath))
	{
		updateStatus(q_("Failed to copy image file."));
		return "";
	}

	// update path log
	if (groupName == TEST_TEXNAME)
	{
		imagePathTemp_src = imagePath;
		imagePathTemp_dst = imageUrl;
	}
	else
	{
		imagePath_src = imagePath;
		imagePath_dst = imageUrl;
	}

	return imageUrl;
}

/**
 * @brief Removes the currently selected texture from the config and deletes its image file.
 *
 * Prompts the user for confirmation. If successful, removes from both config and UI list.
 */
void NebulaTexturesDialog::removeTexture()
{
	QListWidgetItem* selectedItem = ui->listWidget->currentItem();
	if (!selectedItem) return;

	QString imageUrl = selectedItem->text();
	if (!askConfirmation(q_("Are you sure to remove this texture?"))) return;

	if (!configManager->load()) return;

	if (configManager->removeSubTileByImageUrl(imageUrl))
	{
		QString imgPath = StelFileMgr::getUserDir() + pluginDir + imageUrl;
		if (QFile::exists(imgPath))
			QFile::remove(imgPath);

		configManager->save();
		delete selectedItem;
		updateStatus(q_("Texture removed."));
	}
}

/**
 * @brief Reloads all custom texture entries and populates the texture list widget.
 */
void NebulaTexturesDialog::reloadData()
{	
	refreshTextures();

	if (!configManager->load())
	{
		ui->listWidget->clear();
		return;
	}

	ui->listWidget->clear();
	QJsonArray tiles = configManager->getSubTiles();

	for (const QJsonValue& tile : tiles)
	{
		QJsonObject obj = tile.toObject();
		QString url = obj["imageUrl"].toString();
		QListWidgetItem* item = new QListWidgetItem(url, ui->listWidget);
		item->setData(Qt::UserRole, obj);
		ui->listWidget->addItem(item);
	}
}

/**
 * @brief Sets the visibility of the custom and default textures based on current UI state.
 *
 * If "show custom" is disabled, only the default texture is shown.
 * If area conflict avoidance is enabled, conflicts will be resolved.
 */
void NebulaTexturesDialog::refreshTextures()
{
	bool showCustom = getShowCustomTextures();
	if (!showCustom)
	{
		tileManager->setTileVisible(CUSTOM_TEXNAME,false);
		tileManager->setTileVisible(DEFAULT_TEXNAME,true);
	}
	else
	{
		tileManager->setTileVisible(CUSTOM_TEXNAME,true);
		if (getAvoidAreaConflict())
			tileManager->resolveConflicts(DEFAULT_TEXNAME, CUSTOM_TEXNAME);
		else
			tileManager->setTileVisible(DEFAULT_TEXNAME,true);
	}
}

/**
 * @brief Moves the view to center on the texture selected in the list widget.
 *
 * Extracts RA/Dec from the texture's stored world coordinates, and navigates to it.
 *
 * @param item The selected list item representing a custom texture.
 */
void NebulaTexturesDialog::gotoSelectedItem(QListWidgetItem* item)
{
	if (!item) return;

	QString imageUrl = item->text();

	if (!configManager->load())
	{
		qWarning() << "[NebulaTextures] Failed to load config for selection.";
		return;
	}

	QJsonObject subTile = configManager->getSubTileByImageUrl(imageUrl);
	if (subTile.isEmpty() || !subTile.contains("worldCoords"))
		return;

	QJsonArray worldCoords = subTile["worldCoords"].toArray();
	if (worldCoords.size() != 1 || !worldCoords[0].isArray())
	{
		qWarning() << "[NebulaTextures] Invalid or missing worldCoords.";
		return;
	}

	QJsonArray corners = worldCoords[0].toArray();
	if (corners.size() != 4)
	{
		qWarning() << "[NebulaTextures] Invalid corner count.";
		return;
	}

	// calculate center
	QPair<double, double> center = SkyCoords::calculateCenter(corners);
	double centerRA = center.first;
	double centerDec = center.second;
	double spinLong = D2R * centerRA;
	double spinLat = D2R * centerDec;

	// move View port
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Vec3d pos;

	mvmgr->setViewUpVector(Vec3d(0., 0., 1.));
	Vec3d aimUp = mvmgr->getViewUpVectorJ2000();
	if (mvmgr->getMountMode() == StelMovementMgr::MountEquinoxEquatorial && fabs(spinLat) > 0.9 * M_PI_2)
	{
		mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat > 0. ? 1. : -1.));
		aimUp = mvmgr->getViewUpVectorJ2000();
	}

	StelUtils::spheToRect(spinLong, spinLat, pos);
	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, aimUp, mvmgr->getAutoMoveDuration());
}
