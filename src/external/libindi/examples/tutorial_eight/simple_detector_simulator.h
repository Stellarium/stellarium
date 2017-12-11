/*
   INDI Developers Manual
   Tutorial #3

   "Simple Detector Driver"

   We develop a simple Detector driver.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file simpleDetector.h
    \brief Construct a basic INDI Detector device that simulates capture & temperature settings. It also generates a random pattern and uploads it as a FITS file.
    \author Ilia Platone

    \example simpleDetector.h
    A simple detector device that can capture stream frames and controls temperature. It returns a FITS image to the client. To build drivers for complex Detectors, please
    refer to the INDI Generic Detector driver template in INDI github (under 3rdparty).
*/

#pragma once

#include "indidetector.h"

class SimpleDetector : public INDI::Detector
{
  public:
    SimpleDetector() = default;

  protected:
    // General device functions
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

    // Detector specific functions
    bool StartCapture(float duration);
    bool CaptureParamsUpdated(float bw, float freq, float bps);
    bool AbortCapture();
    int SetTemperature(double temperature);
    void TimerHit();

  private:
    // Utility functions
    float CalcTimeLeft();
    void setupParams();
    void grabFrame();

    // Are we exposing?
    bool InCapture { false };
    // Struct to keep timing
    struct timeval CapStart { 0, 0 };

    float CaptureRequest { 0 };
    float TemperatureRequest { 0 };
};
