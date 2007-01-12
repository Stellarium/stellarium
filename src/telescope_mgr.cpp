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
#include "StelObject.hpp"
#include "Projector.hpp"
#include "InitParser.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"

#include <algorithm>

#ifdef WIN32
  #include <winsock2.h> // select
#else
  #include <sys/select.h> // select
  #include <sys/time.h>
  #include <signal.h>
#endif

void TelescopeMgr::TelescopeMap::clear(void) {
  for (const_iterator it(begin());it!=end();it++) delete it->second;
  std::map<int,Telescope*>::clear();
}

TelescopeMgr::TelescopeMgr(void) : telescope_font(NULL),telescope_texture(0) {
#ifdef WIN32
  WSADATA wsaData;
  if (WSAStartup(0x202,&wsaData) == 0) {
    wsa_ok = true;
  } else {
    cout << "TelescopeMgr::TelescopeMgr: WSAStartup failed, "
            "you will not be able to control telescopes" << endl;
    wsa_ok = false;
  }
#else
    // SIGPIPE is normal operation when we send while the other side
    // has already closed the socket. We must ignore it:
  signal(SIGPIPE,SIG_IGN);
#endif
}

TelescopeMgr::~TelescopeMgr(void) {
  if (telescope_texture) delete telescope_texture;
#ifdef WIN32
  if (wsa_ok) WSACleanup();
#endif
}

double TelescopeMgr::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye) {
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  prj->set_orthographic_projection();	// set 2D coordinate
  telescope_texture->bind();
  glBlendFunc(GL_ONE,GL_ONE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    if (it->second->isConnected() && it->second->hasKnownPosition()) {
      Vec3d XY;
      if (prj->project_j2000_check(it->second->getObsJ2000Pos(0),XY)) {
        if (telescope_fader.getInterstate() >= 0) {
          glColor4f(circle_color[0],circle_color[1],circle_color[2],
                    telescope_fader.getInterstate());
          double radius = 15;
//          double radius = 0.5*prj->getRadPerPixel()*(M_PI/180)*0.5;
//          if (radius < 15) radius = 15;
//          if (radius > 0.5*prj->getViewportWidth())
//            radius = 0.5*prj->getViewportWidth();
//          if (radius > 0.5*prj->getViewportHeight())
//            radius = 0.5*prj->getViewportHeight();
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
          telescope_texture->bind();
        }
      }
    }
  }
  prj->reset_perspective_projection();
  return 0.;
}

void TelescopeMgr::update(double deltaTime) {
  name_fader.update((int)(deltaTime*1000));
  telescope_fader.update((int)(deltaTime*1000));
}

vector<StelObject> TelescopeMgr::searchAround(const Vec3d& vv, double limitFov, const Navigator * nav, const Projector * prj) const
{
  vector<StelObject> result;
  if (!getFlagTelescopes())
  	return result;
  Vec3d v(vv);
  v.normalize();
  double cos_lim_fov = cos(limitFov * M_PI/180.);
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();it++) {
    if (it->second->getObsJ2000Pos(0).dot(v) >= cos_lim_fov) {
      result.push_back(it->second);
    }
  }
  return result;
}

StelObject TelescopeMgr::searchByNameI18n(const wstring &nameI18n) const {
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

void TelescopeMgr::setFontSize(float font_size) {
	telescope_font = &StelApp::getInstance().getFontManager()
                       .getStandardFont(StelApp::getInstance()
                                .getLocaleMgr().getSkyLanguage(), font_size);
}


void TelescopeMgr::init(const InitParser& conf, LoadingBar& lb) {
  setFontSize(12.f);
  if (telescope_texture) {
    delete telescope_texture;
    telescope_texture = 0;
  }
  StelApp::getInstance().getTextureManager().setDefaultParams();
  telescope_texture = &StelApp::getInstance().getTextureManager().createTexture("telescope.png");
#ifdef WIN32
  if (!wsa_ok) return;
#endif
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
  
	setFlagTelescopes(conf.get_boolean("astro:flag_telescopes"));
	setFlagTelescopeName(conf.get_boolean("astro:flag_telescope_name"));  
  
  StelApp::getInstance().getGlobalObjectMgr().registerStelObjectMgr(this);
}

void TelescopeMgr::telescopeGoto(int telescope_nr,const Vec3d &j2000_pos) {
  TelescopeMap::const_iterator it(telescope_map.find(telescope_nr));
  if (it != telescope_map.end()) {
    it->second->telescopeGoto(j2000_pos);
  }
}


void TelescopeMgr::communicate(void) {
  if (!telescope_map.empty()) {
//    long long int t = GetNow();
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
//    t = GetNow() - t;
//    cout << "TelescopeMgr::communicate: " << t << endl;
  }
}
