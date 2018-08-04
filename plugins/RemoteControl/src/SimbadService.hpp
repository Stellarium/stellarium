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

#ifndef SIMBADSERVICE_HPP
#define SIMBADSERVICE_HPP

#include "AbstractAPIService.hpp"

class StelCore;
class StelObjectMgr;

//! @ingroup remoteControl
//! Allows SIMBAD object lookups like SearchDialog uses.
//!
//! @see \ref rcSimbadService
//! @note This service supports threaded operation.
class SimbadService : public AbstractAPIService
{
	Q_OBJECT
public:
	SimbadService(QObject* parent = Q_NULLPTR);

	//! Simbad lookups dont block the main thread
	virtual bool isThreadSafe() const Q_DECL_OVERRIDE { return true; }
	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("simbad"); }
	//! @brief Implements the HTTP GET method
	//! @see \ref rcSimbadServiceGET
	virtual void get(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
private:
	QString simbadServerUrl;
};



#endif
