/*
 * RTS2 Json stellarium plugin
 * 
 * Copyright (C) 2014-2016 Petr Kubanek
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

#ifndef _TELESCOPE_CLIENT_JSON_RTS2_HPP_
#define _TELESCOPE_CLIENT_JSON_RTS2_HPP_

#include "QNetworkAccessManager"
#include "QNetworkRequest"
#include "QNetworkReply"
#include "QUrl"

#include "TelescopeClient.hpp"

#include "StelCore.hpp"

//! RTS2 JSON telescope.
class TelescopeClientJsonRts2 : public TelescopeClient
{
	Q_OBJECT
public:
	TelescopeClientJsonRts2(const QString &name, const QString &params, Equinox eq = EquinoxJ2000);
	~TelescopeClientJsonRts2(void);
	virtual bool isConnected(void) const;

private:
	QNetworkAccessManager networkManager;
	Equinox equinox;
	unsigned int port;
	QHostAddress address;
	QUrl baseurl;
	QNetworkRequest request;
	Vec3d position;

	Vec3d getJ2000EquatorialPos(const StelCore* core=0) const;
	void telescopeGoto(const Vec3d &j2000Pos);
	bool hasKnownPosition(void) const;

private slots:
	void replyFinished(QNetworkReply *reply);
};

#endif // _TELESCOPE_CLIENT_JSON_RTS2_HPP_
