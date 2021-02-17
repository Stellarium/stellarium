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

#include "basedevice.h"
#include "indidriver.h"
#include "indilogger.h"

#include <stdint.h>

namespace Connection
{
class Interface;
class Serial;
class TCP;
}
/**
 * @brief COMMUNICATION_TAB Where all the properties required to connect/disconnect from
 * a device are located. Usually such properties may include port number, IP address, or
 * any property necessarily to establish a connection to the device.
 */
extern const char *COMMUNICATION_TAB;

/**
 * @brief MAIN_CONTROL_TAB Where all the primary controls for the device are located.
 */
extern const char *MAIN_CONTROL_TAB;

/**
 * @brief CONNECTION_TAB Where all device connection settings (serial, usb, ethernet) are defined and controlled.
 */
extern const char *CONNECTION_TAB;

/**
 * @brief MOTION_TAB Where all the motion control properties of the device are located.
 */
extern const char *MOTION_TAB;

/**
 * @brief DATETIME_TAB Where all date and time setting properties are located.
 */
extern const char *DATETIME_TAB;

/**
 * @brief SITE_TAB Where all site information setting are located.
 */
extern const char *SITE_TAB;

/**
 * @brief OPTIONS_TAB Where all the driver's options are located. Those may include auxiliary controls, driver
 * metadata, version information..etc.
 */
extern const char *OPTIONS_TAB;

/**
 * @brief FILTER_TAB Where all the properties for filter wheels are located.
 */
extern const char *FILTER_TAB;

/**
 * @brief FOCUS_TAB Where all the properties for focuser are located.
 */
extern const char *FOCUS_TAB;

/**
 * @brief GUIDE_TAB Where all the properties for guiding are located.
 */
extern const char *GUIDE_TAB;

/**
 * @brief ALIGNMENT_TAB Where all the properties for guiding are located.
 */
extern const char *ALIGNMENT_TAB;

/**
 * @brief INFO_TAB Where all the properties for general information are located.
 */
extern const char *INFO_TAB;

/**
 * \class INDI::DefaultDevice
 * \brief Class to provide extended functionality for devices in addition
 * to the functionality provided by INDI::BaseDevice. This class should \e only be subclassed by
 * drivers directly as it is linked with main(). Virtual drivers cannot employ INDI::DefaultDevice.
 *
 * INDI::DefaultDevice provides capability to add Debug, Simulation, and Configuration controls.
 * These controls (switches) are defined to the client. Configuration options permit saving and
 * loading of AS-IS property values.
 *
 * \see <a href='tutorial__four_8h_source.html'>Tutorial Four</a>
 * \author Jasem Mutlaq
 */
class INDI::DefaultDevice : public INDI::BaseDevice
{
    public:
        DefaultDevice();
        virtual ~DefaultDevice() override;

        /** \brief Add Debug, Simulation, and Configuration options to the driver */
        void addAuxControls();

        /** \brief Add Debug control to the driver */
        void addDebugControl();

        /** \brief Add Simulation control to the driver */
        void addSimulationControl();

        /** \brief Add Configuration control to the driver */
        void addConfigurationControl();

        /** \brief Add Polling period control to the driver */
        void addPollPeriodControl();

        /** \brief Set all properties to IDLE state */
        void resetProperties();

        /**
         * \brief Define number vector to client & register it. Alternatively, IDDefNumber can
         * be used but the property will not get registered and the driver will not be able to
         * save configuration files.
         * \param nvp The number vector property to be defined
         */
        void defineNumber(INumberVectorProperty *nvp);

        /**
         * \brief Define text vector to client & register it. Alternatively, IDDefText can be
         * used but the property will not get registered and the driver will not be able to save
         * configuration files.
         * \param tvp The text vector property to be defined
         */
        void defineText(ITextVectorProperty *tvp);

        /**
         * \brief Define switch vector to client & register it. Alternatively, IDDefswitch can be
         * used but the property will not get registered and the driver will not be able to save
         * configuration files.
         * \param svp The switch vector property to be defined
         */
        void defineSwitch(ISwitchVectorProperty *svp);

