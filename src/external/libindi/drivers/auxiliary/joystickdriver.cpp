/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

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

#include "joystickdriver.h"

#include <cstring>

#define MAX_JOYSTICKS 3

JoyStickDriver::JoyStickDriver()
{
    active      = false;
    pollMS      = 100;
    joystick_fd = 0;
    joystick_ev = new js_event();
    joystick_st = new joystick_state();

    strncpy(dev_path, JOYSTICK_DEV, 256);

    joystickCallbackFunc = joystickEvent;
    axisCallbackFunc     = axisEvent;
    buttonCallbackFunc   = buttonEvent;
}

JoyStickDriver::~JoyStickDriver()
{
    Disconnect();
    delete joystick_st;
    delete joystick_ev;
}

void JoyStickDriver::setJoystickCallback(joystickFunc JoystickCallback)
{
    joystickCallbackFunc = JoystickCallback;
}

void JoyStickDriver::setAxisCallback(axisFunc AxisCallback)
{
    axisCallbackFunc = AxisCallback;
}

void JoyStickDriver::setButtonCallback(buttonFunc buttonCallback)
{
    buttonCallbackFunc = buttonCallback;
}

void JoyStickDriver::setPort(const char *port)
{
    strncpy(dev_path, port, 256);
}

bool JoyStickDriver::Connect()
{
    joystick_fd = open(dev_path, O_RDONLY | O_NONBLOCK);

    if (joystick_fd > 0)
    {
        ioctl(joystick_fd, JSIOCGNAME(256), name);
        ioctl(joystick_fd, JSIOCGVERSION, &version);
        ioctl(joystick_fd, JSIOCGAXES, &axes);
        ioctl(joystick_fd, JSIOCGBUTTONS, &buttons);

        joystick_st->axis.reserve(axes);
        joystick_st->button.reserve(buttons);
        active = true;
        pthread_create(&thread, 0, &JoyStickDriver::loop, this);
        return true;
    }

    return false;
}

bool JoyStickDriver::Disconnect()
{
    if (joystick_fd > 0)
    {
        active = false;
        pthread_join(thread, 0);
        close(joystick_fd);
    }

    joystick_fd = 0;

    return true;
}

void *JoyStickDriver::loop(void *obj)
{
    while (reinterpret_cast<JoyStickDriver *>(obj)->active)
        reinterpret_cast<JoyStickDriver *>(obj)->readEv();
    return obj;
}

void JoyStickDriver::readEv()
{
    int bytes = read(joystick_fd, joystick_ev, sizeof(*joystick_ev));
    if (bytes > 0)
    {
        joystick_ev->type &= ~JS_EVENT_INIT;
        if (joystick_ev->type & JS_EVENT_BUTTON)
        {
            joystick_st->button[joystick_ev->number] = joystick_ev->value;
            buttonCallbackFunc(joystick_ev->number, joystick_ev->value);
        }
        if (joystick_ev->type & JS_EVENT_AXIS)
        {
            joystick_st->axis[joystick_ev->number] = joystick_ev->value;
            int joystick_n                         = joystick_ev->number;
            if (joystick_n % 2 != 0)
                joystick_n--;
            if (joystick_n / 2.0 < MAX_JOYSTICKS)
            {
                joystick_position pos = joystickPosition(joystick_n);
                joystickCallbackFunc(joystick_n / 2, pos.r, pos.theta);
            }

            axisCallbackFunc(joystick_ev->number, joystick_ev->value);
        }
    }
    else
        usleep(pollMS * 1000);
}

joystick_position JoyStickDriver::joystickPosition(int n)
{
    joystick_position pos;

    if (n > -1 && n < axes)
    {
        int i0 = n, i1 = n + 1;
        float x0 = joystick_st->axis[i0] / 32767.0f, y0 = -joystick_st->axis[i1] / 32767.0f;
        float x = x0 * sqrt(1 - pow(y0, 2) / 2.0f), y = y0 * sqrt(1 - pow(x0, 2) / 2.0f);

        pos.x = x0;
        pos.y = y0;

        pos.theta = atan2(y, x) * (180.0 / 3.141592653589);
        pos.r     = sqrt(pow(y, 2) + pow(x, 2));

        // For direction keys and scale/throttle keys
        if (pos.r == 0)
        {
            pos.r = -joystick_st->axis[i1];

            if (pos.r < 0)
            {
                // Left
                if ((i0 % 2 == 0))
                    pos.theta = 180;
                // Down
                else
                    pos.theta = 270;
            }
            else
            {
                // Up
                if ((i0 % 2 == 0))
                    pos.theta = 90;
                // Right
                else
                    pos.theta = 0;
            }
        }
        else if (pos.theta < 0)
            pos.theta += 360;

        // Make sure to reset angle if magnitude is zero
        if (pos.r == 0)
            pos.theta = 0;
    }
    else
    {
        pos.theta = pos.r = pos.x = pos.y = 0.0f;
    }

    return pos;
}

bool JoyStickDriver::buttonPressed(int n)
{
    return n > -1 && n < buttons ? joystick_st->button[n] : 0;
}

void JoyStickDriver::setPoll(int ms)
{
    pollMS = ms;
}

void JoyStickDriver::joystickEvent(int joystick_n, double mag, double angle)
{
    (void)joystick_n;
    (void)mag;
    (void)angle;
}

void JoyStickDriver::axisEvent(int axis_n, int value)
{
    (void)axis_n;
    (void)value;
}

void JoyStickDriver::buttonEvent(int button_n, int button_value)
{
    (void)button_n;
    (void)button_value;
}

const char *JoyStickDriver::getName()
{
    return name;
}

__u32 JoyStickDriver::getVersion()
{
    return version;
}

__u8 JoyStickDriver::getNumOfJoysticks()
{
    int n_joysticks = axes / 2;

    if (n_joysticks > MAX_JOYSTICKS)
        n_joysticks = MAX_JOYSTICKS;

    return n_joysticks;
}

__u8 JoyStickDriver::getNumOfAxes()
{
    return axes;
}

__u8 JoyStickDriver::getNumrOfButtons()
{
    return buttons;
}
