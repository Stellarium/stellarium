/*
    Lunatico Armadillo Platypus Focuser

    (c) Lunatico Astronomia 2017, Jaime Alemany
    Based on previous drivers by Jasem Mutlaq (mutlaqja@ikarustech.com)

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

class ArmPlat : public INDI::Focuser
{
  public:
    ArmPlat();
    virtual ~ArmPlat() = default;

    virtual bool Handshake();
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

protected:
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool AbortFocuser();
    virtual void TimerHit();
    virtual bool saveConfigItems(FILE *fp);

  private:
    bool slpSendRxInt( char *command, int *rcode );
    bool getIntResultCode( char *sent, char *rxed, int *rcode );
    bool getCurrentPos( uint32_t *curPos );
    bool getCurrentTemp( uint32_t *curTemp );
    bool sync(uint32_t newPosition);
    bool setMaxSpeed(uint16_t nspeed);
    bool setTempSensorInUse(uint16_t sensor);
    bool setWiring(uint16_t newwiring);
    bool setHalfStep( bool active );
    bool setBacklash(uint16_t value);
    bool setMotorType(uint16_t type);
    bool setPort(uint16_t newport );
    bool echo();
    bool Connect();

    uint16_t backlash { 0 };
    uint16_t tempSensInUse { 0 };
    bool isMoving = false;
    bool portWarned = false;
    int16_t port = -1;
    int16_t halfstep = -1;
    int16_t wiring = -1;
    int16_t speed = -1;
    int16_t motortype = -1;

    // Temperature probe
    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;
    ISwitch IntExtTempSensorS[2];
    ISwitchVectorProperty IntExtTempSensorSP;
    enum { INT_TEMP_SENSOR, EXT_TEMP_SENSOR };

    // Motor Mode
    ISwitch MotorTypeS[4];
    ISwitchVectorProperty MotorTypeSP;
    enum { MOTOR_UNIPOLAR, MOTOR_BIPOLAR, MOTOR_DC, MOTOR_STEPDIR };

    // Halfstep
    ISwitch HalfStepS[2];
    ISwitchVectorProperty HalfStepSP;
    enum { HALFSTEP_OFF, HALFSTEP_ON };

    // Enable/Disable backlash
    ISwitch BacklashCompensationS[2];
    ISwitchVectorProperty BacklashCompensationSP;
    enum { BACKLASH_ENABLED, BACKLASH_DISABLED };

    // Port
#   ifdef ARMADILLO
    enum { PORT_MAIN, PORT_EXP, TOTAL_PORTS };
#   else
    enum { PORT_MAIN, PORT_EXP, PORT_THIRD, TOTAL_PORTS };
#   endif
    ISwitch PerPortS[TOTAL_PORTS];
    ISwitchVectorProperty PerPortSP;

    // Backlash Value
    INumber BacklashN[1];
    INumberVectorProperty BacklashNP;

    // Motor wiring 
    ISwitch WiringS[4];
    ISwitchVectorProperty WiringSP;
    enum { WIRING_LUNATICO_NORMAL, WIRING_LUNATICO_REVERSED, WIRING_RFMOONLITE_NORMAL, WIRING_RFMOONLITE_REVERSED };

    // Maximum Speed
    INumber MaxSpeedN[1];
    INumberVectorProperty MaxSpeedNP;

    // Sync motor pos
    INumber SyncN[1];
    INumberVectorProperty SyncNP;


    // Firmware Version
    IText FirmwareVersionT[1];
    ITextVectorProperty FirmwareVersionTP;
};
