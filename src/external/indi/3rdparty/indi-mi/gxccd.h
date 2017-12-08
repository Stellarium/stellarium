/*
 * The Moravian Instruments (MI) camera library.
 *
 * Copyright 2016, Moravian Instruments <http://www.gxccd.com, linux@gxccd.com>
 * All rights reserved.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Default ETH Adapter IP address and port.
 * You can change these in adapter's web configuration and set path to your
 * config file with gxccd_configure() or pass new values to gxccd_configure_eth().
 */
#define GXETH_DEFAULT_IP   "192.168.0.5"
#define GXETH_DEFAULT_PORT 48899

/*
 * Data types used in this driver library.
 * Following types are defined in "stdint.h".
 * typedef short int            int16_t;
 * typedef unsigned char        uint8_t;
 * typedef unsigned short int   uint16_t;
 * typedef unsigned int         uint32_t;
 */

/* Typedef for prototype of enumeration function. */
typedef void (*enum_callback_t)(int /*camera_id*/);

/* Typedef for prototype of anonymous camera structure. */
typedef struct camera camera_t;

/* =============================================================================
 * Structure of .ini configuration file.

  [driver]
  IP = 192.168.0.5
  Port = 48899
  ConnectTimeout = 1000
  SendTimeout = 3000
  ReceiveTimeout = 30000
  MicrometerFilterOffsets = false
  ClearTime = 15

  [filters]
  Luminance, LGray, 0
  Red, LRed, 330
  Green, LGreen, 330
  Blue, LBlue, 330
  Clear, Black, 330

 *------------------------------------------------------------------------------
 * "IP" and "Port" are used for ETH adapter configuration and are ignored in
 * USB variant.
 * Values in [driver] section are used by default and therefore are optional.
 * "ConnectTimeout" is used when driver is connecting to adapter (in
 * gxccd_enumerate_eth() or gxccd_initialize_eth()).
 * "SendTimeout" and "ReceiveTimeout" are used during communication with
 * adapter.
 * All these values apply in erroneous states (network failure, adapter failure,
 * etc.) and it is not necessary to change them.
 * "MicrometerFilterOffsets" is explained in the following section.
 * "ClearTime" is interval in seconds in which the driver periodically clears
 * the CCD chip. There is no need to change the default value (15 seconds).
 * If you want to use advanced USB functions below, you can turn this feature by
 * setting "ClearTime" to 0 or -1 and clear the CCD manualy with gxusb_clear().
 *------------------------------------------------------------------------------
 * Section [filters] is for G2/G3/G4 camera filter wheel configuration.
 * There is no way how to determine the actual filters in the filter wheel
 * automatically. You must create .ini file with this section.
 *
 * Every line in this section describes one filter position. Filter description
 * is a comma-separated list of three values:
 *
 *   1. Filter name:    This name is returned to the client application, which
 *                      can use it to list available filters in the filter wheel.
 *
 *   2. Filter color:   This color can be used by client application to display
 *                      the filter name with a color, hinting the filter type.
 *                      The color can be expressed by a name (see below) or
 *                      directly by decimal number representing the particular
 *                      color (0 is black, 16777215 (0xffffff) is white).
 *
 *   3. Filter offset:  Distance to move the focuser to refocus upon filter
 *                      change. Plan-parallel glass shifts the actual focus
 *                      position back for 1/3 of the glass thickness (exact
 *                      value depends on the glass refraction index, but for
 *                      almost all glasses 1/3 is very close to exact value).
 *                      Refocusing is useful when changing filters of different
 *                      thickness among exposures or when some exposures are
 *                      performed through filters and other without filters at
 *                      all.
 *
 * Posible filter color names are Red, LRed, Blue, LBlue, Green, LGreen, Cyan,
 * LCyan, Magenta, LMagenta, Yellow, LYellow, LGray, DGray, White, Black.
 *
 * Filter offsets can be defined in focuser dependent units (steps) or in
 * micrometers (μm). If the micrometers are used, it is necessary to inform
 * driver by the "MicrometerFilterOffsets" parameter in the [driver] section
 * of the ini file. Value of the "MicrometerFilterOffsets" parameter can be
 * expressed as keywords "true" or "false" (default) as well as numbers 1 (for
 * true) or 0 (for false).
 * ===========================================================================*/

