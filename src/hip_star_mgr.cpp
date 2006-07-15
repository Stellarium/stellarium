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

// class used to manage groups of Stars

#include <string>
#include "hip_star_mgr.h"
#include "hip_star.h"
#include "stel_object.h"
#include "s_texture.h"
#include "grid.h"
#include "bytes.h"
#include "stellarium.h"
#include "navigator.h"
#include "stel_utility.h"
#include "translator.h"

#define RADIUS_STAR 1.

// construct and load all data
HipStarMgr::HipStarMgr() :
	limitingMag(6.5f),
	starZones(NULL),
	StarArray(NULL),
	starArraySize(0), // This is the full data array including HP, HP SAO (not duplicated)
	starTexture(NULL)
{
	starZones = new vector<HipStar*>[HipGrid.getNbPoints()];
}


HipStarMgr::~HipStarMgr()
{
	delete starTexture;
	starTexture=NULL;

///	delete starcTexture;
///	starcTexture=NULL;
	
	delete HipStar::starFont;
	HipStar::starFont=NULL;

	delete [] starZones;
	starZones=NULL;

	delete [] StarArray;
	StarArray=NULL;

	delete [] StarFlatArray;
	StarFlatArray=NULL;
}

void HipStarMgr::setLabelColor(const Vec3f& c) {HipStar::label_color = c;}
Vec3f HipStarMgr::getLabelColor(void) {return HipStar::label_color;}

void HipStarMgr::setCircleColor(const Vec3f& c) {HipStar::circle_color = c;}
Vec3f HipStarMgr::getCircleColor(void) {return HipStar::circle_color;}

void HipStarMgr::init(float font_size, const string& font_name, const string& hipCatFile,
	const string& commonNameFile, const string& sciNameFile, LoadingBar& lb)
{
	load_data(hipCatFile, lb);
	load_double(hipCatFile);
	load_variable(hipCatFile);

    load_common_names(commonNameFile);
	load_sci_names(sciNameFile);
	
///	starcTexture = new s_texture("starc64x64.png",TEX_LOAD_TYPE_PNG_BLEND3);  // Load star chart texture
	starTexture = new s_texture("star16x16.png",TEX_LOAD_TYPE_PNG_SOLID);  // Load star texture

    HipStar::starFont = new s_font(font_size, font_name);
    if (!HipStar::starFont)
    {
	    printf("Can't create starFont\n");
        assert(0);
    }	
}

// Load from file ( create the stream and call the Read function )
void HipStarMgr::load_data(const string& hipCatFile, LoadingBar& lb)
{
    cout << "Loading Hipparcos star data...";
    FILE * hipFile;
    hipFile = NULL;

    hipFile=fopen(hipCatFile.c_str(),"rb");
    if (!hipFile)
    {
        cerr << "ERROR " << hipCatFile << " NOT FOUND" << endl;
        assert(0);
    }

	// Read number of stars in the Hipparcos catalog
    unsigned int catalogSize=0;
    fread((char*)&catalogSize,4,1,hipFile);
    LE_TO_CPU_INT32(catalogSize,catalogSize);

    starArraySize = catalogSize;//120417;

    // Create the sequential array
    StarArray = new HipStar[starArraySize];
	
	StarFlatArray = new HipStar*[starArraySize];
	for (int i=0;i<starArraySize;++i)
	{
		StarFlatArray[i] = NULL;
	}
	
	// Read binary file Hipparcos catalog  
	unsigned int data_drop =0;
    HipStar * e = NULL;
    for(int i=0;i<starArraySize;i++)
    {
		if (!(i%2000) || (i == starArraySize-1))
		{
			// Draw loading bar
			lb.SetMessage(_("Loading Hipparcos catalog:") + StelUtility::intToWstring((i == starArraySize-1 ? starArraySize: i)) + 
				L"/" + StelUtility::intToWstring(starArraySize));
			lb.Draw((float)i/starArraySize);
		}
		
		e = &StarArray[i];
		e->HP=(unsigned int)i;
        if (!e->read(hipFile))
        {
			data_drop++;
        	continue;
        }
        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);
		StarFlatArray[e->HP]=e;
    }
    fclose(hipFile);

	printf("(%d stars loaded [%d dropped]).\n", starArraySize-data_drop,data_drop);

    // sort stars by magnitude for faster rendering
    for(int i=0;i < HipGrid.getNbPoints();i++) {
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(HipStarMagComparer()));
    }
}

