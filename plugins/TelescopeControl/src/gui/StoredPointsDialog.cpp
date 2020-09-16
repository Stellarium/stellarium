/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2015 Pavel Klimenko aka rich <dzy4@mail.ru> (this file)
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

#include "StoredPointsDialog.hpp"
#include "ui_storedPointsDialog.h"

StoredPointsDialog::StoredPointsDialog(): StelDialog("TelescopeControlStoredPoints")
{
	ui = new Ui_StoredPoints;
	storedPointsListModel = new QStandardItemModel(0, ColumnCount);
}

StoredPointsDialog::~StoredPointsDialog()
{
	delete ui;
	delete storedPointsListModel;
}

void StoredPointsDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void StoredPointsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->pushButtonAddPoint,   SIGNAL(clicked()), this, SLOT(buttonAddPressed()));
	connect(ui->pushButtonRemovePoint,SIGNAL(clicked()), this, SLOT(buttonRemovePressed()));
	connect(ui->pushButtonClearList,  SIGNAL(clicked()), this, SLOT(buttonClearPressed()));

	connect(ui->pushButtonCurrent, SIGNAL(clicked()), this, SLOT(getCurrentObjectInfo()));
	connect(ui->pushButtonCenter, SIGNAL(clicked()), this, SLOT(getCenterInfo()));

	//Initializing the list of telescopes
	storedPointsListModel->setColumnCount(ColumnCount);
	setHeaderNames();

	ui->treeViewStoredPoints->setModel(storedPointsListModel);
	ui->treeViewStoredPoints->header()->setSectionsMovable(false);
	ui->treeViewStoredPoints->header()->setSectionResizeMode(ColumnSlot, QHeaderView::ResizeToContents);
	ui->treeViewStoredPoints->header()->setStretchLastSection(true);

	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSSymbols);
}

void StoredPointsDialog::populatePointsList(QVariantList list)
{
	for (int i = 0; i < list.count(); i++ )
	{
		QVariant var = list.at(i);
		storedPoint sp = var.value<storedPoint>();
		// convert double to DisplayFormat
		AngleSpinBox tmpRA;
		AngleSpinBox tmpDec;

		tmpRA.setDisplayFormat(AngleSpinBox::HMSLetters);
		tmpDec.setDisplayFormat(AngleSpinBox::DMSSymbols);

		tmpRA.setRadians(sp.radiansRA);
		tmpDec.setRadians(sp.radiansDec);

		addModelRow(sp.number, sp.name, tmpRA.text(), tmpDec.text());
	}
}

void StoredPointsDialog::setHeaderNames()
{
	QStringList headerStrings;
	// TRANSLATORS: Symbol for "number"
	headerStrings << q_("#");
	headerStrings << q_("Name");
	headerStrings << q_("Right Ascension (J2000)");
	headerStrings << q_("Declination (J2000)");
	storedPointsListModel->setHorizontalHeaderLabels(headerStrings);
}

void StoredPointsDialog::buttonAddPressed()
{
	double radiansRA  = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();
	QString name      = ui->lineEditStoredPointName->text().trimmed();

	if (name.isEmpty())
		return;

	int lastRow = storedPointsListModel->rowCount();

	addModelRow(lastRow, name, ui->spinBoxRA->text(), ui->spinBoxDec->text());

	ui->lineEditStoredPointName->setText("");
	ui->lineEditStoredPointName->setFocus();

	emit addStoredPoint(lastRow, name, radiansRA, radiansDec);
}

void StoredPointsDialog::buttonRemovePressed()
{
	int number = ui->treeViewStoredPoints->currentIndex().row();
	storedPointsListModel->removeRow(number);

	emit removeStoredPoint(number);
}

void StoredPointsDialog::buttonClearPressed()
{
	storedPointsListModel->clear();

	emit clearStoredPoints();
}

void StoredPointsDialog::addModelRow(int number, QString name, QString RA, QString Dec)
{
	QStandardItem* tempItem = Q_NULLPTR;

	tempItem = new QStandardItem(QString::number(number+1));
	tempItem->setEditable(false);
	storedPointsListModel->setItem(number, ColumnSlot, tempItem);

	tempItem = new QStandardItem(name);
	tempItem->setEditable(false);
	storedPointsListModel->setItem(number, ColumnName, tempItem);

	tempItem = new QStandardItem(RA);
	tempItem->setEditable(false);
	storedPointsListModel->setItem(number, ColumnRA, tempItem);

	tempItem = new QStandardItem(Dec);
	tempItem->setEditable(false);
	storedPointsListModel->setItem(number, ColumnDec, tempItem);
}

void StoredPointsDialog::getCurrentObjectInfo()
{
	const QList<StelObjectP>& selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (!selected.isEmpty())
	{
		double dec_j2000 = 0;
		double ra_j2000 = 0;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000, selected[0]->getJ2000EquatorialPos(StelApp::getInstance().getCore()));
		ui->spinBoxRA->setRadians(ra_j2000);
		ui->spinBoxDec->setRadians(dec_j2000);
		ui->lineEditStoredPointName->setText(selected[0]->getNameI18n());
	}
}

void StoredPointsDialog::getCenterInfo()
{
	StelCore *core = StelApp::getInstance().getCore();
	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	Vec3d centerPosition;
	Vector2<qreal> center = projector->getViewportCenter();
	projector->unProject(center[0], center[1], centerPosition);
	double dec_j2000 = 0;
	double ra_j2000 = 0;
	StelUtils::rectToSphe(&ra_j2000,&dec_j2000,core->equinoxEquToJ2000(centerPosition, StelCore::RefractionOff)); // GZ for 0.15.2: Not sure about RefractionOff. This just keeps old behaviour.
	ui->spinBoxRA->setRadians(ra_j2000);
	ui->spinBoxDec->setRadians(dec_j2000);

	ui->lineEditStoredPointName->clear(); // if previous saved
	ui->lineEditStoredPointName->setFocus();
}
