/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�eau
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


#include <iostream>

// Class used to manage group of constellation
#include "constellation.h"
#include "constellation_mgr.h"

// constructor which loads all data from appropriate files
Constellation_mgr::Constellation_mgr(string _data_dir, string _sky_culture, string _sky_locale, Hip_Star_mgr *_hip_stars, string _font_filename, int barx, int bary, Vec3f _lines_color, Vec3f _names_color) :
  asterFont(NULL), lines_color(_lines_color), names_color(_names_color), hipStarMgr(_hip_stars), 
  dataDir( _data_dir), skyCulture(_sky_culture), selected(NULL)
{

  // load font
  asterFont = new s_font(12.,"spacefont", dataDir + _font_filename);
  if (!asterFont) {
    printf("Can't create asterFont\n");
    exit(-1);
  }

  load(dataDir + "sky_cultures/" + skyCulture + "/constellationship.fab",
       dataDir + "sky_cultures/" + skyCulture + "/constellationsart.fab", hipStarMgr, dataDir + _font_filename, barx, bary);

  // load translated labels
  set_sky_locale(_sky_locale);
  skyLocale = _sky_locale;

  art_switch = new linear_switchor();
  lines_switch = new linear_switchor();
  names_switch = new linear_switchor();
}


Constellation_mgr::~Constellation_mgr()
{
	vector<Constellation *>::iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();iter++)
    {
      delete (*iter);     
	  asterisms.erase(iter);
      iter--;  // important!
    }
	
    if (asterFont) delete asterFont;
    asterFont = NULL;
	if (Constellation::constellation_font) delete Constellation::constellation_font;
	Constellation::constellation_font = NULL;
	
	delete art_switch;
	delete lines_switch;
	delete names_switch;
}

int Constellation_mgr::set_sky_culture(string _sky_culture, const string& _font_fileName, int barx, int bary)
{

  if( _sky_culture == skyCulture ) return 1;  // no change

  // load new culture data
  printf(_("Changing sky culture to %s\n"), _sky_culture.c_str() );
  if( load(dataDir + "sky_cultures/" + _sky_culture + "/constellationship.fab",
       dataDir + "sky_cultures/" + _sky_culture + "/constellationsart.fab", 
	   hipStarMgr, _font_fileName, barx, bary) ) {

    skyCulture = _sky_culture;
    
    // load translated labels
    set_sky_locale(skyLocale);
    return 1; 
  }
  return 0;
}

