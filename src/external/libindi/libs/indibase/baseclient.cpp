/*******************************************************************************
  Copyright(c) 2011 Jasem Mutlaq. All rights reserved.

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

#include "baseclient.h"

#include "indistandardproperty.h"
#include "base64.h"
#include "basedevice.h"
#include "locale_compat.h"

#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <stdarg.h>
#include <cstring>
#include <algorithm>

#ifdef _WINDOWS
#include <WinSock2.h>
#include <windows.h>

#define net_read(x,y,z) recv(x,y,z,0)
#define net_write(x,y,z) send(x,(const char *)(y),z,0)
#define net_close closesocket

#pragma comment(lib, "Ws2_32.lib")
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define net_read read
#define net_write write
#define net_close close
#endif

#ifdef _MSC_VER
# define snprintf _snprintf
#endif

#define MAXINDIBUF 49152

INDI::BaseClient::BaseClient() : cServer("localhost"), cPort(7624)
{
    sConnected = false;
    verbose    = false;

    timeout_sec = 3;
    timeout_us  = 0;
}

INDI::BaseClient::~BaseClient()
{
    clear();
}

void INDI::BaseClient::clear()
{
    while (!cDevices.empty())
    {
        delete cDevices.back();
        cDevices.pop_back();
    }
    cDevices.clear();
    while (!blobModes.empty())
    {
        delete blobModes.back();
        blobModes.pop_back();
    }
    blobModes.clear();
}

void INDI::BaseClient::setServer(const char *hostname, unsigned int port)
{
    cServer = hostname;
    cPort   = port;
}

void INDI::BaseClient::watchDevice(const char *deviceName)
{
    // Watch for duplicates. Should have used std::set from the beginning but let's
    // avoid changing API now.
    if (std::find(cDeviceNames.begin(), cDeviceNames.end(), deviceName) != cDeviceNames.end())
        return;

    cDeviceNames.emplace_back(deviceName);
}

void INDI::BaseClient::watchProperty(const char *deviceName, const char *propertyName)
{
    watchDevice(deviceName);
    cWatchProperties[deviceName].insert(propertyName);
}

bool INDI::BaseClient::connectServer()
{
#ifdef _WINDOWS
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        IDLog("Error at WSAStartup()\n");
        return false;
    }
#endif

    struct timeval ts;
    ts.tv_sec  = timeout_sec;
    ts.tv_usec = timeout_us;

    struct sockaddr_in serv_addr;
    struct hostent *hp;
    int ret = 0;

    /* lookup host address */
    hp = gethostbyname(cServer.c_str());
    if (!hp)
    {
        perror("gethostbyname");
        return false;
    }

    /* create a socket to the INDI server */
    (void)memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
    serv_addr.sin_port        = htons(cPort);
#ifdef _WINDOWS
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        IDLog("Socket error: %d\n", WSAGetLastError());
        WSACleanup();
        return false;
    }
#else
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return false;
    }
#endif

    /* set the socket in non-blocking */
    //set socket nonblocking flag
#ifdef _WINDOWS
    u_long iMode = 0;
    iResult = ioctlsocket(sockfd, FIONBIO, &iMode);
    if (iResult != NO_ERROR)
    {
        IDLog("ioctlsocket failed with error: %ld\n", iResult);
        return false;
    }
#else
    int flags = 0;
    if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0)
        return false;

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        return false;
#endif

    //clear out descriptor sets for select
    //add socket to the descriptor sets
    fd_set rset, wset;
    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset; //structure assignment okok

    /* connect */
    if ((ret = ::connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        if (errno != EINPROGRESS)
        {
            perror("connect");
            net_close(sockfd);
            return false;
        }
    }

    /* If it is connected, continue, otherwise wait */
    if (ret != 0)
    {
        //we are waiting for connect to complete now
        if ((ret = select(sockfd + 1, &rset, &wset, nullptr, &ts)) < 0)
            return false;
        //we had a timeout
        if (ret == 0)
        {
#ifdef _WINDOWS
            IDLog("select timeout\n");
#else
            errno = ETIMEDOUT;
            perror("select timeout");
#endif
            return false;
        }
    }

    /* we had a positivite return so a descriptor is ready */
#ifndef _WINDOWS
    int error     = 0;
    socklen_t len = sizeof(error);
    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
    {
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
        {
            perror("getsockopt");
            return false;
        }
    }
    else
        return false;

    /* check if we had a socket error */
    if (error)
    {
        errno = error;
        perror("socket");
        return false;
    }
#endif

#ifndef _WINDOWS
    int pipefd[2];
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);

    if (ret < 0)
    {
        IDLog("notify pipe: %s\n", strerror(errno));
        return false;
    }

    m_receiveFd = pipefd[0];
    m_sendFd    = pipefd[1];
