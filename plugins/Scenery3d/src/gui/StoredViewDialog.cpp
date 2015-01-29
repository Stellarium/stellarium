#include "StoredViewDialog.hpp"
#include "StoredViewDialog_p.hpp"
#include "Scenery3dMgr.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"




StoredViewDialog::StoredViewDialog(QObject *parent) : StelDialog(parent)
{
	ui = new Ui_storedViewDialogForm;
}

StoredViewDialog::~StoredViewDialog()
{
	delete ui;
}

void StoredViewDialog::retranslate()
{
	if(dialog)
		ui->retranslateUi(dialog);
}

void StoredViewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(ui->closeStelWindow, &QPushButton::clicked, this, &StelDialog::close);

	mgr = GETSTELMODULE(Scenery3dMgr);
	Q_ASSERT(mgr);

	connect(ui->pushButtonAddView, &QPushButton::clicked, this, &StoredViewDialog::addUserView);
	connect(ui->pushButtonLoadView, &QPushButton::clicked, this, &StoredViewDialog::loadView);
	//also allow doubleclick to load view
	connect(ui->listView, &QListView::doubleClicked, this, &StoredViewDialog::loadView);
	connect(ui->pushButtonDeleteView, &QPushButton::clicked, this, &StoredViewDialog::deleteView);

	connect(ui->lineEditTitle, &QLineEdit::editingFinished, this, &StoredViewDialog::updateCurrentView);
	connect(ui->textEditDescription, &CustomTextEdit::editingFinished, this, &StoredViewDialog::updateCurrentView);

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->textEditDescription->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	//we use a sorta MVC system here
	viewModel = new StoredViewModel(ui->listView);
	ui->listView->setModel(viewModel);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &StoredViewDialog::updateViewSelection);

	connect(mgr, &Scenery3dMgr::currentSceneChanged, viewModel, &StoredViewModel::setScene);
	connect(viewModel, &QAbstractItemModel::modelReset, this, &StoredViewDialog::resetViewSelection);
	viewModel->setScene(mgr->getCurrentScene());
}

void StoredViewDialog::updateCurrentView()
{
	int idx = ui->listView->selectionModel()->currentIndex().row();

	if(idx>=0)
	{
		StoredView& view = viewModel->getViewAtIdx(idx);
		view.label = ui->lineEditTitle->text();
		view.description = ui->textEditDescription->toHtml();
		viewModel->updatedAtIdx(idx);
	}
}

void StoredViewDialog::updateViewSelection(const QModelIndex &idx)
{
	StoredView& view = viewModel->getViewAtIdx(idx.row());

	ui->lineEditTitle->setEnabled(true);
	ui->textEditDescription->setEnabled(true);
	ui->lineEditTitle->setReadOnly(view.isGlobal);
	ui->textEditDescription->setReadOnly(view.isGlobal);

	ui->lineEditTitle->setText(view.label);
	ui->textEditDescription->setHtml(view.description);

	ui->pushButtonDeleteView->setEnabled(!view.isGlobal);
	ui->pushButtonLoadView->setEnabled(true);
}

void StoredViewDialog::resetViewSelection()
{
	qDebug()<<"model reset";

	ui->pushButtonAddView->setEnabled(viewModel->getScene().isValid);

	//disable all
	ui->lineEditTitle->setEnabled(false);
	ui->lineEditTitle->clear();
	ui->textEditDescription->setEnabled(false);
	ui->textEditDescription->clear();
	ui->pushButtonLoadView->setEnabled(false);
	ui->pushButtonDeleteView->setEnabled(false);
}

void StoredViewDialog::loadView()
{
	int idx = ui->listView->selectionModel()->currentIndex().row();
	if(idx>=0)
	{
		mgr->setView(viewModel->getViewAtIdx(idx));
	}
}

void StoredViewDialog::deleteView()
{
	int idx = ui->listView->selectionModel()->currentIndex().row();

	if(idx>=0)
	{
		viewModel->deleteView(idx);
	}
}

void StoredViewDialog::addUserView()
{
	StoredView sv = mgr->getCurrentView();
	sv.label = "New user view";

	SceneInfo info = viewModel->getScene();
	sv.description = QString("Grid coordinates (%1): %2m, %3m, %4m").arg(info.gridName)
			.arg(sv.position[0], 0, 'f', 2).arg(sv.position[1],0,'f',2).arg(sv.position[2],0,'f',2);

	QModelIndex idx = viewModel->addUserView(sv);
	ui->listView->setCurrentIndex(idx);
}
