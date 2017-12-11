/*
    RoboFocus
    Copyright (C) 2006 Markus Wildi (markus.wildi@datacomm.ch)
                  2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "robofocus.h"

#include "indicom.h"

#include <memory>
#include <cstring>
#include <termios.h>

#define RF_MAX_CMD  9
#define RF_TIMEOUT  3

#define BACKLASH_READOUT  99999
#define MAXTRAVEL_READOUT 99999

#define currentSpeed            SpeedN[0].value
#define currentPosition         FocusAbsPosN[0].value
#define currentTemperature      TemperatureN[0].value
#define currentBacklash         SetBacklashN[0].value
#define currentDuty             SettingsN[0].value
#define currentDelay            SettingsN[1].value
#define currentTicks            SettingsN[2].value
#define currentRelativeMovement FocusRelPosN[0].value
#define currentAbsoluteMovement FocusAbsPosN[0].value
#define currentSetBacklash      SetBacklashN[0].value
#define currentMinPosition      MinMaxPositionN[0].value
#define currentMaxPosition      MinMaxPositionN[1].value
#define currentMaxTravel        MaxTravelN[0].value

#define POLLMS 1000

#define SETTINGS_TAB "Settings"

std::unique_ptr<RoboFocus> roboFocus(new RoboFocus());

void ISGetProperties(const char *dev)
{
    roboFocus->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    roboFocus->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    roboFocus->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    roboFocus->ISNewNumber(dev, name, values, names, n);
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
    roboFocus->ISSnoopDevice(root);
}

RoboFocus::RoboFocus()
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
}

bool RoboFocus::initProperties()
{
    INDI::Focuser::initProperties();

    /* Focuser temperature */
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", 0, 65000., 0., 10000.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    /* Settings of the Robofocus */
    IUFillNumber(&SettingsN[0], "Duty cycle", "Duty cycle", "%6.0f", 0., 255., 0., 1.0);
    IUFillNumber(&SettingsN[1], "Step Delay", "Step delay", "%6.0f", 0., 255., 0., 1.0);
    IUFillNumber(&SettingsN[2], "Motor Steps", "Motor steps per tick", "%6.0f", 0., 255., 0., 1.0);
    IUFillNumberVector(&SettingsNP, SettingsN, 3, getDeviceName(), "FOCUS_SETTINGS", "Settings", SETTINGS_TAB, IP_RW, 0,
                       IPS_IDLE);

    /* Power Switches of the Robofocus */
    IUFillSwitch(&PowerSwitchesS[0], "1", "Switch 1", ISS_OFF);
    IUFillSwitch(&PowerSwitchesS[1], "2", "Switch 2", ISS_OFF);
    IUFillSwitch(&PowerSwitchesS[2], "3", "Switch 3", ISS_OFF);
    IUFillSwitch(&PowerSwitchesS[3], "4", "Switch 4", ISS_ON);
    IUFillSwitchVector(&PowerSwitchesSP, PowerSwitchesS, 4, getDeviceName(), "SWTICHES", "Power", SETTINGS_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    /* Robofocus should stay within these limits */
    IUFillNumber(&MinMaxPositionN[0], "MINPOS", "Minimum Tick", "%6.0f", 1., 65000., 0., 100.);
    IUFillNumber(&MinMaxPositionN[1], "MAXPOS", "Maximum Tick", "%6.0f", 1., 65000., 0., 55000.);
    IUFillNumberVector(&MinMaxPositionNP, MinMaxPositionN, 2, getDeviceName(), "FOCUS_MINMAXPOSITION", "Extrema",
                       SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    IUFillNumber(&MaxTravelN[0], "MAXTRAVEL", "Maximum travel", "%6.0f", 1., 64000., 0., 10000.);
    IUFillNumberVector(&MaxTravelNP, MaxTravelN, 1, getDeviceName(), "FOCUS_MAXTRAVEL", "Max. travel", SETTINGS_TAB,
                       IP_RW, 0, IPS_IDLE);

    /* Set Robofocus position register to this position */
    IUFillNumber(&SetRegisterPositionN[0], "SETPOS", "Position", "%6.0f", 0, 64000., 0., 0.);
    IUFillNumberVector(&SetRegisterPositionNP, SetRegisterPositionN, 1, getDeviceName(), "FOCUS_REGISTERPOSITION",
                       "Sync", SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    /* Backlash */
    IUFillNumber(&SetBacklashN[0], "SETBACKLASH", "Backlash", "%6.0f", -255., 255., 0., 0.);
    IUFillNumberVector(&SetBacklashNP, SetBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Set Register", SETTINGS_TAB,
                       IP_RW, 0, IPS_IDLE);

    /* Relative and absolute movement */
    FocusRelPosN[0].min   = -5000.;
    FocusRelPosN[0].max   = 5000.;
    FocusRelPosN[0].value = 100;
    FocusRelPosN[0].step  = 100;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = 64000.;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1000;

    simulatedTemperature = 600.0;
    simulatedPosition    = 20000;

    addDebugControl();
    addSimulationControl();

    return true;
}

bool RoboFocus::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);
        defineSwitch(&PowerSwitchesSP);
        defineNumber(&SettingsNP);
        defineNumber(&MinMaxPositionNP);
        defineNumber(&MaxTravelNP);
        defineNumber(&SetRegisterPositionNP);
        defineNumber(&SetBacklashNP);
        defineNumber(&FocusRelPosNP);
        defineNumber(&FocusAbsPosNP);

        GetFocusParams();

        DEBUG(INDI::Logger::DBG_DEBUG, "RoboFocus paramaters readout complete, focuser ready for use.");
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(SettingsNP.name);
        deleteProperty(PowerSwitchesSP.name);
        deleteProperty(MinMaxPositionNP.name);
        deleteProperty(MaxTravelNP.name);
        deleteProperty(SetRegisterPositionNP.name);
        deleteProperty(SetBacklashNP.name);
        deleteProperty(FocusRelPosNP.name);
        deleteProperty(FocusAbsPosNP.name);
    }

    return true;
}

