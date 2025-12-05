/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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

#include "ScmConstellationDialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelObjectMgr.hpp"
#include "types/DrawingMode.hpp"
#include "ui_scmConstellationDialog.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>

ScmConstellationDialog::ScmConstellationDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmConstellationDialog")
	, maker(maker)
{
	assert(maker != nullptr);

	ui        = new Ui_scmConstellationDialog;
	imageItem = new ScmConstellationImage;
}

ScmConstellationDialog::~ScmConstellationDialog()
{
	delete ui;
	qDebug() << "SkyCultureMaker: Unloaded the ScmConstellationDialog";
}

void ScmConstellationDialog::loadFromConstellation(scm::ScmConstellation *constellation)
{
	if (constellation == nullptr)
	{
		qWarning() << "ScmConstellationDialog::loadFromConstellation: constellation is null";
		return;
	}

	if (!isDialogInitialized)
	{
		createDialogContent();
	}
	else
	{
		resetDialog();
	}
	setIsDarkConstellation(constellation->getIsDarkConstellation());

	// Save the constellation that is currently being edited
	constellationBeingEdited = constellation;

	constellationId            = constellation->getId();
	constellationEnglishName   = constellation->getEnglishName();
	constellationPlaceholderId = constellation->getId();
	constellationNativeName    = constellation->getNativeName();
	constellationPronounce     = constellation->getPronounce();
	constellationIPA           = constellation->getIPA();
	constellationDescription   = constellation->getDescription();

	ui->enNameLE->setText(constellationEnglishName);
	ui->idLE->setText(constellationId);
	ui->natNameLE->setText(constellationNativeName.value_or(""));
	ui->pronounceLE->setText(constellationPronounce.value_or(""));
	ui->ipaLE->setText(constellationIPA.value_or(""));
	ui->description->setText(constellationDescription);

	// Hide the original constellation while editing
	constellation->isHidden = true;
	// Load the lines to ScmDraw
	maker->getScmDraw()->loadLines(constellation->getLines());

	// Loads the artwork
	imageItem->setArtwork(constellation->getArtwork());
	ui->artwork_image->centerOn(imageItem);
	ui->artwork_image->fitInView(imageItem, Qt::KeepAspectRatio);
	ui->artwork_image->show();

	updateArtwork();
}

void ScmConstellationDialog::setIsDarkConstellation(bool isDark)
{
	// make sure the dialog is initialized to avoid any ui null pointers
	if (!isDialogInitialized)
	{
		createDialogContent();
	}

	scm::ScmDraw *draw = maker->getScmDraw();
	if (draw == nullptr)
	{
		qWarning() << "SkyCultureMaker: ScmConstellationDialog::setIsDarkConstellation: ScmDraw is null";
		return;
	}

	// the value changed, so we should reset some data from the previous mode
	if (isDarkConstellation != isDark)
	{
		// reset drawn lines as they are not compatible between modes
		draw->resetDrawing();

		// reset artwork as well
		ui->bind_star->setEnabled(false);
		imageItem->hide();
		imageItem->resetAnchors();
		maker->setTempArtwork(nullptr);

		activeTool = scm::DrawTools::None;
		ui->penBtn->setChecked(false);
		ui->eraserBtn->setChecked(false);
		maker->setDrawTool(scm::DrawTools::None);

		isDarkConstellation = isDark;
	}

	if (draw != nullptr)
	{
		draw->setDrawingMode(isDark ? scm::DrawingMode::Coordinates : scm::DrawingMode::StarsAndDSO);
	}

	updateTranslatableStrings();
}

void ScmConstellationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTranslatableStrings();
	}
}

void ScmConstellationDialog::close()
{
	setVisible(false);
	maker->setIsLineDrawEnabled(false);
	maker->setCanCreateConstellations(true);
}

