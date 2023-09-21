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
#include <QRegularExpression>

#include "ui_helpDialogGui.h"
#include "HelpDialog.hpp"

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelLogger.hpp"
#include "StelStyle.hpp"
#include "StelActionMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelJsonParser.hpp"
#include "StelTranslator.hpp"
#include "ContributorsList.hpp"

HelpDialog::HelpDialog(QObject* parent)
	: StelDialog("Help", parent)
	, message("")
	, updateState(CompleteNoUpdates)
	, networkManager(nullptr)
	, downloadReply(nullptr)
{
	ui = new Ui_helpDialogForm;	
}

HelpDialog::~HelpDialog()
{
	delete ui;
	ui = nullptr;
}

void HelpDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateHelpText();
		updateAboutText();
		updateTabBarListWidgetWidth();
	}
}

void HelpDialog::styleChanged(const QString &style)
{
	StelDialog::styleChanged(style);
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
	ui->logPathLabel->setText(QString("%1:").arg(StelLogger::getLogFileName()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateLog(int)));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshLog()));

	// Config page
	ui->configPathLabel->setText(QString("%1:").arg(StelApp::getInstance().getSettings()->fileName()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateConfig(int)));
	connect(ui->refreshConfigButton, SIGNAL(clicked()), this, SLOT(refreshConfig()));

	// Set up download manager for checker of updates
	networkManager = StelApp::getInstance().getNetworkAccessManager();
	updateState = CompleteNoUpdates;
	connect(ui->checkUpdatesButton, SIGNAL(clicked()), this, SLOT(checkUpdates()));
	connect(this, SIGNAL(checkUpdatesComplete(void)), this, SLOT(updateAboutText()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	updateTabBarListWidgetWidth();

	connect((dynamic_cast<StelGui*>(StelApp::getInstance().getGui())), &StelGui::htmlStyleChanged, this, [=](const QString &style){
		ui->helpBrowser->document()->setDefaultStyleSheet(style);
		ui->aboutBrowser->document()->setDefaultStyleSheet(style);
	});
}

void HelpDialog::setKeyButtonState(bool state)
{
	ui->editShortcutsButton->setEnabled(state);
}

void HelpDialog::checkUpdates()
{
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	// In Qt6 this is no longer available. Here we have to assume we are connected and test the reply.
	// There is QNetworkInformation in Qt6.1, but it may give wrong results under certain circumstances.
	// https://doc.qt.io/qt-6/qnetworkinformation.html
	if (networkManager->networkAccessible()==QNetworkAccessManager::Accessible)
#endif
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
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
#endif
		downloadReply = networkManager->get(request);

		updateState = HelpDialog::Updating;
	}
}

