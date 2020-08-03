/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef SCENERY3DDIALOG_HPP
#define SCENERY3DDIALOG_HPP

#include "StelDialog.hpp"
#include "S3DEnum.hpp"
#include "ui_scenery3dDialog.h"

class Scenery3d;
struct SceneInfo;

class Scenery3dDialog : public StelDialog
{
	Q_OBJECT
public:
	Scenery3dDialog(QObject* parent = Q_NULLPTR);
	virtual ~Scenery3dDialog() Q_DECL_OVERRIDE;

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

protected:
	virtual void createDialogContent() Q_DECL_OVERRIDE;

private slots:
	void on_comboBoxShadowFiltering_currentIndexChanged(int index);
	void on_comboBoxCubemapMode_currentIndexChanged(int index);
	void on_sliderTorchStrength_valueChanged(int value);
	void on_sliderTorchRange_valueChanged(int value);
	void on_checkBoxDefaultScene_stateChanged(int value);
	void on_comboBoxCubemapSize_currentIndexChanged(int index);
	void on_comboBoxShadowmapSize_currentIndexChanged(int index);

	void scenery3dChanged(QListWidgetItem* item);

	void updateTorchStrength(float val);
	void updateTorchRange(float val);
	void updateLazyDrawingInterval(double val);
	void updateShadowFilterQuality(S3DEnum::ShadowFilterQuality quality);
	void updateSecondDominantFaceEnabled();

	void updateCurrentScene(const SceneInfo& sceneInfo);

	void initResolutionCombobox(QComboBox* cb);
	void setResolutionCombobox(QComboBox* cb, uint val);

	void updateShortcutStrings();

	void updateToolTipStrings();

private:
	//! Connects the UI to update events from the Scenery3dMgr
	void createUpdateConnections();
	//! This updates the whole GUI to represent current Scenery3dMgr values
	void setToInitialValues();
	void updateTextBrowser(const SceneInfo& si);

	QVector<QAbstractButton*> shortcutButtons;
	Ui_scenery3dDialogForm* ui;
	Scenery3d* mgr;
};

#endif
