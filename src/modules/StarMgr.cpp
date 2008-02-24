/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 *
 * The big star catalogue extension to Stellarium:
 * Author and Copyright: Johannes Gajdosik, 2006
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

// class used to manage groups of Stars

#include <config.h>
#include <QTextStream>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QRegExp>
#include <QDebug>

#include "Projector.hpp"
#include "StarMgr.hpp"
#include "StelObject.hpp"
#include "STexture.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "ToneReproducer.hpp"
#include "Translator.hpp"
#include "GeodesicGrid.hpp"
#include "LoadingBar.hpp"
#include "Translator.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "bytes.h"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelIniParser.hpp"

#include "ZoneArray.hpp"
#include "StringArray.hpp"

#include <string>
#include <list>

#include <errno.h>
#include <string.h>
#include <unistd.h>

using namespace BigStarCatalogExtension;

static StringArray spectral_array;
static StringArray component_array;

string StarMgr::convertToSpectralType(int index) {
  if (index < 0 || index >= spectral_array.getSize()) {
    cout << "convertToSpectralType: bad index: " << index
         << ", max: " << spectral_array.getSize() << endl;
    return "";
  }
  return spectral_array[index];
}

string StarMgr::convertToComponentIds(int index) {
  if (index < 0 || index >= component_array.getSize()) {
    cout << "convertToComponentIds: bad index: " << index
         << ", max: " << component_array.getSize() << endl;
    return "";
  }
  return component_array[index];
}










void StarMgr::initTriangle(int lev,int index,
                           const Vec3d &c0,
                           const Vec3d &c1,
                           const Vec3d &c2) {
  ZoneArrayMap::const_iterator it(zone_arrays.find(lev));
  if (it!=zone_arrays.end()) it->second->initTriangle(index,c0,c1,c2);
}


StarMgr::StarMgr(void) :
    hip_index(new HipIndexStruct[NR_OF_HIP+1]),
	fontSize(13.),
    starFont(0)
{
	setObjectName("StarMgr");
  if (hip_index == 0) {
    cerr << "ERROR: StarMgr::StarMgr: no memory" << endl;
    exit(1);
  }
  max_geodesic_grid_level = -1;
  last_max_search_level = -1;
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double StarMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return StelApp::getInstance().getModuleMgr().getModule("ConstellationMgr")->getCallOrder(actionName)+10;
	return 0;
}


StarMgr::~StarMgr(void) {
  ZoneArrayMap::iterator it(zone_arrays.end());
  while (it!=zone_arrays.begin()) {
    --it;
    delete it->second;
    it->second = NULL;
  }
  zone_arrays.clear();
  if (hip_index) delete[] hip_index;
}

bool StarMgr::flagSciNames = true;
double StarMgr::current_JDay = 0;
map<int,string> StarMgr::common_names_map;
map<int,wstring> StarMgr::common_names_map_i18n;
map<string,int> StarMgr::common_names_index;
map<wstring,int> StarMgr::common_names_index_i18n;

map<int,wstring> StarMgr::sci_names_map_i18n;
map<wstring,int> StarMgr::sci_names_index_i18n;

wstring StarMgr::getCommonName(int hip) {
  map<int,wstring>::const_iterator it(common_names_map_i18n.find(hip));
  if (it!=common_names_map_i18n.end()) return it->second;
  return L"";
}

wstring StarMgr::getSciName(int hip) {
  map<int,wstring>::const_iterator it(sci_names_map_i18n.find(hip));
  if (it!=sci_names_map_i18n.end()) return it->second;
  return L"";
}




void StarMgr::init() {
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	load_data();
	double fontSize = 12;
	starFont = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getSkyLanguage(), fontSize);

	setFlagStars(conf->value("astro/flag_stars", true).toBool());
	setFlagNames(conf->value("astro/flag_star_name",true).toBool());
	setMaxMagName(conf->value("stars/max_mag_star_name",1.5).toDouble());
	
	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);

	StelApp::getInstance().getTextureManager().setDefaultParams();
	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur2.png");   // Load pointer texture
}

