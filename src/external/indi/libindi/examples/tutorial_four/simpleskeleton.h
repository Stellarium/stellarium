/*
    Tutorial Four
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#pragma once

/** \file simpleskeleton.h
    \brief Construct a basic INDI CCD device that demonstrates ability to define properties from a skeleton file.
    \author Jasem Mutlaq

    \example simpleskeleton.h
    A skeleton file is an external XML file with the driver properties already defined. This tutorial illustrates how to create a driver from
    a skeleton file and parse/process the properties. The skeleton file name is tutorial_four_sk.xml
    \note Please note that if you create your own skeleton file, you must append _sk postfix to your skeleton file name.
*/

#include "defaultdevice.h"

class SimpleSkeleton : public INDI::DefaultDevice
{
  public:
    SimpleSkeleton() = default;
    ~SimpleSkeleton() = default;

    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);

  private:
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool Connect();
    virtual bool Disconnect();
};