#endif

    sConnected = true;

    /*int result = pthread_create(&listen_thread, nullptr, &INDI::BaseClient::listenHelper, this);

    if (result != 0)
    {
        sConnected = false;
        perror("thread");
        return false;
    }*/

    listen_thread = new std::thread(listenHelper, this);

    serverConnected();

    return true;
}

bool INDI::BaseClient::disconnectServer()
{
    //IDLog("Server disconnected called\n");
    if (sConnected == false)
        return true;

    sConnected = false;

#ifdef _WINDOWS
    net_close(sockfd);
    WSACleanup();
#else
    shutdown(sockfd, SHUT_RDWR);
    while (write(m_sendFd, "1", 1) <= 0)
#endif

    clear();

    cDeviceNames.clear();

    listen_thread->join();
    delete(listen_thread);
    listen_thread = nullptr;
    //pthread_join(listen_thread, nullptr);

    int exit_code = 0;
    serverDisconnected(exit_code);

    return true;
}

bool INDI::BaseClient::isServerConnected() const
{
    return sConnected;
}

void INDI::BaseClient::connectDevice(const char *deviceName)
{
    setDriverConnection(true, deviceName);
}

void INDI::BaseClient::disconnectDevice(const char *deviceName)
{
    setDriverConnection(false, deviceName);
}

void INDI::BaseClient::setDriverConnection(bool status, const char *deviceName)
{
    INDI::BaseDevice *drv                 = getDevice(deviceName);
    ISwitchVectorProperty *drv_connection = nullptr;

    if (drv == nullptr)
    {
        IDLog("INDI::BaseClient: Error. Unable to find driver %s\n", deviceName);
        return;
    }

    drv_connection = drv->getSwitch(INDI::SP::CONNECTION);

    if (drv_connection == nullptr)
        return;

    // If we need to connect
    if (status)
    {
        // If there is no need to do anything, i.e. already connected.
        if (drv_connection->sp[0].s == ISS_ON)
            return;

        IUResetSwitch(drv_connection);
        drv_connection->s       = IPS_BUSY;
        drv_connection->sp[0].s = ISS_ON;
        drv_connection->sp[1].s = ISS_OFF;

        sendNewSwitch(drv_connection);
    }
    else
    {
        // If there is no need to do anything, i.e. already disconnected.
        if (drv_connection->sp[1].s == ISS_ON)
            return;

        IUResetSwitch(drv_connection);
        drv_connection->s       = IPS_BUSY;
        drv_connection->sp[0].s = ISS_OFF;
        drv_connection->sp[1].s = ISS_ON;

        sendNewSwitch(drv_connection);
    }
}

INDI::BaseDevice *INDI::BaseClient::getDevice(const char *deviceName)
{
    for (auto &device : cDevices)
    {
        if (!strcmp(deviceName, device->getDeviceName()))
            return device;
    }
    return nullptr;
}

void *INDI::BaseClient::listenHelper(void *context)
{
    (static_cast<INDI::BaseClient *>(context))->listenINDI();
    return nullptr;
}

