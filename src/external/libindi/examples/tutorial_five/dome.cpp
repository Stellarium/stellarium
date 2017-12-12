/*
   INDI Developers Manual
   Tutorial #5 - Snooping

   Dome

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file dome.cpp
    \brief Construct a dome device that the user may operate to open or close the dome shutter door. This driver is \e snooping on the Rain Detector rain property status.
    If rain property state is alert, we close the dome shutter door if it is open, and we prevent the user from opening it until the rain threat passes.
    \author Jasem Mutlaq

    \example dome.cpp
    The dome driver \e snoops on the rain detector signal and watches whether rain is detected or not. If it is detector and the dome is closed, it performs
    no action, but it also prevents you from opening the dome due to rain. If the dome is open, it will automatically starts closing the shutter. In order
    snooping to work, both drivers must be started by the same indiserver (or chained INDI servers):
    \code indiserver tutorial_dome tutorial_rain \endcode
    The dome driver keeps a copy of RainL light property from the rain driver. This makes it easy to parse the property status once an update from the rain
    driver arrives in the dome driver. Alternatively, you can directly parse the XML root element in ISSnoopDevice(XMLEle *root) to extract the required data.
*/

#include "dome.h"

#include <memory>
#include <cstring>
#include <unistd.h>

std::unique_ptr<Dome> dome(new Dome());

void ISGetProperties(const char *dev)
{
    dome->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    dome->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    dome->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    dome->ISNewNumber(dev, name, values, names, n);
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
    dome->ISSnoopDevice(root);
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool Dome::Connect()
{
    IDMessage(getDeviceName(), "Dome connected successfully!");
    return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool Dome::Disconnect()
{
    IDMessage(getDeviceName(), "Dome disconnected successfully!");
    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *Dome::getDefaultName()
{
    return "Dome";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool Dome::initProperties()
{
    // Must init parent properties first!
    INDI::DefaultDevice::initProperties();

    IUFillSwitch(&ShutterS[0], "Open", "", ISS_ON);
    IUFillSwitch(&ShutterS[1], "Close", "", ISS_OFF);
    IUFillSwitchVector(&ShutterSP, ShutterS, 2, getDeviceName(), "Shutter Door", "", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // We init here the property we wish to "snoop" from the target device
    IUFillLight(&RainL[0], "Status", "", IPS_IDLE);
    // Make sure to set the device name to "Rain Detector" since we are snooping on rain detector device.
    IUFillLightVector(&RainLP, RainL, 1, "Rain Detector", "Rain Alert", "", MAIN_CONTROL_TAB, IPS_IDLE);

    return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool Dome::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineSwitch(&ShutterSP);
        /* Let's listen for Rain Alert property in the device Rain */
        IDSnoopDevice("Rain Detector", "Rain Alert");
    }
    else
        // We're disconnected
        deleteProperty(ShutterSP.name);

    return true;
}

/********************************************************************************************
** Client is asking us to update a switch
*********************************************************************************************/
bool Dome::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, ShutterSP.name) == 0)
        {
            IUUpdateSwitch(&ShutterSP, states, names, n);

            ShutterSP.s = IPS_BUSY;

            if (ShutterS[0].s == ISS_ON)
            {
                if (RainL[0].s == IPS_ALERT)
                {
                    ShutterSP.s   = IPS_ALERT;
                    ShutterS[0].s = ISS_OFF;
                    ShutterS[1].s = ISS_ON;
                    IDSetSwitch(&ShutterSP, "It is raining, cannot open Shutter.");
                    return true;
                }

                IDSetSwitch(&ShutterSP, "Shutter is opening.");
            }
            else
                IDSetSwitch(&ShutterSP, "Shutter is closing.");

            sleep(5);

            ShutterSP.s = IPS_OK;

            if (ShutterS[0].s == ISS_ON)
                IDSetSwitch(&ShutterSP, "Shutter is open.");
            else
                IDSetSwitch(&ShutterSP, "Shutter is closed.");

            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

/********************************************************************************************
** We received snooped property update from rain detector device
*********************************************************************************************/
bool Dome::ISSnoopDevice(XMLEle *root)
{
    IPState old_state = RainL[0].s;

    /* If the "Rain Alert" property gets updated in the Rain device, we will receive a notification. We need to process the new values of Rain Alert and update the local version
       of the property.*/
    if (IUSnoopLight(root, &RainLP) == 0)
    {
        // If the dome is connected and rain is Alert */
        if (RainL[0].s == IPS_ALERT)
        {
            // If dome is open, then close it */
            if (ShutterS[0].s == ISS_ON)
                closeShutter();
            else
                IDMessage(getDeviceName(), "Rain Alert Detected! Dome is already closed.");
        }
        else if (old_state == IPS_ALERT && RainL[0].s != IPS_ALERT)
            IDMessage(getDeviceName(), "Rain threat passed. Opening the dome is now safe.");

        return true;
    }

    return false;
}

/********************************************************************************************
** Close shutter
*********************************************************************************************/
void Dome::closeShutter()
{
    ShutterSP.s = IPS_BUSY;

    IDSetSwitch(&ShutterSP, "Rain Alert! Shutter is closing...");

    sleep(5);

    ShutterS[0].s = ISS_OFF;
    ShutterS[1].s = ISS_ON;

    ShutterSP.s = IPS_OK;

    IDSetSwitch(&ShutterSP, "Shutter is closed.");
}
