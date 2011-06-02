/*
 * Copyright (C) 2011 Alexander Wolf
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

#ifndef SNE_HPP_
#define SNE_HPP_

#include "StelModule.hpp"
#include <QFont>

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class SNe : public StelModule
{
public:
	SNe();
	virtual ~SNe();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
private:
	// Font used for displaying our text
	QFont font;

	//! replace the json file with the default from the compiled-in resource
	void restoreDefaultJsonFile(void);

	//! read the json file and create the meteor showers.
	void readJsonFile(void);

	//! Creates a backup of the meteors.json file called meteors.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	QString sneJsonPath;

};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SNeStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*SNE_HPP_*/
