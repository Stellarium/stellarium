/*
    Filter Interface
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

#include <stdint.h>

/**
 * \class FocuserInterface
   \brief Provides interface to implement focuser functionality.

   A focuser can be an independent device, or an embedded focuser within another device (e.g. Camera).

   \e IMPORTANT: initFocuserProperties() must be called before any other function to initialize the focuser properties.

   \e IMPORTANT: processFocuserNumber() and processFocuserSwitch() must be called in your driver's ISNewNumber() and ISNewSwitch functions recepectively.
\author Jasem Mutlaq
*/
namespace INDI
{

class FocuserInterface
{
  public:
    enum FocusDirection
    {
        FOCUS_INWARD,
        FOCUS_OUTWARD
    };

    enum
    {
        FOCUSER_CAN_ABS_MOVE       = 1 << 0, /*!< Can the focuser move by absolute position? */
        FOCUSER_CAN_REL_MOVE       = 1 << 1, /*!< Can the focuser move by relative position? */
        FOCUSER_CAN_ABORT          = 1 << 2, /*!< Is it possible to abort focuser motion? */
        FOCUSER_HAS_VARIABLE_SPEED = 1 << 3  /*!< Can the focuser move in different configurable speeds? */
    } FocuserCapability;

    /**
     * @brief GetFocuserCapability returns the capability of the focuser
     */
    uint32_t GetFocuserCapability() const { return capability; }

    /**
     * @brief SetFocuserCapability sets the focuser capabilities. All capabilities must be initialized.
     * @param cap pointer to focuser capability struct.
     */
    void SetFocuserCapability(uint32_t cap);

    /**
     * @return True if the focuser has absolute position encoders.
     */
    bool CanAbsMove() { return capability & FOCUSER_CAN_ABS_MOVE; }

    /**
     * @return True if the focuser has relative position encoders.
     */
    bool CanRelMove() { return capability & FOCUSER_CAN_REL_MOVE; }

    /**
     * @return True if the focuser motion can be aborted.
     */
    bool CanAbort() { return capability & FOCUSER_CAN_ABORT; }

    /**
     * @return True if the focuser has multiple speeds.
     */
    bool HasVariableSpeed() { return capability & FOCUSER_HAS_VARIABLE_SPEED; }

  protected:
    FocuserInterface() = default;
    virtual ~FocuserInterface() = default;

    /**
     * \brief Initilize focuser properties. It is recommended to call this function within
     * initProperties() of your primary device
     * \param deviceName Name of the primary device
     * \param groupName Group or tab name to be used to define focuser properties.
     */
    void initFocuserProperties(const char *deviceName, const char *groupName);

    /** \brief Process focus number properties */
    bool processFocuserNumber(const char *dev, const char *name, double values[], char *names[], int n);

    /** \brief Process focus switch properties */
    bool processFocuserSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    /**
     * @brief SetFocuserSpeed Set Focuser speed
     * @param speed focuser speed
     * @return true if successful, false otherwise
     */
    virtual bool SetFocuserSpeed(int speed);

    /**
     * \brief MoveFocuser the focuser in a particular direction with a specific speed for a
     * finite duration.
     * \param dir Direction of focuser, either FOCUS_INWARD or FOCUS_OUTWARD.
     * \param speed Speed of focuser if supported by the focuser.
     * \param duration The timeout in milliseconds before the focus motion halts.
     * \return Return IPS_OK if motion is completed and focuser reached requested position.
     * Return IPS_BUSY if focuser started motion to requested position and is in progress.
     * Return IPS_ALERT if there is an error.
     */
    virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration);

    /**
     * \brief MoveFocuser the focuser to an absolute position.
     * \param ticks The new position of the focuser.
     * \return Return IPS_OK if motion is completed and focuser reached requested position. Return
     * IPS_BUSY if focuser started motion to requested position and is in progress.
     * Return IPS_ALERT if there is an error.
     */
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);

    /**
     * \brief MoveFocuser the focuser to an relative position.
     * \param dir Direction of focuser, either FOCUS_INWARD or FOCUS_OUTWARD.
     * \param ticks The relative ticks to move.
     * \return Return IPS_OK if motion is completed and focuser reached requested position. Return
     * IPS_BUSY if focuser started motion to requested position and is in progress.
     * Return IPS_ALERT if there is an error.
     */
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);

    /**
     * @brief AbortFocuser all focus motion
     * @return True if abort is successful, false otherwise.
     */
    virtual bool AbortFocuser();

    INumberVectorProperty FocusSpeedNP;
    INumber FocusSpeedN[1];
    ISwitchVectorProperty FocusMotionSP; //  A Switch in the client interface to park the scope
    ISwitch FocusMotionS[2];
    INumberVectorProperty FocusTimerNP;
    INumber FocusTimerN[1];
    INumberVectorProperty FocusAbsPosNP;
    INumber FocusAbsPosN[1];
    INumberVectorProperty FocusRelPosNP;
    INumber FocusRelPosN[1];
    ISwitchVectorProperty AbortSP;
    ISwitch AbortS[1];

    uint32_t capability;

    char focuserName[MAXINDIDEVICE];
    double lastTimerValue;
};
}
