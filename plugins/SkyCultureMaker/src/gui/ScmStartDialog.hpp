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

#ifndef SCMSTARTDIALOG_HPP
#define SCMSTARTDIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialog.hpp"
#include <QObject>

#ifdef SCM_CONVERTER_ENABLED_CPP
# include "ScmConvertDialog.hpp"
#endif

class Ui_scmStartDialog;

class ScmStartDialog : public StelDialog
{
protected:
	void createDialogContent() override;

public:
	ScmStartDialog(SkyCultureMaker *maker);
	~ScmStartDialog() override;

	/**
	 * @brief Check if the converter dialog is currently visible.
	 * @return true if the converter dialog is visible, false otherwise.
	 */
	bool isConverterDialogVisible();

	/**
	 * @brief Set the visibility of the converter dialog.
	 * @param b The boolean value to be set.
	 */
	void setConverterDialogVisibility(bool b);

public slots:
	void retranslate() override;
	void close() override;

protected slots:
	void handleFontChanged();

private slots:
	void startScmCreationProcess();

private:
	Ui_scmStartDialog *ui  = nullptr;
	SkyCultureMaker *maker = nullptr;
#ifdef SCM_CONVERTER_ENABLED_CPP
	ScmConvertDialog *converterDialog = nullptr;
#endif
};

#endif // SCMSTARTDIALOG_HPP