void INDI::BaseClient::listenINDI()
{
    char buffer[MAXINDIBUF];
    char msg[MAXRBUF];
    int err_code = 0;
#ifdef _WINDOWS
    SOCKET maxfd = 0;
#else
    int maxfd = 0;
#endif
    fd_set rs;
    XMLEle **nodes = nullptr;
    XMLEle *root = nullptr;
    int inode = 0;

    AutoCNumeric locale;

    if (cDeviceNames.empty())
    {
        char cmd[MAXRBUF] = {0};
        snprintf(cmd, MAXRBUF, "<getProperties version='%g'/>\n", INDIV);
        sendString(cmd);
        if (verbose)
            IDLog("%s\n", cmd);
    }
    else
    {
        for (auto oneDevice : cDeviceNames)
        {
            // If there are no specific properties to watch, we watch the complete device
            if (cWatchProperties.find(oneDevice) == cWatchProperties.end())
            {
                char cmd[MAXRBUF] = {0};
                snprintf(cmd, MAXRBUF, "<getProperties version='%g' device='%s'/>\n", INDIV, oneDevice.c_str());
                sendString(cmd);
                if (verbose)
                    IDLog("%s\n", cmd);
            }
            else
            {
                for (auto oneProperty : cWatchProperties[oneDevice])
                {
                    char cmd[MAXRBUF] = {0};
                    snprintf(cmd, MAXRBUF, "<getProperties version='%g' device='%s' name='%s'/>\n",
                             INDIV, oneDevice.c_str(), oneProperty.c_str());
                    sendString(cmd);
                    if (verbose)
                        IDLog("%s\n", cmd);
                }
            }
        }
    }

    locale.Restore();

    FD_ZERO(&rs);

    FD_SET(sockfd, &rs);
    if (sockfd > maxfd)
        maxfd = sockfd;

#ifndef _WINDOWS
    FD_SET(m_receiveFd, &rs);
    if (m_receiveFd > maxfd)
        maxfd = m_receiveFd;
#endif

    clear();
    lillp = newLilXML();

    /* read from server, exit if find all requested properties */
    while (sConnected)
    {
        int n = select(maxfd + 1, &rs, nullptr, nullptr, nullptr);

        if (n < 0)
        {
            IDLog("INDI server %s/%d disconnected.\n", cServer.c_str(), cPort);
            net_close(sockfd);
            break;
        }

#ifndef _WINDOWS
        // Received termination string from main thread
        if (n > 0 && FD_ISSET(m_receiveFd, &rs))
        {
            sConnected = false;
            break;
        }
#endif

        if (n > 0 && FD_ISSET(sockfd, &rs))
        {
#ifdef _WINDOWS
            n = recv(sockfd, buffer, MAXINDIBUF, 0);
#else
            n = recv(sockfd, buffer, MAXINDIBUF, MSG_DONTWAIT);
#endif
            if (n <= 0)
            {
                if (n == 0)
                {
                    IDLog("INDI server %s/%d disconnected.\n", cServer.c_str(), cPort);
                    net_close(sockfd);
                    break;
                }
                else
                    continue;
            }

            nodes = parseXMLChunk(lillp, buffer, n, msg);

            if (!nodes)
            {
                if (msg[0])
                {
                    IDLog("Bad XML from %s/%d: %s\n%s\n", cServer.c_str(), cPort, msg, buffer);
                    return;
                }
                return;
            }
            root = nodes[inode];
            while (root)
            {
                if (verbose)
                    prXMLEle(stderr, root, 0);

                if ((err_code = dispatchCommand(root, msg)) < 0)
                {
                    // Silenty ignore property duplication errors
                    if (err_code != INDI_PROPERTY_DUPLICATED)
                    {
                        IDLog("Dispatch command error(%d): %s\n", err_code, msg);
                        prXMLEle(stderr, root, 0);
                    }
                }

                delXMLEle(root); // not yet, delete and continue
                inode++;
                root = nodes[inode];
            }
            free(nodes);
            inode = 0;
        }
    }

    delLilXML(lillp);

    serverDisconnected((sConnected == false) ? 0 : -1);
    sConnected = false;


    //pthread_exit(0);
}

