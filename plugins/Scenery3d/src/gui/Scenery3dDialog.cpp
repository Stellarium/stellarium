#include "Scenery3dDialog.hpp"
#include "Scenery3dMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"

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

    // Fill the scenery list
    QListWidget* l = ui->scenery3dListWidget;
    l->blockSignals(true);
    l->clear();
    Scenery3dMgr* smgr = GETSTELMODULE(Scenery3dMgr);
    l->addItems(smgr->getAllScenery3dNames());
    l->setCurrentItem(l->findItems(smgr->getCurrentScenery3dName(), Qt::MatchExactly).at(0));
    l->blockSignals(false);
    //ui->scenery3dTextBrowser->setHtml(smgr->getCurrentScenery3dHtmlDescription());
    //ui->useAsDefaultLandscapeCheckBox->setChecked(lmgr->getDefaultLandscapeID()==lmgr->getCurrentLandscapeID());
    //ui->useAsDefaultLandscapeCheckBox->setEnabled(lmgr->getDefaultLandscapeID()!=lmgr->getCurrentLandscapeID());
}
