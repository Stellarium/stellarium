/*******************************************************************************
  Copyright(c) 2011 Jasem Mutlaq. All rights reserved.

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

#include "defaultdevice.h"
#include "indicom.h"
#include "indistandardproperty.h"
#include "connectionplugins/connectionserial.h"

#include <cstdlib>
#include <cstring>
#include <assert.h>

const char *COMMUNICATION_TAB = "Communication";
const char *MAIN_CONTROL_TAB  = "Main Control";
const char *CONNECTION_TAB    = "Connection";
const char *MOTION_TAB        = "Motion Control";
const char *DATETIME_TAB      = "Date/Time";
const char *SITE_TAB          = "Site Management";
const char *OPTIONS_TAB       = "Options";
const char *FILTER_TAB        = "Filter Wheel";
const char *FOCUS_TAB         = "Focuser";
const char *GUIDE_TAB         = "Guide";
const char *ALIGNMENT_TAB     = "Alignment";
const char *INFO_TAB          = "General Info";

void timerfunc(void *t)
{
    //fprintf(stderr,"Got a timer hit with %x\n",t);
    INDI::DefaultDevice *devPtr = static_cast<INDI::DefaultDevice *>(t);
    if (devPtr != nullptr)
    {
        //  this was for my device
        //  but we dont have a way of telling
        //  WHICH timer was hit :(
        devPtr->TimerHit();
    }
    return;
}

namespace INDI
{

DefaultDevice::DefaultDevice()
{
    pDebug      = false;
    pSimulation = false;
    isInit      = false;

    majorVersion        = 1;
    minorVersion        = 0;
    interfaceDescriptor = GENERAL_INTERFACE;
    memset(&ConnectionModeSP, 0, sizeof(ConnectionModeSP));
}

DefaultDevice::~DefaultDevice()
{
}

bool DefaultDevice::loadConfig(bool silent, const char *property)
{
    char errmsg[MAXRBUF];
    bool pResult = IUReadConfig(nullptr, deviceID, property, silent ? 1 : 0, errmsg) == 0 ? true : false;

    if (!silent)
    {
        if (pResult)
        {
            LOG_DEBUG("Configuration successfully loaded.");
        }
        else
            LOG_INFO("No previous configuration found. To save driver configuration, click Save Configuration in Options tab");
    }

    IUSaveDefaultConfig(nullptr, nullptr, deviceID);

    return pResult;
}

bool DefaultDevice::saveConfigItems(FILE *fp)
{
    IUSaveConfigSwitch(fp, &DebugSP);
    IUSaveConfigNumber(fp, &PollPeriodNP);
    if (ConnectionModeS != nullptr)
        IUSaveConfigSwitch(fp, &ConnectionModeSP);

    if (activeConnection)
        activeConnection->saveConfigItems(fp);

    return INDI::Logger::saveConfigItems(fp);
}

bool DefaultDevice::saveAllConfigItems(FILE *fp)
{
    std::vector<INDI::Property *>::iterator orderi;

    ISwitchVectorProperty *svp = nullptr;
    INumberVectorProperty *nvp = nullptr;
    ITextVectorProperty *tvp   = nullptr;
    IBLOBVectorProperty *bvp   = nullptr;

    for (orderi = pAll.begin(); orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr  = (*orderi)->getProperty();

        switch (pType)
        {
            case INDI_NUMBER:
                nvp = static_cast<INumberVectorProperty *>(pPtr);
                //IDLog("Trying to save config for number %s\n", nvp->name);
                IUSaveConfigNumber(fp, nvp);
                break;
            case INDI_TEXT:
                tvp = static_cast<ITextVectorProperty *>(pPtr);
                IUSaveConfigText(fp, tvp);
                break;
            case INDI_SWITCH:
                svp = static_cast<ISwitchVectorProperty *>(pPtr);
                /* Never save CONNECTION property. Don't save switches with no switches on if the rule is one of many */
                if (!strcmp(svp->name, INDI::SP::CONNECTION) || (svp->r == ISR_1OFMANY && !IUFindOnSwitch(svp)))
                    continue;
                IUSaveConfigSwitch(fp, svp);
                break;
            case INDI_BLOB:
                bvp = static_cast<IBLOBVectorProperty *>(pPtr);
                IUSaveConfigBLOB(fp, bvp);
                break;
            case INDI_LIGHT:
            case INDI_UNKNOWN:
                break;
        }
    }
    return true;
}

