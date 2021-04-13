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
#include "StelSkyDrawer.hpp"
#include "SolarSystem.hpp"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QDir>

using namespace std;

// constructor which loads all data from appropriate files
ConstellationMgr::ConstellationMgr(StarMgr *_hip_stars)
	: hipStarMgr(_hip_stars),
	  isolateSelected(false),
	  constellationPickEnabled(false),
	  constellationDisplayStyle(ConstellationMgr::constellationsTranslated),
	  artFadeDuration(2.),
	  artIntensity(0),
	  artIntensityMinimumFov(1.0),
	  artIntensityMaximumFov(2.0),
	  artDisplayed(0),
	  boundariesDisplayed(0),
	  linesDisplayed(0),
	  namesDisplayed(0),
	  checkLoadingData(false),
	  constellationLineThickness(1),
	  constellationBoundariesThickness(1)
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

	lastLoadedSkyCulture = "dummy";
	asterFont.setPixelSize(conf->value("viewing/constellation_font_size", 15).toInt());
	setFlagLines(conf->value("viewing/flag_constellation_drawing").toBool());
	setFlagLabels(conf->value("viewing/flag_constellation_name").toBool());
	setFlagBoundaries(conf->value("viewing/flag_constellation_boundaries",false).toBool());	
	setArtIntensity(conf->value("viewing/constellation_art_intensity", 0.5f).toFloat());
	setArtFadeDuration(conf->value("viewing/constellation_art_fade_duration",2.f).toFloat());
	setFlagArt(conf->value("viewing/flag_constellation_art").toBool());
	setFlagIsolateSelected(conf->value("viewing/flag_constellation_isolate_selected", false).toBool());
	setFlagConstellationPick(conf->value("viewing/flag_constellation_pick", false).toBool());
	setConstellationLineThickness(conf->value("viewing/constellation_line_thickness", 1).toInt());
	setConstellationBoundariesThickness(conf->value("viewing/constellation_boundaries_thickness", 1).toInt());
	// The setting for developers
	setFlagCheckLoadingData(conf->value("devel/check_loading_constellation_data","false").toBool());

	QString starloreDisplayStyle=conf->value("viewing/constellation_name_style", "translated").toString();
	static const QMap<QString, ConstellationDisplayStyle>map={
		{ "translated",  constellationsTranslated},
		{ "native",      constellationsNative},
		{ "abbreviated", constellationsAbbreviated},
		{ "english",     constellationsEnglish}};
	if (!map.contains(starloreDisplayStyle))
	{
		qDebug() << "Warning: viewing/constellation_name_style (" << starloreDisplayStyle << ") invalid. Using translated style.";
		conf->setValue("viewing/constellation_name_style", "translated");
	}
	setConstellationDisplayStyle(map.value(starloreDisplayStyle, constellationsTranslated));

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setLinesColor(Vec3f(conf->value("color/const_lines_color", defaultColor).toString()));
	setBoundariesColor(Vec3f(conf->value("color/const_boundary_color", "0.8,0.3,0.3").toString()));
	setLabelsColor(Vec3f(conf->value("color/const_names_color", defaultColor).toString()));

	StelObjectMgr *objectManager = GETSTELMODULE(StelObjectMgr);
	objectManager->registerStelObjectMgr(this);
	connect(objectManager, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),
			this, SLOT(selectedObjectChange(StelModule::StelModuleSelectAction)));
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(updateSkyCulture(const QString&)));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Constellation_Lines", displayGroup, N_("Constellation lines"), "linesDisplayed", "C");
	addAction("actionShow_Constellation_Art", displayGroup, N_("Constellation art"), "artDisplayed", "R");
	addAction("actionShow_Constellation_Labels", displayGroup, N_("Constellation labels"), "namesDisplayed", "V");
	addAction("actionShow_Constellation_Boundaries", displayGroup, N_("Constellation boundaries"), "boundariesDisplayed", "B");
	addAction("actionShow_Constellation_Isolated", displayGroup, N_("Select single constellation"), "isolateSelected"); // no shortcut, sync with GUI
	addAction("actionShow_Constellation_Deselect", displayGroup, N_("Remove selection of constellations"), this, "deselectConstellations()", "W");
	addAction("actionShow_Constellation_Select", displayGroup, N_("Select all constellations"), this, "selectAllConstellations()", "Alt+W");
	// Reload the current sky culture
	addAction("actionShow_Starlore_Reload", displayGroup, N_("Reload the sky culture"), this, "reloadSkyCulture()", "Ctrl+Alt+I");
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
	updateSkyCulture(StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureID());
}

