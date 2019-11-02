/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2010 Bogdan Marinov (this file)
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

#include <QDebug>

#include "Dialog.hpp"
#include "AngleSpinBox.hpp"
#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "VecMath.hpp"
#include "TelescopeControl.hpp"
#include "SlewDialog.hpp"
#include "ui_slewDialog.h"
#include "TelescopeClient.hpp"
#include "INDI/TelescopeClientINDI.hpp"

using namespace TelescopeControlGlobals;


SlewDialog::SlewDialog()
	: StelDialog("TelescopeControlSlew")
	, storedPointsDialog(Q_NULLPTR)
{
	ui = new Ui_slewDialog();
	
    //TODO: Fix this - it's in the same plugin
    telescopeManager = GETSTELMODULE(TelescopeControl);
}

SlewDialog::~SlewDialog()
{	
	delete ui;
	storedPointsDialog = Q_NULLPTR;
}

void SlewDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void SlewDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->radioButtonHMS, SIGNAL(toggled(bool)), this, SLOT(setFormatHMS(bool)));
	connect(ui->radioButtonDMS, SIGNAL(toggled(bool)), this, SLOT(setFormatDMS(bool)));
	connect(ui->radioButtonDecimal, SIGNAL(toggled(bool)), this, SLOT(setFormatDecimal(bool)));

	connect(ui->pushButtonSlew, SIGNAL(clicked()), this, SLOT(slew()));
	connect(ui->pushButtonSync, SIGNAL(clicked()), this, SLOT(sync()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()), this, SLOT(showConfiguration()));

	connect(telescopeManager, SIGNAL(clientConnected(int, QString)), this, SLOT(addTelescope(int, QString)));
	connect(telescopeManager, SIGNAL(clientDisconnected(int)), this, SLOT(removeTelescope(int)));

	connect(ui->comboBoxStoredPoints, SIGNAL(currentIndexChanged(int)), this, SLOT(getStoredPointInfo()));
	connect(ui->toolButtonStoredPoints, SIGNAL(clicked()), this, SLOT(editStoredPoints()));

	connect(ui->pushButtonCurrent, SIGNAL(clicked()), this, SLOT(getCurrentObjectInfo()));
	connect(ui->pushButtonCenter, SIGNAL(clicked()), this, SLOT(getCenterInfo()));

	QObject::connect(ui->comboBoxTelescope, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SlewDialog::onCurrentTelescopeChanged);

	//Coordinates are in HMS by default:
	ui->radioButtonHMS->setChecked(true);

	storedPointsDialog = new StoredPointsDialog;
	// add point and remove
	connect(storedPointsDialog, SIGNAL(addStoredPoint(int, QString, double, double)), this, SLOT(addStoredPointToComboBox(int, QString, double, double)));
	// remove point
	connect(storedPointsDialog, SIGNAL(removeStoredPoint(int)), this, SLOT(removeStoredPointFromComboBox(int)));
	// clean points
	connect(storedPointsDialog, SIGNAL(clearStoredPoints()), this, SLOT(clearStoredPointsFromComboBox()));


	updateTelescopeList();
	updateStoredPointsList();
}

void SlewDialog::showConfiguration()
{
	//Hack to work around having no direct way to do display the window
	telescopeManager->configureGui(true);
}

void SlewDialog::setFormatHMS(bool set)
{
	if (!set)
		return;

	ui->spinBoxRA->setDecimals(2);
	ui->spinBoxDec->setDecimals(2);
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSSymbols);
}

void SlewDialog::setFormatDMS(bool set)
{
	if (!set)
		return;

	ui->spinBoxRA->setDecimals(2);
	ui->spinBoxDec->setDecimals(2);
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DMSSymbols);
}

void SlewDialog::setFormatDecimal(bool set)
{
	if (!set)
		return;

	ui->spinBoxRA->setDecimals(6);
	ui->spinBoxDec->setDecimals(6);
	ui->spinBoxRA->setDisplayFormat(AngleSpinBox::DecimalDeg);
	ui->spinBoxDec->setDisplayFormat(AngleSpinBox::DecimalDeg);
}

void SlewDialog::updateTelescopeList()
{
	connectedSlotsByName.clear();
	ui->comboBoxTelescope->clear();

	QHash<int, QString> connectedSlotsByNumber = telescopeManager->getConnectedClientsNames();
	for (const auto slot : connectedSlotsByNumber.keys())
	{
		QString telescopeName = connectedSlotsByNumber.value(slot);
		connectedSlotsByName.insert(telescopeName, slot);
		ui->comboBoxTelescope->addItem(telescopeName);
	}
	
	updateTelescopeControls();
}

void SlewDialog::updateTelescopeControls()
{
	bool connectedTelescopeAvailable = !connectedSlotsByName.isEmpty();
	ui->groupBoxSlew->setVisible(connectedTelescopeAvailable);
	ui->labelNoTelescopes->setVisible(!connectedTelescopeAvailable);
	if (connectedTelescopeAvailable)
		ui->comboBoxTelescope->setCurrentIndex(0);
}