bool DefaultDevice::purgeConfig()
{
    char errmsg[MAXRBUF];
    if (IUPurgeConfig(nullptr, deviceID, errmsg) == -1)
    {
        LOGF_WARN("%s", errmsg);
        return false;
    }

    LOG_INFO("Configuration file successfully purged.");
    return true;
}

bool DefaultDevice::saveConfig(bool silent, const char *property)
{
    //std::vector<orderPtr>::iterator orderi;
    char errmsg[MAXRBUF];

    FILE *fp = nullptr;

    if (property == nullptr)
    {
        fp = IUGetConfigFP(nullptr, deviceID, "w", errmsg);

        if (fp == nullptr)
        {
            if (!silent)
                LOGF_WARN("Failed to save configuration. %s", errmsg);
            return false;
        }

        IUSaveConfigTag(fp, 0, getDeviceName(), silent ? 1 : 0);

        saveConfigItems(fp);

        IUSaveConfigTag(fp, 1, getDeviceName(), silent ? 1 : 0);

        fclose(fp);

        IUSaveDefaultConfig(nullptr, nullptr, deviceID);

        LOG_DEBUG("Configuration successfully saved.");
    }
    else
    {
        fp = IUGetConfigFP(nullptr, deviceID, "r", errmsg);

        if (fp == nullptr)
        {
            //if (!silent)
            //   LOGF_ERROR("Error saving configuration. %s", errmsg);
            //return false;
            // If we don't have an existing file pointer, save all properties.
            return saveConfig(silent);
        }

        LilXML *lp   = newLilXML();
        XMLEle *root = readXMLFile(fp, lp, errmsg);

        fclose(fp);
        delLilXML(lp);

        if (root == nullptr)
            return false;

        XMLEle *ep         = nullptr;
        bool propertySaved = false;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            const char *elemName = findXMLAttValu(ep, "name");
            const char *tagName  = tagXMLEle(ep);

            if (strcmp(elemName, property))
                continue;

            if (!strcmp(tagName, "newSwitchVector"))
            {
                ISwitchVectorProperty *svp = getSwitch(elemName);
                if (svp == nullptr)
                {
                    delXMLEle(root);
                    return false;
                }

                XMLEle *sw = nullptr;
                for (sw = nextXMLEle(ep, 1); sw != nullptr; sw = nextXMLEle(ep, 0))
                {
                    ISwitch *oneSwitch = IUFindSwitch(svp, findXMLAttValu(sw, "name"));
                    if (oneSwitch == nullptr)
                    {
                        delXMLEle(root);
                        return false;
                    }
                    char formatString[MAXRBUF];
                    snprintf(formatString, MAXRBUF, "      %s\n", sstateStr(oneSwitch->s));
                    editXMLEle(sw, formatString);
                }

                propertySaved = true;
                break;
            }
            else if (!strcmp(tagName, "newNumberVector"))
            {
                INumberVectorProperty *nvp = getNumber(elemName);
                if (nvp == nullptr)
                {
                    delXMLEle(root);
                    return false;
                }

                XMLEle *np = nullptr;
                for (np = nextXMLEle(ep, 1); np != nullptr; np = nextXMLEle(ep, 0))
                {
                    INumber *oneNumber = IUFindNumber(nvp, findXMLAttValu(np, "name"));
                    if (oneNumber == nullptr)
                        return false;

                    char formatString[MAXRBUF];
                    snprintf(formatString, MAXRBUF, "      %.20g\n", oneNumber->value);
                    editXMLEle(np, formatString);
                }

                propertySaved = true;
                break;
            }
            else if (!strcmp(tagName, "newTextVector"))
            {
                ITextVectorProperty *tvp = getText(elemName);
                if (tvp == nullptr)
                {
                    delXMLEle(root);
                    return false;
                }

                XMLEle *tp = nullptr;
                for (tp = nextXMLEle(ep, 1); tp != nullptr; tp = nextXMLEle(ep, 0))
                {
                    IText *oneText = IUFindText(tvp, findXMLAttValu(tp, "name"));
                    if (oneText == nullptr)
                        return false;

                    char formatString[MAXRBUF];
                    snprintf(formatString, MAXRBUF, "      %s\n", oneText->text ? oneText->text : "");
                    editXMLEle(tp, formatString);
                }

                propertySaved = true;
                break;
            }
        }

        if (propertySaved)
        {
            fp = IUGetConfigFP(nullptr, deviceID, "w", errmsg);
            prXMLEle(fp, root, 0);
            fclose(fp);
            delXMLEle(root);
            LOGF_DEBUG("Configuration successfully saved for %s.", property);
            return true;
        }
        else
        {
            delXMLEle(root);
            // If property does not exist, save the whole thing
            return saveConfig(silent);
        }
    }

    return true;
}

