/*
 * Copyright (C) 2017 Alexander Wolf
 * Copyright (C) 2017 Teresa Huertas Roldán
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

#ifndef NOMENCLATUREMGR_HPP
#define NOMENCLATUREMGR_HPP

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "NomenclatureItem.hpp"

#include <QFont>
#include <QMultiHash>

class StelPainter;
class QSettings;

typedef QSharedPointer<NomenclatureItem> NomenclatureItemP;

class NomenclatureMgr : public StelObjectModule
{
	Q_OBJECT	
	Q_PROPERTY(bool nomenclatureDisplayed
		   READ getFlagLabels
		   WRITE setFlagLabels
		   NOTIFY nomenclatureDisplayedChanged)
	Q_PROPERTY(bool localNomenclatureHided
		   READ getFlagHideLocalNomenclature
		   WRITE setFlagHideLocalNomenclature
		   NOTIFY localNomenclatureHidingChanged)
	Q_PROPERTY(Vec3f nomenclatureColor
		   READ getColor
		   WRITE setColor
		   NOTIFY nomenclatureColorChanged)

public:
	NomenclatureMgr();
	virtual ~NomenclatureMgr() Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual void deinit() Q_DECL_OVERRIDE;
	virtual void update(double)  Q_DECL_OVERRIDE{;}
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual void drawPointer(StelCore* core, StelPainter& painter);
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nomenclatures.
	//! @param limitFov the field of view around the position v in which to search for nomenclatures.
	//! @param core the StelCore to use for computations.
	//! @return a list containing the NomenclatureItems located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const Q_DECL_OVERRIDE;

	//! Return the matching satellite object's pointer if exists or Q_NULLPTR.
	//! @param nameI18n The case in-sensitive localized NomenclatureItem name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const Q_DECL_OVERRIDE;

	//! Return the matching satellite if exists or Q_NULLPTR.
	//! @param name The case in-sensitive english NomenclatureItem name
	virtual StelObjectP searchByName(const QString& name) const Q_DECL_OVERRIDE;

	virtual StelObjectP searchByID(const QString &id) const Q_DECL_OVERRIDE { return qSharedPointerCast<StelObject>(searchByEnglishName(id)); }

	virtual QStringList listAllObjects(bool inEnglish) const Q_DECL_OVERRIDE;
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const Q_DECL_OVERRIDE;
	virtual QString getName() const Q_DECL_OVERRIDE { return "Geological features"; }
	virtual QString getStelObjectType() const Q_DECL_OVERRIDE { return NomenclatureItem::NOMENCLATURE_TYPE; }

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Other public methods
	//! Get a pointer to a nomenclature item.
	//! @param nomenclatureItemEnglishName the English name of the desired object.
	//! @return The matching nomenclature item pointer if exists or Q_NULLPTR.
	NomenclatureItemP searchByEnglishName(QString nomenclatureItemEnglishName) const;
	//! Set the color used to draw nomenclature items.
	//! @param c The color of the nomenclature items (R,G,B)
	//! @code
	//! // example of usage in scripts
	//! NomenclatureMgr.setColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColor(const Vec3f& c);
	//! Get the current color used to draw nomenclature items.
	//! @return current color
	const Vec3f& getColor(void) const;

	//! Set flag which determines if nomenclature labels are drawn or hidden.
	void setFlagLabels(bool b);
	//! Get the current value of the flag which determines if nomenclature labels are drawn or hidden.
	bool getFlagLabels() const;

	//! Set flag which determines if nomenclature labels are drawn or hidden on the celestial body of observer.
	void setFlagHideLocalNomenclature(bool b);
	//! Get the current value of the flag which determines if nomenclature labels are drawn or hidden on the celestial body of observer.
	bool getFlagHideLocalNomenclature() const;

	//! Translate nomenclature names.
	void updateI18n();

	void updateNomenclatureData();

signals:
	void nomenclatureDisplayedChanged(bool b);
	void localNomenclatureHidingChanged(bool b);
	void nomenclatureColorChanged(const Vec3f & color) const;

private slots:
	//! Connect from StelApp to reflect font size change.
	void setFontSize(int size){font.setPixelSize(size);}

private:
	SolarSystem* ssystem;

	//! Load nomenclature for solar system bodies
	void loadNomenclature();

	// Font used for displaying our text
	QFont font;
	QSettings* conf;
	StelTextureSP texPointer;	
	QMultiHash<PlanetP, NomenclatureItemP> nomenclatureItems;
};

#endif /* NOMENCLATUREMGR_HPP */
