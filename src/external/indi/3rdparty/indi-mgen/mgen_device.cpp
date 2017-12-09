/*
    INDI 3rd party driver
    Lacerta MGen Autoguider INDI driver, implemented with help from
    Tommy (teleskopaustria@gmail.com) and Zoltan (mgen@freemail.hu).

    Teleskop & Mikroskop Zentrum (www.teleskop.austria.com)
    A-1050 WIEN, Schönbrunner Strasse 96
    +43 699 1197 0808 (Shop in Wien und Rechnungsanschrift)
    A-4020 LINZ, Gärtnerstrasse 16
    +43 699 1901 2165 (Shop in Linz)

    Lacerta GmbH
    UmsatzSt. Id. Nr.: AT U67203126
    Firmenbuch Nr.: FN 379484s

    Copyright (C) 2017 by TallFurryMan (eric.dejouhanet@gmail.com)

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
#include <pthread.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#include "indilogger.h"

#include "mgen.h"
#include "mgenautoguider.h"
#include "mgen_device.h"

MGenDevice::MGenDevice()
    : _lock(), ftdi(NULL), is_device_connected(false), tried_turn_on(false), mode(OPM_UNKNOWN), vid(0), pid(0)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

MGenDevice::~MGenDevice()
{
    pthread_mutex_destroy(&_lock);
}

bool MGenDevice::lock()
{
    return !pthread_mutex_lock(&_lock);
}
void MGenDevice::unlock()
{
    pthread_mutex_unlock(&_lock);
}

void MGenDevice::enable()
{
    if (lock())
    {
        is_device_connected = true;
        unlock();
    }
}

void MGenDevice::disable()
{
    if (lock())
    {
        is_device_connected = false;
        if (ftdi)
        {
            ftdi_usb_close(ftdi);
            ftdi_free(ftdi);
        }
        unlock();
    }
}

int MGenDevice::Connect(unsigned short vid, unsigned short pid)
{
    int res = 0;

    if (lock())
    {
        enable();

        struct ftdi_version_info ftdi_version = ftdi_get_library_version();
        _S("Connecting device %04X:%04X using FTDI %s v%d.%d.%d snapshot %s", vid, pid, ftdi_version.version_str,
           ftdi_version.major, ftdi_version.minor, ftdi_version.micro, ftdi_version.snapshot_str);

        /* Cleanup in case we try to reconnect after turning on */
        if (ftdi)
        {
            ftdi_usb_close(ftdi);
            ftdi_free(ftdi);
        }

        ftdi = ftdi_new();

        if (!ftdi)
        {
            _E("FTDI context initialization failed", "");
        }
        else if ((res = ftdi_set_interface(ftdi, INTERFACE_ANY)) < 0)
        {
            _E("failed setting FTDI interface to ANY (%d: %s)", res, ftdi_get_error_string(ftdi));
        }
        else if ((res = ftdi_usb_open(ftdi, vid, pid)) < 0)
        {
            _E("device 0x%04X:0x%04X not found (%d: %s)", vid, pid, res, ftdi_get_error_string(ftdi));

            if ((res = ftdi_set_interface(ftdi, INTERFACE_ANY)) < 0)
            {
                _E("failed setting FTDI interface to ANY (%d: %s)", res, ftdi_get_error_string(ftdi));
            }
            else
            {
                struct ftdi_device_list *devlist;
                if ((res = ftdi_usb_find_all(ftdi, &devlist, 0, 0)) < 0)
                {
                    _E("no FTDI device found (%d: %s)", res, ftdi_get_error_string(ftdi));
                }
                else
                {
                    if (devlist)
                        for (struct ftdi_device_list const *dev_index = devlist; dev_index; dev_index = dev_index->next)
                        {
                            struct libusb_device_descriptor desc;
                            memset(&desc,0,sizeof(desc));

                            if (libusb_get_device_descriptor(dev_index->dev, &desc) < 0)
                            {
                                _S("device %p returned by libftdi is unreadable", dev_index->dev);
                                continue;
                            }

                            _S("detected FTDI device 0x%04X:0x%04X", desc.idVendor, desc.idProduct);
                        }
                    else
                        _S("no FTDI device enumerated", "");

                    ftdi_list_free(&devlist);
                }
            }
        }
        else if (setOpMode(OPM_UNKNOWN) < 0)
        {
            /* TODO: Not good, the device doesn't support our settings - out of spec, bail out */
            _E("failed configuring device serial line", "");
        }
        else
        {
            this->vid = vid;
            this->pid = pid;
            _S("FTDI device 0x%04X:0x%04X connected successfully", vid, pid);
            unlock();
            return 0;
        }

        disable();
        unlock();
    }

    return res;
}

