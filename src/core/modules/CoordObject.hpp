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

#ifndef COORDOBJECT_HPP
#define COORDOBJECT_HPP

//#include <QVariant>
#include <QString>
//#include <QStringList>
//#include <QFont>
//#include <QList>

#include "StelObject.hpp"
#include "StelTranslator.hpp"
#include "VecMath.hpp"


//! @class CoordObject
//!
//! The CoordObject is used for tracking otherwise object-less vertices of dark constellations.
//! Coordinates are in J2000 frame only, but may undergo aberration.
class CoordObject : public StelObject
{
public:
	static const QString COORDOBJECT_TYPE;

	CoordObject(const QString& name, const Vec3d& coordJ2000);
	~CoordObject() override {};

	//! Get the type of object
	QString getType(void) const override
	{
		return COORDOBJECT_TYPE;
	}

	//! Get the type of object
	QString getObjectType(void) const override
	{
		return N_("CoordObject");
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	//! @return ID (name).
	QString getID(void) const override
	{
		return name;
	}

	//! Get the J2000 coordinates.
	//! @param core is ignored and may be nullptr.
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	//! Get the localized name of coord object. It is never shown, therefore just the name.
	QString getNameI18n(void) const override
	{
		return name;
	}
	//! Get the english name of coord object, equals getID().
	QString getEnglishName(void) const override
	{
		return name;
	}
	QString getInfoString(const StelCore *core, const InfoStringGroup& flags=StelObject::AllInfo) const override {return QString();}

private:
	Vec3d XYZ;    //!< J2000 position, normalized
	QString name; //!< This should be unique for debugging purposes, but is not guaranteed nor required.
};

//! @typedef CoordObjectP
//! Intrusive pointer used to manage StelObject with smart pointers
typedef QSharedPointer<CoordObject> CoordObjectP;

Q_DECLARE_METATYPE(CoordObjectP)

#endif // COORDOBJECT_HPP
