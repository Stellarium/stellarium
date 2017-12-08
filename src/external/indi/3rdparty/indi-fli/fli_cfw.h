#if 0
    FLI CFW
    INDI Interface for Finger Lakes Instrument Filter Wheels
    Copyright (C) 2003-2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#endif

#pragma once

#include <libfli.h>
#include <indifilterwheel.h>
#include <string.h>

using namespace std;

#define MAX_FILTERS_SIZE 20

class FLICFW : public INDI::FilterWheel
{
  public:
    FLICFW();
    virtual ~FLICFW();

    const char *getDefaultName();

    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    virtual bool SelectFilter(int);
    virtual int QueryFilter();
    void TimerHit();

  private:
    typedef struct
    {
        flidomain_t domain;
        char *dname;
        char *name;
        char model[32];
        long HWRevision;
        long FWRevision;
        long current_pos;
        long count;
    } filter_t;

    ISwitch PortS[4];
    ISwitchVectorProperty PortSP;

    IText FilterInfoT[3];
    ITextVectorProperty FilterInfoTP;

    ISwitch FilterS[2];
    ISwitchVectorProperty FilterSP;

    int timerID;
    bool sim;

    flidev_t fli_dev;
    filter_t FLIFilter;

    bool findFLICFW(flidomain_t domain);
    void turnWheel();
    bool setupParams();
};
