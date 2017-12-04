/*******************************************************************************
 Copyright(c) 2016 CloudMakers, s. r. o.. All rights reserved.

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

#include "dome_script.h"

#include "indicom.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

#define POLLMS  2000
#define MAXARGS 20

typedef enum
{
    SCRIPT_CONNECT = 1,
    SCRIPT_DISCONNECT,
    SCRIPT_STATUS,
    SCRIPT_OPEN,
    SCRIPT_CLOSE,
    SCRIPT_PARK,
    SCRIPT_UNPARK,
    SCRIPT_GOTO,
    SCRIPT_MOVE_CW,
    SCRIPT_MOVE_CCW,
    SCRIPT_ABORT,
    SCRIPT_COUNT
} scripts;

std::unique_ptr<DomeScript> scope_script(new DomeScript());

void ISGetProperties(const char *dev)
{
    scope_script->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    scope_script->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    scope_script->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    scope_script->ISNewNumber(dev, name, values, names, n);
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
    scope_script->ISSnoopDevice(root);
}

DomeScript::DomeScript()
{
    SetDomeCapability(DOME_CAN_PARK | DOME_CAN_ABORT | DOME_CAN_ABS_MOVE | DOME_HAS_SHUTTER);
}

const char *DomeScript::getDefaultName()
{
    return (const char *)"Dome Scripting Gateway";
}

bool DomeScript::initProperties()
{
    INDI::Dome::initProperties();
    SetParkDataType(PARK_AZ);
#ifdef OSX_EMBEDED_MODE
    IUFillText(&ScriptsT[0], "FOLDER", "Folder", "/usr/local/share/indi/scripts");
#else
    IUFillText(&ScriptsT[0], "FOLDER", "Folder", "/usr/share/indi/scripts");
#endif
    IUFillText(&ScriptsT[SCRIPT_CONNECT], "SCRIPT_CONNECT", "Connect script", "connect.py");
    IUFillText(&ScriptsT[SCRIPT_DISCONNECT], "SCRIPT_DISCONNECT", "Disconnect script", "disconnect.py");
    IUFillText(&ScriptsT[SCRIPT_STATUS], "SCRIPT_STATUS", "Get status script", "status.py");
    IUFillText(&ScriptsT[SCRIPT_OPEN], "SCRIPT_OPEN", "Open shutter script", "open.py");
    IUFillText(&ScriptsT[SCRIPT_CLOSE], "SCRIPT_CLOSE", "Close shutter script", "close.py");
    IUFillText(&ScriptsT[SCRIPT_PARK], "SCRIPT_PARK", "Park script", "park.py");
    IUFillText(&ScriptsT[SCRIPT_UNPARK], "SCRIPT_UNPARK", "Unpark script", "unpark.py");
    IUFillText(&ScriptsT[SCRIPT_GOTO], "SCRIPT_GOTO", "Goto script", "goto.py");
    IUFillText(&ScriptsT[SCRIPT_MOVE_CW], "SCRIPT_MOVE_CW", "Move clockwise script", "move_cw.py");
    IUFillText(&ScriptsT[SCRIPT_MOVE_CCW], "SCRIPT_MOVE_CCW", "Move counter clockwise script", "move_ccw.py");
    IUFillText(&ScriptsT[SCRIPT_ABORT], "SCRIPT_ABORT", "Abort motion script", "abort.py");
    IUFillTextVector(&ScriptsTP, ScriptsT, SCRIPT_COUNT, getDefaultName(), "SCRIPTS", "Scripts", OPTIONS_TAB, IP_RW, 60,
                     IPS_IDLE);
    return true;
}

bool DomeScript::saveConfigItems(FILE *fp)
{
    INDI::Dome::saveConfigItems(fp);
    IUSaveConfigText(fp, &ScriptsTP);
    return true;
}

void DomeScript::ISGetProperties(const char *dev)
{
    INDI::Dome::ISGetProperties(dev);
    defineText(&ScriptsTP);
}

bool DomeScript::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, ScriptsTP.name) == 0)
        {
            IUUpdateText(&ScriptsTP, texts, names, n);
            IDSetText(&ScriptsTP, nullptr);
            return true;
        }
    }
    return Dome::ISNewText(dev, name, texts, names, n);
}

bool DomeScript::RunScript(int script, ...)
{
    int pid = fork();
    if (pid == -1)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Fork failed");
        return false;
    }
    else if (pid == 0)
    {
        char tmp[256];

        strncpy(tmp, ScriptsT[script].text, sizeof(tmp));

        char **args = (char **)malloc(MAXARGS * sizeof(char *));
        int arg     = 1;
        char *p     = tmp;

        while (arg < MAXARGS)
        {
            char *pp = strstr(p, " ");

            if (pp == nullptr)
                break;
            *pp++       = 0;
            args[arg++] = pp;
            p           = pp;
        }
        va_list ap;
        va_start(ap, script);
        while (arg < MAXARGS)
        {
            char *pp    = va_arg(ap, char *);
            args[arg++] = pp;
            if (pp == nullptr)
                break;
        }
        va_end(ap);
        char path[256];
        snprintf(path, 256, "%s/%s", ScriptsT[0].text, tmp);
        execvp(path, args);
        DEBUG(INDI::Logger::DBG_DEBUG, "Failed to execute script");
        exit(0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        DEBUGF(INDI::Logger::DBG_DEBUG, "Script %s returned %d", ScriptsT[script].text, status);
        return status == 0;
    }
}

bool DomeScript::updateProperties()
{
    INDI::Dome::updateProperties();
    if (isConnected())
    {
        if (InitPark())
        {
            SetAxis1ParkDefault(0);
        }
        else
        {
            SetAxis1Park(0);
            SetAxis1ParkDefault(0);
        }
        TimerHit();
    }
    return true;
}

void DomeScript::TimerHit()
{
    if (!isConnected())
        return;
    char *name  = tmpnam(nullptr);
    bool status = RunScript(SCRIPT_STATUS, name, nullptr);
    if (status)
    {
        int parked = 0, shutter = 0;
        float az   = 0;
        FILE *file = fopen(name, "r");
        int ret    = 0;

        ret = fscanf(file, "%d %d %f", &parked, &shutter, &az);
        fclose(file);
        unlink(name);
        DomeAbsPosN[0].value = az = round(range360(az) * 10) / 10;
        if (parked != 0)
        {
            if (getDomeState() == DOME_PARKING || getDomeState() == DOME_UNPARKED)
            {
                SetParked(true);
                TargetAz = az;
                DEBUG(INDI::Logger::DBG_SESSION, "Park succesfully executed");
            }
        }
        else
        {
            if (getDomeState() == DOME_UNPARKING || getDomeState() == DOME_PARKED)
            {
                SetParked(false);
                TargetAz = az;
                DEBUG(INDI::Logger::DBG_SESSION, "Unpark succesfully executed");
            }
        }
        if (std::round(az * 10) != std::round(TargetAz * 10))
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "Moving %g -> %g %d", std::round(az * 10) / 10,
                   std::round(TargetAz * 10) / 10, getDomeState());
            IDSetNumber(&DomeAbsPosNP, nullptr);
        }
        else if (getDomeState() == DOME_MOVING)
        {
            setDomeState(DOME_SYNCED);
            IDSetNumber(&DomeAbsPosNP, nullptr);
        }
        if (shutterState == SHUTTER_OPENED)
        {
            if (shutter == 0)
            {
                shutterState    = SHUTTER_CLOSED;
                DomeShutterSP.s = IPS_OK;
                IDSetSwitch(&DomeShutterSP, nullptr);
                DEBUG(INDI::Logger::DBG_SESSION, "Shutter was succesfully closed");
            }
        }
        else
        {
            if (shutter == 1)
            {
                shutterState    = SHUTTER_OPENED;
                DomeShutterSP.s = IPS_OK;
                IDSetSwitch(&DomeShutterSP, nullptr);
                DEBUG(INDI::Logger::DBG_SESSION, "Shutter was succesfully opened");
            }
        }
    }
    else
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to read status");
    }
    SetTimer(POLLMS);
    if (!isParked() && TimeSinceUpdate++ > 4)
    {
        TimeSinceUpdate = 0;
        UpdateMountCoords();
    }
}

bool DomeScript::Connect()
{
    if (isConnected())
        return true;

    bool status = RunScript(SCRIPT_CONNECT, nullptr);

    if (status)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Successfully connected");
    }
    return status;
}

bool DomeScript::Disconnect()
{
    bool status = RunScript(SCRIPT_DISCONNECT, nullptr);
    if (status)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Successfully disconnected");
    }
    return status;
}

IPState DomeScript::Park()
{
    bool status = RunScript(SCRIPT_PARK, nullptr);
    if (status)
    {
        return IPS_BUSY;
    }
    DEBUG(INDI::Logger::DBG_ERROR, "Failed to park");
    return IPS_ALERT;
}

IPState DomeScript::UnPark()
{
    bool status = RunScript(SCRIPT_UNPARK, nullptr);
    if (status)
    {
        return IPS_BUSY;
    }
    DEBUG(INDI::Logger::DBG_ERROR, "Failed to unpark");
    return IPS_ALERT;
}

IPState DomeScript::ControlShutter(ShutterOperation operation)
{
    if (RunScript(operation == SHUTTER_OPEN ? SCRIPT_OPEN : SCRIPT_CLOSE, nullptr))
    {
        return IPS_BUSY;
    }
    DEBUGF(INDI::Logger::DBG_ERROR, "Failed to %s shutter", operation == SHUTTER_OPEN ? "open" : "close");
    return IPS_ALERT;
}

IPState DomeScript::MoveAbs(double az)
{
    char _az[16];
    snprintf(_az, 16, "%f", round(az * 10) / 10);
    bool status = RunScript(SCRIPT_GOTO, _az, nullptr);
    if (status)
    {
        TargetAz = az;
        return IPS_BUSY;
    }
    return IPS_ALERT;
}

IPState DomeScript::Move(DomeDirection dir, DomeMotionCommand operation)
{
    if (operation == MOTION_START)
    {
        if (RunScript(dir == DOME_CW ? SCRIPT_MOVE_CW : SCRIPT_MOVE_CCW, nullptr))
        {
            DomeAbsPosNP.s = IPS_BUSY;
            TargetAz       = -1;
        }
        else
        {
            DomeAbsPosNP.s = IPS_ALERT;
        }
    }
    else
    {
        if (RunScript(SCRIPT_ABORT, nullptr))
        {
            DomeAbsPosNP.s = IPS_IDLE;
        }
        else
        {
            DomeAbsPosNP.s = IPS_ALERT;
        }
    }
    IDSetNumber(&DomeAbsPosNP, nullptr);
    return ((operation == MOTION_START) ? IPS_BUSY : IPS_OK);
}

bool DomeScript::Abort()
{
    bool status = RunScript(SCRIPT_ABORT, nullptr);
    if (status)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Successfully aborted");
    }
    return status;
}
