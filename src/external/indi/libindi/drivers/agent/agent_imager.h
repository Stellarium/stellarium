/*******************************************************************************
 Copyright(c) 2013-2016 CloudMakers, s. r. o. All rights reserved.

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

#pragma once

#include "baseclient.h"
#include "defaultdevice.h"

#define MAX_GROUP_COUNT 16

class Group
{
  public:
    explicit Group(int id);

    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

    void defineProperties();
    void deleteProperties();

    INumberVectorProperty GroupSettingsNP;
    INumber GroupSettingsN[4];
  private:
    char groupName[16];
    char groupSettingsName[32];
};

class Imager : public virtual INDI::DefaultDevice, public virtual INDI::BaseClient
{
  public:
    Imager();
    virtual ~Imager() = default;

    // DefaultDevice

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

    // BaseClient

    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property *property);
    virtual void removeDevice(INDI::BaseDevice *dp);
    virtual void newBLOB(IBLOB *bp);
    virtual void newSwitch(ISwitchVectorProperty *svp);
    virtual void newNumber(INumberVectorProperty *nvp);
    virtual void newText(ITextVectorProperty *tvp);
    virtual void newLight(ILightVectorProperty *lvp);
    virtual void newMessage(INDI::BaseDevice *dp, int messageID);
    virtual void serverConnected();
    virtual void serverDisconnected(int exit_code);

  protected:
    virtual const char *getDefaultName();
    virtual bool Connect();
    virtual bool Disconnect();

  private:
    bool isRunning();
    bool isCCDConnected();
    bool isFilterConnected();
    void defineProperties();
    void deleteProperties();
    void initiateNextFilter();
    void initiateNextCapture();
    void startBatch();
    void abortBatch();
    void batchDone();
    void initiateDownload();

    char format[16];
    int group { 0 };
    int maxGroup { 0 };
    int image { 0 };
    int maxImage { 0 };
    char *controlledCCD { nullptr };
    char *controlledFilterWheel { nullptr };

    ITextVectorProperty ControlledDeviceTP;
    IText ControlledDeviceT[2];
    INumberVectorProperty GroupCountNP;
    INumber GroupCountN[1];
    INumberVectorProperty ProgressNP;
    INumber ProgressN[3];
    ISwitchVectorProperty BatchSP;
    ISwitch BatchS[2];
    ILightVectorProperty StatusLP;
    ILight StatusL[2];
    ITextVectorProperty ImageNameTP;
    IText ImageNameT[2];
    INumberVectorProperty DownloadNP;
    INumber DownloadN[2];
    IBLOBVectorProperty FitsBP;
    IBLOB FitsB[1];

    INumberVectorProperty CCDImageExposureNP;
    INumber CCDImageExposureN[1];
    INumberVectorProperty CCDImageBinNP;
    INumber CCDImageBinN[2];
    ISwitch CCDUploadS[3];
    ISwitchVectorProperty CCDUploadSP;
    IText CCDUploadSettingsT[2];
    ITextVectorProperty CCDUploadSettingsTP;

    INumberVectorProperty FilterSlotNP;
    INumber FilterSlotN[1];

    Group *groups[MAX_GROUP_COUNT];
};
