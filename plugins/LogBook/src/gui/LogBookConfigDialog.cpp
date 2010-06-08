/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "LogBook.hpp"
#include "LogBookCommon.hpp"
#include "LogBookConfigDialog.hpp"
#include "dataMappers/BarlowsDataMapper.hpp"
#include "dataMappers/FiltersDataMapper.hpp"
#include "dataMappers/ImagersDataMapper.hpp"
#include "dataMappers/ObserversDataMapper.hpp"
#include "dataMappers/OcularsDataMapper.hpp"
#include "dataMappers/OpticsDataMapper.hpp"
#include "dataMappers/SitesDataMapper.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelMainGraphicsView.hpp"
#include "StelModuleMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"

#include "ui_LogBookConfigDialog.h"

#include <QDebug>
#include <QDoubleValidator>
#include <QDataWidgetMapper>
#include <QFile>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlRelationalTableModel>
#include <QSqlTableModel>
#include <QStringListModel>
#include <QSqlRelationalDelegate>

LogBookConfigDialog::LogBookConfigDialog(QMap<QString, QSqlTableModel *> theTableModels)
{
	tableModels = theTableModels;
	ui = new Ui_LogBookConfigDialogForm();
	barlowsWidget = new Ui_BarlowsWidget();
	filtersWidget = new Ui_FiltersWidget();
	imagersWidget = new Ui_ImagersWidget();
	observersWidget = new Ui_ObserversWidget();
	ocularsWidget = new Ui_OcularsWidget();
	opticsWidget = new Ui_OpticsWidget();
	sitesWidget = new Ui_SitesWidget();
}

LogBookConfigDialog::~LogBookConfigDialog()
{
	delete ui;
	ui = NULL;
	delete barlowsWidget;
	barlowsWidget = NULL;
	delete filtersWidget;
	filtersWidget = NULL;
	delete imagersWidget;
	imagersWidget = NULL;
	delete observersWidget;
	observersWidget = NULL;
	delete ocularsWidget;
	ocularsWidget = NULL;
	delete opticsWidget;
	opticsWidget = NULL;
	delete sitesWidget;
	sitesWidget = NULL;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
void LogBookConfigDialog::languageChanged()
{
	if (dialog) {
		ui->retranslateUi(dialog);
	}
}

void LogBookConfigDialog::styleChanged()
{
	// Nothing for now
}

void LogBookConfigDialog::updateStyle()
{
	if(dialog) {
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		const StelStyle pluginStyle = GETSTELMODULE(LogBook)->getModuleStyleSheet(gui->getStelStyle());
		dialog->setStyleSheet(pluginStyle.qtStyleSheet);
	}
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Public Slot Methods
#endif
/* ********************************************************************* */
void LogBookConfigDialog::closeWindow()
{
	setVisible(false);
	StelMainGraphicsView::getInstance().scene()->setActiveWindow(0);
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Protected Methods
#endif
/* ********************************************************************* */
void LogBookConfigDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	setupWidgets();
	setupListViews();

	//Initialize the style
	updateStyle();
}

void LogBookConfigDialog::setupListViews()
{
	barlowsWidget->barlowsListView->setModel(tableModels[BARLOWS]);
	barlowsWidget->barlowsListView->setModelColumn(1);
	new BarlowsDataMapper(barlowsWidget, tableModels, this);

	filtersWidget->filtersListView->setModel(tableModels[FILTERS]);
	filtersWidget->filtersListView->setModelColumn(1);
	new FiltersDataMapper(filtersWidget, tableModels, this);

	imagersWidget->imagersListView->setModel(tableModels[IMAGERS]);
	imagersWidget->imagersListView->setModelColumn(1);
	new ImagersDataMapper(imagersWidget, tableModels, this);

	observersWidget->observersListView->setModel(tableModels[OBSERVERS]);
	observersWidget->observersListView->setModelColumn(1);
	new ObserversDataMapper(observersWidget, tableModels, this);

	ocularsWidget->ocularsListView->setModel(tableModels[OCULARS]);
	ocularsWidget->ocularsListView->setModelColumn(1);
	new OcularsDataMapper(ocularsWidget, tableModels, this);

	opticsWidget->opticsListView->setModel(tableModels[OPTICS]);
	opticsWidget->opticsListView->setModelColumn(1);
	new OpticsDataMapper(opticsWidget, tableModels, this);

	sitesWidget->sitesListView->setModel(tableModels[SITES]);
	sitesWidget->sitesListView->setModelColumn(1);
	new SitesDataMapper(sitesWidget, tableModels, this);
}

void LogBookConfigDialog::setupWidgets()
{
	// Layout the stacked widget
	// First, remove the exists default pages
	QWidget *existingPage = ui->stackedWidget->widget(0);
	ui->stackedWidget->removeWidget(existingPage);
	delete existingPage;
	existingPage = ui->stackedWidget->widget(0);
	ui->stackedWidget->removeWidget(existingPage);
	delete existingPage;
	
	// Now add the new pages
	QWidget *barlows = new QWidget(ui->stackedWidget);
	barlowsWidget->setupUi(barlows);
	ui->stackedWidget->addWidget(barlows);
	widgets[BARLOWS] = barlows;
	
	QWidget *filters = new QWidget(ui->stackedWidget);
	filtersWidget->setupUi(filters);
	ui->stackedWidget->addWidget(filters);
	widgets[FILTERS] = filters;
	
	QWidget *imagers = new QWidget(ui->stackedWidget);
	imagersWidget->setupUi(imagers);
	ui->stackedWidget->addWidget(imagers);
	widgets[IMAGERS] = imagers;
	
	QWidget *observers = new QWidget(ui->stackedWidget);
	observersWidget->setupUi(observers);
	ui->stackedWidget->addWidget(observers);
	widgets[OBSERVERS] = observers;
	
	QWidget *oculars = new QWidget(ui->stackedWidget);
	ocularsWidget->setupUi(oculars);
	ui->stackedWidget->addWidget(oculars);
	widgets[OCULARS] = oculars;
	
	QWidget *optics = new QWidget(ui->stackedWidget);
	opticsWidget->setupUi(optics);
	ui->stackedWidget->addWidget(optics);
	widgets[OPTICS] = optics;
	/*
	QStringList items;
    items << tr("Astrograph") << tr("Binoculars") << tr("Cassegrain") << tr("Dobsonian") << tr("Maksutov")
		  << tr("Newton") << tr("Naked Eye") << tr("Refractor") << tr("Ritchey-Chretien") << tr("Schmidt-Cassegrain") 
	   	  << tr("Solar") << tr("Spotting");
	QStringListModel *typeModel = new QStringListModel(items, this);
	opticsWidget->typeComboBox->setModel(typeModel);
	*/
	QWidget *sites = new QWidget(ui->stackedWidget);
	sitesWidget->setupUi(sites);
	ui->stackedWidget->addWidget(sites);
	widgets[SITES] = sites;
	
	// Layout the ListWidget
	ui->pagesListWidget->addItem(N_("Barlows"));
	ui->pagesListWidget->addItem(N_("Filters"));
	ui->pagesListWidget->addItem(N_("Imagers"));
	ui->pagesListWidget->addItem(N_("Observers"));
	ui->pagesListWidget->addItem(N_("Oculars"));
	ui->pagesListWidget->addItem(N_("Optics"));
	ui->pagesListWidget->addItem(N_("Sites"));
	ui->pagesListWidget->setCurrentRow(0);
	
	connect(ui->pagesListWidget, SIGNAL(currentRowChanged(int)), ui->stackedWidget, SLOT(setCurrentIndex(int)));
}