bool RoboFocus::Handshake()
{
    char firmeware[] = "FV0000000";

    if (isSimulation())
    {
        timerID = SetTimer(POLLMS);
        DEBUG(INDI::Logger::DBG_SESSION, "Simulated Robofocus is online. Getting focus parameters...");
        FocusAbsPosN[0].value = simulatedPosition;
        updateRFFirmware(firmeware);
        return true;
    }

    if ((updateRFFirmware(firmeware)) < 0)
    {
        /* This would be the end*/
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus firmware.");
        return false;
    }

    return true;
}

const char *RoboFocus::getDefaultName()
{
    return "RoboFocus";
}

unsigned char RoboFocus::CheckSum(char *rf_cmd)
{
    char substr[255];
    unsigned char val = 0;

    for (int i = 0; i < 8; i++)
        substr[i] = rf_cmd[i];

    val = CalculateSum(substr);

    if (val != (unsigned char)rf_cmd[8])
        DEBUGF(INDI::Logger::DBG_WARNING, "Checksum: Wrong (%s,%ld), %x != %x", rf_cmd, strlen(rf_cmd), val,
               (unsigned char)rf_cmd[8]);

    return val;
}

unsigned char RoboFocus::CalculateSum(const char *rf_cmd)
{
    unsigned char val = 0;

    for (int i = 0; i < 8; i++)
        val = val + (unsigned char)rf_cmd[i];

    return val % 256;
}

int RoboFocus::SendCommand(char *rf_cmd)
{
    int nbytes_written = 0, err_code = 0;
    char rf_cmd_cks[32], robofocus_error[MAXRBUF];

    unsigned char val = 0;

    val = CalculateSum(rf_cmd);

    for (int i = 0; i < 8; i++)
        rf_cmd_cks[i] = rf_cmd[i];

    rf_cmd_cks[8] = (unsigned char)val;
    rf_cmd_cks[9] = 0;

    if (isSimulation())
        return 0;

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)", rf_cmd_cks[0],
           rf_cmd_cks[1], rf_cmd_cks[2], rf_cmd_cks[3], rf_cmd_cks[4], rf_cmd_cks[5], rf_cmd_cks[6], rf_cmd_cks[7],
           rf_cmd_cks[8]);

    if ((err_code = tty_write(PortFD, rf_cmd_cks, RF_MAX_CMD, &nbytes_written) != TTY_OK))
    {
        tty_error_msg(err_code, robofocus_error, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "TTY error detected: %s", robofocus_error);
        return -1;
    }

    return nbytes_written;
}

int RoboFocus::ReadResponse(char *buf)
{
    char robofocus_error[MAXRBUF];
    char robofocus_char[1];
    int bytesRead = 0;
    int err_code;

    char motion         = 0;
    bool externalMotion = false;

    if (isSimulation())
        return RF_MAX_CMD;

    while (true)
    {
        if ((err_code = tty_read(PortFD, robofocus_char, 1, RF_TIMEOUT, &bytesRead)) != TTY_OK)
        {
            tty_error_msg(err_code, robofocus_error, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "TTY error detected: %s", robofocus_error);
            return -1;
        }

        switch (robofocus_char[0])
        {
            // Catch 'I'
            case 0x49:
                if (motion != 0x49)
                {
                    motion = 0x49;
                    DEBUG(INDI::Logger::DBG_SESSION, "Moving inward...");

                    if (FocusAbsPosNP.s != IPS_BUSY)
                    {
                        externalMotion  = true;
                        FocusAbsPosNP.s = IPS_BUSY;
                        IDSetNumber(&FocusAbsPosNP, nullptr);
                    }
                }
                //usleep(100000);
                break;

            // catch 'O'
            case 0x4F:
                if (motion != 0x4F)
                {
                    motion = 0x4F;
                    DEBUG(INDI::Logger::DBG_SESSION, "Moving outward...");

                    if (FocusAbsPosNP.s != IPS_BUSY)
                    {
                        externalMotion  = true;
                        FocusAbsPosNP.s = IPS_BUSY;
                        IDSetNumber(&FocusAbsPosNP, nullptr);
                    }
                }
                //usleep(100000);
                break;

            // Start of frame
            case 0x46:
                buf[0] = 0x46;
                // Read rest of frame
                if ((err_code = tty_read(PortFD, buf + 1, RF_MAX_CMD - 1, RF_TIMEOUT, &bytesRead)) != TTY_OK)
                {
                    tty_error_msg(err_code, robofocus_error, MAXRBUF);
                    DEBUGF(INDI::Logger::DBG_ERROR, "TTY error detected: %s", robofocus_error);
                    return -1;
                }

                if (motion != 0)
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Stopped.");

                    // If we set it busy due to external motion, let's set it to OK
                    if (externalMotion)
                    {
                        FocusAbsPosNP.s = IPS_OK;
                        IDSetNumber(&FocusAbsPosNP, nullptr);
                    }
                }

                tcflush(PortFD, TCIOFLUSH);
                return (bytesRead + 1);
                break;

            default:
                break;
        }
    }

    return -1;
}

