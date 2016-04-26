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

#ifndef LOCATIONSERVICE_HPP_
#define LOCATIONSERVICE_HPP_

#include "AbstractAPIService.hpp"

class StelCore;
class StelLocationMgr;
class SolarSystem;

//! @ingroup remoteControl
//! Provides methods to look up location-related information, and change the current location
//!
//! ## Service methods
//! @copydetails getImpl()
//! @copydetails postImpl()
//!
//! @sa LocationSearchService
class LocationService : public AbstractAPIService
{
	Q_OBJECT
public:
	LocationService(const QByteArray& serviceName, QObject* parent = 0);

	virtual ~LocationService() {}

protected:
	//! @brief Implements the HTTP GET requests
	//! @details
	//! ### GET operations
	//! #### list
	//! Returns the list of all stored location IDs (keys of StelLocationMgr::getAllMap) as JSON string array
	//! #### countrylist
	//! Returns the list of all known countries (StelLocaleMgr::getAllCountryNames), as a JSON array of objects of format
	//! @code
	//! {
	//!	name, //the english country name
	//!	name_i18n //the localized country name (current language)
	//! }
	//! @endcode
	//! #### planetlist
	//! Returns the list of all solar system planet names (SolarSystem::getAllPlanetsEnglishNames), as a JSON array of objects of format
	//! @code
	//! {
	//!	name, //the english planet
	//!	name_i18n //the localized planet name (current language)
	//! }
	//! @endcode
	//! #### planetimage
	//! Parameters: <tt>planet (String)</tt>\n
	//! Returns the planet texture image for the \p planet (english name)
	//!
	virtual void getImpl(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;
	//! @brief Implements the HTTP POST requests
	//! @details
	//! ### POST operations
	//! #### setlocationfields
	//! Parameters: <tt>id (String) | ( [latitude (Number)] [longitude (Number)] [altitude (Number)] [name (String)] [country (String)] [planet (String)] )</tt>\n
	//! Changes and moves to a new location.
	//! If \p id is given, all other parameters are ignored, and a location is searched from the named locations using StelLocationMgr::locationForString with the \p id.
	//! Else, the other parameters change the specific field of the current StelLocation.
	virtual void postImpl(const QByteArray &operation, const APIParameters& parameters, const QByteArray &data, APIServiceResponse &response) Q_DECL_OVERRIDE;
private:
	StelCore* core;
	StelLocationMgr* locMgr;
	SolarSystem* ssys;
};



#endif
