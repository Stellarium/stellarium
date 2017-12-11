/*
  Optec Gemini Focuser Rotator INDI driver
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

#include "gemini.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <termios.h>
#include <unistd.h>

#define GEMINI_MAX_RETRIES        1
#define GEMINI_TIMEOUT            3
#define GEMINI_MAXBUF             16
#define GEMINI_TEMPERATURE_FREQ   20 /* Update every 20 POLLMS cycles. For POLLMS 500ms = 10 seconds freq */
#define GEMINI_POSITION_THRESHOLD 5  /* Only send position updates to client if the diff exceeds 5 steps */

#define FOCUS_SETTINGS_TAB "Settings"
#define STATUS_TAB   "Status"
#define ROTATOR_TAB "Rotator"
#define HUB_TAB "Hub"

#define POLLMS 1000

std::unique_ptr<Gemini> geminiFR(new Gemini());

void ISGetProperties(const char *dev)
{
    geminiFR->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    geminiFR->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    geminiFR->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    geminiFR->ISNewNumber(dev, name, values, names, n);
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
    geminiFR->ISSnoopDevice(root);
}

/************************************************************************************
 *
* ***********************************************************************************/
Gemini::Gemini() : RotatorInterface(this)
{
    focusMoveRequest = 0;
    focuserSimPosition      = 0;

    // Can move in Absolute & Relative motions and can AbortFocuser motion.
    SetFocuserCapability(FOCUSER_CAN_ABORT | FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);

    // Rotator capabilities
    SetRotatorCapability(ROTATOR_CAN_ABORT | ROTATOR_CAN_HOME | ROTATOR_CAN_REVERSE);

    isFocuserAbsolute = true;
    isFocuserHoming   = false;

    focuserSimStatus[STATUS_MOVING]   = ISS_OFF;
    focuserSimStatus[STATUS_HOMING]   = ISS_OFF;
    focuserSimStatus[STATUS_HOMED]    = ISS_OFF;
    focuserSimStatus[STATUS_FFDETECT] = ISS_OFF;
    focuserSimStatus[STATUS_TMPPROBE] = ISS_ON;
    focuserSimStatus[STATUS_REMOTEIO] = ISS_ON;
    focuserSimStatus[STATUS_HNDCTRL]  = ISS_ON;
    focuserSimStatus[STATUS_REVERSE]  = ISS_OFF;

    DBG_FOCUS = INDI::Logger::getInstance().addDebugLevel("Verbose", "Verbose");
}

/************************************************************************************
 *
* ***********************************************************************************/
Gemini::~Gemini()
{
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::initProperties()
{
    INDI::Focuser::initProperties();

    ////////////////////////////////////////////////////////////
    // Focuser Properties
    ///////////////////////////////////////////////////////////

    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50, 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Enable/Disable temperature compensation
    IUFillSwitch(&TemperatureCompensateS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&TemperatureCompensateSP, TemperatureCompensateS, 2, getDeviceName(), "T. Compensation", "",
                       FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Enable/Disable temperature compensation on start
    IUFillSwitch(&TemperatureCompensateOnStartS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateOnStartS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&TemperatureCompensateOnStartSP, TemperatureCompensateOnStartS, 2, getDeviceName(),
                       "T. Compensation @Start", "", FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Temperature Coefficient
    IUFillNumber(&TemperatureCoeffN[0], "A", "", "%.f", -9999, 9999, 100., 0.);
    IUFillNumber(&TemperatureCoeffN[1], "B", "", "%.f", -9999, 9999, 100., 0.);
    IUFillNumber(&TemperatureCoeffN[2], "C", "", "%.f", -9999, 9999, 100., 0.);
    IUFillNumber(&TemperatureCoeffN[3], "D", "", "%.f", -9999, 9999, 100., 0.);
    IUFillNumber(&TemperatureCoeffN[4], "E", "", "%.f", -9999, 9999, 100., 0.);
    IUFillNumberVector(&TemperatureCoeffNP, TemperatureCoeffN, 5, getDeviceName(), "T. Coeff", "", FOCUS_SETTINGS_TAB,
                       IP_RW, 0, IPS_IDLE);

    // Enable/Disable Home on Start
    IUFillSwitch(&FocuserHomeOnStartS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&FocuserHomeOnStartS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&FocuserHomeOnStartSP, FocuserHomeOnStartS, 2, getDeviceName(), "FOCUSER_HOME_ON_START", "Home on Start",
                       FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Enable/Disable temperature Mode
    IUFillSwitch(&TemperatureCompensateModeS[0], "A", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateModeS[1], "B", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateModeS[2], "C", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateModeS[3], "D", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateModeS[4], "E", "", ISS_OFF);
    IUFillSwitchVector(&TemperatureCompensateModeSP, TemperatureCompensateModeS, 5, getDeviceName(), "Compensate Mode",
                       "", FOCUS_SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Enable/Disable backlash
    IUFillSwitch(&FocuserBacklashCompensationS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&FocuserBacklashCompensationS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&FocuserBacklashCompensationSP, FocuserBacklashCompensationS, 2, getDeviceName(), "FOCUSER_BACKLASH_COMPENSATION", "Backlash Compensation",
                       FOCUS_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Backlash Value
    IUFillNumber(&FocuserBacklashN[0], "Value", "", "%.f", 0, 99, 5., 0.);
    IUFillNumberVector(&FocuserBacklashNP, FocuserBacklashN, 1, getDeviceName(), "FOCUSER_BACKLASH", "Backlash", FOCUS_SETTINGS_TAB, IP_RW, 0,
                       IPS_IDLE);

    // Go to home/center
    IUFillSwitch(&FocuserGotoS[GOTO_CENTER], "Center", "", ISS_OFF);
    IUFillSwitch(&FocuserGotoS[GOTO_HOME], "Home", "", ISS_OFF);
    IUFillSwitchVector(&FocuserGotoSP, FocuserGotoS, 2, getDeviceName(), "FOCUSER_GOTO", "Goto", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Focus Status indicators
    IUFillLight(&FocuserStatusL[STATUS_MOVING], "Is Moving", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_HOMING], "Is Homing", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_HOMED], "Is Homed", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_FFDETECT], "FF Detect", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_TMPPROBE], "Tmp Probe", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_REMOTEIO], "Remote IO", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_HNDCTRL], "Hnd Ctrl", "", IPS_IDLE);
    IUFillLight(&FocuserStatusL[STATUS_REVERSE], "Reverse", "", IPS_IDLE);
    IUFillLightVector(&FocuserStatusLP, FocuserStatusL, 8, getDeviceName(), "FOCUSER_STATUS", "Focuser", STATUS_TAB, IPS_IDLE);

    ////////////////////////////////////////////////////////////
    // Rotator Properties
    ///////////////////////////////////////////////////////////

    // Enable/Disable Home on Start
    IUFillSwitch(&RotatorHomeOnStartS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&RotatorHomeOnStartS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&RotatorHomeOnStartSP, RotatorHomeOnStartS, 2, getDeviceName(), "ROTATOR_HOME_ON_START", "Home on Start",
                       ROTATOR_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Rotator Status indicators
    IUFillLight(&RotatorStatusL[STATUS_MOVING], "Is Moving", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_HOMING], "Is Homing", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_HOMED], "Is Homed", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_FFDETECT], "FF Detect", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_TMPPROBE], "Tmp Probe", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_REMOTEIO], "Remote IO", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_HNDCTRL], "Hnd Ctrl", "", IPS_IDLE);
    IUFillLight(&RotatorStatusL[STATUS_REVERSE], "Reverse", "", IPS_IDLE);
    IUFillLightVector(&RotatorStatusLP, RotatorStatusL, 8, getDeviceName(), "ROTATOR_STATUS", "Rotator", STATUS_TAB, IPS_IDLE);

    INDI::RotatorInterface::initProperties(ROTATOR_TAB);

    // Rotator Ticks
    IUFillNumber(&RotatorAbsPosN[0], "ROTATOR_ABSOLUTE_POSITION", "Ticks", "%.f", 0., 0., 0., 0.);
    IUFillNumberVector(&RotatorAbsPosNP, RotatorAbsPosN, 1, getDeviceName(), "ABS_ROTATOR_POSITION", "Goto", ROTATOR_TAB, IP_RW, 0, IPS_IDLE );
