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
Constellation_mgr::Constellation_mgr(string _data_dir, string _sky_culture, string _sky_locale,
									 Hip_Star_mgr * _hip_stars, string _font_filename, int barx, int bary,
									 Vec3f _lines_color, Vec3f _names_color) : 
									 asterFont(NULL),
									 lines_color(_lines_color),
									 names_color(_names_color),
									 hipStarMgr(_hip_stars),
									 dataDir(_data_dir),
									 selected(NULL)
{
	asterFont = new s_font(12., "spacefont", dataDir + _font_filename);
	assert(asterFont);

	if (!Constellation::constellation_font)
		Constellation::constellation_font = new s_font(12., "spacefont", dataDir + _font_filename);	// load Font
	assert(Constellation::constellation_font);	
	
	set_sky_locale(_sky_locale);
	
	skyCulture = "undefined";
	set_sky_culture(_sky_culture, barx, bary);
}


Constellation_mgr::~Constellation_mgr()
{
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		delete(*iter);
		asterisms.erase(iter);
		iter--;					// important!
	}

	if (asterFont) delete asterFont;
	asterFont = NULL;
	if (Constellation::constellation_font) delete Constellation::constellation_font;
	Constellation::constellation_font = NULL;
}

void Constellation_mgr::set_sky_culture(string _sky_culture, int barx, int bary)
{
	if (_sky_culture == skyCulture)	return;	// no change
	skyCulture = _sky_culture;
	
	// load new culture data
	printf(_("Loading constellation for sky culture: \"%s\"\n"), _sky_culture.c_str());
	load_line_and_art(dataDir + "sky_cultures/" + _sky_culture + "/constellationship.fab",
		dataDir + "sky_cultures/" + _sky_culture + "/constellationsart.fab", barx, bary);
	// load translated labels for that culture
	set_sky_locale(skyLocale);
}

// Load line and art data from files
void Constellation_mgr::load_line_and_art(const string & fileName, const string & artfileName, int barx, int bary)
{
	FILE *fic = fopen(fileName.c_str(), "r");
	if (!fic)
	{
		printf("Can't open %s\n", fileName.c_str());
		assert(0);
	}

	// delete existing data, if any
	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		delete(*iter);
		asterisms.erase(iter);
		iter--;					// important!
	}

	Constellation *cons = NULL;
	while (!feof(fic))
	{
		cons = new Constellation;
		assert(cons);
		cons->read(fic, hipStarMgr);
		asterisms.push_back(cons);
	}
	fclose(fic);

	fic = fopen(artfileName.c_str(), "r");
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
	glDisable(GL_BLEND);

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
		sprintf(tmpstr, _("Loading Constellation Art: %d/%d"), current, total);

		glDisable(GL_TEXTURE_2D);

		// black out background of text for redraws (so can keep sky unaltered)
		glColor3f(0, 0, 0);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1, 0);		// Bottom Right
			glVertex3f(barx + 302, bary + 36, 0.0f);
			glTexCoord2i(0, 0);		// Bottom Left
			glVertex3f(barx - 2, bary + 36, 0.0f);
			glTexCoord2i(1, 1);		// Top Right
			glVertex3f(barx + 302, bary + 22, 0.0f);
			glTexCoord2i(0, 1);		// Top Left
			glVertex3f(barx - 2, bary + 22, 0.0f);
		glEnd();
		glColor3f(1, 1, 1);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1, 0);		// Bottom Right
			glVertex3f(barx + 302, bary + 22, 0.0f);
			glTexCoord2i(0, 0);		// Bottom Left
			glVertex3f(barx - 2, bary + 22, 0.0f);
			glTexCoord2i(1, 1);		// Top Right
			glVertex3f(barx + 302, bary - 2, 0.0f);
			glTexCoord2i(0, 1);		// Top Left
			glVertex3f(barx - 2, bary - 2, 0.0f);
		glEnd();
		glColor3f(0.0f, 0.0f, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2i(1, 0);		// Bottom Right
			glVertex3f(barx + 300 * current / total, bary + 20, 0.0f);
			glTexCoord2i(0, 0);		// Bottom Left
			glVertex3f(barx, bary + 20, 0.0f);
			glTexCoord2i(1, 1);		// Top Right
			glVertex3f(barx + 300 * current / total, bary, 0.0f);
			glTexCoord2i(0, 1);		// Top Left
			glVertex3f(barx, bary, 0.0f);
		glEnd();
		glColor3f(1, 1, 1);
		glEnable(GL_TEXTURE_2D);
		Constellation::constellation_font->print(barx - 2, bary + 35, tmpstr);
		SDL_GL_SwapBuffers();	// And swap the buffers


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

			Vec3f s1 = hipStarMgr->search(hp1)->get_prec_earth_equ_pos();
			Vec3f s2 = hipStarMgr->search(hp2)->get_prec_earth_equ_pos();
			Vec3f s3 = hipStarMgr->search(hp3)->get_prec_earth_equ_pos();

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

// Draw all the constellations in the vector
void Constellation_mgr::draw(Projector * prj) const
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	prj->set_orthographic_projection();	// set 2D coordinate

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_optim(prj, lines_color);
	}

	prj->reset_perspective_projection();
}


