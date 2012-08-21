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
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelLogger.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

HelpDialog::HelpDialog()
{
	ui = new Ui_helpDialogForm;
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

	updateText();

	ui->logPathLabel->setText(QString("%1/log.txt:").arg(StelFileMgr::getUserDir()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateLog(int)));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshLog()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
}

void HelpDialog::updateLog(int)
{
	if(ui->stackedWidget->currentWidget() == ui->page_3)
		refreshLog();
}

void HelpDialog::refreshLog()
{
	ui->logBrowser->setPlainText(StelLogger::getLog());
}

QString HelpDialog::getHelpText(void)
{
	QString htmlText = "<html><head><title>" + Qt::escape(q_("Stellarium Help")) + "</title></head><body>\n"
			+ "<h2>" + Qt::escape(q_("Keys")) + "</h2>\n";
	// Describe keys for those keys which do not have actions.
	htmlText += "<table cellpadding=\"10%\">\n";
	// navigate
	htmlText += "<tr><td><b>" + Qt::escape(q_("Arrow keys & left mouse drag")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Pan view around the sky")) + "</td></tr>\n";
	// zoom in/out with PageUp/PageDown
	htmlText += "<tr><td><b>" + Qt::escape(q_("Page Up/Down")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Zoom in/out")) + "</td></tr>\n";
	// zoom in/out with arrows
	htmlText += "<tr><td><b>" + Qt::escape(q_("CTRL + Up/Down")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Zoom in/out")) + "</td></tr>\n";
	// select object
	htmlText += "<tr><td><b>" + Qt::escape(q_("Left click")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Select object")) + "</td></tr>\n";
	// clear selection
	htmlText += "<tr><td><b>" + Qt::escape(q_("Right click")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Clear selection")) + "</td></tr>\n";
#ifdef Q_OS_MAC
	htmlText += "<tr><td><b>" + Qt::escape(q_("CTRL + Left click")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Clear selection")) + "</td></tr>\n";
#endif
	// edit shortcuts
	htmlText += "<tr><td><b>" + Qt::escape(q_("F7")) + "</b></td>";
	htmlText += "<td>" + Qt::escape(q_("Show and edit all keyboard shortcuts")) + "</td></tr>\n";
	htmlText += "</table>";

	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	htmlText += "<h2>" + Qt::escape(q_("Further Reading")) + "</h2>\n";
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
	newHtml += "<h3>Copyright &copy; 2000-2012 Stellarium Developers</h3>";
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

bool HelpDialog::helpItemSort(const QPair<QString, QString>& p1, const QPair<QString, QString>& p2)
{
	// To be 100% proper, we should sort F1 F2 F11 F12 in that order, although
	// right now we will get F1 F11 F12 F2.  However, at time of writing, no group
	// of keys has F1-F9, and one from F10-F12 in it, so it doesn't really matter.
	// -MNG 2008-06-01
	if (p1.first.split(",").at(0).size()!=p2.first.split(",").at(0).size())
		return p1.first.size() < p2.first.size();
	else
		return p1.first < p2.first;
}

bool HelpDialog::helpGroupSort(const QString& s1, const QString& s2)
{
	QString s1c = s1.toUpper();
	QString s2c = s2.toUpper();

	if (s1c=="" || s1c==QString(N_("Miscellaneous")).toUpper())
		s1c = "ZZZ" + s1c;
	if (s2c=="" || s2c==QString(N_("Miscellaneous")).toUpper())
		s2c = "ZZZ" + s2c;
	if (s1c=="DEBUG")
		s1c = "ZZZZ" + s1c;
	if (s2c=="DEBUG")
		s2c = "ZZZZ" + s2c;

	return s1c < s2c;
}

void HelpDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
		current = previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}
