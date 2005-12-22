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

#include "nebula_mgr.h"
#include "stellarium.h"
#include "s_texture.h"
#include "s_font.h"
#include "navigator.h"
#include "stel_sdl.h"

#define RADIUS_NEB 1.

NebulaMgr::NebulaMgr() :
	showNGC(false),
	showMessier(false)
{
}

NebulaMgr::~NebulaMgr()
{
	vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++)
    {
		delete (*iter);
    }

    if (Nebula::tex_circle) delete Nebula::tex_circle;
	Nebula::tex_circle = NULL;

	if (Nebula::nebula_font) delete Nebula::nebula_font;
	Nebula::nebula_font = NULL;
}

// read from stream
bool NebulaMgr::read(float font_size, const string& font_name, const string& fileName, LoadingBar& lb)
{
	read_NGC_catalog(fileName, lb);
	read_Sharpless_catalog(fileName, lb);
	read_Cadwell_catalog(fileName, lb);
	read_messier_textures(fileName, lb);
/*	
	printf(_("Loading messier nebulas data "));
	
	std::ifstream inf(fileName.c_str());
	if (!inf.is_open())
	{
		printf("Can't open nebula catalog %s\n", fileName.c_str());
		assert(0);
	}
	
	// determine total number to be loaded for percent complete display
	string record;
	int total=0;
	while(!std::getline(inf, record).eof())
	{
		++total;
	}
	inf.clear();
	inf.seekg(0);
	
	printf(_("(%d deep space objects)...\n"), total);
	
	int current = 0;
	char tmpstr[512];
	while(!std::getline(inf, record).eof())
	{
		// Draw loading bar
		++current;
		snprintf(tmpstr, 512, _("Loading Nebula Data: %d/%d"), current, total);
		lb.SetMessage(tmpstr);
		lb.Draw((float)current/total);
		
		Nebula *e = new Nebula;
		if (!e->read(record)) // reading error
		{
			printf("Error while parsing nebula %s\n", e->name.c_str());
			delete e;
			e = NULL;
		} 
		else
		{
			neb_array.push_back(e);
		}
	}
*/

	if (!Nebula::nebula_font) Nebula::nebula_font = new s_font(font_size, font_name); // load Font
	if (!Nebula::nebula_font)
	{
		printf("Can't create nebulaFont\n");
		return(1);
	}
	
	if (!Nebula::tex_circle) 
		Nebula::tex_circle = new s_texture("neb");   // Load circle texture
	
	return true;
}

// Draw all the Nebulaes
void NebulaMgr::draw(int hint_ON, Projector* prj, const Navigator * nav, ToneReproductor* eye, bool draw_tex, bool _gravity_label, float max_mag_name, bool bright_nebulae)
{
	Nebula::gravity_label = _gravity_label;
	Nebula::hints_brightness = hints_fader.getInterstate();

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
	if (draw_mode == DM_NORMAL) glBlendFunc(GL_ONE, GL_ONE);
	else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // charting
	    
    Vec3f pXYZ; 

	prj->set_orthographic_projection();

	bool hints = (hints_fader.getInterstate() != 0);

    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{   

		//		if ((showMessier && (*iter)->Messier_nb != 0) ||
		//	(showNGC && ((*iter)->NGC_nb != 0 || (*iter)->IC_nb != 0)))

		// TODO correct the names, make just one variable
		if (showNGC || (showMessier && (*iter)->hasTex()))
		{

			// improve performance by skipping if too small to see
			if ((hints  && (*iter)->mag <= max_mag_name)
				|| (*iter)->get_on_screen_size(prj, nav)>5) {
				
				// correct for precession
				pXYZ = nav->prec_earth_equ_to_earth_equ((*iter)->XYZ);
				
				// project in 2D to check if the nebula is in screen
				if ( !prj->project_earth_equ_check(pXYZ,(*iter)->XY) ) continue;
				
				if (draw_mode == DM_NORMAL)
				{
					if (draw_tex && (*iter)->get_on_screen_size(prj, nav)>5) 
					{
						if ((*iter)->hasTex())
							(*iter)->draw_tex(prj, eye, bright_nebulae && (*iter)->get_on_screen_size(prj, nav)>15 );
						else 
							if (hints) (*iter)->draw_no_tex(prj, nav, eye);
					}				
				}
				else
					(*iter)->draw_chart(prj, nav, bright_nebulae);	// charting

				if (hints) (*iter)->draw_name(hint_ON, prj);
				if (hints && !draw_mode) (*iter)->draw_circle(prj, nav);
			}
		}
	}
			
	prj->reset_perspective_projection();
}

