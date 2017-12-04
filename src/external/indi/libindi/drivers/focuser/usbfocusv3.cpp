/*
    USB Focus V3
    Copyright (C) 2016 G. Schmidt

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

#include "usbfocusv3.h"

#include "indicom.h"

#include <cmath>
#include <cstring>
#include <memory>

#include <termios.h>
#include <unistd.h>

#define USBFOCUSV3_TIMEOUT 3

#define POLLMS 500

#define SRTUS 25000

/***************************** Class USBFocusV3 *******************************/

std::unique_ptr<USBFocusV3> usbFocusV3(new USBFocusV3());

void ISGetProperties(const char *dev)
{
    usbFocusV3->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    usbFocusV3->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    usbFocusV3->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    usbFocusV3->ISNewNumber(dev, name, values, names, n);
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
    usbFocusV3->ISSnoopDevice(root);
}

USBFocusV3::USBFocusV3()
{
    // Can move in Absolute & Relative motions, can AbortFocuser motion, and has variable speed.
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT | FOCUSER_HAS_VARIABLE_SPEED);
}

bool USBFocusV3::initProperties()
{
    INDI::Focuser::initProperties();

    /*** init controller parameters ***/

    direction = 0;
    stepmode  = 1;
    speed     = 3;
    stepsdeg  = 20;
    tcomp_thr = 5;
    firmware  = 0;
    maxpos    = 65535;

    /*** init driver parameters ***/

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 3;
    FocusSpeedN[0].value = 2;

    /* Step Mode */
    IUFillSwitch(&StepModeS[0], "Half Step", "", ISS_ON);
    IUFillSwitch(&StepModeS[1], "Full Step", "", ISS_OFF);
    IUFillSwitchVector(&StepModeSP, StepModeS, 2, getDeviceName(), "Step Mode", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    /* Direction */
    IUFillSwitch(&RotDirS[0], "Standard rotation", "", ISS_ON);
    IUFillSwitch(&RotDirS[1], "Reverse rotation", "", ISS_OFF);
    IUFillSwitchVector(&RotDirSP, RotDirS, 2, getDeviceName(), "Rotation Mode", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    /* Focuser temperature */
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50., 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Maximum Position
    IUFillNumber(&MaxPositionN[0], "MAXPOSITION", "Maximum position", "%5.0f", 1., 65535., 0., 65535.);
    IUFillNumberVector(&MaxPositionNP, MaxPositionN, 1, getDeviceName(), "FOCUS_MAXPOSITION", "Max. Position",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // Temperature Settings
    IUFillNumber(&TemperatureSettingN[0], "Coefficient", "", "%3.0f", 0., 999., 1., 15.);
    IUFillNumber(&TemperatureSettingN[1], "Threshold", "", "%3.0f", 0., 999., 1., 10.);
    IUFillNumberVector(&TemperatureSettingNP, TemperatureSettingN, 2, getDeviceName(), "Temp. Settings", "",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    /* Temperature Compensation Sign */
    IUFillSwitch(&TempCompSignS[0], "negative", "", ISS_OFF);
    IUFillSwitch(&TempCompSignS[1], "positive", "", ISS_ON);
    IUFillSwitchVector(&TempCompSignSP, TempCompSignS, 2, getDeviceName(), "TComp. Sign", "", OPTIONS_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // Compensate for temperature
    IUFillSwitch(&TemperatureCompensateS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&TemperatureCompensateSP, TemperatureCompensateS, 2, getDeviceName(), "Temp. Comp.", "",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Reset
    IUFillSwitch(&ResetS[0], "Reset", "", ISS_OFF);
    IUFillSwitchVector(&ResetSP, ResetS, 1, getDeviceName(), "Reset", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Firmware version
    IUFillNumber(&FWversionN[0], "FIRMWARE", "Firmware Version", "%5.0f", 0., 65535., 1., 0.);
    IUFillNumberVector(&FWversionNP, FWversionN, 1, getDeviceName(), "FW_VERSION", "Firmware", OPTIONS_TAB, IP_RO, 0,
                       IPS_IDLE);

    /* Relative and absolute movement */
    FocusRelPosN[0].min   = 0.;
    FocusRelPosN[0].max   = float(maxpos);
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step  = 1;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = float(maxpos);
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1;

    addDebugControl();

    return true;
}

bool USBFocusV3::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);
        defineNumber(&MaxPositionNP);
        defineSwitch(&StepModeSP);
        defineSwitch(&RotDirSP);
        defineNumber(&MaxPositionNP);
        defineNumber(&TemperatureSettingNP);
        defineSwitch(&TempCompSignSP);
        defineSwitch(&TemperatureCompensateSP);
        defineSwitch(&ResetSP);
        defineNumber(&FWversionNP);

        GetFocusParams();

        loadConfig(true);

        DEBUG(INDI::Logger::DBG_SESSION, "USB Focus V3 paramaters updated, focuser ready for use.");
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(MaxPositionNP.name);
        deleteProperty(StepModeSP.name);
        deleteProperty(RotDirSP.name);
        deleteProperty(TemperatureSettingNP.name);
        deleteProperty(TempCompSignSP.name);
        deleteProperty(TemperatureCompensateSP.name);
        deleteProperty(ResetSP.name);
        deleteProperty(FWversionNP.name);
    }

    return true;
}

bool USBFocusV3::Handshake()
{
    if (Ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "USB Focus V3 is online. Getting focus parameters...");
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from USB Focus V3, please ensure USB Focus V3 controller "
                                     "is powered and the port is correct.");
    return false;
}

const char *USBFocusV3::getDefaultName()
{
    return "USBFocusV3";
}

bool USBFocusV3::Ack()
{
    char cmd[] = UFOCDEVID;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[UFORIDLEN + 1];

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "Error requesting focuser ID: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORIDLEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "Error reading focuser ID: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORIDLEN] = '\0';

    } while (oneMoreRead(resp, UFORIDLEN));

    if (strncmp(resp, UFOID, UFORIDLEN) == 0)
    {
        return true;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "USB Focus V3 not properly identified! Answer was: %s.", resp);
        return false;
    }
}

