#include <QTimer>

#include "Scenery3dDialog.hpp"
#include "Scenery3dMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"

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

void Scenery3dDialog::languageChanged()
{

}

void Scenery3dDialog::createDialogContent()
{
    ui->setupUi(dialog);
    connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

    connect(ui->scenery3dListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this,
            SLOT(scenery3dChanged(QListWidgetItem*)));

    connect(ui->checkBoxEnableShadows, SIGNAL(stateChanged(int)), this,
            SLOT(renderingShadowmapChanged()));

    connect(ui->checkBoxEnableBump, SIGNAL(stateChanged(int)), this,
            SLOT(renderingBumpChanged()));

    connect(ui->checkBoxEnableFiltering, SIGNAL(stateChanged(int)), this,
            SLOT(renderingShadowsFilterChanged()));

    connect(ui->checkBoxFilterHQ, SIGNAL(stateChanged(int)), this,
            SLOT(renderingShadowsFilterHQChanged()));

    // Fill the scenery list
    QListWidget* l = ui->scenery3dListWidget;
    l->blockSignals(true);
    l->clear();
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    l->addItems(smgr->getAllScenery3dNames());
    QList<QListWidgetItem*> currentItems = l->findItems(smgr->getCurrentScenery3dName(), Qt::MatchExactly);
    if (currentItems.size() > 0) {
        l->setCurrentItem(currentItems.at(0));
    }
    l->blockSignals(false);
    ui->scenery3dTextBrowser->setHtml(smgr->getCurrentScenery3dHtmlDescription());
    //ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getDefaultLandscapeID()==lmgr->getCurrentLandscapeID());
    //ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getDefaultLandscapeID()!=lmgr->getCurrentLandscapeID());

    QTimer* refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(updateFromProgram()));
    refreshTimer->start(200);

    //renderingShadowmapChanged();
    //renderingBumpChanged();
}


void Scenery3dDialog::scenery3dChanged(QListWidgetItem* item)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setCurrentScenery3dName(item->text());
    StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
    Q_ASSERT(gui);
    ui->scenery3dTextBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
    ui->scenery3dTextBrowser->setHtml(smgr->getCurrentScenery3dHtmlDescription());
}

void Scenery3dDialog::renderingShadowmapChanged(void)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setEnableShadows(ui->checkBoxEnableShadows->isChecked());
}

void Scenery3dDialog::renderingBumpChanged(void)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setEnableBumps(ui->checkBoxEnableBump->isChecked());
}

void Scenery3dDialog::renderingShadowsFilterChanged(void)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setEnableShadowsFilter(ui->checkBoxEnableFiltering->isChecked());
}

void Scenery3dDialog::renderingShadowsFilterHQChanged(void)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setEnableShadowsFilterHQ(ui->checkBoxFilterHQ->isChecked());
}


// Update the widget to make sure it is synchrone if a value was changed programmatically
void Scenery3dDialog::updateFromProgram()
{
        if (!dialog->isVisible())
                return;

    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    Q_ASSERT(smgr);
    ui->checkBoxEnableBump->setChecked(smgr->getEnableBumps());
    ui->checkBoxEnableShadows->setChecked(smgr->getEnableShadows());
    ui->checkBoxEnableFiltering->setChecked(smgr->getEnableShadowsFilter());
    ui->checkBoxFilterHQ->setChecked(smgr->getEnableShadowsFilterHQ());
}
