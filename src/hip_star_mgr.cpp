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
	HPStarFlatArray(NULL),
	HPStarArraySize(0),
	HDStarFlatArray(NULL),
	HDStarArraySize(0),
	SAOStarFlatArray(NULL),
	SAOStarArraySize(0),
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

	delete [] HPStarFlatArray;
	HPStarFlatArray=NULL;

	delete [] HDStarFlatArray;
	HDStarFlatArray=NULL;

	delete [] SAOStarFlatArray;
	HDStarFlatArray=NULL;

#ifdef BUILDING_DATA_FILES_SAO
	delete [] SAOStarArray;
	SAOStarArray=NULL;
#endif
}

void HipStarMgr::init(float font_size, const string& font_name, const string& hipCatFile,
	const string& commonNameFile, const string& sciNameFile, LoadingBar& lb)
{
#ifdef BUILDING_DATA_FILES_SAO
	load_SAO_HIP_NAME_data(hipCatFile, lb, 9.); // The 9 is the limiting magnitude to save!
#endif

#ifndef DATA_FILES_USE_SAO
	load_data(hipCatFile, lb);
#else
	load_combined_bin(hipCatFile, lb);
//	load_combined(hipCatFile, lb); // ASCII version
#endif

#ifdef BUILDING_DATA_FILES_DOUBLE
	load_doubleraw(hipCatFile);
	save_double();
#else
	load_double(hipCatFile);
#endif

#ifdef BUILDING_DATA_FILES_VARIABLE
	load_variable(hipCatFile);
	save_variable();
#else
	load_variable(hipCatFile);
#endif

    load_common_names(commonNameFile);
	load_sci_names(sciNameFile);
	
	starcTexture = new s_texture("starc64x64.png",TEX_LOAD_TYPE_PNG_BLEND3);  // Load star chart texture
	starTexture = new s_texture("star16x16.png",TEX_LOAD_TYPE_PNG_SOLID);  // Load star texture

    HipStar::starFont = new s_font(font_size, font_name);
    if (!HipStar::starFont)
    {
	    printf("Can't create starFont\n");
        exit(-1);
    }	
}

#ifndef DATA_FILES_USE_SAO

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
        exit(-1);
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

	HPStarFlatArray = new HipStar*[starArraySize];
	for (int i=0;i<HPStarArraySize;++i)
	{
		HPStarFlatArray[i] = NULL;
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
		HPStarFlatArray[e->HP]=e;
    }
    fclose(hipFile);

	printf("(%d stars loaded [%d dropped]).\n", starArraySize-data_drop,data_drop);

    // sort stars by magnitude for faster rendering
    for(int i=0;i < HipGrid.getNbPoints();i++) {
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(HipStarMagComparer()));
    }
}

#endif

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
		if (star)
		{
			char c=line[0];
			int i=0;
			while(c!='|' && i<256){c=line[i];++i;}
			string sciName = &(line[i]);
			sciName.erase(sciName.length()-1, 1);

			// remove underscores
			for (string::size_type j=0;j<star->sciName.length();++j) 
			{
				if (star->sciName[j]=='_') star->sciName[j]=' ';
			}
			sciName = stripConstellation(sciName);
			star->sciName = translateGreek(sciName);
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
	float max_fov = MY_MAX( prj->get_fov(), prj->get_fov()*prj->viewW()/prj->viewH());

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
	//	static Vec3d equPos;

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
	if (cat == "HD") return searchHD(num);
	return searchSAO(num);
}	

// Search the star by HP number
HipStar *HipStarMgr::searchHP(unsigned int _HP)
{
	if (_HP != 0 && _HP < (unsigned int)starArraySize && HPStarFlatArray[_HP] 
		&& HPStarFlatArray[_HP]->HP == _HP)
		return HPStarFlatArray[_HP];
    return NULL;
}

// Search the star by HD number
HipStar *HipStarMgr::searchHD(unsigned int _HD)
{
	if (_HD != 0 && _HD < (unsigned int)HDStarArraySize && HDStarFlatArray[_HD] 
		&& HDStarFlatArray[_HD]->HD == _HD)
		return HDStarFlatArray[_HD];
    return NULL;
}

