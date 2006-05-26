/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
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

#include "telescope_mgr.h"
#include "telescope.h"
#include "init_parser.h"

#include <algorithm>

#ifdef WIN32
  #include <winsock2.h> // select
#else
  #include <sys/select.h> // select
#endif

void TelescopeMgr::TelescopeMap::clear(void) {
  for (const_iterator it(begin());it!=end();it++) delete it->second;
  std::map<int,Telescope*>::clear();
}

TelescopeMgr::TelescopeMgr(void) : telescope_font(0),telescope_texture(0) {
}

TelescopeMgr::~TelescopeMgr(void) {
  if (telescope_font) delete telescope_font;
  if (telescope_texture) delete telescope_texture;
}

void TelescopeMgr::draw(const Projector *prj,const Navigator *nav) const {
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  prj->set_orthographic_projection();	// set 2D coordinate
  glBindTexture (GL_TEXTURE_2D,telescope_texture->getID());
  glBlendFunc(GL_ONE,GL_ONE);
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    if (it->second->isConnected()) {
      Vec3d XY;
      if (prj->project_j2000_check(it->second->getObsJ2000Pos(),XY)) {
        if (telescope_fader.getInterstate() >= 0) {
          glColor4f(circle_color[0],circle_color[1],circle_color[2],
                    telescope_fader.getInterstate());
          const double radius = 15.0;
          glBegin(GL_QUADS);
          glTexCoord2i(0,0);
          glVertex2d(XY[0]-radius,XY[1]-radius); // Bottom left
          glTexCoord2i(1,0);
          glVertex2d(XY[0]+radius,XY[1]-radius); // Bottom right
          glTexCoord2i(1,1);
          glVertex2d(XY[0]+radius,XY[1]+radius); // Top right
          glTexCoord2i(0,1);
          glVertex2d(XY[0]-radius,XY[1]+radius); // Top left
          glEnd();
        }
        if (name_fader.getInterstate() >= 0) {
          glColor4f(label_color[0],label_color[1],label_color[2],
                    name_fader.getInterstate());
          if (prj->getFlagGravityLabels()) {
            prj->print_gravity180(telescope_font, XY[0],XY[1],
                                  it->second->getNameI18n(), 1, 6, -4);
          } else {
            telescope_font->print(XY[0]+6,XY[1]-4,it->second->getNameI18n());
          }
          glBindTexture(GL_TEXTURE_2D,telescope_texture->getID());
        }
      }
    }
  }
  prj->reset_perspective_projection();
}

void TelescopeMgr::update(int delta_time) {
  name_fader.update(delta_time);
  telescope_fader.update(delta_time);
}

vector<StelObject*> TelescopeMgr::search_around(Vec3d pos,
                                                double lim_fov) const {
  vector<StelObject*> result;
  pos.normalize();
  double cos_lim_fov = cos(lim_fov * M_PI/180.);
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    if (it->second->getObsJ2000Pos().dot(pos) >= cos_lim_fov) {
      result.push_back(it->second);
    }
  }
  return result;
}

StelObject *TelescopeMgr::searchByNameI18n(const wstring &nameI18n) const {
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    if (it->second->getNameI18n() == nameI18n) return it->second;
  }
  return 0;
}

vector<wstring> TelescopeMgr::listMatchingObjectsI18n(
                                const wstring& objPrefix,
                                unsigned int maxNbItem) const {
  vector<wstring> result;
  if (maxNbItem==0) return result;
  wstring objw = objPrefix;
  std::transform(objw.begin(),objw.end(),objw.begin(),::toupper);
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    wstring constw = it->second->getNameI18n().substr(0, objw.size());
    std::transform(constw.begin(),constw.end(),constw.begin(),::toupper);
    if (constw==objw) {
      result.push_back(it->second->getNameI18n());
    }
  }
  sort(result.begin(),result.end());
  if (result.size()>maxNbItem) {
    result.erase(result.begin()+maxNbItem, result.end());
  }
  return result;
}

void TelescopeMgr::setFont(float font_size,const string &font_name) {
  if (telescope_font) delete telescope_font;
  telescope_font = new s_font(font_size, font_name);
  assert(telescope_font);
}

void TelescopeMgr::init(const InitParser &conf) {
  if (telescope_texture) {
    delete telescope_texture;
    telescope_texture = 0;
  }
  telescope_texture = new s_texture("star16x16.png",TEX_LOAD_TYPE_PNG_SOLID);
  telescope_map.clear();
  for (int i=0;i<9;i++) {
    const char name[2] = {'0'+i,'\0'};
    const string url = conf.get_str("telescopes",name,"");
    if (!url.empty()) {
      Telescope *t = Telescope::create(url);
      if (t) {
        telescope_map[i] = t;
      }
    }
  }
}

void TelescopeMgr::telescopeGoto(int telescope_nr,const Vec3d &j2000_pos) {
  TelescopeMap::const_iterator it(telescope_map.find(telescope_nr));
  if (it != telescope_map.end()) {
    it->second->telescopeGoto(j2000_pos);
  }
}


void TelescopeMgr::communicate(void) {
  if (!telescope_map.empty()) {
    fd_set read_fds,write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    int fd_max = -1;
    for (TelescopeMap::const_iterator it(telescope_map.begin());
         it!=telescope_map.end();it++) {
      it->second->prepareSelectFds(read_fds,write_fds,fd_max);
    }
    if (fd_max >= 0) {
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      const int select_rc = select(fd_max+1,&read_fds,&write_fds,0,&tv);
      if (select_rc > 0) {
        for (TelescopeMap::const_iterator it(telescope_map.begin());
             it!=telescope_map.end();it++) {
          it->second->handleSelectFds(read_fds,write_fds);
        }
      }
    }
  }
}