void SlewDialog::addTelescope(int slot, QString name)
{
	if (slot <=0 || name.isEmpty())
		return;

	connectedSlotsByName.insert(name, slot);
	ui->comboBoxTelescope->addItem(name);

	updateTelescopeControls();
}

void SlewDialog::removeTelescope(int slot)
{
	if (slot <= 0)
		return;

	QString name = connectedSlotsByName.key(slot, QString());
	if (name.isEmpty())
		return;

	connectedSlotsByName.remove(name);

	int index = ui->comboBoxTelescope->findText(name);
	if (index != -1)
	{
		ui->comboBoxTelescope->removeItem(index);
	}
	else
	{
		//Something very wrong just happened, so:
		updateTelescopeList();
		return;
	}

	updateTelescopeControls();
}

QSharedPointer<TelescopeClient> SlewDialog::currentTelescope() const
{
	int slot = connectedSlotsByName.value(ui->comboBoxTelescope->currentText());
	return telescopeManager->telescopeClient(slot);
}

void SlewDialog::slew()
{
	double radiansRA  = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	auto telescope = currentTelescope();
	if (!telescope)
		return;

	StelObjectP selectObject = Q_NULLPTR;
	telescope->telescopeGoto(targetPosition, selectObject);
}

void SlewDialog::sync()
{
	double radiansRA  = ui->spinBoxRA->valueRadians();
	double radiansDec = ui->spinBoxDec->valueRadians();

	Vec3d targetPosition;
	StelUtils::spheToRect(radiansRA, radiansDec, targetPosition);

	auto telescope = currentTelescope();
	if (!telescope)
		return;

	StelObjectP selectObject = Q_NULLPTR;
	telescope->telescopeSync(targetPosition, selectObject);
}

void SlewDialog::getCurrentObjectInfo()
{
	const QList<StelObjectP>& selected = GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (!selected.isEmpty()) {
		double dec_j2000 = 0;
		double ra_j2000 = 0;
		StelUtils::rectToSphe(&ra_j2000,&dec_j2000,selected[0]->getJ2000EquatorialPos(StelApp::getInstance().getCore()));
		ui->spinBoxRA->setRadians(ra_j2000);
		ui->spinBoxDec->setRadians(dec_j2000);
	}
}

void SlewDialog::getCenterInfo()
{
	StelCore *core = StelApp::getInstance().getCore();
	const StelProjectorP projector = core->getProjection(StelCore::FrameEquinoxEqu);
	Vec3d centerPosition;
	Vector2<qreal> center = projector->getViewportCenter();
	projector->unProject(center[0], center[1], centerPosition);
	double dec_j2000 = 0;
	double ra_j2000 = 0;
	StelUtils::rectToSphe(&ra_j2000,&dec_j2000,core->equinoxEquToJ2000(centerPosition, StelCore::RefractionOff));
	ui->spinBoxRA->setRadians(ra_j2000);
	ui->spinBoxDec->setRadians(dec_j2000);
}

void SlewDialog::editStoredPoints()
{
	storedPointsDialog->setVisible(true);
	QVariantList qvl;
	for (int i = 1;i< ui->comboBoxStoredPoints->count(); i++)
	{
		QVariant var = ui->comboBoxStoredPoints->itemData(i);
		qvl.append(var);
	}
	storedPointsDialog->populatePointsList(qvl);
}

void SlewDialog::updateStoredPointsList()
{
	//user point from file
	loadPointsFromFile();
}

void SlewDialog::addStoredPointToComboBox(int number, QString name, double radiansRA, double radiansDec)
{
	if (!ui->comboBoxStoredPoints->count())
	{
		ui->comboBoxStoredPoints->addItem(q_("Select one"));
	}
	storedPoint sp;
	sp.number = number;
	sp.name = name;
	sp.radiansRA = radiansRA;
	sp.radiansDec= radiansDec;

	QVariant var;
	var.setValue(sp);
	ui->comboBoxStoredPoints->addItem(name,var);

	savePointsToFile();
}

void SlewDialog::removeStoredPointFromComboBox(int number)
{
	ui->comboBoxStoredPoints->removeItem(number+1);
	if (1 == ui->comboBoxStoredPoints->count())
	{
		ui->comboBoxStoredPoints->removeItem(0);
	}
	savePointsToFile();
}

void SlewDialog::clearStoredPointsFromComboBox()
{
	ui->comboBoxStoredPoints->blockSignals(true);
	ui->comboBoxStoredPoints->clear();
	ui->comboBoxStoredPoints->addItem(q_("Select one"));
	ui->comboBoxStoredPoints->blockSignals(false);
	savePointsToFile();
}

void SlewDialog::getStoredPointInfo()
{
	QVariant var = ui->comboBoxStoredPoints->currentData();
	storedPoint sp = var.value<storedPoint>();

	ui->spinBoxRA->setRadians(sp.radiansRA);
	ui->spinBoxDec->setRadians(sp.radiansDec);
}

