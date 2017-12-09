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

#pragma once

#define ERROR_MSG_LENGTH 250

class EQMod;

class EQModError
{
  public:
    enum Severity
    {
        ErrDisconnect = 1,
        ErrInvalidCmd,
        ErrCmdFailed,
        ErrInvalidParameter
    } severity;
    char message[ERROR_MSG_LENGTH];

    EQModError(Severity sev, const char *msg, ...);
    const char *severityString();
    bool DefaultHandleException(EQMod *device);
};
