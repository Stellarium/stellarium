/*
 Starlight Xpress Filter Wheel INDI Driver

 Copyright (c) 2012 Cloudmakers, s. r. o.
 All Rights Reserved.

 Code is based on SX Filter Wheel INDI Driver by Gerry Rozema
 Copyright(c) 2010 Gerry Rozema.
 All rights reserved.

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 The full GNU General Public License is included in this distribution in the
 file called LICENSE.
 */

#pragma once

#include <indifilterwheel.h>

#include <hidapi.h>

class SXWHEEL : public INDI::FilterWheel
{
  private:
    hid_device *handle;
    int SendWheelMessage(int a, int b);

  public:
    SXWHEEL();
    ~SXWHEEL();

    bool Connect();
    bool Disconnect();
    const char *getDefaultName();

    bool initProperties();

    int QueryFilter();
    bool SelectFilter(int);
    void TimerHit();
};