int INDI::BaseClient::dispatchCommand(XMLEle *root, char *errmsg)
{
    if (!strcmp(tagXMLEle(root), "message"))
        return messageCmd(root, errmsg);
    else if (!strcmp(tagXMLEle(root), "delProperty"))
        return delPropertyCmd(root, errmsg);
    // Just ignore any getProperties we might get
    else if (!strcmp(tagXMLEle(root), "getProperties"))
        return INDI_PROPERTY_DUPLICATED;

    /* Get the device, if not available, create it */
    INDI::BaseDevice *dp = findDev(root, 1, errmsg);
    if (dp == nullptr)
    {
        strcpy(errmsg, "No device available and none was created");
        return INDI_DEVICE_NOT_FOUND;
    }

    // Ignore echoed newXXX
    if (strstr(tagXMLEle(root), "new"))
        return 0;

    // If device is set to BLOB_ONLY, we ignore everything else
    // not related to blobs
    if (getBLOBMode(dp->getDeviceName()) == B_ONLY)
    {
        if (!strcmp(tagXMLEle(root), "defBLOBVector"))
            return dp->buildProp(root, errmsg);
        else if (!strcmp(tagXMLEle(root), "setBLOBVector"))
            return dp->setValue(root, errmsg);

        // Ignore everything else
        return 0;
    }

    // If we are asked to watch for specific properties only, we ignore everything else
    if (cWatchProperties.size() > 0)
    {
        const char *device = findXMLAttValu(root, "device");
        const char *name = findXMLAttValu(root, "name");
        if (device && name)
        {
            if (cWatchProperties.find(device) == cWatchProperties.end() ||
                    cWatchProperties[device].find(name) == cWatchProperties[device].end())
                return 0;
        }
    }

    if ((!strcmp(tagXMLEle(root), "defTextVector")) || (!strcmp(tagXMLEle(root), "defNumberVector")) ||
            (!strcmp(tagXMLEle(root), "defSwitchVector")) || (!strcmp(tagXMLEle(root), "defLightVector")) ||
            (!strcmp(tagXMLEle(root), "defBLOBVector")))
        return dp->buildProp(root, errmsg);
    else if (!strcmp(tagXMLEle(root), "setTextVector") || !strcmp(tagXMLEle(root), "setNumberVector") ||
             !strcmp(tagXMLEle(root), "setSwitchVector") || !strcmp(tagXMLEle(root), "setLightVector") ||
             !strcmp(tagXMLEle(root), "setBLOBVector"))
        return dp->setValue(root, errmsg);

    return INDI_DISPATCH_ERROR;
}

/* delete the property in the given device, including widgets and data structs.
 * when last property is deleted, delete the device too.
 * if no property name attribute at all, delete the whole device regardless.
 * return 0 if ok, else -1 with reason in errmsg[].
 */
int INDI::BaseClient::delPropertyCmd(XMLEle *root, char *errmsg)
{
    XMLAtt *ap;
    INDI::BaseDevice *dp;

    /* dig out device and optional property name */
    dp = findDev(root, 0, errmsg);
    if (!dp)
        return INDI_DEVICE_NOT_FOUND;

    dp->checkMessage(root);

    ap = findXMLAtt(root, "name");

    /* Delete property if it exists, otherwise, delete the whole device */
    if (ap)
    {
        INDI::Property *rProp = dp->getProperty(valuXMLAtt(ap));
        if (rProp == nullptr)
        {
            // Silently ignore B_ONLY clients.
            if (blobModes[0]->blobMode == B_ONLY)
                return 0;

            snprintf(errmsg, MAXRBUF, "Cannot delete property %s as it is not defined yet. Check driver.", valuXMLAtt(ap));
            return -1;
        }
        removeProperty(rProp);
        int errCode = dp->removeProperty(valuXMLAtt(ap), errmsg);

        return errCode;
    }
    // delete the whole device
    else
        return deleteDevice(dp->getDeviceName(), errmsg);
}

int INDI::BaseClient::deleteDevice(const char *devName, char *errmsg)
{
    std::vector<INDI::BaseDevice *>::iterator devicei;

    for (devicei = cDevices.begin(); devicei != cDevices.end();)
    {
        if (!strcmp(devName, (*devicei)->getDeviceName()))
        {
            removeDevice(*devicei);
            delete *devicei;
            devicei = cDevices.erase(devicei);
            return 0;
        }
        else
            ++devicei;
    }

    snprintf(errmsg, MAXRBUF, "Device %s not found", devName);
    return INDI_DEVICE_NOT_FOUND;
}

INDI::BaseDevice *INDI::BaseClient::findDev(const char *devName, char *errmsg)
{
    std::vector<INDI::BaseDevice *>::const_iterator devicei;

    for (devicei = cDevices.begin(); devicei != cDevices.end(); ++devicei)
    {
        if (!strcmp(devName, (*devicei)->getDeviceName()))
            return (*devicei);
    }

    snprintf(errmsg, MAXRBUF, "Device %s not found", devName);
    return nullptr;
}