// Search the star by SAO number
HipStar *HipStarMgr::searchSAO(unsigned int _SAO)
{
	if (_SAO != 0 && _SAO < (unsigned int)SAOStarArraySize && SAOStarFlatArray[_SAO] 
			&& SAOStarFlatArray[_SAO]->SAO == _SAO)
		return SAOStarFlatArray[_SAO];
    return NULL;
}

/*
void HipStarMgr::Save(void)
{
	FILE * fic = fopen("cat.fab","wb");
    fwrite((char*)&starArraySize,4,1,fic);

    for(int i=0;i<starArraySize;i++)
    {
    	HipStar * s = StarArray[i];
    	float RAf=0, DEf=0;
    	unsigned short int mag=0;
    	unsigned short int type=0;

    	if (s)
    	{
    		RAf=s->r; DEf=s->d;
    		mag = s->magp;
    		type = s->typep;
    	}
    	fwrite((char*)&RAf,4,1,fic);
    	fwrite((char*)&DEf,4,1,fic);
    	fwrite((char*)&mag,2,1,fic);
    	fwrite((char*)&type,2,1,fic);
    }
   	fclose(fic);
}*/

#ifdef BUILDING_DATA_FILES_DOUBLE

// Load the double stars from the raw hip file

bool HipStarMgr::load_doubleraw(const string& hipCatFile)
{
	char record[1024];
	string dataDir = hipCatFile;
    HipStar *e = NULL;
	int HP;
	FILE *dataFile;
	int i;
	
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << "Loading Hipparcos double stars...";
	string hipData = dataDir + "hipparcos_d.dat";

    dataFile = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        cerr << "Hipparcos double star data file " << hipData << " not found" << endl;
//		MessageBox(NULL,string("ERROR " + hipData + " NOT FOUND").c_str(),"Stellarium", MB_OK | MB_ICONINFORMATION);
		return false;
    }

	while (fgets(record,1024,dataFile))
	{
		sscanf(&record[57],"%u", &HP);
		e = HPStarFlatArray[HP];
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

// Save the double stars to the double star file

bool HipStarMgr::save_double(void)
{
	int i;
	
    cout << "Saving double stars ascii data" << endl;

	string saoData = "d:/temp/stellarium/double_txt.dat";
    FILE *dataFile = fopen(saoData.c_str(),"w");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

	// scan the HP data for HP only entries
   	for (i=0;i<=starArraySize;++i)
	{
	    HipStar *e = StarFlatArray[i];
		if (e && e->doubleStar) 
        	fprintf(dataFile,"%d\n", e->HP);
	}
	
    fclose(dataFile);
    cout << "(Save completed)" <<endl;
    return true;
}

#else

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
		e = HPStarFlatArray[HP];
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
#endif

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
#ifdef BUILDING_DATA_FILES_VARIABLE
	string hipData = dataDir+"hipparcos_v.dat";
#else
	string hipData = dataDir+"variable_txt.dat";
#endif
    dataFile = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        cerr << "Hipparcos variable star data file " << hipData << " not found" << endl;
//		MessageBox(NULL,string("ERROR " + hipData + " NOT FOUND").c_str(),"Stellarium", MB_OK | MB_ICONINFORMATION);
		return false;
    }

	while (fgets(record,1024,dataFile))
	{
		sscanf(record,"%u", &HP);
		e = HPStarFlatArray[HP];
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

#ifdef BUILDING_DATA_FILES_VARIABLE

// Save the variable stars to the variable star file

bool HipStarMgr::save_variable(void)
{
	int i;
	
    cout << "Saving variable stars ascii data" << endl;

	string saoData = "d:/temp/stellarium/variable_txt.dat";
    FILE *dataFile = fopen(saoData.c_str(),"w");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

	// scan the HP data for HP only entries
   	for (i=0;i<=starArraySize;++i)
	{
	    HipStar *e = StarFlatArray[i];
		if (e && e->variableStar) 
        	fprintf(dataFile,"%d\n", e->HP);
	}
	
    fclose(dataFile);
    cout << "(Save completed)" <<endl;
    return true;
}
#endif

#ifdef BUILDING_DATA_FILES_SAO

bool HipStarMgr::load_combined(const string& hipCatFile, LoadingBar& lb)
{
	char tmpstr[1024], record[1024];
	int catalogSize;
	string dataDir = hipCatFile;
    HipStar *e = NULL;
	int HD, HP, SAO;
	FILE *dataFile;
	int i;
	
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

	// load the hip data

    cout << "Loading Hipparcos/SAO star data...";
	string hipData = dataDir + "catalog_txt.dat";
    dataFile = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        cerr << "Hipparcos/SAO star data file " << hipData << " not found" << endl;
        exit(-1);
    }
    
	// Read number of stars in the HIP catalog
	fscanf(dataFile,"SIZE %d\n",&catalogSize);
	fscanf(dataFile,"HP %d\n",&HP);
	fscanf(dataFile,"HD %d\n",&HD);
	fscanf(dataFile,"SAO %d\n",&SAO);
	fgets(record,1024,dataFile);
	
    starArraySize = catalogSize;  	// 285090
    HPStarArraySize = HP;			// 120416
    HDStarArraySize = HD;			// 359021
    SAOStarArraySize = SAO;			// 258997

	// create the data library
    StarArray = new HipStar[starArraySize+1];

    // Create the sequential array
	StarFlatArray = new HipStar*[starArraySize+1];
	for (i=0;i<=starArraySize;++i)
	{
		StarFlatArray[i] = NULL;
	}

	HPStarFlatArray = new HipStar*[HPStarArraySize+1];
	for (i=0;i<=HPStarArraySize;++i)
	{
		HPStarFlatArray[i] = NULL;
	}

    // Create the sequential array
	HDStarFlatArray = new HipStar*[HDStarArraySize+1];
	for (i=0;i<=HDStarArraySize;++i)
	{
		HDStarFlatArray[i] = NULL;
	}

    // Create the sequential array
	SAOStarFlatArray = new HipStar*[SAOStarArraySize+1];
	for (i=0;i<=SAOStarArraySize;++i)
	{
		SAOStarFlatArray[i] = NULL;
	}

	i = 0;
	float tempDE, tempRA;
	while (fgets(record,1024,dataFile))
	{
		if (!(i%2000) || (i == catalogSize-1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading HIP/SAO catalogs: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}

		e = &StarArray[i];
		
		sscanf(record,"%u %u %u %f %f %f %f %c\n",
					&e->HP,&e->HD,&e->SAO,&tempRA, &tempDE, &e->Mag,
					&e->Distance, &e->SpType);
		
		if (e->SpType == '-')
			e->SpType = ' ';
				
		tempRA*=M_PI/180.;    // Convert from deg to rad
		tempDE*=M_PI/180.;    // Convert from deg to rad

		sphe_to_rect(tempRA,tempDE,e->XYZ);
		e->XYZ *= RADIUS_STAR;

		e->setColor(e->SpType);

        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);

		StarFlatArray[i]=e;
		if (e->HP) HPStarFlatArray[e->HP]=e;
		if (e->HD) HDStarFlatArray[e->HD]=e;
		if (e->SAO) SAOStarFlatArray[e->SAO]=e;
		i++;
	}
    fclose(dataFile);
	cout << "(" << i << " stars loaded)" << endl;

    // sort stars by magnitude for faster rendering
    for(i=0;i < HipGrid.getNbPoints();i++) 
	{
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(Hip_Star_Mag_Comparer()));
    }
    
    return true;
}
#endif