void ScmConstellationDialog::createDialogContent()
{
	isDialogInitialized = true;
	ui->setupUi(dialog);
	imageItem->hide();
	ui->artwork_image->setScene(imageItem->scene());
	ui->bind_star->setEnabled(false);

	setIsDarkConstellation(false);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmConstellationDialog::close);
	connect(ui->tabs, &QTabWidget::currentChanged, this, &ScmConstellationDialog::tabChanged);

	connect(ui->penBtn, &QPushButton::toggled, this, &ScmConstellationDialog::togglePen);
	connect(ui->eraserBtn, &QPushButton::toggled, this, &ScmConstellationDialog::toggleEraser);
	connect(ui->undoBtn, &QPushButton::clicked, this, &ScmConstellationDialog::triggerUndo);

	connect(ui->upload_image, &QPushButton::clicked, this, &ScmConstellationDialog::triggerUploadImage);
	connect(ui->remove_image, &QPushButton::clicked, this, &ScmConstellationDialog::triggerRemoveImage);
	connect(ui->bind_star, &QPushButton::clicked, this, &ScmConstellationDialog::bindSelectedStar);
	imageItem->setAnchorSelectionChangedCallback(
		[this]()
		{
			this->ui->bind_star->setEnabled(this->imageItem->hasAnchorSelection());
			this->updateArtwork();
		});
	imageItem->setAnchorPositionChangedCallback([this]() { this->updateArtwork(); });

	updateTranslatableStrings();
	ui->tooltipBtn->setToolTip(artworkToolTip);

	connect(ui->saveBtn, &QPushButton::clicked, this, &ScmConstellationDialog::saveConstellation);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmConstellationDialog::cancel);

	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &ScmConstellationDialog::handleFontChanged);
	connect(&StelApp::getInstance(), &StelApp::guiFontSizeChanged, this, &ScmConstellationDialog::handleFontChanged);

	handleFontChanged();

	// LABELS TAB

	connect(ui->enNameLE, &QLineEdit::textChanged, this,
	        [this]()
	        {
			constellationEnglishName = ui->enNameLE->text();

			QString newConstId         = constellationEnglishName.toLower().replace(" ", "_");
			constellationPlaceholderId = newConstId;
			ui->idLE->setPlaceholderText(newConstId);
		});
	connect(ui->idLE, &QLineEdit::textChanged, this, [this]() { constellationId = ui->idLE->text(); });
	connect(ui->natNameLE, &QLineEdit::textChanged, this,
	        [this]()
	        {
			constellationNativeName = ui->natNameLE->text();
			if (constellationNativeName->isEmpty())
			{
				constellationNativeName = std::nullopt;
			}
		});
	connect(ui->pronounceLE, &QLineEdit::textChanged, this,
	        [this]()
	        {
			constellationPronounce = ui->pronounceLE->text();
			if (constellationPronounce->isEmpty())
			{
				constellationPronounce = std::nullopt;
			}
		});
	connect(ui->ipaLE, &QLineEdit::textChanged, this,
	        [this]()
	        {
			constellationIPA = ui->ipaLE->text();
			if (constellationIPA->isEmpty())
			{
				constellationIPA = std::nullopt;
			}
		});
	connect(ui->description, &QTextEdit::textChanged, this,
	        [this]() { constellationDescription = ui->description->toPlainText(); });
}

void ScmConstellationDialog::handleFontChanged()
{
	QFont labelsTitleFont = QApplication::font();
	labelsTitleFont.setPixelSize(labelsTitleFont.pixelSize() + 2);
	labelsTitleFont.setBold(true);
	ui->labelsTitle->setFont(labelsTitleFont);
}

void ScmConstellationDialog::togglePen(bool checked)
{
	if (checked)
	{
		ui->eraserBtn->setChecked(false);
		activeTool = scm::DrawTools::Pen;
		maker->setDrawTool(activeTool);
		ui->drawInfoBox->setText(helpDrawInfoPen);
	}
	else
	{
		activeTool = scm::DrawTools::None;
		maker->setDrawTool(activeTool);
		ui->drawInfoBox->setText("");
	}
}

