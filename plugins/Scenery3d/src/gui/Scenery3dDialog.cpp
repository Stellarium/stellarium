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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Scenery3dDialog.hpp"
#include "Scenery3dDialog_p.hpp"
#include "Scenery3dMgr.hpp"
#include "SceneInfo.hpp"
#include "S3DEnum.hpp"
#include "StoredViewDialog.hpp"

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

#include <QLineEdit>
#include <QStandardItemModel>
#include <QTimer>

Scenery3dDialog::Scenery3dDialog(QObject* parent) : StelDialog(parent)
{
	ui = new Ui_scenery3dDialogForm;
}

Scenery3dDialog::~Scenery3dDialog()
{
	delete ui;
}

void Scenery3dDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);

		//update description
		SceneInfo si = mgr->getLoadingScene(); //the scene that is currently being loaded
		if(!si.isValid)
			si = mgr->getCurrentScene(); //the scene that is currently displayed
		updateTextBrowser(si);

		foreach(QAbstractButton* but, shortcutButtons)
		{
			//replace stored text with re-translated one
			but->setProperty("stelOriginalText",but->text());
		}

		updateShortcutStrings();
	}
}

void Scenery3dDialog::createDialogContent()
{
	//manager should be created at this point
	mgr = GETSTELMODULE(Scenery3dMgr);
	Q_ASSERT(mgr);

	//additionally, Scenery3dMgr::init should have been called to make sure the correct values are set for hardware support

	//load Ui from form file
	ui->setupUi(dialog);

	//hook up retranslate event
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &Scenery3dDialog::retranslate);

	//change ui a bit
	ui->comboBoxCubemapMode->setModel(new CubemapModeListModel(ui->comboBoxCubemapMode));

	//connect UI events
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->scenery3dListWidget, &QListWidget::currentItemChanged, this, &Scenery3dDialog::scenery3dChanged);

	//checkboxes can connect directly to manager
	connect(ui->checkBoxEnablePixelLight, SIGNAL(clicked(bool)),mgr,
		SLOT(setEnablePixelLighting(bool)));
	connect(ui->checkBoxEnableShadows, SIGNAL(clicked(bool)), mgr,
		SLOT(setEnableShadows(bool)));
	connect(ui->checkBoxEnableBump, SIGNAL(clicked(bool)), mgr,
		SLOT(setEnableBumps(bool)));
	connect(ui->checkBoxEnableLazyDrawing, &QCheckBox::clicked, mgr, &Scenery3dMgr::setEnableLazyDrawing);
	connect(ui->checkBoxDominantFace, &QCheckBox::clicked, mgr, &Scenery3dMgr::setOnlyDominantFaceWhenMoving);
	connect(ui->checkBoxSecondDominantFace, &QCheckBox::clicked, mgr, &Scenery3dMgr::setSecondDominantFaceWhenMoving);
	connect(ui->checkBoxPCSS, &QCheckBox::clicked, mgr, &Scenery3dMgr::setEnablePCSS);
	connect(ui->spinLazyDrawingInterval, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), mgr, &Scenery3dMgr::setLazyDrawingInterval);

	connect(ui->checkBoxSimpleShadows, &QCheckBox::clicked, mgr, &Scenery3dMgr::setUseSimpleShadows);
	connect(ui->checkBoxCubemapShadows, &QCheckBox::clicked, mgr, &Scenery3dMgr::setUseFullCubemapShadows);

	//hook up some Scenery3d actions
	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();
	StelAction* ac = acMgr->findAction("actionShow_Scenery3d_torchlight");
	if(ac)
	{
		ui->checkBoxTorchlight->setProperty("stelActionKey",ac->getId());
		ui->checkBoxTorchlight->setProperty("stelOriginalText",ui->checkBoxTorchlight->text());
		ui->checkBoxTorchlight->setChecked(ac->isChecked());
		connect(ac,&StelAction::toggled,ui->checkBoxTorchlight, &QCheckBox::setChecked);
		connect(ui->checkBoxTorchlight,&QCheckBox::toggled,ac, &StelAction::setChecked);
		connect(ac,&StelAction::changed,this,&Scenery3dDialog::updateShortcutStrings);
		shortcutButtons.append(ui->checkBoxTorchlight);
	}
	ac = acMgr->findAction("actionShow_Scenery3d_locationinfo");
	if(ac)
	{
		ui->checkBoxShowGridCoordinates->setProperty("stelActionKey",ac->getId());
		ui->checkBoxShowGridCoordinates->setProperty("stelOriginalText",ui->checkBoxShowGridCoordinates->text());
		ui->checkBoxShowGridCoordinates->setChecked(ac->isChecked());
		connect(ac,&StelAction::toggled,ui->checkBoxShowGridCoordinates, &QCheckBox::setChecked);
		connect(ui->checkBoxShowGridCoordinates,&QCheckBox::toggled,ac, &StelAction::setChecked);
		connect(ac,&StelAction::changed,this,&Scenery3dDialog::updateShortcutStrings);
		shortcutButtons.append(ui->checkBoxShowGridCoordinates);
	}

	//connectSlotsByName does not work in our case (because this class does not "own" the GUI in the Qt sense)
	//the "new" syntax is extremly ugly in case signals have overloads
	connect(ui->comboBoxCubemapMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxCubemapMode_currentIndexChanged);
	connect(ui->comboBoxShadowFiltering, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxShadowFiltering_currentIndexChanged);
	connect(ui->comboBoxCubemapSize,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxCubemapSize_currentIndexChanged);
	connect(ui->comboBoxShadowmapSize,static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxShadowmapSize_currentIndexChanged);

	connect(ui->sliderTorchStrength, &QSlider::valueChanged, this, &Scenery3dDialog::on_sliderTorchStrength_valueChanged);
	connect(ui->sliderTorchRange, &QSlider::valueChanged, this, &Scenery3dDialog::on_sliderTorchRange_valueChanged);
	connect(ui->checkBoxDefaultScene, &QCheckBox::stateChanged, this, &Scenery3dDialog::on_checkBoxDefaultScene_stateChanged);

	connect(ui->pushButtonOpenStoredViewDialog, &QPushButton::clicked, mgr, &Scenery3dMgr::showStoredViewDialog);

	updateShortcutStrings();
	createUpdateConnections();

	// Fill the scenery list
	QListWidget* l = ui->scenery3dListWidget;
	l->blockSignals(true);
	l->addItems( SceneInfo::getAllSceneNames() );

	SceneInfo current = mgr->getCurrentScene();

	if(!current.isValid)
	{
		//try to see if there is a scene currently in the loading process, and use this for gui selection
		current = mgr->getLoadingScene();
	}

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->scenery3dTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->checkBoxDefaultScene->setVisible(false);
	ui->pushButtonOpenStoredViewDialog->setVisible(false);

	if(current.isValid)
	{
		QList<QListWidgetItem*> currentItems = l->findItems(current.name, Qt::MatchExactly);
		if (currentItems.size() > 0) {
			l->setCurrentItem(currentItems.at(0));

			ui->checkBoxDefaultScene->blockSignals(true);
			ui->checkBoxDefaultScene->setChecked(current.id == mgr->getDefaultScenery3dID());
			ui->checkBoxDefaultScene->blockSignals(false);
			ui->checkBoxDefaultScene->setVisible(true);
			ui->pushButtonOpenStoredViewDialog->setVisible(true);
		}
		updateTextBrowser(current);
	}
	l->blockSignals(false);

	//init resolution boxes
	initResolutionCombobox(ui->comboBoxCubemapSize);
	initResolutionCombobox(ui->comboBoxShadowmapSize);

	setToInitialValues();
}