void ConstellationMgr::updateSkyCulture(const QString& skyCultureDir)
{
	// Check if the sky culture changed since last load, if not don't load anything
	if (lastLoadedSkyCulture == skyCultureDir)
		return;

	// Find constellation art.  If this doesn't exist, warn, but continue using ""
	// the loadLinesAndArt function knows how to handle this (just loads lines).
	QString conArtFile = StelFileMgr::findFile("skycultures/"+skyCultureDir+"/constellationsart.fab");
	if (conArtFile.isEmpty())
	{
		qDebug() << "No constellationsart.fab file found for sky culture dir" << QDir::toNativeSeparators(skyCultureDir);
	}

	// first of all, remove constellations from the list of selected objects in StelObjectMgr, since we are going to delete them
	deselectConstellations();

	QString fic = StelFileMgr::findFile("skycultures/"+skyCultureDir+"/constellationship.fab");
	if (fic.isEmpty())
		qWarning() << "ERROR loading constellation lines and art from file: " << fic;
	else
		loadLinesAndArt(fic, conArtFile, skyCultureDir);

	// load constellation names
	fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellation_names.eng.fab");
	if (fic.isEmpty())
		qWarning() << "ERROR loading constellation names from file: " << fic;
	else
		loadNames(fic);

	// load seasonal rules
	loadSeasonalRules(StelFileMgr::findFile("skycultures/" + skyCultureDir + "/seasonal_rules.fab"));

	// Translate constellation names for the new sky culture
	updateI18n();

	// load constellation boundaries
	StelApp *app = &StelApp::getInstance();
	int idx = app->getSkyCultureMgr().getCurrentSkyCultureBoundariesIdx();
	if (idx>=0)
	{
		// OK, the current sky culture has boundaries!
		if (idx==1)
		{
			// boundaries = own
			fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellation_boundaries.dat");
			if (fic.isEmpty()) // Check old file name (backward compatibility)
				fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellations_boundaries.dat");
		}
		else
		{
			// boundaries = generic
			fic = StelFileMgr::findFile("data/constellation_boundaries.dat");
		}

		if (fic.isEmpty())
			qWarning() << "ERROR loading constellation boundaries file: " << fic;
		else
			loadBoundaries(fic);
	}

	lastLoadedSkyCulture = skyCultureDir;

	if (getFlagCheckLoadingData())
	{
		int i = 1;
		for (auto* constellation : constellations)
		{
			qWarning() << "[Constellation] #" << i << " abbr:" << constellation->abbreviation << " name:" << constellation->getEnglishName() << " segments:" << constellation->numberOfSegments;
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
		// setSelected(Q_NULLPTR);
		return;
	}

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
			setSelectedConst(static_cast<Constellation *>(newSelectedConst[0].data()));
		}
	}
	else
	{
		QList<StelObjectP> newSelectedObject;
		if (StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureBoundariesIdx()==0) // generic IAU boundaries
			newSelectedObject = omgr->getSelectedObject();
		else
			newSelectedObject = omgr->getSelectedObject("Star");

		if (!newSelectedObject.empty())
		{
			setSelected(newSelectedObject[0].data());
		}
		else
		{
			setSelected(Q_NULLPTR);
		}
	}
}

