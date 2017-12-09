/*
    Kuwait National Radio Observatory
    INDI Driver for SpectraCyber Hydrogen Line Spectrometer
    Communication: RS232 <---> USB

    Copyright (C) 2009 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

    Change Log:

    Format of BLOB data is:

    ########### ####### ########## ## ###
    Julian_Date Voltage Freqnuency RA DEC

*/

#include "spectracyber.h"

#include "config.h"

#include <indicom.h>

#include <libnova/julian_day.h>

#include <memory>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define mydev         "SpectraCyber"
#define BASIC_GROUP   "Main Control"
#define OPTIONS_GROUP "Options"

#define current_freq FreqNP->np[0].value
#define CONT_CHANNEL 0
#define SPEC_CHANNEL 1

const int POLLMS = 1000;

//const int SPECTROMETER_READ_BUFFER  = 16;
const int SPECTROMETER_ERROR_BUFFER = 128;
const int SPECTROMETER_CMD_LEN      = 5;
const int SPECTROMETER_CMD_REPLY    = 4;

//const double SPECTROMETER_MIN_FREQ  = 46.4;
const double SPECTROMETER_REST_FREQ = 48.6;
//const double SPECTROMETER_MAX_FREQ  = 51.2;
const double SPECTROMETER_RF_FREQ   = 1371.805;

const unsigned int SPECTROMETER_OFFSET = 0x050;

/* 90 Khz Rest Correction */
const double SPECTROMETER_REST_CORRECTION = 0.090;

const char *contFMT = ".ascii_cont";
const char *specFMT = ".ascii_spec";

// We declare an auto pointer to spectrometer.
std::unique_ptr<SpectraCyber> spectracyber(new SpectraCyber());

void ISGetProperties(const char *dev)
{
    spectracyber->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    spectracyber->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    spectracyber->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    spectracyber->ISNewNumber(dev, name, values, names, num);
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
    spectracyber->ISSnoopDevice(root);
}

/****************************************************************
**
**
*****************************************************************/
SpectraCyber::SpectraCyber()
{
    // Command pre-limeter
    command[0] = '!';

    telescopeID = NULL;

    srand(time(NULL));

    buildSkeleton("indi_spectracyber_sk.xml");

    // Optional: Add aux controls for configuration, debug & simulation
    addAuxControls();

    setVersion(SPECTRACYBER_VERSION_MAJOR, SPECTRACYBER_VERSION_MINOR);
}

/****************************************************************
**
**
*****************************************************************/
SpectraCyber::~SpectraCyber()
{
}

