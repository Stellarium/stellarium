/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// class used to manage groups of Nebulas

#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QRegExp>

#include "StelApp.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "STexture.hpp"
#include "Navigator.hpp"
#include "SkyDrawer.hpp"
#include "Translator.hpp"
#include "LoadingBar.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"

#include "SkyImageTile.hpp"

void NebulaMgr::setLabelsColor(const Vec3f& c) {Nebula::labelColor = c;}
const Vec3f &NebulaMgr::getLabelsColor(void) const {return Nebula::labelColor;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::circleColor = c;}
const Vec3f &NebulaMgr::getCirclesColor(void) const {return Nebula::circleColor;}
void NebulaMgr::setCircleScale(float scale) {Nebula::circleScale = scale;}
float NebulaMgr::getCircleScale(void) const {return Nebula::circleScale;}


NebulaMgr::NebulaMgr(void) : nebGrid(200), displayNoTexture(false)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	Nebula::texCircle = STextureSP();
	
	vector<Nebula *>::iterator iter;
	for(iter=nebArray.begin();iter!=nebArray.end();iter++)
	{
		delete (*iter);
	}
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MilkyWay")->getCallOrder(actionName)+10;
	return 0;
}

// read from stream
void NebulaMgr::init()
{
	// TODO: mechanism to specify which sets get loaded at start time.
	// candidate methods:
	// 1. config file option (list of sets to load at startup)
	// 2. load all
	// 3. flag in nebula_textures.fab (yuk)
	// 4. info.ini file in each set containing a "load at startup" item
	// For now (0.9.0), just load the default set
	loadNebulaSet("default");

	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	double fontSize = 12;
	Nebula::nebulaFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	Nebula::texCircle = StelApp::getInstance().getTextureManager().createTexture("neb.png");   // Load circle texture

	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur5.png");   // Load pointer texture
	
	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setHintsAmount(conf->value("astro/nebula_hints_amount", 3).toDouble());
	setLabelsAmount(conf->value("astro/nebula_labels_amount", 3).toDouble());
	setCircleScale(conf->value("astro/nebula_scale",1.0f).toDouble());
	setFlagDisplayNoTexture(conf->value("astro/flag_nebula_display_no_texture", false).toBool());

	updateI18n();
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);
}

// Draw all the Nebulae
void NebulaMgr::draw(StelCore* core)
{
	Projector* prj = core->getProjection();
	SkyDrawer* skyDrawer = core->getSkyDrawer();
	
	Nebula::hintsBrightness = hintsFader.getInterstate()*flagShow.getInterstate();
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	Vec3f pXYZ;
	
	prj->setCurrentFrame(Projector::FrameJ2000);
	// Use a 1 degree margin
	const double margin = 1.*M_PI/180.*prj->getPixelPerRadAtCenter();
	const StelGeom::ConvexPolygon& p = prj->getViewportConvexPolygon(margin, margin);
	nebGrid.filterIntersect(p);
	
	// Print all the nebulae of all the selected zones
	Nebula* n;

	// speed up the computation of n->getOnScreenSize(core)>5:
	const float size_limit = 5./prj->getPixelPerRadAtCenter()*180./M_PI;
	float maxMagHints = skyDrawer->getLimitMagnitude()*1.2-2.+(hintsAmount*1.2f)-2.f;
	float maxMagLabels = skyDrawer->getLimitMagnitude()-2.+(labelsAmount*1.2f)-2.f;
	
	for (TreeGrid::const_iterator iter = nebGrid.begin(); iter != nebGrid.end(); ++iter)
	{
		n = static_cast<Nebula*>(*iter);

		// improve performance by skipping if too small to see
		if (n->angularSize>size_limit || (hintsFader.getInterstate()>0.0001 && n->mag <= maxMagHints))
		{
			prj->project(n->XYZ,n->XY);
			n->drawLabel(core, maxMagLabels);
			n->drawHints(core, maxMagHints);
		}
	}
	drawPointer(core);
	//nebGrid.draw(prj, p);
}

void NebulaMgr::drawPointer(const StelCore* core)
{
	const Navigator* nav = core->getNavigation();
	const Projector* prj = core->getProjection();
	
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Nebula");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getObsJ2000Pos(nav);
		Vec3d screenpos;
		
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
		glColor3f(0.4f,0.5f,0.8f);
		texPointer->bind();
		
		glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode

		float size = obj->getOnScreenSize(core);

		size+=20.f + 10.f*std::sin(2.f * StelApp::getInstance().getTotalRunTime());
		prj->drawSprite2dMode(screenpos[0]-size/2, screenpos[1]-size/2, 20, 90);
		prj->drawSprite2dMode(screenpos[0]-size/2, screenpos[1]+size/2, 20, 0);
		prj->drawSprite2dMode(screenpos[0]+size/2, screenpos[1]+size/2, 20, -90);
		prj->drawSprite2dMode(screenpos[0]+size/2, screenpos[1]-size/2, 20, -180);
	}
}

void NebulaMgr::updateSkyCulture()
{;}

