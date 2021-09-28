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

#pragma once

#include "defaultdevice.h"

#include <ciso646> // detect std::lib
#include <functional>

namespace INDI
{
/**
 * \class INDI::Controller
 * @brief The Controller class provides functionality to access a controller (e.g. joystick) input and send it to the requesting driver.
 *
 * - To use the class in an INDI::DefaultDevice based driver:
 *
 *    -# Set the callback functions for each of input type requested.
 *
 *    -# Map the properties you wish to /e listen to from the joystick driver by specifying
 *       the type of input requested and the initial default value.
 *       For example:
 *       \code{.cpp}
 *       mapController("ABORT", "Abort Telescope", INDI::Controller::CONTROLLER_BUTTON, "BUTTON_1");
 *       \endcode
 *       After mapping all the desired controls, call the class initProperties() function.
 *       If the user enables joystick support in the driver and presses a button on the joystick, the button
 *       callback function will be invoked with the name & state of the button.
 *
 *    -# Call the Controller's ISGetProperties(), initProperties(), updateProperties(), saveConfigItems(), and
 *       ISNewXXX functions from the same standard functions in your driver.
 *
 * The class communicates with INDI joystick driver which in turn enumerates the game pad and provides
 * three types of constrcuts:
 * <ul>
 * <li><b>Joysticks</b>: Each joystick displays a normalized magnitude [0 to 1] and an angle. The angle is measured counter clock wise starting from
 *  the right/east direction [0 to 360]. They are defined as JOYSTICK_# where # is the joystick number.</li>
 * <li><b>Axes</b>: Each joystick has two or more axes. Each axis has a raw value and angle. The raw value ranges from -32767.0 to 32767.0 They are
 * defined as AXIS_# where # is the axis number.</li>
 * <li><b>Buttons</b>: Buttons are either on or off. They are defined as BUTTON_# where # is the button number.</li>
 * </ul>
 *
 * \note All indexes start from 1. i.e. There is no BUTTON_0 or JOYSTICK_0.
 * \see See the LX200 Generic & Celestron GPS drivers for an example implementation.
 * \warning Both the indi_joystick driver and the driver using this class must be running in the same INDI server
 *         (or chained INDI servers) in order for it to work as it depends on snooping among drivers.
 * \author Jasem Multaq
 */
class Controller
{
  public:
    typedef enum { CONTROLLER_JOYSTICK, CONTROLLER_AXIS, CONTROLLER_BUTTON, CONTROLLER_UNKNOWN } ControllerType;

    /**
         * @brief joystickFunc Joystick callback function signature.
         */
    typedef std::function<void(const char *joystick_n, double mag, double angle, void *context)> joystickFunc;

    /**
         * @brief axisFunc Axis callback function signature.
         */
    typedef std::function<void(const char *axis_n, double value, void *context)> axisFunc;

    /**
         * @brief buttonFunc Button callback function signature.
         */
    typedef std::function<void(const char *button_n, ISState state, void *context)> buttonFunc;

    /**
         * @brief Controller Default ctor
         * @param cdevice INDI::DefaultDevice device
         */
    Controller(INDI::DefaultDevice *cdevice);
    virtual ~Controller();

    virtual void ISGetProperties(const char *dev);
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISSnoopDevice(XMLEle *root);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool saveConfigItems(FILE *fp);

    /**
         * @brief mapController adds a new property to the joystick's settings.
         * @param propertyName Name
         * @param propertyLabel Label
         * @param type The input type of the property. This value cannot be updated.
         * @param initialValue Initial value for the property.
         */
    void mapController(const char *propertyName, const char *propertyLabel, ControllerType type,
                       const char *initialValue);

    /**
         * @brief clearMap clears all properties added previously by mapController()
         */
    void clearMap();

    /**
         * @brief setJoystickCallback Sets the callback function when a new joystick input is detected.
         * @param joystickCallback the callback function.
         */
    void setJoystickCallback(joystickFunc joystickCallback);

    /**
         * @brief setAxisCallback Sets the callback function when a new axis input is detected.
         * @param axisCallback the callback function.
         */
    void setAxisCallback(axisFunc axisCallback);

    /**
         * @brief setButtonCallback Sets the callback function when a new button input is detected.
         * @param buttonCallback the callback function.
         */
    void setButtonCallback(buttonFunc buttonCallback);

    ControllerType getControllerType(const char *name);
    const char *getControllerSetting(const char *name);

  protected:
    static void joystickEvent(const char *joystick_n, double mag, double angle, void *context);
    static void axisEvent(const char *axis_n, int value, void *context);
    static void buttonEvent(const char *button_n, int value, void *context);

    void enableJoystick();
    void disableJoystick();

    joystickFunc joystickCallbackFunc;
    buttonFunc buttonCallbackFunc;
    axisFunc axisCallbackFunc;

    INDI::DefaultDevice *device;

  private:
    /* Joystick Support */
    ISwitchVectorProperty UseJoystickSP;
    ISwitch UseJoystickS[2];

    ITextVectorProperty JoystickSettingTP;
    IText *JoystickSettingT = nullptr;
};
}
