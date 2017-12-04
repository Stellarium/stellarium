/*
 GigE interface wrapper on araviss
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
#include "ArvGeneric.h"

using namespace arv;

const char *ArvGeneric::_str_val(const char *s)
{
    return (s ? s : "None");
}
const char *ArvGeneric::vendor_name()
{
    return this->_str_val(this->cam.vendor_name);
}
const char *ArvGeneric::model_name()
{
    return this->_str_val(this->cam.model_name);
}
const char *ArvGeneric::device_id()
{
    return this->_str_val(this->cam.device_id);
}
min_max_property<int> ArvGeneric::get_bin_x()
{
    return min_max_property<int>(this->cam.bin_x);
}
min_max_property<int> ArvGeneric::get_bin_y()
{
    return min_max_property<int>(this->cam.bin_y);
}
min_max_property<int> ArvGeneric::get_x_offset()
{
    return min_max_property<int>(this->cam.x_offset);
}
min_max_property<int> ArvGeneric::get_y_offset()
{
    return min_max_property<int>(this->cam.y_offset);
}
min_max_property<int> ArvGeneric::get_width()
{
    return min_max_property<int>(this->cam.width);
}
min_max_property<int> ArvGeneric::get_height()
{
    return min_max_property<int>(this->cam.height);
}
min_max_property<int> ArvGeneric::get_bpp()
{
    return min_max_property<int>(16, 16, 16);
}
min_max_property<double> ArvGeneric::get_pixel_pitch()
{
    return min_max_property<double>(this->cam.pixel_pitch);
}
min_max_property<double> ArvGeneric::get_exposure()
{
    return min_max_property<double>(this->cam.exposure);
}
min_max_property<double> ArvGeneric::get_gain()
{
    return min_max_property<double>(this->cam.gain);
}
min_max_property<double> ArvGeneric::get_frame_rate()
{
    return min_max_property<double>(this->cam.frame_rate);
}

template <typename T>
bool ArvGeneric::_get_bounds(void (*fn_arv_bounds)(::ArvCamera *, T *min, T *max), min_max_property<T> *prop)
{
    T min, max;
    fn_arv_bounds(this->camera, &min, &max);
    prop->update(min, max);
}

bool ArvGeneric::is_exposing()
{
    return this->stream_active;
}
bool ArvGeneric::is_connected()
{
    return (this->camera ? true : false);
}
bool ArvGeneric::_stream_active()
{
    return this->stream_active;
}

ArvGeneric::ArvGeneric(void *camera_device) : ArvCamera(camera_device)
{
    this->_init();
    this->camera = (::ArvCamera *)camera_device;
    this->dev    = arv_camera_get_device(this->camera);

    this->cam.model_name  = arv_camera_get_model_name(this->camera);
    this->cam.vendor_name = arv_camera_get_vendor_name(this->camera);
    this->cam.device_id   = arv_camera_get_device_id(this->camera);
}

ArvGeneric::~ArvGeneric()
{
    if (this->is_connected())
    {
        this->disconnect();
    }
    this->_init();
}

bool ArvGeneric::connect()
{
    /* (Re-)connect by means of the device-id */
    if (!this->camera)
    {
        this->camera = ::arv_camera_new(this->cam.device_id);
        if (!this->camera)
            return false;

        this->dev             = arv_camera_get_device(this->camera);
        this->cam.model_name  = arv_camera_get_model_name(this->camera);
        this->cam.vendor_name = arv_camera_get_vendor_name(this->camera);
        this->cam.device_id   = arv_camera_get_device_id(this->camera);
    }
    return true;
}

bool ArvGeneric::_configure(void)
{
    this->_set_initial_config();
    return this->_get_initial_config();
}

void ArvGeneric::_init()
{
    this->camera        = NULL;
    this->buffer        = NULL;
    this->stream        = NULL;
    this->stream_active = false;

    /* Don't clear device_id, its needed to re-attach with connect() */
}

