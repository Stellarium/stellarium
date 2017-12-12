/*
    Pegasus DMFC Focuser
    Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "dmfc.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <cstring>
#include <memory>

#include <termios.h>
#include <unistd.h>

#define DMFC_TIMEOUT 3
#define FOCUS_SETTINGS_TAB "Settings"
#define TEMPERATURE_THRESHOLD 0.1

#define POLLMS 500

std::unique_ptr<DMFC> dmfc(new DMFC());

void ISGetProperties(const char *dev)
{
    dmfc->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    dmfc->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    dmfc->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    dmfc->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    dmfc->ISSnoopDevice(root);
}

DMFC::DMFC()
{
    // Can move in Absolute & Relative motions, can AbortFocuser motion.
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
}

bool DMFC::initProperties()
{
    INDI::Focuser::initProperties();

    // Sync
    IUFillNumber(&SyncN[0], "FOCUS_SYNC_OFFSET", "Offset", "%6.0f", 0, 60000., 0., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    // Focuser temperature
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50, 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Reverse direction
    IUFillSwitch(&ReverseS[DIRECTION_NORMAL], "Normal", "", ISS_ON);
    IUFillSwitch(&ReverseS[DIRECTION_REVERSED], "Reverse", "", ISS_OFF);
    IUFillSwitchVector(&ReverseSP, ReverseS, 2, getDeviceName(), "Reverse", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY,
                       0, IPS_IDLE);

    // Max Speed
    IUFillNumber(&MaxSpeedN[0], "Value", "", "%6.2f", 100, 1000., 100., 400.);
    IUFillNumberVector(&MaxSpeedNP, MaxSpeedN, 1, getDeviceName(), "MaxSpeed", "", FOCUS_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Enable/Disable backlash
    IUFillSwitch(&BacklashCompensationS[BACKLASH_ENABLED], "Enable", "", ISS_OFF);
    IUFillSwitch(&BacklashCompensationS[BACKLASH_DISABLED], "Disable", "", ISS_ON);
    IUFillSwitchVector(&BacklashCompensationSP, BacklashCompensationS, 2, getDeviceName(), "Backlash Compensation", "",
                       FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Backlash Value
    IUFillNumber(&BacklashN[0], "Value", "", "%.f", 0, 9999, 100., 0.);
    IUFillNumberVector(&BacklashNP, BacklashN, 1, getDeviceName(), "Backlash", "", FOCUS_SETTINGS_TAB, IP_RW, 0,
                       IPS_IDLE);

    // Encoders
    IUFillSwitch(&EncoderS[ENCODERS_ON], "On", "", ISS_ON);
    IUFillSwitch(&EncoderS[ENCODERS_OFF], "Off", "", ISS_OFF);
    IUFillSwitchVector(&EncoderSP, EncoderS, 2, getDeviceName(), "Encoders", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Motor Modes
    IUFillSwitch(&MotorTypeS[MOTOR_DC], "DC", "", ISS_OFF);
    IUFillSwitch(&MotorTypeS[MOTOR_STEPPER], "Stepper", "", ISS_ON);
    IUFillSwitchVector(&MotorTypeSP, MotorTypeS, 2, getDeviceName(), "Motor Type", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // LED
    IUFillSwitch(&LEDS[LED_OFF], "Off", "", ISS_ON);
    IUFillSwitch(&LEDS[LED_ON], "On", "", ISS_OFF);
    IUFillSwitchVector(&LEDSP, LEDS, 2, getDeviceName(), "LED", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Firmware Version
    IUFillText(&FirmwareVersionT[0], "Version", "Version", "");
    IUFillTextVector(&FirmwareVersionTP, FirmwareVersionT, 1, getDeviceName(), "Firmware", "Firmware", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);


    // Relative and absolute movement
    FocusRelPosN[0].min   = 0.;
    FocusRelPosN[0].max   = 50000.;
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step  = 1000;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = 100000.;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1000;

    addDebugControl();

    updatePeriodMS = POLLMS;

    serialConnection->setDefaultBaudRate(Connection::Serial::B_19200);

    return true;
}

bool DMFC::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);        
        defineNumber(&SyncNP);

        defineSwitch(&ReverseSP);
        defineSwitch(&BacklashCompensationSP);
        defineNumber(&BacklashNP);
        defineSwitch(&EncoderSP);
        defineSwitch(&MotorTypeSP);
        defineNumber(&MaxSpeedNP);
        defineSwitch(&LEDSP);
        defineText(&FirmwareVersionTP);
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(SyncNP.name);

        deleteProperty(ReverseSP.name);
        deleteProperty(BacklashCompensationSP.name);
        deleteProperty(BacklashNP.name);
        deleteProperty(EncoderSP.name);
        deleteProperty(MotorTypeSP.name);
        deleteProperty(MaxSpeedNP.name);
        deleteProperty(LEDSP.name);
        deleteProperty(FirmwareVersionTP.name);
    }

    return true;
}

bool DMFC::Handshake()
{
    if (ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "DMFC is online. Getting focus parameters...");
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION,
          "Error retreiving data from DMFC, please ensure DMFC controller is powered and the port is correct.");
    return false;
}

const char *DMFC::getDefaultName()
{
    return "Pegasus DMFC";
}

bool DMFC::ack()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[2] = {0};
    char res[16]={0};

    cmd[0] = '#';
    cmd[1] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%#02X>", cmd[0]);

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, cmd, 2, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, res, 0xA, DMFC_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    // Get rid of 0xA
    res[nbytes_read-1] = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    tcflush(PortFD, TCIOFLUSH);

    return (!strcmp(res, "OK_DMFCN"));
}

bool DMFC::sync(uint32_t newPosition)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "W:%d", newPosition);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    // Set Position
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "sync error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::move(uint32_t newPosition)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "M:%d", newPosition);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    // Set Position
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "move error: %s.", errstr);
        return false;
    }

    return true;
}


bool DMFC::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        /////////////////////////////////////////////
        // Backlash
        /////////////////////////////////////////////
        if (!strcmp(name, BacklashCompensationSP.name))
        {
            IUUpdateSwitch(&BacklashCompensationSP, states, names, n);
            bool rc = false;
            if (IUFindOnSwitchIndex(&BacklashCompensationSP) == BACKLASH_ENABLED)
                rc = setBacklash(BacklashN[0].value);
            else
                rc = setBacklash(0);

            BacklashCompensationSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&BacklashCompensationSP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // Encoders
        /////////////////////////////////////////////
        else if (!strcmp(name, EncoderSP.name))
        {
            IUUpdateSwitch(&EncoderSP, states, names, n);
            bool rc = setEncodersEnabled(EncoderS[ENCODERS_ON].s == ISS_ON);
            EncoderSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&EncoderSP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // LED
        /////////////////////////////////////////////
        else if (!strcmp(name, LEDSP.name))
        {
            IUUpdateSwitch(&LEDSP, states, names, n);
            bool rc = setLedEnabled(LEDS[LED_ON].s == ISS_ON);
            LEDSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&LEDSP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // Reverse
        /////////////////////////////////////////////
        else if (!strcmp(name, ReverseSP.name))
        {
            IUUpdateSwitch(&ReverseSP, states, names, n);
            bool rc = setReverseEnabled(ReverseS[DIRECTION_REVERSED].s == ISS_ON);
            ReverseSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&ReverseSP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // Motor Type
        /////////////////////////////////////////////
        else if (!strcmp(name, MotorTypeSP.name))
        {
            IUUpdateSwitch(&MotorTypeSP, states, names, n);
            bool rc = setMotorType(IUFindOnSwitchIndex(&MotorTypeSP));
            MotorTypeSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&MotorTypeSP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool DMFC::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        /////////////////////////////////////////////
        // Sync
        /////////////////////////////////////////////
        if (strcmp(name, SyncNP.name) == 0)
        {
            IUUpdateNumber(&SyncNP, values, names, n);
            bool rc = sync(SyncN[0].value);
            SyncNP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetNumber(&SyncNP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // Backlash
        /////////////////////////////////////////////
        else if (strcmp(name, BacklashNP.name) == 0)
        {
            IUUpdateNumber(&BacklashNP, values, names, n);
            // Only updaet backlash value if compensation is enabled
            if (BacklashCompensationS[BACKLASH_ENABLED].s == ISS_ON)
            {
                bool rc = setBacklash(BacklashN[0].value);
                BacklashNP.s = rc ? IPS_OK : IPS_ALERT;
            }
            else
                BacklashNP.s = IPS_OK;

            IDSetNumber(&BacklashNP, nullptr);
            return true;
        }
        /////////////////////////////////////////////
        // MaxSpeed
        /////////////////////////////////////////////
        else if (strcmp(name, MaxSpeedNP.name) == 0)
        {
            IUUpdateNumber(&MaxSpeedNP, values, names, n);
            bool rc = setMaxSpeed(MaxSpeedN[0].value);
            MaxSpeedNP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetNumber(&MaxSpeedNP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

bool DMFC::updateFocusParams()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[3] = {0};
    char res[64];
    cmd[0] = 'A';
    cmd[1] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%#02X>", cmd[0]);

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, cmd, 2, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "GetFocusParams error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, res, 0xA, DMFC_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "GetFocusParams error: %s.", errstr);
        return false;
    }

    res[nbytes_read-1] = 0;
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    tcflush(PortFD, TCIOFLUSH);

    char *token = std::strtok(res, ":");

    // #1 Status
    if (token == nullptr || strcmp(token, "OK_DMFCN"))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid status response.");
        return false;
    }

    // #2 Version
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid version response.");
        return false;
    }

    if (FirmwareVersionT[0].text == nullptr || strcmp(FirmwareVersionT[0].text, token))
    {
        IUSaveText(&FirmwareVersionT[0], token);
        FirmwareVersionTP.s = IPS_OK;
        IDSetText(&FirmwareVersionTP, nullptr);
    }

    // #3 Motor Type
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid motor mode response.");
        return false;
    }

    int motorType = atoi(token);
    if (motorType != IUFindOnSwitchIndex(&MotorTypeSP) && motorType >= 0 && motorType <= 1)
    {
        IUResetSwitch(&MotorTypeSP);
        MotorTypeS[motorType].s = ISS_ON;
        MotorTypeSP.s = IPS_OK;
        IDSetSwitch(&MotorTypeSP, nullptr);
    }

    // #4 Temperature
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid temperature response.");
        return false;
    }

    double temperature = atof(token);
    if (temperature == -127)
    {
        TemperatureNP.s = IPS_ALERT;
        IDSetNumber(&TemperatureNP, nullptr);
    }
    else
    {
        if (fabs(temperature - TemperatureN[0].value) > TEMPERATURE_THRESHOLD)
        {
            TemperatureN[0].value = temperature;
            TemperatureNP.s = IPS_OK;
            IDSetNumber(&TemperatureNP, nullptr);
        }
    }

    // #5 Position
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid position response.");
        return false;
    }

    currentPosition = atoi(token);
    if (currentPosition != FocusAbsPosN[0].value)
    {
        FocusAbsPosN[0].value = currentPosition;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    // #6 Moving Status
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid moving satus response.");
        return false;
    }

    isMoving = (token[0] == '1');

    // #7 LED Status
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid LED response.");
        return false;
    }

    int ledStatus = atoi(token);
    if (ledStatus != IUFindOnSwitchIndex(&LEDSP) && ledStatus >= 0 && ledStatus <= 1)
    {
        IUResetSwitch(&LEDSP);
        LEDS[ledStatus].s = ISS_ON;
        LEDSP.s = IPS_OK;
        IDSetSwitch(&LEDSP, nullptr);
    }

    // #8 Reverse Status
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid reverse response.");
        return false;
    }

    int reverseStatus = atoi(token);
    if (reverseStatus != IUFindOnSwitchIndex(&ReverseSP) && reverseStatus >= 0 && reverseStatus <= 1)
    {
        IUResetSwitch(&ReverseSP);
        ReverseS[reverseStatus].s = ISS_ON;
        ReverseSP.s = IPS_OK;
        IDSetSwitch(&ReverseSP, nullptr);
    }

    // #9 Encoder status
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid encoder response.");
        return false;
    }

    int encoderStatus = atoi(token);
    if (encoderStatus != IUFindOnSwitchIndex(&EncoderSP) && encoderStatus >= 0 && encoderStatus <= 1)
    {
        IUResetSwitch(&EncoderSP);
        EncoderS[encoderStatus].s = ISS_ON;
        EncoderSP.s = IPS_OK;
        IDSetSwitch(&EncoderSP, nullptr);
    }

    // #10 Backlash
    token = std::strtok(NULL, ":");

    if (token == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid encoder response.");
        return false;
    }

    int backlash = atoi(token);
    // If backlash is zero then compensation is disabled
    if (backlash == 0 && BacklashCompensationS[BACKLASH_ENABLED].s == ISS_ON)
    {
        BacklashCompensationS[BACKLASH_ENABLED].s = ISS_OFF;
        BacklashCompensationS[BACKLASH_DISABLED].s = ISS_ON;
        BacklashCompensationSP.s = IPS_IDLE;
        IDSetSwitch(&BacklashCompensationSP, nullptr);
    }
    else if (backlash > 0 && (BacklashCompensationS[BACKLASH_DISABLED].s == ISS_ON || backlash != BacklashN[0].value))
    {
        if (backlash != BacklashN[0].value)
        {
            BacklashN[0].value = backlash;
            BacklashNP.s = IPS_OK;
            IDSetNumber(&BacklashNP, NULL);
        }

        if (BacklashCompensationS[BACKLASH_DISABLED].s == ISS_ON)
        {
            BacklashCompensationS[BACKLASH_ENABLED].s = ISS_OFF;
            BacklashCompensationS[BACKLASH_DISABLED].s = ISS_ON;
            BacklashCompensationSP.s = IPS_IDLE;
            IDSetSwitch(&BacklashCompensationSP, nullptr);
        }
    }

    return true;
}

bool DMFC::setMaxSpeed(uint16_t speed)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "S:%d", speed);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Set Speed
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setMaxSpeed error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::setReverseEnabled(bool enable)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "N:%d", enable ? 1 : 0);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Reverse
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Reverse error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::setLedEnabled(bool enable)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "L:%d", enable ? 2 : 1);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Led
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Led error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::setEncodersEnabled(bool enable)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "E:%d", enable ? 0 : 1);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Encoders
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Encoder error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::setBacklash(uint16_t value)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "C:%d", value);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Backlash
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Backlash error: %s.", errstr);
        return false;
    }

    return true;
}

bool DMFC::setMotorType(uint8_t type)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "E:%d", (type == MOTOR_STEPPER) ? 1 : 0);
    cmd[strlen(cmd)] = 0xA;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    // Motor Type
    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Motor type error: %s.", errstr);
        return false;
    }

    return true;
}

IPState DMFC::MoveAbsFocuser(uint32_t targetTicks)
{
    targetPosition = targetTicks;

    bool rc = false;

    rc = move(targetPosition);

    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState DMFC::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double newPosition = 0;
    bool rc = false;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = move(newPosition);

    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s       = IPS_BUSY;

    return IPS_BUSY;
}

void DMFC::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(updatePeriodMS);
        return;
    }

    bool rc = updateFocusParams();

    if (rc)
    {
        if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
        {
            if (isMoving == false)
            {
                FocusAbsPosNP.s = IPS_OK;
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusAbsPosNP, nullptr);
                IDSetNumber(&FocusRelPosNP, nullptr);
                DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
            }
        }
    }

    SetTimer(updatePeriodMS);
}

bool DMFC::AbortFocuser()
{
    int nbytes_written;
    char cmd[2] = { 'H', 0xA };

    if (tty_write(PortFD, cmd, 2, &nbytes_written) == TTY_OK)
    {
        FocusAbsPosNP.s = IPS_IDLE;
        FocusRelPosNP.s = IPS_IDLE;
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        return true;
    }
    else
        return false;
}

bool DMFC::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &ReverseSP);
    IUSaveConfigNumber(fp, &BacklashNP);
    IUSaveConfigSwitch(fp, &BacklashCompensationSP);
    IUSaveConfigSwitch(fp, &EncoderSP);
    IUSaveConfigSwitch(fp, &MotorTypeSP);
    IUSaveConfigNumber(fp, &MaxSpeedNP);
    IUSaveConfigSwitch(fp, &LEDSP);

    return true;
}
