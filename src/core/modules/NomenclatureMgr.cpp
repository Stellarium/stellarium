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

NomenclatureMgr::NomenclatureMgr()
{
	setObjectName("NomenclatureMgr");
	conf = StelApp::getInstance().getSettings();
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
	loadNomenclature();

	setColor(StelUtils::strToVec3f(conf->value("color/planet_nomenclature_color", "0.1,1.0,0.1").toString()));
	setFlagLabels(conf->value("astro/flag_planets_nomenclature", false).toBool());
	setFlagHideLocalNomenclature(conf->value("astro/flag_hide_local_nomenclature", true).toBool());

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));	
	connect(ssystem, SIGNAL(solarSystemDataReloaded()), this, SLOT(updateNomenclatureData()));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Planets_Nomenclature", displayGroup, N_("Nomenclature labels"), "nomenclatureDisplayed", "Alt+N");
}

void NomenclatureMgr::updateNomenclatureData()
{
	bool flag = getFlagLabels();
	loadNomenclature();
	updateI18n();
	setFlagLabels(flag);
}

void NomenclatureMgr::loadNomenclature()
{
	qDebug() << "Loading nomenclature for Solar system bodies ...";

	nomenclatureItems.clear();	

	// regular expression to find the comments and empty lines
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

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
	QRegExp recRx("^\\s*(\\w+)\\s+(\\d+)\\s+_[(]\"(.*)\"[)]\\s+(\\w+)\\s+([\\-\\+\\.\\d]+)\\s+([\\-\\+\\.\\d]+)\\s+([\\-\\+\\.\\d]+)(.*)");
	QRegExp ctxRx("(.*)\",\\s*\"(.*)");
	QString record, ctxt;

	QStringList nTypes; // codes for types of features (details: https://planetarynames.wr.usgs.gov/DescriptorTerms)
	nTypes << "AL"	<< "AR"	<< "AS"	<< "CA"	<< "CB"	<< "CH"	<< "CM"	<< "CO"	<< "CR"	<< "AA"
	       << "DO"	<< "ER"	<< "FA"	<< "FR"	<< "FE"	<< "FL"	<< "FM"	<< "FO"	<< "FT"	<< "IN"
	       << "LA"	<< "LB"	<< "LU"	<< "LC"	<< "LF"	<< "LG"	<< "LE"	<< "LI"	<< "LN"	<< "MA"
	       << "ME"	<< "MN"	<< "MO"	<< "OC"	<< "PA"	<< "PE"	<< "PL"	<< "PM"	<< "PU"	<< "PR"
	       << "RE"	<< "RT"	<< "RI"	<< "RU"	<< "SF"	<< "SC"	<< "SE"	<< "SI"	<< "SU"	<< "TA"
	       << "TE"	<< "TH"	<< "UN"	<< "VA"	<< "VS"	<< "VI";

	QString surfNamesFile = StelFileMgr::findFile("data/nomenclature.dat"); // compressed version of file nomenclature.fab
	if (!surfNamesFile.isEmpty()) // OK, the file is exist!
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
		QString name, ntypecode, planet = "", planetName = "", context = "";
		NomenclatureItem::NomenclatureItemType ntype;
		float latitude, longitude, size;
		QStringList faultPlanets;

		while (!buf.atEnd())
		{
			record = QString::fromUtf8(buf.readLine());
			lineNumber++;

			// Skip comments
			if (commentRx.exactMatch(record))
				continue;

			totalRecords++;
			if (!recRx.exactMatch(record))
				qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in surface nomenclature file" << QDir::toNativeSeparators(surfNamesFile);
			else
			{
				// Read the planet name
				planet	= recRx.capturedTexts().at(1).trimmed();
				// Read the ID of feature
				featureId	= recRx.capturedTexts().at(2).toInt();
				// Read the name of feature and context
				ctxt		= recRx.capturedTexts().at(3).trimmed();
				if (ctxRx.exactMatch(ctxt))
				{
					name = ctxRx.capturedTexts().at(1).trimmed();
					context = ctxRx.capturedTexts().at(2).trimmed();
				}
				else
				{
					name = ctxt;
					context = "";
				}
				// Read the type of feature
				ntypecode	= recRx.capturedTexts().at(4).trimmed();
				switch (nTypes.indexOf(ntypecode.toUpper()))
				{
					case 0:
						ntype = NomenclatureItem::niAlbedoFeature;
						break;
					case 1:
						ntype = NomenclatureItem::niArcus;
						break;
					case 2:
						ntype = NomenclatureItem::niAstrum;
						break;
					case 3:
						ntype = NomenclatureItem::niCatena;
						break;
					case 4:
						ntype = NomenclatureItem::niCavus;
						break;
					case 5:
						ntype = NomenclatureItem::niChaos;
						break;
					case 6:
						ntype = NomenclatureItem::niChasma;
						break;
					case 7:
						ntype = NomenclatureItem::niCollis;
						break;
					case 8:
						ntype = NomenclatureItem::niCorona;
						break;
					case 9:
						ntype = NomenclatureItem::niCrater;
						break;
					case 10:
						ntype = NomenclatureItem::niDorsum;
						break;
					case 11:
						ntype = NomenclatureItem::niEruptiveCenter;
						break;
					case 12:
						ntype = NomenclatureItem::niFacula;
						break;
					case 13:
						ntype = NomenclatureItem::niFarrum;
						break;
					case 14:
						ntype = NomenclatureItem::niFlexus;
						break;
					case 15:
						ntype = NomenclatureItem::niFluctus;
						break;
					case 16:
						ntype = NomenclatureItem::niFlumen;
						break;
					case 17:
						ntype = NomenclatureItem::niFossa;
						break;
					case 18:
						ntype = NomenclatureItem::niFretum;
						break;
					case 19:
						ntype = NomenclatureItem::niInsula;
						break;
					case 20:
						ntype = NomenclatureItem::niLabes;
						break;
					case 21:
						ntype = NomenclatureItem::niLabyrinthus;
						break;
					case 22:
						ntype = NomenclatureItem::niLacuna;
						break;
					case 23:
						ntype = NomenclatureItem::niLacus;
						break;
					case 24:
						ntype = NomenclatureItem::niLandingSite;
						break;
					case 25:
						ntype = NomenclatureItem::niLargeRingedFeature;
						break;
					case 26:
						ntype = NomenclatureItem::niLenticula;
						break;
					case 27:
						ntype = NomenclatureItem::niLinea;
						break;
					case 28:
						ntype = NomenclatureItem::niLingula;
						break;
					case 29:
						ntype = NomenclatureItem::niMacula;
						break;
					case 30:
						ntype = NomenclatureItem::niMare;
						break;
					case 31:
						ntype = NomenclatureItem::niMensa;
						break;
					case 32:
						ntype = NomenclatureItem::niMons;
						break;
					case 33:
						ntype = NomenclatureItem::niOceanus;
						break;
					case 34:
						ntype = NomenclatureItem::niPalus;
						break;
					case 35:
						ntype = NomenclatureItem::niPatera;
						break;
					case 36:
						ntype = NomenclatureItem::niPlanitia;
						break;
					case 37:
						ntype = NomenclatureItem::niPlanum;
						break;
					case 38:
						ntype = NomenclatureItem::niPlume;
						break;
					case 39:
						ntype = NomenclatureItem::niPromontorium;
						break;
					case 40:
						ntype = NomenclatureItem::niRegio;
						break;
					case 41:
						ntype = NomenclatureItem::niReticulum;
						break;
					case 42:
						ntype = NomenclatureItem::niRima;
						break;
					case 43:
						ntype = NomenclatureItem::niRupes;
						break;
					case 44:
						ntype = NomenclatureItem::niSatelliteFeature;
						break;
					case 45:
						ntype = NomenclatureItem::niScopulus;
						break;
					case 46:
						ntype = NomenclatureItem::niSerpens;
						break;
					case 47:
						ntype = NomenclatureItem::niSinus;
						break;
					case 48:
						ntype = NomenclatureItem::niSulcus;
						break;
					case 49:
						ntype = NomenclatureItem::niTerra;
						break;
					case 50:
						ntype = NomenclatureItem::niTessera;
						break;
					case 51:
						ntype = NomenclatureItem::niTholus;
						break;
					case 52:
						ntype = NomenclatureItem::niUnda;
						break;
					case 53:
						ntype = NomenclatureItem::niVallis;
						break;
					case 54:
						ntype = NomenclatureItem::niVastitas;
						break;
					case 55:
						ntype = NomenclatureItem::niVirga;
						break;
					default:
						ntype = NomenclatureItem::niUNDEFINED;
						break;
				}
				// Read the latitude of feature
				latitude	= recRx.capturedTexts().at(5).toFloat();
				// Read the longitude of feature
				longitude	= recRx.capturedTexts().at(6).toFloat();
				// Read the size of feature
				size		= recRx.capturedTexts().at(7).toFloat();

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
					faultPlanets << planet;
			}
		}

		buf.close();
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "items of planetary surface nomenclature";

		faultPlanets.removeDuplicates();
		int err = faultPlanets.size();
		if (err>0)
			qDebug() << "WARNING - The next planets to assign nomenclature items is not found:" << faultPlanets.join(", ");
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
	painter.setFont(font);
	const SphericalCap& viewportRegion = painter.getProjector()->getBoundingCap();

	for (const auto& p : nomenclatureItems.uniqueKeys())
	{
		// Early exit if the planet is not visible or too small to render the
		// labels.
		const Vec3d equPos = p->getJ2000EquatorialPos(core);
		const double r = p->getEquatorialRadius() * p->getSphereScale();
		double angularSize = atan2(r, equPos.length());
		double screenSize = angularSize * painter.getProjector()->getPixelPerRadAtCenter();
		if (screenSize < 50)
			continue;
		Vec3d n = equPos; n.normalize();
		SphericalCap boundingCap(n, cos(angularSize));
		if (!viewportRegion.intersects(boundingCap))
			continue;
		if (p->getVMagnitude(core) >= 20.)
			continue;

		// Render all the items of this planet.
		for (auto i = nomenclatureItems.find(p); i != nomenclatureItems.end() && i.key() == p; ++i)
		{
			const NomenclatureItemP& nItem = i.value();
			if (nItem)
				nItem->draw(core, &painter);
		}
	}

	if (GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(core, painter);
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

		const Vec3f& c(obj->getInfoColor());
		painter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		painter.setBlending(true);
		painter.drawSprite2dMode(screenpos[0], screenpos[1], 13.f, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

QList<StelObjectP> NomenclatureMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;

	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	Vec3d equPos;

	for (const auto& nItem : nomenclatureItems)
	{
		equPos = nItem->getJ2000EquatorialPos(core);
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
		{
			result.append(qSharedPointerCast<StelObject>(nItem));
		}
	}


	return result;
}

StelObjectP NomenclatureMgr::searchByName(const QString& englishName) const
{
	if (getFlagLabels())
	{
		for (const auto& nItem : nomenclatureItems)
		{
			if (nItem->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature && nItem->getEnglishName().toUpper() == englishName.toUpper())
			{
				return qSharedPointerCast<StelObject>(nItem);
			}
		}
	}

	return Q_NULLPTR;
}

StelObjectP NomenclatureMgr::searchByNameI18n(const QString& nameI18n) const
{
	if (getFlagLabels())
	{
		for (const auto& nItem : nomenclatureItems)
		{
			if (nItem->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature && nItem->getNameI18n().toUpper() == nameI18n.toUpper())
			{
				return qSharedPointerCast<StelObject>(nItem);
			}
		}
	}

	return Q_NULLPTR;
}

QStringList NomenclatureMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords, bool inEnglish) const
{
	return StelObjectModule::listMatchingObjects(objPrefix, maxNbItem, useStartOfWords, inEnglish);
}

QStringList NomenclatureMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;

	if (getFlagLabels())
	{
		if (inEnglish)
		{
			for (const auto& nItem : nomenclatureItems)
			{
				if (nItem->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature)
					result << nItem->getEnglishName();
			}
		}
		else
		{
			for (const auto& nItem : nomenclatureItems)
			{
				if (nItem->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature)
					result << nItem->getNameI18n();
			}
		}
	}

	return result;
}

