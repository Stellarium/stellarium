/*******************************************************************************
 Copyright(c) 2016 Philippe Besson. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "ifwoptec.h"

#include "indicom.h"
#include "indicontroller.h"
#include "connectionplugins/connectionserial.h"

#include <memory>
#include <regex>
#include <cstring>
#include <unistd.h>

std::unique_ptr<FilterIFW> filter_ifw(new FilterIFW());

/************************************************************************************
*
************************************************************************************/
void ISInit()
{
    static int isInit = 0;

    if (isInit == 1)
        return;

    isInit = 1;
    if (filter_ifw.get() == nullptr)
        filter_ifw.reset(new FilterIFW());
}

/************************************************************************************
*
************************************************************************************/
void ISGetProperties(const char *dev)
{
    ISInit();
    filter_ifw->ISGetProperties(dev);
}

/************************************************************************************
*
************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ISInit();
    filter_ifw->ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
*
************************************************************************************/
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();
    filter_ifw->ISNewText(dev, name, texts, names, n);
}

/************************************************************************************
*
************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ISInit();
    filter_ifw->ISNewNumber(dev, name, values, names, n);
}

/************************************************************************************
*
************************************************************************************/
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

/************************************************************************************
*
************************************************************************************/
void ISSnoopDevice(XMLEle *root)
{
    filter_ifw->ISSnoopDevice(root);
}

/************************************************************************************
*
************************************************************************************/
FilterIFW::FilterIFW()
{
    //ctor
    setVersion(VERSION, SUBVERSION);
    strncpy(filterSim, filterSim5, sizeof(filterSim)); // For simulation mode

    // Set communication to serail only and avoid driver crash at starting up
    setFilterConnection(CONNECTION_SERIAL);

    // We add an additional debug level so we can log verbose member function starting
    // DBG_TAG is used by macro DEBUGTAG() define in ifwoptec.h
    int DBG_TAG = 0;

    DBG_TAG = INDI::Logger::getInstance().addDebugLevel("Function tag", "Tag");
}

/************************************************************************************
*
************************************************************************************/
const char *FilterIFW::getDefaultName()
{
    return (const char *)"Optec IFW";
}