void Scenery3dDialog::createUpdateConnections()
{
	//connect Scenery3d update events
	connect(mgr, &Scenery3dMgr::enablePixelLightingChanged, ui->checkBoxEnablePixelLight, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::enablePixelLightingChanged, ui->checkBoxEnableShadows, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enablePixelLightingChanged, ui->checkBoxEnableBump, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enableShadowsChanged, ui->checkBoxEnableShadows, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::enableBumpsChanged, ui->checkBoxEnableBump, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::enablePCSSChanged,ui->checkBoxPCSS,&QCheckBox::setChecked);

	connect(mgr, &Scenery3dMgr::enableShadowsChanged, ui->checkBoxSimpleShadows, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enableShadowsChanged, ui->checkBoxCubemapShadows, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::useSimpleShadowsChanged, ui->checkBoxSimpleShadows, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::useFullCubemapShadowsChanged, ui->checkBoxCubemapShadows, &QCheckBox::setChecked);

	connect(mgr, &Scenery3dMgr::cubemappingModeChanged, ui->comboBoxCubemapMode, &QComboBox::setCurrentIndex);
	connect(mgr, &Scenery3dMgr::shadowFilterQualityChanged, this, &Scenery3dDialog::updateShadowFilterQuality);

	connect(mgr, &Scenery3dMgr::torchStrengthChanged, this, &Scenery3dDialog::updateTorchStrength);
	connect(mgr, &Scenery3dMgr::torchRangeChanged, this, &Scenery3dDialog::updateTorchRange);

	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->checkBoxEnableLazyDrawing, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->labelLazyDrawingInterval, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->spinLazyDrawingInterval, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->checkBoxDominantFace, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, this, &Scenery3dDialog::updateSecondDominantFaceEnabled);

	connect(mgr, &Scenery3dMgr::lazyDrawingIntervalChanged, this, &Scenery3dDialog::updateLazyDrawingInterval);
	connect(mgr, &Scenery3dMgr::onlyDominantFaceWhenMovingChanged, ui->checkBoxDominantFace, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::onlyDominantFaceWhenMovingChanged, this, &Scenery3dDialog::updateSecondDominantFaceEnabled);
	connect(mgr, &Scenery3dMgr::secondDominantFaceWhenMovingChanged, ui->checkBoxSecondDominantFace, &QCheckBox::setChecked);

	connect(mgr, &Scenery3dMgr::currentSceneChanged, this, &Scenery3dDialog::updateCurrentScene);
}

