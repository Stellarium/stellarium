#if 0
  This file is part of the AAG Cloud Watcher INDI Driver.
  A driver for the AAG Cloud Watcher (AAGware - http://www.aagware.eu/)

  Copyright (C) 2012-2015 Sergio Alonso (zerjioi@ugr.es)



  AAG Cloud Watcher INDI Driver is free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  AAG Cloud Watcher INDI Driver is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with AAG Cloud Watcher INDI Driver.  If not, see
  <http://www.gnu.org/licenses/>.
  
  Anemometer code contributed by Joao Bento.
#endif

/* Our driver header */
#include "indi_aagcloudwatcher.h"

#include "config.h"

#include <defaultdevice.h>

#include <cstring>
#include <cmath>
#include <memory>

#define ABS_ZERO 273.15

/* auto pointer */
std::unique_ptr<AAGCloudWatcher> cloudWatcher(new AAGCloudWatcher());

AAGCloudWatcher::AAGCloudWatcher()
{
    setVersion(AAG_VERSION_MAJOR, AAG_VERSION_MINOR);

    DEBUG(INDI::Logger::DBG_DEBUG, "Initializing from AAG Cloud Watcher device...");

    cwc = NULL;

    lastReadPeriod = 3.0;

    heatingStatus = normal;

    pulseStartTime = -1;
    wetStartTime   = -1;

    desiredSensorTemperature = 0;
    globalRainSensorHeater   = -1;
}

AAGCloudWatcher::~AAGCloudWatcher()
{
}

/**********************************************************************
** Initialize all properties & set default values.
**********************************************************************/
bool AAGCloudWatcher::initProperties()
{
    DefaultDevice::initProperties();
    buildSkeleton("indi_aagcloudwatcher_sk.xml");
    return true;
}

/*************************************************************************
** Define Basic properties to clients.
*************************************************************************/
void AAGCloudWatcher::ISGetProperties(const char *dev)
{
    static int configLoaded = 0;
    // Ask the default driver first to send properties.
    INDI::DefaultDevice::ISGetProperties(dev);

    // If no configuration is load before, then load it now.
    if (configLoaded == 0)
    {
        loadConfig();
        configLoaded = 1;
    }
}

/*****************************************************************************
** Process Text properties
*****************************************************************************/
bool AAGCloudWatcher::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDefaultName()))
    {
        return false;
    }

    ITextVectorProperty *tvp = getText(name);

    if (!tvp)
    {
        return false;
    }

    // Are we updating the serial port
    if (!strcmp(tvp->name, "serial"))
    {
        IUUpdateText(tvp, texts, names, n);
        tvp->s = IPS_OK;
        IDSetText(tvp, NULL);

        return true;
    }

    return false;
}

