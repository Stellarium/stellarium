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

#ifndef STELACTIONSERVICE_HPP
#define STELACTIONSERVICE_HPP

#include "AbstractAPIService.hpp"

class StelActionMgr;

//! @ingroup remoteControl
//! Provides services related to StelAction.
//! See also the StelAction related operations of MainService.
//!
//! @see \ref rcStelActionService
//!
class StelActionService : public AbstractAPIService
{
	Q_OBJECT
public:
	StelActionService(QObject* parent = Q_NULLPTR);

	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("stelaction"); }
	//! @brief Implements the HTTP GET method
	//! @see \ref rcStelActionServiceGET
	virtual void get(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	//! @brief Implements the HTTP POST method
	//! @see \ref rcStelActionServicePOST
	virtual void post(const QByteArray &operation, const APIParameters &parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;
private:
	StelActionMgr* actionMgr;
};



#endif