// Load common names from file 
int HipStarMgr::load_common_names(const string& commonNameFile)
{
	cout << "Load star names from " << commonNameFile << endl;

	// clear existing names (would be faster if they were in separate array
	// since relatively few are named)
    for (int i=0; i<starArraySize; ++i)
    {
		StarArray[i].englishCommonName = "";
		StarArray[i].commonNameI18 = L"";
    }
	
    FILE *cnFile;
    cnFile=fopen(commonNameFile.c_str(),"r");
    if (!cnFile)
    {   
        cerr << "Warning " << commonNameFile << " not found." << endl;
        return 0;
    }

	common_names_map.clear();
//    if (!lstCommonNames.empty())
//    {
//       lstCommonNames.clear();
//       lstCommonNamesHP.clear();
//    }
	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    HipStar *star;
	fgets(line, 256, cnFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = searchHPprivate(tmp);
		if (star)
		{
			char c=line[0];
			int i=0;
			while(c!='|' && i<256){c=line[i];++i;}
			star->englishCommonName =  &(line[i]);
			// remove newline
			star->englishCommonName.erase(star->englishCommonName.length()-1, 1);

			// remove underscores
			for (string::size_type j=0;j<star->englishCommonName.length();++j) {
				if (star->englishCommonName[j]=='_') star->englishCommonName[j]=' ';
			}
			star->commonNameI18 = _(star->englishCommonName.c_str());
            common_names_map.push_back(star);
		}
	} while(fgets(line, 256, cnFile));

    fclose(cnFile);
    return 1;
}


// Load scientific names from file 
void HipStarMgr::load_sci_names(const string& sciNameFile)
{
	// clear existing names (would be faster if they were in separate arrays
	// since relatively few are named)
    for (int i=0; i<starArraySize; i++)
	{
		StarArray[i].sciName = L"";
    }

	FILE *snFile;
    snFile=fopen(sciNameFile.c_str(),"r");
    if (!snFile)
    {   
        cerr << "Warning " << sciNameFile.c_str() << " not found" << endl;
        return;
    }

	sci_names_map.clear();

	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    HipStar *star;
	fgets(line, 256, snFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = searchHPprivate(tmp);
		if (star && star->sciName==L"")
		{
			char c=line[0];
			int i=0;
			while(c!='|' && i<256){c=line[i];++i;}
			char* tempc = &(line[i]);
			while(c!='_' && i<256){c=line[i];++i;}
			line[i-1]=' ';
			string sciName = tempc;
			sciName.erase(sciName.length()-1, 1);
			star->sciName = Translator::UTF8stringToWstring(sciName);
            sci_names_map.push_back(star);
		}
	} while(fgets(line, 256, snFile));

    fclose(snFile);
}

// Draw all the stars
void HipStarMgr::draw(Vec3f equ_vision, ToneReproductor* eye, Projector* prj)
{

	// If stars are turned off don't waste time below
	// projecting all stars just to draw disembodied labels
	if(!starsFader.getInterstate()) return;

	// Set temporary static variable for optimization
	if (flagStarTwinkle) HipStar::twinkle_amount = twinkleAmount;
	else HipStar::twinkle_amount = 0;
	HipStar::star_scale = starScale * starsFader.getInterstate();
	HipStar::star_mag_scale = starMagScale;
	HipStar::gravity_label = gravityLabel;
	HipStar::names_brightness = names_fader.getInterstate() 
		* starsFader.getInterstate();
	HipStar::eye = eye;
	HipStar::proj = prj;
	
	if (flagPointStar) {
		// TODO: fade on/off with starsFader
		if( starsFader.getInterstate())
			drawPoint(equ_vision, eye, prj);
		return;
	}
	
	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
	// Find the star zones which are in the screen
	int nbZones=0;

	// FOV is currently measured vertically, so need to adjust for wide screens
	// TODO: projector should probably use largest measurement itself
	float max_fov = MY_MAX( prj->get_fov(), prj->get_fov()*prj->getViewportWidth()/prj->getViewportHeight());

	nbZones = HipGrid.Intersect(equ_vision, max_fov*M_PI/180.f*1.2f);
	static int * zoneList = HipGrid.getResult();
	//	float maxMag = limiting_mag-1 + 60.f/prj->get_fov();
	float maxMag = limitingMag-1 + 60.f/max_fov;

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<HipStar *>::iterator end;
	static vector<HipStar *>::iterator iter;
	HipStar* h;

	// Bind the star texture
	//if (draw_mode == DM_NORMAL) 
	glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	//else glBindTexture (GL_TEXTURE_2D, starcTexture->getID());

	// Set the draw mode
	//if (draw_mode == DM_NORMAL) 
	glBlendFunc(GL_ONE, GL_ONE);
	//else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // charting

	Vec3d XY;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
	    for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag > maxMag) break;
			if(!prj->project_j2000_check(h->XYZ, XY)) continue;

			// if (draw_mode == DM_NORMAL)	
			//{
				h->draw(XY);
				if (names_fader.getInterstate() && h->Mag < maxMagStarName)
				{
					if (h->draw_name(XY))
						glBindTexture (GL_TEXTURE_2D, starTexture->getID());
				}
			//}
			//else
