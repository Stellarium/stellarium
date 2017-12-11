/*******************************************************************************
  Copyright(c) 2013 Jasem Mutlaq. All rights reserved.

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

#pragma once

#include "defaultdevice.h"

class JoyStickDriver;

/**
 * @brief The JoyStick class provides an INDI driver that displays event data from game pads. The INDI driver can be encapsulated in any other driver
 * via snooping on properties of interesting.
 *
 */
class JoyStick : public INDI::DefaultDevice
{
  public:
    JoyStick();
    virtual ~JoyStick();

    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISSnoopDevice(XMLEle *root) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

    static void joystickHelper(int joystick_n, double mag, double angle);
    static void axisHelper(int axis_n, int value);
    static void buttonHelper(int button_n, int value);

  protected:
    //  Generic indi device entries
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual const char *getDefaultName() override;

    bool saveConfigItems(FILE *fp) override;

    void setupParams();

    void joystickEvent(int joystick_n, double mag, double angle);
    void axisEvent(int axis_n, int value);
    void buttonEvent(int button_n, int value);

    INumberVectorProperty *JoyStickNP;
    INumber *JoyStickN;

    INumberVectorProperty AxisNP;
    INumber *AxisN;

    ISwitchVectorProperty ButtonSP;
    ISwitch *ButtonS;

    ITextVectorProperty PortTP; //  A text vector that stores out physical port name
    IText PortT[1];

    ITextVectorProperty JoystickInfoTP;
    IText JoystickInfoT[5];

    JoyStickDriver *driver;
};