/* add new device */
INDI::BaseDevice *INDI::BaseClient::addDevice(XMLEle *dep, char *errmsg)
{
    //devicePtr dp(new INDI::BaseDriver());
    INDI::BaseDevice *dp = new INDI::BaseDevice();
    char *device_name;

    /* allocate new INDI::BaseDriver */
    XMLAtt *ap = findXMLAtt(dep, "device");
    if (!ap)
    {
        strncpy(errmsg, "Unable to find device attribute in XML element. Cannot add device.", MAXRBUF);
        delete (dp);
        return nullptr;
    }

    device_name = valuXMLAtt(ap);

    dp->setMediator(this);
    dp->setDeviceName(device_name);

    cDevices.push_back(dp);

    newDevice(dp);

    /* ok */
    return dp;
}

INDI::BaseDevice *INDI::BaseClient::findDev(XMLEle *root, int create, char *errmsg)
{
    XMLAtt *ap;
    INDI::BaseDevice *dp;
    char *dn;

    /* get device name */
    ap = findXMLAtt(root, "device");
    if (!ap)
    {
        snprintf(errmsg, MAXRBUF, "No device attribute found in element %s", tagXMLEle(root));
        return (nullptr);
    }

    dn = valuXMLAtt(ap);

    if (*dn == '\0')
    {
        snprintf(errmsg, MAXRBUF, "Device name is empty! %s", tagXMLEle(root));
        return (nullptr);
    }

    dp = findDev(dn, errmsg);

    if (dp)
        return dp;

    /* not found, create if ok */
    if (create)
        return (addDevice(root, errmsg));

    snprintf(errmsg, MAXRBUF, "INDI: <%s> no such device %s", tagXMLEle(root), dn);
    return nullptr;
}

/* a general message command received from the device.
 * return 0 if ok, else -1 with reason in errmsg[].
 */
int INDI::BaseClient::messageCmd(XMLEle *root, char *errmsg)
{
    INDI::BaseDevice *dp = findDev(root, 0, errmsg);

    if (dp)
        dp->checkMessage(root);
    else
    {
        XMLAtt *message;
        XMLAtt *time_stamp;

        char msgBuffer[MAXRBUF];

        /* prefix our timestamp if not with msg */
        time_stamp = findXMLAtt(root, "timestamp");

        /* finally! the msg */
        message = findXMLAtt(root, "message");
        if (!message)
        {
            strncpy(errmsg, "No message content found.", MAXRBUF);
            return -1;
        }

        if (time_stamp)
            snprintf(msgBuffer, MAXRBUF, "%s: %s", valuXMLAtt(time_stamp), valuXMLAtt(message));
        else
        {
            char ts[32];
            struct tm *tp;
            time_t t;
            time(&t);
            tp = gmtime(&t);
            strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tp);
            snprintf(msgBuffer, MAXRBUF, "%s: %s", ts, valuXMLAtt(message));
        }

        std::string finalMsg = msgBuffer;

        newUniversalMessage(finalMsg);
    }

    return (0);
}

void INDI::BaseClient::newUniversalMessage(std::string message)
{
    IDLog("%s\n", message.c_str());
}

void INDI::BaseClient::sendNewText(ITextVectorProperty *tvp)
{
    tvp->s = IPS_BUSY;

    sendString("<newTextVector\n");
    sendString("  device='%s'\n", tvp->device);
    sendString("  name='%s'\n>", tvp->name);

    for (int i = 0; i < tvp->ntp; i++)
    {
        sendString("  <oneText\n");
        sendString("    name='%s'>\n", tvp->tp[i].name);
        sendString("      %s\n", tvp->tp[i].text);
        sendString("  </oneText>\n");
    }
    sendString("</newTextVector>\n");

}

void INDI::BaseClient::sendNewText(const char *deviceName, const char *propertyName, const char *elementName,
                                   const char *text)
{
    INDI::BaseDevice *drv = getDevice(deviceName);

    if (drv == nullptr)
        return;

    ITextVectorProperty *tvp = drv->getText(propertyName);

    if (tvp == nullptr)
        return;

    IText *tp = IUFindText(tvp, elementName);

    if (tp == nullptr)
        return;

    IUSaveText(tp, text);

    sendNewText(tvp);
}

