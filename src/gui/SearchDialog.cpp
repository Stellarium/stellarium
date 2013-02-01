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

#include "StelObjectMgr.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>

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

SearchDialog::SearchDialog() : simbadReply(NULL)
{
	ui = new Ui_searchDialogForm;
	simbadSearcher = new SimbadSearcher(this);
	objectMgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(objectMgr);

	flagHasSelectedText = false;

	greekLetters.insert("alpha", QString(QChar(0x03B1)));
	greekLetters.insert("beta", QString(QChar(0x03B2)));
	greekLetters.insert("gamma", QString(QChar(0x03B3)));
	greekLetters.insert("delta", QString(QChar(0x03B4)));
	greekLetters.insert("epsilon", QString(QChar(0x03B5)));
    
	greekLetters.insert("zeta", QString(QChar(0x03B6)));
	greekLetters.insert("eta", QString(QChar(0x03B7)));
	greekLetters.insert("theta", QString(QChar(0x03B8)));
	greekLetters.insert("iota", QString(QChar(0x03B9)));
	greekLetters.insert("kappa", QString(QChar(0x03BA)));
	
	greekLetters.insert("lambda", QString(QChar(0x03BB)));
	greekLetters.insert("mu", QString(QChar(0x03BC)));
	greekLetters.insert("nu", QString(QChar(0x03BD)));
	greekLetters.insert("xi", QString(QChar(0x03BE)));
	greekLetters.insert("omicron", QString(QChar(0x03BF)));
	
	greekLetters.insert("pi", QString(QChar(0x03C0)));
	greekLetters.insert("rho", QString(QChar(0x03C1)));
	greekLetters.insert("sigma", QString(QChar(0x03C3))); // second lower-case sigma shouldn't affect anything
	greekLetters.insert("tau", QString(QChar(0x03C4)));
	greekLetters.insert("upsilon", QString(QChar(0x03C5)));
	
	greekLetters.insert("phi", QString(QChar(0x03C6)));
	greekLetters.insert("chi", QString(QChar(0x03C7)));
	greekLetters.insert("psi", QString(QChar(0x03C8)));
	greekLetters.insert("omega", QString(QChar(0x03C9)));

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	useSimbad = conf->value("search/flag_search_online", true).toBool();	
	simbadServerUrl = conf->value("search/simbad_server_url", DEF_SIMBAD_URL).toString();
}

SearchDialog::~SearchDialog()
{
	delete ui;
	if (simbadReply)
	{
		simbadReply->deleteLater();
		simbadReply = NULL;
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
		updateListTab();
	}
}

void SearchDialog::styleChanged()
{
	// Nothing for now
}

// Initialize the dialog widgets and connect the signals/slots
void SearchDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(textChanged(const QString&)),
		this, SLOT(onSearchTextChanged(const QString&)));
	connect(ui->pushButtonGotoSearchSkyObject, SIGNAL(clicked()), this, SLOT(gotoObject()));
	onSearchTextChanged(ui->lineEditSearchSkyObject->text());
	connect(ui->lineEditSearchSkyObject, SIGNAL(returnPressed()), this, SLOT(gotoObject()));
	connect(ui->lineEditSearchSkyObject, SIGNAL(selectionChanged()), this, SLOT(setHasSelectedFlag()));

	ui->lineEditSearchSkyObject->installEventFilter(this);
	ui->RAAngleSpinBox->setDisplayFormat(AngleSpinBox::HMSLetters);
	ui->DEAngleSpinBox->setDisplayFormat(AngleSpinBox::DMSSymbols);
	ui->DEAngleSpinBox->setPrefixType(AngleSpinBox::NormalPlus);

	connect(ui->RAAngleSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
	connect(ui->DEAngleSpinBox, SIGNAL(valueChanged()), this, SLOT(manualPositionChanged()));
    
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

	connect(ui->checkBoxUseSimbad, SIGNAL(clicked(bool)),
		this, SLOT(enableSimbadSearch(bool)));
	ui->checkBoxUseSimbad->setChecked(useSimbad);

	populateSimbadServerList();
	int idx = ui->serverListComboBox->findData(simbadServerUrl, Qt::UserRole, Qt::MatchCaseSensitive);
	if (idx==-1)
	{
		// Use University of Strasbourg as default
		idx = ui->serverListComboBox->findData(QVariant(DEF_SIMBAD_URL), Qt::UserRole, Qt::MatchCaseSensitive);
	}
	ui->serverListComboBox->setCurrentIndex(idx);
	connect(ui->serverListComboBox, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(selectSimbadServer(int)));

	// list views initialization
	connect(ui->objectTypeComboBox, SIGNAL(activated(int)), this, SLOT(updateListWidget(int)));
	connect(ui->searchInListLineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchListChanged(QString)));
	connect(ui->searchInEnglishCheckBox, SIGNAL(toggled(bool)), this, SLOT(updateListTab()));
	updateListTab();
}

