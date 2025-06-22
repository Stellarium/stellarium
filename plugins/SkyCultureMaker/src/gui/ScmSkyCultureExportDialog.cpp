#include "ScmSkyCultureExportDialog.hpp"
#include "ui_scmSkyCultureExportDialog.h"

ScmSkyCultureExportDialog::ScmSkyCultureExportDialog(SkyCultureMaker *maker)
	: StelDialogSeparate("ScmSkyCultureExportDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmSkyCultureExportDialog;
}

ScmSkyCultureExportDialog::~ScmSkyCultureExportDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}
}

void ScmSkyCultureExportDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmSkyCultureExportDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmSkyCultureExportDialog::close);
	connect(ui->exportBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::exportSkyCulture);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::close);
}

void ScmSkyCultureExportDialog::exportSkyCulture()
{
	// TODO: Export sky culture as json

	bool success = maker->saveSkyCultureDescription();

	if (success)
	{
		maker->setSkyCultureDialogInfoLabel("Sky culture exported successfully!");
	}
	else
	{
		maker->setSkyCultureDialogInfoLabel("WARNING: Failed to export sky culture.");
		qDebug() << "SkyCultureMaker: Failed to export sky culture.";
	}

	ScmSkyCultureExportDialog::close();
}