#if 0


    // Rotator Degree
    IUFillNumber(&RotatorAbsAngleN[0], "ANGLE", "Degrees", "%.2f", 0, 360., 10., 0.);
    IUFillNumberVector(&RotatorAbsAngleNP, RotatorAbsAngleN, 1, getDeviceName(), "ABS_ROTATOR_ANGLE", "Angle", ROTATOR_TAB, IP_RW, 0, IPS_IDLE );

    // Abort Rotator
    IUFillSwitch(&AbortRotatorS[0], "ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&AbortRotatorSP, AbortRotatorS, 1, getDeviceName(), "ROTATOR_ABORT_MOTION", "Abort Motion", ROTATOR_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Rotator Go to home/center
    IUFillSwitch(&RotatorGotoS[GOTO_CENTER], "Center", "", ISS_OFF);
    IUFillSwitch(&RotatorGotoS[GOTO_HOME], "Home", "", ISS_OFF);
    IUFillSwitchVector(&RotatorGotoSP, RotatorGotoS, 2, getDeviceName(), "ROTATOR_GOTO", "Goto", ROTATOR_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);
#endif

    // Enable/Disable backlash
    IUFillSwitch(&RotatorBacklashCompensationS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&RotatorBacklashCompensationS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&RotatorBacklashCompensationSP, RotatorBacklashCompensationS, 2, getDeviceName(), "ROTATOR_BACKLASH_COMPENSATION", "Backlash Compensation",
                       ROTATOR_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Backlash Value
    IUFillNumber(&RotatorBacklashN[0], "Value", "", "%.f", 0, 99, 5., 0.);
    IUFillNumberVector(&RotatorBacklashNP, RotatorBacklashN, 1, getDeviceName(), "ROTATOR_BACKLASH", "Backlash", ROTATOR_TAB, IP_RW, 0,
                       IPS_IDLE);

    ////////////////////////////////////////////////////////////
    // Hub Properties
    ///////////////////////////////////////////////////////////

    // Focus name configure in the HUB
    IUFillText(&HFocusNameT[DEVICE_FOCUSER], "FocusName", "Focuser name", "");
    IUFillText(&HFocusNameT[DEVICE_ROTATOR], "RotatorName", "Rotator name", "");
    IUFillTextVector(&HFocusNameTP, HFocusNameT, 2, getDeviceName(), "HUBNAMES", "HUB", HUB_TAB, IP_RW, 0,
                     IPS_IDLE);

    // Led intensity value
    IUFillNumber(&LedN[0], "Intensity", "", "%.f", 0, 100, 5., 0.);
    IUFillNumberVector(&LedNP, LedN, 1, getDeviceName(), "Led", "", HUB_TAB, IP_RW, 0, IPS_IDLE);

    // Reset to Factory setting
    IUFillSwitch(&ResetS[0], "Factory", "", ISS_OFF);
    IUFillSwitchVector(&ResetSP, ResetS, 1, getDeviceName(), "Reset", "", HUB_TAB, IP_RW, ISR_ATMOST1, 0,
                       IPS_IDLE);

    addAuxControls();

    serialConnection->setDefaultBaudRate(Connection::Serial::B_115200);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        // Focuser Properties
        defineNumber(&TemperatureNP);
        defineNumber(&TemperatureCoeffNP);
        defineSwitch(&TemperatureCompensateModeSP);
        defineSwitch(&TemperatureCompensateSP);
        defineSwitch(&TemperatureCompensateOnStartSP);
        defineSwitch(&FocuserBacklashCompensationSP);
        defineNumber(&FocuserBacklashNP);
        defineSwitch(&FocuserHomeOnStartSP);
        defineSwitch(&FocuserGotoSP);
        defineLight(&FocuserStatusLP);

        // Rotator Properties
        INDI::RotatorInterface::updateProperties();
        /*

        defineNumber(&RotatorAbsAngleNP);
        defineSwitch(&AbortRotatorSP);
        defineSwitch(&RotatorGotoSP);
        defineSwitch(&ReverseRotatorSP);
        */
        defineNumber(&RotatorAbsPosNP);
        defineSwitch(&RotatorBacklashCompensationSP);
        defineNumber(&RotatorBacklashNP);
        defineSwitch(&RotatorHomeOnStartSP);        
        defineLight(&RotatorStatusLP);

        // Hub Properties
        defineText(&HFocusNameTP);
        defineSwitch(&ResetSP);
        defineNumber(&LedNP);

        if (getFocusConfig() && getRotatorConfig())
            DEBUG(INDI::Logger::DBG_SESSION, "Gemini paramaters updated, rotating focuser ready for use.");
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to retrieve rotating focuser configuration settings...");
            return false;
        }
    }
    else
    {
        // Focuser Properties
        deleteProperty(TemperatureNP.name);
        deleteProperty(TemperatureCoeffNP.name);
        deleteProperty(TemperatureCompensateModeSP.name);
        deleteProperty(TemperatureCompensateSP.name);
        deleteProperty(TemperatureCompensateOnStartSP.name);
        deleteProperty(FocuserBacklashCompensationSP.name);
        deleteProperty(FocuserBacklashNP.name);
        deleteProperty(FocuserGotoSP.name);
        deleteProperty(FocuserHomeOnStartSP.name);
        deleteProperty(FocuserStatusLP.name);

        // Rotator Properties
        INDI::RotatorInterface::updateProperties();
        /*
        deleteProperty(RotatorAbsAngleNP.name);
        deleteProperty(AbortRotatorSP.name);
        deleteProperty(RotatorGotoSP.name);
        deleteProperty(ReverseRotatorSP.name);
        */

        deleteProperty(RotatorAbsPosNP.name);
        deleteProperty(RotatorBacklashCompensationSP.name);
        deleteProperty(RotatorBacklashNP.name);
        deleteProperty(RotatorHomeOnStartSP.name);

        deleteProperty(RotatorStatusLP.name);

        // Hub Properties
        deleteProperty(HFocusNameTP.name);
        deleteProperty(LedNP.name);
        deleteProperty(ResetSP.name);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::Handshake()
{
    if (ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Gemini is online. Getting focus parameters...");
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from Gemini, please ensure Gemini controller is "
                                     "powered and the port is correct.");
    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
const char *Gemini::getDefaultName()
{
    // Has to be overide by child instance
    return "Gemini Focusing Rotator";
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{    
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Temperature Compensation
        if (strcmp(TemperatureCompensateSP.name, name) == 0)
        {
            int prevIndex = IUFindOnSwitchIndex(&TemperatureCompensateSP);
            IUUpdateSwitch(&TemperatureCompensateSP, states, names, n);
            if (setTemperatureCompensation(TemperatureCompensateS[0].s == ISS_ON))
            {
                TemperatureCompensateSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&TemperatureCompensateSP);
                TemperatureCompensateSP.s           = IPS_ALERT;
                TemperatureCompensateS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&TemperatureCompensateSP, nullptr);
            return true;
        }

        // Temperature Compensation on Start
        if (!strcmp(TemperatureCompensateOnStartSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&TemperatureCompensateOnStartSP);
            IUUpdateSwitch(&TemperatureCompensateOnStartSP, states, names, n);
            if (setTemperatureCompensationOnStart(TemperatureCompensateOnStartS[0].s == ISS_ON))
            {
                TemperatureCompensateOnStartSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&TemperatureCompensateOnStartSP);
                TemperatureCompensateOnStartSP.s           = IPS_ALERT;
                TemperatureCompensateOnStartS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&TemperatureCompensateOnStartSP, nullptr);
            return true;
        }

        // Temperature Compensation Mode
        if (!strcmp(TemperatureCompensateModeSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&TemperatureCompensateModeSP);
            IUUpdateSwitch(&TemperatureCompensateModeSP, states, names, n);
            char mode = IUFindOnSwitchIndex(&TemperatureCompensateModeSP) + 'A';
            if (setTemperatureCompensationMode(mode))
            {
                TemperatureCompensateModeSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&TemperatureCompensateModeSP);
                TemperatureCompensateModeSP.s           = IPS_ALERT;
                TemperatureCompensateModeS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&TemperatureCompensateModeSP, nullptr);
            return true;
        }

        // Focuser Home on Start Enable/Disable
        if (!strcmp(FocuserHomeOnStartSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&FocuserHomeOnStartSP);
            IUUpdateSwitch(&FocuserHomeOnStartSP, states, names, n);
            if (homeOnStart(DEVICE_FOCUSER, FocuserHomeOnStartS[0].s == ISS_ON))
            {
                FocuserHomeOnStartSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&FocuserHomeOnStartSP);
                FocuserHomeOnStartSP.s           = IPS_ALERT;
                FocuserHomeOnStartS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&FocuserHomeOnStartSP, nullptr);
            return true;
        }

        // Rotator Home on Start Enable/Disable
        if (!strcmp(RotatorHomeOnStartSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&RotatorHomeOnStartSP);
            IUUpdateSwitch(&RotatorHomeOnStartSP, states, names, n);
            if (homeOnStart(DEVICE_ROTATOR, RotatorHomeOnStartS[0].s == ISS_ON))
            {
                RotatorHomeOnStartSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&RotatorHomeOnStartSP);
                RotatorHomeOnStartSP.s           = IPS_ALERT;
                RotatorHomeOnStartS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&RotatorHomeOnStartSP, nullptr);
            return true;
        }

        // Focuser Backlash enable/disable
        if (!strcmp(FocuserBacklashCompensationSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&FocuserBacklashCompensationSP);
            IUUpdateSwitch(&FocuserBacklashCompensationSP, states, names, n);
            if (setBacklashCompensation(DEVICE_FOCUSER, FocuserBacklashCompensationS[0].s == ISS_ON))
            {
                FocuserBacklashCompensationSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&FocuserBacklashCompensationSP);
                FocuserBacklashCompensationSP.s           = IPS_ALERT;
                FocuserBacklashCompensationS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&FocuserBacklashCompensationSP, nullptr);
            return true;
        }

        // Rotator Backlash enable/disable
        if (!strcmp(RotatorBacklashCompensationSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&RotatorBacklashCompensationSP);
            IUUpdateSwitch(&RotatorBacklashCompensationSP, states, names, n);
            if (setBacklashCompensation(DEVICE_ROTATOR, RotatorBacklashCompensationS[0].s == ISS_ON))
            {
                RotatorBacklashCompensationSP.s = IPS_OK;
            }
            else
            {
                IUResetSwitch(&RotatorBacklashCompensationSP);
                RotatorBacklashCompensationSP.s           = IPS_ALERT;
                RotatorBacklashCompensationS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&RotatorBacklashCompensationSP, nullptr);
            return true;
        }

        // Reset to Factory setting
        if (strcmp(ResetSP.name, name) == 0)
        {
            IUResetSwitch(&ResetSP);
            if (resetFactory())
                ResetSP.s = IPS_OK;
            else
                ResetSP.s = IPS_ALERT;

            IDSetSwitch(&ResetSP, nullptr);
            return true;
        }

        // Focser Go to home/center
        if (!strcmp(FocuserGotoSP.name, name))
        {
            IUUpdateSwitch(&FocuserGotoSP, states, names, n);

            if (FocuserGotoS[GOTO_HOME].s == ISS_ON)
            {
                if (home(DEVICE_FOCUSER))
                {
                    FocuserGotoSP.s = IPS_BUSY;
                    FocusAbsPosNP.s = IPS_BUSY;
                    IDSetNumber(&FocusAbsPosNP, nullptr);
                    isFocuserHoming = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Focuser moving to home position...");
                }
                else
                    FocuserGotoSP.s = IPS_ALERT;
            }
            else
            {
                if (center(DEVICE_FOCUSER))
                {
                    FocuserGotoSP.s = IPS_BUSY;
                    DEBUG(INDI::Logger::DBG_SESSION, "Focuser moving to center position...");
                    FocusAbsPosNP.s = IPS_BUSY;
                    IDSetNumber(&FocusAbsPosNP, nullptr);
                }
                else
                    FocuserGotoSP.s = IPS_ALERT;
            }

            IDSetSwitch(&FocuserGotoSP, nullptr);
            return true;
        }

        // Process all rotator properties
        if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processSwitch(dev, name, states, names, n))
                return true;
        }

        // Rotator Go to home/center