void Constellation_mgr::draw_art(Projector * prj, navigator * nav)
{
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	prj->set_orthographic_projection();

	vector < Constellation * >::const_iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
	{
		(*iter)->draw_art_optim(prj, nav);
	}

	prj->reset_perspective_projection();
	glDisable(GL_CULL_FACE);
}

// Draw the names of all the constellations
void Constellation_mgr::draw_names(Projector * prj, bool _gravity_label)
{
	Constellation::gravity_label = _gravity_label;
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	prj->set_orthographic_projection();	// set 2D coordinate

	vector < Constellation * >::iterator iter;
	for (iter = asterisms.begin(); iter != asterisms.end(); iter++)
	{
		// Check if in the field of view
		if (prj->project_prec_earth_equ_check((*iter)->XYZname, (*iter)->XYname))
			(*iter)->draw_name(asterFont, prj, names_color);
	}

	prj->reset_perspective_projection();
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


// update constellation names for a new locale
void Constellation_mgr::set_sky_locale(const string & _sky_locale)
{
	skyLocale = _sky_locale;
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

	string filename = dataDir + "sky_cultures/" + skyCulture + "/constellation_names." + _sky_locale + ".fab";
	cnFile = fopen(filename.c_str(), "r");
	if (!cnFile)
	{
		printf("WARNING %s NOT FOUND\n", filename.c_str());
		return;
	}

	// find matching constellation and update name
	while (!feof(cnFile))
	{
		fscanf(cnFile, "%s\t%s\n", short_name, cname);
		aster = find_from_short_name(string(short_name));
		if (aster != NULL)
		{
			aster->set_name(cname);
		}
	}
	fclose(cnFile);
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


void Constellation_mgr::show_art(bool b)
{
	if (selected)
	{
		selected->show_art(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->show_art(b);
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

void Constellation_mgr::show_lines(bool b)
{
	if (selected)
	{
		selected->show_line(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->show_line(b);
	}
}

void Constellation_mgr::show_names(bool b)
{
	if (selected)
	{
		selected->show_name(b);
	}
	else
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
			(*iter)->show_name(b);
	}
}

void Constellation_mgr::set_selected(Constellation * c)
{
	selected = c;

	// update states for other constellations to fade them out
	if (selected != NULL)
	{
		vector < Constellation * >::const_iterator iter;
		for (iter = asterisms.begin(); iter != asterisms.end(); ++iter)
		{
			if ((*iter) != selected)
			{
				(*iter)->show_line(false);
				(*iter)->show_name(false);
				(*iter)->show_art(false);
			}
		}
	}
	else
	{
		// TODO reset normal state (then don't need to set each loop in stel_core)
	}
}
