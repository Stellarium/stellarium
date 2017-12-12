/*
    NFocus
    Copyright (C) 2013 Felix Krämer (rigelsys@felix-kraemer.de)

    Based on the work for robofocus by
                  2006 Markus Wildi (markus.wildi@datacomm.ch)
                  2011 Jasem Mutlaq (mutlaqja@ikarustech.com)


    2017-05-31: Jasem Mutlaq
    + Removed obsolete properties and functions
    + Added proper Sync.
    + Using INDI::Logger whenever possible.
    + Removed timer-based motion.
    + Set Focuser capabilities on startup.
    + Removed duplicate properties.

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

#include "nfocus.h"

#include "indicom.h"

#include <cstring>
#include <memory>
#include <termios.h>

#define NF_MAX_CMD  8  /* cmd length */
#define NF_TIMEOUT  15 /* com timeout */
#define NF_STEP_RES 5  /* step res for a given time period */

#define NF_MAX_TRIES 3
#define NF_MAX_DELAY 100000

#define BACKLASH_READOUT    0
#define MAXTRAVEL_READOUT   99999
#define INOUTSCALAR_READOUT 1

#define currentSpeed            SpeedN[0].value
#define currentPosition         FocusAbsPosN[0].value
#define currentTemperature      TemperatureN[0].value
#define currentOnTime           SettingsN[0].value
#define currentOffTime          SettingsN[1].value
#define currentFastDelay        SettingsN[2].value
#define currentInOutScalar      InOutScalarN[0].value
#define currentRelativeMovement FocusRelPosN[0].value
#define currentAbsoluteMovement FocusAbsPosN[0].value
#define currentSetBacklash      SetBacklashN[0].value
#define currentMinPosition      MinMaxPositionN[0].value
#define currentMaxPosition      MinMaxPositionN[1].value
#define currentMaxTravel        MaxTravelN[0].value

std::unique_ptr<NFocus> nFocus(new NFocus());

void ISGetProperties(const char *dev)
{
    nFocus->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    nFocus->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    nFocus->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    nFocus->ISNewNumber(dev, name, values, names, n);
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
    nFocus->ISSnoopDevice(root);
}

NFocus::NFocus()
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);
}