#if 0
        if (!strcmp(RotatorGotoSP.name, name))
        {
            IUUpdateSwitch(&RotatorGotoSP, states, names, n);

            if (RotatorGotoS[GOTO_HOME].s == ISS_ON)
            {
                if (home(DEVICE_ROTATOR))
                {
                    RotatorGotoSP.s = IPS_BUSY;
                    RotatorAbsPosNP.s = IPS_BUSY;
                    IDSetNumber(&RotatorAbsPosNP, nullptr);
                    isRotatorHoming = true;
                    DEBUG(INDI::Logger::DBG_SESSION, "Rotator moving to home position...");
                }
                else
                    RotatorGotoSP.s = IPS_ALERT;
            }
            else
            {
                if (center(DEVICE_ROTATOR))
                {
                    RotatorGotoSP.s = IPS_BUSY;
                    DEBUG(INDI::Logger::DBG_SESSION, "Rotator moving to center position...");
                    RotatorAbsPosNP.s = IPS_BUSY;
                    IDSetNumber(&RotatorAbsPosNP, nullptr);
                }
                else
                    RotatorGotoSP.s = IPS_ALERT;
            }

            IDSetSwitch(&RotatorGotoSP, nullptr);
            return true;
        }

        // Reverse Direction
        if (!strcmp(ReverseRotatorSP.name, name))
        {
            IUUpdateSwitch(&ReverseRotatorSP, states, names, n);

            if (reverseRotator(ReverseRotatorS[0].s == ISS_ON))
                ReverseRotatorSP.s = IPS_OK;
            else
                ReverseRotatorSP.s = IPS_ALERT;

            IDSetSwitch(&ReverseRotatorSP, nullptr);
            return true;
        }

        // Halt Rotator
        if (!strcmp(AbortRotatorSP.name, name))
        {
            if (halt(DEVICE_ROTATOR))
            {
                RotatorAbsPosNP.s = RotatorAbsAngleNP.s = RotatorGotoSP.s = IPS_IDLE;
                IDSetNumber(&RotatorAbsPosNP, nullptr);
                IDSetNumber(&RotatorAbsAngleNP, nullptr);
                IUResetSwitch(&RotatorGotoSP);
                IDSetSwitch(&RotatorGotoSP, nullptr);

                AbortRotatorSP.s = IPS_OK;
            }
            else
                AbortRotatorSP.s = IPS_ALERT;

            IDSetSwitch(&AbortRotatorSP, nullptr);
            return true;
        }