void SlewDialog::onMove(double angle, double speed)
{
	auto telescope = currentTelescope();
	if (telescope)
		telescope->move(angle, speed);
}

void SlewDialog::onCurrentTelescopeChanged()
{
	// remove previous controlWidget
	QLayoutItem* child;
	while ((child = ui->controlWidgetLayout->takeAt(0)) != Q_NULLPTR)
	{
		delete child->widget();
		delete child;
	}

    auto telescope = currentTelescope();
    if (!telescope)
        return;

    auto controlWidget = telescope->createControlWidget(telescope);
    if (!controlWidget)
        return;

    ui->controlWidgetLayout->addWidget(controlWidget);
}

void SlewDialog::savePointsToFile()
{
	//Open/create the JSON file
	QString pointsJsonPath = StelFileMgr::findFile("modules/TelescopeControl", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/points.json";
	if (pointsJsonPath.isEmpty())
	{
		qWarning() << "SlewDialog: Error saving points";
		return;
	}
	QFile pointsJsonFile(pointsJsonPath);
	if(!pointsJsonFile.open(QFile::WriteOnly|QFile::Text))
	{
		qWarning() << "SlewDialog: Points can not be saved. A file can not be open for writing:"
				   << QDir::toNativeSeparators(pointsJsonPath);
		return;
	}

	storedPointsDescriptions.clear();
	// All user stored points from comboBox
	for (int i = 1;i< ui->comboBoxStoredPoints->count(); i++)
	{
		QVariant var = ui->comboBoxStoredPoints->itemData(i);
		storedPoint sp = var.value<storedPoint>();
		QVariantMap point;
		point.insert("number",        sp.number);
		point.insert("name",          sp.name);
		point.insert("radiansRA",     sp.radiansRA);
		point.insert("radiansDec",    sp.radiansDec);

		storedPointsDescriptions.insert(QString::number(sp.number),point);
	}
	//Add the version:
	storedPointsDescriptions.insert("version", QString(TELESCOPE_CONTROL_PLUGIN_VERSION));
	//Convert the tree to JSON
	StelJsonParser::write(storedPointsDescriptions, &pointsJsonFile);
	pointsJsonFile.flush();//Is this necessary?
	pointsJsonFile.close();
}

void SlewDialog::loadPointsFromFile()
{
	QVariantMap result;
	QString pointsJsonPath = StelFileMgr::findFile("modules/TelescopeControl", static_cast<StelFileMgr::Flags>(StelFileMgr::Directory|StelFileMgr::Writable)) + "/points.json";

	if (pointsJsonPath.isEmpty())
	{
		qWarning() << "SlewDialog: Error loading points";
		return;
	}
	if(!QFileInfo(pointsJsonPath).exists())
	{
		qWarning() << "SlewDialog::loadPointsFromFile(): No pointss loaded. File is missing:"
				   << QDir::toNativeSeparators(pointsJsonPath);
		storedPointsDescriptions = result;
		return;
	}

	QFile pointsJsonFile(pointsJsonPath);

	QVariantMap map;

	if(!pointsJsonFile.open(QFile::ReadOnly))
	{
		qWarning() << "SlewDialog: No points loaded. Can't open for reading"
				   << QDir::toNativeSeparators(pointsJsonPath);
		storedPointsDescriptions = result;
		return;
	}
	else
	{
		map = StelJsonParser::parse(&pointsJsonFile).toMap();
		pointsJsonFile.close();
	}

	//File contains any points?
	if(map.isEmpty())
	{
		storedPointsDescriptions = result;
		return;
	}

	QString version = map.value("version", "0.0.0").toString();
	if(version < QString(TELESCOPE_CONTROL_PLUGIN_VERSION))
	{
		QString newName = pointsJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
		if(pointsJsonFile.rename(newName))
		{
			qWarning() << "SlewDialog: The existing version of points.json is obsolete. Backing it up as "
					   << QDir::toNativeSeparators(newName);
			qWarning() << "SlewDialog: A blank points.json file will have to be created.";
			storedPointsDescriptions = result;
			return;
		}
		else
		{
			qWarning() << "SlewDialog: The existing version of points.json is obsolete. Unable to rename.";
			storedPointsDescriptions = result;
			return;
		}
	}
	map.remove("version");//Otherwise it will try to read it as a point

	//Read pointss, if any
	QMapIterator<QString, QVariant> node(map);

	if(node.hasNext())
	{
		ui->comboBoxStoredPoints->addItem(q_("Select one"));
		do
		{
			node.next();

			QVariantMap point = node.value().toMap();
			storedPoint sp;
			sp.name       = point.value("name").toString();
			sp.number     = point.value("number").toInt();
			sp.radiansRA  = point.value("radiansRA").toDouble();
			sp.radiansDec = point.value("radiansDec").toDouble();

			QVariant var;
			var.setValue(sp);
			ui->comboBoxStoredPoints->addItem(sp.name,var);
		} while (node.hasNext());
	}
}