void ScmConstellationDialog::toggleEraser(bool checked)
{
	if (checked)
	{
		ui->penBtn->setChecked(false);
		activeTool = scm::DrawTools::Eraser;
		maker->setDrawTool(activeTool);
		ui->drawInfoBox->setText(helpDrawInfoEraser);
	}
	else
	{
		activeTool = scm::DrawTools::None;
		maker->setDrawTool(activeTool);
		ui->drawInfoBox->setText("");
	}
}

void ScmConstellationDialog::triggerUndo()
{
	maker->triggerDrawUndo();
	togglePen(true);
}

void ScmConstellationDialog::triggerUploadImage()
{
	QString filePath = QFileDialog::getOpenFileName(ui->artworkTab, q_("Open Artwork"), lastUsedImageDirectory,
	                                                q_("Images (*.png *.jpg *.jpeg)"));
	QFileInfo fileInfo(filePath);
	lastUsedImageDirectory = fileInfo.absolutePath();

	if (!fileInfo.isFile())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Chosen path is not a valid file:\n") + filePath);
		return;
	}

	if (!(fileInfo.suffix().compare("PNG", Qt::CaseInsensitive) == 0 ||
	      fileInfo.suffix().compare("JPG", Qt::CaseInsensitive) == 0 ||
	      fileInfo.suffix().compare("JPEG", Qt::CaseInsensitive) == 0))
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Chosen file is not a PNG, JPG or JPEG image:\n") + filePath);
		return;
	}

	QPixmap image = QPixmap(fileInfo.absoluteFilePath());
	imageItem->setImage(image);
	imageItem->show();
	ui->artwork_image->centerOn(imageItem);
	ui->artwork_image->fitInView(imageItem, Qt::KeepAspectRatio);
	ui->artwork_image->show();

	updateArtwork();
}

void ScmConstellationDialog::triggerRemoveImage()
{
	imageItem->resetArtwork();
	imageItem->resetAnchors();

	updateArtwork();
}

void ScmConstellationDialog::bindSelectedStar()
{
	if (!imageItem->hasAnchorSelection())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("No anchor was selected. Please select an anchor to bind to."));
		qDebug() << "SkyCultureMaker: No anchor was selected.";
		return;
	}

	StelApp &app             = StelApp::getInstance();
	StelObjectMgr &objectMgr = app.getStelObjectMgr();

	if (!objectMgr.getWasSelected())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("No star was selected to bind to the current selected anchor."));
		qDebug() << "SkyCultureMaker: No star was selected to bind to.";
		return;
	}

	StelObjectP stelObj = objectMgr.getLastSelectedObject();
	assert(stelObj != nullptr); // Checked through getWasSelected
	if (stelObj->getType().compare("star", Qt::CaseInsensitive) != 0 &&
	    stelObj->getType().compare("nebula", Qt::CaseInsensitive) != 0)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("The selected object must be of type Star or Nebula."));
		qDebug() << "SkyCultureMaker: The selected object is not of type Star, got " << stelObj->getType();
		return;
	}

	ScmConstellationImageAnchor *anchor = imageItem->getSelectedAnchor();
	if (anchor == nullptr)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("No anchor was selected. Please select an anchor to bind to."));
		qDebug() << "SkyCultureMaker: No anchor was selected";
		return;
	}

	bool success = anchor->trySetStarHip(stelObj->getID());
	if (success == false)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("The selected object must contain a HIP number."));
		qDebug() << "SkyCultureMaker: The object does not contain a HIP, id = " << stelObj->getID();
		return;
	}

	updateArtwork();
}

void ScmConstellationDialog::tabChanged(int index)
{
	Q_UNUSED(index);
	ui->penBtn->setChecked(false);
	ui->eraserBtn->setChecked(false);
	maker->setDrawTool(scm::DrawTools::None);
}