#endif
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Set device nickname to the HUB itself
        if (!strcmp(name, HFocusNameTP.name))
        {
            IUUpdateText(&HFocusNameTP, texts, names, n);
            if (setNickname(DEVICE_FOCUSER, HFocusNameT[DEVICE_FOCUSER].text) && setNickname(DEVICE_ROTATOR, HFocusNameT[DEVICE_ROTATOR].text))
                HFocusNameTP.s = IPS_OK;
            else
                HFocusNameTP.s = IPS_ALERT;
            IDSetText(&HFocusNameTP, nullptr);
            return true;
        }
    }
    return INDI::Focuser::ISNewText(dev, name, texts, names, n);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Temperature Coefficient
        if (!strcmp(TemperatureCoeffNP.name, name))
        {
            IUUpdateNumber(&TemperatureCoeffNP, values, names, n);
            for (int i = 0; i < n; i++)
            {
                if (setTemperatureCompensationCoeff('A' + i, TemperatureCoeffN[i].value) == false)
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Failed to set temperature coefficeints.");
                    TemperatureCoeffNP.s = IPS_ALERT;
                    IDSetNumber(&TemperatureCoeffNP, nullptr);
                    return false;
                }
            }

            TemperatureCoeffNP.s = IPS_OK;
            IDSetNumber(&TemperatureCoeffNP, nullptr);
            return true;
        }

        // Focuser Backlash Value
        if (!strcmp(FocuserBacklashNP.name, name))
        {
            IUUpdateNumber(&FocuserBacklashNP, values, names, n);
            if (setBacklashCompensationSteps(DEVICE_FOCUSER, FocuserBacklashN[0].value) == false)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to set focuser backlash value.");
                FocuserBacklashNP.s = IPS_ALERT;
                IDSetNumber(&FocuserBacklashNP, nullptr);
                return false;
            }

            FocuserBacklashNP.s = IPS_OK;
            IDSetNumber(&FocuserBacklashNP, nullptr);
            return true;
        }

        // Rotator Backlash Value
        if (!strcmp(RotatorBacklashNP.name, name))
        {
            IUUpdateNumber(&RotatorBacklashNP, values, names, n);
            if (setBacklashCompensationSteps(DEVICE_ROTATOR, RotatorBacklashN[0].value) == false)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to set rotator backlash value.");
                RotatorBacklashNP.s = IPS_ALERT;
                IDSetNumber(&RotatorBacklashNP, nullptr);
                return false;
            }

            RotatorBacklashNP.s = IPS_OK;
            IDSetNumber(&RotatorBacklashNP, nullptr);
            return true;
        }

        // Set LED intensity to the HUB itself via function setLedLevel()
        if (!strcmp(LedNP.name, name))
        {
            IUUpdateNumber(&LedNP, values, names, n);
            if (setLedLevel(LedN[0].value))
                LedNP.s = IPS_OK;
            else
                LedNP.s = IPS_ALERT;
            DEBUGF(INDI::Logger::DBG_SESSION, "Focuser LED level intensity : %f", LedN[0].value);
            IDSetNumber(&LedNP, nullptr);
            return true;
        }

        // Set Rotator Absolute Steps
        if (!strcmp(RotatorAbsPosNP.name, name))
        {
            IUUpdateNumber(&RotatorAbsPosNP, values, names, n);
            RotatorAbsPosNP.s = MoveAbsRotatorTicks(static_cast<uint32_t>(RotatorAbsPosN[0].value));
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            return true;
        }

        if (strstr(name, "ROTATOR"))
        {
            if (INDI::RotatorInterface::processNumber(dev, name, values, names, n))
                return true;
        }

#if 0
        // Set Rotator Absolute Steps
        if (!strcmp(RotatorAbsPosNP.name, name))
        {
            IUUpdateNumber(&RotatorAbsPosNP, values, names, n);
            RotatorAbsPosNP.s = MoveAbsRotatorTicks(static_cast<uint32_t>(RotatorAbsPosN[0].value));
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            return true;
        }

        // Set Rotator Absolute Angle
        if (!strcmp(RotatorAbsAngleNP.name, name))
        {
            IUUpdateNumber(&RotatorAbsAngleNP, values, names, n);
            RotatorAbsAngleNP.s = MoveAbsRotatorAngle(RotatorAbsAngleN[0].value);
            IDSetNumber(&RotatorAbsAngleNP, nullptr);
            return true;
        }
