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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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
#include "StelMainGraphicsView.hpp"

#include "StelAppGraphicsItem.hpp"


HelpDialog::HelpDialog()
{
	ui = new Ui_helpDialogForm;

	// Make some key and mouse bindings translatable. Keys starting with
	// "!NUMBER-" are made up; the number is there to keep the entries
	// sorted (at least relative to each other).
	specialKeys["Space"] = N_("Space");
	specialKeys["!01-arrows-and-left-drag"] = N_("Arrow keys & left mouse drag");
	specialKeys["!02-page-up/down"] = N_("Page Up/Down");
	specialKeys["!03-ctrl+up/down"] = N_("CTRL + Up/Down");
	specialKeys["!04-left-click"] = N_("Left click");
	specialKeys["!05-right-click"] = N_("Right click");
	specialKeys["!06-ctrl+left-click"] = N_("CTRL + Left click");

	// Add keys for those keys which do not have actions.
	QString group = N_("Movement and Selection");
	setKey(group, "", "!01-arrows-and-left-drag", N_("Pan view around the sky"));
	setKey(group, "", "!02-page-up/down", N_("Zoom in/out"));
	setKey(group, "", "!03-ctrl+up/down", N_("Zoom in/out"));
	setKey(group, "", "!04-left-click", N_("Select object"));
	setKey(group, "", "!05-right-click", N_("Clear selection"));
#ifdef MACOSX
	setKey(group, "", "!06-ctrl+left-click", N_("Clear selection"));
#endif
}

HelpDialog::~HelpDialog()
{
	delete ui;
	ui = NULL;
}

void HelpDialog::languageChanged()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void HelpDialog::createDialogContent()
{
		ui->setupUi(dialog);
		connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
		
		updateText();
}

void HelpDialog::setKey(QString group, QString oldKey, QString newKey, QString description)
{
	// For adding keys like this, the choice of a QMultiMap seems like
	// madness.  However, when we update the text it does the grouping
	// for us... we have to live with ugliness in one of these functions
	// and it seems easier here.

	// For new key bindings we just insert and return
	if (oldKey.isEmpty())
	{
		keyData.insert(group, QPair<QString, QString>(newKey, description));
		// if (ui->helpBrowser!=NULL)
		// 	this->updateText();
		return;
	}
	
	// Else delete the old entry if we can find it, and then insert the
	// new entry.  Here's where the multimap makes us wince...
	QMultiMap<QString, QPair<QString, QString> >::iterator i = keyData.begin();
	while (i != keyData.end()) {
		QMultiMap<QString, QPair<QString, QString> >::iterator prev = i;
		++i;
		if (prev.value().first == oldKey)
		{
			keyData.erase(prev);
		}
	}

	keyData.insert(group, QPair<QString, QString>(newKey, description));
	// if (ui->helpBrowser!=NULL)
	// 	this->updateText();
}

QString HelpDialog::getHeaderText(void)
{
	return "<html><head><title>" + Qt::escape(q_("Stellarium Help")) + "</title></head><body>\n"
		+ "<a name=\"top\"><h2>" + Qt::escape(q_("Keys")) + "</h2></a>\n";
}

QString HelpDialog::getFooterText(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString footer = "<h2>" + Qt::escape(q_("Further Reading")) + "</h2>\n";
	footer += Qt::escape(q_("The following links are external web links, and will launch your web browser:\n"));
	footer += "<p>" + Qt::escape(q_("The Stellarium User Guide:")) + " <a href=\"http://downloads.sourceforge.net/stellarium/stellarium_user_guide-0.9.1-1.pdf\">PDF</a> | <a href=\"http://porpoisehead.net/mysw/stellarium_user_guide_html-0.9.0-1/\">HTML</a></p>";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{Frequently Asked Questions} about Stellarium.  Answers too.")).replace(a_rx, "<a href=\"http://www.stellarium.org/wiki/index.php/FAQ\">\\1</a>");
	footer += "</p>\n";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{The Stellarium Wiki} - General information.  You can also find user-contributed landscapes and scripts here.")).replace(a_rx, "<a href=\"http://stellarium.org/wiki/\">\\1</a>");
	footer += "</p>\n";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{Support ticket system} - if you need help using Stellarium, post a support request here and we'll try to help.")).replace(a_rx, "<a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454374\">\\1</a>");
	footer += "</p>\n";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{Bug reporting system} - if something doesn't work properly and is not listed in the FAQ list, you can open bug reports here.")).replace(a_rx, "<a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454373\">\\1</a>");
	footer += "</p>\n";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{Feature request system} - if you have an idea for a new feature, send it to us. We can't promise to implement every idea, but we appreciate the feedback and review the list when we are planning future features.")).replace(a_rx, "<a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454376\">\\1</a>");
	footer += "</p>\n";

	footer += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	footer += Qt::escape(q_("{Forums} - discuss Stellarium with other users.")).replace(a_rx, "<a href=\"http://sourceforge.net/forum/forum.php?forum_id=278769\">\\1</a>");
	footer += "</p>\n";

	footer += "</body></html>\n";

	return footer;
}

void HelpDialog::updateText(void)
{
	// Here's how we will build the help text for the keys:
	// 1.  Get a unique list of groups by asking for the keys and then converting the
	//     resulting QList into a QSet.  
	// 2   Converet back to a QList, sort and move the empty string to the end of the
	//     list if it is present (this is the miscellaneous group).
	// 3   Iterate over the QSet of groups names doing:
	// 3.1  add the group title
	// 3.2  Use QMultiMap::values(key) to get a list of QPair<QString, QString> 
	//      which describe the key binding (QPair::first) and the help text for
	//      that key binding (QPair::second).
	// 3.3  Sort this list by the first value in the pait, courtesy of qSort and
	//      HelpDialog::helpItemSort
	// 3.4  Iterate over the sorted list adding key and description for each item
	
	QString newHtml(getHeaderText());
	newHtml += "<table cellpadding=\"10%\">\n";

	QList<QString> groups = keyData.keys().toSet().toList(); // 1 + 2
	qSort(groups.begin(), groups.end(), HelpDialog::helpGroupSort);
	
	// 3
	QString lastGroup;  // to group "" and "Miscellaneous into one
	foreach (QString group, groups)
	{
		QString groupDescription = group;
		if (group.isEmpty())
			groupDescription = N_("Miscellaneous");

		if (lastGroup!=groupDescription)
		{
			// 3.1
			newHtml += "<tr></tr><tr><td><b><u>" + Qt::escape(q_(groupDescription)) + ":</u></b></td></tr>\n";
		}
		lastGroup = groupDescription;

		// 3.2
		QList< QPair<QString, QString> > keys = keyData.values(group);

		// 3.3
		qSort(keys.begin(), keys.end(), HelpDialog::helpItemSort);

		// 3.4
		for(int i=0; i<keys.size(); i++)
		{
			QString key = keys.at(i).first; // the string which holds the key, e.g. "F1"

			// For some keys we need to translate from th QT string to something
			// more readable
			QString specKey = specialKeys[key];
			if (!specKey.isEmpty())
				key = q_(specKey);

			// Finally, add HTML table data for the key as it's help text
			newHtml += "<tr><td><b>" + Qt::escape(key) + "</b></td>";
			newHtml += "<td>" + Qt::escape(q_( keys.at(i).second)) + "</td></tr>\n";
		}
	}

	newHtml += "</table>";
	newHtml += getFooterText();
	ui->helpBrowser->clear();
	ui->helpBrowser->insertHtml(newHtml);
	ui->helpBrowser->scrollToAnchor("top");
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
