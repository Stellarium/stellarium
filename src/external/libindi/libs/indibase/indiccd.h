/*******************************************************************************
 Copyright(c) 2010, 2011 Gerry Rozema, Jasem Mutlaq. All rights reserved.

 Rapid Guide support added by CloudMakers, s. r. o.
 Copyright(c) 2013 CloudMakers, s. r. o. All rights reserved.

 Star detection algorithm is based on PHD Guiding by Craig Stark
 Copyright (c) 2006-2010 Craig Stark. All rights reserved.

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

#include "defaultdevice.h"

#include "indiguiderinterface.h"

#include <fitsio.h>

#include <memory>
#include <cstring>

#include <stdint.h>

extern const char *IMAGE_SETTINGS_TAB;
extern const char *IMAGE_INFO_TAB;
extern const char *GUIDE_HEAD_TAB;
extern const char *RAPIDGUIDE_TAB;

namespace INDI
{

class StreamManager;

/**
 * @brief The CCDChip class provides functionality of a CCD Chip within a CCD.
 */
class CCDChip
{
  public:
    CCDChip();
    ~CCDChip();

    typedef enum { LIGHT_FRAME = 0, BIAS_FRAME, DARK_FRAME, FLAT_FRAME } CCD_FRAME;
    typedef enum { FRAME_X, FRAME_Y, FRAME_W, FRAME_H } CCD_FRAME_INDEX;
    typedef enum { BIN_W, BIN_H } CCD_BIN_INDEX;
    typedef enum {
        CCD_MAX_X,
        CCD_MAX_Y,
        CCD_PIXEL_SIZE,
        CCD_PIXEL_SIZE_X,
        CCD_PIXEL_SIZE_Y,
        CCD_BITSPERPIXEL
    } CCD_INFO_INDEX;

    /**
     * @brief getXRes Get the horizontal resolution in pixels of the CCD Chip.
     * @return the horizontal resolution of the CCD Chip.
     */
    inline int getXRes() { return XRes; }

    /**
     * @brief Get the vertical resolution in pixels of the CCD Chip.
     * @return the horizontal resolution of the CCD Chip.
     */
    inline int getYRes() { return YRes; }

    /**
     * @brief getSubX Get the starting left coordinates (X) of the frame.
     * @return the starting left coordinates (X) of the image.
     */
    inline int getSubX() { return SubX; }

    /**
     * @brief getSubY Get the starting top coordinates (Y) of the frame.
     * @return the starting top coordinates (Y) of the image.
     */
    inline int getSubY() { return SubY; }

    /**
     * @brief getSubW Get the width of the frame
     * @return unbinned width of the frame
     */
    inline int getSubW() { return SubW; }

    /**
     * @brief getSubH Get the height of the frame
     * @return unbinned height of the frame
     */
    inline int getSubH() { return SubH; }

    /**
     * @brief getBinX Get horizontal binning of the CCD chip.
     * @return horizontal binning of the CCD chip.
     */
    inline int getBinX() { return BinX; }

    /**
     * @brief getBinY Get vertical binning of the CCD chip.
     * @return vertical binning of the CCD chip.
     */
    inline int getBinY() { return BinY; }

    /**
     * @brief getPixelSizeX Get horizontal pixel size in microns.
     * @return horizontal pixel size in microns.
     */
    inline float getPixelSizeX() { return PixelSizex; }

    /**
     * @brief getPixelSizeY Get vertical pixel size in microns.
     * @return vertical pixel size in microns.
     */
    inline float getPixelSizeY() { return PixelSizey; }

    /**
     * @brief getBPP Get CCD Chip depth (bits per pixel).
     * @return bits per pixel.
     */
    inline int getBPP() { return BPP; }

    /**
     * @brief getFrameBufferSize Get allocated frame buffer size to hold the CCD image frame.
     * @return allocated frame buffer size to hold the CCD image frame.
     */
    inline int getFrameBufferSize() { return RawFrameSize; }

    /**
     * @brief getExposureLeft Get exposure time left in seconds.
     * @return exposure time left in seconds.
     */
    inline double getExposureLeft() { return ImageExposureN[0].value; }

    /**
     * @brief getExposureDuration Get requested exposure duration for the CCD chip in seconds.
     * @return requested exposure duration for the CCD chip in seconds.
     */
    inline double getExposureDuration() { return exposureDuration; }

    /**
     * @brief getExposureStartTime
     * @return exposure start time in ISO 8601 format.
     */
    const char *getExposureStartTime();