#endif

    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::ack()
{
    const char *cmd = "<F100GETDNN>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "Castor", 16);
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);
        DEBUGF(INDI::Logger::DBG_SESSION, "%s is detected.", response);

        // Read 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);

        tcflush(PortFD, TCIFLUSH);

        return true;
    }

    tcflush(PortFD, TCIFLUSH);
    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::getFocusConfig()
{
    const char *cmd = "<F100GETCFG>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[64];
    int nbytes_read    = 0;
    int nbytes_written = 0;
    char key[16];

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "!00", sizeof(response));
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        /*if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }*/
    }

    /*if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if ((strcmp(response, "CONFIG1")) && (strcmp(response, "CONFIG2")))
            return false;
    }*/

    memset(response, 0, sizeof(response));

    // Nickname
    if (isSimulation())
    {
        strncpy(response, "NickName=Tommy\n", sizeof(response));
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char nickname[16];
    int rc = sscanf(response, "%16[^=]=%16[^\n]s", key, nickname);

    if (rc != 2)
        return false;

    IUSaveText(&HFocusNameT[0], nickname);
    IDSetText(&HFocusNameTP, nullptr);

    HFocusNameTP.s = IPS_OK;
    IDSetText(&HFocusNameTP, nullptr);

    memset(response, 0, sizeof(response));

    // Get Max Position
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "Max Pos = %06d\n", 100000);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    uint32_t maxPos = 0;
    rc = sscanf(response, "%16[^=]=%d", key, &maxPos);
    if (rc == 2)
    {
        FocusAbsPosN[0].max = maxPos;
        FocusAbsPosN[0].step = maxPos / 50.0;
        FocusAbsPosN[0].min = 0;

        FocusRelPosN[0].max  = maxPos / 2;
        FocusRelPosN[0].step = maxPos / 100.0;
        FocusRelPosN[0].min  = 0;

        IUUpdateMinMax(&FocusAbsPosNP);
        IUUpdateMinMax(&FocusRelPosNP);

        maxControllerTicks = maxPos;
    }
    else
        return false;

    memset(response, 0, sizeof(response));

    // Get Device Type
    if (isSimulation())
    {
        strncpy(response, "Dev Typ = A\n", sizeof(response));
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    // Get Status Parameters
    memset(response, 0, sizeof(response));

    // Temperature Compensation On?
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TComp ON = %d\n", TemperatureCompensateS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCompOn;
    rc = sscanf(response, "%16[^=]=%d", key, &TCompOn);
    if (rc != 2)
        return false;

    IUResetSwitch(&TemperatureCompensateSP);
    TemperatureCompensateS[0].s = TCompOn ? ISS_ON : ISS_OFF;
    TemperatureCompensateS[0].s = TCompOn ? ISS_OFF : ISS_ON;
    TemperatureCompensateSP.s   = IPS_OK;
    IDSetSwitch(&TemperatureCompensateSP, nullptr);

    memset(response, 0, sizeof(response));

    // Temperature Coeff A
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TempCo A = %d\n", (int)TemperatureCoeffN[FOCUS_A_COEFF].value);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCoeffA;
    rc = sscanf(response, "%16[^=]=%d", key, &TCoeffA);
    if (rc != 2)
        return false;

    TemperatureCoeffN[FOCUS_A_COEFF].value = TCoeffA;

    memset(response, 0, sizeof(response));

    // Temperature Coeff B
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TempCo B = %d\n", (int)TemperatureCoeffN[FOCUS_B_COEFF].value);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCoeffB;
    rc = sscanf(response, "%16[^=]=%d", key, &TCoeffB);
    if (rc != 2)
        return false;

    TemperatureCoeffN[FOCUS_B_COEFF].value = TCoeffB;

    memset(response, 0, sizeof(response));

    // Temperature Coeff C
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TempCo C = %d\n", (int)TemperatureCoeffN[FOCUS_C_COEFF].value);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCoeffC;
    rc = sscanf(response, "%16[^=]=%d", key, &TCoeffC);
    if (rc != 2)
        return false;

    TemperatureCoeffN[FOCUS_C_COEFF].value = TCoeffC;

    memset(response, 0, sizeof(response));

    // Temperature Coeff D
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TempCo D = %d\n", (int)TemperatureCoeffN[FOCUS_D_COEFF].value);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCoeffD;
    rc = sscanf(response, "%16[^=]=%d", key, &TCoeffD);
    if (rc != 2)
        return false;

    TemperatureCoeffN[FOCUS_D_COEFF].value = TCoeffD;

    memset(response, 0, sizeof(response));

    // Temperature Coeff E
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TempCo E = %d\n", (int)TemperatureCoeffN[FOCUS_E_COEFF].value);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCoeffE;
    rc = sscanf(response, "%16[^=]=%d", key, &TCoeffE);
    if (rc != 2)
        return false;

    TemperatureCoeffN[FOCUS_E_COEFF].value = TCoeffE;

    TemperatureCoeffNP.s = IPS_OK;
    IDSetNumber(&TemperatureCoeffNP, nullptr);

    memset(response, 0, sizeof(response));

    // Temperature Compensation Mode
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TC Mode = %c\n", 'C');
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char compensateMode;
    rc = sscanf(response, "%16[^=]= %c", key, &compensateMode);
    if (rc != 2)
        return false;

    IUResetSwitch(&TemperatureCompensateModeSP);
    int index = compensateMode - 'A';
    if (index >= 0 && index <= 5)
    {
        TemperatureCompensateModeS[index].s = ISS_ON;
        TemperatureCompensateModeSP.s       = IPS_OK;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Invalid index %d for compensation mode.", index);
        TemperatureCompensateModeSP.s = IPS_ALERT;
    }

    IDSetSwitch(&TemperatureCompensateModeSP, nullptr);

    // Backlash Compensation
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "BLC En = %d\n", FocuserBacklashCompensationS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int BLCCompensate;
    rc = sscanf(response, "%16[^=]=%d", key, &BLCCompensate);
    if (rc != 2)
        return false;

    IUResetSwitch(&FocuserBacklashCompensationSP);
    FocuserBacklashCompensationS[0].s = BLCCompensate ? ISS_ON : ISS_OFF;
    FocuserBacklashCompensationS[1].s = BLCCompensate ? ISS_OFF : ISS_ON;
    FocuserBacklashCompensationSP.s   = IPS_OK;
    IDSetSwitch(&FocuserBacklashCompensationSP, nullptr);

    // Backlash Value
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "BLC Stps = %d\n", 50);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int BLCValue;
    rc = sscanf(response, "%16[^=]=%d", key, &BLCValue);
    if (rc != 2)
        return false;

    FocuserBacklashN[0].value = BLCValue;
    FocuserBacklashNP.s       = IPS_OK;
    IDSetNumber(&FocuserBacklashNP, nullptr);

    // Temperature Compensation on Start
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "TC Start = %d\n", TemperatureCompensateOnStartS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int TCOnStart;
    rc = sscanf(response, "%16[^=]=%d", key, &TCOnStart);
    if (rc != 2)
        return false;

    IUResetSwitch(&TemperatureCompensateOnStartSP);
    TemperatureCompensateOnStartS[0].s = TCOnStart ? ISS_ON : ISS_OFF;
    TemperatureCompensateOnStartS[1].s = TCOnStart ? ISS_OFF : ISS_ON;
    TemperatureCompensateOnStartSP.s   = IPS_OK;
    IDSetSwitch(&TemperatureCompensateOnStartSP, nullptr);

    // Get Status Parameters
    memset(response, 0, sizeof(response));

    // Home on start on?
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "HOnStart = %d\n", FocuserHomeOnStartS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int StartOnHome;
    rc = sscanf(response, "%16[^=]=%d", key, &StartOnHome);
    if (rc != 2)
        return false;

    IUResetSwitch(&FocuserHomeOnStartSP);
    FocuserHomeOnStartS[0].s = StartOnHome ? ISS_ON : ISS_OFF;
    FocuserHomeOnStartS[1].s = StartOnHome ? ISS_OFF : ISS_ON;
    FocuserHomeOnStartSP.s   = IPS_OK;
    IDSetSwitch(&FocuserHomeOnStartSP, nullptr);

    // Added By Philippe Besson the 28th of June for 'END' evalution
    // END is reached
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        strncpy(response, "END\n", 16);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';

        // Display the response to be sure to have read the complet TTY Buffer.
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (strcmp(response, "END"))
            return false;
    }
    // End of added code by Philippe Besson

    tcflush(PortFD, TCIFLUSH);

    focuserConfigurationComplete = true;

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::getRotatorStatus()
{
    const char *cmd = "<R100GETSTA>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read    = 0;
    int nbytes_written = 0;
    char key[16];

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "!00", 16);
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        /*if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }*/
    }

    ///////////////////////////////////////
    // #1 Get Current Position
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "CurrStep = %06d\n", rotatorSimPosition);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int currPos = 0;
    int rc = sscanf(response, "%16[^=]=%d", key, &currPos);
    if (rc == 2)
    {
        // Do not spam unless there is an actual change
        if (RotatorAbsPosN[0].value != currPos)
        {
            RotatorAbsPosN[0].value = currPos;
            IDSetNumber(&RotatorAbsPosNP, nullptr);
        }
    }
    else
        return false;

    ///////////////////////////////////////
    // #2 Get Target Position
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "TargStep = %06d\n", targetFocuserPosition);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    ///////////////////////////////////////
    // #3 Get Current PA
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "CurenPA = %06d\n", rotatorSimPA);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int currPA = 0;
    rc = sscanf(response, "%16[^=]=%d", key, &currPA);
    if (rc == 2)
    {
        // Only send when above a threshold
        double diffPA = fabs(GotoRotatorN[0].value - currPA / 1000.0);
        if (diffPA >= 0.01)
        {
            GotoRotatorN[0].value = currPA / 1000.0;
            IDSetNumber(&GotoRotatorNP, nullptr);
        }
    }
    else
        return false;

    ///////////////////////////////////////
    // #3 Get Target PA
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "TargetPA = %06d\n", targetFocuserPosition);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    // Get Status Parameters

    ///////////////////////////////////////
    // #5 is Moving?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsMoving = %d\n", (rotatorSimStatus[STATUS_MOVING] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int isMoving;
    rc = sscanf(response, "%16[^=]=%d", key, &isMoving);
    if (rc != 2)
        return false;

    RotatorStatusL[STATUS_MOVING].s = isMoving ? IPS_BUSY : IPS_IDLE;

    ///////////////////////////////////////
    // #6 is Homing?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsHoming = %d\n", (rotatorSimStatus[STATUS_HOMING] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int _isHoming;
    rc = sscanf(response, "%16[^=]=%d", key, &_isHoming);
    if (rc != 2)
        return false;

    RotatorStatusL[STATUS_HOMING].s = _isHoming ? IPS_BUSY : IPS_IDLE;

    // We set that isHoming in process, but we don't set it to false here it must be reset in TimerHit
    if (RotatorStatusL[STATUS_HOMING].s == IPS_BUSY)
        isRotatorHoming = true;

    ///////////////////////////////////////
    // #6 is Homed?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsHomed = %d\n", (rotatorSimStatus[STATUS_HOMED] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int isHomed;
    rc = sscanf(response, "%16[^=]=%d", key, &isHomed);
    if (rc != 2)
        return false;

    RotatorStatusL[STATUS_HOMED].s = isHomed ? IPS_OK : IPS_IDLE;
    IDSetLight(&RotatorStatusLP, nullptr);

    // Added By Philippe Besson the 28th of June for 'END' evalution
    // END is reached
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        strncpy(response, "END\n", 16);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';

        // Display the response to be sure to have read the complet TTY Buffer.
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (strcmp(response, "END"))
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Invalid END response.");
            return false;
        }
    }
    // End of added code by Philippe Besson

    tcflush(PortFD, TCIFLUSH);

    return true;

}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::getRotatorConfig()
{
    const char *cmd = "<R100GETCFG>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[64];
    int nbytes_read    = 0;
    int nbytes_written = 0;
    char key[16];

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "!00", sizeof(response));
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        /*if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }*/
    }

    /*if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if ((strcmp(response, "CONFIG1")) && (strcmp(response, "CONFIG2")))
            return false;
    }*/

    memset(response, 0, sizeof(response));
    ////////////////////////////////////////////////////////////
    // Nickname
    ////////////////////////////////////////////////////////////
    if (isSimulation())
    {
        strncpy(response, "NickName=Juli\n", sizeof(response));
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char nickname[16];
    int rc = sscanf(response, "%16[^=]=%16[^\n]s", key, nickname);

    if (rc != 2)
        return false;

    IUSaveText(&HFocusNameT[DEVICE_ROTATOR], nickname);
    HFocusNameTP.s = IPS_OK;
    IDSetText(&HFocusNameTP, nullptr);

    memset(response, 0, sizeof(response));

    ////////////////////////////////////////////////////////////
    // Get Max steps
    ////////////////////////////////////////////////////////////
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "MaxSteps = %06d\n", 100000);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    uint32_t maxPos = 0;
    rc = sscanf(response, "%16[^=]=%d", key, &maxPos);
    if (rc == 2)
    {
        RotatorAbsPosN[0].min = 0;
        RotatorAbsPosN[0].max = maxPos;
        RotatorAbsPosN[0].step = maxPos/50.0;
        IUUpdateMinMax(&RotatorAbsPosNP);
    }
    else
        return false;

    memset(response, 0, sizeof(response));

    ////////////////////////////////////////////////////////////
    // Get Device Type
    ////////////////////////////////////////////////////////////
    if (isSimulation())
    {
        strncpy(response, "Dev Type = B\n", sizeof(response));
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    // Get Status Parameters
    memset(response, 0, sizeof(response));

    ////////////////////////////////////////////////////////////
    // Backlash Compensation
    ////////////////////////////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "BLCSteps = %d\n", RotatorBacklashCompensationS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int BLCCompensate;
    rc = sscanf(response, "%16[^=]=%d", key, &BLCCompensate);
    if (rc != 2)
        return false;

    IUResetSwitch(&RotatorBacklashCompensationSP);
    RotatorBacklashCompensationS[0].s = BLCCompensate ? ISS_ON : ISS_OFF;
    RotatorBacklashCompensationS[1].s = BLCCompensate ? ISS_OFF : ISS_ON;
    RotatorBacklashCompensationSP.s   = IPS_OK;
    IDSetSwitch(&RotatorBacklashCompensationSP, nullptr);

    ////////////////////////////////////////////////////////////
    // Backlash Value
    ////////////////////////////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "BLCSteps = %d\n", 50);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int BLCValue;
    rc = sscanf(response, "%16[^=]=%d", key, &BLCValue);
    if (rc != 2)
        return false;

    RotatorBacklashN[0].value = BLCValue;
    RotatorBacklashNP.s       = IPS_OK;
    IDSetNumber(&RotatorBacklashNP, nullptr);

    ////////////////////////////////////////////////////////////
    // Home on start on?
    ////////////////////////////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, sizeof(response), "HOnStart = %d\n", RotatorHomeOnStartS[0].s == ISS_ON ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    int StartOnHome;
    rc = sscanf(response, "%16[^=]=%d", key, &StartOnHome);
    if (rc != 2)
        return false;

    IUResetSwitch(&RotatorHomeOnStartSP);
    RotatorHomeOnStartS[0].s = StartOnHome ? ISS_ON : ISS_OFF;
    RotatorHomeOnStartS[1].s = StartOnHome ? ISS_OFF : ISS_ON;
    RotatorHomeOnStartSP.s   = IPS_OK;
    IDSetSwitch(&RotatorHomeOnStartSP, nullptr);

    ////////////////////////////////////////////////////////////
    // Reverse?
    ////////////////////////////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "Reverse = %d\n", (rotatorSimStatus[STATUS_REVERSE] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int reverse;
    rc = sscanf(response, "%16[^=]=%d", key, &reverse);
    if (rc != 2)
        return false;

    RotatorStatusL[STATUS_REVERSE].s = reverse ? IPS_OK : IPS_IDLE;

    // If reverse is enable and switch shows disabled, let's change that
    // same thing is reverse is disabled but switch is enabled
    if ((reverse && ReverseRotatorS[1].s == ISS_ON) || (!reverse && ReverseRotatorS[0].s == ISS_ON))
    {
        IUResetSwitch(&ReverseRotatorSP);
        ReverseRotatorS[0].s = (reverse == 1) ? ISS_ON : ISS_OFF;
        ReverseRotatorS[1].s = (reverse == 0) ? ISS_ON : ISS_OFF;
        IDSetSwitch(&ReverseRotatorSP, nullptr);
    }

    RotatorStatusLP.s = IPS_OK;
    IDSetLight(&RotatorStatusLP, nullptr);

    ////////////////////////////////////////////////////////////
    // Max Speed - Not used
    ////////////////////////////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "MaxSpeed = %d\n", 800);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    // Added By Philippe Besson the 28th of June for 'END' evalution
    // END is reached
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        strncpy(response, "END\n", 16);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';

        // Display the response to be sure to have read the complet TTY Buffer.
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (strcmp(response, "END"))
            return false;
    }
    // End of added code by Philippe Besson

    tcflush(PortFD, TCIFLUSH);

    rotatorConfigurationComplete = true;

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::getFocusStatus()
{
    const char *cmd = "<F100GETSTA>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read    = 0;
    int nbytes_written = 0;
    char key[16];

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "!00", 16);
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        /*if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }*/
    }

    // Get Temperature
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        strncpy(response, "CurrTemp = +21.7\n", 16);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    float temperature = 0;
    int rc = sscanf(response, "%16[^=]=%f", key, &temperature);
    if (rc == 2)
    {
        TemperatureN[0].value = temperature;
        IDSetNumber(&TemperatureNP, nullptr);
    }
    else
    {
        char np[8];
        int rc = sscanf(response, "%16[^=]= %s", key, np);

        if (rc != 2 || strcmp(np, "NP"))
        {
            if (TemperatureNP.s != IPS_ALERT)
            {
                TemperatureNP.s = IPS_ALERT;
                IDSetNumber(&TemperatureNP, nullptr);
            }
            return false;
        }
    }

    ///////////////////////////////////////
    // #1 Get Current Position
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "CurrStep = %06d\n", focuserSimPosition);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    uint32_t currPos = 0;
    rc = sscanf(response, "%16[^=]=%d", key, &currPos);
    if (rc == 2)
    {
        FocusAbsPosN[0].value = currPos;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }
    else
        return false;

    ///////////////////////////////////////
    // #2 Get Target Position
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "TargStep = %06d\n", targetFocuserPosition);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    // Get Status Parameters

    ///////////////////////////////////////
    // #3 is Moving?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsMoving = %d\n", (focuserSimStatus[STATUS_MOVING] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int isMoving;
    rc = sscanf(response, "%16[^=]=%d", key, &isMoving);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_MOVING].s = isMoving ? IPS_BUSY : IPS_IDLE;

    ///////////////////////////////////////
    // #4 is Homing?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsHoming = %d\n", (focuserSimStatus[STATUS_HOMING] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int _isHoming;
    rc = sscanf(response, "%16[^=]=%d", key, &_isHoming);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_HOMING].s = _isHoming ? IPS_BUSY : IPS_IDLE;
    // For relative focusers home is not applicable.
    if (isFocuserAbsolute == false)
        FocuserStatusL[STATUS_HOMING].s = IPS_IDLE;

    // We set that isHoming in process, but we don't set it to false here it must be reset in TimerHit
    if (FocuserStatusL[STATUS_HOMING].s == IPS_BUSY)
        isFocuserHoming = true;

    ///////////////////////////////////////
    // #6 is Homed?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "IsHomed = %d\n", (focuserSimStatus[STATUS_HOMED] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int isHomed;
    rc = sscanf(response, "%16[^=]=%d", key, &isHomed);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_HOMED].s = isHomed ? IPS_OK : IPS_IDLE;
    // For relative focusers home is not applicable.
    if (isFocuserAbsolute == false)
        FocuserStatusL[STATUS_HOMED].s = IPS_IDLE;

    ///////////////////////////////////////
    // #7 Temperature probe?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "TempProb = %d\n", (focuserSimStatus[STATUS_TMPPROBE] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int TmpProbe;
    rc = sscanf(response, "%16[^=]=%d", key, &TmpProbe);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_TMPPROBE].s = TmpProbe ? IPS_OK : IPS_IDLE;

    ///////////////////////////////////////
    // #8 Remote IO?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "RemoteIO = %d\n", (focuserSimStatus[STATUS_REMOTEIO] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int RemoteIO;
    rc = sscanf(response, "%16[^=]=%d", key, &RemoteIO);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_REMOTEIO].s = RemoteIO ? IPS_OK : IPS_IDLE;

    ///////////////////////////////////////
    // #9 Hand controller?
    ///////////////////////////////////////
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        snprintf(response, 32, "HCStatus = %d\n", (focuserSimStatus[STATUS_HNDCTRL] == ISS_ON) ? 1 : 0);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }
    response[nbytes_read - 1] = '\0';
    DEBUGF(DBG_FOCUS, "RES (%s)", response);

    int HndCtlr;
    rc = sscanf(response, "%16[^=]=%d", key, &HndCtlr);
    if (rc != 2)
        return false;

    FocuserStatusL[STATUS_HNDCTRL].s = HndCtlr ? IPS_OK : IPS_IDLE;

    FocuserStatusLP.s = IPS_OK;
    IDSetLight(&FocuserStatusLP, nullptr);

    // Added By Philippe Besson the 28th of June for 'END' evalution
    // END is reached
    memset(response, 0, sizeof(response));
    if (isSimulation())
    {
        strncpy(response, "END\n", 16);
        nbytes_read = strlen(response);
    }
    else if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';

        // Display the response to be sure to have read the complet TTY Buffer.
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (strcmp(response, "END"))
        {
            DEBUG(INDI::Logger::DBG_WARNING, "Invalid END response.");
            return false;
        }
    }
    // End of added code by Philippe Besson

    tcflush(PortFD, TCIFLUSH);

    return true;

}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setLedLevel(int level)
// Write via the serial port to the HUB the selected LED intensity level

