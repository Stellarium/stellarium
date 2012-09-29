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

#include <vector>
#include <QDebug>
#include <QFile>
#include <QImageReader>
#include <QSettings>
#include <QRegExp>
#include <QString>
#include <QStringList>

#include "ConstellationMgr.hpp"
#include "Constellation.hpp"
#include "StarMgr.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "renderer/StelRenderer.hpp"
#include "StelProjector.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"

using namespace std;

// constructor which loads all data from appropriate files
ConstellationMgr::ConstellationMgr(StarMgr *_hip_stars)
	: hipStarMgr(_hip_stars),
	  artFadeDuration(2.0f),
	  artIntensity(0.5f),
	  artDisplayed(0),
	  boundariesDisplayed(0),
	  linesDisplayed(0),
	  namesDisplayed(0)
{
	setObjectName("ConstellationMgr");
	Q_ASSERT(hipStarMgr);
	isolateSelected = false;
	asterFont.setPixelSize(15);
}

ConstellationMgr::~ConstellationMgr()
{
	std::vector<Constellation *>::iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		delete(*iter);
	}

	vector<vector<Vec3f> *>::iterator iter1;
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
	asterFont.setPixelSize(conf->value("viewing/constellation_font_size", 16).toInt());
	setFlagLines(conf->value("viewing/flag_constellation_drawing").toBool());
	setFlagLabels(conf->value("viewing/flag_constellation_name").toBool());
	setFlagBoundaries(conf->value("viewing/flag_constellation_boundaries",false).toBool());
	setArtIntensity(conf->value("viewing/constellation_art_intensity", 0.5f).toFloat());
	setArtFadeDuration(conf->value("viewing/constellation_art_fade_duration",2.f).toFloat());
	setFlagArt(conf->value("viewing/flag_constellation_art").toBool());
	setFlagIsolateSelected(conf->value("viewing/flag_constellation_isolate_selected",
					   conf->value("viewing/flag_constellation_pick", false).toBool() ).toBool());

	StelObjectMgr *objectManager = GETSTELMODULE(StelObjectMgr);
	objectManager->registerStelObjectMgr(this);
	connect(objectManager, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)), 
			this, SLOT(selectedObjectChange(StelModule::StelModuleSelectAction)));
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(app, SIGNAL(skyCultureChanged(const QString&)), this, SLOT(updateSkyCulture(const QString&)));
	connect(app, SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
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
	QString conArtFile;
	try
	{
		conArtFile = StelFileMgr::findFile("skycultures/"+skyCultureDir+"/constellationsart.fab");
	}
	catch (std::runtime_error& e)
	{
		qDebug() << "No constellationsart.fab file found for sky culture " << skyCultureDir;
	}

	try
	{
		// first of all, remove constellations from the list of selected objects in StelObjectMgr, since we are going to delete them
		deselectConstellations();

		loadLinesAndArt(StelFileMgr::findFile("skycultures/"+skyCultureDir+"/constellationship.fab"), conArtFile, skyCultureDir);

		// load constellation names
		loadNames(StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellation_names.eng.fab"));

		// Translate constellation names for the new sky culture
		updateI18n();
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR: while loading new constellation data for sky culture "
			<< skyCultureDir << ", reason: " << e.what() << endl;
	}

	// TODO: do we need to have an else { clearBoundaries(); } ?
	// load constellation boundaries
	try
	{
		loadBoundaries(StelFileMgr::findFile("skycultures/" + skyCultureDir + "/constellations_boundaries.dat"));
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR loading constellation boundaries file: " << e.what();
	}

	lastLoadedSkyCulture = skyCultureDir;
}

void ConstellationMgr::setStelStyle(const QString& section)
{
	QSettings* conf = StelApp::getInstance().getSettings();

	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLinesColor(StelUtils::strToVec3f(conf->value(section+"/const_lines_color", defaultColor).toString()));
	setBoundariesColor(StelUtils::strToVec3f(conf->value(section+"/const_boundary_color", "0.8,0.3,0.3").toString()));
	setLabelsColor(StelUtils::strToVec3f(conf->value(section+"/const_names_color", defaultColor).toString()));
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
		// setSelected(NULL);
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
		const QList<StelObjectP> newSelectedStar = omgr->getSelectedObject("Star");
		if (!newSelectedStar.empty())
		{
//			if (!added)
//				setSelected(NULL);
			setSelected(newSelectedStar[0].data());
		}
		else
		{
//			if (!added)
				setSelected(NULL);
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

void ConstellationMgr::loadLinesAndArt(const QString &fileName, const QString &artfileName, const QString& cultureName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open constellation data file" << fileName  << "for culture" << cultureName;
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
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		delete(*iter);

	asterisms.clear();
	Constellation *cons = NULL;

	// read the file, adding a record per non-comment line
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
			cons->artFader.setMaxValue(artIntensity);
			cons->setFlagArt(artDisplayed);
			cons->setFlagBoundaries(boundariesDisplayed);
			cons->setFlagLines(linesDisplayed);
			cons->setFlagLabels(namesDisplayed);
			asterisms.push_back(cons);
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading constellation rec at line " << currentLineNumber << "for culture" << cultureName;
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
		qWarning() << "Can't open constellation art file" << fileName  << "for culture" << cultureName;
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
	QString tmpstr;

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

		cons = NULL;
		cons = findFromAbbreviation(shortname);
		if (!cons)
		{
			qWarning() << "ERROR in constellation art file at line" << currentLineNumber << "for culture" << cultureName
			           << "constellation" << shortname << "unknown";
		}
		else
		{
			QString texturePath(texfile);
			try
			{
				texturePath = StelFileMgr::findFile("skycultures/"+cultureName+"/"+texfile);
			}
			catch (std::runtime_error& e)
			{
				// if the texture isn't found in the skycultures/[culture] directory,
				// try the central textures diectory.
				qWarning() << "WARNING, could not locate texture file " << texfile
					 << " in the skycultures/" << cultureName
					 << " directory...  looking in general textures/ directory...";
				try
				{
					texturePath = StelFileMgr::findFile(QString("textures/")+texfile);
				}
				catch(std::exception& e2)
				{
					qWarning() << "ERROR: could not find texture, " << texfile << ": " << e2.what();
				}
			}

			cons->artTexturePath = texturePath;

			// This is one part that is less convenient than before the GL refactor 
			// (due to the StelRenderer not being globally (StelCore) available.
			// We need to determine texture size manually here.

			// Try to get the size from the file without loading data
			QImageReader im(texturePath);
			if (!im.canRead())
			{
				qWarning() << "Texture dimensions not available";
			}
			const QSize size = im.canRead() ? im.size() : QSize(64, 64);
			const int texSizeX = size.width();
			const int texSizeY = size.height();

			StelCore* core = StelApp::getInstance().getCore();
			Vec3d s1 = hipStarMgr->searchHP(hp1)->getJ2000EquatorialPos(core);
			Vec3d s2 = hipStarMgr->searchHP(hp2)->getJ2000EquatorialPos(core);
			Vec3d s3 = hipStarMgr->searchHP(hp3)->getJ2000EquatorialPos(core);

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3d s4 = s1 + ((s2 - s1) ^ (s3 - s1));
			Mat4d B(s1[0], s1[1], s1[2], 1, 
			        s2[0], s2[1], s2[2], 1,
			        s3[0], s3[1], s3[2], 1,
			        s4[0], s4[1], s4[2], 1);
			Mat4d A(x1, texSizeY - y1, 0.f, 1.f,
			        x2, texSizeY - y2, 0.f, 1.f, 
			        x3, texSizeY - y3, 0.f, 1.f, 
			        x1, texSizeY - y1, texSizeX, 1.f);
			Mat4d X = B * A.inverse();

			cons->texCoordTo3D = Mat4f(X[0]  , X[1]  , X[2]  , X[3],
			                           X[4]  , X[5]  , X[6]  , X[7],
			                           X[8]  , X[9]  , X[10] , X[11],
			                           X[12] , X[13] , X[14] , X[15]);


			Vec3d tmp(X * Vec3d(0.5 * texSizeX, 0.5 * texSizeY, 0.));
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

void ConstellationMgr::draw(StelCore* core, class StelRenderer* renderer)
{
	const StelProjectorP projector = core->getProjection(StelCore::FrameJ2000);
	drawLines(renderer, projector, core);
	drawNames(renderer, projector, asterFont);
	drawArt(renderer, projector);
	drawBoundaries(renderer, projector);
}

// Draw constellations art textures
void ConstellationMgr::drawArt(StelRenderer* renderer, StelProjectorP projector) const
{
	renderer->setBlendMode(BlendMode_Add);

	vector < Constellation * >::const_iterator iter;
	SphericalRegionP region = projector->getViewportConvexPolygon();
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		Constellation* cons = *iter;

		if(NULL == cons->artTexture && !cons->artTexturePath.isEmpty())
		{
			cons->artTexture = renderer->createTexture(cons->artTexturePath);
		}
		if(NULL == cons->artVertices)
		{
			// Tesselate on the plane assuming a tangential projection for the image
			const int resolution = 5;
			cons->generateArtVertices(renderer, resolution);
		}

		cons->drawArtOptim(renderer, projector, *region);
	}
}

// Draw constellations lines
void ConstellationMgr::drawLines(StelRenderer* renderer, StelProjectorP projector, const StelCore* core) const
{
	renderer->setBlendMode(BlendMode_Alpha);
	const SphericalCap& viewportHalfspace = projector->getBoundingCap();
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->drawOptim(renderer, projector, core, viewportHalfspace);
	}
}

// Draw the names of all the constellations
void ConstellationMgr::drawNames(StelRenderer* renderer, StelProjectorP projector, QFont& font) const
{
	renderer->setBlendMode(BlendMode_Alpha);
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		// Check if in the field of view
		if (projector->projectCheck((*iter)->XYZname, (*iter)->XYname))
		{
			(*iter)->drawName(renderer, font);\
		}
	}
}

Constellation *ConstellationMgr::isStarIn(const StelObject* s) const
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if ((*iter)->isStarIn(s))
		{
			return (*iter);
		}
	}
	return NULL;
}

