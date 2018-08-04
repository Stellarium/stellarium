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

#ifndef STELPROPERTYSERVICE_HPP
#define STELPROPERTYSERVICE_HPP

#include "AbstractAPIService.hpp"
#include "StelPropertyMgr.hpp"

//! @ingroup remoteControl
//! Provides services related to StelProperty.
//! See also the StelProperty related operations of MainService.
//!
//! @see \ref rcStelPropertyService
//!
class StelPropertyService : public AbstractAPIService
{
	Q_OBJECT
public:
	StelPropertyService(QObject* parent = Q_NULLPTR);

	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("stelproperty"); }
	//! @brief Implements the HTTP GET method
	//! @see \ref rcStelPropertyServiceGET
	virtual void get(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	//! @brief Implements the HTTP POST method
	//! @see \ref rcStelPropertyServicePOST
	virtual void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;
private:
	StelPropertyMgr* propMgr;
};

#endif
