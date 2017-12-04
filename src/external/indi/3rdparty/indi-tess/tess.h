/*
    NACHO MAS 2013 (mas.ignacio at gmail.com)
    Driver for TESS custom hardware. See README

    Base on tutorial_Four
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#ifndef inditess_H
#define inditess_H

#include <defaultdevice.h>
#include <indicom.h>
#include "tess_algebra.h"

#define TESS_TIMEOUT         0.5 /* FD timeout in seconds */
#define TESS_BUFFER          255
#define TESS_MSG_SIZE        92
#define MAX_CONNETIONS_FAILS 10 /* Max number of connections fails after disconnect */
#define ALTAZ_FAST_CHANGE    10 /* If ALTAZ COORDs change more disactivate filter to increse responsiveness */

class inditess : public INDI::DefaultDevice
{
  public:
    inditess();
    ~inditess();

    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    float HzToMag(float HzTSL);

  protected:
    virtual void TimerHit();

  private:
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool Connect();
    virtual bool Disconnect();
    int HrztoEqu(double alt, double az, double *ra, double *dec);
    float filter;
    bool is_connected(void);
    int fd;
    float fH, tO, tA, aX, aY, aZ, mX, mY, mZ;
    float Lat, Lon;
    float az_offset, alt_offset;
    vector a_avg, m_avg, heading, MagMax, MagMin;
    vector p;
};

#endif
