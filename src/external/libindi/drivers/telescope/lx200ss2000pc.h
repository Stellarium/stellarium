/*
    SkySensor2000 PC
    Copyright (C) 2015 Camiel Severijns

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

class LX200SS2000PC : public LX200Generic
{
  public:
    LX200SS2000PC(void);

    virtual const char *getDefaultName(void);
    virtual bool updateTime(ln_date *utc, double utc_offset);
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

  protected:
    virtual void getBasicData(void);
    virtual bool isSlewComplete(void);
    virtual bool saveConfigItems(FILE *fp);
    virtual bool setUTCOffset(double offset);
    //bool setUTCOffset(const int offset_in_hours);

  private:
    bool getCalendarDate(int &year, int &month, int &day);
    bool setCalenderDate(int year, int month, int day);


    bool updateLocation(double latitude, double longitude, double elevation);
    int setLongitude(double Long);
    int setLatitude(double Long);
    int sendCommand(int fd, const char *data);

    INumber SlewAccuracyN[2];
    INumberVectorProperty SlewAccuracyNP;

    static const int ShortTimeOut;
    static const int LongTimeOut;
};

