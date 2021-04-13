/*******************************************************************************
 Copyright(c) 2010, 2017 Ilia Platone, Jasem Mutlaq. All rights reserved.

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

#include <fitsio.h>

#include <memory>
#include <cstring>

#include <stdint.h>

extern const char *CAPTURE_SETTINGS_TAB;
extern const char *CAPTURE_INFO_TAB;
extern const char *GUIDE_HEAD_TAB;

/**
 * @brief The DetectorDevice class provides functionality of a Detector Device within a Detector.
 */
class DetectorDevice
{
  public:
    DetectorDevice();
    ~DetectorDevice();

    typedef enum {
        DETECTOR_SAMPLERATE,
        DETECTOR_FREQUENCY,
        DETECTOR_BITSPERSAMPLE,
    } DETECTOR_INFO_INDEX;

    typedef enum {
        DETECTOR_BLOB_CONTINUUM,
        DETECTOR_BLOB_SPECTRUM,
    } DETECTOR_BLOB_INDEX;

    /**
     * @brief getBPS Get Detector depth (bits per sample).
     * @return bits per sample.
     */
    inline int getBPS() { return BPS; }

    /**
     * @brief getContinuumBufferSize Get allocated continuum buffer size to hold the Detector captured stream.
     * @return allocated continuum buffer size to hold the Detector capture stream.
     */
    inline int getContinuumBufferSize() { return ContinuumBufferSize; }

    /**
     * @brief getSpectrumBufferSize Get allocated spectrum buffer size to hold the Detector spectrum.
     * @return allocated spectrum buffer size (in doubles) to hold the Detector spectrum.
     */
    inline int getSpectrumBufferSize() { return SpectrumBufferSize; }

    /**
     * @brief getCaptureLeft Get Capture time left in seconds.
     * @return Capture time left in seconds.
     */
    inline double getCaptureLeft() { return FramedCaptureN[0].value; }

    /**
     * @brief getSampleRate Get requested SampleRate for the Detector device in Hz.
     * @return requested SampleRate for the Detector device in Hz.
     */
    inline double getSampleRate() { return samplerate; }

    /**
     * @brief getSamplingFrequency Get requested Capture frequency for the Detector device in Hz.
     * @return requested Capture frequency for the Detector device in Hz.
     */
    inline double getFrequency() { return Frequency; }

    /**
     * @brief getCaptureDuration Get requested Capture duration for the Detector device in seconds.
     * @return requested Capture duration for the Detector device in seconds.
     */
    inline double getCaptureDuration() { return captureDuration; }

    /**
     * @brief getCaptureStartTime
     * @return Capture start time in ISO 8601 format.
     */
    const char *getCaptureStartTime();

    /**
     * @brief getContinuumBuffer Get raw buffer of the continuum stream of the Detector device.
     * @return raw continuum buffer of the Detector device.
     */
    inline uint8_t *getContinuumBuffer() { return ContinuumBuffer; }

    /**
     * @brief getSpectrumBuffer Get raw buffer of the spectrum of the Detector device.
     * @return raw continuum buffer of the Detector device.
     */
    inline double *getSpectrumBuffer() { return SpectrumBuffer; }

    /**
     * @brief setContinuumBuffer Set raw frame buffer pointer.
     * @param buffer pointer to continuum buffer
     * /note Detector Device allocates the frame buffer internally once SetContinuumBufferSize is called
     * with allocMem set to true which is the default behavior. If you allocated the memory
     * yourself (i.e. allocMem is false), then you must call this function to set the pointer
     * to the raw frame buffer.
     */
    void setContinuumBuffer(uint8_t *buffer) { ContinuumBuffer = buffer; }

    /**
     * @brief setSpectrumBuffer Set raw frame buffer pointer.
     * @param buffer pointer to spectrum buffer
     * /note Detector Device allocates the frame buffer internally once SetSpectrumBufferSize is called
     * with allocMem set to true which is the default behavior. If you allocated the memory
     * yourself (i.e. allocMem is false), then you must call this function to set the pointer
     * to the raw frame buffer.
     */
    void setSpectrumBuffer(double *buffer) { SpectrumBuffer = buffer; }

    /**
     * @brief Return Detector Info Property
     */
    INumberVectorProperty *getDetectorSettings() { return &DetectorSettingsNP; }

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
     * @brief setContinuumBufferSize Set desired continuum buffer size. The function will allocate memory
     * accordingly. The frame size depends on the desired capture time, sampling frequency, and
     * sample depth of the Detector device (bps). You must set the frame size any time any of
     * the prior parameters gets updated.
     * @param nbuf size of buffer in bytes.
     * @param allocMem if True, it will allocate memory of nbut size bytes.
     */
    void setContinuumBufferSize(int nbuf, bool allocMem = true);