bool ArvGeneric::disconnect()
{
    if (this->is_connected())
    {
        this->_test_exposure_and_abort();
        g_clear_object(&this->camera);
    }
    this->_init();
}

bool ArvGeneric::_set_initial_config()
{
    /* Configure "manual" mode
     *      (1) disable auto exposure
     *      (2) disable auto framerate (to enable maximum possible exposure time)
     *      (3) set binning to 1x1
     *      (4) set software trigger */
    arv_camera_set_binning(camera, 1, 1);
    arv_camera_set_gain_auto(camera, ARV_AUTO_OFF);
    arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF);
    arv_camera_set_trigger(camera, "Software");
    return true;
}

bool ArvGeneric::_get_initial_config()
{
    this->_get_bounds<gint>(arv_camera_get_x_binning_bounds, &this->cam.bin_x);
    this->_get_bounds<gint>(arv_camera_get_y_binning_bounds, &this->cam.bin_y);
    this->_get_bounds<gint>(arv_camera_get_x_offset_bounds, &this->cam.x_offset);
    this->_get_bounds<gint>(arv_camera_get_y_offset_bounds, &this->cam.y_offset);
    this->_get_bounds<gint>(arv_camera_get_width_bounds, &this->cam.width);
    this->_get_bounds<gint>(arv_camera_get_height_bounds, &this->cam.height);
    this->_get_bounds<double>(arv_camera_get_frame_rate_bounds, &this->cam.frame_rate);
    this->_get_bounds<double>(arv_camera_get_exposure_time_bounds, &this->cam.exposure);
    this->_get_bounds<double>(arv_camera_get_gain_bounds, &this->cam.gain);

    this->cam.vendor_name = arv_camera_get_vendor_name(camera);
    this->cam.model_name  = arv_camera_get_model_name(camera);
    this->cam.device_id   = arv_camera_get_device_id(camera);

    /* No GVCP call for this..., specialize if necessary */
    this->cam.pixel_pitch.set_single(1.0);

    return true;
}

int ArvGeneric::get_frame_byte_size()
{
    return arv_camera_get_payload(this->camera);
}

void ArvGeneric::set_geometry(int const x, int const y, int const w, int const h)
{
    this->cam.x_offset.set(x);
    this->cam.y_offset.set(y);
    this->cam.width.set(w);
    this->cam.height.set(h);

    arv_camera_set_region(this->camera, this->cam.x_offset.val(), this->cam.y_offset.val(), this->cam.width.val(),
                          this->cam.height.val());
}

void ArvGeneric::update_geometry(void)
{
    gint x, y, w, h, binx, biny;

    arv_camera_get_region(this->camera, &x, &y, &w, &h);
    arv_camera_get_binning(this->camera, &binx, &biny);

    this->cam.x_offset.set(x);
    this->cam.y_offset.set(y);
    this->cam.width.set(w);
    this->cam.height.set(h);
    this->cam.bin_x.set(binx);
    this->cam.bin_y.set(biny);
}

void ArvGeneric::set_bin(int const bin_x, int const bin_y)
{
    this->cam.bin_x.set(bin_x);
    this->cam.bin_y.set(bin_y);

    arv_camera_set_binning(this->camera, this->cam.bin_x.val(), this->cam.bin_y.val());
}

void ArvGeneric::_test_exposure_and_abort(void)
{
    if (this->_stream_active())
        this->exposure_abort();
}

template <typename T>
void ArvGeneric::_set_cam_exposure_property(void (*arv_set)(::ArvCamera *, T), min_max_property<T> *prop,
                                            T const new_val)
{
    this->_test_exposure_and_abort();
    prop->set(new_val);
    arv_set(this->camera, prop->val());
}

void ArvGeneric::set_gain(double const val)
{
    this->_set_cam_exposure_property(arv_camera_set_gain, &this->cam.gain, val);
}
void ArvGeneric::set_exposure_time(double const val)
{
    this->_set_cam_exposure_property(arv_camera_set_exposure_time, &this->cam.exposure, val);
}

