/*******************************************************************************
  Copyright (C) 2013 Jasem Mutlaq <mutlaqja@ikarustech.com>

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

#include "indicontroller.h"

#include <cstring>

namespace INDI
{
Controller::Controller(DefaultDevice *cdevice)
{
    device = cdevice;

    JoystickSettingT      = nullptr;
    JoystickSettingTP.ntp = 0;

    joystickCallbackFunc = joystickEvent;
    axisCallbackFunc     = axisEvent;
    buttonCallbackFunc   = buttonEvent;
}

Controller::~Controller()
{
    for (int i = 0; i < JoystickSettingTP.ntp; i++)
        free(JoystickSettingT[i].aux0);

    free(JoystickSettingT);
}

void Controller::mapController(const char *propertyName, const char *propertyLabel, ControllerType type,
                               const char *initialValue)
{
    if (JoystickSettingT == nullptr)
        JoystickSettingT = static_cast<IText *>(malloc(sizeof(IText)));

    // Ignore duplicates
    for (int i = 0; i < JoystickSettingTP.ntp; i++)
    {
        if (!strcmp(propertyName, JoystickSettingT[i].name))
            return;
    }

    IText *buf = static_cast<IText *>(realloc(JoystickSettingT, (JoystickSettingTP.ntp + 1) * sizeof(IText)));
    if (buf == nullptr)
    {
        free (JoystickSettingT);
        perror("Failed to allocate memory for joystick controls.");
        return;
    }
    else
        JoystickSettingT = buf;

    ControllerType *ctype = static_cast<ControllerType *>(malloc(sizeof(ControllerType)));
    *ctype                = type;

    memset(JoystickSettingT + JoystickSettingTP.ntp, 0, sizeof(IText));
    IUFillText(&JoystickSettingT[JoystickSettingTP.ntp], propertyName, propertyLabel, initialValue);

    JoystickSettingT[JoystickSettingTP.ntp++].aux0 = ctype;

    IUFillTextVector(&JoystickSettingTP, JoystickSettingT, JoystickSettingTP.ntp, device->getDeviceName(),
                     "JOYSTICKSETTINGS", "Settings", "Joystick", IP_RW, 0, IPS_IDLE);
}

void Controller::clearMap()
{
    for (int i = 0; i < JoystickSettingTP.ntp; i++)
    {
        free(JoystickSettingT[i].aux0);
        free(JoystickSettingT[i].text);
    }

    JoystickSettingTP.ntp = 0;
    free(JoystickSettingT);
    JoystickSettingT = nullptr;
}

bool Controller::initProperties()
{
    IUFillSwitch(&UseJoystickS[0], "ENABLE", "Enable", ISS_OFF);
    IUFillSwitch(&UseJoystickS[1], "DISABLE", "Disable", ISS_ON);
    IUFillSwitchVector(&UseJoystickSP, UseJoystickS, 2, device->getDeviceName(), "USEJOYSTICK", "Joystick", OPTIONS_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    return true;
}

void Controller::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, device->getDeviceName()))
        return;

    if (device->isConnected())
    {
        device->defineSwitch(&UseJoystickSP);

        if (JoystickSettingT && UseJoystickS[0].s == ISS_ON)
            device->defineText(&JoystickSettingTP);
    }
}

bool Controller::updateProperties()
{
    if (device->isConnected())
    {
        device->defineSwitch(&UseJoystickSP);

        if (JoystickSettingT && UseJoystickS[0].s == ISS_ON)
            device->defineText(&JoystickSettingTP);
    }
    else
    {
        device->deleteProperty(UseJoystickSP.name);
        device->deleteProperty(JoystickSettingTP.name);
    }

    return true;
}

bool Controller::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, device->getDeviceName()) == 0)
    {
        // Enable joystick support
        if (!strcmp(name, UseJoystickSP.name))
        {
            IUUpdateSwitch(&UseJoystickSP, states, names, n);

            UseJoystickSP.s = IPS_OK;

            if (UseJoystickSP.sp[0].s == ISS_ON)
                enableJoystick();
            else
                disableJoystick();

            IDSetSwitch(&UseJoystickSP, nullptr);

            return true;
        }
    }

    return false;
}

bool Controller::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, device->getDeviceName()) == 0)
    {
        if (!strcmp(name, "JOYSTICKSETTINGS") && n <= JoystickSettingTP.ntp)
        {
            for (int i = 0; i < JoystickSettingTP.ntp; i++)
            {
                IText *tp = IUFindText(&JoystickSettingTP, names[i]);
                if (tp)
                {
                    ControllerType cType  = getControllerType(texts[i]);
                    ControllerType myType = *(static_cast<ControllerType *>(JoystickSettingT[i].aux0));
                    if (cType != myType)
                    {
                        JoystickSettingTP.s = IPS_ALERT;
                        IDSetText(&JoystickSettingTP, nullptr);
                        DEBUGFDEVICE(dev, INDI::Logger::DBG_ERROR, "Cannot change controller type to %s.", texts[i]);
                        return false;
                    }
                }
            }

            IUUpdateText(&JoystickSettingTP, texts, names, n);

            for (int i = 0; i < n; i++)
            {
                if (strstr(JoystickSettingT[i].text, "JOYSTICK_"))
                    IDSnoopDevice("Joystick", JoystickSettingT[i].text);
            }

            JoystickSettingTP.s = IPS_OK;
            IDSetText(&JoystickSettingTP, nullptr);
            return true;
        }
    }

    return false;
}

bool Controller::ISSnoopDevice(XMLEle *root)
{
    XMLEle *ep = nullptr;
    double mag = 0, angle = 0;

    // If joystick is disabled, do not process anything.
    if (UseJoystickSP.sp[0].s == ISS_OFF)
        return false;

    const char *propName = findXMLAttValu(root, "name");

    // Check axis
    if (!strcmp("JOYSTICK_AXES", propName))
    {
        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            const char *elemName = findXMLAttValu(ep, "name");
            const char *setting  = getControllerSetting(elemName);
            if (setting == nullptr)
                continue;

            mag = atof(pcdataXMLEle(ep));

            axisCallbackFunc(setting, mag, device);
        }
    }
    // Check buttons
    else if (!strcmp("JOYSTICK_BUTTONS", propName))
    {
        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            const char *elemName = findXMLAttValu(ep, "name");
            const char *setting  = getControllerSetting(elemName);

            if (setting == nullptr)
                continue;

            buttonCallbackFunc(setting, strcmp(pcdataXMLEle(ep), "Off") ? ISS_ON : ISS_OFF, device);
        }
    }
    // Check joysticks
    else if (strstr(propName, "JOYSTICK_"))
    {
        const char *setting = getControllerSetting(propName);

        // We don't have this here, so let's not process it
        if (setting == nullptr)
            return false;

        for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
        {
            if (!strcmp("JOYSTICK_MAGNITUDE", findXMLAttValu(ep, "name")))
                mag = atof(pcdataXMLEle(ep));
            else if (!strcmp("JOYSTICK_ANGLE", findXMLAttValu(ep, "name")))
                angle = atof(pcdataXMLEle(ep));
        }

        joystickCallbackFunc(setting, mag, angle, device);
    }

    return false;
}

bool Controller::saveConfigItems(FILE *fp)
{
    IUSaveConfigSwitch(fp, &UseJoystickSP);
    IUSaveConfigText(fp, &JoystickSettingTP);

    return true;
}

void Controller::enableJoystick()
{
    device->defineText(&JoystickSettingTP);

    for (int i = 0; i < JoystickSettingTP.ntp; i++)
    {
        if (strstr(JoystickSettingTP.tp[i].text, "JOYSTICK_"))
            IDSnoopDevice("Joystick", JoystickSettingTP.tp[i].text);
    }

    IDSnoopDevice("Joystick", "JOYSTICK_AXES");
    IDSnoopDevice("Joystick", "JOYSTICK_BUTTONS");
}

void Controller::disableJoystick()
{
    device->deleteProperty(JoystickSettingTP.name);
}

void Controller::setJoystickCallback(joystickFunc JoystickCallback)
{
    joystickCallbackFunc = JoystickCallback;
}

void Controller::setAxisCallback(axisFunc AxisCallback)
{
    axisCallbackFunc = AxisCallback;
}

void Controller::setButtonCallback(buttonFunc buttonCallback)
{
    buttonCallbackFunc = buttonCallback;
}

void Controller::joystickEvent(const char *joystick_n, double mag, double angle, void *context)
{
    INDI_UNUSED(joystick_n);
    INDI_UNUSED(mag);
    INDI_UNUSED(angle);
    INDI_UNUSED(context);
}

void Controller::axisEvent(const char *axis_n, int value, void *context)
{
    INDI_UNUSED(axis_n);
    INDI_UNUSED(value);
    INDI_UNUSED(context);
}

void Controller::buttonEvent(const char *button_n, int button_value, void *context)
{
    INDI_UNUSED(button_n);
    INDI_UNUSED(button_value);
    INDI_UNUSED(context);
}

Controller::ControllerType Controller::getControllerType(const char *name)
{
    ControllerType targetType = CONTROLLER_UNKNOWN;

    if (strstr(name, "JOYSTICK_"))
        targetType = CONTROLLER_JOYSTICK;
    else if (strstr(name, "AXIS_"))
        targetType = CONTROLLER_AXIS;
    else if (strstr(name, "BUTTON_"))
        targetType = CONTROLLER_BUTTON;

    return targetType;
}

const char *Controller::getControllerSetting(const char *name)
{
    for (int i = 0; i < JoystickSettingTP.ntp; i++)
        if (!strcmp(JoystickSettingT[i].text, name))
            return JoystickSettingT[i].name;

    return nullptr;
}
}