void StarMgr::setGrid(GeodesicGrid* geodesic_grid) {
  geodesic_grid->visitTriangles(max_geodesic_grid_level,initTriangleFunc,this);
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
    it->second->scaleAxis();
  }
}


void StarMgr::drawPointer(const Projector* prj, const Navigator * nav)
{
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Star");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getObsJ2000Pos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
	
		glColor3fv(obj->getInfoColor());
		float diameter = 26.f;
		texPointer->bind();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
        prj->drawSprite2dMode(screenpos[0], screenpos[1], diameter, StelApp::getInstance().getTotalRunTime()*40.);
	}
}

void StarMgr::setColorScheme(const QSettings* conf, const QString& section)
{
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelColor(StelUtils::str_to_vec3f(conf->value(section+"/star_label_color", defaultColor).toString()));
}

/***************************************************************************
 Load star catalogue data from files.
 If a file is not found, it will be skipped.
***************************************************************************/
void StarMgr::load_data()
{
	LoadingBar& lb = *StelApp::getInstance().getLoadingBar();
			
	// Please do not init twice:
	assert(max_geodesic_grid_level < 0);

	cout << "Loading star data..." << endl;

	QString iniFile;
	try
	{
		iniFile = StelApp::getInstance().getFileMgr().findFile("stars/default/stars.ini");
	}
	catch (exception& e)
	{
		qWarning() << "ERROR - could not find stars/default/stars.ini : " << e.what() << iniFile;
		return;
	}

	QSettings conf(iniFile, StelIniFormat);
	if (conf.status() != QSettings::NoError)
	{
		qWarning() << "ERROR while parsing " << iniFile;
		return;
	}
				         
	for (int i=0; i<100; i++)
	{
		char key_name[64];
		sprintf(key_name,"cat_file_name_%02d",i);
		const QString cat_file_name = conf.value(QString("stars/")+key_name,"").toString();
		if (!cat_file_name.isEmpty()) {
			lb.SetMessage(q_("Loading catalog %1").arg(cat_file_name));
			ZoneArray *const z = ZoneArray::create(*this,cat_file_name,lb);
			if (z)
			{
				if (max_geodesic_grid_level < z->level)
				{
					max_geodesic_grid_level = z->level;
				}
				ZoneArray *&pos(zone_arrays[z->level]);
				if (pos)
				{
					cerr << qPrintable(cat_file_name) << ", " << z->level
						 << ": duplicate level" << endl;
					delete z;
				}
				else
				{
					pos = z;
				}
			}
		}
	}

	for (int i=0; i<=NR_OF_HIP; i++)
	{
		hip_index[i].a = 0;
		hip_index[i].z = 0;
		hip_index[i].s = 0;
	}
	for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
	                it != zone_arrays.end();it++)
	{
		it->second->updateHipIndex(hip_index);
	}

	const QString cat_hip_sp_file_name = conf.value("stars/cat_hip_sp_file_name","").toString();
	if (cat_hip_sp_file_name.isEmpty())
	{
		cerr << "ERROR: stars:cat_hip_sp_file_name not found" << endl;
	}
	else
	{
		try
		{
			spectral_array.initFromFile(StelApp::getInstance().getFileMgr().findFile("stars/default/" + cat_hip_sp_file_name));
		}
		catch (exception& e)
		{
			qWarning() << "ERROR while loading data from "
			     << ("stars/default/" + cat_hip_sp_file_name)
		    	 << ": " << e.what();
		}
	}

	const QString cat_hip_cids_file_name = conf.value("stars/cat_hip_cids_file_name","").toString();
	if (cat_hip_cids_file_name.isEmpty())
	{
		cerr << "ERROR: stars:cat_hip_cids_file_name not found" << endl;
	}
	else
	{
		try
		{
			component_array.initFromFile(StelApp::getInstance().getFileMgr()
			        .findFile("stars/default/" + cat_hip_cids_file_name));
		}
		catch (exception& e)
		{
			qWarning() << "ERROR while loading data from "
			     << ("stars/default/" + cat_hip_cids_file_name)
		    	 << ": " << e.what() << endl;
		}
	}

	last_max_search_level = max_geodesic_grid_level;
	cout << "finished, max_geodesic_level: " << max_geodesic_grid_level << endl;
}