bool DefaultDevice::loadDefaultConfig()
{
    char configDefaultFileName[MAXRBUF];
    char errmsg[MAXRBUF];
    bool pResult = false;

    if (getenv("INDICONFIG"))
        snprintf(configDefaultFileName, MAXRBUF, "%s.default", getenv("INDICONFIG"));
    else
        snprintf(configDefaultFileName, MAXRBUF, "%s/.indi/%s_config.xml.default", getenv("HOME"), deviceID);

    LOGF_DEBUG("Requesting to load default config with: %s", configDefaultFileName);

    pResult = IUReadConfig(configDefaultFileName, deviceID, nullptr, 0, errmsg) == 0 ? true : false;

    if (pResult)
        LOG_INFO("Default configuration loaded.");
    else
        LOGF_INFO("Error loading default configuraiton. %s", errmsg);

    return pResult;
}

bool DefaultDevice::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    // ignore if not ours //
    if (strcmp(dev, deviceID))
        return false;

    ISwitchVectorProperty *svp = getSwitch(name);

    if (!svp)
        return false;

    ////////////////////////////////////////////////////
    // Connection
    ////////////////////////////////////////////////////
    if (!strcmp(svp->name, ConnectionSP.name))
    {
        bool rc = false;

        for (int i = 0; i < n; i++)
        {
            if (!strcmp(names[i], "CONNECT") && (states[i] == ISS_ON))
            {
                // If disconnected, try to connect.
                if (isConnected() == false)
                {
                    rc = Connect();

                    if (rc)
                    {
                        // Connection is successful, set it to OK and updateProperties.
                        setConnected(true, IPS_OK);
                        updateProperties();
                    }
                    else
                        setConnected(false, IPS_ALERT);
                }
                else
                    // Already connected, tell client we're connected already.
                    setConnected(true);
            }
            else if (!strcmp(names[i], "DISCONNECT") && (states[i] == ISS_ON))
            {
                // If connected, try to disconnect.
                if (isConnected() == true)
                {
                    rc = Disconnect();
                    // Disconnection is successful, set it IDLE and updateProperties.
                    if (rc)
                    {
                        setConnected(false, IPS_IDLE);
                        updateProperties();
                    }
                    else
                        setConnected(true, IPS_ALERT);
                }
                // Already disconnected, tell client we're disconnected already.
                else
                    setConnected(false, IPS_IDLE);
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////
    // Connection Mode
    ////////////////////////////////////////////////////
    if (!strcmp(name, ConnectionModeSP.name))
    {
        IUUpdateSwitch(&ConnectionModeSP, states, names, n);

        int activeConnectionIndex = IUFindOnSwitchIndex(&ConnectionModeSP);

        if (activeConnectionIndex >= 0 && activeConnectionIndex < static_cast<int>(connections.size()))
        {
            activeConnection = connections[activeConnectionIndex];
            activeConnection->Activated();

            for (Connection::Interface *oneConnection : connections)
            {
                if (oneConnection == activeConnection)
                    continue;

                oneConnection->Deactivated();
            }

            ConnectionModeSP.s = IPS_OK;
        }
        else
            ConnectionModeSP.s = IPS_ALERT;

        IDSetSwitch(&ConnectionModeSP, nullptr);

        return true;
    }

    ////////////////////////////////////////////////////
    // Debug
    ////////////////////////////////////////////////////
    if (!strcmp(svp->name, "DEBUG"))
    {
        IUUpdateSwitch(svp, states, names, n);
        ISwitch *sp = IUFindOnSwitch(svp);

        assert(sp != nullptr);

        if (!strcmp(sp->name, "ENABLE"))
            setDebug(true);
        else
            setDebug(false);

        return true;
    }

    ////////////////////////////////////////////////////
    // Simulation
    ////////////////////////////////////////////////////
    if (!strcmp(svp->name, "SIMULATION"))
    {
        IUUpdateSwitch(svp, states, names, n);
        ISwitch *sp = IUFindOnSwitch(svp);

        assert(sp != nullptr);

        if (!strcmp(sp->name, "ENABLE"))
            setSimulation(true);
        else
            setSimulation(false);
        return true;
    }

    ////////////////////////////////////////////////////
    // Configuration
    ////////////////////////////////////////////////////
    if (!strcmp(svp->name, "CONFIG_PROCESS"))
    {
        IUUpdateSwitch(svp, states, names, n);
        ISwitch *sp = IUFindOnSwitch(svp);
        IUResetSwitch(svp);
        bool pResult = false;

        // Not suppose to happen (all switches off) but let's handle it anyway
        if (sp == nullptr)
        {
            svp->s = IPS_IDLE;
            IDSetSwitch(svp, nullptr);
            return true;
        }

        if (!strcmp(sp->name, "CONFIG_LOAD"))
            pResult = loadConfig();
        else if (!strcmp(sp->name, "CONFIG_SAVE"))
            pResult = saveConfig();
        else if (!strcmp(sp->name, "CONFIG_DEFAULT"))
            pResult = loadDefaultConfig();
        else if (!strcmp(sp->name, "CONFIG_PURGE"))
            pResult = purgeConfig();

        if (pResult)
            svp->s = IPS_OK;
        else
            svp->s = IPS_ALERT;

        IDSetSwitch(svp, nullptr);
        return true;
    }

    ////////////////////////////////////////////////////
    // Debugging and Logging Levels
    ////////////////////////////////////////////////////
    if (!strcmp(svp->name, "DEBUG_LEVEL") || !strcmp(svp->name, "LOGGING_LEVEL") || !strcmp(svp->name, "LOG_OUTPUT"))
    {
        bool rc = Logger::ISNewSwitch(dev, name, states, names, n);

        if (!strcmp(svp->name, "LOG_OUTPUT"))
        {
            ISwitch *sw = IUFindSwitch(svp, "FILE_DEBUG");
            if (sw && sw->s == ISS_ON)
                DEBUGF(Logger::DBG_SESSION, "Session log file %s", Logger::getLogFile().c_str());
        }

        return rc;
    }

    bool rc = false;
    for (Connection::Interface *oneConnection : connections)
        rc |= oneConnection->ISNewSwitch(dev, name, states, names, n);

    return rc;
}

bool DefaultDevice::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ////////////////////////////////////////////////////
    // Polling Period
    ////////////////////////////////////////////////////
    if (!strcmp(name, PollPeriodNP.name))
    {
        IUUpdateNumber(&PollPeriodNP, values, names, n);
        PollPeriodNP.s = IPS_OK;
        POLLMS = static_cast<uint32_t>(PollPeriodN[0].value);
        IDSetNumber(&PollPeriodNP, nullptr);
        return true;
    }

    for (Connection::Interface *oneConnection : connections)
        oneConnection->ISNewNumber(dev, name, values, names, n);

    return false;
}

bool DefaultDevice::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    for (Connection::Interface *oneConnection : connections)
        oneConnection->ISNewText(dev, name, texts, names, n);

    return false;
}