/*
 * An attempt to load .ini configuration file is made during enumeration and
 * initialization. If the configuration file is not set, the code looks for
 * "gxccd.ini" or "gxccd.camera_id.ini" (for example "gxccd.1111.ini", where
 * "camera_id" is 1111) in .config directory located in current user's home
 * directory (e.g. /home/username/.config/gxccd.ini) or in directory with
 * application executable binary.
 * If none of these files is found, default values (mentioned above) are used.
 */

/*
 * Sets global path to your configuration .ini file.
 */
void gxccd_configure(const char *ini_path);

/*
 * Configures ip address and/or port of ethernet adapter.
 * To configure port only, pass NULL or empty string in "ip".
 * To configure ip address only, pass 0 in "port".
 */
void gxccd_configure_eth(const char *ip, uint16_t port);

/*
 * Enumerates all cameras currently connected to your computer (_usb) or the
 * ethernet adapter (_eth).
 * You have to provide callback function, which is called for each connected
 * camera and the camera identifier (camera ID) is passed as an argument to this
 * callback function.
 * If your application is designed to work with one camera only or the camera
 * identifier is known, gxccd_enumerate_*() needs not to be called at all.
 */
/* USB variant */
void gxccd_enumerate_usb(enum_callback_t callback);
/* Ethernet variant */
void gxccd_enumerate_eth(enum_callback_t callback);

/*
 * Driver is designed to handle multiple cameras at once. It distinguishes
 * individual cameras using pointer to individual instances.
 * This function returns pointer to initialized structure or NULL in case of
 * error.
 * "camera_id" is camera indentifier (e.g. obtained by gxccd_enumerate_*()
 * function) and is required. If you have one camera connected, you can pass -1
 * as "camera_id".
 */
/* USB variant */
camera_t *gxccd_initialize_usb(int camera_id);
/* Ethernet variant */
camera_t *gxccd_initialize_eth(int camera_id);

/*
 * Disconnects from camera and releases other resources. The memory pointed by
 * "camera" becomes invalid and you must not pass it to any of the following
 * functions!
 */
void gxccd_release(camera_t *camera);

/*==============================================================================
 * RULES FOR THE FOLLOWING FUNCTIONS:
 * 1. Each function works with initialized camera_t structure.
 * 2. Every parameter is required.
 * 3. On success returns 0, on error the value -1 is returned and application
 *    can obtain error explanation by calling gxccd_get_last_error().
 *============================================================================*/

/* Standard gxccd_get_boolean_parameter() indexes */
enum
{
    GBP_CONNECTED = 0,             /* true if camera currently connected */
    GBP_SUB_FRAME,                 /* true if camera supports sub-frame read */
    GBP_READ_MODES,                /* true if camera supports multiple read modes */
    GBP_SHUTTER,                   /* true if camera is equipped with mechanical shutter */
    GBP_COOLER,                    /* true if camera is equipped with active CCD cooler */
    GBP_FAN,                       /* true if camera fan can be controlled */
    GBP_FILTERS,                   /* true if camera controls filter wheel */
    GBP_GUIDE,                     /* true if camera is capable to guide the telescope
                             mount */
    GBP_WINDOW_HEATING,            /* true if camera can control the CCD window heating */
    GBP_PREFLASH,                  /* true if camera can use CCD preflash */
    GBP_ASYMMETRIC_BINNING,        /* true if camera horizontal and vertical binning
                             can differ */
    GBP_MICROMETER_FILTER_OFFSETS, /* true if filter focusing offsets are
                                    expressed in micrometers */
    GBP_POWER_UTILIZATION,         /* true if camera can return power utilization in
                             gxccd_get_value() */
    GBP_GAIN,                      /* true if camera can return gain in gxccd_get_value() */
    GBP_CONFIGURED = 127,          /* true if camera is configured */
    GBP_RGB,                       /* true if camera has Bayer RGBG filters on CCD */
    GBP_CMY,                       /* true if camera has CMY filters on CCD */
    GBP_CMYG,                      /* true if camera has CMYG filters on CCD */
    GBP_DEBAYER_X_ODD,             /* true if camera Bayer masks starts on horizontal
                             odd pixel */
    GBP_DEBAYER_Y_ODD,             /* true if camera Bayer masks starts on vertical
                             odd pixel */
    GBP_INTERLACED = 256           /* true if CCD detector is interlaced
                             (else progressive) */
};

