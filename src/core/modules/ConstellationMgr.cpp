/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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


#include "ConstellationMgr.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelProjector.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "Planet.hpp"
#include "StelUtils.hpp"
#include "precession.h"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFont>

// constructor which loads all data from appropriate files
ConstellationMgr::ConstellationMgr(StarMgr *_hip_stars)
	: hipStarMgr(_hip_stars),
	  isolateSelected(false),
	  flagConstellationPick(false),
	  artFadeDuration(2.),
	  artIntensity(0),
	  artIntensityMinimumFov(1.0),
	  artIntensityMaximumFov(2.0),
	  artDisplayed(false),
	  boundariesDisplayed(false),
	  boundariesFadeDuration(1.),
	  linesDisplayed(false),
	  linesFadeDuration(0.),
	  namesDisplayed(false),
	  namesFadeDuration(1.),
	  hullsDisplayed(false),
	  hullsFadeDuration(1.),
	  zodiacFadeDuration(1.),
	  lunarSystemFadeDuration(1.),
	  checkLoadingData(false),
	  fontSize(15),
	  constellationLineThickness(1),
	  boundariesThickness(1),
	  hullsThickness(1),
	  zodiacThickness(1),
	  lunarSystemThickness(1),
	  zodiacColor(1.,1.,0.),
	  lunarSystemColor(0., 1., 0.5)
{
	setObjectName("ConstellationMgr");
	Q_ASSERT(hipStarMgr);
}

ConstellationMgr::~ConstellationMgr()
{
	for (auto* constellation : constellations)
	{
		delete constellation;
	}

	for (auto* segment : allBoundarySegments)
	{
		delete segment;
	}
}

void ConstellationMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setFontSize(conf->value("viewing/constellation_font_size", 15).toInt());
	setFlagLines(conf->value("viewing/flag_constellation_drawing", false).toBool());
	setFlagLabels(conf->value("viewing/flag_constellation_name", false).toBool());
	setFlagBoundaries(conf->value("viewing/flag_constellation_boundaries", false).toBool());
	setFlagHulls(conf->value("viewing/flag_constellation_hulls", false).toBool());
	setFlagZodiac(conf->value("viewing/flag_skyculture_zodiac", false).toBool());
	setFlagLunarSystem(conf->value("viewing/flag_skyculture_lunarsystem", false).toBool());
	setArtIntensity(conf->value("viewing/constellation_art_intensity", 0.5f).toFloat());
	setArtFadeDuration(conf->value("viewing/constellation_art_fade_duration",2.f).toFloat());
	setFlagArt(conf->value("viewing/flag_constellation_art", false).toBool());
	setFlagIsolateSelected(conf->value("viewing/flag_constellation_isolate_selected", false).toBool());
	setFlagConstellationPick(conf->value("viewing/flag_constellation_pick", false).toBool());
	setConstellationLineThickness(conf->value("viewing/constellation_line_thickness", 1).toInt());
	setBoundariesThickness(conf->value("viewing/constellation_boundaries_thickness", 1).toInt());
	setBoundariesFadeDuration(conf->value("viewing/constellation_boundaries_fade_duration", 1.0f).toFloat());
	setHullsThickness(conf->value("viewing/constellation_hulls_thickness", 1).toInt());
	setHullsFadeDuration(conf->value("viewing/constellation_hulls_fade_duration", 1.0f).toFloat());
	setZodiacThickness(conf->value("viewing/skyculture_zodiac_thickness", 1).toInt());
	setZodiacFadeDuration(conf->value("viewing/skyculture_zodiac_fade_duration", 1.0f).toFloat());
	setHullsThickness(conf->value("viewing/skyculture_lunarsystem_thickness", 1).toInt());
	setLunarSystemFadeDuration(conf->value("viewing/skyculture_lunarsystem_fade_duration", 1.0f).toFloat());

	setLinesFadeDuration(conf->value("viewing/constellation_lines_fade_duration", 1.0f).toFloat());
	setLabelsFadeDuration(conf->value("viewing/constellation_labels_fade_duration", 1.0f).toFloat());
	// The setting for developers
	setFlagCheckLoadingData(conf->value("devel/check_loading_constellation_data","false").toBool());

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setLinesColor(Vec3f(conf->value("color/const_lines_color", defaultColor).toString()));
	setBoundariesColor(Vec3f(conf->value("color/const_boundary_color", "0.8,0.3,0.3").toString()));
	setHullsColor(Vec3f(conf->value("color/const_hull_color", "0.6,0.2,0.2").toString()));
	setZodiacColor(Vec3f(conf->value("color/skyculture_zodiac_color", "1.0,1.0,0.0").toString()));
	setLunarSystemColor(Vec3f(conf->value("color/skyculture_lunarsystem_color", "0.0,1.0,0.5").toString()));
	setLabelsColor(Vec3f(conf->value("color/const_names_color", defaultColor).toString()));

	StelObjectMgr *objectManager = GETSTELMODULE(StelObjectMgr);
	objectManager->registerStelObjectMgr(this);
	connect(objectManager, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),
			this, SLOT(selectedObjectChange(StelModule::StelModuleSelectAction)));
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), &StelSkyCultureMgr::currentSkyCultureChanged, this, &ConstellationMgr::updateSkyCulture);

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Constellation_Lines", displayGroup, N_("Constellation lines"), "linesDisplayed", "C");
	addAction("actionShow_Constellation_Art", displayGroup, N_("Constellation art"), "artDisplayed", "R");
	addAction("actionShow_Constellation_Labels", displayGroup, N_("Constellation labels"), "namesDisplayed", "V");
	addAction("actionShow_Constellation_Boundaries", displayGroup, N_("Constellation boundaries"), "boundariesDisplayed", "B");
	addAction("actionShow_Zodiac", displayGroup, N_("Zodiac (if defined in skyculture)"), "zodiacDisplayed");
	addAction("actionShow_LunarSystem", displayGroup, N_("Lunar stations/mansions (if defined in skyculture)"), "lunarSystemDisplayed");

	if (conf->value("gui/skyculture_enable_hulls", "false").toBool())
		addAction("actionShow_Constellation_Hulls", displayGroup, N_("Constellation areas (hulls)"), "hullsDisplayed", "Shift+B");
	addAction("actionShow_Constellation_Isolated", displayGroup, N_("Select single constellation"), "isolateSelected"); // no shortcut, sync with GUI
	addAction("actionShow_Constellation_Deselect", displayGroup, N_("Remove selection of constellations"), this, "deselectConstellations()", "W");
	addAction("actionShow_Constellation_Select", displayGroup, N_("Select all constellations"), this, "selectAllConstellations()", "Alt+W");
	// Reload the current sky culture
	addAction("actionShow_SkyCulture_Reload", displayGroup, N_("Reload the sky culture"), this, "reloadSkyCulture()", "Ctrl+Alt+I");
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double ConstellationMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr")->getCallOrder(actionName)+10;
	return 0;
}

void ConstellationMgr::reloadSkyCulture()
{
	StelApp::getInstance().getSkyCultureMgr().reloadSkyCulture();
}

void ConstellationMgr::updateSkyCulture(const StelSkyCulture& skyCulture)
{
	// first of all, remove constellations from the list of selected objects in StelObjectMgr, since we are going to delete them
	deselectConstellations();
	loadLinesNamesAndArt(skyCulture);

	// Load optional StelSkyCultureSkyPartitions, or delete obsolete ones. [PRELIMINARY]
	if (skyCulture.zodiac.count()>1)
	{
		zodiac = StelSkyCultureSkyPartitionP(new StelSkyCultureSkyPartition(skyCulture.zodiac));
		zodiac->centerLine->setDisplayed(true);
	}
	else if (zodiac) zodiac.clear();

	if (skyCulture.lunarSystem.count()>1)
	{
		lunarSystem = StelSkyCultureSkyPartitionP(new StelSkyCultureSkyPartition(skyCulture.lunarSystem));
		lunarSystem->centerLine->setDisplayed(true);
	}
	else if (lunarSystem) lunarSystem.clear();

	// Translate constellation names for the new sky culture
	updateI18n();

	loadBoundaries(skyCulture.boundaries, skyCulture.boundariesEpoch);

	if (getFlagCheckLoadingData())
	{
		int i = 1;
		for (auto* constellation : std::as_const(constellations))
		{
			qInfo() << "[Constellation] #" << i << " abbr:" << constellation->abbreviation << " name:" << constellation->getEnglishName() << " segments:" << constellation->numberOfSegments;
			i++;
		}
	}
}

