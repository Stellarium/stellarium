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

#ifndef MGEN_DEVICE_H
#define MGEN_DEVICE_H

class MGenDevice
{
  protected:
    pthread_mutex_t _lock;
    struct ftdi_context *ftdi;
    bool is_device_connected;
    bool tried_turn_on;
    IOMode mode;
    unsigned short vid, pid;

  public:
    bool lock();
    void unlock();

  public:
    /** \brief Connecting a device identified by VID:PID.
     *
     * This function only connects to the FTDI device.
     *
     * \param vid is the Vendor identifier of the device to connect (0x0403 for instance).
     * \param pid is the Product identifier of the device to connect (0x6001 for instance).
     * \return 0 if successful
     * \return the return code of the last ftdi failure, and disable() is called.
     * \note PID:VID=0:0 is used as default to connect to the first available FTDI device.
     * \note If PID:VID connection is not successful, enumerates and log devices to debug output.
     */
    /** @{ */
    int Connect(unsigned short vid = 0, unsigned short pid = 0);
    bool isConnected() const { return is_device_connected; }
    /** @} */

    /** \brief Enabling the device for use.
     *
     * This function thread-safely marks the device as connected.
     * \warning This function does not run a connection, use Connect() for this purpose.
     */
    void enable();

    /** \brief Disabling the device from use.
     *
     * This function thread-safely marks the device as disconnected and deletes its FTDI context.
     */
    void disable();

  public:
    /** \brief Writing the query field of a command to the device.
     * \return the number of bytes written, or -1 if the command is invalid or device is not accessible.
     * \throw IOError when device communication is malfunctioning.
     */
    int write(IOBuffer const &); //throw(IOError);

    /** \brief Reading the answer part of a command from the device.
     * \return the number of bytes read, or -1 if the command is invalid or device is not accessible.
     * \throw IOError when device communication is malfunctioning.
     */
    int read(IOBuffer &); //throw(IOError);

  public:
    /** \brief Turning the device on.
     *
     * This function uses the FTDI special functions to obtain the same functionality as a long press on ESC to
     * turn the M-Gen device on.
     *
     * \note Switches operational mode to UNKNOWN if successful.
     */
    int TurnPowerOn();

  public:
    /** \brief Returning the current operational mode the device is in.
     * \return the IOMode the device was set to by setOpMode().
     */
    IOMode getOpMode() const { return mode; }

    /** \brief Setting the current operational mode of the device.
     *
     * This function thread-safely updates the operational mode, and switch the device line to the adequate baudrate.
     *
     * \return 0 if successful, else a FTDI failure code.
     */
    int setOpMode(IOMode);

    /** \internal Debug helper to stringify the operational mode.
     */
    static char const * DBG_OpModeString(IOMode);

  public:
    MGenDevice();
    virtual ~MGenDevice();
};

#endif