/* Returns true or false in "value" depending on the "index". */
int gxccd_get_boolean_parameter(camera_t *camera, int index, bool *value);

/* Standard gxccd_get_integer_parameter() indexes */
enum
{
    GIP_CAMERA_ID = 0,        /* Identifier of the current camera */
    GIP_CHIP_W,               /* CCD detector width in pixels */
    GIP_CHIP_D,               /* CCD detector depth in pixels */
    GIP_PIXEL_W,              /* CCD pixel width in nanometers */
    GIP_PIXEL_D,              /* CCD pixel depth in nanometers */
    GIP_MAX_BINNING_X,        /* Maximum binning in horizontal direction */
    GIP_MAX_BINNING_Y,        /* Maximum binning in vertical direction */
    GIP_READ_MODES,           /* Number of read modes offered by the camera */
    GIP_FILTERS,              /* Number of filters offered by the camera */
    GIP_MINIMAL_EXPOSURE,     /* Shortest exposure time in microseconds (µs) */
    GIP_MAXIMAL_EXPOSURE,     /* Longest exposure time in milliseconds (ms) */
    GIP_MAXIMAL_MOVE_TIME,    /* Longest time to move the telescope in
                                 milliseconds (ms) */
    GIP_DEFAULT_READ_MODE,    /* Read mode to be used as default */
    GIP_PREVIEW_READ_MODE,    /* Read mode to be used for preview (fast read) */
    GIP_MAX_WINDOW_HEATING,   /* Maximal value for gxccd_set_window_heating() */
    GIP_MAX_FAN,              /* Maximal value for gxccd_set_fan() */
    GIP_FIRMWARE_MAJOR = 128, /* Camera firmware version (optional) */
    GIP_FIRMWARE_MINOR,
    GIP_FIRMWARE_BUILD,
    GIP_DRIVER_MAJOR, /* Driver version (optional) */
    GIP_DRIVER_MINOR,
    GIP_DRIVER_BUILD,
    GIP_FLASH_MAJOR, /* Flash version (optional) */
    GIP_FLASH_MINOR,
    GIP_FLASH_BUILD,
};

/* Returns integer in "value" depending on the "index". */
int gxccd_get_integer_parameter(camera_t *camera, int index, int *value);

/* Standard gxccd_get_string_parameter() indexes */
enum
{
    GSP_CAMERA_DESCRIPTION = 0, /* Camera description */
    GSP_MANUFACTURER,           /* Manufacturer name */
    GSP_CAMERA_SERIAL,          /* Camera serial number */
    GSP_CHIP_DESCRIPTION        /* Used CCD detector description */
};

/*
 * Returns string in "buf" depending on the "index".
 * The caller must specify the size of the buffer in "size".
 */
int gxccd_get_string_parameter(camera_t *camera, int index, char *buf, size_t size);

/* Standard gxccd_get_value() indexes */
enum
{
    GV_CHIP_TEMPERATURE = 0,    /* Current temperature of the CCD detector
                                 in deg. Celsius */
    GV_HOT_TEMPERATURE,         /* Current temperature of the cooler hot side
                                 in deg. Celsius */
    GV_CAMERA_TEMPERATURE,      /* Current temperature inside the camera
                                 in deg. Celsius */
    GV_ENVIRONMENT_TEMPERATURE, /* Current temperature of the environment air
                                 in deg. Celsius */
    GV_SUPPLY_VOLTAGE = 10,     /* Current voltage of the camera power supply */
    GV_POWER_UTILIZATION,       /* Current utilization of the CCD cooler
                                 (rational number from 0.0 to 1.0) */
    GV_ADC_GAIN = 20            /* Current gain of A/D converter in electrons/ADU */
};