bool USBFocusV3::getControllerStatus()
{
    char cmd[] = UFOCREADPARAM;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[UFORSTLEN + 1];

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "getControllerStatus error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORSTLEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "getControllerStatus error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORSTLEN] = '\0';

    } while (oneMoreRead(resp, UFORSTLEN));

    sscanf(resp, "C=%u-%u-%u-%u-%u-%u-%u", &direction, &stepmode, &speed, &stepsdeg, &tcomp_thr, &firmware, &maxpos);
    return true;
}

bool USBFocusV3::updateStepMode()
{
    IUResetSwitch(&StepModeSP);

    if (stepmode == 1)
        StepModeS[0].s = ISS_ON;
    else if (stepmode == 0)
        StepModeS[1].s = ISS_ON;
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser step value (%d)", stepmode);
        return false;
    }

    return true;
}

bool USBFocusV3::updateRotDir()
{
    IUResetSwitch(&RotDirSP);

    if (direction == 0)
        RotDirS[0].s = ISS_ON;
    else if (direction == 1)
        RotDirS[1].s = ISS_ON;
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: rotation direction  (%d)", direction);
        return false;
    }

    return true;
}

bool USBFocusV3::updateTemperature()
{
    char cmd[] = UFOCREADTEMP;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[UFORTEMPLEN + 1];
    float temp;

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updateTemperature error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORTEMPLEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updateTemperature error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORTEMPLEN] = '\0';

    } while (oneMoreRead(resp, UFORTEMPLEN));

    rc = sscanf(resp, "T=%f", &temp);

    if (rc > 0)
    {
        TemperatureN[0].value = temp;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser temperature value (%s)", resp);
        return false;
    }

    return true;
}

bool USBFocusV3::updateFWversion()
{
    FWversionN[0].value = firmware;
    return true;
}

bool USBFocusV3::updatePosition()
{
    char cmd[] = UFOCREADPOS;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[UFORPOSLEN + 1];
    int pos = -1;

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORPOSLEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORPOSLEN] = '\0';

    } while (oneMoreRead(resp, UFORPOSLEN));

    rc = sscanf(resp, "P=%u", &pos);

    if (rc > 0)
    {
        FocusAbsPosN[0].value = pos;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser position value (%s)", resp);
        return false;
    }

    return true;
}