void ConstellationMgr::selectedObjectChange(StelModule::StelModuleSelectAction action)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	const QList<StelObjectP> newSelected = omgr->getSelectedObject();
	if (newSelected.empty())
	{
		// Even if do not have anything selected, KEEP constellation selection intact
		// (allows viewing constellations without distraction from star pointer animation)
		// setSelected(nullptr);
		return;
	}
#ifndef NDEBUG
	else
	{
		qDebug() << "ConstellationMgr::selectedObjectChange(): selected objects:";
		foreach (StelObjectP obj, newSelected)
			qDebug() << "   " << obj->getID();
	}
#endif

	const QList<StelObjectP> newSelectedConst = omgr->getSelectedObject("Constellation");
	if (!newSelectedConst.empty())
	{
		// If removing this selection
		if(action == StelModule::RemoveFromSelection)
		{
			unsetSelectedConst(static_cast<Constellation *>(newSelectedConst[0].data()));
		}
		else
		{
			// Add constellation to selected list (do not select a star, just the constellation)
			QList<Constellation*>cList;
			cList.append(static_cast<Constellation *>(newSelectedConst[0].data()));
			setSelectedConst(cList);
		}
	}
	else
	{
		QList<StelObjectP> newSelectedObject = omgr->getSelectedObject();
		if (!newSelectedObject.empty())
		{
			setSelected(newSelectedObject[0].data());
		}
		else
		{
			setSelected(nullptr);
		}
	}
}

void ConstellationMgr::deselectConstellations(void)
{
	static StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	if (getFlagIsolateSelected())
	{
		// The list of selected constellations is empty, but...
		if (selected.size()==0)
		{
			// ...let's unselect all constellations for guarantee
			for (auto& constellation : constellations)
			{
				constellation->setFlagLines(false);
				constellation->setFlagLabels(false);
				constellation->setFlagArt(false);
				constellation->setFlagBoundaries(false);
				constellation->setFlagHull(false);
			}
		}

		// If any constellation is selected at the moment, then let's do not touch to it!
		if (omgr->getWasSelected() && !selected.empty())
			selected.pop_back();

		// Let's hide all previously selected constellations
		for (auto& constellation : selected)
		{
			constellation->setFlagLines(false);
			constellation->setFlagLabels(false);
			constellation->setFlagArt(false);
			constellation->setFlagBoundaries(false);
			constellation->setFlagHull(false);
		}
	}
	else
	{
		const QList<StelObjectP> newSelectedConst = omgr->getSelectedObject("Constellation");
		if (!newSelectedConst.empty())
			omgr->unSelect();
	}
	selected.clear();
}

void ConstellationMgr::selectAllConstellations()
{
	// We really must select in a loop! (GH:#4415)
	for (auto* constellation : qAsConst(constellations))
	{
		QList<Constellation *>cList;
		cList.append(constellation);
		setSelectedConst(cList);
	}
}

void ConstellationMgr::selectConstellation(const QString &englishName)
{
	if (!getFlagIsolateSelected())
		setFlagIsolateSelected(true); // Enable isolated selection

	bool found = false;
	QList<Constellation *>cList;
	for (auto& constellation : constellations)
	{
		if (constellation->getEnglishName().toLower()==englishName.toLower())
		{
			cList.append(constellation);
			setSelectedConst(cList);
			found = true;
		}
	}
	if (!found)
		qDebug() << "The constellation" << englishName << "was not found";
}

void ConstellationMgr::selectConstellationByObjectName(const QString &englishName)
{
	if (!getFlagIsolateSelected())
		setFlagIsolateSelected(true); // Enable isolated selection

	if (StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureBoundariesType()==StelSkyCulture::BoundariesType::IAU)
		setSelectedConst(isObjectIn(GETSTELMODULE(StelObjectMgr)->searchByName(englishName).data(), false));
	else
		setSelectedConst(isObjectIn(GETSTELMODULE(StelObjectMgr)->searchByName(englishName).data(), true));
}

void ConstellationMgr::deselectConstellation(const QString &englishName)
{
	if (!getFlagIsolateSelected())
		setFlagIsolateSelected(true); // Enable isolated selection

	bool found = false;
	for (auto& constellation : constellations)
	{
		if (constellation->getEnglishName().toLower()==englishName.toLower())
		{
			unsetSelectedConst(constellation);
			found = true;
		}
	}

	if (selected.size()==0 && found)
	{
		// Let's remove the selection for all constellations if the list of selected constellations is empty
		for (auto& constellation : constellations)
		{
			constellation->setFlagLines(false);
			constellation->setFlagLabels(false);
			constellation->setFlagArt(false);
			constellation->setFlagBoundaries(false);
			constellation->setFlagHull(false);
		}
	}

	if (!found)
		qDebug() << "The constellation" << englishName << "was not found";
}

void ConstellationMgr::setLinesColor(const Vec3f& color)
{
	if (color != Constellation::lineColor)
	{
		Constellation::lineColor = color;
		emit linesColorChanged(color);
	}
}

Vec3f ConstellationMgr::getLinesColor() const
{
	return Constellation::lineColor;
}

void ConstellationMgr::setBoundariesColor(const Vec3f& color)
{
	if (Constellation::boundaryColor != color)
	{
		Constellation::boundaryColor = color;
		emit boundariesColorChanged(color);
	}
}

Vec3f ConstellationMgr::getBoundariesColor() const
{
	return Constellation::boundaryColor;
}

void ConstellationMgr::setHullsColor(const Vec3f& color)
{
	if (Constellation::hullColor != color)
	{
		Constellation::hullColor = color;
		emit hullsColorChanged(color);
	}
}

Vec3f ConstellationMgr::getHullsColor() const
{
	return Constellation::hullColor;
}

void ConstellationMgr::setZodiacColor(const Vec3f& color)
{
	if (zodiacColor != color)
	{
		zodiacColor = color;
		emit zodiacColorChanged(color);
	}
}

Vec3f ConstellationMgr::getZodiacColor() const
{
	return zodiacColor;
}

void ConstellationMgr::setLunarSystemColor(const Vec3f& color)
{
	if (lunarSystemColor != color)
	{
		lunarSystemColor = color;
		emit lunarSystemColorChanged(color);
	}
}

Vec3f ConstellationMgr::getLunarSystemColor() const
{
	return lunarSystemColor;
}

void ConstellationMgr::setLabelsColor(const Vec3f& color)
{
	if (Constellation::labelColor != color)
	{
		Constellation::labelColor = color;
		emit namesColorChanged(color);
	}
}

Vec3f ConstellationMgr::getLabelsColor() const
{
	return Constellation::labelColor;
}

void ConstellationMgr::setFontSize(const int newFontSize)
{
	fontSize=newFontSize;
	StelApp::immediateSave("viewing/constellation_font_size", newFontSize);
	emit fontSizeChanged(newFontSize);
}

int ConstellationMgr::getFontSize() const
{
	return fontSize;
}

void ConstellationMgr::setConstellationLineThickness(const int thickness)
{
	if(thickness!=constellationLineThickness)
	{
		constellationLineThickness = qMax(1, thickness); // cannot be 0 or neg.

		StelApp::immediateSave("viewing/constellation_line_thickness", thickness);
		emit constellationLineThicknessChanged(thickness);
	}
}

void ConstellationMgr::setBoundariesThickness(const int thickness)
{
	if(thickness!=boundariesThickness)
	{
		boundariesThickness = qMax(1, thickness); // cannot be 0 or neg.

		StelApp::immediateSave("viewing/constellation_boundaries_thickness", thickness);
		emit boundariesThicknessChanged(thickness);
	}
}

