/*
 * Stellarium
 * Copyright (C) 2015 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef SPORADICMETEORMGR_HPP
#define SPORADICMETEORMGR_HPP

#include "SporadicMeteor.hpp"
#include "StelModule.hpp"

//! @class SporadicMeteorMgr
//! Simulates a sporadic meteor shower, with a random color and a random radiant.
//! @author Marcos Cardinot <mcardinot@gmail.com>
class SporadicMeteorMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(int zhr READ getZHR WRITE setZHR NOTIFY zhrChanged)
public:
	//! Constructor
	//! @param zhr Zenith Hourly Rate
	//! @param maxv Maximum meteor velocity
	SporadicMeteorMgr(int zhr, int maxv);
	virtual ~SporadicMeteorMgr();

	// Methods defined in the StelModule class
	virtual void init();
	virtual void draw(StelCore* core);
	virtual void update(double deltaTime);
	virtual double getCallOrder(StelModuleActionName actionName) const;

public slots:
	// Methods callable from script and GUI
	//! Get the current zenith hourly rate.
	int getZHR() const { return m_zhr; }
	//! Set the zenith hourly rate.
	void setZHR(int zhr);

	//! Set flag used to turn on and off meteor rendering.
	void setFlagShow(bool b) { m_flagShow = b; }
	//! Get value of flag used to turn on and off meteor rendering.
	bool getFlagShow() const { return m_flagShow; }

	//! Set the maximum velocity in km/s
	void setMaxVelocity(int maxv) { m_maxVelocity = maxv; }

	//! Set flag for enable activity of meteors when atmosphere is disabled.
	//! @note option for planetariums
	void setFlagForcedMeteorsActivity(bool b) {if(b!=m_flagForcedShow ){ m_flagForcedShow=b;}}
	//! Get flag for enable activity of meteors when atmosphere is disabled.
	//! @note option for planetariums
	bool getFlagForcedMeteorsActivity() const {return m_flagForcedShow;}

signals:
	void zhrChanged(int);

private:
	QList<SporadicMeteor*> activeMeteors;
	StelTextureSP m_bolideTexture;
	int m_zhr;
	int m_maxVelocity;
	bool m_flagShow;
	bool m_flagForcedShow;
};

#endif // SPORADICMETEORMGR_HPP