void HelpDialog::downloadComplete(QNetworkReply *reply)
{
	if (reply == nullptr)
		return;

	disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));

	if (reply->error() || reply->bytesAvailable()==0)
	{
		qWarning() << "Error: While trying to access"
			   << reply->url().toString()
			   << "the following error occurred:"
			   << reply->errorString();

		reply->deleteLater();
		downloadReply = nullptr;
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
	QStringList v = latestVersion.split(".");
	v.append("0"); // the latest number (PATCH) is always 0 for releases

	QString appVersion = StelUtils::getApplicationVersion();
	QStringList c = appVersion.split(".");
	int r = StelUtils::compareVersions(latestVersion, appVersion);
	if (r==-1 || c.count()>v.count() || c.last().contains("-"))
		message = q_("Looks like you are using the development version of Stellarium.");
	else if (r==0)
		message = q_("This is latest stable version of Stellarium.");
	else
		message = QString("%1 <a href='%2'>%3</a>").arg(q_("This version of Stellarium is outdated!"), map["html_url"].toString(), q_("Download new version."));

	reply->deleteLater();
	downloadReply = nullptr;

	emit checkUpdatesComplete();
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

void HelpDialog::updateConfig(int)
{
	if (ui->stackedWidget->currentWidget() == ui->pageConfig)
		refreshConfig();
}

void HelpDialog::refreshLog() const
{
	ui->logBrowser->setPlainText(StelLogger::getLog());
	QScrollBar *sb = ui->logBrowser->verticalScrollBar();
	sb->setValue(sb->maximum());
}

void HelpDialog::refreshConfig() const
{
	QFile config(StelApp::getInstance().getSettings()->fileName());
	if(config.open(QIODevice::ReadOnly))
	{
		ui->configBrowser->setPlainText(config.readAll());
		ui->configBrowser->setFont(QFont("Courier New"));
		config.close();
	}
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
	htmlText += "<table cellpadding='10%' width='100%'>\n";
	// Describe keys for those keys which do not have actions.
	// navigate
	htmlText += "<tr><td>" + q_("Pan view around the sky").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Arrow keys & left mouse drag").toHtmlEscaped() + "</b></td></tr>\n";
	// zoom in/out
	htmlText += "<tr><td rowspan='2'>" + q_("Zoom in/out").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + hotkeyTextWrapper("Page Up") + "/" + hotkeyTextWrapper("Page Down") + "</b></td></tr>\n";
	htmlText += "<tr><td><b>" + hotkeyTextWrapper("Ctrl+Up") + "/" + hotkeyTextWrapper("Ctrl+Down") + "</b></td></tr>\n";
	// time dragging/scrolling
	QString delimiter = "";
	#ifdef Q_OS_MACOS
	// TRANSLATORS: The char mean "and"
	delimiter = QString(" %1 ").arg(q_("&"));
	#endif
	htmlText += "<tr><td>" + q_("Time dragging").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText) + delimiter + q_("left mouse drag").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: minutes").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: hours").toHtmlEscaped() + "</td><td><b>" +
	#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
			QKeySequence(Qt::CTRL | Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: days").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL | Qt::ALT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: years").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	#else
			QKeySequence(Qt::CTRL + Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: days").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL + Qt::ALT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	htmlText += "<tr><td>" + q_("Time scrolling: years").toHtmlEscaped() + "</td><td><b>" +
			QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("mouse wheel").toHtmlEscaped() + "</b></td></tr>";
	#endif

	// select object
	htmlText += "<tr><td>" + q_("Select object").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + q_("Left click").toHtmlEscaped() + "</b></td></tr>\n";
	// clear selection
	htmlText += "<tr><td>";
	htmlText += q_("Clear selection").toHtmlEscaped() + "</td>";
	#ifdef Q_OS_MACOS
	htmlText += "<td><b>" + QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText) + delimiter + q_("& left click").toHtmlEscaped() + "</b></td></tr>\n";
	#else
	htmlText += "<td><b>" + q_("Right click").toHtmlEscaped() + "</b></td></tr>\n";
	#endif
	// add custom marker
	htmlText += "<tr><td>" + q_("Add custom marker").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("left click").toHtmlEscaped() + "</b></td></tr>\n";
	// delete one custom marker
	htmlText += "<tr><td>" + q_("Delete marker closest to mouse cursor").toHtmlEscaped() + "</td>";
	htmlText += "<td><b>" + QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText) + delimiter + q_("right click").toHtmlEscaped() + "</b></td></tr>\n";
	// delete all custom markers
	htmlText += "<tr><td>" + q_("Delete all custom markers").toHtmlEscaped() + "</td>";
	#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	htmlText += "<td><b>" + QKeySequence(Qt::SHIFT | Qt::ALT).toString(QKeySequence::NativeText) + delimiter + q_("right click").toHtmlEscaped() + "</b></td></tr>\n";
	#else
	htmlText += "<td><b>" + QKeySequence(Qt::SHIFT + Qt::ALT).toString(QKeySequence::NativeText) + delimiter + q_("right click").toHtmlEscaped() + "</b></td></tr>\n";
	#endif

	htmlText += "</table>\n<p>" +
			q_("Below are listed only the actions with assigned keys. Further actions may be available via the \"%1\" button.")
			.arg(ui->editShortcutsButton->text()).toHtmlEscaped() +
			"</p><table cellpadding='10%' width='100%'>\n";

	// Append all StelAction shortcuts.
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	typedef QPair<QString, QString> KeyDescription;
	QString keydelimiter = QString("</b> %1 <b>").arg(q_("or"));
	QVector<KeyDescription> groups;
	const QStringList groupList = actionMgr->getGroupList();
	for (const auto &group : groupList)
	{
		groups.append(KeyDescription(q_(group), group));
	}
	groups.append(KeyDescription(q_("Text User Interface (TUI)"), "TUI")); // Special case: TUI
	std::sort(groups.begin(), groups.end());
	for (const auto &group : groups)
	{
		QVector<KeyDescription> descriptions;
		const QList<StelAction *>actionList = actionMgr->getActionList(group.second);
		for (auto* action : actionList)
		{
			QString text = action->getText();
			QStringList keys = { action->getShortcut().toString(QKeySequence::NativeText), action->getAltShortcut().toString(QKeySequence::NativeText)};
			keys.removeAll(QString("")); // remove empty shortcuts
			if (keys.count()>0)
				descriptions.append(KeyDescription(text, keys.join(keydelimiter)));
		}
		std::sort(descriptions.begin(), descriptions.end());
		if (descriptions.count()>0)
		{
			htmlText += "<tr><td colspan='2'>&nbsp;</td></tr>";
			htmlText += "<tr><td colspan='2'><b><u>" + group.first.toHtmlEscaped() + ":</u></b></td></tr>\n";
			for (const auto& desc : descriptions)
			{
				htmlText += "<tr><td>" + desc.first.toHtmlEscaped() + "</td>";
				htmlText += "<td><b>" + desc.second + "</b></td></tr>\n";
			}
		}
		if (group.second=="TUI") // Special case: TUI
		{
			htmlText += "<tr><td colspan='2'>&nbsp;</td></tr>";
			htmlText += "<tr><td colspan='2'><b><u>" + group.first.toHtmlEscaped() + ":</u></b></td></tr>\n";
			htmlText += "<tr><td>" + q_("Activate TUI") + "</td>";
			htmlText += "<td><b>" + hotkeyTextWrapper("Alt+T") + "</b></td></tr>\n";
		}
	}

	htmlText += "<tr><td colspan='2'>&nbsp;</td></tr>";
	htmlText += "<tr><td colspan='2'><b><u>" + q_("Special local keys") + ":</u></b></td></tr>\n";
	htmlText += "<tr><td colspan='2'>" + q_("All these hotkeys are locally available to run when specific window or tab is opened.") + "</td></tr>";
	htmlText += "<tr><td colspan='2'><b><em>" + q_("Script console") + ":</em></b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Load script from file") + "</td>";
	htmlText += "<td><b>" + hotkeyTextWrapper("Ctrl+Shift+O") + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Save script to file") + "</td>";
	htmlText += "<td><b>" + hotkeyTextWrapper("Ctrl+Shift+S") + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Run script") + "</td>";
	htmlText += "<td><b>" + hotkeyTextWrapper("Ctrl+Return") + "</b></td></tr>\n";
	htmlText += "<tr><td colspan='2'><b><em>" + q_("Astronomical calculations") + ":</em></b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Update positions") + "</td>";
	QString shiftF10 = hotkeyTextWrapper("Shift+F10");
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Calculate ephemeris") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	// TRANSLATOR: RTS = rise, transit, set
	htmlText += "<tr><td>" + q_("Calculate RTS") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Calculate phenomena") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Calculate solar eclipses") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Calculate lunar eclipses") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";
	htmlText += "<tr><td>" + q_("Calculate transits") + "</td>";
	htmlText += "<td><b>" + shiftF10 + "</b></td></tr>\n";

	htmlText += "</table>";

	// Regexp to replace {text} with an HTML link.
	static const QRegularExpression a_rx("[{]([^{]*)[}]");

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

