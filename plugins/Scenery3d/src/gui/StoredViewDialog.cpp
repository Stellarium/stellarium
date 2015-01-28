#include "StoredViewDialog.hpp"
#include "Scenery3dMgr.hpp"
#include "SceneInfo.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"


class StoredViewModel : public QAbstractListModel
{
public:
	StoredViewModel(QObject* parent = NULL) : QAbstractListModel(parent)
	{ }

	int rowCount(const QModelIndex &parent) const
	{
		return global.size() + user.size();
	}

	QVariant data(const QModelIndex &index, int role) const
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			return getViewAtIdx(index.row()).label;
		}
		return QVariant();
	}

	const StoredView& getViewAtIdx(int idx) const
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}

	StoredView& getViewAtIdx(int idx)
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}

	void setData(const StoredViewList& global, const StoredViewList& user)
	{
		this->beginResetModel();
		this->global = global;
		this->user = user;
		this->endResetModel();
	}
private:
	StoredViewList global, user;
};

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

	connect(mgr, &Scenery3dMgr::currentSceneChanged, this, &StoredViewDialog::initializeList);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &StoredViewDialog::updateViewSelection);

	viewModel = new StoredViewModel(ui->listView);
	connect(viewModel, &QAbstractItemModel::modelReset, this, &StoredViewDialog::resetViewSelection);

	ui->listView->setModel(viewModel);

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);


	initializeList();
}

void StoredViewDialog::initializeList()
{
	SceneInfo info = mgr->getCurrentScene();

	StoredViewList global = StoredView::getGlobalViewsForScene(info);
	StoredViewList user = StoredView::getGlobalViewsForScene(info);

	viewModel->setData(global,user);

	ui->pushButtonAddView->setEnabled(info.isValid);
}

void StoredViewDialog::updateViewSelection(const QModelIndex &idx)
{
	StoredView& view = viewModel->getViewAtIdx(idx.row());

	ui->lineEditTitle->setEnabled(true);
	ui->plainTextEditDescription->setEnabled(true);
	ui->lineEditTitle->setReadOnly(view.isGlobal);
	ui->plainTextEditDescription->setReadOnly(view.isGlobal);

	ui->lineEditTitle->setText(view.label);
	ui->plainTextEditDescription->setPlainText(view.description);

	ui->pushButtonDeleteView->setEnabled(!view.isGlobal);
	ui->pushButtonLoadView->setEnabled(true);
}

void StoredViewDialog::resetViewSelection()
{
	qDebug()<<"model reset";

	//disable all
	ui->lineEditTitle->setEnabled(false);
	ui->plainTextEditDescription->setEnabled(false);
	ui->pushButtonLoadView->setEnabled(false);
}
