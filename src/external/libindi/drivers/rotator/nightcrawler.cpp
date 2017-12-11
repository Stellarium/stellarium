/*
    NightCrawler NightCrawler Focuser & Rotator
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

#include "nightcrawler.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <termios.h>

#define NIGHTCRAWLER_TIMEOUT 3
#define NIGHTCRAWLER_THRESHOLD 0.1

#define NC_25_STEPS 374920
#define NC_30_STEPS 444080
#define NC_35_STEPS 505960

#define POLLMS 500
#define ROTATOR_TAB "Rotator"
#define AUX_TAB "Aux"
#define SETTINGS_TAB "Settings"

// Well, it is time I name something, even if simple, after Tommy, my loyal German Shephard companion.
// By the time of writing this, he is almost 4 years old. Live long and prosper, my good boy!
std::unique_ptr<NightCrawler> tommyGoodBoy(new NightCrawler());

void ISGetProperties(const char *dev)
{
    tommyGoodBoy->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    tommyGoodBoy->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int n)
{
    tommyGoodBoy->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    tommyGoodBoy->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                char *formats[], char *names[], int n)
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

void ISSnoopDevice (XMLEle *root)
{
    tommyGoodBoy->ISSnoopDevice(root);
}

NightCrawler::NightCrawler() : RotatorInterface(this)
{
    // Can move in Absolute & Relative motions, can AbortFocuser motion, and has variable speed.
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
    SetRotatorCapability(ROTATOR_CAN_ABORT | ROTATOR_CAN_HOME | ROTATOR_CAN_SYNC);
}

bool NightCrawler::initProperties()
{
    INDI::Focuser::initProperties();

    FocusSpeedN[0].min = 1;
    FocusSpeedN[0].max = 1;
    FocusSpeedN[0].value = 1;

    // Focus Sync
    IUFillNumber(&SyncFocusN[0], "FOCUS_SYNC_OFFSET", "Ticks", "%.f", 0, 100000., 0., 0.);
    IUFillNumberVector(&SyncFocusNP, SyncFocusN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE );

    // Temperature + Voltage Sensors
    IUFillNumber(&SensorN[SENSOR_TEMPERATURE], "TEMPERATURE", "Temperature (C)", "%.2f", -100, 100., 1., 0.);
    IUFillNumber(&SensorN[SENSOR_VOLTAGE], "VOLTAGE", "Voltage (V)", "%.2f", 0, 20., 1., 0.);
    IUFillNumberVector(&SensorNP, SensorN, 2, getDeviceName(), "SENSORS", "Sensors", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE );

    // Temperature offset
    IUFillNumber(&TemperatureOffsetN[0], "OFFSET", "Offset", "%.2f", -15, 15., 1., 0.);
    IUFillNumberVector(&TemperatureOffsetNP, TemperatureOffsetN, 1, getDeviceName(), "TEMPERATURE_OFFSET", "Temperature", MAIN_CONTROL_TAB, IP_WO, 0, IPS_IDLE );

    // Motor Step Delay
    IUFillNumber(&FocusStepDelayN[0], "FOCUS_STEP", "Value", "%.f", 7, 100., 1., 7.);
    IUFillNumberVector(&FocusStepDelayNP, FocusStepDelayN, 1, getDeviceName(), "FOCUS_STEP_DELAY", "Step Rate", SETTINGS_TAB, IP_RW, 0, IPS_IDLE );

    // Limit Switch
    IUFillLight(&LimitSwitchL[ROTATION_SWITCH], "ROTATION_SWITCH", "Rotation Home", IPS_OK);
    IUFillLight(&LimitSwitchL[OUT_SWITCH], "OUT_SWITCH", "Focus Out Limit", IPS_OK);
    IUFillLight(&LimitSwitchL[IN_SWITCH], "IN_SWITCH", "Focus In Limit", IPS_OK);
    IUFillLightVector(&LimitSwitchLP, LimitSwitchL, 3, getDeviceName(), "LIMIT_SWITCHES", "Limit Switch", SETTINGS_TAB, IPS_IDLE);

    // Home selection
    IUFillSwitch(&HomeSelectionS[MOTOR_FOCUS], "FOCUS", "Focuser", ISS_ON);
    IUFillSwitch(&HomeSelectionS[MOTOR_ROTATOR], "ROTATOR", "Rotator", ISS_ON);
    IUFillSwitch(&HomeSelectionS[MOTOR_AUX], "AUX", "Aux", ISS_OFF);
    IUFillSwitchVector(&HomeSelectionSP, HomeSelectionS, 3, getDeviceName(), "HOME_SELECTION", "Home Select", SETTINGS_TAB, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);

    // Home Find
    IUFillSwitch(&FindHomeS[0], "FIND", "Start", ISS_OFF);
    IUFillSwitchVector(&FindHomeSP, FindHomeS, 1, getDeviceName(), "FIND_HOME", "Home Find", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Encoders
    IUFillSwitch(&EncoderS[0], "ENABLED", "Enabled", ISS_ON);
    IUFillSwitch(&EncoderS[1], "DISABLED", "Disabled", ISS_OFF);
    IUFillSwitchVector(&EncoderSP, EncoderS, 2, getDeviceName(), "ENCODERS", "Encoders", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Brightness
    IUFillNumber(&BrightnessN[BRIGHTNESS_DISPLAY], "BRIGHTNESS_DISPLAY", "Display", "%.f", 0, 255., 10., 150.);
    IUFillNumber(&BrightnessN[BRIGHTNESS_SLEEP], "BRIGHTNESS_SLEEP", "Sleep", "%.f", 1, 255., 10., 16.);
    IUFillNumberVector(&BrightnessNP, BrightnessN, 2, getDeviceName(), "BRIGHTNESS", "Brightness", SETTINGS_TAB, IP_RW, 0, IPS_IDLE );

    //////////////////////////////////////////////////////
    // Rotator Properties
    /////////////////////////////////////////////////////

    INDI::RotatorInterface::initProperties(ROTATOR_TAB);

    // Rotator Ticks
    IUFillNumber(&RotatorAbsPosN[0], "ROTATOR_ABSOLUTE_POSITION", "Ticks", "%.f", 0., 100000., 1000., 0.);
    IUFillNumberVector(&RotatorAbsPosNP, RotatorAbsPosN, 1, getDeviceName(), "ABS_ROTATOR_POSITION", "Goto", ROTATOR_TAB, IP_RW, 0, IPS_IDLE );

    // Rotator Step Delay
    IUFillNumber(&RotatorStepDelayN[0], "ROTATOR_STEP", "Value", "%.f", 7, 100., 1., 7.);
    IUFillNumberVector(&RotatorStepDelayNP, RotatorStepDelayN, 1, getDeviceName(), "ROTATOR_STEP_DELAY", "Step Rate", ROTATOR_TAB, IP_RW, 0, IPS_IDLE );

    //////////////////////////////////////////////////////
    // Aux Properties
    /////////////////////////////////////////////////////

    // Aux GOTO
    IUFillNumber(&GotoAuxN[0], "AUX_ABSOLUTE_POSITION", "Ticks", "%.f", 0, 100000., 0., 0.);
    IUFillNumberVector(&GotoAuxNP, GotoAuxN, 1, getDeviceName(), "ABS_AUX_POSITION", "Goto", AUX_TAB, IP_RW, 0, IPS_IDLE );

    // Abort Aux
    IUFillSwitch(&AbortAuxS[0], "ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&AbortAuxSP, AbortAuxS, 1, getDeviceName(), "AUX_ABORT_MOTION", "Abort Motion", AUX_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Aux Sync
    IUFillNumber(&SyncAuxN[0], "AUX_SYNC_TICK", "Ticks", "%.f", 0, 100000., 0., 0.);
    IUFillNumberVector(&SyncAuxNP, SyncAuxN, 1, getDeviceName(), "SYNC_AUX", "Sync", AUX_TAB, IP_RW, 0, IPS_IDLE );

    // Aux Step Delay
    IUFillNumber(&AuxStepDelayN[0], "AUX_STEP", "Value", "%.f", 7, 100., 1., 7.);
    IUFillNumberVector(&AuxStepDelayNP, AuxStepDelayN, 1, getDeviceName(), "AUX_STEP_DELAY", "Step Rate", AUX_TAB, IP_RW, 0, IPS_IDLE );

    /* Relative and absolute movement */
    FocusRelPosN[0].min = 0.;
    FocusRelPosN[0].max = 50000.;
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step = 1000;

    FocusAbsPosN[0].min = 0.;
    FocusAbsPosN[0].max = 100000.;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step = 1000;

    addDebugControl();

    updatePeriodMS = POLLMS;

    serialConnection->setDefaultBaudRate(Connection::Serial::B_57600);

    return true;
}

