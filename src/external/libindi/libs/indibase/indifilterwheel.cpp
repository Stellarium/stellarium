/*******************************************************************************
  Copyright(c) 2010, 2011 Gerry Rozema. All rights reserved.

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

#include "indifilterwheel.h"

#include "indicontroller.h"
#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"

#include <cstring>

namespace INDI
{

FilterWheel::FilterWheel() : FilterInterface(this)
{
    controller = new Controller(this);

    controller->setJoystickCallback(joystickHelper);
    controller->setButtonCallback(buttonHelper);
}

bool FilterWheel::initProperties()
{
    DefaultDevice::initProperties();

    FilterInterface::initProperties(FILTER_TAB);

    controller->mapController("Change Filter", "Change Filter", Controller::CONTROLLER_JOYSTICK, "JOYSTICK_1");
    controller->mapController("Reset", "Reset", Controller::CONTROLLER_BUTTON, "BUTTON_1");

    controller->initProperties();

    setDriverInterface(FILTER_INTERFACE);

    if (filterConnection & CONNECTION_SERIAL)
    {
        serialConnection = new Connection::Serial(this);
        serialConnection->registerHandshake([&]() { return callHandshake(); });
        registerConnection(serialConnection);
    }

    if (filterConnection & CONNECTION_TCP)
    {
        tcpConnection = new Connection::TCP(this);
        tcpConnection->registerHandshake([&]() { return callHandshake(); });

        registerConnection(tcpConnection);
    }

    return true;
}

void FilterWheel::ISGetProperties(const char *dev)
{
    DefaultDevice::ISGetProperties(dev);
    if (isConnected())
        FilterInterface::updateProperties();

    controller->ISGetProperties(dev);
    return;
}

bool FilterWheel::updateProperties()
{
    // Update default device
    DefaultDevice::updateProperties();

    // Update Filter Interface
    FilterInterface::updateProperties();

    // Update controller
    controller->updateProperties();

    return true;
}

bool FilterWheel::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    controller->ISNewSwitch(dev, name, states, names, n);
    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool FilterWheel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "FILTER_SLOT") == 0)
        {
            FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }
    }

    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool FilterWheel::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FilterNameTP->name) == 0)
        {
            FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }

    controller->ISNewText(dev, name, texts, names, n);
    return DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool FilterWheel::saveConfigItems(FILE *fp)
{
    DefaultDevice::saveConfigItems(fp);

    FilterInterface::saveConfigItems(fp);

    controller->saveConfigItems(fp);

    return true;
}

int FilterWheel::QueryFilter()
{
    return -1;
}

bool FilterWheel::SelectFilter(int)
{
    return false;
}

bool FilterWheel::ISSnoopDevice(XMLEle *root)
{
    controller->ISSnoopDevice(root);

    return DefaultDevice::ISSnoopDevice(root);
}

void FilterWheel::joystickHelper(const char *joystick_n, double mag, double angle, void *context)
{
    static_cast<FilterWheel *>(context)->processJoystick(joystick_n, mag, angle);
}

void FilterWheel::buttonHelper(const char *button_n, ISState state, void *context)
{
    static_cast<FilterWheel *>(context)->processButton(button_n, state);
}

void FilterWheel::processJoystick(const char *joystick_n, double mag, double angle)
{
    if (!strcmp(joystick_n, "Change Filter"))
    {
        // Put high threshold
        if (mag > 0.9)
        {
            // North
            if (angle > 0 && angle < 180)
            {
                // Previous switch
                if (FilterSlotN[0].value == FilterSlotN[0].min)
                    TargetFilter = FilterSlotN[0].max;
                else
                    TargetFilter = FilterSlotN[0].value - 1;

                SelectFilter(TargetFilter);
            }
            // South
            if (angle > 180 && angle < 360)
            {
                // Next Switch
                if (FilterSlotN[0].value == FilterSlotN[0].max)
                    TargetFilter = FilterSlotN[0].min;
                else
                    TargetFilter = FilterSlotN[0].value + 1;

                SelectFilter(TargetFilter);
            }
        }
    }
}

void FilterWheel::processButton(const char *button_n, ISState state)
{
    //ignore OFF
    if (state == ISS_OFF)
        return;

    // Reset
    if (!strcmp(button_n, "Reset"))
    {
        TargetFilter = FilterSlotN[0].min;
        SelectFilter(TargetFilter);
    }
}

bool FilterWheel::Handshake()
{
    return false;
}

bool FilterWheel::callHandshake()
{
    if (filterConnection > 0)
    {
        if (getActiveConnection() == serialConnection)
            PortFD = serialConnection->getPortFD();
        else if (getActiveConnection() == tcpConnection)
            PortFD = tcpConnection->getPortFD();
    }

    return Handshake();
}

void FilterWheel::setFilterConnection(const uint8_t &value)
{
    uint8_t mask = CONNECTION_SERIAL | CONNECTION_TCP | CONNECTION_NONE;

    if (value == 0 || (mask & value) == 0)
    {
        DEBUGF(Logger::DBG_ERROR, "Invalid connection mode %d", value);
        return;
    }

    filterConnection = value;
}
}
