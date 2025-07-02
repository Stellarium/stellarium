#include "ScmStartDialog.hpp"
#include "ui_scmStartDialog.h"
#include <cassert>
#include <QDebug>

#ifdef SCM_CONVERTER_ENABLED_CPP
# include "ScmConvertDialog.hpp"
#endif // SCM_CONVERTER_ENABLED_CPP

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

ScmStartDialog::ScmStartDialog(SkyCultureMaker *maker)
	: StelDialog("ScmStartDialog")
	, maker(maker)
{
	assert(maker != nullptr);
	ui = new Ui_scmStartDialog;
}

ScmStartDialog::~ScmStartDialog()
{
	if (ui != nullptr)
	{
		delete ui;
	}

	qDebug() << "Unloaded the ScmStartDialog";
}

void ScmStartDialog::retranslate()
{
	if (dialog)
	{
		// Issue #117
		// ui->retranslateUi(dialog);
	}
}

void ScmStartDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmStartDialog::closeDialog);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Buttons
	connect(ui->scmStartCancelpushButton, &QPushButton::clicked, this, &ScmStartDialog::closeDialog); // Cancel
	connect(ui->scmStartCreatepushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::startScmCreationProcess); // Create
	connect(ui->scmStartEditpushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::closeDialog); // Edit - TODO: add logic (currently closing the window)

/* =============================================== SkyCultureConverter ============================================== */
#ifdef SCM_CONVERTER_ENABLED_CPP
	ui->scmStartConvertpushButton->setToolTip(
		tr("Convert SkyCultures from the old (fib) format to the new (json) format"));
	connect(ui->scmStartConvertpushButton, &QPushButton::clicked, this,
	        [this]()
	        {
			if (!converterDialog)
			{
				converterDialog = new ScmConvertDialog(maker);
			}
			maker->setStartDialogVisibility(false); // Hide the start dialog
			converterDialog->setVisible(true);
		});
#else   // SCM_CONVERTER_ENABLED_CPP is not defined
	// Converter is disabled, so disable the button
	ui->scmStartConvertpushButton->setEnabled(false);
	ui->scmStartConvertpushButton->setToolTip(
		tr("Converter is only available from Qt6.5 onwards, currently build with version %1")
			.arg(QT_VERSION_STR));
#endif  // SCM_CONVERTER_ENABLED_CPP
/* ================================================================================================================== */
}

void ScmStartDialog::startScmCreationProcess()
{
	dialog->setVisible(false);                  // Close the dialog before starting the editor
	maker->setSkyCultureDialogVisibility(true); // Start the editor dialog for creating a new Sky Culture
	maker->setNewSkyCulture();

	SkyCultureMaker::setActionToggle("actionShow_DateTime_Window_Global", true);
	SkyCultureMaker::setActionToggle("actionShow_Location_Window_Global", true);
	SkyCultureMaker::setActionToggle("actionShow_Ground", false);
	SkyCultureMaker::setActionToggle("actionShow_Atmosphere", false);
	SkyCultureMaker::setActionToggle("actionShow_MeteorShowers", false);
	SkyCultureMaker::setActionToggle("actionShow_Satellite_Hints", false);
}

void ScmStartDialog::closeDialog()
{
	maker->setIsScmEnabled(false); // Disable the Sky Culture Maker
}

bool ScmStartDialog::isConverterDialogVisible()
{
#ifdef SCM_CONVERTER_ENABLED_CPP
	if (converterDialog != nullptr)
	{
		return converterDialog->visible();
	}
	else
	{
		return false;
	}
#else
	return false; // Converter dialog is not available
#endif
}

void ScmStartDialog::setConverterDialogVisibility(bool b)
{
#ifdef SCM_CONVERTER_ENABLED_CPP
	if (converterDialog != nullptr)
	{
		if (b != converterDialog->visible())
		{
			converterDialog->setVisible(b);
		}
	}
	else
	{
		qWarning() << "Converter dialog is not initialized!";
	}
#else
	Q_UNUSED(b);
	qWarning() << "Converter dialog is not available in this build!";
#endif // SCM_CONVERTER_ENABLED_CPP
}