void Scenery3dDialog::updateShortcutStrings()
{
	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();

	foreach(QAbstractButton* bt, shortcutButtons)
	{
		QVariant v = bt->property("stelActionKey");
		QVariant t = bt->property("stelOriginalText");
		if(v.isValid() && t.isValid())
		{
			QString s = v.toString();
			QString text = t.toString();
			StelAction* ac = acMgr->findAction(s);
			if(ac)
			{
				bt->setText(text.arg(ac->getShortcut().toString(QKeySequence::NativeText)));
			}
		}
	}
}

void Scenery3dDialog::initResolutionCombobox(QComboBox *cb)
{
	bool oldval = cb->blockSignals(true);

	uint maxResolution = mgr->getMaximumFramebufferSize();
	for(uint i = 256;i<=qMin(4096u,maxResolution);i*=2)
	{
		cb->addItem(QString::number(i),i);
	}

	cb->blockSignals(oldval);
}

void Scenery3dDialog::on_comboBoxCubemapSize_currentIndexChanged(int index)
{
	bool ok;
	uint val = ui->comboBoxCubemapSize->currentData().toUInt(&ok);
	if(ok)
	{
		mgr->setCubemapSize(val);
	}
}

void Scenery3dDialog::on_comboBoxShadowmapSize_currentIndexChanged(int index)
{
	bool ok;
	uint val = ui->comboBoxShadowmapSize->currentData().toUInt(&ok);
	if(ok)
	{
		mgr->setShadowmapSize(val);
	}
}

void Scenery3dDialog::on_comboBoxShadowFiltering_currentIndexChanged(int index)
{
	qDebug()<<index;
	mgr->setShadowFilterQuality(static_cast<S3DEnum::ShadowFilterQuality>(index));
}

void Scenery3dDialog::on_comboBoxCubemapMode_currentIndexChanged(int index)
{
	mgr->setCubemappingMode(static_cast<S3DEnum::CubemappingMode>(index));
}