    /**
     * @brief getFrameBuffer Get raw frame buffer of the CCD chip.
     * @return raw frame buffer of the CCD chip.
     */
    inline uint8_t *getFrameBuffer() { return RawFrame; }

    /**
     * @brief setFrameBuffer Set raw frame buffer pointer.
     * @param buffer pointer to frame buffer
     * /note CCD Chip allocates the frame buffer internally once SetFrameBufferSize is called
     * with allocMem set to true which is the default behavior. If you allocated the memory
     * yourself (i.e. allocMem is false), then you must call this function to set the pointer
     * to the raw frame buffer.
     */
    void setFrameBuffer(uint8_t *buffer) { RawFrame = buffer; }

    /**
     * @brief isCompressed
     * @return True if frame is compressed, false otherwise.
     */
    inline bool isCompressed() { return SendCompressed; }

    /**
     * @brief isInterlaced
     * @return True if CCD chip is Interlaced, false otherwise.
     */
    inline bool isInterlaced() { return Interlaced; }

    /**
     * @brief getFrameType
     * @return CCD Frame type
     */
    inline CCD_FRAME getFrameType() { return FrameType; }

    /**
     * @brief getFrameTypeName returns CCD Frame type name
     * @param fType type of frame
     * @return CCD Frame type name
     */
    const char *getFrameTypeName(CCD_FRAME fType);

    /**
     * @brief Return CCD Info Property
     */
    INumberVectorProperty *getCCDInfo() { return &ImagePixelSizeNP; }

    /**
     * @brief setResolution set CCD Chip resolution
     * @param x width
     * @param y height
     */
    void setResolution(int x, int y);

    /**
     * @brief setFrame Set desired frame resolutoin for an exposure.
     * @param subx Left position.
     * @param suby Top position.
     * @param subw unbinned width of the frame.
     * @param subh unbinned height of the frame.
     */
    void setFrame(int subx, int suby, int subw, int subh);

    /**
     * @brief setBin Set CCD Chip binnig
     * @param hor Horizontal binning.
     * @param ver Vertical binning.
     */
    void setBin(int hor, int ver);

    /**
     * @brief setMinMaxStep for a number property element
     * @param property Property name
     * @param element Element name
     * @param min Minimum element value
     * @param max Maximum element value
     * @param step Element step value
     * @param sendToClient If true (default), the element limits are updated and is sent to the
     * client. If false, the element limits are updated without getting sent to the client.
     */
    void setMinMaxStep(const char *property, const char *element, double min, double max, double step,
                       bool sendToClient = true);

    /**
     * @brief setPixelSize Set CCD Chip pixel size
     * @param x Horziontal pixel size in microns.
     * @param y Vertical pixel size in microns.
     */
    void setPixelSize(float x, float y);

    /**
     * @brief setCompressed Set whether a frame is compressed after exposure?
     * @param cmp If true, compress frame.
     */
    void setCompressed(bool cmp);

    /**
     * @brief setInterlaced Set whether the CCD chip is interlaced or not?
     * @param intr If true, the CCD chip is interlaced.
     */
    void setInterlaced(bool intr);

    /**
     * @brief setFrameBufferSize Set desired frame buffer size. The function will allocate memory
     * accordingly. The frame size depends on the desired frame resolution (Left, Top, Width, Height),
     * depth of the CCD chip (bpp), and binning settings. You must set the frame size any time any of
     * the prior parameters gets updated.
     * @param nbuf size of buffer in bytes.
     * @param allocMem if True, it will allocate memory of nbut size bytes.
     */
    void setFrameBufferSize(int nbuf, bool allocMem = true);

    /**
     * @brief setBPP Set depth of CCD chip.
     * @param bpp bits per pixel
     */
    void setBPP(int bpp);

    /**
     * @brief setFrameType Set desired frame type for next exposure.
     * @param type desired CCD frame type.
     */
    void setFrameType(CCD_FRAME type);

    /**
     * @brief setExposureDuration Set desired CCD frame exposure duration for next exposure. You must
     * call this function immediately before starting the actual exposure as it is used to calculate
     * the timestamp used for the FITS header.
     * @param duration exposure duration in seconds.
     */
    void setExposureDuration(double duration);

    /**
     * @brief setExposureLeft Update exposure time left. Inform the client of the new exposure time
     * left value.
     * @param duration exposure duration left in seconds.
     */
    void setExposureLeft(double duration);

    /**
     * @brief setExposureFailed Alert the client that the exposure failed.
     */
    void setExposureFailed();

