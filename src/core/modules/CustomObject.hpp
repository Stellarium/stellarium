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

class StelPainter;

class CustomObject : public StelObject
{
	friend class CustomObjectMgr;
public:
	static const QString CUSTOMOBJECT_TYPE;

	CustomObject(const QString& codesignation, const Vec3d& coordinates, const bool isVisible);
	~CustomObject();

	//! Get the type of object
	virtual QString getType(void) const
	{
		return CUSTOMOBJECT_TYPE;
	}

	virtual QString getID(void) const
	{
		return designation;
	}

	virtual float getSelectPriority(const StelCore* core) const;

	//! Get an HTML string to describe the object
	//! @param core A pointer to the core
	//! @flags a set of flags with information types to include.
	virtual QString getInfoString(const StelCore* core, const InfoStringGroup& flags) const;
	virtual Vec3f getInfoColor(void) const;
	virtual Vec3d getJ2000EquatorialPos(const StelCore*) const
	{
		return XYZ;
	}
	//! Get the visual magnitude of custom object
	virtual float getVMagnitude(const StelCore* core) const;	
	//! Get the angular size of custom object
	virtual double getAngularSize(const StelCore* core) const;
	//! Get the localized name of custom object
	virtual QString getNameI18n(void) const;
	//! Get the english name of custom object
	virtual QString getEnglishName(void) const
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
