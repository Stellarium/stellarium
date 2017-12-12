/*
    Dust Cap Interface
    Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indilightboxinterface.h"

#include "indilogger.h"

#include <cstring>

namespace INDI
{

LightBoxInterface::LightBoxInterface(DefaultDevice *device, bool isDimmable)
{
    this->device      = device;
    this->isDimmable  = isDimmable;
    FilterIntensityN  = nullptr;
    currentFilterSlot = 0;
}

LightBoxInterface::~LightBoxInterface()
{
}

void LightBoxInterface::initLightBoxProperties(const char *deviceName, const char *groupName)
{
    // Turn on/off light
    IUFillSwitch(&LightS[FLAT_LIGHT_ON], "FLAT_LIGHT_ON", "On", ISS_OFF);
    IUFillSwitch(&LightS[FLAT_LIGHT_OFF], "FLAT_LIGHT_OFF", "Off", ISS_OFF);
    IUFillSwitchVector(&LightSP, LightS, 2, deviceName, "FLAT_LIGHT_CONTROL", "Flat Light", groupName, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // Light Intensity
    IUFillNumber(&LightIntensityN[0], "FLAT_LIGHT_INTENSITY_VALUE", "Value", "%.f", 0, 255, 10, 0);
    IUFillNumberVector(&LightIntensityNP, LightIntensityN, 1, deviceName, "FLAT_LIGHT_INTENSITY", "Brightness",
                       groupName, IP_RW, 0, IPS_IDLE);

    // Active Devices
    IUFillText(&ActiveDeviceT[0], "ACTIVE_FILTER", "Filter", "Filter Simulator");
    IUFillTextVector(&ActiveDeviceTP, ActiveDeviceT, 1, deviceName, "ACTIVE_DEVICES", "Snoop devices", OPTIONS_TAB,
                     IP_RW, 60, IPS_IDLE);

    // Filter duration
    IUFillNumberVector(&FilterIntensityNP, nullptr, 0, deviceName, "FLAT_LIGHT_FILTER_INTENSITY", "Filter Intensity",
                       "Preset", IP_RW, 60, IPS_OK);

    IDSnoopDevice(ActiveDeviceT[0].text, "FILTER_SLOT");
    IDSnoopDevice(ActiveDeviceT[0].text, "FILTER_NAME");
}

void LightBoxInterface::isGetLightBoxProperties(const char *deviceName)
{
    INDI_UNUSED(deviceName);

    device->defineText(&ActiveDeviceTP);
    char errmsg[MAXRBUF];
    IUReadConfig(nullptr, device->getDeviceName(), "ACTIVE_DEVICES", 1, errmsg);
}

bool LightBoxInterface::updateLightBoxProperties()
{
    if (device->isConnected() == false)
    {
        if (FilterIntensityN)
        {
            device->deleteProperty(FilterIntensityNP.name);
            FilterIntensityNP.nnp = 0;
            delete (FilterIntensityN);
            FilterIntensityN = nullptr;
        }
    }

    return true;
}

bool LightBoxInterface::processLightBoxSwitch(const char *dev, const char *name, ISState *states, char *names[],
                                                    int n)
{
    if (strcmp(dev, device->getDeviceName()) == 0)
    {
        // Light
        if (!strcmp(LightSP.name, name))
        {
            int prevIndex = IUFindOnSwitchIndex(&LightSP);
            IUUpdateSwitch(&LightSP, states, names, n);
            bool rc = EnableLightBox(LightS[FLAT_LIGHT_ON].s == ISS_ON ? true : false);

            LightSP.s = rc ? IPS_OK : IPS_ALERT;

            if (!rc)
            {
                IUResetSwitch(&LightSP);
                LightS[prevIndex].s = ISS_ON;
            }

            IDSetSwitch(&LightSP, nullptr);

            return true;
        }
    }

    return false;
}

bool LightBoxInterface::processLightBoxNumber(const char *dev, const char *name, double values[], char *names[],
                                                    int n)
{
    if (strcmp(dev, device->getDeviceName()) == 0)
    {
        // Light Intensity
        if (!strcmp(LightIntensityNP.name, name))
        {
            double prevValue = LightIntensityN[0].value;
            IUUpdateNumber(&LightIntensityNP, values, names, n);

            bool rc = SetLightBoxBrightness(LightIntensityN[0].value);
            if (rc)
                LightIntensityNP.s = IPS_OK;
            else
            {
                LightIntensityN[0].value = prevValue;
                LightIntensityNP.s       = IPS_ALERT;
            }

            IDSetNumber(&LightIntensityNP, nullptr);

            return true;
        }

        if (!strcmp(FilterIntensityNP.name, name))
        {
            if (FilterIntensityN == nullptr)
            {
                for (int i = 0; i < n; i++)
                    addFilterDuration(names[i], values[i]);

                device->defineNumber(&FilterIntensityNP);

                return true;
            }

            IUUpdateNumber(&FilterIntensityNP, values, names, n);
            FilterIntensityNP.s = IPS_OK;
            IDSetNumber(&FilterIntensityNP, nullptr);

            return true;
        }
    }

    return false;
}

bool LightBoxInterface::processLightBoxText(const char *dev, const char *name, char *texts[], char *names[],
                                                  int n)
{
    if (strcmp(dev, device->getDeviceName()) == 0)
    {
        if (!strcmp(name, ActiveDeviceTP.name))
        {
            ActiveDeviceTP.s = IPS_OK;
            IUUpdateText(&ActiveDeviceTP, texts, names, n);
            //  Update client display
            IDSetText(&ActiveDeviceTP, nullptr);

            IDSnoopDevice(ActiveDeviceT[0].text, "FILTER_SLOT");
            IDSnoopDevice(ActiveDeviceT[0].text, "FILTER_NAME");
            return true;
        }
    }

    return false;
}

bool LightBoxInterface::EnableLightBox(bool enable)
{
    INDI_UNUSED(enable);
    // Must be implemented by child class
    return false;
}

bool LightBoxInterface::SetLightBoxBrightness(uint16_t value)
{
    INDI_UNUSED(value);
    // Must be implemented by child class
    return false;
}

bool LightBoxInterface::snoopLightBox(XMLEle *root)
{
    if (isDimmable == false)
        return false;

    XMLEle *ep           = nullptr;
    const char *propName = findXMLAttValu(root, "name");

    if (FilterIntensityN == nullptr && !strcmp(propName, "FILTER_NAME"))
    {
        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            // If new, add.
            addFilterDuration(pcdataXMLEle(ep), 0);
        }

        device->defineNumber(&FilterIntensityNP);
        char errmsg[MAXRBUF];
        IUReadConfig(nullptr, device->getDeviceName(), "FLAT_LIGHT_FILTER_INTENSITY", 1, errmsg);

        if (device->isConnected())
        {
            if (currentFilterSlot < FilterIntensityNP.nnp)
            {
                double duration = FilterIntensityN[currentFilterSlot].value;
                if (duration > 0)
                    SetLightBoxBrightness(duration);
            }
        }
    }
    else if (!strcmp(propName, "FILTER_SLOT"))
    {
        // Only accept IPS_OK/IPS_IDLE state
        if (strcmp(findXMLAttValu(root, "state"), "Ok") && strcmp(findXMLAttValu(root, "state"), "Idle"))
            return false;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            const char *elemName = findXMLAttValu(ep, "name");

            if (!strcmp(elemName, "FILTER_SLOT_VALUE"))
            {
                currentFilterSlot = atoi(pcdataXMLEle(ep)) - 1;
                break;
            }
        }

        if (FilterIntensityN && device->isConnected())
        {
            if (currentFilterSlot < FilterIntensityNP.nnp)
            {
                double duration = FilterIntensityN[currentFilterSlot].value;
                if (duration > 0)
                    SetLightBoxBrightness(duration);
            }
        }
    }

    return false;
}

void LightBoxInterface::addFilterDuration(const char *filterName, uint16_t filterDuration)
{
    if (FilterIntensityN == nullptr)
    {
        FilterIntensityN = (INumber *)malloc(sizeof(INumber));
        DEBUGDEVICE(device->getDeviceName(), Logger::DBG_SESSION, "Filter intensity preset created.");
    }
    else
    {
        // Ensure no duplicates
        for (int i = 0; i < FilterIntensityNP.nnp + 1; i++)
        {
            if (!strcmp(filterName, FilterIntensityN[i].name))
                return;
        }

        FilterIntensityN = (INumber *)realloc(FilterIntensityN, (FilterIntensityNP.nnp + 1) * sizeof(INumber));
    }

    IUFillNumber(&FilterIntensityN[FilterIntensityNP.nnp], filterName, filterName, "%0.f", 0, LightIntensityN[0].max,
                 LightIntensityN[0].step, filterDuration);

    FilterIntensityNP.nnp++;
    FilterIntensityNP.np = FilterIntensityN;
}

bool LightBoxInterface::saveLightBoxConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &ActiveDeviceTP);
    if (FilterIntensityN != nullptr)
        IUSaveConfigNumber(fp, &FilterIntensityNP);

    return true;
}
}
