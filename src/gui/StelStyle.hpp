/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _STELSTYLE_HPP_
#define _STELSTYLE_HPP_

#include <QString>
#include <QByteArray>

//! @class StelStyle 
//! Holds the information related to a color style for GUI and modules of Stellarium
class StelStyle
{
public:
	//! The content of the associated Qt Style Sheet for styling widgets
	QByteArray qtStyleSheet;
	
	//! The content of the associated Html Style Sheet for styling rich text
	QByteArray htmlStyleSheet;
	
	//! The name of the config.ini section where the modules store style data
	QString confSectionName;
};

#endif // _STELSTYLE_HPP_
