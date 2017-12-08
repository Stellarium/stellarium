/*
    10micron INDI driver

    Copyright (C) 2017 Hans Lambermont

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "lx200generic.h"

typedef enum {
    GSTAT_TRACKING                    = 0,
    GSTAT_STOPPED                     = 1,
    GSTAT_PARKING                     = 2,
    GSTAT_UNPARKING                   = 3,
    GSTAT_SLEWING_TO_HOME             = 4,
    GSTAT_PARKED                      = 5,
    GSTAT_SLEWING_OR_STOPPING         = 6,
    GSTAT_NOT_TRACKING_AND_NOT_MOVING = 7,
    GSTAT_MOTORS_TOO_COLD             = 8,
    GSTAT_TRACKING_OUTSIDE_LIMITS     = 9,
    GSTAT_FOLLOWING_SATELLITE         = 10,
    GSTAT_NEED_USEROK                 = 11,
    GSTAT_UNKNOWN_STATUS              = 98,
    GSTAT_ERROR                       = 99
} _10MICRON_GSTAT;

class LX200_10MICRON : public LX200Generic
{
  public:
    LX200_10MICRON();
    ~LX200_10MICRON() {}

    virtual const char *getDefaultName() override;
    virtual bool Handshake() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual bool ReadScopeStatus() override;
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool SyncConfigBehaviour(bool cmcfg);

    // TODO move this thing elsewhere
    int monthToNumber(const char *monthName);
    int setStandardProcedureWithoutRead(int fd, const char *data);

  protected:
    virtual void getBasicData() override;

    IText ProductT[4];
    ITextVectorProperty ProductTP;

  private:
    int fd = -1; // short notation for PortFD/sockfd
    bool getMountInfo();

    int OldGstat = -1;
};
