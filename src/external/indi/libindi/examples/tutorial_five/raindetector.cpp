/*
   INDI Developers Manual
   Tutorial #5 - Snooping

   Rain Detector

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file raindetector.cpp
    \brief Construct a rain detector device that the user may operate to raise a rain alert. This rain light property defined by this driver is \e snooped by the Dome driver
           then takes whatever appropiate action to protect the dome.
    \author Jasem Mutlaq
*/

#include "raindetector.h"

#include <memory>
#include <cstring>

std::unique_ptr<RainDetector> rainDetector(new RainDetector());

void ISGetProperties(const char *dev)
{
    rainDetector->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    rainDetector->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    rainDetector->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    rainDetector->ISNewNumber(dev, name, values, names, n);
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
    rainDetector->ISSnoopDevice(root);
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool RainDetector::Connect()
{
    IDMessage(getDeviceName(), "Rain Detector connected successfully!");
    return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool RainDetector::Disconnect()
{
    IDMessage(getDeviceName(), "Rain Detector disconnected successfully!");
    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *RainDetector::getDefaultName()
{
    return "Rain Detector";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool RainDetector::initProperties()
{
    // Must init parent properties first!
    INDI::DefaultDevice::initProperties();

    IUFillLight(&RainL[0], "Status", "", IPS_IDLE);
    IUFillLightVector(&RainLP, RainL, 1, getDeviceName(), "Rain Alert", "", MAIN_CONTROL_TAB, IPS_IDLE);

    IUFillSwitch(&RainS[0], "On", "", ISS_OFF);
    IUFillSwitch(&RainS[1], "Off", "", ISS_OFF);
    IUFillSwitchVector(&RainSP, RainS, 2, getDeviceName(), "Control Rain", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool RainDetector::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineLight(&RainLP);
        defineSwitch(&RainSP);
    }
    else
    // We're disconnected
    {
        deleteProperty(RainLP.name);
        deleteProperty(RainSP.name);
    }

    return true;
}

/********************************************************************************************
** Client is asking us to update a switch
*********************************************************************************************/
bool RainDetector::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, RainSP.name) == 0)
        {
            IUUpdateSwitch(&RainSP, states, names, n);

            if (RainS[0].s == ISS_ON)
            {
                RainL[0].s = IPS_ALERT;
                RainLP.s   = IPS_ALERT;
                IDSetLight(&RainLP, "Alert! Alert! Rain detected!");
            }
            else
            {
                RainL[0].s = IPS_IDLE;
                RainLP.s   = IPS_OK;
                IDSetLight(&RainLP, "Rain threat passed. The skies are clear.");
            }

            RainSP.s = IPS_OK;
            IDSetSwitch(&RainSP, nullptr);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}
