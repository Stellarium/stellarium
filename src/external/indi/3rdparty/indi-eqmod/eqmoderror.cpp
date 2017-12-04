/* Copyright 2012 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "eqmoderror.h"

#include "eqmod.h"

EQModError::EQModError(Severity sev, const char *msg, ...)
{
    if (msg)
    {
        va_list ap;
        va_start(ap, msg);
        vsnprintf(message, ERROR_MSG_LENGTH, msg, ap);
        va_end(ap);
    }
    else
        message[0] = '\0';
    severity = sev;
}

const char *EQModError::severityString()
{
    switch (severity)
    {
        case EQModError::ErrInvalidCmd:
            return ("Invalid command");
        case EQModError::ErrCmdFailed:
            return ("Command failed");
        case EQModError::ErrInvalidParameter:
            return ("Invalid parameter");
        case EQModError::ErrDisconnect:
            return ("");
        default:
            return ("Unknown severity");
    }
}

bool EQModError::DefaultHandleException(EQMod *device)
{
    switch (severity)
    {
        case EQModError::ErrInvalidCmd:
        case EQModError::ErrCmdFailed:
        case EQModError::ErrInvalidParameter:
            DEBUGFDEVICE(device->getDeviceName(), INDI::Logger::DBG_WARNING, "Warning: %s -> %s", severityString(),
                         message);
            return true;
        case EQModError::ErrDisconnect:
            DEBUGFDEVICE(device->getDeviceName(), INDI::Logger::DBG_ERROR, "Error: %s -> %s", severityString(),
                         message);
            device->Disconnect();
            return false;
        default:
            DEBUGFDEVICE(device->getDeviceName(), INDI::Logger::DBG_ERROR, "Unhandled exception: %s -> %s",
                         severityString(), message);
            device->Disconnect();
            return false;
            break;
    }
}