bool NFocus::initProperties()
{
    INDI::Focuser::initProperties();

    // No speed for nfocus
    FocusSpeedN[0].min = FocusSpeedN[0].max = FocusSpeedN[0].value = 1;

    IUUpdateMinMax(&FocusSpeedNP);

    /* Focuser temperature */
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", 0, 65000., 0., 10000.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    /* Settings of the Nfocus */
    IUFillNumber(&SettingsN[0], "ON time", "ON waiting time", "%6.0f", 10., 250., 0., 73.);
    IUFillNumber(&SettingsN[1], "OFF time", "OFF waiting time", "%6.0f", 1., 250., 0., 15.);
    IUFillNumber(&SettingsN[2], "Fast Mode Delay", "Fast Mode Delay", "%6.0f", 0., 255., 0., 9.);
    IUFillNumberVector(&SettingsNP, SettingsN, 3, getDeviceName(), "FOCUS_SETTINGS", "Settings", OPTIONS_TAB, IP_RW, 0,
                       IPS_IDLE);

    /* tick scaling factor. Inward dir ticks times this factor is considered to
    be the same as outward dir tick numbers. This is to compensate  for DC motor
    incapabilities with regards to exact positioning at least a bit on the mass
    dependent path.... */
    IUFillNumber(&InOutScalarN[0], "In/Out Scalar", "In/Out Scalar", "%1.2f", 0., 2., 1., 1.0);
    IUFillNumberVector(&InOutScalarNP, InOutScalarN, 1, getDeviceName(), "FOCUS_DIRSCALAR", "Direction scaling factor",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    /* Nfocus should stay within these limits */
    IUFillNumber(&MinMaxPositionN[0], "MINPOS", "Minimum Tick", "%6.0f", -65000., 65000., 0., -65000.);
    IUFillNumber(&MinMaxPositionN[1], "MAXPOS", "Maximum Tick", "%6.0f", 1., 65000., 0., 65000.);
    IUFillNumberVector(&MinMaxPositionNP, MinMaxPositionN, 2, getDeviceName(), "FOCUS_MINMAXPOSITION", "Extrema",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    IUFillNumber(&MaxTravelN[0], "MAXTRAVEL", "Maximum travel", "%6.0f", 1., 64000., 0., 10000.);
    IUFillNumberVector(&MaxTravelNP, MaxTravelN, 1, getDeviceName(), "FOCUS_MAXTRAVEL", "Max. travel", OPTIONS_TAB,
                       IP_RW, 0, IPS_IDLE);

    /* Set Nfocus position register to this position */
    IUFillNumber(&SyncN[0], "FOCUS_SYNC_OFFSET", "Offset", "%.f", 0, 64000., 0., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    /* Relative and absolute movement */
    FocusRelPosN[0].min   = 0;
    FocusRelPosN[0].max   = MinMaxPositionN[1].value;
    FocusRelPosN[0].value = 1000;
    FocusRelPosN[0].step  = 100;

    FocusAbsPosN[0].min   = MinMaxPositionN[0].min;
    FocusAbsPosN[0].max   = MinMaxPositionN[0].max;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 10000;

    addAuxControls();

    return true;
}

bool NFocus::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);
        defineNumber(&SettingsNP);
        defineNumber(&InOutScalarNP);
        defineNumber(&MinMaxPositionNP);
        defineNumber(&MaxTravelNP);
        defineNumber(&SyncNP);

        if (GetFocusParams())
            DEBUG(INDI::Logger::DBG_SESSION, "NFocus is ready.");
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(SettingsNP.name);
        deleteProperty(InOutScalarNP.name);
        deleteProperty(MinMaxPositionNP.name);
        deleteProperty(MaxTravelNP.name);
        deleteProperty(SyncNP.name);
    }

    return true;
}

bool NFocus::Handshake()
{
    // TODO add an actual check here
    return true;
}

const char *NFocus::getDefaultName()
{
    return "NFocus";
}

int NFocus::SendCommand(char *rf_cmd)
{
    int nbytes_written = 0, err_code = 0;
    char nfocus_error[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", rf_cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);

        if ((err_code = tty_write(PortFD, rf_cmd, strlen(rf_cmd) + 1, &nbytes_written) != TTY_OK))
        {
            tty_error_msg(err_code, nfocus_error, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "TTY error detected: %s", nfocus_error);
            return -1;
        }
    }

    return 0;
}

int NFocus::SendRequest(char *rf_cmd)
{
    int nbytes_read = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", rf_cmd);

    SendCommand(rf_cmd);

    if (!isSimulation())
    {
        nbytes_read = ReadResponse(rf_cmd, strlen(rf_cmd), NF_TIMEOUT);

        if (nbytes_read < 1)
            return nbytes_read;

        rf_cmd[nbytes_read - 1] = 0;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", rf_cmd);

    return 0;
}

int NFocus::ReadResponse(char *buf, int nbytes, int timeout)
{
    char nfocus_error[MAXRBUF];
    int bytesRead      = 0;
    int totalBytesRead = 0;
    int err_code;

    while (totalBytesRead < nbytes)
    {
        if ((err_code = tty_read(PortFD, buf + totalBytesRead, nbytes - totalBytesRead, timeout, &bytesRead)) != TTY_OK)
        {
            tty_error_msg(err_code, nfocus_error, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "TTY error detected: %s", nfocus_error);
            return -1;
        }

        if (bytesRead < 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "TTY error detected: Bytes read < 0");
            return -1;
        }

        totalBytesRead += bytesRead;
    }

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", buf);

    /*
    if (isDebug())
    {
        fprintf(stderr, "READ : (%s,%d), %d\n", buf, 9, totalBytesRead) ;
        fprintf(stderr, "READ : ") ;
        for(int i = 0; i < 9; i++)
        {

            fprintf(stderr, "0x%2x ", (unsigned char)buf[i]) ;
        }
    }*/

    return 9;
}

