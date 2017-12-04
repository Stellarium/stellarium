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

#include <time.h>
#include <list>
#include <sys/time.h>

#include "indidevapi.h"
#include "eventloop.h"

#include "indi_gige.h"

#define TIME_VAL_INIT(x)  \
    ({                    \
        (x)->tv_sec  = 0; \
        (x)->tv_usec = 0; \
    })
#define TIME_VAL_ISSET(x) (((x)->tv_sec != 0) && ((x)->tv_usec != 0))
#define TIME_VAL_US(x)    (((x)->tv_sec) * 1000000 + ((x)->tv_usec))
#define TIME_VAL_GET(x)   (gettimeofday(x, NULL))

#define TIMER_TRANSFER_TIMEOUT_US (5000000UL) /* Allow for relatively large link-layer delays */
#define TIMER_EXPOSURE_TIMEOUT_US (200000UL)  /* GigE cameras are very precise, so set 100ms time-out */

#define TIMER_US_TO_MS (1000)
#define TIMER_US_TO_S  (1000000)
#define TIMER_TICK_MS  (100)
#define CAPS           (CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME)

#define FOR_EVERY_CAMERA                       \
    {                                          \
        std::list<GigECCD *>::iterator camera; \
        for (camera = cameras.begin(); camera != cameras.end(); ++camera)

static std::list<GigECCD *> cameras;

static void cleanup()
{
    FOR_EVERY_CAMERA { delete (*camera); }
}
}

void ISInit()
{
    static bool has_init = false;
    if (!has_init)
    {
        has_init = true;

        arv::ArvCamera *camera = arv::ArvFactory::find_first_available();
        GigECCD *indi_camera   = new GigECCD(camera);
        cameras.push_back(indi_camera);
    }
    atexit(cleanup);
}

void ISGetProperties(const char *dev)
{
    ISInit();
    FOR_EVERY_CAMERA
    {
        if (dev == NULL || !strcmp(dev, (*camera)->name))
        {
            (*camera)->ISGetProperties(dev);
            //??????????????????
            if (dev != NULL)
                break;
        }
    }
}
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    FOR_EVERY_CAMERA
    {
        if (dev == NULL || !strcmp(dev, (*camera)->name))
        {
            (*camera)->ISNewSwitch(dev, name, states, names, num);
            if (dev != NULL)
                break;
        }
    }
}
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    FOR_EVERY_CAMERA
    {
        if (dev == NULL || !strcmp(dev, (*camera)->name))
        {
            (*camera)->ISNewText(dev, name, texts, names, num);
            if (dev != NULL)
                break;
        }
    }
}
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    FOR_EVERY_CAMERA
    {
        if (dev == NULL || !strcmp(dev, (*camera)->name))
        {
            (*camera)->ISNewNumber(dev, name, values, names, num);
            if (dev != NULL)
                break;
        }
    }
}
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
    ISInit();

    FOR_EVERY_CAMERA { (*camera)->ISSnoopDevice(root); }
}
}

const char *GigECCD::getDefaultName()
{
    return this->name;
}

GigECCD::GigECCD(arv::ArvCamera *camera)
{
    this->camera = camera;
    snprintf(this->name, sizeof(this->name), "%s", this->camera->model_name());
    setDeviceName(this->name);
}

GigECCD::~GigECCD()
{
}

bool GigECCD::initProperties()
{
    INDI::CCD::initProperties();
    this->SetCCDCapability((CAPS));
    this->addConfigurationControl();
    this->addDebugControl();
    return true;
}

