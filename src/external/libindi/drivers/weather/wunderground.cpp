/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Weather Underground (TM) Weather Driver

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

#include "wunderground.h"

#include "gason.h"
#include "locale_compat.h"

#include <curl/curl.h>

#include <memory>
#include <cstring>

// We declare an auto pointer to WunderGround.
std::unique_ptr<WunderGround> wunderGround(new WunderGround());

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void ISGetProperties(const char *dev)
{
    wunderGround->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    wunderGround->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    wunderGround->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    wunderGround->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    wunderGround->ISSnoopDevice(root);
}

WunderGround::WunderGround()
{
    setVersion(1, 0);

    wunderLat  = -1000;
    wunderLong = -1000;

    setWeatherConnection(CONNECTION_NONE);
}

WunderGround::~WunderGround()
{
}

const char *WunderGround::getDefaultName()
{
    return (const char *)"WunderGround";
}

bool WunderGround::Connect()
{
    if (wunderAPIKeyT[0].text == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Weather Underground API Key is not available. Please register your API key at "
                                       "www.wunderground.com and save it under Options.");
        return false;
    }

    return true;
}

bool WunderGround::Disconnect()
{
    return true;
}

bool WunderGround::initProperties()
{
    INDI::Weather::initProperties();

    IUFillText(&wunderAPIKeyT[0], "API_KEY", "API Key", nullptr);
    IUFillTextVector(&wunderAPIKeyTP, wunderAPIKeyT, 1, getDeviceName(), "WUNDER_API_KEY", "Wunder", OPTIONS_TAB, IP_RW,
                     60, IPS_IDLE);

    addParameter("WEATHER_FORECAST", "Weather", 0, 0, 0, 1);
    addParameter("WEATHER_TEMPERATURE", "Temperature (C)", -10, 30, -20, 40);
    addParameter("WEATHER_WIND_SPEED", "Wind (kph)", 0, 20, 0, 40);
    addParameter("WEATHER_WIND_GUST", "Gust (kph)", 0, 20, 0, 50);
    addParameter("WEATHER_RAIN_HOUR", "Precip (mm)", 0, 0, 0, 0);

    setCriticalParameter("WEATHER_FORECAST");
    setCriticalParameter("WEATHER_TEMPERATURE");
    setCriticalParameter("WEATHER_WIND_SPEED");
    setCriticalParameter("WEATHER_RAIN_HOUR");

    addDebugControl();

    return true;
}

void WunderGround::ISGetProperties(const char *dev)
{
    INDI::Weather::ISGetProperties(dev);

    defineText(&wunderAPIKeyTP);

    loadConfig(true, "WUNDER_API_KEY");
}

bool WunderGround::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(wunderAPIKeyTP.name, name))
        {
            IUUpdateText(&wunderAPIKeyTP, texts, names, n);
            wunderAPIKeyTP.s = IPS_OK;
            IDSetText(&wunderAPIKeyTP, nullptr);
            return true;
        }
    }

    return INDI::Weather::ISNewText(dev, name, texts, names, n);
}

bool WunderGround::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    wunderLat  = latitude;
    wunderLong = (longitude > 180) ? (longitude - 360) : longitude;

    return true;
}

IPState WunderGround::updateWeather()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    char requestURL[MAXRBUF];

    // If location is not updated yet, return busy
    if (wunderLat == -1000 || wunderLong == -1000)
        return IPS_BUSY;

    AutoCNumeric locale;

    snprintf(requestURL, MAXRBUF, "http://api.wunderground.com/api/%s/conditions/q/%g,%g.json", wunderAPIKeyT[0].text,
             wunderLat, wunderLong);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, requestURL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    char srcBuffer[readBuffer.size()];
    strncpy(srcBuffer, readBuffer.c_str(), readBuffer.size());
    char *source = srcBuffer;
    // do not forget terminate source string with 0
    char *endptr;
    JsonValue value;
    JsonAllocator allocator;
    int status = jsonParse(source, &endptr, &value, allocator);
    if (status != JSON_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s at %zd", jsonStrError(status), endptr - source);
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s", requestURL);
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s", readBuffer.c_str());
        return IPS_ALERT;
    }

    JsonIterator it;
    JsonIterator observationIterator;

    for (it = begin(value); it != end(value); ++it)
    {
        if (!strcmp(it->key, "current_observation"))
        {
            for (observationIterator = begin(it->value); observationIterator != end(it->value); ++observationIterator)
            {
                if (!strcmp(observationIterator->key, "weather"))
                {
                    char *value = observationIterator->value.toString();

                    if (!strcmp(value, "Clear"))
                        setParameterValue("WEATHER_FORECAST", 0);
                    else if (!strcmp(value, "Unknown") || !strcmp(value, "Scattered Clouds") ||
                             !strcmp(value, "Partly Cloudy") || !strcmp(value, "Overcast") ||
                             !strcmp(value, "Patches of Fog") || !strcmp(value, "Partial Fog") ||
                             !strcmp(value, "Light Haze"))
                        setParameterValue("WEATHER_FORECAST", 1);
                    else
                        setParameterValue("WEATHER_FORECAST", 2);

                    DEBUGF(INDI::Logger::DBG_SESSION, "Weather condition: %s", value);
                }
                else if (!strcmp(observationIterator->key, "temp_c"))
                {
                    if (observationIterator->value.isDouble())
                        setParameterValue("WEATHER_TEMPERATURE", observationIterator->value.toNumber());
                    else
                        setParameterValue("WEATHER_TEMPERATURE", atof(observationIterator->value.toString()));
                }
                else if (!strcmp(observationIterator->key, "wind_kph"))
                {
                    if (observationIterator->value.isDouble())
                        setParameterValue("WEATHER_WIND_SPEED", observationIterator->value.toNumber());
                    else
                        setParameterValue("WEATHER_WIND_SPEED", atof(observationIterator->value.toString()));
                }
                else if (!strcmp(observationIterator->key, "wind_gust_kph"))
                {
                    if (observationIterator->value.isDouble())
                        setParameterValue("WEATHER_WIND_GUST", observationIterator->value.toNumber());
                    else
                        setParameterValue("WEATHER_WIND_GUST", atof(observationIterator->value.toString()));
                }
                else if (!strcmp(observationIterator->key, "precip_1hr_metric"))
                {
                    char *value = observationIterator->value.toString();
                    double mm   = -1;
                    if (!strcmp(value, "--"))
                        setParameterValue("WEATHER_RAIN_HOUR", 0);
                    else
                    {
                        mm = atof(value);
                        if (mm >= 0)
                            setParameterValue("WEATHER_RAIN_HOUR", mm);
                    }
                }
            }
        }
    }

    return IPS_OK;
}

bool WunderGround::saveConfigItems(FILE *fp)
{
    INDI::Weather::saveConfigItems(fp);

    IUSaveConfigText(fp, &wunderAPIKeyTP);

    return true;
}