void ConstellationMgr::setHullsThickness(const int thickness)
{
	if(thickness!=hullsThickness)
	{
		hullsThickness = qMax(1, thickness); // cannot be 0 or neg.

		StelApp::immediateSave("viewing/constellation_hulls_thickness", thickness);
		emit hullsThicknessChanged(thickness);
	}
}

void ConstellationMgr::setZodiacThickness(const int thickness)
{
	if(thickness!=zodiacThickness)
	{
		zodiacThickness = qMax(1, thickness); // cannot be 0 or neg.

		StelApp::immediateSave("viewing/skyculture_zodiac_thickness", thickness);
		emit zodiacThicknessChanged(thickness);
	}
}

void ConstellationMgr::setLunarSystemThickness(const int thickness)
{
	if(thickness!=lunarSystemThickness)
	{
		lunarSystemThickness = qMax(1, thickness); // cannot be 0 or neg.

		StelApp::immediateSave("viewing/skyculture_lunar_system_thickness", thickness);
		emit lunarSystemThicknessChanged(thickness);
	}
}

void ConstellationMgr::loadLinesNamesAndArt(const StelSkyCulture &culture)
{
	constellations.clear();
	Constellation::seasonalRuleEnabled = false;

	int readOk = 0;
	for (const auto& constellationData : culture.constellations)
	{
		Constellation*const cons = new Constellation;
		const auto consObj = constellationData.toObject();
		if (!cons->read(consObj, hipStarMgr))
		{
			delete cons;
			continue;
		}
		++readOk;
		constellations.push_back(cons);

		cons->artOpacity = artIntensity;
		cons->artFader.setDuration(static_cast<int>(artFadeDuration * 1000.f));
		cons->lineFader.setDuration(static_cast<int>(linesFadeDuration * 1000.f));
		cons->boundaryFader.setDuration(static_cast<int>(boundariesFadeDuration * 1000.f));
		cons->hullFader.setDuration(static_cast<int>(hullsFadeDuration * 1000.f));
		cons->nameFader.setDuration(static_cast<int>(namesFadeDuration * 1000.f));
		cons->setFlagArt(artDisplayed);
		cons->setFlagBoundaries(boundariesDisplayed);
		cons->setFlagHull(hullsDisplayed);
		cons->setFlagLines(linesDisplayed);
		cons->setFlagLabels(namesDisplayed);

		// Now load constellation art

		const auto imgVal = consObj["image"];
		if (imgVal.isUndefined())
			continue;
		const auto imgData = imgVal.toObject();
		const auto anchors = imgData["anchors"].toArray();
		if (anchors.size() < 3)
		{
			qWarning().nospace() << "Bad number of anchors (" << anchors.size() << ") for image in constellation "
			                     << consObj["id"].toString();
			continue;
		}
		const auto anchor1 = anchors[0].toObject();
		const auto anchor2 = anchors[1].toObject();
		const auto anchor3 = anchors[2].toObject();
		const auto xy1 = anchor1["pos"].toArray();
		const auto xy2 = anchor2["pos"].toArray();
		const auto xy3 = anchor3["pos"].toArray();

		const int x1 = xy1[0].toInt();
		const int y1 = xy1[1].toInt();
		const int x2 = xy2[0].toInt();
		const int y2 = xy2[1].toInt();
		const int x3 = xy3[0].toInt();
		const int y3 = xy3[1].toInt();
		const int hp1 = StelUtils::getLongLong(anchor1["hip"]);
		const int hp2 = StelUtils::getLongLong(anchor2["hip"]);
		const int hp3 = StelUtils::getLongLong(anchor3["hip"]);

		const auto texfile = imgData["file"].toString();

		auto texturePath = culture.path+"/"+texfile;
		if (!QFileInfo::exists(texturePath))
		{
			qWarning() << "ERROR: could not find texture" << QDir::toNativeSeparators(texfile);
			texturePath.clear();
		}

		cons->artTexture = StelApp::getInstance().getTextureManager().createTextureThread(texturePath, StelTexture::StelTextureParams(true));

		const auto sizeData = imgData["size"].toArray();
		if (sizeData.size() != 2)
		{
			qWarning().nospace() << "Bad length of \"size\" array for image in constellation "
			                     << consObj["id"].toString();
			continue;
		}
		const int texSizeX = sizeData[0].toInt(), texSizeY = sizeData[1].toInt();

		StelCore* core = StelApp::getInstance().getCore();
		StelObjectP s1obj = hipStarMgr->searchHP(static_cast<int>(hp1));
		StelObjectP s2obj = hipStarMgr->searchHP(static_cast<int>(hp2));
		StelObjectP s3obj = hipStarMgr->searchHP(static_cast<int>(hp3));

		// check for null pointers
		if (s1obj.isNull() || s2obj.isNull() || s3obj.isNull())
		{
			qWarning() << "ERROR: could not find stars:" << hp1 << hp2 << hp3 << " in constellation" << consObj["id"].toString();
			continue;
		}

		const Vec3d s1 = s1obj->getJ2000EquatorialPos(core);
		const Vec3d s2 = s2obj->getJ2000EquatorialPos(core);
		const Vec3d s3 = s3obj->getJ2000EquatorialPos(core);

		// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
		// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate space
		// We need 3 stars and the 4th point is deduced from the others to get a normal base
		// X = B inv(A)
		Vec3d s4 = s1 + ((s2 - s1) ^ (s3 - s1));
		Mat4d B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
		Mat4d A(x1, texSizeY - static_cast<int>(y1), 0., 1., x2, texSizeY - static_cast<int>(y2), 0., 1., x3, texSizeY - static_cast<int>(y3), 0., 1., x1, texSizeY - static_cast<int>(y1), texSizeX, 1.);
		Mat4d X = B * A.inverse();

		// Tessellate on the plane assuming a tangential projection for the image
		static const int nbPoints=5;
		QVector<Vec2f> texCoords;
		texCoords.reserve(nbPoints*nbPoints*6);
		for (int j=0;j<nbPoints;++j)
		{
			for (int i=0;i<nbPoints;++i)
			{
				texCoords << Vec2f((static_cast<float>(i))/nbPoints, (static_cast<float>(j))/nbPoints);
				texCoords << Vec2f((static_cast<float>(i)+1.f)/nbPoints, (static_cast<float>(j))/nbPoints);
				texCoords << Vec2f((static_cast<float>(i))/nbPoints, (static_cast<float>(j)+1.f)/nbPoints);
				texCoords << Vec2f((static_cast<float>(i)+1.f)/nbPoints, (static_cast<float>(j))/nbPoints);
				texCoords << Vec2f((static_cast<float>(i)+1.f)/nbPoints, (static_cast<float>(j)+1.f)/nbPoints);
				texCoords << Vec2f((static_cast<float>(i))/nbPoints, (static_cast<float>(j)+1.f)/nbPoints);
			}
		}

		QVector<Vec3d> contour;
		contour.reserve(texCoords.size());
		for (const auto& v : std::as_const(texCoords))
		{
			Vec3d vertex = X * Vec3d(static_cast<double>(v[0]) * texSizeX, static_cast<double>(v[1]) * texSizeY, 0.);
			// Originally the projected texture plane remained as tangential plane.
			// The vertices should however be reduced to the sphere for correct aberration:
			vertex.normalize();
			contour << vertex;
		}

		cons->artPolygon.vertex=contour;
		cons->artPolygon.texCoords=texCoords;
		cons->artPolygon.primitiveType=StelVertexArray::Triangles;

		Vec3d tmp(X * Vec3d(0.5*texSizeX, 0.5*texSizeY, 0.));
		tmp.normalize();
		Vec3d tmp2(X * Vec3d(0., 0., 0.));
		tmp2.normalize();
		cons->boundingCap.n=tmp;
		cons->boundingCap.d=tmp*tmp2;
	}

	qInfo().noquote() << "Loaded" << readOk << "/" << constellations.size() << "constellation records successfully for culture" << culture.id;

	// Set current states
	setFlagArt(artDisplayed);
	setFlagLines(linesDisplayed);
	setFlagLabels(namesDisplayed);
	setFlagBoundaries(boundariesDisplayed);
	setFlagHulls(hullsDisplayed);
}

void ConstellationMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	//sPainter.setFont(asterFont);
	QFont font=QGuiApplication::font();
	font.setPixelSize(fontSize);
	sPainter.setFont(font);
	drawLines(sPainter, core);
	Vec3d vel(0.);
	if (core->getUseAberration())
	{
		vel = core->getAberrationVec(core->getJDE());
	}
	drawNames(sPainter, vel);
	drawArt(sPainter, vel);
	drawBoundaries(sPainter, vel);
	drawHulls(sPainter, vel);
	if (core->getCurrentPlanet()->getEnglishName()=="Earth")
	{
		drawZodiac(sPainter, vel);
		drawLunarSystem(sPainter, vel);
	}
}

// Draw constellations art textures
void ConstellationMgr::drawArt(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setCullFace(true);

	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	for (auto* constellation : constellations)
	{
		constellation->drawArtOptim(sPainter, *region, obsVelocity);
	}

	sPainter.setCullFace(false);
}

// Draw constellations lines
void ConstellationMgr::drawLines(StelPainter& sPainter, const StelCore* core) const
{
	const float scale = sPainter.getProjector()->getScreenScale();

	sPainter.setBlending(true);
	if (constellationLineThickness>1 || scale>1.f)
		sPainter.setLineWidth(constellationLineThickness*scale); // set line thickness
	sPainter.setLineSmooth(true);

	const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();
	for (auto* constellation : constellations)
	{
		constellation->drawOptim(sPainter, core, viewportHalfspace);
	}
	if (constellationLineThickness>1 || scale>1.f)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw the names of all the constellations
void ConstellationMgr::drawNames(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	sPainter.setBlending(true);
	for (auto* constellation : constellations)
	{
		for (int i=0; i<constellation->XYZname.size(); ++i)
		{
			Vec3d XYZname=constellation->XYZname.at(i);
			XYZname.normalize();
			XYZname+=obsVelocity;
			XYZname.normalize();

			Vec3d xyName;
			// Check if in the field of view
			if (sPainter.getProjector()->projectCheck(XYZname, xyName))
				constellation->drawName(xyName, sPainter);
		}
	}
}

Constellation* ConstellationMgr::findFromAbbreviation(const QString& abbreviation) const
{
	for (auto* constellation : constellations)
	{
		if (constellation->abbreviation.compare(abbreviation, Qt::CaseInsensitive) == 0)
		{
#ifndef NDEBUG
			if (constellation->abbreviation != abbreviation)
				qDebug() << "ConstellationMgr::findFromAbbreviation: not a perfect match, but sufficient:" << constellation->abbreviation << "vs." << abbreviation;
#endif
			return constellation;
		}
	}
	return nullptr;
}

QStringList ConstellationMgr::getConstellationsEnglishNames()
{
	QStringList constellationsEnglishNames;
	for (auto* constellation : std::as_const(constellations))
	{
		constellationsEnglishNames.append(constellation->getEnglishName());
	}
	return  constellationsEnglishNames;
}

void ConstellationMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();

	for (auto* constellation : std::as_const(constellations))
	{
		QString context = constellation->context;

		constellation->culturalName.translatedI18n = trans.tryQtranslate(constellation->culturalName.translated, context);
		if (constellation->culturalName.translatedI18n.isEmpty())
		{
			if (context.isEmpty())
				constellation->culturalName.translatedI18n = q_(constellation->culturalName.translated);
			else
				constellation->culturalName.translatedI18n = qc_(constellation->culturalName.translated, context);
		}

		constellation->culturalName.pronounceI18n = trans.tryQtranslate(constellation->culturalName.pronounce, context);
		if (constellation->culturalName.pronounceI18n.isEmpty())
		{
			if (context.isEmpty())
				constellation->culturalName.pronounceI18n = q_(constellation->culturalName.pronounce);
			else
				constellation->culturalName.pronounceI18n = qc_(constellation->culturalName.pronounce, context);
		}

		constellation->culturalName.bynameI18n = trans.tryQtranslate(constellation->culturalName.byname, context);
		if (constellation->culturalName.bynameI18n.isEmpty())
		{
			if (context.isEmpty())
				constellation->culturalName.bynameI18n = q_(constellation->culturalName.byname);
			else
				constellation->culturalName.bynameI18n = qc_(constellation->culturalName.byname, context);
		}

		const QString abbrContext = "abbreviation"; // fixed context for all abbreviations
		constellation->abbreviationI18n = trans.tryQtranslate(constellation->abbreviation, abbrContext).trimmed();
		if (constellation->abbreviationI18n.isEmpty())
		{
			constellation->abbreviationI18n = qc_(constellation->abbreviation, abbrContext).trimmed();
		}
	}
	if (zodiac)
		zodiac->updateI18n();
	if (lunarSystem)
		lunarSystem->updateI18n();
}

// update faders
void ConstellationMgr::update(double deltaTime)
{
	//calculate FOV fade value, linear fade between artIntensityMaximumFov and artIntensityMinimumFov
	static StelCore *core = StelApp::getInstance().getCore();
	double fov = core->getMovementMgr()->getCurrentFov();
	Constellation::artIntensityFovScale = static_cast<float>(qBound(0.0,(fov - artIntensityMinimumFov) / (artIntensityMaximumFov - artIntensityMinimumFov),1.0));

	if (hullsDisplayed) // Computational cost is fortunately negligible, but at least by rapid aberration changes we really need dense interval. So, basically, every frame...
		recreateHulls();

	const int delta = static_cast<int>(deltaTime*1000);
	for (auto* constellation : std::as_const(constellations))
	{
		constellation->update(delta);
	}
	zodiacFader.update(delta);
	lunarSystemFader.update(delta);
	if (zodiac)
		zodiac->update(deltaTime);
	if (lunarSystem)
		lunarSystem->update(deltaTime);
}

void ConstellationMgr::setArtIntensity(const float intensity)
{
	if ((artIntensity - intensity) != 0.0f)
	{
		artIntensity = intensity;

		for (auto* constellation : std::as_const(constellations))
		{
			constellation->artOpacity = artIntensity;
		}
		StelApp::immediateSave("viewing/constellation_art_intensity", intensity);
		emit artIntensityChanged(static_cast<double>(intensity));
	}
}

float ConstellationMgr::getArtIntensity() const
{
	return artIntensity;
}

void ConstellationMgr::setArtIntensityMinimumFov(const double fov)
{
	artIntensityMinimumFov = fov;
}

double ConstellationMgr::getArtIntensityMinimumFov() const
{
	return artIntensityMinimumFov;
}

void ConstellationMgr::setArtIntensityMaximumFov(const double fov)
{
	artIntensityMaximumFov = fov;
}

double ConstellationMgr::getArtIntensityMaximumFov() const
{
	return artIntensityMaximumFov;
}