bool GigECCD::_update_geometry(void)
{
    /* Get actual values */
    this->camera->update_geometry();

    /* Sync these with INDI */
    PrimaryCCD.setBin(this->camera->get_bin_x().val(), this->camera->get_bin_y().val());
    PrimaryCCD.setFrame(this->camera->get_x_offset().val(), this->camera->get_y_offset().val(),
                        this->camera->get_width().val(), this->camera->get_height().val());

    /* Sanity checks, reserve buffers */
    int const width           = this->camera->get_width().val();
    int const height          = this->camera->get_height().val();
    int const frame_byte_size = this->camera->get_frame_byte_size();
    int const indi_bufsize    = PrimaryCCD.getSubW() * PrimaryCCD.getSubH() * PrimaryCCD.getBPP() / 8;

    if (indi_bufsize != frame_byte_size)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unexpected INDI image buffer size, has %i bytes, camera has %i", indi_bufsize,
               frame_byte_size);
        PrimaryCCD.setFrameBufferSize(0);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Reserving INDI image buffer size %i bytes", indi_bufsize);
        PrimaryCCD.setFrameBufferSize(frame_byte_size);
    }
}

void GigECCD::_update_indi_properties(void)
{
    DEBUG(INDI::Logger::DBG_SESSION, "update_indi_properties()");
    IUFillNumber(&this->indiprop_gain[0], "Range", "", "%g", (double)this->camera->get_gain().min(),
                 (double)this->camera->get_gain().max(), 1., (double)this->camera->get_gain().val());
    IUFillNumberVector(&this->indiprop_gain_prop, this->indiprop_gain, 1, getDeviceName(), "Gain", "", MAIN_CONTROL_TAB,
                       IP_RW, 60, IPS_IDLE);

    IUFillText(&indiprop_info[0], "Vendor Name", "", this->camera->vendor_name());
    IUFillText(&indiprop_info[1], "Model Name", "", this->camera->model_name());
    IUFillText(&indiprop_info[2], "Device ID", "", this->camera->device_id());
    IUFillTextVector(&indiprop_info_prop, indiprop_info, 3, getDeviceName(), "Camera Info", "", MAIN_CONTROL_TAB, IP_RO,
                     0, IPS_IDLE);

    defineText(&indiprop_info_prop);
    defineNumber(&this->indiprop_gain_prop);
}

void GigECCD::_delete_indi_properties(void)
{
    this->deleteProperty(this->indiprop_gain_prop.name);
    this->deleteProperty(this->indiprop_info_prop.name);
}

//Initial call
bool GigECCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (this->camera->is_connected())
    {
        this->_update_indi_properties();
        this->SetCCDParams(this->camera->get_width().max(), this->camera->get_height().max(),
                           this->camera->get_bpp().val(), this->camera->get_pixel_pitch().val(),
                           this->camera->get_pixel_pitch().val());

        (void)this->_update_geometry();
        this->timer_id = this->SetTimer(TIMER_TICK_MS);
    }
    else
    {
        rmTimer(this->timer_id);
        this->_delete_indi_properties();
    }

    return true;
}

bool GigECCD::Connect()
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s", __PRETTY_FUNCTION__);
    return camera->connect();
}

bool GigECCD::Disconnect()
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s", __PRETTY_FUNCTION__);
#if 0
    //TODO: re-iterate and acquire proper camera from AvrFactory (based on ID?)
    return camera->disconnect();
#endif
    return true;
}

bool GigECCD::StartExposure(float duration)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s exposure_time=%.4f", __PRETTY_FUNCTION__, duration);
    /* Driver will clamp to lowest possible exposure */
    if (PrimaryCCD.getFrameType() == CCDChip::BIAS_FRAME)
        duration = 0;

    camera->set_exposure_time((double)(duration)*1000000.0);

    TIME_VAL_INIT(&this->exposure_transfer_time);
    TIME_VAL_GET(&this->exposure_start_time);

    camera->exposure_start();
    return camera->is_exposing();
}

bool GigECCD::AbortExposure()
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s", __PRETTY_FUNCTION__);
    camera->exposure_abort();
    return true;
}

void GigECCD::_update_image(uint8_t const *const data, size_t size)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "Receiving %i bytes image", size);

    size_t const frame_buf_size = PrimaryCCD.getFrameBufferSize();

    if ((size == frame_buf_size) && (data != NULL))
    {
        uint8_t *const image = PrimaryCCD.getFrameBuffer();
        memcpy(image, (void *const)data, frame_buf_size);
        this->ExposureComplete(&PrimaryCCD);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unexpected failure during image download. Framebuf has %i bytes, got %i",
               frame_buf_size, size);
        this->_handle_failed();
    }
}