bool NightCrawler::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        // Focus
        defineNumber(&SyncFocusNP);
        defineNumber(&SensorNP);
        defineNumber(&TemperatureOffsetNP);
        defineNumber(&FocusStepDelayNP);
        defineLight(&LimitSwitchLP);
        defineSwitch(&EncoderSP);
        defineNumber(&BrightnessNP);
        defineSwitch(&HomeSelectionSP);
        defineSwitch(&FindHomeSP);

        // Rotator
        INDI::RotatorInterface::updateProperties();
        defineNumber(&RotatorAbsPosNP);
        defineNumber(&RotatorStepDelayNP);        

        // Aux
        defineNumber(&GotoAuxNP);
        defineSwitch(&AbortAuxSP);
        defineNumber(&SyncAuxNP);
        defineNumber(&AuxStepDelayNP);        
    }
    else
    {
        // Focus
        deleteProperty(SyncFocusNP.name);
        deleteProperty(SensorNP.name);
        deleteProperty(TemperatureOffsetNP.name);
        deleteProperty(FocusStepDelayNP.name);
        deleteProperty(LimitSwitchLP.name);        
        deleteProperty(EncoderSP.name);
        deleteProperty(BrightnessNP.name);
        deleteProperty(FindHomeSP.name);
        deleteProperty(HomeSelectionSP.name);

        // Rotator
        INDI::RotatorInterface::updateProperties();
        deleteProperty(RotatorAbsPosNP.name);
        deleteProperty(RotatorStepDelayNP.name);

        // Aux
        deleteProperty(GotoAuxNP.name);
        deleteProperty(AbortAuxSP.name);
        deleteProperty(SyncAuxNP.name);
        deleteProperty(AuxStepDelayNP.name);
    }

    return true;
}

