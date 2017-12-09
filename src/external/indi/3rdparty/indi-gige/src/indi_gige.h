/*
 GigE interface for INDI based on aravis
 Copyright (C) 2016 Hendrik Beijeman (hbeyeman@gmail.com)

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

#ifndef GENERIC_CCD_H
#define GENERIC_CCD_H

#include <indiccd.h>
#include <iostream>

#include "ArvInterface.h"

using namespace std;

class GigECCD : public INDI::CCD
{
  public:
    GigECCD(arv::ArvCamera *camera);
    virtual ~GigECCD();

    const char *getDefaultName();

    bool initProperties();
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    bool StartExposure(float duration);
    bool AbortExposure();

  protected:
    void TimerHit();
    virtual bool UpdateCCDFrame(int x, int y, int w, int h);
    virtual bool UpdateCCDBin(int binx, int biny);
    virtual bool UpdateCCDFrameType(CCDChip::CCD_FRAME fType);

  private:
    void _delete_indi_properties(void);
    void _update_indi_properties(void);
    bool _update_geometry(void);
    void _update_image(uint8_t const *const data, size_t size);
    static void _receive_image_hook(void *const class_ptr, uint8_t const *const data, size_t size);

    void _handle_failed(void);
    void _handle_timeout(struct timeval *const tv, uint32_t timeout_us);

    arv::ArvCamera *camera;
    char name[32];
    int timer_id;
    struct timeval exposure_start_time;
    struct timeval exposure_transfer_time;

    /* Indi properties */

    INumber indiprop_gain[1];
    INumberVectorProperty indiprop_gain_prop;
    IText indiprop_info[3];
    ITextVectorProperty indiprop_info_prop;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

    friend void ::ISGetProperties(const char *dev);
    friend void ::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num);
    friend void ::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num);
    friend void ::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num);
    friend void ::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                            char *formats[], char *names[], int n);
};

#endif // GENERIC_CCD_H
