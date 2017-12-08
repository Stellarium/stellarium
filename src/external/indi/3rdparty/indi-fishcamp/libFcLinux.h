// libFcLinux.h
//
// Based upon windows version of 5-15-09
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

#include <libusb-1.0/libusb.h>

typedef unsigned char UInt8;
typedef signed char SInt8;
typedef unsigned short UInt16;
typedef signed short SInt16;
typedef unsigned long UInt32;
typedef signed long SInt32;

// list of USB command codes
typedef enum {

    fcNOP,
    fcRST,
    fcGETINFO,
    fcSETREG,
    fcGETREG,
    fcSETINTTIME,
    fcSTARTEXP,
    fcABORTEXP,
    fcGETSTATE,
    fcSETFGTP,
    fcRDSCANLINE,
    fcGETIMAGESTATS,
    fcSETROI,
    fcSETBIN,
    fcSETRELAY,
    fcCLRRELAY,
    fcPULSERELAY,
    fcSETLED,
    fcSETTEMP,
    fcGETTEMP,
    fcGETRAWFRAME,
    fcTURNOFFTEC,
    fcSETREADMODE,
    fcSETGAIN,
    fcSETOFFSET,
    fcSETGUIDEINTTIME,
    fcSTARTGUIDEEXP,
    fcABORTGUIDEEXP,
    fcGETGUIDESTATE,
    fcGETPEDESTAL,
    fcSETREG32,
    fcGETREG32,
    fcSETPROPERTY,
    fcGETPROPERTY

} fc_cmd;

// progress codes for the fcUsb_FindCameras routine
typedef enum {

    fcFindCam_notYetStarted,
    fcFindCam_looking4supported, // @"Looking for supported cameras"
    fcFindCam_initializingUSB,   // @"Initializing camera USB controller"
    fcFindCam_initializingIP,    // @"Initializing camera Image Processor"
    fcFindCam_finished           // @"Done looking for supported cameras"

} fcFindCamState;

// structure used to hold the return information from the fcGETINFO command
typedef struct
{
    UInt16 boardVersion;
    UInt16 boardRevision;
    UInt16 fpgaVersion;
    UInt16 fpgaRevision;
    UInt16 width;
    UInt16 height;
    UInt16 pixelWidth;
    UInt16 pixelHeight;
    UInt8 camSerialStr[32]; // 'C' string format
    UInt8 camNameStr[32];   // 'C' string format
} fc_camInfo;

// list of relays
typedef enum {

    fcRELAYNORTH,
    fcRELAYSOUTH,
    fcRELAYWEST,
    fcRELAYEAST

} fc_relay;

// list of data formats
typedef enum {

    fc_8b_data,
    fc_10b_data,
    fc_12b_data,
    fc_14b_data,
    fc_16b_data

} fc_dataFormat;

// list of data transfer modes
typedef enum {

    fc_classicDataXfr,
    fc_DMAwFBDataXfr,
    fc_DMASensor2USBDataXfr

} fc_dataXfrModes;

// list of image filters
typedef enum {

    fc_filter_none,
    fc_filter_3x3,
    fc_filter_5x5,
    fc_filter_hotPixel

} fc_imageFilter;

// properties that we have defined so far.  First camera that
// supports properties is the StarfishPRO 4M with the KAI-04022 chip
typedef enum {

    fcPROP_NUMSAMPLES,
    fcPROP_PATTERNENABLE,
    fcPROP_PIXCAPTURE,
    fcPROP_SHUTTERMODE,
    fcPROP_MOVESHUTTERFLAG,
    fcPROP_COLNORM,       // not available to the end user
    fcPROP_SERVOOPENPOS,  // not available to the end user
    fcPROP_SERVOCLOSEDPOS // not available to the end user

} fc_property;

// This is the framework initialization routine and needs to be called once upon application startup
void fcUsb_init(void);

// Call this routine to enable / disable entries in the log file
// By default, logging is turned on.  The log file will be created
// in C:\Program Files\fishcamp\starfish_log.txt
//
// loggingState = true to turn logging on
//
void fcUsb_setLogging(bool loggingState);

// This is the framework close routine and needs to be called just before application termination
void fcUsb_close(void);

// the prefered way of finding and opening a communications link to any of our supported cameras
// This routine will call fcUsb_OpenCameraDriver and fcUsb_CloseCameraDriver routines to do its job
//
// will return the actual number of cameras found.
//
// be carefull, this routine can take a long time (> 5 secs) to execute
//
// routine will return the number of supported cameras discovered.
//
//int fcUsb_FindCameras(struct libusb_context *ctx);
int fcUsb_FindCameras(void);

// call this routine to know how what state the fcUsb_FindCameras routine is currently executing
// will return a result of fcFindCamState type
//
int fcUsb_GetFindCamsState(void);

// call this routine to know how long it will take for the fcUsb_FindCameras routine to complete.
//
float fcUsb_GetFindCamsPercentComplete(void);