bool USBFocusV3::updateMaxPos()
{
    MaxPositionN[0].value = maxpos;
    FocusAbsPosN[0].max   = maxpos;
    return true;
}

bool USBFocusV3::updateTempCompSettings()
{
    TemperatureSettingN[0].value = stepsdeg;
    TemperatureSettingN[1].value = tcomp_thr;
    return true;
}

bool USBFocusV3::updateTempCompSign()
{
    char cmd[] = UFOCGETSIGN;

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[UFORSIGNLEN + 1];
    unsigned int sign;

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updateTempCompSign error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORSIGNLEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "updateTempCompSign error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORSIGNLEN] = '\0';

    } while (oneMoreRead(resp, UFORSIGNLEN));

    rc = sscanf(resp, "A=%u", &sign);

    if (rc > 0)
    {
        IUResetSwitch(&TempCompSignSP);

        if (sign == 1)
            TempCompSignS[0].s = ISS_ON;
        else if (sign == 0)
            TempCompSignS[1].s = ISS_ON;
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: temp. comp. sign  (%d)", sign);
            return false;
        }
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: temp. comp. sign value (%s)", resp);
        return false;
    }

    return true;
}

bool USBFocusV3::updateSpeed()
{
    int drvspeed;

    switch (speed)
    {
        case UFOPSPDAV:
            drvspeed = 3;
            break;
        case UFOPSPDSL:
            drvspeed = 2;
            break;
        case UFOPSPDUS:
            drvspeed = 1;
            break;
        default:
            drvspeed = 0;
            break;
    }

    if (drvspeed != 0)
    {
        currentSpeed         = drvspeed;
        FocusSpeedN[0].value = drvspeed;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser speed value (%d)", speed);
        return false;
    }

    return true;
}

bool USBFocusV3::setAutoTempCompThreshold(unsigned int thr)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCTLEN + 1];
    char resp[UFORDONELEN + 1];

    snprintf(cmd, UFOCTLEN + 1, "%s%03u", UFOCSETTCTHR, thr);

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, UFOCTLEN, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setAutoTempCompThreshold error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORDONELEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setAutoTempCompThreshold error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORDONELEN] = '\0';

        if (strncmp(resp, UFORSDONE, strlen(UFORSDONE)) == 0)
        {
            tcomp_thr = thr;
            return true;
        }

    } while (oneMoreRead(resp, UFORDONELEN));

    sprintf(errstr, "did not receive DONE.");
    DEBUGF(INDI::Logger::DBG_ERROR, "setAutoTempCompThreshold error: %s.", errstr);

    return false;
}

bool USBFocusV3::setTemperatureCoefficient(unsigned int coefficient)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCTLEN + 1];
    char resp[UFORDONELEN + 1];

    snprintf(cmd, UFOCTLEN + 1, "%s%03u", UFOCSETSTDEG, coefficient);

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, UFOCTLEN, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCoefficient error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        resp[UFORDONELEN] = '\0';

        if (strncmp(resp, UFORSDONE, strlen(UFORSDONE)) == 0)
        {
            stepsdeg = coefficient;
            return true;
        }

    } while (oneMoreRead(resp, UFORDONELEN));

    sprintf(errstr, "did not receive DONE.");
    DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCoefficient error: %s.", errstr);

    return false;
}

bool USBFocusV3::reset()
{
    char cmd[] = UFOCRESET;

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "reset error: %s.", errstr);
        return false;
    }

    GetFocusParams();

    return true;
}