int RoboFocus::updateRFPosition(double *value)
{
    float temp;
    char rf_cmd[RF_MAX_CMD];
    int robofocus_rc;

    DEBUG(INDI::Logger::DBG_DEBUG, "Querying Position...");

    if (isSimulation())
    {
        *value = simulatedPosition;
        return 0;
    }

    strncpy(rf_cmd, "FG000000", 9);

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    if (sscanf(rf_cmd, "FD%6f", &temp) < 1)
        return -1;

    *value = (double)temp;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Position: %g", *value);

    return 0;
}

int RoboFocus::updateRFTemperature(double *value)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Update Temperature: %g", value);

    float temp;
    char rf_cmd[32];
    int robofocus_rc;

    strncpy(rf_cmd, "FT000000", 9);

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if (isSimulation())
        snprintf(rf_cmd, 32, "FT%6g", simulatedTemperature);
    else if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    if (sscanf(rf_cmd, "FT%6f", &temp) < 1)
        return -1;

    *value = (double)temp / 2. - 273.15;

    return 0;
}

int RoboFocus::updateRFBacklash(double *value)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Update Backlash: %g", value);

    float temp;
    char rf_cmd[32];
    char vl_tmp[4];
    int robofocus_rc;
    int sign = 0;

    if (isSimulation())
        return 0;

    if (*value == BACKLASH_READOUT)
    {
        strncpy(rf_cmd, "FB000000", 9);
    }
    else
    {
        rf_cmd[0] = 'F';
        rf_cmd[1] = 'B';

        if (*value > 0)
        {
            rf_cmd[2] = '3';
        }
        else
        {
            *value    = -*value;
            rf_cmd[2] = '2';
        }
        rf_cmd[3] = '0';
        rf_cmd[4] = '0';

        if (*value > 99)
        {
            sprintf(vl_tmp, "%3d", (int)*value);
        }
        else if (*value > 9)
        {
            sprintf(vl_tmp, "0%2d", (int)*value);
        }
        else
        {
            sprintf(vl_tmp, "00%1d", (int)*value);
        }
        rf_cmd[5] = vl_tmp[0];
        rf_cmd[6] = vl_tmp[1];
        rf_cmd[7] = vl_tmp[2];
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    if (sscanf(rf_cmd, "FB%1d%5f", &sign, &temp) < 1)
        return -1;

    *value = (double)temp;

    if ((sign == 2) && (*value > 0))
    {
        *value = -(*value);
    }

    return 0;
}

int RoboFocus::updateRFFirmware(char *rf_cmd)
{
    int robofocus_rc;

    DEBUG(INDI::Logger::DBG_DEBUG, "Querying RoboFocus Firmware...");

    strncpy(rf_cmd, "FV000000", 9);

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if (isSimulation())
        strncpy(rf_cmd, "SIM", 4);
    else if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    return 0;
}

int RoboFocus::updateRFMotorSettings(double *duty, double *delay, double *ticks)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Update Motor Settings: Duty (%g), Delay (%g), Ticks(%g)", *duty, *delay, *ticks);

    char rf_cmd[32];
    int robofocus_rc;

    if (isSimulation())
    {
        *duty  = 100;
        *delay = 0;
        *ticks = 0;
        return 0;
    }

    if ((*duty == 0) && (*delay == 0) && (*ticks == 0))
    {
        strncpy(rf_cmd, "FC000000", 9);
    }
    else
    {
        rf_cmd[0] = 'F';
        rf_cmd[1] = 'C';
        rf_cmd[2] = (char)*duty;
        rf_cmd[3] = (char)*delay;
        rf_cmd[4] = (char)*ticks;
        rf_cmd[5] = '0';
        rf_cmd[6] = '0';
        rf_cmd[7] = '0';
        rf_cmd[8] = 0;
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    *duty  = (float)rf_cmd[2];
    *delay = (float)rf_cmd[3];
    *ticks = (float)rf_cmd[4];

    return 0;
}

