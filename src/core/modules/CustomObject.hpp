/*
 * Copyright (C) 2016 Alexander Wolf
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

#ifndef CUSTOMOBJECT_HPP
#define CUSTOMOBJECT_HPP

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>

#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelFader.hpp"
#include "StelTranslator.hpp"

class StelPainter;

//! @class CustomObject
//! TODO GZ says PLEASE Document this thoroughly! If possible recombine CustomObject and StelMarker & derivatives, or remove one of the two. Or describe why we need both.
//!
//! The CustomObject is currently used for custom Markers which can be set and removed interactively. It is drawn exactly at the coordinates given and is not subject to aberration.
//! As discovered in #2769 it is also used to plot Simbad results, which need to be plotted with aberration.
class CustomObject : public StelObject
{
	friend class CustomObjectMgr;
public:
	static const QString CUSTOMOBJECT_TYPE;

	//!
	CustomObject(const QString& codesignation, const Vec3d& coordJ2000, const bool isaMarker);
	~CustomObject() override;

	//! Get the type of object
	QString getType(void) const override
	{
		return CUSTOMOBJECT_TYPE;
	}

	//! Get the type of object
	QString getObjectType(void) const override
	{
		return (isMarker ? N_("custom marker") : N_("custom object"));
	}
	QString getObjectTypeI18n(void) const override
	{
		return q_(getObjectType());
	}

	QString getID(void) const override
	{
		return designation;
	}

	float getSelectPriority(const StelCore* core) const override;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const override;
	Vec3f getInfoColor(void) const override;
	Vec3d getJ2000EquatorialPos(const StelCore* core) const override;
	//! Get the visual magnitude of custom object
	float getVMagnitude(const StelCore* core) const override;
	//! Get the localized name of custom object
	QString getNameI18n(void) const override;
	//! Get the english name of custom object
	QString getEnglishName(void) const override
	{
		return designation;
	}

	void update(double deltaTime);

private:
	bool initialized;

	Vec3d XYZ;                         // holds J2000 position

	StelTextureSP markerTexture;
	static Vec3f markerColor;
	static float markerSize;
	static float selectPriority;

	void draw(StelCore* core, StelPainter *painter);

	QString designation;
	bool isMarker;	

	LinearFader labelsFader;
};

#endif // CUSTOMOBJECT_HPP
