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

#pragma once

#include "indiapi.h"
#include "indibase.h"

#include <string>
#include <vector>

#include <thread>

#ifdef _WINDOWS
#include <WinSock2.h>
#endif

#define MAXRBUF 2048

/**
 * \class INDI::BaseClient
   \brief Class to provide basic client functionality.

   BaseClient enables accelerated development of INDI Clients by providing a framework that facilitates communication, device
   handling, and event notification. By subclassing BaseClient, clients can quickly connect to an INDI server, and query for
   a set of INDI::BaseDevice devices, and read and write properties seamlessly. Event driven programming is possible due to
   notifications upon reception of new devices or properties.

   Upon connecting to an INDI server, it creates a dedicated thread to handle all incoming traffic. The thread is terminated
   when disconnectServer() is called or when a communication error occurs.

   \attention All notifications functions defined in INDI::BaseMediator <b>must</b> be implemented in the client class even if
   they are not used because these are pure virtual functions.

   \see <a href="http://indilib.org/develop/tutorials/107-client-development-tutorial.html">INDI Client Tutorial</a> for more details.
   \author Jasem Mutlaq

 */
class INDI::BaseClient : public INDI::BaseMediator
{
  public:
    BaseClient();
    virtual ~BaseClient();

    /** \brief Set the server host name and port
        \param hostname INDI server host name or IP address.
        \param port INDI server port.
    */
    void setServer(const char *hostname, unsigned int port);

    /** \brief Add a device to the watch list.

        A client may select to receive notifications of only a specific device or a set of devices.
        If the client encounters any of the devices set via this function, it will create a corresponding
        INDI::BaseDevice object to handle them. If no devices are watched, then all devices owned by INDI server
        will be created and handled.
    */
    void watchDevice(const char *deviceName);

    /** \brief Connect to INDI server.

        \returns True if the connection is successful, false otherwise.
        \note This function blocks until connection is either successull or unsuccessful.
    */
    bool connectServer();

    /** \brief Disconnect from INDI server.

        Disconnects from INDI servers. Any devices previously created will be deleted and memory cleared.
        \return True if disconnection is successful, false otherwise.
    */
    bool disconnectServer();

    bool isServerConnected() const;

    /** \brief Connect to INDI driver
        \param deviceName Name of the device to connect to.
    */
    void connectDevice(const char *deviceName);

    /** \brief Disconnect INDI driver
        \param deviceName Name of the device to disconnect.
    */
    void disconnectDevice(const char *deviceName);

    /** \param deviceName Name of device to search for in the list of devices owned by INDI server,
         \returns If \e deviceName exists, it returns an instance of the device. Otherwise, it returns NULL.
    */
    INDI::BaseDevice *getDevice(const char *deviceName);

    /** \returns Returns a vector of all devices created in the client.
    */
    const std::vector<INDI::BaseDevice *> &getDevices() const { return cDevices; }

    /**
     * @brief getDevices Returns list of devices that belong to a particular @ref INDI::BaseDevice::DRIVER_INTERFACE "DRIVER_INTERFACE" class.
     *
     * For example, to get a list of guide cameras:
     @code{.cpp}
      std::vector<INDI::BaseDevice *> guideCameras;
      getDevices(guideCameras, CCD_INTERFACE | GUIDE_INTERFACE);
      for (INDI::BaseDevice *device : guideCameras)
             cout << "Guide Camera Name: " << device->getDeviceName();
     @endcode
     * @param deviceList Supply device list to be filled by the function.
     * @param driverInterface ORed DRIVER_INTERFACE values to select the desired class of devices.
     * @return True if one or more devices are found for the supplied driverInterface, false if no matching devices found.

     */
    bool getDevices(std::vector<INDI::BaseDevice *> &deviceList, uint16_t driverInterface);

    /** \brief Set Binary Large Object policy mode

      Set the BLOB handling mode for the client. The client may either receive:
      <ul>
      <li>Only BLOBS</li>
      <li>BLOBs mixed with normal messages</li>
      <li>Normal messages only, no BLOBs</li>
      </ul>

      If \e dev and \e prop are supplied, then the BLOB handling policy is set for this particular device and property.
      if \e prop is NULL, then the BLOB policy applies to the whole device.

      \param blobH BLOB handling policy
      \param dev name of device, required.
      \param prop name of property, optional.
    */
    void setBLOBMode(BLOBHandling blobH, const char *dev, const char *prop = NULL);

    /**
     * @brief getBLOBMode Get Binary Large Object policy mode IF set previously by setBLOBMode
     * @param dev name of device.
     * @param prop property name, can be NULL to return overall device policy if it exists.
     * @return BLOB Policy, if not found, it always returns B_ALSO
     */
    BLOBHandling getBLOBMode(const char *dev, const char *prop = NULL);