int RoboFocus::updateRFPositionRelativeInward(double value)
{
    char rf_cmd[32];
    int robofocus_rc;
    //float temp ;
    rf_cmd[0] = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Update Relative Position Inward: %g", value);

    if (isSimulation())
    {
        simulatedPosition += value;
        //value = simulatedPosition;
        return 0;
    }

    if (value > 9999)
    {
        sprintf(rf_cmd, "FI0%5d", (int)value);
    }
    else if (value > 999)
    {
        sprintf(rf_cmd, "FI00%4d", (int)value);
    }
    else if (value > 99)
    {
        sprintf(rf_cmd, "FI000%3d", (int)value);
    }
    else if (value > 9)
    {
        sprintf(rf_cmd, "FI0000%2d", (int)value);
    }
    else
    {
        sprintf(rf_cmd, "FI00000%1d", (int)value);
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    return 0;
}

int RoboFocus::updateRFPositionRelativeOutward(double value)
{
    char rf_cmd[32];
    int robofocus_rc;
    //float temp ;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Update Relative Position Outward: %g", value);

    if (isSimulation())
    {
        simulatedPosition -= value;
        //value = simulatedPosition;
        return 0;
    }

    rf_cmd[0] = 0;

    if (value > 9999)
    {
        sprintf(rf_cmd, "FO0%5d", (int)value);
    }
    else if (value > 999)
    {
        sprintf(rf_cmd, "FO00%4d", (int)value);
    }
    else if (value > 99)
    {
        sprintf(rf_cmd, "FO000%3d", (int)value);
    }
    else if (value > 9)
    {
        sprintf(rf_cmd, "FO0000%2d", (int)value);
    }
    else
    {
        sprintf(rf_cmd, "FO00000%1d", (int)value);
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    return 0;
}

int RoboFocus::updateRFPositionAbsolute(double value)
{
    char rf_cmd[32];
    int robofocus_rc;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Moving Absolute Position: %g", value);

    if (isSimulation())
    {
        simulatedPosition = value;
        return 0;
    }

    rf_cmd[0] = 0;

    if (value > 9999)
    {
        sprintf(rf_cmd, "FG0%5d", (int)value);
    }
    else if (value > 999)
    {
        sprintf(rf_cmd, "FG00%4d", (int)value);
    }
    else if (value > 99)
    {
        sprintf(rf_cmd, "FG000%3d", (int)value);
    }
    else if (value > 9)
    {
        sprintf(rf_cmd, "FG0000%2d", (int)value);
    }
    else
    {
        sprintf(rf_cmd, "FG00000%1d", (int)value);
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    return 0;
}

int RoboFocus::updateRFPowerSwitches(int s, int new_sn, int *cur_s1LL, int *cur_s2LR, int *cur_s3RL, int *cur_s4RR)
{
    INDI_UNUSED(s);
    char rf_cmd[32];
    char rf_cmd_tmp[32];
    int robofocus_rc;
    int i = 0;

    if (isSimulation())
    {
        return 0;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "Get switch status...");

    /* Get first the status */
    strncpy(rf_cmd_tmp, "FP000000", 9);

    if ((robofocus_rc = SendCommand(rf_cmd_tmp)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd_tmp)) < 0)
        return robofocus_rc;

    for (i = 0; i < 9; i++)
    {
        rf_cmd[i] = rf_cmd_tmp[i];
    }

    if (rf_cmd[new_sn + 4] == '2')
    {
        rf_cmd[new_sn + 4] = '1';
    }
    else
    {
        rf_cmd[new_sn + 4] = '2';
    }

    rf_cmd[8] = 0;

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    *cur_s1LL = *cur_s2LR = *cur_s3RL = *cur_s4RR = ISS_OFF;

    if (rf_cmd[4] == '2')
    {
        *cur_s1LL = ISS_ON;
    }

    if (rf_cmd[5] == '2')
    {
        *cur_s2LR = ISS_ON;
    }

    if (rf_cmd[6] == '2')
    {
        *cur_s3RL = ISS_ON;
    }

    if (rf_cmd[7] == '2')
    {
        *cur_s4RR = ISS_ON;
    }
    return 0;
}

int RoboFocus::updateRFMaxPosition(double *value)
{
    DEBUG(INDI::Logger::DBG_DEBUG, "Query max position...");

    float temp;
    char rf_cmd[32];
    char vl_tmp[6];
    int robofocus_rc;
    char waste[1];

    if (isSimulation())
    {
        return 0;
    }

    if (*value == MAXTRAVEL_READOUT)
    {
        strncpy(rf_cmd, "FL000000", 9);
    }
    else
    {
        rf_cmd[0] = 'F';
        rf_cmd[1] = 'L';
        rf_cmd[2] = '0';

        if (*value > 9999)
        {
            sprintf(vl_tmp, "%5d", (int)*value);
        }
        else if (*value > 999)
        {
            sprintf(vl_tmp, "0%4d", (int)*value);
        }
        else if (*value > 99)
        {
            sprintf(vl_tmp, "00%3d", (int)*value);
        }
        else if (*value > 9)
        {
            sprintf(vl_tmp, "000%2d", (int)*value);
        }
        else
        {
            sprintf(vl_tmp, "0000%1d", (int)*value);
        }
        rf_cmd[3] = vl_tmp[0];
        rf_cmd[4] = vl_tmp[1];
        rf_cmd[5] = vl_tmp[2];
        rf_cmd[6] = vl_tmp[3];
        rf_cmd[7] = vl_tmp[4];
        rf_cmd[8] = 0;
    }

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    if ((robofocus_rc = ReadResponse(rf_cmd)) < 0)
        return robofocus_rc;

    if (sscanf(rf_cmd, "FL%1c%5f", waste, &temp) < 1)
        return -1;

    *value = (double)temp;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Max position: %g", *value);

    return 0;
}

int RoboFocus::updateRFSetPosition(const double *value)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Set Max position: %g", *value);

    char rf_cmd[32];
    char vl_tmp[6];
    int robofocus_rc;

    if (isSimulation())
    {
        simulatedPosition = *value;
        return 0;
    }

    rf_cmd[0] = 'F';
    rf_cmd[1] = 'S';
    rf_cmd[2] = '0';

    if (*value > 9999)
    {
        sprintf(vl_tmp, "%5d", (int)*value);
    }
    else if (*value > 999)
    {
        sprintf(vl_tmp, "0%4d", (int)*value);
    }
    else if (*value > 99)
    {
        sprintf(vl_tmp, "00%3d", (int)*value);
    }
    else if (*value > 9)
    {
        sprintf(vl_tmp, "000%2d", (int)*value);
    }
    else
    {
        sprintf(vl_tmp, "0000%1d", (int)*value);
    }
    rf_cmd[3] = vl_tmp[0];
    rf_cmd[4] = vl_tmp[1];
    rf_cmd[5] = vl_tmp[2];
    rf_cmd[6] = vl_tmp[3];
    rf_cmd[7] = vl_tmp[4];
    rf_cmd[8] = 0;

    if ((robofocus_rc = SendCommand(rf_cmd)) < 0)
        return robofocus_rc;

    return 0;
}

bool RoboFocus::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PowerSwitchesSP.name) == 0)
        {
            int ret      = -1;
            int nset     = 0;
            int i        = 0;
            int new_s    = -1;
            int new_sn   = -1;
            int cur_s1LL = 0;
            int cur_s2LR = 0;
            int cur_s3RL = 0;
            int cur_s4RR = 0;

            ISwitch *sp;

            PowerSwitchesSP.s = IPS_BUSY;
            IDSetSwitch(&PowerSwitchesSP, nullptr);

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the SettingsNP property */
                sp = IUFindSwitch(&PowerSwitchesSP, names[i]);

                /* If the state found is  (PowerSwitchesS[0]) then process it */

                if (sp == &PowerSwitchesS[0])
                {
                    new_s  = (states[i]);
                    new_sn = 0;
                    nset++;
                }
                else if (sp == &PowerSwitchesS[1])
                {
                    new_s  = (states[i]);
                    new_sn = 1;
                    nset++;
                }
                else if (sp == &PowerSwitchesS[2])
                {
                    new_s  = (states[i]);
                    new_sn = 2;
                    nset++;
                }
                else if (sp == &PowerSwitchesS[3])
                {
                    new_s  = (states[i]);
                    new_sn = 3;
                    nset++;
                }
            }
            if (nset == 1)
            {
                cur_s1LL = cur_s2LR = cur_s3RL = cur_s4RR = 0;

                if ((ret = updateRFPowerSwitches(new_s, new_sn, &cur_s1LL, &cur_s2LR, &cur_s3RL, &cur_s4RR)) < 0)
                {
                    PowerSwitchesSP.s = IPS_ALERT;
                    IDSetSwitch(&PowerSwitchesSP, "Unknown error while reading  Robofocus power swicht settings");
                    return true;
                }
            }
            else
            {
                /* Set property state to idle */
                PowerSwitchesSP.s = IPS_IDLE;

                IDSetNumber(&SettingsNP, "Power switch settings absent or bogus.");
                return true;
            }
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool RoboFocus::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    int nset = 0, i = 0;

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, SettingsNP.name) == 0)
        {
            /* new speed */
            double new_duty  = 0;
            double new_delay = 0;
            double new_ticks = 0;
            int ret          = -1;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the SettingsNP property */
                INumber *eqp = IUFindNumber(&SettingsNP, names[i]);

                /* If the number found is  (SettingsN[0]) then process it */
                if (eqp == &SettingsN[0])
                {
                    new_duty = (values[i]);
                    nset += static_cast<int>(new_duty >= 0 && new_duty <= 255);
                }
                else if (eqp == &SettingsN[1])
                {
                    new_delay = (values[i]);
                    nset += static_cast<int>(new_delay >= 0 && new_delay <= 255);
                }
                else if (eqp == &SettingsN[2])
                {
                    new_ticks = (values[i]);
                    nset += static_cast<int>(new_ticks >= 0 && new_ticks <= 255);
                }
            }

            /* Did we process the three numbers? */
            if (nset == 3)
            {
                /* Set the robofocus state to BUSY */
                SettingsNP.s = IPS_BUSY;

                IDSetNumber(&SettingsNP, nullptr);

                if ((ret = updateRFMotorSettings(&new_duty, &new_delay, &new_ticks)) < 0)
                {
                    IDSetNumber(&SettingsNP, "Changing to new settings failed");
                    return false;
                }

                currentDuty  = new_duty;
                currentDelay = new_delay;
                currentTicks = new_ticks;

                SettingsNP.s = IPS_OK;
                IDSetNumber(&SettingsNP, "Motor settings are now  %3.0f %3.0f %3.0f", currentDuty, currentDelay,
                            currentTicks);
                return true;
            }
            else
            {
                /* Set property state to idle */
                SettingsNP.s = IPS_IDLE;

                IDSetNumber(&SettingsNP, "Settings absent or bogus.");
                return false;
            }
        }

        if (strcmp(name, SetBacklashNP.name) == 0)
        {
            double new_back = 0;
            int nset        = 0;
            int ret         = -1;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the SetBacklashNP property */
                INumber *eqp = IUFindNumber(&SetBacklashNP, names[i]);

                /* If the number found is SetBacklash (SetBacklashN[0]) then process it */
                if (eqp == &SetBacklashN[0])
                {
                    new_back = (values[i]);

                    /* limits */
                    nset += static_cast<int>(new_back >= -0xff && new_back <= 0xff);
                }

                if (nset == 1)
                {
                    /* Set the robofocus state to BUSY */
                    SetBacklashNP.s = IPS_BUSY;
                    IDSetNumber(&SetBacklashNP, nullptr);

                    if ((ret = updateRFBacklash(&new_back)) < 0)
                    {
                        SetBacklashNP.s = IPS_IDLE;
                        IDSetNumber(&SetBacklashNP, "Setting new backlash failed.");

                        return false;
                    }

                    currentSetBacklash = new_back;
                    SetBacklashNP.s    = IPS_OK;
                    IDSetNumber(&SetBacklashNP, "Backlash is now  %3.0f", currentSetBacklash);
                    return true;
                }
                else
                {
                    SetBacklashNP.s = IPS_IDLE;
                    IDSetNumber(&SetBacklashNP, "Need exactly one parameter.");

                    return false;
                }
            }
        }

        if (strcmp(name, MinMaxPositionNP.name) == 0)
        {
            /* new positions */
            double new_min = 0;
            double new_max = 0;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the MinMaxPositionNP property */
                INumber *mmpp = IUFindNumber(&MinMaxPositionNP, names[i]);

                /* If the number found is  (MinMaxPositionN[0]) then process it */
                if (mmpp == &MinMaxPositionN[0])
                {
                    new_min = (values[i]);
                    nset += static_cast<int>(new_min >= 1 && new_min <= 65000);
                }
                else if (mmpp == &MinMaxPositionN[1])
                {
                    new_max = (values[i]);
                    nset += static_cast<int>(new_max >= 1 && new_max <= 65000);
                }
            }

            /* Did we process the two numbers? */
            if (nset == 2)
            {
                /* Set the robofocus state to BUSY */
                MinMaxPositionNP.s = IPS_BUSY;

                currentMinPosition = new_min;
                currentMaxPosition = new_max;

                MinMaxPositionNP.s = IPS_OK;
                IDSetNumber(&MinMaxPositionNP, "Minimum and Maximum settings are now  %3.0f %3.0f", currentMinPosition,
                            currentMaxPosition);
                return true;
            }
            else
            {
                /* Set property state to idle */
                MinMaxPositionNP.s = IPS_IDLE;

                IDSetNumber(&MinMaxPositionNP, "Minimum and maximum limits absent or bogus.");

                return false;
            }
        }

        if (strcmp(name, MaxTravelNP.name) == 0)
        {
            double new_maxt = 0;
            int ret         = -1;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the MinMaxPositionNP property */
                INumber *mmpp = IUFindNumber(&MaxTravelNP, names[i]);

                /* If the number found is  (MaxTravelN[0]) then process it */
                if (mmpp == &MaxTravelN[0])
                {
                    new_maxt = (values[i]);
                    nset += static_cast<int>(new_maxt >= 1 && new_maxt <= 64000);
                }
            }
            /* Did we process the one number? */
            if (nset == 1)
            {
                IDSetNumber(&MinMaxPositionNP, nullptr);

                if ((ret = updateRFMaxPosition(&new_maxt)) < 0)
                {
                    MaxTravelNP.s = IPS_IDLE;
                    IDSetNumber(&MaxTravelNP, "Changing to new maximum travel failed");
                    return false;
                }

                currentMaxTravel = new_maxt;
                MaxTravelNP.s    = IPS_OK;
                IDSetNumber(&MaxTravelNP, "Maximum travel is now  %3.0f", currentMaxTravel);
                return true;
            }
            else
            {
                /* Set property state to idle */

                MaxTravelNP.s = IPS_IDLE;
                IDSetNumber(&MaxTravelNP, "Maximum travel absent or bogus.");

                return false;
            }
        }

        if (strcmp(name, SetRegisterPositionNP.name) == 0)
        {
            double new_apos = 0;
            int nset        = 0;
            int ret         = -1;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the SetRegisterPositionNP property */
                INumber *srpp = IUFindNumber(&SetRegisterPositionNP, names[i]);

                /* If the number found is SetRegisterPosition (SetRegisterPositionN[0]) then process it */
                if (srpp == &SetRegisterPositionN[0])
                {
                    new_apos = (values[i]);

                    /* limits are absolute */
                    nset += static_cast<int>(new_apos >= 0 && new_apos <= 64000);
                }

                if (nset == 1)
                {
                    if ((new_apos < currentMinPosition) || (new_apos > currentMaxPosition))
                    {
                        SetRegisterPositionNP.s = IPS_ALERT;
                        IDSetNumber(&SetRegisterPositionNP, "Value out of limits  %5.0f", new_apos);
                        return false;
                    }

                    /* Set the robofocus state to BUSY */
                    SetRegisterPositionNP.s = IPS_BUSY;
                    IDSetNumber(&SetRegisterPositionNP, nullptr);

                    if ((ret = updateRFSetPosition(&new_apos)) < 0)
                    {
                        SetRegisterPositionNP.s = IPS_OK;
                        IDSetNumber(&SetRegisterPositionNP,
                                    "Read out of the set position to %3d failed. Trying to recover the position", ret);

                        if ((ret = updateRFPosition(&currentPosition)) < 0)
                        {
                            FocusAbsPosNP.s = IPS_ALERT;
                            IDSetNumber(&FocusAbsPosNP, "Unknown error while reading  Robofocus position: %d", ret);

                            SetRegisterPositionNP.s = IPS_IDLE;
                            IDSetNumber(&SetRegisterPositionNP, "Relative movement failed.");
                        }

                        SetRegisterPositionNP.s = IPS_OK;
                        IDSetNumber(&SetRegisterPositionNP, nullptr);

                        FocusAbsPosNP.s = IPS_OK;
                        IDSetNumber(&FocusAbsPosNP, "Robofocus position recovered %5.0f", currentPosition);
                        DEBUG(INDI::Logger::DBG_DEBUG, "Robofocus position recovered resuming normal operation");
                        /* We have to leave here, because new_apos is not set */
                        return true;
                    }
                    currentPosition         = new_apos;
                    SetRegisterPositionNP.s = IPS_OK;
                    IDSetNumber(&SetRegisterPositionNP, "Robofocus register set to %5.0f", currentPosition);

                    FocusAbsPosNP.s = IPS_OK;
                    IDSetNumber(&FocusAbsPosNP, "Robofocus position is now %5.0f", currentPosition);

                    return true;
                }
                else
                {
                    SetRegisterPositionNP.s = IPS_IDLE;
                    IDSetNumber(&SetRegisterPositionNP, "Need exactly one parameter.");

                    return false;
                }

                if ((ret = updateRFPosition(&currentPosition)) < 0)
                {
                    FocusAbsPosNP.s = IPS_ALERT;
                    DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error while reading  Robofocus position: %d", ret);
                    IDSetNumber(&FocusAbsPosNP, nullptr);

                    return false;
                }

                SetRegisterPositionNP.s       = IPS_OK;
                SetRegisterPositionN[0].value = currentPosition;
                IDSetNumber(&SetRegisterPositionNP, "Robofocus has accepted new register setting");

                FocusAbsPosNP.s = IPS_OK;
                DEBUGF(INDI::Logger::DBG_SESSION, "Robofocus new position %5.0f", currentPosition);
                IDSetNumber(&FocusAbsPosNP, nullptr);

                return true;
            }
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