// Load from file
int Constellation_mgr::load(const string& fileName, const string& artfileName, Hip_Star_mgr * _VouteCeleste, const string& _font_fileName, int barx, int bary)
{
  printf(_("Loading constellation data...\n"));

  FILE * fic = fopen(fileName.c_str(),"r");
  if (!fic) {
    printf("Can't open %s\n",fileName.c_str());
    return 0;
  }

  // delete existing data, if any
  vector<Constellation *>::iterator iter;
  for(iter=asterisms.begin();iter!=asterisms.end();++iter) {
    delete (*iter);
    asterisms.erase(iter);
    iter--;  // important!
  }

  Constellation * cons = NULL;
  while(!feof(fic)) {
    cons = new Constellation;
    if (cons && cons->read(fic, _VouteCeleste))
      {
            asterisms.push_back(cons);
      }
    else
      {
	if (cons) delete cons;
      }
  }
  fclose(fic);
  
  if (!Constellation::constellation_font) Constellation::constellation_font = new s_font(12.,"spacefont", _font_fileName); // load Font
  if (!Constellation::constellation_font)
    {
      printf("Can't create constellation font\n");
    }	
  
  fic = fopen(artfileName.c_str(),"r");
  if (!fic)
    {
      printf("Can't open %s\n",artfileName.c_str());
      return 1;  // no art, but still loaded constellation data
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
	int total=0;	
	
	// determine total number to be loaded for percent complete display
    while( fgets(tmpstr,2000,fic) ) {
      total++;
    }
    rewind(fic);	
	
	int current=0;
    glDisable(GL_BLEND);
	
    while(!feof(fic))
    {
        if (fscanf(fic,"%s %s %u %u %u %u %u %u %u %u %u\n",shortname,texfile,&x1,&y1,&hp1,&x2,&y2,&hp2,
			&x3,&y3,&hp3)!=11)
		{
			if (feof(fic))
			{
				// Empty constellation file
				fclose(fic);
				return 1;  // no art is OK
			}

			printf("ERROR while loading art for constellation %s\n", shortname);
			
		}

		// Draw loading bar
		sprintf(tmpstr, _("Loading Constellation Art: %d/%d"), current, total);

		glDisable(GL_TEXTURE_2D);
		
		// black out background of text for redraws (so can keep sky unaltered)
		glColor3f(0,0,0);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1,0);              // Bottom Right
			glVertex3f(barx+302,bary+36, 0.0f);
			glTexCoord2i(0,0);              // Bottom Left
			glVertex3f(barx-2,bary+36, 0.0f);
			glTexCoord2i(1,1);              // Top Right
			glVertex3f(barx+302, bary+22,0.0f);
			glTexCoord2i(0,1);              // Top Left
			glVertex3f(barx-2,bary+22,0.0f);
		glEnd ();
		glColor3f(1,1,1);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1,0);              // Bottom Right
			glVertex3f(barx+302,bary+22, 0.0f);
			glTexCoord2i(0,0);              // Bottom Left
			glVertex3f(barx-2,bary+22, 0.0f);
			glTexCoord2i(1,1);              // Top Right
			glVertex3f(barx+302, bary-2,0.0f);
			glTexCoord2i(0,1);              // Top Left
			glVertex3f(barx-2,bary-2,0.0f);
		glEnd ();
		glColor3f(0.0f,0.0f,1.0f);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1,0);              // Bottom Right
			glVertex3f(barx+300*current/total,bary+20, 0.0f);
			glTexCoord2i(0,0);              // Bottom Left
			glVertex3f(barx,bary+20, 0.0f);
			glTexCoord2i(1,1);              // Top Right
			glVertex3f(barx+300*current/total, bary,0.0f);
			glTexCoord2i(0,1);              // Top Left
			glVertex3f(barx,bary,0.0f);
		glEnd ();
		glColor3f(1,1,1);
		glEnable(GL_TEXTURE_2D);
		Constellation::constellation_font->print(barx-2,bary+35, tmpstr);
		SDL_GL_SwapBuffers();				// And swap the buffers
	
		
		cons = NULL;
		cons = find_from_short_name(shortname);
		if (!cons)
		{
			printf("ERROR : Can't find constellation called : %s\n",shortname);
		} else {

		  cons->art_tex = new s_texture(texfile);
		  texSize = cons->art_tex->getSize();

		  Vec3f s1 = _VouteCeleste->search(hp1)->get_prec_earth_equ_pos();
		  Vec3f s2 = _VouteCeleste->search(hp2)->get_prec_earth_equ_pos();
		  Vec3f s3 = _VouteCeleste->search(hp3)->get_prec_earth_equ_pos();

		  // To transform from texture coordinate to 2d coordinate we need to find X with XA = B
		  // A formed of 4 points in texture coordinate, B formed with 4 points in 3d coordinate
		  // We need 3 stars and the 4th point is deduced from the other to get an normal base
		  // X = B inv(A)
		  Vec3f s4 = s1 + (s2-s1)^(s3-s1);
		  Mat4f B(s1[0], s1[1], s1[2], 1, s2[0], s2[1], s2[2], 1, s3[0], s3[1], s3[2], 1, s4[0], s4[1], s4[2], 1);
		  Mat4f A(x1, texSize-y1, 0.f, 1.f, x2, texSize-y2, 0.f, 1.f,
			  x3, texSize-y3, 0.f, 1.f, x1, texSize-y1, texSize, 1.f);
		  Mat4f X = B * A.inverse();
		  
		  cons->art_vertex[0] = Vec3f(X*Vec3f(0,0,0));
		  cons->art_vertex[1] = Vec3f(X*Vec3f(texSize/2,0,0));
		  cons->art_vertex[2] = Vec3f(X*Vec3f(texSize/2,texSize/2,0));
		  cons->art_vertex[3] = Vec3f(X*Vec3f(0,texSize/2,0));
		  cons->art_vertex[4] = Vec3f(X*Vec3f(texSize/2 + texSize/2,0,0));
		  cons->art_vertex[5] = Vec3f(X*Vec3f(texSize/2 + texSize/2,texSize/2,0));
		  cons->art_vertex[6] = Vec3f(X*Vec3f(texSize/2 + texSize/2, texSize/2 + texSize/2,0));
		  cons->art_vertex[7] = Vec3f(X*Vec3f(texSize/2 + 0, texSize/2 + texSize/2,0));
		  cons->art_vertex[8] = Vec3f(X*Vec3f(0, texSize/2 + texSize/2,0));
		  
		  current++;
		}
		
    }
    fclose(fic);

    return 1;
}