bool NightCrawler::Handshake()
{
    if (Ack())
        return true;

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from NightCrawler, please ensure NightCrawler controller is powered and the port is correct.");
    return false;
}

const char * NightCrawler::getDefaultName()
{
    return "NightCrawler";
}

bool NightCrawler::Ack()
{
    bool rcFirmware = getFirmware();
    bool rcType = getFocuserType();

    return (rcFirmware && rcType);
}

bool NightCrawler::getFirmware()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[64];

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, "PV#", 3, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "getFirmware error: %s.", errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, resp, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "getFirmware error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_SESSION, "Firmware %s", resp);

    return true;
}

bool NightCrawler::getFocuserType()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[64];

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, "PF#", 3, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "getFirmware error: %s.", errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, resp, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "getFirmware error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_SESSION, "Focuser Type %s", resp);

    if (strcmp(resp, "2.5 NC") == 0)
    {
        RotatorAbsPosN[0].min = -NC_25_STEPS;
        RotatorAbsPosN[0].max = NC_25_STEPS;
    }
    else if (strcmp(resp, "3.0 NC") == 0)
    {
        RotatorAbsPosN[0].min = -NC_30_STEPS;
        RotatorAbsPosN[0].max = NC_30_STEPS;
    }
    else
    {
        RotatorAbsPosN[0].min = -NC_35_STEPS;
        RotatorAbsPosN[0].max = NC_35_STEPS;
    }

    ticksPerDegree = RotatorAbsPosN[0].max / 360.0;

    return true;
}

