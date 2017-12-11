/*******************************************************************************
 Copyright(c) 2011 Gerry Rozema. All rights reserved.

 Upgrade to libusb 1.0 by CloudMakers, s. r. o.
 Copyright(c) 2013 CloudMakers, s. r. o. All rights reserved.

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

#include "indiusbdevice.h"

#include <config-usb.h>

#ifndef USB1_HAS_LIBUSB_ERROR_NAME
const char *LIBUSB_CALL libusb_error_name(int errcode)
{
    static char buffer[30];
    sprintf(buffer, "error %d", errcode);
    return buffer;
}
#endif

static libusb_context *ctx = nullptr;

namespace INDI
{

USBDevice::USBDevice()
{
    dev            = nullptr;
    usb_handle     = nullptr;
    OutputEndpoint = 0;
    InputEndpoint  = 0;

    if (ctx == nullptr)
    {
        int rc = libusb_init(&ctx);
        if (rc < 0)
        {
            fprintf(stderr, "USBDevice: Can't initialize libusb\n");
        }
    }
}

USBDevice::~USBDevice()
{
    libusb_exit(ctx);
}

libusb_device *USBDevice::FindDevice(int vendor, int product, int searchindex)
{
    int index = 0;
    libusb_device **usb_devices;
    struct libusb_device_descriptor descriptor;
    ssize_t total = libusb_get_device_list(ctx, &usb_devices);
    if (total < 0)
    {
        fprintf(stderr, "USBDevice: Can't get device list\n");
        return 0;
    }
    for (int i = 0; i < total; i++)
    {
        libusb_device *device = usb_devices[i];
        if (!libusb_get_device_descriptor(device, &descriptor))
        {
            if (descriptor.idVendor == vendor && descriptor.idProduct == product)
            {
                if (index == searchindex)
                {
                    libusb_ref_device(device);
                    libusb_free_device_list(usb_devices, 1);
                    fprintf(stderr, "Found device %04x/%04x/%d\n", descriptor.idVendor, descriptor.idProduct, index);
                    return device;
                }
                else
                {
                    fprintf(stderr, "Skipping device %04x/%04x/%d\n", descriptor.idVendor, descriptor.idProduct, index);
                    index++;
                }
            }
            else
            {
                fprintf(stderr, "Skipping device %04x/%04x\n", descriptor.idVendor, descriptor.idProduct);
            }
        }
    }
    libusb_free_device_list(usb_devices, 1);
    return nullptr;
}

int USBDevice::Open()
{
    if (dev == nullptr)
        return -1;

    int rc = libusb_open(dev, &usb_handle);

    if (rc >= 0)
    {
        if (libusb_kernel_driver_active(usb_handle, 0) == 1)
        {
            rc = libusb_detach_kernel_driver(usb_handle, 0);
            if (rc < 0)
            {
                fprintf(stderr, "USBDevice: libusb_detach_kernel_driver -> %s\n", libusb_error_name(rc));
            }
        }
        if (rc >= 0)
        {
            rc = libusb_claim_interface(usb_handle, 0);
            if (rc < 0)
            {
                fprintf(stderr, "USBDevice: libusb_claim_interface -> %s\n", libusb_error_name(rc));
            }
        }
        return FindEndpoints();
    }
    return rc;
}

void USBDevice::Close()
{
    libusb_close(usb_handle);
}

int USBDevice::FindEndpoints()
{
    struct libusb_config_descriptor *config;
    struct libusb_interface_descriptor *interface;
    int rc = libusb_get_config_descriptor(dev, 0, &config);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_get_config_descriptor -> %s\n", libusb_error_name(rc));
        return rc;
    }

    interface = (struct libusb_interface_descriptor *)&(config->interface[0].altsetting[0]);
    for (int i = 0; i < interface->bNumEndpoints; i++)
    {
        fprintf(stderr, "Endpoint %04x %04x\n", interface->endpoint[i].bEndpointAddress,
                interface->endpoint[i].bmAttributes);
        int dir = interface->endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK;
        if (dir == LIBUSB_ENDPOINT_IN)
        {
            fprintf(stderr, "Got an input endpoint\n");
            InputEndpoint = interface->endpoint[i].bEndpointAddress;
            InputType     = interface->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;
        }
        else if (dir == LIBUSB_ENDPOINT_OUT)
        {
            fprintf(stderr, "Got an output endpoint\n");
            OutputEndpoint = interface->endpoint[i].bEndpointAddress;
            OutputType     = interface->endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK;
        }
    }
    return 0;
}

int USBDevice::ReadInterrupt(unsigned char *buf, int count, int timeout)
{
    int transferred;
    int rc = libusb_interrupt_transfer(usb_handle, InputEndpoint, buf, count, &transferred, timeout);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_interrupt_transfer -> %s\n", libusb_error_name(rc));
    }
    return rc < 0 ? rc : transferred;
}

int USBDevice::WriteInterrupt(unsigned char *buf, int count, int timeout)
{
    int transferred;
    int rc = libusb_interrupt_transfer(usb_handle, OutputEndpoint, buf, count, &transferred, timeout);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_interrupt_transfer -> %s\n", libusb_error_name(rc));
    }
    return rc < 0 ? rc : transferred;
}

int USBDevice::ReadBulk(unsigned char *buf, int count, int timeout)
{
    int transferred;
    int rc = libusb_bulk_transfer(usb_handle, InputEndpoint, buf, count, &transferred, timeout);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_bulk_transfer -> %s\n", libusb_error_name(rc));
    }
    return rc < 0 ? rc : transferred;
}

int USBDevice::WriteBulk(unsigned char *buf, int count, int timeout)
{
    int transferred;
    int rc = libusb_bulk_transfer(usb_handle, OutputEndpoint, buf, count, &transferred, timeout);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_bulk_transfer -> %s\n", libusb_error_name(rc));
    }
    return rc < 0 ? rc : transferred;
}

int USBDevice::ControlMessage(unsigned char request_type, unsigned char request, unsigned int value,
                                    unsigned int index, unsigned char *data, unsigned char len)
{
    int rc = libusb_control_transfer(usb_handle, request_type, request, value, index, data, len, 5000);
    if (rc < 0)
    {
        fprintf(stderr, "USBDevice: libusb_control_transfer -> %s\n", libusb_error_name(rc));
    }
    return rc;
}

}