        /**
         * \brief Define light vector to client & register it. Alternatively, IDDeflight can be
         * used but the property will not get registered and the driver will not be able to save
         * configuration files.
         * \param lvp The light vector property to be defined
         */
        void defineLight(ILightVectorProperty *lvp);

        /**
         * \brief Define BLOB vector to client & register it. Alternatively, IDDefBLOB can be
         * used but the property will not get registered and the driver will not be able to
         * save configuration files.
         * \param bvp The BLOB vector property to be defined
         */
        void defineBLOB(IBLOBVectorProperty *bvp);

        /**
         * \brief Delete a property and unregister it. It will also be deleted from all clients.
         * \param propertyName name of property to be deleted.
         */
        virtual bool deleteProperty(const char *propertyName);

        /**
         * \brief Set connection switch status in the client.
         * \param status If true, the driver will attempt to connect to the device (CONNECT=ON).
         * If false, it will attempt to disconnect the device.
         * \param status True to set CONNECT on, false to set DISCONNECT on.
         * \param state State of CONNECTION properti, by default IPS_OK.
         * \param msg A message to be sent along with connect/disconnect command, by default nullptr.
         */
        virtual void setConnected(bool status, IPState state = IPS_OK, const char *msg = nullptr);

        /**
         * \brief Set a timer to call the function TimerHit after ms milliseconds
         * \param ms timer duration in milliseconds.
         * \return id of the timer to be used with RemoveTimer
        */
        int SetTimer(uint32_t ms);

        /**
         * \brief Remove timer added with SetTimer
         * \param id ID of the timer as returned from SetTimer
         */
        void RemoveTimer(int id);

        /** \brief Callback function to be called once SetTimer duration elapses. */
        virtual void TimerHit();

        /** \return driver executable filename */
        virtual const char *getDriverExec()
        {
            return me;
        }

        /** \return driver name */
        virtual const char *getDriverName()
        {
            return getDefaultName();
        }

        /**
         * \brief Set driver version information to be defined in DRIVER_INFO property as vMajor.vMinor
         * \param vMajor major revision number
         * \param vMinor minor revision number
         */
        void setVersion(uint16_t vMajor, uint16_t vMinor)
        {
            majorVersion = vMajor;
            minorVersion = vMinor;
        }

        /** \return Major driver version number. */
        uint16_t getMajorVersion()
        {
            return majorVersion;
        }

        /** \return Minor driver version number. */
        uint16_t getMinorVersion()
        {
            return minorVersion;
        }

        /**
         * \brief define the driver's properties to the client.
         * Usually, only a minimum set of properties are defined to the client in this function
         * if the device is in disconnected state. Those properties should be enough to enable the
         * client to establish a connection to the device. In addition to CONNECT/DISCONNECT, such
         * properties may include port name, IP address, etc. You should check if the device is
         * already connected, and if this is true, then you must define the remainder of the
         * the properties to the client in this function. Otherwise, the remainder of the driver's
         * properties are defined to the client in updateProperties() function which is called when
         * a client connects/disconnects from a device.
         * \param dev name of the device
         * \note This function is called by the INDI framework, do not call it directly. See LX200
         * Generic driver for an example implementation
         */
        virtual void ISGetProperties(const char *dev);

        /**
         * \brief Process the client newSwitch command
         * \note This function is called by the INDI framework, do not call it directly.
         * \returns True if any property was successfully processed, false otherwise.
         */
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

        /**
         * \brief Process the client newNumber command
         * \note This function is called by the INDI framework, do not call it directly.
         * \returns True if any property was successfully processed, false otherwise.
         */
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

        /**
         * \brief Process the client newSwitch command
         * \note This function is called by the INDI framework, do not call it directly.
         * \returns True if any property was successfully processed, false otherwise.
         */
        virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