::ArvBuffer *ArvGeneric::_buffer_create(void)
{
    ::ArvBuffer *buffer;

    /* Ensure no buffers in stream */
    while (1)
    {
        buffer = arv_stream_try_pop_buffer(this->stream);
        if (buffer)
            g_clear_object(&buffer);
        else
            break;
    }

    gint const payload = arv_camera_get_payload(this->camera);
    buffer             = arv_buffer_new(payload, NULL);
    arv_stream_push_buffer(this->stream, buffer);
    return buffer;
}

::ArvStream *ArvGeneric::_stream_create(void)
{
    ::ArvStream *stream = arv_camera_create_stream(this->camera, NULL, NULL);
    return stream;
}

void ArvGeneric::_stream_start()
{
    this->stream_active = true;

    /* Start the acquisition stream */
    arv_camera_set_acquisition_mode(this->camera, ARV_ACQUISITION_MODE_SINGLE_FRAME);
    arv_camera_start_acquisition(this->camera);
}

void ArvGeneric::_stream_stop()
{
    /* stop the acquisition stream */
    arv_camera_stop_acquisition(this->camera);
    g_object_unref(this->stream);

    this->stream_active = false;
}

void ArvGeneric::_trigger_exposure()
{
    /* Trigger for an exposure */
    arv_camera_software_trigger(this->camera);
}

void ArvGeneric::exposure_start(void)
{
    this->_test_exposure_and_abort();
    this->stream = this->_stream_create();
    this->buffer = this->_buffer_create();

    this->_stream_start();
    this->_trigger_exposure();
}

void ArvGeneric::exposure_abort(void)
{
    if (this->_stream_active())
    {
        arv_camera_abort_acquisition(this->camera);
        this->_stream_stop();
    }
}

void ArvGeneric::_get_image(void (*fn_image_callback)(void *const, uint8_t const *const, size_t), void *const usr_ptr)
{
    ArvBuffer *const popped_buf = arv_stream_timeout_pop_buffer(this->stream, 100000);
    if ((popped_buf != NULL) && (popped_buf == this->buffer) &&
        arv_buffer_get_status(this->buffer) == ARV_BUFFER_STATUS_SUCCESS)
    {
        if (fn_image_callback != NULL)
        {
            size_t size;
            uint8_t const *const data = (uint8_t const *const)arv_buffer_get_data(this->buffer, &size);
            fn_image_callback(usr_ptr, data, size);
        }
    }
    else
    {
        //TODO: failure...
    }
}

ARV_EXPOSURE_STATUS ArvGeneric::exposure_poll(void (*fn_image_callback)(void *const, uint8_t const *const, size_t),
                                              void *const usr_ptr)
{
    if (!this->_stream_active())
        return ARV_EXPOSURE_UNKNOWN;

    ::ArvBufferStatus const status = arv_buffer_get_status(this->buffer);
    switch (status)
    {
        case ARV_BUFFER_STATUS_CLEARED:
            return ARV_EXPOSURE_BUSY;
        case ARV_BUFFER_STATUS_FILLING:
            return ARV_EXPOSURE_FILLING;
        case ARV_BUFFER_STATUS_UNKNOWN:
            return ARV_EXPOSURE_UNKNOWN;
        case ARV_BUFFER_STATUS_SUCCESS:
            this->_get_image(fn_image_callback, usr_ptr);
            this->_stream_stop();
            return ARV_EXPOSURE_FINISHED;
        case ARV_BUFFER_STATUS_TIMEOUT:
        case ARV_BUFFER_STATUS_MISSING_PACKETS:
        case ARV_BUFFER_STATUS_WRONG_PACKET_ID:
        case ARV_BUFFER_STATUS_SIZE_MISMATCH:
        case ARV_BUFFER_STATUS_ABORTED:
            this->_stream_stop();
            return ARV_EXPOSURE_FAILED;
        default:
            return ARV_EXPOSURE_UNKNOWN;
    }
}
