#ifndef APOGEE_CCD_H
#define APOGEE_CCD_H

#if 0
    Apogee CCD
    INDI Driver for Apogee CCDs and Filter Wheels
    Copyright (C) 2014 Jasem Mutlaq <mutlaqja AT ikarustech DOT com>

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

#endif

#include <indiccd.h>
#include <indiguiderinterface.h>
#include <iostream>

#include <ApogeeCam.h>
#include <FindDeviceEthernet.h>
#include <FindDeviceUsb.h>

class ApogeeCCD : public INDI::CCD
{
  public:
    ApogeeCCD();
    virtual ~ApogeeCCD();

    enum
    {
        NETWORK_SUBNET,
        NETWORK_ADDRESS
    };

    const char *getDefaultName();

    void ISGetProperties(const char *dev);
    bool initProperties();
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    int SetTemperature(double temperature);
    bool StartExposure(float duration);
    bool AbortExposure();

    void TimerHit();

    virtual bool UpdateCCDFrame(int x, int y, int w, int h);
    virtual bool UpdateCCDBin(int binx, int biny);

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

  protected:
    void debugTriggered(bool enabled);
    bool saveConfigItems(FILE *fp);

  private:
    ApogeeCam *ApgCam;

    INumber CoolerN[1];
    INumberVectorProperty CoolerNP;

    ISwitch CoolerS[2];
    ISwitchVectorProperty CoolerSP;

    ISwitch ReadOutS[2];
    ISwitchVectorProperty ReadOutSP;

    ISwitch PortTypeS[2];
    ISwitchVectorProperty PortTypeSP;

    IText NetworkInfoT[2];
    ITextVectorProperty NetworkInfoTP;

    IText CamInfoT[2];
    ITextVectorProperty CamInfoTP;

    ISwitch FanStatusS[4];
    ISwitchVectorProperty FanStatusSP;

    double minDuration;
    double ExposureRequest;
    int imageWidth, imageHeight;
    int timerID;
    INDI::CCDChip::CCD_FRAME imageFrameType;
    struct timeval ExpStart;

    std::string ioInterface;
    std::string subnet;
    std::string firmwareRev;
    std::string modelStr;
    FindDeviceEthernet look4cam;
    FindDeviceUsb lookUsb;
    CamModel::PlatformType model;

    void checkStatus(const Apg::Status status);
    std::vector<std::string> MakeTokens(const std::string &str, const std::string &separator);
    std::string GetItemFromFindStr(const std::string &msg, const std::string &item);
    //std::string GetAddress( const std::string & msg );
    std::string GetUsbAddress(const std::string &msg);
    std::string GetEthernetAddress(const std::string &msg);
    std::string GetIPAddress(const std::string &msg);
    CamModel::PlatformType GetModel(const std::string &msg);
    uint16_t GetID(const std::string &msg);
    uint16_t GetFrmwrRev(const std::string &msg);

    bool IsDeviceFilterWheel(const std::string &msg);
    bool IsAscent(const std::string &msg);
    void printInfo(const std::string &model, uint16_t maxImgRows, uint16_t maxImgCols);

    float CalcTimeLeft(timeval, float);
    int grabImage();
    bool getCameraParams();
    void activateCooler(bool enable);
};

#endif // APOGEE_CCD_H
