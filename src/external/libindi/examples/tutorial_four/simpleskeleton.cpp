#if 0
Simple Skeleton - Tutorial Four
Demonstration of libindi v0.7 capabilities.

Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

#endif

/** \file simpleskeleton.cpp
    \brief Construct a basic INDI CCD device that demonstrates ability to define properties from a skeleton file.
    \author Jasem Mutlaq

    \example simpleskeleton.cpp
    A skeleton file is an external XML file with the driver properties already defined. This tutorial illustrates how to create a driver from
    a skeleton file and parse/process the properties. The skeleton file name is tutorial_four_sk.xml
    \note Please note that if you create your own skeleton file, you must append _sk postfix to your skeleton file name.
*/

#include "simpleskeleton.h"

#include <cstdlib>
#include <cstring>
#include <memory>

#include <sys/stat.h>

/* Our simpleSkeleton auto pointer */
std::unique_ptr<SimpleSkeleton> simpleSkeleton(new SimpleSkeleton());

//const int POLLMS = 1000; // Period of update, 1 second.

/**************************************************************************************
**
***************************************************************************************/
void ISGetProperties(const char *dev)
{
    simpleSkeleton->ISGetProperties(dev);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    simpleSkeleton->ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    simpleSkeleton->ISNewText(dev, name, texts, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    simpleSkeleton->ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    simpleSkeleton->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}
/**************************************************************************************
**
***************************************************************************************/
void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

/**************************************************************************************
** Initialize all properties & set default values.
**************************************************************************************/
bool SimpleSkeleton::initProperties()
{
    DefaultDevice::initProperties();

    // This is the default driver skeleton file location
    // Convention is: drivername_sk_xml
    // Default location is /usr/share/indi
    const char *skelFileName = "/usr/share/indi/tutorial_four_sk.xml";
    struct stat st;

    char *skel = getenv("INDISKEL");

    if (skel != nullptr)
        buildSkeleton(skel);
    else if (stat(skelFileName, &st) == 0)
        buildSkeleton(skelFileName);
    else
        IDLog(
            "No skeleton file was specified. Set environment variable INDISKEL to the skeleton path and try again.\n");

    // Optional: Add aux controls for configuration, debug & simulation that get added in the Options tab
    //           of the driver.
    addAuxControls();

    std::vector<INDI::Property *> *pAll = getProperties();

    // Let's print a list of all device properties
    for (int i = 0; i < (int)pAll->size(); i++)
        IDLog("Property #%d: %s\n", i, pAll->at(i)->getName());

    return true;
}

/**************************************************************************************
** Define Basic properties to clients.
***************************************************************************************/
void SimpleSkeleton::ISGetProperties(const char *dev)
{
    static int configLoaded = 0;

    // Ask the default driver first to send properties.
    INDI::DefaultDevice::ISGetProperties(dev);

    // If no configuration is load before, then load it now.
    if (configLoaded == 0)
    {
        loadConfig();
        configLoaded = 1;
    }
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
bool SimpleSkeleton::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    INDI_UNUSED(name);
    INDI_UNUSED(texts);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
    // Ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    return false;
}

/**************************************************************************************
**
***************************************************************************************/
bool SimpleSkeleton::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    INumberVectorProperty *nvp = getNumber(name);

    if (nvp == nullptr)
        return false;

    if (!isConnected())
    {
        nvp->s = IPS_ALERT;
        IDSetNumber(nvp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (strcmp(nvp->name, "Number Property") != 0)
    {
        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, nullptr);

        return true;
    }

    return false;
}

/**************************************************************************************
**
***************************************************************************************/
bool SimpleSkeleton::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int lightState = 0;
    int lightIndex = 0;

    // ignore if not ours
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    if (INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n))
        return true;

    ISwitchVectorProperty *svp = getSwitch(name);
    ILightVectorProperty *lvp  = getLight("Light Property");

    if (!isConnected())
    {
        svp->s = IPS_ALERT;
        IDSetSwitch(svp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (svp == nullptr || lvp == nullptr)
        return false;

    if (strcmp(svp->name, "Menu") == 0)
    {
        IUUpdateSwitch(svp, states, names, n);
        ISwitch *onSW = IUFindOnSwitch(svp);
        lightIndex    = IUFindOnSwitchIndex(svp);

        if (lightIndex < 0 || lightIndex > lvp->nlp)
            return false;

        if (onSW != nullptr)
        {
            lightState            = rand() % 4;
            svp->s                = IPS_OK;
            lvp->s                = IPS_OK;
            lvp->lp[lightIndex].s = (IPState)lightState;

            IDSetSwitch(svp, "Setting to switch %s is successful. Changing corresponding light property to %s.",
                        onSW->name, pstateStr(lvp->lp[lightIndex].s));
            IDSetLight(lvp, nullptr);
        }
        return true;
    }

    return false;
}

bool SimpleSkeleton::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                               char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return false;

    IBLOBVectorProperty *bvp = getBLOB(name);

    if (bvp == nullptr)
        return false;

    if (!isConnected())
    {
        bvp->s = IPS_ALERT;
        IDSetBLOB(bvp, "Cannot change property while device is disconnected.");
        return false;
    }

    if (strcmp(bvp->name, "BLOB Test") == 0)
    {
        IUUpdateBLOB(bvp, sizes, blobsizes, blobs, formats, names, n);

        IBLOB *bp = IUFindBLOB(bvp, names[0]);

        if (bp == nullptr)
            return false;

        IDLog("Received BLOB with name %s, format %s, and size %d, and bloblen %d\n", bp->name, bp->format, bp->size,
              bp->bloblen);

        char *blobBuffer = new char[bp->bloblen + 1];
        strncpy(blobBuffer, ((char *)bp->blob), bp->bloblen);
        blobBuffer[bp->bloblen] = '\0';

        IDLog("BLOB Content:\n##################################\n%s\n##################################\n",
              blobBuffer);

        delete[] blobBuffer;

        bp->size = 0;
        bvp->s   = IPS_OK;
        IDSetBLOB(bvp, nullptr);
    }

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SimpleSkeleton::Connect()
{
    return true;
}

bool SimpleSkeleton::Disconnect()
{
    return true;
}

const char *SimpleSkeleton::getDefaultName()
{
    return "Simple Skeleton";
}
