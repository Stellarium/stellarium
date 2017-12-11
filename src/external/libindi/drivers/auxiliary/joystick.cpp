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

#include "joystick.h"

#include "joystickdriver.h"
#include "indistandardproperty.h"

#include <memory>
#include <cstring>

#define POLLMS 250

// We declare an auto pointer to joystick.
std::unique_ptr<JoyStick> joystick(new JoyStick());

void ISGetProperties(const char *dev)
{
    joystick->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    joystick->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    joystick->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    joystick->ISNewNumber(dev, name, values, names, n);
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
    joystick->ISSnoopDevice(root);
}

JoyStick::JoyStick()
{
    driver = new JoyStickDriver();

    JoyStickNP = nullptr;
    JoyStickN  = nullptr;
    AxisN      = nullptr;
    ButtonS    = nullptr;
}

JoyStick::~JoyStick()
{
    //dtor

    delete (driver);
}

const char *JoyStick::getDefaultName()
{
    return (const char *)"Joystick";
}

bool JoyStick::Connect()
{
    bool rc = driver->Connect();

    if (rc)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Joystick is online.");

        setupParams();
    }
    else
        DEBUG(INDI::Logger::DBG_SESSION, "Error: cannot find Joystick device.");

    return rc;
}

bool JoyStick::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Joystick is offline.");

    return driver->Disconnect();
}

void JoyStick::setupParams()
{
    char propName[16]={0}, propLabel[16]={0};

    if (driver == nullptr)
        return;

    int nAxis      = driver->getNumOfAxes();
    int nJoysticks = driver->getNumOfJoysticks();
    int nButtons   = driver->getNumrOfButtons();

    JoyStickNP = new INumberVectorProperty[nJoysticks];
    JoyStickN  = new INumber[nJoysticks * 2];

    AxisN = new INumber[nAxis];

    ButtonS = new ISwitch[nButtons];

    for (int i = 0; i < nJoysticks * 2; i += 2)
    {
        snprintf(propName, 16, "JOYSTICK_%d", i / 2 + 1);
        snprintf(propLabel, 16, "Joystick %d", i / 2 + 1);

        IUFillNumber(&JoyStickN[i], "JOYSTICK_MAGNITUDE", "Magnitude", "%g", -32767.0, 32767.0, 0, 0);
        IUFillNumber(&JoyStickN[i + 1], "JOYSTICK_ANGLE", "Angle", "%g", 0, 360.0, 0, 0);
        IUFillNumberVector(&JoyStickNP[i / 2], JoyStickN + i, 2, getDeviceName(), propName, propLabel, "Monitor", IP_RO,
                           0, IPS_IDLE);
    }

    for (int i = 0; i < nAxis; i++)
    {
        snprintf(propName, 16, "AXIS_%d", i + 1);
        snprintf(propLabel, 16, "Axis %d", i + 1);

        IUFillNumber(&AxisN[i], propName, propLabel, "%g", -32767.0, 32767.0, 0, 0);
    }

    IUFillNumberVector(&AxisNP, AxisN, nAxis, getDeviceName(), "JOYSTICK_AXES", "Axes", "Monitor", IP_RO, 0, IPS_IDLE);

    for (int i = 0; i < nButtons; i++)
    {
        snprintf(propName, 16, "BUTTON_%d", i + 1);
        snprintf(propLabel, 16, "Button %d", i + 1);

        IUFillSwitch(&ButtonS[i], propName, propLabel, ISS_OFF);
    }

    IUFillSwitchVector(&ButtonSP, ButtonS, nButtons, getDeviceName(), "JOYSTICK_BUTTONS", "Buttons", "Monitor", IP_RO,
                       ISR_NOFMANY, 0, IPS_IDLE);
}