Constellation* ConstellationMgr::findFromAbbreviation(const QString& abbreviation) const
{
	// search in uppercase only
	QString tname = abbreviation.toUpper();

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		if ((*iter)->abbreviation == tname)
			return (*iter);
	}
	return NULL;
}

// Can't find constellation from a position because it's not well localized
QList<StelObjectP> ConstellationMgr::searchAround(const Vec3d&, double, const StelCore*) const
{
	return QList<StelObjectP>();
}

void ConstellationMgr::loadNames(const QString& namesFile)
{
	// Constellation not loaded yet
	if (asterisms.empty()) return;

	// clear previous names
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << namesFile;
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	QRegExp commentRx("^(\\s*#.*|\\s*)$");

	// lines which look like records - we use the RE to extract the fields
	// which will be available in recRx.capturedTexts()
	QRegExp recRx("^\\s*(\\w+)\\s+\"(.*)\"\\s+_[(]\"(.*)\"[)]\\n");

	// Some more variables to use in the parsing
	Constellation *aster;
	QString record, shortName;

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
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in constellation names file" << namesFile;
		}
		else
		{
			shortName = recRx.capturedTexts().at(1);
			aster = findFromAbbreviation(shortName);
			// If the constellation exists, set the English name
			if (aster != NULL)
			{
				aster->nativeName = recRx.capturedTexts().at(2);
				aster->englishName = recRx.capturedTexts().at(3);
				readOk++;
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

void ConstellationMgr::updateI18n()
{
	StelTranslator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->nameI18 = trans.qtranslate((*iter)->englishName);
	}
}

// update faders
void ConstellationMgr::update(double deltaTime)
{
	vector < Constellation * >::const_iterator iter;
	const int delta = (int)(deltaTime*1000);
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->update(delta);
	}
}

