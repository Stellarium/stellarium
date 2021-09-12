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

#include "connectioninterface.h"

#include <stdint.h>
#include <cstdlib>
#include <string>

namespace Connection
{
/**
 * @brief The TCP class manages connection with devices over the network via TCP/IP.
 * Upon successfull connection, reads & writes from and to the device are performed via the returned file descriptor
 * using standard UNIX read/write functions.
 */

class TCP : public Interface
{
  public:
    enum ConnectionType
    {
        TYPE_TCP = 0,
        TYPE_UDP
    };

    TCP(INDI::DefaultDevice *dev);
    virtual ~TCP() = default;

    virtual bool Connect();

    virtual bool Disconnect();

    virtual void Activated();

    virtual void Deactivated();

    virtual std::string name() { return "CONNECTION_TCP"; }

    virtual std::string label() { return "Ethernet"; }

    virtual const char *host() const { return AddressT[0].text; }
    virtual uint32_t port() const { return atoi(AddressT[1].text); }
    ConnectionType connectionType() const { return static_cast<ConnectionType>(IUFindOnSwitchIndex(&TcpUdpSP)); }

    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool saveConfigItems(FILE *fp);

    int getPortFD() const { return PortFD; }
    void setDefaultHost(const char *addressHost);
    void setDefaultPort(uint32_t addressPort);
    void setConnectionType(int type);

  protected:
    // IP Address/Port
    ITextVectorProperty AddressTP;
    IText AddressT[2] {};

    ISwitch TcpUdpS[2];
    ISwitchVectorProperty TcpUdpSP;

    int sockfd                   = -1;
    const uint8_t SOCKET_TIMEOUT = 5;

    int PortFD = -1;
};
}