// Load common names from file 
int StarMgr::load_common_names(const QString& commonNameFile) {
  common_names_map.clear();
  common_names_map_i18n.clear();
  common_names_index.clear();
  common_names_index_i18n.clear();

  cout << "Loading star names from " << qPrintable(commonNameFile) << endl;

  FILE *cnFile;
  cnFile=fopen(QFile::encodeName(commonNameFile).constData(),"r");
  if (!cnFile)
  {
	  cerr << "Warning " << qPrintable(commonNameFile) << " not found." << endl;
    return 0;
  }

  // Assign names to the matching stars, now support spaces in names
  char line[256];
  while (fgets(line, sizeof(line), cnFile)) {
    line[sizeof(line)-1] = '\0';
    unsigned int hip;
    if (sscanf(line,"%u",&hip)!=1) {
		cerr << "ERROR: StarMgr::load_common_names(" << qPrintable(commonNameFile)
           << "): bad line: \"" << line << '"' << endl;
      qFatal("Error while loading star common names.");
    }
    unsigned int i = 0;
    while (line[i]!='|' && i<sizeof(line)-2) ++i;
    i++;
    QString englishCommonName =  line+i;
    // remove newline
    englishCommonName.chop(1);

//cout << hip << ": \"" << englishCommonName << '"' << endl;

    // remove underscores
    englishCommonName.replace(QChar('_'), "_");
    const wstring commonNameI18n = q_(englishCommonName).toStdWString();
    wstring commonNameI18n_cap = commonNameI18n;
    transform(commonNameI18n.begin(), commonNameI18n.end(),
              commonNameI18n_cap.begin(), ::toupper);

    common_names_map[hip] = englishCommonName.toStdString();
    common_names_index[englishCommonName.toStdString()] = hip;
    common_names_map_i18n[hip] = commonNameI18n;
    common_names_index_i18n[commonNameI18n_cap] = hip;
  }

  fclose(cnFile);
  return 1;
}


// Load scientific names from file 
void StarMgr::load_sci_names(const QString& sciNameFile)
{
  sci_names_map_i18n.clear();
  sci_names_index_i18n.clear();

  cout << "Loading star sci names from " << qPrintable(sciNameFile) << endl;

  FILE *snFile;
  snFile=fopen(QFile::encodeName(sciNameFile).constData(),"r");
  if (!snFile)
  {
	cerr << "Warning " << qPrintable(sciNameFile) << " not found." << endl;
	return;
	}

  // Assign names to the matching stars, now support spaces in names
  char line[256];
  while (fgets(line, sizeof(line), snFile)) {
    line[sizeof(line)-1] = '\0';
    unsigned int hip;
    if (sscanf(line,"%u",&hip)!=1) {
		cerr << "ERROR: StarMgr::load_sci_names(" << qPrintable(sciNameFile)
           << "): bad line: \"" << line << '"' << endl;
		qFatal("Error while loading star sci names.");
    }
	if (sci_names_map_i18n.find(hip)!=sci_names_map_i18n.end())
		continue;
    unsigned int i = 0;
    while (line[i]!='|' && i<sizeof(line)-2) ++i;
    i++;
    char *tempc = line+i;
    string sci_name = tempc;
    sci_name.erase(sci_name.length()-1, 1);
    wstring sci_name_i18n = StelUtils::stringToWstring(sci_name);

    // remove underscores
    for (wstring::size_type j=0;j<sci_name_i18n.length();++j) {
      if (sci_name_i18n[j]==L'_') sci_name_i18n[j]=L' ';
    }

    wstring sci_name_i18n_cap = sci_name_i18n;
    transform(sci_name_i18n.begin(), sci_name_i18n.end(),
              sci_name_i18n_cap.begin(), ::toupper);
    
    sci_names_map_i18n[hip] = sci_name_i18n;
    sci_names_index_i18n[sci_name_i18n_cap] = hip;
  }

  fclose(snFile);
}