bool NightCrawler::gotoMotor(MotorType type, int32_t position)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSN %d#", type+1, position);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return startMotor(type);
}

bool NightCrawler::getPosition(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};
    int position = -1e6;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dGP#", type+1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 8, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    position = atoi(res);

    if (position != -1e6)
    {
        if (type == MOTOR_FOCUS)
            FocusAbsPosN[0].value = position;
        else if (type == MOTOR_ROTATOR)
            RotatorAbsPosN[0].value = position;
        else
            GotoAuxN[0].value = position;

        return true;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Invalid Position! %d", position);
    return false;
}

bool NightCrawler::ISNewSwitch (const char * dev, const char * name, ISState * states, char * names[], int n)
{   
    if(strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, HomeSelectionSP.name) == 0)
        {
            bool atLeastOne = false;

            for (int i=0; i < n; i++)
            {
                if (states[i] == ISS_ON)
                {
                    atLeastOne = true;
                    break;
                }
            }

            if (!atLeastOne)
            {
                HomeSelectionSP.s = IPS_ALERT;
                DEBUG(INDI::Logger::DBG_ERROR, "At least one selection must be on.");
                IDSetSwitch(&HomeSelectionSP, nullptr);
                return false;
            }

            IUUpdateSwitch(&HomeSelectionSP, states, names, n);
            HomeSelectionSP.s = IPS_OK;
            IDSetSwitch(&HomeSelectionSP, nullptr);
            return true;
        }
        else if (strcmp(name, FindHomeSP.name) == 0)
        {
            uint8_t selection = 0;

            if (HomeSelectionS[MOTOR_FOCUS].s == ISS_ON)
                selection |= 0x01;
            if (HomeSelectionS[MOTOR_ROTATOR].s == ISS_ON)
                selection |= 0x02;
            if (HomeSelectionS[MOTOR_AUX].s == ISS_ON)
                selection |= 0x04;

            if (findHome(selection))
            {
                FindHomeSP.s = IPS_BUSY;
                FindHomeS[0].s = ISS_ON;
                DEBUG(INDI::Logger::DBG_WARNING, "Homing process can take up to 10 minutes. You cannot control the unit until the process is fully complete.");
            }
            else
            {
                FindHomeSP.s = IPS_ALERT;
                FindHomeS[0].s = ISS_OFF;
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to start homing process.");
            }

            IDSetSwitch(&FindHomeSP, nullptr);
            return true;
        }
        else if (strcmp(name, EncoderSP.name) == 0)
        {
            IUUpdateSwitch(&EncoderSP, states, names, n);
            EncoderSP.s = setEncodersEnabled(EncoderS[0].s == ISS_ON) ? IPS_OK : IPS_ALERT;
            if (EncoderSP.s == IPS_OK)
                DEBUGF(INDI::Logger::DBG_SESSION, "Encoders are %s", (EncoderS[0].s == ISS_ON) ? "ON" : "OFF");
            IDSetSwitch(&EncoderSP, nullptr);
            return true;
        }
        else if (strcmp(name, AbortAuxSP.name) == 0)
        {
            AbortAuxSP.s = stopMotor(MOTOR_AUX) ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&AbortAuxSP, nullptr);
            if (AbortAuxSP.s == IPS_OK)
            {
                if (GotoAuxNP.s != IPS_OK)
                {
                    GotoAuxNP.s = IPS_OK;
                    IDSetNumber(&GotoAuxNP, nullptr);
                }
            }
            return true;
        }
        else if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processSwitch(dev, name, states, names, n))
                return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool NightCrawler::ISNewNumber (const char * dev, const char * name, double values[], char * names[], int n)
{    
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, SyncFocusNP.name) == 0)
        {
            bool rc = syncMotor(MOTOR_FOCUS, static_cast<uint32_t>(values[0]));
            SyncFocusNP.s = rc ? IPS_OK : IPS_ALERT;
            if (rc)
                SyncFocusN[0].value = values[0];

            IDSetNumber(&SyncFocusNP, nullptr);
            return true;
        }       
        else if (strcmp(name, SyncAuxNP.name) == 0)
        {
            bool rc = syncMotor(MOTOR_AUX, static_cast<uint32_t>(values[0]));
            SyncAuxNP.s = rc ? IPS_OK : IPS_ALERT;
            if (rc)
                SyncAuxN[0].value = values[0];

            IDSetNumber(&SyncAuxNP, nullptr);
            return true;
        }
        else if (strcmp(name, TemperatureOffsetNP.name) == 0)
        {
            bool rc = setTemperatureOffset(values[0]);
            TemperatureOffsetNP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetNumber(&TemperatureOffsetNP, nullptr);
            return true;
        }
        else if (strcmp(name, FocusStepDelayNP.name) == 0)
        {
            bool rc = setStepDelay(MOTOR_FOCUS, static_cast<uint32_t>(values[0]));
            FocusStepDelayNP.s = rc ? IPS_OK : IPS_ALERT;
            if (rc)
                FocusStepDelayN[0].value = values[0];
            IDSetNumber(&FocusStepDelayNP, nullptr);
            return true;
        }
        else if (strcmp(name, RotatorStepDelayNP.name) == 0)
        {
            bool rc = setStepDelay(MOTOR_ROTATOR, static_cast<uint32_t>(values[0]));
            RotatorStepDelayNP.s = rc ? IPS_OK : IPS_ALERT;
            if (rc)
                RotatorStepDelayN[0].value = values[0];
            IDSetNumber(&RotatorStepDelayNP, nullptr);
            return true;
        }
        else if (strcmp(name, AuxStepDelayNP.name) == 0)
        {
            bool rc = setStepDelay(MOTOR_AUX, static_cast<uint32_t>(values[0]));
            AuxStepDelayNP.s = rc ? IPS_OK : IPS_ALERT;
            if (rc)
                AuxStepDelayN[0].value = values[0];
            IDSetNumber(&AuxStepDelayNP, nullptr);
            return true;
        }
        else if (strcmp(name, BrightnessNP.name) == 0)
        {
            IUUpdateNumber(&BrightnessNP, values, names, n);
            bool rcDisplay = setDisplayBrightness(static_cast<uint8_t>(BrightnessN[BRIGHTNESS_DISPLAY].value));
            bool rcSleep = setSleepBrightness(static_cast<uint8_t>(BrightnessN[BRIGHTNESS_SLEEP].value));
            if (rcDisplay && rcSleep)
                BrightnessNP.s = IPS_OK;
            else
                BrightnessNP.s = IPS_ALERT;

            IDSetNumber(&BrightnessNP, nullptr);
            return true;
        }        
        else if (strcmp(name, GotoAuxNP.name) == 0)
        {
           bool rc = gotoMotor(MOTOR_AUX, static_cast<int32_t>(values[0]));
           GotoAuxNP.s = rc ? IPS_BUSY : IPS_OK;
           IDSetNumber(&GotoAuxNP, nullptr);
           DEBUGF(INDI::Logger::DBG_SESSION, "Aux moving to %.f...", values[0]);
           return true;
        }
        else if (strcmp(name, RotatorAbsPosNP.name) == 0)
        {
            RotatorAbsPosNP.s = (gotoMotor(MOTOR_ROTATOR, static_cast<int32_t>(values[0])) ? IPS_BUSY : IPS_ALERT);
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            if (RotatorAbsPosNP.s == IPS_BUSY)
                DEBUGF(INDI::Logger::DBG_SESSION, "Rotator moving to %.f ticks...", values[0]);
            return true;
        }
        else if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processNumber(dev, name, values, names, n))
                return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState NightCrawler::MoveAbsFocuser(uint32_t targetTicks)
{
    targetPosition = targetTicks;

    bool rc = false;

    rc = gotoMotor(MOTOR_FOCUS, targetPosition);

    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState NightCrawler::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double newPosition = 0;
    bool rc = false;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = gotoMotor(MOTOR_FOCUS, newPosition);

    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

void NightCrawler::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(POLLMS);
        return;
    }

    bool rc = false;
    bool sensorsUpdated=false;

    // #1 If we're homing, we check if homing is complete as we cannot check for anything else
    if (FindHomeSP.s == IPS_BUSY || HomeRotatorSP.s == IPS_BUSY)
    {
        if (isHomingComplete())
        {
            HomeRotatorS[0].s = ISS_OFF;
            HomeRotatorSP.s = IPS_OK;
            IDSetSwitch(&HomeRotatorSP, nullptr);

            FindHomeS[0].s = ISS_OFF;
            FindHomeSP.s = IPS_OK;
            IDSetSwitch(&FindHomeSP, nullptr);

            DEBUG(INDI::Logger::DBG_SESSION, "Homing is complete.");
        }

        SetTimer(POLLMS);
        return;
    }

    // #2 Get Temperature
    rc = getTemperature();
    if (rc && fabs(SensorN[SENSOR_TEMPERATURE].value - lastTemperature) > NIGHTCRAWLER_THRESHOLD)
    {
        lastTemperature = SensorN[SENSOR_TEMPERATURE].value;
        sensorsUpdated = true;
    }

    // #3 Get Voltage
    rc = getVoltage();
    if (rc && fabs(SensorN[SENSOR_VOLTAGE].value - lastVoltage) > NIGHTCRAWLER_THRESHOLD)
    {
        lastVoltage = SensorN[SENSOR_VOLTAGE].value;
        sensorsUpdated = true;
    }

    if (sensorsUpdated)
        IDSetNumber(&SensorNP, nullptr);

    // #4 Get Limit Switch Status
    rc = getLimitSwitchStatus();
    if (rc && (LimitSwitchL[ROTATION_SWITCH].s != rotationLimit || LimitSwitchL[OUT_SWITCH].s != outSwitchLimit
               || LimitSwitchL[IN_SWITCH].s != inSwitchLimit))
    {
        rotationLimit = LimitSwitchL[ROTATION_SWITCH].s;
        outSwitchLimit = LimitSwitchL[OUT_SWITCH].s;
        inSwitchLimit = LimitSwitchL[IN_SWITCH].s;
        IDSetLight(&LimitSwitchLP, nullptr);
    }

    // #5 Focus Position & Status
    bool absFocusUpdated = false;

    if (FocusAbsPosNP.s == IPS_BUSY)
    {
        // Stopped moving
        if (!isMotorMoving(MOTOR_FOCUS))
        {
            FocusAbsPosNP.s = IPS_OK;
            if (FocusRelPosNP.s != IPS_OK)
            {
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }
            absFocusUpdated = true;
        }
    }
    rc = getPosition(MOTOR_FOCUS);
    if (rc && FocusAbsPosN[0].value != lastFocuserPosition)
    {
        lastFocuserPosition = FocusAbsPosN[0].value;
        absFocusUpdated = true;
    }
    if (absFocusUpdated)
        IDSetNumber(&FocusAbsPosNP, nullptr);

    // #6 Rotator Position & Status
    bool absRotatorUpdated = false;

    if (RotatorAbsPosNP.s == IPS_BUSY)
    {
        // Stopped moving
        if (!isMotorMoving(MOTOR_ROTATOR))
        {
            RotatorAbsPosNP.s = IPS_OK;
            GotoRotatorNP.s = IPS_OK;
            absRotatorUpdated = true;
            DEBUG(INDI::Logger::DBG_SESSION, "Rotator motion complete.");
        }
    }
    rc = getPosition(MOTOR_ROTATOR);
    if (rc && RotatorAbsPosN[0].value != lastRotatorPosition)
    {
        lastRotatorPosition = RotatorAbsPosN[0].value;
        GotoRotatorN[0].value = range360(RotatorAbsPosN[0].value / ticksPerDegree);
        absRotatorUpdated = true;
    }
    if (absRotatorUpdated)
    {
        IDSetNumber(&RotatorAbsPosNP, nullptr);
        IDSetNumber(&GotoRotatorNP, nullptr);
    }

    // #7 Aux Position & Status
    bool absAuxUpdated = false;

    if (GotoAuxNP.s == IPS_BUSY)
    {
        // Stopped moving
        if (!isMotorMoving(MOTOR_AUX))
        {
            GotoAuxNP.s = IPS_OK;
            absAuxUpdated = true;
            DEBUG(INDI::Logger::DBG_SESSION, "Aux motion complete.");
        }
    }
    rc = getPosition(MOTOR_AUX);
    if (rc && GotoAuxN[0].value != lastAuxPosition)
    {
        lastAuxPosition = GotoAuxN[0].value;
        absAuxUpdated = true;
    }
    if (absAuxUpdated)
        IDSetNumber(&GotoAuxNP, nullptr);

    SetTimer(POLLMS);
}