// search by name
StelObject * NebulaMgr::search(const string& name)
{
	const string catalogs("NGC IC CW SH");
	string cat;
	unsigned int num;

	string n = name;
    for (string::size_type i=0;i<n.length();++i)
	{
		if (n[i]=='_') n[i]=' ';
	}	
	
	if (name.c_str()[0] == 'M')
	{
		n = n.substr(1);		
		istringstream ss(n);
		ss >> num;
		if (ss.fail()) return NULL;
		cat = "M";
	}
	else
	{
		istringstream ss(n);

		ss >> cat;
	
		// check if a valid catalog reference
		if (catalogs.find(cat,0) == string::npos)
			return NULL;

		ss >> num;
		if (ss.fail()) return NULL;
	}
	
	if (cat == "M") return searchMessier(num);
	if (cat == "NGC") return searchNGC(num);
	if (cat == "SH") return searchSharpless(num);
	if (cat == "CW") return searchCadwell(num);
	return searchIC(num);
}


// Look for a nebulae by XYZ coords
StelObject * NebulaMgr::search(Vec3f Pos)
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
    if (anglePlusProche>RADIUS_NEB*0.999)
    {
        return plusProche;
    }
    else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<StelObject*> NebulaMgr::search_around(Vec3d v, double lim_fov)
{
	vector<StelObject*> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	static Vec3d equPos;

    vector<Nebula*>::iterator iter = neb_array.begin();
    while (iter != neb_array.end())
    {
        equPos = (*iter)->XYZ;
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cos_lim_fov)
		{
			result.push_back(*iter);
		}
        iter++;
    }
	return result;
}

StelObject * NebulaMgr::searchNGC(unsigned int NGC)
{
    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{
		if ((*iter)->NGC_nb == NGC) return (*iter);
    }

    return NULL;
}

StelObject * NebulaMgr::searchSharpless(unsigned int Sharpless)
{
    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{
		if ((*iter)->Sharpless_nb == Sharpless) return (*iter);
    }

    return NULL;
}

StelObject * NebulaMgr::searchCadwell(unsigned int Cadwell)
{
    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{
		if ((*iter)->Cadwell_nb == Cadwell) return (*iter);
    }

    return NULL;
}

StelObject * NebulaMgr::searchIC(unsigned int IC)
{
    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{
		if ((*iter)->IC_nb == IC) return (*iter);
    }

    return NULL;
}

// search by name
StelObject * NebulaMgr::searchMessier(unsigned int M)
{
    vector<Nebula *>::iterator iter;
    for(iter=neb_array.begin();iter!=neb_array.end();iter++) 
	{
		if ((*iter)->Messier_nb == M) return (*iter);
    }

    return NULL;
}


