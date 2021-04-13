/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  List of INDI Stanadrd Properties

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

#include "indibase.h"

namespace INDI
{

/**
 * @namespace INDI::SP
   @brief INDI Standard Properties are common properties standarized across drivers and clients alike.

INDI does not place any special semantics on property names (i.e. properties are just texts, numbers, or switches that represent no physical function). While GUI clients can construct graphical representation of properties in order to permit the user to operate the device, we run into situations where clients and drivers need to agree on the exact meaning of some fundamental properties.
What if some client need to be aware of the existence of some property in order to perform some function useful to the user? How can that client tie itself to such a property if the property can be arbitrary defined by drivers?

The solution is to define Standard Properties in order to establish a level of interoperability among INDI drivers and clients. We propose a set of shared INDI properties that encapsulate the most common characteristics of astronomical instrumentation of interest.
If the semantics of such properties are properly defined, not only it will insure base interoperability, but complete device automation becomes possible as well. Put another way, INDI standard properties are in essence properties that represent a clearly defined characteristic related to the operation of the device drivers.

For example, a very common standard property is EQUATORIAL_EOD_COORD. This property represents the telescope's current RA and DEC. Clients need to be aware of this property in order to, for example, draw the telescope's cross hair on the sky map. If you write a script to control a telescope, you know that any telescope supporting EQUATORIAL_EOD_COORD will behave in an expected manner when the property is invoked.
INDI clients are required to honor standard properties if when and they implement any functions associated with a particular standard property. Furthermore, INDI drivers employing standard properties should strictly adhere to the standard properties structure as defined next.

The properties are defined as string constants. To refer to the property in device drivers, use INDI::StandardProperty::PROPERTY_NAME or the shortcut INDI::SP::PROPERTY_NAME.

The standard properties are divided into the following categories:
<ol>
<li>@ref GeneralProperties "General Properties" shared across multiple devices.</li>
<li>@ref Connection::Interface "Connection Properties"
<ul>
    <li>@ref SerialProperties "Serial Properties" used to communicate with and manage serial devices (including Bluetooth).</li>
    <li>@ref TCPProperties "TCP Properties" used to communicate with and manage devices over the network.</li>
</ul>
</li>
</ol>
@author Jasem Mutlaq
*/
namespace SP
{
/**
 * \defgroup GeneralProperties Standard Properties - General: Common properties shared across devices of multiple genres.
 * The following tables describe standard properties pertaining to generic devices. The name of a standard property and its members must be
 * strictly reserved in all drivers. However, it is permissible to change the label element of properties. You can find numerous uses of the
 * standard properties in the INDI library driver repository.
 *
 * As a <b>general</b> rule of the thumb, the status of properties reflects the command execution result:
 * IPS_OKAY: Command excuted successfully.
 * IPS_BUSY: Command execution under progress.
 * IPS_ALERT: Command execution failed.
 */

/*@{*/

/**
 * @brief Connect to and disconnect from device.
 * Name | Type | Member | Default | Description
 * ---- | ---- | ------ | ------- | -----------
 * CONNECTION | SWITCH | CONNECT | OFF | Establish connection to device
 * CONNECTION | SWITCH | DISCONNECT | ON | Disconnect device
 */
extern const char *CONNECTION;

/*@}*/

/**
 * \defgroup SerialProperties Standard Properties - Serial: Properties used to communicate with and manage serial devices.
 * Serial communication over RS232/485 and Bluetooth. Unless otherwise noted, all the properties are saved in the configuration file so that they are remembered across sessions.
 */

/*@{*/

/**
 * @brief Device serial (or bluetooth) connection port. The default value on Linux is <i>/dev/ttyUSB0</i> while on MacOS it is <i>/dev/cu.usbserial</i>
 * It is part of Connection::SerialInterface to manage connections to serial devices.
 * Name | Type | Member | Default | Description
 * ---- | ---- | ------ | ------- | -----------
 * DEVICE_PORT | TEXT | PORT | /dev/ttyUSB0 | Device serial connection port
 */
extern const char *DEVICE_PORT;

/**
 * @brief Toggle device auto search.
 * If enabled and on connection failure with the default port, the SerialInterface class shall scan the system for available
 * serial ports and attempts connection and handshake with each until successful. Please note if this option is enabled it can take
 * a while before connection is established depending on how many ports are available on the system and the handshake timeout of the
 * the underlying device.
 * Name | Type | Member | Default | Description
 * ---- | ---- | ------ | ------- | -----------
 * DEVICE_AUTO_SEARCH | SWITCH | ENABLED | ON | Auto Search ON
 * DEVICE_AUTO_SEARCH | SWITCH | DISABLED | OFF | Auto Search OFF
 */
extern const char *DEVICE_AUTO_SEARCH;

/**
 * @brief Set device baud rate
 * Name | Type | Member | Default | Description
 * ---- | ---- | ------ | ------- | -----------
 * DEVICE_BAUD_RATE | SWITCH | 9600 | ON | 9600
 * DEVICE_BAUD_RATE | SWITCH | 19200 | OFF | 19200
 * DEVICE_BAUD_RATE | SWITCH | 38400 | OFF | 38400
 * DEVICE_BAUD_RATE | SWITCH | 57600 | OFF | 57600
 * DEVICE_BAUD_RATE | SWITCH | 115200 | OFF | 115200
 * DEVICE_BAUD_RATE | SWITCH | 230400 | OFF | 230400
 */
extern const char *DEVICE_BAUD_RATE;

/*@}*/

/**
 * \defgroup TCPProperties Standard Properties - TCP: Properties used to communicate with and manage devices over the network.
 * Communication with devices over TCP/IP. Unless otherwise noted, all the properties are saved in the configuration file so that they are remembered across sessions.
 */

/*@{*/

/**
 * @brief Device hostname and port.
 * It is part of Connection::TCPInterface to manage connections to devices over the network.
 * Name | Type | Member | Default | Description
 * ---- | ---- | ------ | ------- | -----------
 * DEVICE_ADDRESS | TEXT | ADDRESS |  | Device hostname or IP Address
 * DEVICE_ADDRESS | TEXT | PORT |  | Device port
 */
extern const char *DEVICE_ADDRESS;

/*@}*/

}
} // namespace INDI
