#include "ScmConstellationDialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelObjectMgr.hpp"
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
	imageItem = new ScmImageAnchored;
}

ScmConstellationDialog::~ScmConstellationDialog()
{
	delete ui;
	qDebug() << "Unloaded the ScmConstellationDialog";
}

void ScmConstellationDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmConstellationDialog::close()
{
	maker->setConstellationDialogVisibility(false);
}

void ScmConstellationDialog::createDialogContent()
{
	isDialogInitialized = true;
	ui->setupUi(dialog);
	imageItem->hide();
	ui->artwork_image->setScene(imageItem->scene());
	ui->bind_star->setEnabled(false);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmConstellationDialog::close);

	connect(ui->penBtn, &QPushButton::toggled, this, &ScmConstellationDialog::togglePen);
	connect(ui->eraserBtn, &QPushButton::toggled, this, &ScmConstellationDialog::toggleEraser);
	connect(ui->undoBtn, &QPushButton::clicked, this, &ScmConstellationDialog::triggerUndo);

	connect(ui->upload_image, &QPushButton::clicked, this, &ScmConstellationDialog::triggerUploadImage);
	connect(ui->remove_image, &QPushButton::clicked, this, &ScmConstellationDialog::triggerRemoveImage);
	connect(ui->bind_star, &QPushButton::clicked, this, &ScmConstellationDialog::bindSelectedStar);
	imageItem->setAnchorSelectionChangedCallback(
		[this]() { this->ui->bind_star->setEnabled(this->imageItem->hasAnchorSelection()); });

	connect(ui->saveBtn, &QPushButton::clicked, this, &ScmConstellationDialog::saveConstellation);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmConstellationDialog::cancel);

	// LABELS TAB
	connect(ui->enNameTE, &QTextEdit::textChanged, this,
	        [this]()
	        {
			constellationEnglishName = ui->enNameTE->toPlainText();

			QString newConstId         = constellationEnglishName.toLower().replace(" ", "_");
			constellationPlaceholderId = newConstId;
			ui->idTE->setPlaceholderText(newConstId);
		});
	connect(ui->idTE, &QTextEdit::textChanged, this, [this]() { constellationId = ui->idTE->toPlainText(); });
	connect(ui->natNameTE, &QTextEdit::textChanged, this,
	        [this]()
	        {
			constellationNativeName = ui->natNameTE->toPlainText();
			if (constellationNativeName->isEmpty())
			{
				constellationNativeName = std::nullopt;
			}
		});
	connect(ui->pronounceTE, &QTextEdit::textChanged, this,
	        [this]()
	        {
			constellationPronounce = ui->pronounceTE->toPlainText();
			if (constellationPronounce->isEmpty())
			{
				constellationPronounce = std::nullopt;
			}
		});
	connect(ui->ipaTE, &QTextEdit::textChanged, this,
	        [this]()
	        {
			constellationIPA = ui->ipaTE->toPlainText();
			if (constellationIPA->isEmpty())
			{
				constellationIPA = std::nullopt;
			}
		});
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
	QString filePath = QFileDialog::getOpenFileName(ui->artworkTab, "Open Artwork", lastUsedImageDirectory,
	                                                "Images (*.png *.jpg *.jpeg)");
	QFileInfo fileInfo(filePath);
	lastUsedImageDirectory = fileInfo.absolutePath();

	if (!fileInfo.isFile())
	{
		ui->infoLbl->setText("Choosen path is not a valid file:\n" + filePath);
		return;
	}

	if (!(fileInfo.suffix().toUpper().compare("PNG") || fileInfo.suffix().toUpper().compare("JPG") ||
	      fileInfo.suffix().toUpper().compare("JPEG")))
	{
		ui->infoLbl->setText("Choosen file is not a PNG, JPG or JPEG image:\n" + filePath);
		return;
	}

	// Reset text
	ui->infoLbl->setText("");

	QPixmap image = QPixmap(fileInfo.absoluteFilePath());
	imageItem->setImage(image);
	imageItem->show();
	ui->artwork_image->centerOn(imageItem);
	ui->artwork_image->fitInView(imageItem, Qt::KeepAspectRatio);
	ui->artwork_image->show();
}

void ScmConstellationDialog::triggerRemoveImage()
{
	imageItem->hide();
	imageItem->resetAnchors();
}

