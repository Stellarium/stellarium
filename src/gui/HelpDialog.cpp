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
#include <QVBoxLayout>
#include <QWidget>
#include <QFrame>
#include <QSettings>
#include <QResizeEvent>
#include <QSize>
#include <QMultiMap>
#include <QList>
#include <QSet>
#include <QPair>
#include <QtAlgorithms>
#include <QDebug>

#include "ui_helpDialogGui.h"
#include "HelpDialog.hpp"

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelLogger.hpp"
#include "StelShortcutGroup.hpp"
#include "StelShortcutMgr.hpp"
#include "StelStyle.hpp"

HelpDialog::HelpDialog() : keyMgr(0)
{
	ui = new Ui_helpDialogForm;
	keyMgr = StelApp::getInstance().getStelShortcutManager();
	Q_ASSERT(keyMgr);
}

HelpDialog::~HelpDialog()
{
	delete ui;
	ui = NULL;
}

void HelpDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void HelpDialog::styleChanged()
{
	if (dialog)
	{
		updateText();
	}
}

void HelpDialog::updateIconsColor()
{
	QPixmap pixmap(50, 50);
	QStringList icons;
	icons << "help" << "info" << "logs";
	bool redIcon = false;
	if (StelApp::getInstance().getVisionModeNight())
		redIcon = true;

	foreach(const QString &iconName, icons)
	{
		pixmap.load(":/graphicGui/tabicon-" + iconName +".png");
		if (redIcon)
			pixmap = StelButton::makeRed(pixmap);

		ui->stackListWidget->item(icons.indexOf(iconName))->setIcon(QIcon(pixmap));
	}
}

void HelpDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(QString)), this, SLOT(updateIconsColor()));
	ui->stackedWidget->setCurrentIndex(0);
	updateIconsColor();
	ui->stackListWidget->setCurrentRow(0);
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// Help page
	updateText();
	connect(ui->editShortcutsButton, SIGNAL(clicked()),
	        this, SLOT(showShortcutsWindow()));

	// Log page
	ui->logPathLabel->setText(QString("%1/log.txt:").arg(StelFileMgr::getUserDir()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateLog(int)));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshLog()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}


void HelpDialog::showShortcutsWindow()
{
	QAction* action =
	        keyMgr->getGuiAction("actionShow_Shortcuts_Window_Global");
	if (action)
	{
		if (action->isChecked())
			action->setChecked(false);
		action->setChecked(true);
	}
}

void HelpDialog::updateLog(int)
{
	if (ui->stackedWidget->currentWidget() == ui->pageLog)
		refreshLog();
}

void HelpDialog::refreshLog()
{
	ui->logBrowser->setPlainText(StelLogger::getLog());
}

