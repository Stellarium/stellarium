/*
 * Copyright (C) 2013 Felix Zeltner
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

#include "FlightMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "SolarSystem.hpp"
#include "StelModuleMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelObjectMgr.hpp"
#include "Planes.hpp"
#include "StelUtils.hpp"

FlightMgr::FlightMgr(QObject *parent) :
	QObject(parent), displayBrightness(0), lastSelectedObject(NULL)
{
	pathDrawMode = Flight::SelectedOnly;
	labelsVisible = true;
	earth = GETSTELMODULE(SolarSystem)->getEarth();
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur5.png");
	Flight::updateObserverPos(StelApp::getInstance().getCore()->getCurrentLocation());
	this->connect(GETSTELMODULE(StelObjectMgr), SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), SLOT(updateSelectedObject()));
	dataSource = NULL;
}

FlightMgr::~FlightMgr()
{
	texPointer.clear();
}

void FlightMgr::draw(StelCore *core)
{
	// Don't render anything if we aren't on earth or before the year 2000
	if (core->getCurrentLocation().planetName != earth->getEnglishName() ||
			core->getJD() < 2451545.0)
	{
		return;
	}
	StelProjectorP projection = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(projection);
	//painter.setColor(0, 1, 0, displayBrightness);
	painter.setColor(Flight::getFlightInfoColour()[0], Flight::getFlightInfoColour()[1], Flight::getFlightInfoColour()[2], displayBrightness);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	Planes::planeTexture->bind();
	Flight::numPaths = 0;
	Flight::numVisible = 0;
	if (dataSource)
	{
		QList<FlightP> *l = dataSource->getRelevantFlights();
		if (l)
		{
			for (int i = 0; i < l->size(); ++i)
			{
				l->at(i)->draw(core, painter, pathDrawMode, labelsVisible);
			}
		}
	}
	//qDebug() << Flight::numVisible << " visible, " << Flight::numPaths << " paths";

	// Draw selection rectangle
	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(core, painter);
	}
}

void FlightMgr::drawPointer(StelCore *core, StelPainter &painter)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);

	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject(QStringLiteral("Flight"));
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		FlightP flight = obj.staticCast<Flight>();
		Vec3d pos=obj->getJ2000EquatorialPos(core);
		Vec3d screenpos;

		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos) || !flight->isVisible())
			return;
		texPointer->bind();

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		// Size on screen
		float size = 2.f*obj->getAngularRadius(core)*M_PI_180f*prj->getPixelPerRadAtCenter();
		size += 12.f + 3.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		// size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]-size/2, 20, 90);
		painter.drawSprite2dMode(screenpos[0]-size/2, screenpos[1]+size/2, 20, 0);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]+size/2, 20, -90);
		painter.drawSprite2dMode(screenpos[0]+size/2, screenpos[1]-size/2, 20, -180);
	}
}

void FlightMgr::update(double deltaTime)
{
	double jdate = StelApp::getInstance().getCore()->getJD();
	if (dataSource)
	{
		dataSource->update(deltaTime, jdate, StelApp::getInstance().getCore()->getTimeRate());
	}
	if (StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName())
	{
		return;
	}
	if (!dataSource)
	{
		return;
	}
	QList<FlightP> *l = dataSource->getRelevantFlights();
	if (!l)
	{
		return;
	}
	for (int i = 0; i < l->size(); ++i)
	{
		(*l)[i]->update(jdate);
	}
}

QList<StelObjectP> FlightMgr::searchAround(const Vec3d &av, double limitFov, const StelCore *core) const
{
	QList<StelObjectP> results;
	if (!GETSTELMODULE(Planes)->isEnabled() || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName() || !dataSource)
	{
		return results;
	}

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	QList<FlightP> *l = dataSource->getRelevantFlights();
	if (!l)
	{
		return results;
	}
	for (int i = 0; i < l->size(); ++i)
	{
		const FlightP f = l->at(i);
		if (f->isVisible())
		{
			equPos = f->getJ2000EquatorialPos(StelApp::getInstance().getCore());
			equPos.normalize();
			if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
			{
				results.append(qSharedPointerCast<StelObject>(f));
			}
		}
	}
	return results;
}

StelObjectP FlightMgr::searchByNameI18n(const QString &nameI18n) const
{
	return searchByName(nameI18n);
}

StelObjectP FlightMgr::searchByName(const QString &name) const
{
	if (!GETSTELMODULE(Planes)->isEnabled() || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName() || !dataSource)
	{
		return NULL;
	}

	QList<FlightP> *l = dataSource->getRelevantFlights();
	if (!l)
	{
		return NULL;
	}
	for (int i = 0; i < l->size(); ++i)
	{
		const FlightP f = l->at(i);
		if (f->isVisible() && (f->getCallsign().toUpper() == name.toUpper()
							   || f->getAddress().toUpper() == name.toUpper()))
		{
			return qSharedPointerCast<StelObject>(f);
		}
	}
	return NULL;
}

QStringList FlightMgr::listMatchingObjectsI18n(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
{
	return listMatchingObjects(objPrefix, maxNbItem, useStartOfWords);
}

QStringList FlightMgr::listMatchingObjects(const QString &objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList results;
	if (!GETSTELMODULE(Planes)->isEnabled() || StelApp::getInstance().getCore()->getCurrentLocation().planetName != earth->getEnglishName() || !dataSource)
	{
		return results;
	}

	QList<FlightP> *l = dataSource->getRelevantFlights();
	if (!l)
	{
		return results;
	}
	for (int i = 0; i < l->size(); ++i)
	{
		const FlightP f = l->at(i);
		if (f->isVisible())
		{
			if (useStartOfWords && (f->getCallsign().startsWith(objPrefix, Qt::CaseInsensitive)
									|| f->getAddress().startsWith(objPrefix, Qt::CaseInsensitive)))
			{
				results.append(f->getEnglishName());
			}
			else if (!useStartOfWords && (f->getCallsign().contains(objPrefix, Qt::CaseInsensitive)
											|| f->getAddress().contains(objPrefix, Qt::CaseInsensitive)))
			{
				results.append(f->getEnglishName());
			}
		}
	}
	results.sort();
	if (results.size() > maxNbItem)
	{
		results.erase(results.begin() + maxNbItem, results.end());
	}
	return results;
}

QStringList FlightMgr::listAllObjects(bool inEnglish) const
{
	QStringList list;
	if (!dataSource)
	{
		return list;
	}
	QList<FlightP> *l = dataSource->getRelevantFlights();
	if (!l)
	{
		return list;
	}
	for (int i = 0; i < l->size(); ++i)
	{
		const FlightP f = l->at(i);
		if (inEnglish)
		{
			list.append(f->getEnglishName());
		}
		else
		{
			list.append(f->getNameI18n());
		}
	}
	return list;
}

void FlightMgr::setDataSource(FlightDataSource *source)
{
	if (dataSource != source)
	{
		if (dataSource)
		{
			dataSource->deinit();
		}
		dataSource = source;
		dataSource->init();
		dataSource->updateRelevantFlights(StelApp::getInstance().getCore()->getJD(), StelApp::getInstance().getCore()->getTimeRate());
	}
}

void FlightMgr::updateSelectedObject()
{
	qDebug() << "selectedObjectChanged()";
	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject(QStringLiteral("Flight"));
		if (!newSelected.empty())
		{
			const StelObjectP obj = newSelected[0];
			qDebug() << obj->getType();
			FlightP flight = obj.staticCast<Flight>();
			if (flight != lastSelectedObject)
			{
				if (lastSelectedObject)
				{
					lastSelectedObject->setFlightSelected(false);
				}
				flight->setFlightSelected(true);
				lastSelectedObject = flight;
			}
		}
		else
		{
			if (lastSelectedObject)
			{
				lastSelectedObject->setFlightSelected(false);
			}
			lastSelectedObject = FlightP(NULL);
		}
	}
}

void FlightMgr::setInterpEnabled(bool interp)
{
	ADSBData::useInterp = interp;
}
