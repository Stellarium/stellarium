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

// Class used to manage group of constellation

#include <iostream>
#include <fstream>

#include "constellation.h"
#include "constellation_mgr.h"
#include "stel_utility.h"

// constructor which loads all data from appropriate files
Constellation_mgr::Constellation_mgr(Hip_Star_mgr *_hip_stars) : 
	asterFont(NULL),
	hipStarMgr(_hip_stars),
	selected(NULL)
{
	assert(hipStarMgr);
	isolateSelected = false;
}

void Constellation_mgr::set_font(float font_size, const string& font_name)
{
	if (asterFont) delete asterFont;
	asterFont = new s_font(font_size, font_name);
	assert(asterFont);
}

Constellation_mgr::~Constellation_mgr()
{
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		delete(*iter);
	}

	if (asterFont) delete asterFont;
	asterFont = NULL;
}

// Load line and art data from files
void Constellation_mgr::load_lines_and_art(const string & fileName, const string & artfileName, LoadingBar& lb)
{
	std::ifstream inf(fileName.c_str());

	if (!inf.is_open())
	{
		printf("Can't open constellation data file %s\n", fileName.c_str());
		assert(0);
	}

	// delete existing data, if any
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		delete(*iter);
	}
	asterisms.clear();

	Constellation *cons = NULL;

	string record;
	int line=0;
	while(!std::getline(inf, record).eof())
	{
		line++;
		cons = new Constellation;
		assert(cons);
		if(cons->read(record, hipStarMgr))
		{
			asterisms.push_back(cons);
		} else
		{ 
			printf(_("ERROR on line %d of %s\n"), line, fileName.c_str());
			delete cons;
		}
	}
	inf.close();

	FILE *fic = fopen(artfileName.c_str(), "r");
	if (!fic)
	{
		printf("Can't open %s\n", artfileName.c_str());
		return; // no art, but still loaded constellation data
	}

	// Read the constellation art file with the following format :
	// ShortName texture_file x1 y1 hp1 x2 y2 hp2
	// Where :
	// shortname is the international short name (i.e "Lep" for Lepus)
	// texture_file is the graphic file of the art texture
	// x1 y1 are the x and y texture coordinates in pixels of the star of hipparcos number hp1
	// x2 y2 are the x and y texture coordinates in pixels of the star of hipparcos number hp2
	// The coordinate are taken with (0,0) at the top left corner of the image file
	char shortname[20];
	char texfile[255];
	unsigned int x1, y1, x2, y2, x3, y3, hp1, hp2, hp3;
	int texSize;
	char tmpstr[2000];
	int total = 0;

	// determine total number to be loaded for percent complete display
	while (fgets(tmpstr, 2000, fic)) {++total;}
	rewind(fic);

	int current = 0;

	while (!feof(fic))
	{
		if (fscanf(fic, "%s %s %u %u %u %u %u %u %u %u %u\n", shortname, texfile, &x1, &y1, &hp1, &x2, &y2, &hp2, &x3, &y3, &hp3) != 11)
		{
			if (feof(fic))
			{
				// Empty constellation file
				fclose(fic);
				return;		// no art is OK
			}
			printf("ERROR while loading art for constellation %s\n", shortname);
			assert(0);
		}

		// Draw loading bar
		sprintf(tmpstr, _("Loading Constellation Art: %d/%d"), current+1, total);
		lb.SetMessage(tmpstr);
		lb.Draw((float)(current+1)/total);
		
		cons = NULL;
		cons = find_from_short_name(shortname);
		if (!cons)
		{
			printf("ERROR : Can't find constellation called : %s\n", shortname);
		}
		else
		{
			cons->art_tex = new s_texture(texfile);
			texSize = cons->art_tex->getSize();

			Vec3f s1 = hipStarMgr->searchHP(hp1)->get_prec_earth_equ_pos();
			Vec3f s2 = hipStarMgr->searchHP(hp2)->get_prec_earth_equ_pos();
			Vec3f s3 = hipStarMgr->searchHP(hp3)->get_prec_earth_equ_pos();

			// To transform from texture coordinate to 2d coordinate we need to find X with XA = B
			// A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
			// We need 3 stars and the 4th point is deduced from the other to get an normal base
			// X = B inv(A)
			Vec3f s4 = s1 + (s2 - s1) ^ (s3 - s1);
			Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
			Mat4f A(x1, texSize - y1, 0.f, 1.f, x2, texSize - y2, 0.f, 1.f, x3, texSize - y3, 0.f, 1.f, x1, texSize - y1, texSize, 1.f);
			Mat4f X = B * A.inverse();

			cons->art_vertex[0] = Vec3f(X * Vec3f(0, 0, 0));
			cons->art_vertex[1] = Vec3f(X * Vec3f(texSize / 2, 0, 0));
			cons->art_vertex[2] = Vec3f(X * Vec3f(texSize / 2, texSize / 2, 0));
			cons->art_vertex[3] = Vec3f(X * Vec3f(0, texSize / 2, 0));
			cons->art_vertex[4] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, 0, 0));
			cons->art_vertex[5] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, texSize / 2, 0));
			cons->art_vertex[6] = Vec3f(X * Vec3f(texSize / 2 + texSize / 2, texSize / 2 + texSize / 2, 0));
			cons->art_vertex[7] = Vec3f(X * Vec3f(texSize / 2 + 0, texSize / 2 + texSize / 2, 0));
			cons->art_vertex[8] = Vec3f(X * Vec3f(0, texSize / 2 + texSize / 2, 0));

			current++;
		}
	}
	fclose(fic);
}