{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<H100SETLED%d>", level);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "SET", 16);
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);
        tcflush(PortFD, TCIFLUSH);

        if (!strcmp(response, "SET"))
            return true;
        else
            return false;
    }

    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setNickname(DeviceType type, const char *nickname)
// Write via the serial port to the HUB the choiced nikname of he focuser
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<%c100SETDNN%s>", (type == DEVICE_FOCUSER ? 'F' : 'R'), nickname);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

   tcflush(PortFD, TCIFLUSH);
   return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::halt(DeviceType type)
{
    char cmd[32];;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<%c100DOHALT>", (type == DEVICE_FOCUSER ? 'F' : 'R'));
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        if (type == DEVICE_FOCUSER)
            focuserSimStatus[STATUS_MOVING] = ISS_OFF;
        else
        {
            rotatorSimStatus[STATUS_MOVING] = ISS_OFF;
            isRotatorHoming = false;
        }
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    isRotatorHoming = false;

    tcflush(PortFD, TCIFLUSH);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::home(DeviceType type)
{
    char cmd[32];;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<%c100DOHOME>", (type == DEVICE_FOCUSER ? 'F' : 'R'));
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        if (type == DEVICE_FOCUSER)
        {
            focuserSimStatus[STATUS_HOMING] = ISS_ON;
            targetFocuserPosition = 0;
        }
        else
        {
            rotatorSimStatus[STATUS_HOMING] = ISS_ON;
            targetRotatorPosition = 0;
        }
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    tcflush(PortFD, TCIFLUSH);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::homeOnStart(DeviceType type, bool enable)
{
    char cmd[32];;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<%c100SETHOS%d>", (type == DEVICE_FOCUSER ? 'F' : 'R'), enable ? 1 : 0);
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    tcflush(PortFD, TCIFLUSH);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::center(DeviceType type)
{
    if (type == DEVICE_ROTATOR)
        return MoveAbsRotatorTicks(RotatorAbsPosN[0].max/2);

    const char * cmd = "<F100CENTER>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        if (type == DEVICE_FOCUSER)
        {
            focuserSimStatus[STATUS_MOVING] = ISS_ON;
            targetFocuserPosition = FocusAbsPosN[0].max / 2;
        }
        else
        {
            rotatorSimStatus[STATUS_MOVING] = ISS_ON;
            targetRotatorPosition = RotatorAbsPosN[0].max / 2;
        }

    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    tcflush(PortFD, TCIFLUSH);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setTemperatureCompensation(bool enable)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<F100SETTCE%d>", enable ? 1 : 0);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setTemperatureCompensationMode(char mode)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<F100SETTCM%c>", mode);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setTemperatureCompensationCoeff(char mode, int16_t coeff)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<F100SETTCC%c%c%04d>", mode, coeff >= 0 ? '+' : '-', (int)std::abs(coeff));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setTemperatureCompensationOnStart(bool enable)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<F100SETTCS%d>", enable ? 1 : 0);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    tcflush(PortFD, TCIFLUSH);

    if (isSimulation() == false)
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setBacklashCompensation(DeviceType type, bool enable)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<%c100SETBCE%d>", (type == DEVICE_FOCUSER ? 'F' : 'R'), enable ? 1 : 0);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::setBacklashCompensationSteps(DeviceType type, uint16_t steps)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read=0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<%c100SETBCS%02d>", (type == DEVICE_FOCUSER ? 'F' : 'R'), steps);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::reverseRotator(bool enable)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<R100SETREV%d>", enable ? 1 : 0);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation() == false)
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        // Read the 'END'
        tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::resetFactory()
{
    const char *cmd = "<H100RESETH>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "SET", 16);
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;

        if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);
        tcflush(PortFD, TCIFLUSH);

        if (!strcmp(response, "SET"))
        {
            return true;
            getFocusConfig();
            getRotatorConfig();
        }
        else
            return false;
    }

    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::isResponseOK()
{
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read = 0;

    memset(response, 0, sizeof(response));

    if (isSimulation())
    {
        strcpy(response, "!00");
        nbytes_read = strlen(response) + 1;
    }
    else
    {
        if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "TTY error: %s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (!strcmp(response, "!00"))
            return true;
        else
        {
            memset(response, 0, sizeof(response));
            while (strstr(response, "END") == nullptr)
            {
                if ((errcode = tty_read_section(PortFD, response, 0xA, GEMINI_TIMEOUT, &nbytes_read)) != TTY_OK)
                {
                    tty_error_msg(errcode, errmsg, MAXRBUF);
                    DEBUGF(INDI::Logger::DBG_ERROR, "TTY error: %s", errmsg);
                    return false;
                }
                response[nbytes_read - 1] = '\0';
                DEBUGF(INDI::Logger::DBG_ERROR, "Controller error: %s", response);
            }

            return false;
        }
    }
    return true;
}

