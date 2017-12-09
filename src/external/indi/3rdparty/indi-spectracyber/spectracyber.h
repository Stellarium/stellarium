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
    2009-09-17: Creating spectrometer class (JM)

*/

#pragma once

#include <defaultdevice.h>

#include <string>

#define MAXBLEN 64

class SpectraCyber : public INDI::DefaultDevice
{
  public:
    enum SpectrometerCommand
    {
        IF_GAIN,      // IF Gain
        CONT_GAIN,    // Continuum Gain
        SPEC_GAIN,    // Spectral Gain
        CONT_TIME,    // Continuum Channel Integration Constant
        SPEC_TIME,    // Spectral Channel Integration Constant
        NOISE_SOURCE, // Noise Source Control
        CONT_OFFSET,  // Continuum DC Offset
        SPEC_OFFSET,  // Spectral DC Offset
        RECV_FREQ,    // Receive Frequency
        READ_CHANNEL, // Read Channel Value
        BANDWIDTH,    // Bandwidth
        RESET         // Reset All
    };

    enum SpectrometerChannel
    {
        CONTINUUM_CHANNEL,
        SPECTRAL_CHANNEL
    };
    enum SpectrometerError
    {
        NO_ERROR,
        BAUD_RATE_ERROR,
        FLASH_MEMORY_ERROR,
        WRONG_COMMAND_ERROR,
        WRONG_PARAMETER_ERROR,
        FATAL_ERROR
    };

    SpectraCyber();
    ~SpectraCyber();

    // Standard INDI interface functions
    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISSnoopDevice(XMLEle *root) override;

  protected:
    virtual const char *getDefaultName() override;

    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual void TimerHit() override;

    //void reset_all_properties(bool reset_to_idle=false);
    bool update_freq(double nFreq);

  private:
    INumberVectorProperty *FreqNP;
    INumberVectorProperty *ScanNP;
    ISwitchVectorProperty *ScanSP;
    ISwitchVectorProperty *ChannelSP;
    IBLOBVectorProperty *DataStreamBP;
    IText *telescopeID;

    // Snooping On
    INumber EquatorialCoordsRN[2];
    INumberVectorProperty EquatorialCoordsRNP;

    // Functions
    virtual bool initProperties() override;
    bool init_spectrometer();
    void abort_scan();
    bool read_channel();
    bool dispatch_command(SpectrometerCommand command);
    int get_on_switch(ISwitchVectorProperty *sp);
    bool reset();

    // Variables
    std::string type_name;
    std::string default_port;

    int fd;
    char bLine[MAXBLEN];
    char command[5];
    double start_freq, target_freq, sample_rate, JD, chanValue;
};