bool AAGCloudWatcher::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDefaultName()))
    {
        return false;
    }

    INumberVectorProperty *nvp = getNumber(name);

    if (!nvp)
    {
        return false;
    }

    if (!strcmp(nvp->name, "heaterParameters"))
    {
        for (int i = 0; i < 8; i++)
        {
            if ((strcmp(names[i], "tempLow") == 0) || (strcmp(names[i], "tempHigh") == 0))
            {
                if (values[i] < -50)
                {
                    values[i] = -50;
                }
                else if (values[i] > 100)
                {
                    values[i] = 100;
                }
            }

            if ((strcmp(names[i], "deltaHigh") == 0) || (strcmp(names[i], "deltaLow") == 0))
            {
                if (values[i] < 0)
                {
                    values[i] = 0;
                }
                else if (values[i] > 50)
                {
                    values[i] = 50;
                }
            }

            if ((strcmp(names[i], "min") == 0))
            {
                if (values[i] < 10)
                {
                    values[i] = 10;
                }
                else if (values[i] > 20)
                {
                    values[i] = 20;
                }
            }

            if ((strcmp(names[i], "heatImpulseTemp") == 0))
            {
                if (values[i] < 1)
                {
                    values[i] = 1;
                }
                else if (values[i] > 30)
                {
                    values[i] = 30;
                }
            }

            if ((strcmp(names[i], "heatImpulseDuration") == 0))
            {
                if (values[i] < 0)
                {
                    values[i] = 0;
                }
                else if (values[i] > 600)
                {
                    values[i] = 600;
                }
            }

            if ((strcmp(names[i], "heatImpulseCycle") == 0))
            {
                if (values[i] < 60)
                {
                    values[i] = 60;
                }
                else if (values[i] > 1000)
                {
                    values[i] = 1000;
                }
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "refresh"))
    {
        if (values[0] < 10)
        {
            values[0] = 10;
        }
        if (values[0] > 60)
        {
            values[0] = 60;
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "skyCorrection"))
    {
        for (int i = 0; i < 5; i++)
        {
            if (values[i] < -999)
            {
                values[i] = -999;
            }
            if (values[i] > 999)
            {
                values[i] = 999;
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "limitsCloud"))
    {
        for (int i = 0; i < n; i++)
        {
            if (values[i] < -50)
            {
                values[i] = -50;
            }
            if (values[i] > 100)
            {
                values[i] = 100;
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "limitsRain"))
    {
        for (int i = 0; i < n; i++)
        {
            if (values[i] < 0)
            {
                values[i] = 0;
            }
            if (values[i] > 100000)
            {
                values[i] = 100000;
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "limitsBrightness"))
    {
        for (int i = 0; i < n; i++)
        {
            if (values[i] < 0)
            {
                values[i] = 0;
            }
            if (values[i] > 1000000)
            {
                values[i] = 1000000;
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    if (!strcmp(nvp->name, "limitsWind"))
    {
        for (int i = 0; i < n; i++)
        {
            if (values[i] < 0)
            {
                values[i] = 0;
            }
            if (values[i] > 100)
            {
                values[i] = 100;
            }
        }

        IUUpdateNumber(nvp, values, names, n);
        nvp->s = IPS_OK;
        IDSetNumber(nvp, NULL);

        return true;
    }

    return false;
}

bool AAGCloudWatcher::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    // ignore if not ours //

    if (strcmp(dev, getDefaultName()))
    {
        return false;
    }

    if (INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n) == true)
    {
        return true;
    }

    ISwitchVectorProperty *svp = getSwitch(name);

    if (!svp)
    {
        return false;
    }

    int error = 0;
    if (!strcmp(svp->name, "deviceSwitch"))
    {
        char *namesSw[2];
        ISState statesSw[2];
        statesSw[0] = ISS_ON;
        statesSw[1] = ISS_OFF;
        namesSw[0]  = const_cast<char *>("open");
        namesSw[1]  = const_cast<char *>("close");

        ISState openState;

        if (strcmp(names[0], "open") == 0)
        {
            openState = states[0];
        }
        else
        {
            openState = states[1];
        }

        if (openState == ISS_ON)
        {
            if (isConnected())
            {
                bool r = cwc->openSwitch();

                if (!r)
                {
                    statesSw[0] = ISS_OFF;
                    statesSw[1] = ISS_ON;
                }
            }
            else
            {
                statesSw[0] = ISS_ON;
                statesSw[1] = ISS_OFF;
                error       = 1;
            }
        }
        else
        {
            if (isConnected())
            {
                bool r = cwc->closeSwitch();

                if (r)
                {
                    statesSw[0] = ISS_OFF;
                    statesSw[1] = ISS_ON;
                }
            }
            else
            {
                statesSw[0] = ISS_ON;
                statesSw[1] = ISS_OFF;
                error       = 1;
            }
        }

        IUUpdateSwitch(svp, statesSw, namesSw, 2);
        if (error)
        {
            svp->s = IPS_IDLE;
        }
        else
        {
            svp->s = IPS_OK;
        }
        IDSetSwitch(svp, NULL);

        return true;
    }

    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

int AAGCloudWatcher::getRefreshPeriod()
{
    INumberVectorProperty *nvp = getNumber("refresh");

    if (!nvp)
    {
        return 3;
    }

    int refreshValue = int(nvp->np[0].value);

    return refreshValue;
}

float AAGCloudWatcher::getLastReadPeriod()
{
    return lastReadPeriod;
}

bool AAGCloudWatcher::isWetRain()
{
    ISwitchVectorProperty *svpRC = getSwitch("rainConditions");

    for (int i = 0; i < svpRC->nsp; i++)
    {
        if (strcmp("wet", svpRC->sp[i].name) == 0)
        {
            if (svpRC->sp[i].s == ISS_ON)
            {
                return true;
            }
        }

        if (strcmp("rain", svpRC->sp[i].name) == 0)
        {
            if (svpRC->sp[i].s == ISS_ON)
            {
                return true;
            }
        }
    }

    return false;
}

bool AAGCloudWatcher::heatingAlgorithm()
{
    INumberVectorProperty *heaterParameters = getNumber("heaterParameters");
    float tempLow                           = getNumberValueFromVector(heaterParameters, "tempLow");
    float tempHigh                          = getNumberValueFromVector(heaterParameters, "tempHigh");
    float deltaLow                          = getNumberValueFromVector(heaterParameters, "deltaLow");
    float deltaHigh                         = getNumberValueFromVector(heaterParameters, "deltaHigh");
    float heatImpulseTemp                   = getNumberValueFromVector(heaterParameters, "heatImpulseTemp");
    float heatImpulseDuration               = getNumberValueFromVector(heaterParameters, "heatImpulseDuration");
    float heatImpulseCycle                  = getNumberValueFromVector(heaterParameters, "heatImpulseCycle");
    float min                               = getNumberValueFromVector(heaterParameters, "min");

    INumberVectorProperty *sensors = getNumber("sensors");
    float ambient                  = getNumberValueFromVector(sensors, "ambientTemperatureSensor");
    float rainSensorTemperature    = getNumberValueFromVector(sensors, "rainSensorTemperature");

    INumberVectorProperty *ref = getNumber("refresh");
    float refresh              = getNumberValueFromVector(ref, "refreshPeriod");

    if (globalRainSensorHeater == -1)
    { // If not already setted
        globalRainSensorHeater = getNumberValueFromVector(sensors, "rainSensorHeater");
    }

    time_t currentTime = time(NULL);

    if ((isWetRain()) && (heatingStatus == normal))
    { // We check if sensor is wet.
        if (wetStartTime == -1)
        { // First moment wet
            wetStartTime = time(NULL);
        }
        else
        { // We have been wet for a while

            if (currentTime - wetStartTime >= heatImpulseCycle)
            { // We have been a cycle wet. Apply pulse
                wetStartTime   = -1;
                heatingStatus  = increasingToPulse;
                pulseStartTime = -1;
            }
        }
    }
    else
    { // is not wet
        wetStartTime = -1;
    }

    if (heatingStatus == pulse)
    {
        if (currentTime - pulseStartTime > heatImpulseDuration)
        { // Pulse ends
            heatingStatus  = normal;
            wetStartTime   = -1;
            pulseStartTime = -1;
        }
    }

    if (heatingStatus == normal)
    {
        // Compute desired temperature

        if (ambient < tempLow)
        {
            desiredSensorTemperature = deltaLow;
        }
        else if (ambient > tempHigh)
        {
            desiredSensorTemperature = ambient + deltaHigh;
        }
        else
        { // Between tempLow and tempHigh
            float delt = ((((ambient - tempLow) / (tempHigh - tempLow)) * (deltaHigh - deltaLow)) + deltaLow);

            desiredSensorTemperature = ambient + delt;

            if (desiredSensorTemperature < tempLow)
            {
                desiredSensorTemperature = deltaLow;
            }
        }
    }
    else
    {
        desiredSensorTemperature = ambient + heatImpulseTemp;
    }

    if (heatingStatus == increasingToPulse)
    {
        if (rainSensorTemperature < desiredSensorTemperature)
        {
            globalRainSensorHeater = 100.0;
        }
        else
        {
            // the pulse starts
            pulseStartTime = time(NULL);
            heatingStatus  = pulse;
        }
    }

    if ((heatingStatus == normal) || (heatingStatus == pulse))
    { // Check desired temperature and act accordingly
        // Obtain the difference in temperature and modifier
        float dif             = fabs(desiredSensorTemperature - rainSensorTemperature);
        float refreshModifier = sqrt(refresh / 10.0);
        float modifier        = 1;

        if (dif > 8)
        {
            modifier = (1.4 / refreshModifier);
        }
        else if (dif > 4)
        {
            modifier = (1.2 / refreshModifier);
        }
        else if (dif > 3)
        {
            modifier = (1.1 / refreshModifier);
        }
        else if (dif > 2)
        {
            modifier = (1.06 / refreshModifier);
        }
        else if (dif > 1)
        {
            modifier = (1.04 / refreshModifier);
        }
        else if (dif > 0.5)
        {
            modifier = (1.02 / refreshModifier);
        }
        else if (dif > 0.3)
        {
            modifier = (1.01 / refreshModifier);
        }

        if (rainSensorTemperature > desiredSensorTemperature)
        {   // Lower heating
            //   IDLog("Temp: %f, Desired: %f, Lowering: %f %f -> %f\n", rainSensorTemperature, desiredSensorTemperature, modifier, globalRainSensorHeater, globalRainSensorHeater / modifier);
            globalRainSensorHeater /= modifier;
        }
        else
        {   // increase heating
            //   IDLog("Temp: %f, Desired: %f, Increasing: %f %f -> %f\n", rainSensorTemperature, desiredSensorTemperature, modifier, globalRainSensorHeater, globalRainSensorHeater * modifier);
            globalRainSensorHeater *= modifier;
        }
    }

    if (globalRainSensorHeater < min)
    {
        globalRainSensorHeater = min;
    }
    if (globalRainSensorHeater > 100.0)
    {
        globalRainSensorHeater = 100.0;
    }

    int rawSensorHeater = int(globalRainSensorHeater * 1023.0 / 100.0);

    cwc->setPWMDutyCycle(rawSensorHeater);

    // Sending heater status to clients
    char *namesSw[3];
    ISState statesSw[3];
    statesSw[0] = ISS_OFF;
    statesSw[1] = ISS_OFF;
    statesSw[2] = ISS_OFF;
    namesSw[0]  = const_cast<char *>("normal");
    namesSw[1]  = const_cast<char *>("increasing");
    namesSw[2]  = const_cast<char *>("pulse");

    if (heatingStatus == normal)
    {
        statesSw[0] = ISS_ON;
    }
    else if (heatingStatus == increasingToPulse)
    {
        statesSw[1] = ISS_ON;
    }
    else if (heatingStatus == pulse)
    {
        statesSw[2] = ISS_ON;
    }

    ISwitchVectorProperty *svp = getSwitch("heaterStatus");
    IUUpdateSwitch(svp, statesSw, namesSw, 3);
    svp->s = IPS_OK;
    IDSetSwitch(svp, NULL);
    return true;
}

void ISPoll(void *p)
{
    AAGCloudWatcher *cw = (AAGCloudWatcher *)p;

    if (cw->isConnected())
    {
        cw->sendData();

        cw->heatingAlgorithm();

        int secs = cw->getRefreshPeriod() - cw->getLastReadPeriod();

        if (secs < 1)
        {
            secs = 1;
        }

        IEAddTimer(secs * 1000, ISPoll, p); // Create a timer to send parameters
    }
}

bool AAGCloudWatcher::isConnected()
{
    if (cwc != NULL)
    {
        return true;
    }

    return false;
}

bool AAGCloudWatcher::sendData()
{
    CloudWatcherData data;

    int r = cwc->getAllData(&data);

    if (!r)
    {
        return false;
    }

    int N_DATA = 11;
    double values[N_DATA];
    char *names[N_DATA];

    names[0]  = const_cast<char *>("supply");
    values[0] = data.supply;

    names[1]  = const_cast<char *>("sky");
    values[1] = data.sky;

    names[2]  = const_cast<char *>("sensor");
    values[2] = data.sensor;

    names[3]  = const_cast<char *>("ambient");
    values[3] = data.ambient;

    names[4]  = const_cast<char *>("rain");
    values[4] = data.rain;

    names[5]  = const_cast<char *>("rainHeater");
    values[5] = data.rainHeater;

    names[6]  = const_cast<char *>("rainTemp");
    values[6] = data.rainTemperature;

    names[7]  = const_cast<char *>("LDR");
    values[7] = data.ldr;

    names[8]       = const_cast<char *>("readCycle");
    values[8]      = data.readCycle;
    lastReadPeriod = data.readCycle;

    names[9]  = const_cast<char *>("windSpeed");
    values[9] = data.windSpeed;

    names[10]  = const_cast<char *>("totalReadings");
    values[10] = data.totalReadings;

    INumberVectorProperty *nvp = getNumber("readings");
    IUUpdateNumber(nvp, values, names, N_DATA);
    nvp->s = IPS_OK;
    IDSetNumber(nvp, NULL);

    int N_ERRORS = 5;
    double valuesE[N_ERRORS];
    char *namesE[N_ERRORS];

    namesE[0]  = const_cast<char *>("internalErrors");
    valuesE[0] = data.internalErrors;

    namesE[1]  = const_cast<char *>("firstAddressByteErrors");
    valuesE[1] = data.firstByteErrors;

    namesE[2]  = const_cast<char *>("commandByteErrors");
    valuesE[2] = data.commandByteErrors;

    namesE[3]  = const_cast<char *>("secondAddressByteErrors");
    valuesE[3] = data.secondByteErrors;

    namesE[4]  = const_cast<char *>("pecByteErrors");
    valuesE[4] = data.pecByteErrors;

    INumberVectorProperty *nvpE = getNumber("unitErrors");
    IUUpdateNumber(nvpE, valuesE, namesE, N_ERRORS);
    nvpE->s = IPS_OK;
    IDSetNumber(nvpE, NULL);

    int N_SENS = 8;
    double valuesS[N_SENS];
    char *namesS[N_SENS];

    float skyTemperature = float(data.sky) / 100.0;
    namesS[0]            = const_cast<char *>("infraredSky");
    valuesS[0]           = skyTemperature;

    namesS[1]  = const_cast<char *>("infraredSensor");
    valuesS[1] = float(data.sensor) / 100.0;

    namesS[2]  = const_cast<char *>("rainSensor");
    valuesS[2] = data.rain;

    float rainSensorTemperature = data.rainTemperature;
    if (rainSensorTemperature > 1022)
    {
        rainSensorTemperature = 1022;
    }
    if (rainSensorTemperature < 1)
    {
        rainSensorTemperature = 1;
    }
    rainSensorTemperature = constants.rainPullUpResistance / ((1023.0 / rainSensorTemperature) - 1.0);
    rainSensorTemperature = log(rainSensorTemperature / constants.rainResistanceAt25);
    rainSensorTemperature =
        1.0 / (rainSensorTemperature / constants.rainBetaFactor + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;

    namesS[3]  = const_cast<char *>("rainSensorTemperature");
    valuesS[3] = rainSensorTemperature;

    float rainSensorHeater = data.rainHeater;
    rainSensorHeater       = 100.0 * rainSensorHeater / 1023.0;
    namesS[4]              = const_cast<char *>("rainSensorHeater");
    valuesS[4]             = rainSensorHeater;

    float ambientLight = float(data.ldr);
    if (ambientLight > 1022.0)
    {
        ambientLight = 1022.0;
    }
    if (ambientLight < 1)
    {
        ambientLight = 1.0;
    }
    ambientLight = constants.ldrPullUpResistance / ((1023.0 / ambientLight) - 1.0);
    namesS[5]    = const_cast<char *>("brightnessSensor");
    valuesS[5]   = ambientLight;

    float ambientTemperature = data.ambient;

    if (ambientTemperature == -10000)
    {
        ambientTemperature = float(data.sensor) / 100.0;
    }
    else
    {
        if (ambientTemperature > 1022)
        {
            ambientTemperature = 1022;
        }
        if (ambientTemperature < 1)
        {
            ambientTemperature = 1;
        }
        ambientTemperature = constants.ambientPullUpResistance / ((1023.0 / ambientTemperature) - 1.0);
        ambientTemperature = log(ambientTemperature / constants.ambientResistanceAt25);
        ambientTemperature =
            1.0 / (ambientTemperature / constants.ambientBetaFactor + 1.0 / (ABS_ZERO + 25.0)) - ABS_ZERO;
    }

    namesS[6]  = const_cast<char *>("ambientTemperatureSensor");
    valuesS[6] = ambientTemperature;

    INumberVectorProperty *nvpSky = getNumber("skyCorrection");
    float k1                      = getNumberValueFromVector(nvpSky, "k1");
    float k2                      = getNumberValueFromVector(nvpSky, "k2");
    float k3                      = getNumberValueFromVector(nvpSky, "k3");
    float k4                      = getNumberValueFromVector(nvpSky, "k4");
    float k5                      = getNumberValueFromVector(nvpSky, "k5");

    float correctedTemperature =
        skyTemperature - ((k1 / 100.0) * (ambientTemperature - k2 / 10.0) +
                          (k3 / 100.0) * pow(exp(k4 / 1000 * ambientTemperature), (k5 / 100.0)));

    namesS[7]  = const_cast<char *>("correctedInfraredSky");
    valuesS[7] = correctedTemperature;

    INumberVectorProperty *nvpS = getNumber("sensors");
    IUUpdateNumber(nvpS, valuesS, namesS, N_SENS);
    nvpS->s = IPS_OK;
    IDSetNumber(nvpS, NULL);

    ISState states[2];
    char *namesSw[2];
    namesSw[0] = const_cast<char *>("open");
    namesSw[1] = const_cast<char *>("close");
    //IDLog("%d\n", data.switchStatus);
    if (data.switchStatus == 1)
    {
        states[0] = ISS_OFF;
        states[1] = ISS_ON;
    }
    else
    {
        states[0] = ISS_ON;
        states[1] = ISS_OFF;
    }

    ISwitchVectorProperty *svpSw = getSwitch("deviceSwitch");
    IUUpdateSwitch(svpSw, states, namesSw, 2);
    svpSw->s = IPS_OK;
    IDSetSwitch(svpSw, NULL);

    INumberVectorProperty *nvpLimits = getNumber("limitsCloud");
    int clearLimit                   = getNumberValueFromVector(nvpLimits, "clear");
    int cloudyLimit                  = getNumberValueFromVector(nvpLimits, "cloudy");
    int overcastLimit                = getNumberValueFromVector(nvpLimits, "overcast");

    ISState statesCloud[4];
    char *namesCloud[4];
    namesCloud[0]  = const_cast<char *>("clear");
    namesCloud[1]  = const_cast<char *>("cloudy");
    namesCloud[2]  = const_cast<char *>("overcast");
    namesCloud[3]  = const_cast<char *>("unknown");
    statesCloud[0] = ISS_OFF;
    statesCloud[1] = ISS_OFF;
    statesCloud[2] = ISS_OFF;
    statesCloud[3] = ISS_OFF;
    //IDLog("%d\n", data.switchStatus);
    if (correctedTemperature < clearLimit)
    {
        statesCloud[0] = ISS_ON;
    }
    else if (correctedTemperature < cloudyLimit)
    {
        statesCloud[1] = ISS_ON;
    }
    else if (correctedTemperature < overcastLimit)
    {
        statesCloud[2] = ISS_ON;
    }
    else
    {
        statesCloud[3] = ISS_ON;
    }

    ISwitchVectorProperty *svpCC = getSwitch("cloudConditions");
    IUUpdateSwitch(svpCC, statesCloud, namesCloud, 4);
    svpCC->s = IPS_OK;
    IDSetSwitch(svpCC, NULL);

    nvpLimits     = getNumber("limitsRain");
    int dryLimit  = getNumberValueFromVector(nvpLimits, "dry");
    int wetLimit  = getNumberValueFromVector(nvpLimits, "wet");
    int rainLimit = getNumberValueFromVector(nvpLimits, "rain");

    ISState statesRain[4];
    char *namesRain[4];
    namesRain[0]  = const_cast<char *>("dry");
    namesRain[1]  = const_cast<char *>("wet");
    namesRain[2]  = const_cast<char *>("rain");
    namesRain[3]  = const_cast<char *>("unknown");
    statesRain[0] = ISS_OFF;
    statesRain[1] = ISS_OFF;
    statesRain[2] = ISS_OFF;
    statesRain[3] = ISS_OFF;
    //IDLog("%d\n", data.switchStatus);
    if (data.rain < rainLimit)
    {
        statesRain[3] = ISS_ON;
    }
    else if (data.rain < wetLimit)
    {
        statesRain[2] = ISS_ON;
    }
    else if (data.rain < dryLimit)
    {
        statesRain[1] = ISS_ON;
    }
    else
    {
        statesRain[0] = ISS_ON;
    }

    ISwitchVectorProperty *svpRC = getSwitch("rainConditions");
    IUUpdateSwitch(svpRC, statesRain, namesRain, 4);
    svpRC->s = IPS_OK;
    IDSetSwitch(svpRC, NULL);

    nvpLimits          = getNumber("limitsBrightness");
    int darkLimit      = getNumberValueFromVector(nvpLimits, "dark");
    int lightLimit     = getNumberValueFromVector(nvpLimits, "light");
//    int veryLightLimit = getNumberValueFromVector(nvpLimits, "veryLight");

    ISState statesBrightness[3];
    char *namesBrightness[3];
    namesBrightness[0]  = const_cast<char *>("dark");
    namesBrightness[1]  = const_cast<char *>("light");
    namesBrightness[2]  = const_cast<char *>("veryLight");
    statesBrightness[0] = ISS_OFF;
    statesBrightness[1] = ISS_OFF;
    statesBrightness[2] = ISS_OFF;
    //IDLog("%d\n", data.switchStatus);
    if (ambientLight > darkLimit)
    {
        statesBrightness[0] = ISS_ON;
    }
    else if (ambientLight > lightLimit)
    {
        statesBrightness[1] = ISS_ON;
    }
    else
    {
        statesBrightness[2] = ISS_ON;
    }

    ISwitchVectorProperty *svpBC = getSwitch("brightnessConditions");
    IUUpdateSwitch(svpBC, statesBrightness, namesBrightness, 3);
    svpBC->s = IPS_OK;
    IDSetSwitch(svpBC, NULL);

    int windSpeed         = data.windSpeed;
    nvpLimits             = getNumber("limitsWind");
    int calmLimit         = getNumberValueFromVector(nvpLimits, "calm");
    int moderateWindLimit = getNumberValueFromVector(nvpLimits, "moderateWind");

    INumberVectorProperty *consts = getNumber("constants");
    int anemometerStatus          = getNumberValueFromVector(consts, "anemometerStatus");

    ISState statesWind[4];
    char *namesWind[4];
    namesWind[0]  = const_cast<char *>("calm");
    namesWind[1]  = const_cast<char *>("moderateWind");
    namesWind[2]  = const_cast<char *>("strongWind");
    namesWind[3]  = const_cast<char *>("unknown");
    statesWind[0] = ISS_OFF;
    statesWind[1] = ISS_OFF;
    statesWind[2] = ISS_OFF;
    statesWind[3] = ISS_OFF;
    //IDLog("%d\n", data.switchStatus);

    if (anemometerStatus)
    {
        if (windSpeed < calmLimit)
        {
            statesWind[0] = ISS_ON;
        }
        else if (windSpeed < moderateWindLimit)
        {
            statesWind[1] = ISS_ON;
        }
        else
        {
            statesWind[2] = ISS_ON;
        }
    }
    else
    {
        statesWind[3] = ISS_ON;
    }

    ISwitchVectorProperty *svpWC = getSwitch("windConditions");
    IUUpdateSwitch(svpWC, statesWind, namesWind, 4);
    svpWC->s = IPS_OK;
    IDSetSwitch(svpWC, NULL);
    return true;
}

double AAGCloudWatcher::getNumberValueFromVector(INumberVectorProperty *nvp, const char *name)
{
    for (int i = 0; i < nvp->nnp; i++)
    {
        if (strcmp(name, nvp->np[i].name) == 0)
        {
            return nvp->np[i].value;
        }
    }
    return 0;
}

bool AAGCloudWatcher::resetData()
{
    CloudWatcherData data;

    int r = cwc->getAllData(&data);

    if (!r)
    {
        return false;
    }

    int N_DATA = 11;
    double values[N_DATA];
    char *names[N_DATA];

    names[0]  = const_cast<char *>("supply");
    values[0] = 0;

    names[1]  = const_cast<char *>("sky");
    values[1] = 0;

    names[2]  = const_cast<char *>("sensor");
    values[2] = 0;

    names[3]  = const_cast<char *>("ambient");
    values[3] = 0;

    names[4]  = const_cast<char *>("rain");
    values[4] = 0;

    names[5]  = const_cast<char *>("rainHeater");
    values[5] = 0;

    names[6]  = const_cast<char *>("rainTemp");
    values[6] = 0;

    names[7]  = const_cast<char *>("LDR");
    values[7] = 0;

    names[8]  = const_cast<char *>("readCycle");
    values[8] = 0;

    names[9]  = const_cast<char *>("windSpeed");
    values[9] = 0;

    names[10]  = const_cast<char *>("totalReadings");
    values[10] = 0;

    INumberVectorProperty *nvp = getNumber("readings");
    IUUpdateNumber(nvp, values, names, N_DATA);
    nvp->s = IPS_IDLE;
    IDSetNumber(nvp, NULL);

    int N_ERRORS = 5;
    double valuesE[N_ERRORS];
    char *namesE[N_ERRORS];

    namesE[0]  = const_cast<char *>("internalErrors");
    valuesE[0] = 0;

    namesE[1]  = const_cast<char *>("firstAddressByteErrors");
    valuesE[1] = 0;

    namesE[2]  = const_cast<char *>("commandByteErrors");
    valuesE[2] = 0;

    namesE[3]  = const_cast<char *>("secondAddressByteErrors");
    valuesE[3] = 0;

    namesE[4]  = const_cast<char *>("pecByteErrors");
    valuesE[4] = 0;

    INumberVectorProperty *nvpE = getNumber("unitErrors");
    IUUpdateNumber(nvpE, valuesE, namesE, N_ERRORS);
    nvpE->s = IPS_IDLE;
    IDSetNumber(nvpE, NULL);

    int N_SENS = 8;
    double valuesS[N_SENS];
    char *namesS[N_SENS];

    namesS[0]  = const_cast<char *>("infraredSky");
    valuesS[0] = 0.0;

    namesS[1]  = const_cast<char *>("infraredSensor");
    valuesS[1] = 0.0;

    namesS[2]  = const_cast<char *>("rainSensor");
    valuesS[2] = 0.0;

    namesS[3]  = const_cast<char *>("rainSensorTemperature");
    valuesS[3] = 0.0;

    namesS[4]  = const_cast<char *>("rainSensorHeater");
    valuesS[4] = 0.0;

    namesS[5]  = const_cast<char *>("brightnessSensor");
    valuesS[5] = 0.0;

    namesS[6]  = const_cast<char *>("ambientTemperatureSensor");
    valuesS[6] = 0.0;

    namesS[7]  = const_cast<char *>("correctedInfraredSky");
    valuesS[7] = 0.0;

    INumberVectorProperty *nvpS = getNumber("sensors");
    IUUpdateNumber(nvpS, valuesS, namesS, N_SENS);
    nvpS->s = IPS_IDLE;
    IDSetNumber(nvpS, NULL);

    ISwitchVectorProperty *svpSw = getSwitch("deviceSwitch");
    svpSw->s                     = IPS_IDLE;
    IDSetSwitch(svpSw, NULL);

    ISwitchVectorProperty *svpCC = getSwitch("cloudConditions");
    svpCC->s                     = IPS_IDLE;
    IDSetSwitch(svpCC, NULL);

    ISwitchVectorProperty *svpRC = getSwitch("rainConditions");
    svpRC->s                     = IPS_IDLE;
    IDSetSwitch(svpRC, NULL);

    ISwitchVectorProperty *svpBC = getSwitch("brightnessConditions");
    svpBC->s                     = IPS_IDLE;
    IDSetSwitch(svpBC, NULL);

    ISwitchVectorProperty *svp = getSwitch("heaterStatus");
    svp->s                     = IPS_IDLE;
    IDSetSwitch(svp, NULL);
    return true;
}

bool AAGCloudWatcher::sendConstants()
{
    INumberVectorProperty *nvp = getNumber("constants");

    ITextVectorProperty *tvp = getText("FW");

    int r = cwc->getConstants(&constants);

    if (!r)
    {
        return false;
    }

    int N_CONSTANTS = 11;
    double values[N_CONSTANTS];
    char *names[N_CONSTANTS];

    names[0]  = const_cast<char *>("internalSerialNumber");
    values[0] = constants.internalSerialNumber;

    names[1]  = const_cast<char *>("zenerVoltage");
    values[1] = constants.zenerVoltage;

    names[2]  = const_cast<char *>("LDRMaxResistance");
    values[2] = constants.ldrMaxResistance;

    names[3]  = const_cast<char *>("LDRPullUpResistance");
    values[3] = constants.ldrPullUpResistance;

    names[4]  = const_cast<char *>("rainBetaFactor");
    values[4] = constants.rainBetaFactor;

    names[5]  = const_cast<char *>("rainResistanceAt25");
    values[5] = constants.rainResistanceAt25;

    names[6]  = const_cast<char *>("rainPullUpResistance");
    values[6] = constants.rainPullUpResistance;

    names[7]  = const_cast<char *>("ambientBetaFactor");
    values[7] = constants.ambientBetaFactor;

    names[8]  = const_cast<char *>("ambientResistanceAt25");
    values[8] = constants.ambientResistanceAt25;

    names[9]  = const_cast<char *>("ambientPullUpResistance");
    values[9] = constants.ambientPullUpResistance;

    names[10]  = const_cast<char *>("anemometerStatus");
    values[10] = constants.anemometerStatus;

    IUUpdateNumber(nvp, values, names, N_CONSTANTS);
    nvp->s = IPS_OK;
    IDSetNumber(nvp, NULL);

    char *valuesT[1];
    char *namesT[1];

    namesT[0]  = const_cast<char *>("firmwareVersion");
    valuesT[0] = constants.firmwareVersion;

    IUUpdateText(tvp, valuesT, namesT, 1);
    tvp->s = IPS_OK;
    IDSetText(tvp, NULL);
    return true;
}

bool AAGCloudWatcher::resetConstants()
{
    INumberVectorProperty *nvp = getNumber("constants");

    ITextVectorProperty *tvp = getText("FW");

    CloudWatcherConstants constants;

    int r = cwc->getConstants(&constants);

    if (!r)
    {
        return false;
    }

    int N_CONSTANTS = 11;
    double values[N_CONSTANTS];
    char *names[N_CONSTANTS];

    names[0]  = const_cast<char *>("internalSerialNumber");
    values[0] = 0;

    names[1]  = const_cast<char *>("zenerVoltage");
    values[1] = 0;

    names[2]  = const_cast<char *>("LDRMaxResistance");
    values[2] = 0;

    names[3]  = const_cast<char *>("LDRPullUpResistance");
    values[3] = 0;

    names[4]  = const_cast<char *>("rainBetaFactor");
    values[4] = 0;

    names[5]  = const_cast<char *>("rainResistanceAt25");
    values[5] = 0;

    names[6]  = const_cast<char *>("rainPullUpResistance");
    values[6] = 0;

    names[7]  = const_cast<char *>("ambientBetaFactor");
    values[7] = 0;

    names[8]  = const_cast<char *>("ambientResistanceAt25");
    values[8] = 0;

    names[9]  = const_cast<char *>("ambientPullUpResistance");
    values[9] = 0;

    names[10]  = const_cast<char *>("anemometerStatus");
    values[10] = 0;

    IUUpdateNumber(nvp, values, names, N_CONSTANTS);
    nvp->s = IPS_IDLE;
    IDSetNumber(nvp, NULL);

    char *valuesT[1];
    char *namesT[1];

    namesT[0]  = const_cast<char *>("firmwareVersion");
    valuesT[0] = const_cast<char *>("-");

    IUUpdateText(tvp, valuesT, namesT, 1);
    tvp->s = IPS_IDLE;
    IDSetText(tvp, NULL);
    return true;
}

bool AAGCloudWatcher::Connect()
{
    if (cwc == NULL)
    {
        ITextVectorProperty *tvp = getText("serial");

        if (!tvp)
        {
            return false;
        }

        //  IDLog("%s\n", tvp->tp[0].text);

        cwc = new CloudWatcherController(tvp->tp[0].text, false);

        int check = cwc->checkCloudWatcher();

        if (check)
        {
            IDMessage(getDefaultName(), "Connected to AAG Cloud Watcher\n");

            sendConstants();

            IEAddTimer(1., ISPoll, this); // Create a timer to send parameters
        }
        else
        {
            IDMessage(getDefaultName(), "Could not connect to AAG Cloud Watcher. Check port and / or cable.\n");

            delete cwc;

            cwc = NULL;

            return false;
        }
    }

    return true;
}

bool AAGCloudWatcher::Disconnect()
{
    if (cwc != NULL)
    {
        resetData();
        resetConstants();

        cwc->setPWMDutyCycle(1);

        delete cwc;

        cwc = NULL;

        IDMessage(getDefaultName(), "Disconnected from AAG Cloud Watcher\n");
    }
    return true;
}

const char *AAGCloudWatcher::getDefaultName()
{
    return "AAG Cloud Watcher";
}

void ISGetProperties(const char *dev)
{
    cloudWatcher->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    cloudWatcher->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    cloudWatcher->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    cloudWatcher->ISNewNumber(dev, name, values, names, n);
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
    INDI_UNUSED(root);
}