void NebulaMgr::setColorScheme(const QSettings* conf, const QString& section)
{
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelsColor(StelUtils::strToVec3f(conf->value(section+"/nebula_label_color", defaultColor).toString()));
	setCirclesColor(StelUtils::strToVec3f(conf->value(section+"/nebula_circleColor", defaultColor).toString()));
}

// Search by name
StelObject* NebulaMgr::search(const QString& name)
{
	QString uname = name.toUpper();
	vector <Nebula*>::const_iterator iter;

	for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
	{
		QString testName = (*iter)->getEnglishName().toUpper();
		if (testName==uname) return *iter;
	}
	
	// If no match found, try search by catalog reference
	QRegExp catNumRx("^(M|NGC|IC)\\s*(\\d+)$");
	if (catNumRx.exactMatch(uname))
	{
		QString cat = catNumRx.capturedTexts().at(1);
		int num = catNumRx.capturedTexts().at(2).toInt();
		
		if (cat == "M") return searchM(num);
		if (cat == "NGC") return searchNGC(num);
		if (cat == "IC") return searchIC(num);
	}
	return NULL;
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	try
	{
		loadNGC(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000.dat"));
		loadNGCNames(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000names.dat"));
	}
	catch (exception& e)
	{
		qWarning() << "ERROR while loading nebula data set " << setName << ": " << e.what();
	}
}

// Look for a nebulae by XYZ coords
StelObject* NebulaMgr::search(Vec3f Pos)
{
	Pos.normalize();
	vector<Nebula *>::iterator iter;
	Nebula * plusProche=NULL;
	float anglePlusProche=0.;
	for(iter=nebArray.begin();iter!=nebArray.end();iter++)
	{
		if ((*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2]>anglePlusProche)
		{
			anglePlusProche=(*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2];
			plusProche=(*iter);
		}
	}
	if (anglePlusProche>0.999)
	{
		return plusProche;
	}
	else return NULL;
}

// Return a stl vector containing the nebulas located inside the limFov circle around position v
vector<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	vector<StelObjectP> result;
	if (!getFlagShow())
		return result;
		
	Vec3d v(av);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	static Vec3d equPos;

	vector<Nebula*>::const_iterator iter = nebArray.begin();
	while (iter != nebArray.end())
	{
		equPos = (*iter)->XYZ;
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cosLimFov)
		{

			// NOTE: non-labeled nebulas are not returned!
			// Otherwise cursor select gets invisible nebulas - Rob
			//if((*iter)->getNameI18n() != L"") 
				result.push_back(StelObjectP(*iter));
		}
		iter++;
	}
	return result;
}

Nebula *NebulaMgr::searchM(unsigned int M)
{
	vector<Nebula *>::iterator iter;
	for(iter=nebArray.begin();iter!=nebArray.end();iter++)
	{
		if ((*iter)->M_nb == M) return (*iter);
	}

	return NULL;
}

Nebula *NebulaMgr::searchNGC(unsigned int NGC)
{
	vector<Nebula *>::iterator iter;
	for(iter=nebArray.begin();iter!=nebArray.end();++iter)
	{
		if ((*iter)->NGC_nb == NGC)
			return (*iter);
	}
	return NULL;
}

Nebula *NebulaMgr::searchIC(unsigned int IC)
{
	vector<Nebula *>::iterator iter;
	for(iter=nebArray.begin();iter!=nebArray.end();iter++)
	{
		if ((*iter)->IC_nb == IC) return (*iter);
	}

	return NULL;
}


// read from stream
bool NebulaMgr::loadNGC(const QString& catNGC)
{
	LoadingBar& lb = *StelApp::getInstance().getLoadingBar();
	QFile in(catNGC);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	int totalRecords=0;
	QString record;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!in.atEnd()) 
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.exactMatch(record))
			totalRecords++;
	}

	// rewind the file to the start
	in.seek(0);

	int currentLineNumber = 0;	// what input line we are on
	int currentRecordNumber = 0;	// what record number we are on
	int readOk = 0;			// how many records weree rad without problems
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		++currentLineNumber;

		// skip comments
		if (commentRx.exactMatch(record))
			continue;

		++currentRecordNumber;

		// Update the status bar every 200 record 
		if (!(currentRecordNumber%200) || (currentRecordNumber == totalRecords))
		{
			lb.SetMessage(q_("Loading NGC catalog: %1/%2").arg(currentRecordNumber).arg(totalRecords));
			lb.Draw((float)currentRecordNumber/totalRecords);
		}

		// Create a new Nebula record
		Nebula *e = new Nebula;
		if (!e->readNGC((char*)record.toLocal8Bit().data())) // reading error
		{
			delete e;
			e = NULL;
		}
		else
		{
			nebArray.push_back(e);
			nebGrid.insert(e);
			++readOk;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC records";
	return true;
}


