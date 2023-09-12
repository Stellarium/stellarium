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

#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTextureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "NomenclatureMgr.hpp"
#include "NomenclatureItem.hpp"

#include <QSettings>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QRegularExpression>

NomenclatureMgr::NomenclatureMgr() : StelObjectModule()
{
	setObjectName("NomenclatureMgr");
	font.setPixelSize(StelApp::getInstance().getScreenFontSize());
	connect(&StelApp::getInstance(), SIGNAL(screenFontSizeChanged(int)), this, SLOT(setFontSize(int)));
	ssystem = GETSTELMODULE(SolarSystem);
}

NomenclatureMgr::~NomenclatureMgr()
{
	StelApp::getInstance().getStelObjectMgr().unSelect();
}

double NomenclatureMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName)+10.;
	return 0;
}

void NomenclatureMgr::init()
{
	texPointer = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

	// Load the nomenclature
	NomenclatureItem::createNameLists();
	loadNomenclature();
	loadSpecialNomenclature();

	QSettings *conf = StelApp::getInstance().getSettings();
	setColor(Vec3f(                    conf->value("color/planet_nomenclature_color", "0.1,1.0,0.1").toString()));
	setFlagShowNomenclature(           conf->value("astro/flag_planets_nomenclature", false).toBool());
	setFlagShowTerminatorZoneOnly(     conf->value("astro/flag_planets_nomenclature_terminator_only", false).toBool());
	setTerminatorMinAltitude(          conf->value("astro/planet_nomenclature_solar_altitude_min", -5).toInt());
	setTerminatorMaxAltitude(          conf->value("astro/planet_nomenclature_solar_altitude_max", 40).toInt());
	setFlagOutlineCraters(             conf->value("astro/flag_planets_nomenclature_outline_craters", false).toBool());
	setFlagHideLocalNomenclature(      conf->value("astro/flag_hide_local_nomenclature", true).toBool());
	setFlagShowSpecialNomenclatureOnly(conf->value("astro/flag_special_nomenclature_only", false).toBool());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));	
	connect(ssystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateNomenclatureData()));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Planets_Nomenclature", displayGroup, N_("Nomenclature labels"), "flagShowNomenclature", "Alt+N");
	addAction("actionShow_Planets_Nomenclature_SpecialPoints_Only", displayGroup, N_("Special nomenclature points only"), "specialNomenclatureOnlyDisplayed");
}

void NomenclatureMgr::updateNomenclatureData()
{
	bool flag = getFlagShowNomenclature();
	loadNomenclature();
	loadSpecialNomenclature();
	updateI18n();
	setFlagShowNomenclature(flag);
}

void NomenclatureMgr::loadSpecialNomenclature()
{
	int featureId = 50000;
	const QList<PlanetP> ss = ssystem->getAllPlanets();
	for (const auto& p: ss)
	{
		const double size = p->getEquatorialRadius()*AU*0.25; // formal radius of point is 25% of equatorial radius
		NomenclatureItemP nomNP = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("North Pole"), "", NomenclatureItem::niSpecialPointPole, 90., 0., size));
		if (!nomNP.isNull())
			nomenclatureItems.insert(p, nomNP);
		featureId++;
		NomenclatureItemP nomSP = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("South Pole"), "", NomenclatureItem::niSpecialPointPole, -90., 0., size));
		if (!nomSP.isNull())
			nomenclatureItems.insert(p, nomSP);
		featureId++;
		// longitude is fake, used just to define the object
		NomenclatureItemP nomE = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("East"), "", NomenclatureItem::niSpecialPointEast, 0., 0., size));
		if (!nomE.isNull())
			nomenclatureItems.insert(p, nomE);
		featureId++;
		// longitude is fake, used just to define the object
		NomenclatureItemP nomW = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("West"), "", NomenclatureItem::niSpecialPointWest, 0., 180., size));
		if (!nomW.isNull())
			nomenclatureItems.insert(p, nomW);
		featureId++;
		// longitude is fake, used just to define the object
		NomenclatureItemP nomC = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("Centre"), "", NomenclatureItem::niSpecialPointCenter, 0., 180., size));
		if (!nomC.isNull())
			nomenclatureItems.insert(p, nomC);
		featureId++;
		// longitude is fake, used just to define the object
		NomenclatureItemP nomS = NomenclatureItemP(new NomenclatureItem(p, featureId, N_("Subsolar"), "", NomenclatureItem::niSpecialPointSubSolar, 0., 180., size));
		if (!nomS.isNull())
			nomenclatureItems.insert(p, nomS);
		featureId++;
	}
}

