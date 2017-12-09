/*****************************************************************************************
NAME
 Global

DESCRIPTION
 Contains global constants (including error conditions) and type definitions

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 MWB 04.16.06 Original Version
*****************************************************************************************/

#pragma once
#include "WinTypes.h"
#include "wincompat.h"
#include "FilterWheel.h"
#include <string>

//Optionally include Logging code
#define LOG

//****************************************************************************************
// Constants
//****************************************************************************************

const int MAX_DEVICES = 31;       // Maximum devices supported
const int MAX_PKT_LENGTH = 128;   // Maximum packet length (in bytes)
const int READ_TIMEOUT = 15000;    // Amount of time in milliseconds
const int WRITE_TIMEOUT = 5000;   // Amount of time in milliseconds 
const int SHORT_READ_TIMEOUT = 1000;
const int SHORT_WRITE_TIMEOUT = 1000;
const int LONG_READ_TIMEOUT = 20000;
const int LONG_WRITE_TIMEOUT = 20000;
const int MINIMUM_READ_TIMEOUT = 1000;
const int MINIMUM_WRITE_TIMEOUT = 1000;

const int PKT_COMMAND = 0;        // Offset to packet command byte
const int PKT_LENGTH = 1;         // Offset to packet length byte
const int PKT_HEAD_LENGTH = 2;    // Number of bytes for the packet header

const int AUTOZEROSATTHRESHOLD = 10000;
const int AUTOZEROMAXADU = 64000;
const int AUTOZEROSKIPSTARTPIXELS = 32;
const int AUTOZEROSKIPENDPIXELS = 32;

//****************************************************************************************
// TYPE DEFINITIONS
//****************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_CCDSpecs_t
{
	double  minExp;
	double  maxExp;
	int	  MaxADU;
	double  EADUHigh;
	double  EADULow;
	double  EFull;
} QSI_CCDSpecs;

//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_DeviceDetails_t
{
	bool    HasCamera = 0;
	bool    HasShutter = 0;
	bool    HasFilter = 0;
	bool    HasRelays = 0;
	bool    HasTempReg = 0;
	int     ArrayColumns = 0;
	int     ArrayRows = 0;
	double  XAspect = 0;
	double  YAspect = 0;
	int     MaxHBinning = 0;
	int     MaxVBinning = 0;
	bool    AsymBin = 0;
	bool    TwoTimesBinning = 0;
	USHORT  NumRowsPerBlock = 0;    // Not currently used; calculated in "QSI_PlugIn::TransferImage" function, see "iPixelsPerRead"
	bool    ControlEachBlock = 0;   // Not currently used; handled by "Show D/L Progress" in Advanced Dialog
	int     NumFilters = 0;
	char    cModelNumber[33] = "";
	char    cModelName[33] = "";
	char    cSerialNumber[33] = "";
	bool    HasFilterTrim = 0;
	bool	HasCMD_GetTemperatureEx = 0;
	bool	HasCMD_StartExposureEx = 0;
	bool	HasCMD_SetFilterTrim = 0;
	bool	HasCMD_HSRExposure = 0;
	bool	HasCMD_PVIMode = 0;
	bool	HasCMD_LockCamera = 0;
	bool	HasCMD_BasicHWTrigger = 0;
	// From Feature bytes
	std::string ModelBaseNumber;	// "683" or "RS8.3"
	std::string ModelNumber;		// "683ws" or "RS8.3ws" for user display
	// from low eeprom config memory:
	std::string ModelBaseType;		// "504" Model number, leading numeric digits for matching hex files
	std::string	ModelType;			// "504ws" Full model type number field
	std::string ModelName;			// "QSI 500 Series Camera"
	std::string SerialNumber;		// "05001234"
	//
} QSI_DeviceDetails;

//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_ExposureSettings_t
{
  UINT Duration = 0;
  BYTE DurationUSec = 0;
  int ColumnOffset = 0;
  int RowOffset = 0;
  int ColumnsToRead = 0;
  int RowsToRead = 0;
  int BinFactorX = 0;
  int BinFactorY = 0;
  bool OpenShutter = 0;
  bool FastReadout = 0;
  bool HoldShutterOpen = 0;
  bool UseExtTrigger = 0;
  bool StrobeShutterOutput = 0;
  int ExpRepeatCount = 0;
  bool ProbeForImplemented = 0;
} QSI_ExposureSettings;