void Constellation_mgr::draw(Projector * prj, navigator * nav) const
{
	prj->set_orthographic_projection();
	
	draw_lines(prj);
	draw_names(prj);
	draw_art(prj, nav);
	
	prj->reset_perspective_projection();
}

// Draw constellations art textures
void Constellation_mgr::draw_art(Projector * prj, navigator * nav) const
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_art_optim(prj, nav);
	}

	glDisable(GL_CULL_FACE);
}

// Draw constellations lines
void Constellation_mgr::draw_lines(Projector * prj) const
{

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_optim(prj);
	}
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector * prj) const
{
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		// Check if in the field of view
		if (prj->project_prec_earth_equ_check((*iter)->XYZname, (*iter)->XYname))
			(*iter)->draw_name(asterFont, prj);
	}
}

Constellation *Constellation_mgr::is_star_in(const Hip_Star * s) const
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if ((*iter)->is_star_in(s))
			return (*iter);
	}
	return NULL;
}

Constellation *Constellation_mgr::find_from_short_name(const string & shortname) const
{
	// search in uppercase only
	string tname = shortname;
	transform(tname.begin(), tname.end(), tname.begin(),::toupper);
	
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		// Check if the star is in one of the constellation
		if (string((*iter)->short_name) == tname)
			return (*iter);
	}
	return NULL;
}


// Read constellation names from the given file
void Constellation_mgr::load_names(const string& names_file)
{
	vector < Constellation * >::const_iterator iter;
	char short_name[4];
	char cname[200];
	Constellation *aster;
	
	// Constellation not loaded yet
	if (asterisms.empty()) return;
	
	// clear previous names
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->set_name("");
	}

	// read in translated common names from file
	FILE *cnFile;
	cnFile = NULL;

	cnFile = fopen(names_file.c_str(), "r");
	if (!cnFile)
	{
		printf("WARNING %s NOT FOUND\n", names_file.c_str());
		return;
	}

	// find matching constellation and update name
	while (!feof(cnFile))
	{
		fscanf(cnFile, "%s\t%s\n", short_name, cname);
		aster = find_from_short_name(string(short_name));
		if (aster != NULL)
		{

			int i=0;
			while(cname[i] != 0 && i<199) {
				if(cname[i]=='_') cname[i]=' ';
				i++;
			}
			aster->set_name(cname);
		}
	}
	fclose(cnFile);
}

vector <string> Constellation_mgr::getNames(void) 
{
     vector<string> names;
  	 vector < Constellation * >::const_iterator iter;
     string name;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		name = (*iter)->getName();
		transform(name.begin(), name.end(), name.begin(), ::tolower);
		transform(name.begin(), name.begin()+1, name.begin(), ::toupper);
        names.push_back(name);
     }
     return names;
}

vector<string> Constellation_mgr::getShortNames(void)
{
    vector<string> names;
	vector < Constellation * >::const_iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
        names.push_back((*iter)->getShortName());
    return names;
}

string Constellation_mgr::get_short_name_by_name(string _name) {

	vector < Constellation * >::const_iterator iter;

	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		if( str_compare_case_insensitive(_name, (*iter)->getName()) == 0) return (*iter)->getShortName();
	}
	return "";

}


// update faders
void Constellation_mgr::update(int delta_time)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->update(delta_time);
    }
}


void Constellation_mgr::set_art_intensity(float _max)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_max_value(_max);
}

void Constellation_mgr::set_art_fade_duration(float duration)
{
	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		(*iter)->art_fader.set_duration((int) (duration * 1000.f));
}

void Constellation_mgr::set_flag_lines(bool b)
{
	if (selected && isolateSelected)
	{
		selected->set_flag_lines(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->set_flag_lines(b);
	}
}

void Constellation_mgr::set_flag_art(bool b)
{
	if (selected && isolateSelected)
	{
		selected->set_flag_art(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->set_flag_art(b);
	}
}


void Constellation_mgr::set_flag_names(bool b)
{
	if (selected && isolateSelected)
	{
		selected->set_flag_name(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->set_flag_name(b);
	}
}

void Constellation_mgr::set_selected_const(Constellation * c)
{
	// update states for other constellations to fade them out
	if (c != NULL)
	{
		Constellation* cc;
		// Propagate old parameters new newly selected constellation
		if (selected) cc=selected;
		else cc=*(asterisms.begin());
		c->set_flag_lines(cc->get_flag_lines());
		c->set_flag_name(cc->get_flag_name());
		c->set_flag_art(cc->get_flag_art());
		selected = c;
				
		if (isolateSelected)
		{
		    vector < Constellation * >::const_iterator iter;
		    for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		    {
                if ((*iter) != selected)
	    	    {
	    	        (*iter)->set_flag_lines(false);
		            (*iter)->set_flag_name(false);
		            (*iter)->set_flag_art(false);
		        }
             }
        }
		else
		{
			vector < Constellation * >::const_iterator iter;
			for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			{
					(*iter)->set_flag_lines(c->get_flag_lines());
					(*iter)->set_flag_name(c->get_flag_name());
					(*iter)->set_flag_art(c->get_flag_art());
			}
		}
	}
	else
	{
		if (selected==NULL) return;
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		{
			if ((*iter) != selected)
			{
				(*iter)->set_flag_lines(selected->get_flag_lines());
				(*iter)->set_flag_name(selected->get_flag_name());
				(*iter)->set_flag_art(selected->get_flag_art());
			}
		}
		selected = NULL;
	}
}
