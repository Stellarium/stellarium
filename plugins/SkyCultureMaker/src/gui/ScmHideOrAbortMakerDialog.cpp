#include "ScmHideOrAbortMakerDialog.hpp"
#include "ui_scmHideOrAbortMakerDialog.h"
#include <cassert>
#include <QDebug>

ScmStartDialog::ScmHideOrAbortMakerDialog(SkyCultureMaker *maker)
	: StelDialog("ScmHideOrAbortMakerDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_ScmHideOrAbortMakerDialog;
}

ScmHideOrAbortMakerDialog::~ScmHideOrAbortMakerDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}

	qDebug() << "Unloaded the ScmHideOrAbortMakerDialog";
}

void ScmHideOrAbortMakerDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmHideOrAbortMakerDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmHideOrAbortMakerDialog::closeDialog);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Buttons
	connect(ui->scmMakerAbortButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::closeDialog); // Abort
	connect(ui->scmMakerHidehButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::hideScmCreationProcess); // Hide
	connect(ui->scmMakerCancelButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::closeDialog); // Abort
}

// TODO
void ScmHideOrAbortMakerDialog::hideScmCreationProcess()
{
	// dialog->setVisible(false);                  // Close the dialog before starting the editor
	// maker->setSkyCultureDialogVisibility(true); // Start the editor dialog for creating a new Sky Culture
	// maker->setNewSkyCulture();
}

// TODO
void ScmHideOrAbortMakerDialog::abortScmCreationProcess() {}

// TODO
void ScmHideOrAbortMakerDialog::closeDialog()
{
}
