/*
 * Copyright (C) 2013 Felix Zeltner
 * Copyright (C) 2026 Kamil Zaraś (astronow.pl)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef PLANESDIALOG_HPP
#define PLANESDIALOG_HPP

#include "StelDialog.hpp"

class Ui_planesDialog;
class Planes;

class PlanesDialog : public StelDialog
{
	Q_OBJECT

public:
	PlanesDialog();
	~PlanesDialog() override;

public slots:
	void retranslate() override;
	void updateFromPlugin();
	void setStatus(const QString& status);

protected:
	void createDialogContent() override;

private slots:
	void setEnabledFlag(bool enabled);
	void setShowLabels(bool enabled);
	void setShowButton(bool enabled);
	void setLabelMode(int index);
	void setProvider(int index);
	void setRadius(int radiusNm);
	void setFetchInterval(int seconds);
	void triggerRefresh();
	void openExternalLink(const QString& url);

private:
	void updateComboTexts();
	void setAboutHtml();

	Planes* planes;
	Ui_planesDialog* ui;
};

#endif