QString HelpDialog::getHelpText(void)
{
	QString htmlText = "<html><head><title>";
	htmlText += Qt::escape(q_("Stellarium Help"));
	htmlText += "</title></head><body>\n";
	
	// WARNING! Section titles are re-used below!
	htmlText += "<p align=\"center\"><a href=\"#keys\">" +
	            Qt::escape(q_("Keys")) +
	            "</a> &bull; <a href=\"#links\">" +
	            Qt::escape(q_("Further Reading")) +
	            "</a></p>\n";
	
	htmlText += "<h2 id='keys'>" + Qt::escape(q_("Keys")) + "</h2>\n";
	htmlText += "<table cellpadding=\"10%\">\n";
	// Describe keys for those keys which do not have actions.
	// navigate
	htmlText += "<tr><td>" + Qt::escape(q_("Pan view around the sky")) + "</td>";
	htmlText += "<td><b>" + Qt::escape(q_("Arrow keys & left mouse drag")) + "</b></td></tr>\n";
	// zoom in/out
	htmlText += "<tr><td rowspan='2'>" + Qt::escape(q_("Zoom in/out")) +
	            "</td>";
	htmlText += "<td><b>" + Qt::escape(q_("Page Up/Down")) +
	            "</b></td></tr>\n";
	htmlText += "<tr><td><b>" + Qt::escape(q_("CTRL + Up/Down")) +
	            "</b></td></tr>\n";
	// select object
	htmlText += "<tr><td>" + Qt::escape(q_("Select object")) + "</td>";
	htmlText += "<td><b>" + Qt::escape(q_("Left click")) + "</b></td></tr>\n";
	// clear selection
	htmlText += "<tr>";
#ifdef Q_OS_MAC
	htmlText += "<td rowspan='2'>";
#else
	htmlText += "<td>";
#endif
	htmlText += Qt::escape(q_("Clear selection")) + "</td>";
	htmlText += "<td><b>" + Qt::escape(q_("Right click")) + "</b></td></tr>\n";
#ifdef Q_OS_MAC
	htmlText += "<tr><td><b>" + Qt::escape(q_("CTRL + Left click")) + "</b></td></tr>\n";
	//htmlText += "<td>" + Qt::escape(q_("Clear selection")) + "</td>";
#endif
	
	htmlText += "</table>\n<p>" +
	            Qt::escape(
	                q_("Below are listed only the actions with assigned keys. Further actions may be available via the \"%1\" button.")
	                .arg(ui->editShortcutsButton->text())
	                ) +
	            "</p><table cellpadding=\"10%\">\n";
	
	QList<StelShortcutGroup*> groups = keyMgr->getGroupList();
	foreach (const StelShortcutGroup* group, groups)
	{
		QString groupName= group->getText();
		if (groupName.isEmpty())
			groupName = group->getId();
		
		QList< KeyDescription > descriptions;
		QList<StelShortcut*> shortcuts = group->getActionList();
		if (shortcuts.isEmpty())
			continue;
		
		foreach (const StelShortcut* shortcut, shortcuts)
		{
			QString text = q_(shortcut->getText());
			QKeySequence primary = shortcut->getPrimaryKey();
			if (primary.isEmpty())
			{
				// TODO: Decide whether to display undefined actions.
				continue;
			}
			QString keyString = primary.toString(QKeySequence::NativeText);
			descriptions.append(KeyDescription(text, keyString));
		}
		if (descriptions.isEmpty())
			continue;
		// Sort by translated description:
		// - on one hand, pre-determined order is lost
		// - on the other, easier for the users
		qSort(descriptions);
		
		htmlText += "<tr></tr><tr><td><b><u>" + Qt::escape(q_(groupName)) +
		            ":</u></b></td></tr>\n";
		foreach (const KeyDescription& desc, descriptions)
		{
			htmlText += "<tr><td>" + Qt::escape(desc.first) + "</td>";
			htmlText += "<td><b>" + Qt::escape(desc.second) +
			            "</b></td></tr>\n";
		}
	}
	
	// edit shortcuts
//	htmlText += "<tr><td><b>" + Qt::escape(q_("F7")) + "</b></td>";
//	htmlText += "<td>" + Qt::escape(q_("Show and edit all keyboard shortcuts")) + "</td></tr>\n";
	htmlText += "</table>";

	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	// WARNING! Section titles are re-used above!
	htmlText += "<h2 id=\"links\">" + Qt::escape(q_("Further Reading")) + "</h2>\n";
	htmlText += Qt::escape(q_("The following links are external web links, and will launch your web browser:\n"));
	htmlText += "<p><a href=\"http://stellarium.org/wiki/index.php/Category:User%27s_Guide\">" + Qt::escape(q_("The Stellarium User Guide")) + "</a>";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += Qt::escape(q_("{Frequently Asked Questions} about Stellarium.  Answers too.")).replace(a_rx, "<a href=\"http://www.stellarium.org/wiki/index.php/FAQ\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += Qt::escape(q_("{The Stellarium Wiki} - General information.  You can also find user-contributed landscapes and scripts here.")).replace(a_rx, "<a href=\"http://stellarium.org/wiki/\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += Qt::escape(q_("{Support ticket system} - if you need help using Stellarium, post a support request here and we'll try to help.")).replace(a_rx, "<a href=\"http://answers.launchpad.net/stellarium/+addquestion\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += Qt::escape(q_("{Bug reporting and feature request system} - if something doesn't work properly or is missing and is not listed in the tracker, you can open bug reports here.")).replace(a_rx, "<a href=\"http://bugs.launchpad.net/stellarium/\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += Qt::escape(q_("{Forums} - discuss Stellarium with other users.")).replace(a_rx, "<a href=\"http://sourceforge.net/forum/forum.php?forum_id=278769\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "</body></html>\n";

	return htmlText;
}

void HelpDialog::updateText(void)
{
	QString newHtml = getHelpText();
	ui->helpBrowser->clear();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->helpBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->helpBrowser->insertHtml(newHtml);
	ui->helpBrowser->scrollToAnchor("top");

	// populate About tab
	newHtml = "<h1>" + StelUtils::getApplicationName() + "</h1>";
	// Note: this legal notice is not suitable for traslation
	newHtml += "<h3>Copyright &copy; 2000-2013 Stellarium Developers</h3>";
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
	newHtml += "<h3>" + Qt::escape(q_("Developers")) + "</h3><ul>";
	newHtml += "<li>" + Qt::escape(q_("Project coordinator & lead developer: %1").arg(QString("Fabien Ch%1reau").arg(QChar(0x00E9)))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Doc author/developer: %1").arg(QString("Matthew Gates"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Bogdan Marinov"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Timothy Reaves"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Guillaume Ch%1reau").arg(QChar(0x00E9)))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Georg Zotti"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Alexander Wolf"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Tester: %1").arg(QString("Barry Gerdes"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Tester: %1").arg(QString("Khalid AlAjaji"))) + "</li></ul>";
	newHtml += "<h3>" + Qt::escape(q_("Past Developers")) + "</h3>";
	newHtml += "<p>"  + Qt::escape(q_("Several people have made significant contributions, but are no longer active. Their work has made a big difference to the project:")) + "</p><ul>";
	newHtml += "<li>" + Qt::escape(q_("Graphic/other designer: %1").arg(QString("Johan Meuris"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Johannes Gajdosik"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Rob Spearman"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Andr%1s Mohari").arg(QChar(0x00E1)))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("Developer: %1").arg(QString("Mike Storm"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("OSX Developer: %1").arg(QString("Nigel Kerr"))) + "</li>";
	newHtml += "<li>" + Qt::escape(q_("OSX Developer: %1").arg(QString("Diego Marcos"))) + "</li></ul>";
	newHtml += "<p>";
	ui->aboutBrowser->clear();
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
