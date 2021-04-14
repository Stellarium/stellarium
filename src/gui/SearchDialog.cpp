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
#include "SpecialMarkersMgr.hpp"
#include "CustomObjectMgr.hpp"
#include "SolarSystem.hpp"

#include "StelObjectMgr.hpp"
#include "StelGui.hpp"
#include "StelUtils.hpp"

#include "StelFileMgr.hpp"
#include "StelJsonParser.hpp"

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
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QFileDialog>
#include <QDir>
#include <QSet>
#include <QDialog>
#include <QAbstractItemModel>

#include "SimbadSearcher.hpp"

// Start of members for class CompletionListModel
CompletionListModel::CompletionListModel(QObject* parent):
	QStringListModel(parent),
	selectedIdx(0)
{
}

CompletionListModel::CompletionListModel(const QStringList &string, QObject* parent):
	QStringListModel(string, parent),
	selectedIdx(0)
{
}

CompletionListModel::~CompletionListModel()
{
}

void CompletionListModel::setValues(const QStringList& v, const QStringList& rv)
{
	values=v;
	recentValues=rv;
	updateText();
}

void CompletionListModel::appendRecentValues(const QStringList& v)
{
	recentValues+=v;
}

void CompletionListModel::appendValues(const QStringList& v)
{
	values+=v;
	updateText();
}

void CompletionListModel::clearValues()
{
	// Default: Show recent values
	values.clear();
	values = recentValues;
	selectedIdx=0;
	updateText();
}

QString CompletionListModel::getSelected() const
{
	if (values.isEmpty())
		return QString();
	return values.at(selectedIdx);
}

void CompletionListModel::selectNext()
{
	++selectedIdx;
	if (selectedIdx>=values.size())
		selectedIdx=0;
	updateText();
}

void CompletionListModel::selectPrevious()
{
	--selectedIdx;
	if (selectedIdx<0)
		selectedIdx = values.size()-1;
	updateText();
}

void CompletionListModel::selectFirst()
{
	selectedIdx=0;
	updateText();
}

void CompletionListModel::updateText()
{
	this->setStringList(values);
}

QVariant CompletionListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	    return QVariant();

	// Bold recent objects
	if(role == Qt::FontRole)
	{
	    QFont font;
	    bool toBold = recentValues.contains(index.data(Qt::DisplayRole).toString()) ?
				    true : false;
	    font.setBold(toBold);
	    return font;
	}

	return QStringListModel::data(index, role);
}

// Start of members for class SearchDialog

const char* SearchDialog::DEF_SIMBAD_URL = "https://simbad.u-strasbg.fr/";
SearchDialog::SearchDialogStaticData SearchDialog::staticData;
QString SearchDialog::extSearchText = "";

SearchDialog::SearchDialog(QObject* parent)
	: StelDialog("Search", parent)
	, simbadReply(Q_NULLPTR)
	, listModel(Q_NULLPTR)
	, proxyModel(Q_NULLPTR)
	, flagHasSelectedText(false)
{
	setObjectName("SearchDialog");
	ui = new Ui_searchDialogForm;
	simbadSearcher = new SimbadSearcher(this);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objectMgr);

	StelApp::getInstance().getStelPropertyManager()->registerObject(this);
	conf = StelApp::getInstance().getSettings();
	enableSimbadSearch(conf->value("search/flag_search_online", true).toBool());
	useStartOfWords = conf->value("search/flag_start_words", false).toBool();
	useLockPosition = conf->value("search/flag_lock_position", true).toBool();
	useFOVCenterMarker = conf->value("search/flag_fov_center_marker", true).toBool();
	fovCenterMarkerState = GETSTELMODULE(SpecialMarkersMgr)->getFlagFOVCenterMarker();
	simbadServerUrl = conf->value("search/simbad_server_url", DEF_SIMBAD_URL).toString();
	setCurrentCoordinateSystemKey(conf->value("search/coordinate_system", "equatorialJ2000").toString());	

	setSimbadQueryDist( conf->value("search/simbad_query_dist",  30).toInt());
	setSimbadQueryCount(conf->value("search/simbad_query_count",  3).toInt());
	setSimbadGetsIds(   conf->value("search/simbad_query_IDs",        true ).toBool());
	setSimbadGetsSpec(  conf->value("search/simbad_query_spec",       false).toBool());
	setSimbadGetsMorpho(conf->value("search/simbad_query_morpho",     false).toBool());
	setSimbadGetsTypes( conf->value("search/simbad_query_types",      false).toBool());
	setSimbadGetsDims(  conf->value("search/simbad_query_dimensions", false).toBool());

	shiftPressed = false;

	// Init CompletionListModel
	searchListModel = new CompletionListModel();

	// Find recent object search data file
	recentObjectSearchesJsonPath = StelFileMgr::findFile("data", (StelFileMgr::Flags)(StelFileMgr::Directory | StelFileMgr::Writable)) + "/recentObjectSearches.json";
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
		populateRecentSearch();
		updateListTab();
	}
}

