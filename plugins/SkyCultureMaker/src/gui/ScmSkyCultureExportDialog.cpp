#include "ScmSkyCultureExportDialog.hpp"
#include "ui_scmSkyCultureExportDialog.h"
#include "ScmSkyCulture.hpp"
#include <QJsonObject>
#include <QJsonDocument>

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
	connect(ui->saveBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::saveSkyCulture);
	connect(ui->saveAndExitBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::saveAndExitSkyCulture);
	connect(ui->cancelBtn, &QPushButton::clicked, this, &ScmSkyCultureExportDialog::close);
}

void ScmSkyCultureExportDialog::saveSkyCulture()
{
	if (maker == nullptr) {
		qWarning() << "SkyCultureMaker: maker is nullptr. Cannot export sky culture.";
		ScmSkyCultureExportDialog::close();
		return;
	}

	scm::ScmSkyCulture* currentSkyCulture = maker->getCurrentSkyCulture();
	if (currentSkyCulture == nullptr) {
		qWarning() << "SkyCultureMaker: current sky culture is nullptr. Cannot export.";
		maker->setSkyCultureDialogInfoLabel("ERROR: No sky culture is set.");
		ScmSkyCultureExportDialog::close();
		return;
	}

	// TODO: Export sky culture as json file (#88)
	qDebug() << "Exporting sky culture...";
	QJsonObject scJsonObject = currentSkyCulture->toJson();
	QJsonDocument scJsonDoc(scJsonObject);
 	qDebug().noquote() << scJsonDoc.toJson(QJsonDocument::Compact);
	// TODO: the error handling here should be improved once we also have to
	// check whether the json file was successfully saved (#88)

	bool savedDescriptionSuccessfully = maker->saveSkyCultureDescription();

	if (savedDescriptionSuccessfully)
	{
		maker->setSkyCultureDialogInfoLabel("Sky culture exported successfully!");
	}
	else
	{
		maker->setSkyCultureDialogInfoLabel("WARNING: Failed to export sky culture description.");
		qWarning() << "SkyCultureMaker: Failed to export sky culture description.";
	}

	ScmSkyCultureExportDialog::close();
}

void ScmSkyCultureExportDialog::saveAndExitSkyCulture()
{
	saveSkyCulture();
	maker->resetScmDialogs();
	maker->hideAllDialogsAndDisableSCM();
}
