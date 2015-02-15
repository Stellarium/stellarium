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
		updateShortcutStrings();
	}
}

void Scenery3dDialog::createDialogContent()
{
	//manager should be created at this point
	mgr = GETSTELMODULE(Scenery3dMgr);
	Q_ASSERT(mgr);

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

	//hook up some Scenery3d actions
	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();
	StelAction* ac = acMgr->findAction("actionShow_Scenery3d_torchlight");
	if(ac)
	{
		ui->checkBoxTorchlight->setProperty("stelActionKey",ac->getId());
		ui->checkBoxTorchlight->setChecked(ac->isChecked());
		connect(ac,&StelAction::toggled,ui->checkBoxTorchlight, &QCheckBox::setChecked);
		connect(ui->checkBoxTorchlight,&QCheckBox::toggled,ac, &StelAction::setChecked);
		//reacting to key combo changes does not work yet
		//connect(ac,&StelAction::changed,this,&Scenery3dDialog::updateShortcutStrings);
		shortcutButtons.append(ui->checkBoxTorchlight);
	}
	ac = acMgr->findAction("actionShow_Scenery3d_locationinfo");
	if(ac)
	{
		ui->checkBoxShowGridCoordinates->setProperty("stelActionKey",ac->getId());
		ui->checkBoxShowGridCoordinates->setChecked(ac->isChecked());
		connect(ac,&StelAction::toggled,ui->checkBoxShowGridCoordinates, &QCheckBox::setChecked);
		connect(ui->checkBoxShowGridCoordinates,&QCheckBox::toggled,ac, &StelAction::setChecked);
		//connect(ac,&StelAction::changed,this,&Scenery3dDialog::updateShortcutStrings);
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

	updateFromManager();
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

	connect(mgr, &Scenery3dMgr::cubemappingModeChanged, ui->comboBoxCubemapMode, &QComboBox::setCurrentIndex);
	connect(mgr, &Scenery3dMgr::isGeometryShaderSupportedChanged, dynamic_cast<CubemapModeListModel*>(ui->comboBoxCubemapMode->model()), &CubemapModeListModel::setGSSupported);
	connect(mgr, &Scenery3dMgr::shadowFilterQualityChanged, ui->comboBoxShadowFiltering, &QComboBox::setCurrentIndex);

	connect(mgr, &Scenery3dMgr::torchStrengthChanged, this, &Scenery3dDialog::updateTorchStrength);
	connect(mgr, &Scenery3dMgr::torchRangeChanged, this, &Scenery3dDialog::updateTorchRange);

	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->checkBoxEnableLazyDrawing, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->labelLazyDrawingInterval, &QCheckBox::setVisible);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->spinLazyDrawingInterval, &QCheckBox::setVisible);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->checkBoxDominantFace, &QCheckBox::setVisible);
	connect(mgr, &Scenery3dMgr::enableLazyDrawingChanged, ui->checkBoxSecondDominantFace, &QCheckBox::setVisible);

	connect(mgr, &Scenery3dMgr::lazyDrawingIntervalChanged, this, &Scenery3dDialog::updateLazyDrawingInterval);
	connect(mgr, &Scenery3dMgr::onlyDominantFaceWhenMovingChanged, ui->checkBoxDominantFace, &QCheckBox::setChecked);
	connect(mgr, &Scenery3dMgr::onlyDominantFaceWhenMovingChanged, ui->checkBoxSecondDominantFace, &QCheckBox::setEnabled);
	connect(mgr, &Scenery3dMgr::secondDominantFaceWhenMovingChanged, ui->checkBoxSecondDominantFace, &QCheckBox::setChecked);

	connect(mgr, &Scenery3dMgr::currentSceneChanged, this, &Scenery3dDialog::updateCurrentScene);
}

void Scenery3dDialog::updateShortcutStrings()
{
	StelActionMgr* acMgr = StelApp::getInstance().getStelActionManager();

	foreach(QAbstractButton* bt, shortcutButtons)
	{
		QVariant v = bt->property("stelActionKey");
		if(v.isValid())
		{
			QString s = v.toString();
			StelAction* ac = acMgr->findAction(s);
			if(ac)
			{
				bt->setText(bt->text().arg(ac->getShortcut().toString(QKeySequence::NativeText)));
			}
		}
	}
}

void Scenery3dDialog::initResolutionCombobox(QComboBox *cb)
{
	bool oldval = cb->blockSignals(true);
	for(uint i = 256;i<=4096;i*=2)
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

// Update the widget to make sure it is synchrone if a value was changed programmatically
void Scenery3dDialog::updateFromManager()
{
	bool pix = mgr->getEnablePixelLighting();
	ui->checkBoxEnablePixelLight->setChecked(pix);

	ui->checkBoxEnableBump->setChecked(mgr->getEnableBumps());
	ui->checkBoxEnableBump->setEnabled(pix);
	ui->checkBoxEnableShadows->setChecked(mgr->getEnableShadows());
	ui->checkBoxEnableShadows->setEnabled(pix);
	ui->checkBoxPCSS->setChecked(mgr->getEnablePCSS());

	ui->comboBoxShadowFiltering->setCurrentIndex(mgr->getShadowFilterQuality());
	ui->comboBoxCubemapMode->setCurrentIndex(mgr->getCubemappingMode());

	updateTorchStrength(mgr->getTorchStrength());
	updateTorchRange(mgr->getTorchRange());

	bool val = mgr->getEnableLazyDrawing();
	ui->checkBoxEnableLazyDrawing->setChecked(val);

	ui->checkBoxDominantFace->setChecked(mgr->getOnlyDominantFaceWhenMoving());
	ui->checkBoxSecondDominantFace->setEnabled(mgr->getOnlyDominantFaceWhenMoving());
	ui->checkBoxSecondDominantFace->setChecked(mgr->getSecondDominantFaceWhenMoving());

	ui->labelLazyDrawingInterval->setVisible(val);
	ui->spinLazyDrawingInterval->setVisible(val);
	ui->checkBoxDominantFace->setVisible(val);
	ui->checkBoxSecondDominantFace->setVisible(val);

	updateLazyDrawingInterval(mgr->getLazyDrawingInterval());

	setResolutionCombobox(ui->comboBoxCubemapSize,mgr->getCubemapSize());
	setResolutionCombobox(ui->comboBoxShadowmapSize,mgr->getShadowmapSize());

	CubemapModeListModel* model = dynamic_cast<CubemapModeListModel*>(ui->comboBoxCubemapMode->model());
	model->setGSSupported(mgr->getIsGeometryShaderSupported());
}