void SearchDialog::setCurrentCoordinateSystemKey(QString key)
{
	const QMetaEnum& en = metaObject()->enumerator(metaObject()->indexOfEnumerator("CoordinateSystem"));
	CoordinateSystem coordSystem = static_cast<CoordinateSystem>(en.keyToValue(key.toLatin1().data()));
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
	connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)), this, SLOT(onSearchTextChanged(const QString&)));
	connect(ui->simbadCooQueryButton, SIGNAL(clicked()), this, SLOT(lookupCoordinates()));
	connect(GETSTELMODULE(StelObjectMgr), SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), this, SLOT(clearSimbadText(StelModule::StelModuleSelectAction)));
	connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
	connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(selectionChanged()), this, SLOT(setHasSelectedFlag()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

	ui->lineEditSearchSkyObject->installEventFilter(this);	

	// Kinetic scrolling
	kineticScrollingList << ui->objectsListView;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	populateCoordinateSystemsList();
	populateCoordinateAxis();
	int idx = ui->coordinateSystemComboBox->findData(getCurrentCoordinateSystemKey(), Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1) // Use equatorialJ2000 as default
		idx = ui->coordinateSystemComboBox->findData(QVariant("equatorialJ2000"), Qt::UserRole, Qt::MatchCaseSensitive);
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

	connectBoolProperty(ui->simbadGroupBox, "SearchDialog.useSimbad");	
	connectIntProperty(ui->searchRadiusSpinBox,    "SearchDialog.simbadDist");
	connectIntProperty(ui->resultsSpinBox,         "SearchDialog.simbadCount");
	connectBoolProperty(ui->allIDsCheckBox,        "SearchDialog.simbadGetIds");
	connectBoolProperty(ui->spectralClassCheckBox, "SearchDialog.simbadGetSpec");
	connectBoolProperty(ui->morphoCheckBox,        "SearchDialog.simbadGetMorpho");
	connectBoolProperty(ui->typesCheckBox,         "SearchDialog.simbadGetTypes");
	connectBoolProperty(ui->dimsCheckBox,          "SearchDialog.simbadGetDims");

	populateSimbadServerList();
	idx = ui->serverListComboBox->findData(simbadServerUrl, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1) // Use University of Strasbourg as default
		idx = ui->serverListComboBox->findData(QVariant(DEF_SIMBAD_URL), Qt::UserRole, Qt::MatchCaseSensitive);
	ui->serverListComboBox->setCurrentIndex(idx);
	connect(ui->serverListComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSimbadServer(int)));

	connect(ui->checkBoxUseStartOfWords, SIGNAL(clicked(bool)), this, SLOT(enableStartOfWordsAutofill(bool)));
	ui->checkBoxUseStartOfWords->setChecked(useStartOfWords);

	connect(ui->checkBoxFOVCenterMarker, SIGNAL(clicked(bool)), this, SLOT(enableFOVCenterMarker(bool)));
	ui->checkBoxFOVCenterMarker->setChecked(useFOVCenterMarker);

	connect(ui->checkBoxLockPosition, SIGNAL(clicked(bool)), this, SLOT(enableLockPosition(bool)));
	ui->checkBoxLockPosition->setChecked(useLockPosition);

	// list views initialization
	listModel = new QStringListModel(this);
	proxyModel = new QSortFilterProxyModel(ui->objectsListView);
	proxyModel->setSourceModel(listModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->objectsListView->setModel(proxyModel);

	connect(ui->objectTypeComboBox, SIGNAL(activated(int)), this, SLOT(updateListView(int)));
	connect(ui->searchInListLineEdit, SIGNAL(textChanged(const QString&)), proxyModel, SLOT(setFilterWildcard(const QString&)));
	connect(ui->searchInEnglishCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateListTab()));
	QAction *clearAction = ui->searchInListLineEdit->addAction(QIcon(":/graphicGui/uieBackspaceInputButton.png"), QLineEdit::ActionPosition::TrailingPosition);
	connect(clearAction, SIGNAL(triggered()), this, SLOT(searchListClear()));
	updateListTab();

	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));
	// Set the focus directly on the line editDe	if (ui->tabWidget->currentIndex()==0)
	ui->lineEditSearchSkyObject->setFocus();

	connect(StelApp::getInstance().getCore(), SIGNAL(updateSearchLists()), this, SLOT(updateListTab()));

	QString style = "QLabel { color: rgb(238, 238, 238); }";
	ui->simbadStatusLabel->setStyleSheet(style);
	ui->labelGreekLetterTitle->setStyleSheet(style);
	ui->simbadCooStatusLabel->setStyleSheet(style);

	// Get data from previous session
	loadRecentSearches();

	// Create list model view
	ui->searchListView->setModel(searchListModel);
	searchListModel->setStringList(searchListModel->getValues());

	// Auto display recent searches
	QStringList recentMatches = listMatchingRecentObjects("", recentObjectSearchesData.maxSize, useStartOfWords);
	resetSearchResultDisplay(recentMatches, recentMatches);
	setPushButtonGotoSearch();

	// Update max size of "recent object searches"
	connect(ui->recentSearchSizeSpinBox, SIGNAL(editingFinished()), this, SLOT(recentSearchSizeEditingFinished()));
	// Clear data from recent search object
	connect(ui->recentSearchClearDataPushButton, SIGNAL(clicked()), this, SLOT(recentSearchClearDataClicked()));
	populateRecentSearch();
}

