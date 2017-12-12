#if 0
Simple Client Tutorial
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

/** \file tutorial_client.cpp
    \brief Construct a basic INDI client that demonstrates INDI::Client capabilities. This client must be used with tutorial_three device "Simple CCD".
    \author Jasem Mutlaq

    \example tutorial_client.cpp
    Construct a basic INDI client that demonstrates INDI::Client capabilities. This client must be used with tutorial_three device "Simple CCD".
    To run the example, you must first run tutorial_three:
    \code indiserver tutorial_three \endcode
    Then in another terminal, run the client:
    \code tutorial_client \endcode
    The client will connect to the CCD driver and attempts to change the CCD temperature.
*/

#include "tutorial_client.h"

#include "indibase/basedevice.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

#define MYCCD "Simple CCD"

/* Our client auto pointer */
std::unique_ptr<MyClient> camera_client(new MyClient());

int main(int /*argc*/, char **/*argv*/)
{
    camera_client->setServer("localhost", 7624);

    camera_client->watchDevice(MYCCD);

    camera_client->connectServer();

    camera_client->setBLOBMode(B_ALSO, MYCCD, nullptr);

    std::cout << "Press any key to terminate the client.\n";
    std::string term;
    std::cin >> term;
}

/**************************************************************************************
**
***************************************************************************************/
MyClient::MyClient()
{
    ccd_simulator = nullptr;
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::setTemperature()
{
    INumberVectorProperty *ccd_temperature = nullptr;

    ccd_temperature = ccd_simulator->getNumber("CCD_TEMPERATURE");

    if (ccd_temperature == nullptr)
    {
        IDLog("Error: unable to find CCD Simulator CCD_TEMPERATURE property...\n");
        return;
    }

    ccd_temperature->np[0].value = -20;
    sendNewNumber(ccd_temperature);
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::takeExposure()
{
    INumberVectorProperty *ccd_exposure = nullptr;

    ccd_exposure = ccd_simulator->getNumber("CCD_EXPOSURE");

    if (ccd_exposure == nullptr)
    {
        IDLog("Error: unable to find CCD Simulator CCD_EXPOSURE property...\n");
        return;
    }

    // Take a 1 second exposure
    IDLog("Taking a 1 second exposure.\n");
    ccd_exposure->np[0].value = 1;
    sendNewNumber(ccd_exposure);
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newDevice(INDI::BaseDevice *dp)
{
    if (strcmp(dp->getDeviceName(), MYCCD) == 0)
        IDLog("Receiving %s Device...\n", dp->getDeviceName());

    ccd_simulator = dp;
}

/**************************************************************************************
**
*************************************************************************************/
void MyClient::newProperty(INDI::Property *property)
{
    if (strcmp(property->getDeviceName(), MYCCD) == 0 && strcmp(property->getName(), "CONNECTION") == 0)
    {
        connectDevice(MYCCD);
        return;
    }

    if (strcmp(property->getDeviceName(), MYCCD) == 0 && strcmp(property->getName(), "CCD_TEMPERATURE") == 0)
    {
        if (ccd_simulator->isConnected())
        {
            IDLog("CCD is connected. Setting temperature to -20 C.\n");
            setTemperature();
        }
        return;
    }
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newNumber(INumberVectorProperty *nvp)
{
    // Let's check if we get any new values for CCD_TEMPERATURE
    if (strcmp(nvp->name, "CCD_TEMPERATURE") == 0)
    {
        IDLog("Receving new CCD Temperature: %g C\n", nvp->np[0].value);

        if (nvp->np[0].value == -20)
        {
            IDLog("CCD temperature reached desired value!\n");
            takeExposure();
        }
    }
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newMessage(INDI::BaseDevice *dp, int messageID)
{
    if (strcmp(dp->getDeviceName(), MYCCD) != 0)
        return;

    IDLog("Recveing message from Server:\n\n########################\n%s\n########################\n\n",
          dp->messageQueue(messageID).c_str());
}

/**************************************************************************************
**
***************************************************************************************/
void MyClient::newBLOB(IBLOB *bp)
{
    // Save FITS file to disk
    std::ofstream myfile;

    myfile.open("ccd_simulator.fits", std::ios::out | std::ios::binary);

    myfile.write(static_cast<char *>(bp->blob), bp->bloblen);

    myfile.close();

    IDLog("Received image, saved as ccd_simulator.fits\n");
}
