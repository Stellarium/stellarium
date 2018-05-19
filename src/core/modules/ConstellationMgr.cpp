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
	  constellationLineThickness(1)
{
	setObjectName("ConstellationMgr");
	Q_ASSERT(hipStarMgr);
}

ConstellationMgr::~ConstellationMgr()
{
	std::vector<Constellation *>::iterator iter;

	for (iter = constellations.begin(); iter != constellations.end(); iter++)
	{
		delete(*iter);
	}

	vector<vector<Vec3d> *>::iterator iter1;
	for (iter1 = allBoundarySegments.begin(); iter1 != allBoundarySegments.end(); ++iter1)
	{
		delete (*iter1);
	}
	allBoundarySegments.clear();
}

void ConstellationMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	lastLoadedSkyCulture = "dummy";
	asterFont.setPixelSize(conf->value("viewing/constellation_font_size", 14).toInt());
	setFlagLines(conf->value("viewing/flag_constellation_drawing").toBool());
	setFlagLabels(conf->value("viewing/flag_constellation_name").toBool());
	setFlagBoundaries(conf->value("viewing/flag_constellation_boundaries",false).toBool());	
	setArtIntensity(conf->value("viewing/constellation_art_intensity", 0.5f).toFloat());
	setArtFadeDuration(conf->value("viewing/constellation_art_fade_duration",2.f).toFloat());
	setFlagArt(conf->value("viewing/flag_constellation_art").toBool());
	setFlagIsolateSelected(conf->value("viewing/flag_constellation_isolate_selected", false).toBool());
	setFlagConstellationPick(conf->value("viewing/flag_constellation_pick", false).toBool());
	setConstellationLineThickness(conf->value("viewing/constellation_line_thickness", 1).toInt());

	QString starloreDisplayStyle=conf->value("viewing/constellation_name_style", "translated").toString();
	if (starloreDisplayStyle=="translated")
	{
		setConstellationDisplayStyle(constellationsTranslated);
	}
	else if (starloreDisplayStyle=="native")
	{
		setConstellationDisplayStyle(constellationsNative);
	}
	else if (starloreDisplayStyle=="abbreviated")
	{
		setConstellationDisplayStyle(constellationsAbbreviated);
	}
	else if (starloreDisplayStyle=="english")
	{
		setConstellationDisplayStyle(constellationsEnglish);
	}
	else
	{
		qDebug() << "Warning: viewing/constellation_name_style (" << starloreDisplayStyle << ") invalid. Using translated style.";
		conf->setValue("viewing/constellation_name_style", "translated");
		setConstellationDisplayStyle(constellationsTranslated);
	}

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setLinesColor(StelUtils::strToVec3f(conf->value("color/const_lines_color", defaultColor).toString()));
	setBoundariesColor(StelUtils::strToVec3f(conf->value("color/const_boundary_color", "0.8,0.3,0.3").toString()));
	setLabelsColor(StelUtils::strToVec3f(conf->value("color/const_names_color", defaultColor).toString()));

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
			fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellations_boundaries.dat");
		}
		else
		{
			// boundaries = generic
			fic = StelFileMgr::findFile("data/constellations_boundaries.dat");
		}

		if (fic.isEmpty())
			qWarning() << "ERROR loading constellation boundaries file: " << fic;
		else
			loadBoundaries(fic);

	}

	lastLoadedSkyCulture = skyCultureDir;
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
			unsetSelectedConst((Constellation *)newSelectedConst[0].data());
		}
		else
		{
			// Add constellation to selected list (do not select a star, just the constellation)
			setSelectedConst((Constellation *)newSelectedConst[0].data());
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
	selected.clear();
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	const QList<StelObjectP> currSelection = omgr->getSelectedObject();
	if (currSelection.empty())
	{
		return;
	}

	QList<StelObjectP> newSelection;
	foreach(const StelObjectP& obj, currSelection)
	{
		if (obj->getType() != "Constellation")
		{
			newSelection.push_back(obj);
		}
	}
	omgr->setSelectedObject(newSelection, StelModule::ReplaceSelection);
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
	if (asterFont.pixelSize() != newFontSize)
	{
		asterFont.setPixelSize(newFontSize);
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
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}
	in.seek(0);

	// delete existing data, if any
	vector < Constellation * >::iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
		delete(*iter);

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
			cons->artFader.setDuration((int) (artFadeDuration * 1000.f));
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
			Vec3d s1 = hipStarMgr->searchHP(hp1)->getJ2000EquatorialPos(core);
			Vec3d s2 = hipStarMgr->searchHP(hp2)->getJ2000EquatorialPos(core);
			Vec3d s3 = hipStarMgr->searchHP(hp3)->getJ2000EquatorialPos(core);

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3d s4 = s1 + ((s2 - s1) ^ (s3 - s1));
			Mat4d B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
			Mat4d A(x1, texSizeY - y1, 0.f, 1.f, x2, texSizeY - y2, 0.f, 1.f, x3, texSizeY - y3, 0.f, 1.f, x1, texSizeY - y1, texSizeX, 1.f);
			Mat4d X = B * A.inverse();

			// Tesselate on the plan assuming a tangential projection for the image
			static const int nbPoints=5;
			QVector<Vec2f> texCoords;
			texCoords.reserve(nbPoints*nbPoints*6);
			for (int j=0;j<nbPoints;++j)
			{
				for (int i=0;i<nbPoints;++i)
				{
					texCoords << Vec2f(((float)i)/nbPoints, ((float)j)/nbPoints);
					texCoords << Vec2f(((float)i+1.f)/nbPoints, ((float)j)/nbPoints);
					texCoords << Vec2f(((float)i)/nbPoints, ((float)j+1.f)/nbPoints);
					texCoords << Vec2f(((float)i+1.f)/nbPoints, ((float)j)/nbPoints);
					texCoords << Vec2f(((float)i+1.f)/nbPoints, ((float)j+1.f)/nbPoints);
					texCoords << Vec2f(((float)i)/nbPoints, ((float)j+1.f)/nbPoints);
				}
			}

			QVector<Vec3d> contour;
			contour.reserve(texCoords.size());
			foreach (const Vec2f& v, texCoords)
				contour << X * Vec3d(v[0]*texSizeX, v[1]*texSizeY, 0.);

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

	vector < Constellation * >::const_iterator iter;
	SphericalRegionP region = sPainter.getProjector()->getViewportConvexPolygon();
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->drawArtOptim(sPainter, *region);
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
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->drawOptim(sPainter, core, viewportHalfspace);
	}
	if (constellationLineThickness>1)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw the names of all the constellations
