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

#include "TelescopeMgr.hpp"
#include "Telescope.hpp"
#include "StelObject.hpp"
#include "Projector.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelFontMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"

#include <algorithm>
#include <QSettings>
#include <QString>
#include <QStringList>

#ifdef WIN32
  #include <winsock2.h> // select
#else
  #include <sys/select.h> // select
  #include <sys/time.h>
  #include <signal.h>
#endif

void TelescopeMgr::TelescopeMap::clear(void) {
  for (const_iterator it(begin());it!=end();++it) delete it->second;
  std::map<int,Telescope*>::clear();
}

TelescopeMgr::TelescopeMgr(void) : telescope_font(NULL) {
	setObjectName("TelescopeMgr");
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
#ifdef WIN32
  if (wsa_ok) WSACleanup();
#endif
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double TelescopeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ACTION_DRAW)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName)+10;
	return 0;
}

double TelescopeMgr::draw(StelCore* core)
{
	Navigator* nav = core->getNavigation();
	Projector* prj = core->getProjection();
	
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  prj->setCurrentFrame(Projector::FRAME_J2000);
  telescope_texture->bind();
  glBlendFunc(GL_ONE,GL_ONE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();++it) {
    if (it->second->isConnected() && it->second->hasKnownPosition()) {
      Vec3d XY;
      if (prj->projectCheck(it->second->getObsJ2000Pos(0),XY)) {
        if (telescope_fader.getInterstate() >= 0) {
          glColor4f(circle_color[0],circle_color[1],circle_color[2],
                    telescope_fader.getInterstate());
          glDisable(GL_TEXTURE_2D);
          for (std::list<double>::const_iterator
               it2(it->second->getOculars().begin());
               it2!=it->second->getOculars().end();++it2) {
            prj->drawCircle(XY[0],XY[1],
                            0.5*prj->getPixelPerRadAtCenter()*(M_PI/180)*(*it2));
          }
          glEnable(GL_TEXTURE_2D);
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
          glColor4f(label_color[0],label_color[1],label_color[2], name_fader.getInterstate());
          prj->drawText(telescope_font, XY[0],XY[1],it->second->getNameI18n(), 0, 6, -4, false);
          telescope_texture->bind();
        }
      }
    }
  }
  
  drawPointer(prj, nav);
  
  return 0.;
}

void TelescopeMgr::update(double deltaTime) {
  name_fader.update((int)(deltaTime*1000));
  telescope_fader.update((int)(deltaTime*1000));
 	// communicate with the telescopes:
	communicate();
}

void TelescopeMgr::setColorScheme(const QSettings* conf, const QString& section)
{
	// Load colors from config file
	QString defaultColor = conf->value(section+"/default_color").toString();
	set_label_color(StelUtils::str_to_vec3f(conf->value(section+"/telescope_label_color", defaultColor).toString()));
	set_circle_color(StelUtils::str_to_vec3f(conf->value(section+"/telescope_circle_color", defaultColor).toString()));
}

vector<StelObjectP> TelescopeMgr::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
  vector<StelObjectP> result;
  if (!getFlagTelescopes())
  	return result;
  Vec3d v(vv);
  v.normalize();
  double cos_lim_fov = cos(limitFov * M_PI/180.);
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();++it) {
    if (it->second->getObsJ2000Pos(0).dot(v) >= cos_lim_fov) {
      result.push_back(it->second);
    }
  }
  return result;
}

StelObjectP TelescopeMgr::searchByNameI18n(const QString &nameI18n) const {
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();++it) {
    if (it->second->getNameI18n() == nameI18n) return it->second;
  }
  return 0;
}


StelObjectP TelescopeMgr::searchByName(const QString &name) const {
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();++it) {
    if (it->second->getEnglishName() == name) return it->second;
  }
  return 0;
}

QStringList TelescopeMgr::listMatchingObjectsI18n(
                                const QString& objPrefix,
                                int maxNbItem) const {
  QStringList result;
  if (maxNbItem==0) return result;

  QString objw = objPrefix.toUpper();
  for (TelescopeMap::const_iterator it(telescope_map.begin());
       it!=telescope_map.end();++it) {
    QString constw = it->second->getNameI18n().mid(0, objw.size()).toUpper();
    if (constw==objw) {
      result << it->second->getNameI18n();
    }
  }
  result.sort();
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


void TelescopeMgr::init() {
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	setFontSize(12.f);
	StelApp::getInstance().getTextureManager().setDefaultParams();
	telescope_texture = StelApp::getInstance().getTextureManager().createTexture("telescope.png");
#ifdef WIN32
	if (!wsa_ok) return;
#endif
	telescope_map.clear();
	for (int i=0;i<9;i++) 
	{
		char name[2] = {'0'+i,'\0'};
		const QString telescope_name(name);
		const QString url = conf->value("telescopes/"+telescope_name,"").toString();
		if (!url.isEmpty()) 
		{
			Telescope *t = Telescope::create(url.toStdString());
			if (t) 
			{
				for (int j=0;j<9;j++) 
				{
					name[0] = '0'+j;
					const double fov = conf->value("telescopes/"+telescope_name+"_ocular_"+name, -1.0).toDouble();
					t->addOcular(fov);
				}
				telescope_map[i] = t;
			}
		}
	}

	setFlagTelescopes(conf->value("astro/flag_telescopes",false).toBool());
	setFlagTelescopeName(conf->value("astro/flag_telescope_name",false).toBool());  

	StelApp::getInstance().getStelObjectMgr().registerStelObjectMgr(this);

	// Load pointer texture
	texPointer = StelApp::getInstance().getTextureManager().createTexture("pointeur2.png");   
}


void TelescopeMgr::drawPointer(const Projector* prj, const Navigator * nav)
{
	const std::vector<StelObjectP> newSelected = StelApp::getInstance().getStelObjectMgr().getSelectedObject("Telescope");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getObsJ2000Pos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
	
		glColor3fv(obj->getInfoColor());
		texPointer->bind();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
        
        prj->drawSprite2dMode(screenpos[0], screenpos[1], 50., StelApp::getInstance().getTotalRunTime()*40.);
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
//    long long int t = GetNow();
    fd_set read_fds,write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    int fd_max = -1;
    for (TelescopeMap::const_iterator it(telescope_map.begin());
         it!=telescope_map.end();++it) {
      it->second->prepareSelectFds(read_fds,write_fds,fd_max);
    }
    if (fd_max >= 0) {
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      const int select_rc = select(fd_max+1,&read_fds,&write_fds,0,&tv);
      if (select_rc > 0) {
        for (TelescopeMap::const_iterator it(telescope_map.begin());
             it!=telescope_map.end();++it) {
          it->second->handleSelectFds(read_fds,write_fds);
        }
      }
    }
//    t = GetNow() - t;
//    cout << "TelescopeMgr::communicate: " << t << endl;
  }
}
