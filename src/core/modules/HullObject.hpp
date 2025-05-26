/*
 * Copyright (C) 2025 Georg Zotti
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

#ifndef HULLOBJECT_HPP
#define HULLOBJECT_HPP

//#include <QVariant>
#include <QString>
//#include <QStringList>
//#include <QFont>
//#include <QList>

#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "VecMath.hpp"


class StelPainter;

//! @class HullObject
//!
//! The HullObject is used for tracking otherwise object-less vertices of dark constellations.
class HullObject : public StelObject
{
//	friend class CustomObjectMgr;
public:
	static const QString HULLOBJECT_TYPE;

	HullObject(const QString& name, const Vec3d& coordJ2000);
	~HullObject() override {};

	//! Get the type of object
	QString getType(void) const override
	{
		return HULLOBJECT_TYPE;
	}

	//! Get the type of object
	QString getObjectType(void) const override
	{
		return N_("hull node object");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return name;
	}

	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	//! Get the localized name of hull object
	QString getNameI18n(void) const override;
	//! Get the english name of hull object
	QString getEnglishName(void) const override
	{
		return name;
	}
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const override {return QString();}

private:
	Vec3d XYZ;                         // holds J2000 position
	QString name;
};

//! @typedef HullObjectP
//! Intrusive pointer used to manage StelObject with smart pointers
typedef QSharedPointerNoDelete<HullObject> HullObjectP;

Q_DECLARE_METATYPE(HullObjectP)


#endif // HULLOBJECT_HPP
