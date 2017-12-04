/*
    Induino general propose driver. Allow using arduino boards
    as general I/O 	
    Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

    Base on INDIDUINO Four
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#pragma once

#include <defaultdevice.h>

class Firmata;

/* NAMES in the xml skeleton file to 
   used to define I/O arduino mapping*/

#define MAX_IO_PIN                128
#define MAX_SKELTON_FILE_NAME_LEN 504

typedef enum { DI, DO, AI, AO, I2C_I, I2C_O, SERVO } IOTYPEStr;

typedef struct
{
    IOTYPEStr IOType;
    int pin;
    double MulScale;
    double AddScale;
    double OnAngle;
    double OffAngle;
    double buttonIncValue;
    char *SwitchButton;
    char *UpButton;
    char *DownButton;
    char *defName;
    char *defVectorName;
} IO;

class indiduino : public INDI::DefaultDevice
{
  public:
    indiduino();
    ~indiduino();

    virtual bool initProperties() override;
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual void TimerHit() override;
    /** \brief Called when connected state changes, to add/remove properties */
    virtual bool updateProperties() override;

    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISSnoopDevice(XMLEle *root) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n) override;

    // Joystick helpers
    static void joystickHelper(const char *joystick_n, double mag, double angle, void *context);
    static void buttonHelper(const char *button_n, ISState state, void *context);
    static void axisHelper(const char *axis_n, double value, void *context);

    void processJoystick(const char *joystick_n, double mag, double angle);
    void processButton(const char *button_n, ISState state);
    void processAxis(const char *axis_n, double value);

  protected:
    virtual const char *getDefaultName() override;

  private:
    char skelFileName[MAX_SKELTON_FILE_NAME_LEN];
    IO iopin[MAX_IO_PIN];

    bool setPinModesFromSKEL();
    bool readInduinoXml(XMLEle *ioep, int npin);
    Firmata *sf;
    INDI::Controller *controller;
};