void ConstellationMgr::deselectConstellations(void)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	if (getFlagIsolateSelected())
	{
		// The list of selected constellations is empty, but...
		if (selected.size()==0)
		{
			// ...let's unselect all constellations for guarantee
			for (auto* constellation : constellations)
			{
				constellation->setFlagLines(false);
				constellation->setFlagLabels(false);
				constellation->setFlagArt(false);
				constellation->setFlagBoundaries(false);
			}
		}

		// If any constellation is selected at the moment, then let's do not touch to it!
		if (omgr->getWasSelected() && selected.size()>0)
			selected.pop_back();

		// Let's hide all previously selected constellations
		for (auto* constellation : selected)
		{
			constellation->setFlagLines(false);
			constellation->setFlagLabels(false);
			constellation->setFlagArt(false);
			constellation->setFlagBoundaries(false);
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
	for (auto* constellation : constellations)
	{
		setSelectedConst(constellation);
	}
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

void ConstellationMgr::setFontSize(const float newFontSize)
{
	if (asterFont.pixelSize() - newFontSize != 0.0f)
	{
		asterFont.setPixelSize(static_cast<int>(newFontSize));
		emit fontSizeChanged(newFontSize);
	}
}

float ConstellationMgr::getFontSize() const
{
	return asterFont.pixelSize();
}

void ConstellationMgr::setConstellationDisplayStyle(ConstellationDisplayStyle style)
{
	if(style!=constellationDisplayStyle)
	{
		constellationDisplayStyle=style;
		if (constellationDisplayStyle==constellationsTranslated)
			GETSTELMODULE(SolarSystem)->setFlagTranslatedNames(true);
		else
			GETSTELMODULE(SolarSystem)->setFlagTranslatedNames(false);

		emit constellationsDisplayStyleChanged(constellationDisplayStyle);
	}
}

QString ConstellationMgr::getConstellationDisplayStyleString(ConstellationDisplayStyle style)
{
	return (style == constellationsAbbreviated ? "abbreviated" : (style == constellationsNative ? "native" : "translated"));
}

ConstellationMgr::ConstellationDisplayStyle ConstellationMgr::getConstellationDisplayStyle()
{
	return constellationDisplayStyle;
}

void ConstellationMgr::setConstellationLineThickness(const int thickness)
{
	if(thickness!=constellationLineThickness)
	{
		constellationLineThickness = thickness;
		if (constellationLineThickness<=0) // The line can not be negative or zero thickness
			constellationLineThickness = 1;

		emit constellationLineThicknessChanged(thickness);
	}
}

void ConstellationMgr::setConstellationBoundariesThickness(const int thickness)
{
	if(thickness!=constellationBoundariesThickness)
	{
		constellationBoundariesThickness = thickness;
		if (constellationBoundariesThickness<=0) // The line can not be negative or zero thickness
			constellationBoundariesThickness = 1;

		emit constellationBoundariesThicknessChanged(thickness);
	}
}

void ConstellationMgr::loadLinesAndArt(const QString &fileName, const QString &artfileName, const QString& cultureName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation data file" << QDir::toNativeSeparators(fileName)  << "for culture" << cultureName;
		Q_ASSERT(0);
	}

	int totalRecords=0;
	QString record;
	QRegExp commentRx("^(\\s*#.*|\\s*)$"); // pure comment lines or empty lines
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}
	in.seek(0);

	// delete existing data, if any
	for (auto* constellation : constellations)
		delete constellation;

	constellations.clear();
	Constellation *cons = Q_NULLPTR;

	// read the file of line patterns, adding a record per non-comment line
	int currentLineNumber = 0;	// line in file
	int readOk = 0;			// count of records processed OK
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		currentLineNumber++;
		if (commentRx.exactMatch(record))
			continue;

		cons = new Constellation;
		if(cons->read(record, hipStarMgr))
		{
			cons->artOpacity = artIntensity;
			cons->artFader.setDuration(static_cast<int>(artFadeDuration * 1000.f));
			cons->setFlagArt(artDisplayed);
			cons->setFlagBoundaries(boundariesDisplayed);
			cons->setFlagLines(linesDisplayed);
			cons->setFlagLabels(namesDisplayed);
			constellations.push_back(cons);
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading constellation lines record at line " << currentLineNumber << "for culture" << cultureName;
			delete cons;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation records successfully for culture" << cultureName;

	// Set current states
	setFlagArt(artDisplayed);
	setFlagLines(linesDisplayed);
	setFlagLabels(namesDisplayed);
	setFlagBoundaries(boundariesDisplayed);

	// It's possible to have no art - just constellations
	if (artfileName.isNull() || artfileName.isEmpty())
		return;
	QFile fic(artfileName);
	if (!fic.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation art file" << QDir::toNativeSeparators(fileName)  << "for culture" << cultureName;
		return;
	}

	totalRecords=0;
	while (!fic.atEnd())
	{
		record = QString::fromUtf8(fic.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}
	fic.seek(0);

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	QString shortname;
	QString texfile;
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;

	currentLineNumber = 0;	// line in file
	readOk = 0;		// count of records processed OK

	while (!fic.atEnd())
	{
		++currentLineNumber;
		record = QString::fromUtf8(fic.readLine());
		if (commentRx.exactMatch(record))
			continue;

		// prevent leaving zeros on numbers from being interpretted as octal numbers
		record.replace(" 0", " ");
		QTextStream rStr(&record);
		rStr >> shortname >> texfile >> x1 >> y1 >> hp1 >> x2 >> y2 >> hp2 >> x3 >> y3 >> hp3;
		if (rStr.status()!=QTextStream::Ok)
		{
			qWarning() << "ERROR parsing constellation art record at line" << currentLineNumber << "of art file for culture" << cultureName;
			continue;
		}

		// Draw loading bar
// 		lb.SetMessage(q_("Loading Constellation Art: %1/%2").arg(currentLineNumber).arg(totalRecords));
// 		lb.Draw((float)(currentLineNumber)/totalRecords);

		cons = Q_NULLPTR;
		cons = findFromAbbreviation(shortname);
		if (!cons)
		{
			qWarning() << "ERROR in constellation art file at line" << currentLineNumber << "for culture" << cultureName
					   << "constellation" << shortname << "unknown";
		}
		else
		{
			QString texturePath = StelFileMgr::findFile("skycultures/"+cultureName+"/"+texfile);
			if (texturePath.isEmpty())
			{
				qWarning() << "ERROR: could not find texture, " << QDir::toNativeSeparators(texfile);
			}

			cons->artTexture = StelApp::getInstance().getTextureManager().createTextureThread(texturePath);

			int texSizeX = 0, texSizeY = 0;
			if (cons->artTexture==Q_NULLPTR || !cons->artTexture->getDimensions(texSizeX, texSizeY))
			{
				qWarning() << "Texture dimension not available";
			}

			StelCore* core = StelApp::getInstance().getCore();
			Vec3d s1 = hipStarMgr->searchHP(static_cast<int>(hp1))->getJ2000EquatorialPos(core);
			Vec3d s2 = hipStarMgr->searchHP(static_cast<int>(hp2))->getJ2000EquatorialPos(core);
			Vec3d s3 = hipStarMgr->searchHP(static_cast<int>(hp3))->getJ2000EquatorialPos(core);

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3d s4 = s1 + ((s2 - s1) ^ (s3 - s1));
			Mat4d B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
			Mat4d A(x1, texSizeY - static_cast<int>(y1), 0., 1., x2, texSizeY - static_cast<int>(y2), 0., 1., x3, texSizeY - static_cast<int>(y3), 0., 1., x1, texSizeY - static_cast<int>(y1), texSizeX, 1.);
			Mat4d X = B * A.inverse();

			// Tesselate on the plan assuming a tangential projection for the image
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
			for (const auto& v : texCoords)
				contour << X * Vec3d(static_cast<double>(v[0]) * texSizeX, static_cast<double>(v[1]) * texSizeY, 0.);

			cons->artPolygon.vertex=contour;
			cons->artPolygon.texCoords=texCoords;
			cons->artPolygon.primitiveType=StelVertexArray::Triangles;

			Vec3d tmp(X * Vec3d(0.5*texSizeX, 0.5*texSizeY, 0.));
			tmp.normalize();
			Vec3d tmp2(X * Vec3d(0., 0., 0.));
			tmp2.normalize();
			cons->boundingCap.n=tmp;
			cons->boundingCap.d=tmp*tmp2;
			++readOk;
		}
	}

	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation art records successfully for culture" << cultureName;
	fic.close();
}

void ConstellationMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(asterFont);
	drawLines(sPainter, core);
	drawNames(sPainter);
	drawArt(sPainter);
	drawBoundaries(sPainter);
}

// Draw constellations art textures
void ConstellationMgr::drawArt(StelPainter& sPainter) const
{
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	sPainter.setCullFace(true);

	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	for (auto* constellation : constellations)
	{
		constellation->drawArtOptim(sPainter, *region);
	}

	sPainter.setCullFace(false);
}

// Draw constellations lines
void ConstellationMgr::drawLines(StelPainter& sPainter, const StelCore* core) const
{
	sPainter.setBlending(true);
	if (constellationLineThickness>1)
		sPainter.setLineWidth(constellationLineThickness); // set line thickness
	sPainter.setLineSmooth(true);

	const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();
	for (auto* constellation : constellations)
	{
		constellation->drawOptim(sPainter, core, viewportHalfspace);
	}
	if (constellationLineThickness>1)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw the names of all the constellations
void ConstellationMgr::drawNames(StelPainter& sPainter) const
{
	sPainter.setBlending(true);
	for (auto* constellation : constellations)
	{
		// Check if in the field of view
		if (sPainter.getProjector()->projectCheck(constellation->XYZname, constellation->XYname))
			constellation->drawName(sPainter, constellationDisplayStyle);
	}
}

Constellation *ConstellationMgr::isStarIn(const StelObject* s) const
{
	for (auto* constellation : constellations)
	{
		// Check if the star is in one of the constellation
		if (constellation->isStarIn(s))
		{
			return constellation;
		}
	}
	return Q_NULLPTR;
}

Constellation* ConstellationMgr::findFromAbbreviation(const QString& abbreviation) const
{
	// search in uppercase only
	//QString tname = abbreviation.toUpper();

	for (auto* constellation : constellations)
	{
		//if (constellation->abbreviation.toUpper() == tname)
		if (constellation->abbreviation.compare(abbreviation, Qt::CaseInsensitive) == 0)
		{
			//if (constellation->abbreviation != abbreviation)
			//	qDebug() << "ConstellationMgr::findFromAbbreviation: not a perfect match, but sufficient:" << constellation->abbreviation << "vs." << abbreviation;
			return constellation;
		}
		//else qDebug() << "Comparison mismatch: " << abbreviation << "vs." << constellation->abbreviation;
	}
	return Q_NULLPTR;
}

// Can't find constellation from a position because it's not well localized
QList<StelObjectP> ConstellationMgr::searchAround(const Vec3d&, double, const StelCore*) const
{
	return QList<StelObjectP>();
}

void ConstellationMgr::loadNames(const QString& namesFile)
{
	// Constellation not loaded yet
	if (constellations.empty()) return;

	// clear previous names
	for (auto* constellation : constellations)
	{
		constellation->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	constellationsEnglishNames.clear();

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	// abbreviation is allowed to start with a dot to mark as "hidden".
	QRegExp recRx("^\\s*(\\.?\\w+)\\s+\"(.*)\"\\s+_[(]\"(.*)\"[)]\\s*(\\w*)\\n");
	QRegExp ctxRx("(.*)\",\\s*\"(.*)");

	// Some more variables to use in the parsing
	Constellation *aster;
	QString record, shortName, ctxt;

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!commonNameFile.atEnd())
	{
		record = QString::fromUtf8(commonNameFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;

		if (!recRx.exactMatch(record))
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in constellation names file" << QDir::toNativeSeparators(namesFile) << ":" << record;
		}
		else
		{
			shortName = recRx.cap(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != Q_NULLPTR)
			{
				aster->nativeName = recRx.cap(2);
				ctxt = recRx.cap(3);
				if (ctxRx.exactMatch(ctxt))
				{
					aster->englishName = ctxRx.cap(1);
					aster->context = ctxRx.cap(2);
				}
				else
				{
					aster->englishName = ctxt;
					aster->context = "";
				}
				readOk++;
				// Some skycultures already have empty nativeNames. Fill those.
				if (aster->nativeName.isEmpty())
					aster->nativeName=aster->englishName;

				constellationsEnglishNames << aster->englishName;
			}
			else
			{
				qWarning() << "WARNING - constellation abbreviation" << shortName << "not found when loading constellation names";
			}
		}
	}
	commonNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "constellation names";
}

QStringList ConstellationMgr::getConstellationsEnglishNames()
{
	return  constellationsEnglishNames;
}

void ConstellationMgr::loadSeasonalRules(const QString& rulesFile)
{
	// Constellation not loaded yet
	if (constellations.empty()) return;

	bool flag = true;
	if (rulesFile.isEmpty())
		flag = false;

	// clear previous rules
	for (auto* constellation : constellations)
	{
		constellation->beginSeason = 1;
		constellation->endSeason = 12;
		constellation->seasonalRuleEnabled = flag;
	}

	// Current starlore didn't support the seasonal rules
	if (!flag)
		return;

	// Open file
	QFile seasonalRulesFile(rulesFile);
	if (!seasonalRulesFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(rulesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	QRegExp recRx("^\\s*(\\w+)\\s+(\\w+)\\s+(\\w+)\\n");

	// Some more variables to use in the parsing
	Constellation *aster;
	QString record, shortName;

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!seasonalRulesFile.atEnd())
	{
		record = QString::fromUtf8(seasonalRulesFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.exactMatch(record))
			continue;

		totalRecords++;

		if (!recRx.exactMatch(record))
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in seasonal rules file" << QDir::toNativeSeparators(rulesFile);
		}
		else
		{
			shortName = recRx.cap(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != Q_NULLPTR)
			{
				aster->beginSeason = recRx.cap(2).toInt();
				aster->endSeason = recRx.cap(3).toInt();
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - constellation abbreviation" << shortName << "not found when loading seasonal rules for constellations";
			}
		}
	}
	seasonalRulesFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "seasonal rules";
}

void ConstellationMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (auto* constellation : constellations)
	{
		constellation->nameI18 = trans.qtranslate(constellation->englishName, constellation->context);
	}
}

// update faders
void ConstellationMgr::update(double deltaTime)
{
	//calculate FOV fade value, linear fade between artIntensityMaximumFov and artIntensityMinimumFov
	double fov = StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov();
	Constellation::artIntensityFovScale = qBound(0.0,(fov - artIntensityMinimumFov) / (artIntensityMaximumFov - artIntensityMinimumFov),1.0);

	const int delta = static_cast<int>(deltaTime*1000);
	for (auto* constellation : constellations)
	{
		constellation->update(delta);
	}
}

void ConstellationMgr::setArtIntensity(const float intensity)
{
	if ((artIntensity - intensity) != 0.0f)
	{
		artIntensity = intensity;

		for (auto* constellation : constellations)
		{
			constellation->artOpacity = artIntensity;
		}

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
	if (artFadeDuration != duration)
	{
		artFadeDuration = duration;

		for (auto* constellation : constellations)
		{
			constellation->artFader.setDuration(static_cast<int>(duration * 1000.f));
		}
		emit artFadeDurationChanged(duration);
	}
}

float ConstellationMgr::getArtFadeDuration() const
{
	return artFadeDuration;
}

void ConstellationMgr::setFlagLines(const bool displayed)
{
	if(linesDisplayed != displayed)
	{
		linesDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto* constellation : selected)
			{
				constellation->setFlagLines(linesDisplayed);
			}
		}
		else
		{
			for (auto* constellation : constellations)
			{
				constellation->setFlagLines(linesDisplayed);
			}
		}
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
			for (auto* constellation : selected)
			{
				constellation->setFlagBoundaries(boundariesDisplayed);
			}
		}
		else
		{
			for (auto* constellation : constellations)
			{
				constellation->setFlagBoundaries(boundariesDisplayed);
			}
		}
		emit boundariesDisplayedChanged(displayed);
	}
}