void SearchDialog::populateRecentSearch()
{
	// Tooltip for recentSearchSizeSpinBox
	QString toolTipComment = QString("%1: %2 | %3: %4 - %5 %6")
			.arg(qc_("Default", "search tool"))
			.arg(defaultMaxSize)
			.arg(qc_("Range", "search tool"))
			.arg(ui->recentSearchSizeSpinBox->minimum())
			.arg(ui->recentSearchSizeSpinBox->maximum())
			.arg(qc_("searches", "search tool"));
	ui->recentSearchSizeSpinBox->setToolTip(toolTipComment);
	setRecentSearchClearDataPushButton();
}

void SearchDialog::changeTab(int index)
{
	if (index==0) // Search Tab
		ui->lineEditSearchSkyObject->setFocus();

	if (index==2) // Position
	{
		if (useFOVCenterMarker)
			GETSTELMODULE(SpecialMarkersMgr)->setFlagFOVCenterMarker(true);
	}
	else
		GETSTELMODULE(SpecialMarkersMgr)->setFlagFOVCenterMarker(fovCenterMarkerState);

	if (index==3) // Lists
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
	if (dialog && ui->simbadStatusLabel) ui->simbadStatusLabel->clear();
	if (dialog && ui->simbadCooStatusLabel) ui->simbadCooStatusLabel->clear();	
	if (dialog && ui->simbadTab) ui->simbadTab->setEnabled(enable);
	emit simbadUseChanged(enable);
}

void SearchDialog::setSimbadQueryDist(int dist)
{
	simbadDist=dist;
	conf->setValue("search/simbad_query_dist", simbadDist);
	emit simbadQueryDistChanged(dist);
}

void SearchDialog::setSimbadQueryCount(int count)
{
	simbadCount=count;
	conf->setValue("search/simbad_query_count", simbadCount);
	emit simbadQueryCountChanged(count);
}
void SearchDialog::setSimbadGetsIds(bool b)
{
	simbadGetIds=b;
	conf->setValue("search/simbad_query_IDs", b);
	emit simbadGetsIdsChanged(b);
}

void SearchDialog::setSimbadGetsSpec(bool b)
{
	simbadGetSpec=b;
	conf->setValue("search/simbad_query_spec", b);
	emit simbadGetsSpecChanged(b);
}

void SearchDialog::setSimbadGetsMorpho(bool b)
{
	simbadGetMorpho=b;
	conf->setValue("search/simbad_query_morpho", b);
	emit simbadGetsMorphoChanged(b);
}

void SearchDialog::setSimbadGetsTypes(bool b)
{
	simbadGetTypes=b;
	conf->setValue("search/simbad_query_types", b);
	emit simbadGetsTypesChanged(b);
}

void SearchDialog::setSimbadGetsDims(bool b)
{
	simbadGetDims=b;
	conf->setValue("search/simbad_query_dimensions", b);
	emit simbadGetsDimsChanged(b);
}

void SearchDialog::recentSearchSizeEditingFinished()
{
	// Update max size in dialog and user data
	int maxSize = ui->recentSearchSizeSpinBox->value();
	setRecentSearchSize(maxSize);
	maxSize = recentObjectSearchesData.maxSize; // Might not be the same

	// Save maxSize to user's data
	saveRecentSearches();

	// Update search result on "Object" tab
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
}

void SearchDialog::recentSearchClearDataClicked()
{
	// Clear recent list from current run
	recentObjectSearchesData.recentList.clear();

	// Save empty list to user's data file
	saveRecentSearches();

	// Update search result on "Object" tab
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
}

