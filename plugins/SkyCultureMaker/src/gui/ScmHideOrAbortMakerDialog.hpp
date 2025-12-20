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

#ifndef SCMHIDEORABORTMAKERDIALOG_HPP
#define SCMHIDEORABORTMAKERDIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include <QObject>

class Ui_scmHideOrAbortMakerDialog;

class ScmHideOrAbortMakerDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;

public:
	ScmHideOrAbortMakerDialog(SkyCultureMaker *maker);
	~ScmHideOrAbortMakerDialog() override;

public slots:
	void retranslate() override;

protected slots:
	void handleFontChanged();

private:
	Ui_scmHideOrAbortMakerDialog *ui = nullptr;
	SkyCultureMaker *maker           = nullptr;
};

#endif // SCMHIDEORABORTMAKERDIALOG_HPP