void Scenery3dDialog::on_sliderTorchStrength_valueChanged(int value)
{
	float val =  ((float)value)  / ui->sliderTorchStrength->maximum();
	mgr->setTorchStrength(val);
}

void Scenery3dDialog::on_sliderTorchRange_valueChanged(int value)
{
	float val = value / 100.0f;
	mgr->setTorchRange(val);
}

void Scenery3dDialog::updateCurrentScene(const SceneInfo &sceneInfo)
{
	if(sceneInfo.isValid)
	{
		ui->scenery3dListWidget->blockSignals(true);
		QList<QListWidgetItem*> currentItems = ui->scenery3dListWidget->findItems(sceneInfo.name, Qt::MatchExactly);
		if(currentItems.size()>0)
		{
			ui->scenery3dListWidget->setCurrentItem(currentItems.at(0));
		}
		ui->scenery3dListWidget->blockSignals(false);
		updateTextBrowser(sceneInfo);

		ui->checkBoxDefaultScene->blockSignals(true);
		ui->checkBoxDefaultScene->setVisible(true);
		ui->pushButtonOpenStoredViewDialog->setVisible(true);
		ui->checkBoxDefaultScene->setChecked(sceneInfo.id == mgr->getDefaultScenery3dID());
		ui->checkBoxDefaultScene->blockSignals(false);
	}
}

void Scenery3dDialog::on_checkBoxDefaultScene_stateChanged(int value)
{
	QList<QListWidgetItem*> sel = ui->scenery3dListWidget->selectedItems();
	if(sel.size()==0)
		return;

	if(value)
	{
		QString id = SceneInfo::getIDFromName(sel.at(0)->text());
		if(!id.isEmpty())
			mgr->setDefaultScenery3dID(id);
	}
	else
		mgr->setDefaultScenery3dID("");
}

void Scenery3dDialog::scenery3dChanged(QListWidgetItem* item)
{
	SceneInfo info = mgr->loadScenery3dByName(item->text());

	if(info.isValid) //this if makes sure the .ini has been loaded
	{
		ui->checkBoxDefaultScene->blockSignals(true);
		ui->checkBoxDefaultScene->setChecked(info.id == mgr->getDefaultScenery3dID());
		ui->checkBoxDefaultScene->blockSignals(false);

		updateTextBrowser(info);
		ui->checkBoxDefaultScene->setVisible(true);
	}
	else
	{
		ui->pushButtonOpenStoredViewDialog->setVisible(false);
		ui->checkBoxDefaultScene->setVisible(false);
		ui->checkBoxDefaultScene->setChecked(false);
	}
}

void Scenery3dDialog::updateTextBrowser(const SceneInfo &si)
{
	if(!si.isValid)
	{
		ui->scenery3dTextBrowser->setHtml("");
		ui->scenery3dTextBrowser->setSearchPaths(QStringList());
	}
	else
	{
		QStringList list;
		list<<si.fullPath;
		ui->scenery3dTextBrowser->setSearchPaths(list);

		//first, try to find a localized description
		QString desc = si.getLocalizedHTMLDescription();
		if(desc.isEmpty())
		{
			//use the "old" way to create an description from data contained in the .ini
			desc = QString("<h3>%1</h3>").arg(si.name);
			desc+= si.description;
			desc+="<br><br>";
			desc+="<b>"+q_("Author: ")+"</b>";
			desc+= si.author;
		}

		ui->scenery3dTextBrowser->setHtml(desc);
	}
}

void Scenery3dDialog::setResolutionCombobox(QComboBox *cb, uint val)
{
	cb->blockSignals(true);
	int idx = cb->findData(val);
	if(idx>=0)
	{
		//valid entry
		cb->setCurrentIndex(idx);
	}
	else
	{
		//add entry for the current value, making sure it is at first place
		cb->clear();
		cb->addItem(QString::number(val),val);
		initResolutionCombobox(cb);
		cb->setCurrentIndex(0);
	}
	cb->blockSignals(false);
}