bool NightCrawler::AbortFocuser()
{
    return stopMotor(MOTOR_FOCUS);
}

bool NightCrawler::syncMotor(MotorType type, uint32_t position)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSP %d#", type+1, position);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::startMotor(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSM#", type+1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::stopMotor(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSQ#", type+1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::isMotorMoving(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dGM#", type+1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (strcmp("01", res) == 0);
}

bool NightCrawler::getTemperature()
{
    char cmd[16] = "GT#";
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    SensorN[SENSOR_TEMPERATURE].value = atoi(res) / 10.0;

    return true;
}

bool NightCrawler::getVoltage()
{
    char cmd[16] = "GV#";
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    SensorN[SENSOR_VOLTAGE].value = atoi(res) / 10.0;

    return true;
}

bool NightCrawler::setTemperatureOffset(double offset)
{
    char cmd[16] = {0};

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "Pt %03d#", static_cast<int>(offset*10));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    return true;
}

bool NightCrawler::getStepDelay(MotorType type)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSR#", type+1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    if (type == MOTOR_FOCUS)
        FocusStepDelayN[0].value = atoi(res);
    else if (type == MOTOR_ROTATOR)
        RotatorStepDelayN[0].value = atoi(res);
    else
        AuxStepDelayN[0].value = atoi(res);

    return true;
}

