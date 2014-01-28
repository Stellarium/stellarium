/*
 * Copyright (C) 2013 Felix Zeltner
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



#ifndef FLIGHTDATASOURCE_HPP
#define FLIGHTDATASOURCE_HPP

#include "Flight.hpp"
#include <QtCore>


class FlightDataSource : public QObject
{
    Q_OBJECT
public:
    FlightDataSource(double updateInterval)
    {
        this->updateInterval = updateInterval;
        time = 0;
        lastUpdate = 0;
    }

    //! Return a list of flights that match the current time
    //! and should therefor be displayed.
    virtual QList<FlightP> *getRelevantFlights() = 0;

    //! Load Flight objects relevant for time jd and unload old ones
    //! @param jd the current time
    //! @param fwd is the time moving forward or backward
    virtual void updateRelevantFlights(double jd, double rate) = 0;

    //! Called from FlightMgr::update()
    void update(double deltaTime, double jd, double rate)
    {
        time +=  deltaTime;
        if (time - lastUpdate > updateInterval) {
            lastUpdate = time;
            updateRelevantFlights(jd, rate);
        }
    }

    //! Gets called before this data source is being used.
    virtual void init() = 0;

    //! Gets called when this data source stops being in use.
    virtual void deinit() = 0;

protected:
    double updateInterval;
    double time;
    double lastUpdate;
};

#endif // FLIGHTDATASOURCE_HPP
