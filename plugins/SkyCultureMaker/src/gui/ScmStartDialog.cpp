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

	qDebug() << "SkyCultureMaker: Unloaded the ScmStartDialog";
}

void ScmStartDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
	}
}

void ScmStartDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// SCM just after merge creates misformatted description.md. For now, inform the feature testers.
	ui->welcomeLabel->setText(QString("%1<br/><bold>%2</bold>").arg(q_("Welcome to the Sky Culture Maker!"), q_("Note: Test only. The result does not yet comply to Stellarium's formatting rules.")));

	// connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &ScmStartDialog::handleFontChanged);
	connect(&StelApp::getInstance(), &StelApp::guiFontSizeChanged, this, &ScmStartDialog::handleFontChanged);

	// Buttons
	connect(ui->scmStartCancelpushButton, &QPushButton::clicked, this, &ScmStartDialog::close); // Cancel
	connect(ui->scmStartCreatepushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::startScmCreationProcess); // Create
	connect(ui->scmStartEditpushButton, &QPushButton::clicked, this,
	        &ScmStartDialog::close); // Edit - TODO: add logic (currently closing the window)

	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmStartDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	// Init the correct font
	handleFontChanged();

/* =============================================== SkyCultureConverter ============================================== */
#ifdef SCM_CONVERTER_ENABLED_CPP
	ui->scmStartConvertpushButton->setToolTip(
	        q_("Convert sky cultures from the legacy (fab) format to the new (json) format"));
	connect(ui->scmStartConvertpushButton, &QPushButton::clicked, this,
	        [this]()
	        {
			if (!converterDialog)
			{
				converterDialog = new ScmConvertDialog(maker);
			}
			maker->stopScm(); // Stop SCM if running
			converterDialog->setVisible(true);
		});
#else   // SCM_CONVERTER_ENABLED_CPP is not defined
	// Converter is disabled, so hide the button
	ui->scmStartConvertpushButton->setVisible(false);
#endif  // SCM_CONVERTER_ENABLED_CPP
	/* ================================================================================================================== */
}

void ScmStartDialog::handleFontChanged()
{
	// Set welcome label font size
	QFont welcomeLabelFont = QApplication::font();
	welcomeLabelFont.setPixelSize(welcomeLabelFont.pixelSize() + 4);
	welcomeLabelFont.setBold(true);
	ui->welcomeLabel->setFont(welcomeLabelFont);
}

void ScmStartDialog::startScmCreationProcess()
{
	// Close the dialog before starting the editor
	maker->setDialogVisibility(scm::DialogID::StartDialog, false);
	// Start the editor dialog for creating a new Sky Culture
	maker->setDialogVisibility(scm::DialogID::SkyCultureDialog, true);
	maker->setNewSkyCulture();

	// GZ: Unclear why those dialogs are called at plugin start.
	//SkyCultureMaker::setActionToggle("actionShow_DateTime_Window_Global", true);
	//SkyCultureMaker::setActionToggle("actionShow_Location_Window_Global", true);
	SkyCultureMaker::setActionToggle("actionShow_Ground", false);
	SkyCultureMaker::setActionToggle("actionShow_Atmosphere", false);
	SkyCultureMaker::setActionToggle("actionShow_MeteorShowers", false);
	SkyCultureMaker::setActionToggle("actionShow_Satellite_Hints", false);
}

void ScmStartDialog::close()
{
	maker->stopScm();
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
		qWarning() << "SkyCultureMaker: Converter dialog is not initialized!";
	}
#else
	Q_UNUSED(b);
	qWarning() << "SkyCultureMaker: Converter dialog is not available in this build!";
#endif // SCM_CONVERTER_ENABLED_CPP
}