void NomenclatureMgr::loadNomenclature()
{
	qDebug() << "Loading nomenclature for Solar system bodies ...";

	nomenclatureItems.clear();	

	// regular expression to find the comments and empty lines
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

	// regular expression to find the nomenclature data
	// Rules:
	// One rule per line. Each rule contains seven elements with white space (or "tab char") as delimiter.
	// Format:
	//	planet name				: string
	//	ID of surface feature			: unique integer (feature id obtained from https://planetarynames.wr.usgs.gov/)
	//	translatable name of surface feature	: string
	//	type of surface feature			: string (2-char code)
	//	latitude of surface feature		: float (decimal degrees)
	//	longitude of surface feature		: float (decimal degrees)
	//	diameter of surface feature		: float (kilometers)
	static const QRegularExpression recRx("^\\s*(\\w+)\\s+(\\d+)\\s+_[(]\"(.*)\"[)]\\s+(\\w+)\\s+([\\-\\+\\.\\d]+)\\s+([\\-\\+\\.\\d]+)\\s+([\\-\\+\\.\\d]+)(.*)");
	static const QRegularExpression ctxRx("(.*)\",\\s*\"(.*)");

	QString surfNamesFile = StelFileMgr::findFile("data/nomenclature.dat"); // compressed version of file nomenclature.fab
	if (!surfNamesFile.isEmpty()) // OK, the file exists!
	{
		// Open file
		QFile planetSurfNamesFile(surfNamesFile);
		if (!planetSurfNamesFile.open(QIODevice::ReadOnly))
		{
			qDebug() << "Cannot open file" << QDir::toNativeSeparators(surfNamesFile);
			return;
		}
		QByteArray data = StelUtils::uncompress(planetSurfNamesFile);
		planetSurfNamesFile.close();
		//check if decompressing was successful
		if(data.isEmpty())
		{
			qDebug() << "Could not decompress file" << QDir::toNativeSeparators(surfNamesFile);
			return;
		}
		//create and open a QBuffer for reading
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);

		// keep track of how many records we processed.
		int totalRecords=0;
		int readOk=0;
		int lineNumber=0;

		PlanetP p;

		int featureId;
		QString name, planet = "", planetName = "", context = "";
		NomenclatureItem::NomenclatureItemType ntype;
		double latitude, longitude, size;
		QStringList missingPlanets;

		while (!buf.atEnd())
		{
			QString record = QString::fromUtf8(buf.readLine());
			lineNumber++;

			// Skip comments
			if (commentRx.match(record).hasMatch())
				continue;

			totalRecords++;
			QRegularExpressionMatch recMatch=recRx.match(record);
			if (!recMatch.hasMatch())
				qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in surface nomenclature file" << QDir::toNativeSeparators(surfNamesFile);
			else
			{
				// Read planet name, feature ID, context
				planet	= recMatch.captured(1).trimmed();
				featureId	= recMatch.captured(2).toInt();
				QString ctxt	= recMatch.captured(3).trimmed();
				QRegularExpressionMatch ctxMatch=ctxRx.match(ctxt);
				if (ctxMatch.hasMatch())
				{
					name = ctxMatch.captured(1).trimmed();
					context = ctxMatch.captured(2).trimmed();
				}
				else
				{
					name = ctxt;
					context = "";
				}
				// Read the type of feature
				QString ntypecode	= recMatch.captured(4).trimmed();
				ntype = NomenclatureItem::getNomenclatureItemType(ntypecode.toUpper());
				// Read lat/long/size of feature
				latitude	= recMatch.captured(5).toDouble();
				longitude	= recMatch.captured(6).toDouble();
				size		= recMatch.captured(7).toDouble();

				if (planetName.isEmpty() || planet!=planetName)
				{
					p = ssystem->searchByEnglishName(planet);
					if (p.isNull()) // is it a minor planet?
						p = ssystem->searchMinorPlanetByEnglishName(planet);
					planetName = planet;					
				}

				if (!p.isNull())
				{
					NomenclatureItemP nom = NomenclatureItemP(new NomenclatureItem(p, featureId, name, context, ntype, latitude, longitude, size));
					if (!nom.isNull())
						nomenclatureItems.insert(p, nom);

					readOk++;
				}
				else
					missingPlanets.append(planet);
			}
		}

		buf.close();
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "items of planetary surface nomenclature";

		missingPlanets.removeDuplicates();
		if (!missingPlanets.isEmpty())
			// Nothing to worry about - We still don't include all objects.
			qInfo() << "INFO: Cannot find these planetary objects to assign nomenclature items:" << missingPlanets.join(", ");
	}
}