        /**
         * \brief Process the client newBLOB command
         * \note This function is called by the INDI framework, do not call it directly.
         * \returns True if any property was successfully processed, false otherwise.
         */
        virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                               char *formats[], char *names[], int n);

        /**
         * \brief Process a snoop event from INDI server. This function is called when a snooped property is
         * updated in a snooped driver.
         * \note This function is called by the INDI framework, do not call it directly.
         * \returns True if any property was successfully processed, false otherwise.
         */
        virtual bool ISSnoopDevice(XMLEle *root);

        /**
         * @return getInterface Return the interface declared by the driver.
         */
        virtual uint16_t getDriverInterface() override;

        /**
         * @brief setInterface Set driver interface. By default the driver interface is set to GENERAL_DEVICE.
         * You may send an ORed list of DeviceInterface values.
         * @param value ORed list of DeviceInterface values.
         */
        void setDriverInterface(uint16_t value);

    protected:
        /**
         * @brief setDynamicPropertiesBehavior controls handling of dynamic properties. Dyanmic properties
         * are those generated from an external skeleton XML file. By default all properties, including
         * dynamic properties, are defined to the client in ISGetProperties(). Furthermore, when
         * űdeleteProperty(properyName) is called, the dynamic property is deleted by default, and can only
         * be restored by calling buildSkeleton(filename) again. However, it is sometimes desirable to skip
         * the definition of the dynamic properties on startup and delegate this task to the child class.
         * To control this behavior, set enabled to false.
         * @param defineEnabled True to define all dynamic properties in INDI::DefaultDevice own
         * ISGetProperties() on startup. False to skip defining dynamic properties.
         * @param deleteEnabled True to delete dynamic properties from memory in deleteProperty(name).
         * False to keep dynamic property in the properties list, but delete it from the client.
         * @note This function has no effect on regular properties initialized directly by the driver.
         */
        void setDynamicPropertiesBehavior(bool defineEnabled, bool deleteEnabled)
        {
            defineDynamicProperties = defineEnabled;
            deleteDynamicProperties = deleteEnabled;
        }

        // Configuration

        /**
         * \brief Load the last saved configuration file
         * \param silent if true, don't report any error or notification messages.
         * \param property Name of property to load configuration for. If nullptr, all properties in the
         * configuration file are loaded which is the default behavior.
         * \return True if successful, false otherwise.
         */
        virtual bool loadConfig(bool silent = false, const char *property = nullptr);

        /**
         * \brief Save the current properties in a configuration file
         * \param silent if true, don't report any error or notification messages.
         * \param property Name of specific property to save while leaving all others properties in the
         * file as is.
         * \return True if successful, false otherwise.
         */
        virtual bool saveConfig(bool silent = false, const char *property = nullptr);

        /**
         * @brief purgeConfig Remove config file from disk.
         * @return True if successful, false otherwise.
         */
        virtual bool purgeConfig();

        /**
         * @brief saveConfigItems Save specific properties in the provide config file handler. Child
         * class usually override this function to save their own properties and the base class
         * saveConfigItems(fp) must be explicitly called by each child class. The Default Device
         * saveConfigItems(fp) only save Debug properties options in the config file.
         * @param fp Pointer to config file handler
         * @return True if successful, false otherwise.
         */
        virtual bool saveConfigItems(FILE *fp);

        /**
         * @brief saveAllConfigItems Save all the drivers' properties in the configuration file
         * @param fp pointer to config file handler
         * @return  True if successful, false otherwise.
         */
        virtual bool saveAllConfigItems(FILE *fp);

        /**
         * \brief Load the default configuration file
         * \return True if successful, false otherwise.
         */
        virtual bool loadDefaultConfig();

        // Simulatin & Debug

        /**
         * \brief Toggle driver debug status
         * A driver can be more verbose if Debug option is enabled by the client.
         * \param enable If true, the Debug option is set to ON.
         */
        void setDebug(bool enable);

        /**
         * \brief Toggle driver simulation status
         * A driver can run in simulation mode if Simulation option is enabled by the client.
         * \param enable If true, the Simulation option is set to ON.
         */
        void setSimulation(bool enable);

        /**
         * \brief Inform driver that the debug option was triggered.
         * This function is called after setDebug is triggered by the client. Reimplement this
         * function if your driver needs to take specific action after debug is enabled/disabled.
         * Otherwise, you can use isDebug() to check if simulation is enabled or disabled.
         * \param enable If true, the debug option is set to ON.
         */
        virtual void debugTriggered(bool enable);

        /**
         * \brief Inform driver that the simulation option was triggered.
         * This function is called after setSimulation is triggered by the client. Reimplement this
         * function if your driver needs to take specific action after simulation is enabled/disabled.
         * Otherwise, you can use isSimulation() to check if simulation is enabled or disabled.
         * \param enable If true, the simulation option is set to ON.
         */
        virtual void simulationTriggered(bool enable);

        /** \return True if Debug is on, False otherwise. */
        bool isDebug();

        /** \return True if Simulation is on, False otherwise. */
        bool isSimulation();

        /**
         * \brief Initilize properties initial state and value. The child class must implement this function.
         * \return True if initilization is successful, false otherwise.
         */
        virtual bool initProperties();

        /**
         * \brief updateProperties is called whenever there is a change in the CONNECTION status of
         * the driver. This will enable the driver to react to changes of switching ON/OFF a device.
         * For example, a driver may only define a set of properties after a device is connected, but
         * not before.
         * \return True if update is successful, false otherwise.
         */
        virtual bool updateProperties();

        /**
         * \brief Connect to the device. INDI::DefaultDevice implementation connects to appropriate
         * connection interface (Serial or TCP) governed by connectionMode. If connection is successful,
         * it proceed to call Handshake() function to ensure communication with device is successful.
         * For other communication interface, override the method in the child class implementation
         * \return True if connection is successful, false otherwise
         */
        virtual bool Connect();

        /**
         * \brief Disconnect from device
         * \return True if successful, false otherwise
         */
        virtual bool Disconnect();

        /**
         * @brief registerConnection Add new connection plugin to the existing connection pool. The
         * connection type shall be defined to the client in ISGetProperties()
         * @param newConnection Pointer to new connection plugin
         */
        void registerConnection(Connection::Interface *newConnection);

        /**
         * @brief unRegisterConnection Remove connection from existing pool
         * @param existingConnection pointer to connection interface
         * @return True if connection is removed, false otherwise.
         */
        bool unRegisterConnection(Connection::Interface *existingConnection);

        /** @return Return actively selected connection plugin */
        Connection::Interface *getActiveConnection()
        {
            return activeConnection;
        }

        void setDefaultPollingPeriod(uint32_t period);
        void setPollingPeriodRange(uint32_t minimum, uint32_t maximum);
        uint32_t getPollingPeriod()
        {
            return static_cast<uint32_t>(PollPeriodN[0].value);
        }

        /** \return Default name of the device. */
        virtual const char *getDefaultName() = 0;

        /// Period in milliseconds to call TimerHit(). Default 1000 ms
        uint32_t POLLMS = 1000;

    private:
        bool isInit { false };
        bool pDebug { false };
        bool pSimulation { false };

        uint16_t majorVersion { 1 };
        uint16_t minorVersion { 0 };
        uint16_t interfaceDescriptor { 0 };

        ISwitch DebugS[2];
        ISwitch SimulationS[2];
        ISwitch ConfigProcessS[4];
        ISwitch ConnectionS[2];
        INumber PollPeriodN[1];

        ISwitchVectorProperty DebugSP;
        ISwitchVectorProperty SimulationSP;
        ISwitchVectorProperty ConfigProcessSP;
        ISwitchVectorProperty ConnectionSP;
        INumberVectorProperty PollPeriodNP;

        IText DriverInfoT[4] {};
        ITextVectorProperty DriverInfoTP;

        // Connection modes
        ISwitch *ConnectionModeS = nullptr;
        ISwitchVectorProperty ConnectionModeSP;

        std::vector<Connection::Interface *> connections;
        Connection::Interface *activeConnection = nullptr;

        // Connection Plugins
        friend class Connection::Serial;
        friend class Connection::TCP;
        friend class FilterInterface;

        bool defineDynamicProperties = true;
        bool deleteDynamicProperties = true;
};