// call this routine before making any other calls to this camera
// This routine will assign the designated camera to your application
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_OpenCamera(int camNum);

// call this routine after you are finished making calls to this camera
// This routine will free the designated camera for other applications to use
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_CloseCamera(int camNum);

// call this routine to know how many of our supported cameras are available for use.
//
int fcUsb_GetNumCameras(void);

// call this routine to see if we have any of our supported cameras attached.
// will return TRUE if we have at least one supported camera.
//
bool fcUsb_haveCamera(void);

// return the numeric serial number of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_GetCameraSerialNum(int camNum);

// return the numeric vendorID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_GetCameraVendorID(int camNum);

// return the numeric productID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_GetCameraProductID(int camNum);

int fcUsb_cmd_nop(int camNum);

// send the rst command to the starfish camera
//
int fcUsb_cmd_rst(int camNum);

// send the fcGETINFO command to the starfish camera
// read the return information
//
int fcUsb_cmd_getinfo(int camNum, fc_camInfo *camInfo);

//
// call to set a low level register in the camera
//
// for Starfish camera:
//		- call to set a low level register in the micron image sensor
//		- enter with the micron address register, and register data value
//		  refer to the micron image sensor documentation for details
//		  on available registers and bit definitions
//
// for IBIS1300
//		- call to set the serial configuration word on the camera
//		- enter with regAddress = 0, dataValue
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_cmd_setRegister(int camNum, UInt16 regAddress, UInt16 dataValue);

//
// call to get a low level register from the micron image sensor
// valid camNum is 1 -> fcUsb_GetNumCameras()
// refer to the micron image sensor documentation for details
// on available registers and bit definitions
//
// not a valid call for the IBIS1300 camera
//
UInt16 fcUsb_cmd_getRegister(int camNum, UInt16 regAddress);

// call to set the integration time register.
// valid camNum is 1 -> fcUsb_GetNumCameras()
// 'theTime' specifies the number of milliseconds desired for the integration.
//  only 22 LSB significant giving a range of 0.001 -> 4194 seconds
// the starfish only has a range of 0.001 -> 300 seconds
//
// This command is valid for both the Starfish and IBIS1300 cameras.
// When sent to the IBIS1300 camera, it defines the integration time of
// the main imager portion of the sensor.
//
int fcUsb_cmd_setIntegrationTime(int camNum, UInt32 theTime);

// command to set the integration time period of the guider portion of
// the IBIS1300 image sensor.  This command is not recognized by the Starfish camera
//
int fcUsb_cmd_setGuiderIntegrationTime(int camNum, UInt32 theTime);

// send the 'start exposure' command to the camera
//
// when sent to the IBIS1300 camera, the command start and exposure
// with the main imager section of the chip.
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_cmd_startExposure(int camNum);

// send the 'start exposure' command to the guider portion of the
// IBIS1300 image sensor.    This command is not recognized by the Starfish camera
//
int fcUsb_cmd_startGuiderExposure(int camNum);

// send the 'abort exposure' command to the camera
//
int fcUsb_cmd_abortExposure(int camNum);

// send the 'abort Guider exposure' command to the camera
// This command is not recognized by the Starfish camera
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_cmd_abortGuiderExposure(int camNum);

// send a command to get the current camera state
//
UInt16 fcUsb_cmd_getState(int camNum);

// send a command to get the current camera state of the guider sensor
// This command is not recognized by the Starfish camera
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
// return values are:
//	0 - idle
//	1 - integrating
//	2 - processing image
//
UInt16 fcUsb_cmd_getGuiderState(int camNum);

// turn on/off the frame grabber's test pattern generator
// 0 = off, 1 = on
//
int fcUsb_cmd_setFrameGrabberTestPattern(int camNum, UInt16 state);

// here to read a specific scan line from the camera's frame grabber buffer
//
int fcUsb_cmd_rdScanLine(int camNum, UInt16 lineNum, UInt16 Xmin, UInt16 Xmax, UInt16 *lineBuffer);

// here to specify a new ROI to the sensor
// X = 0 -> 2047
// Y = 0 -> 1535
//
// This command is not supported by the IBIS1300 camera
//
int fcUsb_cmd_setRoi(int camNum, UInt16 left, UInt16 top, UInt16 right, UInt16 bottom);

// set the binning mode of the camera.  Valid binModes are 1, 2, 3
// valid camNum is 1 -> fcUsb_GetNumCameras()
// This command is not supported by the current starfish camera
//
int fcUsb_cmd_setBin(int camNum, UInt16 binMode);

// turn on one of the relays on the camera.  whichRelay is one of enum fc_relay
//
int fcUsb_cmd_setRelay(int camNum, int whichRelay);

// turn off one of the relays on the camera.  whichRelay is one of enum fc_relay
//
int fcUsb_cmd_clearRelay(int camNum, int whichRelay);

