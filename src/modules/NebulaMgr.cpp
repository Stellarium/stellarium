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

#include <fstream>
#include <algorithm>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>

#include "StelApp.hpp"
#include "NebulaMgr.hpp"
#include "Nebula.hpp"
#include "STexture.hpp"
#include "Navigator.hpp"
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

void NebulaMgr::setNamesColor(const Vec3f& c) {Nebula::label_color = c;}
const Vec3f &NebulaMgr::getNamesColor(void) const {return Nebula::label_color;}
void NebulaMgr::setCirclesColor(const Vec3f& c) {Nebula::circle_color = c;}
const Vec3f &NebulaMgr::getCirclesColor(void) const {return Nebula::circle_color;}
void NebulaMgr::setCircleScale(float scale) {Nebula::circleScale = scale;}
float NebulaMgr::getCircleScale(void) const {return Nebula::circleScale;}
void NebulaMgr::setFlagBright(bool b) {Nebula::flagBright = b;}
bool NebulaMgr::getFlagBright(void) const {return Nebula::flagBright;}


NebulaMgr::NebulaMgr(void) : nebGrid(10000), displayNoTexture(false)
{
	setObjectName("NebulaMgr");
}

NebulaMgr::~NebulaMgr()
{
	vector<Nebula *>::iterator iter;
	for(iter=neb_array.begin();iter!=neb_array.end();iter++)
	{
		delete (*iter);
	}
	
	Nebula::tex_circle = STextureSP();
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double NebulaMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
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
	Nebula::nebula_font = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	Nebula::tex_circle = StelApp::getInstance().getTextureManager().createTexture("neb.png");   // Load circle texture

	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur5.png");   // Load pointer texture
	
	setFlagShowTexture(true);
	setFlagShow(conf->value("astro/flag_nebula",true).toBool());
	setFlagHints(conf->value("astro/flag_nebula_name",false).toBool());
	setMaxMagHints(conf->value("astro/max_mag_nebula_name", 99).toDouble());
	setCircleScale(conf->value("astro/nebula_scale",1.0f).toDouble());
	setFlagDisplayNoTexture(conf->value("astro/flag_nebula_display_no_texture", false).toBool());
	setFlagBright(conf->value("astro/flag_bright_nebulae",false).toBool());
	
	updateI18n();
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);
}

// Draw all the Nebulae
double NebulaMgr::draw(StelCore* core)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	ToneReproducer* eye = core->getToneReproducer();
	
	Nebula::hints_brightness = hintsFader.getInterstate()*flagShow.getInterstate();
	Nebula::flagShowTexture = flagShowTexture;
	
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	Vec3f pXYZ;
	
	prj->setCurrentFrame(Projector::FRAME_J2000);
	// Use a 1 degree margin
	const double margin = 1.*M_PI/180.*prj->getPixelPerRadAtCenter();
	const StelGeom::ConvexPolygon& p = prj->getViewportConvexPolygon(margin, margin);
	nebGrid.filterIntersect(p);
	
	// Print all the stars of all the selected zones
	Nebula* n;

	// speed up the computation of n->getOnScreenSize(prj, nav)>5:
	const float size_limit = 5./prj->getPixelPerRadAtCenter()*180./M_PI;

	for (TreeGrid::const_iterator iter = nebGrid.begin(); iter != nebGrid.end(); ++iter)
	{
		n = static_cast<Nebula*>(*iter);
		if (!displayNoTexture && !n->hasTex()) continue;

		// improve performance by skipping if too small to see
		// TODO: skip if too faint to see
		if (n->angular_size>size_limit || (hintsFader.getInterstate()>0.0001 && n->mag <= getMaxMagHints()))
		{
			prj->project(n->XYZ,n->XY);

			if (n->angular_size>size_limit)
			{
				if (n->hasTex())
					n->draw_tex(prj, nav, eye);
				else n->draw_no_tex(prj, nav, eye);
			}

			if (hintsFader.getInterstate()>0.00001 && n->mag <= getMaxMagHints())
			{
				n->draw_name(prj);
				n->draw_circle(prj, nav);
			} 
		}
	}
	drawPointer(prj, nav);
	//nebGrid.draw(prj, p);

	return 0;
}

void NebulaMgr::drawPointer(const Projector* prj, const Navigator * nav)
{
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

		float size = obj->getOnScreenSize(prj, nav);

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
	setNamesColor(StelUtils::str_to_vec3f(conf->value(section+"/nebula_label_color", defaultColor).toString()));
	setCirclesColor(StelUtils::str_to_vec3f(conf->value(section+"/nebula_circle_color", defaultColor).toString()));
}

