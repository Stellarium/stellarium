#include "ScmConstellationDialog.hpp"
#include "ui_scmConstellationDialog.h"
#include "StelGui.hpp"

ScmConstellationDialog::ScmConstellationDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmConstellationDialog")
	, maker(maker)
{
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
	connect(ui->enNameTE,
		&QTextEdit::textChanged,
		this,
		[this]()
		{
			constellationEnglishName = ui->enNameTE->toPlainText();

			QString newConstId = constellationEnglishName.toLower().replace(" ", "_");
			constellationPlaceholderId = newConstId;
			ui->idTE->setPlaceholderText(newConstId);
		});
	connect(ui->idTE, &QTextEdit::textChanged, this, [this]() { constellationId = ui->idTE->toPlainText(); });
	connect(ui->natNameTE,
		&QTextEdit::textChanged,
		this,
		[this]()
		{
			constellationNativeName = ui->natNameTE->toPlainText();
			if (constellationNativeName->isEmpty())
			{
				constellationNativeName = std::nullopt;
			}
		});
	connect(ui->pronounceTE,
		&QTextEdit::textChanged,
		this,
		[this]()
		{
			constellationPronounce = ui->pronounceTE->toPlainText();
			if (constellationPronounce->isEmpty())
			{
				constellationPronounce = std::nullopt;
			}
		});
	connect(ui->ipaTE,
		&QTextEdit::textChanged,
		this,
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

bool ScmConstellationDialog::canConstellationBeSaved()
{
	// shouldnt happen
	if (nullptr == maker->getCurrentSkyCulture())
	{

		return false;
	}

	if (constellationEnglishName.isEmpty())
	{
		ui->infoLbl->setText("WARNING: Could not save: English name is empty");
		return false;
	}

	if (constellationId.isEmpty() && constellationPlaceholderId.isEmpty())
	{
		ui->infoLbl->setText("WARNING: Could not save: Constellation ID is empty");
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
		auto stars = maker->getScmDraw()->getStars();
		QString id = constellationId.isEmpty() ? constellationPlaceholderId : constellationId;
		maker->getCurrentSkyCulture()->addConstellation(id, coordinates, stars);
		scm::ScmConstellation *constellationObj = maker->getCurrentSkyCulture()->getConstellation(id);

		constellationObj->setEnglishName(constellationEnglishName);
		constellationObj->setNativeName(constellationNativeName);
		constellationObj->setPronounce(constellationPronounce);
		constellationObj->setIPA(constellationIPA);

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
}