void RoboFocus::GetFocusParams()
{
    int ret      = -1;
    int cur_s1LL = 0;
    int cur_s2LR = 0;
    int cur_s3RL = 0;
    int cur_s4RR = 0;

    if ((ret = updateRFPosition(&currentPosition)) < 0)
    {
        FocusAbsPosNP.s = IPS_ALERT;
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error while reading  Robofocus position: %d", ret);
        IDSetNumber(&FocusAbsPosNP, nullptr);
        return;
    }

    FocusAbsPosNP.s = IPS_OK;
    IDSetNumber(&FocusAbsPosNP, nullptr);

    if ((ret = updateRFTemperature(&currentTemperature)) < 0)
    {
        TemperatureNP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus temperature.");
        IDSetNumber(&TemperatureNP, nullptr);
        return;
    }

    TemperatureNP.s = IPS_OK;
    IDSetNumber(&TemperatureNP, nullptr);

    currentBacklash = BACKLASH_READOUT;
    if ((ret = updateRFBacklash(&currentBacklash)) < 0)
    {
        SetBacklashNP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus backlash.");
        IDSetNumber(&SetBacklashNP, nullptr);
        return;
    }
    SetBacklashNP.s = IPS_OK;
    IDSetNumber(&SetBacklashNP, nullptr);

    currentDuty = currentDelay = currentTicks = 0;

    if ((ret = updateRFMotorSettings(&currentDuty, &currentDelay, &currentTicks)) < 0)
    {
        SettingsNP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus motor settings.");
        IDSetNumber(&SettingsNP, nullptr);
        return;
    }

    SettingsNP.s = IPS_OK;
    IDSetNumber(&SettingsNP, nullptr);

    if ((ret = updateRFPowerSwitches(-1, -1, &cur_s1LL, &cur_s2LR, &cur_s3RL, &cur_s4RR)) < 0)
    {
        PowerSwitchesSP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus power switch settings.");
        IDSetSwitch(&PowerSwitchesSP, nullptr);
        return;
    }

    PowerSwitchesS[0].s = PowerSwitchesS[1].s = PowerSwitchesS[2].s = PowerSwitchesS[3].s = ISS_OFF;

    if (cur_s1LL == ISS_ON)
    {
        PowerSwitchesS[0].s = ISS_ON;
    }
    if (cur_s2LR == ISS_ON)
    {
        PowerSwitchesS[1].s = ISS_ON;
    }
    if (cur_s3RL == ISS_ON)
    {
        PowerSwitchesS[2].s = ISS_ON;
    }
    if (cur_s4RR == ISS_ON)
    {
        PowerSwitchesS[3].s = ISS_ON;
    }
    PowerSwitchesSP.s = IPS_OK;
    IDSetSwitch(&PowerSwitchesSP, nullptr);

    currentMaxTravel = MAXTRAVEL_READOUT;
    if ((ret = updateRFMaxPosition(&currentMaxTravel)) < 0)
    {
        MaxTravelNP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Unknown error while reading Robofocus maximum travel");
        IDSetNumber(&MaxTravelNP, nullptr);
        return;
    }
    MaxTravelNP.s = IPS_OK;
    IDSetNumber(&MaxTravelNP, nullptr);
}