void SearchDialog::setRecentSearchClearDataPushButton()
{
	// Enable clear button if recent list is greater than 0
	bool toEnable = recentObjectSearchesData.recentList.size() > 0;
	ui->recentSearchClearDataPushButton->setEnabled(toEnable);
	// Tool tip depends on recent list size
	QString toolTipText;
	toolTipText = toEnable ? q_("Clear search history: delete all search objects data") : q_("Clear search history: no data to delete");
	ui->recentSearchClearDataPushButton->setToolTip(toolTipText);
}

void SearchDialog::enableStartOfWordsAutofill(bool enable)
{
	useStartOfWords = enable;
	conf->setValue("search/flag_start_words", useStartOfWords);

	// Update search result on "Object" tab
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
}

void SearchDialog::enableLockPosition(bool enable)
{
	useLockPosition = enable;
	conf->setValue("search/flag_lock_position", useLockPosition);
}

void SearchDialog::enableFOVCenterMarker(bool enable)
{
	useFOVCenterMarker = enable;
	fovCenterMarkerState = GETSTELMODULE(SpecialMarkersMgr)->getFlagFOVCenterMarker();
	conf->setValue("search/flag_fov_center_marker", useFOVCenterMarker);
}

void SearchDialog::setSimpleStyle()
{
	ui->AxisXSpinBox->setVisible(false);
	ui->AxisXSpinBox->setVisible(false);
	ui->simbadStatusLabel->setVisible(false);
	ui->simbadCooStatusLabel->setVisible(false);
	ui->AxisXLabel->setVisible(false);
	ui->AxisYLabel->setVisible(false);
	ui->coordinateSystemLabel->setVisible(false);
	ui->coordinateSystemComboBox->setVisible(false);
}


void SearchDialog::manualPositionChanged()
{
	searchListModel->clearValues();
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

	switch (getCurrentCoordinateSystem())
	{
		case equatorialJ2000:
		{
			StelUtils::spheToRect(spinLong, spinLat, pos);
			if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat)> (0.9*M_PI_2)) )
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

			if ( (mountMode==StelMovementMgr::MountEquinoxEquatorial) && (fabs(spinLat)> (0.9*M_PI_2)) )
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
			pos = core->altAzToJ2000(pos, StelCore::RefractionOff);
			core->setTimeRate(0.);

			if ( (mountMode==StelMovementMgr::MountAltAzimuthal) && (fabs(spinLat)> (0.9*M_PI_2)) )
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
			if ( (mountMode==StelMovementMgr::MountGalactic) && (fabs(spinLat)> (0.9*M_PI_2)) )
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
			if ( (mountMode==StelMovementMgr::MountSupergalactic) && (fabs(spinLat)> (0.9*M_PI_2)) )
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
			StelUtils::eclToEqu(spinLong, spinLat, GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(2451545.0), &ra, &dec);
			StelUtils::spheToRect(ra, dec, pos);
			break;
		}
		case ecliptic:
		{
			double ra, dec;
			StelUtils::eclToEqu(spinLong, spinLat, GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(core->getJDE()), &ra, &dec);
			StelUtils::spheToRect(ra, dec, pos);
			pos = core->equinoxEquToJ2000(pos, StelCore::RefractionOff);
			break;
		}
	}
	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, aimUp, mvmgr->getAutoMoveDuration());
	mvmgr->setFlagLockEquPos(useLockPosition);
}

