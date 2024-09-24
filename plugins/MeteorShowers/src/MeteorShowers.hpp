/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#ifndef METEORSHOWERS_HPP
#define METEORSHOWERS_HPP

#include "StelObjectModule.hpp"
#include "MeteorShower.hpp"
#include "MeteorShowersMgr.hpp"

typedef QSharedPointer<MeteorShower> MeteorShowerP;

//! @class MeteorShowers
//! This class manages a collection of MeteorShower objects.
//! It inherits from MeteorShowersMgr and is basically used to
//! reimplement the methods defined in the StelObjectModule class.
//! @author Marcos Cardinot <mcardinot@gmail.com>
//! @ingroup meteorShowers
class MeteorShowers : public StelObjectModule
{
	Q_OBJECT
public:
	//! @struct SearchResult
	struct SearchResult
	{
		QString code;
		QString name;
		QString type;
		double peak;
		int peakyear;
		int zhrMax;
		int zhrMin;
		QPair<int, QString> activityClass;
	};

	//! Constructor
	MeteorShowers(MeteorShowersMgr *mgr);

	void init(void) override {}

	//! Destructor
	~MeteorShowers() override;

	//! Update
	//! @param deltaTime the time increment in seconds since the last call.
	void update(double deltaTime) override;

	//! Draw
	void draw(StelCore* core) override;

	//! Loads all meteor showers contained in a QVariantMap.
	//! @param map
	void loadMeteorShowers(const QVariantMap& map);

	//! Find all meteor_shower events in a given date interval
	//! @param dateFrom
	//! @param dateTo
	//! @return list
	QList<SearchResult> searchEvents(int year) const;

	//
	// Methods defined in StelObjectModule class
	//
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;
	StelObjectP searchByName(const QString& name) const override;
	StelObjectP searchByID(const QString &id) const override;

	QStringList listAllObjects(bool inEnglish) const override;
	QString getName() const  override{ return "Meteor Showers"; }
	QString getStelObjectType() const override { return MeteorShower::METEORSHOWER_TYPE; }

private:
	MeteorShowersMgr* m_mgr;
	QList<MeteorShowerP> m_meteorShowers;

	//! Draw pointer
	void drawPointer(StelCore* core);
};

#endif /*METEORSHOWERS_HPP*/