bool USBFocusV3::MoveFocuserUF(FocusDirection dir, unsigned int rticks)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCMLEN + 1];

    unsigned int ticks;

    if ((dir == FOCUS_INWARD) && (rticks > FocusAbsPosN[0].value))
    {
        ticks = FocusAbsPosN[0].value;
        DEBUGF(INDI::Logger::DBG_WARNING, "Requested %u ticks but inward movement has been limited to %u ticks", rticks,
               ticks);
    }
    else if ((dir == FOCUS_OUTWARD) && ((FocusAbsPosN[0].value + rticks) > MaxPositionN[0].value))
    {
        ticks = MaxPositionN[0].value - FocusAbsPosN[0].value;
        DEBUGF(INDI::Logger::DBG_WARNING, "Requested %u ticks but outward movement has been limited to %u ticks",
               rticks, ticks);
    }
    else
    {
        ticks = rticks;
    }

    if (dir == FOCUS_INWARD)
        snprintf(cmd, UFOCMLEN + 1, "%s%05u", UFOCMOVEIN, ticks);
    else
        snprintf(cmd, UFOCMLEN + 1, "%s%05u", UFOCMOVEOUT, ticks);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if ((rc = tty_write(PortFD, cmd, UFOCMLEN, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "MoveFocuserUF error: %s.", errstr);
        return false;
    }

    return true;
}

bool USBFocusV3::setStepMode(FocusStepMode mode)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCSMLEN + 1];

    if (mode == FOCUS_HALF_STEP)
        snprintf(cmd, UFOCSMLEN + 1, "%s", UFOCSETHSTEPS);
    else
        snprintf(cmd, UFOCSMLEN + 1, "%s", UFOCSETFSTEPS);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if ((rc = tty_write(PortFD, cmd, UFOCSMLEN, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setStepMode error: %s.", errstr);
        return false;
    }
    else
    {
        stepmode = mode;
    }

    return true;
}

bool USBFocusV3::setRotDir(unsigned int dir)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCDLEN + 1];

    if (dir == UFOPSDIR)
        snprintf(cmd, UFOCDLEN + 1, "%s", UFOCSETSDIR);
    else
        snprintf(cmd, UFOCDLEN + 1, "%s", UFOCSETRDIR);

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if ((rc = tty_write(PortFD, cmd, UFOCDLEN, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setRotDir error: %s.", errstr);
        return false;
    }
    else
    {
        direction = dir;
    }

    return true;
}

bool USBFocusV3::setMaxPos(unsigned int maxp)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCMMLEN + 1];
    char resp[UFORDONELEN + 1];

    if (maxp >= 1 && maxp <= 65535)
    {
        snprintf(cmd, UFOCMMLEN + 1, "%s%05u", UFOCSETMAX, maxp);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Focuser max. pos. value %d out of bounds", maxp);
        return false;
    }

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, UFOCMMLEN, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setMaxPos error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORDONELEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setMaxPos error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORDONELEN] = '\0';

        if (strncmp(resp, UFORSDONE, strlen(UFORSDONE)) == 0)
        {
            maxpos              = maxp;
            FocusAbsPosN[0].max = maxpos;
            return true;
        }

    } while (oneMoreRead(resp, UFORDONELEN));

    sprintf(errstr, "did not receive DONE.");
    DEBUGF(INDI::Logger::DBG_ERROR, "setMaxPos error: %s.", errstr);

    return false;
}

bool USBFocusV3::setSpeed(unsigned short drvspeed)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCSLEN + 1];
    char resp[UFORDONELEN + 1];

    unsigned int spd;

    switch (drvspeed)
    {
        case 3:
            spd = UFOPSPDAV;
            break;
        case 2:
            spd = UFOPSPDSL;
            break;
        case 1:
            spd = UFOPSPDUS;
            break;
        default:
            spd = UFOPSPDERR;
            break;
    }

    if (spd != UFOPSPDERR)
    {
        snprintf(cmd, UFOCSLEN + 1, "%s%03u", UFOCSETSPEED, spd);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Focuser speed value %d out of bounds", drvspeed);
        return false;
    }

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, UFOCSLEN, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setSpeed error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORDONELEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setSpeed error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORDONELEN] = '\0';

        if (strncmp(resp, UFORSDONE, strlen(UFORSDONE)) == 0)
        {
            speed = spd;
            return true;
        }

    } while (oneMoreRead(resp, UFORDONELEN));

    sprintf(errstr, "did not receive DONE.");
    DEBUGF(INDI::Logger::DBG_ERROR, "setSpeed error: %s.", errstr);

    return false;
}