void ConstellationMgr::setArtIntensity(const double intensity)
{
	if (artIntensity != intensity)
	{
		artIntensity = intensity;
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		{
			(*iter)->artFader.setMaxValue(artIntensity);
		}
		emit artIntensityChanged(intensity);
	}
}

double ConstellationMgr::getArtIntensity() const
{
	return artIntensity;
}

void ConstellationMgr::setArtFadeDuration(const float duration)
{
	if(artFadeDuration != duration)
	{
		artFadeDuration = duration;
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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

void ConstellationMgr::setFlagLabels(bool displayed)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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

StelObject* ConstellationMgr::getSelected(void) const {
	return *selected.begin();  // TODO return all or just remove this method
}

void ConstellationMgr::setSelected(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != NULL) setSelectedConst(c);
}

StelObjectP ConstellationMgr::setSelectedStar(const QString& abbreviation)
{
	Constellation * c = findFromAbbreviation(abbreviation);
	if(c != NULL)
	{
		setSelectedConst(c);
		return c->getBrightestStarInConstellation();
	}
	return NULL;
}

void ConstellationMgr::setSelectedConst(Constellation * c)
{
	// update states for other constellations to fade them out
	if (c != NULL)
	{
		selected.push_back(c);

		// Propagate current settings to newly selected constellation
		c->setFlagLines(getFlagLines());
		c->setFlagLabels(getFlagLabels());
		c->setFlagArt(getFlagArt());
		c->setFlagBoundaries(getFlagBoundaries());

		if (isolateSelected)
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			{

				bool match = 0;
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
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
	if (c != NULL)
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
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
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
	Constellation *cons = NULL;
	unsigned int i, j;

	// delete existing boundaries if any exist
	vector<vector<Vec3f> *>::iterator iter;
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
		qWarning() << "Boundary file " << boundaryFile << " not found";
		return false;
	}

	QTextStream istr(&dataFile);
	float DE, RA;
	Vec3f XYZ;
	unsigned num, numc;
	vector<Vec3f> *points = NULL;
	QString consname;
	i = 0;
	while (!istr.atEnd())
	{
		points = new vector<Vec3f>;

		num = 0;
		istr >> num;
		if(num == 0) continue;  // empty line

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

		if (cons) cons->sharedBoundarySegments.push_back(points);
		i++;

	}
	dataFile.close();
	qDebug() << "Loaded" << i << "constellation boundary segments";
	delete points;

	return true;
}

void ConstellationMgr::drawBoundaries(StelRenderer* renderer, StelProjectorP projector) const
{
	renderer->setBlendMode(BlendMode_None);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->drawBoundaryOptim(renderer, projector);
	}
}

StelObjectP ConstellationMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	vector <Constellation*>::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString objwcap = (*iter)->nameI18.toUpper();
		if (objwcap==objw) return *iter;
	}
	return NULL;
}

StelObjectP ConstellationMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	vector <Constellation*>::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString objwcap = (*iter)->englishName.toUpper();
		if (objwcap==objw) return *iter;

		objwcap = (*iter)->abbreviation.toUpper();
		if (objwcap==objw) return *iter;
	}
	return NULL;
}

QStringList ConstellationMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		QString constw = (*iter)->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << (*iter)->getNameI18n();
			if (result.size()==maxNbItem)
				return result;
		}
	}
	return result;
}

QStringList ConstellationMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		foreach(Constellation* constellation, asterisms)
		{
			result << constellation->getEnglishName();
		}
	}
	else
	{
		foreach(Constellation* constellation, asterisms)
		{
			result << constellation->getNameI18n();
		}
	}
	return result;
}
