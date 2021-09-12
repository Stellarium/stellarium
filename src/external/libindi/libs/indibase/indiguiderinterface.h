/*
    Guider Interface
    Copyright (C) 2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indibase.h"

/**
 * @class GuiderInterface
 * @brief Provides interface to implement guider (ST4) port functionality.
 *
 * The child class implements GuideXXXX() functions and returns:
 * IPS_OK if the guide operation is completed in the function, which is usually appropriate for
 * very short guiding pulses.
 * IPS_BUSY if the guide operation is in progress and will take time to complete. In this
 * case, the child class must call GuideComplete() once the guiding pulse is complete.
 * IPS_ALERT if the guide operation failed.
 *
 * \e IMPORTANT: initGuiderProperties() must be called before any other function to initialize
 * the guider properties.
 * \e IMPORATNT: processGuiderProperties() must be called in your driver's ISNewNumber(..)
 * function. processGuiderProperties() will call the guide functions
 * GuideXXXX functions according to the driver.
 *
 * @author Jasem Mutlaq
 */

#include <stdint.h>

namespace INDI
{

class GuiderInterface
{
  public:
    /**
     * @brief Guide north for ms milliseconds. North is defined as DEC+
     * @return IPS_OK if operation is completed successfully, IPS_BUSY if operation will take take to
     * complete, or IPS_ALERT if operation failed.
     */
    virtual IPState GuideNorth(uint32_t ms) = 0;

    /**
     * @brief Guide south for ms milliseconds. South is defined as DEC-
     * @return IPS_OK if operation is completed successfully, IPS_BUSY if operation will take take to
     * complete, or IPS_ALERT if operation failed.
     */
    virtual IPState GuideSouth(uint32_t ms) = 0;

    /**
     * @brief Guide east for ms milliseconds. East is defined as RA+
     * @return IPS_OK if operation is completed successfully, IPS_BUSY if operation will take take to
     * complete, or IPS_ALERT if operation failed.
     */
    virtual IPState GuideEast(uint32_t ms) = 0;

    /**
     * @brief Guide west for ms milliseconds. West is defined as RA-
     * @return IPS_OK if operation is completed successfully, IPS_BUSY if operation will take take to
     * complete, or IPS_ALERT if operation failed.
     */
    virtual IPState GuideWest(uint32_t ms) = 0;

    /**
     * @brief Call GuideComplete once the guiding pulse is complete.
     * @param axis Axis of completed guiding operation.
     */
    virtual void GuideComplete(INDI_EQ_AXIS axis);

  protected:
    GuiderInterface();
    ~GuiderInterface();

    /**
     * @brief Initilize guider properties. It is recommended to call this function within
     * initProperties() of your primary device
     * @param deviceName Name of the primary device
     * @param groupName Group or tab name to be used to define guider properties.
     */
    void initGuiderProperties(const char *deviceName, const char *groupName);

    /**
     * @brief Call this function whenever client updates GuideNSNP or GuideWSP properties in the
     * primary device. This function then takes care of issuing the corresponding GuideXXXX
     * function accordingly.
     * @param name device name
     * @param values value as passed by the client
     * @param names names as passed by the client
     * @param n number of values and names pair to process.
     */
    void processGuiderProperties(const char *name, double values[], char *names[], int n);

    INumber GuideNSN[2];
    INumberVectorProperty GuideNSNP;
    INumber GuideWEN[2];
    INumberVectorProperty GuideWENP;
};
}