//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_AdvEnabledOptions_t
{
  bool LEDIndicatorOn = 0;
  bool SoundOn = 0;
  bool FanMode = 0;
  bool CameraGain = 0;
  bool ShutterPriority = 0;
  bool AntiBlooming = 0;
  bool PreExposureFlush = 0;
  bool ShowDLProgress = 0;
  bool Optimizations = 0;
} QSI_AdvEnabledOptions;
//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_FilterDesc_t
{
	char	Name[32] = "";
	long	FocusOffset = 0;
} QSI_FilterDesc;
//////////////////////////////////////////////////////////////////////////////////////////
// ...
typedef struct QSI_AdvSettings_t
{
  bool LEDIndicatorOn = 0;
  bool SoundOn = 0;
  bool ShowDLProgress = 0;
  bool OptimizeReadoutSpeed = 0;
  int FanModeIndex = 0;
  int CameraGainIndex = 0;
  int ShutterPriorityIndex = 0;
  int AntiBloomingIndex = 0;
  int PreExposureFlushIndex = 0;
  bool FilterTrimEnabled = 0;
  FilterWheel fwWheel;
} QSI_AdvSettings;

typedef struct QSI_AutoZeroData_t
{
	bool zeroEnable = 0;
	USHORT zeroLevel = 0;
	USHORT pixelCount = 0;
} QSI_AutoZeroData;

typedef struct QSI_IOTimeouts_t
{
	int ShortRead = 0;
	int ShortWrite = 0;
	int StandardRead = 0;
	int StandardWrite = 0;
	int ExtendedRead = 0;
	int ExtendedWrite = 0;
} QSI_IOTimeouts;

typedef enum QSICameraState_t
{							// Highest priority at top of list
	CCD_ERROR				= 0,	// Camera is not available
	CCD_FILTERWHEELMOVING	= 1,	// Waiting for filter wheel to finish moving
	CCD_FLUSHING			= 2,	// Flushing CCD chip or camera otherwise busy
	CCD_WAITTRIGGER			= 3,	// Waiting for an external trigger event
	CCD_DOWNLOADING			= 4,	// Downloading the image from camera hardware
	CCD_READING				= 5,	// Reading the CCD chip into camera hardware
	CCD_EXPOSING			= 6,	// Exposing dark or light frame
	CCD_IDLE				= 7,	// Camera idle
}QSICameraState;

typedef enum QSICoolerState_t
{
	COOL_OFF				= 0,	// Cooler is off
	COOL_ON					= 1,	// Cooler is on
	COOL_ATAMBIENT			= 2,	// Cooler is on and regulating at ambient temperature (optional)
	COOL_GOTOAMBIENT		= 3,	// Cooler is on and ramping to ambient
	COOL_NOCONTROL			= 4,	// Cooler cannot be controlled on this camera open loop
	COOL_INITIALIZING		= 5,	// Cooler control is initializing (optional -- displays "Please Wait")
	COOL_INCREASING			= 6,	// Cooler temperature is going up    NEW 2000/02/04
	COOL_DECREASING			= 7,	// Cooler temperature is going down  NEW 2000/02/04
	COOL_BROWNOUT			= 8,	// Cooler brownout condition  NEW 2001/09/06
}QSICoolerState;

//****************************************************************************************
// Error constants from FTDI, repeated here for reference.
//****************************************************************************************
/*
FT_STATUS (DWORD)
FT_OK = 0
FT_INVALID_HANDLE = 1
FT_DEVICE_NOT_FOUND = 2
FT_DEVICE_NOT_OPENED = 3
FT_IO_ERROR = 4
FT_INSUFFICIENT_RESOURCES = 5
FT_INVALID_PARAMETER = 6
FT_INVALID_BAUD_RATE = 7
FT_DEVICE_NOT_OPENED_FOR_ERASE = 8
FT_DEVICE_NOT_OPENED_FOR_WRITE = 9
FT_FAILED_TO_WRITE_DEVICE = 10
FT_EEPROM_READ_FAILED = 11
FT_EEPROM_WRITE_FAILED = 12
FT_EEPROM_ERASE_FAILED = 13
FT_EEPROM_NOT_PRESENT = 14
FT_EEPROM_NOT_PROGRAMMED = 15
FT_INVALID_ARGS = 16
FT_NOT_SUPPORTED = 17
FT_OTHER_ERROR = 18
*/
//////////////////////////////////////////////////////////////////////////////////////////
//
enum QSI_ReturnStates
{
	ALL_OK = 0,

	ERR_CAM_OverTemp		= 1,
	ERR_CAM_UnderTemp		= 2,
	ERR_CAM_UnderVolt		= 3,
	ERR_CAM_OverVolt		= 4,
	ERR_CAM_Filter			= 5,
	ERR_CAM_Shutter			= 6,

