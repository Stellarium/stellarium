#include "ScmConstellationDialog.hpp"
#include "StelGui.hpp"
#include "ui_scmConstellationDialog.h"
#include <cassert>

ScmConstellationDialog::ScmConstellationDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmConstellationDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmConstellationDialog;
}

ScmConstellationDialog::~ScmConstellationDialog()
{
	delete ui;
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
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmConstellationDialog::close);

	connect(ui->penBtn, &QPushButton::toggled, this, &ScmConstellationDialog::togglePen);
	connect(ui->eraserBtn, &QPushButton::toggled, this, &ScmConstellationDialog::toggleEraser);
	connect(ui->undoBtn, &QPushButton::clicked, this, &ScmConstellationDialog::triggerUndo);

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
	}
	else
	{
		activeTool = scm::DrawTools::None;
		maker->setDrawTool(activeTool);
	}
}

void ScmConstellationDialog::toggleEraser(bool checked)
{
	if (checked)
	{
		ui->penBtn->setChecked(false);
		activeTool = scm::DrawTools::Eraser;
		maker->setDrawTool(activeTool);
	}
	else
	{
		activeTool = scm::DrawTools::None;
		maker->setDrawTool(activeTool);
	}
}

void ScmConstellationDialog::triggerUndo()
{
	maker->triggerDrawUndo();
	togglePen(true);
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

	// reset ScmDraw
	maker->resetScmDraw();
}