int NFocus::updateNFTemperature(double *value)
{
    float temp;
    char rf_cmd[32];
    int ret_read_tmp;

    strncpy(rf_cmd, ":RT", 4);

    if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
        return ret_read_tmp;

    // Always set to 20C for simulation
    if (isSimulation())
        strncpy(rf_cmd, "200", 32);

    if (sscanf(rf_cmd, "%6f", &temp) == -888)
        return -1;

    *value = (double)temp / 10.;

    return 0;
}

int NFocus::updateNFInOutScalar(double *value)
{
    double temp = currentInOutScalar;

    *value = (double)temp;

    return temp;
}

int NFocus::updateNFMotorSettings(double *onTime, double *offTime, double *fastDelay)
{
    char rf_cmd[32];
    int ret_read_tmp;

    if (isSimulation())
        return 0;

    if ((*onTime >= 100) && (*onTime <= 250))
    {
        snprintf(rf_cmd, 32, ":CO%3d#", (int)*onTime);
    }
    else if ((*onTime >= 10) && (*onTime <= 99))
    {
        snprintf(rf_cmd, 32, ":CO0%2d#", (int)*onTime);
    }

    if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
        return ret_read_tmp;

    strncpy(rf_cmd, ":RO\0", 4);
    if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
        return ret_read_tmp;

    *onTime = (float)atof(rf_cmd);

    if ((*offTime >= 100) && (*offTime <= 250))
    {
        snprintf(rf_cmd, 32, ":CF%3d#", (int)*offTime);
    }
    else if ((*offTime >= 10) && (*offTime <= 99))
    {
        snprintf(rf_cmd, 32, ":CF0%2d#", (int)*offTime);
    }
    else if ((*offTime >= 1) && (*offTime <= 9))
    {
        snprintf(rf_cmd, 32, ":CF00%1d#", (int)*offTime);
    }

    if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
        return ret_read_tmp;

    strncpy(rf_cmd, ":RF\0", 4);
    if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
        return ret_read_tmp;

    *offTime = (float)atof(rf_cmd);

    if ((*fastDelay >= 1) && (*fastDelay <= 9))
    {
        snprintf(rf_cmd, 32, ":CS00%1d#", (int)*fastDelay);
    }

    if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
        return ret_read_tmp;

    strncpy(rf_cmd, ":RS\0", 4);
    if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
        return ret_read_tmp;

    *fastDelay = (float)atof(rf_cmd);

    return 0;
}

int NFocus::moveNFInward(const double *value)
{
    char rf_cmd[32];
    int ret_read_tmp;
    rf_cmd[0] = 0;

    int newval = (int)(currentInOutScalar * (*value));
    DEBUGF(INDI::Logger::DBG_DEBUG, "Moving %d real steps but virtually counting %.0f", newval, *value);

    if (!isSimulation())
    {
        do
        {
            if (newval > 999)
            {
                strncpy(rf_cmd, ":F01999#\0", 9);
                newval -= 999;
            }
            else if (newval > 99)
            {
                snprintf(rf_cmd, 32, ":F01%3d#", (int)newval);
                newval = 0;
            }
            else if (newval > 9)
            {
                snprintf(rf_cmd, 32, ":F010%2d#", (int)newval);
                newval = 0;
            }
            else
            {
                snprintf(rf_cmd, 32, ":F0100%1d#", (int)newval);
                newval = 0;
            }

            if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
                return ret_read_tmp;

            do
            {
                strncpy(rf_cmd, "S\0", 2);
                if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
                    return ret_read_tmp;
            } while (atoi(rf_cmd) != 0);
        } while (newval > 0);
    }

    currentPosition -= *value;
    return 0;
}

