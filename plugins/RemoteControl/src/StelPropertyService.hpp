/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2016 Florian Schaukowitsch
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

#ifndef STELPROPERTYSERVICE_HPP_
#define STELPROPERTYSERVICE_HPP_

#include "AbstractAPIService.hpp"
#include "StelPropertyMgr.hpp"

class StelPropertyService : public AbstractAPIService
{
	Q_OBJECT
public:
	StelPropertyService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~StelPropertyService() {}
protected:
	virtual void getImpl(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	virtual void postImpl(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;
private:
	StelPropertyMgr* propMgr;
};

#endif
