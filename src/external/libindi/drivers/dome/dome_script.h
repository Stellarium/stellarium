/*******************************************************************************
 Copyright(c) 2016 CloudMakers, s. r. o.. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 *******************************************************************************/

#pragma once

#include "indidome.h"

class DomeScript : public INDI::Dome
{
  public:
    DomeScript();
    virtual ~DomeScript() = default;

    virtual const char *getDefaultName();
    virtual bool initProperties();
    virtual bool saveConfigItems(FILE *fp);

    void ISGetProperties(const char *dev);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool updateProperties();

  protected:
    void TimerHit();
    virtual bool Connect();
    virtual bool Disconnect();
    virtual IPState Move(DomeDirection dir, DomeMotionCommand operation);
    virtual IPState MoveAbs(double az);
    virtual IPState Park();
    virtual IPState UnPark();
    virtual IPState ControlShutter(ShutterOperation operation);
    virtual bool Abort();

  private:
    bool ReadDomeStatus();
    bool RunScript(int script, ...);

    ITextVectorProperty ScriptsTP;
    IText ScriptsT[15];
    double TargetAz { 0 };
    int TimeSinceUpdate { 0 };
};
