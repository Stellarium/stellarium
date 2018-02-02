/*
 * Stellarium
 * Copyright (C) 2008 Guillaume Chereau
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

#include "Dialog.hpp"
#include "SearchDialog.hpp"
#include "ui_searchDialogGui.h"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelTranslator.hpp"
#include "Planet.hpp"
#include "CustomObjectMgr.hpp"

#include "StelObjectMgr.hpp"
#include "StelGui.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtAlgorithms>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QMenu>
#include <QMetaEnum>
#include <QClipboard>

#include "SimbadSearcher.hpp"

// Start of members for class CompletionLabel
CompletionLabel::CompletionLabel(QWidget* parent) : QLabel(parent), selectedIdx(0)
{
}

CompletionLabel::~CompletionLabel()
{
}

void CompletionLabel::setValues(const QStringList& v)
{
	values=v;
	updateText();
}

void CompletionLabel::appendValues(const QStringList& v)
{
	values+=v;
	updateText();
}

void CompletionLabel::clearValues()
{
	values.clear();
	selectedIdx=0;
	updateText();
}

QString CompletionLabel::getSelected()
{
	if (values.isEmpty())
		return QString();
	return values.at(selectedIdx);
}

void CompletionLabel::selectNext()
{
	++selectedIdx;
	if (selectedIdx>=values.size())
		selectedIdx=0;
	updateText();
}

void CompletionLabel::selectPrevious()
{
	--selectedIdx;
	if (selectedIdx<0)
		selectedIdx = values.size()-1;
	updateText();
}

void CompletionLabel::selectFirst()
{
	selectedIdx=0;
	updateText();
}

void CompletionLabel::updateText()
{
	QString newText;

	// Regenerate the list with the selected item in bold
	for (int i=0;i<values.size();++i)
	{
		if (i==selectedIdx)
			newText+="<b>"+values[i]+"</b>";
		else
			newText+=values[i];
		if (i!=values.size()-1)
			newText += ", ";
	}
	setText(newText);
}

// Start of members for class SearchDialog

const char* SearchDialog::DEF_SIMBAD_URL = "http://simbad.u-strasbg.fr/";
SearchDialog::SearchDialogStaticData SearchDialog::staticData;
QString SearchDialog::extSearchText = "";

SearchDialog::SearchDialog(QObject* parent)
	: StelDialog("Search", parent)
	, simbadReply(Q_NULLPTR)
	, flagHasSelectedText(false)
{
	ui = new Ui_searchDialogForm;
	simbadSearcher = new SimbadSearcher(this);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objectMgr);

	conf = StelApp::getInstance().getSettings();
	useSimbad = conf->value("search/flag_search_online", true).toBool();	
	useStartOfWords = conf->value("search/flag_start_words", false).toBool();
	useLockPosition = conf->value("search/flag_lock_position", true).toBool();
	simbadServerUrl = conf->value("search/simbad_server_url", DEF_SIMBAD_URL).toString();
	setCurrentCoordinateSystemKey(conf->value("search/coordinate_system", "equatorialJ2000").toString());	
}

SearchDialog::~SearchDialog()
{
	delete ui;
	if (simbadReply)
	{
		simbadReply->deleteLater();
		simbadReply = Q_NULLPTR;
	}
}

void SearchDialog::retranslate()
{
	if (dialog)
	{
		QString text(ui->lineEditSearchSkyObject->text());
		ui->retranslateUi(dialog);
		ui->lineEditSearchSkyObject->setText(text);
		populateSimbadServerList();
		populateCoordinateSystemsList();
		populateCoordinateAxis();
		updateListTab();
	}
}

void SearchDialog::styleChanged()
{
	// Nothing for now
}

void SearchDialog::setCurrentCoordinateSystemKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("CoordinateSystem"));
	CoordinateSystem coordSystem = (CoordinateSystem)en.keyToValue(key.toLatin1().data());
	if (coordSystem<0)
	{
		qWarning() << "[Search Tool] Unknown coordinate system: " << key << "setting \"equatorialJ2000\" instead";
		coordSystem = equatorialJ2000;
	}
	setCurrentCoordinateSystem(coordSystem);
}

QString SearchDialog::getCurrentCoordinateSystemKey() const
{
	return metaObject()->enumerator(metaObject()->indexOfEnumerator("CoordinateSystem")).key(currentCoordinateSystem);
}

void SearchDialog::populateCoordinateSystemsList()
{
	Q_ASSERT(ui->coordinateSystemComboBox);

	QComboBox* csys = ui->coordinateSystemComboBox;

	//Save the current selection to be restored later
	csys->blockSignals(true);
	int index = csys->currentIndex();
	QVariant selectedSystemId = csys->itemData(index);
	csys->clear();
	//For each coordinate system, display the localized name and store the key as user
	//data. Unfortunately, there's no other way to do this than with a cycle.
	csys->addItem(qc_("Equatorial (J2000.0)", "coordinate system"), "equatorialJ2000");
	csys->addItem(qc_("Equatorial", "coordinate system"), "equatorial");
	csys->addItem(qc_("Horizontal", "coordinate system"), "horizontal");
	csys->addItem(qc_("Galactic", "coordinate system"), "galactic");
	csys->addItem(qc_("Supergalactic", "coordinate system"), "supergalactic");
	csys->addItem(qc_("Ecliptic", "coordinate system"), "ecliptic");
	csys->addItem(qc_("Ecliptic (J2000.0)", "coordinate system"), "eclipticJ2000");

	//Restore the selection
	index = csys->findData(selectedSystemId, Qt::UserRole, Qt::MatchCaseSensitive);
	csys->setCurrentIndex(index);
	csys->blockSignals(false);
}

void SearchDialog::populateCoordinateAxis()
{
	bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	bool xnormal = true;

	ui->AxisXSpinBox->setDecimals(2);
	ui->AxisYSpinBox->setDecimals(2);

	// Allow rotating through longitudinal coordinates, but stop at poles.
	ui->AxisXSpinBox->setMinimum(  0., true);
	ui->AxisXSpinBox->setMaximum(360., true);
	ui->AxisXSpinBox->setWrapping(true);
	ui->AxisYSpinBox->setMinimum(-90., true);
	ui->AxisYSpinBox->setMaximum( 90., true);
	ui->AxisYSpinBox->setWrapping(false);

	switch (getCurrentCoordinateSystem()) {		
		case equatorialJ2000:
		case equatorial:
		{
			ui->AxisXLabel->setText(q_("Right ascension"));
			ui->AxisXSpinBox->setDisplayFormat(AngleSpinBox::HMSLetters);
			ui->AxisXSpinBox->setPrefixType(AngleSpinBox::Normal);
			ui->AxisYLabel->setText(q_("Declination"));
			ui->AxisYSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
			ui->AxisYSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
			xnormal = true;
			break;
		}
		case horizontal:
		{
			ui->AxisXLabel->setText(q_("Azimuth"));
			ui->AxisXSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbolsUnsigned);
			ui->AxisXSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
			ui->AxisYLabel->setText(q_("Altitude"));
			ui->AxisYSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
			ui->AxisYSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
			xnormal = false;
			break;
		}
		case ecliptic:
		case eclipticJ2000:
		case galactic:
		case supergalactic:
		{
			ui->AxisXLabel->setText(q_("Longitude"));
			ui->AxisXSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbolsUnsigned);
			ui->AxisXSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
			ui->AxisYLabel->setText(q_("Latitude"));
			ui->AxisYSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
			ui->AxisYSpinBox->setPrefixType(AngleSpinBox::NormalPlus);
			xnormal = false;
			break;
		}
	}

	if (withDecimalDegree)
	{
		ui->AxisXSpinBox->setDecimals(5);
		ui->AxisYSpinBox->setDecimals(5);
		ui->AxisXSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
		ui->AxisYSpinBox->setDisplayFormat(AngleSpinBox::DecimalDeg);
		ui->AxisXSpinBox->setPrefixType(AngleSpinBox::NormalPlus);

	}
	else
	{
		if (xnormal)
			ui->AxisXSpinBox->setPrefixType(AngleSpinBox::Normal);
	}
}

void SearchDialog::setCoordinateSystem(int csID)
{
	QString currentCoordinateSystemID = ui->coordinateSystemComboBox->itemData(csID).toString();
	setCurrentCoordinateSystemKey(currentCoordinateSystemID);
	populateCoordinateAxis();
	ui->AxisXSpinBox->setRadians(0.);
	ui->AxisYSpinBox->setRadians(0.);
	conf->setValue("search/coordinate_system", currentCoordinateSystemID);
}

// Initialize the dialog widgets and connect the signals/slots
void SearchDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance(), SIGNAL(flagShowDecimalDegreesChanged(bool)), this, SLOT(populateCoordinateAxis()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)),
		     this, SLOT(onSearchTextChanged(const QString&)));
	connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
	connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(selectionChanged()), this, SLOT(setHasSelectedFlag()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	ui->lineEditSearchSkyObject->installEventFilter(this);	

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->objectsListWidget;
	installKineticScrolling(addscroll);
#endif

	populateCoordinateSystemsList();
	populateCoordinateAxis();
	int idx = ui->coordinateSystemComboBox->findData(getCurrentCoordinateSystemKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use equatorialJ2000 as default
		idx = ui->coordinateSystemComboBox->findData(QVariant("equatorialJ2000"), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->coordinateSystemComboBox->setCurrentIndex(idx);
	connect(ui->coordinateSystemComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCoordinateSystem(int)));
	connect(ui->AxisXSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
	connect(ui->AxisYSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
    
	connect(ui->alphaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->betaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->gammaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->deltaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->epsilonPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->zetaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->etaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->thetaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->iotaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->kappaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->lambdaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->muPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->nuPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->xiPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->omicronPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->piPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->rhoPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->sigmaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->tauPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->upsilonPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->phiPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->chiPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->psiPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));
	connect(ui->omegaPushButton, SIGNAL(clicked(bool)), this, SLOT(greekLetterClicked()));

	connect(ui->simbadGroupBox, SIGNAL(clicked(bool)), this, SLOT(enableSimbadSearch(bool)));
	ui->simbadGroupBox->setChecked(useSimbad);

	populateSimbadServerList();
	idx = ui->serverListComboBox->findData(simbadServerUrl, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use University of Strasbourg as default
		idx = ui->serverListComboBox->findData(QVariant(DEF_SIMBAD_URL), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->serverListComboBox->setCurrentIndex(idx);
	connect(ui->serverListComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSimbadServer(int)));

	connect(ui->checkBoxUseStartOfWords, SIGNAL(clicked(bool)), this, SLOT(enableStartOfWordsAutofill(bool)));
	ui->checkBoxUseStartOfWords->setChecked(useStartOfWords);

	connect(ui->checkBoxLockPosition, SIGNAL(clicked(bool)), this, SLOT(enableLockPosition(bool)));
	ui->checkBoxLockPosition->setChecked(useLockPosition);

	// list views initialization
	connect(ui->objectTypeComboBox, SIGNAL(activated(int)), this, SLOT(updateListWidget(int)));
	connect(ui->searchInListLineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchListChanged(QString)));
	connect(ui->searchInEnglishCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateListTab()));	
	updateListTab();

	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));
	// Set the focus directly on the line edit
	if (ui->tabWidget->currentIndex()==0)
		ui->lineEditSearchSkyObject->setFocus();
}

void SearchDialog::changeTab(int index)
{
	if (index==0) // First tab: Search
		ui->lineEditSearchSkyObject->setFocus();

	if (index==2) // Third tab: Lists
	{
		updateListTab();
		ui->searchInListLineEdit->setFocus();
	}
}

void SearchDialog::setHasSelectedFlag()
{
	flagHasSelectedText = true;
}

void SearchDialog::enableSimbadSearch(bool enable)
{
	useSimbad = enable;	
	conf->setValue("search/flag_search_online", useSimbad);
	ui->simbadStatusLabel->clear();
}

void SearchDialog::enableStartOfWordsAutofill(bool enable)
{
	useStartOfWords = enable;
	conf->setValue("search/flag_start_words", useStartOfWords);
}

void SearchDialog::enableLockPosition(bool enable)
{
	useLockPosition = enable;
	conf->setValue("search/flag_lock_position", useLockPosition);
}

void SearchDialog::setSimpleStyle()
{
	ui->AxisXSpinBox->setVisible(false);
	ui->AxisXSpinBox->setVisible(false);
	ui->simbadStatusLabel->setVisible(false);
	ui->AxisXLabel->setVisible(false);
	ui->AxisYLabel->setVisible(false);
	ui->coordinateSystemLabel->setVisible(false);
	ui->coordinateSystemComboBox->setVisible(false);
}


void SearchDialog::manualPositionChanged()
{
	ui->completionLabel->clearValues();
	StelCore* core = StelApp::getInstance().getCore();
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);	
	Vec3d pos;
	Vec3d aimUp;
	double spinLong=ui->AxisXSpinBox->valueRadians();
	double spinLat=ui->AxisYSpinBox->valueRadians();

	// Since 0.15: proper handling of aimUp vector. This does not depend on the searched coordinate system, but on the MovementManager's C.S.
	// However, if those are identical, we have a problem when we want to look right into the pole. (e.g. zenith), which requires a special up vector.
	// aimUp depends on MovementMgr::MountMode mvmgr->mountMode!
	mvmgr->setViewUpVector(Vec3d(0., 0., 1.));
	aimUp=mvmgr->getViewUpVectorJ2000();
	StelMovementMgr::MountMode mountMode=mvmgr->getMountMode();

	switch (getCurrentCoordinateSystem()) {
		case equatorialJ2000:
		{
			StelUtils::spheToRect(spinLong, spinLat, pos);
			if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat)> (0.9*M_PI/2.0)) )
			{
				// make up vector more stable.
				// Strictly mount should be in a new J2000 mode, but this here also stabilizes searching J2000 coordinates.
				mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat>0. ? 1. : -1. ));
				aimUp=mvmgr->getViewUpVectorJ2000();
			}
			break;
		}
		case equatorial:
		{
			StelUtils::spheToRect(spinLong, spinLat, pos);
			pos = core->equinoxEquToJ2000(pos, StelCore::RefractionOff);

			if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat)> (0.9*M_PI/2.0)) )
			{
				// make up vector more stable.
				mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat>0. ? 1. : -1. ));
				aimUp=mvmgr->getViewUpVectorJ2000();
			}
			break;
		}
		case horizontal:
		{
			double cx;
			cx = 3.*M_PI - spinLong; // N is zero, E is 90 degrees
			if (cx > 2.*M_PI)
				cx -= 2.*M_PI;
			StelUtils::spheToRect(cx, spinLat, pos);
			pos = core->altAzToJ2000(pos);

			if ( (mountMode==StelMovementMgr::MountAltAzimuthal) && (fabs(spinLat)> (0.9*M_PI/2.0)) )
			{
				// make up vector more stable.
				mvmgr->setViewUpVector(Vec3d(-cos(cx), -sin(cx), 0.) * (spinLat>0. ? 1. : -1. ));
				aimUp=mvmgr->getViewUpVectorJ2000();
			}
			break;
		}
		case galactic:
		{
			StelUtils::spheToRect(spinLong, spinLat, pos);
			pos = core->galacticToJ2000(pos);
			if ( (mountMode==StelMovementMgr::MountGalactic) && (fabs(spinLat)> (0.9*M_PI/2.0)) )
			{
				// make up vector more stable.
				mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat>0. ? 1. : -1. ));
				aimUp=mvmgr->getViewUpVectorJ2000();
			}
			break;
		}
		case supergalactic:
		{
			StelUtils::spheToRect(spinLong, spinLat, pos);
			pos = core->supergalacticToJ2000(pos);
			if ( (mountMode==StelMovementMgr::MountSupergalactic) && (fabs(spinLat)> (0.9*M_PI/2.0)) )
			{
				// make up vector more stable.
				mvmgr->setViewUpVector(Vec3d(-cos(spinLong), -sin(spinLong), 0.) * (spinLat>0. ? 1. : -1. ));
				aimUp=mvmgr->getViewUpVectorJ2000();
			}
			break;
		}
		case eclipticJ2000:
		{
			double ra, dec;
			StelUtils::eclToEqu(spinLong, spinLat, core->getCurrentPlanet()->getRotObliquity(2451545.0), &ra, &dec);
			StelUtils::spheToRect(ra, dec, pos);
			break;
		}
		case ecliptic:
		{
			double ra, dec;
			StelUtils::eclToEqu(spinLong, spinLat, core->getCurrentPlanet()->getRotObliquity(core->getJDE()), &ra, &dec);
			StelUtils::spheToRect(ra, dec, pos);
			pos = core->equinoxEquToJ2000(pos, StelCore::RefractionOff);
			break;
		}
	}
	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, aimUp, 0.05);
	mvmgr->setFlagLockEquPos(useLockPosition);
}

void SearchDialog::onSearchTextChanged(const QString& text)
{
	// This block needs to go before the trimmedText.isEmpty() or the SIMBAD result does not
	// get properly cleared.
	if (useSimbad) {
		if (simbadReply) {
			disconnect(simbadReply,
				   SIGNAL(statusChanged()),
				   this,
				   SLOT(onSimbadStatusChanged()));
			delete simbadReply;
			simbadReply=Q_NULLPTR;
		}
		simbadResults.clear();
	}

	QString trimmedText = text.trimmed().toLower();
	if (trimmedText.isEmpty()) {
		ui->completionLabel->clearValues();
		ui->completionLabel->selectFirst();
		ui->simbadStatusLabel->setText("");
		ui->pushButtonGotoSearchSkyObject->setEnabled(false);
	} else {
		if (useSimbad)
		{
			simbadReply = simbadSearcher->lookup(simbadServerUrl, trimmedText, 4);
			onSimbadStatusChanged();
			connect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		}

		QString greekText = substituteGreek(trimmedText);
		QStringList matches;
		if(greekText != trimmedText)
		{
			matches  = objectMgr->listMatchingObjects(trimmedText, 8, useStartOfWords, false);
			matches += objectMgr->listMatchingObjects(trimmedText, 8, useStartOfWords, true);
			matches += objectMgr->listMatchingObjects(greekText, (18 - matches.size()), useStartOfWords, false);
			matches += objectMgr->listMatchingObjects(greekText, (18 - matches.size()), useStartOfWords, true);
		}
		else
		{
			matches  = objectMgr->listMatchingObjects(trimmedText, 13, useStartOfWords, false);
			matches += objectMgr->listMatchingObjects(trimmedText, 13, useStartOfWords, true);
		}

		// remove possible duplicates from completion list
		matches.removeDuplicates();

		matches.sort(Qt::CaseInsensitive);
		// objects with short names should be searched first
		// examples: Moon, Hydra (moon); Jupiter, Ghost of Jupiter
		stringLengthCompare comparator;
		qSort(matches.begin(), matches.end(), comparator);

		ui->completionLabel->setValues(matches);
		ui->completionLabel->selectFirst();

		// Update push button enabled state
		ui->pushButtonGotoSearchSkyObject->setEnabled(true);
	}
}


// Called when the current simbad query status changes
void SearchDialog::onSimbadStatusChanged()
{
	Q_ASSERT(simbadReply);
	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupErrorOccured)
	{
		ui->simbadStatusLabel->setText(QString("%1: %2")
					       .arg(q_("Simbad Lookup Error"))
					       .arg(simbadReply->getErrorString()));
		if (ui->completionLabel->isEmpty())
			ui->pushButtonGotoSearchSkyObject->setEnabled(false);
	}
	else
	{
		ui->simbadStatusLabel->setText(QString("%1: %2")
					       .arg(q_("Simbad Lookup"))
					       .arg(simbadReply->getCurrentStatusString()));
		// Query not over, don't disable button
		ui->pushButtonGotoSearchSkyObject->setEnabled(true);
	}

	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupFinished)
	{
		simbadResults = simbadReply->getResults();
		ui->completionLabel->appendValues(simbadResults.keys());
		// Update push button enabled state
		ui->pushButtonGotoSearchSkyObject->setEnabled(!ui->completionLabel->isEmpty());
	}

	if (simbadReply->getCurrentStatus()!=SimbadLookupReply::SimbadLookupQuerying)
	{
		disconnect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		delete simbadReply;
		simbadReply=Q_NULLPTR;

		// Update push button enabled state
		ui->pushButtonGotoSearchSkyObject->setEnabled(!ui->completionLabel->isEmpty());
	}
}

void SearchDialog::greekLetterClicked()
{
	QPushButton *sender = reinterpret_cast<QPushButton *>(this->sender());
	QLineEdit* sso = ui->lineEditSearchSkyObject;
	if (sender) {
		if (flagHasSelectedText)
		{
			sso->setText(sender->text());
			flagHasSelectedText = false;
		}
		else
			sso->setText(sso->text() + sender->text());
	}
	sso->setFocus();
}

void SearchDialog::gotoObject()
{
	gotoObject(ui->completionLabel->getSelected());
}

void SearchDialog::gotoObject(const QString &nameI18n)
{
	if (nameI18n.isEmpty())
		return;

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	if (simbadResults.contains(nameI18n))
	{
		if (objectMgr->findAndSelect(nameI18n))
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
				close();
				ui->lineEditSearchSkyObject->clear();
				ui->completionLabel->clearValues();
				// Can't point to home planet
				if (newSelected[0]->getEnglishName()!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
				{
					mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
					mvmgr->setFlagTracking(true);
				}
				else
				{
					GETSTELMODULE(StelObjectMgr)->unSelect();
				}
			}
		}
		else
		{
			close();
			GETSTELMODULE(CustomObjectMgr)->addCustomObject(nameI18n, simbadResults[nameI18n]);
			ui->lineEditSearchSkyObject->clear();
			ui->completionLabel->clearValues();
			if (objectMgr->findAndSelect(nameI18n))
			{
				const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
				// Can't point to home planet
				if (newSelected[0]->getEnglishName()!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
				{
					mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
					mvmgr->setFlagTracking(true);
				}
				else
				{
					GETSTELMODULE(StelObjectMgr)->unSelect();
				}
			}
		}
	}
	else if (objectMgr->findAndSelectI18n(nameI18n) || objectMgr->findAndSelect(nameI18n))
	{
		const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
		if (!newSelected.empty())
		{
			close();
			ui->lineEditSearchSkyObject->clear();
			ui->completionLabel->clearValues();
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
			{
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
			else
			{
				GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
	}
	simbadResults.clear();
}

void SearchDialog::gotoObject(QListWidgetItem *item)
{
	gotoObject(item->text());
}

void SearchDialog::searchListChanged(const QString &newText)
{
	QList<QListWidgetItem*> items = ui->objectsListWidget->findItems(newText, Qt::MatchStartsWith);
	ui->objectsListWidget->clearSelection();
	if (!items.isEmpty())
	{
		items.at(0)->setSelected(true);
		ui->objectsListWidget->scrollToItem(items.at(0));
	}
}

bool SearchDialog::eventFilter(QObject*, QEvent *event)
{
	if (event->type() == QEvent::KeyRelease)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Down)
		{
			ui->completionLabel->selectNext();
			event->accept();
			return true;
		}
		if (keyEvent->key() == Qt::Key_Up)
		{
			ui->completionLabel->selectPrevious();
			event->accept();
			return true;
		}
	}
	if (event->type() == QEvent::Show)
	{
		if (!extSearchText.isEmpty())
		{
			ui->lineEditSearchSkyObject->setText(extSearchText);
			ui->lineEditSearchSkyObject->selectAll();
			extSearchText.clear();
		}
	}


	return false;
}

QString SearchDialog::substituteGreek(const QString& keyString)
{
	if (!keyString.contains(' '))
		return getGreekLetterByName(keyString);
	else
	{
		QStringList nameComponents = keyString.split(" ", QString::SkipEmptyParts);
		if(!nameComponents.empty())
			nameComponents[0] = getGreekLetterByName(nameComponents[0]);
		return nameComponents.join(" ");
	}
}

QString SearchDialog::getGreekLetterByName(const QString& potentialGreekLetterName)
{
	if(staticData.greekLetters.contains(potentialGreekLetterName))
		return staticData.greekLetters[potentialGreekLetterName.toLower()];

	// There can be indices (e.g. "α1 Cen" instead of "α Cen A"), so strip
	// any trailing digit.
	int lastCharacterIndex = potentialGreekLetterName.length()-1;
	if(potentialGreekLetterName.at(lastCharacterIndex).isDigit())
	{
		QChar digit = potentialGreekLetterName.at(lastCharacterIndex);
		QString name = potentialGreekLetterName.left(lastCharacterIndex);
		if(staticData.greekLetters.contains(name))
			return staticData.greekLetters[name.toLower()] + digit;
	}

	return potentialGreekLetterName;
}

void SearchDialog::populateSimbadServerList()
{
	Q_ASSERT(ui->serverListComboBox);

	QComboBox* servers = ui->serverListComboBox;
	//Save the current selection to be restored later
	servers->blockSignals(true);
	int index = servers->currentIndex();
	QVariant selectedUrl = servers->itemData(index);
	servers->clear();
	//For each server, display the localized description and store the URL as user data.
	servers->addItem(q_("University of Strasbourg (France)"), DEF_SIMBAD_URL);
	servers->addItem(q_("Harvard University (USA)"), "http://simbad.harvard.edu/");

	//Restore the selection
	index = servers->findData(selectedUrl, Qt::UserRole, Qt::MatchCaseSensitive);
	servers->setCurrentIndex(index);
	servers->model()->sort(0);
	servers->blockSignals(false);
}

void SearchDialog::selectSimbadServer(int index)
{
	if (index < 0)
		simbadServerUrl = DEF_SIMBAD_URL;
	else
		simbadServerUrl = ui->serverListComboBox->itemData(index).toString();

	conf->setValue("search/simbad_server_url", simbadServerUrl);
}

void SearchDialog::updateListWidget(int index)
{
	QString moduleId = ui->objectTypeComboBox->itemData(index).toString();
	ui->objectsListWidget->clear();
	bool englishNames = ui->searchInEnglishCheckBox->isChecked();
	ui->objectsListWidget->addItems(objectMgr->listAllModuleObjects(moduleId, englishNames));
	ui->objectsListWidget->sortItems(Qt::AscendingOrder);
	connect(ui->objectsListWidget, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(gotoObject(QListWidgetItem*)),
		Qt::UniqueConnection); //bugfix: prevent multiple connections, which seems to have happened before
}

void SearchDialog::updateListTab()
{
	if (StelApp::getInstance().getLocaleMgr().getAppLanguage().startsWith("en"))
	{
		// hide "names in English" checkbox
		ui->searchInEnglishCheckBox->hide();
	}
	else
	{
		ui->searchInEnglishCheckBox->show();
	}
	ui->objectTypeComboBox->blockSignals(true);
	ui->objectTypeComboBox->clear();	
	QMap<QString, QString> modulesMap = objectMgr->objectModulesMap();
	for (QMap<QString, QString>::const_iterator it = modulesMap.begin(); it != modulesMap.end(); ++it)
	{
		if (!objectMgr->listAllModuleObjects(it.key(), ui->searchInEnglishCheckBox->isChecked()).isEmpty())
		{
			QString moduleName = (ui->searchInEnglishCheckBox->isChecked() ? it.value(): q_(it.value()));
			ui->objectTypeComboBox->addItem(moduleName, QVariant(it.key()));
		}
	}	
	ui->objectTypeComboBox->model()->sort(0, Qt::AscendingOrder);
	ui->objectTypeComboBox->blockSignals(false);
	updateListWidget(ui->objectTypeComboBox->currentIndex());
}

void SearchDialog::showContextMenu(const QPoint &pt)
{
	QMenu *menu = ui->lineEditSearchSkyObject->createStandardContextMenu();
	menu->addSeparator();
	QString clipText;
	QClipboard *clipboard = QApplication::clipboard();
	if (clipboard)
		clipText = clipboard->text();
	if (!clipText.isEmpty())
	{
		if (clipText.length()>12)
			clipText = clipText.right(9) + "...";
		clipText = "\t(" + clipText + ")";
	}

	menu->addAction(q_("Paste and Search") + clipText, this, SLOT(pasteAndGo()));
	menu->exec(ui->lineEditSearchSkyObject->mapToGlobal(pt));
	delete menu;
}

void SearchDialog::pasteAndGo()
{
	ui->lineEditSearchSkyObject->clear(); // clear current text
	ui->lineEditSearchSkyObject->paste(); // paste text from clipboard
	gotoObject(); // go to first finded object
}