// read from stream
bool NebulaMgr::read_NGC_catalog(const string& fileName, LoadingBar& lb)
{
	char recordstr[512], tmpstr[512];
	string dataDir = fileName;
	unsigned int catalogSize, i;
	unsigned int data_drop;

	// create the texture string (no path or extension)
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << _("Loading NGC data...");
	string ngcData = dataDir + "ngc2000.dat";
    FILE * ngcFile = fopen(ngcData.c_str(),"rb");
    if (!ngcFile)
    {
        cout << "NGC data file " << ngcData << " not found" << endl;
		return false;
    }

	// read the number of lines
    catalogSize=0;
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
			snprintf(tmpstr, 512, _("Loading NGC catalog: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}
		Nebula *e = new Nebula;
		int temp = e->read_NGC(recordstr);
		if (!temp) // reading error
		{
			cout << "Error while parsing nebula " << e->name << endl;
			delete e;
			e = NULL;
			data_drop++;
		} 
		else
		{
			neb_array.push_back(e);
		}
		i++;
	}
    fclose(ngcFile);
	cout << "(" << i << " items loaded [" << data_drop << " dropped])" << endl;

    cout << _("Loading NGC name data...");
	string ngcNameData = dataDir + "ngc2000names.dat";
    FILE * ngcNameFile = fopen(ngcNameData.c_str(),"rb");
    if (!ngcNameFile)
    {
        cout << "NGC name data file " << ngcNameData << " not found" << endl;
		return false;
    }
    
    // Read the names of the NGC objects
	i = 0;
	char n[40];
	int nb, k;
	string name;
	Nebula *e;
	
	while (fgets(recordstr,512,ngcNameFile))
	{
		ostringstream oss;
		sscanf(&recordstr[38],"%d",&nb);
		if (recordstr[37] == 'I')
		{
			oss << "IC " << nb;
			e = (Nebula*)searchIC(nb);
		}
		else
		{
			oss << "NGC " << nb;
			e = (Nebula*)searchNGC(nb);
		}
		name = oss.str();
		
		if (e)
		{
			strncpy(n, recordstr, 36);
			// trim the white spaces at the back
			n[36] = 0;
			k = 36;
			while (n[--k] == ' ' && k > 0);
			n[k+1] = 0;
			
			// also a messier - we will call it a messier then
			if (!strncmp(n, "M ",2))
			{
				istringstream iss(string(n).substr(1));  // remove the 'M'
				ostringstream oss;

				int num;
				string old = name;
				
				iss >> num;
				oss << "M" << num;
				e->name = oss.str();

				oss << "-" << old;
				string check = oss.str();
				e->longname = oss.str();
				e->Messier_nb = num;
			}
			else
			{
				ostringstream oss;
				oss << name << "-" << string(n);
				e->longname = oss.str();
			}
		}
		else
			cout << endl << "...no position data for " << name;
		i++;
	}
    fclose(ngcNameFile);
	cout << "(" << i << " items loaded)" << endl;

	return true;
}

// read from stream
bool NebulaMgr::read_Sharpless_catalog(const string& fileName, LoadingBar& lb)
{
	char recordstr[512], tmpstr[512];
	string dataDir = fileName;
	unsigned int catalogSize, i;
	unsigned int data_drop;

	// create the texture string (no path or extension)
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << _("Loading Sharpless data...");
	string SharplessData = dataDir + "sharpless.dat";
    FILE * SharplessFile = fopen(SharplessData.c_str(),"rb");
    if (!SharplessFile)
    {
        cout << "Sharpless data file " << SharplessData << " not found" << endl;
		return false;
    }

	// read the number of lines
    catalogSize=0;
	while (fgets(recordstr,512,SharplessFile)) catalogSize++;
	rewind(SharplessFile);

	// Read the Sharpless entries
	i = 0;
	data_drop = 0;
	while (fgets(recordstr,512,SharplessFile)) // temporary for testing
	{
		if (!(i%200) || (i == catalogSize-1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading Sharpless catalog: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}
		Nebula *e = new Nebula;
		int temp = e->read_Sharpless(recordstr);
		if (!temp) // reading error
		{
			cout << "Error while parsing Sharpless nebula " << e->name << endl;
			delete e;
			e = NULL;
			data_drop++;
		} 
		else
		{
			neb_array.push_back(e);
		}
		i++;
	}
    fclose(SharplessFile);
	cout << "(" << i << " items loaded [" << data_drop << " dropped])" << endl;

	return true;
}

// read from stream
bool NebulaMgr::read_Cadwell_catalog(const string& fileName, LoadingBar& lb)
{
	char recordstr[512], tmpstr[512];
	string dataDir = fileName;
	unsigned int catalogSize, i;
	unsigned int data_drop;
	int nb, ref_nb;
	char ref_type;

	// create the texture string (no path or extension)
	unsigned int loc = dataDir.rfind("/");
	
	if (loc != string::npos)
		dataDir = dataDir.substr(0,loc+1);

    cout << _("Loading Cadwell reference data...");
	string CadwellData = dataDir + "cadwell.dat";
    FILE * CadwellFile = fopen(CadwellData.c_str(),"rb");
    if (!CadwellFile)
    {
        cout << "Cadwell data file " << CadwellData << " not found" << endl;
		return false;
    }

	// read the number of lines
    catalogSize=0;
	while (fgets(recordstr,512,CadwellFile)) catalogSize++;
	rewind(CadwellFile);

	// Read the Cadwell entries
	i = 0;
	data_drop = 0;
	Nebula *e;
	while (fgets(recordstr,512,CadwellFile)) // temporary for testing
	{
		if (!(i%200) || (i == catalogSize-1))
		{
			// Draw loading bar
			snprintf(tmpstr, 512, _("Loading Cadwell catalog: %d/%d"), i == catalogSize-1 ? catalogSize : i, catalogSize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/catalogSize);
		}
		istringstream ss(recordstr);
		ss >> nb >> ref_type >> ref_nb;
		
		if (ref_type == 'N')
			e = (Nebula *)searchNGC(nb);
		else if (ref_type == 'I')
			e = (Nebula *)searchIC(nb);
		else if (ref_type == 'S')
			e = (Nebula *)searchSharpless(nb);
		else if (ref_type == 'R')
			e = new Nebula;
		else
			e = NULL;
			
		if (!e)
		{
			cout << "Error reading line " << i << ":" << recordstr << endl;
		} 
		else
		{
			e->Cadwell_nb = nb;
			if (ref_type == 'R')
			{
				int rahr;
		    	float ramin;
			    int dedeg;
			    float demin;
			    char sign;
			    string name;

				ss >> rahr >> ramin;
				ss >> sign;
				ss >> dedeg >> demin;
				float RaRad = (double)rahr+ramin/60;
				float DecRad = (float)dedeg+demin/60;
				if (sign == '-') DecRad *= -1.;

				RaRad*=M_PI/12.;     // Convert from hours 	to rad
				DecRad*=M_PI/180.;    // Convert from deg to rad

	    		// Calc the Cartesian coord with RA and DE
		    	sphe_to_rect(RaRad,DecRad,e->XYZ);
			    e->XYZ*=RADIUS_NEB;
				e->mag = 4;
				e->angular_size = (50)/2/60*M_PI/180;
				e->luminance = mag_to_luminance(e->mag, e->angular_size*e->angular_size*3600);
				if (e->luminance < 0) e->luminance = .0075;

				ss >> name;
				e->name = name;
				e->longname = name;
				ss >> e->typeDesc;
				
				if (e->typeDesc == "Oc") { e->nType = Nebula::NEB_OC; e->typeDesc = "Oc"; }
				else { e->nType = Nebula::NEB_UNKNOWN; e->typeDesc = ""; }

				neb_array.push_back(e);
			}
		}
	}
    fclose(CadwellFile);
	cout << "(" << i << " items loaded [" << data_drop << " dropped])" << endl;

	return true;
}

bool NebulaMgr::read_messier_textures(const string& fileName, LoadingBar& lb)
{	
	cout << _("Loading Messier textures...");
	
	std::ifstream inf(fileName.c_str());
	if (!inf.is_open())
	{
		cout << "Can't open nebula catalog " << fileName << endl;
		return false;
	}
	
	// determine total number to be loaded for percent complete display
	string record;
	int total=0;
	while(!getline(inf, record).eof())
	{
		++total;
	}
	inf.clear();
	inf.seekg(0);
	
	cout << "(" << total << _(" deep space objects)") << endl;
	
	int current = 0;
	int NGC;
	char tmpstr[512];
	while(!getline(inf, record).eof())
	{
		// Draw loading bar
		++current;
		snprintf(tmpstr, 512, _("Loading Nebula Textures: %d/%d"), current, total);
		lb.SetMessage(tmpstr);
		lb.Draw((float)current/total);
		
		istringstream istr(record);
		istr >> NGC;
		
		Nebula *e = (Nebula*)searchNGC(NGC);
		if (e)
		{
			if (!e->read(record)) // reading error
				cout << "Error while parsing messier nebula " << e->name << endl;
		}
	}
	return true;
}
