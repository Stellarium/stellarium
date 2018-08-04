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

#ifndef VIEWSERVICE_HPP
#define VIEWSERVICE_HPP

#include "AbstractAPIService.hpp"

class StelCore;
class LandscapeMgr;
class StelSkyCultureMgr;

//! @ingroup remoteControl
//! Provides services related to the view (landscape, sky culture, projection).
//!
//! @see \ref rcViewService
class ViewService : public AbstractAPIService
{
	Q_OBJECT
public:
	ViewService(QObject* parent = Q_NULLPTR);

	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("view"); }
	//! @brief Implements the HTTP GET operations
	//! @see \ref rcViewServiceGET
	virtual void get(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
private:
	StelCore* core;
	LandscapeMgr* lsMgr;
	StelSkyCultureMgr* skyCulMgr;
};

#endif