bool JoyStick::initProperties()
{
    INDI::DefaultDevice::initProperties();

    IUFillText(&PortT[0], "PORT", "Port", "/dev/input/js0");
    IUFillTextVector(&PortTP, PortT, 1, getDeviceName(), INDI::SP::DEVICE_PORT, "Ports", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    IUFillText(&JoystickInfoT[0], "JOYSTICK_NAME", "Name", "");
    IUFillText(&JoystickInfoT[1], "JOYSTICK_VERSION", "Version", "");
    IUFillText(&JoystickInfoT[2], "JOYSTICK_NJOYSTICKS", "# Joysticks", "");
    IUFillText(&JoystickInfoT[3], "JOYSTICK_NAXES", "# Axes", "");
    IUFillText(&JoystickInfoT[4], "JOYSTICK_NBUTTONS", "# Buttons", "");
    IUFillTextVector(&JoystickInfoTP, JoystickInfoT, 5, getDeviceName(), "JOYSTICK_INFO", "Joystick Info",
                     MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    addDebugControl();

    return true;
}

bool JoyStick::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        char buf[8];
        // Name
        IUSaveText(&JoystickInfoT[0], driver->getName());
        // Version
        snprintf(buf, 8, "%d", driver->getVersion());
        IUSaveText(&JoystickInfoT[1], buf);
        // # of Joysticks
        snprintf(buf, 8, "%d", driver->getNumOfJoysticks());
        IUSaveText(&JoystickInfoT[2], buf);
        // # of Axes
        snprintf(buf, 8, "%d", driver->getNumOfAxes());
        IUSaveText(&JoystickInfoT[3], buf);
        // # of buttons
        snprintf(buf, 8, "%d", driver->getNumrOfButtons());
        IUSaveText(&JoystickInfoT[4], buf);

        defineText(&JoystickInfoTP);

        for (int i = 0; i < driver->getNumOfJoysticks(); i++)
            defineNumber(&JoyStickNP[i]);

        defineNumber(&AxisNP);
        defineSwitch(&ButtonSP);

        // N.B. Only set callbacks AFTER we define our properties above
        // because these calls backs otherwise can be called asynchronously
        // and they mess up INDI XML output
        driver->setJoystickCallback(joystickHelper);
        driver->setAxisCallback(axisHelper);
        driver->setButtonCallback(buttonHelper);
    }
    else
    {
        deleteProperty(JoystickInfoTP.name);

        for (int i = 0; i < driver->getNumOfJoysticks(); i++)
            deleteProperty(JoyStickNP[i].name);

        deleteProperty(AxisNP.name);
        deleteProperty(ButtonSP.name);

        delete[] JoyStickNP;
        delete[] JoyStickN;
        delete[] AxisN;
        delete[] ButtonS;
    }

    return true;
}

void JoyStick::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    defineText(&PortTP);
    loadConfig(true, INDI::SP::DEVICE_PORT);

    if (isConnected())
    {
        for (int i = 0; i < driver->getNumOfJoysticks(); i++)
            defineNumber(&JoyStickNP[i]);

        defineNumber(&AxisNP);
        defineSwitch(&ButtonSP);
    }
}

bool JoyStick::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool JoyStick::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PortTP.name) == 0)
        {
            PortTP.s = IPS_OK;
            IUUpdateText(&PortTP, texts, names, n);
            //  Update client display
            IDSetText(&PortTP, nullptr);

            driver->setPort(PortT[0].text);

            return true;
        }
    }

    return DefaultDevice::ISNewText(dev, name, texts, names, n);
}

void JoyStick::joystickHelper(int joystick_n, double mag, double angle)
{
    joystick->joystickEvent(joystick_n, mag, angle);
}

void JoyStick::buttonHelper(int button_n, int value)
{
    joystick->buttonEvent(button_n, value);
}

void JoyStick::axisHelper(int axis_n, int value)
{
    joystick->axisEvent(axis_n, value);
}

void JoyStick::joystickEvent(int joystick_n, double mag, double angle)
{
    if (!isConnected())
        return;

    if (isDebug())
        IDLog("joystickEvent[%d]: %g @ %g\n", joystick_n, mag, angle);

    if (mag == 0)
        JoyStickNP[joystick_n].s = IPS_IDLE;
    else
        JoyStickNP[joystick_n].s = IPS_BUSY;

    JoyStickNP[joystick_n].np[0].value = mag;
    JoyStickNP[joystick_n].np[1].value = angle;

    IDSetNumber(&JoyStickNP[joystick_n], nullptr);
}

void JoyStick::axisEvent(int axis_n, int value)
{
    if (!isConnected())
        return;

    if (isDebug())
        IDLog("axisEvent[%d]: %d\n", axis_n, value);

    if (value == 0)
        AxisNP.s = IPS_IDLE;
    else
        AxisNP.s = IPS_BUSY;

    AxisNP.np[axis_n].value = value;

    IDSetNumber(&AxisNP, nullptr);
}

void JoyStick::buttonEvent(int button_n, int value)
{
    if (!isConnected())
        return;

    if (isDebug())
        IDLog("buttonEvent[%d]: %s\n", button_n, value > 0 ? "On" : "Off");

    ButtonSP.s          = IPS_OK;
    ButtonS[button_n].s = (value == 0) ? ISS_OFF : ISS_ON;

    IDSetSwitch(&ButtonSP, nullptr);
}

bool JoyStick::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);

    IUSaveConfigText(fp, &PortTP);

    return true;
}
