/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Watchdog driver.

  The driver expects a heartbeat from the client every X minutes. If no heartbeat
  is received, the driver executes the shutdown procedures.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#include "watchdog.h"
#include "watchdogclient.h"

#include <memory>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

#define POLLMS 1000

// We declare unique pointer to my lovely German Shephard Juli (http://indilib.org/images/juli_tommy.jpg)
std::unique_ptr<WatchDog> goodgirrrl(new WatchDog());

void ISGetProperties(const char *dev)
{
    goodgirrrl->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    goodgirrrl->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    goodgirrrl->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    goodgirrrl->ISNewNumber(dev, name, values, names, n);
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
    goodgirrrl->ISSnoopDevice(root);
}

WatchDog::WatchDog()
{
    setVersion(0, 2);
    setDriverInterface(AUX_INTERFACE);

    watchdogClient = new WatchDogClient();

    watchDogTimer = -1;

    shutdownStage = WATCHDOG_IDLE;
}

WatchDog::~WatchDog()
{
    delete (watchdogClient);
}

const char *WatchDog::getDefaultName()
{
    return (const char *)"WatchDog";
}

bool WatchDog::Connect()
{
    if (HeartBeatN[0].value > 0)
    {
        DEBUGF(INDI::Logger::DBG_SESSION,
               "Watchdog is enabled. Shutdown is triggered after %g minutes of communication loss with the client.",
               HeartBeatN[0].value);
        watchDogTimer = SetTimer(HeartBeatN[0].value * 60 * 1000);
    }
    else
        DEBUG(INDI::Logger::DBG_SESSION, "Watchdog is disabled.");

    return true;
}

bool WatchDog::Disconnect()
{
    if (watchDogTimer > 0)
    {
        RemoveTimer(watchDogTimer);
        DEBUG(INDI::Logger::DBG_SESSION, "Watchdog is disabled.");
    }

    shutdownStage = WATCHDOG_IDLE;

    return true;
}