bool ConstellationMgr::getFlagBoundaries(void) const
{
	return boundariesDisplayed;
}

void ConstellationMgr::setFlagArt(const bool displayed)
{
	if (artDisplayed != displayed)
	{
		artDisplayed = displayed;
		if (!selected.empty() && isolateSelected)
		{
			for (auto* constellation : selected)
			{
				constellation->setFlagArt(artDisplayed);
			}
		}
		else
		{
			for (auto* constellation : constellations)
			{
				constellation->setFlagArt(artDisplayed);
			}
		}
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
			for (auto* constellation : selected)
				constellation->setFlagLabels(namesDisplayed);
		}
		else
		{
			for (auto* constellation : constellations)
				constellation->setFlagLabels(namesDisplayed);
		}
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

		// when turning off isolated selection mode, clear exisiting isolated selections.
		if (!isolateSelected)
		{
			for (auto* constellation : constellations)
			{
				constellation->setFlagLines(getFlagLines());
				constellation->setFlagLabels(getFlagLabels());
				constellation->setFlagArt(getFlagArt());
				constellation->setFlagBoundaries(getFlagBoundaries());
			}
		}
		emit isolateSelectedChanged(isolate);
	}
}

bool ConstellationMgr::getFlagIsolateSelected(void) const
{
	return isolateSelected;
}

