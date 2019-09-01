/*
    Rotator Interface
    Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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
#include <stdint.h>

/**
 * \class RotatorInterface
   \brief Provides interface to implement Rotator functionality.

   A Rotator can be an independent device, or an embedded Rotator within another device (usually a rotating focuser). Child class must implement all the
   pure virtual functions. Only absolute position Rotators are supported. Angle is ranged from 0 to 360 increasing clockwise when looking at the back
   of the camera.

   \e IMPORTANT: initRotatorProperties() must be called before any other function to initilize the Rotator properties.

   \e IMPORTANT: processRotatorNumber() must be called in your driver's ISNewNumber() function. Similary, processRotatorSwitch() must be called in ISNewSwitch()

\author Jasem Mutlaq
*/
namespace INDI
{

class RotatorInterface
{
  public:    
    /**
     * \struct RotatorCapability
     * \brief Holds the capabilities of a Rotator.
     */
    enum
    {
        ROTATOR_CAN_ABORT          = 1 << 0, /** Can the Rotator abort motion once started? */
        ROTATOR_CAN_HOME           = 1 << 1, /** Can the Rotator go to home position? */
        ROTATOR_CAN_SYNC           = 1 << 2, /** Can the Rotator sync to specific tick? */
        ROTATOR_CAN_REVERSE        = 1 << 3, /** Can the Rotator reverse direction? */
    } RotatorCapability;

    /**
     * @brief GetRotatorCapability returns the capability of the Rotator
     */
    uint32_t GetRotatorCapability() const { return rotatorCapability; }

    /**
     * @brief SetRotatorCapability sets the Rotator capabilities. All capabilities must be initialized.
     * @param cap pointer to Rotator capability struct.
     */
    void SetRotatorCapability(uint32_t cap) { rotatorCapability = cap; }

    /**
     * @return Whether Rotator can abort.
     */
    bool CanAbort() { return rotatorCapability & ROTATOR_CAN_ABORT;}

    /**
     * @return Whether Rotator can go to home position.
     */
    bool CanHome()  { return rotatorCapability & ROTATOR_CAN_HOME;}

    /**
     * @return Whether Rotator can sync ticks position to a new one.
     */
    bool CanSync()  { return rotatorCapability & ROTATOR_CAN_SYNC;}

    /**
     * @return Whether Rotator can reverse direction.
     */
    bool CanReverse()  { return rotatorCapability & ROTATOR_CAN_REVERSE;}

protected:
    explicit RotatorInterface(DefaultDevice *defaultDevice);

    /**
     * \brief Initilize Rotator properties. It is recommended to call this function within
     * initProperties() of your primary device
     * \param groupName Group or tab name to be used to define Rotator properties.
     */
    void initProperties(const char *groupName);

    /**
     * @brief updateRotatorProperties Define or Delete Rotator properties based on the connection status of the base device
     * @return True if successful, false otherwise.
     */
    bool updateProperties();

    /** \brief Process Rotator number properties */
    bool processNumber(const char *dev, const char *name, double values[], char *names[], int n);

    /** \brief Process Rotator switch properties */
    bool processSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    /**
     * @brief MoveRotator Go to specific angle
     * @param angle Target angle in degrees.
     * @return State of operation: IPS_OK is motion is completed, IPS_BUSY if motion in progress, IPS_ALERT on error.
     */
    virtual IPState MoveRotator(double angle) = 0;

    /**
     * @brief SyncRotator Set current angle as the supplied angle without moving the rotator.
     * @param ticks Desired new angle.
     * @return True if successful, false otherwise.
     */
    virtual bool SyncRotator(double angle);

    /**
     * @brief HomeRotator Go to home position.
     * @return State of operation: IPS_OK is motion is completed, IPS_BUSY if motion in progress, IPS_ALERT on error.
     */
    virtual IPState HomeRotator();

    /**
     * @brief ReverseRotator Reverse the direction of the rotator. CW is usually the normal direction, and CCW is the reversed direction.
     * @param enable if True, reverse direction. If false, revert to normal direction.
     * @return True if successful, false otherwise.
     */
    virtual bool ReverseRotator(bool enabled);

    /**
     * @brief AbortRotator Abort all motion
     * @return True if successful, false otherwise.
     */
    virtual bool AbortRotator();

    INumber GotoRotatorN[1];
    INumberVectorProperty GotoRotatorNP;

    INumber SyncRotatorN[1];
    INumberVectorProperty SyncRotatorNP;

    ISwitch AbortRotatorS[1];
    ISwitchVectorProperty AbortRotatorSP;

    ISwitch HomeRotatorS[1];
    ISwitchVectorProperty HomeRotatorSP;

    ISwitch ReverseRotatorS[2];
    ISwitchVectorProperty ReverseRotatorSP;
    enum { REVERSE_ENABLED, REVERSE_DISABLED };

    uint32_t rotatorCapability = 0;
    DefaultDevice *m_defaultDevice { nullptr };
};

}
