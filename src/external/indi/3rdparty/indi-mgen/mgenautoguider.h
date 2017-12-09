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

#ifndef MGENAUTOGUIDER_H
#define MGENAUTOGUIDER_H

/** \file mgenautoguider.h
    \brief This is a very simple INDI device for the Lacerta MGen Autoguider.
    \author TallFurryMan (eric.dejouhanet@gmail.com)

    Summary: this driver attempts to connect to a FTDI device with the VID:PID
    of the MGen Autoguider on the USB bus. It tries to understand in which mode
    the device is currently, eventually powers it on and switches it to
    application mode in order to use its features. It then provides a view on
    the remote user interface, and enables the end-user to navigate in that
    interface by clicking buttons.

    The Lacerta MGen driver communicates through libftdi1. It would be possible
    to us the standard VCP driver "ftdi_sio" bundled with the Linux kernel
    through a TTY standard device, but some operations such as turning the
    device on or resetting would not be available.

    In its current implementation, the INDI driver derives from INDI::CCD in
    order to forward a view on the remote UI and a way to navigate in that UI.
    However, none of the features of a CCD is implemented.

    The connection algorithm tries to understand in which state the device is,
    and tries to turn it on if it is in low power. The ability to turn the
    device on is is especially useful as the default state of the power supplied
    autoguider is low power. This feature would not possible with the standard
    ftdi_sio driver.

    \see 80-LacertaMgen.rules for an example of udev rules unbinding the MGen
    device from driver ftdi_sio, leaving it free to communicate with libftdi1
    and libusb. The rules detects the serial name "MGS05443" in events from
    subsystems "usb" and "tty" to request the kernel to unbind the device from
    driver "ftdi_sio".

    The PID:VID identifier is harcoded to the default value for the Lacerta
    MGen, although it could be entered as a driver property: 0x403:0x6001.

    To use the Lacerta MGen in Ekos, connect Ekos to an INDI server executing
    the driver. Once connected, Ekos will periodically (default is 0.25fps)
    display the remote user interface in the preview panel of the FITS viewer.
    You can then navigate using the buttons in the tab "Remote UI". To increase
    the responsiveness of the remote UI, move the slider in tab "Remote UI" to
    increase the frame rate value. Warning, a frame rate too high may drop the
    connection (my tests show that it is the FITS viewer which seems to crack
    under pressure, crashing Ekos and KStars). To stop frame updates once you
    don't need them anymore (e.g. when everything is configured and guiding is
    started), set the frame rate to 0. To improve the communication speed, you
    may compress the frames and the expense of computing power on the INDI
    server (this is disabled by default by INDI::CCD, but is recommended).

    \todo Find a better way to display the remote user interface than a preview
    panel from non-functional INDI::CCD :)

    \todo Screen recognition, in order to define navigation scenarii providing
    additional features (star detection, calibration, guider start/stop, PEC
    measurement...)

    \todo A proper client in Ekos providing calibration, guider start/stop and
    frame fetching.

    \todo Support BOOT and APPLICATION mode switch, in order to retrieve
    additional versions and power the device off. No feature updating the
    firmware is planned (too dangerous).

    \todo Support Star Drift measurement function. This provides a measurement
    of the periodic error of the mount.

    \todo Support Random Displacement function. This allows a sequence of guided
    captures to be offset randomly, improving the result of stacking the
    captures. This requires a way to control the shutter of the guided CCD. MGen
    offers control for open-collector shutter systems such as Canon EOS cameras.

    \todo Support External Exposure function.

    \todo Support Filesystem features to read logs and statistics files, and to
    do housework in the available ~2MB.
*/

#include "indidevapi.h"
#include "indiccd.h"

class MGenAutoguider : public INDI::CCD
{
  public:
    MGenAutoguider();
    virtual ~MGenAutoguider();

  public:
    static MGenAutoguider &instance();

  public:
    virtual bool ISNewNumber(char const *dev, char const *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  public:
    /** \brief Returning the current operational mode of the device.
     * \return the current operational mode of the device.
     */
    IOMode getOpMode() const;

  protected:
    /** \internal The device object used to communicate with the MGen.
     */
    class MGenDevice *device;

  protected:
    /** \internal Version information retrieved from the device.
     */
    struct version
    {
        struct timespec timestamp; /*!< The last time this structure was read from the device. */
        struct firmware
        {
            IText text;                   /*!< Firmware version as read from the device. */
            ITextVectorProperty property; /*!< Firmware version INDI property. */
        } firmware;
        version(): timestamp({ .tv_sec = 0, .tv_nsec = 0 }) {}
    } version;

  protected:
    struct voltage
    {
        struct timespec timestamp; /*!< The last time this structure was read from the device. */
        struct numbers
        {
            INumber
                logic; /*!< Logic supply voltage, [4.8V,5.1V] with DC power, ~4.7V with USB power, stable over 4.5V. */
            INumber input;     /*!< DC input voltage, must be in [9V,15V] (max DC input 20V). */
            INumber reference; /*!< Reference voltage, ~1.23V (more than 10% difference shows internal problem). */
        } levels;
        INumberVectorProperty property; /*!< Voltage INDI property. */
        voltage(): timestamp({ .tv_sec = 0, .tv_nsec = 0 }) {}
    } voltage;

  protected:
    struct ui
    {
        int timer;                 /*!< The timer counting for the refresh event updating the remote user interface. */
        bool is_enabled;           /*!< Whether the remote UI is being transferred to the client. */
        struct timespec timestamp; /*!< The last time this structure was read from the device. */
        struct remote
        {
            ISwitch switches[2]; /*!< Remote UI enable/disable. */
            ISwitchVectorProperty property; /* Remote UI INDI property. */
        } remote;
        struct framerate
        {
            INumber number; /*!< Frame rate value, in frames per second - more frames increase risk of disconnection. */
            INumberVectorProperty property; /* Frame rate INDI property. */
        } framerate;
        struct buttons
        {
            ISwitch switches[6];                 /*!< Button switches for ESC, SET, UP, LEFT, RIGHT and DOWN. */
            ISwitchVectorProperty properties[2]; /*!< Button INDI properties, {ESC,SET} and {UP,LEFT,RIGHT,DOWN}. */
        } buttons;
        ui(): timer(0), is_enabled(false), timestamp({ .tv_sec = 0, .tv_nsec = 0 }) {}
    } ui;

  protected:
    struct heartbeat
    {
        struct timespec timestamp; /*!< The last time this structure was read from the device. */
        unsigned int no_ack_count; /*!< Number of times device didn't acknowledge NOP1, used as connection keepalive. */
        heartbeat(): timestamp({ .tv_sec = 0, .tv_nsec = 0 }), no_ack_count(0) {}
    } heartbeat;

  protected:
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void TimerHit();

  protected:
    virtual bool Connect();
    virtual bool Disconnect();

  protected:
    virtual const char *getDefaultName();

  protected:
    /** \internal Sending a NOP1 to the device to check it is still acknowledging.
     * This function will be called periodically to check if the device is still alive.
     * \return true if command was acknoldeged.
     * \return false if command was not acknowledged, and disconnect the device after 5 failures.
     */
    bool getHeartbeat();
};

#endif // MGENAUTOGUIDER_H