bool ScmConstellationDialog::canConstellationBeSaved() const
{
	// shouldnt happen
	scm::ScmSkyCulture *currentSkyCulture = maker->getCurrentSkyCulture();
	if (currentSkyCulture == nullptr)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: Sky Culture is not set"));
		qDebug() << "SkyCultureMaker: Could not save: Sky Culture is not set";
		return false;
	}

	if (constellationEnglishName.isEmpty())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: English name is empty"));
		qDebug() << "SkyCultureMaker: Could not save: English name is empty";
		return false;
	}

	// It is okay for the constellationId to be empty, as long as the constellationPlaceholderId is set
	QString finalId = constellationId.isEmpty() ? constellationPlaceholderId : constellationId;
	if (finalId.isEmpty())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: Constellation ID is empty"));
		qDebug() << "SkyCultureMaker: Could not save: Constellation ID is empty";
		return false;
	}

	// Not editing a constellation, but the ID already exists
	if (constellationBeingEdited == nullptr && currentSkyCulture->getConstellation(finalId) != nullptr)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: Constellation with this ID already exists"));
		qDebug() << "SkyCultureMaker: Could not save: Constellation with this ID already exists, id = "
			 << finalId;
		return false;
	}
	// Editing a constellation, but the ID already exists and is not the same as the one being edited
	else if (constellationBeingEdited != nullptr && constellationBeingEdited->getId() != finalId &&
	         currentSkyCulture->getConstellation(finalId) != nullptr)
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: Constellation with this ID already exists"));
		qDebug() << "SkyCultureMaker: Could not save: Constellation with this ID already exists, id = "
			 << finalId;
		return false;
	}

	// Check if drawnStars is empty
	auto drawnConstellation = maker->getScmDraw()->getConstellationLines();
	if (drawnConstellation.empty())
	{
		maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
		                            q_("Could not save: The constellation does not contain any drawings"));
		qDebug() << "SkyCultureMaker: Could not save: The constellation does not contain any drawings";
		return false;
	}

	// Check if an artwork was added and all anchors have a binding
	if (imageItem->isVisible())
	{
		if (!imageItem->isImageAnchored())
		{
			maker->showUserErrorMessage(this->dialog, ui->titleBar->title(),
			                            q_("Could not save: An artwork is attached, but not all "
			                               "anchors have a star bound."));
			qDebug() << "SkyCultureMaker: Could not save: An artwork is attached, but not all "
				    "anchors have a star bound.";
			return false;
		}
	}

	return true;
}

void ScmConstellationDialog::cancel()
{
	if (constellationBeingEdited != nullptr)
	{
		// If we are editing a constellation, we need to show the original one again
		constellationBeingEdited->isHidden = false;
	}
	resetDialog();
	ScmConstellationDialog::close();
}

void ScmConstellationDialog::saveConstellation()
{
	if (canConstellationBeSaved())
	{
		auto lines = maker->getScmDraw()->getConstellationLines();
		QString id = constellationId.isEmpty() ? constellationPlaceholderId : constellationId;

		scm::ScmSkyCulture *culture = maker->getCurrentSkyCulture();
		assert(culture != nullptr); // already checked by canConstellationBeSaved

		// delete the original constellation if we are editing one
		if (constellationBeingEdited != nullptr)
		{
			culture->removeConstellation(constellationBeingEdited->getId());
		}

		scm::ScmConstellation &constellation = culture->addConstellation(id, lines,
		                                                                 isDarkConstellation);

		constellation.setEnglishName(constellationEnglishName);
		constellation.setNativeName(constellationNativeName);
		constellation.setPronounce(constellationPronounce);
		constellation.setIPA(constellationIPA);
		constellation.setDescription(constellationDescription);
		if (imageItem->isVisible() && imageItem->getArtwork().getHasArt())
		{
			constellation.setArtwork(imageItem->getArtwork());
		}

		maker->updateSkyCultureDialog();
		resetDialog();
		ScmConstellationDialog::close();
	}
}