#ifdef BUILDING_DATA_FILES_SAO

bool HipStarMgr::save_record(FILE *dataFile, HipStar *e)
{
	float tempDE, tempRA;
	char sp;

	if (e)
	{
		rect_to_sphe(&tempRA,&tempDE,e->XYZ);
		tempRA*=(180./M_PI);
		tempDE*=(180./M_PI);
		sp = e->SpType;
		if (sp == ' ')
			sp = '-';
		
		fprintf(dataFile,"%06u %06u %06u %14.9f %14.9f %5.2f %8.2f %c\n",
			e->HP,e->HD,e->SAO,tempRA, tempDE, e->Mag, e->Distance, sp);
	}
	return true;
}

bool HipStarMgr::save_name_record(FILE *dataFile, unsigned int HP, string name)
{
	string savename;
	
	if (name != "")
	{
		savename = "";
		savename += name;
		for (string::size_type i=0;i<savename.length();++i)
		{
			if (savename[i]==' ') savename[i]='_';
		}
		if (savename == "") savename += ".";
		fprintf(dataFile,"%6u|%s\n",HP, savename.c_str());
	}
	return true;
}

bool HipStarMgr::save_combined(void)
{
	int HPonly = 0;
	int SAOonly = 0;
	float maxmag;
	int i;
	
	// scan the HP data for HP only entries
   	for (i=0;i<=starArraySize;++i)
	{
		if (StarFlatArray[i] && !StarFlatArray[i]->SAO) 
			HPonly++;
	}
	
	// scan the SAO data for SAO only entries
   	for (i=0;i<=SAOStarArraySize;++i)
	{
		if (SAOStarFlatArray[i] && !SAOStarFlatArray[i]->HP)
			SAOonly++;
	}

    cout << "Saving combined star ascii data" << endl;
	string saoData = "d:/temp/stellarium/catalog_txt.dat";
    FILE *dataFile = fopen(saoData.c_str(),"w");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

	// Save the data header
	fprintf(dataFile,"SIZE %d\n", CombinedTotal);
	fprintf(dataFile,"HP %d\n", HPStarArraySize);
	fprintf(dataFile,"HD %d\n", HDStarArraySize);
	fprintf(dataFile,"SAO %d\n", SAOStarArraySize);
	fprintf(dataFile,"--HP-- --HD-- --SAO- -------RA----- ------DEC----- -MAG- --DIST-- SPTYPE\n");

	// save the common and hp onyl entries
	maxmag = 0;
   	for (i=0;i<=starArraySize;++i)
	{
		save_record(dataFile, StarFlatArray[i]);
		if (StarFlatArray[i] && StarFlatArray[i]->Mag > maxmag)
				maxmag = StarFlatArray[i]->Mag;
	}
	cout << "...Max Magnitude HIP catalog = " << maxmag << endl;

	// Save the SAO only entries
	maxmag = 0;
   	for (i=0;i<=SAOStarArraySize;++i)
	{
		if (SAOStarFlatArray[i])
		{
			if (!SAOStarFlatArray[i]->HP)
				save_record(dataFile, SAOStarFlatArray[i]);
			if (SAOStarFlatArray[i]->Mag > maxmag) 
				maxmag = SAOStarFlatArray[i]->Mag;
		}
	}
	cout << "...Max Magnitude SAO catalog = " << maxmag << endl;
    fclose(dataFile);
    
	// write the names into a file for translations

    cout << "...Saving scientific names" << endl;
	saoData = "d:/temp/stellarium/catalog_hpscinames.dat";
    dataFile = fopen(saoData.c_str(),"w");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

   	for (i=0;i<=starArraySize;++i)
	{
		if (StarFlatArray[i])
			save_name_record(dataFile, StarFlatArray[i]->HP, StarFlatArray[i]->OrigSciName);
	}
	// No SAO names
    fclose(dataFile);

    cout << "...Saving common names" << endl;
	saoData = "d:/temp/stellarium/catalog_hpcommonnames.dat";
    dataFile = fopen(saoData.c_str(),"w");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

   	for (i=0;i<=starArraySize;++i)
	{
		if (StarFlatArray[i])
			save_name_record(dataFile, StarFlatArray[i]->HP, StarFlatArray[i]->CommonName);
	}
	// No SAO names
    fclose(dataFile);
    cout << "(Save completed)" <<endl;
    
    return true;
}

