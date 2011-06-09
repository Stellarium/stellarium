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
#include <QVariantMap>
#include <QDateTime>
#include <QList>

typedef struct
{
	QString name;
	QString type;
	float maxMagnitude;
	double peakJD;
	double ra;
	double de;
} supernova;

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

	//! read the json file and create list of supernovaes.
	void readJsonFile(void);

	//! Creates a backup of the sne.json file called sne.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupJsonFile(bool deleteOriginal=false);

	//! parse JSON file and load supernovaes to map
	QVariantMap loadSNeMap(QString path=QString());

	//! set items for array of struct from data map
	void setSNeMap(const QVariantMap& map);

	//! compute visible magnitude for supernova
	//! @param peakJD is date of maximum of brightness of supernova on Julian Day
	//! @param maxMag is maximum magnitude for supernova
	//! @param sntype is type of supernova
	//! @param currentJD is current Julian Day
	//! @return current magnitude
	double computeSNeMag(double peakJD, float maxMag, QString sntype, double currentJD);

	QString sneJsonPath;

	QList<supernova> snstar;

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