int MGenDevice::TurnPowerOn()
{
    /* Don't try to turn on twice - caller need to delete us before retrying */
    if (tried_turn_on)
        return 1;

    /* Perhaps the device is not turned on, so try to press ESC for a short time */
    _S("trying to turn device on", "");

    unsigned char cbus_dir = 1 << 1, cbus_val = 1 << 1; /* Spec uses unitialized variables here */
    int res = 0;

    if ((res = ftdi_set_bitmode(ftdi, (cbus_dir << 4) + cbus_val, 0x20)) < 0)
    {
        _E("failed depressing ESC to turn device on", "");
        disable();
        return res;
    }

    usleep(250000);
    cbus_val &= ~(1 << 1);

    if ((res = ftdi_set_bitmode(ftdi, (cbus_dir << 4) + cbus_val, 0x20)) < 0)
    {
        _E("failed releasing ESC to turn device on", "");
        disable();
        return res;
    }

    /* Wait for the device to turn on */
    sleep(5);

    _D("turned device on, retrying connection", "");
    tried_turn_on = true;
    return Connect(vid, pid);
}

int MGenDevice::setOpMode(IOMode const _mode)
{
    int baudrate = 0;

    switch (_mode)
    {
        case OPM_COMPATIBLE:
            baudrate = 9600;
            break;

        case OPM_APPLICATION:
            baudrate = 250000;
            break;

        case OPM_UNKNOWN:
        default:
            baudrate = 9600;
            break;
    }

    _D("switching device to baudrate %d for mode %s", baudrate, DBG_OpModeString(_mode));

    int res = 0;

    if (lock())
    {
        if ((res = ftdi_set_baudrate(ftdi, baudrate)) < 0)
        {
            /* TODO: Not good, the device doesn't support our settings - out of spec, bail out */
            _E("failed updating device connection using %d bauds (%d: %s)", baudrate, res, ftdi_get_error_string(ftdi));
        }
        else if ((res = ftdi_set_line_property(ftdi, BITS_8, STOP_BIT_1, NONE)) < 0)
        {
            /* TODO: Not good, the device doesn't support our settings - out of spec, bail out */
            _E("failed setting device line properties to 8-N-1 (%d: %s)", res, ftdi_get_error_string(ftdi));
        }
        /* Purge I/O buffers */
        else if ((res = ftdi_usb_purge_buffers(ftdi)) < 0)
        {
            _E("failed purging I/O buffers (%d: %s)", res, ftdi_get_error_string(ftdi));
        }
        /* Set latency to minimal 2ms */
        else if ((res = ftdi_set_latency_timer(ftdi, 2)) < 0)
        {
            _E("failed setting latency timer (%d: %s)", res, ftdi_get_error_string(ftdi));
        }
        else
        {
            _D("successfully switched device to baudrate %d.", baudrate);
            mode = _mode;
            unlock();
            return 0;
        }

        unlock();
    }

    return res;
}

int MGenDevice::write(IOBuffer const &query) //throw(IOError)
{
    if (!ftdi)
        return -1;

    _D("writing %d bytes to device: %02X %02X %02X %02X %02X ...", query.size(), query.size() > 0 ? query[0] : 0,
       query.size() > 1 ? query[1] : 0, query.size() > 2 ? query[2] : 0, query.size() > 3 ? query[3] : 0,
       query.size() > 4 ? query[4] : 0);
    int const bytes_written = ftdi_write_data(ftdi, query.data(), query.size());

    /* FIXME: Adjust this to optimize how long device absorb the command for */
    usleep(20000);

    if (bytes_written < 0)
        throw IOError(bytes_written);

    return bytes_written;
}

int MGenDevice::read(IOBuffer &answer) //throw(IOError)
{
    if (!ftdi)
        return -1;

    if (answer.size() > 0)
    {
        _D("reading %d bytes from device", answer.size());
        int const bytes_read = ftdi_read_data(ftdi, answer.data(), answer.size());

        if (bytes_read < 0)
            throw IOError(bytes_read);

        _D("read %d bytes from device: %02X %02X %02X %02X %02X ...", bytes_read, answer.size() > 0 ? answer[0] : 0,
           answer.size() > 1 ? answer[1] : 0, answer.size() > 2 ? answer[2] : 0, answer.size() > 3 ? answer[3] : 0,
           answer.size() > 4 ? answer[4] : 0);
        return bytes_read;
    }

    return 0;
}

char const * MGenDevice::DBG_OpModeString(IOMode mode)
{
    switch (mode)
    {
        case OPM_UNKNOWN:
            return "UNKNOWN";
        case OPM_COMPATIBLE:
            return "COMPATIBLE";
        case OPM_APPLICATION:
            return "APPLICATION";
        default:
            return "???";
    }
}
