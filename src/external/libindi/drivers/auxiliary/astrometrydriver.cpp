/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI Astrometry.net Driver

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

#include "astrometrydriver.h"

#include <memory>

#include <cerrno>
#include <cstring>

#include <zlib.h>

// We declare an auto pointer to AstrometryDriver.
std::unique_ptr<AstrometryDriver> astrometry(new AstrometryDriver());

#define POLLMS 1000

void ISGetProperties(const char *dev)
{
    astrometry->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    astrometry->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    astrometry->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    astrometry->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    astrometry->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    astrometry->ISSnoopDevice(root);
}

AstrometryDriver::AstrometryDriver()
{
    setVersion(1, 0);
}

bool AstrometryDriver::initProperties()
{
    INDI::DefaultDevice::initProperties();

    /**********************************************/
    /**************** Astrometry ******************/
    /**********************************************/

    // Solver Enable/Disable
    IUFillSwitch(&SolverS[0], "ASTROMETRY_SOLVER_ENABLE", "Enable", ISS_OFF);
    IUFillSwitch(&SolverS[1], "ASTROMETRY_SOLVER_DISABLE", "Disable", ISS_ON);
    IUFillSwitchVector(&SolverSP, SolverS, 2, getDeviceName(), "ASTROMETRY_SOLVER", "Solver", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // Solver Settings
    IUFillText(&SolverSettingsT[ASTROMETRY_SETTINGS_BINARY], "ASTROMETRY_SETTINGS_BINARY", "Solver",
               "/usr/bin/solve-field");
    IUFillText(&SolverSettingsT[ASTROMETRY_SETTINGS_OPTIONS], "ASTROMETRY_SETTINGS_OPTIONS", "Options",
               "--no-verify --no-plots --no-fits2fits --resort --downsample 2 -O");
    IUFillTextVector(&SolverSettingsTP, SolverSettingsT, 2, getDeviceName(), "ASTROMETRY_SETTINGS", "Settings",
                     MAIN_CONTROL_TAB, IP_WO, 0, IPS_IDLE);

    // Solver Results
    IUFillNumber(&SolverResultN[ASTROMETRY_RESULTS_PIXSCALE], "ASTROMETRY_RESULTS_PIXSCALE", "Pixscale (arcsec/pixel)",
                 "%g", 0, 10000, 1, 0);
    IUFillNumber(&SolverResultN[ASTROMETRY_RESULTS_ORIENTATION], "ASTROMETRY_RESULTS_ORIENTATION",
                 "Orientation (E of N) °", "%g", -360, 360, 1, 0);
    IUFillNumber(&SolverResultN[ASTROMETRY_RESULTS_RA], "ASTROMETRY_RESULTS_RA", "RA (J2000)", "%g", 0, 24, 1, 0);
    IUFillNumber(&SolverResultN[ASTROMETRY_RESULTS_DE], "ASTROMETRY_RESULTS_DE", "DE (J2000)", "%g", -90, 90, 1, 0);
    IUFillNumber(&SolverResultN[ASTROMETRY_RESULTS_PARITY], "ASTROMETRY_RESULTS_PARITY", "Parity", "%g", -1, 1, 1, 0);
    IUFillNumberVector(&SolverResultNP, SolverResultN, 5, getDeviceName(), "ASTROMETRY_RESULTS", "Results",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Solver Data Blob
    IUFillBLOB(&SolverDataB[0], "ASTROMETRY_DATA_BLOB", "Image", "");
    IUFillBLOBVector(&SolverDataBP, SolverDataB, 1, getDeviceName(), "ASTROMETRY_DATA", "Upload", MAIN_CONTROL_TAB,
                     IP_WO, 60, IPS_IDLE);

    /**********************************************/
    /**************** Snooping ********************/
    /**********************************************/

    // Snooped Devices
    IUFillText(&ActiveDeviceT[0], "ACTIVE_CCD", "CCD", "CCD Simulator");
    IUFillTextVector(&ActiveDeviceTP, ActiveDeviceT, 1, getDeviceName(), "ACTIVE_DEVICES", "Snoop devices", OPTIONS_TAB,
                     IP_RW, 60, IPS_IDLE);

    // Primary CCD Chip Data Blob
    IUFillBLOB(&CCDDataB[0], "CCD1", "Image", "");
    IUFillBLOBVector(&CCDDataBP, CCDDataB, 1, ActiveDeviceT[0].text, "CCD1", "Image Data", "Image Info", IP_RO, 60,
                     IPS_IDLE);

    IDSnoopDevice(ActiveDeviceT[0].text, "CCD1");
    IDSnoopBLOBs(ActiveDeviceT[0].text, "CCD1", B_ONLY);

    addDebugControl();

    return true;
}

void AstrometryDriver::ISGetProperties(const char *dev)
{
    DefaultDevice::ISGetProperties(dev);

    defineText(&ActiveDeviceTP);
    loadConfig(true, "ACTIVE_DEVICES");
}

bool AstrometryDriver::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineSwitch(&SolverSP);
        defineText(&SolverSettingsTP);
        defineBLOB(&SolverDataBP);
    }
    else
    {
        if (SolverS[0].s == ISS_ON)
        {
            deleteProperty(SolverResultNP.name);
        }
        deleteProperty(SolverSP.name);
        deleteProperty(SolverSettingsTP.name);
        deleteProperty(SolverDataBP.name);
    }

    return true;
}