void SearchDialog::setHasSelectedFlag()
{
	flagHasSelectedText = true;
}

void SearchDialog::enableSimbadSearch(bool enable)
{
	useSimbad = enable;
	
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("search/flag_search_online", useSimbad);	
}

void SearchDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);

	// Set the focus directly on the line edit
	if (ui->lineEditSearchSkyObject->isVisible())
		ui->lineEditSearchSkyObject->setFocus();
}

void SearchDialog::setSimpleStyle()
{
	ui->RAAngleSpinBox->setVisible(false);
	ui->DEAngleSpinBox->setVisible(false);
	ui->simbadStatusLabel->setVisible(false);
	ui->raDecLabel->setVisible(false);
}


void SearchDialog::manualPositionChanged()
{
	ui->completionLabel->clearValues();
	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	Vec3d pos;
	StelUtils::spheToRect(ui->RAAngleSpinBox->valueRadians(), ui->DEAngleSpinBox->valueRadians(), pos);
	mvmgr->setFlagTracking(false);
	mvmgr->moveToJ2000(pos, 0.05);
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
			simbadReply=NULL;
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
			simbadReply = simbadSearcher->lookup(simbadServerUrl, trimmedText, 3);
			onSimbadStatusChanged();
			connect(simbadReply, SIGNAL(statusChanged()), this, SLOT(onSimbadStatusChanged()));
		}

		QString greekText = substituteGreek(trimmedText);
		QStringList matches;
		if(greekText != trimmedText) {
			matches = objectMgr->listMatchingObjectsI18n(trimmedText, 3);
			matches += objectMgr->listMatchingObjectsI18n(greekText, (5 - matches.size()));
		} else {
			matches = objectMgr->listMatchingObjectsI18n(trimmedText, 5);
		}

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
		simbadReply=NULL;

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
	QString name = ui->completionLabel->getSelected();
	gotoObject(name);
}

void SearchDialog::gotoObject(const QString &nameI18n)
{
	if (nameI18n.isEmpty())
		return;

	StelMovementMgr* mvmgr = GETSTELMODULE(StelMovementMgr);
	if (simbadResults.contains(nameI18n))
	{
		close();
		Vec3d pos = simbadResults[nameI18n];
		objectMgr->unSelect();
		mvmgr->moveToJ2000(pos, mvmgr->getAutoMoveDuration());
		ui->lineEditSearchSkyObject->clear();
		ui->completionLabel->clearValues();
	}
	else if (objectMgr->findAndSelectI18n(nameI18n))
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
	QString objName = item->text();
	gotoObject(objName);
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
	if(greekLetters.contains(potentialGreekLetterName))
		return greekLetters[potentialGreekLetterName.toLower()];

	// There can be indices (e.g. "α1 Cen" instead of "α Cen A"), so strip
	// any trailing digit.
	int lastCharacterIndex = potentialGreekLetterName.length()-1;
	if(potentialGreekLetterName.at(lastCharacterIndex).isDigit())
	{
		QChar digit = potentialGreekLetterName.at(lastCharacterIndex);
		QString name = potentialGreekLetterName.left(lastCharacterIndex);
		if(greekLetters.contains(name))
			return greekLetters[name.toLower()] + digit;
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

	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("search/simbad_server_url", simbadServerUrl);
}

void SearchDialog::updateListWidget(int index)
{
	QString moduleId = ui->objectTypeComboBox->itemData(index).toString();
	ui->objectsListWidget->clear();
	bool englishNames = ui->searchInEnglishCheckBox->isChecked();
	ui->objectsListWidget->addItems(objectMgr->listAllModuleObjects(moduleId, englishNames));
	ui->objectsListWidget->sortItems(Qt::AscendingOrder);
	connect(ui->objectsListWidget, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(gotoObject(QListWidgetItem*)));
}

void SearchDialog::updateListTab()
{
	if (StelApp::getInstance().getLocaleMgr().getAppLanguage() == "en")
	{
		// hide "names in English" checkbox
		ui->searchInEnglishCheckBox->hide();
	}
	else
	{
		ui->searchInEnglishCheckBox->show();
	}
	ui->objectTypeComboBox->clear();
	QMap<QString, QString> modulesMap = objectMgr->objectModulesMap();
	for (QMap<QString, QString>::const_iterator it = modulesMap.begin(); it != modulesMap.end(); ++it)
	{
		if (!objectMgr->listAllModuleObjects(it.key(), ui->searchInEnglishCheckBox->isChecked()).isEmpty())
		{
			QString moduleName = (ui->searchInEnglishCheckBox->isChecked() ?
															it.value(): q_(it.value()));
			ui->objectTypeComboBox->addItem(moduleName, QVariant(it.key()));
		}
	}
	updateListWidget(ui->objectTypeComboBox->currentIndex());
}
