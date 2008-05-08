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
#include <QSettings>
#include <QResizeEvent>
#include <QSize>
#include <QDebug>
#include <QMultiMap>
#include <QMap>
#include <QMapIterator>
#include <QPair>
#include <QFrame>

#include "ui_helpDialogGui.h"

#include "HelpDialog.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelMainWindow.hpp"




HelpDialog::HelpDialog() 
	: dialog(0)
{
	ui = new Ui_helpDialogForm;

	specialKeys["Space"] = N_("Space");
	specialKeys["@arrows-and-left-drag"] = N_("Arrow keys & left mouse drag");
	specialKeys["@page-up/down"] = N_("Page Up/Down");
	specialKeys["@ctrl+up/down"] = N_("CTRL + Up Down");
	specialKeys["@left-click"] = N_("Left click");
	specialKeys["@right-click"] = N_("Right click");
	specialKeys["@ctrl+left-click"] = N_("CTRL + Left click");

	// Add keys for those keys which do not have actions.
	QString group = N_("Movement and selection");
	setKey(group, "", "@arrows-and-left-drag", N_("Pan view around the sky"));
	setKey(group, "", "@page-up/down", N_("Zoom"));
	setKey(group, "", "@ctrl+up/down", N_("Zoom"));
	setKey(group, "", "@left-click", N_("Select object"));
	setKey(group, "", "@right-click", N_("Unselect"));
	setKey(group, "", "@ctrl+left-click", N_("Unselect"));
}

void HelpDialog::languageChanged()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateText();
	}
}

void HelpDialog::close()
{
        emit closed();
}

void HelpDialog::setVisible(bool v)
{
        if (v)
        {
                dialog = new DialogFrame(&StelMainWindow::getInstance());
                ui->setupUi(dialog);
		updateText();

                dialog->raise();
                dialog->move(190, 90); // TODO: put in the center of the screen
                dialog->setVisible(true);
                connect(ui->closeHelp, SIGNAL(clicked()), this, SLOT(close()));
        }
        else
        {
                dialog->deleteLater();
                dialog = 0;
        }
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

	QString footer = "<h3>" + Qt::escape(q_("Further Reading")) + "</h3>\n";
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
	QString newHtml(getHeaderText());

	QMapIterator<QString, QPair<QString, QString> > i(keyData);
	QString lastGroup;
	bool firstGroup = true;
	newHtml += "<table cellpadding=\"10%\">\n";
	while(i.hasNext())
	{
		i.next();
		QString thisGroup(i.key());
		if (thisGroup.isEmpty())
			thisGroup = N_("Misc");

		if (thisGroup != lastGroup)
		{
			if (firstGroup)
				firstGroup = false;

			newHtml += "<tr></tr><tr><td><b><u>" + Qt::escape(q_(thisGroup)) + ":</u></b></td></tr>\n";
		}

		// Translate special keys
		QString key = i.value().first;
		QString spec_key = specialKeys[key];
		if (!spec_key.isEmpty())
			key = q_(spec_key);

		newHtml += "<tr><td><b>" + Qt::escape(key) + "</b></td>";
		newHtml += "<td>" + Qt::escape(q_(i.value().second)) + "</td></tr>\n";
		lastGroup = thisGroup;
	}

	newHtml += "</table>";
	newHtml += getFooterText();
	
	ui->helpBrowser->clear();
	ui->helpBrowser->insertHtml(newHtml);
	ui->helpBrowser->scrollToAnchor("top");
}