bool HipStarMgr::save_record_bin(FILE *dataFile, HipStar *e)
{
	float tempDE, tempRA;
	char sp;

	if (e)
	{
		rect_to_sphe(&tempRA,&tempDE,e->XYZ);
		tempRA*=(180./M_PI);
		tempDE*=(180./M_PI);
		sp = e->SpType;
		if (sp == ' ')
			sp = '-';

		fwrite(&e->HP,4,1,dataFile);
		fwrite(&e->HD,4,1,dataFile);
		fwrite(&e->SAO,4,1,dataFile);
		fwrite(&tempRA,4,1,dataFile);
		fwrite(&tempDE,4,1,dataFile);
		fwrite(&e->Mag,4,1,dataFile);
		fwrite(&e->Distance,4,1,dataFile);
		fwrite(&sp,1,1,dataFile);
	}
	return true;
}

bool HipStarMgr::save_combined_bin(void)
{
	int HPonly = 0;
	int SAOonly = 0;
	int i;

	// scan the HP data for HP only entries
   	for (i=0;i<=starArraySize;++i)
	{
		if (StarFlatArray[i] && !StarFlatArray[i]->SAO) 
			HPonly++;
	}
	
	// scan the SAO data for SAO only entries
   	for (i=0;i<=SAOStarArraySize;++i)
	{
		if (SAOStarFlatArray[i] && !SAOStarFlatArray[i]->HP)
			SAOonly++;
	}

    cout << "Saving SAO star binary data" << endl;
	string saoData = "d:/temp/stellarium/catalog_bin.dat";
    FILE * dataFile = fopen(saoData.c_str(),"wb");
    if (!dataFile)
    {
        cerr << "Data file " << saoData << " would not open for saving" << endl;
        exit(-1);
    }

	// Save the data header
	fwrite(&CombinedTotal,4,1,dataFile);
	fwrite(&HPStarArraySize,4,1,dataFile);
	fwrite(&HDStarArraySize,4,1,dataFile);
	fwrite(&SAOStarArraySize,4,1,dataFile);

	// save the common and hp onyl entries
   	for (int i=0;i<=starArraySize;++i)
	{
		save_record_bin(dataFile, StarFlatArray[i]);
	}

	// Save the SAO only entries
   	for (int i=0;i<=SAOStarArraySize;++i)
	{
		save_record_bin(dataFile, SAOStarFlatArray[i]);
	}
    fclose(dataFile);

    return true;
}

