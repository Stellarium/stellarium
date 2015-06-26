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

#ifndef SIMBADSERVICE_HPP_
#define SIMBADSERVICE_HPP_

#include "AbstractAPIService.hpp"

class StelCore;
class StelObjectMgr;

class SimbadService : public AbstractAPIService
{
	Q_OBJECT
public:
	SimbadService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~SimbadService() {}

	//! Simbad lookups dont block the main thread
	bool supportsThreadedOperation() const Q_DECL_OVERRIDE { return true; }
protected:
	virtual void getImpl(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
private:
	QString simbadServerUrl;
};



#endif