    /**
     * @brief setSpectrumBufferSize Set desired spectrum buffer size. The function will allocate memory
     * accordingly. The frame size depends on the size of the spectrum. You must set the frame size any
     * time the spectrum size changes.
     * @param nbuf size of buffer in doubles.
     * @param allocMem if True, it will allocate memory of nbut size doubles.
     */
    void setSpectrumBufferSize(int nbuf, bool allocMem = true);

    /**
     * @brief setSampleRate Set depth of Detector device.
     * @param bpp bits per pixel
     */
    void setSampleRate(float sr);

    /**
     * @brief setFrequency Set the frequency observed.
     * @param capfreq Capture frequency
     */
    void setFrequency(float freq);

    /**
     * @brief setBPP Set depth of Detector device.
     * @param bpp bits per pixel
     */
    void setBPS(int bps);

    /**
     * @brief setCaptureDuration Set desired Detector frame Capture duration for next Capture. You must
     * call this function immediately before starting the actual Capture as it is used to calculate
     * the timestamp used for the FITS header.
     * @param duration Capture duration in seconds.
     */
    void setCaptureDuration(double duration);

    /**
     * @brief setCaptureLeft Update Capture time left. Inform the client of the new Capture time
     * left value.
     * @param duration Capture duration left in seconds.
     */
    void setCaptureLeft(double duration);

    /**
     * @brief setCaptureFailed Alert the client that the Capture failed.
     */
    void setCaptureFailed();

    /**
     * @return Get number of FITS axis in capture. By default 2
     */
    int getNAxis() const;

    /**
     * @brief setNAxis Set FITS number of axis
     * @param value number of axis
     */
    void setNAxis(int value);

    /**
     * @brief setCaptureExtension Set capture exntension
     * @param ext extension (fits, jpeg, raw..etc)
     */
    void setCaptureExtension(const char *ext);

    /**
     * @return Return capture extension (fits, jpeg, raw..etc)
     */
    char *getCaptureExtension() { return captureExtention; }

    /**
     * @return True if Detector is currently exposing, false otherwise.
     */
    bool isCapturing() { return (FramedCaptureNP.s == IPS_BUSY); }

  private:
    /// # of Axis
    int NAxis;
    /// Bytes per Sample
    int BPS;
    double samplerate;
    double Frequency;
    uint8_t *ContinuumBuffer;
    int ContinuumBufferSize;
    double *SpectrumBuffer;
    int SpectrumBufferSize;
    double captureDuration;
    timeval startCaptureTime;
    char captureExtention[MAXINDIBLOBFMT];

    INumberVectorProperty FramedCaptureNP;
    INumber FramedCaptureN[1];

    INumberVectorProperty DetectorSettingsNP;
    INumber DetectorSettingsN[4];

    ISwitchVectorProperty AbortCaptureSP;
    ISwitch AbortCaptureS[1];

    IBLOB FitsB[2];
    IBLOBVectorProperty FitsBP;

    friend class INDI::Detector;
};

/**
 * \class INDI::Detector
 * \brief Class to provide general functionality of Monodimensional Detector.
 *
 * The Detector capabilities must be set to select which features are exposed to the clients.
 * SetDetectorCapability() is typically set in the constructor or initProperties(), but can also be
 * called after connection is established with the Detector, but must be called /em before returning
 * true in Connect().
 *
 * Developers need to subclass INDI::Detector to implement any driver for Detectors within INDI.
 *
 * \example Detector Simulator
 * \author Jasem Mutlaq, Ilia Platone
 *
 */

namespace INDI
{

class Detector : public DefaultDevice
{
  public:
    Detector();
    virtual ~Detector();

    enum
    {
        DETECTOR_CAN_ABORT      = 1 << 0, /*!< Can the Detector Capture be aborted?  */
        DETECTOR_HAS_SHUTTER    = 1 << 1, /*!< Does the Detector have a mechanical shutter?  */
        DETECTOR_HAS_COOLER     = 1 << 2, /*!< Does the Detector have a cooler and temperature control?  */
        DETECTOR_HAS_CONTINUUM  = 1 << 3,  /*!< Does the Detector support live streaming?  */
        DETECTOR_HAS_SPECTRUM   = 1 << 4,  /*!< Does the Detector support spectrum analysis?  */
    } DetectorCapability;

    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISSnoopDevice(XMLEle *root);

  protected:
    /**
     * @brief GetDetectorCapability returns the Detector capabilities.
     */
    uint32_t GetDetectorCapability() const { return capability; }

    /**
     * @brief SetDetectorCapability Set the Detector capabilities. Al fields must be initilized.
     * @param cap pointer to DetectorCapability struct.
     */
    void SetDetectorCapability(uint32_t cap);

    /**
     * @return True if Detector can abort Capture. False otherwise.
     */
    bool CanAbort() { return capability & DETECTOR_CAN_ABORT; }

    /**
     * @return True if Detector has mechanical or electronic shutter. False otherwise.
     */
    bool HasShutter() { return capability & DETECTOR_HAS_SHUTTER; }