IPState RoboFocus::MoveAbsFocuser(uint32_t targetTicks)
{
    int ret   = -1;
    targetPos = targetTicks;

    if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Error, requested position is out of range.");
        return IPS_ALERT;
    }

    if ((ret = updateRFPositionAbsolute(targetPos)) < 0)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Read out of the absolute movement failed %3d", ret);
        return IPS_ALERT;
    }

    RemoveTimer(timerID);
    timerID = SetTimer(250);
    return IPS_BUSY;
}

IPState RoboFocus::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    return MoveAbsFocuser(FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1)));
}

bool RoboFocus::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &SettingsNP);
    IUSaveConfigNumber(fp, &SetBacklashNP);

    return INDI::Focuser::saveConfigItems(fp);
}

void RoboFocus::TimerHit()
{
    if (!isConnected())
        return;

    double prevPos = currentPosition;
    double newPos  = 0;

    if (FocusAbsPosNP.s == IPS_OK || FocusAbsPosNP.s == IPS_IDLE)
    {
        int rc = updateRFPosition(&newPos);
        if (rc >= 0)
        {
            currentPosition = newPos;
            if (prevPos != currentPosition)
                IDSetNumber(&FocusAbsPosNP, nullptr);
        }
    }
    else if (FocusAbsPosNP.s == IPS_BUSY)
    {
        float newPos            = 0;
        int nbytes_read         = 0;
        char rf_cmd[RF_MAX_CMD] = { 0 };

        //nbytes_read= ReadUntilComplete(rf_cmd, RF_TIMEOUT) ;

        nbytes_read = ReadResponse(rf_cmd);

        rf_cmd[nbytes_read - 1] = 0;

        if (nbytes_read != 9 || (sscanf(rf_cmd, "FD0%5f", &newPos) <= 0))
        {
            DEBUGF(INDI::Logger::DBG_WARNING,
                   "Bogus position: (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X) - Bytes read: %d",
                   rf_cmd[0], rf_cmd[1], rf_cmd[2], rf_cmd[3], rf_cmd[4], rf_cmd[5], rf_cmd[6], rf_cmd[7], rf_cmd[8],
                   nbytes_read);
            timerID = SetTimer(POLLMS);
            return;
        }
        else if (nbytes_read < 0)
        {
            FocusAbsPosNP.s = IPS_ALERT;
            DEBUG(INDI::Logger::DBG_ERROR, "Read error! Reconnect and try again.");
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return;
        }

        currentPosition = newPos;

        if (currentPosition == targetPos)
        {
            FocusAbsPosNP.s = IPS_OK;

            if (FocusRelPosNP.s == IPS_BUSY)
            {
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }
        }

        IDSetNumber(&FocusAbsPosNP, nullptr);
        if (FocusAbsPosNP.s == IPS_BUSY)
        {
            timerID = SetTimer(250);
            return;
        }
    }

    timerID = SetTimer(POLLMS);
}

bool RoboFocus::AbortFocuser()
{
    DEBUG(INDI::Logger::DBG_DEBUG, "Aborting focuser...");

    int nbytes_written;
    const char *buf = "\r";

    return tty_write(PortFD, buf, strlen(buf), &nbytes_written) == TTY_OK;
}