    /**
     * @return Get number of FITS axis in image. By default 2
     */
    int getNAxis() const;

    /**
     * @brief setNAxis Set FITS number of axis
     * @param value number of axis
     */
    void setNAxis(int value);

    /**
     * @brief setImageExtension Set image exntension
     * @param ext extension (fits, jpeg, raw..etc)
     */
    void setImageExtension(const char *ext);

    /**
     * @return Return image extension (fits, jpeg, raw..etc)
     */
    char *getImageExtension() { return imageExtention; }

    /**
     * @return True if CCD is currently exposing, false otherwise.
     */
    bool isExposing() { return (ImageExposureNP.s == IPS_BUSY); }

    /**
     * @brief binFrame Perform softwre binning on the CCD frame. Only use this function if hardware
     * binning is not supported.
     */
    void binFrame();

  private:
    /// Native x resolution of the ccd
    int XRes;
    /// Native y resolution of the ccd
    int YRes;
    /// Left side of the subframe we are requesting
    int SubX;
    /// Top of the subframe requested
    int SubY;
    /// UNBINNED width of the subframe
    int SubW;
    /// UNBINNED height of the subframe
    int SubH;
    /// Binning requested in the x direction
    int BinX;
    /// Binning requested in the y direction
    int BinY;
    /// # of Axis
    int NAxis;
    /// Pixel size in microns, x direction
    float PixelSizex;
    /// Pixel size in microns, y direction
    float PixelSizey;
    /// Bytes per Pixel
    int BPP;
    bool Interlaced = false;
    uint8_t *RawFrame = nullptr;
    uint8_t *BinFrame = nullptr;
    int RawFrameSize = 0;
    bool SendCompressed = false;
    CCD_FRAME FrameType;
    double exposureDuration;
    timeval startExposureTime;
    int lastRapidX;
    int lastRapidY;
    char imageExtention[MAXINDIBLOBFMT];

    INumberVectorProperty ImageExposureNP;
    INumber ImageExposureN[1];

    ISwitchVectorProperty AbortExposureSP;
    ISwitch AbortExposureS[1];

    INumberVectorProperty ImageFrameNP;
    INumber ImageFrameN[4];

    INumberVectorProperty ImageBinNP;
    INumber ImageBinN[2];

    INumberVectorProperty ImagePixelSizeNP;
    INumber ImagePixelSizeN[6];

    ISwitch FrameTypeS[5];
    ISwitchVectorProperty FrameTypeSP;

    ISwitch CompressS[2];
    ISwitchVectorProperty CompressSP;

    IBLOB FitsB;
    IBLOBVectorProperty FitsBP;

    ISwitch RapidGuideS[2];
    ISwitchVectorProperty RapidGuideSP;

    ISwitch RapidGuideSetupS[3];
    ISwitchVectorProperty RapidGuideSetupSP;

    INumber RapidGuideDataN[3];
    INumberVectorProperty RapidGuideDataNP;

    ISwitch ResetS[1];
    ISwitchVectorProperty ResetSP;

    friend class CCD;
    friend class StreamRecoder;
};

/**
 * \class CCD
 * \brief Class to provide general functionality of CCD cameras with a single CCD sensor, or a
 * primary CCD sensor in addition to a secondary CCD guide head.
 *
 * The CCD capabilities must be set to select which features are exposed to the clients.
 * SetCCDCapability() is typically set in the constructor or initProperties(), but can also be
 * called after connection is established with the CCD, but must be called /em before returning
 * true in Connect().
 *
 * It also implements the interface to perform guiding. The class enable the ability to \e snoop
 * on telescope equatorial coordinates and record them in the FITS file before upload. It also
 * snoops Sky-Quality-Meter devices to record sky quality in units of Magnitudes-Per-Arcsecond-Squared
 * (MPASS) in the FITS header.
 *
 * Support for streaming is available (Linux only) and is handled by the StreamRecorder class.
 *
 * Developers need to subclass INDI::CCD to implement any driver for CCD cameras within INDI.
 *
 * \example CCD Simulator
 * \author Jasem Mutlaq, Gerry Rozema
 *
 */
class CCD : public DefaultDevice, GuiderInterface
{
  public:
    CCD();
    virtual ~CCD();