bool USBFocusV3::setTemperatureCompensation(bool enable)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCTCLEN + 1];

    if (enable)
        snprintf(cmd, UFOCTCLEN + 1, "%s", UFOCSETAUTO);
    else
        snprintf(cmd, UFOCTCLEN + 1, "%s", UFOCSETMANU);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if ((rc = tty_write(PortFD, cmd, UFOCTCLEN, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCompensation error: %s.", errstr);
        return false;
    }

    return true;
}

bool USBFocusV3::setTempCompSign(unsigned int sign)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[UFOCTLEN + 1];
    char resp[UFORDONELEN + 1];

    snprintf(cmd, UFOCDLEN + 1, "%s%1u", UFOCSETSIGN, sign);

    do
    {
        tcflush(PortFD, TCIOFLUSH);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

        if ((rc = tty_write(PortFD, cmd, UFOCDLEN, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setTempCompSign error: %s.", errstr);
            return false;
        }

        usleep(SRTUS);

        if ((rc = tty_read(PortFD, resp, UFORDONELEN, USBFOCUSV3_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "setTempCompSign error: %s.", errstr);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s.", resp);

        usleep(SRTUS);

        resp[UFORDONELEN] = '\0';

        if (strncmp(resp, UFORSDONE, strlen(UFORSDONE)) == 0)
        {
            return true;
        }

    } while (oneMoreRead(resp, UFORDONELEN));

    sprintf(errstr, "did not receive DONE.");
    DEBUGF(INDI::Logger::DBG_ERROR, "setTempCompSign error: %s.", errstr);

    return false;
}

bool USBFocusV3::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(StepModeSP.name, name) == 0)
        {
            bool rc          = false;
            int current_mode = IUFindOnSwitchIndex(&StepModeSP);
            IUUpdateSwitch(&StepModeSP, states, names, n);
            int target_mode = IUFindOnSwitchIndex(&StepModeSP);
            if (current_mode == target_mode)
            {
                StepModeSP.s = IPS_OK;
                IDSetSwitch(&StepModeSP, nullptr);
            }

            if (target_mode == 0)
                rc = setStepMode(FOCUS_HALF_STEP);
            else
                rc = setStepMode(FOCUS_FULL_STEP);

            if (!rc)
            {
                IUResetSwitch(&StepModeSP);
                StepModeS[current_mode].s = ISS_ON;
                StepModeSP.s              = IPS_ALERT;
                IDSetSwitch(&StepModeSP, nullptr);
                return false;
            }

            StepModeSP.s = IPS_OK;
            IDSetSwitch(&StepModeSP, nullptr);
            return true;
        }

        if (strcmp(RotDirSP.name, name) == 0)
        {
            bool rc          = false;
            int current_mode = IUFindOnSwitchIndex(&RotDirSP);
            IUUpdateSwitch(&RotDirSP, states, names, n);
            int target_mode = IUFindOnSwitchIndex(&RotDirSP);
            if (current_mode == target_mode)
            {
                RotDirSP.s = IPS_OK;
                IDSetSwitch(&RotDirSP, nullptr);
            }

            if (target_mode == 0)
                rc = setRotDir(UFOPSDIR);
            else
                rc = setRotDir(UFOPRDIR);

            if (!rc)
            {
                IUResetSwitch(&RotDirSP);
                RotDirS[current_mode].s = ISS_ON;
                RotDirSP.s              = IPS_ALERT;
                IDSetSwitch(&RotDirSP, nullptr);
                return false;
            }

            RotDirSP.s = IPS_OK;
            IDSetSwitch(&RotDirSP, nullptr);
            return true;
        }

        if (strcmp(TemperatureCompensateSP.name, name) == 0)
        {
            int last_index = IUFindOnSwitchIndex(&TemperatureCompensateSP);
            IUUpdateSwitch(&TemperatureCompensateSP, states, names, n);

            bool rc = setTemperatureCompensation((TemperatureCompensateS[0].s == ISS_ON));

            if (!rc)
            {
                TemperatureCompensateSP.s = IPS_ALERT;
                IUResetSwitch(&TemperatureCompensateSP);
                TemperatureCompensateS[last_index].s = ISS_ON;
                IDSetSwitch(&TemperatureCompensateSP, nullptr);
                return false;
            }

            TemperatureCompensateSP.s = IPS_OK;
            IDSetSwitch(&TemperatureCompensateSP, nullptr);
            return true;
        }

        if (strcmp(TempCompSignSP.name, name) == 0)
        {
            bool rc          = false;
            int current_mode = IUFindOnSwitchIndex(&TempCompSignSP);
            IUUpdateSwitch(&TempCompSignSP, states, names, n);
            int target_mode = IUFindOnSwitchIndex(&TempCompSignSP);
            if (current_mode == target_mode)
            {
                TempCompSignSP.s = IPS_OK;
                IDSetSwitch(&TempCompSignSP, nullptr);
            }

            if (target_mode == 0)
                rc = setTempCompSign(UFOPNSIGN);
            else
                rc = setTempCompSign(UFOPPSIGN);

            if (!rc)
            {
                IUResetSwitch(&TempCompSignSP);
                TempCompSignS[current_mode].s = ISS_ON;
                TempCompSignSP.s              = IPS_ALERT;
                IDSetSwitch(&TempCompSignSP, nullptr);
                return false;
            }

            TempCompSignSP.s = IPS_OK;
            IDSetSwitch(&TempCompSignSP, nullptr);
            return true;
        }

        if (strcmp(ResetSP.name, name) == 0)
        {
            IUResetSwitch(&ResetSP);

            if (reset())
                ResetSP.s = IPS_OK;
            else
                ResetSP.s = IPS_ALERT;

            IDSetSwitch(&ResetSP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool USBFocusV3::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, MaxPositionNP.name) == 0)
        {
            IUUpdateNumber(&MaxPositionNP, values, names, n);
            if (!setMaxPos(MaxPositionN[0].value))
            {
                MaxPositionNP.s = IPS_ALERT;
                IDSetNumber(&MaxPositionNP, nullptr);
                return false;
            }
            MaxPositionNP.s = IPS_OK;
            IDSetNumber(&MaxPositionNP, nullptr);
            return true;
        }

        if (strcmp(name, TemperatureSettingNP.name) == 0)
        {
            IUUpdateNumber(&TemperatureSettingNP, values, names, n);
            if (!setAutoTempCompThreshold(TemperatureSettingN[1].value) ||
                !setTemperatureCoefficient(TemperatureSettingN[0].value))
            {
                TemperatureSettingNP.s = IPS_ALERT;
                IDSetNumber(&TemperatureSettingNP, nullptr);
                return false;
            }

            TemperatureSettingNP.s = IPS_OK;
            IDSetNumber(&TemperatureSettingNP, nullptr);
            return true;
        }

        if (strcmp(name, FWversionNP.name) == 0)
        {
            IUUpdateNumber(&FWversionNP, values, names, n);
            FWversionNP.s = IPS_OK;
            IDSetNumber(&FWversionNP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

bool USBFocusV3::oneMoreRead(char *response, unsigned int maxlen)
{
    if (strncmp(response, UFORSACK, std::min((unsigned int)strlen(UFORSACK), maxlen)) == 0)
    {
        return true;
    }

    if (strncmp(response, UFORSEQU, std::min((unsigned int)strlen(UFORSEQU), maxlen)) == 0)
    {
        return true;
    }

    if (strncmp(response, UFORSAUTO, std::min((unsigned int)strlen(UFORSAUTO), maxlen)) == 0)
    {
        return true;
    }

    if (strncmp(response, UFORSERR, std::min((unsigned int)strlen(UFORSERR), maxlen)) == 0)
    {
        return true;
    }

    if (strncmp(response, UFORSDONE, std::min((unsigned int)strlen(UFORSDONE), maxlen)) == 0)
    {
        return true;
    }

    if (strncmp(response, UFORSRESET, std::min((unsigned int)strlen(UFORSRESET), maxlen)) == 0)
    {
        return true;
    }

    return false;
}

void USBFocusV3::GetFocusParams()
{
    getControllerStatus();

    if (updatePosition())
        IDSetNumber(&FocusAbsPosNP, nullptr);

    if (updateMaxPos())
    {
        IDSetNumber(&MaxPositionNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    if (updateTemperature())
        IDSetNumber(&TemperatureNP, nullptr);

    if (updateTempCompSettings())
        IDSetNumber(&TemperatureSettingNP, nullptr);

    if (updateTempCompSign())
        IDSetSwitch(&TempCompSignSP, nullptr);

    if (updateSpeed())
        IDSetNumber(&FocusSpeedNP, nullptr);

    if (updateStepMode())
        IDSetSwitch(&StepModeSP, nullptr);

    if (updateRotDir())
        IDSetSwitch(&RotDirSP, nullptr);

    if (updateFWversion())
        IDSetNumber(&FWversionNP, nullptr);
}

bool USBFocusV3::SetFocuserSpeed(int speed)
{
    bool rc = false;

    rc = setSpeed(speed);

    if (!rc)
        return false;

    currentSpeed = speed;

    FocusSpeedNP.s = IPS_OK;
    IDSetNumber(&FocusSpeedNP, nullptr);

    return true;
}

IPState USBFocusV3::MoveAbsFocuser(uint32_t targetTicks)
{
    long ticks;

    targetPos = targetTicks;

    ticks = targetPos - FocusAbsPosN[0].value;

    bool rc = false;

    if (ticks < 0)
        rc = MoveFocuserUF(FOCUS_INWARD, (unsigned int)labs(ticks));
    else if (ticks > 0)
        rc = MoveFocuserUF(FOCUS_OUTWARD, (unsigned int)labs(ticks));

    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState USBFocusV3::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    bool rc = false;
    uint32_t aticks;

    if ((dir == FOCUS_INWARD) && (ticks > FocusAbsPosN[0].value))
    {
        aticks = FocusAbsPosN[0].value;
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Requested %u ticks but relative inward movement has been limited to %u ticks", ticks, aticks);
        ticks = aticks;
    }
    else if ((dir == FOCUS_OUTWARD) && ((FocusAbsPosN[0].value + ticks) > MaxPositionN[0].value))
    {
        aticks = MaxPositionN[0].value - FocusAbsPosN[0].value;
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Requested %u ticks but relative outward movement has been limited to %u ticks", ticks, aticks);
        ticks = aticks;
    }

    rc = MoveFocuserUF(dir, (unsigned int)ticks);

    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s       = IPS_BUSY;

    return IPS_BUSY;
}

void USBFocusV3::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(POLLMS);
        return;
    }

    bool rc = updatePosition();

    if (rc)
    {
        if (fabs(lastPos - FocusAbsPosN[0].value) > 5)
        {
            IDSetNumber(&FocusAbsPosNP, nullptr);
            lastPos = FocusAbsPosN[0].value;
        }
    }

    rc = updateTemperature();

    if (rc)
    {
        if (fabs(lastTemperature - TemperatureN[0].value) >= 0.5)
        {
            IDSetNumber(&TemperatureNP, nullptr);
            lastTemperature = TemperatureN[0].value;
        }
    }

    if (FocusTimerNP.s == IPS_BUSY)
    {
        float remaining = CalcTimeLeft(focusMoveStart, focusMoveRequest);

        if (remaining <= 0)
        {
            FocusTimerNP.s       = IPS_OK;
            FocusTimerN[0].value = 0;
            AbortFocuser();
        }
        else
            FocusTimerN[0].value = remaining * 1000.0;

        IDSetNumber(&FocusTimerNP, nullptr);
    }

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (fabs(targetPos - FocusAbsPosN[0].value) < 1)
        {
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            IDSetNumber(&FocusRelPosNP, nullptr);
            lastPos = FocusAbsPosN[0].value;
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
        }
    }

    SetTimer(POLLMS);
}

bool USBFocusV3::AbortFocuser()
{
    char cmd[] = UFOCABORT;
    int nbytes_written;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s.", cmd);

    if (tty_write(PortFD, cmd, strlen(cmd), &nbytes_written) == TTY_OK)
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

float USBFocusV3::CalcTimeLeft(timeval start, float req)
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince =
        (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = req - timesince;
    return timeleft;
}