void NomenclatureMgr::deinit()
{
	nomenclatureItems.clear();
	texPointer.clear();
}

void NomenclatureMgr::draw(StelCore* core)
{
	StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter painter(prj);
	painter.setBlending(true);

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	    drawPointer(core, painter);

	if (NomenclatureItem::labelsFader.getInterstate()<=0.f)
	    return;

	painter.setFont(font);
	const SphericalCap& viewportRegion = painter.getProjector()->getBoundingCap();

	for (const auto& p : nomenclatureItems.uniqueKeys())
	{
		// Early exit if the planet is not visible or too small to render the labels.
		const Vec3d equPos = p->getJ2000EquatorialPos(core);
		const double r = p->getEquatorialRadius() * static_cast<double>(p->getSphereScale());
		double angularSize = atan2(r, equPos.norm());
		double screenSize = angularSize * static_cast<double>(painter.getProjector()->getPixelPerRadAtCenter());
		if (screenSize < 50)
			continue;
		Vec3d n = equPos; n.normalize();
		SphericalCap boundingCap(n, cos(angularSize));
		if (!viewportRegion.intersects(boundingCap))
			continue;
		if (p->getVMagnitude(core) >= 20.f)
			continue;

		// Render all the items of this planet.
		for (auto i = nomenclatureItems.find(p); i != nomenclatureItems.end() && i.key() == p; ++i)
		{
			const NomenclatureItemP& nItem = i.value();
			if (nItem)
				nItem->draw(core, &painter);
		}
	}
}

void NomenclatureMgr::drawPointer(StelCore* core, StelPainter& painter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("NomenclatureItem");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(core);

		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!painter.getProjector()->project(pos, screenpos))
			return;

		painter.setColor(obj->getInfoColor());
		texPointer->bind();
		painter.drawSprite2dMode(static_cast<float>(screenpos[0]), static_cast<float>(screenpos[1]), 13.f, static_cast<float>(StelApp::getInstance().getTotalRunTime()*40.));
	}
}

QList<StelObjectP> NomenclatureMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;

	const bool withAberration=core->getUseAberration();
	Vec3d v(av);
	v.normalize();
	if (withAberration)
	{
	    Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
	    StelCore::matVsop87ToJ2000.transfo(vel);
	    vel*=core->getAberrationFactor()*(AU/(86400.0*SPEED_OF_LIGHT));
	    v+=vel;
	    v.normalize();
	}

	const double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& nItem : nomenclatureItems)
	{
		equPos = nItem->getJ2000EquatorialPos(core);
		equPos.normalize();
		if (equPos.dot(v) >= cosLimFov)
		{
			result.append(qSharedPointerCast<StelObject>(nItem));
		}
	}
	return result;
}