// Load from file 
bool HipStarMgr::load_SAO_HIP_NAME_data(const string& hipCatFile, LoadingBar& lb, float maxmag)
{
	char tmpstr[1024], record[1024];
	int catalogSize;
	string dataDir = hipCatFile;
	FILE *dataFile;
	unsigned int data_drop;
    HipStar *e = NULL;
	int num, max = 0;
	int snum, smax = 0;
	int hnum, hmax = 0;
	int HD, HP, SAO;
    int exclHD = 0;
    int i;
    
	CombinedTotal = 0;

	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

	// load the hip data

    printf("Loading Hipparcos star data...");
	string hipData = dataDir+"1239_hip_main.dat";
    dataFile  = fopen(hipData.c_str(),"r");
    if (!dataFile)
    {
        printf("ERROR %s NOT FOUND\n",hipData.c_str());
        exit(-1);
    }

	// Read number of stars in the HIP catalog
    catalogSize=0;
    float Mag;
	while (fgets(record,1024,dataFile))
	{
		sscanf(&record[351],"%f",&Mag);
		if (Mag < -2) Mag = 14;				// trap -9.99's, sirius = -1.03!
		if (Mag <= maxmag)
			catalogSize++;

		sscanf(&record[2],"%d",&num); 	// HP
		if (num > 0 && num > max) max = num;

		sscanf(&record[465],"%d",&hnum);	// HD 358431 max
		if (hnum > hmax) hmax = hnum;
	}
	
	int size;
	sscanf(&record[2],"%d", &size); // 118322 records
	rewind(dataFile);
	
    starArraySize = max; // 120416
    HPStarArraySize = max; // 120416

	// create the data library
    StarArray = new HipStar[starArraySize+1];

    // Create the sequential array
	StarFlatArray = new HipStar*[starArraySize+1];
	for (i=0;i<=starArraySize;++i)
	{
		StarFlatArray[i] = NULL;
	}
	
	HPStarFlatArray = new HipStar*[starArraySize+1];
	for (i=0;i<=starArraySize;++i)
	{
		HPStarFlatArray[i] = NULL;
	}

	i = 0;
	data_drop = 0;
	while (fgets(record,1024,dataFile))
	{
		if (!(i%2000) || (i == catalogSize-1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading HIP catalog: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}

		sscanf(&record[2],"%d",&HP);
		
		e = &StarArray[HP];
        if (!e->readHP(record, maxmag))
        {
			data_drop++;
			continue;
		}
		else
			CombinedTotal++;

        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);
		HPStarFlatArray[e->HP]=e;
		StarFlatArray[e->HP]=e;
		i++;
	}
    fclose(dataFile);
    
	cout << "(" << starArraySize-data_drop << " stars loaded";
	if (data_drop > 0) cout << "[" << data_drop << " dropped]";
	cout << ")" << endl;

	// scan the SAO data  for max SAO and HD
	
    cout << "Loading SAO star data ";
	string saoData = dataDir + "5015_catalog.dat";
    FILE * saoFile = fopen(saoData.c_str(),"r");
    if (!saoFile)
    {
        cerr << "SAO data file " << saoData << " not found" << endl;
        return false;
    }

	// calculate the SAO HD record count
    catalogSize=0;
	while (fgets(record,512,saoFile))
	{
		sscanf(&record[103],"%f",&Mag);	// SAO
		if (Mag < -1) Mag = 14;				// trap -9.99's
		if (Mag <= maxmag)
			catalogSize++; // 258942 records

		sscanf(&record[0],"%d",&num);
		if (num > 0 && num > max) max = num; // SAO 258997

		sscanf(&record[11],"%d",&snum);
		if (snum > smax) smax = snum;	// HD 359021 max
	}
	
	sscanf(record,"%d", &size); // 258997 (last record)
	rewind(saoFile);

    SAOStarArraySize = max; // 359021 (mkax record number)

	// create the data library
    SAOStarArray = new HipStar[SAOStarArraySize+1];

    // Create the sequential array
	SAOStarFlatArray = new HipStar*[SAOStarArraySize+1];
	for (i=0;i<=SAOStarArraySize;++i)
	{
		SAOStarFlatArray[i] = NULL;
	}

	if (smax > hmax) HDStarArraySize = smax;
	else HDStarArraySize = hmax;
    
    // Create the sequential array
	HDStarFlatArray = new HipStar*[HDStarArraySize+1];
	for (i=0;i<=HDStarArraySize;++i)
	{
		HDStarFlatArray[i] = NULL;
	}

	// fill in the HD table from the Hipparcos
	for (i=0;i<=HPStarArraySize;++i)
	{
		if (HPStarFlatArray[i] && HPStarFlatArray[i]->HD)
			HDStarFlatArray[HPStarFlatArray[i]->HD] = HPStarFlatArray[i];
	}

	// Read the SAO data
	i = 0;
	data_drop = 0;
	while (fgets(record,512,saoFile))
	{
		sscanf(record,"%d", &SAO);
		if (!(i%2000) || (i == catalogSize - 1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading SAO catalog: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}

		sscanf(&record[11],"%d",&HD);
		if (HD > 0 && HD <= HDStarArraySize && HDStarFlatArray[HD])
		{
			e = HDStarFlatArray[HD];
			e->SAO = SAO;
			SAOStarFlatArray[e->SAO]=e;
		}
		else
		{
			// No HD value
			sscanf(&record[103],"%f",&Mag);
			if (Mag < -1) Mag = 14; // some have a value of -9.99

			// only accept less than 9
//			if (Mag <= 9.f)
			{
				exclHD++;
				e = &SAOStarArray[SAO];
	    	    if (!e->readSAO(record,maxmag))
	        	{
					data_drop++;
					continue;
				}
				else
					CombinedTotal++;
				// was invalid or unreferencfed in SAO
		        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);
				e->HD = 0;
				SAOStarFlatArray[e->SAO]=e;
			}
		}
		i++;
	}
    fclose(saoFile);
	cout << "(" << SAOStarArraySize-data_drop << " stars loaded";
	if (data_drop > 0) cout << "[" << data_drop << " dropped]";
	cout << ")" << endl;

    cout << "Loading star name data " << endl;
	string nameData = dataDir + "4022_index.dat";
    FILE * nameFile = fopen(nameData.c_str(),"r");
    if (!nameFile)
    {
        cerr << "Star name data file " << nameData << " not found" << endl;
        exit(-1);
    }
	// read the number of lines
	i = 0;
	char n[50],cons[50],common[50];	
	string sci, com, org;
	int j,k;
	while (fgets(record,512,nameFile))
	{
		// get the sao and HD numbers
		sscanf(&record[93],"%d %d",&SAO, &HD);
		sci = "";
		com = "";
		org = "";

		n[0] = 0;
		cons[0]= 0;

		// get the star number and constellation
		if (record[109] != ' ') 
		{
			if (record[109]) sscanf(&record[109],"%6s",n);
			// the constellation data goes a bit out of kilter!
			if (record[114]) sscanf(&record[114],"%6s",cons);
			else if (record[115]) sscanf(&record[115],"%6s",cons);
		}

		sci += n;
		if (sci.substr(0,3) == "THE") sci = "TET";   // kludge
		if (sci.substr(0,2) == "XI") sci = "XS";   // kludge
		org += sci;
		if (sci != "") sci = translateGreek(sci);
		if (cons[0]) 
		{
			sci += " ";
			sci += cons;
			org += " ";
			org += cons;
		}
	
		// get the common name which may have a few parts
		common[0] = 0;
		if (record[120] != ' ') 
		{
			j = 120; k = 0;
			while (record[j] != '\n' && k < 20) common[k++] = tolower(record[j++]);
			common[k] = 0;
			while (common[--k] == ' ' && k > 0);
			common[k+1] = 0;
			k = 0;
			common[k] = toupper(common[k]);
			while (common[++k])
			{
				if (common[k-1] == ' ' && common[k] != ' ')
					common[k] = toupper(common[k]);
			}
			
			com += common;
		}
		else 
			com = "";
			
		// printf("%d %d %d (%s) [%s] {%s}\n",i,sao,HD, n,cons, c.c_str());

		if (HD > 0 && HD <= HDStarArraySize)
			e = HDStarFlatArray[HD];
		else if (SAO > 0 && SAO <= SAOStarArraySize)
			e = SAOStarFlatArray[SAO];
		else
			e = NULL;

		if (e)
		{
			e->CommonName += com;
			e->SciName += sci;
			e->ShortSciName = stripConstellation(sci);
			e->OrigSciName += org;
		}
		i++;
	}
    fclose(nameFile);
    
    save_combined();
    save_combined_bin();

	cout << "(" << i << " names loaded)" << endl;

    // sort stars by magnitude for faster rendering
    for(i=0;i < HipGrid.getNbPoints();i++) {
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(Hip_Star_Mag_Comparer()));
    }
	return true;
}