int StarMgr::getMaxSearchLevel() const
{
  int rval = -1;
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
    const float mag_min = 0.001f*it->second->mag_min;
    float rcmag[2];
    if (StelApp::getInstance().getCore()->getSkyDrawer()->computeRCMag(mag_min,rcmag) < 0)
		break;
    rval = it->first;
  }
  return rval;
}


// Draw all the stars
double StarMgr::draw(StelCore* core)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	SkyDrawer* skyDrawer = core->getSkyDrawer();
	
    current_JDay = nav->getJDay();

    // If stars are turned off don't waste time below
    // projecting all stars just to draw disembodied labels
    if(!starsFader.getInterstate()) return 0.;

	int max_search_level = getMaxSearchLevel();
	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid()->search(prj->unprojectViewport(),max_search_level);

    // Set temporary static variable for optimization
    const float names_brightness = names_fader.getInterstate() * starsFader.getInterstate();
    
    prj->setCurrentFrame(Projector::FRAME_J2000);

	// Prepare openGL for drawing many stars
	skyDrawer->prepareDraw();

    // draw all the stars of all the selected zones
    float rcmag_table[2*256];
//static int count = 0;
//count++;
	
    for (ZoneArrayMap::const_iterator it(zone_arrays.begin()); it!=zone_arrays.end();it++)
	{
		const float mag_min = 0.001f*it->second->mag_min;
		// if (maxMag < mag_min) break;
		const float k = (0.001f*it->second->mag_range)/it->second->mag_steps;
		for (int i=it->second->mag_steps-1;i>=0;i--)
		{
			const float mag = mag_min+k*i;
			if (skyDrawer->computeRCMag(mag,rcmag_table + 2*i) < 0)
			{
				if (i==0) goto exit_loop;
			}
			if (skyDrawer->getFlagPointStar())
			{
				rcmag_table[2*i+1] *= starsFader.getInterstate();
			}
			else
			{
				rcmag_table[2*i] *= starsFader.getInterstate();
			}
		}
		last_max_search_level = it->first;
	
		unsigned int max_mag_star_name = 0;
		if (names_fader.getInterstate())
		{
			int x = (int)((maxMagStarName-mag_min)/k);
			if (x > 0) max_mag_star_name = x;
		}
		int zone;
		for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);(zone = it1.next()) >= 0;)
		{
			it->second->draw(zone,true,rcmag_table,prj,
							max_mag_star_name,names_brightness,
							starFont);
		}
		for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first);(zone = it1.next()) >= 0;)
		{
			it->second->draw(zone,false,rcmag_table,prj,
							max_mag_star_name,names_brightness,
							starFont);
		}
    }
    exit_loop:

	drawPointer(prj, nav);

    return 0.;
}






// Look for a star by XYZ coords
StelObjectP StarMgr::search(Vec3d pos) const {
assert(0);
  pos.normalize();
  vector<StelObjectP > v = searchAround(pos,
                                        0.8, // just an arbitrary number
                                        NULL);
  StelObjectP nearest;
  double cos_angle_nearest = -10.0;
  for (vector<StelObjectP >::const_iterator it(v.begin());it!=v.end();it++) {
    const double c = (*it)->getObsJ2000Pos(0)*pos;
    if (c > cos_angle_nearest) {
      cos_angle_nearest = c;
      nearest = *it;
    }
  }
  return nearest;
}