bool DefaultDevice::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                              char *formats[], char *names[], int n)
{
    for (Connection::Interface *oneConnection : connections)
        oneConnection->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);

    return false;
}

bool DefaultDevice::ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
    return false;
}

void DefaultDevice::addDebugControl()
{
    registerProperty(&DebugSP, INDI_SWITCH);
    pDebug = false;
}

void DefaultDevice::addSimulationControl()
{
    registerProperty(&SimulationSP, INDI_SWITCH);
    pSimulation = false;
}

void DefaultDevice::addConfigurationControl()
{
    registerProperty(&ConfigProcessSP, INDI_SWITCH);
}

void DefaultDevice::addPollPeriodControl()
{
    registerProperty(&PollPeriodNP, INDI_NUMBER);
}

void DefaultDevice::addAuxControls()
{
    addDebugControl();
    addSimulationControl();
    addConfigurationControl();
    addPollPeriodControl();
}

void DefaultDevice::setDebug(bool enable)
{
    if (pDebug == enable)
    {
        DebugSP.s = IPS_OK;
        IDSetSwitch(&DebugSP, nullptr);
        return;
    }

    IUResetSwitch(&DebugSP);

    if (enable)
    {
        ISwitch *sp = IUFindSwitch(&DebugSP, "ENABLE");
        if (sp)
        {
            sp->s = ISS_ON;
            LOG_INFO("Debug is enabled.");
        }
    }
    else
    {
        ISwitch *sp = IUFindSwitch(&DebugSP, "DISABLE");
        if (sp)
        {
            sp->s = ISS_ON;
            LOG_INFO("Debug is disabled.");
        }
    }

    pDebug = enable;

    // Inform logger
    if (Logger::updateProperties(enable) == false)
        DEBUG(Logger::DBG_WARNING, "setLogDebug: Logger error");

    debugTriggered(enable);
    DebugSP.s = IPS_OK;
    IDSetSwitch(&DebugSP, nullptr);
}