/**************************************************************************************
*
***************************************************************************************/
bool FilterIFW::initProperties()
{
    INDI::FilterWheel::initProperties();

    // Settings
    IUFillText(&WheelIDT[0], "ID", "ID", "-");
    IUFillTextVector(&WheelIDTP, WheelIDT, 1, getDeviceName(), "WHEEL_ID", "Wheel", FILTER_TAB, IP_RO, 60, IPS_IDLE);

    // Command
    IUFillSwitch(&HomeS[0], "HOME", "Home", ISS_OFF);
    IUFillSwitchVector(&HomeSP, HomeS, 1, getDeviceName(), "HOME", "Home", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Within simulation mode, provide possibilities to select the kind of filter wheel: 5 or 8 filters
    IUFillSwitch(&FilterNbrS[0], "VAL5", "5", ISS_ON);
    IUFillSwitch(&FilterNbrS[1], "VAL6", "6", ISS_OFF);
    IUFillSwitch(&FilterNbrS[2], "VAL8", "8", ISS_OFF);
    IUFillSwitch(&FilterNbrS[3], "VAL9", "9", ISS_OFF);
    IUFillSwitchVector(&FilterNbrSP, FilterNbrS, 4, getDeviceName(), "FILTER_NBR", "Filter nbr", FILTER_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // User could choice to unrestrict chars set to set the filternames if he accepts to have crazy display name on IFW box
    // Within simulation mode, provide possibilities to select the kind of filter wheel: 5 or 8 filters
    IUFillSwitch(&CharSetS[0], "RES", "Restricted", ISS_ON);
    IUFillSwitch(&CharSetS[1], "UNRES", "All", ISS_OFF);
    IUFillSwitchVector(&CharSetSP, CharSetS, 2, getDeviceName(), "CHARSET", "Chars allowed", FILTER_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // Firmware of the IFW
    IUFillText(&FirmwareT[0], "FIRMWARE", "Firmware", "Unknown");
    IUFillTextVector(&FirmwareTP, FirmwareT, 1, getDeviceName(), "FIRMWARE_ID", "IFW", FILTER_TAB, IP_RO, 60, IPS_IDLE);

    serialConnection->setDefaultBaudRate(Connection::Serial::B_19200);

    addAuxControls();

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::updateProperties()
{
    if (isConnected())
    {
        defineSwitch(&HomeSP);
        defineText(&FirmwareTP);
        defineText(&WheelIDTP); // ID of the wheel first in Filter tab page
        if (isSimulation())
            defineSwitch(
                &FilterNbrSP); // Then the button only for Simulation to select the number of filter of the Wheel (5 or 8)
        defineSwitch(&CharSetSP);
        defineNumber(&FilterSlotNP);
        controller->updateProperties();

        GetFirmware(); // Try to get Firmware version of the IFW. NOt all Firmware support this function
        moveHome();    // Initialisation of the physical IFW
    }
    else
    {
        deleteProperty(HomeSP.name);
        deleteProperty(FirmwareTP.name);
        deleteProperty(WheelIDTP.name);
        deleteProperty(CharSetSP.name);
        deleteProperty(FilterNbrSP.name);
        deleteProperty(FilterSlotNP.name);
        deleteProperty(FilterNameTP->name);
        controller->updateProperties();
    }

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::WriteTTY(char *command)
{
    char cmd[OPTEC_MAXLEN_CMD];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    snprintf(cmd, OPTEC_MAXLEN_CMD, "%s%s", command, "\n\r");
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (!isSimulation())
    {
        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }
    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::ReadTTY(char *resp, char *simulation, int timeout)
{
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[OPTEC_MAXLEN_RESP + 1];
    int nbytes_read = 0;

    memset(response, 0, sizeof(response));

    if (isSimulation())
    {
        strncpy(response, simulation, sizeof(response));
        nbytes_read = strlen(response) + 2; // +2 for simulation = "\n\r" see below
    }
    else
    {
        if ((errcode = tty_read_section(PortFD, response, 0xd, timeout, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s() TTY error: %s", __FUNCTION__, "errmsg");
            return false;
        }
    }

    if (nbytes_read <= 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Controller error: Nothing returned by the IFW");
        response[0] = '\0';
        return false;
    }

    response[nbytes_read - 2] = '\0'; //Remove control char from string (\n\r)
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);
    strncpy(resp, response, sizeof(response));
    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::Handshake()
{
    char response[OPTEC_MAXLEN_RESP + 1];
    memset(response, 0, sizeof(response));

    if (!WriteTTY((char *)"WSMODE"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        return false;
    }

    if (!ReadTTY(response, (char *)"!", OPTEC_TIMEOUT))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
        return false;
    }

    if (strcmp(response, "!") != 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "failed, wrong response from IFW");
        DEBUGF(INDI::Logger::DBG_DEBUG, "Response : (%s)", response);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Success, response from IFW is : %s", response);
    DEBUG(INDI::Logger::DBG_SESSION, "IFW is online");

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::Disconnect()
{
    DEBUGTAG();
    char response[OPTEC_MAXLEN_RESP + 1];
    memset(response, 0, sizeof(response));

    if (!WriteTTY((char *)"WEXITS"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        return false;
    }

    if (!ReadTTY(response, (char *)"END", OPTEC_TIMEOUT))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
        return false;
    }

    if (strcmp(response, "END") != 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "failed, wrong response from IFW");
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "IFW return in manual mode, response from IFW is : %s", response);
    DEBUG(INDI::Logger::DBG_SESSION, "IFW is offline.");

    return INDI::FilterWheel::Disconnect();
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // User has changed one or more names from filter related to the Wheel ID present in the IFW
        if (strcmp(FilterNameTP->name, name) == 0)
        {
            // Only these chars are allowed to be able to the IFW display to show names correctly
            std::regex rx("^[A-Z0-9=.#/%[:space:]-]{1,8}$");

            bool match = true;
            //Check only if user allowed chars restriction
            if (CharSetS[0].s == ISS_ON)
            {
                for (int i = 0; i < n; i++)
                {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "FilterName request N°%d : %s", i, texts[i]);
                    match = std::regex_match(texts[i], rx);
                    if (!match)
                        break;
                }
            }

            if (match)
            {
                IUUpdateText(FilterNameTP, texts, names, n);
                FilterNameTP->s = SetFilterNames() ? IPS_OK : IPS_ALERT;
                IDSetText(FilterNameTP, nullptr);
            }
            else
            {
                FilterNameTP->s = IPS_ALERT;
                IDSetText(FilterNameTP, nullptr);
                DEBUG(INDI::Logger::DBG_SESSION, "WARNING *****************************************************");
                DEBUG(INDI::Logger::DBG_SESSION,
                      "One of the filter name is not valid. It should not have more than 8 chars");
                DEBUG(INDI::Logger::DBG_SESSION, "Valid chars are A to Z, 0 to 9 = . # / - percent or space");
                DEBUG(INDI::Logger::DBG_SESSION, "WARNING *****************************************************");
                return false;
            }
            return true;
        }
    }
    return INDI::FilterWheel::ISNewText(dev, name, texts, names, n);
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(HomeSP.name, name) == 0)
        {
            bool result = true;
            // User request the IWF reset (Home procedure will read the Wheel ID, load from EEProm the filters names and goes to filter N°1
            IUUpdateSwitch(&HomeSP, states, names, n);
            IUResetSwitch(&HomeSP);
            DEBUG(INDI::Logger::DBG_SESSION, "Executing Home command...");

            FilterNameTP->s = IPS_BUSY;
            IDSetText(FilterNameTP, nullptr);

            if (!moveHome())
            {
                HomeSP.s = IPS_ALERT;
                result   = false;
            }
            else
            {
                DEBUG(INDI::Logger::DBG_DEBUG, "Getting filter information...");

                if (!(GetFilterNames() && GetFilterPos() != 0))
                {
                    HomeSP.s = IPS_ALERT;
                    result   = false;
                }
            }

            IDSetSwitch(&HomeSP, nullptr);
            if (!result)
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "%s() failed to get information", __FUNCTION__);
                DEBUG(INDI::Logger::DBG_SESSION, "Please check unit and press 'Home' button");
                return false;
            }

            return true;
        }

        if (strcmp(FilterNbrSP.name, name) == 0)
        {
            IUUpdateSwitch(&FilterNbrSP, states, names, n);

            // Is simulation active, User can change from 5 postions wheel to 6, 8 or 9 ones
            // Check if selection is different from active one

            if ((FilterNbrS[0].s == ISS_ON) & (FilterSlotN[0].max != 5))
            {
                strncpy(filterSim, filterSim5, sizeof(filterSim));
                FilterNbrSP.s = (GetFilterNames() && GetFilterPos() != 0) ? IPS_OK : IPS_ALERT;
            }
            else if ((FilterNbrS[1].s == ISS_ON) & (FilterSlotN[0].max != 6))
            {
                strncpy(filterSim, filterSim6, sizeof(filterSim));
                FilterNbrSP.s = (GetFilterNames() && GetFilterPos() != 0) ? IPS_OK : IPS_ALERT;
            }
            else if ((FilterNbrS[2].s == ISS_ON) & (FilterSlotN[0].max != 8))
            {
                strncpy(filterSim, filterSim8, sizeof(filterSim));
                FilterNbrSP.s = (GetFilterNames() && GetFilterPos() != 0) ? IPS_OK : IPS_ALERT;
            }
            else if ((FilterNbrS[3].s == ISS_ON) & (FilterSlotN[0].max != 9))
            {
                strncpy(filterSim, filterSim9, sizeof(filterSim));
                FilterNbrSP.s = (GetFilterNames() && GetFilterPos() != 0) ? IPS_OK : IPS_ALERT;
            }
            else
                FilterNbrSP.s = IPS_OK;

            if (FilterNbrSP.s == IPS_ALERT)
            {
                IDSetSwitch(&FilterNbrSP, "%s() failed to change number of filters", __FUNCTION__);
                return false;
            }
            else
                IDSetSwitch(&FilterNbrSP, nullptr);

            return true;
        }

        // Set switch from user selection to allowed use of all chars or restricted to display IFW
        // 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ=.#/-%
        if (strcmp(CharSetSP.name, name) == 0)
        {
            IUUpdateSwitch(&CharSetSP, states, names, n);
            CharSetSP.s = IPS_OK;
            IDSetSwitch(&CharSetSP, nullptr);
            return true;
        }
    }

    return INDI::FilterWheel::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
*
************************************************************************************/
void FilterIFW::simulationTriggered(bool enable)
{
    // toogle buttons to select 5 or 8 filters depend if Simulation active or not
    if (enable)
    {
        if (isConnected())
        {
            defineSwitch(&FilterNbrSP);
        }
    }
    else
        deleteProperty(FilterNbrSP.name);
}

/************************************************************************************
*
************************************************************************************/
void FilterIFW::TimerHit()
{
    // not use with IFW
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::SelectFilter(int f)
{
    DEBUGTAG();
    bool result = true;
    char cmd[32]={0};
    char response[OPTEC_MAXLEN_RESP + 1];

    memset(response, 0, sizeof(response));
    snprintf(cmd, 32, "%s%d", "WGOTO", f);

    FilterSlotNP.s = IPS_BUSY;
    IDSetNumber(&FilterSlotNP, "*** Moving to filter n° %d ***", f);

    if (!WriteTTY(cmd))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = false;
    }
    else
    {
        if (isSimulation())
        {
            // Time depend of rotation direction. Goes via shortest way
            int maxFilter = FilterSlotN[0].max;
            int way1, way2;
            if (f > actualSimFilter)
            {
                // CW calcul
                way1 = f - actualSimFilter;
                // CCW calcul
                way2 = actualSimFilter + maxFilter - f;
            }
            else
            {
                // CCW calcul
                way1 = actualSimFilter - f;
                // CW calcul
                way2 = maxFilter - actualSimFilter + f;
            }
            // About 2 second to change 1 position
            if (way1 < way2)
                sleep(2 * way1);
            else
                sleep(2 * way2);

            // Save actual value for Simulation
            actualSimFilter = f;
        }

        if (!ReadTTY(response, (char *)"*", OPTEC_TIMEOUT_MOVE))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
            result = false;
        }
        else if (strncmp(response, "*", 1) != 0)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Error: %s", response);
            PRINT_ER(response);
            result = false;
        }
    }

    if (!result)
    {
        FilterSlotNP.s = IPS_ALERT;
        IDSetNumber(&FilterSlotNP, "*** UNABLE TO SELECT THE FILTER ***");
        return false;
    }

    // As to be called when filter has moved to new position:
    SelectFilterDone(GetFilterPos());

    FilterSlotNP.s = IPS_OK;
    IDSetNumber(&FilterSlotNP, "Selected filter position reached");
    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::GetFilterNames()
{
    DEBUGTAG();
    bool result = true;
    char filterName[MAXINDINAME];
    char filterLabel[MAXINDILABEL];
    char filterList[OPTEC_MAXLEN_NAMES + 9]; // tempo string used fo display filtername debug information
    char response[OPTEC_MAXLEN_RESP + 1];
    int lenResponse = 0; // Nbr of char in the response string
    int maxFilter   = 0;

    memset(response, 0, sizeof(response));

    FilterNameTP->s = IPS_BUSY;
    IDSetText(FilterNameTP, nullptr);

    if (!WriteTTY((char *)"WREAD"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = false;
    }
    else if (!ReadTTY(response, filterSim, OPTEC_TIMEOUT))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
        result = false;
    }

    if (result)
    {
        // Check the size of response to know if this is a 5 or 8 postion wheel as from R2.x IFW support both
        lenResponse = strlen(response);

        switch (lenResponse)
        {
            case 40:
                maxFilter = 5;
                break;
            case 48:
                maxFilter = 6;
                break;
            case 64:
                maxFilter = 8;
                break;
            case 72:
                maxFilter = 9;
                break;
            default:
                maxFilter = 0; // Means error somewhere
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Length of response %d", lenResponse);
        DEBUGF(INDI::Logger::DBG_DEBUG, "MaxFilter  %d", maxFilter);
        if (maxFilter != 0)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Success, response from IFW is : %s", response);

            // Start parsing from IFW message
            char *p = response;
            char filterNameIFW[OPTEC_MAX_FILTER][9];
            filterList[0] = '\0';

            for (int i = 0; i < maxFilter; i++)
            {
                strncpy(filterNameIFW[i], p, OPTEC_LEN_FLTNAME);
                filterNameIFW[i][OPTEC_LEN_FLTNAME] = '\0';
                p                                   = p + OPTEC_LEN_FLTNAME;
                DEBUGF(INDI::Logger::DBG_DEBUG, "filterNameIFW[%d] : %s", i, filterNameIFW[i]);
                strncat(filterList, filterNameIFW[i], OPTEC_LEN_FLTNAME);
                strncat(filterList, "/", 1);
            }
            filterList[strlen(filterList) - 1] = '\0'; //Remove last "/"

            DEBUG(INDI::Logger::DBG_DEBUG, "Redo filters name list");
            // Set new max value on the filter_slot property
            FilterSlotN[0].max = maxFilter;
            if (isSimulation())
                actualSimFilter = FilterSlotN[0].value = 1;
            IUUpdateMinMax(&FilterSlotNP);
            IDSetNumber(&FilterSlotNP, nullptr);

            deleteProperty(FilterNameTP->name);

            if (FilterNameT != nullptr)
                delete [] FilterNameT;
            FilterNameT = new IText[maxFilter];

            for (int i = 0; i < maxFilter; i++)
            {
                snprintf(filterName, MAXINDINAME, "FILTER_SLOT_NAME_%d", i + 1);
                snprintf(filterLabel, MAXINDILABEL, "Filter n° %d", i + 1);
                IUFillText(&FilterNameT[i], filterName, filterLabel, filterNameIFW[i]);
            }

            IUFillTextVector(FilterNameTP, FilterNameT, maxFilter, getDeviceName(), "FILTER_NAME", "Filters", FilterSlotNP.group,
                             IP_RW, 0, IPS_OK);
            defineText(FilterNameTP);

            // filterList only use for purpose information
            // Remove space from filterList
            char *withSpace    = filterList;
            char *withoutSpace = filterList;
            while (*withSpace != '\0')
            {
                if (*withSpace != ' ')
                {
                    *withoutSpace = *withSpace;
                    withoutSpace++;
                }
                withSpace++;
            }
            *withoutSpace = '\0';

            IDSetText(FilterNameTP, "IFW Filters name -> %s", filterList);
            return true;
        }
        else
            DEBUGF(INDI::Logger::DBG_ERROR, "List of filters name is wrong Nbr char red are: %s", lenResponse);
    }

    FilterNameTP->s = IPS_ALERT;
    DEBUG(INDI::Logger::DBG_ERROR, "Failed to read filter names!");
    IDSetText(FilterNameTP, nullptr);
    return false;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::SetFilterNames()
{
    DEBUGTAG();
    bool result = true;
    char cmd[72]={0};
    char tempo[OPTEC_LEN_FLTNAME + 1];
    char response[OPTEC_MAXLEN_RESP + 1];
    int tempolen;
    memset(response, 0, sizeof(response));

    FilterNameTP->s = FilterSlotNP.s = WheelIDTP.s = IPS_BUSY;
    IDSetText(FilterNameTP, "*** Saving filters name to IFW... ***");
    IDSetNumber(&FilterSlotNP, nullptr);
    IDSetText(&WheelIDTP, nullptr);

    snprintf(cmd, 8, "WLOAD%s*", WheelIDT[0].text);

    for (int i = 0; i < FilterSlotN[0].max; i++)
    {
        // Prepare string in tempo with blank space at right to complete to 8 chars for each filter name
        memset(tempo, ' ', sizeof(tempo));
        //Check max len of 8 char for the filter name
        tempolen = strlen(FilterNameT[i].text);
        if (tempolen > OPTEC_LEN_FLTNAME)
            tempolen = OPTEC_LEN_FLTNAME;
        //memcpy(tempo + (8 - tempolen), FilterNameT[i].text, tempolen);    // spaces at begin of name
        memcpy(tempo, FilterNameT[i].text, tempolen); // spaces at the end of name
        tempo[8] = '\0';
        strcat(cmd, tempo);
        strncpy(FilterNameT[i].text, tempo, OPTEC_LEN_FLTNAME);
        FilterNameT[i].text[OPTEC_LEN_FLTNAME] = '\0';

        DEBUGF(INDI::Logger::DBG_DEBUG, "Value of the command :%s", cmd);
        //memset(response, 0, sizeof(tempo));
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Length of the command to write to IFW = %d", strlen(cmd));

    if (isSimulation())
    {
        strncpy(filterSim, cmd + 7, OPTEC_MAXLEN_NAMES);
        filterSim[OPTEC_MAXLEN_NAMES] = '\0';
    }

    if (!WriteTTY(cmd))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        FilterNameTP->s = IPS_ALERT;
        IDSetText(FilterNameTP, nullptr);
        result = false;
        // Have to wait at least 10 ms for EEPROM writing before next command
        // Wait 50 mS to be safe
        usleep(50000);
    }
    else
    {
        if (!ReadTTY(response, (char *)"!", OPTEC_TIMEOUT))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
            result = false;
        }
        else
        {
            if (strncmp(response, "ER=", 3) == 0)
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Error: %s", response);
                PRINT_ER(response);
                result = false;
            }
        }
    }

    if (!result)
    {
        FilterNameTP->s = IPS_ALERT;
        IDSetText(FilterNameTP, "*** UNABLE TO WRITE FILTERS NAME ***");
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Filters name are saved in IFW");

    // Interface not ready before the message "DATA OK" disapear from the display IFW
    for (int i = OPTEC_WAIT_DATA_OK; i > 0; i--)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Please wait for HOME command start... %d", i);
        sleep(1);
    }

    // Do HOME command to load EEProm new names and getFilter to read new value to validate
    FilterNameTP->s = moveHome() ? IPS_OK : IPS_ALERT;
    IDSetText(FilterNameTP, nullptr);

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::GetWheelID()
{
    DEBUGTAG();
    bool result = true;
    char response[OPTEC_MAXLEN_RESP + 1];

    memset(response, 0, sizeof(response));

    WheelIDTP.s = IPS_BUSY;
    IDSetText(&WheelIDTP, nullptr);

    if (!WriteTTY((char *)"WIDENT"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = false;
    }
    else
    {
        if (!ReadTTY(response, (char *)"C", OPTEC_TIMEOUT))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
            result = false;
        }
        else if (strncmp(response, "ER=", 3) == 0)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Get wheel ID error: %s", response);
            PRINT_ER(response);
            result = false;
        }
    }
    if (!result)
    {
        WheelIDTP.s = IPS_ALERT;
        IDSetText(&WheelIDTP, "*** UNABLE TO GET WHEEL ID ***");
        return false;
    }

    WheelIDTP.s = IPS_OK;
    IUSaveText(&WheelIDT[0], response);
    IDSetText(&WheelIDTP, "IFW wheel active is %s", response);

    return true;
}

/************************************************************************************
*
************************************************************************************/
int FilterIFW::GetFilterPos()
{
    DEBUGTAG();
    int result = 1;
    char response[OPTEC_MAXLEN_RESP + 1];
    char filter[2]={0};

    memset(response, 0, sizeof(response));

    FilterSlotNP.s = IPS_BUSY;
    IDSetNumber(&FilterSlotNP, nullptr);

    if (!WriteTTY((char *)"WFILTR"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = -1;
    }
    else
    {
        // actualSimFilter for simulation value. int value need to be char*
        snprintf(filter, 2, "%d", actualSimFilter);

        if (!ReadTTY(response, filter, OPTEC_TIMEOUT))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
            result = -1;
        }
    }

    if (result == -1)
    {
        FilterSlotNP.s = IPS_ALERT;
        IDSetNumber(&FilterSlotNP, "*** UNABLE TO GET ACTIVE FILTER ***");
        return result;
    }

    result               = atoi(response);
    FilterSlotN[0].value = result;
    FilterSlotNP.s       = IPS_OK;
    IDSetNumber(&FilterSlotNP, "IFW filter active is n° %s -> %s", response, FilterNameT[result - 1].text);
    return result;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::moveHome()
{
    DEBUGTAG();
    bool result = true;
    char response[OPTEC_MAXLEN_RESP + 1];

    memset(response, 0, sizeof(response));

    HomeSP.s = WheelIDTP.s = FilterSlotNP.s = IPS_BUSY;
    IDSetSwitch(&HomeSP, "*** Initialisation of the IFW. Please wait... ***");
    IDSetText(&WheelIDTP, nullptr);
    IDSetNumber(&FilterSlotNP, nullptr);

    if (!WriteTTY((char *)"WHOME"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = false;
    }
    else
    {
        if (isSimulation())
            sleep(10); // About the same time as real filter

        if (!ReadTTY(response, (char *)"A", OPTEC_TIMEOUT_WHOME))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read from TTY", __FUNCTION__);
            result = false;
        }
        else
        {
            if (strncmp(response, "ER=", 3) == 0)
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Move to Home error: %s", response);
                PRINT_ER(response);
                result = false;
            }
        }
    }

    if (!result || !GetWheelID() || !GetFilterNames() || (GetFilterPos() <= 0))
    {
        HomeSP.s = WheelIDTP.s = IPS_ALERT;
        IDSetSwitch(&HomeSP, "*** INITIALISATION FAILED ***");
        return false;
    }

    HomeSP.s = WheelIDTP.s = IPS_OK;
    IDSetSwitch(&HomeSP, "IFW ready");
    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::GetFirmware()
{
    DEBUGTAG();
    bool result = true;
    char response[OPTEC_MAXLEN_RESP + 1];

    memset(response, 0, sizeof(response));

    FirmwareTP.s = IPS_BUSY;
    IDSetText(&FirmwareTP, nullptr);

    if (!WriteTTY((char *)"WVAAAA"))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to write to TTY", __FUNCTION__);
        result = false;
    }
    else
    {
        if (!ReadTTY(response, (char *)"V= 2.04", OPTEC_TIMEOUT_FIRMWARE))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "(Function %s()) failed to read to TTY", __FUNCTION__);
            result = false;
        }
        else if (strncmp(response, "ER=", 3) == 0)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Get wheel ID error: %s", response);
            PRINT_ER(response);
            result = false;
        }
    }
    if (!result)
    {
        FirmwareTP.s = IPS_ALERT;
        IDSetText(&FirmwareTP, "*** UNABLE TO GET FIRMWARE ***");
        return false;
    }

    // remove chars fomr the string to get only the nzuméric value of the Firmware version
    char *p = nullptr;

    for (int i = 0; i < (int)strlen(response); i++)
    {
        if (isdigit(response[i]) != 0)
        {
            p = response + i;
            break;
        }
    }

    FirmwareTP.s = IPS_OK;
    IUSaveText(&FirmwareT[0], p);
    IDSetText(&FirmwareTP, "IFW Firmware is %s", response);

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::saveConfigItems(FILE *fp)
{
    INDI::FilterWheel::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &CharSetSP);
    IUSaveConfigSwitch(fp, &FilterNbrSP);

    return true;
}

/************************************************************************************
*
************************************************************************************/
bool FilterIFW::loadConfig(bool silent, const char *property)
{
    bool result;

    if (property == nullptr)
    {
        result = INDI::DefaultDevice::loadConfig(silent, "CHARSET");
        result = (INDI::DefaultDevice::loadConfig(silent, "FILTER_NBR") && result);
        result = (INDI::DefaultDevice::loadConfig(silent, "USEJOYSTICK") && result);
        result = (INDI::DefaultDevice::loadConfig(silent, "JOYSTICKSETTINGS") && result);
    }
    else
        result = INDI::DefaultDevice::loadConfig(silent, property);

    return result;
}
