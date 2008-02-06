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

#include "StelHelpBrowser.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include <QString>
#include <QTextBrowser>
#include <QWidget>
#include <QSettings>
#include <QResizeEvent>
#include <QSize>
#include <QDebug>
#include <QMultiMap>
#include <QMap>
#include <QMapIterator>
#include <QPair>

StelHelpBrowser::StelHelpBrowser(QWidget* parent)
	: QTextBrowser(parent)
{
	this->setReadOnly(true);
	this->setOpenExternalLinks(true);
	this->setOpenLinks(true);

	// TODO: internationalise the non tag strings here
	headerText = "<html><head><title>Stellarium Help</title></head><body>\n";
	headerText += "<a name=\"top\"><h2>Keys</h2></a>\n";

	footerText = "<h3>Further Reading</h3>\n";
	footerText += "The following links are external web links, and will launch your web browser:\n";
	footerText += "<ul>\n";
	footerText += "<li>The Stellarium User Guide: <a href=\"http://downloads.sourceforge.net/stellarium/stellarium_user_guide-0.9.1-1.pdf\">PDF</a> | <a href=\"http://porpoisehead.net/mysw/stellarium_user_guide_html-0.9.0-1/\">HTML</a></li>";
	footerText += "<li><a href=\"http://www.stellarium.org/wiki/index.php/FAQ\">Frequently Asked Questions</a> about Stellarium.  Answers too.</li>\n";
	footerText += "<li><a href=\"http://stellarium.org/wiki/\">The Stellarium Wiki</a> - General information.  You can also find user-contributed landscapes and scripts here.</li>\n";
	footerText += "<li><a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454374\">Support ticket system</a> - if you need help using Stellarium, post a support request here and we'll try to help.</li>\n";
	footerText += "<li><a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454373\">Bug reporting system</a> - if something doesn't work properly and is not listed in the FAQ list, you can open bug reports here.</li>\n";
	footerText += "<li><a href=\"http://sourceforge.net/tracker/?group_id=48857&atid=454376\">Feature request system</a> - if you have an idea for a new feature, send it to us. We can't promise to implement every idea, but we appreciate the feedback and review the list when we are planning future features.</li>\n";
	footerText += "<li><a href=\"http://sourceforge.net/forum/forum.php?forum_id=278769\">Forums</a> - discuss Stellarium with other users.</li>\n";
	footerText += "</ul></body></html>\n";

	updateText();
}

StelHelpBrowser::~StelHelpBrowser()
{

}

void StelHelpBrowser::setKey(QString group, QString oldKey, QString newKey, QString description)
{
	// For adding keys like this, the choice of a QMultiMap seems like
	// madness.  However, when we update the text it does the grouping
	// for us... we have to live with ugliness in one of these functions
	// and it seems easier here.

	// For new key bindings we just insert and return
	if (oldKey.isEmpty())
	{
		keyData.insert(group, QPair<QString, QString>(newKey, description));
		this->updateText();
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
	this->updateText();
}

void StelHelpBrowser::updateText(void)
{
	QString newHtml(headerText);

	QMapIterator<QString, QPair<QString, QString> > i(keyData);
	QString lastGroup;
	bool firstGroup = true;
	newHtml += "<table cellpadding=\"10%\">\n";
	while(i.hasNext())
	{
		i.next();
		QString thisGroup(i.key());
		if (thisGroup.isEmpty())
			thisGroup = "Misc";

		if (thisGroup != lastGroup)
		{
			if (firstGroup)
				firstGroup = false;

			newHtml += "<tr></tr><tr><td><b><u>" + thisGroup + ":</u></b></td></tr>\n";
		}

		newHtml += "<tr><td><b>" + i.value().first + "</b></td>";
		newHtml += "<td>" + i.value().second + "</td></tr>\n";
		lastGroup = thisGroup;
	}

	newHtml += "</table>";
	newHtml += footerText;
	
	this->clear();
	this->insertHtml(newHtml);
	this->scrollToAnchor("top");
}