void Scenery3dDialog::updateShadowFilterQuality(S3DEnum::ShadowFilterQuality quality)
{
	ui->checkBoxPCSS->setEnabled(quality == S3DEnum::SFQ_HIGH || quality == S3DEnum::SFQ_LOW);
	ui->comboBoxShadowFiltering->setCurrentIndex(quality);
}

void Scenery3dDialog::updateTorchRange(float val)
{
	ui->sliderTorchRange->blockSignals(true);
	ui->sliderTorchRange->setValue(val * 100.0f);
	ui->sliderTorchRange->blockSignals(false);
}

void Scenery3dDialog::updateTorchStrength(float val)
{
	ui->sliderTorchStrength->blockSignals(true);
	ui->sliderTorchStrength->setValue(val * 100.0f);
	ui->sliderTorchStrength->blockSignals(false);
}

void Scenery3dDialog::updateLazyDrawingInterval(float val)
{
	ui->spinLazyDrawingInterval->blockSignals(true);
	ui->spinLazyDrawingInterval->setValue(val);
	ui->spinLazyDrawingInterval->blockSignals(false);
}

void Scenery3dDialog::updateSecondDominantFaceEnabled()
{
	ui->checkBoxSecondDominantFace->setEnabled(mgr->getOnlyDominantFaceWhenMoving() && mgr->getEnableLazyDrawing());
}

// Update the widget to make sure it is synchrone if a value was changed programmatically
void Scenery3dDialog::setToInitialValues()
{
	bool pix = mgr->getEnablePixelLighting();
	ui->checkBoxEnablePixelLight->setChecked(pix);

	ui->checkBoxEnableBump->setChecked(mgr->getEnableBumps());
	ui->checkBoxEnableBump->setEnabled(pix);
	ui->checkBoxEnableShadows->setChecked(mgr->getEnableShadows());
	ui->checkBoxEnableShadows->setEnabled(pix);
	ui->checkBoxPCSS->setChecked(mgr->getEnablePCSS());

	ui->checkBoxSimpleShadows->setEnabled(mgr->getEnableShadows());
	ui->checkBoxCubemapShadows->setEnabled(mgr->getEnableShadows());
	ui->checkBoxSimpleShadows->setChecked(mgr->getUseSimpleShadows());
	ui->checkBoxCubemapShadows->setChecked(mgr->getUseFullCubemapShadows());

	updateShadowFilterQuality(mgr->getShadowFilterQuality());

	ui->comboBoxCubemapMode->setCurrentIndex(mgr->getCubemappingMode());

	updateTorchStrength(mgr->getTorchStrength());
	updateTorchRange(mgr->getTorchRange());

	bool val = mgr->getEnableLazyDrawing();
	ui->checkBoxEnableLazyDrawing->setChecked(val);

	ui->checkBoxDominantFace->setChecked(mgr->getOnlyDominantFaceWhenMoving());
	updateSecondDominantFaceEnabled();
	ui->checkBoxSecondDominantFace->setChecked(mgr->getSecondDominantFaceWhenMoving());

	ui->labelLazyDrawingInterval->setEnabled(val);
	ui->spinLazyDrawingInterval->setEnabled(val);
	ui->checkBoxDominantFace->setEnabled(val);

	updateLazyDrawingInterval(mgr->getLazyDrawingInterval());

	setResolutionCombobox(ui->comboBoxCubemapSize,mgr->getCubemapSize());
	setResolutionCombobox(ui->comboBoxShadowmapSize,mgr->getShadowmapSize());

	//hide some stuff depending on hardware support
	if(!mgr->getIsShadowFilteringSupported())
	{
		ui->labelFilterQuality->setVisible(false);
		ui->comboBoxShadowFiltering->setVisible(false);
		ui->checkBoxPCSS->setVisible(false);
	}

	if(!mgr->getAreShadowsSupported())
	{
		ui->labelShadowmapSize->setVisible(false);
		ui->comboBoxShadowmapSize->setVisible(false);
		ui->checkBoxEnableShadows->setVisible(false);
		ui->checkBoxCubemapShadows->setVisible(false);
		ui->checkBoxSimpleShadows->setVisible(false);
	}
}
