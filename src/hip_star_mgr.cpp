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

	delete starcTexture;
	starcTexture=NULL;
	
	delete HipStar::starFont;
	HipStar::starFont=NULL;

	delete [] starZones;
	starZones=NULL;

	delete [] StarArray;
	StarArray=NULL;

	delete [] StarFlatArray;
	StarFlatArray=NULL;
}

void HipStarMgr::init(float font_size, const string& font_name, const string& hipCatFile,
	const string& commonNameFile, const string& sciNameFile, LoadingBar& lb)
{
	load_data(hipCatFile, lb);
	load_double(hipCatFile);
	load_variable(hipCatFile);

    load_common_names(commonNameFile);
	load_sci_names(sciNameFile);
	
	starcTexture = new s_texture("starc64x64.png",TEX_LOAD_TYPE_PNG_BLEND3);  // Load star chart texture
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
			wostringstream os;
			os << _("Loading Hipparcos catalog:") << (i == starArraySize-1 ? starArraySize: i) << L"/" << starArraySize;
			lb.SetMessage(os.str());
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
	// clear existing names (would be faster if they were in separate array
	// since relatively few are named)
    for (int i=0; i<starArraySize; ++i)
    {
		StarArray[i].englishCommonName = "";
    }
	
    FILE *cnFile;
    cnFile=fopen(commonNameFile.c_str(),"r");
    if (!cnFile)
    {   
        cerr << "Warning " << commonNameFile << " not found." << endl;
        return 0;
    }

    if (!lstCommonNames.empty())
    {
       lstCommonNames.clear();
       lstCommonNamesHP.clear();
    }
	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    HipStar *star;
	fgets(line, 256, cnFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = searchHP(tmp);
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
			lstCommonNames.push_back(star->commonNameI18);
			lstCommonNamesHP.push_back(tmp);
		}
	} while(fgets(line, 256, cnFile));

    fclose(cnFile);
    return 1;
}

unsigned int HipStarMgr::getCommonNameHP(wstring _commonname)
{
    unsigned int i = 0;

	while (i < lstCommonNames.size())
    {
        if (fcompare(_commonname,lstCommonNames[i]) == 0)
           return lstCommonNamesHP[i];
        i++;
    }
    return 0;
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

	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    HipStar *star;
	fgets(line, 256, snFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = searchHP(tmp);
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
		}
	} while(fgets(line, 256, snFile));

    fclose(snFile);
}

// Draw all the stars
void HipStarMgr::draw(Vec3f equ_vision, ToneReproductor* eye, Projector* prj)
{
	// Set temporary static variable for optimization
	if (flagStarTwinkle) HipStar::twinkle_amount = twinkleAmount;
	else HipStar::twinkle_amount = 0;
	HipStar::star_scale = starScale * starsFader.getInterstate();
	HipStar::star_mag_scale = starMagScale;
	HipStar::gravity_label = gravityLabel;
	HipStar::names_brightness = names_fader.getInterstate();
	HipStar::eye = eye;
	HipStar::proj = prj;
	
	if (flagPointStar) drawPoint(equ_vision, eye, prj);
	
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


	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
	    for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag > maxMag) break;
			if(!prj->project_j2000_check(h->XYZ, h->XY)) continue;

			// if (draw_mode == DM_NORMAL)	
			//{
				h->draw();
				if (names_fader.getInterstate() && h->Mag < maxMagStarName)
				{
					if (h->draw_name())
						glBindTexture (GL_TEXTURE_2D, starTexture->getID());
				}
			//}
			//else
// 			{
// 				h->draw_chart();
// 				if (names_fader.getInterstate() && h->Mag < maxMagStarName)
// 				{
// 					// need to rebind the star texture after font printing
// 					if (h->draw_name())
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
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
		for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag>maxMag) break;
			if(!prj->project_j2000_check(h->XYZ, h->XY)) continue;
			h->draw_point();
			if (!h->commonNameI18.empty() && names_fader.getInterstate() && h->Mag<maxMagStarName)
			{
				h->draw_name();
				glBindTexture (GL_TEXTURE_2D, starTexture->getID());
			}
		}
	}

    prj->reset_perspective_projection();
}

// Look for a star by XYZ coords
HipStar * HipStarMgr::search(Vec3f Pos)
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
vector<StelObject*> HipStarMgr::search_around(Vec3d v, double lim_fov)
{
	vector<StelObject*> result;
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

HipStar *HipStarMgr::search(const string& name)
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
HipStar *HipStarMgr::searchHP(unsigned int _HP)
{
	if (_HP != 0 && _HP < (unsigned int)starArraySize && StarFlatArray[_HP] 
		&& StarFlatArray[_HP]->HP == _HP)
		return StarFlatArray[_HP];
    return NULL;
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