StelObjectP NomenclatureMgr::searchByName(const QString& englishName) const
{
	if (getFlagShowNomenclature())
	{
		NomenclatureItem::NomenclatureItemType niType;
		for (const auto& nItem : nomenclatureItems)
		{
			niType = nItem->getNomenclatureType();
			if (niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest && nItem->getEnglishName().toUpper() == englishName.toUpper())
			{
				return qSharedPointerCast<StelObject>(nItem);
			}
		}
	}
	return Q_NULLPTR;
}

StelObjectP NomenclatureMgr::searchByNameI18n(const QString& nameI18n) const
{
	if (getFlagShowNomenclature())
	{
		NomenclatureItem::NomenclatureItemType niType;
		for (const auto& nItem : nomenclatureItems)
		{
			niType = nItem->getNomenclatureType();
			if (niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest  && nItem->getNameI18n().toUpper() == nameI18n.toUpper())
			{
				return qSharedPointerCast<StelObject>(nItem);
			}
		}
	}
	return Q_NULLPTR;
}

QStringList NomenclatureMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;

	if (getFlagShowNomenclature())
	{
		NomenclatureItem::NomenclatureItemType niType;
		if (inEnglish)
		{
			for (const auto& nItem : nomenclatureItems)
			{
				niType = nItem->getNomenclatureType();
				if (niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest)
					result << nItem->getEnglishName();
			}
		}
		else
		{
			for (const auto& nItem : nomenclatureItems)
			{
				niType = nItem->getNomenclatureType();
				if (niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest)
					result << nItem->getNameI18n();
			}
		}
	}
	return result;
}

QStringList NomenclatureMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;

	if (getFlagShowNomenclature())
	{
		int type = objType.toInt();
		switch (type)
		{
			case 0:
			{
				NomenclatureItem::NomenclatureItemType niType;
				for (const auto& nItem : nomenclatureItems)
				{
					niType = nItem->getNomenclatureType();
					if (nItem->getPlanet()->getEnglishName().contains(objType, Qt::CaseSensitive) && niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest)
					{
						if (inEnglish)
							result << nItem->getEnglishName();
						else
							result << nItem->getNameI18n();
					}
				}
				break;
			}
			default:
			{
				for (const auto& nItem : nomenclatureItems)
				{
					if (nItem->getNomenclatureType()==type)
					{
						if (inEnglish)
							result << nItem->getEnglishName();
						else
							result << nItem->getNameI18n();
					}
				}
				break;
			}
		}
		result.removeDuplicates();
	}

	return result;
}

NomenclatureItemP NomenclatureMgr::searchByEnglishName(QString nomenclatureItemEnglishName) const
{
	if (getFlagShowNomenclature())
	{
		NomenclatureItem::NomenclatureItemType niType;
		for (const auto& p : nomenclatureItems)
		{
			niType = p->getNomenclatureType();
			if (niType!=NomenclatureItem::niSatelliteFeature && niType!=NomenclatureItem::niSpecialPointPole && niType!=NomenclatureItem::niSpecialPointEast && niType!=NomenclatureItem::niSpecialPointWest && p->getEnglishName() == nomenclatureItemEnglishName)
				return p;
		}
	}

	return NomenclatureItemP();
}

void NomenclatureMgr::setColor(const Vec3f& c)
{
	NomenclatureItem::color = c;
	emit nomenclatureColorChanged(c);
}

const Vec3f& NomenclatureMgr::getColor(void) const
{
	return NomenclatureItem::color;
}

void NomenclatureMgr::setFlagShowNomenclature(bool b)
{
	if (getFlagShowNomenclature() != b)
	{
		NomenclatureItem::setFlagLabels(b);
		emit flagShowNomenclatureChanged(b);
	}
}

bool NomenclatureMgr::getFlagShowNomenclature() const
{
	return NomenclatureItem::getFlagLabels();
}