	ERR_USB_Load 			= 50,
	ERR_USB_LoadFunction 	= 51,
	ERR_USB_OpenFailed		= 100,
	ERR_USB_WriteFailed		= 101,
	ERR_USB_ReadFailed		= 102,

	// Packet Interface
	ERR_PKT_OpenFailed        =  200,    // Open device failed
	ERR_PKT_SetTimeOutFailed  =  300,    // Set timeouts (Rx & Tx) failed
	ERR_PKT_CloseFailed       =  400,    // Close device failed
	ERR_PKT_CheckQueuesFailed =  500,    // Check of Tx and Rx queues failed
	ERR_PKT_BothQueuesDirty   =  600,    // Both Rx and Tx queues dirty
	ERR_PKT_RxQueueDirty      =  700,    // Rx queue dirty
	ERR_PKT_TxQueueDirty      =  800,    // Tx queue dirty
	ERR_PKT_SendInitFailed    =  900,    // 
	ERR_PKT_TxPacketTooLong   = 1000,   // Length of Tx packet is greater than MAX_PKT_LENGTH
	ERR_PKT_TxFailed          = 1100,   // Write of Tx packet failed (header+data)
	ERR_PKT_TxNone            = 1200,   // None of Tx packet was sent
	ERR_PKT_TxTooLittle       = 1300,   // Not all of Tx packet data was sent
	ERR_PKT_RxHeaderFailed    = 1400,   // Read of Rx packet header failed
	ERR_PKT_RxBadHeader       = 1500,   // Tx command and Rx command did not match
	ERR_PKT_RxPacketTooLong   = 1600,   // Length of Rx packet is greater than MAX_PKT_LENGTH
	ERR_PKT_RxFailed          = 1700,   // Read of Rx packet data failed
	ERR_PKT_RxNone            = 1800,   // None of Rx packet was read
	ERR_PKT_RxTooLittle       = 1900,   // Not all of Rx packet data was received
	ERR_PKT_BlockInitFailed   = 2100,   // 
	ERR_PKT_BlockRxFailed     = 2200,   // 
	ERR_PKT_BlockRxTooLittle  = 2300,   // 
	ERR_PKT_SetLatencyFailed  = 2400,
	ERR_PKT_ResetDeviceFailed = 2500,
	ERR_PKT_SetUSBParmsFailed = 2600,
	ERR_PKT_NoConnection	  = 2700,

	// Device Interface
	ERR_IFC_InitCamera        =  10000,  // 
	ERR_IFC_GetDeviceDetails  =  20000,  // 
	ERR_IFC_StartExposure     =  30000,  // 
	ERR_IFC_AbortExposure     =  40000,  //
	ERR_IFC_TransferImage     =  50000,  // 
	ERR_IFC_ReadBlock         =  60000,  // 
	ERR_IFC_GetDeviceState    =  70000,  // 
	ERR_IFC_SetTemperature    =  80000,  // 
	ERR_IFC_GetTemperature    =  90000,  // 
	ERR_IFC_ActivateRelay     = 100000, // 
	ERR_IFC_IsRelayDone       = 110000, // 
	ERR_IFC_SetFilterWheel    = 120000, // 
	ERR_IFC_CameraNotOpen     = 130000, // 
	ERR_IFC_FilterNotOpen     = 140000, // 
	ERR_IFC_CameraError       = 150000, // 
	ERR_IFC_CameraHasNoFilter = 160000, // 
	ERR_IFC_FilterAlreadyOpen = 170000, // 
	ERR_IFC_Initialize        = 180000, // 
	ERR_IFC_CountDevices      = 190000, //
	ERR_IFC_ListSerial        = 200000, // 
	ERR_IFC_ListDescription   = 210000, // 
	ERR_IFC_ListMismatch      = 220000, // 
	ERR_IFC_GetDeviceInfo     = 230000, //
	ERR_IFC_AbortRelays	  	  = 240000, //
	ERR_IFC_GetLastExposure   = 250000, //
	ERR_IFC_CanAbortExposure  = 260000, //
	ERR_IFC_CanStopExposure   = 270000, //
	ERR_IFC_GetFilterPosition = 280000, //
	ERR_IFC_GetCCDSpecs	      = 290000, //
	ERR_IFC_GetAdvDetails	  = 300000,	//
	ERR_IFC_NegAutoZero	  	  = 310000,	//
	ERR_IFC_SendAdvSettings   = 320000, //
	ERR_IFC_TriggerCCDError   = 330000, //
	ERR_IFC_NotSupported	  = 340000, //
	ERR_IFC_GetShutterStateError	  = 350000
};



