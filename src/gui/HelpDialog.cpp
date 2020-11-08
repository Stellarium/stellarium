/*
 * Stellarium
 * Copyright (C) 2007 Matthew Gates
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

#include <QString>
#include <QTextBrowser>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QResizeEvent>
#include <QSize>
#include <QMultiMap>
#include <QList>
#include <QSet>
#include <QPair>
#include <QtAlgorithms>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSysInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ui_helpDialogGui.h"
#include "HelpDialog.hpp"

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelLogger.hpp"
#include "StelStyle.hpp"
#include "StelActionMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelJsonParser.hpp"

HelpDialog::HelpDialog(QObject* parent)
	: StelDialog("Help", parent)
	, message("")
	, updateState(CompleteNoUpdates)
	, networkManager(Q_NULLPTR)
	, downloadReply(Q_NULLPTR)
{
	ui = new Ui_helpDialogForm;	
}

HelpDialog::~HelpDialog()
{
	delete ui;
	ui = Q_NULLPTR;
}

void HelpDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateHelpText();
		updateAboutText();
	}
}

void HelpDialog::styleChanged()
{
	if (dialog)
	{
		updateHelpText();
		updateAboutText();
	}
}

void HelpDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Kinetic scrolling
	kineticScrollingList << ui->helpBrowser << ui->aboutBrowser << ui->logBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}


	// Help page
	StelMovementMgr* mmgr = GETSTELMODULE(StelMovementMgr);
	updateHelpText();
	setKeyButtonState(mmgr->getFlagEnableMoveKeys());
	connect(ui->editShortcutsButton, SIGNAL(clicked()), this, SLOT(showShortcutsWindow()));
	connect(StelApp::getInstance().getStelActionManager(), SIGNAL(shortcutsChanged()), this, SLOT(updateHelpText()));
	connect(mmgr, SIGNAL(flagEnableMoveKeysChanged(bool)), this, SLOT(setKeyButtonState(bool)));

	// About page
	updateAboutText();

	// Log page	
	ui->logPathLabel->setText(QString("%1/log.txt:").arg(StelFileMgr::getUserDir()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateLog(int)));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshLog()));

	// Set up download manager for checker of updates
	networkManager = StelApp::getInstance().getNetworkAccessManager();
	updateState = CompleteNoUpdates;
	connect(ui->checkUpdatesButton, SIGNAL(clicked()), this, SLOT(checkUpdates()));
	connect(this, SIGNAL(checkUpdatesComplete(void)), this, SLOT(updateAboutText()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}

void HelpDialog::setKeyButtonState(bool state)
{
	ui->editShortcutsButton->setEnabled(state);
}

void HelpDialog::checkUpdates()
{
	if (networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
	{
		if (updateState==HelpDialog::Updating)
		{
			qWarning() << "Already checking updates...";
			return;
		}

		QUrl API("https://api.github.com/repos/Stellarium/stellarium/releases/latest");
		connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
		QNetworkRequest request;
		request.setUrl(API);
		request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());
		request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
		downloadReply = networkManager->get(request);

		updateState = HelpDialog::Updating;
	}
}

void HelpDialog::downloadComplete(QNetworkReply *reply)
{
	if (reply == Q_NULLPTR)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "Error: While trying to access"
			   << reply->url().toString()
			   << "the following error occured:"
			   << reply->errorString();

		reply->deleteLater();
		downloadReply = Q_NULLPTR;
		updateState = HelpDialog::DownloadError;
		message = q_("Cannot check updates...");
		return;
	}

	QVariantMap map;
	try
	{
		map = StelJsonParser::parse(reply->readAll()).toMap();
	}
	catch (std::runtime_error &e)
	{
		qDebug() << "Answer format is wrong! Error: " << e.what();
		message = q_("Cannot check updates...");
		return;
	}

	updateState = HelpDialog::CompleteUpdates;

	QString latestVersion = map["name"].toString();
	latestVersion.replace("v","", Qt::CaseInsensitive);

	QString appVersion = StelUtils::getApplicationVersion();
	QStringList c = appVersion.split(".");	
	int r = StelUtils::compareVersions(latestVersion, appVersion);
	if (r==-1 || c.count()>3 || c.last().contains("-"))
		message = q_("Looks like you are using the development version of Stellarium.");
	else if (r==0)
		message = q_("This is latest stable version of Stellarium.");
	else
		message = QString("%1 <a href='%2'>%3</a>").arg(q_("This version of Stellarium is outdated!"), map["html_url"].toString(), q_("Download new version."));

	reply->deleteLater();
	downloadReply = Q_NULLPTR;

	emit(checkUpdatesComplete());
}

void HelpDialog::showShortcutsWindow()
{
	StelAction* action = StelApp::getInstance().getStelActionManager()->findAction("actionShow_Shortcuts_Window_Global");
	if (action)
		action->setChecked(true);
}

void HelpDialog::updateLog(int)
{
	if (ui->stackedWidget->currentWidget() == ui->pageLog)
		refreshLog();
}

void HelpDialog::refreshLog() const
{
	ui->logBrowser->setPlainText(StelLogger::getLog());
	QScrollBar *sb = ui->logBrowser->verticalScrollBar();
	sb->setValue(sb->maximum());
}

void HelpDialog::updateHelpText(void) const
{
	QString htmlText = "<html><head><title>";
	htmlText += q_("Stellarium Help").toHtmlEscaped();
	htmlText += "</title></head><body>\n";

	// WARNING! Section titles are re-used below!
	htmlText += "<p align=\"center\"><a href=\"#keys\">" +
		    q_("Keys").toHtmlEscaped() +
		    "</a> &bull; <a href=\"#links\">" +
		    q_("Further Reading").toHtmlEscaped() +
		    "</a></p>\n";

	htmlText += "<h2 id='keys'>" + q_("Keys").toHtmlEscaped() + "</h2>\n";
	htmlText += "<table cellpadding=\"10%\">\n";
	// Describe keys for those keys which do not have actions.
	// navigate
	htmlText += "<tr><td>" + q_("Pan view around the sky").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Arrow keys & left mouse drag").toHtmlEscaped() + "</b></td></tr>\n";
	// zoom in/out
	htmlText += "<tr><td rowspan='2'>" + q_("Zoom in/out").toHtmlEscaped() +
		    "</td>";
	htmlText += "<td><b>" + q_("Page Up/Down").toHtmlEscaped() +
		    "</b></td></tr>\n";
	htmlText += "<tr><td><b>" + q_("Ctrl+Up/Down").toHtmlEscaped() +
		    "</b></td></tr>\n";
	// time dragging/scrolling
	htmlText += "<tr><td>" + q_("Time dragging").toHtmlEscaped() + "</td><td><b>" +
			q_("Ctrl & left mouse drag").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: minutes").toHtmlEscaped() + "</td><td><b>" +
			q_("Ctrl & mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: hours").toHtmlEscaped() + "</td><td><b>" +
			q_("Ctrl+Shift & mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: days").toHtmlEscaped() + "</td><td><b>" +
			q_("Ctrl+Alt & mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: years").toHtmlEscaped() + "</td><td><b>" +
			q_("Ctrl+Alt+Shift & mouse wheel").toHtmlEscaped() + "</b></td></tr>";

	// select object
	htmlText += "<tr><td>" + q_("Select object").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Left click").toHtmlEscaped() + "</b></td></tr>\n";
	// clear selection
	htmlText += "<tr><td>";
	htmlText += q_("Clear selection").toHtmlEscaped() + "</td>";
#ifdef Q_OS_MAC
	htmlText += "<td><b>" + q_("Ctrl & left click").toHtmlEscaped() + "</b></td></tr>\n";
#else
	htmlText += "<td><b>" + q_("Right click").toHtmlEscaped() + "</b></td></tr>\n";
#endif
	// add custom marker
	htmlText += "<tr><td>" + q_("Add custom marker").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Shift & left click").toHtmlEscaped() + "</b></td></tr>\n";
	// delete one custom marker
	htmlText += "<tr><td>" + q_("Delete marker closest to mouse cursor").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Shift & right click").toHtmlEscaped() + "</b></td></tr>\n";
	// delete all custom markers
	htmlText += "<tr><td>" + q_("Delete all custom markers").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Shift & Alt & right click").toHtmlEscaped() + "</b></td></tr>\n";

	htmlText += "</table>\n<p>" +
			q_("Below are listed only the actions with assigned keys. Further actions may be available via the \"%1\" button.")
			.arg(ui->editShortcutsButton->text()).toHtmlEscaped() +
		    "</p><table cellpadding=\"10%\">\n";

	// Append all StelAction shortcuts.
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	typedef QPair<QString, QString> KeyDescription;
	for (auto group : actionMgr->getGroupList())
	{
		QList<KeyDescription> descriptions;
		for (auto* action : actionMgr->getActionList(group))
		{
			if (action->getShortcut().isEmpty())
				continue;
			QString text = action->getText();
			QString key =  action->getShortcut().toString(QKeySequence::NativeText);
			descriptions.append(KeyDescription(text, key));
		}
		std::sort(descriptions.begin(), descriptions.end());
		htmlText += "<tr></tr><tr><td><b><u>" + q_(group) +
			    ":</u></b></td></tr>\n";
		for (const auto& desc : descriptions)
		{
			htmlText += "<tr><td>" + desc.first.toHtmlEscaped() + "</td>";
			htmlText += "<td><b>" + desc.second.toHtmlEscaped() +
				    "</b></td></tr>\n";
		}
	}

	htmlText += "<tr></tr><tr><td><b><u>" + q_("Text User Interface (TUI)") +
		    ":</u></b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Activate TUI") + "</td>";
	htmlText += "<td><b>Alt+T</b></td></tr>\n";
	htmlText += "</table>";

	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	// WARNING! Section titles are re-used above!
	htmlText += "<h2 id=\"links\">" + q_("Further Reading").toHtmlEscaped() + "</h2>\n";
	htmlText += q_("The following links are external web links, and will launch your web browser:\n").toHtmlEscaped();

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Frequently Asked Questions} about Stellarium.  Answers too.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/wiki/FAQ\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{The Stellarium Wiki} - general information.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/wiki\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{The landscapes} - user-contributed landscapes for Stellarium.").toHtmlEscaped().replace(a_rx, "<a href=\"https://stellarium.org/landscapes.html\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{The scripts} - user-contributed and official scripts for Stellarium.").toHtmlEscaped().replace(a_rx, "<a href=\"https://stellarium.org/scripts.html\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Bug reporting and feature request system} - if something doesn't work properly or is missing and is not listed in the tracker, you can open bug reports here.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Google Groups} - discuss Stellarium with other users.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Open Collective} - donations to the Stellarium development team.").toHtmlEscaped().replace(a_rx, "<a href=\"https://opencollective.com/stellarium\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "</body></html>\n";

	ui->helpBrowser->clear();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
		ui->helpBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->helpBrowser->insertHtml(htmlText);
	ui->helpBrowser->scrollToAnchor("top");
}

void HelpDialog::updateAboutText(void) const
{
	QStringList contributors;
	contributors << "Vladislav Bataron" << "Barry Gerdes" << "Peter Walser" << "Michal Sojka"
		     << "Nick Fedoseev" << "Clement Sommelet" << "Ivan Marti-Vidal" << "Nicolas Martignoni"
		     << "Oscar Roig Felius" << "M.S. Adityan" << "Tomasz Buchert" << "Adam Majer"
		     << "Roland Bosa" << "Łukasz 'sil2100' Zemczak" << "Gábor Péterffy"
		     << "Mircea Lite" << "Alexey Dokuchaev" << "William Formyduval" << "Daniel De Mickey"
		     << "François Scholder" << "Anton Samoylov" << "Mykyta Sytyi" << "Shantanu Agarwal"
		     << "Teemu Nätkinniemi" << "Kutaibaa Akraa" << "J.L.Canales" << "Leonid Froenchenko"
		     << "Peter Mousley" << "Greg Alexander" << "Yuri Chornoivan" << "Daniel Michalik"
		     << "Hleb Valoshka" << "Matthias Drochner" << "Kenan Dervišević" << "Alex Gamper"
		     << "Volker Hören" << "Max Digruber" << "Dan Smale" << "Victor Reijs"
		     << "Tanmoy Saha" << "Oleg Ginzburg" << "Peter Hickey" << "Bernd Kreuss"
		     << "Alexander Miller" << "Eleni Maria Stea" << "Kirill Snezhko"
		     << "Simon Parzer" << "Peter Neubauer" << "Andrei Borza" << "Allan Johnson"
		     << "Felix Zeltner" << "Paolo Cancedda" << "Ross Mitchell" << "David Baucum"
		     << "Maciej Serylak" << "Adriano Steffler" << "Sibi Antony" << "Tony Furr"
		     << "misibacsi" << "Pavel Klimenko" << "Rumen G. Bogdanovski" << "Colin Gaudion"
		     << "Annette S. Lee" << "Vancho Stojkoski" << "Robert S. Fuller" << "Giuseppe Putzolu"
		     << "henrysky" << "Nick Kanel" << "Petr Kubánek" << "Matwey V. Kornilov"
		     << "Alessandro Siniscalchi" << "Ruslan Kabatsayev" << "Pawel Stolowski"
		     << "Antoine Jacoutot" << "Sebastian Jennen" << "Matt Hughes" << "Sun Shuwei"
		     << "Alexey Sokolov" << "Paul Krizak" << "ChrUnger" << "Minmin Gong" << "Andy Kirkham"
		     << "Michael Dickens" << "Patrick (zero0cool0)" << "Martín Bernardi" << "Sebastian Garcia"
		     << "Wolfgang Laun" << "Alexandros Kosiaris" << "Alexander Duytschaever";
	contributors.sort();

	// populate About tab
	QString newHtml = "<h1>" + StelUtils::getApplicationName() + "</h1>";
	// Note: this legal notice is not suitable for translation
	newHtml += QString("<h3>Copyright &copy; %1 Stellarium Developers</h3>").arg(COPYRIGHT_YEARS);
	if (!message.isEmpty())
		newHtml += "<p><strong>" + message + "</strong></p>";
	// newHtml += "<p><em>Version 0.15 is dedicated in memory of our team member Barry Gerdes.</em></p>";
	newHtml += "<p>This program is free software; you can redistribute it and/or ";
	newHtml += "modify it under the terms of the GNU General Public License ";
	newHtml += "as published by the Free Software Foundation; either version 2 ";
	newHtml += "of the License, or (at your option) any later version.</p>";
	newHtml += "<p>This program is distributed in the hope that it will be useful, ";
	newHtml += "but WITHOUT ANY WARRANTY; without even the implied ";
	newHtml += "warranty of MERCHANTABILITY or FITNESS FOR A ";
	newHtml += "PARTICULAR PURPOSE.  See the GNU General Public ";
	newHtml += "License for more details.</p>";
	newHtml += "<p>You should have received a copy of the GNU General Public ";
	newHtml += "License along with this program; if not, write to:</p>";
	newHtml += "<pre>Free Software Foundation, Inc.\n";
	newHtml += "51 Franklin Street, Suite 500\n";
	newHtml += "Boston, MA  02110-1335, USA.\n</pre>";
	newHtml += "<p><a href=\"http://www.fsf.org\">www.fsf.org</a></p>";
	newHtml += "<h3>" + q_("Developers").toHtmlEscaped() + "</h3><ul>";
	newHtml += "<li>" + q_("Project coordinator & lead developer: %1").arg(QString("Fabien Ch%1reau").arg(QChar(0x00E9))).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Graphic/other designer: %1").arg(QString("Martín Bernardi")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Guillaume Ch%1reau").arg(QChar(0x00E9))).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Georg Zotti")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Alexander Wolf")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Former Developers").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Several people have made significant contributions, but are no longer active. Their work has made a big difference to the project:").toHtmlEscaped() + "</p><ul>";
	newHtml += "<li>" + q_("Graphic/other designer: %1").arg(QString("Johan Meuris")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Doc author/developer: %1").arg(QString("Matthew Gates")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Johannes Gajdosik")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Rob Spearman")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Bogdan Marinov")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Timothy Reaves")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Florian Schaukowitsch")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Andr%1s Mohari").arg(QChar(0x00E1))).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Mike Storm")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Ferdinand Majerech")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Jörg Müller")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Marcos Cardinot")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Nigel Kerr")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Diego Marcos")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<li>" + q_("Continuous Integration: %1").arg(QString("Hans Lambermont")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Tester: %1").arg(QString("Khalid AlAjaji")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Contributors").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Several people have made contributions to the project and their work has made Stellarium better (sorted alphabetically): %1.").arg(contributors.join(", ")).toHtmlEscaped() + "</p>";
	newHtml += "<p>";

	ui->aboutBrowser->clear();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
		ui->aboutBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->aboutBrowser->insertHtml(newHtml);
	ui->aboutBrowser->scrollToAnchor("top");
}

void HelpDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}