    enum
    {
        CCD_CAN_BIN        = 1 << 0, /*!< Does the CCD support binning?  */
        CCD_CAN_SUBFRAME   = 1 << 1, /*!< Does the CCD support setting ROI?  */
        CCD_CAN_ABORT      = 1 << 2, /*!< Can the CCD exposure be aborted?  */
        CCD_HAS_GUIDE_HEAD = 1 << 3, /*!< Does the CCD have a guide head?  */
        CCD_HAS_ST4_PORT   = 1 << 4, /*!< Does the CCD have an ST4 port?  */
        CCD_HAS_SHUTTER    = 1 << 5, /*!< Does the CCD have a mechanical shutter?  */
        CCD_HAS_COOLER     = 1 << 6, /*!< Does the CCD have a cooler and temperature control?  */
        CCD_HAS_BAYER      = 1 << 7, /*!< Does the CCD send color data in bayer format?  */
        CCD_HAS_STREAMING  = 1 << 8  /*!< Does the CCD support live video streaming?  */
    } CCDCapability;

    typedef enum { UPLOAD_CLIENT, UPLOAD_LOCAL, UPLOAD_BOTH } CCD_UPLOAD_MODE;

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

  protected:
    /**
     * @brief GetCCDCapability returns the CCD capabilities.
     */
    uint32_t GetCCDCapability() const { return capability; }

    /**
     * @brief SetCCDCapability Set the CCD capabilities. Al fields must be initilized.
     * @param cap pointer to CCDCapability struct.
     */
    void SetCCDCapability(uint32_t cap);

    /**
     * @return True if CCD can abort exposure. False otherwise.
     */
    bool CanAbort() { return capability & CCD_CAN_ABORT; }

    /**
     * @return True if CCD supports binning. False otherwise.
     */
    bool CanBin() { return capability & CCD_CAN_BIN; }

    /**
     * @return True if CCD supports subframing. False otherwise.
     */
    bool CanSubFrame() { return capability & CCD_CAN_SUBFRAME; }

    /**
     * @return True if CCD has guide head. False otherwise.
     */
    bool HasGuideHead() { return capability & CCD_HAS_GUIDE_HEAD; }

    /**
     * @return True if CCD has mechanical or electronic shutter. False otherwise.
     */
    bool HasShutter() { return capability & CCD_HAS_SHUTTER; }

    /**
     * @return True if CCD has ST4 port for guiding. False otherwise.
     */
    bool HasST4Port() { return capability & CCD_HAS_ST4_PORT; }

    /**
     * @return True if CCD has cooler and temperature can be controlled. False otherwise.
     */
    bool HasCooler() { return capability & CCD_HAS_COOLER; }

    /**
     * @return True if CCD sends image data in bayer format. False otherwise.
     */
    bool HasBayer() { return capability & CCD_HAS_BAYER; }

    /**
     * @return  True if the CCD supports live video streaming. False otherwise.
     */
    bool HasStreaming() { return capability & CCD_HAS_STREAMING; }

    /**
     * @brief Set CCD temperature
     * @param temperature CCD temperature in degrees celcius.
     * @return 0 or 1 if setting the temperature call to the hardware is successful. -1 if an
     * error is encountered.
     * Return 0 if setting the temperature to the requested value takes time.
     * Return 1 if setting the temperature to the requested value is complete.
     * \note Upon returning 0, the property becomes BUSY. Once the temperature reaches the requested
     * value, change the state to OK.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual int SetTemperature(double temperature);

    /**
     * \brief Start exposing primary CCD chip
     * \param duration Duration in seconds
     * \return true if OK and exposure will take some time to complete, false on error.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool StartExposure(float duration);

    /**
     * \brief Uploads target Chip exposed buffer as FITS to the client. Dervied classes should class
     * this function when an exposure is complete.
     * @param targetChip chip that contains upload image data
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool ExposureComplete(CCDChip *targetChip);

    /**
     * \brief Abort ongoing exposure
     * \return true is abort is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool AbortExposure();

    /**
     * \brief Start exposing guide CCD chip
     * \param duration Duration in seconds
     * \return true if OK and exposure will take some time to complete, false on error.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool StartGuideExposure(float duration);

    /**
     * \brief Abort ongoing exposure
     * \return true is abort is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool AbortGuideExposure();

    /**
     * \brief CCD calls this function when CCD Frame dimension needs to be updated in the
     * hardware. Derived classes should implement this function
     * \param x Subframe X coordinate in pixels.
     * \param y Subframe Y coordinate in pixels.
     * \param w Subframe width in pixels.
     * \param h Subframe height in pixels.
     * \note (0,0) is defined as most left, top pixel in the subframe.
     * \return true is CCD chip update is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool UpdateCCDFrame(int x, int y, int w, int h);

    /**
     * \brief CCD calls this function when Guide head frame dimension is updated by the
     * client. Derived classes should implement this function
     * \param x Subframe X coordinate in pixels.
     * \param y Subframe Y coordinate in pixels.
     * \param w Subframe width in pixels.
     * \param h Subframe height in pixels.
     * \note (0,0) is defined as most left, top pixel in the subframe.
     * \return true is CCD chip update is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool UpdateGuiderFrame(int x, int y, int w, int h);

    /**
     * \brief CCD calls this function when CCD Binning needs to be updated in the hardware.
     * Derived classes should implement this function
     * \param hor Horizontal binning.
     * \param ver Vertical binning.
     * \return true is CCD chip update is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool UpdateCCDBin(int hor, int ver);

    /**
     * \brief CCD calls this function when Guide head binning is updated by the client.
     * Derived classes should implement this function
     * \param hor Horizontal binning.
     * \param ver Vertical binning.
     * \return true is CCD chip update is successful, false otherwise.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     */
    virtual bool UpdateGuiderBin(int hor, int ver);