void SearchDialog::onSearchTextChanged(const QString& text)
{
	clearSimbadText(StelModule::ReplaceSelection);
	// This block needs to go before the trimmedText.isEmpty() or the SIMBAD result does not
	// get properly cleared.
	if (useSimbad)
	{
		if (simbadReply)
		{
			disconnect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
			delete simbadReply;
			simbadReply=Q_NULLPTR;
		}
		simbadResults.clear();
	}

	// Use to adjust matches to be within range of maxNbItem
	int maxNbItem;
	QString trimmedText = text.trimmed();
	if (trimmedText.isEmpty())
	{
		searchListModel->clearValues();

		maxNbItem = recentObjectSearchesData.maxSize;
		// Auto display recent searches
		QStringList recentMatches = listMatchingRecentObjects(trimmedText, maxNbItem, useStartOfWords);
		resetSearchResultDisplay(recentMatches, recentMatches);

		ui->simbadStatusLabel->setText("");
		ui->simbadCooStatusLabel->setText("");
		setPushButtonGotoSearch();
	}
	else
	{
		if (useSimbad)
		{
			simbadReply = simbadSearcher->lookup(simbadServerUrl, trimmedText, 4);
			onSimbadStatusChanged();
			connect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		}

		// Get possible objects
		QStringList matches;
		QStringList recentMatches;
		QStringList allMatches;

		QString greekText = substituteGreek(trimmedText);

		int trimmedTextMaxNbItem = 13;
		int greekTextMaxMbItem = 0;

		if(greekText != trimmedText)
		{
			trimmedTextMaxNbItem = 8;
			greekTextMaxMbItem = 18;

			// Get recent matches
			// trimmedText
			recentMatches = listMatchingRecentObjects(trimmedText, trimmedTextMaxNbItem, useStartOfWords);
			// greekText
			recentMatches += listMatchingRecentObjects(greekText, (greekTextMaxMbItem - recentMatches.size()), useStartOfWords);

			// Get rest of matches
			// trimmedText
			matches = objectMgr->listMatchingObjects(trimmedText, trimmedTextMaxNbItem, useStartOfWords);
			// greekText
			matches += objectMgr->listMatchingObjects(greekText, (greekTextMaxMbItem - matches.size()), useStartOfWords);
		}
		else
		{
			trimmedTextMaxNbItem = 13;

			// Get recent matches
			recentMatches = listMatchingRecentObjects(trimmedText, trimmedTextMaxNbItem, useStartOfWords);

			// Get rest of matches
			matches  = objectMgr->listMatchingObjects(trimmedText, trimmedTextMaxNbItem, useStartOfWords);
		}
		// Check in case either number changes since they were
		// hard coded
		maxNbItem  = qMax(greekTextMaxMbItem, trimmedTextMaxNbItem);

		// Clean up matches
		adjustMatchesResult(allMatches, recentMatches, matches, maxNbItem);

		// Updates values
		resetSearchResultDisplay(allMatches, recentMatches);

		// Update push button enabled state
		setPushButtonGotoSearch();
	}

	// Goto object when clicking in list
	connect(ui->searchListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(gotoObject(const QModelIndex&)), Qt::UniqueConnection);
	connect(ui->searchListView, SIGNAL(activated(const QModelIndex&)), this, SLOT(gotoObject(const QModelIndex&)), Qt::UniqueConnection);
}

void SearchDialog::updateRecentSearchList(const QString &nameI18n)
{
	if(nameI18n.isEmpty())
		return;

	// Prepend & remove duplicates
	recentObjectSearchesData.recentList.prepend(nameI18n);
	recentObjectSearchesData.recentList.removeDuplicates();

	adjustRecentList(recentObjectSearchesData.maxSize);

	// Auto display recent searches
	QStringList recentMatches = listMatchingRecentObjects("", recentObjectSearchesData.maxSize, useStartOfWords);
	resetSearchResultDisplay(recentMatches, recentMatches);
}

void SearchDialog::adjustRecentList(int maxSize)
{	
	// Check if max size was updated recently
	maxSize = (maxSize >= 0) ? maxSize : recentObjectSearchesData.maxSize;
	recentObjectSearchesData.maxSize = maxSize;

	// Max amount of saved values "allowed"
	int spinBoxMaxSize = ui->recentSearchSizeSpinBox->maximum();

	// Only removing old searches if the list grows larger than the largest
	// "allowed" size (to retain data in case the user switches from
	// high to low size)
	if( recentObjectSearchesData.recentList.size() > spinBoxMaxSize)
		recentObjectSearchesData.recentList = recentObjectSearchesData.recentList.mid(0, spinBoxMaxSize);
}

void SearchDialog::adjustMatchesResult(QStringList &allMatches, QStringList& recentMatches, QStringList& matches, int maxNbItem)
{
	int tempSize;
	QStringList tempMatches; // unsorted matches use for calculation
	// not displaying

	// remove possible duplicates from completion list
	matches.removeDuplicates();

	matches.sort(Qt::CaseInsensitive);
	// objects with short names should be searched first
	// examples: Moon, Hydra (moon); Jupiter, Ghost of Jupiter
	stringLengthCompare comparator;
	std::sort(matches.begin(), matches.end(), comparator);

	// Adjust recent matches to prefered max size
	recentMatches = recentMatches.mid(0, recentObjectSearchesData.maxSize);

	// Find total size of both matches
	tempMatches << recentMatches << matches; // unsorted
	tempMatches.removeDuplicates();
	tempSize = tempMatches.size();

	// Adjust match size to be within range
	if(tempSize>maxNbItem)
	{
		int i = tempSize - maxNbItem;
		matches = matches.mid(0, matches.size() - i);
	}

	// Combine list: ordered by recent searches then relevance
	allMatches << recentMatches << matches;

	// Remove possible duplicates from both listQSt
	allMatches.removeDuplicates();
}