void ConstellationMgr::setFlagConstellationPick(const bool mode)
{
	constellationPickEnabled = mode;
}

bool ConstellationMgr::getFlagConstellationPick(void) const
{
	return constellationPickEnabled;
}

StelObject* ConstellationMgr::getSelected(void) const
{
	return *selected.begin();  // TODO return all or just remove this method
}

void ConstellationMgr::setSelected(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != Q_NULLPTR) setSelectedConst(c);
}

StelObjectP ConstellationMgr::setSelectedStar(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != Q_NULLPTR)
	{
		setSelectedConst(c);
		return c->getBrightestStarInConstellation();
	}
	return Q_NULLPTR;
}

void ConstellationMgr::setSelectedConst(Constellation * c)
{
	// update states for other constellations to fade them out
	if (c != Q_NULLPTR)
	{
		selected.push_back(c);

		if (isolateSelected)
		{
			if (!getFlagConstellationPick())
			{
				// Propagate current settings to newly selected constellation
				c->setFlagLines(getFlagLines());
				c->setFlagLabels(getFlagLabels());
				c->setFlagArt(getFlagArt());
				c->setFlagBoundaries(getFlagBoundaries());

				for (auto* constellation : constellations)
				{
					bool match = false;
					for (auto* selected_constellation : selected)
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
					}
				}
			}
			else
			{
				for (auto* constellation : constellations)
				{
					constellation->setFlagLines(false);
					constellation->setFlagLabels(false);
					constellation->setFlagArt(false);
					constellation->setFlagBoundaries(false);
				}

				// Propagate current settings to newly selected constellation
				c->setFlagLines(getFlagLines());
				c->setFlagLabels(getFlagLabels());
				c->setFlagArt(getFlagArt());
				c->setFlagBoundaries(getFlagBoundaries());
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
		for (auto* constellation : constellations)
		{
			constellation->setFlagLines(getFlagLines());
			constellation->setFlagLabels(getFlagLabels());
			constellation->setFlagArt(getFlagArt());
			constellation->setFlagBoundaries(getFlagBoundaries());
		}

		// And remove all selections
		selected.clear();
	}
}

//! Remove a constellation from the selected constellation list
void ConstellationMgr::unsetSelectedConst(Constellation * c)
{
	if (c != Q_NULLPTR)
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
			for (auto* constellation : constellations)
			{
				constellation->setFlagLines(getFlagLines());
				constellation->setFlagLabels(getFlagLabels());
				constellation->setFlagArt(getFlagArt());
				constellation->setFlagBoundaries(getFlagBoundaries());
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

			Constellation::singleSelected = true;  // For boundaries
		}
	}
}