/****************************************************************
**
**
*****************************************************************/
void SpectraCyber::ISGetProperties(const char *dev)
{
    static int propInit = 0;

    INDI::DefaultDevice::ISGetProperties(dev);

    if (propInit == 0)
    {
        loadConfig();

        propInit = 1;

        ITextVectorProperty *tProp = getText("ACTIVE_DEVICES");

        if (tProp)
        {
            telescopeID = IUFindText(tProp, "ACTIVE_TELESCOPE");

            if (telescopeID && strlen(telescopeID->text) > 0)
            {
                IDSnoopDevice(telescopeID->text, "EQUATORIAL_EOD_COORD");
            }
        }
    }
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::ISSnoopDevice(XMLEle *root)
{
    if (IUSnoopNumber(root, &EquatorialCoordsRNP) != 0)
    {
        if (isDebug())
            IDLog("Error processing snooped EQUATORIAL_EOD_COORD_REQUEST value! No RA/DEC information available.\n");

        return true;
    }
    //else
    //    IDLog("Received RA: %g - DEC: %g\n", EquatorialCoordsRNP.np[0].value, EquatorialCoordsRNP.np[1].value);

    return false;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::initProperties()
{
    IDLog("init Properties in spectracyber\n");

    INDI::DefaultDevice::initProperties();

    FreqNP = getNumber("Freq (Mhz)");
    if (FreqNP == NULL)
        IDMessage(getDeviceName(), "Error: Frequency property is missing. Spectrometer cannot be operated.");

    ScanNP = getNumber("Scan Parameters");
    if (ScanNP == NULL)
        IDMessage(getDeviceName(), "Error: Scan parameters property is missing. Spectrometer cannot be operated.");

    ChannelSP = getSwitch("Channels");
    if (ChannelSP == NULL)
        IDMessage(getDeviceName(), "Error: Channel property is missing. Spectrometer cannot be operated.");

    ScanSP = getSwitch("Scan");
    if (ScanSP == NULL)
        IDMessage(getDeviceName(), "Error: Channel property is missing. Spectrometer cannot be operated.");

    DataStreamBP = getBLOB("Data");
    if (DataStreamBP == NULL)
        IDMessage(getDeviceName(), "Error: BLOB data property is missing. Spectrometer cannot be operated.");

    if (DataStreamBP)
        DataStreamBP->bp[0].blob = (char *)malloc(MAXBLEN * sizeof(char));

    /**************************************************************************/
    // Equatorial Coords - SET
    IUFillNumber(&EquatorialCoordsRN[0], "RA", "RA  H:M:S", "%10.6m", 0., 24., 0., 0.);
    IUFillNumber(&EquatorialCoordsRN[1], "DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0.);
    IUFillNumberVector(&EquatorialCoordsRNP, EquatorialCoordsRN, NARRAY(EquatorialCoordsRN), "", "EQUATORIAL_EOD_COORD",
                       "Equatorial AutoSet", "", IP_RW, 0, IPS_IDLE);
    /**************************************************************************/
    return true;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::Connect()
{
    ITextVectorProperty *tProp = getText("DEVICE_PORT");

    if (isConnected())
        return true;

    if (tProp == NULL)
        return false;

    if (isSimulation())
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "%s Spectrometer: Simulating connection to port %s.", type_name.c_str(),
               tProp->tp[0].text);
        SetTimer(POLLMS);
        return true;
    }

    if (tty_connect(tProp->tp[0].text, 2400, 8, 0, 1, &fd) != TTY_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR,
               "Error connecting to port %s. Make sure you have BOTH read and write permission to the port.",
               tProp->tp[0].text);
        return false;
    }

    // We perform initial handshake check by resetting all parameter and watching for echo reply
    if (reset() == true)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Spectrometer is online. Retrieving preliminary data...");
        SetTimer(POLLMS);
        return init_spectrometer();
    }
    else
    {
        DEBUG(INDI::Logger::DBG_ERROR,
              "Spectrometer echo test failed. Please recheck connection to spectrometer and try again.");
        return false;
    }
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::init_spectrometer()
{
    // Enable speed mode
    if (isSimulation())
    {
        IDMessage(getDeviceName(), "%s Spectrometer: Simulating spectrometer init.", type_name.c_str());
        return true;
    }

    return true;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::Disconnect()
{
    tty_disconnect(fd);

    return true;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) != 0)
        return false;

    static double cont_offset = 0;
    static double spec_offset = 0;

    INumberVectorProperty *nProp = getNumber(name);

    if (nProp == NULL)
        return false;

    if (isConnected() == false)
    {
        resetProperties();
        IDMessage(getDeviceName(), "Spectrometer is offline. Connect before issiuing any commands.");
        return false;
    }

    // IF Gain
    if (!strcmp(nProp->name, "70 Mhz IF"))
    {
        double last_value = nProp->np[0].value;

        if (IUUpdateNumber(nProp, values, names, n) < 0)
            return false;

        if (dispatch_command(IF_GAIN) == false)
        {
            nProp->np[0].value = last_value;
            nProp->s           = IPS_ALERT;
            IDSetNumber(nProp, "Error dispatching IF gain command to spectrometer. Check logs.");
            return false;
        }

        nProp->s = IPS_OK;
        IDSetNumber(nProp, NULL);

        return true;
    }

    // DC Offset
    if (!strcmp(nProp->name, "DC Offset"))
    {
        if (IUUpdateNumber(nProp, values, names, n) < 0)
            return false;

        // Check which offset change, if none, return gracefully
        if (nProp->np[CONTINUUM_CHANNEL].value != cont_offset)
        {
            if (dispatch_command(CONT_OFFSET) == false)
            {
                nProp->np[CONTINUUM_CHANNEL].value = cont_offset;
                nProp->s                           = IPS_ALERT;
                IDSetNumber(nProp, "Error dispatching continuum DC offset command to spectrometer. Check logs.");
                return false;
            }

            cont_offset = nProp->np[CONTINUUM_CHANNEL].value;
        }

        if (nProp->np[SPECTRAL_CHANNEL].value != spec_offset)
        {
            if (dispatch_command(SPEC_OFFSET) == false)
            {
                nProp->np[SPECTRAL_CHANNEL].value = spec_offset;
                nProp->s                          = IPS_ALERT;
                IDSetNumber(nProp, "Error dispatching spectral DC offset command to spectrometer. Check logs.");
                return false;
            }

            spec_offset = nProp->np[SPECTRAL_CHANNEL].value;
        }

        // No Change, return
        nProp->s = IPS_OK;
        IDSetNumber(nProp, NULL);
        return true;
    }

    // Freq Change
    if (!strcmp(nProp->name, "Freq (Mhz)"))
        return update_freq(values[0]);

    // Scan Options
    if (!strcmp(nProp->name, "Scan Parameters"))
    {
        if (IUUpdateNumber(nProp, values, names, n) < 0)
            return false;

        nProp->s = IPS_OK;
        IDSetNumber(nProp, NULL);
        return true;
    }
    return true;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) != 0)
        return false;

    ITextVectorProperty *tProp = getText(name);

    if (tProp == NULL)
        return false;

    // Device Port Text
    if (!strcmp(tProp->name, "DEVICE_PORT"))
    {
        if (IUUpdateText(tProp, texts, names, n) < 0)
            return false;

        tProp->s = IPS_OK;
        IDSetText(tProp, "Port updated.");

        return true;
    }

    // Telescope Source
    if (!strcmp(tProp->name, "ACTIVE_DEVICES"))
    {
        telescopeID = IUFindText(tProp, "ACTIVE_TELESCOPE");

        if (telescopeID && strcmp(texts[0], telescopeID->text))
        {
            if (IUUpdateText(tProp, texts, names, n) < 0)
                return false;

            strncpy(EquatorialCoordsRNP.device, tProp->tp[0].text, MAXINDIDEVICE);

            IDMessage(getDeviceName(), "Active telescope updated to %s. Please save configuration.", telescopeID->text);

            IDSnoopDevice(telescopeID->text, "EQUATORIAL_EOD_COORD");
        }

        tProp->s = IPS_OK;
        IDSetText(tProp, NULL);
        return true;
    }

    return true;
}