void SearchDialog::resetSearchResultDisplay(QStringList allMatches,
					       QStringList recentMatches)
{
	// Updates values
	searchListModel->appendValues(allMatches);
	searchListModel->appendRecentValues(recentMatches);

	// Update display
	searchListModel->setValues(allMatches, recentMatches);
	searchListModel->selectFirst();

	// Update highlight to top
	ui->searchListView->scrollToTop();
	int row = searchListModel->getSelectedIdx();
	ui->searchListView->setCurrentIndex(searchListModel->index(row));

	// Enable clear data button
	setRecentSearchClearDataPushButton();
}

void SearchDialog::setPushButtonGotoSearch()
{
	// Empty search and empty recently search object list
	if (searchListModel->isEmpty() && (recentObjectSearchesData.recentList.size() == 0))
		ui->pushButtonGotoSearchSkyObject->setEnabled(false); // Do not enable search button
	else
		ui->pushButtonGotoSearchSkyObject->setEnabled(true); // Do enable search  button
}

void SearchDialog::loadRecentSearches()
{
	QVariantMap map;
	QFile jsonFile(recentObjectSearchesJsonPath);
	if(!jsonFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Search] Can not open data file for recent searches"
			   << QDir::toNativeSeparators(recentObjectSearchesJsonPath);

		// Use default value for recent search size
		setRecentSearchSize(ui->recentSearchSizeSpinBox->value());
	}
	else
	{
		try
		{
			int readMaxSize;

			map = StelJsonParser::parse(jsonFile.readAll()).toMap();
			jsonFile.close();

			QVariantMap recentSearchData = map.value("recentObjectSearches").toMap();

			// Get user's maxSize data (if possible)
			readMaxSize = recentSearchData.value("maxSize").toInt();
			 // Non-negative size only
			recentObjectSearchesData.maxSize = (readMaxSize >= 0) ? readMaxSize : recentObjectSearchesData.maxSize;

			// Update dialog size to match user's preference
			ui->recentSearchSizeSpinBox->setValue(recentObjectSearchesData.maxSize);

			// Get user's recentList data (if possible)
			recentObjectSearchesData.recentList = recentSearchData.value("recentList").toStringList();
		}
		catch (std::runtime_error &e)
		{
			qWarning() << "[Search] File format is Wrong! Error:"
				   << e.what();
			return;
		}
	}
}

void SearchDialog::saveRecentSearches()
{
	if(recentObjectSearchesJsonPath.isEmpty())
	{
		qWarning() << "[Search] Error in saving recent object searches";
		return;
	}

	QFile jsonFile(recentObjectSearchesJsonPath);
	if(!jsonFile.open(QFile::WriteOnly | QFile::Text))
	{
		qWarning() << "[Search] Recent search could not be save. A file can not be open for writing:"
			   << QDir::toNativeSeparators(recentObjectSearchesJsonPath);
		return;
	}

	QVariantMap rslDataList;
	rslDataList.insert("maxSize", recentObjectSearchesData.maxSize);
	rslDataList.insert("recentList", recentObjectSearchesData.recentList);
	
	QVariantMap rsList;
	rsList.insert("recentObjectSearches", rslDataList);

	// Convert the tree to JSON
	StelJsonParser::write(rsList, &jsonFile);
	jsonFile.flush();
	jsonFile.close();
}

QStringList SearchDialog::listMatchingRecentObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;

	if(maxNbItem <= 0)
		return result;

	// For all recent objects:
	for (int i = 0; i < recentObjectSearchesData.recentList.size(); i++)
	{
		bool toAppend = useStartOfWords ? recentObjectSearchesData.recentList[i].startsWith(objPrefix, Qt::CaseInsensitive)
						: recentObjectSearchesData.recentList[i].contains(objPrefix, Qt::CaseInsensitive);

		if(toAppend)
			result.append(recentObjectSearchesData.recentList[i]);

		if (result.size() >= maxNbItem)
			break;
	}
	return result;
}


void SearchDialog::lookupCoordinates()
{
	if (!useSimbad)
		return;

	StelCore * core=StelApp::getInstance().getCore();
	const QList<StelObjectP>& sel=GETSTELMODULE(StelObjectMgr)->getSelectedObject();
	if (sel.length()==0)
		return;

	Vec3d coords=sel.at(0)->getJ2000EquatorialPos(core);

	simbadReply = simbadSearcher->lookupCoords(simbadServerUrl, coords, getSimbadQueryCount(), 500,
						   getSimbadQueryDist(), getSimbadGetsIds(), getSimbadGetsTypes(),
						   getSimbadGetsSpec(), getSimbadGetsMorpho(), getSimbadGetsDims());
	onSimbadStatusChanged();
	connect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
}

