/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

 Connection Plugin Interface

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

#include "indidevapi.h"

#include <functional>
#include <string>

namespace INDI
{
class DefaultDevice;
}

/**
 * Namespace to encapsulate all INDI Connection Plugins. Each INDI connection plugin is responsible of managing communications
 * with a specific physical or logical medium (e.g. serial or ethernet).
 */
namespace Connection
{
/**
 * @brief The Interface class class is the base class for all INDI connection plugins.
 *
 * Each plugin implements the connection details specific to a particular medium (e.g. serial). After the connection to the medium is successful, a <i>handshake</i> is
 * initialted to make sure the device is online and responding to commands. The child class employing the plugin must register
 * the handshake to perform the actual low-level communication with the device.
 *
 * @see Connection::Serial
 * @see Connection::TCP
 * @see INDI::Telescope utilizes both serial and tcp plugins to communicate with mounts.
 */
class Interface
{
  public:
    /**
     * @brief Connect Connect to device via the implemented communication medium. Do not perform any handshakes.
     * @return True if successful, false otherwise.
     */
    virtual bool Connect() = 0;

    /**
     * @brief Disconnect Disconnect from device.
     * @return True if successful, false otherwise.
     */
    virtual bool Disconnect() = 0;

    /**
     * @brief Activated Function called by the framework when the plugin is activated (i.e. selected by the user). It is usually used to define properties
     * pertaining to the specific plugin functionalities.
     */
    virtual void Activated() = 0;

    /**
     * @brief Deactivated Function called by the framework when the plugin is deactivated. It is usually used to delete properties by were defined
     * previously since the plugin is no longer active.
     */
    virtual void Deactivated() = 0;

    /**
     * @return Plugin name
     */
    virtual std::string name() = 0;

    /**
     * @return Plugin friendly label presented to the client/user.
     */
    virtual std::string label() = 0;

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

    virtual bool saveConfigItems(FILE *fp);

    /**
     * @brief registerHandshake Register a handshake function to be called once the intial connection to the device is established.
     * @param callback Handshake function callback
     * @see INDI::Telescope
     */
    void registerHandshake(std::function<bool()> callback);

  protected:
    Interface(INDI::DefaultDevice *dev);
    virtual ~Interface();

    const char *getDeviceName();
    std::function<bool()> Handshake;
    INDI::DefaultDevice *device = NULL;
};
}
