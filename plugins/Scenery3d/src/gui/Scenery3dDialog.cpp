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

	//connect Scenery3d update events
	connect(mgr, SIGNAL(enablePixelLightingChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableShadowsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableBumpsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, &Scenery3dMgr::cubemappingModeChanged, this, &Scenery3dDialog::updateFromManager);
	connect(mgr, SIGNAL(isGeometryShaderSupportedChanged(bool)), SLOT(updateFromManager()));

	//this is the modern type-safe way to connect signals to slots (with compile-time checking)
	connect(mgr, &Scenery3dMgr::shadowFilterQualityChanged,this, &Scenery3dDialog::updateFromManager);

    // Fill the scenery list
    QListWidget* l = ui->scenery3dListWidget;
    l->blockSignals(true);
    l->addItems( SceneInfo::getAllSceneNames() );

    SceneInfo current = mgr->getCurrentScene();

    StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
    Q_ASSERT(gui);
    ui->scenery3dTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

    if(current.isValid)
    {
	    QList<QListWidgetItem*> currentItems = l->findItems(current.name, Qt::MatchExactly);
	    if (currentItems.size() > 0) {
		    l->setCurrentItem(currentItems.at(0));
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


void Scenery3dDialog::scenery3dChanged(QListWidgetItem* item)
{
	SceneInfo info = mgr->loadScenery3dByName(item->text());
	if(info.isValid) //this if makes sure the .ini has been loaded
	{
		ui->scenery3dTextBrowser->setHtml(getHtmlDescription(info));
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

	CubemapModeListModel* model = dynamic_cast<CubemapModeListModel*>(ui->comboBoxCubemapMode->model());
	model->setGSSupported(mgr->getIsGeometryShaderSupported());
}
