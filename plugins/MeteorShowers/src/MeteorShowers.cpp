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

#include <QtMath>

#include "MeteorShowers.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelUtils.hpp"

MeteorShowers::MeteorShowers(MeteorShowersMgr* mgr)
	: m_mgr(mgr)
{
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

MeteorShowers::~MeteorShowers()
{
	m_meteorShowers.clear();
}

void MeteorShowers::update(double deltaTime)
{
	StelCore* core = StelApp::getInstance().getCore();
	for (const auto& ms : m_meteorShowers)
	{
		ms->update(core, deltaTime);
	}
}

void MeteorShowers::draw(StelCore* core)
{
	for (const auto& ms : m_meteorShowers)
	{
		ms->draw(core);
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core);
	}
}

void MeteorShowers::drawPointer(StelCore* core)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("MeteorShower");
	if (newSelected.empty())
	{
		return;
	}

	const StelObjectP obj = newSelected[0];
	Vec3d pos = obj->getJ2000EquatorialPos(core);

	// Compute 2D pos and return if outside screen
	Vec3d screenpos;
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	if (!painter.getProjector()->project(pos, screenpos))
	{
		return;
	}

	const Vec3f& c(obj->getInfoColor());
	painter.setColor(c[0],c[1],c[2]);
	m_mgr->getPointerTexture()->bind();

	painter.setBlending(true);

	float size = static_cast<float>(obj->getAngularSize(core)) * M_PI_180f * static_cast<float>(painter.getProjector()->getPixelPerRadAtCenter());
	size += 20.f + 10.f * sinf(2.f * static_cast<float>(StelApp::getInstance().getTotalRunTime()));

	painter.drawSprite2dMode(static_cast<float>(screenpos[0])-size/2, static_cast<float>(screenpos[1])-size/2, 10.f, 90);
	painter.drawSprite2dMode(static_cast<float>(screenpos[0])-size/2, static_cast<float>(screenpos[1])+size/2, 10.f, 0);
	painter.drawSprite2dMode(static_cast<float>(screenpos[0])+size/2, static_cast<float>(screenpos[1])+size/2, 10.f, -90);
	painter.drawSprite2dMode(static_cast<float>(screenpos[0])+size/2, static_cast<float>(screenpos[1])-size/2, 10.f, -180);
	painter.setColor(1, 1, 1, 0);
}

void MeteorShowers::loadMeteorShowers(const QVariantMap& map)
{
	m_meteorShowers.clear();
	for (auto msKey : map.keys())
	{
		QVariantMap msData = map.value(msKey).toMap();
		msData["showerID"] = msKey;

		MeteorShowerP ms(new MeteorShower(m_mgr, msData));
		if (ms->getStatus() != MeteorShower::INVALID)
		{
			m_meteorShowers.append(ms);
		}
	}
}

QList<MeteorShowers::SearchResult> MeteorShowers::searchEvents(QDate dateFrom, QDate dateTo) const
{
	QList<SearchResult> result;
	bool found;
	QDate date;
	MeteorShower::Activity a;
	SearchResult r;
	for (const auto& ms : m_meteorShowers)
	{
		date = dateFrom;
		while(date.operator <=(dateTo))
		{
			found = false;
			a = ms->hasConfirmedShower(date, found);
			r.type = q_("Confirmed");
			if (!found)
			{
				a = ms->hasGenericShower(date, found);
				r.type = q_("Generic");
			}

			if (found)
			{
				r.name = ms->getNameI18n();
				r.peak = a.peak;
				if (a.zhr == -1) {
					r.zhrMin = a.variable.at(0);
					r.zhrMax = a.variable.at(1);
				} else {
					r.zhrMin = a.zhr;
					r.zhrMax = a.zhr;
				}
				result.append(r);
				break;
			}
			date = date.addDays(1);
		}
	}
	return result;
}

QList<StelObjectP> MeteorShowers::searchAround(const Vec3d& av, double limitFov, const StelCore*) const
{
	QList<StelObjectP> result;
	if (!m_mgr->getEnablePlugin())
	{
		return result;
	}

	Vec3d v(av);
	v.normalize();
	double cosLimFov = qCos(limitFov * M_PI/180.);
	Vec3d equPos;
	for (const auto& ms : m_meteorShowers)
	{
		if (ms->enabled())
		{
			equPos = ms->getJ2000EquatorialPos(Q_NULLPTR);
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2] >= cosLimFov)
			{
				result.append(qSharedPointerCast<StelObject>(ms));
			}
		}
	}
	return result;
}

StelObjectP MeteorShowers::searchByName(const QString& englishName) const
{
	if (!m_mgr->getEnablePlugin())
	{
		return Q_NULLPTR;
	}

	for (const auto& ms : m_meteorShowers)
	{
		bool sameEngName = ms->getEnglishName().toUpper() == englishName.toUpper();
		bool desigIsEngName = ms->getDesignation().toUpper() == englishName.toUpper();
		bool emptyDesig = ms->getDesignation().isEmpty();
		if (sameEngName || (desigIsEngName && !emptyDesig))
		{
			return qSharedPointerCast<StelObject>(ms);
		}
	}
	return Q_NULLPTR;
}

StelObjectP MeteorShowers::searchByID(const QString &id) const
{
	for (const auto& ms : m_meteorShowers)
	{
		if (ms->getID() == id)
			return qSharedPointerCast<StelObject>(ms);
	}
	return Q_NULLPTR;
}

StelObjectP MeteorShowers::searchByNameI18n(const QString& nameI18n) const
{
	if (!m_mgr->getEnablePlugin())
	{
		return Q_NULLPTR;
	}

	for (const auto& ms : m_meteorShowers)
	{
		if (ms->getNameI18n().toUpper() == nameI18n.toUpper())
		{
			return qSharedPointerCast<StelObject>(ms);
		}
	}
	return Q_NULLPTR;
}

QStringList MeteorShowers::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (!m_mgr->getEnablePlugin())
		return result;

	if (inEnglish)
	{
		for (const auto& ms : m_meteorShowers)
		{
			result.append(ms->getEnglishName());
		}
	}
	else
	{
		for (const auto& ms : m_meteorShowers)
		{
			result.append(ms->getNameI18n());
		}
	}
	return result;
}