/* Returns float in "value" depending on the "index". */
int gxccd_get_value(camera_t *camera, int index, float *value);

/*
 * Sets the required chip temperature.
 * If the camera has no cooler, this function has no effect.
 * "temp" is expressed in degrees Celsius.
 */
int gxccd_set_temperature(camera_t *camera, float temp);

/*
 * Sets the maximum speed with which the driver changes chip temperature.
 * If the camera has no cooler, this function has no effect.
 * "temp_ramp" is expressed in degrees Celsius per minute.
 */
int gxccd_set_temperature_ramp(camera_t *camera, float temp_ramp);

/*
 * Sets the required read binning.
 * If the camera does not support binning, this function has no effect.
 */
int gxccd_set_binning(camera_t *camera, int x, int y);

/*
 * If the camera is equipped with preflash electronics, this function sets it.
 * "preflash_time" defines time for which the preflash LED inside the camera is
 * switched on. "clear_num" defines how many times the CCD has to be cleared
 * after the preflash. Actual values of these parameters depends on
 * the particular camera model (e.g. number and luminance of the LEDs used etc.).
 * Gx series of CCD cameras typically need less than 1 second to completely
 * saturate the CCD ("preflash_time"). Number of subsequent clears should be
 * at last 2, but more than 4 or 5 clears is not useful, too.
 */
int gxccd_set_preflash(camera_t *camera, double preflash_time, int clear_num);

/*
 * Starts new exposure.
 * "exp_time" is the required exposure time in seconds. "use_shutter" parameter
 * tells the driver the dark frame (without light) has to be acquired (false),
 * or the shutter has to be opened and closed to acquire normal light image (true).
 * Sub-frame coordinates are passed in "x", "y", "w" and "h".
 * If the camera does not support sub-frame read, "x" and "y" must be 0 and "w"
 * and "h" must be the chip pixel dimensions.
 * The y-axis grows down, 0 is at the top.
 */
int gxccd_start_exposure(camera_t *camera, double exp_time, bool use_shutter, int x, int y, int w, int h);

/*
 * When the exposure already started by gxccd_start_exposure() call has to be
 * terminated before the exposure time expires, this function has to be called.
 * Parameter "download" indicates whether the image should be digitized, because
 * the user will call gxccd_read_image() later or the image should be discarded.
 */
int gxccd_abort_exposure(camera_t *camera, bool download);

/*
 * When the exposure already started by gxccd_start_exposure() call, parameter
 * "ready" is false if the exposure is sill running. When the exposure finishes
 * and it is possible to call gxccd_read_image(), parameter "ready" is true.
 * It is recommended to count the exposure time in the application despite
 * the fact the exact exposure time is determined by the camera/driver and to
 * start calling of gxccd_image_ready() only upon the exposure time expiration.
 * Starting to call gxccd_image_ready() in the infinite loop immediately after
 * gxccd_start_exposure() and call it for the whole exposure time (and thus
 * keeping at last one CPU core at 100% utilization) is a bad programming
 * manner (politely expressed).
 */
int gxccd_image_ready(camera_t *camera, bool *ready);

/*
 * When gxccd_image_ready() returns "ready" == true, it is possible to call
 * gxccd_read_image(). Driver returns 16 bits per pixel (2 bytes) matrix copied
 * to "buf" address. The buffer must be allocated by the caller, driver does not
 * allocate any memory. The "size" parameter specifies allocated memory block
 * length in bytes (not in pixels!). It has to be greater or equal to image size
 * in bytes else the function fails.
 * Application can use: size = wanted_w * 2 * wanted_h;
 *
 * Format of the returned buffer:
 *   - one-dimensional array formed from lines (rows) stacked one after another
 *   - orientation of the image is similar to Cartesian coordinate system,
 *     pixel [0, 0] (first line) is located at bottom left of the resulting image,
 *     x coordinate grows right and y coordinate grows up
 *   - data is in little-endian encoding -> lower byte first
 *
 * Example with width = 2000px and height = 1000px:
 *   - allocate one-dimensional buffer: malloc(2000*2*1000) (malloc(width*2*height))
 *   - bottom left pixel's lower byte is located at buffer[0] and higher byte is
 *     at buffer[1]
 *   - first line has width = 2000 * 2 (bytes) -> bottom right pixel is located
 *     at buffer[3998] and buffer[3999] (width * 2 - 2 and width * 2 - 1)
 *   - top left pixel is at buffer[3996000] and buffer[3996001]
 *     ((height - 1) * width * 2 and (height - 1) * width * 2 + 1)
 *   - top right pixel is at buffer[3999998] and buffer[3999999]
 *     ((height - 1) * width * 2 + width * 2 - 2 and (height - 1) * width * 2
 *     + width * 2 - 1)
 */