bool ConstellationMgr::loadBoundaries(const QString& boundaryFile)
{
	Constellation *cons = Q_NULLPTR;
	unsigned int i, j;

	// delete existing boundaries if any exist
	for (auto* segment : allBoundarySegments)
	{
		delete segment;
	}
	allBoundarySegments.clear();

	qDebug() << "Loading constellation boundary data ... ";

	// Modified boundary file by Torsten Bronger with permission
	// http://pp3.sourceforge.net
	QFile dataFile(boundaryFile);
	if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Boundary file " << QDir::toNativeSeparators(boundaryFile) << " not found";
		return false;
	}

	double DE, RA;
	Vec3d XYZ;
	unsigned num, numc;
	vector<Vec3d> *points = Q_NULLPTR;
	QString consname, record, data = "";
	i = 0;

	// Added support of comments for constellation_boundaries.dat file
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!dataFile.atEnd())
	{
		// Read the line
		record = QString::fromUtf8(dataFile.readLine());

		// Skip comments
		if (commentRx.exactMatch(record))
			continue;

		// Append the data
		data.append(record);
	}

	// Read and parse the data without comments
	QTextStream istr(&data);
	while (!istr.atEnd())
	{
		num = 0;
		istr >> num;
		if(num == 0)
			continue; // empty line

		points = new vector<Vec3d>;

		for (j=0;j<num;j++)
		{
			istr >> RA >> DE;

			RA*=M_PI/12.;     // Convert from hours to rad
			DE*=M_PI/180.;    // Convert from deg to rad

			// Calc the Cartesian coord with RA and DE
			StelUtils::spheToRect(RA,DE,XYZ);
			points->push_back(XYZ);
		}

		// this list is for the de-allocation
		allBoundarySegments.push_back(points);

		istr >> numc;
		// there are 2 constellations per boundary

		for (j=0;j<numc;j++)
		{
			istr >> consname;
			// not used?
			if (consname == "SER1" || consname == "SER2") consname = "SER";

			cons = findFromAbbreviation(consname);
			if (!cons)
				qWarning() << "ERROR while processing boundary file - cannot find constellation: " << consname;
			else
				cons->isolatedBoundarySegments.push_back(points);
		}

		if (cons)
		{
			cons->sharedBoundarySegments.push_back(points);
			points=Q_NULLPTR; // Avoid Coverity resource leak warning. (CID48925).
		}
		i++;
	}
	dataFile.close();
	qDebug() << "Loaded" << i << "constellation boundary segments";
	if (points)
	{
		delete points; // See if Coverity complains here? (CID48925).
		points=Q_NULLPTR;
	}

	return true;
}

