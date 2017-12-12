/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Weather Device Class

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#pragma once

#include "defaultdevice.h"

#include <list>

namespace Connection
{
class Serial;
class TCP;
}

/**
 * \class Weather
 * \brief Class to provide general functionality of a weather device.
 *
 * The Weather provides a simple interface for weather devices. Parameters such as temperature,
 * wind, humidity etc can be added by the child class as supported by the physical device. With each
 * parameter, the caller specifies the minimum and maximum ranges of OK and WARNING zones. Any value
 * outside of the warning zone is automatically treated as ALERT.
 *
 * The class also specifies the list of critical parameters for observatory operations. When any of
 * the parameters changes state to WARNING or ALERT, then the overall state of the WEATHER_STATUS
 * property reflects the worst state of any individual parameter. The WEATHER_STATUS property may be
 * used by clients to determine whether to proceed with observation tasks or not, and
 * whether to take any safety measures to protect the observatory from severe weather conditions.
 *
 * The child class should start by first adding all the weather parameters via the addParameter()
 * function, then set all the critial parameters via the setCriticalParameter() function, and finally call
 * generateParameterRanges() function to generate all the parameter ranges properties.
 *
 * Weather update period is controlled by the WEATHER_UPDATE property which stores the update period
 * in seconds and calls updateWeather() every X seconds as given in the property.
 *
 * \e IMPORTANT: GEOGRAPHIC_COORD stores latitude and longitude in INDI specific format, refer to
 * <a href="http://indilib.org/develop/developer-manual/101-standard-properties.html">INDI Standard
 * Properties</a> for details.
 *
 * \author Jasem Mutlaq
 */
namespace INDI
{
class Weather : public DefaultDevice
{
  public:
    enum WeatherLocation
    {
        LOCATION_LATITUDE,
        LOCATION_LONGITUDE,
        LOCATION_ELEVATION
    };

    /** \struct WeatherConnection
     * \brief Holds the connection mode of the Weather.
     */
    enum
    {
        CONNECTION_NONE   = 1 << 0, /** Do not use any connection plugin */
        CONNECTION_SERIAL = 1 << 1, /** For regular serial and bluetooth connections */
        CONNECTION_TCP    = 1 << 2  /** For Wired and WiFI connections */
    } WeatherConnection;

    Weather();
    virtual ~Weather();

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

    virtual bool ISSnoopDevice(XMLEle *root);

  protected:
    /**
     * @brief updateWeather Update weather conditions from device or service. The function should
     * not change the state of any property in the device as this is handled by Weather. It
     * should only update the raw values.
     * @return Return overall state. The state should be IPS_OK if data is valid. IPS_BUSY if
     * weather update is in progress. IPS_ALERT is there is an error. The clients will only accept
     * values with IPS_OK state.
     */
    virtual IPState updateWeather();

    /**
     * @brief TimerHit Keep calling updateWeather() until it is successful, if it fails upon first
     * connection.
     */
    virtual void TimerHit();

    /** \brief Update telescope location settings
     *  \param latitude Site latitude in degrees.
     *  \param longitude Site latitude in degrees increasing eastward from Greenwich (0 to 360).
     *  \param elevation Site elevation in meters.
     *  \return True if successful, false otherwise
     *  \note This function performs no action unless subclassed by the child class if required.
     */
    virtual bool updateLocation(double latitude, double longitude, double elevation);

    /**
     * @brief addParameter Add a physical weather measurable parameter to the weather driver.
     * The weather value has three zones:
     * <ol>
     * <li>OK: Set minimum and maximum values for acceptable values.</li>
     * <li>Warning: Set minimum and maximum values for values outside of Ok range and in the
     * dangerous warning zone.</li>
     * <li>Alert: Any value outsize of Ok and Warning zone is marked as Alert.</li>
     * </ol>
     * @param name Name of parameter
     * @param label Label of paremeter (in GUI)
     * @param minimumOK Minimum OK value.
     * @param maximumOK Maximum OK value.
     * @param minimumWarning Minimum Warning value.
     * @param maximumWarning Maximum Warning value.
     */
    void addParameter(std::string name, std::string label, double minimumOK, double maximumOK, double minimumWarning,
                      double maximumWarning);

    /**
     * @brief setCriticalParameter Set parameter that is considered critical to the operation of the
     * observatory. The parameter state can affect the overall weather driver state which signals
     * the client to take appropriate action depending on the severity of the state.
     * @param param Name of critical parameter.
     * @return True if critical parameter was set, false if parameter is not found.
     */
    bool setCriticalParameter(std::string param);

    /**
     * @brief setParameterValue Update weather parameter value
     * @param name name of weather parameter
     * @param value new value of weather parameter;
     */
    void setParameterValue(std::string name, double value);

    /**
     * @brief setWeatherConnection Set Weather connection mode. Child class should call this
     * in the constructor before Weather registers any connection interfaces
     * @param value ORed combination of WeatherConnection values.
     */
    void setWeatherConnection(const uint8_t &value);

    /**
     * @return Get current Weather connection mode
     */
    uint8_t getWeatherConnection() const;

    virtual bool saveConfigItems(FILE *fp);

    /** \brief perform handshake with device to check communication */
    virtual bool Handshake();

    // A number vector that stores lattitude and longitude
    INumberVectorProperty LocationNP;
    INumber LocationN[3];

    // Refresh data
    ISwitch RefreshS[1];
    ISwitchVectorProperty RefreshSP;

    // Parameters
    INumber *ParametersN;
    INumberVectorProperty ParametersNP;

    // Parameter Ranges
    INumberVectorProperty *ParametersRangeNP;
    uint8_t nRanges;

    // Weather status
    ILight *critialParametersL;
    ILightVectorProperty critialParametersLP;

    // Active devices to snoop
    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[1];

    // Update Period
    INumber UpdatePeriodN[1];
    INumberVectorProperty UpdatePeriodNP;

    Connection::Serial *serialConnection = NULL;
    Connection::TCP *tcpConnection       = NULL;

    int PortFD = -1;

  private:
    bool processLocationInfo(double latitude, double longitude, double elevation);
    void createParameterRange(std::string name, std::string label);
    void updateWeatherState();
    int updateTimerID;

    bool callHandshake();
    uint8_t weatherConnection = CONNECTION_SERIAL | CONNECTION_TCP;
};
}