void DefaultDevice::setSimulation(bool enable)
{
    if (pSimulation == enable)
    {
        SimulationSP.s = IPS_OK;
        IDSetSwitch(&SimulationSP, nullptr);
        return;
    }

    IUResetSwitch(&SimulationSP);

    if (enable)
    {
        ISwitch *sp = IUFindSwitch(&SimulationSP, "ENABLE");
        if (sp)
        {
            LOG_INFO("Simulation is enabled.");
            sp->s = ISS_ON;
        }
    }
    else
    {
        ISwitch *sp = IUFindSwitch(&SimulationSP, "DISABLE");
        if (sp)
        {
            sp->s = ISS_ON;
            LOG_INFO("Simulation is disabled.");
        }
    }

    pSimulation = enable;
    simulationTriggered(enable);
    SimulationSP.s = IPS_OK;
    IDSetSwitch(&SimulationSP, nullptr);
}

bool DefaultDevice::isDebug()
{
    return pDebug;
}

bool DefaultDevice::isSimulation()
{
    return pSimulation;
}

void DefaultDevice::debugTriggered(bool enable)
{
    INDI_UNUSED(enable);
}

void DefaultDevice::simulationTriggered(bool enable)
{
    INDI_UNUSED(enable);
}

void DefaultDevice::ISGetProperties(const char *dev)
{
    if (isInit == false)
    {
        if (dev != nullptr)
            setDeviceName(dev);
        else if (*getDeviceName() == '\0')
        {
            char *envDev = getenv("INDIDEV");
            if (envDev != nullptr)
                setDeviceName(envDev);
            else
                setDeviceName(getDefaultName());
        }

        strncpy(ConnectionSP.device, getDeviceName(), MAXINDIDEVICE);
        initProperties();
        addConfigurationControl();

        // If we have no connections, move Driver Info to General Info tab
        if (connections.size() == 0)
            strncpy(DriverInfoTP.group, INFO_TAB, MAXINDINAME);
    }

    for (INDI::Property *oneProperty : pAll)
    {
        INDI_PROPERTY_TYPE pType = oneProperty->getType();
        void *pPtr = oneProperty->getProperty();

        if (defineDynamicProperties == false && oneProperty->isDynamic())
            continue;

        switch (pType)
        {
            case INDI_NUMBER:
                IDDefNumber(static_cast<INumberVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_TEXT:
                IDDefText(static_cast<ITextVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_SWITCH:
                IDDefSwitch(static_cast<ISwitchVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_LIGHT:
                IDDefLight(static_cast<ILightVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_BLOB:
                IDDefBLOB(static_cast<IBLOBVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_UNKNOWN:
                break;
        }
    }

    // Remember debug & logging settings
    if (isInit == false)
    {
        loadConfig(true, "DEBUG");
        loadConfig(true, "DEBUG_LEVEL");
        loadConfig(true, "LOGGING_LEVEL");
        loadConfig(true, "POLLING_PERIOD");
        loadConfig(true, "LOG_OUTPUT");
    }

    if (ConnectionModeS == nullptr)
    {
        if (connections.size() > 0)
        {
            ConnectionModeS = static_cast<ISwitch *>(malloc(connections.size() * sizeof(ISwitch)));
            ISwitch *sp     = ConnectionModeS;
            for (Connection::Interface *oneConnection : connections)
            {
                IUFillSwitch(sp++, oneConnection->name().c_str(), oneConnection->label().c_str(), ISS_OFF);
            }

            activeConnection     = connections[0];
            ConnectionModeS[0].s = ISS_ON;
            IUFillSwitchVector(&ConnectionModeSP, ConnectionModeS, connections.size(), getDeviceName(),
                               "CONNECTION_MODE", "Connection Mode", CONNECTION_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

            defineSwitch(&ConnectionModeSP);
            activeConnection->Activated();
            loadConfig(true, "CONNECTION_MODE");
        }
    }

    isInit = true;
}

void DefaultDevice::resetProperties()
{
    std::vector<INDI::Property *>::iterator orderi;

    for (orderi = pAll.begin(); orderi != pAll.end(); ++orderi)
    {
        INDI_PROPERTY_TYPE pType = (*orderi)->getType();
        void *pPtr  = (*orderi)->getProperty();

        switch (pType)
        {
            case INDI_NUMBER:
                static_cast<INumberVectorProperty *>(pPtr)->s = IPS_IDLE;
                IDSetNumber(static_cast<INumberVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_TEXT:
                static_cast<ITextVectorProperty *>(pPtr)->s = IPS_IDLE;
                IDSetText(static_cast<ITextVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_SWITCH:
                static_cast<ISwitchVectorProperty *>(pPtr)->s = IPS_IDLE;
                IDSetSwitch(static_cast<ISwitchVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_LIGHT:
                static_cast<ILightVectorProperty *>(pPtr)->s = IPS_IDLE;
                IDSetLight(static_cast<ILightVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_BLOB:
                static_cast<IBLOBVectorProperty *>(pPtr)->s = IPS_IDLE;
                IDSetBLOB(static_cast<IBLOBVectorProperty *>(pPtr), nullptr);
                break;
            case INDI_UNKNOWN:
                break;
        }
    }
}

void DefaultDevice::setConnected(bool status, IPState state, const char *msg)
{
    ISwitch *sp                = nullptr;
    ISwitchVectorProperty *svp = getSwitch(INDI::SP::CONNECTION);
    if (!svp)
        return;

    IUResetSwitch(svp);

    // Connect
    if (status)
    {
        sp = IUFindSwitch(svp, "CONNECT");
        if (!sp)
            return;
        sp->s = ISS_ON;
    }
    // Disconnect
    else
    {
        sp = IUFindSwitch(svp, "DISCONNECT");
        if (!sp)
            return;
        sp->s = ISS_ON;
    }

    svp->s = state;

    if (msg == nullptr)
        IDSetSwitch(svp, nullptr);
    else
        IDSetSwitch(svp, "%s", msg);
}

//  This is a helper function
//  that just encapsulates the Indi way into our clean c++ way of doing things
int DefaultDevice::SetTimer(uint32_t ms)
{
    return IEAddTimer(ms, timerfunc, this);
}

//  Just another helper to help encapsulate indi into a clean class
void DefaultDevice::RemoveTimer(int id)
{
    IERmTimer(id);
    return;
}

//  This is just a placeholder
//  This function should be overriden by child classes if they use timers
//  So we should never get here
void DefaultDevice::TimerHit()
{
    return;
}

bool DefaultDevice::updateProperties()
{
    //  The base device has no properties to update
    return true;
}

uint16_t DefaultDevice::getDriverInterface()
{
    return interfaceDescriptor;
}

void DefaultDevice::setDriverInterface(uint16_t value)
{
    char interfaceStr[16];
    interfaceDescriptor = value;
    snprintf(interfaceStr, 16, "%d", interfaceDescriptor);
    IUSaveText(&DriverInfoT[3], interfaceStr);
}

bool DefaultDevice::initProperties()
{
    char versionStr[16];
    char interfaceStr[16];

    snprintf(versionStr, 16, "%d.%d", majorVersion, minorVersion);
    snprintf(interfaceStr, 16, "%d", interfaceDescriptor);

    IUFillSwitch(&ConnectionS[0], "CONNECT", "Connect", ISS_OFF);
    IUFillSwitch(&ConnectionS[1], "DISCONNECT", "Disconnect", ISS_ON);
    IUFillSwitchVector(&ConnectionSP, ConnectionS, 2, getDeviceName(), INDI::SP::CONNECTION, "Connection", "Main Control",
                       IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    registerProperty(&ConnectionSP, INDI_SWITCH);

    IUFillText(&DriverInfoT[0], "DRIVER_NAME", "Name", getDriverName());
    IUFillText(&DriverInfoT[1], "DRIVER_EXEC", "Exec", getDriverExec());
    IUFillText(&DriverInfoT[2], "DRIVER_VERSION", "Version", versionStr);
    IUFillText(&DriverInfoT[3], "DRIVER_INTERFACE", "Interface", interfaceStr);
    IUFillTextVector(&DriverInfoTP, DriverInfoT, 4, getDeviceName(), "DRIVER_INFO", "Driver Info", CONNECTION_TAB,
                     IP_RO, 60, IPS_IDLE);
    registerProperty(&DriverInfoTP, INDI_TEXT);

    IUFillSwitch(&DebugS[0], "ENABLE", "Enable", ISS_OFF);
    IUFillSwitch(&DebugS[1], "DISABLE", "Disable", ISS_ON);
    IUFillSwitchVector(&DebugSP, DebugS, NARRAY(DebugS), getDeviceName(), "DEBUG", "Debug", "Options", IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&SimulationS[0], "ENABLE", "Enable", ISS_OFF);
    IUFillSwitch(&SimulationS[1], "DISABLE", "Disable", ISS_ON);
    IUFillSwitchVector(&SimulationSP, SimulationS, NARRAY(SimulationS), getDeviceName(), "SIMULATION", "Simulation",
                       "Options", IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&ConfigProcessS[0], "CONFIG_LOAD", "Load", ISS_OFF);
    IUFillSwitch(&ConfigProcessS[1], "CONFIG_SAVE", "Save", ISS_OFF);
    IUFillSwitch(&ConfigProcessS[2], "CONFIG_DEFAULT", "Default", ISS_OFF);
    IUFillSwitch(&ConfigProcessS[3], "CONFIG_PURGE", "Purge", ISS_OFF);
    IUFillSwitchVector(&ConfigProcessSP, ConfigProcessS, NARRAY(ConfigProcessS), getDeviceName(), "CONFIG_PROCESS",
                       "Configuration", "Options", IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    IUFillNumber(&PollPeriodN[0], "PERIOD_MS", "Period (ms)", "%.f", 10, 600000, 1000, POLLMS);
    IUFillNumberVector(&PollPeriodNP, PollPeriodN, 1, getDeviceName(), "POLLING_PERIOD", "Polling", "Options", IP_RW, 0, IPS_IDLE);

    INDI::Logger::initProperties(this);

    // Ready the logger
    std::string logFile = getDriverExec();

    DEBUG_CONF(logFile, Logger::file_off | Logger::screen_on, Logger::defaultlevel, Logger::defaultlevel);

    return true;
}

bool DefaultDevice::deleteProperty(const char *propertyName)
{
    char errmsg[MAXRBUF];

    if (propertyName == nullptr)
    {
        //while(!pAll.empty()) delete bar.back(), bar.pop_back();
        IDDelete(getDeviceName(), nullptr, nullptr);
        return true;
    }

    // Keep dynamic properties in existing property list so they can be reused
    if (deleteDynamicProperties == false)
    {
        INDI::Property *prop = getProperty(propertyName);
        if (prop && prop->isDynamic())
        {
            IDDelete(getDeviceName(), propertyName, nullptr);
            return true;
        }
    }

    if (removeProperty(propertyName, errmsg) == 0)
    {
        IDDelete(getDeviceName(), propertyName, nullptr);
        return true;
    }
    else
        return false;
}

void DefaultDevice::defineNumber(INumberVectorProperty *nvp)
{
    registerProperty(nvp, INDI_NUMBER);
    IDDefNumber(nvp, nullptr);
}

void DefaultDevice::defineText(ITextVectorProperty *tvp)
{
    registerProperty(tvp, INDI_TEXT);
    IDDefText(tvp, nullptr);
}

void DefaultDevice::defineSwitch(ISwitchVectorProperty *svp)
{
    registerProperty(svp, INDI_SWITCH);
    IDDefSwitch(svp, nullptr);
}

void DefaultDevice::defineLight(ILightVectorProperty *lvp)
{
    registerProperty(lvp, INDI_LIGHT);
    IDDefLight(lvp, nullptr);
}

void DefaultDevice::defineBLOB(IBLOBVectorProperty *bvp)
{
    registerProperty(bvp, INDI_BLOB);
    IDDefBLOB(bvp, nullptr);
}

bool DefaultDevice::Connect()
{
    if (isConnected())
        return true;

    if (activeConnection == nullptr)
    {
        LOG_ERROR("No active connection defined.");
        return false;
    }

    bool rc = activeConnection->Connect();

    if (rc)
    {
        saveConfig(true, "CONNECTION_MODE");
        if (POLLMS > 0)
            SetTimer(POLLMS);
    }

    return rc;
}

bool DefaultDevice::Disconnect()
{
    if (isSimulation())
    {
        DEBUGF(Logger::DBG_SESSION, "%s is offline.", getDeviceName());
        return true;
    }

    if (activeConnection)
    {
        bool rc = activeConnection->Disconnect();
        if (rc)
        {
            DEBUGF(Logger::DBG_SESSION, "%s is offline.", getDeviceName());
            return true;
        }
        else
            return false;
    }

    return false;
}

void DefaultDevice::registerConnection(Connection::Interface *newConnection)
{
    connections.push_back(newConnection);
}

bool DefaultDevice::unRegisterConnection(Connection::Interface *existingConnection)
{
    auto i = std::begin(connections);

    while (i != std::end(connections))
    {
        if (*i == existingConnection)
        {
            i = connections.erase(i);
            return true;
        }
        else
            ++i;
    }

    return false;
}

void DefaultDevice::setDefaultPollingPeriod(uint32_t period)
{
    PollPeriodN[0].value = period;
    POLLMS = period;
}

void DefaultDevice::setPollingPeriodRange(uint32_t minimum, uint32_t maximum)
{
    PollPeriodN[0].min = minimum;
    PollPeriodN[0].max = maximum;
    IUUpdateMinMax(&PollPeriodNP);
}

}
