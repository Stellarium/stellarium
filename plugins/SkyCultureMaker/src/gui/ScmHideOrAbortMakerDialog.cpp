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

	qDebug() << "SkyCultureMaker: Unloaded the ScmHideOrAbortMakerDialog";
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

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->titleBar, &TitleBar::closeClicked, this, &ScmHideOrAbortMakerDialog::close);
	connect(ui->titleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(&StelApp::getInstance(), &StelApp::fontChanged, this, &ScmHideOrAbortMakerDialog::handleFontChanged);
	connect(&StelApp::getInstance(), &StelApp::guiFontSizeChanged, this,
	        &ScmHideOrAbortMakerDialog::handleFontChanged);
	handleFontChanged();

	// Buttons
	connect(ui->scmMakerAbortButton, &QPushButton::clicked, maker,
			&SkyCultureMaker::stopScm); // Abort
	connect(ui->scmMakerHideButton, &QPushButton::clicked, maker,
			&SkyCultureMaker::hideScm); // Hide
	connect(ui->scmMakerCancelButton, &QPushButton::clicked, this,
	        &ScmHideOrAbortMakerDialog::close); // Cancel
}

void ScmHideOrAbortMakerDialog::handleFontChanged()
{
	QFont questionFont = QApplication::font();
	questionFont.setPixelSize(questionFont.pixelSize() + 4);
	ui->questionLbl->setFont(questionFont);
}
