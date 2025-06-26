#include "ScmHideOrAbortMakerDialog.hpp"
#include "ui_scmHideOrAbortMakerDialog.h"
#include <cassert>
#include <QDebug>

ScmHideOrAbortMakerDialog::ScmHideOrAbortMakerDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmHideOrAbortMakerDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmHideOrAbortMakerDialog;
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
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmHideOrAbortMakerDialog::cancelDialog);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Buttons
	connect(ui->scmMakerAbortButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::abortScmCreationProcess); // Abort
	connect(ui->scmMakerHidehButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::hideScmCreationProcess); // Hide
	connect(ui->scmMakerCancelButton, &QPushButton::clicked, this, &ScmHideOrAbortMakerDialog::cancelDialog); // Cancel
}

// TODO: save state of the current sky culture
void ScmHideOrAbortMakerDialog::hideScmCreationProcess()
{
	maker->saveScmDialogVisibilityState();
	maker->hideAllDialogsAndDisableSCM();
	maker->setHideOrAbortMakerDialogVisibility(false);
}

// TODO: clear the current sky culture
void ScmHideOrAbortMakerDialog::abortScmCreationProcess()
{
	qDebug() << "Unloaded the ScmHideOrAbortMakerDialog 1";
	maker->resetScmDialogs();
	qDebug() << "Unloaded the ScmHideOrAbortMakerDialog 2";
	maker->hideAllDialogsAndDisableSCM();
	qDebug() << "Unloaded the ScmHideOrAbortMakerDialog 3";
	maker->setHideOrAbortMakerDialogVisibility(false);
	qDebug() << "Unloaded the ScmHideOrAbortMakerDialog 4";
}

void ScmHideOrAbortMakerDialog::cancelDialog()
{
	maker->setHideOrAbortMakerDialogVisibility(false);
}