/****************************************************************
**
**
*****************************************************************/
bool SpectraCyber::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) != 0)
        return false;

    // First process parent!
    if (INDI::DefaultDevice::ISNewSwitch(getDeviceName(), name, states, names, n) == true)
        return true;

    ISwitchVectorProperty *sProp = getSwitch(name);

    if (sProp == NULL)
        return false;

    if (isConnected() == false)
    {
        resetProperties();
        IDMessage(getDeviceName(), "Spectrometer is offline. Connect before issiuing any commands.");
        return false;
    }

    // Scan
    if (!strcmp(sProp->name, "Scan"))
    {
        if (!FreqNP || !DataStreamBP)
            return false;

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (sProp->sp[1].s == ISS_ON)
        {
            if (sProp->s == IPS_BUSY)
            {
                sProp->s        = IPS_IDLE;
                FreqNP->s       = IPS_IDLE;
                DataStreamBP->s = IPS_IDLE;

                IDSetNumber(FreqNP, NULL);
                IDSetBLOB(DataStreamBP, NULL);
                IDSetSwitch(sProp, "Scan stopped.");
                return false;
            }

            sProp->s = IPS_OK;
            IDSetSwitch(sProp, NULL);
            return true;
        }

        sProp->s        = IPS_BUSY;
        DataStreamBP->s = IPS_BUSY;

        // Compute starting freq  = base_freq - low
        if (sProp->sp[SPEC_CHANNEL].s == ISS_ON)
        {
            start_freq  = (SPECTROMETER_RF_FREQ + SPECTROMETER_REST_FREQ) - abs((int)ScanNP->np[0].value) / 1000.;
            target_freq = (SPECTROMETER_RF_FREQ + SPECTROMETER_REST_FREQ) + abs((int)ScanNP->np[1].value) / 1000.;
            sample_rate = ScanNP->np[2].value * 5;
            FreqNP->np[0].value = start_freq;
            FreqNP->s           = IPS_BUSY;
            IDSetNumber(FreqNP, NULL);
            IDSetSwitch(sProp, "Starting spectral scan from %g MHz to %g MHz in steps of %g KHz...", start_freq,
                        target_freq, sample_rate);
        }
        else
            IDSetSwitch(sProp, "Starting continuum scan @ %g MHz...", FreqNP->np[0].value);

        return true;
    }

    // Continuum Gain Control
    if (!strcmp(sProp->name, "Continuum Gain"))
    {
        int last_switch = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (dispatch_command(CONT_GAIN) == false)
        {
            sProp->s = IPS_ALERT;
            IUResetSwitch(sProp);
            sProp->sp[last_switch].s = ISS_ON;
            IDSetSwitch(sProp, "Error dispatching continuum gain command to spectrometer. Check logs.");
            return false;
        }

        sProp->s = IPS_OK;
        IDSetSwitch(sProp, NULL);

        return true;
    }

    // Spectral Gain Control
    if (!strcmp(sProp->name, "Spectral Gain"))
    {
        int last_switch = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (dispatch_command(SPEC_GAIN) == false)
        {
            sProp->s = IPS_ALERT;
            IUResetSwitch(sProp);
            sProp->sp[last_switch].s = ISS_ON;
            IDSetSwitch(sProp, "Error dispatching spectral gain command to spectrometer. Check logs.");
            return false;
        }

        sProp->s = IPS_OK;
        IDSetSwitch(sProp, NULL);

        return true;
    }

    // Continuum Integration Control
    if (!strcmp(sProp->name, "Continuum Integration (s)"))
    {
        int last_switch = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (dispatch_command(CONT_TIME) == false)
        {
            sProp->s = IPS_ALERT;
            IUResetSwitch(sProp);
            sProp->sp[last_switch].s = ISS_ON;
            IDSetSwitch(sProp, "Error dispatching continuum integration command to spectrometer. Check logs.");
            return false;
        }

        sProp->s = IPS_OK;
        IDSetSwitch(sProp, NULL);

        return true;
    }

    // Spectral Integration Control
    if (!strcmp(sProp->name, "Spectral Integration (s)"))
    {
        int last_switch = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (dispatch_command(SPEC_TIME) == false)
        {
            sProp->s = IPS_ALERT;
            IUResetSwitch(sProp);
            sProp->sp[last_switch].s = ISS_ON;
            IDSetSwitch(sProp, "Error dispatching spectral integration command to spectrometer. Check logs.");
            return false;
        }

        sProp->s = IPS_OK;
        IDSetSwitch(sProp, NULL);

        return true;
    }

    // Bandwidth Control
    if (!strcmp(sProp->name, "Bandwidth (Khz)"))
    {
        int last_switch = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        if (dispatch_command(BANDWIDTH) == false)
        {
            sProp->s = IPS_ALERT;
            IUResetSwitch(sProp);
            sProp->sp[last_switch].s = ISS_ON;
            IDSetSwitch(sProp, "Error dispatching bandwidth change command to spectrometer. Check logs.");
            return false;
        }

        sProp->s = IPS_OK;
        IDSetSwitch(sProp, NULL);

        return true;
    }

    // Channel selection
    if (!strcmp(sProp->name, "Channels"))
    {
        static int lastChannel;

        lastChannel = get_on_switch(sProp);

        if (IUUpdateSwitch(sProp, states, names, n) < 0)
            return false;

        sProp->s = IPS_OK;
        if (ScanSP->s == IPS_BUSY && lastChannel != get_on_switch(sProp))
        {
            abort_scan();
            IDSetSwitch(sProp, "Scan aborted due to change of channel selection.");
        }
        else
            IDSetSwitch(sProp, NULL);

        return true;
    }

    // Reset
    if (!strcmp(sProp->name, "Reset"))
    {
        if (reset() == true)
        {
            sProp->s = IPS_OK;
            IDSetSwitch(sProp, NULL);
        }
        else
        {
            sProp->s = IPS_ALERT;
            IDSetSwitch(sProp, "Error dispatching reset parameter command to spectrometer. Check logs.");
            return false;
        }
        return true;
    }

    return true;
}