    /**
     * \brief CCD calls this function when CCD frame type needs to be updated in the hardware.
     * \param fType Frame type
     * \return true is CCD chip update is successful, false otherwise.
     * \note It is \e not mandatory to implement this function in the child class. The CCD hardware
     * layer may either set the frame type when this function is called, or (optionally) before an
     * exposure is started.
     */
    virtual bool UpdateCCDFrameType(CCDChip::CCD_FRAME fType);

    /**
     * \brief CCD calls this function when client upload mode switch is updated.
     * \param mode upload mode. UPLOAD_CLIENT only sends the upload the client application. UPLOAD_BOTH saves the frame and uploads it to the client. UPLOAD_LOCAL only saves
     * the frame locally.
     * \return true if mode is changed successfully, false otherwise.
     * \note By default this function is implemented in the base class and returns true. Override if necessary.
     */
    virtual bool UpdateCCDUploadMode(CCD_UPLOAD_MODE mode) { INDI_UNUSED(mode); return true; }

    /**
     * \brief CCD calls this function when Guide frame type is updated by the client.
     * \param fType Frame type
     * \return true is CCD chip update is successful, false otherwise.
     * \note It is \e not mandatory to implement this function in the child class. The CCD hardware
     * layer may either set the frame type when this function is called, or (optionally) before an
     * exposure is started.
     */
    virtual bool UpdateGuiderFrameType(CCDChip::CCD_FRAME fType);

    /**
     * \brief Setup CCD paramters for primary CCD. Child classes call this function to update
     * CCD parameters
     * \param x Frame X coordinates in pixels.
     * \param y Frame Y coordinates in pixels.
     * \param bpp Bits Per Pixels.
     * \param xf X pixel size in microns.
     * \param yf Y pixel size in microns.
     */
    virtual void SetCCDParams(int x, int y, int bpp, float xf, float yf);

    /**
     * \brief Setup CCD paramters for guide head CCD. Child classes call this function to update
     * CCD parameters
     * \param x Frame X coordinates in pixels.
     * \param y Frame Y coordinates in pixels.
     * \param bpp Bits Per Pixels.
     * \param xf X pixel size in microns.
     * \param yf Y pixel size in microns.
     */
    virtual void SetGuiderParams(int x, int y, int bpp, float xf, float yf);

    /**
     * \brief Guide northward for ms milliseconds
     * \param ms Duration in milliseconds.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     * \return True if successful, false otherwise.
     */
    virtual IPState GuideNorth(float ms);

    /**
     * \brief Guide southward for ms milliseconds
     * \param ms Duration in milliseconds.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     * \return 0 if successful, -1 otherwise.
     */
    virtual IPState GuideSouth(float ms);

    /**
     * \brief Guide easward for ms milliseconds
     * \param ms Duration in milliseconds.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     * \return 0 if successful, -1 otherwise.
     */
    virtual IPState GuideEast(float ms);

    /**
     * \brief Guide westward for ms milliseconds
     * \param ms Duration in milliseconds.
     * \note This function is not implemented in CCD, it must be implemented in the child class
     * \return 0 if successful, -1 otherwise.
     */
    virtual IPState GuideWest(float ms);

    /**
     * @brief StartStreaming Start live video streaming
     * @return True if successful, false otherwise.
     */
    virtual bool StartStreaming();

    /**
     * @brief StopStreaming Stop live video streaming
     * @return True if successful, false otherwise.
     */
    virtual bool StopStreaming();