// search by name
StelObject* NebulaMgr::search(const string& name)
{
	string uname = name;
	transform(uname.begin(), uname.end(), uname.begin(), ::toupper);
	vector <Nebula*>::const_iterator iter;

	for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
	{
		string testName = (*iter)->getEnglishName();
		transform(testName.begin(), testName.end(), testName.begin(), ::toupper);
		//		if(testName != "" ) cout << ">" << testName << "< " << endl;
		if (testName==uname) return *iter;
	}
	
	// If no match found, try search by catalog reference

 	string         cat;
	istringstream  iss_cats;
	string         n = name;
	int            num;
	bool           catfound = false;

	iss_cats.str("M NGC ARP TCP");

	while(iss_cats >> cat) {
		if ( ! n.find(cat, 0) ) {
			n.replace(0, cat.length(), "");
			catfound = true;
			break;
		} 
	}

	if ( ! catfound ) { 
		return NULL;
	}

	istringstream ss(n); 
	ss >> num;
	if ( ss.fail() ) {
		return NULL;
	}
	
	if (cat == "M") return searchM(num);
	if (cat == "NGC") return searchNGC(num);
	if (cat == "IC") return searchIC(num);
	// if (cat == "UGC") return searchUGC(num);
	return NULL;
}

void NebulaMgr::loadNebulaSet(const QString& setName)
{
	try
	{
		loadNGC(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000.dat"));
		loadNGCNames(StelApp::getInstance().getFileMgr().findFile("nebulae/" + setName + "/ngc2000names.dat"));
		loadTextures(setName);
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
	for(iter=neb_array.begin();iter!=neb_array.end();iter++)
	{
		if ((*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2]>anglePlusProche)
		{
			anglePlusProche=(*iter)->XYZ[0]*Pos[0]+(*iter)->XYZ[1]*Pos[1]+(*iter)->XYZ[2]*Pos[2];
			plusProche=(*iter);
		}
	}
	if (anglePlusProche>Nebula::RADIUS_NEB*0.999)
	{
		return plusProche;
	}
	else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<StelObjectP> NebulaMgr::searchAround(const Vec3d& av, double limitFov, const StelCore* core) const
{
	vector<StelObjectP> result;
	if (!getFlagShow())
		return result;
		
	Vec3d v(av);
	v.normalize();
	double cos_lim_fov = cos(limitFov * M_PI/180.);
	static Vec3d equPos;

	vector<Nebula*>::const_iterator iter = neb_array.begin();
	while (iter != neb_array.end())
	{
		equPos = (*iter)->XYZ;
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cos_lim_fov)
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
	for(iter=neb_array.begin();iter!=neb_array.end();iter++)
	{
		if ((*iter)->M_nb == M) return (*iter);
	}

	return NULL;
}

Nebula *NebulaMgr::searchNGC(unsigned int NGC)
{
	vector<Nebula *>::iterator iter;
	for(iter=neb_array.begin();iter!=neb_array.end();++iter)
	{
		if ((*iter)->NGC_nb == NGC)
			return (*iter);
	}
	return NULL;
}

Nebula *NebulaMgr::searchIC(unsigned int IC)
{
	vector<Nebula *>::iterator iter;
	for(iter=neb_array.begin();iter!=neb_array.end();iter++)
	{
		if ((*iter)->IC_nb == IC) return (*iter);
	}

	return NULL;
}

/*StelObject NebulaMgr::searchUGC(unsigned int UGC)
{
	vector<Nebula *>::iterator iter;
	for(iter=neb_array.begin();iter!=neb_array.end();iter++)
	{
		if ((*iter)->UGC_nb == UGC) return (*iter);
	}

	return NULL;
}*/

// read from stream
bool NebulaMgr::loadNGC(const QString& catNGC)
{
	char recordstr[512];
	unsigned int i;
	unsigned int data_drop;

	LoadingBar& lb = *StelApp::getInstance().getLoadingBar();
	
	cout << "Loading NGC data... ";
	FILE * ngcFile = fopen(QFile::encodeName(catNGC).constData(),"rb");
	if (!ngcFile)
	{
		qWarning() << "NGC data file " << catNGC << " not found";
		return false;
	}

	// read the number of lines
	unsigned int catalogSize = 0;
	while (fgets(recordstr,512,ngcFile)) catalogSize++;
	rewind(ngcFile);

	// Read the NGC entries
	i = 0;
	data_drop = 0;
	while (fgets(recordstr,512,ngcFile)) // temporary for testing
	{
		if (!(i%200) || (i == catalogSize-1))
		{
			// Draw loading bar
			lb.SetMessage(QString("Loading NGC catalog: %1/%2").arg((i == catalogSize-1 ? catalogSize : i))
			                                                   .arg(catalogSize));
			lb.Draw((float)i/catalogSize);
		}
		Nebula *e = new Nebula;
		if (!e->readNGC(recordstr)) // reading error
		{
			delete e;
			e = NULL;
			data_drop++;
		}
		else
		{
			neb_array.push_back(e);
			nebGrid.insert(e);
		}
		i++;
	}
	fclose(ngcFile);
	printf("(%d items loaded [%d dropped])\n", i, data_drop);
    //printf("Grid depth = %d\n", nebGrid.depth());
	return true;
}


bool NebulaMgr::loadNGCNames(const QString& catNGCNames)
{
	char recordstr[512];
	cout << "Loading NGC name data...";
	FILE * ngcNameFile = fopen(QFile::encodeName(catNGCNames).constData(),"rb");
	if (!ngcNameFile)
	{
		qWarning() << "NGC name data file " << catNGCNames << " not found.";
		return false;
	}

	// Read the names of the NGC objects
	int i = 0;
	char n[40];
	int nb, k;
	Nebula *e;

	while (fgets(recordstr,512,ngcNameFile))
	{

		sscanf(&recordstr[38],"%d",&nb);
		if (recordstr[37] == 'I')
		{
			e = (Nebula*)searchIC(nb);
		}
		else
		{
			e = (Nebula*)searchNGC(nb);
		}

		strncpy(n, recordstr, 36);
		// trim the white spaces at the back
		n[36] = '\0';
		k = 36;
		while (n[--k] == ' ' && k >= 0)
		{
			n[k] = '\0';
		}
		
		if (e)
		{
			// If the name is not a messier number perhaps one is already
			// defined for this object
			if (strncmp(n, "M ",2))
			{
				e->englishName = n;
			}

			// If it's a messiernumber, we will call it a messier if there is no better name
			if (!strncmp(n, "M ",2))
			{
				istringstream iss(string(n).substr(1));  // remove the 'M'
				ostringstream oss;

				int num;

				// Let us keep the right number in the Messier catalog
				iss >> num;
				e->M_nb=(unsigned int)(num);
				
				oss << "M" << num;
				e->englishName = oss.str();
			}
		}
		else
			cerr << endl << "...no position data for " << n;
		i++;
	}
	fclose(ngcNameFile);
	cout << "( " << i << " names loaded)" << endl;

	return true;
}

bool NebulaMgr::loadTextures(const QString& setName)
{
	LoadingBar& lb = *StelApp::getInstance().getLoadingBar();
	cout << "Loading Nebula Textures for set " << qPrintable(setName) << "...";
	QString texFile;
	try
	{
		texFile = StelApp::getInstance().getFileMgr().findFile("nebulae/"+setName+"/nebula_textures.fab");
		std::ifstream inf(QFile::encodeName(texFile).constData());
		if (!inf.is_open())
		{
			throw (runtime_error(string("cannot open file for reading: ") + qPrintable(texFile)));
		}
	
		// determine total number to be loaded for percent complete display
		string record;
		int total=0;
		while(!getline(inf, record).eof())
		{
			if (record.empty() || record[0]=='#')
				continue;
			++total;
		}
		inf.clear();
		inf.seekg(0);

		int current = 0;
		int NGC;
		while(!getline(inf, record).eof())
		{
			// Draw loading bar
			if (record.empty() || record[0]=='#')
				continue;

			++current;
			lb.SetMessage(QString("Loading Nebula Textures:%1/%2").arg(current).arg(total));
			lb.Draw((float)current/total);

			istringstream istr(record);
			istr >> NGC;

			Nebula *e = (Nebula*)searchNGC(NGC);
			if (e)
			{
				if (!e->readTexture(setName, record)) // reading error
				{
					cerr << "Error while reading nebula texture " << qPrintable(setName) << "/" << e->englishName << endl;
					cerr << "Record was " << record << endl;
				}
			} 
			else
			{
				// Allow non NGC nebulas/textures!
				if (NGC != -1)
					cout << "Nebula with unrecognized NGC number " << NGC << endl;
				e = new Nebula;
				if (!e->readTexture(setName, record))
				{
					// reading error
					cerr << "Error while reading texture " << e->englishName << endl;
					delete e;
				} 
				else 
				{
					neb_array.push_back(e);
					nebGrid.insert(e);
				}
			}
	
		}
		cout << "(" << total << " textures loaded)" << endl;
		return true;	
	}
	catch (exception& e)
	{
		qWarning() << "ERROR: unable to load nebula texture set \"" << setName << "\": " << e.what();
		return false;		
	}
}

void NebulaMgr::updateI18n()
{
	Translator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	vector<Nebula*>::iterator iter;
	for( iter = neb_array.begin(); iter < neb_array.end(); iter++ )
	{
		(*iter)->translateName(trans);
	}
	double fontSize = 12;
	Nebula::nebula_font = &StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}


//! Return the matching Nebula object's pointer if exists or NULL
StelObjectP NebulaMgr::searchByNameI18n(const wstring& nameI18n) const
{
	wstring objw = nameI18n;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	vector <Nebula*>::const_iterator iter;
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.substr(0, 3) == L"NGC")
	{
		for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
		{
			if ((L"NGC" + StelUtils::intToWstring((*iter)->NGC_nb)) == objw ||
			 (L"NGC " + StelUtils::intToWstring((*iter)->NGC_nb)) == objw)
			 return *iter;
		}
	}
	
	// Search by common names
	for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
	{
		wstring objwcap = (*iter)->nameI18;
		transform(objwcap.begin(), objwcap.end(), objwcap.begin(), ::toupper);
		if (objwcap==objw) return *iter;
	}
	
	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.substr(0, 1) == L"M")
	{
		for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
		{
			if ((L"M" + StelUtils::intToWstring((*iter)->M_nb)) == objw ||
			 (L"M " + StelUtils::intToWstring((*iter)->M_nb)) == objw)
			 return *iter;
		}
	}
	
	return NULL;
}


//! Return the matching Nebula object's pointer if exists or NULL
StelObjectP NebulaMgr::searchByName(const string& name) const
{
	string objw = name;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	vector <Nebula*>::const_iterator iter;
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	if (objw.substr(0, 3) == "NGC")
	{
		for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
		{
			if (("NGC" + StelUtils::intToString((*iter)->NGC_nb)) == objw ||
			 ("NGC " + StelUtils::intToString((*iter)->NGC_nb)) == objw)
			 return *iter;
		}
	}
	
	// Search by common names
	for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
	{
		string objwcap = (*iter)->englishName;
		transform(objwcap.begin(), objwcap.end(), objwcap.begin(), ::toupper);
		if (objwcap==objw) return *iter;
	}
	
	// Search by Messier numbers (possible formats are "M31" or "M 31")
	if (objw.substr(0, 1) == "M")
	{
		for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
		{
			if (("M" + StelUtils::intToString((*iter)->M_nb)) == objw ||
			 ("M " + StelUtils::intToString((*iter)->M_nb)) == objw)
			 return *iter;
		}
	}
	
	return NULL;
}


//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
vector<wstring> NebulaMgr::listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem) const
{
	vector<wstring> result;
	if (maxNbItem==0) return result;
		
	wstring objw = objPrefix;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	
	vector <Nebula*>::const_iterator iter;
	
	// Search by messier objects number (possible formats are "M31" or "M 31")
	if (objw.size() >= 1 && objw[0]==L'M')
	{
		for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
		{
			if ((*iter)->M_nb==0) continue;
			wstring constw = L"M" + StelUtils::intToWstring((*iter)->M_nb);
			wstring constws = constw.substr(0, objw.size());
			if (constws==objw)
			{
				result.push_back(constw);
				continue;	// Prevent adding both forms for name
			}
			constw = L"M " + StelUtils::intToWstring((*iter)->M_nb);
			constws = constw.substr(0, objw.size());
			if (constws==objw)
			{
				result.push_back(constw);
			}
		}
	}
	
	// Search by NGC numbers (possible formats are "NGC31" or "NGC 31")
	for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
	{
		if ((*iter)->NGC_nb==0) continue;
		wstring constw = L"NGC" + StelUtils::intToWstring((*iter)->NGC_nb);
		wstring constws = constw.substr(0, objw.size());
		if (constws==objw)
		{
			result.push_back(constw);
			continue;
		}
		constw = L"NGC " + StelUtils::intToWstring((*iter)->NGC_nb);
		constws = constw.substr(0, objw.size());
		if (constws==objw)
		{
			result.push_back(constw);
		}
	}
	
	// Search by common names
	for (iter = neb_array.begin(); iter != neb_array.end(); ++iter)
	{
		wstring constw = (*iter)->nameI18.substr(0, objw.size());
		transform(constw.begin(), constw.end(), constw.begin(), ::toupper);
		if (constw==objw)
		{
			result.push_back((*iter)->nameI18);
		}
	}
	
	sort(result.begin(), result.end());
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());
	
	return result;
}