bool SpectraCyber::dispatch_command(SpectrometerCommand command_type)
{
    char spectrometer_error[SPECTROMETER_ERROR_BUFFER];
    int err_code = 0, nbytes_written = 0, final_value = 0;
    // Maximum of 3 hex digits in addition to null terminator
    char hex[5];
    INumberVectorProperty *nProp = NULL;
    ISwitchVectorProperty *sProp = NULL;

    tcflush(fd, TCIOFLUSH);

    switch (command_type)
    {
        // Intermediate Frequency Gain
        case IF_GAIN:
            nProp = getNumber("70 Mhz IF");
            if (nProp == NULL)
                return false;
            command[1] = 'A';
            command[2] = '0';
            // Equation is
            // Value = ((X - 10) * 63) / 15.75, where X is the user selection (10dB to 25.75dB)
            final_value = (int)((nProp->np[0].value - 10) * 63) / 15.75;
            sprintf(hex, "%02X", (uint16_t)final_value);
            command[3] = hex[0];
            command[4] = hex[1];
            break;

        // Continuum Gain
        case CONT_GAIN:
            sProp = getSwitch("Continuum Gain");
            if (sProp == NULL)
                return false;
            command[1]  = 'G';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(sProp);
            sprintf(hex, "%d", (uint8_t)final_value);
            command[4] = hex[0];
            break;

        // Continuum Integration
        case CONT_TIME:
            sProp = getSwitch("Continuum Integration (s)");
            if (sProp == NULL)
                return false;
            command[1]  = 'I';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(sProp);
            sprintf(hex, "%d", (uint8_t)final_value);
            command[4] = hex[0];
            break;

        // Spectral Gain
        case SPEC_GAIN:
            sProp = getSwitch("Spectral Gain");
            if (sProp == NULL)
                return false;
            command[1]  = 'K';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(sProp);
            sprintf(hex, "%d", (uint8_t)final_value);
            command[4] = hex[0];

            break;

        // Spectral Integration
        case SPEC_TIME:
            sProp = getSwitch("Spectral Integration (s)");
            if (sProp == NULL)
                return false;
            command[1]  = 'L';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(sProp);
            sprintf(hex, "%d", (uint8_t)final_value);
            command[4] = hex[0];
            break;

        // Continuum DC Offset
        case CONT_OFFSET:
            nProp = getNumber("DC Offset");
            if (nProp == NULL)
                return false;
            command[1]  = 'O';
            final_value = (int)nProp->np[CONTINUUM_CHANNEL].value / 0.001;
            sprintf(hex, "%03X", (uint32_t)final_value);
            command[2] = hex[0];
            command[3] = hex[1];
            command[4] = hex[2];
            break;

        // Spectral DC Offset
        case SPEC_OFFSET:
            nProp = getNumber("DC Offset");
            if (nProp == NULL)
                return false;
            command[1]  = 'J';
            final_value = (int)nProp->np[SPECTRAL_CHANNEL].value / 0.001;
            sprintf(hex, "%03X", (uint32_t)final_value);
            command[2] = hex[0];
            command[3] = hex[1];
            command[4] = hex[2];
            break;

        // FREQ
        case RECV_FREQ:
            command[1] = 'F';
            // Each value increment is 5 Khz. Range is 050h to 3e8h.
            // 050h corresponds to 46.4 Mhz (min), 3e8h to 51.2 Mhz (max)
            // To compute the desired received freq, we take the diff (target - min) / 0.005
            // 0.005 Mhz = 5 Khz
            // Then we add the diff to 050h (80 in decimal) to get the final freq.
            // e.g. To set 50.00 Mhz, diff = 50 - 46.4 = 3.6 / 0.005 = 800 = 320h
            //      Freq = 320h + 050h (or 800 + 80) = 370h = 880 decimal

            final_value = (int)((FreqNP->np[0].value + SPECTROMETER_REST_CORRECTION - FreqNP->np[0].min) / 0.005 +
                                SPECTROMETER_OFFSET);
            sprintf(hex, "%03X", (uint32_t)final_value);
            if (isDebug())
                IDLog("Required Freq is: %.3f --- Min Freq is: %.3f --- Spec Offset is: %d -- Final Value (Dec): %d "
                      "--- Final Value (Hex): %s\n",
                      FreqNP->np[0].value, FreqNP->np[0].min, SPECTROMETER_OFFSET, final_value, hex);
            command[2] = hex[0];
            command[3] = hex[1];
            command[4] = hex[2];
            break;

        // Read Channel
        case READ_CHANNEL:
            command[1]  = 'D';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(ChannelSP);
            command[4]  = (final_value == 0) ? '0' : '1';
            break;

        // Bandwidth
        case BANDWIDTH:
            sProp = getSwitch("Bandwidth (Khz)");
            if (sProp == NULL)
                return false;
            command[1]  = 'B';
            command[2]  = '0';
            command[3]  = '0';
            final_value = get_on_switch(sProp);
            //sprintf(hex, "%x", final_value);
            command[4] = (final_value == 0) ? '0' : '1';
            break;

        // Reset
        case RESET:
            command[1] = 'R';
            command[2] = '0';
            command[3] = '0';
            command[4] = '0';
            break;

        // Noise source
        case NOISE_SOURCE:
            // TODO: Do something here?
            break;
    }

    if (isDebug())
        IDLog("Dispatching command #%s#\n", command);

    if (isSimulation())
        return true;

    if ((err_code = tty_write(fd, command, SPECTROMETER_CMD_LEN, &nbytes_written) != TTY_OK))
    {
        tty_error_msg(err_code, spectrometer_error, SPECTROMETER_ERROR_BUFFER);
        if (isDebug())
            IDLog("TTY error detected: %s\n", spectrometer_error);
        return false;
    }

    return true;
}