    /**
     * @return True if Detector has cooler and temperature can be controlled. False otherwise.
     */
    bool HasCooler() { return capability & DETECTOR_HAS_COOLER; }

    /**
     * @return  True if the Detector supports live streaming. False otherwise.
     */
    bool HasContinuum() { return capability & DETECTOR_HAS_CONTINUUM; }

    /**
     * @return  True if the Detector supports live streaming. False otherwise.
     */
    bool HasSpectrum() { return capability & DETECTOR_HAS_SPECTRUM; }

    /**
     * @brief Set Detector temperature
     * @param temperature Detector temperature in degrees celsius.
     * @return 0 or 1 if setting the temperature call to the hardware is successful. -1 if an
     * error is encountered.
     * Return 0 if setting the temperature to the requested value takes time.
     * Return 1 if setting the temperature to the requested value is complete.
     * \note Upon returning 0, the property becomes BUSY. Once the temperature reaches the requested
     * value, change the state to OK.
     * \note This function is not implemented in Detector, it must be implemented in the child class
     */
    virtual int SetTemperature(double temperature);

    /**
     * \brief Start capture from the Detector device
     * \param duration Duration in seconds
     * \return true if OK and Capture will take some time to complete, false on error.
     * \note This function is not implemented in Detector, it must be implemented in the child class
     */
    virtual bool StartCapture(float duration);

    /**
     * \brief Set common capture params
     * \param sr Detector samplerate (in Hz)
     * \param cfreq Capture frequency of the detector (Hz, observed frequency).
     * \param sfreq Sampling frequency of the detector (Hz, electronic speed of the detector).
     * \param bps Bit resolution of a single sample.
     * \return true if OK and Capture will take some time to complete, false on error.
     * \note This function is not implemented in Detector, it must be implemented in the child class
     */
    virtual bool CaptureParamsUpdated(float sr, float freq, float bps);

    /**
     * \brief Uploads target Device exposed buffer as FITS to the client. Dervied classes should class
     * this function when an Capture is complete.
     * @param targetDevice device that contains upload capture data
     * \note This function is not implemented in Detector, it must be implemented in the child class
     */
    virtual bool CaptureComplete(DetectorDevice *targetDevice);

    /**
     * \brief Abort ongoing Capture
     * \return true is abort is successful, false otherwise.
     * \note This function is not implemented in Detector, it must be implemented in the child class
     */
    virtual bool AbortCapture();

    /**
     * \brief Setup Detector parameters for the Detector. Child classes call this function to update
     * Detector parameters
     * \param samplerate Detector samplerate (in Hz)
     * \param freq Center frequency of the detector (Hz, observed frequency).
     * \param bps Bit resolution of a single sample.
     */
    virtual void SetDetectorParams(float samplerate, float freq, float bps);

    /**
     * \brief Add FITS keywords to a fits file
     * \param fptr pointer to a valid FITS file.
     * \param targetDevice The target device to extract the keywords from.
     * \param blobIndex The blob index of this FITS (0: continuum, 1: spectrum).
     * \note In additional to the standard FITS keywords, this function write the following
     * keywords the FITS file:
     * <ul>
     * <li>EXPTIME: Total Capture Time (s)</li>
     * <li>DATAMIN: Minimum value</li>
     * <li>DATAMAX: Maximum value</li>
     * <li>INSTRUME: Detector Name</li>
     * <li>DATE-OBS: UTC start date of observation</li>
     * </ul>
     *
     * To add additional information, override this function in the child class and ensure to call
     * Detector::addFITSKeywords.
     */
    virtual void addFITSKeywords(fitsfile *fptr, DetectorDevice *targetDevice, int blobIndex);

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

    double RA, Dec;
    double primaryAperture;
    double primaryFocalLength;
    bool InCapture;

    bool AutoLoop;
    bool SendCapture;
    bool ShowMarker;

    float CaptureTime;

    // Sky Quality
    double MPSAS;

    std::vector<std::string> FilterNames;
    int CurrentFilterSlot;

    DetectorDevice PrimaryDetector;

    //  We are going to snoop these from a telescope
    INumberVectorProperty EqNP;
    INumber EqN[2];

    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[4];

    INumber TemperatureN[1];
    INumberVectorProperty TemperatureNP;

    IText FileNameT[1];
    ITextVectorProperty FileNameTP;

    ISwitch DatasetS[1];
    ISwitchVectorProperty DatasetSP;

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
        TELESCOPE_PRIMARY
    };

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

    bool uploadFile(DetectorDevice *targetDevice, const void *fitsData, size_t totalBytes, bool sendCapture, bool saveCapture, int blobindex);
    void getMinMax(double *min, double *max, uint8_t *buf, int len, int bpp);
    int getFileIndex(const char *dir, const char *prefix, const char *ext);
};

}
