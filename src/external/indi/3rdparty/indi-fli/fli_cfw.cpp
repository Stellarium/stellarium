#if 0
    FLI Filter Wheels
    INDI Interface for Finger Lakes Instrument Filter Wheels
    Copyright (C) 2003-2012 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#endif

#include <memory>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "indidevapi.h"
#include "eventloop.h"

#include "fli_cfw.h"

#define POLLMS 1000 /* Polling time (ms) */

std::unique_ptr<FLICFW> fliCFW(new FLICFW());

const flidomain_t Domains[] = { FLIDOMAIN_USB, FLIDOMAIN_SERIAL, FLIDOMAIN_PARALLEL_PORT, FLIDOMAIN_INET };

void ISGetProperties(const char *dev)
{
    fliCFW->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    fliCFW->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    fliCFW->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    fliCFW->ISNewNumber(dev, name, values, names, num);
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
    fliCFW->ISSnoopDevice(root);
}

FLICFW::FLICFW()
{
    sim = false;
}

FLICFW::~FLICFW()
{
}

const char *FLICFW::getDefaultName()
{
    return (char *)"FLI CFW";
}

bool FLICFW::initProperties()
{
    // Init parent properties first
    INDI::FilterWheel::initProperties();

    IUFillSwitch(&PortS[0], "USB", "USB", ISS_ON);
    IUFillSwitch(&PortS[1], "SERIAL", "Serial", ISS_OFF);
    IUFillSwitch(&PortS[2], "PARALLEL", "Parallel", ISS_OFF);
    IUFillSwitch(&PortS[3], "INET", "INet", ISS_OFF);
    IUFillSwitchVector(&PortSP, PortS, 4, getDeviceName(), "PORTS", "Port", MAIN_CONTROL_TAB, IP_WO, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillText(&FilterInfoT[0], "Model", "", "");
    IUFillText(&FilterInfoT[1], "HW Rev", "", "");
    IUFillText(&FilterInfoT[2], "FW Rev", "", "");
    IUFillTextVector(&FilterInfoTP, FilterInfoT, 3, getDeviceName(), "Model", "", "Filter Info", IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&FilterS[0], "FILTER_CW", "+", ISS_OFF);
    IUFillSwitch(&FilterS[1], "FILTER_CCW", "-", ISS_OFF);
    IUFillSwitchVector(&FilterSP, FilterS, 2, getDeviceName(), "FILTER_WHEEL_MOTION", "Turn Wheel", FILTER_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);
    return true;
}

void FLICFW::ISGetProperties(const char *dev)
{
    INDI::FilterWheel::ISGetProperties(dev);

    defineSwitch(&PortSP);

    addAuxControls();
}

bool FLICFW::updateProperties()
{
    INDI::FilterWheel::updateProperties();

    if (isConnected())
    {
        defineSwitch(&FilterSP);
        defineText(&FilterInfoTP);

        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(FilterSP.name);
        deleteProperty(FilterInfoTP.name);
        rmTimer(timerID);
    }

    return true;
}

bool FLICFW::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        /* Ports */
        if (!strcmp(name, PortSP.name))
        {
            if (IUUpdateSwitch(&PortSP, states, names, n) < 0)
                return false;

            PortSP.s = IPS_OK;
            IDSetSwitch(&PortSP, NULL);
            return true;
        }

        /* Filter Wheel */
        if (!strcmp(name, FilterSP.name))
        {
            if (IUUpdateSwitch(&FilterSP, states, names, n) < 0)
                return false;
            turnWheel();
            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::FilterWheel::ISNewSwitch(dev, name, states, names, n);
}

bool FLICFW::Connect()
{
    int err = 0;

    IDMessage(getDeviceName(), "Attempting to find the FLI CFW...");

    sim = isSimulation();

    if (sim)
    {
        setupParams();
        return true;
    }

    if (isDebug())
    {
        IDLog("Connecting CFW\n");
        IDLog("Attempting to find the filter wheel\n");
    }

    int portSwitchIndex = IUFindOnSwitchIndex(&PortSP);

    if (findFLICFW(Domains[portSwitchIndex]) == false)
    {
        IDMessage(getDeviceName(), "Error: no filter wheels were detected.");

        if (isDebug())
            IDLog("Error: no filter wheels were detected.\n");

        return false;
    }

    if ((err = FLIOpen(&fli_dev, FLIFilter.name, FLIDEVICE_FILTERWHEEL | FLIFilter.domain)))
    {
        IDMessage(getDeviceName(), "Error: FLIOpen() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("Error: FLIOpen() failed. %s.\n", strerror((int)-err));

        return false;
    }

    /* Success! */
    IDMessage(getDeviceName(), "Filter wheel is online. Retrieving basic data.");
    if (isDebug())
        IDLog("Filter wheel is online. Retrieving basic data.\n");

    setupParams();

    return true;
}

bool FLICFW::Disconnect()
{
    int err;

    if (sim)
        return true;

    if ((err = FLIClose(fli_dev)))
    {
        IDMessage(getDeviceName(), "Error: FLIClose() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("Error: FLIClose() failed. %s.\n", strerror((int)-err));

        return false;
    }

    IDMessage(getDeviceName(), "Filter wheel is offline.");
    return true;
}

bool FLICFW::setupParams()
{
    int err = 0;

    if (isDebug())
        IDLog("In setupParams\n");

    char hw_rev[16], fw_rev[16];

    ////////////////////////////
    // 1. Get Filter wheels Model
    ////////////////////////////
    if (!sim && (err = FLIGetModel(fli_dev, FLIFilter.model, 32)))
    {
        IDMessage(getDeviceName(), "FLIGetModel() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetModel() failed. %s.\n", strerror((int)-err));
        return false;
    }

    if (sim)
        IUSaveText(&FilterInfoT[0], getDeviceName());
    else
        IUSaveText(&FilterInfoT[0], FLIFilter.model);

    ///////////////////////////
    // 2. Get Hardware revision
    ///////////////////////////
    if (sim)
        FLIFilter.HWRevision = 1;
    else if ((err = FLIGetHWRevision(fli_dev, &FLIFilter.HWRevision)))
    {
        IDMessage(getDeviceName(), "FLIGetHWRevision() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetHWRevision() failed. %s.\n", strerror((int)-err));

        return false;
    }

    snprintf(hw_rev, 16, "%ld", FLIFilter.HWRevision);
    IUSaveText(&FilterInfoT[1], hw_rev);

    ///////////////////////////
    // 3. Get Firmware revision
    ///////////////////////////
    if (sim)
        FLIFilter.FWRevision = 1;
    else if ((err = FLIGetFWRevision(fli_dev, &FLIFilter.FWRevision)))
    {
        IDMessage(getDeviceName(), "FLIGetFWRevision() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetFWRevision() failed. %s.\n", strerror((int)-err));

        return false;
    }

    snprintf(fw_rev, 16, "%ld", FLIFilter.FWRevision);
    IUSaveText(&FilterInfoT[2], fw_rev);

    IDSetText(&FilterInfoTP, NULL);
    ///////////////////////////
    // 4. Filter position
    ///////////////////////////

    if (sim)
        FLIFilter.current_pos = 0;
    else
    {
        // on first contact fliter wheel reports position -1
        // to avoid wrong number presented in client dialog:
        SelectFilter(FilterSlotN[0].min);

        if ((err = FLIGetFilterPos(fli_dev, &FLIFilter.current_pos)))
        {
            IDMessage(getDeviceName(), "FLIGetFilterPos() failed. %s.", strerror((int)-err));

            if (isDebug())
                IDLog("FLIGetFilterPos() failed. %s.\n", strerror((int)-err));
            return false;
        }
    }

    ///////////////////////////
    // 5. filter max limit
    ///////////////////////////
    if (sim)
        FLIFilter.count = 5; // 5 slot wheel
    else if ((err = FLIGetFilterCount(fli_dev, &FLIFilter.count)))
    {
        IDMessage(getDeviceName(), "FLIGetFilterCount() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetFilterCount() failed. %s.\n", strerror((int)-err));
        return false;
    }

    FilterSlotN[0].min   = 1;
    FilterSlotN[0].max   = FLIFilter.count;
    FilterSlotN[0].value = FLIFilter.current_pos + 1;
    IUUpdateMinMax(&FilterSlotNP);

    return true;
}

bool FLICFW::SelectFilter(int targetFilter)
{
    int err     = 0;
    long filter = targetFilter - 1;

    if (isDebug())
        IDLog("Requested filter position is %ld\n", filter);

    if (sim)
    {
        FLIFilter.current_pos = filter;
    }
    else if ((err = FLISetFilterPos(fli_dev, filter)))
    {
        IDMessage(getDeviceName(), "FLIGetFilterCount() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetFilterCount() failed. %s.\n", strerror((int)-err));
        return false;
    }

    FLIFilter.current_pos = filter;

    SelectFilterDone(targetFilter);

    return true;
}

int FLICFW::QueryFilter()
{
    int err = 0;
    long newFilter;

    if (sim)
        newFilter = FLIFilter.current_pos;
    else if ((err = FLIGetFilterPos(fli_dev, &newFilter)))
    {
        IDMessage(getDeviceName(), "FLIGetFilterPos() failed. %s.", strerror((int)-err));

        if (isDebug())
            IDLog("FLIGetFilterPos() failed. %s.\n", strerror((int)-err));
        return false;
    }

    if (isDebug())
        IDLog("Current filter position is %ld\n", newFilter + 1);

    return newFilter + 1;
}

void FLICFW::turnWheel()
{
    long current_filter = FLIFilter.current_pos + 1;

    if (current_filter > FLIFilter.count)
        current_filter = FLIFilter.count;

    if (isDebug())
        IDLog("Current Filter Pos: %ld\n", current_filter);

    switch (FilterS[0].s)
    {
        case ISS_ON:
            if (current_filter < FilterSlotN[0].max)
                current_filter++;
            else
                current_filter = FilterSlotN[0].min;
            break;

        case ISS_OFF:
            if (current_filter > FilterSlotN[0].min)
                current_filter--;
            else
                current_filter = FilterSlotN[0].max;
            break;
    }

    if (isDebug())
        IDLog("Target Filter Pos: %ld\n", current_filter);

    SelectFilter(current_filter);

    IUResetSwitch(&FilterSP);
    FilterSP.s = IPS_OK;
    IDSetSwitch(&FilterSP, NULL);
}

void FLICFW::TimerHit()
{
}

bool FLICFW::findFLICFW(flidomain_t domain)
{
    char **names;
    long err;

    if (isDebug())
        IDLog("In find Filter wheel, the domain is %ld\n", domain);

    if ((err = FLIList(domain | FLIDEVICE_FILTERWHEEL, &names)))
    {
        if (isDebug())
            IDLog("FLIList() failed. %s\n", strerror((int)-err));
        return false;
    }

    if (names != NULL && names[0] != NULL)
    {
        for (int i = 0; names[i] != NULL; i++)
        {
            for (int j = 0; names[i][j] != '\0'; j++)
                if (names[i][j] == ';')
                {
                    names[i][j] = '\0';
                    break;
                }
        }

        FLIFilter.domain = domain;

        switch (domain)
        {
            case FLIDOMAIN_PARALLEL_PORT:
                FLIFilter.dname = strdup("parallel port");
                break;

            case FLIDOMAIN_USB:
                FLIFilter.dname = strdup("USB");
                break;

            case FLIDOMAIN_SERIAL:
                FLIFilter.dname = strdup("serial");
                break;

            case FLIDOMAIN_INET:
                FLIFilter.dname = strdup("inet");
                break;

            default:
                FLIFilter.dname = strdup("Unknown domain");
        }

        FLIFilter.name = strdup(names[0]);

        if ((err = FLIFreeList(names)))
        {
            if (isDebug())
                IDLog("FLIFreeList() failed. %s.\n", strerror((int)-err));
            return false;
        }

    } /* end if */
    else
    {
        if ((err = FLIFreeList(names)))
        {
            if (isDebug())
                IDLog("FLIFreeList() failed. %s.\n", strerror((int)-err));
            return false;
        }

        return false;
    }

    if (isDebug())
        IDLog("FindFLICFW() finished successfully.\n");

    return true;
}
