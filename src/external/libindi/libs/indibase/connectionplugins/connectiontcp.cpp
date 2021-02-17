/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

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

#include "connectiontcp.h"

#include "indilogger.h"
#include "indistandardproperty.h"

#include <cerrno>
#include <netdb.h>
#include <cstring>
#include <unistd.h>

#ifdef __FreeBSD__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace Connection
{
extern const char *CONNECTION_TAB;

TCP::TCP(INDI::DefaultDevice *dev) : Interface(dev, CONNECTION_TCP)
{
    char defaultHostname[MAXINDINAME] = {0};
    char defaultPort[MAXINDINAME] = {0};

    // Try to load the port from the config file. If that fails, use default port.
    IUGetConfigText(dev->getDeviceName(), INDI::SP::DEVICE_ADDRESS, "ADDRESS", defaultHostname, MAXINDINAME);
    IUGetConfigText(dev->getDeviceName(), INDI::SP::DEVICE_ADDRESS, "PORT", defaultPort, MAXINDINAME);

    // Address/Port
    IUFillText(&AddressT[0], "ADDRESS", "Address", defaultHostname);
    IUFillText(&AddressT[1], "PORT", "Port", defaultPort);
    IUFillTextVector(&AddressTP, AddressT, 2, getDeviceName(), "DEVICE_ADDRESS", "Server", CONNECTION_TAB,
                     IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&TcpUdpS[0], "TCP", "TCP", ISS_ON);
    IUFillSwitch(&TcpUdpS[1], "UDP", "UDP", ISS_OFF);
    IUFillSwitchVector(&TcpUdpSP, TcpUdpS, 2, getDeviceName(), "CONNECTION_TYPE", "Connection Type",
                       CONNECTION_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

}

bool TCP::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (!strcmp(dev, m_Device->getDeviceName()))
    {
        // TCP Server settings
        if (!strcmp(name, AddressTP.name))
        {
            IUUpdateText(&AddressTP, texts, names, n);
            AddressTP.s = IPS_OK;
            IDSetText(&AddressTP, nullptr);
            return true;
        }
    }

    return false;
}

bool TCP::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (!strcmp(dev, m_Device->getDeviceName()))
    {
        if (!strcmp(name, TcpUdpSP.name))
        {
            IUUpdateSwitch(&TcpUdpSP, states, names, n);
            TcpUdpSP.s = IPS_OK;

            IDSetSwitch(&TcpUdpSP, nullptr);

            return true;
        }
    }

    return false;
}

bool TCP::Connect()
{
    if (AddressT[0].text == nullptr || AddressT[0].text[0] == '\0' || AddressT[1].text == nullptr ||
            AddressT[1].text[0] == '\0')
    {
        LOG_ERROR("Error! Server address is missing or invalid.");
        return false;
    }

    const char *hostname = AddressT[0].text;
    const char *port     = AddressT[1].text;

    LOGF_INFO("Connecting to %s@%s ...", hostname, port);

    if (m_Device->isSimulation() == false)
    {
        struct sockaddr_in serv_addr;
        struct hostent *hp = nullptr;
        int ret            = 0;

        struct timeval ts;
        ts.tv_sec  = SOCKET_TIMEOUT;
        ts.tv_usec = 0;

        if (sockfd != -1)
            close(sockfd);

        // Lookup host name or IPv4 address
        hp = gethostbyname(hostname);
        if (!hp)
        {
            LOG_ERROR("Failed to lookup IP Address or hostname.");
            return false;
        }

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family      = AF_INET;
        serv_addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
        serv_addr.sin_port        = htons(atoi(port));

        int socketType = 0;
        if (TcpUdpS[0].s == ISS_ON)
        {
            socketType = SOCK_STREAM;
        }
        else
        {
            socketType = SOCK_DGRAM;
        }

        if ((sockfd = socket(AF_INET, socketType, 0)) < 0)
        {
            LOG_ERROR("Failed to create socket.");
            return false;
        }

        // Connect to the mount
        if ((ret = ::connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
        {
            LOGF_ERROR("Failed to connect to mount %s@%s: %s.", hostname, port, strerror(errno));
            close(sockfd);
            sockfd = -1;
            return false;
        }

        // Set the socket receiving and sending timeouts
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&ts, sizeof(struct timeval));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&ts, sizeof(struct timeval));
    }

    PortFD = sockfd;

    LOG_DEBUG("Connection successful, attempting handshake...");
    bool rc = Handshake();

    if (rc)
    {
        LOGF_INFO("%s is online.", getDeviceName());
        m_Device->saveConfig(true, "DEVICE_ADDRESS");
        m_Device->saveConfig(true, "CONNECTION_TYPE");
    }
    else
        LOG_DEBUG("Handshake failed.");

    return rc;
}

bool TCP::Disconnect()
{
    if (sockfd > 0)
    {
        close(sockfd);
        sockfd = PortFD = -1;
    }

    return true;
}

void TCP::Activated()
{
    m_Device->defineText(&AddressTP);
    m_Device->defineSwitch(&TcpUdpSP);
    m_Device->loadConfig(true, "DEVICE_ADDRESS");
    m_Device->loadConfig(true, "CONNECTION_TYPE");
}

void TCP::Deactivated()
{
    m_Device->deleteProperty(AddressTP.name);
    m_Device->deleteProperty(TcpUdpSP.name);
}

bool TCP::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &AddressTP);
    IUSaveConfigSwitch(fp, &TcpUdpSP);

    return true;
}

void TCP::setDefaultHost(const char *addressHost)
{
    IUSaveText(&AddressT[0], addressHost);
}

void TCP::setDefaultPort(uint32_t addressPort)
{
    char portStr[8];
    snprintf(portStr, 8, "%d", addressPort);
    IUSaveText(&AddressT[1], portStr);
}

void TCP::setConnectionType(int type)
{
    IUResetSwitch(&TcpUdpSP);
    TcpUdpS[type].s = ISS_ON;
    IDSetSwitch(&TcpUdpSP, nullptr);
}
}