    /**
     * \brief Add FITS keywords to a fits file
     * \param fptr pointer to a valid FITS file.
     * \param targetChip The target chip to extract the keywords from.
     * \note In additional to the standard FITS keywords, this function write the following
     * keywords the FITS file:
     * <ul>
     * <li>EXPTIME: Total Exposure Time (s)</li>
     * <li>DARKTIME (if applicable): Total Exposure Time (s)</li>
     * <li>PIXSIZE1: Pixel Size 1 (microns)</li>
     * <li>PIXSIZE2: Pixel Size 2 (microns)</li>
     * <li>BINNING: Binning HOR x VER</li>
     * <li>FRAME: Frame Type</li>
     * <li>DATAMIN: Minimum value</li>
     * <li>DATAMAX: Maximum value</li>
     * <li>INSTRUME: CCD Name</li>
     * <li>DATE-OBS: UTC start date of observation</li>
     * </ul>
     *
     * To add additional information, override this function in the child class and ensure to call
     * CCD::addFITSKeywords.
     */
    virtual void addFITSKeywords(fitsfile *fptr, CCDChip *targetChip);

    /** A function to just remove GCC warnings about deprecated conversion */
    void fits_update_key_s(fitsfile *fptr, int type, std::string name, void *p, std::string explanation, int *status);

    /**
     * @brief activeDevicesUpdated Inform children that ActiveDevices property was updated so they can
     * snoop on the updated devices if desired.
     */
    virtual void activeDevicesUpdated() {}

    /**
     * @brief saveConfigItems Save configuration items in XML file.
     * @param fp pointer to file to write to
     * @return True if successful, false otherwise
     */
    virtual bool saveConfigItems(FILE *fp);

    void GuideComplete(INDI_EQ_AXIS axis);

    // Epoch Position
    double RA, Dec;

    // J2000 Position
    double J2000RA;
    double J2000DE;

    double primaryFocalLength, primaryAperture, guiderFocalLength, guiderAperture;
    bool InExposure;
    bool InGuideExposure;
    bool RapidGuideEnabled;
    bool GuiderRapidGuideEnabled;

    bool AutoLoop;
    bool GuiderAutoLoop;
    bool SendImage;
    bool GuiderSendImage;
    bool ShowMarker;
    bool GuiderShowMarker;

    float ExposureTime;
    float GuiderExposureTime;

    // Sky Quality
    double MPSAS;

    // Rotator Angle
    double RotatorAngle;

    // Airmas
    double Airmass;
    double Latitude;
    double Longitude;

    std::vector<std::string> FilterNames;
    int CurrentFilterSlot;

    std::unique_ptr<StreamManager> Streamer;

    CCDChip PrimaryCCD;
    CCDChip GuideCCD;

    //  We are going to snoop these from a telescope
    INumberVectorProperty EqNP;
    INumber EqN[2];

    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[4];
    enum
    {
        SNOOP_MOUNT,
        SNOOP_ROTATOR,
        SNOOP_FILTER_WHEEL,
        SNOOP_SQM
    };

    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    IText BayerT[3];
    ITextVectorProperty BayerTP;

    IText FileNameT[1];
    ITextVectorProperty FileNameTP;

    ISwitch UploadS[3];
    ISwitchVectorProperty UploadSP;

    IText UploadSettingsT[2];
    ITextVectorProperty UploadSettingsTP;
    enum
    {
        UPLOAD_DIR,
        UPLOAD_PREFIX
    };

    ISwitch TelescopeTypeS[2];
    ISwitchVectorProperty TelescopeTypeSP;
    enum
    {
        TELESCOPE_PRIMARY,
        TELESCOPE_GUIDE
    };

    // WCS
    ISwitch WorldCoordS[2];
    ISwitchVectorProperty WorldCoordSP;

    // WCS CCD Rotation
    INumber CCDRotationN[1];
    INumberVectorProperty CCDRotationNP;

    // FITS Header
    IText FITSHeaderT[2];
    ITextVectorProperty FITSHeaderTP;
    enum
    {
        FITS_OBSERVER,
        FITS_OBJECT
    };

  private:
    uint32_t capability;

    bool ValidCCDRotation;

    bool uploadFile(CCDChip *targetChip, const void *fitsData, size_t totalBytes, bool sendImage, bool saveImage);
    void getMinMax(double *min, double *max, CCDChip *targetChip);
    int getFileIndex(const char *dir, const char *prefix, const char *ext);

    friend class StreamManager;
};
}