// 			{
// 				h->draw_chart(XY);
// 				if (names_fader.getInterstate() && h->Mag < maxMagStarName)
// 				{
// 					// need to rebind the star texture after font printing
// 					if (h->draw_name(XY))
// 						glBindTexture (GL_TEXTURE_2D, starcTexture->getID());
// 				}
// 			}

		}
	}
	
	prj->reset_perspective_projection();
}

// Draw all the stars
void HipStarMgr::drawPoint(Vec3f equ_vision, ToneReproductor* _eye, Projector* prj)
{
	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	glBlendFunc(GL_ONE, GL_ONE);

	// Find the star zones which are in the screen
	int nbZones=0;
	nbZones = HipGrid.Intersect(equ_vision, prj->get_fov()*M_PI/180.f*1.2f);
	int * zoneList = HipGrid.getResult();
	float maxMag = 5.5f+60.f/prj->get_fov();

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<HipStar *>::iterator end;
	static vector<HipStar *>::iterator iter;
	HipStar* h;
	Vec3d XY;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
		for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag>maxMag) break;
			if(!prj->project_j2000_check(h->XYZ, XY)) continue;
			h->draw_point(XY);
			if (!h->commonNameI18.empty() && names_fader.getInterstate() && h->Mag<maxMagStarName)
			{
				h->draw_name(XY);
				//				glBindTexture (GL_TEXTURE_2D, starTexture->getID());
			}
		}
	}

    prj->reset_perspective_projection();
}

// Look for a star by XYZ coords
StelObject HipStarMgr::search(Vec3f Pos) const
{
    Pos.normalize();
    HipStar * nearest=NULL;
    float angleNearest=0.;

    for(int i=0; i<starArraySize; i++)
    {
		if (!StarFlatArray[i]) continue;
		if 	(StarFlatArray[i]->XYZ[0]*Pos[0] + StarFlatArray[i]->XYZ[1]*Pos[1] +
			StarFlatArray[i]->XYZ[2]*Pos[2]>angleNearest)
    	{
			angleNearest = StarFlatArray[i]->XYZ[0]*Pos[0] +
				StarFlatArray[i]->XYZ[1]*Pos[1] + StarFlatArray[i]->XYZ[2]*Pos[2];
        	nearest=StarFlatArray[i];
    	}
    }
    if (angleNearest>RADIUS_STAR*0.9999)
    {   
	    return nearest;
    }
    else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<StelObject> HipStarMgr::search_around(Vec3d v, double lim_fov) const
{
	vector<StelObject> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);

	for(int i=0; i<starArraySize; i++)
    {
		if (!StarFlatArray[i]) continue;
		if (StarFlatArray[i]->XYZ[0]*v[0] + StarFlatArray[i]->XYZ[1]*v[1] + StarFlatArray[i]->XYZ[2]*v[2]>=cos_lim_fov)
    	{
			result.push_back(StarFlatArray[i]);
    	}
    }
	return result;
}


// Load the double stars from the double star file
bool HipStarMgr::load_double(const string& hipCatFile)
{
	char record[1024];
	string dataDir = hipCatFile;
    HipStar *e = NULL;
	int HP;
	FILE *dataFile;
	int i=0;
	
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << "Loading Hipparcos double stars...";
	string hipData = dataDir + "double_txt.dat";

    dataFile = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        cerr << "Hipparcos double star data file " << hipData << " not found" << endl;
		return false;
    }

	while (fgets(record,1024,dataFile))
	{
		sscanf(record,"%u", &HP);
		e = StarFlatArray[HP];
		if (e)
		{
			e->doubleStar = true;
		}
		
		i++;
	}
    fclose(dataFile);

	cout << "(" << i << " stars loaded)" << endl;
	return true;
}