// Return a stl vector containing the stars located
// inside the lim_fov circle around position v
vector<StelObjectP > StarMgr::searchAround(const Vec3d& vv,
                                           double lim_fov, // degrees
										   const StelCore* core) const {
  vector<StelObjectP > result;
  if (!getFlagStars())
  	return result;
  	
  Vec3d v(vv);
  v.normalize();
    // find any vectors h0 and h1 (length 1), so that h0*v=h1*v=h0*h1=0
  int i;
  {
    const double a0 = fabs(v[0]);
    const double a1 = fabs(v[1]);
    const double a2 = fabs(v[2]);
    if (a0 <= a1) {
      if (a0 <= a2) i = 0;
      else i = 2;
    } else {
      if (a1 <= a2) i = 1;
      else i = 2;
    }
  }
  Vec3d h0(0.0,0.0,0.0);
  h0[i] = 1.0;
  Vec3d h1 = h0 ^ v;
  h1.normalize();
  h0 = h1 ^ v;
  h0.normalize();
    // now we have h0*v=h1*v=h0*h1=0.
    // construct a region with 4 corners e0,e1,e2,e3 inside which
    // all desired stars must be:
  double f = 1.4142136 * tan(lim_fov * M_PI/180.0);
  h0 *= f;
  h1 *= f;
  Vec3d e0 = v + h0;
  Vec3d e1 = v + h1;
  Vec3d e2 = v - h0;
  Vec3d e3 = v - h1;
  f = 1.0/e0.length();
  e0 *= f;
  e1 *= f;
  e2 *= f;
  e3 *= f;
    // search the triangles
 	const GeodesicSearchResult* geodesic_search_result = core->getGeodesicGrid()->search(e0,e1,e2,e3,last_max_search_level);
    // iterate over the stars inside the triangles:
  f = cos(lim_fov * M_PI/180.);
  for (ZoneArrayMap::const_iterator it(zone_arrays.begin());
       it!=zone_arrays.end();it++) {
//cout << "search inside(" << it->first << "):";
    int zone;
    for (GeodesicSearchInsideIterator it1(*geodesic_search_result,it->first);
         (zone = it1.next()) >= 0;) {
      it->second->searchAround(zone,v,f,result);
//cout << " " << zone;
    }
//cout << endl << "search border(" << it->first << "):";
    for (GeodesicSearchBorderIterator it1(*geodesic_search_result,it->first);
         (zone = it1.next()) >= 0;) {
      it->second->searchAround(zone,v,f,result);
//cout << " " << zone;
    }
//cout << endl << endl;
  }
  return result;
}






//! Update i18 names from english names according to passed translator.
//! The translation is done using gettext with translated strings defined in translations.h
void StarMgr::updateI18n() {
  Translator trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
  common_names_map_i18n.clear();
  common_names_index_i18n.clear();
  for (map<int,string>::iterator it(common_names_map.begin());
       it!=common_names_map.end();it++) {
    const int i = it->first;
    const wstring t(trans.translate(QString::fromStdString(it->second)).toStdWString());
    common_names_map_i18n[i] = t;
    wstring t_cap = t;
    transform(t.begin(), t.end(), t_cap.begin(), ::toupper);
    common_names_index_i18n[t_cap] = i;
  }
  starFont = &StelApp::getInstance().getFontManager().getStandardFont(trans.getTrueLocaleName(), fontSize);
}


StelObjectP StarMgr::search(const string& name) const
{
    const string catalogs("HP HD SAO");

    string n = name;
    for (string::size_type i=0;i<n.length();++i)
    {
        if (n[i]=='_') n[i]=' ';
    }    
    
    istringstream ss(n);
    string cat;
    unsigned int num;
    
    ss >> cat;
    
    // check if a valid catalog reference
    if (catalogs.find(cat,0) == string::npos)
    {
        // try see if the string is a HP number
        istringstream cat_to_num(cat);
        cat_to_num >> num;
        if (!cat_to_num.fail()) return searchHP(num);
        return NULL;
    }

    ss >> num;
    if (ss.fail()) return NULL;

    if (cat == "HP") return searchHP(num);
    assert(0);
    return NULL;
}    

// Search the star by HP number
StelObjectP StarMgr::searchHP(int _HP) const {
  if (0 < _HP && _HP <= NR_OF_HIP) {
    const Star1 *const s = hip_index[_HP].s;
    if (s) {
      const SpecialZoneArray<Star1> *const a = hip_index[_HP].a;
      const SpecialZoneData<Star1> *const z = hip_index[_HP].z;
      return s->createStelObject(a,z);
    }
  }
  return StelObjectP();
}