const char *AstrometryDriver::getDefaultName()
{
    return (const char *)"Astrometry";
}

bool AstrometryDriver::Connect()
{
    return true;
}

bool AstrometryDriver::Disconnect()
{
    return true;
}

bool AstrometryDriver::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool AstrometryDriver::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                                 char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, SolverDataBP.name) == 0)
        {
            SolverDataBP.s = IPS_OK;
            IDSetBLOB(&SolverDataBP, nullptr);

            // If the client explicitly uploaded the data then we solve it.
            if (SolverS[0].s == ISS_OFF)
            {
                SolverS[0].s = ISS_ON;
                SolverS[1].s = ISS_OFF;
                SolverSP.s   = IPS_BUSY;
                DEBUG(INDI::Logger::DBG_SESSION, "Astrometry solver is enabled.");
                defineNumber(&SolverResultNP);
            }

            processBLOB(reinterpret_cast<uint8_t *>(blobs[0]), static_cast<uint32_t>(sizes[0]),
                        static_cast<uint32_t>(blobsizes[0]));

            return true;
        }
    }

    return INDI::DefaultDevice::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool AstrometryDriver::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        if (strcmp(name, ActiveDeviceTP.name) == 0)
        {
            ActiveDeviceTP.s = IPS_OK;
            IUUpdateText(&ActiveDeviceTP, texts, names, n);
            IDSetText(&ActiveDeviceTP, nullptr);

            // Update the property name!
            strncpy(CCDDataBP.device, ActiveDeviceT[0].text, MAXINDIDEVICE);
            IDSnoopDevice(ActiveDeviceT[0].text, "CCD1");
            IDSnoopBLOBs(ActiveDeviceT[0].text, "CCD1", B_ONLY);

            //  We processed this one, so, tell the world we did it
            return true;
        }

        if (strcmp(name, SolverSettingsTP.name) == 0)
        {
            IUUpdateText(&SolverSettingsTP, texts, names, n);
            SolverSettingsTP.s = IPS_OK;
            IDSetText(&SolverSettingsTP, nullptr);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool AstrometryDriver::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Astrometry Enable/Disable
        if (strcmp(name, SolverSP.name) == 0)
        {
            pthread_mutex_lock(&lock);

            IUUpdateSwitch(&SolverSP, states, names, n);
            SolverSP.s = IPS_OK;

            if (SolverS[0].s == ISS_ON)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Astrometry solver is enabled.");
                defineNumber(&SolverResultNP);
            }
            else
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Astrometry solver is disabled.");
                deleteProperty(SolverResultNP.name);
            }

            IDSetSwitch(&SolverSP, nullptr);

            pthread_mutex_unlock(&lock);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool AstrometryDriver::ISSnoopDevice(XMLEle *root)
{
    if (SolverS[0].s == ISS_ON && IUSnoopBLOB(root, &CCDDataBP) == 0)
    {
        processBLOB(reinterpret_cast<uint8_t *>(CCDDataB[0].blob), static_cast<uint32_t>(CCDDataB[0].size),
                    static_cast<uint32_t>(CCDDataB[0].bloblen));
        return true;
    }

    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool AstrometryDriver::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &ActiveDeviceTP);
    IUSaveConfigText(fp, &SolverSettingsTP);
    return true;
}