bool NightCrawler::setStepDelay(MotorType type, uint32_t delay)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "%dSR %03d#", type+1, delay);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::getLimitSwitchStatus()
{
    char cmd[16] = "GS#";
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    int value = atoi(res);

    LimitSwitchL[ROTATION_SWITCH].s = (value & 0x01) ? IPS_ALERT : IPS_OK;
    LimitSwitchL[OUT_SWITCH].s      = (value & 0x02) ? IPS_ALERT : IPS_OK;
    LimitSwitchL[IN_SWITCH].s       = (value & 0x04) ? IPS_ALERT : IPS_OK;

    return true;
}

bool NightCrawler::findHome(uint8_t motorTypes)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "SH %02d#", motorTypes);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::isHomingComplete()
{
    char res[16] = {0};
    int nbytes_read = 0, rc = -1;

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        // No error as we are waiting until controller returns "OK#"
        DEBUG(INDI::Logger::DBG_DEBUG, "Waiting for NightCrawler to complete homing...");
        return false;
    }

    res[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (strcmp("OK", res) == 0);
}

bool NightCrawler::setEncodersEnabled(bool enable)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "PE %s#", enable ? "01" : "00");

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read_section(PortFD, res, '#', NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return true;
}

bool NightCrawler::setDisplayBrightness(uint8_t value)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "PD %03d#", value);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::setSleepBrightness(uint8_t value)
{
    char cmd[16] = {0};
    char res[16] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, 16, "PL %03d#", value);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, NIGHTCRAWLER_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    res[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    return (res[0] == '#');
}

