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

#pragma once

#include "defaultdevice.h"

#include <pthread.h>

class AstrometryDriver : public INDI::DefaultDevice
{
  public:
    enum
    {
        ASTROMETRY_SETTINGS_BINARY,
        ASTROMETRY_SETTINGS_OPTIONS
    };

    enum
    {
        ASTROMETRY_RESULTS_PIXSCALE,
        ASTROMETRY_RESULTS_ORIENTATION,
        ASTROMETRY_RESULTS_RA,
        ASTROMETRY_RESULTS_DE,
        ASTROMETRY_RESULTS_PARITY
    };

    AstrometryDriver();
    ~AstrometryDriver() = default;

    virtual void ISGetProperties(const char *dev);
    virtual bool initProperties();
    virtual bool updateProperties();

    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

    static void *runSolverHelper(void *context);

  protected:
    //  Generic indi device entries
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();

    virtual bool saveConfigItems(FILE *fp);

    // Astrometry

    // Enable/Disable solver
    ISwitch SolverS[2];
    ISwitchVectorProperty SolverSP;

    // Solver Settings
    IText SolverSettingsT[2];
    ITextVectorProperty SolverSettingsTP;

    // Solver Results
    INumber SolverResultN[5];
    INumberVectorProperty SolverResultNP;

    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[1];

    IBLOBVectorProperty SolverDataBP;
    IBLOB SolverDataB[1];

    IBLOB CCDDataB[1];
    IBLOBVectorProperty CCDDataBP;

  private:
    // Run solver thread
    void runSolver();

    /**
     * @brief processBLOB Read blob FITS. Uncompress if necessary, write to temporary file, and run
     * solver against it.
     * @param data raw data FITS buffer
     * @param size size of FITS data
     * @param len size of raw data. If no compression is used then len = size. If compression is used,
     * then len is the compressed buffer size and size is the uncompressed final valid data size.
     * @return True if blob buffer was processed correctly and solver started, false otherwise.
     */
    bool processBLOB(uint8_t *data, uint32_t size, uint32_t len);

    // Thread for listenINDI()
    pthread_t solverThread;
    pthread_mutex_t lock;
};