// generate a pulse on one of the relays on the camera.  whichRelay is one of enum fc_relay.
// pulse width parameters are in mS.  you can specify the hi and lo period of the pulse.
// if 'repeats' is true then the pulse will loop.
// call this routine with 'onMs' = 0 to abort any pulse operation in progress
//
int fcUsb_cmd_pulseRelay(int camNum, int whichRelay, int onMs, int offMs, bool repeats);

// Tell the camera what temperature setpoint to use for the TEC controller
// this will also turn on cooling to the camera.
//
// Specify the temperature with a signed integer that is 100x the actual
// temperature required. This allows us to represent a temperature with
// 2 decimal places of accuracy while still passing an integer value.
//
int fcUsb_cmd_setTemperature(int camNum, SInt16 theTemp);

// Get the temperature of the image sensor.
//
// Will return a signed integer that is 100x the actual sensor temperature.
// This allows us to represent a temperature with 2 decimal places of
// accuracy while still passing an integer value.
//
SInt16 fcUsb_cmd_getTemperature(int camNum);

// will return a number from 0 -> 100;  To be interpreted as a percent.
//
UInt16 fcUsb_cmd_getTECPowerLevel(int camNum);

// will return TRUE if the TEC power cable is plugged into the camera
//
bool fcUsb_cmd_getTECInPowerOK(int camNum);

// command camera to turn off the TEC cooler
//
int fcUsb_cmd_turnOffCooler(int camNum);

// here to read an entire frame in RAW format
//
int fcUsb_cmd_getRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer);

// only the 'fc_classicDataXfr' data transfer mode is supported on the PC platform.

// here to define some image readout modes of the camera.  The state of these bits will be
// used during the call to fcUsb_cmd_startExposure.  When an exposure is started and subsequent
// image readout is begun, the camera will assume an implied fcUsb_cmd_getRawFrame command
// when the 'DataXfrReadMode' is a '1' or '2' and begin uploading pixel data to the host as the image
// is being read from the sensor.
//
// DataFormat can be one of 8, 10, 12, 14, 16
//     8 - packed into a single byte
//     others - packed into a 16 bit word
//
int fcUsb_cmd_setReadMode(int camNum, int DataXfrReadMode, int DataFormat);

// set some register defaults for the starfish camera
void fcUsb_setStarfishDefaultRegs(int camNum);

// here to set the analog gain on the camera.
//
// theGain - the gain number desired.
//
// Valid gains are between 0 and 15.  For the IBIS camera this corresponds to 1.28 to 17.53 as per the sensor data sheet
//
int fcUsb_cmd_setCameraGain(int camNum, UInt16 theGain);

// here to set the analog offset on the camera.
//
// theOffset - the offset number desired.
//
// Valid offsets are between 0 and 15.
//
int fcUsb_cmd_setCameraOffset(int camNum, UInt16 theOffset);

// here to set the filter type used for image processing
// The specified filter will be performed on any images
// transfered from the camera.
//
// 'theImageFilter' is one of fc_imageFilter
//
void fcUsb_cmd_setImageFilter(int camNum, int theImageFilter);

// call to get the black level pedestal of the image sensor.  This is usefull if you wish
// to subtract out the pedestal from the image returned from the camera.
//
// the return value will be scalled according to the currently set fc_dataFormat. See the
// fcUsb_cmd_setReadMode routine for information on the pixel format
//
UInt16 fcUsb_cmd_getBlackPedestal(int camNum);

// call this routine to set a camera property.  Various cameras support different properties
// the following properties are defined:
//
//  1) fcPROP_NUMSAMPLES - enter with 'propertyValue' set to the number desired.  Valid
//                         numbers are binary numbers starting with 4 (4, 8, 16, 32, 64, 128)
//
//  2) fcPROP_PATTERNENABLE - enter with 'propertyValue' set to '1' to enable the frame grabber
//                         pattern generator.  '0' to get true sensor data
//
//  3) fcPROP_PIXCAPTURE - enter with 'propertyValue' set to '0' for normal CDS pixel sampling,
//                         '1' for pixel level only and '2' for reset level only.
//
//  4) fcPROP_SHUTTERMODE - enter with 'propertyValue' set to '0' for AUTOMATIC shutter,
//                         '1' for MANUAL shutter.
//
//  5) fcPROP_SHUTTERPRIORITY - enter with 'propertyValue' set to '0' for MECHANICAL shutter priority,
//                         '1' for ELECTRONIC shutter priority
//
//  6) fcPROP_MOVESHUTTERFLAG = enter with 'propertyValue' set to '0' to CLOSE the mechanical shutter,
//                         '1' to OPEN the mechanical shutter
//
//  7) fcPROP_COLNORM - enter with 'propertyValue' set to '0' for no column level normalization
//                         '1' for column level normalization.
//
//  8) fcPROP_SERVOOPENPOS - enter with 'propertyValue' set to servo pulse period used for the OPEN position
//
//  9) fcPROP_SERVOCLOSEDPOS - enter with 'propertyValue' set to servo pulse period used for the CLOSED position
//
//
void fcUsb_cmd_setCameraProperty(int camNum, int propertyType, int propertyValue);