void SearchDialog::clearSimbadText(StelModule::StelModuleSelectAction)
{
	ui->simbadCooResultsTextBrowser->clear();
}

// Called when the current simbad query status changes
void SearchDialog::onSimbadStatusChanged()
{
	Q_ASSERT(simbadReply);
	int index = ui->tabWidget->currentIndex();
	QString info;
	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupErrorOccured)
	{
		info = QString("%1: %2").arg(q_("Simbad Lookup Error")).arg(simbadReply->getErrorString());
		index==1 ? ui->simbadCooStatusLabel->setText(info) : ui->simbadStatusLabel->setText(info);
		setPushButtonGotoSearch();
		ui->simbadCooResultsTextBrowser->clear();
	}
	else
	{
		info = QString("%1: %2").arg(q_("Simbad Lookup")).arg(simbadReply->getCurrentStatusString());
		index==1 ? ui->simbadCooStatusLabel->setText(info) : ui->simbadStatusLabel->setText(info);
		// Query not over, don't disable button
		ui->pushButtonGotoSearchSkyObject->setEnabled(true);
	}

	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadLookupFinished)
	{
		simbadResults = simbadReply->getResults();
		searchListModel->appendValues(simbadResults.keys());
		// Update push button enabled state
		setPushButtonGotoSearch();
	}

	if (simbadReply->getCurrentStatus()==SimbadLookupReply::SimbadCoordinateLookupFinished)
	{
		QString ret = simbadReply->getResult();
		ui->simbadCooResultsTextBrowser->setText(ret);
	}

	if (simbadReply->getCurrentStatus()!=SimbadLookupReply::SimbadLookupQuerying)
	{
		disconnect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		delete simbadReply;
		simbadReply=Q_NULLPTR;

		// Update push button enabled state
		setPushButtonGotoSearch();
	}
}

void SearchDialog::greekLetterClicked()
{
	QPushButton *sender = reinterpret_cast<QPushButton *>(this->sender());
	QLineEdit* sso = ui->lineEditSearchSkyObject;
	QString text;
	if (sender)
	{
		shiftPressed ? text = sender->text().toUpper() : text = sender->text();
		if (flagHasSelectedText)
		{
			sso->setText(text);
			flagHasSelectedText = false;
		}
		else
			sso->setText(sso->text() + text);
	}
	sso->setFocus();
}

void SearchDialog::gotoObject()
{
	gotoObject(searchListModel->getSelected());
}

void SearchDialog::gotoObject(const QString &nameI18n)
{
	if (nameI18n.isEmpty())
		return;

	// Save recent search list
	updateRecentSearchList(nameI18n);
	saveRecentSearches();

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	if (simbadResults.contains(nameI18n))
	{
		if (objectMgr->findAndSelect(nameI18n))
		{
			const QList<StelObjectP> newSelected = objectMgr->getSelectedObject();
			if (!newSelected.empty())
			{
				close();
				ui->lineEditSearchSkyObject->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F

				// Can't point to home planet
				if (newSelected[0]->getEnglishName()!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
				{
					mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
					mvmgr->setFlagTracking(true);
				}
				else
					GETSTELMODULE(StelObjectMgr)->unSelect();
			}
		}
		else
		{
			close();
			GETSTELMODULE(CustomObjectMgr)->addCustomObject(nameI18n, simbadResults[nameI18n]);
			ui->lineEditSearchSkyObject->clear();
			searchListModel->clearValues();
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
					GETSTELMODULE(StelObjectMgr)->unSelect();
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
			
			// Can't point to home planet
			if (newSelected[0]->getEnglishName()!=StelApp::getInstance().getCore()->getCurrentLocation().planetName)
			{
				mvmgr->moveToObject(newSelected[0], mvmgr->getAutoMoveDuration());
				mvmgr->setFlagTracking(true);
			}
			else
				GETSTELMODULE(StelObjectMgr)->unSelect();
		}
	}
	simbadResults.clear();
}

void SearchDialog::gotoObject(const QModelIndex &modelIndex)
{
	gotoObject(modelIndex.model()->data(modelIndex, Qt::DisplayRole).toString());
}

void SearchDialog::searchListClear()
{
	ui->searchInListLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
}

bool SearchDialog::eventFilter(QObject*, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Shift)
		{
			shiftPressed = true;
			event->accept();
			return true;
		}
	}
	if (event->type() == QEvent::KeyRelease)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Down)
		{
			searchListModel->selectNext();
			int row = searchListModel->getSelectedIdx();
			ui->searchListView->scrollTo(searchListModel->index(row));
			ui->searchListView->setCurrentIndex(searchListModel->index(row));
			event->accept();
			return true;
		}
		if (keyEvent->key() == Qt::Key_Up)
		{
			searchListModel->selectPrevious();
			int row = searchListModel->getSelectedIdx();
			ui->searchListView->scrollTo(searchListModel->index(row));
			ui->searchListView->setCurrentIndex(searchListModel->index(row));
			event->accept();
			return true;
		}
		if (keyEvent->key() == Qt::Key_Shift)
		{
			shiftPressed = false;
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
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		QStringList nameComponents = keyString.split(" ", Qt::SkipEmptyParts);
		#else
		QStringList nameComponents = keyString.split(" ", QString::SkipEmptyParts);
		#endif
		if(!nameComponents.empty())
			nameComponents[0] = getGreekLetterByName(nameComponents[0]);
		return nameComponents.join(" ");
	}
}