int NFocus::moveNFOutward(const double *value)
{
    char rf_cmd[32];
    int ret_read_tmp;

    rf_cmd[0] = 0;

    int newval = (int)(*value);

    if (!isSimulation())
    {
        do
        {
            if (newval > 999)
            {
                strncpy(rf_cmd, ":F11999#\0", 9);
                newval -= 999;
            }
            else if (newval > 99)
            {
                snprintf(rf_cmd, 32, ":F11%3d#", (int)newval);
                newval = 0;
            }
            else if (newval > 9)
            {
                snprintf(rf_cmd, 32, ":F110%2d#", (int)newval);
                newval = 0;
            }
            else
            {
                snprintf(rf_cmd, 32, ":F1100%1d#", (int)newval);
                newval = 0;
            }

            if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
                return ret_read_tmp;

            do
            {
                strncpy(rf_cmd, "S\0", 2);
                if ((ret_read_tmp = SendRequest(rf_cmd)) < 0)
                    return ret_read_tmp;
            } while (atoi(rf_cmd) != 0);
        } while (newval > 0);
    }

    currentPosition += *value;
    return 0;
}

int NFocus::setNFAbsolutePosition(const double *value)
{
    double newAbsPos = 0;
    int rc           = 0;

    if ((*value - currentPosition) >= 0)
    {
        newAbsPos = *value - currentPosition;
        rc        = moveNFOutward(&newAbsPos);
    }
    else if ((*value - currentPosition) < 0)
    {
        newAbsPos = currentPosition - *value;
        rc        = moveNFInward(&newAbsPos);
    }

    return rc;
}

int NFocus::setNFMaxPosition(double *value)
{
    float temp;
    char rf_cmd[32];
    char vl_tmp[6];
    int ret_read_tmp;
    char waste[1];

    if (isSimulation())
        return 0;

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
            snprintf(vl_tmp, 6, "%5d", (int)*value);
        }
        else if (*value > 999)
        {
            snprintf(vl_tmp, 6, "0%4d", (int)*value);
        }
        else if (*value > 99)
        {
            snprintf(vl_tmp, 6, "00%3d", (int)*value);
        }
        else if (*value > 9)
        {
            snprintf(vl_tmp, 6, "000%2d", (int)*value);
        }
        else
        {
            snprintf(vl_tmp, 6, "0000%1d", (int)*value);
        }
        rf_cmd[3] = vl_tmp[0];
        rf_cmd[4] = vl_tmp[1];
        rf_cmd[5] = vl_tmp[2];
        rf_cmd[6] = vl_tmp[3];
        rf_cmd[7] = vl_tmp[4];
        rf_cmd[8] = 0;
    }

    if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
        return ret_read_tmp;

    if (sscanf(rf_cmd, "FL%1c%5f", waste, &temp) < 1)
        return -1;

    *value = (double)temp;

    return 0;
}

int NFocus::syncNF(const double *value)
{
    char rf_cmd[32];
    char vl_tmp[6];
    int ret_read_tmp;

    if (isSimulation())
    {
        currentPosition = *value;
        return 0;
    }

    rf_cmd[0] = 'F';
    rf_cmd[1] = 'S';
    rf_cmd[2] = '0';

    if (*value > 9999)
    {
        snprintf(vl_tmp, 6, "%5d", (int)*value);
    }
    else if (*value > 999)
    {
        snprintf(vl_tmp, 6, "0%4d", (int)*value);
    }
    else if (*value > 99)
    {
        snprintf(vl_tmp, 6, "00%3d", (int)*value);
    }
    else if (*value > 9)
    {
        snprintf(vl_tmp, 6, "000%2d", (int)*value);
    }
    else
    {
        snprintf(vl_tmp, 6, "0000%1d", (int)*value);
    }
    rf_cmd[3] = vl_tmp[0];
    rf_cmd[4] = vl_tmp[1];
    rf_cmd[5] = vl_tmp[2];
    rf_cmd[6] = vl_tmp[3];
    rf_cmd[7] = vl_tmp[4];
    rf_cmd[8] = 0;

    if ((ret_read_tmp = SendCommand(rf_cmd)) < 0)
        return ret_read_tmp;

    return 0;
}

