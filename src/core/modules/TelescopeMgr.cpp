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
#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelCore.hpp"
#include "StelStyle.hpp"
#include "StelPainter.hpp"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QDebug>

void TelescopeMgr::deleteAllTelescopes()
{
	foreach (Stel::Telescope* t, telescope_map)
		delete t;
	telescope_map.clear();
}

TelescopeMgr::TelescopeMgr()
{
	setObjectName("TelescopeMgr");
}

TelescopeMgr::~TelescopeMgr(void)
{
}

/*************************************************************************
Reimplementation of the getCallOrder method
*************************************************************************/
double TelescopeMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName)+10;
	return 0;
}

void TelescopeMgr::draw(StelCore* core)
{
	StelNavigator* nav = core->getNavigator();
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(telescope_font);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	telescopeTexture->bind();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	foreach (Stel::Telescope* tel, telescope_map)
	{
		if (tel->isConnected() && tel->hasKnownPosition())
		{
			Vec3d XY;
			if (prj->projectCheck(tel->getJ2000EquatorialPos(nav),XY))
			{
				if (telescopeFader.getInterstate() >= 0)
				{
					sPainter.setColor(circleColor[0],circleColor[1],circleColor[2], telescopeFader.getInterstate());
					sPainter.enableTexture2d(false);
					foreach (double occul, tel->getOculars())
					{
						sPainter.drawCircle(XY[0],XY[1],0.5*prj->getPixelPerRadAtCenter()*(M_PI/180)*(occul));
					}
					sPainter.enableTexture2d(true);
					sPainter.drawSprite2dMode(XY[0],XY[1],15.f);
				}
				if (nameFader.getInterstate() >= 0)
				{
					sPainter.setColor(labelColor[0],labelColor[1],labelColor[2], nameFader.getInterstate());
					sPainter.drawText(XY[0],XY[1],tel->getNameI18n(), 0, 6, -4, false);
					telescopeTexture->bind();
				}
			}
		}
	}
	drawPointer(prj, nav, sPainter);
}

void TelescopeMgr::update(double deltaTime)
{
	nameFader.update((int)(deltaTime*1000));
	telescopeFader.update((int)(deltaTime*1000));
	// communicate with the telescopes:
	communicate();
}

void TelescopeMgr::setStelStyle(const StelStyle& style)
{
	// Load colors from config file
	QSettings* conf = StelApp::getInstance().getSettings();
	QString section = style.confSectionName;
	
	QString defaultColor = conf->value(section+"/default_color").toString();
	setLabelColor(StelUtils::strToVec3f(conf->value(section+"/telescope_label_color", defaultColor).toString()));
	setCircleColor(StelUtils::strToVec3f(conf->value(section+"/telescope_circle_color", defaultColor).toString()));
}

QList<StelObjectP> TelescopeMgr::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagTelescopes())
		return result;
	Vec3d v(vv);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	foreach (Stel::Telescope* tel, telescope_map)
	{
		if (tel->getJ2000EquatorialPos(core->getNavigator()).dot(v) >= cosLimFov)
		{
			result.push_back(tel);
		}
	}
	return result;
}

StelObjectP TelescopeMgr::searchByNameI18n(const QString &nameI18n) const
{
	foreach (Stel::Telescope* tel, telescope_map)
	{
		if (tel->getNameI18n() == nameI18n)
			return tel;
	}
	return 0;
}

StelObjectP TelescopeMgr::searchByName(const QString &name) const
{
	foreach (Stel::Telescope* tel, telescope_map)
	{
		if (tel->getEnglishName() == name)
		return tel;
	}
	return 0;
}

QStringList TelescopeMgr::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();
	foreach (const Stel::Telescope* tel, telescope_map)
	{
		QString constw = tel->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << tel->getNameI18n();
		}
	}
	result.sort();
	if (result.size()>maxNbItem)
	{
		result.erase(result.begin()+maxNbItem, result.end());
	}
	return result;
}

void TelescopeMgr::setFontSize(float fontSize)
{
	telescope_font.setPixelSize(fontSize);
}

void TelescopeMgr::moveTelescopeToSelected(void)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	if (sender() == NULL)
		return;

	StelObjectP selectObject = omgr->getSelectedObject().at(0);
	if (!selectObject)  // should never happen
		return;

	Vec3d objectPos = selectObject->getJ2000EquatorialPos(StelApp::getInstance().getCore()->getNavigator());
	int telNum = sender()->objectName().right(1).toInt();
	telescopeGoto(telNum, objectPos);
}

void TelescopeMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setFontSize(12.f);
	telescopeTexture = StelApp::getInstance().getTextureManager().createTexture("textures/telescope.png");

	deleteAllTelescopes();
	for (int i=0;i<9;i++) 
	{
		char name[2] = {'0'+i,'\0'};
		const QString telescope_name(name);
		const QString url = conf->value("telescopes/"+telescope_name,"").toString();
		//qWarning() << telescope_name;
		if (!url.isEmpty()) 
		{
			qWarning() << url;
			Stel::Telescope *t = Stel::Telescope::create(url);
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

	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);

	// Load pointer texture
	texPointer = StelApp::getInstance().getTextureManager().createTexture("textures/pointeur2.png");
}


void TelescopeMgr::drawPointer(const StelProjectorP& prj, const StelNavigator * nav, StelPainter& sPainter)
{
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Telescope");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos=obj->getJ2000EquatorialPos(nav);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos)) return;
	
		const Vec3d& c(obj->getInfoColor());
		sPainter.setColor(c[0],c[1],c[2]);
		texPointer->bind();
		sPainter.enableTexture2d(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], 25., StelApp::getInstance().getTotalRunTime()*40.);
	}
}

void TelescopeMgr::telescopeGoto(int telescope_nr,const Vec3d &j2000Pos)
{
	QMap<int, Stel::Telescope*>::ConstIterator it(telescope_map.find(telescope_nr));
	if (it != telescope_map.end())
	{
		it.value()->telescopeGoto(j2000Pos);
	}
}

void TelescopeMgr::communicate(void)
{
	if (!telescope_map.empty())
	{
		foreach (Stel::Telescope* tel, telescope_map)
		{
			if(tel->prepareCommunication())
				tel->performCommunication();
		}
	}
//		t = getNow() - t;
//		qDebug() << "TelescopeMgr::communicate: " << t;
}
