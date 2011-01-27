#include "Scenery3dDialog.hpp"
#include "Scenery3dMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"

Scenery3dDialog::Scenery3dDialog()
{
    ui = new Ui_scenery3dDialogForm;
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
            SLOT(renderingOptionsChanged()));

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

    renderingOptionsChanged();
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

void Scenery3dDialog::renderingOptionsChanged(void)
{
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    smgr->setEnableShadows(ui->checkBoxEnableShadows->isChecked());
}