// Draw all the constellations in the vector
void Constellation_mgr::draw(Projector* prj) const
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glColor3fv(lines_color*lines_switch->get_interstate());
    prj->set_orthographic_projection();	// set 2D coordinate
	
	if (selected)
	{
		selected->draw_optim(prj);
	}
	else
	{
		vector<Constellation *>::const_iterator iter;
		for(iter=asterisms.begin();iter!=asterisms.end();++iter)
		{
		(*iter)->draw_optim(prj);
		}
	}
    prj->reset_perspective_projection();
}


void Constellation_mgr::draw_art(Projector* prj, navigator* nav)
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	// for fade in
	glColor3f(art_switch->get_interstate(),art_switch->get_interstate(),art_switch->get_interstate());
	
	prj->set_orthographic_projection();
	
	if (selected)
	{
		selected->draw_art_optim(prj, nav);
	}
	else
	{
		vector<Constellation *>::const_iterator iter;
		for(iter=asterisms.begin();iter!=asterisms.end();++iter)
		{
			(*iter)->draw_art_optim(prj, nav);
		}
	}
	prj->reset_perspective_projection();
	glDisable(GL_CULL_FACE);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector* prj, bool _gravity_label)
{
	Constellation::gravity_label = _gravity_label;
    glColor3fv(names_color*names_switch->get_interstate());
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    prj->set_orthographic_projection();	// set 2D coordinate
	if (selected)
	{
		selected->draw_name(asterFont, prj);
	}
	else
	{	
		vector<Constellation *>::iterator iter;
		for(iter=asterisms.begin();iter!=asterisms.end();iter++)
		{
			// Check if in the field of view
			if ( prj->project_prec_earth_equ_check((*iter)->XYZname, (*iter)->XYname) )
				(*iter)->draw_name(asterFont, prj);
		}
	}
    prj->reset_perspective_projection();
}

Constellation* Constellation_mgr::is_star_in(const Hip_Star * s) const
{
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if ((*iter)->is_star_in(s)) return (*iter);
    }
	return NULL;
}

Constellation* Constellation_mgr::find_from_short_name(const string& shortname) const
{
  // search in uppercase only
  string tname = shortname;
  transform(tname.begin(), tname.end(), tname.begin(), ::toupper);
     
	vector<Constellation *>::const_iterator iter;
    for(iter=asterisms.begin();iter!=asterisms.end();++iter)
    {
		// Check if the star is in one of the constellation
    	if (string((*iter)->short_name)==tname) return (*iter);
    }
	return NULL;
}


// update constellation names for a new locale
int Constellation_mgr::set_sky_locale(const string& _sky_locale) {

  skyLocale = _sky_locale;


  vector<Constellation *>::const_iterator iter;

  char short_name[4];
  char cname[20];
  Constellation* aster;

  // clear previous names
  for(iter=asterisms.begin();iter!=asterisms.end();++iter) {
    (*iter)->set_common_name("");
  }

  // read in translated common names from file
  FILE *cnFile;
  cnFile = NULL;

  string filename = dataDir + "sky_cultures/" + skyCulture + "/constellation_names." + _sky_locale + ".fab";
  cnFile=fopen(filename.c_str(),"r");
  if (!cnFile) {
    printf("WARNING %s NOT FOUND\n",filename.c_str());
    return 0;
  }

  // find matching constellation and update name
  while(!feof(cnFile)) {
    fscanf(cnFile,"%s\t%s\n",short_name,cname);

    aster = find_from_short_name(string(short_name));

    if( aster != NULL ) {
      //      printf( "Found constellation %s (%s)\n", short_name, cname);
      aster->set_common_name(cname);
    }

  }
  fclose(cnFile);
  return 1;
}