void ConstellationMgr::setArtFadeDuration(const float duration)
{
	if (!qFuzzyCompare(artFadeDuration, duration))
	{
		artFadeDuration = duration;

		for (auto &constellation : constellations)
		{
			constellation->artFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		StelApp::immediateSave("viewing/constellation_art_fade_duration", duration);
		emit artFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getArtFadeDuration() const
{
	return artFadeDuration;
}

void ConstellationMgr::setBoundariesFadeDuration(const float duration)
{
	if (!qFuzzyCompare(boundariesFadeDuration, duration))
	{
		boundariesFadeDuration = duration;

		for (auto& constellation : constellations)
		{
			constellation->boundaryFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		StelApp::immediateSave("viewing/constellation_boundaries_fade_duration", duration);
		emit boundariesFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getBoundariesFadeDuration() const
{
	return boundariesFadeDuration;
}

void ConstellationMgr::setHullsFadeDuration(const float duration)
{
	if (!qFuzzyCompare(hullsFadeDuration, duration))
	{
		hullsFadeDuration = duration;

		for (auto& constellation : constellations)
		{
			constellation->hullFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		StelApp::immediateSave("viewing/constellation_hulls_fade_duration", duration);
		emit hullsFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getHullsFadeDuration() const
{
	return hullsFadeDuration;
}

void ConstellationMgr::setZodiacFadeDuration(const float duration)
{
	if (!qFuzzyCompare(zodiacFadeDuration, duration))
	{
		zodiacFadeDuration = duration;
		zodiacFader.setDuration(static_cast<int>(zodiacFadeDuration * 1000.f));
		StelApp::immediateSave("viewing/skyculture_zodiac_fade_duration", duration);
		emit zodiacFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getZodiacFadeDuration() const
{
	return zodiacFadeDuration;
}

void ConstellationMgr::setLunarSystemFadeDuration(const float duration)
{
	if (!qFuzzyCompare(lunarSystemFadeDuration, duration))
	{
		lunarSystemFadeDuration=duration;
		lunarSystemFader.setDuration(static_cast<int>(lunarSystemFadeDuration * 1000.f));
		StelApp::immediateSave("viewing/skyculture_lunarsystem_fade_duration", duration);
		emit lunarSystemFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getLunarSystemFadeDuration() const
{
	return lunarSystemFadeDuration;
}

void ConstellationMgr::setLinesFadeDuration(const float duration)
{
	if (!qFuzzyCompare(linesFadeDuration, duration))
	{
		linesFadeDuration = duration;

		for (auto& constellation : constellations)
		{
			constellation->lineFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		StelApp::immediateSave("viewing/constellation_lines_fade_duration", duration);
		emit linesFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getLinesFadeDuration() const
{
	return linesFadeDuration;
}

void ConstellationMgr::setLabelsFadeDuration(const float duration)
{
	if (!qFuzzyCompare(namesFadeDuration, duration))
	{
		namesFadeDuration = duration;

		for (auto& constellation : constellations)
		{
			constellation->nameFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		StelApp::immediateSave("viewing/constellation_labels_fade_duration", duration);
		emit namesFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getLabelsFadeDuration() const
{
	return namesFadeDuration;
}

void ConstellationMgr::setFlagLines(const bool displayed)
{
	if(linesDisplayed != displayed)
	{
		linesDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto& constellation : selected)
			{
				constellation->setFlagLines(linesDisplayed);
			}
		}
		else
		{
			for (auto& constellation : constellations)
			{
				constellation->setFlagLines(linesDisplayed);
			}
		}
		StelApp::immediateSave("viewing/flag_constellation_drawing", displayed);
		emit linesDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagLines(void) const
{
	return linesDisplayed;
}

void ConstellationMgr::setFlagBoundaries(const bool displayed)
{
	if (boundariesDisplayed != displayed)
	{
		boundariesDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto& constellation : selected)
			{
				constellation->setFlagBoundaries(boundariesDisplayed);
			}
		}
		else
		{
			for (auto& constellation : constellations)
			{
				constellation->setFlagBoundaries(boundariesDisplayed);
			}
		}
		StelApp::immediateSave("viewing/flag_constellation_boundaries", displayed);
		emit boundariesDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagBoundaries(void) const
{
	return boundariesDisplayed;
}

void ConstellationMgr::setFlagHulls(const bool displayed)
{
	if (hullsDisplayed != displayed)
	{
		hullsDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto& constellation : selected)
			{
				constellation->setFlagHull(hullsDisplayed);
			}
		}
		else
		{
			for (auto& constellation : constellations)
			{
				constellation->setFlagHull(hullsDisplayed);
			}
		}
		StelApp::immediateSave("viewing/flag_constellation_hulls", displayed);
		emit hullsDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagHulls(void) const
{
	return hullsDisplayed;
}

void ConstellationMgr::setFlagZodiac(const bool displayed)
{
	if (zodiacFader != displayed)
	{
		zodiacFader = displayed;
		StelApp::immediateSave("viewing/flag_skyculture_zodiac", displayed);
		emit zodiacDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagZodiac(void) const
{
	return zodiacFader;
}

void ConstellationMgr::setFlagLunarSystem(const bool displayed)
{
	if (lunarSystemFader != displayed)
	{
		lunarSystemFader = displayed;
		StelApp::immediateSave("viewing/flag_skyculture_lunarsystem", displayed);
		emit lunarSystemDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagLunarSystem(void) const
{
	return lunarSystemFader;
}

void ConstellationMgr::setFlagArt(const bool displayed)
{
	if (artDisplayed != displayed)
	{
		artDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto& constellation : selected)
			{
				constellation->setFlagArt(artDisplayed);
			}
		}
		else
		{
			for (auto& constellation : constellations)
			{
				constellation->setFlagArt(artDisplayed);
			}
		}
		StelApp::immediateSave("viewing/flag_constellation_art", displayed);
		emit artDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagArt(void) const
{
	return artDisplayed;
}

void ConstellationMgr::setFlagLabels(const bool displayed)
{
	if (namesDisplayed != displayed)
	{
		namesDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto& constellation : selected)
				constellation->setFlagLabels(namesDisplayed);
		}
		else
		{
			for (auto& constellation : constellations)
				constellation->setFlagLabels(namesDisplayed);
		}
		StelApp::immediateSave("viewing/flag_constellation_name", displayed);
		emit namesDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagLabels(void) const
{
	return namesDisplayed;
}

void ConstellationMgr::setFlagIsolateSelected(const bool isolate)
{
	if (isolateSelected != isolate)
	{
		isolateSelected = isolate;

		// when turning off isolated selection mode, clear existing isolated selections.
		if (!isolateSelected)
		{
			for (auto& constellation : constellations)
			{
				constellation->setFlagLines(getFlagLines());
				constellation->setFlagLabels(getFlagLabels());
				constellation->setFlagArt(getFlagArt());
				constellation->setFlagBoundaries(getFlagBoundaries());
				constellation->setFlagHull(getFlagHulls());
			}
		}
		StelApp::immediateSave("viewing/flag_constellation_isolate_selected", isolate);
		emit isolateSelectedChanged(isolate);
	}
}

bool ConstellationMgr::getFlagIsolateSelected(void) const
{
	return isolateSelected;
}

void ConstellationMgr::setFlagConstellationPick(const bool mode)
{
	StelApp::immediateSave("viewing/flag_constellation_pick", mode);
	flagConstellationPick = mode;
	emit flagConstellationPickChanged(mode);
}

bool ConstellationMgr::getFlagConstellationPick(void) const
{
	return flagConstellationPick;
}

QList<Constellation *> ConstellationMgr::getSelected(void) const
{
	return selected;
}

void ConstellationMgr::setSelected(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != nullptr)
	{
		QList<Constellation *>cList;
		cList.append(c);
		setSelectedConst(cList);
	}
}

StelObjectP ConstellationMgr::setSelectedStar(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != nullptr)
	{
		QList<Constellation *>cList;
		cList.append(c);
		setSelectedConst(cList);
		return c->getBrightestStarInConstellation();
	}
	return nullptr;
}

void ConstellationMgr::setSelectedConst(QList<Constellation *> cList)
{
	// update states for other constellations to fade them out
	if (cList.length()>0)
	{
		selected.append(cList);

		if (isolateSelected)
		{
			if (!getFlagConstellationPick())
			{
				// Propagate current settings to newly selected constellation
				cList.at(0)->setFlagLines(getFlagLines());
				cList.at(0)->setFlagLabels(getFlagLabels());
				cList.at(0)->setFlagArt(getFlagArt());
				cList.at(0)->setFlagBoundaries(getFlagBoundaries());
				cList.at(0)->setFlagHull(getFlagHulls());

				for (auto& constellation : constellations)
				{
					bool match = false;
					for (auto& selected_constellation : selected)
					{
						if (constellation == selected_constellation)
						{
							match=true; // this is a selected constellation
							break;
						}
					}

					if(!match)
					{
						// Not selected constellation
						constellation->setFlagLines(false);
						constellation->setFlagLabels(false);
						constellation->setFlagArt(false);
						constellation->setFlagBoundaries(false);
						constellation->setFlagHull(false);
					}
				}
			}
			else
			{
				for (auto& constellation : constellations)
				{
					constellation->setFlagLines(false);
					constellation->setFlagLabels(false);
					constellation->setFlagArt(false);
					constellation->setFlagBoundaries(false);
					constellation->setFlagHull(false);
				}

				// Propagate current settings to newly selected constellation
				cList.at(0)->setFlagLines(getFlagLines());
				cList.at(0)->setFlagLabels(getFlagLabels());
				cList.at(0)->setFlagArt(getFlagArt());
				cList.at(0)->setFlagBoundaries(getFlagBoundaries());
				cList.at(0)->setFlagHull(getFlagHulls());
			}

			Constellation::singleSelected = true;  // For boundaries
		}
		else
			Constellation::singleSelected = false; // For boundaries
	}
	else
	{
		if (selected.empty()) return;

		// Otherwise apply standard flags to all constellations
		for (auto& constellation : constellations)
		{
			constellation->setFlagLines(getFlagLines());
			constellation->setFlagLabels(getFlagLabels());
			constellation->setFlagArt(getFlagArt());
			constellation->setFlagBoundaries(getFlagBoundaries());
			constellation->setFlagHull(getFlagHulls());
		}

		// And remove all selections
		selected.clear();
	}
}

//! Remove a constellation from the selected constellation list
void ConstellationMgr::unsetSelectedConst(Constellation * c)
{
	if (c != nullptr)
	{
		for (auto iter = selected.begin(); iter != selected.end();)
		{
			if( (*iter)->getEnglishName() == c->getEnglishName() )
			{
				iter = selected.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		// If no longer any selection, restore all flags on all constellations
		if (selected.empty())
		{
			// Otherwise apply standard flags to all constellations
			for (auto& constellation : constellations)
			{
				constellation->setFlagLines(getFlagLines());
				constellation->setFlagLabels(getFlagLabels());
				constellation->setFlagArt(getFlagArt());
				constellation->setFlagBoundaries(getFlagBoundaries());
				constellation->setFlagHull(getFlagHulls());
			}

			Constellation::singleSelected = false; // For boundaries
		}
		else if(isolateSelected)
		{
			// No longer selected constellation
			c->setFlagLines(false);
			c->setFlagLabels(false);
			c->setFlagArt(false);
			c->setFlagBoundaries(false);
			c->setFlagHull(false);

			Constellation::singleSelected = true;  // For boundaries
		}
	}
}

bool ConstellationMgr::loadBoundaries(const QJsonArray& boundaryData, const QString& boundariesEpoch)
{
	// delete existing boundaries if any exist
	for (auto* segment : allBoundarySegments)
	{
		delete segment;
	}
	allBoundarySegments.clear();

	bool b1875 = false;
	bool customEdgeEpoch = false;
	Mat4d matCustomEdgeEpochToJ2000;
	if (boundariesEpoch.toUpper() == "B1875")
		b1875 = true;
	else if (boundariesEpoch.toUpper() != "J2000")
	{
		qInfo() << "Custom epoch for boundaries:" << boundariesEpoch;
		customEdgeEpoch = true;
	}
	if (customEdgeEpoch)
	{
		// Allow "Bxxxx.x", "Jxxxx.x", "JDjjjjjjjj.jjj" and pure doubles as JD
		bool ok=false;
		double boundariesEpochJD=StelUtils::J2000;

		if (boundariesEpoch.startsWith("JD", Qt::CaseInsensitive))
		{
			QString boundariesEpochStrV=boundariesEpoch.right(boundariesEpoch.length()-2);
			boundariesEpochJD=boundariesEpochStrV.toDouble(&ok); // pureJD
		}
		else if (boundariesEpoch.startsWith("B", Qt::CaseInsensitive))
		{
			QString boundariesEpochStrV=boundariesEpoch.right(boundariesEpoch.length()-1);
			double boundariesEpochY=boundariesEpochStrV.toDouble(&ok);
			if (ok)
			{
				boundariesEpochJD=StelUtils::getJDFromBesselianEpoch(boundariesEpochY);
			}
		}
		else if (boundariesEpoch.startsWith("J", Qt::CaseInsensitive))
		{
			QString boundariesEpochStrV=boundariesEpoch.right(boundariesEpoch.length()-1);
			double boundariesEpochY=boundariesEpochStrV.toDouble(&ok);
			if (ok)
			{
				boundariesEpochJD=StelUtils::getJDFromJulianEpoch(boundariesEpochY);
			}
		}
		else
			boundariesEpochJD=boundariesEpoch.toDouble(&ok); // pureJD
		if (ok)
		{
			// Create custom precession matrix for transformation of edges to J2000.0
			double epsEpoch, chiEpoch, omegaEpoch, psiEpoch;
			getPrecessionAnglesVondrak(boundariesEpochJD, &epsEpoch, &chiEpoch, &omegaEpoch, &psiEpoch);
			matCustomEdgeEpochToJ2000 = Mat4d::xrotation(84381.406*1./3600.*M_PI/180.) * Mat4d::zrotation(-psiEpoch) * Mat4d::xrotation(-omegaEpoch) * Mat4d::zrotation(chiEpoch);
		}
		else
		{
			qWarning() << "SkyCultureMgr: Cannot parse edges_epoch =" << boundariesEpoch << ". Falling back to J2000.0";
			customEdgeEpoch = false;
		}
	}
	const StelCore& core = *StelApp::getInstance().getCore();
	qInfo().noquote() << "Loading constellation boundary data ... ";

	for (int n = 0; n < boundaryData.size(); ++n)
	{
		const QByteArray ba = boundaryData[n].toString().toLatin1();
		const char *cstr = ba.data();
		char edgeType, edgeDir;
		char dec1_sign, dec2_sign;
		int ra1_h, ra1_m, ra1_s, dec1_d, dec1_m, dec1_s;
		int ra2_h, ra2_m, ra2_s, dec2_d, dec2_m, dec2_s;
		char constellationNames[2][8];
		if (sscanf(cstr,
		           "%*s %c%c "
		           "%d:%d:%d %c%d:%d:%d "
		           "%d:%d:%d %c%d:%d:%d "
		           "%7s %7s",
		           &edgeType, &edgeDir,
		           &ra1_h, &ra1_m, &ra1_s,
		           &dec1_sign, &dec1_d, &dec1_m, &dec1_s,
		           &ra2_h, &ra2_m, &ra2_s,
		           &dec2_sign, &dec2_d, &dec2_m, &dec2_s,
		           constellationNames[0], constellationNames[1]) != 18)
		{
			qWarning().nospace() << "Failed to parse skyculture boundary line: \"" << cstr << "\"";
			continue;
		}

		constexpr double timeSecToRadians = M_PI / (12 * 3600);
		double RA1 = (60. * (60. * ra1_h + ra1_m) + ra1_s) * timeSecToRadians;
		double RA2 = (60. * (60. * ra2_h + ra2_m) + ra2_s) * timeSecToRadians;
		constexpr double angleSecToRad = M_PI / (180 * 3600);
		double DE1 = (60. * (60. * dec1_d + dec1_m) + dec1_s) * (dec1_sign=='-' ? -1 : 1) * angleSecToRad;
		double DE2 = (60. * (60. * dec2_d + dec2_m) + dec2_s) * (dec2_sign=='-' ? -1 : 1) * angleSecToRad;

		Vec3d xyz1;
		StelUtils::spheToRect(RA1,DE1,xyz1);
		Vec3d xyz2;
		StelUtils::spheToRect(RA2,DE2,xyz2);

		const auto points = new std::vector<Vec3d>;

		const bool edgeTypeDirValid = ((edgeType == 'P' || edgeType == 'M') && (edgeDir == '+' || edgeDir == '-')) ||
		                               (edgeType == '_' && edgeDir == '_');
		if (!edgeTypeDirValid)
		{
			qWarning().nospace() << "Bad edge type/direction: must be [PM_][-+_], but is " << QString("%1%2").arg(edgeType).arg(edgeDir)
			                     << ". Will not interpolate the edges.";
			edgeType = '_';
			edgeDir = '_';
		}

		double stepRA = 0, stepDE = 0;
		int numPoints = 2;
		switch (edgeType)
		{
		case 'P': // RA is variable
			if (RA2 > RA1 && edgeDir == '-')
				RA1 += 2*M_PI;
			if (RA2 < RA1 && edgeDir == '+')
				RA1 -= 2*M_PI;
			numPoints = 2 + std::ceil(std::abs(RA1 - RA2) / (M_PI / 64));
			stepRA = (RA2 - RA1) / (numPoints - 1);
			break;
		case 'M': // DE is variable
			if (DE2 > DE1 && edgeDir == '-')
				DE1 += 2*M_PI;
			if (DE2 < DE1 && edgeDir == '+')
				DE1 -= 2*M_PI;
			numPoints = 2 + std::ceil(std::abs(DE1 - DE2) / (M_PI / 64));
			stepDE = (DE2 - DE1) / (numPoints - 1);
			break;
		default: // No interpolation
			stepRA = RA2 - RA1;
			stepDE = DE2 - DE1;
			break;
		}

		for (int n = 0; n < numPoints; ++n)
		{
			const double RA = RA1 + n * stepRA;
			const double DE = DE1 + n * stepDE;
			Vec3d xyz;
			StelUtils::spheToRect(RA,DE,xyz);
			if (b1875) xyz = core.j1875ToJ2000(xyz);
			else if (customEdgeEpoch)
				xyz = matCustomEdgeEpochToJ2000*xyz;
			points->push_back(xyz);
		}

		Constellation *cons = nullptr;
		for (QString consName : constellationNames)
		{
			// not used?
			if (consName == "SER1" || consName == "SER2") consName = "SER";

			cons = findFromAbbreviation(consName);
			if (!cons)
				qWarning() << "ERROR while processing boundary file - cannot find constellation:" << consName;
			else
				cons->isolatedBoundarySegments.push_back(points);
		}

		if (cons)
		{
			cons->sharedBoundarySegments.push_back(points);
			// this list is for the de-allocation
			allBoundarySegments.push_back(points);
		}
		else
		{
			delete points;
		}
	}
	qInfo().noquote() << "Loaded" << boundaryData.size() << "constellation boundary segments";

	return true;
}

void ConstellationMgr::drawBoundaries(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	const float scale = sPainter.getProjector()->getScreenScale();

	sPainter.setBlending(false);
	if (boundariesThickness>1 || scale>1.f)
		sPainter.setLineWidth(boundariesThickness*scale); // set line thickness
	sPainter.setLineSmooth(true);
	for (auto* constellation : constellations)
	{
		constellation->drawBoundaryOptim(sPainter, obsVelocity);
	}
	if (boundariesThickness>1 || scale>1.f)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

void ConstellationMgr::drawHulls(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	const float scale = sPainter.getProjector()->getScreenScale();

	sPainter.setBlending(false);
	if (hullsThickness>1 || scale>1.f)
		sPainter.setLineWidth(hullsThickness*scale); // set line thickness
	sPainter.setLineSmooth(true);
	for (auto* constellation : constellations)
	{
		constellation->drawHullOptim(sPainter, obsVelocity);
	}
	if (hullsThickness>1 || scale>1.f)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw the zodiac, if any is defined in the current skyculture.
// @param obsVelocity is the speed vector of the observer planet to distort zodiac lines by aberration.
void ConstellationMgr::drawZodiac(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	if (!zodiac || zodiacFader.getInterstate()==0.)
		return;

	sPainter.setColor(zodiacColor, zodiacFader.getInterstate());
	zodiac->centerLine->setColor(zodiacColor);
	zodiac->centerLine->setLineThickness(zodiacThickness);
	zodiac->centerLine->setPartThickness(zodiacThickness);
	sPainter.setBlending(true);
	const float oldLineWidth=sPainter.getLineWidth();
	sPainter.setLineWidth(zodiacThickness);
	sPainter.setLineSmooth(true);
	zodiac->draw(sPainter, obsVelocity);
	sPainter.setLineWidth(oldLineWidth); // restore line thickness
}
// Draw the lunar system lines, if any is defined in the current skyculture.
// @param obsVelocity is the speed vector of the observer planet to distort lunarSystem lines by aberration.
void ConstellationMgr::drawLunarSystem(StelPainter& sPainter, const Vec3d &obsVelocity) const
{
	if (!lunarSystem || lunarSystemFader.getInterstate()==0.)
		return;

	sPainter.setColor(lunarSystemColor, lunarSystemFader.getInterstate());
	lunarSystem->centerLine->setColor(lunarSystemColor);
	lunarSystem->centerLine->setLineThickness(lunarSystemThickness);
	lunarSystem->centerLine->setPartThickness(lunarSystemThickness);
	sPainter.setBlending(true);
	const float oldLineWidth=sPainter.getLineWidth();
	sPainter.setLineWidth(lunarSystemThickness);
	sPainter.setLineSmooth(true);
	lunarSystem->draw(sPainter, obsVelocity);
	sPainter.setLineWidth(oldLineWidth); // restore line thickness
}

//! Returns the translated name of the Zodiac system
QString ConstellationMgr::getZodiacSystemName() const
{
	if (zodiac)
		return zodiac->getCulturalName();
	else
		return QString();
}
//! Returns the translated name of the Lunar system
QString ConstellationMgr::getLunarSystemName() const
{
	if (lunarSystem)
		return lunarSystem->getCulturalName();
	else
		return QString();
}
//! Return longitude in the culture's zodiacal longitudes
QString ConstellationMgr::getZodiacCoordinate(Vec3d eqNow) const
{
	if (zodiac)
		return zodiac->getLongitudeCoordinate(eqNow);
	else
		return QString();
}
//! Return lunar station in the culture's Lunar system
QString ConstellationMgr::getLunarSystemCoordinate(Vec3d eqNow) const
{
	if (lunarSystem)
		return lunarSystem->getLongitudeCoordinate(eqNow);
	else
		return QString();
}

StelObjectP ConstellationMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString nameI18nUpper = nameI18n.toUpper();

	for (auto* constellation : constellations)
	{
		if (constellation->culturalName.translatedI18n.toUpper() == nameI18nUpper) return constellation;
		if (constellation->culturalName.pronounceI18n.toUpper()  == nameI18nUpper) return constellation;
	}
	return nullptr;
}

StelObjectP ConstellationMgr::searchByName(const QString& name) const
{
	QString nameUpper = name.toUpper();
	for (auto* constellation : constellations)
	{
		if (constellation->culturalName.translated.toUpper()      == nameUpper) return constellation;
		if (constellation->culturalName.native.toUpper()          == nameUpper) return constellation;
		if (constellation->culturalName.pronounce.toUpper()       == nameUpper) return constellation;
		if (constellation->culturalName.transliteration.toUpper() == nameUpper) return constellation;
		if (constellation->abbreviation.toUpper()                 == nameUpper) return constellation;
	}
	return nullptr;
}

StelObjectP ConstellationMgr::searchByID(const QString &id) const
{
	for (auto* constellation : constellations)
	{
		if (constellation->getID() == id) return constellation;
	}
	return nullptr;
}

QStringList ConstellationMgr::listAllObjects(bool inEnglish) const
{
	// TODO: This is needed for the search dialog.
	QStringList result;
	if (inEnglish)
	{
		for (auto* constellation : constellations)
		{
			result << constellation->getEnglishName();
			result << constellation->culturalName.pronounce;
			result << constellation->culturalName.transliteration;
			result << constellation->culturalName.native;
		}
	}
	else
	{
		for (auto* constellation : constellations)
		{
			result << constellation->getNameI18n();
			result << constellation->culturalName.pronounceI18n;
			result << constellation->culturalName.native;
		}
	}
	result.removeDuplicates();
	result.removeOne(QString(""));
	result.removeOne(QString());
	return result;
}

QString ConstellationMgr::getStelObjectType() const
{
	return Constellation::CONSTELLATION_TYPE;
}

// For modern... SCs (with IAU borders), this can just identify IAU constellations.
// For others: identify from convex hulls.
QList<StelObjectP> ConstellationMgr::searchAround(const Vec3d& v, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;

	if (StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureBoundariesType()==StelSkyCulture::BoundariesType::IAU)
	{
		QString cName=core->getIAUConstellation(v);
		for (auto* constellation : std::as_const(constellations))
		{
			if (constellation->getShortName()==cName)
			{
				result.append(constellation);
			}
		}
#ifndef NDEBUG
		if (result.isEmpty())
			qDebug() << "ConstellationMgr::searchAround(): The IAU constellation" << cName << "was not found";
#endif
	}
	else
	{
	for (auto* constellation : constellations)
	{
		if (constellation->convexHull && constellation->convexHull->contains(v))
		{
#ifndef NDEBUG
			qDebug() << "ConstellationMgr::searchAround(): point in hull of constellation" << constellation->getID();
#endif
			result.append(constellation);
		}
	}
#ifndef NDEBUG
	qDebug() << "Point within hulls of" << result.count() << "constellations";
#endif
	}
	return result;
}

void ConstellationMgr::setSelected(const StelObject *s)
{
	if (!s)
		setSelectedConst(QList<Constellation*>()); // actually deselect...
	else
		setSelectedConst(isObjectIn(s, !(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureBoundariesType()==StelSkyCulture::BoundariesType::IAU)));
}

// Return a QList<Constellation*>, will allow result from overlapping hulls
QList<Constellation*> ConstellationMgr::isObjectIn(const StelObject *s, bool useHull) const
{
	StelCore *core = StelApp::getInstance().getCore();
	QList<Constellation*> result;
	if (useHull)
	{
		for (auto* constellation : constellations)
		{
			Vec3d pos=s->getJ2000EquatorialPos(core);
			pos.normalize();
			if (constellation->convexHull->contains(pos))
				result.append(constellation);
			else foreach(auto &obj, constellation->constellation)
			{
				// A problem persisted in selecting a star defining the hull which was then found just not to be contained!
				if (obj->getID() == s->getID())
				{
					result.append(constellation);
					break;
				}
			}
			// finally also test the hull outliers...
			foreach(auto &obj, constellation->hullExtension)
			{
				if (obj->getID() == s->getID())
				{
					result.append(constellation);
					break;
				}
			}
			// N.B. dark constellations: not forgotten, just not that critically defined around stars!
		}
	}
	else
	{
		QString IAUConstUpper = core->getIAUConstellation(s->getEquinoxEquatorialPos(core)).toUpper();
		for (auto* constellation : constellations)
		{
			// Check if the object is in the constellation
			if (constellation->getShortName().toUpper() == IAUConstUpper)
				result.append(constellation);
		}
	}
	return result;
}

void ConstellationMgr::outputHullAreas(const QString &fileNamePrefix) const
{
	const QString scName=GETSTELMODULE(StelSkyCultureMgr)->getCurrentSkyCultureEnglishName();

	QString fileName=StelFileMgr::getUserDir() + QString("/%1_%2.csv").arg(fileNamePrefix, scName);
	QFile file(fileName);
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	if (file.open(QIODevice::Text | QIODevice::WriteOnly))
#else
	if (file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
#endif
	{
		qInfo().nospace() << "Writing to:" << fileName;
		file.write(QString("ID, Native, English, Translated, Area (sr), Area (sqdeg)\n").toLatin1());
		foreach(const Constellation *constellation, constellations)
		{
			double area_sr=constellation->convexHull->getArea();
			file.write(QString("%1, %2, %3, %4, %5, %6\n").arg(constellation->getID(),
									   constellation->getNameNative(),
									   constellation->getEnglishName(),
									   constellation->getNameI18n(),
									   QString::number(area_sr, 'f', 6),
									   QString::number(area_sr*(M_180_PI*M_180_PI), 'f', 6)).toLatin1());
		}
	}
	else
	{
		qCritical() << "Cannot open file for writing! Output to logfile:";
		foreach(const Constellation *constellation, constellations)
		{
			double area_sr=constellation->convexHull->getArea();
			qInfo().nospace() << constellation->getEnglishName() << ": "
					  <<  area_sr << "sr or " << area_sr*(M_180_PI*M_180_PI) << "°²";
		}
	}
}

//! Create a list of stars within the convex hull of constellation
void ConstellationMgr::starsInHullOf(const QString &englishName, const bool hipOnly, const float maxMag, const QString &fileNamePrefix) const
{
	static StelCore *core=StelApp::getInstance().getCore();
	StelObjectP constell=searchByName(englishName);
	if (!constell)
	{
		qWarning() << "Constellation" << englishName << "not found, not creating star list";
		return;
	}

	QList<StelObjectP> starList=GETSTELMODULE(StarMgr)->searchWithin(constell->getRegion(), core, hipOnly, maxMag);

	// Add the actual hull-defining stars, but only if they are not already included. Unfortunately, we need to test via IDs!
	// Results are prepended, so that usually hull-defining HIP stars are in the beginning of the list.
	Constellation* constel=reinterpret_cast<Constellation*>(constell.data());
	foreach(auto &star, constel->constellation)
	{
		QString id=star->getID();
		bool wanted=true;
		foreach (auto obj, starList)
			if (obj->getID()==id)
			{
				wanted=false;
				break;
			}
		if (wanted)
			starList.prepend(star);
	}
	foreach(auto &star, constel->hullExtension)
	{
		QString id=star->getID();
		bool wanted=true;
		foreach (auto obj, starList)
			if (obj->getID()==id)
			{
				wanted=false;
				break;
			}
		if (wanted)
			starList.prepend(star);
	}

	qInfo().nospace() << starList.length() << " stars within the convex hull (" << constell->getRegion()->getArea()*(M_180_PI*M_180_PI) << " sq degrees) of " << englishName;
	int day, month, year;
	StelUtils::getDateFromJulianDay(core->getJD(), &year, &month, &day);
	const double yearFraction=StelUtils::yearFraction(year, month, day);
	const QString dateString=QString::number(yearFraction, 'f', 1);
	const QString scName=GETSTELMODULE(StelSkyCultureMgr)->getCurrentSkyCultureEnglishName();

	QString fileName=StelFileMgr::getUserDir() + QString("/%1_%2_%3-%4.csv").arg(fileNamePrefix, scName, englishName, QString::number(maxMag, 'f', 2));
	QFile file(fileName);
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	if (file.open(QIODevice::Text | QIODevice::WriteOnly))
#else
	if (file.open(QIODeviceBase::Text | QIODeviceBase::WriteOnly))
#endif
	{
		qInfo().nospace() << "Writing to:" << fileName;
		file.write(QString("ID, mag, RA_J%1, DE_J%1, RA_J%1(h), DE_J%1(deg)\n").arg(dateString).toLatin1());

		foreach(const auto &star, starList)
		{
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, star->getEquinoxEquatorialPos(core));
			file.write(QString("%1, %2, %3, %4, %5, %6\n").arg(star->getID(), QString::number(star->getVMagnitude(core), 'f', 2),
				   StelUtils::radToHmsStr(ra), StelUtils::decDegToDmsStr(dec*M_180_PI),
				   QString::number(ra*M_180_PI / 15., 'f', 6), QString::number(dec*M_180_PI, 'f', 6)).toLatin1());
		}
		file.close();
	}
	else
	{
		qCritical() << "Cannot open file for writing! Dump to logfile:";
		foreach(const auto &star, starList)
		{
			double ra, dec;
			StelUtils::rectToSphe(&ra, &dec, star->getEquinoxEquatorialPos(core));
			qInfo().nospace().noquote() << star->getID() << ", " << star->getVMagnitude(core) << ", " <<
						       StelUtils::radToHmsStr(ra) << ", " << StelUtils::decDegToDmsStr(dec*M_180_PI) <<
						       QString::number(ra*M_180_PI / 15., 'f', 6) << ", " << QString::number(dec*M_180_PI, 'f', 6);
		}
	}
}

void ConstellationMgr::recreateHulls()
{
	for (auto* constellation : std::as_const(constellations))
		constellation->makeConvexHull();
}