#endif

#ifdef DATA_FILES_USE_SAO
// Load from file 
bool HipStarMgr::load_combined_bin(const string& hipCatFile, LoadingBar& lb)
{
	char tmpstr[1024];
	int i;
	string dataDir = hipCatFile;
    HipStar *e = NULL;
	int catalogSize, HD, HP, SAO;
	int xcatalogSize, xHD, xHP, xSAO;
	float DE, RA;
//	float xDE, xRA
//	float Mag, Distance;
//	float xMag, xDistance;
	FILE *dataFile;
	
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

	// load the hip data

    cout << "Loading Hipparcos/SAO star data...";
	string hipData = dataDir +"catalog_bin.dat";
    dataFile = fopen(hipData. c_str(),"rb");
    if (!dataFile)
    {
        cerr << "Data file " << hipData << " not found" << endl;
        exit(-1);
    }

	// Read number of stars in the HIP catalog
	fread(&xcatalogSize,4,1,dataFile);
	fread(&xHP,4,1,dataFile);
	fread(&xHD,4,1,dataFile);
	fread(&xSAO,4,1,dataFile);

	LE_TO_CPU_INT32(catalogSize, xcatalogSize);
	LE_TO_CPU_INT32(HP, xHP);
	LE_TO_CPU_INT32(HD, xHD);
	LE_TO_CPU_INT32(SAO, xSAO);

    starArraySize = catalogSize;  	// 285090
    HPStarArraySize = HP;			// 120416
    HDStarArraySize = HD;			// 359021
    SAOStarArraySize = SAO;			// 258997

	// create the data library
    StarArray = new HipStar[starArraySize+1];

    // Create the sequential array
	StarFlatArray = new HipStar*[starArraySize+1];
	for (int i=0;i<=starArraySize;++i)
	{
		StarFlatArray[i] = NULL;
	}

	HPStarFlatArray = new HipStar*[HPStarArraySize+1];
	for (int i=0;i<=HPStarArraySize;++i)
	{
		HPStarFlatArray[i] = NULL;
	}

    // Create the sequential array
	HDStarFlatArray = new HipStar*[HDStarArraySize+1];
	for (int i=0;i<=HDStarArraySize;++i)
	{
		HDStarFlatArray[i] = NULL;
	}

    // Create the sequential array
	SAOStarFlatArray = new HipStar*[SAOStarArraySize+1];
	for (int i=0;i<=SAOStarArraySize;++i)
	{
		SAOStarFlatArray[i] = NULL;
	}

	i = 0;
	while (i < catalogSize)
	{
		if (!(i%2000) || (i == catalogSize-1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading HIP/SAO catalogs: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}

		e = &StarArray[i];
		
		fread(&e->HP,4,1,dataFile);
		fread(&e->HD,4,1,dataFile);
		fread(&e->SAO,4,1,dataFile);
		fread(&RA,4,1,dataFile);
		fread(&DE,4,1,dataFile);
		fread(&e->Mag,4,1,dataFile);
		fread(&e->Distance,4,1,dataFile);
		fread(&e->SpType,1,1,dataFile);
/*	
		fread(&xHP,4,1,dataFile);
		fread(&xHD,4,1,dataFile);
		fread(&xSAO,4,1,dataFile);
		fread(&xRA,4,1,dataFile);
		fread(&xDE,4,1,dataFile);
		fread(&xMag,4,1,dataFile);
		fread(&xDistance,4,1,dataFile);
		fread(&e->SpType,1,1,dataFile);
		
		LE_TO_CPU_INT32(HP, xHP);
		LE_TO_CPU_INT32(HD, xHD);
		LE_TO_CPU_INT32(SAO, xSAO);
		LE_TO_CPU_FLOAT(RA, xRA);
		LE_TO_CPU_FLOAT(DE, xDE);
		LE_TO_CPU_FLOAT(Mag, xMag);
		LE_TO_CPU_FLOAT(Distance, xDistance);

//		Mag = (5. + mag) / 256.0;
//			if (Mag>250) Mag = Mag - 256;
		e->HP = HP;
		e->HD = HD;
		e->SAO = SAO;
		e->Mag = Mag;
		e->Distance = Distance;
*/	

		
		RA*=M_PI/180.;    // Convert from deg to rad
		DE*=M_PI/180.;    // Convert from deg to rad

		sphe_to_rect(RA,DE,e->XYZ);
		e->XYZ *= RADIUS_STAR;

		if (e->SpType == '-')
			e->SpType = ' ';
		e->setColor(e->SpType);

        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);

		StarFlatArray[i]=e;
		if (e->HP) HPStarFlatArray[e->HP]=e;
		if (e->HD) HDStarFlatArray[e->HD]=e;
		if (e->SAO) SAOStarFlatArray[e->SAO]=e;
		i++;
	}
    fclose(dataFile);
	cout << "(" << i << " stars loaded)" << endl;

    // sort stars by magnitude for faster rendering
    for(int i=0;i < HipGrid.getNbPoints();i++) {
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(Hip_Star_Mag_Comparer()));
    }
	return true;
}

#endif