bool AstrometryDriver::processBLOB(uint8_t *data, uint32_t size, uint32_t len)
{
    FILE *fp = nullptr;
    char imageFileName[MAXRBUF];

    uint8_t *processedData = data;

    // If size != len then we have compressed buffer
    if (size != len)
    {
        uint8_t *dataBuffer = new uint8_t[size];
        uLongf destLen      = size;

        if (dataBuffer == nullptr)
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Unable to allocate memory for data buffer");
            return false;
        }

        int r = uncompress(dataBuffer, &destLen, data, len);
        if (r != Z_OK)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Astrometry compression error: %d", r);
            delete[] dataBuffer;
            return false;
        }

        if (destLen != size)
        {
            DEBUGF(INDI::Logger::DBG_WARNING, "Discrepency between uncompressed data size %ld and expected size %ld",
                   size, destLen);
        }

        processedData = dataBuffer;
    }

    strncpy(imageFileName, "/tmp/ccdsolver.fits", MAXRBUF);

    fp = fopen(imageFileName, "w");
    if (fp == nullptr)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unable to save image file (%s). %s", imageFileName, strerror(errno));
        if (size != len)
            delete[] processedData;

        return false;
    }

    int n = 0;
    for (uint32_t nr = 0; nr < size; nr += n)
        n = fwrite(processedData + nr, 1, size - nr, fp);

    fclose(fp);

    // Do not forget to release uncompressed buffer
    if (size != len)
        delete[] processedData;

    pthread_mutex_lock(&lock);
    SolverSP.s = IPS_BUSY;
    DEBUG(INDI::Logger::DBG_SESSION, "Solving image...");
    IDSetSwitch(&SolverSP, nullptr);
    pthread_mutex_unlock(&lock);

    int result = pthread_create(&solverThread, nullptr, &AstrometryDriver::runSolverHelper, this);

    if (result != 0)
    {
        SolverSP.s = IPS_ALERT;
        DEBUGF(INDI::Logger::DBG_SESSION, "Failed to create solver thread: %s", strerror(errno));
        IDSetSwitch(&SolverSP, nullptr);
    }

    return true;
}

void *AstrometryDriver::runSolverHelper(void *context)
{
    (static_cast<AstrometryDriver *>(context))->runSolver();
    return nullptr;
}

void AstrometryDriver::runSolver()
{
    char cmd[MAXRBUF]={0}, line[256]={0}, parity_str[8]={0};
    float ra = -1000, dec = -1000, angle = -1000, pixscale = -1000, parity = 0;
    snprintf(cmd, MAXRBUF, "%s %s -W /tmp/solution.wcs /tmp/ccdsolver.fits",
             SolverSettingsT[ASTROMETRY_SETTINGS_BINARY].text, SolverSettingsT[ASTROMETRY_SETTINGS_OPTIONS].text);

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s", cmd);
    FILE *handle = popen(cmd, "r");
    if (handle == nullptr)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Failed to run solver: %s", strerror(errno));
        pthread_mutex_lock(&lock);
        SolverSP.s = IPS_ALERT;
        IDSetSwitch(&SolverSP, nullptr);
        pthread_mutex_unlock(&lock);
        return;
    }

    while (fgets(line, sizeof(line), handle) != nullptr)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s", line);

        sscanf(line, "Field rotation angle: up is %f", &angle);
        sscanf(line, "Field center: (RA,Dec) = (%f,%f)", &ra, &dec);
        sscanf(line, "Field parity: %s", parity_str);
        sscanf(line, "%*[^p]pixel scale %f", &pixscale);

        if (strcmp(parity_str, "pos") == 0)
            parity = 1;
        else if (strcmp(parity_str, "neg") == 0)
            parity = -1;

        if (ra != -1000 && dec != -1000 && angle != -1000 && pixscale != -1000)
        {
            // Pixscale is arcsec/pixel. Astrometry result is in arcmin
            SolverResultN[ASTROMETRY_RESULTS_PIXSCALE].value = pixscale;
            // Astrometry.net angle, E of N
            SolverResultN[ASTROMETRY_RESULTS_ORIENTATION].value = angle;
            // Astrometry.net J2000 RA in degrees
            SolverResultN[ASTROMETRY_RESULTS_RA].value = ra;
            // Astrometry.net J2000 DEC in degrees
            SolverResultN[ASTROMETRY_RESULTS_DE].value = dec;
            // Astrometry.net parity
            SolverResultN[ASTROMETRY_RESULTS_PARITY].value = parity;

            SolverResultNP.s = IPS_OK;
            IDSetNumber(&SolverResultNP, nullptr);

            pthread_mutex_lock(&lock);
            SolverSP.s = IPS_OK;
            IDSetSwitch(&SolverSP, nullptr);
            pthread_mutex_unlock(&lock);

            fclose(handle);
            DEBUG(INDI::Logger::DBG_SESSION, "Solver complete.");
            return;
        }

        pthread_mutex_lock(&lock);
        if (SolverS[1].s == ISS_ON)
        {
            SolverSP.s = IPS_IDLE;
            IDSetSwitch(&SolverSP, nullptr);
            pthread_mutex_unlock(&lock);
            fclose(handle);
            DEBUG(INDI::Logger::DBG_SESSION, "Solver cancelled.");
            return;
        }
        pthread_mutex_unlock(&lock);
    }

    fclose(handle);

    pthread_mutex_lock(&lock);
    SolverSP.s = IPS_ALERT;
    IDSetSwitch(&SolverSP, nullptr);
    DEBUG(INDI::Logger::DBG_SESSION, "Solver failed.");
    pthread_mutex_unlock(&lock);

    pthread_exit(nullptr);
}