bool NFocus::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    int nset = 0, i = 0;

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, SettingsNP.name) == 0)
        {
            /* new speed */
            double new_onTime    = 0;
            double new_offTime   = 0;
            double new_fastDelay = 0;
            int ret              = -1;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the SettingsNP property */
                INumber *eqp = IUFindNumber(&SettingsNP, names[i]);

                /* If the number found is  (SettingsN[0]) then process it */
                if (eqp == &SettingsN[0])
                {
                    new_onTime = (values[i]);
                    nset += static_cast<int>(new_onTime >= 10 && new_onTime <= 250);
                }
                else if (eqp == &SettingsN[1])
                {
                    new_offTime = (values[i]);
                    nset += static_cast<int>(new_offTime >= 1 && new_offTime <= 250);
                }
                else if (eqp == &SettingsN[2])
                {
                    new_fastDelay = (values[i]);
                    nset += static_cast<int>(new_fastDelay >= 1 && new_fastDelay <= 9);
                }
            }

            /* Did we process the three numbers? */
            if (nset == 3)
            {
                if ((ret = updateNFMotorSettings(&new_onTime, &new_offTime, &new_fastDelay)) < 0)
                {
                    IDSetNumber(&SettingsNP, "Changing to new settings failed");
                    return false;
                }

                currentOnTime    = new_onTime;
                currentOffTime   = new_offTime;
                currentFastDelay = new_fastDelay;

                SettingsNP.s = IPS_OK;
                IDSetNumber(&SettingsNP, "Motor settings are now  %3.0f %3.0f %3.0f", currentOnTime, currentOffTime,
                            currentFastDelay);
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

        if (strcmp(name, InOutScalarNP.name) == 0)
        {
            double new_ioscalar = 0;

            for (nset = i = 0; i < n; i++)
            {
                /* Find numbers with the passed names in the InOutScalarNP property */
                INumber *eqp = IUFindNumber(&InOutScalarNP, names[i]);

                /* If the number found is SetBacklash (SetBacklashN[0]) then process it */
                if (eqp == &InOutScalarN[0])
                {
                    new_ioscalar = (values[i]);

                    /* limits */
                    nset += static_cast<int>(new_ioscalar >= 0 && new_ioscalar <= 2);
                }

                if (nset == 1)
                {
                    /* Set the nfocus state to BUSY */
                    InOutScalarNP.s = IPS_BUSY;
                    /* kraemerf
                    IDSetNumber(&InOutScalarNP, nullptr);

                        if(( ret= updateNFInOutScalar(&new_ioscalar)) < 0) {

                          InOutScalarNP.s = IPS_IDLE;
                          IDSetNumber(&InOutScalarNP, "Setting new in/out scalar failed.");

                          return false ;
                        }
                    */

                    currentInOutScalar = new_ioscalar;
                    InOutScalarNP.s    = IPS_OK;
                    IDSetNumber(&InOutScalarNP, "Direction Scalar is now  %1.2f", currentInOutScalar);
                    return true;
                }
                else
                {
                    InOutScalarNP.s = IPS_IDLE;
                    IDSetNumber(&InOutScalarNP, "Need exactly one parameter.");

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
                /* Set the nfocus state to BUSY */
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

                if ((ret = setNFMaxPosition(&new_maxt)) < 0)
                {
                    MaxTravelNP.s = IPS_IDLE;
                    IDSetNumber(&MaxTravelNP, "Changing to new maximum travel failed");
                    return false;
                }

                currentMaxTravel    = new_maxt;
                MaxTravelNP.s       = IPS_OK;
                FocusAbsPosN[0].max = currentMaxTravel;
                IUUpdateMinMax(&FocusAbsPosNP);
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

        // Sync
        if (strcmp(name, SyncNP.name) == 0)
        {
            double new_apos = values[0];
            int rc          = 0;
            if ((new_apos < currentMinPosition) || (new_apos > currentMaxPosition))
            {
                SyncNP.s = IPS_ALERT;
                IDSetNumber(&SyncNP, "Value out of limits  %5.0f", new_apos);
                return false;
            }

            if ((rc = syncNF(&new_apos)) < 0)
            {
                SyncNP.s = IPS_ALERT;
                IDSetNumber(&SyncNP, "Read out of the set position to %3d failed.", rc);
                return false;
            }

            DEBUGF(INDI::Logger::DBG_DEBUG, "Focuser sycned to %g ticks", new_apos);
            SyncN[0].value = new_apos;
            SyncNP.s       = IPS_OK;
            IDSetNumber(&SyncNP, nullptr);
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

int NFocus::getNFAbsolutePosition(double *value)
{
    *value = currentPosition;
    return 0;
}

bool NFocus::GetFocusParams()
{
    int ret = -1;

    currentInOutScalar = INOUTSCALAR_READOUT;
    if ((ret = updateNFInOutScalar(&currentInOutScalar)) < 0)
    {
        InOutScalarNP.s = IPS_ALERT;
        IDSetNumber(&InOutScalarNP, "Unknown error while reading  Nfocus direction tick scalar");
        return false;
    }

    InOutScalarNP.s = IPS_OK;
    IDSetNumber(&InOutScalarNP, nullptr);

    if ((ret = updateNFTemperature(&currentTemperature)) < 0)
    {
        TemperatureNP.s = IPS_ALERT;
        IDSetNumber(&TemperatureNP, "Unknown error while reading  Nfocus temperature");
        return false;
    }

    TemperatureNP.s = IPS_OK;
    IDSetNumber(&TemperatureNP, nullptr);

    currentOnTime = currentOffTime = currentFastDelay = 0;

    if ((ret = updateNFMotorSettings(&currentOnTime, &currentOffTime, &currentFastDelay)) < 0)
    {
        SettingsNP.s = IPS_ALERT;
        IDSetNumber(&SettingsNP, "Unknown error while reading  Nfocus motor settings");
        return false;
    }

    SettingsNP.s = IPS_OK;
    IDSetNumber(&SettingsNP, nullptr);

    currentMaxTravel = MAXTRAVEL_READOUT;
    if ((ret = setNFMaxPosition(&currentMaxTravel)) < 0)
    {
        MaxTravelNP.s = IPS_ALERT;
        IDSetNumber(&MaxTravelNP, "Unknown error while reading  Nfocus maximum travel");
        return false;
    }
    MaxTravelNP.s = IPS_OK;
    IDSetNumber(&MaxTravelNP, nullptr);

    return true;
}

IPState NFocus::MoveAbsFocuser(uint32_t targetTicks)
{
    int ret         = -1;
    double new_apos = targetTicks;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Focuser is moving to requested position %d", targetTicks);

    if ((ret = setNFAbsolutePosition(&new_apos)) < 0)
    {
        return IPS_ALERT;
    }

    return IPS_OK;
}

IPState NFocus::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double cur_rpos = 0;
    double new_rpos = 0;
    int ret         = 0;
    bool nset       = false;

    cur_rpos = new_rpos = ticks;
    /* CHECK 2006-01-26, limits are relative to the actual position */
    nset = new_rpos >= -0xffff && new_rpos <= 0xffff;

    if (nset)
    {
        if (dir == FOCUS_OUTWARD)
        {
            if ((currentPosition + new_rpos < currentMinPosition) || (currentPosition + new_rpos > currentMaxPosition))
            {
                IDMessage(getDeviceName(), "Value out of limits %5.0f", currentPosition + new_rpos);
                return IPS_ALERT;
            }
            ret = moveNFOutward(&new_rpos);
        }
        else
        {
            if ((currentPosition - new_rpos < currentMinPosition) || (currentPosition - new_rpos > currentMaxPosition))
            {
                IDMessage(getDeviceName(), "Value out of limits %5.0f", currentPosition - new_rpos);
                return IPS_ALERT;
            }
            ret = moveNFInward(&new_rpos);
        }

        if (ret < 0)
        {
            // We have to leave here, because new_rpos is not set
            return IPS_ALERT;
        }

        currentRelativeMovement = cur_rpos;
        return IPS_OK;
    }
    {
        IDMessage(getDeviceName(), "Value out of limits.");
        return IPS_ALERT;
    }
}

bool NFocus::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &SettingsNP);
    IUSaveConfigNumber(fp, &InOutScalarNP);
    return true;
}