int gxccd_read_image(camera_t *camera, void *buf, size_t size);

/*
 * Enumerates all read modes provided by the camera.
 * This enumeration does not use any callback, the caller passes index
 * beginning with 0 and repeats the call with incremented index until the call
 * returns -1.
 * The caller must specify the size of the buffer in parameter "size".
 */
int gxccd_enumerate_read_modes(camera_t *camera, int index, char *buf, size_t size);

/*
 * Sets required read mode.
 * "mode" is the "index" used in gxccd_enumerate_read_modes() call.
 */
int gxccd_set_read_mode(camera_t *camera, int mode);

/*
 * Enumerates all filters provided by the camera.
 * This enumeration does not use any callback, by the caller passes index
 * beginning with 0 and repeats the call with incremented index until the call
 * returns -1.
 * Returns filter name in "buf". The caller must specify the size of the buffer
 * in parameter size.
 * "color" parameter hints the RGB color (e.g. cyan color is 0x00ffff), which
 * can be used to draw the filter name in the application.
 * "offset" indicates the focuser shift when the particular filter is selected.
 * Units of the "offset" can be micrometers or arbitrary focuser specific units
 * (steps). If the units used are micrometers, driver returns true from
 * gxccd_get_boolean_parameter() with GBP_MICROMETER_FILTER_OFFSETS "index".
 */
int gxccd_enumerate_filters(camera_t *camera, int index, char *buf, size_t size, uint32_t *color, int *offset);

/*
 * Sets the required filter.
 * If the camera is not equipped with filter wheel, this function has no effect.
 */
int gxccd_set_filter(camera_t *camera, int index);

/*
 * If the camera is equipped with cooling fan and allows its control,
 * this function sets the fan rotation speed.
 * The maximum value of the "speed" parameter should be determined by
 * gxccd_get_integer_parameter() call with GIP_MAX_FAN "index".
 * If the particular camera supports only on/off switching, the maximum value
 * should be 1 (fan on), while value 0 means fan off.
 */
int gxccd_set_fan(camera_t *camera, uint8_t speed);

/*
 * If the camera is equipped with CCD cold chamber front window heater
 * and allows its control, this function sets heating intensity.
 * The maximum value of the "heating" parameter should be determined by
 * gxccd_get_integer_parameter() call with GIP_MAX_WINDOW_HEATING "index".
 * If the particular camera supports only on/off switching, the maximum value
 * should be 1 (heating on), while value 0 means heating off.
 */
int gxccd_set_window_heating(camera_t *camera, uint8_t heating);

/*
 * Instructs the camera to initiate telescope movement in the R.A. and/or Dec.
 * axis for the defined period of time (in milliseconds).
 * The sign of the parameters defines the movement direction in the respective
 * coordinate. The maximum length is approx 32.7 seconds.
 * If the camera is not equipped with autoguider port, this function has no
 * effect.
 */
int gxccd_move_telescope(camera_t *camera, int16_t ra_duration_ms, int16_t dec_duration_ms);

/*
 * Sets "moving" to true if the movement started with gxccd_move_telescope()
 * call is still in progress.
 */
int gxccd_move_in_progress(camera_t *camera, bool *moving);

/*
 * If any call fails (returns -1), this function returns failure description
 * in parameter buf.
 * The caller must specify the size of the buffer in parameter "size".
 */
void gxccd_get_last_error(camera_t *camera, char *buf, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