// Set flag which determines if only nomenclature along the terminator should be shown.
void NomenclatureMgr::setFlagShowTerminatorZoneOnly(bool b)
{
	if (b!=NomenclatureItem::showTerminatorZoneOnly)
	{
		NomenclatureItem::showTerminatorZoneOnly=b;
		StelApp::immediateSave("astro/flag_planets_nomenclature_terminator_only", b);
		emit flagShowTerminatorZoneOnlyChanged(b);
	}
}
// Get flag which determines if only nomenclature along the terminator should be shown.
bool NomenclatureMgr::getFlagShowTerminatorZoneOnly() const
{
	return NomenclatureItem::showTerminatorZoneOnly;
}

// Set minimum solar altitude (degrees) to draw only nomenclature along the terminator.
void NomenclatureMgr::setTerminatorMinAltitude(int deg)
{
	if (deg!=NomenclatureItem::terminatorMinAltitude)
	{
		NomenclatureItem::terminatorMinAltitude=qBound(-90, deg, 90);
		StelApp::immediateSave("astro/planet_nomenclature_solar_altitude_min", NomenclatureItem::terminatorMinAltitude);
		emit terminatorMinAltitudeChanged(NomenclatureItem::terminatorMinAltitude);
	}
}
// Get minimum solar altitude (degrees) to draw only nomenclature along the terminator.
int NomenclatureMgr::getTerminatorMinAltitude() const
{
	return NomenclatureItem::terminatorMinAltitude;
}

// Set maximum solar altitude (degrees) to draw only nomenclature along the terminator.
void NomenclatureMgr::setTerminatorMaxAltitude(int deg)
{
	if (deg!=NomenclatureItem::terminatorMaxAltitude)
	{
		NomenclatureItem::terminatorMaxAltitude=qBound(-90, deg, 90);
		StelApp::immediateSave("astro/planet_nomenclature_solar_altitude_max", NomenclatureItem::terminatorMaxAltitude);
		emit terminatorMaxAltitudeChanged(NomenclatureItem::terminatorMaxAltitude);
	}
}
// Get maximum solar altitude (degrees) to draw only nomenclature along the terminator.
int NomenclatureMgr::getTerminatorMaxAltitude() const
{
	return NomenclatureItem::terminatorMaxAltitude;
}

void NomenclatureMgr::setFlagOutlineCraters(bool b)
{
	NomenclatureItem::flagOutlineCraters = b;
	StelApp::immediateSave("astro/flag_planets_nomenclature_outline_craters", b);
	emit flagOutlineCratersChanged(b);
}

bool NomenclatureMgr::getFlagOutlineCraters() const
{
	return NomenclatureItem::flagOutlineCraters;
}

void NomenclatureMgr::setFlagHideLocalNomenclature(bool b)
{
	NomenclatureItem::hideLocalNomenclature = b;
	StelApp::immediateSave("astro/flag_hide_local_nomenclature", b);
	emit localNomenclatureHidingChanged(b);
}

bool NomenclatureMgr::getFlagHideLocalNomenclature() const
{
	return NomenclatureItem::hideLocalNomenclature;
}

void NomenclatureMgr::setFlagShowSpecialNomenclatureOnly(bool b)
{
	NomenclatureItem::showSpecialNomenclatureOnly = b;
	StelApp::immediateSave("astro/flag_special_nomenclature_only", b);
	emit specialNomenclatureOnlyDisplayingChanged(b);
}

bool NomenclatureMgr::getFlagShowSpecialNomenclatureOnly() const
{
	return NomenclatureItem::showSpecialNomenclatureOnly;
}

// Update i18 names from english names according to current sky culture translator
void NomenclatureMgr::updateI18n()
{
	NomenclatureItem::createNameLists();
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getPlanetaryFeaturesTranslator();
	const StelTranslator& transSpecial = StelApp::getInstance().getLocaleMgr().getAppStelTranslator();
	for (const auto& i : qAsConst(nomenclatureItems))
	{
		NomenclatureItem::NomenclatureItemType niType = i->getNomenclatureType();
		if (niType>=NomenclatureItem::niSpecialPointPole)
			i->translateName(transSpecial);
		else
			i->translateName(trans);
	}
}