StelObjectP StarMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	// Search by HP number if it's an HP formated number
	QRegExp rx("^\\s*HP\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx.exactMatch(objw))
	{
		return searchHP(rx.capturedTexts().at(1).toInt());
	}

	// Search by I18n common name
	map<wstring,int>::const_iterator it(common_names_index_i18n.find(objw.toStdWString()));
	if (it!=common_names_index_i18n.end()) 
	{
		return searchHP(it->second);
	}

	// Search by sci name
	it = sci_names_index_i18n.find(objw.toStdWString());
	if (it!=sci_names_index_i18n.end()) 
	{
		return searchHP(it->second);
	}

	return StelObjectP();
}


StelObjectP StarMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();

	// Search by HP number if it's an HP formated number
	QRegExp rx("^\\s*HP\\s*(\\d+)\\s*$", Qt::CaseInsensitive);
	if (rx.exactMatch(objw))
	{
		return searchHP(rx.capturedTexts().at(1).toInt());
	}


	/* Should we try this anyway?
	// Search by common name
	map<wstring,int>::const_iterator it(common_names_index_i18n.find(objw));

	if (it!=common_names_index_i18n.end()) {
		return searchHP(it->second);
	} */

	// Search by sci name
	map<wstring,int>::const_iterator it = sci_names_index_i18n.find(objw.toStdWString());
	if (it!=sci_names_index_i18n.end()) {
		return searchHP(it->second);
	}

	return StelObjectP();
}

//! Find and return the list of at most maxNbItem objects auto-completing
//! the passed object I18n name
QStringList StarMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const 
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();

	// Search for common names
	for (map<wstring,int>::const_iterator it(common_names_index_i18n.lower_bound(objw.toStdWString()));
	     it!=common_names_index_i18n.end();
	     it++) 
	{
		const QString constw(QString::fromStdWString(it->first.substr(0,objw.size())));
		if (constw==objw) {
			if (maxNbItem==0) break;
			result << QString::fromStdWString(getCommonName(it->second));
			maxNbItem--;
		} 
		else 
			break;
	}

	// Search for sci names
	for (map<wstring,int>::const_iterator it(sci_names_index_i18n.lower_bound(objw.toStdWString()));
	     it!=sci_names_index_i18n.end();
	     it++) 
	{
		const QString constw(QString::fromStdWString(it->first.substr(0,objw.size())));
		if (constw==objw) {
			if (maxNbItem==0) break;
			result << QString::fromStdWString(getSciName(it->second));
			maxNbItem--;
		} 
		else 
			break;
	}

	result.sort();
	return result;
}


//! Define font file name and size to use for star names display
void StarMgr::setFontSize(double newFontSize)
{
	fontSize = newFontSize;
	starFont = &StelApp::getInstance().getFontManager().getStandardFont(
		StelApp::getInstance().getLocaleMgr().getSkyLanguage(),fontSize);
}

void StarMgr::updateSkyCulture()
{
	QString skyCultureDir = StelApp::getInstance().getSkyCultureMgr().getSkyCultureDir();
	
	// Load culture star names in english
	try
	{
		load_common_names(StelApp::getInstance().getFileMgr().findFile("skycultures/" + skyCultureDir + "/star_names.fab"));
	}
	catch(exception& e)
	{
		cout << "WARNING: could not load star_names.fab for sky culture " 
			<< qPrintable(skyCultureDir) << ": " << e.what() << endl;	
	}
	
	try
	{
		load_sci_names(StelApp::getInstance().getFileMgr().findFile("stars/default/name.fab"));
	}
	catch(exception& e)
	{
		cout << "WARNING: could not load scientific star names file: " << e.what() << endl;	
	}

	// Turn on sci names/catalog names for western culture only
	setFlagSciNames(skyCultureDir.startsWith("western"));
	updateI18n();
}
