/*
 * File: openssag.h
 *
 * Copyright (c) 2011 Eric J. Holmes, Orion Telescopes & Binoculars
 *
 * Migration to libusb 1.0 by Peter Polakovic
 *
 */

#ifndef __OPEN_SSAG_H__
#define __OPEN_SSAG_H__

/* Orion Telescopes SSAG VID/PID */
#define SSAG_VENDOR_ID  0x1856
#define SSAG_PRODUCT_ID 0x0012

/* uninitialized SSAG VID/PID */
#define SSAG_LOADER_VENDOR_ID  0x1856
#define SSAG_LOADER_PRODUCT_ID 0x0011

/* uninitialized QHY5 VID/PID */
#define QHY5_LOADER_VENDOR_ID  0x1618
#define QHY5_LOADER_PRODUCT_ID 0x0901

typedef struct libusb_device_handle libusb_device_handle;

#ifdef __cplusplus
namespace OpenSSAG
{
#endif
/* Struct used to return image data */
struct raw_image
{
    /* Image height */
    unsigned int width;
    /* Image width */
    unsigned int height;
    /* Pointer to the data. Length should be height * width */
    unsigned char *data;
};

/* Guide Directions (cardinal directions) */
enum guide_direction
{
    guide_east  = 0x10,
    guide_south = 0x20,
    guide_north = 0x40,
    guide_west  = 0x80,
};

struct device_info
{
    /* Null terminated string consisting of the serial number */
    char serial[256];
    /* Next device in list */
    struct device_info *next;
};
#ifdef __cplusplus
class SSAG
{
  private:
    /* Sets buffer mode...or something like that */
    void SetBufferMode();

    /* Sends init packet and pre expose request */
    void InitSequence();

    /* Holds the converted gain */
    unsigned int gain;

    /* Handle to the device */
    libusb_device_handle *handle;

  public:
    /* Constructor */
    SSAG();

    /* Connect to the autoguider. If bootload is set to true and the camera
         * cannot be found, it will attempt to connect to the base device and
         * load the firmware. Defaults to true. */
    bool Connect(bool bootload);
    bool Connect();

    /* Disconnect from the autoguider */
    void Disconnect();

    /* Returns true if the device is currently connected. */
    bool IsConnected();

    /* Gain should be a value between 1 and 15 */
    void SetGain(int gain);

    /* Expose and return the image in raw gray format. Function is blocking. */
    struct raw_image *Expose(int duration);

    /* Start exposure in raw gray format. Function is non-blocking. */
    void StartExposure(int duration);

    /* Cancels an exposure */
    void CancelExposure();

    /* Gets the data from the autoguider's internal buffer */
    unsigned char *ReadBuffer(int timeout);

    /* Issue a guide command through the guider relays. Guide directions
         * can be OR'd together to move in X and Y at the same time.
         *
         * EX. Guide(guide_north | guide_west, 100, 200); */
    void Guide(int direction, int yduration, int xduration);
    void Guide(int direction, int duration);

    /* Frees a raw_image struct */
    void FreeRawImage(raw_image *image);
};

/* This class is used for loading the firmware onto the device after it is
     * plugged in.
     *
     * See Cypress EZUSB fx2 datasheet for more information
     * http://www.keil.com/dd/docs/datashts/cypress/fx2_trm.pdf */
class Loader
{
  private:
    /* Puts the device into reset mode by writing 0x01 to CPUCS */
    void EnterResetMode();

    /* Makes the device exit reset mode by writing 0x00 to CPUCS */
    void ExitResetMode();

    /* Sends firmware to the device */
    bool Upload(unsigned char *data);

    /* Handle to the cypress device */
    libusb_device_handle *handle;

  public:
    /* Connects to SSAG Base */
    bool Connect();

    /* Disconnects from SSAG Base */
    void Disconnect();

    /* Loads the firmware into SSAG's RAM */
    bool LoadFirmware();

    /* Loads the SSAG eeprom onto the camera. You shouldn't use this if you
         * don't know what you're doing.
         * See http://www.cypress.com/?id=4&rID=34127 for more information. */
    bool LoadEEPROM();
};
}
#endif // __cplusplus

#endif /* __OPEN_SSAG_H__ */
