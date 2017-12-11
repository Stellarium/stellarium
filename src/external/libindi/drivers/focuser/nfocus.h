/*
    NFocus
    Copyright (C) 2006 Markus Wildi (markus.wildi@datacomm.ch)
                  2011 Jasem Mutlaq (mutlaqja@ikarustech.com)
		  2013 Felix Krämer (rigelsys@felix-kraemer.de)

    Thanks to Rigel Systems, especially Gene Nolan and Leon Palmer,
    for their support in writing this driver.

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

#include "indifocuser.h"

class NFocus : public INDI::Focuser
{
  public:
    NFocus();
    virtual ~NFocus() = default;

    virtual bool Handshake();
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);

  protected:
    bool saveConfigItems(FILE *fp);

  private:
    unsigned char CalculateSum(char *rf_cmd);
    int SendCommand(char *rf_cmd);
    int SendRequest(char *rf_cmd);
    int ReadResponse(char *buf, int nbytes, int timeout);
    bool GetFocusParams();

    int updateNFTemperature(double *value);
    int updateNFInOutScalar(double *value);
    int updateNFMotorSettings(double *onTime, double *offTime, double *fastDelay);
    int moveNFInward(const double *value);
    int moveNFOutward(const double *value);
    int getNFAbsolutePosition(double *value);
    int setNFAbsolutePosition(const double *value);
    int setNFMaxPosition(double *value);
    int syncNF(const double *value);

    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    INumber SettingsN[3];
    INumberVectorProperty SettingsNP;

    INumber MinMaxPositionN[2];
    INumberVectorProperty MinMaxPositionNP;

    INumber MaxTravelN[1];
    INumberVectorProperty MaxTravelNP;

    INumber SyncN[1];
    INumberVectorProperty SyncNP;

    INumber InOutScalarN[1];
    INumberVectorProperty InOutScalarNP;

    INumber RelMovementN[1];
    INumberVectorProperty RelMovementNP;

    INumber AbsMovementN[1];
    INumberVectorProperty AbsMovementNP;
};