void INDI::BaseClient::sendNewNumber(INumberVectorProperty *nvp)
{
    AutoCNumeric locale;

    nvp->s = IPS_BUSY;

    sendString("<newNumberVector\n");
    sendString("  device='%s'\n", nvp->device);
    sendString("  name='%s'\n>", nvp->name);

    for (int i = 0; i < nvp->nnp; i++)
    {
        sendString("  <oneNumber\n");
        sendString("    name='%s'>\n", nvp->np[i].name);
        sendString("      %g\n", nvp->np[i].value);
        sendString("  </oneNumber>\n");
    }
    sendString("</newNumberVector>\n");
}

void INDI::BaseClient::sendNewNumber(const char *deviceName, const char *propertyName, const char *elementName,
                                     double value)
{
    INDI::BaseDevice *drv = getDevice(deviceName);

    if (drv == nullptr)
        return;

    INumberVectorProperty *nvp = drv->getNumber(propertyName);

    if (nvp == nullptr)
        return;

    INumber *np = IUFindNumber(nvp, elementName);

    if (np == nullptr)
        return;

    np->value = value;

    sendNewNumber(nvp);
}

void INDI::BaseClient::sendNewSwitch(ISwitchVectorProperty *svp)
{
    svp->s            = IPS_BUSY;
    ISwitch *onSwitch = IUFindOnSwitch(svp);

    sendString("<newSwitchVector\n");

    sendString("  device='%s'\n", svp->device);
    sendString("  name='%s'>\n", svp->name);

    if (svp->r == ISR_1OFMANY && onSwitch)
    {
        sendString("  <oneSwitch\n");
        sendString("    name='%s'>\n", onSwitch->name);
        sendString("      %s\n", (onSwitch->s == ISS_ON) ? "On" : "Off");
        sendString("  </oneSwitch>\n");
    }
    else
    {
        for (int i = 0; i < svp->nsp; i++)
        {
            sendString("  <oneSwitch\n");
            sendString("    name='%s'>\n", svp->sp[i].name);
            sendString("      %s\n", (svp->sp[i].s == ISS_ON) ? "On" : "Off");
            sendString("  </oneSwitch>\n");
        }
    }

    sendString("</newSwitchVector>\n");
}

void INDI::BaseClient::sendNewSwitch(const char *deviceName, const char *propertyName, const char *elementName)
{
    INDI::BaseDevice *drv = getDevice(deviceName);

    if (drv == nullptr)
        return;

    ISwitchVectorProperty *svp = drv->getSwitch(propertyName);

    if (svp == nullptr)
        return;

    ISwitch *sp = IUFindSwitch(svp, elementName);

    if (sp == nullptr)
        return;

    sp->s = ISS_ON;

    sendNewSwitch(svp);
}

void INDI::BaseClient::startBlob(const char *devName, const char *propName, const char *timestamp)
{
    sendString("<newBLOBVector\n");
    sendString("  device='%s'\n", devName);
    sendString("  name='%s'\n", propName);
    sendString("  timestamp='%s'>\n", timestamp);
}

void INDI::BaseClient::sendOneBlob(IBLOB *bp)
{
    char nl = '\n';
    int rc = 0;
    uint8_t *encblob = static_cast<uint8_t*>(malloc(4 * bp->size / 3 + 4));
    uint32_t base64Len = to64frombits(encblob, reinterpret_cast<const uint8_t *>(bp->blob), bp->size);

    sendString("  <oneBLOB\n");
    sendString("    name='%s'\n", bp->name);
    sendString("    size='%ud'\n", bp->size);
    sendString("    enclen='%d'\n", base64Len);
    sendString("    format='%s'>\n", bp->format);

    uint32_t written = 0;
    while (written < base64Len)
    {
        // Write 72 chars followed by new line
        uint8_t towrite = ((base64Len - written) > 72) ? 72 : base64Len - written;
        ssize_t wr = net_write(sockfd, encblob + written, towrite);
        if (wr > 0)
            written += wr;
        else if (wr < 0)
        {
            // If temporary error, continue
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
            {
                // Otherwise exist
                fprintf(stderr, "sendOneBlob: %s\n", strerror(errno));
                free(encblob);
                return;
            }
        }
        if ((written % 72) == 0)
            rc = net_write(sockfd, &nl, 1);
    }

    if ((written % 72) != 0)
        rc = net_write(sockfd, &nl, 1);

    free(encblob);

    sendString("   </oneBLOB>\n");
}