bool NebulaMgr::loadNGCNames(const QString& catNGCNames)
{
	qDebug() << "Loading NGC name data ...";
	QFile ngcNameFile(catNGCNames);
	if (!ngcNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "NGC name data file" << catNGCNames << "not found.";
		return false;
	}

	// Read the names of the NGC objects
	QString name, record;
	int totalRecords=0;
	int lineNumber=0;
	int readOk=0;
	int nb;
	Nebula *e;
	QRegExp commentRx("^(\\s*#.*|\\s*)$");
	while (!ngcNameFile.atEnd())
	{
		record = QString::fromUtf8(ngcNameFile.readLine());
		lineNumber++;
		if (commentRx.exactMatch(record))
			continue;
	
		totalRecords++;
		nb = record.mid(38,4).toInt();
		if (record[37] == 'I')
		{
			e = (Nebula*)searchIC(nb);
		}
		else
		{
			e = (Nebula*)searchNGC(nb);
		}

		// get name, trimmed of whitespace
		name = record.left(36).trimmed();
		
		if (e)
		{
			// If the name is not a messier number perhaps one is already
			// defined for this object
			if (name.left(2).toUpper() != "M ")
			{
				e->englishName = name;
			}
			else
			{
				// If it's a messiernumber, we will call it a messier if there is no better name
				name = name.mid(2); // remove "M "

				// read the Messier number
				QTextStream istr(&name);
				int num;
				istr >> num;
				if (istr.status()!=QTextStream::Ok)
				{
					qWarning() << "cannot read Messier number at line" << lineNumber << "of" << catNGCNames;
					continue;
				}

				e->M_nb=(unsigned int)(num);
				e->englishName = QString("M%1").arg(num);
			}

			readOk++;
		}
		else
			qWarning() << "no position data for " << name << "at line" << lineNumber << "of" << catNGCNames;
	}
	ngcNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "NGC name records successfully";

	return true;
}


void NebulaMgr::updateI18n()
{
	Translator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	vector<Nebula*>::iterator iter;
	for( iter = nebArray.begin(); iter < nebArray.end(); iter++ )
	{
		(*iter)->translateName(trans);
	}
	double fontSize = 12;
	Nebula::nebulaFont = &StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}


//! Return the matching Nebula object's pointer if exists or NULL
StelObjectP NebulaMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();
	vector <Nebula*>::const_iterator iter;
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.mid(0, 3) == "NGC")
	{
		for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
		{
			if (QString("NGC%1").arg((*iter)->NGC_nb) == objw || QString("NGC %1").arg((*iter)->NGC_nb) == objw)
				return *iter;
		}
	}
	
	// Search by common names
	for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
	{
		QString objwcap = (*iter)->nameI18.toUpper();
		if (objwcap==objw) 
			return *iter;
	}
	
	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.mid(0, 1) == "M")
	{
		for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
		{
			if (QString("M%1").arg((*iter)->M_nb) == objw || QString("M %1").arg((*iter)->M_nb) == objw)
				return *iter;
		}
	}
	
	return NULL;
}


//! Return the matching Nebula object's pointer if exists or NULL
//! TODO split common parts of this and I18 fn above into a separate fn.
StelObjectP NebulaMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	vector <Nebula*>::const_iterator iter;
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.mid(0, 3) == "NGC")
	{
		for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
		{
			if (QString("NGC%1").arg((*iter)->NGC_nb) == objw || QString("NGC %1").arg((*iter)->NGC_nb) == objw)
				return *iter;
		}
	}
	
	// Search by common names
	for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
	{
		QString objwcap = (*iter)->englishName.toUpper();
		if (objwcap==objw) 
			return *iter;
	}
	
	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.mid(0, 1) == "M")
	{
		for (iter = nebArray.begin(); iter != nebArray.end(); ++iter)
		{
			if (QString("M%1").arg((*iter)->M_nb) == objw || QString("M %1").arg((*iter)->M_nb) == objw)
				return *iter;
		}
	}
	
	return NULL;
}


//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
QStringList NebulaMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;
		
	QString objw = objPrefix.toUpper();
	
	vector <Nebula*>::const_iterator iter;
	
	// Search by messier objects number (possible formats are "M31" or "M 31")
	if (objw.size()>=1 && objw[0]=='M')
	{
		for (iter=nebArray.begin(); iter!=nebArray.end(); ++iter)
		{
			if ((*iter)->M_nb==0) continue;
			QString constw = QString("M%1").arg((*iter)->M_nb);
			QString constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constw;
				continue;	// Prevent adding both forms for name
			}
			constw = QString("M %1").arg((*iter)->M_nb);
			constws = constw.mid(0, objw.size());
			if (constws==objw)
			{
				result << constw;
			}
		}
	}
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	for (iter=nebArray.begin(); iter!=nebArray.end(); ++iter)
	{
		if ((*iter)->NGC_nb==0) continue;
		QString constw = QString("NGC%1").arg((*iter)->NGC_nb);
		QString constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constw;
			continue;
		}
		constw = QString("NGC %1").arg((*iter)->NGC_nb);
		constws = constw.mid(0, objw.size());
		if (constws==objw)
		{
			result << constw;
		}
	}
	
	// Search by common names
	for (iter=nebArray.begin(); iter!=nebArray.end(); ++iter)
	{
		QString constw = (*iter)->nameI18.mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << (*iter)->nameI18;
		}
	}
	
	result.sort();
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());
	
	return result;
}

