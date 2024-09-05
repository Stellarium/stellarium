/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009 Bogdan Marinov (this file,
 * reusing code written by Johannes Gajdosik in 2006)
 * 
 * Johannes Gajdosik wrote in 2006 the original telescope control feature
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 * 
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

#ifndef TELESCOPECLIENTDIRECTLX200_HPP
#define TELESCOPECLIENTDIRECTLX200_HPP

#include <QObject>
#include <QString>

#include "StelApp.hpp"
#include "StelObject.hpp"

#include "common/Server.hpp" //from the telescope server source tree
#include "TelescopeClient.hpp" //from the plug-in's source tree
#include "TelescopeControl.hpp"

class Lx200Connection;

//! Telescope client that connects directly to a Meade LX200 through a serial port.
//! This class has been created by merging the code of TelescopeTCP and ServerLx200.
class TelescopeClientDirectLx200 : public TelescopeClient, public Server
{
	Q_OBJECT
public:
	TelescopeClientDirectLx200(const QString &name, const QString &parameters, TelescopeControl::Equinox eq = TelescopeControl::EquinoxJ2000);
	~TelescopeClientDirectLx200(void) override
	{
		//hangup();
	}
	
	//======================================================================
	// Methods inherited from TelescopeClient
	bool isConnected(void) const override;
	
	//======================================================================
	// Methods inherited from Server
	void step(long long int timeout_micros) override;
	void communicationResetReceived(void);
	void longFormatUsedReceived(bool long_format);
	void raReceived(unsigned int ra_int);
	void decReceived(unsigned int dec_int);
	
private:
	//======================================================================
	// Methods inherited from TelescopeClient
	Vec3d getJ2000EquatorialPos(const StelCore* core=nullptr) const override;
	bool prepareCommunication() override;
	void performCommunication() override;
	void telescopeGoto(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	void telescopeSync(const Vec3d &j2000Pos, StelObjectP selectObject) override;
	bool isTelescopeSyncSupported() const override {return true;}
	void telescopeAbortSlew() override;
	bool isAbortSlewSupported() const override {return true;}
	bool isInitialized(void) const override;
	
	//======================================================================
	// Methods inherited from Server
	void sendPosition(unsigned int ra_int, int dec_int, int status);
	//TODO: Find out if this method is needed. It's called by Connection.
	void gotoReceived(unsigned int ra_int, int dec_int) override;
	void syncReceived(unsigned int ra_int, int dec_int) override;
	
private:
	void hangup(void);
	int time_delay;
	
	InterpolatedPosition interpolatedPosition;
	bool hasKnownPosition(void) const override
	{
		return interpolatedPosition.isKnown();
	}

	TelescopeControl::Equinox equinox;
	
	//======================================================================
	// Members inherited from ServerLx200
	Lx200Connection *lx200;
	bool long_format_used;
	bool answers_received;
	
	unsigned int last_ra;
	bool queue_get_position;
	long long int next_pos_time;
};

#endif // TELESCOPECLIENTDIRECTLX200_HPP
