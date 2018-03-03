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

HelpDialog::HelpDialog(QObject* parent)
	: StelDialog("Help", parent)
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

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->helpBrowser << ui->aboutBrowser << ui->logBrowser;
	installKineticScrolling(addscroll);
#endif

	// Help page
	updateHelpText();
	connect(ui->editShortcutsButton, SIGNAL(clicked()), this, SLOT(showShortcutsWindow()));
	connect(StelApp::getInstance().getStelActionManager(), SIGNAL(shortcutsChanged()), this, SLOT(updateHelpText()));

	// About page
	updateAboutText();

	// Log page
	ui->logPathLabel->setText(QString("%1/log.txt:").arg(StelFileMgr::getUserDir()));
	connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(updateLog(int)));
	connect(ui->refreshButton, SIGNAL(clicked()), this, SLOT(refreshLog()));

	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

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

void HelpDialog::refreshLog()
{
	ui->logBrowser->setPlainText(StelLogger::getLog());
}

void HelpDialog::updateHelpText(void)
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
	foreach (QString group, actionMgr->getGroupList())
	{
		QList<KeyDescription> descriptions;
		foreach (StelAction* action, actionMgr->getActionList(group))
		{
			if (action->getShortcut().isEmpty())
				continue;
			QString text = action->getText();
			QString key =  action->getShortcut().toString(QKeySequence::NativeText);
			descriptions.append(KeyDescription(text, key));
		}
		qSort(descriptions);
		htmlText += "<tr></tr><tr><td><b><u>" + q_(group) +
			    ":</u></b></td></tr>\n";
		foreach (const KeyDescription& desc, descriptions)
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
	htmlText += "<p><a href=\"http://stellarium.sourceforge.net/wiki/index.php/Category:User%27s_Guide\">" + q_("The Stellarium User Guide").toHtmlEscaped() + "</a>";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Frequently Asked Questions} about Stellarium.  Answers too.").toHtmlEscaped().replace(a_rx, "<a href=\"http://www.stellarium.org/wiki/index.php/FAQ\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{The Stellarium Wiki} - General information.  You can also find user-contributed landscapes and scripts here.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Support ticket system} - if you need help using Stellarium, post a support request here and we'll try to help.").toHtmlEscaped().replace(a_rx, "<a href=\"http://answers.launchpad.net/stellarium/+addquestion\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Bug reporting and feature request system} - if something doesn't work properly or is missing and is not listed in the tracker, you can open bug reports here.").toHtmlEscaped().replace(a_rx, "<a href=\"http://bugs.launchpad.net/stellarium/\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	htmlText += q_("{Forums} - discuss Stellarium with other users.").toHtmlEscaped().replace(a_rx, "<a href=\"http://sourceforge.net/forum/forum.php?forum_id=278769\">\\1</a>");
	htmlText += "</p>\n";

	htmlText += "</body></html>\n";
#undef E

	ui->helpBrowser->clear();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->helpBrowser->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->helpBrowser->insertHtml(htmlText);
	ui->helpBrowser->scrollToAnchor("top");
}

void HelpDialog::updateAboutText(void)
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
		     << "Alessandro Siniscalchi";
	contributors.sort();

	// populate About tab
	QString newHtml = "<h1>" + StelUtils::getApplicationName() + "</h1>";
	// Note: this legal notice is not suitable for traslation
	newHtml += QString("<h3>Copyright &copy; %1 Stellarium Developers</h3>").arg(COPYRIGHT_YEARS);
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
	newHtml += "<li>" + q_("Graphic/other designer: %1").arg(QString("Johan Meuris")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Guillaume Ch%1reau").arg(QChar(0x00E9))).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Georg Zotti")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Alexander Wolf")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Marcos Cardinot")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Florian Schaukowitsch")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Continuous Integration: %1").arg(QString("Hans Lambermont")).toHtmlEscaped() + "</li>";	
	newHtml += "<li>" + q_("Tester: %1").arg(QString("Khalid AlAjaji")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Former Developers").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Several people have made significant contributions, but are no longer active. Their work has made a big difference to the project:").toHtmlEscaped() + "</p><ul>";	
	newHtml += "<li>" + q_("Doc author/developer: %1").arg(QString("Matthew Gates")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Johannes Gajdosik")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Rob Spearman")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Bogdan Marinov")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Timothy Reaves")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Andr%1s Mohari").arg(QChar(0x00E1))).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Mike Storm")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Ferdinand Majerech")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("Developer: %1").arg(QString("Jörg Müller")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Nigel Kerr")).toHtmlEscaped() + "</li>";
	newHtml += "<li>" + q_("OSX Developer: %1").arg(QString("Diego Marcos")).toHtmlEscaped() + "</li></ul>";
	newHtml += "<h3>" + q_("Contributors").toHtmlEscaped() + "</h3>";
	newHtml += "<p>"  + q_("Several people have made contributions to the project and their work has made Stellarium better (sorted alphabetically): %1.").arg(contributors.join(", ")).toHtmlEscaped() + "</p>";
	newHtml += "<p>";
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
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
