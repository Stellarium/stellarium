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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StoredViewDialog.hpp"
#include "StoredViewDialog_p.hpp"
#include "Scenery3d.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

StoredViewDialog::StoredViewDialog(QObject *parent) : StelDialog("Scenery3dViews", parent), mgr(Q_NULLPTR), viewModel(Q_NULLPTR)
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
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	mgr = GETSTELMODULE(Scenery3d);
	Q_ASSERT(mgr);

	connect(ui->pushButtonAddView, &QPushButton::clicked, this, &StoredViewDialog::addUserView);
	connect(ui->pushButtonLoadView, &QPushButton::clicked, this, &StoredViewDialog::loadView);
	//also allow doubleclick to load view
	connect(ui->listView, &QListView::doubleClicked, this, &StoredViewDialog::loadView);
	connect(ui->pushButtonDeleteView, &QPushButton::clicked, this, &StoredViewDialog::deleteView);

	connect(ui->lineEditTitle, &QLineEdit::editingFinished, this, &StoredViewDialog::updateCurrentView);
	connect(ui->textEditDescription, &CustomTextEdit::editingFinished, this, &StoredViewDialog::updateCurrentView);

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
		ui->textEditDescription->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));

	//we use a sorta MVC system here
	viewModel = new StoredViewModel(ui->listView);
	ui->listView->setModel(viewModel);
	connect(ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &StoredViewDialog::updateViewSelection);

	connect(mgr, &Scenery3d::currentSceneChanged, viewModel, &StoredViewModel::setScene);
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
	if(idx.isValid())
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
	else
	{
		//for example deletion of last item in the list
		ui->lineEditTitle->setEnabled(false);
		ui->lineEditTitle->clear();
		ui->textEditDescription->setEnabled(false);
		ui->textEditDescription->clear();
		ui->pushButtonLoadView->setEnabled(false);
		ui->pushButtonDeleteView->setEnabled(false);
	}
}

void StoredViewDialog::resetViewSelection()
{
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
		mgr->setView(viewModel->getViewAtIdx(idx), ui->useDateCheckBox->isChecked());
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
	if (ui->useDateCheckBox->isChecked())
	{
		StelCore *core=StelApp::getInstance().getCore();
		sv.jd=core->getJD();
		sv.jdIsRelevant=true;
	}

	SceneInfo info = viewModel->getScene();
	sv.description = QString(q_("Grid coordinates (%1): %2m, %3m, %4m")).arg(info.gridName)
			.arg(sv.position[0], 0, 'f', 2).arg(sv.position[1],0,'f',2).arg(sv.position[2],0,'f',2);

	QModelIndex idx = viewModel->addUserView(sv);
	ui->listView->setCurrentIndex(idx);
}
