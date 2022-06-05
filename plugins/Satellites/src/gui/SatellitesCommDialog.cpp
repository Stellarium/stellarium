/*
 * Stellarium Satellites Plug-in: satellites communications editor
 * Copyright (C) 2022 Alexander Wolf
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Satellites.hpp"
#include "SatellitesCommDialog.hpp"
#include "ui_satellitesCommDialog.h"

#include "StelApp.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"

#include <QMessageBox>

SatellitesCommDialog::SatellitesCommDialog()
	: StelDialog("SatellitesComms")
	, satelliteID(QString())
{
	ui = new Ui_satellitesCommDialog;
	SatellitesMgr = GETSTELMODULE(Satellites);
}

SatellitesCommDialog::~SatellitesCommDialog()
{
	delete ui;
}

void SatellitesCommDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setCommunicationsHeaderNames();
		populateTexts();
	}
}

void SatellitesCommDialog::setVisible(bool visible)
{
	StelDialog::setVisible(visible);
	satelliteID = SatellitesMgr->getLastSelectedSatelliteID();
	if (!satelliteID.isEmpty())
		getSatCommData();
}

void SatellitesCommDialog::createDialogContent()
{
	ui->setupUi(dialog);

	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));

	initListCommunications();

	connect(SatellitesMgr, SIGNAL(satSelectionChanged(QString)), this, SLOT(updateSatID(QString)));
	connect(ui->communicationsTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectCurrentCommLink()));
	connect(ui->addCommLinkButton, SIGNAL(clicked()), this, SLOT(addCommData()));
	connect(ui->removeCommLinkButton, SIGNAL(clicked()), this, SLOT(removeCommData()));

	populateTexts();

	// Set size of buttons
	QSize bs = QSize(26, 26);
	ui->addCommLinkButton->setFixedSize(bs);
	ui->removeCommLinkButton->setFixedSize(bs);
}

void SatellitesCommDialog::populateTexts()
{
	ui->descriptionLineEdit->setToolTip(q_("Callsign with channel description"));
	ui->frequencySpinBox->setSuffix(QString(" %1").arg(qc_("MHz", "frequency")));
	ui->frequencySpinBox->setToolTip(q_("Channel frequency"));
	ui->modulationLineEdit->setToolTip(q_("Signal modulation mode"));
	ui->removeCommLinkButton->setToolTip(q_("Remove selected communication data"));
	ui->addCommLinkButton->setToolTip(q_("Add communication data"));
}

void SatellitesCommDialog::updateSatID(QString satID)
{
	satelliteID = satID;
	ui->communicationsTreeWidget->clearSelection();
	ui->communicationsTreeWidget->clearFocus();
	getSatCommData();
}

void SatellitesCommDialog::getSatCommData()
{
	initListCommunications();
	communications.clear();
	if (satelliteID.isEmpty())
		ui->satelliteLabel->setText("");
	else
	{
		ui->satelliteLabel->setText(QString("%1 / NORAD %2").arg(SatellitesMgr->searchByID(satelliteID)->getNameI18n(), satelliteID));
		communications = SatellitesMgr->getCommunicationData(satelliteID);
		if (communications.count()>0)
		{
			for (const auto& comm : qAsConst(communications))
			{
				fillCommunicationsTable(satelliteID, comm.description, comm.frequency, comm.modulation);
			}
			adjustCommunicationsColumns();
		}
	}
	// cleanup input fields
	ui->descriptionLineEdit->setText("");
	ui->modulationLineEdit->setText("");
	ui->frequencySpinBox->setValue(0.0);
}

void SatellitesCommDialog::setCommunicationsHeaderNames()
{
	communicationsHeader.clear();
	communicationsHeader << q_("Description");
	communicationsHeader << q_("Frequency, MHz");
	communicationsHeader << q_("Modulation");

	ui->communicationsTreeWidget->setHeaderLabels(communicationsHeader);
	adjustCommunicationsColumns();
}

void SatellitesCommDialog::adjustCommunicationsColumns()
{
	// adjust the column width
	for (int i = 0; i < CommsCount; ++i)
	{
		ui->communicationsTreeWidget->resizeColumnToContents(i);
	}
}

void SatellitesCommDialog::initListCommunications()
{
	ui->communicationsTreeWidget->clear();
	ui->communicationsTreeWidget->setColumnCount(CommsCount);
	setCommunicationsHeaderNames();
	ui->communicationsTreeWidget->header()->setSectionsMovable(false);
	ui->communicationsTreeWidget->header()->setDefaultAlignment(Qt::AlignCenter);
	//ui->communicationsTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void SatellitesCommDialog::fillCommunicationsTable(QString satID, QString description, double frequency, QString modulation)
{
	CommsTreeWidgetItem* treeItem = new CommsTreeWidgetItem(ui->communicationsTreeWidget);
	treeItem->setText(CommsDescription, description);
	treeItem->setData(CommsDescription, Qt::UserRole, satID);
	treeItem->setText(CommsFrequency, QString::number(frequency, 'f', 3));
	treeItem->setData(CommsFrequency, Qt::UserRole, frequency);
	treeItem->setTextAlignment(CommsFrequency, Qt::AlignRight);
	treeItem->setText(CommsModulation, modulation);
}

void SatellitesCommDialog::selectCurrentCommLink()
{
	QTreeWidgetItem* linkItem = ui->communicationsTreeWidget->currentItem();
	ui->descriptionLineEdit->setText(linkItem->data(CommsDescription, Qt::DisplayRole).toString());
	ui->modulationLineEdit->setText(linkItem->data(CommsModulation, Qt::DisplayRole).toString());
	ui->frequencySpinBox->setValue(linkItem->data(CommsFrequency, Qt::UserRole).toDouble());
}

void SatellitesCommDialog::addCommData()
{
	QString description = ui->descriptionLineEdit->text();
	QString modulation  = ui->modulationLineEdit->text();
	double frequency    = ui->frequencySpinBox->value();
	if (!description.isEmpty() && frequency>0.)
	{
		CommLink c;
		c.description = description;
		c.frequency = frequency;
		if (!modulation.isEmpty())
			c.modulation = modulation;

		communications.append(c);
		SatellitesMgr->getById(satelliteID)->setCommData(communications);
		SatellitesMgr->saveCatalog();

		getSatCommData();
		// A "hackish" updating list of satellites
		emit SatellitesMgr->customFilterChanged();
	}
	else
		QMessageBox::warning(&StelMainView::getInstance(), q_("Nothing added"), q_("Please fill in description and add frequency before adding!"), QMessageBox::Ok);
}

void SatellitesCommDialog::removeCommData()
{
	QTreeWidgetItem* linkItem = ui->communicationsTreeWidget->currentItem();
	if (linkItem != Q_NULLPTR)
	{
		CommLink c;
		QString modulation = linkItem->data(CommsModulation, Qt::DisplayRole).toString();
		c.description = linkItem->data(CommsDescription, Qt::DisplayRole).toString();
		c.frequency = linkItem->data(CommsFrequency, Qt::UserRole).toDouble();
		if (!modulation.isEmpty())
			c.modulation = modulation;

		QList<CommLink> newComms;
		for (auto& comm : communications)
		{
			if (c.description==comm.description && c.frequency==comm.frequency)
				continue;
			newComms.append(comm);
		}
		communications = newComms;
		SatellitesMgr->getById(satelliteID)->setCommData(communications);
		SatellitesMgr->saveCatalog();

		getSatCommData();
		// A "hackish" updating list of satellites
		emit SatellitesMgr->customFilterChanged();
	}
	else
		QMessageBox::warning(&StelMainView::getInstance(), q_("Nothing deleted"), q_("Please select a line first!"), QMessageBox::Ok);
}
