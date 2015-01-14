#include "Scenery3dDialog.hpp"
#include "Scenery3dDialog_p.hpp"
#include "Scenery3dMgr.hpp"
#include "SceneInfo.hpp"
#include "S3DEnum.hpp"

#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

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
		ui->retranslateUi(dialog);
}

void Scenery3dDialog::createDialogContent()
{
	//manager should be created at this point
	mgr = GETSTELMODULE(Scenery3dMgr);
	Q_ASSERT(mgr);

	//load Ui from form file
	ui->setupUi(dialog);

	//change ui a bit
	StelAction* ac = StelApp::getInstance().getStelActionManager()->findAction("actionShow_Scenery3d_torchlight");
	if(ac)
		ui->labelTorchStrength->setText(ui->labelTorchStrength->text().arg(ac->getShortcut().toString(QKeySequence::NativeText)));
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

	//connectSlotsByName does not work in our case (because this class does not "own" the GUI in the Qt sense)
	//the "new" syntax is extremly ugly in case signals have overloads
	connect(ui->comboBoxCubemapMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxCubemapMode_currentIndexChanged);
	connect(ui->comboBoxShadowFiltering, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Scenery3dDialog::on_comboBoxShadowFiltering_currentIndexChanged);

	connect(ui->sliderTorchStrength, &QSlider::valueChanged, this, &Scenery3dDialog::on_sliderTorchStrength_valueChanged);
	connect(ui->checkBoxDefaultScene, &QCheckBox::stateChanged, this, &Scenery3dDialog::on_checkBoxDefaultScene_stateChanged);

	//connect Scenery3d update events
	connect(mgr, SIGNAL(enablePixelLightingChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableShadowsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableBumpsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, &Scenery3dMgr::cubemappingModeChanged, this, &Scenery3dDialog::updateFromManager);
	connect(mgr, SIGNAL(isGeometryShaderSupportedChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, &Scenery3dMgr::torchStrengthChanged, this, &Scenery3dDialog::updateFromManager);

	//this is the modern type-safe way to connect signals to slots (with compile-time checking)
	connect(mgr, &Scenery3dMgr::shadowFilterQualityChanged,this, &Scenery3dDialog::updateFromManager);

	connect(mgr, &Scenery3dMgr::currentSceneChanged, this, &Scenery3dDialog::updateCurrentScene);

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

	if(current.isValid)
	{
		QList<QListWidgetItem*> currentItems = l->findItems(current.name, Qt::MatchExactly);
		if (currentItems.size() > 0) {
			l->setCurrentItem(currentItems.at(0));

			ui->checkBoxDefaultScene->blockSignals(true);
			ui->checkBoxDefaultScene->setChecked(current.id == mgr->getDefaultScenery3dID());
			ui->checkBoxDefaultScene->blockSignals(false);
			ui->checkBoxDefaultScene->setVisible(true);
		}
		ui->scenery3dTextBrowser->setHtml(getHtmlDescription(current));
	}
	l->blockSignals(false);

	updateFromManager();
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
		ui->scenery3dTextBrowser->setHtml(getHtmlDescription(sceneInfo));

		ui->checkBoxDefaultScene->blockSignals(true);
		ui->checkBoxDefaultScene->setVisible(true);
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

		ui->scenery3dTextBrowser->setHtml(getHtmlDescription(info));
		ui->checkBoxDefaultScene->setVisible(true);
	}
	else
	{
		ui->checkBoxDefaultScene->setVisible(false);
		ui->checkBoxDefaultScene->setChecked(false);
	}
}

QString Scenery3dDialog::getHtmlDescription(const SceneInfo &si) const
{
	QString desc = QString("<h3>%1</h3>").arg(si.name);
	desc += si.description;
	desc+="<br><br>";
	desc+="<b>"+q_("Author: ")+"</b>";
	desc+= si.author;
	return desc;
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

	ui->comboBoxShadowFiltering->setCurrentIndex(mgr->getShadowFilterQuality());
	ui->comboBoxCubemapMode->setCurrentIndex(mgr->getCubemappingMode());

	ui->sliderTorchStrength->blockSignals(true);
	ui->sliderTorchStrength->setValue(mgr->getTorchStrength() * 100.0f);
	ui->sliderTorchStrength->blockSignals(false);

	CubemapModeListModel* model = dynamic_cast<CubemapModeListModel*>(ui->comboBoxCubemapMode->model());
	model->setGSSupported(mgr->getIsGeometryShaderSupported());
}
