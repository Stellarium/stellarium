/*
 * Stellarium Atlante
 * Module Cosmologique Atlante
 */

#include "atlante/AtlanteCosmology.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include <QDebug>

AtlanteCosmology* AtlanteCosmology::instance = nullptr;

AtlanteCosmology::AtlanteCosmology(QObject* parent)
	: QObject(parent)
	, currentMode(CosmologyMode::Standard)
{
	instance = this;
}

AtlanteCosmology::~AtlanteCosmology()
{
	if (instance == this)
		instance = nullptr;
}

AtlanteCosmology* AtlanteCosmology::getInstance()
{
	if (!instance)
	{
		instance = new AtlanteCosmology();
	}
	return instance;
}

Vec3d AtlanteCosmology::computeGeocentricPos(const Vec3d& helioPos, const Vec3d& earthHelioPos)
{
	return helioPos - earthHelioPos;
}

Vec3d AtlanteCosmology::getAtlantePos(const Planet* body)
{
	if (!body)
		return Vec3d(0., 0., 0.);

	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	if (!ssystem)
		return body->getHeliocentricEclipticPos();

	const Planet* earth = ssystem->getEarth().data();
	if (!earth)
		return body->getHeliocentricEclipticPos();

	const Vec3d earthHelioPos = earth->getHeliocentricEclipticPos();

	// Terre : origine géocentrique fixe (0,0,0)
	if (body == earth || body->getEnglishName() == "Earth")
	{
		return Vec3d(0., 0., 0.);
	}

	// Soleil : position opposée à la Terre (-earthHelioPos)
	if (body->getParent() == nullptr || body->getEnglishName() == "Sun")
	{
		return -earthHelioPos;
	}

	// Tout autre corps céleste P : r_helio(P) - r_helio(Terre)
	return computeGeocentricPos(body->getHeliocentricEclipticPos(), earthHelioPos);
}

void AtlanteCosmology::setMode(CosmologyMode mode)
{
	if (currentMode != mode)
	{
		currentMode = mode;
		qDebug() << "[AtlanteCosmology] Mode cosmologique changé vers :" 
		         << (mode == CosmologyMode::AtlanteGeocentric ? "Atlante Géocentrique" : "Standard");
		emit modeChanged(currentMode);
	}
}

void AtlanteCosmology::setModeFromInt(int modeInt)
{
	setMode(modeInt == 1 ? CosmologyMode::AtlanteGeocentric : CosmologyMode::Standard);
}

void AtlanteCosmology::setAtlanteGeocentric(bool enabled)
{
	setMode(enabled ? CosmologyMode::AtlanteGeocentric : CosmologyMode::Standard);
}