bool NightCrawler::saveConfigItems(FILE *fp)
{
    Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &BrightnessNP);
    IUSaveConfigNumber(fp, &FocusStepDelayNP);
    IUSaveConfigNumber(fp, &RotatorStepDelayNP);
    IUSaveConfigNumber(fp, &AuxStepDelayNP);

    return true;
}

IPState NightCrawler::HomeRotator()
{
    if (findHome(0x02))
    {
        FindHomeSP.s = IPS_BUSY;
        FindHomeS[0].s = ISS_ON;
        IDSetSwitch(&FindHomeSP, nullptr);
        DEBUG(INDI::Logger::DBG_WARNING, "Homing process can take up to 10 minutes. You cannot control the unit until the process is fully complete.");
        return IPS_BUSY;
    }
    else
    {
        FindHomeSP.s = IPS_ALERT;
        FindHomeS[0].s = ISS_OFF;
        IDSetSwitch(&FindHomeSP, nullptr);
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to start homing process.");
        return IPS_ALERT;
    }

    return IPS_ALERT;
}

IPState NightCrawler::MoveRotator(double angle)
{
    // Find shortest distance given target degree
    double a=angle;
    double b=GotoRotatorN[0].value;
    double d=fabs(a-b);
    double r=(d > 180) ? 360 - d : d;
    int sign = (a - b >= 0 && a - b <= 180) || (a - b <=-180 && a- b>= -360) ? 1 : -1;

    r *= sign;

    double newTarget = (r+b) * ticksPerDegree;

    if (newTarget < RotatorAbsPosN[0].min)
        newTarget -= RotatorAbsPosN[0].min;
    else if (newTarget > RotatorAbsPosN[0].max)
        newTarget -= RotatorAbsPosN[0].max;

    bool rc = gotoMotor(MOTOR_ROTATOR, static_cast<int32_t>(newTarget));

    if (rc)
    {
        RotatorAbsPosNP.s = IPS_BUSY;
        IDSetNumber(&RotatorAbsPosNP, nullptr);
        return IPS_BUSY;
    }

    return IPS_ALERT;
}

bool NightCrawler::SyncRotator(double angle)
{
    // Find shortest distance given target degree
    double a=angle;
    double b=GotoRotatorN[0].value;
    double d=fabs(a-b);
    double r=(d > 180) ? 360 - d : d;
    int sign = (a - b >= 0 && a - b <= 180) || (a - b <=-180 && a- b>= -360) ? 1 : -1;

    r *= sign;

    double newTarget = (r+b) * ticksPerDegree;

    if (newTarget < RotatorAbsPosN[0].min)
        newTarget -= RotatorAbsPosN[0].min;
    else if (newTarget > RotatorAbsPosN[0].max)
        newTarget -= RotatorAbsPosN[0].max;

    return syncMotor(MOTOR_ROTATOR, static_cast<int32_t>(newTarget));
}

bool NightCrawler::AbortRotator()
{
    bool rc = stopMotor(MOTOR_ROTATOR);
    if (rc && RotatorAbsPosNP.s != IPS_OK)
    {
        RotatorAbsPosNP.s = IPS_OK;
        IDSetNumber(&RotatorAbsPosNP, nullptr);
    }

    return rc;
}
