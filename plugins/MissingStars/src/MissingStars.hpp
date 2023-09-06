/*
 * Copyright (C) 2023 Alexander Wolf
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

#ifndef MISSINGSTARS_HPP
#define MISSINGSTARS_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "MissingStar.hpp"
#include <QFont>
#include <QVariantMap>
#include <QDateTime>
#include <QList>
#include <QSharedPointer>

class QSettings;
class MissingStarsDialog;

class StelPainter;

/*! @defgroup missingStars Missing Stars Plug-in
@{
The %Missing Stars plugin displays the positions some missing stars.

<b>Missing Stars Catalog</b>

The missing stars catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "missingstars.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file.

@}
*/

//! @ingroup missingStars
typedef QSharedPointer<MissingStar> MissingStarP;

//! @class MissingStars
//! Main class of the %Missing Stars plugin.
//! @author Alexander Wolf
//! @ingroup missingStars
class MissingStars : public StelObjectModule
{
	Q_OBJECT
public:	
	MissingStars();
	virtual ~MissingStars() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for missing stars.
	//! @param limitFov the field of view around the position v in which to search for missing stars.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the missing stars located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const Q_DECL_OVERRIDE;

	//! Return the matching missing stars object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized star name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const Q_DECL_OVERRIDE;

	//! Return the matching missing star if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english star name
	virtual StelObjectP searchByName(const QString& name) const Q_DECL_OVERRIDE;

	//! Return the matching missing star if exists, or an "empty" pointer.
	//! @param id The missing star id
	virtual StelObjectP searchByID(const QString &id) const Q_DECL_OVERRIDE
	{
		return qSharedPointerCast<StelObject>(getByID(id));
	}

	virtual QStringList listAllObjects(bool inEnglish) const Q_DECL_OVERRIDE;

	virtual QString getName() const Q_DECL_OVERRIDE { return "Missing Stars"; }
	virtual QString getStelObjectType() const Q_DECL_OVERRIDE { return MissingStar::MISSINGSTAR_TYPE; }

	//! get a star object by identifier
	MissingStarP getByID(const QString& id) const;

	//! Get list of designations for missing stars
	QString getMissingStarsList() const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this
	//! plugin.
	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;

public slots:
	//! Connect this to StelApp font size.
	void setFontSize(int s){font.setPixelSize(s);}
private:
	// Font used for displaying our text
	QFont font;

	//! Read the JSON file and create list of missing stars.
	void readJsonFile(void);

	//! Parse JSON file and load missing stars to map
	QVariantMap loadMissingStarsMap();

	//! Set items for list of struct from data map
	void setMissingStarsMap(const QVariantMap& map);

	StelTextureSP texPointer;
	QList<MissingStarP> missingstars;
	QStringList designations;

	QSettings* conf;

	// GUI
	MissingStarsDialog* configDialog;	
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class MissingStarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const Q_DECL_OVERRIDE;
	virtual StelPluginInfo getPluginInfo() const Q_DECL_OVERRIDE;
	virtual QObjectList getExtensionList() const Q_DECL_OVERRIDE { return QObjectList(); }
};

#endif /* MISSINGSTARS_HPP */