bool WatchDog::initProperties()
{
    INDI::DefaultDevice::initProperties();

    IUFillNumber(&HeartBeatN[0], "WATCHDOG_HEARTBEAT_VALUE", "Threshold (min)", "%g", 0, 180, 10, 0);
    IUFillNumberVector(&HeartBeatNP, HeartBeatN, 1, getDeviceName(), "WATCHDOG_HEARTBEAT", "Heart beat",
                       MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillText(&SettingsT[0], "INDISERVER_HOST", "indiserver host", "localhost");
    IUFillText(&SettingsT[1], "INDISERVER_PORT", "indiserver port", "7624");
    IUFillText(&SettingsT[2], "SHUTDOWN_SCRIPT", "shutdown script", nullptr);
    IUFillTextVector(&SettingsTP, SettingsT, 3, getDeviceName(), "WATCHDOG_SETTINGS", "Settings", MAIN_CONTROL_TAB,
                     IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&ShutdownProcedureS[PARK_MOUNT], "PARK_MOUNT", "Park Mount", ISS_OFF);
    IUFillSwitch(&ShutdownProcedureS[PARK_DOME], "PARK_DOME", "Park Dome", ISS_OFF);
    IUFillSwitch(&ShutdownProcedureS[EXECUTE_SCRIPT], "EXECUTE_SCRIPT", "Execute Script", ISS_OFF);
    IUFillSwitchVector(&ShutdownProcedureSP, ShutdownProcedureS, 3, getDeviceName(), "WATCHDOG_SHUTDOWN", "Shutdown",
                       MAIN_CONTROL_TAB, IP_RW, ISR_NOFMANY, 60, IPS_IDLE);

    IUFillText(&ActiveDeviceT[ACTIVE_TELESCOPE], "ACTIVE_TELESCOPE", "Telescope", "Telescope Simulator");
    IUFillText(&ActiveDeviceT[ACTIVE_DOME], "ACTIVE_DOME", "Dome", "Dome Simulator");
    IUFillTextVector(&ActiveDeviceTP, ActiveDeviceT, 2, getDeviceName(), "ACTIVE_DEVICES", "Active devices",
                     OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    addDebugControl();

    return true;
}

void WatchDog::ISGetProperties(const char *dev)
{
    //  First we let our parent populate
    DefaultDevice::ISGetProperties(dev);

    defineNumber(&HeartBeatNP);
    defineText(&SettingsTP);
    defineSwitch(&ShutdownProcedureSP);
    defineText(&ActiveDeviceTP);

    // Only load config first time and not on subsequent client connections
    if (watchDogTimer == -1)
        loadConfig(true);

    //watchdogClient->setTelescope(ActiveDeviceT[0].text);
    //watchdogClient->setDome(ActiveDeviceT[1].text);
}

bool WatchDog::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //  first check if it's for our device
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(SettingsTP.name, name))
        {
            IUUpdateText(&SettingsTP, texts, names, n);
            SettingsTP.s = IPS_OK;
            IDSetText(&SettingsTP, nullptr);
            return true;
        }

        if (!strcmp(ActiveDeviceTP.name, name))
        {
            if (watchdogClient->isBusy())
            {
                ActiveDeviceTP.s = IPS_ALERT;
                IDSetText(&ActiveDeviceTP, nullptr);
                DEBUG(INDI::Logger::DBG_ERROR, "Cannot change devices names while shutdown is in progress...");
                return true;
            }

            IUUpdateText(&ActiveDeviceTP, texts, names, n);
            ActiveDeviceTP.s = IPS_OK;
            IDSetText(&ActiveDeviceTP, nullptr);

            //watchdogClient->setTelescope(ActiveDeviceT[0].text);
            //watchdogClient->setDome(ActiveDeviceT[1].text);

            return true;
        }
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool WatchDog::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(HeartBeatNP.name, name))
        {
            double prevHeartBeat = HeartBeatN[0].value;

            if (watchdogClient->isBusy())
            {
                HeartBeatNP.s = IPS_ALERT;
                IDSetNumber(&HeartBeatNP, nullptr);
                DEBUG(INDI::Logger::DBG_ERROR, "Cannot change heart beat while shutdown is in progress...");
                return true;
            }

            IUUpdateNumber(&HeartBeatNP, values, names, n);
            HeartBeatNP.s = IPS_OK;

            if (HeartBeatN[0].value == 0)
                DEBUG(INDI::Logger::DBG_SESSION, "Watchdog is disabled.");
            else
            {
                if (isConnected())
                {
                    if (prevHeartBeat != HeartBeatN[0].value)
                        DEBUGF(INDI::Logger::DBG_SESSION,
                               "Watchdog is enabled. Shutdown is triggered after %g minutes of communication loss with "
                               "the client.",
                               HeartBeatN[0].value);

                    DEBUG(INDI::Logger::DBG_DEBUG, "Received heart beat from client.");

                    RemoveTimer(watchDogTimer);
                    watchDogTimer = SetTimer(HeartBeatN[0].value * 60 * 1000);
                }
                else
                    DEBUG(INDI::Logger::DBG_SESSION, "Watchdog is armed. Please connect to enable it.");
            }

            IDSetNumber(&HeartBeatNP, nullptr);

            return true;
        }
    }

    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool WatchDog::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(ShutdownProcedureSP.name, name))
        {
            IUUpdateSwitch(&ShutdownProcedureSP, states, names, n);

            if (ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_ON &&
                (SettingsT[EXECUTE_SCRIPT].text == nullptr || SettingsT[EXECUTE_SCRIPT].text[0] == '\0'))
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error: shutdown script file is not set.");
                ShutdownProcedureSP.s                = IPS_ALERT;
                ShutdownProcedureS[EXECUTE_SCRIPT].s = ISS_OFF;
            }
            else
                ShutdownProcedureSP.s = IPS_OK;
            IDSetSwitch(&ShutdownProcedureSP, nullptr);
            return true;
        }
    }

    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool WatchDog::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &HeartBeatNP);
    IUSaveConfigText(fp, &SettingsTP);
    IUSaveConfigText(fp, &ActiveDeviceTP);
    IUSaveConfigSwitch(fp, &ShutdownProcedureSP);

    return true;
}

