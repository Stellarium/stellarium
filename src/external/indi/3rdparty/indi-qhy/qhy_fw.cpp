/*
 QHY FW loader
 
 Copyright (C) 2015 Peter Polakovic (peter.polakovic@cloudmakers.eu)
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef OSX_EMBEDED_MODE
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "qhy_fw.h"

static struct uninitialized_cameras
{
    int vid;
    int pid;
    const char *loader;
    const char *firmware;
} uninitialized_cameras[] = { { 0x1618, 0xb618, NULL, "IMG0H.HEX" },
                              { 0x1618, 0x0412, NULL, "QHY2.HEX" },
                              { 0x1618, 0x2850, NULL, "QHY2P.HEX" },
                              { 0x1618, 0xb285, NULL, "QHY2S.HEX" },
                              { 0x1618, 0x0901, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x1618, 0x1002, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x0547, 0x1002, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x296a, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x04b4, 0x8613, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x0818, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x081a, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x296e, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x296c, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x16c0, 0x2986, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x1781, 0x0c7c, "QHY5LOADER.HEX", "QHY5.HEX" },
                              { 0x1618, 0x0920, NULL, "QHY5II.HEX" },
                              { 0x1618, 0x0921, NULL, "QHY5II.HEX" },
                              { 0x1618, 0x0259, NULL, "QHY6.HEX" },
                              { 0x1618, 0x4022, NULL, "QHY7.HEX" },
                              { 0x1618, 0x6000, NULL, "QHY8.HEX" },
                              { 0x1618, 0x6002, NULL, "QHY8PRO.HEX" },
                              { 0x1618, 0x6004, NULL, "QHY8L.HEX" },
                              { 0x1618, 0x6006, NULL, "QHY8M.HEX" },
                              { 0x1618, 0x8300, NULL, "QHY9S.HEX" },
                              { 0x1618, 0x1000, NULL, "QHY10.HEX" },
                              { 0x1618, 0x1110, NULL, "QHY11.HEX" },
                              { 0x1618, 0x1200, NULL, "QHY12.HEX" },
                              { 0x1618, 0x1500, NULL, "QHY15.HEX" },
                              { 0x1618, 0x1600, NULL, "QHY16.HEX" },
                              { 0x1618, 0x8050, NULL, "QHY20.HEX" },
                              { 0x1618, 0x6740, NULL, "QHY21.HEX" },
                              { 0x1618, 0x6940, NULL, "QHY22.HEX" },
                              { 0x1618, 0x8140, NULL, "QHY23.HEX" },
                              { 0x1618, 0x1650, NULL, "QHY27.HEX" },
                              { 0x1618, 0x1670, NULL, "QHY28.HEX" },
                              { 0x1618, 0x2950, NULL, "QHY29.HEX" },
                              { 0x1618, 0x8310, NULL, "IC8300.HEX" },
                              { 0x1618, 0x1620, NULL, "IC16200A.HEX" },
                              { 0x1618, 0x1630, NULL, "IC16803.HEX" },
                              { 0x1618, 0x8320, NULL, "IC90A.HEX" },
                              { 0x1618, 0x0930, NULL, "miniCam5.HEX" },
                              { 0, 0, NULL, NULL } };

static int poke(libusb_device_handle *handle, unsigned short addr, unsigned char *data, unsigned length)
{
    int rc;
    unsigned retry = 0;
    while ((rc = libusb_control_transfer(handle,
                                         LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                         0xA0, addr, 0, data, length, 3000)) < 0 &&
           retry < 5)
    {
        if (errno != ETIMEDOUT)
            break;
        retry += 1;
    }
    return rc;
}

static int upload(libusb_device_handle *handle, const char *hex)
{
    int rc;
    unsigned char stop  = 1;
    unsigned char reset = 0;
    FILE *image;
    char path[FILENAME_MAX];
#ifdef OSX_EMBEDED_MODE
    sprintf(path, "%s/Contents/Resources/%s", getenv("INDIPREFIX"), hex);
#else
    sprintf(path, "/usr/local/firmware/%s", hex);
#endif
    image = fopen(path, "r");
    if (image)
    {
        rc = libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     0xA0, 0xe600, 0, &stop, 1, 3000);
        if (rc == 1)
        {
            unsigned char data[1023];
            unsigned short data_addr = 0;
            unsigned data_len        = 0;
            int first_line           = 1;
            for (;;)
            {
                char buf[512], *cp;
                char tmp, type;
                unsigned len;
                unsigned idx, off;
                cp = fgets(buf, sizeof buf, image);
                if (cp == 0)
                {
                    fprintf(stderr, "EOF without EOF record in %s\n", hex);
                    break;
                }
                if (buf[0] == '#')
                    continue;
                if (buf[0] != ':')
                {
                    fprintf(stderr, "Invalid ihex record in %s\n", hex);
                    break;
                }
                cp = strchr(buf, '\n');
                if (cp)
                    *cp = 0;
                tmp    = buf[3];
                buf[3] = 0;
                len    = strtoul(buf + 1, 0, 16);
                buf[3] = tmp;
                tmp    = buf[7];
                buf[7] = 0;
                off    = strtoul(buf + 3, 0, 16);
                buf[7] = tmp;
                if (first_line)
                {
                    data_addr  = off;
                    first_line = 0;
                }
                tmp    = buf[9];
                buf[9] = 0;
                type   = strtoul(buf + 7, 0, 16);
                buf[9] = tmp;
                if (type == 1)
                {
                    break;
                }
                if (type != 0)
                {
                    fprintf(stderr, "Unsupported record type %u in %s\n", type, hex);
                    break;
                }
                if ((len * 2) + 11 > strlen(buf))
                {
                    fprintf(stderr, "Record too short in %s\n", hex);
                    break;
                }
                if (data_len != 0 && (off != (data_addr + data_len) || (data_len + len) > sizeof data))
                {
                    rc = poke(handle, data_addr, data, data_len);
                    if (rc < 0)
                        break;
                    data_addr = off;
                    data_len  = 0;
                }
                for (idx = 0, cp = buf + 9; idx < len; idx += 1, cp += 2)
                {
                    tmp                  = cp[2];
                    cp[2]                = 0;
                    data[data_len + idx] = strtoul(cp, 0, 16);
                    cp[2]                = tmp;
                }
                data_len += len;
            }
            if (rc >= 0 && data_len != 0)
            {
                poke(handle, data_addr, data, data_len);
            }
            rc = libusb_control_transfer(handle,
                                         LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                         0xA0, 0xe600, 0, &reset, 1, 3000);
        }
        fclose(image);
    }
    else
    {
        fprintf(stderr, "Can't open %s\n", hex);
        return 0;
    }
    return rc >= 0;
}

static bool initialize(libusb_device *device, int index)
{
    fprintf(stderr, "VID: %d, PID: %d\n", uninitialized_cameras[index].vid, uninitialized_cameras[index].pid);
    int rc;
    libusb_device_handle *handle;
    rc = libusb_open(device, &handle);
    if (rc >= 0)
    {
        if (libusb_kernel_driver_active(handle, 0) == 1)
        {
            rc = libusb_detach_kernel_driver(handle, 0);
        }
        if (rc >= 0)
        {
            rc = libusb_claim_interface(handle, 0);
        }
        if (rc >= 0)
        {
            if (uninitialized_cameras[index].loader)
            {
                if (!upload(handle, uninitialized_cameras[index].loader))
                {
                    fprintf(stderr, "Can't upload loader\n");
                    return 0;
                }
                usleep(1 * 1000 * 1000);
            }
            upload(handle, uninitialized_cameras[index].firmware);
            rc = libusb_release_interface(handle, 0);
            libusb_close(handle);
        }
    }
    return rc >= 0;
}

void UploadFW()
{
    libusb_context *ctx = NULL;
    int rc              = libusb_init(&ctx);
    if (rc < 0)
    {
        fprintf(stderr, "Can't initialize libusb\n");
    }
    libusb_device **usb_devices;
    ssize_t total = libusb_get_device_list(ctx, &usb_devices);
    if (total < 0)
    {
        fprintf(stderr, "Can't get device list\n");
    }
    int count = 0;
    struct libusb_device_descriptor descriptor;
    for (int i = 0; i < total; i++)
    {
        libusb_device *device = usb_devices[i];
        for (int j = 0; uninitialized_cameras[j].vid; j++)
        {
            if (!libusb_get_device_descriptor(device, &descriptor) &&
                descriptor.idVendor == uninitialized_cameras[j].vid &&
                descriptor.idProduct == uninitialized_cameras[j].pid)
            {
                initialize(device, j);
            }
        }
    }
    usleep(5 * 1000 * 1000);
}
