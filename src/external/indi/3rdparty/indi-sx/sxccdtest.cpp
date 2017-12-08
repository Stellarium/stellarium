/*
 Starlight Xpress CCD INDI Driver

 Copyright (c) 2012-2013 Cloudmakers, s. r. o.
 All Rights Reserved.

 Code is based on SX INDI Driver by Gerry Rozema and Jasem Mutlaq
 Copyright(c) 2010 Gerry Rozema.
 Copyright(c) 2012 Jasem Mutlaq.
 All rights reserved.

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the Free
 Software Foundation; either version 2 of the License, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59
 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 The full GNU General Public License is included in this distribution in the
 file called LICENSE.
 */

#include "sxccdusb.h"

#include "sxconfig.h"

#include <iostream>
#include <memory.h>
#include <unistd.h>

int n;
DEVICE devices[20];
const char *names[20];

HANDLE handles[20];
struct t_sxccd_params params;
unsigned short pixels[10 * 10];

int main()
{
    int i = 0;
    unsigned short us = 0;

    sxDebug(true);

    std::cout << "sx_ccd_test version " << VERSION_MAJOR << "." << VERSION_MINOR << std::endl << std::endl;
    n = sxList(devices, names, 20);
    std::cout << "sxList() -> " << n << std::endl << std::endl;

    for (int j = 0; j < n; j++)
    {
        HANDLE handle;

        std::cout << "testing " << names[j] << " -----------------------------------" << std::endl << std::endl;

        i = sxOpen(devices[j], &handle);
        std::cout << "sxOpen() -> " << i << std::endl << std::endl;

        //i = sxReset(handle);
        //std::cout << "sxReset() -> " << i << std::endl << std::endl;

        us = sxGetCameraModel(handle);
        std::cout << "sxGetCameraModel() -> " << us << std::endl << std::endl;

        //ul = sxGetFirmwareVersion(handle);
        //std::cout << "sxGetFirmwareVersion() -> " << ul << std::endl << std::endl;

        //us = sxGetBuildNumber(handle);
        //std::cout << "sxGetBuildNumber() -> " << us << std::endl << std::endl;

        memset(&params, 0, sizeof(params));
        i = sxGetCameraParams(handle, 0, &params);
        std::cout << "sxGetCameraParams(..., 0,...) -> " << i << std::endl << std::endl;

        i = sxSetTimer(handle, 900);
        std::cout << "sxSetTimer(900) -> " << i << std::endl << std::endl;

        while ((i = sxGetTimer(handle)) > 0)
        {
            std::cout << "sxGetTimer() -> " << i << std::endl << std::endl;
            sleep(1);
        }
        std::cout << "sxGetTimer() -> " << i << std::endl << std::endl;

        if (params.extra_caps & SXUSB_CAPS_SHUTTER)
        {
            i = sxSetShutter(handle, 0);
            std::cout << "sxSetShutter(0) -> " << i << std::endl << std::endl;
            sleep(1);
            i = sxSetShutter(handle, 1);
            std::cout << "sxSetShutter(1) -> " << i << std::endl << std::endl;
        }

        if (params.extra_caps & SXUSB_CAPS_COOLER)
        {
            unsigned short int temp;
            unsigned char status;
            i = sxSetCooler(handle, 1, (-10 + 273) * 10, &status, &temp);
            std::cout << "sxSetCooler() -> " << i << std::endl << std::endl;
        }

        i = sxClearPixels(handle, 0, 0);
        std::cout << "sxClearPixels(..., 0) -> " << i << std::endl << std::endl;

        usleep(1000);

        i = sxLatchPixels(handle, 0, 0, 0, 0, 10, 10, 1, 1);
        std::cout << "sxLatchPixels(..., 0, ...) -> " << i << std::endl << std::endl;

        i = sxReadPixels(handle, pixels, 2 * 10 * 10);
        std::cout << "sxReadPixels() -> " << i << std::endl << std::endl;

        for (int x = 0; x < 10; x++)
        {
            for (int y = 0; y < 10; y++)
                std::cout << pixels[x * 10 + y] << " ";
            std::cout << std::endl;
        }
        std::cout << std::endl;

        if (params.extra_caps & SXCCD_CAPS_GUIDER)
        {
            memset(&params, 0, sizeof(params));
            i = sxGetCameraParams(handle, 1, &params);
            std::cout << "sxGetCameraParams(..., 1,...) -> " << i << std::endl << std::endl;

            i = sxClearPixels(handle, 0, 1);
            std::cout << "sxClearPixels(..., 1) -> " << i << std::endl << std::endl;

            usleep(1000);

            i = sxLatchPixels(handle, 0, 1, 0, 0, 10, 10, 1, 1);
            std::cout << "sxLatchPixels(..., 1, ...) -> " << i << std::endl << std::endl;

            i = sxReadPixels(handle, pixels, 10 * 10);
            std::cout << "sxReadPixels() -> " << i << std::endl << std::endl;

            for (int x = 0; x < 10; x++)
            {
                for (int y = 0; y < 10; y++)
                    std::cout << pixels[x * 10 + y] << " ";
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        sxClose(&handle);
        std::cout << "sxClose() " << std::endl << std::endl;
    }
}
