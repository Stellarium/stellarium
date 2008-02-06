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

#ifndef STELHELPBROWSER_HPP_
#define STELHELPBROWSER_HPP_

#include <QWidget>
#include <QTextBrowser>
#include <QString>
#include <QMultiMap>
#include <QPair>

//! @class StellHelpBrowser
//! Implementation of the help window
class StelHelpBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	StelHelpBrowser(QWidget* parent=0);
	~StelHelpBrowser();

	//! Set a key and description.
	//! @param group is the help group.  e.g. "Movement" or "Time & Date"
	//! @param key is the textual representation of a key, e.g. "CTRL + H"
	//! @param description is a short description of what the key does
	void setKey(QString group, QString oldKey, QString newKey, QString description);

private:
	//! This function concatenates the header, key codes and footer to build
	//! up the help text.
	void updateText(void);

	QString headerText;

	//! This uses the group key description as the key to the map, and a 
	//! containing the helpGroup and description as the map value.
	//! code and description.
	QMultiMap<QString, QPair<QString, QString> > keyData;
	QString footerText;

};

#endif /*STELHELPBROWSER_HPP_*/