int SpectraCyber::get_on_switch(ISwitchVectorProperty *sp)
{
    for (int i = 0; i < sp->nsp; i++)
        if (sp->sp[i].s == ISS_ON)
            return i;

    return -1;
}

bool SpectraCyber::update_freq(double nFreq)
{
    double last_value = FreqNP->np[0].value;

    if (nFreq < FreqNP->np[0].min || nFreq > FreqNP->np[0].max)
        return false;

    FreqNP->np[0].value = nFreq;

    if (dispatch_command(RECV_FREQ) == false)
    {
        FreqNP->np[0].value = last_value;
        FreqNP->s           = IPS_ALERT;
        IDSetNumber(FreqNP, "Error dispatching RECV FREQ command to spectrometer. Check logs.");
        return false;
    }

    if (ScanSP->s != IPS_BUSY)
        FreqNP->s = IPS_OK;

    IDSetNumber(FreqNP, NULL);

    // Delay 0.5s for INT
    usleep(500000);
    return true;
}

bool SpectraCyber::reset()
{
    int err_code = 0, nbytes_read = 0;
    char response[4];
    char err_msg[SPECTROMETER_ERROR_BUFFER];

    if (isDebug())
        IDLog("Attempting to write to spectrometer....\n");

    dispatch_command(RESET);

    if (isDebug())
        IDLog("Attempting to read from spectrometer....\n");

    // Read echo from spectrometer, we're expecting R000
    if ((err_code = tty_read(fd, response, SPECTROMETER_CMD_REPLY, 5, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(err_code, err_msg, 32);
        if (isDebug())
            IDLog("TTY error detected: %s\n", err_msg);
        return false;
    }

    if (isDebug())
        IDLog("Response from Spectrometer: #%c# #%c# #%c# #%c#\n", response[0], response[1], response[2], response[3]);

    if (strstr(response, "R000"))
    {
        if (isDebug())
            IDLog("Echo test passed.\n");

        loadDefaultConfig();

        return true;
    }

    if (isDebug())
        IDLog("Echo test failed.\n");
    return false;
}

void SpectraCyber::TimerHit()
{
    if (!isConnected())
        return;

    char RAStr[16], DecStr[16];

    switch (ScanSP->s)
    {
        case IPS_BUSY:
            if (ChannelSP->sp[CONT_CHANNEL].s == ISS_ON)
                break;

            if (current_freq >= target_freq)
            {
                ScanSP->s = IPS_OK;
                FreqNP->s = IPS_OK;

                IDSetNumber(FreqNP, NULL);
                IDSetSwitch(ScanSP, "Scan complete.");
                SetTimer(POLLMS);
                return;
            }

            if (update_freq(current_freq) == false)
            {
                abort_scan();
                SetTimer(POLLMS);
                return;
            }

            current_freq += sample_rate / 1000.;
            break;

        default:
            break;
    }

    switch (DataStreamBP->s)
    {
        case IPS_BUSY:
            if (ScanSP->s != IPS_BUSY)
            {
                DataStreamBP->s = IPS_IDLE;
                IDSetBLOB(DataStreamBP, NULL);
                break;
            }

            if (read_channel() == false)
            {
                DataStreamBP->s = IPS_ALERT;

                if (ScanSP->s == IPS_BUSY)
                    abort_scan();

                IDSetBLOB(DataStreamBP, NULL);
            }

            JD = ln_get_julian_from_sys();

            // Continuum
            if (ChannelSP->sp[0].s == ISS_ON)
                strncpy(DataStreamBP->bp[0].format, contFMT, MAXINDIBLOBFMT);
            else
                strncpy(DataStreamBP->bp[0].format, specFMT, MAXINDIBLOBFMT);

            fs_sexa(RAStr, EquatorialCoordsRN[0].value, 2, 3600);
            fs_sexa(DecStr, EquatorialCoordsRN[1].value, 2, 3600);

            if (telescopeID && strlen(telescopeID->text) > 0)
                snprintf(bLine, MAXBLEN, "%.8f %.3f %.3f %s %s", JD, chanValue, current_freq, RAStr, DecStr);
            else
                snprintf(bLine, MAXBLEN, "%.8f %.3f %.3f", JD, chanValue, current_freq);

            DataStreamBP->bp[0].bloblen = DataStreamBP->bp[0].size = strlen(bLine);
            memcpy(DataStreamBP->bp[0].blob, bLine, DataStreamBP->bp[0].bloblen);

            //IDLog("\nSTRLEN: %d -- BLOB:'%s'\n", strlen(bLine), (char *) DataStreamBP->bp[0].blob);

            IDSetBLOB(DataStreamBP, NULL);

            break;

        default:
            break;
    }

    SetTimer(POLLMS);
}

void SpectraCyber::abort_scan()
{
    FreqNP->s = IPS_IDLE;
    ScanSP->s = IPS_ALERT;

    IUResetSwitch(ScanSP);
    ScanSP->sp[1].s = ISS_ON;

    IDSetNumber(FreqNP, NULL);
    IDSetSwitch(ScanSP, "Scan aborted due to errors.");
}

bool SpectraCyber::read_channel()
{
    int err_code = 0, nbytes_read = 0;
    char response[SPECTROMETER_CMD_REPLY];
    char err_msg[SPECTROMETER_ERROR_BUFFER];

    if (isSimulation())
    {
        chanValue = ((double)rand()) / ((double)RAND_MAX) * 10.0;
        return true;
    }

    dispatch_command(READ_CHANNEL);
    if ((err_code = tty_read(fd, response, SPECTROMETER_CMD_REPLY, 5, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(err_code, err_msg, 32);
        if (isDebug())
            IDLog("TTY error detected: %s\n", err_msg);
        return false;
    }

    if (isDebug())
        IDLog("Response from Spectrometer: #%s#\n", response);

    int result = 0;
    sscanf(response, "D%x", &result);
    // We divide by 409.5 to scale the value to 0 - 10 VDC range
    chanValue = result / 409.5;

    return true;
}

const char *SpectraCyber::getDefaultName()
{
    return mydev;
}