void INDI::BaseClient::sendOneBlob(const char *blobName, unsigned int blobSize, const char *blobFormat,
                                   void *blobBuffer)
{
    char nl = '\n';
    int rc = 0;
    uint8_t *encblob = static_cast<uint8_t*>(malloc(4 * blobSize / 3 + 4));
    uint32_t base64Len = to64frombits(encblob, reinterpret_cast<const uint8_t *>(blobBuffer), blobSize);

    sendString("  <oneBLOB\n");
    sendString("    name='%s'\n", blobName);
    sendString("    size='%ud'\n", blobSize);
    sendString("    enclen='%d'\n", base64Len);
    sendString("    format='%s'>\n", blobFormat);

    uint32_t written = 0;
    while (written < base64Len)
    {
        // Write 72 chars followed by new line
        uint8_t towrite = ((base64Len - written) > 72) ? 72 : base64Len - written;
        ssize_t wr = net_write(sockfd, encblob + written, towrite);
        if (wr > 0)
            written += wr;
        else if (wr < 0)
        {
            // If temporary error, continue
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
            {
                // Otherwise exist
                fprintf(stderr, "sendOneBlob: %s\n", strerror(errno));
                free(encblob);
                return;
            }
        }
        if ((written % 72) == 0)
            rc = net_write(sockfd, &nl, 1);
    }

    if ((written % 72) != 0)
        rc = net_write(sockfd, &nl, 1);

    free(encblob);

    sendString("   </oneBLOB>\n");
}

void INDI::BaseClient::finishBlob()
{
    sendString("</newBLOBVector>\n");
}

void INDI::BaseClient::setBLOBMode(BLOBHandling blobH, const char *dev, const char *prop)
{
    char blobOpenTag[MAXRBUF];

    if (!dev[0])
        return;

    BLOBMode *bMode = findBLOBMode(std::string(dev), (prop ? std::string(prop) : std::string()));

    if (bMode == nullptr)
    {
        BLOBMode *newMode = new BLOBMode();
        newMode->device   = std::string(dev);
        newMode->property = (prop ? std::string(prop) : std::string());
        newMode->blobMode = blobH;
        blobModes.push_back(newMode);
    }
    else
    {
        // If nothing changed, nothing to to do
        if (bMode->blobMode == blobH)
            return;

        bMode->blobMode = blobH;
    }

    if (prop != nullptr)
        snprintf(blobOpenTag, MAXRBUF, "<enableBLOB device='%s' name='%s'>", dev, prop);
    else
        snprintf(blobOpenTag, MAXRBUF, "<enableBLOB device='%s'>", dev);

    switch (blobH)
    {
        case B_NEVER:
            sendString("%sNever</enableBLOB>\n", blobOpenTag);
            break;
        case B_ALSO:
            sendString("%sAlso</enableBLOB>\n", blobOpenTag);
            break;
        case B_ONLY:
            sendString("%sOnly</enableBLOB>\n", blobOpenTag);
            break;
    }
}

BLOBHandling INDI::BaseClient::getBLOBMode(const char *dev, const char *prop)
{
    BLOBHandling bHandle = B_ALSO;

    BLOBMode *bMode = findBLOBMode(dev, (prop ? std::string(prop) : std::string()));

    if (bMode)
        bHandle = bMode->blobMode;

    return bHandle;
}

INDI::BaseClient::BLOBMode *INDI::BaseClient::findBLOBMode(const std::string &device, const std::string &property)
{
    for (auto &blob : blobModes)
    {
        if (blob->device == device && (property.empty() || blob->property == property))
            return blob;
    }

    return nullptr;
}

bool INDI::BaseClient::getDevices(std::vector<INDI::BaseDevice *> &deviceList, uint16_t driverInterface )
{
    for (INDI::BaseDevice *device : cDevices)
    {
        if (device->getDriverInterface() & driverInterface)
            deviceList.push_back(device);
    }

    return (deviceList.size() > 0);
}

void INDI::BaseClient::sendString(const char *fmt, ...)
{
    int ret = 0;
    char message[MAXRBUF];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(message, MAXRBUF, fmt, ap);
    va_end(ap);
    ret = net_write(sockfd, message, strlen(message));
}