/************************************************************************************
*
* ***********************************************************************************/
IPState Gemini::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_written = 0;

    INDI_UNUSED(speed);

    memset(response, 0, sizeof(response));

    snprintf(cmd, 16, "<F100DOMOVE%c>", (dir == FOCUS_INWARD) ? '0' : '1');

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return IPS_ALERT;
        }

        if (isResponseOK() == false)
            return IPS_ALERT;

        gettimeofday(&focusMoveStart, nullptr);
        focusMoveRequest = duration / 1000.0;
    }

    if (duration <= POLLMS)
    {
        usleep(POLLMS * 1000);
        AbortFocuser();
        return IPS_OK;
    }

    tcflush(PortFD, TCIFLUSH);

    return IPS_BUSY;
}

/************************************************************************************
*
* ***********************************************************************************/
IPState Gemini::MoveAbsFocuser(uint32_t targetTicks)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_written = 0;

    targetFocuserPosition = targetTicks;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<F100MOVABS%06d>", targetTicks);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return IPS_ALERT;
        }

        if (isResponseOK() == false)
            return IPS_ALERT;
    }

    FocusAbsPosNP.s = IPS_BUSY;

    tcflush(PortFD, TCIFLUSH);

    return IPS_BUSY;
}

/************************************************************************************
*
* ***********************************************************************************/
IPState Gemini::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    uint32_t newPosition = 0;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    return MoveAbsFocuser(newPosition);
}