QStringList NomenclatureMgr::listAllObjectsByType(const QString &objType, bool inEnglish) const
{
	QStringList result;

	if (getFlagLabels())
	{
		int type = objType.toInt();
		switch (type)
		{
			case 0:
			{
				for (const auto& nItem : nomenclatureItems)
				{
					if (nItem->getPlanet()->getEnglishName().contains(objType, Qt::CaseSensitive) && nItem->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature)
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
	if (getFlagLabels())
	{
		for (const auto& p : nomenclatureItems)
		{
			if (p->getNomenclatureType()!=NomenclatureItem::niSatelliteFeature && p->getEnglishName() == nomenclatureItemEnglishName)
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

void NomenclatureMgr::setFlagLabels(bool b)
{
	if (getFlagLabels() != b)
	{
		for (const auto& i : nomenclatureItems)
			i->setFlagLabels(b);
		emit nomenclatureDisplayedChanged(b);
	}
}

bool NomenclatureMgr::getFlagLabels() const
{
	for (const auto& i : nomenclatureItems)
	{
		if (i->getFlagLabels())
			return true;
	}
	return false;
}

void NomenclatureMgr::setFlagHideLocalNomenclature(bool b)
{
	NomenclatureItem::hideLocalNomenclature = b;
	emit localNomenclatureHidingChanged(b);
}

bool NomenclatureMgr::getFlagHideLocalNomenclature() const
{
	return NomenclatureItem::hideLocalNomenclature;
}

// Update i18 names from english names according to current sky culture translator
void NomenclatureMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getPlanetaryFeaturesTranslator();
	for (const auto& i : nomenclatureItems)
		i->translateName(trans);
}
