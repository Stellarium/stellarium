#ifndef _SCENERY3DDIALOG_HPP_
#define _SCENERY3DDIALOG_HPP_

#include "StelDialog.hpp"
#include "S3DEnum.hpp"
#include "ui_scenery3dDialog.h"

class Scenery3dMgr;
struct SceneInfo;

class Scenery3dDialog : public StelDialog
{
	Q_OBJECT
public:
	Scenery3dDialog(QObject* parent = NULL);
	~Scenery3dDialog();

public slots:
	void retranslate();

protected:
	void createDialogContent();

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
	void updateLazyDrawingInterval(float val);
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
	Scenery3dMgr* mgr;
};

#endif