QString SearchDialog::getGreekLetterByName(const QString& potentialGreekLetterName)
{
	if(staticData.greekLetters.contains(potentialGreekLetterName))
		return staticData.greekLetters[potentialGreekLetterName];

	// There can be indices (e.g. "α1 Cen" instead of "α Cen A"), so strip
	// any trailing digit.
	int lastCharacterIndex = potentialGreekLetterName.length()-1;
	if(potentialGreekLetterName.at(lastCharacterIndex).isDigit())
	{
		QChar digit = potentialGreekLetterName.at(lastCharacterIndex);
		QString name = potentialGreekLetterName.left(lastCharacterIndex);
		if(staticData.greekLetters.contains(name))
			return staticData.greekLetters[name] + digit;
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

void SearchDialog::setRecentSearchSize(int maxSize)
{
	adjustRecentList(maxSize);
	saveRecentSearches();
	conf->setValue("search/recentSearchSize", recentObjectSearchesData.maxSize);
}

void SearchDialog::selectSimbadServer(int index)
{
	index < 0 ? simbadServerUrl = DEF_SIMBAD_URL : simbadServerUrl = ui->serverListComboBox->itemData(index).toString();
	conf->setValue("search/simbad_server_url", simbadServerUrl);
}

void SearchDialog::updateListView(int index)
{
	QString moduleId = ui->objectTypeComboBox->itemData(index).toString();
	bool englishNames = ui->searchInEnglishCheckBox->isChecked();
	ui->searchInListLineEdit->setText(""); // https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
	ui->objectsListView->blockSignals(true);
	ui->objectsListView->reset();
	listModel->setStringList(objectMgr->listAllModuleObjects(moduleId, englishNames));
	proxyModel->setSourceModel(listModel);
	proxyModel->sort(0, Qt::AscendingOrder);
	ui->objectsListView->blockSignals(false);
	//bugfix: prevent multiple connections, which seems to have happened before
	connect(ui->objectsListView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(gotoObject(const QModelIndex&)), Qt::UniqueConnection);
}

void SearchDialog::updateListTab()
{
	QVariant selectedObjectId;
	QComboBox* objects = ui->objectTypeComboBox;
	int index = objects->currentIndex();
	if (index < 0)
		selectedObjectId = QVariant("AsterismMgr"); // Let's enable "Asterisms" by default!
	else
		selectedObjectId = objects->itemData(index, Qt::UserRole);

	if (StelApp::getInstance().getLocaleMgr().getAppLanguage().startsWith("en"))
		ui->searchInEnglishCheckBox->hide(); // hide "names in English" checkbox
	else
		ui->searchInEnglishCheckBox->show();
	objects->blockSignals(true);
	objects->clear();
	QMap<QString, QString> modulesMap = objectMgr->objectModulesMap();
	for (auto it = modulesMap.begin(); it != modulesMap.end(); ++it)
	{
		// List of objects is not empty, let's add name of module or section!
		if (!objectMgr->listAllModuleObjects(it.key(), ui->searchInEnglishCheckBox->isChecked()).isEmpty())
			objects->addItem(q_(it.value()), QVariant(it.key()));
	}
	index = objects->findData(selectedObjectId, Qt::UserRole, Qt::MatchCaseSensitive);
	objects->setCurrentIndex(index);
	objects->model()->sort(0, Qt::AscendingOrder);
	objects->blockSignals(false);
	updateListView(objects->currentIndex());
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
	// https://wiki.qt.io/Technical_FAQ#Why_does_the_memory_keep_increasing_when_repeatedly_pasting_text_and_calling_clear.28.29_in_a_QLineEdit.3F
	ui->lineEditSearchSkyObject->setText(""); // clear current text
	ui->lineEditSearchSkyObject->paste(); // paste text from clipboard
	gotoObject(); // go to first finded object
}