/************************************************************************************
*
* ***********************************************************************************/
void Gemini::TimerHit()
{
    if (!isConnected())
        return;

    if (focuserConfigurationComplete == false || rotatorConfigurationComplete == false)
    {
        SetTimer(POLLMS);
        return;
    }

    // Focuser Status
    bool statusrc = false;
    for (int i = 0; i < 2; i++)
    {
        statusrc = getFocusStatus();
        if (statusrc)
            break;
    }

    if (statusrc == false)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Unable to read focuser status....");
        SetTimer(POLLMS);
        return;
    }

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (isSimulation())
        {
            if (FocusAbsPosN[0].value < targetFocuserPosition)
                focuserSimPosition += 100;
            else
                focuserSimPosition -= 100;

            focuserSimStatus[STATUS_MOVING] = ISS_ON;

            if (std::abs((int64_t)focuserSimPosition - (int64_t)targetFocuserPosition) < 100)
            {
                FocusAbsPosN[0].value    = targetFocuserPosition;
                focuserSimPosition              = FocusAbsPosN[0].value;
                focuserSimStatus[STATUS_MOVING] = ISS_OFF;
                FocuserStatusL[STATUS_MOVING].s = IPS_IDLE;
                if (focuserSimStatus[STATUS_HOMING] == ISS_ON)
                {
                    FocuserStatusL[STATUS_HOMED].s = IPS_OK;
                    focuserSimStatus[STATUS_HOMING] = ISS_OFF;
                }
            }
        }

        if (isFocuserHoming && FocuserStatusL[STATUS_HOMED].s == IPS_OK)
        {
            isFocuserHoming = false;
            FocuserGotoSP.s = IPS_OK;
            IUResetSwitch(&FocuserGotoSP);
            FocuserGotoS[GOTO_HOME].s = ISS_ON;
            IDSetSwitch(&FocuserGotoSP, nullptr);
            FocusAbsPosNP.s = IPS_OK;
            IDSetNumber(&FocusRelPosNP, nullptr);
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached home position.");
        }
        else if (FocuserStatusL[STATUS_MOVING].s == IPS_IDLE)
        {
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            IDSetNumber(&FocusRelPosNP, nullptr);
            if (FocuserGotoSP.s == IPS_BUSY)
            {
                IUResetSwitch(&FocuserGotoSP);
                FocuserGotoSP.s = IPS_OK;
                IDSetSwitch(&FocuserGotoSP, nullptr);
            }
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
        }
    }
    if (FocuserStatusL[STATUS_HOMING].s == IPS_BUSY && FocuserGotoSP.s != IPS_BUSY)
    {
        FocuserGotoSP.s = IPS_BUSY;
        IDSetSwitch(&FocuserGotoSP, nullptr);
    }

    // Rotator Status
    statusrc = false;
    for (int i = 0; i < 2; i++)
    {
        statusrc = getRotatorStatus();
        if (statusrc)
            break;
    }

    if (statusrc == false)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Unable to read rotator status....");
        SetTimer(POLLMS);
        return;
    }

    if (RotatorAbsPosNP.s == IPS_BUSY || GotoRotatorNP.s == IPS_BUSY)
    {
        /*if (isSimulation())
        {
            if (RotatorAbsPosN[0].value < targetRotatorPosition)
                RotatorSimPosition += 100;
            else
                RotatorSimPosition -= 100;

            RotatorSimStatus[STATUS_MOVING] = ISS_ON;

            if (std::abs((int64_t)RotatorSimPosition - (int64_t)targetRotatorPosition) < 100)
            {
                RotatorAbsPosN[0].value    = targetRotatorPosition;
                RotatorSimPosition              = RotatorAbsPosN[0].value;
                RotatorSimStatus[STATUS_MOVING] = ISS_OFF;
                RotatorStatusL[STATUS_MOVING].s = IPS_IDLE;
                if (RotatorSimStatus[STATUS_HOMING] == ISS_ON)
                {
                    RotatorStatusL[STATUS_HOMED].s = IPS_OK;
                    RotatorSimStatus[STATUS_HOMING] = ISS_OFF;
                }
            }
        }*/

        if (isRotatorHoming && RotatorStatusL[STATUS_HOMED].s == IPS_OK)
        {
            isRotatorHoming = false;
            HomeRotatorSP.s = IPS_OK;
            IUResetSwitch(&HomeRotatorSP);
            IDSetSwitch(&HomeRotatorSP, nullptr);
            RotatorAbsPosNP.s = IPS_OK;
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            GotoRotatorNP.s = IPS_OK;
            IDSetNumber(&GotoRotatorNP, nullptr);
            DEBUG(INDI::Logger::DBG_SESSION, "Rotator reached home position.");
        }
        else if (RotatorStatusL[STATUS_MOVING].s == IPS_IDLE)
        {
            RotatorAbsPosNP.s = IPS_OK;
            IDSetNumber(&RotatorAbsPosNP, nullptr);
            GotoRotatorNP.s = IPS_OK;
            IDSetNumber(&GotoRotatorNP, nullptr);
            if (HomeRotatorSP.s == IPS_BUSY)
            {
                IUResetSwitch(&HomeRotatorSP);
                HomeRotatorSP.s = IPS_OK;
                IDSetSwitch(&HomeRotatorSP, nullptr);
            }
            DEBUG(INDI::Logger::DBG_SESSION, "Rotator reached requested position.");
        }
    }
    if (RotatorStatusL[STATUS_HOMING].s == IPS_BUSY && HomeRotatorSP.s != IPS_BUSY)
    {
        HomeRotatorSP.s = IPS_BUSY;
        IDSetSwitch(&HomeRotatorSP, nullptr);
    }

    SetTimer(POLLMS);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::AbortFocuser()
{
    const char *cmd = "<F100DOHALT>";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    memset(response, 0, sizeof(response));

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strncpy(response, "!00", 16);
        nbytes_read              = strlen(response) + 1;
        focuserSimStatus[STATUS_MOVING] = ISS_OFF;
        focuserSimStatus[STATUS_HOMING] = ISS_OFF;
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if (isResponseOK() == false)
            return false;
    }


    if (FocusRelPosNP.s == IPS_BUSY)
    {
        FocusRelPosNP.s = IPS_IDLE;
        IDSetNumber(&FocusRelPosNP, nullptr);
    }

    FocusTimerNP.s = FocusAbsPosNP.s = FocuserGotoSP.s = IPS_IDLE;
    IUResetSwitch(&FocuserGotoSP);
    IDSetNumber(&FocusAbsPosNP, nullptr);
    IDSetSwitch(&FocuserGotoSP, nullptr);

    tcflush(PortFD, TCIFLUSH);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
float Gemini::calcTimeLeft(timeval start, float req)
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

/************************************************************************************
*
* ***********************************************************************************/
IPState Gemini::MoveAbsRotatorTicks(uint32_t targetTicks)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_written = 0;

    targetRotatorPosition = targetTicks;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<R100MOVABS%06d>", targetTicks);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return IPS_ALERT;
        }

        if (isResponseOK() == false)
            return IPS_ALERT;
    }

    RotatorAbsPosNP.s = IPS_BUSY;

    tcflush(PortFD, TCIFLUSH);

    return IPS_BUSY;
}

/************************************************************************************
*
* ***********************************************************************************/
IPState Gemini::MoveAbsRotatorAngle(double angle)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_written = 0;

    targetRotatorAngle = angle * 1000.0;

    memset(response, 0, sizeof(response));

    snprintf(cmd, 32, "<R100MOVEPA%06d>", targetRotatorAngle);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return IPS_ALERT;
        }

        if (isResponseOK() == false)
            return IPS_ALERT;
    }

    GotoRotatorNP.s = IPS_BUSY;

    tcflush(PortFD, TCIFLUSH);

    return IPS_BUSY;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &TemperatureCompensateSP);
    IUSaveConfigSwitch(fp, &TemperatureCompensateOnStartSP);
    IUSaveConfigNumber(fp, &TemperatureCoeffNP);
    IUSaveConfigSwitch(fp, &TemperatureCompensateModeSP);
    IUSaveConfigSwitch(fp, &FocuserBacklashCompensationSP);
    IUSaveConfigSwitch(fp, &FocuserHomeOnStartSP);
    IUSaveConfigNumber(fp, &FocuserBacklashNP);

    IUSaveConfigSwitch(fp, &ReverseRotatorSP);
    IUSaveConfigSwitch(fp, &RotatorBacklashCompensationSP);
    IUSaveConfigNumber(fp, &RotatorBacklashNP);    
    IUSaveConfigSwitch(fp, &RotatorHomeOnStartSP);

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState Gemini::MoveRotator(double angle)
{
    IPState state = MoveAbsRotatorAngle(angle);
    RotatorAbsPosNP.s = state;
    IDSetNumber(&RotatorAbsPosNP, nullptr);

    return state;
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState Gemini::HomeRotator()
{
    return (home(DEVICE_ROTATOR) ? IPS_BUSY : IPS_ALERT);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool Gemini::ReverseRotator(bool enabled)
{
   return reverseRotator(enabled);
}