// Load the variable stars from the raw hip file
bool HipStarMgr::load_variable(const string& hipCatFile)
{
	char record[1024];
	string dataDir = hipCatFile;
    HipStar *e = NULL;
	int HP;
	FILE *dataFile;
	int i=0;
	
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << "Loading Hipparcos periodic variable stars...";
	string hipData = dataDir+"variable_txt.dat";
    dataFile = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        cerr << "Hipparcos variable star data file " << hipData << " not found" << endl;
		return false;
    }

	while (fgets(record,1024,dataFile))
	{
		sscanf(record,"%u", &HP);
		e = StarFlatArray[HP];
		if (e)
		{
			e->variableStar = true;
		}
		
		i++;
	}
    fclose(dataFile);

	cout << "(" << i << " stars loaded)" << endl;
	return true;
}

//! @brief Update i18 names from english names according to passed translator
//! The translation is done using gettext with translated strings defined in translations.h
void HipStarMgr::translateNames(Translator& trans)
{

	// TODO: separate common names vector would be more efficient
    for (int i=0; i<starArraySize; ++i)
    {
		StarArray[i].commonNameI18 = trans.translate(StarArray[i].englishCommonName);
    }

}


StelObject HipStarMgr::search(const string& name) const
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
HipStar *HipStarMgr::searchHPprivate(unsigned int _HP) const
{
	if (_HP != 0 && _HP < (unsigned int)starArraySize && StarFlatArray[_HP] 
		&& StarFlatArray[_HP]->HP == _HP)
		return StarFlatArray[_HP];
    return NULL;
}

StelObject HipStarMgr::searchHP(unsigned int _HP) const {
  return searchHPprivate(_HP);
}

StelObject HipStarMgr::searchByNameI18n(const wstring& nameI18n) const
{
	wstring objw = nameI18n;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	
	// Search by HP number if it's an HP formated number
	// Please help, if you know a better way to do this:
	if (nameI18n.length() >= 2 && nameI18n[0]==L'H' && nameI18n[1]==L'P')
	{
		bool hp_ok = false;
		wstring::size_type i=2;
		// ignore spaces
		for (;i<nameI18n.length();i++)
		{
			if (nameI18n[i] != L' ') break;
		}
		// parse the number
		unsigned int nr = 0;
		for (;i<nameI18n.length();i++)
		{
			if (hp_ok = (L'0' <= nameI18n[i] && nameI18n[i] <= L'9'))
			{
				nr = 10*nr+(nameI18n[i]-L'0');
			}
			else
			{
				break;
			}
		}
		if (hp_ok)
		{
			return searchHP(nr);
		}
	}
		
	// Search by I18n common name
	std::vector<HipStar*>::const_iterator iter;
	for (iter=common_names_map.begin();iter!=common_names_map.end();++iter)
	{
		wstring objwcap = (*iter)->commonNameI18;
		transform(objwcap.begin(), objwcap.end(), objwcap.begin(), ::toupper);
		if (objwcap==objw) return *iter;
	}
	
	// Search by sci name
	for (iter=sci_names_map.begin();iter!=sci_names_map.end();++iter)
	{
		wstring objwcap = (*iter)->sciName;
		transform(objwcap.begin(), objwcap.end(), objwcap.begin(), ::toupper);
		if (objwcap==objw) return *iter;
	}
	
	return NULL;
}

//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
vector<wstring> HipStarMgr::listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem) const
{
	vector<wstring> result;
	if (maxNbItem==0) return result;
	
	wstring objw = objPrefix;
	transform(objw.begin(), objw.end(), objw.begin(), ::toupper);
	
	// Search by common names
	std::vector<HipStar*>::const_iterator iter;
	for (iter = common_names_map.begin(); iter != common_names_map.end(); ++iter)
	{
		wstring constw = (*iter)->commonNameI18.substr(0, objw.size());
		transform(constw.begin(), constw.end(), constw.begin(), ::toupper);
		if (constw==objw)
		{
			result.push_back((*iter)->commonNameI18);
		}
	}
	
	// Search by sci names
	for (iter = sci_names_map.begin(); iter != sci_names_map.end(); ++iter)
	{
		wstring constw = (*iter)->sciName.substr(0, objw.size());
		transform(constw.begin(), constw.end(), constw.begin(), ::toupper);
		if (constw==objw)
		{
			result.push_back((*iter)->sciName);
		}
	}
	
	sort(result.begin(), result.end());
	if (result.size()>maxNbItem) result.erase(result.begin()+maxNbItem, result.end());
	
	return result;
}


//! Define font file name and size to use for star names display
void HipStarMgr::setFont(float font_size, const string& font_name) {

	if (HipStar::starFont) delete HipStar::starFont;
	HipStar::starFont = new s_font(font_size, font_name);
	assert(HipStar::starFont);

}
