/*******************************************************************************
  Copyright(c) 2013 Jasem Mutlaq. All rights reserved.

  Based on code by Keith Lantz.

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

#include <iostream>
#include <functional>
#include <fcntl.h>
#include <pthread.h>
#include <cmath>
#include <linux/joystick.h>
#include <vector>
#include <unistd.h>

#define JOYSTICK_DEV "/dev/input/js0"

typedef struct
{
    float theta, r, x, y;
} joystick_position;

typedef struct
{
    std::vector<signed short> button;
    std::vector<signed short> axis;
} joystick_state;

/**
 * @brief The JoyStickDriver class provides basic functionality to read events from supported game pads under Linux.
 * It provides functions to read the button, axis, and joystick status and values. By definition, a joystick is the combination of two axis.
 * A game pad may have one or more joysticks depending on the number of reported axis. You can utilize the class in an event driven fashion by using callbacks.
 * The callbacks have a specific signature and must be set. Alternatively, you may query the status and position of the buttons & axis at any time as well.
 *
 * Since the class runs a non-blocking thread, the thread goes to sleep when there are no events detected in order to reduce CPU utilization. The sleep period is set by
 * default to 100 milliseconds and can be adjusted by the \i setPoll function.
 *
 * Each joystick has a normalized magnitude [0 to 1] and an angle. The magnitude is 0 when the stick is not depressed, and 1 when depressed all the way.
 * The angles are measured counter clock wise [0 to 360] with right/east direction being zero.
 *
 * The axis value is reported in raw values [-32767.0 to 32767.0].
 *
 * The buttons are either off (0) or on (1)
 *
 */
class JoyStickDriver
{
  public:
    JoyStickDriver();
    ~JoyStickDriver();

    typedef std::function<void(int joystick_n, double mag, double angle)> joystickFunc;
    typedef std::function<void(int axis_n, double value)> axisFunc;
    typedef std::function<void(int button_n, int value)> buttonFunc;

    bool Connect();
    bool Disconnect();

    void setPort(const char *port);
    void setPoll(int ms);

    const char *getName();
    __u32 getVersion();
    __u8 getNumOfJoysticks();
    __u8 getNumOfAxes();
    __u8 getNumrOfButtons();

    joystick_position joystickPosition(int n);
    bool buttonPressed(int n);

    void setJoystickCallback(joystickFunc joystickCallback);
    void setAxisCallback(axisFunc axisCallback);
    void setButtonCallback(buttonFunc buttonCallback);

  protected:
    static void joystickEvent(int joystick_n, double mag, double angle);
    static void axisEvent(int axis_n, int value);
    static void buttonEvent(int button_n, int value);

    static void *loop(void *obj);
    void readEv();

    joystickFunc joystickCallbackFunc;
    buttonFunc buttonCallbackFunc;
    axisFunc axisCallbackFunc;

  private:
    pthread_t thread;
    bool active;
    int joystick_fd;
    js_event *joystick_ev;
    joystick_state *joystick_st;
    __u32 version;
    __u8 axes;
    __u8 buttons;
    char name[256];
    char dev_path[256];
    int pollMS;
};