void WatchDog::TimerHit()
{
    // Timer is up, we need to start shutdown procedure

    // If nothing to do, then return
    if (ShutdownProcedureS[PARK_DOME].s == ISS_OFF && ShutdownProcedureS[PARK_MOUNT].s == ISS_OFF &&
        ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_OFF)
        return;

    switch (shutdownStage)
    {
        // Connect to server
        case WATCHDOG_IDLE:

            ShutdownProcedureSP.s = IPS_BUSY;
            IDSetSwitch(&ShutdownProcedureSP, nullptr);

            DEBUG(INDI::Logger::DBG_WARNING, "Warning! Heartbeat threshold timed out, executing shutdown procedure...");

            // No need to start client if only we need to execute the script
            if (ShutdownProcedureS[PARK_MOUNT].s == ISS_OFF && ShutdownProcedureS[PARK_DOME].s == ISS_OFF &&
                ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_ON)
            {
                executeScript();
                break;
            }

            // Watch mount if requied
            if (ShutdownProcedureS[PARK_MOUNT].s == ISS_ON)
                watchdogClient->setMount(ActiveDeviceT[0].text);
            // Watch dome
            if (ShutdownProcedureS[PARK_DOME].s == ISS_ON)
                watchdogClient->setDome(ActiveDeviceT[1].text);

            // Set indiserver host and port
            watchdogClient->setServer(SettingsT[0].text, atoi(SettingsT[1].text));

            DEBUG(INDI::Logger::DBG_DEBUG, "Connecting to INDI server...");

            watchdogClient->connectServer();

            shutdownStage = WATCHDOG_CLIENT_STARTED;

            break;

        case WATCHDOG_CLIENT_STARTED:
            // Check if client is ready
            if (watchdogClient->isConnected())
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "Connected to INDI server %s @ %s", SettingsT[0].text,
                       SettingsT[1].text);

                if (ShutdownProcedureS[PARK_MOUNT].s == ISS_ON)
                    parkMount();
                else if (ShutdownProcedureS[PARK_DOME].s == ISS_ON)
                    parkDome();
                else if (ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_ON)
                    executeScript();
            }
            else
                DEBUG(INDI::Logger::DBG_DEBUG, "Waiting for INDI server connection...");
            break;

        case WATCHDOG_MOUNT_PARKED:
        {
            // check if mount is parked
            IPState mountState = watchdogClient->getMountParkState();

            if (mountState == IPS_OK || mountState == IPS_IDLE)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Mount parked.");

                if (ShutdownProcedureS[PARK_DOME].s == ISS_ON)
                    parkDome();
                else if (ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_ON)
                    executeScript();
                else
                    shutdownStage = WATCHDOG_COMPLETE;
            }
        }
        break;

        case WATCHDOG_DOME_PARKED:
        {
            // check if dome is parked
            IPState domeState = watchdogClient->getDomeParkState();

            if (domeState == IPS_OK || domeState == IPS_IDLE)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Dome parked.");

                if (ShutdownProcedureS[EXECUTE_SCRIPT].s == ISS_ON)
                    executeScript();
                else
                    shutdownStage = WATCHDOG_COMPLETE;
            }
        }
        break;

        case WATCHDOG_COMPLETE:
            DEBUG(INDI::Logger::DBG_SESSION, "Shutdown procedure complete.");
            ShutdownProcedureSP.s = IPS_OK;
            IDSetSwitch(&ShutdownProcedureSP, nullptr);
            watchdogClient->disconnectServer();
            shutdownStage = WATCHDOG_IDLE;
            return;

        case WATCHDOG_ERROR:
            ShutdownProcedureSP.s = IPS_ALERT;
            IDSetSwitch(&ShutdownProcedureSP, nullptr);
            return;
    }

    SetTimer(POLLMS);
}

void WatchDog::parkDome()
{
    if (watchdogClient->parkDome() == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error: Unable to park dome! Shutdown procedure terminated.");
        shutdownStage = WATCHDOG_ERROR;
        return;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Parking dome...");
    shutdownStage = WATCHDOG_DOME_PARKED;
}

void WatchDog::parkMount()
{
    if (watchdogClient->parkMount() == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error: Unable to park mount! Shutdown procedure terminated.");
        shutdownStage = WATCHDOG_ERROR;
        return;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Parking mount...");
    shutdownStage = WATCHDOG_MOUNT_PARKED;
}

void WatchDog::executeScript()
{
    // child
    if (fork() == 0)
    {
        int rc = execlp(SettingsT[EXECUTE_SCRIPT].text, SettingsT[EXECUTE_SCRIPT].text, nullptr);

        if (rc)
            exit(rc);
    }
    // parent
    else
    {
        int statval;
        DEBUGF(INDI::Logger::DBG_SESSION, "Executing script %s...", SettingsT[EXECUTE_SCRIPT].text);
        DEBUGF(INDI::Logger::DBG_SESSION, "Waiting for script with PID %d to complete...", getpid());
        wait(&statval);
        if (WIFEXITED(statval))
        {
            int exit_code = WEXITSTATUS(statval);
            DEBUGF(INDI::Logger::DBG_SESSION, "Script complete with exit code %d", exit_code);

            if (exit_code == 0)
                shutdownStage = WATCHDOG_COMPLETE;
            else
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Error: script %s failed. Shutdown procedure terminated.",
                       SettingsT[EXECUTE_SCRIPT].text);
                shutdownStage = WATCHDOG_ERROR;
                return;
            }
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR,
                   "Error: script %s did not terminate with exit. Shutdown procedure terminated.",
                   SettingsT[EXECUTE_SCRIPT].text);
            shutdownStage = WATCHDOG_ERROR;
            return;
        }
    }
}