void ScmConstellationDialog::bindSelectedStar()
{
	if (!imageItem->hasAnchorSelection())
	{
		ui->infoLbl->setText("WARNING: Select an anchor to bind to.");
		return;
	}

	StelApp &app             = StelApp::getInstance();
	StelObjectMgr &objectMgr = app.getStelObjectMgr();

	if (!objectMgr.getWasSelected())
	{
		ui->infoLbl->setText("WARNING: Select a star to bind to the current selected anchor.");
		return;
	}

	StelObjectP stelObj    = objectMgr.getLastSelectedObject();
	ScmImageAnchor *anchor = imageItem->getSelectedAnchor();
	anchor->setStarNameI18n(stelObj->getNameI18n());
}

bool ScmConstellationDialog::canConstellationBeSaved() const
{
	// shouldnt happen
	if (maker->getCurrentSkyCulture() == nullptr)
	{
		ui->infoLbl->setText("WARNING: Could not save: Sky Culture is not set");
		return false;
	}

	if (constellationEnglishName.isEmpty())
	{
		ui->infoLbl->setText("WARNING: Could not save: English name is empty");
		return false;
	}

	// It is okay for the constellationId to be empty, as long as the constellationPlaceholderId is set
	QString finalId = constellationId.isEmpty() ? constellationPlaceholderId : constellationId;
	if (finalId.isEmpty())
	{
		ui->infoLbl->setText("WARNING: Could not save: Constellation ID is empty");
		return false;
	}

	if (maker->getCurrentSkyCulture() != nullptr &&
	    maker->getCurrentSkyCulture()->getConstellation(finalId) != nullptr)
	{
		ui->infoLbl->setText("WARNING: Could not save: Constellation with this ID already exists");
		return false;
	}

	// Check if drawnStars is empty
	auto drawnConstellation = maker->getScmDraw()->getCoordinates();
	if (drawnConstellation.empty())
	{
		ui->infoLbl->setText("WARNING: Could not save: The constellation does not contain any drawings");
		return false;
	}

	// Check if an artwork was added and all anchors have a binding
	if (imageItem->isVisible())
	{
		for (const auto &anchor : imageItem->getAnchors())
		{
			if (anchor.getStarNameI18n().isEmpty())
			{
				ui->infoLbl->setText("WARNING: Could not save: An artwork is attached, but not all "
				                     "anchors have a star bound.");
				return false;
			}
		}
	}

	return true;
}

void ScmConstellationDialog::cancel()
{
	resetDialog();
	ScmConstellationDialog::close();
}

void ScmConstellationDialog::saveConstellation()
{
	if (canConstellationBeSaved())
	{
		auto coordinates = maker->getScmDraw()->getCoordinates();
		auto stars       = maker->getScmDraw()->getStars();
		QString id       = constellationId.isEmpty() ? constellationPlaceholderId : constellationId;

		scm::ScmSkyCulture *culture = maker->getCurrentSkyCulture();
		assert(culture != nullptr); // already checked by canConstellationBeSaved

		scm::ScmConstellation &constellation = culture->addConstellation(id, coordinates, stars);

		constellation.setEnglishName(constellationEnglishName);
		constellation.setNativeName(constellationNativeName);
		constellation.setPronounce(constellationPronounce);
		constellation.setIPA(constellationIPA);

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

	activeTool = scm::DrawTools::None;
	ui->penBtn->setChecked(false);
	ui->eraserBtn->setChecked(false);
	maker->setDrawTool(activeTool);

	constellationId.clear();
	ui->idTE->clear();

	constellationPlaceholderId.clear();
	ui->idTE->setPlaceholderText("");

	constellationEnglishName.clear();
	ui->enNameTE->clear();

	constellationNativeName = std::nullopt;
	ui->natNameTE->clear();

	constellationPronounce = std::nullopt;
	ui->pronounceTE->clear();

	constellationIPA = std::nullopt;
	ui->ipaTE->clear();

	ui->bind_star->setEnabled(false);
	imageItem->hide();
	imageItem->resetAnchors();

	// reset ScmDraw
	maker->resetScmDraw();
}

void ScmConstellationDialog::handleDialogSizeChanged(QSizeF size)
{
	StelDialog::handleDialogSizeChanged(size);

	ui->artwork_image->fitInView(imageItem, Qt::KeepAspectRatio);
}
