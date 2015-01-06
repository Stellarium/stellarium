#include "Scenery3dDialog.hpp"
#include "Scenery3dMgr.hpp"
#include "SceneInfo.hpp"

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
	connect(ui->checkBoxEnableGeometryShader, SIGNAL(clicked(bool)), mgr,
		SLOT(setEnableGeometryShader(bool)));

	//connectSlotsByName does not work in our case
	connect(ui->comboBoxShadowFiltering, SIGNAL(currentIndexChanged(int)),this,
		SLOT(on_comboBoxShadowFiltering_currentIndexChanged(int)));

	//connect Scenery3d update events
	connect(mgr, SIGNAL(enablePixelLightingChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableShadowsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableBumpsChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableShadowsFilterChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableShadowsFilterHQChanged(bool)), SLOT(updateFromManager()));
	connect(mgr, SIGNAL(enableGeometryShaderChanged(bool)),SLOT(updateFromManager()));
	connect(mgr, SIGNAL(isGeometryShaderSupportedChanged(bool)), SLOT(updateFromManager()));

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
	switch(index)
	{
		case 0:
			mgr->setEnableShadowsFilter(false);
			mgr->setEnableShadowsFilterHQ(false);
			break;
		case 1:
			mgr->setEnableShadowsFilterHQ(false);
			mgr->setEnableShadowsFilter(true);
			break;
		case 2:
			mgr->setEnableShadowsFilterHQ(true);
	}
}


void Scenery3dDialog::scenery3dChanged(QListWidgetItem* item)
{
	if(mgr->loadScenery3dByName(item->text())) //this if makes sure the .ini has been loaded
	{
		SceneInfo info = mgr->getLoadingScene();  //use the currently loading scene to display info
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

	if(mgr->getEnableShadowsFilterHQ() && mgr->getEnableShadowsFilter())
		ui->comboBoxShadowFiltering->setCurrentIndex(2);
	else if (mgr->getEnableShadowsFilter())
		ui->comboBoxShadowFiltering->setCurrentIndex(1);
	else
		ui->comboBoxShadowFiltering->setCurrentIndex(0);

	ui->checkBoxEnableGeometryShader->setChecked(mgr->getEnableGeometryShader());
	ui->checkBoxEnableGeometryShader->setEnabled(mgr->getIsGeometryShaderSupported());
}