void ConstellationMgr::drawBoundaries(StelPainter& sPainter) const
{
	sPainter.setBlending(false);
	if (constellationBoundariesThickness>1)
		sPainter.setLineWidth(constellationBoundariesThickness); // set line thickness
	sPainter.setLineSmooth(true);
	for (auto* constellation : constellations)
	{
		constellation->drawBoundaryOptim(sPainter);
	}
	if (constellationBoundariesThickness>1)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

StelObjectP ConstellationMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	for (auto* constellation : constellations)
	{
		QString objwcap = constellation->nameI18.toUpper();
		if (objwcap == objw) return constellation;
	}
	return Q_NULLPTR;
}

StelObjectP ConstellationMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	for (auto* constellation : constellations)
	{
		QString objwcap = constellation->englishName.toUpper();
		if (objwcap == objw) return constellation;

		objwcap = constellation->abbreviation.toUpper();
		if (objwcap == objw) return constellation;
	}
	return Q_NULLPTR;
}

StelObjectP ConstellationMgr::searchByID(const QString &id) const
{
	for (auto* constellation : constellations)
	{
		if (constellation->getID() == id) return constellation;
	}
	return Q_NULLPTR;
}

QStringList ConstellationMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (auto* constellation : constellations)
		{
			result << constellation->getEnglishName();
		}
	}
	else
	{
		for (auto* constellation : constellations)
		{
			result << constellation->getNameI18n();
		}
	}
	return result;
}

QString ConstellationMgr::getStelObjectType() const
{
	return Constellation::CONSTELLATION_TYPE;
}

void ConstellationMgr::setSelected(const StelObject *s)
{
	if (!s)
		setSelectedConst(Q_NULLPTR);
	else
	{
		if (StelApp::getInstance().getSkyCultureMgr().getCurrentSkyCultureBoundariesIdx()==0) // generic IAU boundaries
			setSelectedConst(isObjectIn(s));
		else
			setSelectedConst(isStarIn(s));
	}
}

Constellation* ConstellationMgr::isObjectIn(const StelObject *s) const
{
	StelCore *core = StelApp::getInstance().getCore();
	QString IAUConst = core->getIAUConstellation(s->getEquinoxEquatorialPos(core));
	for (auto* constellation : constellations)
	{
		// Check if the object is in the constellation
		if (constellation->getShortName().toUpper() == IAUConst.toUpper())
			return constellation;
	}
	return Q_NULLPTR;
}
