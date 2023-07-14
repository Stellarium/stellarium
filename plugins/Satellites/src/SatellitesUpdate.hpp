/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

/*
 * The standard QDateTime type only stores to the millisecond but 
 * satellite propargation models use to microsecond timimg. 
 * For Stellarium display purposes millisecond accuracy is 
 * probably good enough. However, the Unit Tests source data
 * expectations from common SGP4 models and therefore fail to
 * agree to routines that do not account for microsecond timing.
 * This class therefore is to allow for us timings. 
 * 
 * Epoch times are stored as JD and JDF.
 */

#ifndef SATELLITES_OMMDATETIME_HPP
#define SATELLITES_OMMDATETIME_HPP

#include <QString>
#include <QDateTime>
#include <QSharedPointer>

//! @class SatellitesUpdate
//! Auxiliary class for the %Satellites plugin.
//! @author Andy Kirkham
//! @ingroup satellites
class SatellitesUpdate
{
public:
	//! @enum UpdateState
	//! Used for keeping track of the download/update status
	enum UpdateState
	{
		Updating,          //!< Update in progress
		CompleteNoUpdates, //!< Update completed, there we no updates
		CompleteUpdates,   //!< Update completed, there were updates
		DownloadError,     //!< Error during download phase
		OtherError         //!< Other error
	};

		//! Flags used to filter the satellites list according to their status.
	enum Status
	{
		Visible,
		NotVisible,
		Both,
		NewlyAdded,
		OrbitError
	};

};

#endif