    // Update
    static void *listenHelper(void *context);

    const char *getHost() { return cServer.c_str(); }
    int getPort() { return cPort; }

    /** \brief Send new Text command to server */
    void sendNewText(ITextVectorProperty *pp);
    /** \brief Send new Text command to server */
    void sendNewText(const char *deviceName, const char *propertyName, const char *elementName, const char *text);
    /** \brief Send new Number command to server */
    void sendNewNumber(INumberVectorProperty *pp);
    /** \brief Send new Number command to server */
    void sendNewNumber(const char *deviceName, const char *propertyName, const char *elementName, double value);
    /** \brief Send new Switch command to server */
    void sendNewSwitch(ISwitchVectorProperty *pp);
    /** \brief Send new Switch command to server */
    void sendNewSwitch(const char *deviceName, const char *propertyName, const char *elementName);

    /** \brief Send opening tag for BLOB command to server */
    void startBlob(const char *devName, const char *propName, const char *timestamp);
    /** \brief Send ONE blob content to server. The BLOB data in raw binary format and will be converted to base64 and sent to server */
    void sendOneBlob(IBLOB *bp);
    /** \brief Send ONE blob content to server. The BLOB data in raw binary format and will be converted to base64 and sent to server */
    void sendOneBlob(const char *blobName, unsigned int blobSize, const char *blobFormat, void *blobBuffer);
    /** \brief Send closing tag for BLOB command to server */
    void finishBlob();

    /**
     * @brief setVerbose Set verbose mode
     * @param enable If true, enable <b>FULL</b> verbose output. Any XML message received, including BLOBs, are printed on
     * standard output. Only use this for debugging purposes.
     */
    void setVerbose(bool enable) { verbose = enable; }

    /**
     * @brief isVerbose Is client in verbose mode?
     * @return Is client in verbose mode?
     */
    bool isVerbose() const { return verbose; }

    /**
     * @brief setConnectionTimeout Set connection timeout. By default it is 3 seconds.
     * @param seconds seconds
     * @param microseconds microseconds
     */
    void setConnectionTimeout(uint32_t seconds, uint32_t microseconds)
    {
        timeout_sec = seconds;
        timeout_us  = microseconds;
    }

  protected:
    /** \brief Dispatch command received from INDI server to respective devices handled by the client */
    int dispatchCommand(XMLEle *root, char *errmsg);

    /** \brief Remove device */
    int deleteDevice(const char *devName, char *errmsg);

    /** \brief Delete property command */
    int delPropertyCmd(XMLEle *root, char *errmsg);

    /** \brief Find and return a particular device */
    INDI::BaseDevice *findDev(const char *devName, char *errmsg);
    /** \brief Add a new device */
    INDI::BaseDevice *addDevice(XMLEle *dep, char *errmsg);
    /** \brief Find a device, and if it doesn't exist, create it if create is set to 1 */
    INDI::BaseDevice *findDev(XMLEle *root, int create, char *errmsg);

    /**  Process messages */
    int messageCmd(XMLEle *root, char *errmsg);

    /**
     * @brief newUniversalMessage Universal messages are sent from INDI server without a specific device. It is addressed to the client overall.
     * @param message content of message.
     * @note The default implementation simply logs the message to stderr. Override to handle the message.
     */
    virtual void newUniversalMessage(std::string message);

  private:
    typedef struct
    {
        std::string device;
        std::string property;
        BLOBHandling blobMode;
    } BLOBMode;

    BLOBMode *findBLOBMode(const std::string& device, const std::string& property);

    /** \brief Connect/Disconnect to INDI driver
        \param status If true, the client will attempt to turn on CONNECTION property within the driver (i.e. turn on the device).
         Otherwise, CONNECTION will be turned off.
        \param deviceName Name of the device to connect to.
    */
    void setDriverConnection(bool status, const char *deviceName);

    /**
     * @brief clear Clear devices and blob modes
     */
    void clear();

    std::thread *listen_thread=nullptr;

#ifdef _WINDOWS
    SOCKET sockfd;
#else
    int sockfd;
    int m_receiveFd;
    int m_sendFd;
#endif

    // Listen to INDI server and process incoming messages
    void listenINDI();

    void sendString(const char *fmt, ...);

    std::vector<INDI::BaseDevice *> cDevices;
    std::vector<std::string> cDeviceNames;
    std::vector<BLOBMode *> blobModes;

    std::string cServer;
    unsigned int cPort;
    bool sConnected;
    bool verbose;

    // Parse & FILE buffers for IO

    LilXML *lillp; /* XML parser context */
    uint32_t timeout_sec, timeout_us;
};
