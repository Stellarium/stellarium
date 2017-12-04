/*
 GigE interface wrapper on aravis
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
#ifndef CPP_ARV_IFACE_H
#define CPP_ARV_IFACE_H

#include <stdint.h>
#include <stddef.h>

namespace arv
{
typedef enum {
    ARV_EXPOSURE_UNKNOWN  = -1, //!< Unknown status?
    ARV_EXPOSURE_FINISHED = 0,  //!< Exposure has finished, image returned
    ARV_EXPOSURE_BUSY,          //!< Exposure is still ungoing
    ARV_EXPOSURE_FILLING,       //!< Exposure is finished, being transferred on the lower layer
    ARV_EXPOSURE_FAILED,        //!< Exposure has finished, with an error, no image returned.

} ARV_EXPOSURE_STATUS;

template <class T>
class min_max_property
{
  public:
    min_max_property() {}
    min_max_property(T const min, T const max, T const val)
    {
        this->_min = min;
        this->_max = max;
        this->_val = val;
    }

    void update(T const min, T const max)
    {
        this->_min = min;
        this->_max = max;
    }

    void set(T const new_val)
    {
        if (new_val > this->_max)
            this->_val = this->_max;
        else if (new_val < this->_min)
            this->_val = this->_min;
        else
            this->_val = new_val;
    }
    void set_single(T const single_val) { this->_min = this->_max = this->_val = single_val; }

    T val() { return this->_val; }
    T min() { return this->_min; }
    T max() { return this->_max; }

  private:
    T _min, _max, _val;
};

class ArvCamera
{
  public:
    ArvCamera(void *camera_device) {}
    virtual bool connect()      = 0;
    virtual bool disconnect()   = 0;
    virtual bool is_connected() = 0;
    virtual bool is_exposing()  = 0;

    /* Get properties */
    virtual const char *vendor_name()                  = 0;
    virtual const char *model_name()                   = 0;
    virtual const char *device_id()                    = 0;
    virtual int get_frame_byte_size()                  = 0;
    virtual min_max_property<int> get_bin_x()          = 0;
    virtual min_max_property<int> get_bin_y()          = 0;
    virtual min_max_property<int> get_x_offset()       = 0;
    virtual min_max_property<int> get_y_offset()       = 0;
    virtual min_max_property<int> get_width()          = 0;
    virtual min_max_property<int> get_height()         = 0;
    virtual min_max_property<int> get_bpp()            = 0;
    virtual min_max_property<double> get_pixel_pitch() = 0;
    virtual min_max_property<double> get_exposure()    = 0;
    virtual min_max_property<double> get_gain()        = 0;
    virtual min_max_property<double> get_frame_rate()  = 0;

    /* Set geometry */
    virtual void set_bin(int const bin_x, int const bin_y)                        = 0;
    virtual void set_geometry(int const x, int const y, int const w, int const h) = 0;
    virtual void update_geometry(void)                                            = 0;

    /* Set exposure */
    virtual void set_exposure_time(double const val) = 0;
    virtual void set_gain(double const val)          = 0;

    virtual void exposure_start(void)                      = 0;
    virtual void exposure_abort(void)                      = 0;
    virtual ARV_EXPOSURE_STATUS exposure_poll(void (*fn_image_callback)(void *const, uint8_t const *const, size_t),
                                              void *const) = 0;
};

class ArvFactory
{
  public:
    static ArvCamera *find_first_available(void);

    //TODO: add iterative support to add all discovered cameras
};

} /* Namepsace */

#endif