QString HelpDialog::hotkeyTextWrapper(const QString hotkey) const
{
	return QKeySequence(hotkey).toString(QKeySequence::NativeText);
}

void HelpDialog::updateAboutText(void) const
{
	QStringList allContributors = StelContributors::contributorsList;
	allContributors.sort();
	allContributors.removeDuplicates();

	typedef QPair<QString, int> donator;
	QVector<donator> financialContributors = {
		// Individuals
		{ "Laurence Holt", 1000 }, { "John Bellora", 570 }, { "Jeff Moe (Spacecruft)", 512 }, { "Vernon Hermsen", 324 },
		{ "Marla Pinaire", 380 }, { "Satish Mallesh", 260 }, { "Vlad Magdalin", 250  }, { "Philippe Renoux", 250 },
		{ "Fito Martin", 250 }, { "SuEllen Shepard", 250 },
		// Organizations
		{ "Astronomie-Werkstatt \"Sterne ohne Grenzen\"", 640 }, { "BairesDev", 2000 }, { "Triplebyte", 280 }
	};
	std::sort(financialContributors.begin(), financialContributors.end(), [](donator i, donator j){ return i.second > j.second; });
	QStringList bestFinancialContributors;
	for (auto fc = financialContributors.begin(); fc != financialContributors.end(); ++fc)
	{
		bestFinancialContributors << fc->first;
	}

	// Regexp to replace {text} with an HTML link.
	static const QRegularExpression a_rx("[{]([^{]*)[}]");

	// populate About tab
	QString newHtml = "<h1>" + StelUtils::getApplicationName() + "</h1>";
	newHtml += QString("<p><strong>%1 %2").arg(q_("Version"), StelUtils::getApplicationVersion());
	newHtml += QString("<br />%1 %2</strong></p>").arg(q_("Based on Qt"), QT_VERSION_STR);
	if (!message.isEmpty())
		newHtml += "<p><strong>" + message + "</strong></p>";
	// Note: this legal notice is not suitable for translation
	newHtml += QString("<h3>%1</h3>").arg(STELLARIUM_COPYRIGHT);
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
	newHtml += "<li>" + q_("Project coordinator & lead developer: %1").arg(QString("Fabien Chéreau")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Graphic/other designer: %1").arg(QString("Martín Bernardi")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Guillaume Chéreau")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Georg Zotti")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Alexander V. Wolf")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Ruslan Kabatsayev")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Worachate Boonplod")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Sky cultures researcher: %1").arg(QString("Susanne M. Hoffmann")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Continuous Integration: %1").arg(QString("Hans Lambermont")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Tester: %1").arg(QString("Khalid AlAjaji")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Former Developers").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Several people have made significant contributions, but are no longer active. Their work has made a big difference to the project:").toHtmlEscaped() + "</p><ul>";
	newHtml += "<li>" + q_("Graphic/other designer: %1").arg(QString("Johan Meuris")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Doc author/developer: %1").arg(QString("Matthew Gates")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Johannes Gajdosik")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Rob Spearman")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Bogdan Marinov")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Timothy Reaves")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Florian Schaukowitsch")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("András Mohari")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Mike Storm")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Ferdinand Majerech")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Jörg Müller")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Marcos Cardinot")).toHtmlEscaped() + "</li>";	
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Nigel Kerr")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Diego Marcos")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Contributors").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Many individuals have made contributions to the project and their work has made Stellarium better. Alphabetically sorted list of all contributors: %1.").arg(allContributors.join(", ")).toHtmlEscaped() + "</p>";
	newHtml += "<h3>" + q_("Financial support").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Many individuals and organizations are supporting the development of Stellarium by donations, and the most generous financial contributors (with donations of $250 or more) are %1.").arg(bestFinancialContributors.join(", ")).toHtmlEscaped();
	// TRANSLATORS: The text between braces is the text of an HTML link.
	newHtml += " " + q_("The full list of financial contributors you may see on our {Open Collective page}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://opencollective.com/stellarium\">\\1</a>") + "</p>";
	newHtml += "<h3>" + q_("Acknowledgment").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("If the Stellarium planetarium was helpful for your research work, the following acknowledgment would be appreciated:").toHtmlEscaped() + "</p>";
	newHtml += "<p><em>"  + q_("This research has made use of the Stellarium planetarium") + "</em></p>";
	newHtml += "<p>Zotti, G., Hoffmann, S. M., Wolf, A., Chéreau, F., & Chéreau, G. (2021). The Simulated Sky: Stellarium for Cultural Astronomy Research. Journal of Skyscape Archaeology, 6(2), 221–258. <a href='https://doi.org/10.1558/jsa.17822'>https://doi.org/10.1558/jsa.17822</a></p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	newHtml += "<p>" + q_("Or you may {download the BibTeX file of the paper} to create another citation format.").toHtmlEscaped().replace(a_rx, "<a href=\"https://stellarium.org/files/stellarium.bib\">\\1</a>") + "</p>";
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

void HelpDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);
	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();
	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;

	// stackListWidget->font() does not work properly!
	// It has a incorrect fontSize in the first loading, which produces the bug#995107.
	QFont font;
	font.setPixelSize(14);
	font.setWeight(QFont::Bold);
	QFontMetrics fontMetrics(font);
	int iconSize = ui->stackListWidget->iconSize().width();
	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		int textWidth = fontMetrics.boundingRect(ui->stackListWidget->item(row)->text()).width();
		width += iconSize > textWidth ? iconSize : textWidth; // use the wider one
		width += 24; // margin - 12px left and 12px right
	}
	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
	ui->stackListWidget->updateGeometry();
}