void ScmConstellationDialog::resetDialog()
{
	// If the dialog was not initialized, the ui elements do not exist yet.
	if (!isDialogInitialized)
	{
		return;
	}

	ui->tabs->setCurrentIndex(0);

	activeTool = scm::DrawTools::None;

	ui->penBtn->setChecked(false);
	ui->eraserBtn->setChecked(false);
	maker->setDrawTool(scm::DrawTools::None);
	setIsDarkConstellation(false);

	constellationId.clear();
	ui->idLE->clear();

	constellationPlaceholderId.clear();
	ui->idLE->setPlaceholderText("");

	constellationEnglishName.clear();
	ui->enNameLE->clear();

	constellationNativeName = std::nullopt;
	ui->natNameLE->clear();

	constellationPronounce = std::nullopt;
	ui->pronounceLE->clear();

	constellationIPA = std::nullopt;
	ui->ipaLE->clear();

	constellationDescription.clear();
	ui->description->clear();

	ui->bind_star->setEnabled(false);
	imageItem->hide();
	imageItem->resetAnchors();
	maker->setTempArtwork(nullptr);

	constellationBeingEdited = nullptr;

	// reset ScmDraw
	maker->resetScmDraw();
}

void ScmConstellationDialog::updateArtwork()
{
	if (!imageItem->isVisible() || !imageItem->isImageAnchored())
	{
		maker->setTempArtwork(nullptr);
		return;
	}

	imageItem->updateAnchors();
	maker->setTempArtwork(&(imageItem->getArtwork()));
}

void ScmConstellationDialog::handleDialogSizeChanged(QSizeF size)
{
	StelDialog::handleDialogSizeChanged(size);

	ui->artwork_image->fitInView(imageItem, Qt::KeepAspectRatio);
}

void ScmConstellationDialog::updateTranslatableStrings()
{
	// check if ui is valid before attempting retranslation
	if(ui == nullptr)
	{
		return;
	}

#if defined(Q_OS_MAC)
	helpDrawInfoPen = q_("Use RightClick or Control + Click to draw a connected line.\n"
	                     "Use Double-RightClick or Control + Double-Click to stop drawing the line.\n"
	                     "Use Command + F to search and connect stars.");
	helpDrawInfoEraser = q_("Hold RightClick or Control + Click to delete the line under the cursor.\n");
#else
	helpDrawInfoPen = q_("Use RightClick to draw a connected line.\n"
	                     "Use Double-RightClick to stop drawing the line.\n"
	                     "Use CTRL + F to search and connect stars.");
	helpDrawInfoEraser = q_("Hold RightClick to delete the line under the cursor.\n");
#endif

	artworkToolTip = q_("Usage:\n"
	                    "1. Upload an image\n"
	                    "2. Three anchor points appear in the center of the image\n"
	                    "   - An anchor is green when selected\n"
	                    "3. Select a star of your choice\n"
	                    "4. Click the 'Bind Star' button\n"
	                    "5. The anchor is shown in a brighter color when bound to a star\n"
	                    "   - The corresponding bound star is automatically selected when an anchor is selected\n"
	                    "   - 'Bind Star' will overwrite the current binding if it already exists");

	// Update title and labels depending on constellation type
	ui->titleBar->setTitle(isDarkConstellation ? q_("SCM: Dark Constellation Editor")
	                                           : q_("SCM: Constellation Editor"));
	ui->labelsTitle->setText(isDarkConstellation ? q_("Please name your Dark Constellation")
	                                             : q_("Please name your Constellation"));

	// Update UI elements that use these strings if they're visible
	if (ui->tooltipBtn != nullptr)
	{
		ui->tooltipBtn->setToolTip(artworkToolTip);
	}

	// Update the drawInfoBox text if a tool is currently active
	if (ui->drawInfoBox != nullptr)
	{
		if (activeTool == scm::DrawTools::Pen)
		{
			ui->drawInfoBox->setText(helpDrawInfoPen);
		}
		else if (activeTool == scm::DrawTools::Eraser)
		{
			ui->drawInfoBox->setText(helpDrawInfoEraser);
		}
	}
}