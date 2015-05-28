/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#ifndef MAINSERVICE_HPP_
#define MAINSERVICE_HPP_

#include "APIController.hpp"

class StelCore;
class StelLocaleMgr;
class StelScriptMgr;

class MainService : public AbstractAPIService
{
	Q_OBJECT
public:
	MainService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~MainService() {}

	virtual void get(const QByteArray& operation,const QMultiMap<QByteArray,QByteArray>& parameters, HttpResponse& response) Q_DECL_OVERRIDE;
	virtual void post(const QByteArray &operation, const QMultiMap<QByteArray, QByteArray> &parameters, const QByteArray &data, HttpResponse &response) Q_DECL_OVERRIDE;
private:
	StelCore* core;
	StelLocaleMgr* localeMgr;
	StelScriptMgr* scriptMgr;

};



#endif