void GigECCD::_receive_image_hook(void *const class_ptr, uint8_t const *const data, size_t size)
{
    GigECCD *const cls = static_cast<GigECCD *const>(class_ptr);
    cls->_update_image(data, size);
}

void GigECCD::_handle_failed(void)
{
    DEBUG(INDI::Logger::DBG_ERROR, "Failure occurred, filling image with black");

    camera->exposure_abort();

    PrimaryCCD.setExposureLeft(0);

    /* Fill with black */
    uint8_t *const image = PrimaryCCD.getFrameBuffer();
    memset(image, 0, PrimaryCCD.getFrameBufferSize());

    this->ExposureComplete(&PrimaryCCD);
}

void GigECCD::_handle_timeout(struct timeval *const tv, uint32_t timeout_us)
{
    if (!TIME_VAL_ISSET(tv))
        TIME_VAL_GET(tv);

    struct timeval now;
    TIME_VAL_GET(&now);

    uint32_t const elapsed       = ((TIME_VAL_US(&now)) - (TIME_VAL_US(tv)));
    uint32_t const exposure_time = (uint32_t)this->camera->get_exposure().val();
    uint32_t const time_left     = exposure_time - elapsed;

    if (elapsed >= exposure_time)
        PrimaryCCD.setExposureLeft(0);
    else
        PrimaryCCD.setExposureLeft((float)time_left / (float)TIMER_US_TO_S);

    if (elapsed > timeout_us)
        this->_handle_failed();
}

void GigECCD::TimerHit()
{
    this->timer_id = this->SetTimer(TIMER_TICK_MS);
    if (!this->camera->is_connected() || !this->camera->is_exposing())
        return;

    arv::ARV_EXPOSURE_STATUS const status = camera->exposure_poll(this->_receive_image_hook, this);
    switch (status)
    {
        case arv::ARV_EXPOSURE_FINISHED:
            /* Nothing to do, ArvCamera automatically unsets is_exposing */
            break;
        case arv::ARV_EXPOSURE_UNKNOWN:
        case arv::ARV_EXPOSURE_FAILED:
            this->_handle_failed();
            break;
        case arv::ARV_EXPOSURE_FILLING:
            this->_handle_timeout(&this->exposure_transfer_time, TIMER_TRANSFER_TIMEOUT_US);
            break;
        case arv::ARV_EXPOSURE_BUSY:
            this->_handle_timeout(&this->exposure_start_time,
                                  ((uint32_t)this->camera->get_exposure().val() + TIMER_EXPOSURE_TIMEOUT_US));
            break;
    }
}

bool GigECCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (!strcmp(dev, this->getDeviceName()))
    {
        if (!strcmp(name, this->indiprop_gain_prop.name))
        {
            IUUpdateNumber(&this->indiprop_gain_prop, values, names, n);
            this->camera->set_gain(this->indiprop_gain[0].value);
            this->indiprop_gain_prop.s = IPS_OK;

            /* Get-back from camera system */
            double actual_value = this->camera->get_gain().val();
            IUUpdateNumber(&this->indiprop_gain_prop, &actual_value, names, n);
            IDSetNumber(&this->indiprop_gain_prop, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool GigECCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s x=%i y=%i w=%i h=%i", __PRETTY_FUNCTION__, x, y, w, h);

    this->camera->set_geometry(x, y, w, h);
    return this->_update_geometry();
}

bool GigECCD::UpdateCCDBin(int binx, int biny)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s binx=%i biny=%i", __PRETTY_FUNCTION__, binx, biny);
    camera->set_bin(binx, biny);
    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

bool GigECCD::UpdateCCDFrameType(CCDChip::CCD_FRAME fType)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "%s", __PRETTY_FUNCTION__);
    PrimaryCCD.setFrameType(fType);
    return true;
}