void ConstellationMgr::drawNames(StelPainter& sPainter) const
{
	sPainter.setBlending(true);
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); iter++)
	{
		// Check if in the field of view
		if (sPainter.getProjector()->projectCheck((*iter)->XYZname, (*iter)->XYname))
			(*iter)->drawName(sPainter, constellationDisplayStyle);
	}
}

Constellation *ConstellationMgr::isStarIn(const StelObject* s) const
{
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if ((*iter)->isStarIn(s))
		{
			return (*iter);
		}
	}
	return Q_NULLPTR;
}

Constellation* ConstellationMgr::findFromAbbreviation(const QString& abbreviation) const
{
	// search in uppercase only
	//QString tname = abbreviation.toUpper();

	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		//if ((*iter)->abbreviation.toUpper() == tname)
		if ((*iter)->abbreviation.compare(abbreviation, Qt::CaseInsensitive) == 0)
		{
			//if ((*iter)->abbreviation != abbreviation)
			//	qDebug() << "ConstellationMgr::findFromAbbreviation: not a perfect match, but sufficient:" << (*iter)->abbreviation << "vs." << abbreviation;
			return (*iter);
		}
		//else qDebug() << "Comparison mismatch: " << abbreviation << "vs." << (*iter)->abbreviation;
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
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	// abbreviation is allowed to start with a dot to mark as "hidden".
	QRegExp recRx("^\\s*(\\.?\\w+)\\s+\"(.*)\"\\s+_[(]\"(.*)\"[)]\\n");
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
			shortName = recRx.capturedTexts().at(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != Q_NULLPTR)
			{
				aster->nativeName = recRx.capturedTexts().at(2);
				ctxt = recRx.capturedTexts().at(3);
				if (ctxRx.exactMatch(ctxt))
				{
					aster->englishName = ctxRx.capturedTexts().at(1);
					aster->context = ctxRx.capturedTexts().at(2);
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

void ConstellationMgr::loadSeasonalRules(const QString& rulesFile)
{
	// Constellation not loaded yet
	if (constellations.empty()) return;

	bool flag = true;
	if (rulesFile.isEmpty())
		flag = false;

	// clear previous names
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->beginSeason = 1;
		(*iter)->endSeason = 12;
		(*iter)->seasonalRuleEnabled = flag;
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
			shortName = recRx.capturedTexts().at(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != Q_NULLPTR)
			{
				aster->beginSeason = recRx.capturedTexts().at(2).toInt();
				aster->endSeason = recRx.capturedTexts().at(3).toInt();
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
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->nameI18 = trans.qtranslate((*iter)->englishName, (*iter)->context);
	}
}

// update faders
void ConstellationMgr::update(double deltaTime)
{
	//calculate FOV fade value, linear fade between artIntensityMaximumFov and artIntensityMinimumFov
	double fov = StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov();
	Constellation::artIntensityFovScale = qBound(0.0,(fov - artIntensityMinimumFov) / (artIntensityMaximumFov - artIntensityMinimumFov),1.0);

	vector < Constellation * >::const_iterator iter;
	const int delta = (int)(deltaTime*1000);
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->update(delta);
	}
}

void ConstellationMgr::setArtIntensity(const float intensity)
{
	if (artIntensity != intensity)
	{
		artIntensity = intensity;

		vector < Constellation * >::const_iterator iter;
		for (iter = constellations.begin(); iter != constellations.end(); ++iter)
		{
			(*iter)->artOpacity = artIntensity;
		}

		emit artIntensityChanged(intensity);
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

		vector < Constellation * >::const_iterator iter;
		for (iter = constellations.begin(); iter != constellations.end(); ++iter)
		{
			(*iter)->artFader.setDuration((int) (duration * 1000.f));
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
		if (selected.begin() != selected.end()  && isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = selected.begin(); iter != selected.end(); ++iter)
			{
				(*iter)->setFlagLines(linesDisplayed);
			}
		}
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
			{
				(*iter)->setFlagLines(linesDisplayed);
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
		if (selected.begin() != selected.end() && isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = selected.begin(); iter != selected.end(); ++iter)
			{
				(*iter)->setFlagBoundaries(boundariesDisplayed);
			}
		}
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
			{
				(*iter)->setFlagBoundaries(boundariesDisplayed);
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
		if (selected.begin() != selected.end() && isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = selected.begin(); iter != selected.end(); ++iter)
			{
				(*iter)->setFlagArt(artDisplayed);
			}
		}
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
			{
				(*iter)->setFlagArt(artDisplayed);
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
		if (selected.begin() != selected.end() && isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = selected.begin(); iter != selected.end(); ++iter)
				(*iter)->setFlagLabels(namesDisplayed);
		}
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
				(*iter)->setFlagLabels(namesDisplayed);
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
			vector < Constellation * >::const_iterator iter;
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
			{
				(*iter)->setFlagLines(getFlagLines());
				(*iter)->setFlagLabels(getFlagLabels());
				(*iter)->setFlagArt(getFlagArt());
				(*iter)->setFlagBoundaries(getFlagBoundaries());
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

				vector < Constellation * >::const_iterator iter;
				for (iter = constellations.begin(); iter != constellations.end(); ++iter)
				{
					bool match = false;
					vector < Constellation * >::const_iterator s_iter;
					for (s_iter = selected.begin(); s_iter != selected.end(); ++s_iter)
					{
						if( (*iter)==(*s_iter) )
						{
							match=true; // this is a selected constellation
							break;
						}
					}

					if(!match)
					{
						// Not selected constellation
						(*iter)->setFlagLines(false);
						(*iter)->setFlagLabels(false);
						(*iter)->setFlagArt(false);
						(*iter)->setFlagBoundaries(false);
					}
				}
			}
			else
			{
				vector < Constellation * >::const_iterator iter;
				for (iter = constellations.begin(); iter != constellations.end(); ++iter)
				{
					(*iter)->setFlagLines(false);
					(*iter)->setFlagLabels(false);
					(*iter)->setFlagArt(false);
					(*iter)->setFlagBoundaries(false);
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
		if (selected.begin() == selected.end()) return;

		// Otherwise apply standard flags to all constellations
		vector < Constellation * >::const_iterator iter;
		for (iter = constellations.begin(); iter != constellations.end(); ++iter)
		{
			(*iter)->setFlagLines(getFlagLines());
			(*iter)->setFlagLabels(getFlagLabels());
			(*iter)->setFlagArt(getFlagArt());
			(*iter)->setFlagBoundaries(getFlagBoundaries());
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

		vector < Constellation * >::const_iterator iter;
		int n=0;
		for (iter = selected.begin(); iter != selected.end(); ++iter)
		{
			if( (*iter)->getEnglishName() == c->getEnglishName() )
			{
				selected.erase(selected.begin()+n, selected.begin()+n+1);
				iter--;
				n--;
			}
			n++;
		}

		// If no longer any selection, restore all flags on all constellations
		if (selected.begin() == selected.end())
		{

			// Otherwise apply standard flags to all constellations
			for (iter = constellations.begin(); iter != constellations.end(); ++iter)
			{
				(*iter)->setFlagLines(getFlagLines());
				(*iter)->setFlagLabels(getFlagLabels());
				(*iter)->setFlagArt(getFlagArt());
				(*iter)->setFlagBoundaries(getFlagBoundaries());
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
	vector<vector<Vec3d> *>::iterator iter;
	for (iter = allBoundarySegments.begin(); iter != allBoundarySegments.end(); ++iter)
	{
		delete (*iter);
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

	// Added support of comments for constellations_boundaries.dat file
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
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		(*iter)->drawBoundaryOptim(sPainter);
	}
}

StelObjectP ConstellationMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	vector <Constellation*>::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		QString objwcap = (*iter)->nameI18.toUpper();
		if (objwcap==objw) return *iter;
	}
	return Q_NULLPTR;
}

StelObjectP ConstellationMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	vector <Constellation*>::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		QString objwcap = (*iter)->englishName.toUpper();
		if (objwcap==objw) return *iter;

		objwcap = (*iter)->abbreviation.toUpper();
		if (objwcap==objw) return *iter;
	}
	return Q_NULLPTR;
}

StelObjectP ConstellationMgr::searchByID(const QString &id) const
{
	vector <Constellation*>::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		if ((*iter)->getID() == id) return *iter;
	}
	return Q_NULLPTR;
}

QStringList ConstellationMgr::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords, bool inEnglish) const
{
	QStringList result;
	if (maxNbItem <= 0)
	{
		return result;
	}

	vector<Constellation*>::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		QString name = inEnglish ? (*iter)->getEnglishName() : (*iter)->getNameI18n();
		if (!matchObjectName(name, objPrefix, useStartOfWords))
		{
			continue;
		}

		result.append(name);
		if (result.size() >= maxNbItem)
		{
			break;
		}
	}

	result.sort();
	return result;
}

QStringList ConstellationMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach(Constellation* constellation, constellations)
		{
			result << constellation->getEnglishName();
		}
	}
	else
	{
		foreach(Constellation* constellation, constellations)
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
	vector < Constellation * >::const_iterator iter;
	for (iter = constellations.begin(); iter != constellations.end(); ++iter)
	{
		// Check if the object is in the constellation
		if ((*iter)->getShortName().toUpper()==IAUConst.toUpper())
			return (*iter);
	}
	return Q_NULLPTR;
}
