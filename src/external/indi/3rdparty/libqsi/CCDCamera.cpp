/*****************************************************************************************
NAME
 CCDCamera.cpp : Implementation of CCCDCamera

DESCRIPTION


COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 DRC 12.19.06 Original Version
*****************************************************************************************/
#include <algorithm>
#include <cctype> 
#include <string>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <math.h>
#include <string.h>

#include "QSI_Global.h"
#include "QSI_Interface.h"
#include "QSIError.h"
#include "QSI_States.h"
#include "QSI_Registry.h"
#include "CCDCamera.h"
#include "qsiapi.h"

QSICriticalSection CCCDCamera::csQSI;

CCCDCamera::CCCDCamera()
{
	m_pusBuffer						= NULL;
	m_bIsConnected					= false;
	m_bIsMainCamera					= true;
	m_USBSerialNumber 				= "";
	m_DeviceDetails.HasCamera		= false;
	m_DeviceDetails.HasShutter		= false;
	m_DeviceDetails.HasFilter		= false;
	m_DeviceDetails.HasRelays		= false;
	m_DeviceDetails.HasTempReg		= false;
	m_DeviceDetails.ArrayColumns	= 0;
	m_DeviceDetails.ArrayRows		= 0;
	m_DeviceDetails.XAspect			= 0;
	m_DeviceDetails.YAspect			= 0;
	m_DeviceDetails.MaxHBinning		= 1;
	m_DeviceDetails.MaxVBinning		= 1;
	m_DeviceDetails.AsymBin			= false;
	m_DeviceDetails.TwoTimesBinning	= false;
	m_DeviceDetails.NumRowsPerBlock	= 0;
	m_DeviceDetails.ControlEachBlock= false;
	m_DeviceDetails.NumFilters		= 0;
	m_DeviceDetails.ModelBaseNumber		= _T("");
	m_DeviceDetails.ModelBaseType		= _T("");
	m_DeviceDetails.ModelName			= _T("");
	m_DeviceDetails.ModelNumber			= _T("");
	m_DeviceDetails.ModelType			= _T("");
	m_DeviceDetails.SerialNumber		= _T("");
	m_ExposureSettings.Duration         = ((UINT)(0.5*100.0));
	m_ExposureSettings.ColumnOffset     = 0;
	m_ExposureSettings.RowOffset        = 0;
	m_ExposureSettings.ColumnsToRead	= 0;
	m_ExposureSettings.RowsToRead       = 0;
	m_ExposureSettings.BinFactorX       = 1;
	m_ExposureSettings.BinFactorY       = 1;
	m_ExposureSettings.OpenShutter      = true;
	m_ExposureSettings.FastReadout      = false;
	m_ExposureSettings.HoldShutterOpen  = false;
	m_ExposureSettings.DurationUSec		= 0;
	m_ExposureSettings.UseExtTrigger	= false;
	m_ExposureSettings.StrobeShutterOutput = false;
	m_ExposureSettings.ExpRepeatCount   = 0;
	m_ExposureSettings.ProbeForImplemented = false;
	gettimeofday(&m_stStartExposure, NULL);
	m_bExposureTaken					= false;
	m_ExposureNumX						= 0;
	m_ExposureNumY						= 0;
	m_CurFilterPos						= 0;
	m_DownloadPending					= false;
	strncpy(m_szLastErrorText, "No Error", LASTERRORTEXTSIZE);
	m_iLastErrorValue					= 0;
	m_AdvEnabledOptions.LEDIndicatorOn	= true;
	m_AdvEnabledOptions.SoundOn			= true;
	m_AdvEnabledOptions.FanMode			= true;
	m_AdvEnabledOptions.CameraGain   	= false;
	m_AdvEnabledOptions.ShutterPriority	= false;
	m_AdvEnabledOptions.AntiBlooming	= false;
	m_AdvEnabledOptions.PreExposureFlush = true;
	m_AdvEnabledOptions.ShowDLProgress	= false;
	m_AdvEnabledOptions.Optimizations	= false;
	m_bStructuredExceptions				= true;

	QSI_Registry rReg;

	m_usLastOverscanMean = 0;
	m_bImageValid = false;
	m_dLastDuration = 0.0;
	m_USBSerialNumber = std::string( "" );
	m_dLastOverscanMean = 0;
	m_dOverscanAdjustment = 0;
	m_verMaintenance = 0;
	m_iOverscanAdjustment = 0;
	m_verAux = 0;
	m_verMajor = 0;
	m_verMinor = 0;

	m_iError = 0;
}

CCCDCamera::~CCCDCamera()
{

}

int  CCCDCamera::get_BinX(short* pVal)
{
	// 
	// BinX
	// ----
	// 
	// Property
	//             CCDCamera.BinX
	// Syntax
	//             CCDCamera.BinX (Short)
	// Exceptions
	//             Must throw an exception for illegal binning values
	// 
	// Remarks
	// 
	// Sets the binning factor for the X axis.  Also returns the current value.  Should
	// default to 1 when the camera link is established.  Note:  driver does not check
	// for compatible sub-frame values when this value is set; rather they are checked
	// upon StartExposure.
	// 

	*pVal = (short)m_ExposureSettings.BinFactorX;

	return S_OK;
}

int  CCCDCamera::put_BinX(SHORT newVal)
{
	// 
	// BinX
	// ----
	// 
	// Property
	//             CCDCamera.BinX
	// Syntax
	//             CCDCamera.BinX (Short)
	// Exceptions
	//             Must throw an exception for illegal binning values
	// 
	// Remarks
	// 
	// Sets the binning factor for the X axis.  Also returns the current value.  Should
	// default to 1 when the camera link is established.  Note:  driver does not check
	// for compatible sub-frame values when this value is set; rather they are checked
	// upon StartExposure.
	// 

	if (newVal <= 0 || newVal > (short)m_DeviceDetails.MaxHBinning)
		return Error ( "Invalid Bin Size", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADBINSIZE) );

	m_ExposureSettings.BinFactorX = (int)newVal;

	return S_OK;
}

int  CCCDCamera::get_BinY(short* pVal)
{
	// 
	// BinY
	// ----
	// 
	// Property
	//             CCDCamera.BinY
	// Syntax
	//             CCDCamera.BinY (Short)
	// Exceptions
	//             Must throw an exception for illegal binning values
	// 
	// Remarks
	// 
	// Sets the binning factor for the Y axis  Also returns the current value.  Should
	// default to 1 when the camera link is established.  Note:  driver does not check
	// for compatible sub-frame values when this value is set; rather they are checked
	// upon StartExposure.
	// 

	*pVal = (short)m_ExposureSettings.BinFactorY;

	return S_OK;
}

int  CCCDCamera::put_BinY(SHORT newVal)
{
	// 
	// BinY
	// ----
	// 
	// Property
	//             CCDCamera.BinY
	// Syntax
	//             CCDCamera.BinY (Short)
	// Exceptions
	//             Must throw an exception for illegal binning values
	// 
	// Remarks
	// 
	// Sets the binning factor for the Y axis  Also returns the current value.  Should
	// default to 1 when the camera link is established.  Note:  driver does not check
	// for compatible sub-frame values when this value is set; rather they are checked
	// upon StartExposure.
	// 

	if (newVal <= 0 || newVal > (short)m_DeviceDetails.MaxVBinning)
		return Error ( "Invalid Bin Size", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADBINSIZE) );

	m_ExposureSettings.BinFactorY = (int)newVal;

	return S_OK;
}

int  CCCDCamera::get_CameraState(CameraState* pVal)
{
	// 
	// CameraState
	// -----------
	// 
	// Property
	//             CCDCamera.CameraState (read only, enumeration)
	// Syntax
	//             CCDCamera.CameraState
	// Exceptions
	//             Must return an exception if the camera status is unavailable.
	// 
	// Remarks
	// 
	// Returns one of the following status information:
	// 
	// Value  State          Meaning
	// -----  -----          -------
	// 0      CameraIdle      At idle state, available to start exposure
	// 1      CameraWaiting   Exposure started but waiting (for shutter, trigger,
	//                         filter wheel, etc.)
	// 2      CameraExposing  Exposure currently in progress
	// 3      CameraReading   CCD array is being read out (digitized)
	// 4      CameraDownload  Down loading data to PC
	// 5      CameraError     Camera error condition serious enough to prevent
	//                         further operations (link fail, etc.).
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Declare variables to use with following call
	bool bFilterState = false;	// Temporary--value not returned to MaxImDL
	bool bShutterOpen = false;	// Temporarily holds value returned from camera
	int iState = 0;				// Temporarily holds value returned from camera

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetDeviceState ( iState, bShutterOpen, bFilterState );
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Get Camera State", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
	//CameraIdle		= 0,
	//CameraWaiting		= 1,
	//CameraExposing	= 2,
	//CameraReading		= 3,
	//CameraDownload	= 4,
	//CameraError		= 5
	//
	switch (iState)
	{
		case CCD_ERROR:
			*pVal = CameraError;
			break;
		case CCD_FILTERWHEELMOVING:
			*pVal = CameraWaiting;
			break;
		case CCD_FLUSHING:
			*pVal = CameraWaiting;
			break;
		case CCD_WAITTRIGGER:
			*pVal = CameraWaiting;
			break;
		case CCD_DOWNLOADING:
			*pVal = CameraDownload;
			break;
		case CCD_READING:
			*pVal = CameraReading;
			break;
		case CCD_EXPOSING:
			*pVal = CameraExposing;
			break;
		case CCD_IDLE:
			*pVal = CameraIdle;
			break;
		default:
			*pVal = CameraIdle;
			break;
	}

	return S_OK;
}

int  CCCDCamera::get_CameraXSize(long* pVal)
{
	// 
	// CameraXSize
	// -----------
	// 
	// Property
	//             CCDCamera.CameraXSize (read only)
	// Syntax
	//             CCDCamera.CameraXSize()
	// Exceptions
	//             Must throw exception if the value is not known
	// 
	// Remarks
	// 
	// Returns the width of the CCD camera chip in un-binned pixels.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (long)m_DeviceDetails.ArrayColumns;

	return S_OK;
}

int  CCCDCamera::get_CameraYSize(long* pVal)
{
	// 
	// CameraYSize
	// -----------
	// 
	// Property
	//             CCDCamera.CameraYSize (read only)
	// Syntax
	//             CCDCamera.CameraYSize
	// Exceptions
	//             Must throw exception if the value is not known
	// 
	// Remarks
	// 
	// Returns the height of the CCD camera chip in un-binned pixels.
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (long)m_DeviceDetails.ArrayRows;

	return S_OK;
}

int  CCCDCamera::get_CanAbortExposure(bool* pVal)
{
	// 
	// CanAbortExposure
	// ----------------
	// 
	// Syntax
	//             CCDCamera.CanAbortExposure (read only)
	// Syntax
	//             CCDCamera.CanAbortExposure
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Returns True if the camera can abort exposures; False if not.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	bool bCanAbort;

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_CanAbortExposure ( bCanAbort );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Get Can Abort", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = bCanAbort;

	return S_OK;
}

int  CCCDCamera::get_CanAsymmetricBin(bool* pVal)
{
	// 
	// CanAsymmetricBin
	// ----------------
	// 
	// Property
	//             CCDCamera.CanAsymmetricBin (read only)
	// Syntax
	//             CCDCamera.CanAsymmetricBin
	// Exceptions
	//             Must throw exception if the value is not known (n.b. normally only
	//             occurs if no link established and camera must be queried)
	// 
	// Remarks
	// 
	// If True, the camera can have different binning on the X and Y axes, as
	// determined by BinX and BinY. If False, the binning must be equal on the X and Y
	// axes.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (bool)m_DeviceDetails.AsymBin;

	return S_OK;
}

int  CCCDCamera::get_CanGetCoolerPower(bool* pVal)
{
	// 
	// CanGetCoolerPower
	// ----------------
	// 
	// Property
	//             CCDCamera.CanGetCoolerPower (read only)
	// Syntax
	//             CCDCamera.CanGetCoolerPower
	// Exceptions
	//             Must throw exception if the value is not known (n.b. normally only
	//             occurs if no link established and camera must be queried)
	// 
	// Remarks
	// 
	// If True, the camera can report cooler power
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = true;

	return S_OK;
}

int  CCCDCamera::get_CanPulseGuide(bool* pVal)
{
	// 
	// CanPulseGuide
	// -------------
	// 
	// Syntax
	//             CCDCamera.CanPulseGuide (read only)
	// Syntax
	//             CCDCamera.CanPulseGuide
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Returns True if the camera can send auto-guider pulses to the telescope mount;
	// False if not.  (Note: this does not provide any indication of whether the
	// auto-guider cable is actually connected.)
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (bool)m_DeviceDetails.HasRelays;

	return S_OK;
}

int  CCCDCamera::get_CanSetCCDTemperature(bool* pVal)
{
	// 
	// CanSetCCDTemperature
	// --------------------
	// 
	// Property
	//             CCDCamera.CanSetCCDTemperature (read only)
	// Syntax
	//             CCDCamera.CanSetCCDTemperature
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// If True, the camera's cooler set point can be adjusted. If False, the camera
	// either uses open-loop cooling or does not have the ability to adjust temperature
	// from software, and setting the TemperatureSetpoint property has no effect.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (bool)m_DeviceDetails.HasTempReg;

	return S_OK;
}

int  CCCDCamera::get_CanStopExposure(bool* pVal)
{
	// 
	// CanStopExposure
	// ---------------
	// 
	// Syntax
	//             CCDCamera.CanStopExposure (read only)
	// Syntax
	//             CCDCamera.CanStopExposure
	// Exceptions
	//             Must throw exception if not supported
	//             Must throw exception if an error condition such as link
	//               failure is present
	// 
	// Remarks
	// 
	// Some cameras support StopExposure, which allows the exposure to be terminated
	// before the exposure timer completes, but will still read out the image.  Returns
	// True if StopExposure is available, False if not.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );


	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	bool bCanStop;

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_CanStopExposure ( bCanStop );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Get Can Stop", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = bCanStop;

	return S_OK;
}

int  CCCDCamera::get_CCDTemperature(double* pVal)
{
	// 
	// CCDTemperature
	// --------------
	// 
	// Property
	//             CCDCamera.CCDTemperature  (read only)
	// Syntax
	//             CCDCamera.CCDTemperature
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the current CCD temperature in degrees Celsius. Only valid if
	// CanControlTemperature is True.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	//CoolerState &State,        // Current temperature controller status
	int iState = 0;
	double Temp = 0;             // Temperature of CCD chip (FLT_MAX if not available)
	double TempAmbient = 0;      // Ambient temperature if available (FLT_MAX if not)
	unsigned short Power = 0;    // Power level in percent (0-100), return > 100 if not available

	if ( m_iError )
	{
		// Once the error is caught, we must clear it to allow another attempt at this command
		int err = m_iError;
		m_iError = 0;
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, err) );
	}

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetTemperature ( iState, Temp, TempAmbient, Power );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Get CCD Temperature", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = Temp;

	return S_OK;
}

int CCCDCamera::get_Connected(bool* pVal)	
{
	// 
	// Connected
	// ---------
	// 
	// Property
	//             CCDCamera.Connected
	// Syntax
	//             CCDCamera.Connected (Boolean)
	// Exceptions
	//             Must throw exception if unsuccessful.
	// 
	// Remarks
	// 
	// Controls the link between the driver and the camera. Set True to enable the
	// link. Set False to disable the link (this does not switch off the cooler).
	// You can also read the property to check whether it is connected.
	//
	*pVal = (bool)m_bIsConnected;
	return S_OK;
};

int  CCCDCamera::get_CoolerOn(bool* pVal)
{
	// 
	// CoolerOn
	// --------
	// 
	// Property
	//             CCDCamera.CoolerOn
	// Syntax
	//             CCDCamera.CoolerOn (Boolean)
	// Exceptions
	//             Must throw exception if not supported
	//             Must throw exception if an error condition such as link
	//               failure is present
	// 
	// Remarks
	// 
	// Turns on and off the camera cooler, and returns the current on/off state.
	// Warning: turning the cooler off when the cooler is operating at high delta-T
	// (typically >20C below ambient) may result in thermal shock.  Repeated thermal
	// shock may lead to damage to the sensor or cooler stack.  Please consult the
	// documentation supplied with the camera for further information.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	//CoolerState &State,        // Current temperature controller status
	int iState = 0;
	double Temp = 0;             // Temperature of CCD chip (FLT_MAX if not available)
	double TempAmbient = 0;      // Ambient temperature if available (FLT_MAX if not)
	unsigned short Power = 0;    // Power level in percent (0-100), return > 100 if not available

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetTemperature ( iState, Temp, TempAmbient, Power );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Get Cooler State", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = iState == 0?false:true;

	return S_OK;
}

int  CCCDCamera::put_CoolerOn(bool newVal)
{
	// 
	// CoolerOn
	// --------
	// 
	// Property
	//             CCDCamera.CoolerOn
	// Syntax
	//             CCDCamera.CoolerOn (Boolean)
	// Exceptions
	//             Must throw exception if not supported
	//             Must throw exception if an error condition such as link
	//               failure is present
	// 
	// Remarks
	// 
	// Turns on and off the camera cooler, and returns the current on/off state.
	// Warning: turning the cooler off when the cooler is operating at high delta-T
	// (typically >20C below ambient) may result in thermal shock.  Repeated thermal
	// shock may lead to damage to the sensor or cooler stack.  Please consult the
	// documentation supplied with the camera for further information.
	// 
	
	double dCurSetPoint;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

		// Check for previous error
	if ( m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetSetPoint( dCurSetPoint);
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Get Current Temp Set Point", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	//On		  GoToAmbient  Action
	//------------------------------------------------------
	//False     False        Cooler off
	//False     True         Cooler off
	//True      False        Cooler on, set temperature
	//True      True         Cooler on, warm CCD to ambient

	//Warm to ambient is a controlled temperature increased to ambient
	//at some safe rate that the camera decides, as opposed to just turning off the cooler.

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetTemperature ( newVal, /*GoToAmbient*/ false, dCurSetPoint );
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Change Cooler State", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int  CCCDCamera::get_CoolerPower(double* pVal)
{
	// 
	// CoolerPower
	// -----------
	// 
	// Property
	//             CCDCamera.CoolerPower (read only)
	// Syntax
	//             CCDCamera.CoolerPower(double)
	// Exceptions
	//             Must throw exception if not supported
	//             Must throw exception if an error condition such as link
	//               failure is present
	// 
	// Remarks
	// 
	// Returns the present cooler power level, in percent.  Returns zero if CoolerOn is
	// False.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	//CoolerState &State,        // Current temperature controller status
	int iState = 0;
	double Temp = 0;             // Temperature of CCD chip (FLT_MAX if not available)
	double TempAmbient = 0;      // Ambient temperature if available (FLT_MAX if not)
	unsigned short Power = 0;    // Power level in percent (0-100), return > 100 if not available
	bool vbCoolerOn = 0;

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Returns zero if cooler is off
	get_CoolerOn(&vbCoolerOn);
	if (vbCoolerOn==0)
	{
		*pVal = 0.0;
		return S_OK;
	}

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetTemperature ( iState, Temp, TempAmbient, Power );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Get CCD Temperature", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = Power;

	return S_OK;
}

int  CCCDCamera::get_Description(std::string& pVal)
{
	// 
	// Description
	// -----------
	// 
	// Property
	//             CCDCamera.Description (read only)
	// Syntax
	//             CCDCamera.Description(& std::string)
	// Exceptions
	//             Must throw exception if description unavailable
	// 
	// Remarks
	// 
	// Returns a description of the camera model, such as manufacturer and model
	// number. Any ASCII characters may be used. The string shall not exceed 68
	// characters (for compatibility with FITS headers).
	// 

	if (!m_bIsConnected)
	{
		pVal = std::string("Camera not connected");
		return S_OK;
	}
	
	std::string info = m_DeviceDetails.ModelNumber;
	info.append(_T(" HW "));
	info.append(m_HWVersion);
	info.append(_T(" FW "));
	info.append(m_FWVersion);
	
	pVal = std::string("QSI ") + info;
	return S_OK;
}

int  CCCDCamera::get_ElectronsPerADU(double* pVal)
{
	// 
	// ElectronsPerADU
	// ---------------
	// 
	// Property
	//             CCDCamera.ElectronsPerADU (read only)
	// Syntax
	//             CCDCamera.ElectronsPerADU(double)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the gain of the camera in photo-electrons per A/D unit. (Some cameras have
	// multiple gain modes; these should be selected via the SetupDialog and thus are
	// static during a session.)
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	double gain;

	if (m_AdvEnabledOptions.CameraGain)
	{
		if (m_AdvSettings.CameraGainIndex==1)
			gain = m_QSIInterface.m_CCDSpecs.EADULow;
		else
			gain = m_QSIInterface.m_CCDSpecs.EADUHigh;
	}
	else
	{
		gain = m_QSIInterface.m_CCDSpecs.EADUHigh;
	}

	*pVal = gain;

	return S_OK;
}

int  CCCDCamera::get_FullWellCapacity(double* pVal)
{
	// 
	// FullWellCapacity
	// ---------------
	// 
	// Property
	//             CCDCamera.FullWellCapacity (read only)
	// Syntax
	//             CCDCamera.FullWellCapacity(double)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Reports the full well capacity of the camera in electrons, at the current camera
	// settings (binning, SetupDialog settings, etc.)
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = m_QSIInterface.m_CCDSpecs.EFull;

	return S_OK;
}

int  CCCDCamera::get_HasShutter(bool* pVal)
{
	// 
	// HasShutter
	// ----------
	// 
	// Property
	//             CCDCamera.HasShutter (read only)
	// Syntax
	//             CCDCamera.HasShutter(bool)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// If True, the camera has a mechanical shutter. If False, the camera does not have
	// a shutter.  If there is no shutter, the StartExposure command will ignore the
	// Light parameter.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (bool)m_DeviceDetails.HasShutter;

	return S_OK;
}

int  CCCDCamera::get_HeatSinkTemperature(double* pVal)
{
	// 
	// HeatSinkTemperature
	// -------------------
	// 
	// Property
	//             CCDCamera.HeatSinkTemperature  (read only)
	// Syntax
	//             CCDCamera.HeatSinkTemperature(double *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the current heat sink temperature (called "ambient temperature" by some
	// manufacturers) in degrees Celsius. Only valid if CanControlTemperature is True.
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	//CoolerState &State,        // Current temperature controller status
	int iState = 0;
	double Temp = 0;             // Temperature of CCD chip (FLT_MAX if not available)
	double TempAmbient = 0;      // Ambient temperature if available (FLT_MAX if not)
	unsigned short Power = 0;    // Power level in percent (0-100), return > 100 if not available

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetTemperature ( iState, Temp, TempAmbient, Power );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ("Cannot Get Ambient Temperature", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = TempAmbient;

	return S_OK;
}

int (CCCDCamera::get_ImageArraySize)(int& xSize, int& ySize, int& elementSize)
{
	// 
	// ImageArraySize
	// ----------
	// 
	// Property
	//             CCDCamera.ImageArraySize  (read only)
	// Syntax
	//             CCDCamera.ImageArraySize(x,y, elementSize)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the size parameters of the pending image. X,y in pixels, elementSize in bytes
	// 
	// 

	if ( !m_bIsConnected )
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if ( !m_bExposureTaken )
		return Error ( "No Exposure Taken", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOIMAGEAVAILABLE) );

	if ( ! ( m_DownloadPending || m_bImageValid ) )
		return Error ( "No Image Available", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOIMAGEAVAILABLE) );

	xSize = m_ExposureNumX;
	ySize = m_ExposureNumY;
	elementSize = 2;

	return S_OK;
}

int  CCCDCamera::get_ImageArray(unsigned short* pVal)
{
	// 
	// ImageArray
	// ----------
	// 
	// Property
	//             CCDCamera.ImageArray  (read only)
	// Syntax
	//             CCDCamera.ImageArray(short *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns a array of short of size NumX * NumY containing the pixel values from
	// the last exposure. The application must allocate the buffer for the image
	// values based on the parameters returned from the get-ImageArraySize call.
	// 
	// 

	if ( !m_bIsConnected )
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	FillImageBuffer(true); // Retrieve data from the camera

	if ( !m_bImageValid )
		return Error ( _T("No Image Available"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOIMAGEAVAILABLE) );

	m_iError = m_QSIInterface.AdjustZero(m_pusBuffer, pVal, m_ExposureSettings.ColumnsToRead, m_ExposureSettings.RowsToRead, m_iOverscanAdjustment, m_AutoZeroData.zeroEnable);
	return S_OK;
}

int CCCDCamera::get_ImageArray(double* pVal)
{
	// 
	// ImageArray
	// ----------
	// 
	// Property
	//             CCDCamera.ImageArray  (read only)
	// Syntax
	//             CCDCamera.ImageArray(double*)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns a array of double of size NumX * NumY containing the pixel values from
	// the last exposure. The application must allocate the buffer for the image
	// values based on the parameters returned from the get-ImageArraySize call.
	// 
	// 

	if ( !m_bIsConnected )
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	FillImageBuffer(true);

	if ( !m_bImageValid )
		return Error ( _T("No Image Available"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOIMAGEAVAILABLE) );

	m_iError = m_QSIInterface.AdjustZero(m_pusBuffer, pVal, m_ExposureSettings.ColumnsToRead, m_ExposureSettings.RowsToRead, m_dOverscanAdjustment, m_AutoZeroData.zeroEnable);
	return S_OK;
}

int  CCCDCamera::get_ImageReady(bool* pVal)
{
	// 
	// ImageReady
	// ----------
	// 
	// Property
	//             CCDCamera.ImageReady (read only)
	// Syntax
	//             CCDCamera.ImageReady(bool)
	// Exceptions
	//             Must throw exception if hardware or communications link error
	//               has occurred.
	// 
	// Remarks
	// 
	// If True, there is an image from the camera available. If False, no image
	// is available and attempts to use the ImageArray method will produce an
	// exception.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_bExposureTaken)
	{
		*pVal = false;
		return S_OK;
	}

	// Declare variables to use with following call
	bool bFilterState = false;	// Temporary--value not returned to MaxImDL
	bool bShutterOpen = false;	// Temporarily holds value returned from camera
	int iState = 0;				// Temporarily holds value returned from camera

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetDeviceState ( iState, bShutterOpen, bFilterState );
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Get Camera State", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (iState==CCD_ERROR)
	{
		return Error ( "Trigger Timeout", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_TRIGGERTIMEOUT) );
	}

	*pVal = iState==CCD_IDLE?true:false;

	return S_OK;
}

int  CCCDCamera::get_IsPulseGuiding(bool* pVal)
{
	// 
	// IsPulseGuiding
	// --------------
	// 
	// Property
	//             CCDCamera.IsPulseGuiding (read only)
	// Syntax
	//             CCDCamera.IsPulseGuiding(bool *)
	// Exceptions
	//             Must throw exception if hardware or communications link error
	//               has occurred.
	// 
	// Remarks
	// 
	// If True, pulse guiding is in progress. Required if the PulseGuide() method
	// (which is non-blocking) is implemented. See the PulseGuide() method.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Declare variable to use with following call
	bool bIsRelayState = false;

	// Check for previous error
	if( this->m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	csQSI.Lock();
	this->m_iError = m_QSIInterface.CMD_IsRelayDone( bIsRelayState );
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot Get Guiding Status", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = !bIsRelayState;

	return S_OK;
}

int  CCCDCamera::get_LastError(std::string& pVal)
{
	// 
	// LastError
	// ---------
	// 
	// Property
	//             CCDCamera.LastError (read only)
	// Syntax
	//             CCDCamera.LastError(std::string &)
	// Exceptions
	//             
	// 
	// Remarks
	// 
	// Reports the last error condition reported by the camera hardware or communications
	// link.  The string may contain a text message or simply an error code.  The error
	// value is cleared the next time any method is called.
	// 

	char tcsBuf[20];
	sprintf(tcsBuf, "0x%08x", m_iLastErrorValue);
	
	std::string bsVal(tcsBuf);
	pVal = bsVal + std::string(": ") + std::string(m_szLastErrorText);

	m_szLastErrorText[0] = 0;
	m_iLastErrorValue = 0;

	return S_OK;
}

int  CCCDCamera::get_LastExposureDuration(double* pVal)
{
	// 
	// LastExposureDuration
	// --------------------
	// 
	// Property
	//             CCDCamera.LastExposureDuration (read only)
	// Syntax
	//             CCDCamera.LastExposureDuration(double*)
	// Exceptions
	//             Must throw exception if not supported or no exposure
	//               has been taken
	// 
	// Remarks
	// 
	// Reports the actual exposure duration in seconds (i.e. shutter open time).  This
	// may differ from the exposure time requested due to shutter latency, camera timing
	// precision, etc.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (!m_bExposureTaken)
		return Error ( "No Exposure Taken", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = m_dLastDuration;

	return S_OK;
}

int  CCCDCamera::get_LastExposureStartTime(std::string& pVal)
{
	// 
	// LastExposureStartTime
	// ---------------------
	// 
	// Property
	//             CCDCamera.LastStartTime (read only)
	// Syntax
	//             CCDCamera.LastStartTime(std::string &)
	// Exceptions
	//             Must throw exception if not supported or no exposure
	//             has been taken
	// 
	// Remarks
	// 
	// Reports the actual exposure start in the FITS-standard
	// CCYY-MM-DDThh:mm:ss[.sss...] format.
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_bExposureTaken)
		return Error ( "No Exposure Taken", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOEXPOSURE) );

	char tcsBuf[32];
	tm * ptm;
	
	ptm = gmtime(&m_stStartExposure.tv_sec);

	snprintf(tcsBuf, 32, "%04d-%02d-%02dT%02d:%02d:%02d.%03d", 
				ptm->tm_year + 1900, 
				ptm->tm_mon + 1, 
				ptm->tm_mday, 
				ptm->tm_hour, 
				ptm->tm_min, 
				ptm->tm_sec, 
				(int)(m_stStartExposure.tv_usec / 1000));

	std::string bsVal(tcsBuf);
	pVal = bsVal;

	return S_OK;
}

int  CCCDCamera::get_MaxADU(long* pVal)
{
	// 
	// MaxADU
	// ------
	// 
	// Property
	//             CCDCamera.MaxADU (read only)
	// Syntax
	//             CCDCamera.MaxADU(long *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Reports the maximum ADU value the camera can produce.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = (long)m_QSIInterface.m_CCDSpecs.MaxADU;

	return S_OK;
}

int  CCCDCamera::get_MaxBinX(short* pVal)
{
	// 
	// MaxBinX
	// -------
	// 
	// Property
	//             CCDCamera.MaxBinX (read only)
	// Syntax
	//             CCDCamera.MaxBinX(short *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// If AsymmetricBinning = False, returns the maximum allowed binning factor. If
	// AsymmetricBinning = True, returns the maximum allowed binning factor for the X
	// axis.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (short)m_DeviceDetails.MaxHBinning;

	return S_OK;
}

int  CCCDCamera::get_MaxBinY(short* pVal)
{
	// 
	// MaxBinY
	// -------
	// 
	// Property
	//             CCDCamera.MaxBinY (read only)
	// Syntax
	//             CCDCamera.MaxBinY(short *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// If AsymmetricBinning = False, equals MaxBinX. If AsymmetricBinning = True,
	// returns the maximum allowed binning factor for the Y axis.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.AsymBin)
		*pVal = (short)m_DeviceDetails.MaxHBinning;
	else
		*pVal = (short)m_DeviceDetails.MaxVBinning;

	return S_OK;
}

int  CCCDCamera::get_NumX(long* pVal)
{
	// 
	// NumX
	// ----
	// 
	// Property
	//             CCDCamera.NumX
	// Syntax
	//             CCDCamera.NumX (long *)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame width. Also returns the current value.  If binning is active,
	// value is in binned pixels.  No error check is performed when the value is set.
	// Should default to CameraXSize.
	// 

	*pVal = (long)m_ExposureSettings.ColumnsToRead;

	return S_OK;
}

int  CCCDCamera::put_NumX(long newVal)
{
	// 
	// NumX
	// ----
	// 
	// Property
	//             CCDCamera.NumX
	// Syntax
	//             CCDCamera.NumX (long)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame width. Also returns the current value.  If binning is active,
	// value is in binned pixels.  No error check is performed when the value is set.
	// Should default to CameraXSize.
	// 

	m_ExposureSettings.ColumnsToRead = (int)newVal;

	return S_OK;
}

int  CCCDCamera::get_NumY(long* pVal)
{
	// 
	// NumY
	// ----
	// 
	// Property
	//             CCDCamera.NumY
	// Syntax
	//             CCDCamera.NumY (long *)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame height. Also returns the current value.  If binning is active,
	// value is in binned pixels.  No error check is performed when the value is set.
	// Should default to CameraYSize.
	// 

	*pVal = (long)m_ExposureSettings.RowsToRead;

	return S_OK;
}

int  CCCDCamera::put_NumY(long newVal)
{
	// 
	// NumY
	// ----
	// 
	// Property
	//             CCDCamera.NumY
	// Syntax
	//             CCDCamera.NumY (long)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame height. Also returns the current value.  If binning is active,
	// value is in binned pixels.  No error check is performed when the value is set.
	// Should default to CameraYSize.
	// 

	m_ExposureSettings.RowsToRead = (int)newVal;

	return S_OK;
}

int  CCCDCamera::get_PixelSizeX(double* pVal)
{
	// 
	// PixelSizeX
	// ----------
	// 
	// Property
	//             CCDCamera.PixelSizeX(read only)
	// Syntax
	//             CCDCamera.PixelSizeX(double)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the width of the CCD chip pixels in microns, as provided by the camera
	// driver.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (((double)m_DeviceDetails.XAspect) / 100.0);

	return S_OK;
}

int  CCCDCamera::get_PixelSizeY(double* pVal)
{
	// 
	// PixelSizeY
	// ----------
	// 
	// Property
	//             CCDCamera.PixelSizeY(read only)
	// Syntax
	//             CCDCamera.PixelSizeY(double *)
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the height of the CCD chip pixels in microns, as provided by the camera
	// driver.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = (((double)m_DeviceDetails.YAspect) / 100.0);

	return S_OK;
}

int  CCCDCamera::get_SetCCDTemperature(double* pVal)
{
	// 
	// SetCCDTemperature
	// -----------------
	// 
	// Property
	//             CCDCamera.SetCCDTemperature
	// Syntax
	//             CCDCamera.SetCCDTemperature (Double *)
	// Exceptions
	//             Must throw exception if command not successful.
	//             Must throw exception if CanSetCCDTemperature is False.
	// 
	// Remarks
	// 
	// Sets the camera cooler setpoint in degrees Celsius, and returns the current
	// setpoint.
	// 
	// Note:  camera hardware and/or driver should perform cooler ramping, to prevent
	// thermal shock and potential damage to the CCD array or cooler stack.
	// 
	double dCurSetPoint;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if ( m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetSetPoint( dCurSetPoint);
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Get Current CCD Temperature Set Point", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = dCurSetPoint;
	return S_OK;
}

int  CCCDCamera::put_SetCCDTemperature(double newVal)
{
	// 
	// SetCCDTemperature
	// -----------------
	// 
	// Property
	//             CCDCamera.SetCCDTemperature
	// Syntax
	//             CCDCamera.SetCCDTemperature (double)
	// Exceptions
	//             Must throw exception if command not successful.
	//             Must throw exception if CanSetCCDTemperature is False.
	// 
	// Remarks
	// 
	// Sets the camera cooler set-point in degrees Celsius, and returns the current
	// setpoint.
	// 
	// Note:  camera hardware and/or driver should perform cooler ramping, to prevent
	// thermal shock and potential damage to the CCD array or cooler stack.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	bool bCoolerOn;

		// Check for previous error
	if ( m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (newVal > 100.0 || newVal < -100.0)
		return Error ( "Temperature Out of Range", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_INVALIDTEMP) );

	m_iError = this->get_CoolerOn(&bCoolerOn);
	if ( m_iError ) 
		return Error ( "Cannot Get Current Cooler State", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	//On		  GoToAmbient  Action
	//------------------------------------------------------
	//False     False        Cooler off
	//False     True         Cooler off
	//True      False        Cooler on, set temperature
	//True      True         Cooler on, warm CCD to ambient

	//Warm to ambient is a controlled temperature increased to ambient
	//at some safe rate that the camera decides, as opposed to just turning off the cooler.

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetTemperature ( bCoolerOn, /*GoToAmbient*/ false, newVal );
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Change Cooler Temp", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int  CCCDCamera::get_StartX(long* pVal)
{
	// 
	// StartX
	// ------
	// 
	// Property
	//             CCDCamera.StartX
	// Syntax
	//             CCDCamera.StartX(long *)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the subframe start position for the X axis (0 based). Also returns the
	// current value.  If binning is active, value is in binned pixels.
	// 

	*pVal = (long)m_ExposureSettings.ColumnOffset;

	return S_OK;
}

int  CCCDCamera::put_StartX(long newVal)
{
	// 
	// StartX
	// ------
	// 
	// Property
	//             CCDCamera.StartX
	// Syntax
	//             CCDCamera.StartX(long)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame start position for the X axis (0 based). Also returns the
	// current value.  If binning is active, value is in binned pixels.
	// 

	m_ExposureSettings.ColumnOffset = (int)newVal;

	return S_OK;
}

int  CCCDCamera::get_StartY(long* pVal)
{
	// 
	// StartY
	// ------
	// 
	// Property
	//             CCDCamera.StartY
	// Syntax
	//             CCDCamera.StartY(long *)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame start position for the Y axis (0 based). Also returns the
	// current value.  If binning is active, value is in binned pixels.
	// 

	*pVal = (long)m_ExposureSettings.RowOffset;

	return S_OK;
}

int  CCCDCamera::put_StartY(long newVal)
{
	// 
	// StartY
	// ------
	// 
	// Property
	//             CCDCamera.StartY
	// Syntax
	//             CCDCamera.StartY (long)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Sets the sub-frame start position for the Y axis (0 based). Also returns the
	// current value.  If binning is active, value is in binned pixels.
	// 

	m_ExposureSettings.RowOffset = (int)newVal;

	return S_OK;
}

int  CCCDCamera::AbortExposure(void)
{
	// 
	// AbortExposure
	// -------------
	// 
	// Syntax
	//             CCDCamera.AbortExposure()
	// Parameters
	//             None.
	// Returns
	//             Nothing.
	// Exceptions
	//             Must throw exception if camera is not idle and abort is
	//               unsuccessful (or not possible, e.g. during down load).
	//             Must throw exception if hardware or communications error
	//               occurs.
	//             Must NOT throw an exception if the camera is already idle.
	// 
	// Remarks
	// 
	// Aborts the current exposure, if any, and returns the camera to Idle state.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous errors
	if( this->m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	m_DownloadPending = false;
	m_bImageValid = false;
	// Send command
	csQSI.Lock();
	this->m_iError = m_QSIInterface.CMD_AbortExposure();
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot Abort Exposure", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int  CCCDCamera::PulseGuide(GuideDirections Direction, long Duration)
{
	// 
	// PulseGuide
	// -----------
	// 
	// Syntax
	//             CCDCamera.PulseGuide(Direction, Duration)
	// Parameters
	//             GuideDirections Direction - direction in which the guide-rate
	//                                         motion is to be made
	//             Long Duration - Duration of the guide-rate motion (milliseconds)
	// Returns
	//             Nothing.
	// Exceptions
	//             Must throw exception if PulseGuide command is unsupported or
	//             the command is unsuccessful.
	// 
	// Remarks
	// 
	// This method may return immediately after the move has started, in which case
	// back-to-back dual axis pulse-guiding can be supported. Use the IsPulseGuiding
	// property to detect when all moves have completed.
	// 
	// Symbolic Constants
	// 
	// The (symbolic) values for GuideDirections are:
	// 
	// Constant     Value      Description
	// --------     -----      -----------
	// guideNorth     0        North (+ declination/elevation)
	// guideSouth     1        South (- declination/elevation)
	// guideEast      2        East (+ right ascension/azimuth)
	// guideWest      3        West (- right ascension/azimuth)
	// 
	// Note: directions are nominal and may depend on exact mount wiring.  guideNorth
	// must be opposite guideSouth, and guideEast must be opposite guideWest.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Relay units are 1/100ths of a second, or 10ms
	//
	// 500 series valid range is 1 to 5000 or -1 to -5000.
	//
	Duration /= 10;	// Adjust millisecond to 10ms camera increment
	int X, Y;
	bool bRelaysDone;

	X = 0;
	Y = 0;
	switch (Direction)
	{
		case guideNorth:
			Y = (int)Duration;
			break;
		case guideSouth:
			Y = -(int)Duration;
			break;
		case guideEast:
			X = (int)Duration;
			break;
		case guideWest:
			X = -(int)Duration;
			break;
		default:
			break;
	}

	// Check for previous error
	if ( m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Adjust for max increment
	if (X >  5000) X =  5000;
	if (X < -5000) X = -5000;
	if (Y >  5000) Y =  5000;
	if (Y < -5000) Y = -5000;

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_IsRelayDone( bRelaysDone );
	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Get Relay Status", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADRELAYSTATUS) );
	
	if (!bRelaysDone || ((X == 0) && (Y == 0)))
	{
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_AbortRelays();
		csQSI.Unlock();
		if ( m_iError ) 
			return Error ( "Cannot Abort Relays", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADABORTRELAYS) );
	}

	// Send command
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_ActivateRelay ( (short) X, (short) Y );
	csQSI.Unlock();

	if ( m_iError ) 
		return Error ( "Cannot Activate Relays", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_RELAYERROR) );

	return S_OK;
}

int  CCCDCamera::StartExposure(double Duration, bool Light)
{
	// 
	// StartExposure
	// -------------
	// 
	// Syntax
	//             CCDCamera.StartExposure ( Duration, Light )
	// Parameters
	//             Double Duration - Duration of exposure in seconds
	//             Boolean Light - True for light frame, False for dark frame
	//               (ignored if no shutter)
	// Returns
	//             Boolean - True if successful
	// Exceptions
	//             Must throw exception if NumX, NumY, XBin, YBin, StartX, StartY,
	//               or Duration parameters are invalid.
	//             Must throw exception if CanAsymmetricBin is False and
	//               BinX <> BinY
	//             Must throw exception if the exposure cannot be started for
	//               any reason, such as a hardware or communications error
	// 
	// Remarks
	// 
	// Starts an exposure. Use ImageReady to check when the exposure is complete.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if( this->m_iError ) 
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Make sure image to be exposed (including offsets and binning) isn't greater than the CCD size
	if ((m_ExposureSettings.ColumnOffset + m_ExposureSettings.ColumnsToRead) * m_ExposureSettings.BinFactorX > m_DeviceDetails.ArrayColumns) 
		return Error ( "Invalid Row Size", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADROWSIZE) );
	if ((m_ExposureSettings.RowOffset + m_ExposureSettings.RowsToRead) * m_ExposureSettings.BinFactorY > m_DeviceDetails.ArrayRows) 
		return Error ( "Invalid Column Size", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADCOLSIZE) );
  
	// Make sure binning is within boundaries specified by camera
	if (m_ExposureSettings.BinFactorX > m_DeviceDetails.MaxHBinning || m_ExposureSettings.BinFactorY > m_DeviceDetails.MaxVBinning) 
		return Error ( "Invalid Binning Mode", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_INVALIDBIN) );
	//
	// Some cameras allow for vertical asymmetric binning
	// In this case, BinFactorX always equals 1, and BinFactorY can be 1 to MaxVBinning
	// So we only need to check asymmetric binning in the case where BinFactorX != 1

	if (m_ExposureSettings.BinFactorX != m_ExposureSettings.BinFactorY)
	{
		if (!m_DeviceDetails.AsymBin)
		{
			if (m_ExposureSettings.BinFactorX != 1 || m_ExposureSettings.BinFactorY != 2)
			{
				return Error ( "Asymmetric Binning Not Allowed", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOASYMBIN) );
			}
		}
	}

	if (((Duration < m_QSIInterface.m_CCDSpecs.minExp) || (Duration > m_QSIInterface.m_CCDSpecs.maxExp)) && (Duration != 0.0))	// minExp and maxExp are doubles in units of seconds
		return Error ( "Invalid Exposure Duration", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADEXPOSURE) );

	// m_ExposureSettings.Duration = (UINT)(Duration * 100);	// exposure time is in units of 10 milliseconds.

	// Save image dimensions as per spec
	// ColumnsToRead and RowsToRead could be changes by the user
	// during the exposure
	m_ExposureNumX = m_ExposureSettings.ColumnsToRead;
	m_ExposureNumY = m_ExposureSettings.RowsToRead;

	m_ExposureSettings.OpenShutter      = Light;

	// ASCOM requires light parameter to be ignored if no shutter present
	if (!m_DeviceDetails.HasShutter)
		m_ExposureSettings.OpenShutter = true;

	// TODO expose properties for:
	//m_ExposureSettings.FastReadout      = FastReadout;
	//m_ExposureSettings.HoldShutterOpen  = HoldShutterOpen;

	m_ExposureSettings.UseExtTrigger    = false;
	m_ExposureSettings.StrobeShutterOutput = false;
	m_ExposureSettings.ExpRepeatCount   = 0;
	m_ExposureSettings.ProbeForImplemented = false;

	m_dLastDuration = Duration;

	// Send command
	if (m_DeviceDetails.HasCMD_StartExposureEx)
	{
		double fExposure;
		double fFract;
		double fIntPart;

		fExposure = Duration;			// Float exposure time in second increments
		fExposure = fExposure * 100;			// Float in 10ms increments
		fFract = modf(fExposure, &fIntPart);

		m_ExposureSettings.Duration         = static_cast<UINT>(fIntPart + 0.5);
		m_ExposureSettings.DurationUSec     = static_cast<UINT>((fFract * 100.0) + 0.5);
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_StartExposureEx( m_ExposureSettings );
		csQSI.Unlock();
	}
	else
	{
		m_ExposureSettings.Duration         = (UINT)((Duration * 100) + 0.5);
		m_ExposureSettings.DurationUSec     = 0;
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_StartExposure( m_ExposureSettings );
		csQSI.Unlock();
	}

	if( this->m_iError ) 
		return Error ( "Cannot Start Exposure", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Record start time
	gettimeofday(&m_stStartExposure, NULL);

	m_DownloadPending = true;
	m_bExposureTaken = true;
	m_bImageValid = false;

	return S_OK;
}

int  CCCDCamera::StopExposure(void)
{
	// 
	// StopExposure
	// -------------
	// 
	// Syntax
	//             CCDCamera.StopExposure()
	// Parameters
	//             None.
	// Returns
	//             Nothing.
	// Exceptions
	//             Must throw an exception if CanStopExposure is False
	//             Must throw an exception if no exposure is in progress
	//             Must throw an exception if the camera or link has an error
	//               condition
	//             Must throw an exception if for any reason no image readout
	//               will be available.
	// 
	// Remarks
	// 
	// Stops the current exposure, if any.  If an exposure is in progress, the readout
	// process is initiated.  Ignored if readout is already in process.
	// 

	return Error ( "Not Supported", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
}

	//Advanced settings
int CCCDCamera::get_SoundEnabled(bool& pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	pVal = m_AdvSettings.SoundOn;

	return S_OK;
}

int CCCDCamera::put_SoundEnabled(bool newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.SoundOn)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.SoundOn = newVal;

	Registry.SetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_LEDEnabled(bool& pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	pVal = m_AdvSettings.LEDIndicatorOn;

	return S_OK;
}

int CCCDCamera::put_LEDEnabled(bool newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.LEDIndicatorOn)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.LEDIndicatorOn = newVal;

	Registry.SetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_ReadoutSpeed(ReadoutSpeed& pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	pVal = m_AdvSettings.OptimizeReadoutSpeed?FastReadout:HighImageQuality;

	return S_OK;
}

int CCCDCamera::put_ReadoutSpeed(ReadoutSpeed newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.Optimizations)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.OptimizeReadoutSpeed = newVal;

	Registry.SetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );	
	return S_OK;
}

int CCCDCamera::get_FanMode(FanMode& pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	pVal = (FanMode)m_AdvSettings.FanModeIndex;

	return S_OK;
}

int CCCDCamera::put_FanMode(FanMode newVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.FanMode)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;
	std::string strSerialNumber(m_USBSerialNumber);
	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.FanModeIndex = newVal;

	Registry.SetAdvancedSetupSettings( strSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_FlushCycles(FlushCycles& pVal)
{
	return Error ( "No longer support.  Use PreExposureFlush", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
}

int CCCDCamera::put_FlushCycles(FlushCycles newVal)
{
	return Error ( "No longer support.  Use PreExposureFlush", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
}

int CCCDCamera::get_ShutterMode(ShutterMode& pVal)
{
	return Error ( "No longer support.  Use ShutterPriority", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
}

int CCCDCamera::put_ShutterMode(ShutterMode newVal)
{
	return Error ( "No longer support.  Use ShutterPriority", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
}

int (CCCDCamera::get_FilterCount)(int& count)
{
	// 
	// Names
	// -----
	// 
	// Property
	//             FilterWheel.FilterCount
	// Syntax
	//             FilterWheel.FilterCount(int &)
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// Returns the number of filter positions available
	// 
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No Filter Wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	count = m_DeviceDetails.NumFilters;

	return S_OK;
}

int  CCCDCamera::get_Names( std::string pVal[])
{
	// 
	// Names
	// -----
	// 
	// Property
	//             FilterWheel.Names
	// Syntax
	//             FilterWheel.Names(std::string[])
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the name given to the
	// filter position.  These names would usually be set up by the user via the
	// SetupDialog.  The number of slots N can be determined from the length of
	// the array.  If filter names are not available, then it should report back
	// "Filter 1", "Filter 2", etc.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No Filter Wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	int iNumFilters = m_AdvSettings.fwWheel.Filters.size();

	for (int i = 0; i < iNumFilters; i++)
	{
		pVal[i] = m_AdvSettings.fwWheel.Filters[i].Name;
	}

	return S_OK;
}
int  CCCDCamera::put_Names( std::string newVal[])
{
	// 
	// Names
	// -----
	// 
	// Property
	//             FilterWheel.Names
	// Syntax
	//             FilterWheel.Names(std::string[])
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the name given to the
	// filter position.  These names would usually be set up by the user via the
	// SetupDialog.  The number of slots N can be determined from the length of
	// the array.  If filter names are not available, then it should report back
	// "Filter 1", "Filter 2", etc.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No Filter Wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	int iNumFilters = (int)m_AdvSettings.fwWheel.Filters.size() <= (int)m_DeviceDetails.NumFilters?m_AdvSettings.fwWheel.Filters.size():m_DeviceDetails.NumFilters;

	for (int i = 0; i <  iNumFilters; i++)
	{
		m_AdvSettings.fwWheel.Filters[i].Name = newVal[i];
	}

	m_AdvSettings.fwWheel.SaveToRegistry(m_USBSerialNumber);
	
	return S_OK;
}
int  CCCDCamera::get_Position( short* pVal )
{
	// 
	// Position
	// ---------
	// 
	// Property
	//             FilterWheel.Position
	// Syntax
	//             FilterWheel.Position(short *)
	// Exceptions
	//             Must throw exception if filter number is invalid
	//             Must throw exception if link error occurs
	// 
	// Remarks
	// 
	// Write number between 0 and N-1, where N is the number of filter slots (see
	// Filter.Names). Starts filter wheel rotation immediately when written*. Reading
	// the property gives current slot number (if wheel stationary) or -1 if wheel is
	// moving. This is mandatory; valid slot numbers shall not be reported back while
	// the filter wheel is rotating past filter positions.
	// 
	// Note that some filter wheels are built into the camera (one driver, two
	// interfaces).  Some cameras may not actually rotate the wheel until the
	// exposure is triggered.  In this case, the written value is available
	// immediately as the read value, and -1 is never produced.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	int iCameraState = 0;
	bool bShutterOpen = true;
	bool bFilterState = false;

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No Filter Wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );


	csQSI.Lock();
	this->m_iError = m_QSIInterface.CMD_GetDeviceState ( iCameraState, bShutterOpen, bFilterState );
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Filter Wheel Get Status Failed", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
	
	if (bFilterState)
		*pVal = (short)-1;
	else
	{
		int iPos;
		// Send command
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_GetFilterPosition ( iPos );
		csQSI.Unlock();

		if ( m_iError ) 
			return Error ( "Cannot Get Filter Position", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		*pVal = (short)iPos;
	}

	return S_OK;	
}
int  CCCDCamera::put_Position( short newVal)
{
	// 
	// Position
	// ---------
	// 
	// Property
	//             FilterWheel.Position
	// Syntax
	//             FilterWheel.Position (Short)
	// Exceptions
	//             Must throw exception if filter number is invalid
	//             Must throw exception if link error occurs
	// 
	// Remarks
	// 
	// Write number between 0 and N-1, where N is the number of filter slots (see
	// Filter.Names). Starts filter wheel rotation immediately when written*. Reading
	// the property gives current slot number (if wheel stationary) or -1 if wheel is
	// moving. This is mandatory; valid slot numbers shall not be reported back while
	// the filter wheel is rotating past filter positions.
	// 
	// Note that some filter wheels are built into the camera (one driver, two
	// interfaces).  Some cameras may not actually rotate the wheel until the
	// exposure is triggered.  In this case, the written value is available
	// immediately as the read value, and -1 is never produced.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
	if (newVal < 0 || newVal > (m_DeviceDetails.NumFilters - 1 ))
		return Error ( "Invalid Filter Number", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_INVALIDFILTERNUMBER) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetFilterWheel( newVal );
 	csQSI.Unlock();
	if ( m_iError ) 
		return Error ( "Cannot Set Filter", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	m_CurFilterPos = (int)newVal;
	
	return S_OK;
}
int  CCCDCamera::get_FocusOffset( long pVal[] )
{
	// 
	// FocusOffsets
	// ------------
	// 
	// Property
	//             FilterWheel.FocusOffsets
	// Syntax
	//             FilterWheel.FocusOffsets(long[])
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the focus offset for
	// the given filter position.  These values are focuser- and filter
	// -dependent, and  would usually be set up by the user via the SetupDialog.
	// The number of slots N can be determined from the length of the array.
	// If focuser offsets are not available, then it should report back 0 for all
	// array values.
	// 
	//

	if (!m_bIsConnected)
		return Error ( "Not connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No filter wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	int iNumFilters = m_AdvSettings.fwWheel.Filters.size();

	for (int i = 0; i < iNumFilters; i++)
	{
		pVal[i] = (long)m_AdvSettings.fwWheel.Filters[i].Offset;
	}

	return S_OK;	
}
int  CCCDCamera::put_FocusOffset( long newVal[])
{
	// 
	// FocusOffsets
	// ------------
	// 
	// Property
	//             FilterWheel.FocusOffsets
	// Syntax
	//             FilterWheel.FocusOffsets(long[])
	// Exceptions
	//             Must throw exception if filter number is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the focus offset for
	// the given filter position.  These values are focuser- and filter
	// -dependent, and  would usually be set up by the user via the SetupDialog.
	// The number of slots N can be determined from the length of the array.
	// If focuser offsets are not available, then it should report back 0 for all
	// array values.
	// 
	//

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( "No Filter Wheel", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	int iNumFilters = (int)m_AdvSettings.fwWheel.Filters.size() <= (int)m_DeviceDetails.NumFilters?m_AdvSettings.fwWheel.Filters.size():m_DeviceDetails.NumFilters;

	for (int i = 0; i < iNumFilters; i++)
	{
		m_AdvSettings.fwWheel.Filters[i].Offset = newVal[i];
	}

	m_AdvSettings.fwWheel.SaveToRegistry(m_USBSerialNumber);

	return S_OK;
}
int  CCCDCamera::get_FilterWheelConnected(bool * pVal)
{
	// 
	// Connected
	// ---------
	// 
	// Property
	//             FilterWheel.Connected
	// Syntax
	//             FilterWheel.Connected (bool)
	// Exceptions
	//             Must throw exception if connection attempt fails
	// 
	// Remarks
	// 
	// Controls the link between the driver and the filter wheel. Set True to enable
	// the link. Set False to disable the link. You can also read the property to check
	// 
	// whether it is connected.
	// 
	return GetFilterConnected(pVal);
}
int  CCCDCamera::put_FilterWheelConnected(bool newVal)
{
	// 
	// Connected
	// ---------
	// 
	// Property
	//             FilterWheel.Connected
	// Syntax
	//             FilterWheel.Connected (bool)
	// Exceptions
	//             Must throw exception if connection attempt fails
	// 
	// Remarks
	// 
	// Controls the link between the driver and the filter wheel. Set True to enable
	// the link. Set False to disable the link. You can also read the property to check
	// 
	// whether it is connected.
	// 

	return PutFilterConnected(newVal);
}

int CCCDCamera::put_Connected(bool bCon)
{
	// 
	// Connected
	// ---------
	// 
	// Property
	//             CCDCamera.Connected
	// Syntax
	//             CCDCamera.Connected (bool)
	// Exceptions
	//             Must throw exception if unsuccessful.
	// 
	// Remarks
	// 
	// Controls the link between the driver and the camera. Set True to enable the
	// link. Set False to disable the link (this does not switch off the cooler).
	// You can also read the property to check whether it is connected.
	//
	m_bIsConnected = false;
	if (bCon)
	{
		QSI_Registry Registry;

		// If we are re-initializing, make sure old storage deleted
		CloseCamera();

		// Make sure we don't get that pesky first connect failure; see function definition for details
		csQSI.Lock();
		m_QSIInterface.Initialize();
		csQSI.Unlock();

		// Determine what role we're in (either main or guider camera)
		m_bIsMainCamera = GetCameraRole();

		//////////////////////////////////////////////////////////////////////////////////////////////
		// Routine for default selection of camera if only one QSI camera is connected
		// Modified to account for other NON-QSI FTDI devices
		// Always use the available camera if there is only one camera and
		// the user has not specified a specific camera
		//
		if ( m_USBSerialNumber == _T("") )
		{		
			std::string csFirstQSISerial;
			CameraID cID;
			
			int iCount;

			csQSI.Lock();	
			iCount = m_QSIInterface.CountDevices();
			csQSI.Unlock();
				
			if (iCount == 0)
				return Error ( "Cannot open camera connection", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

			csQSI.Lock();
			m_iError = m_QSIInterface.GetDeviceInfo(0, cID);
			csQSI.Unlock();
			if( m_iError ) 
				return Error ( "Cannot open camera connection, no device description", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

			csFirstQSISerial = cID.SerialNumber;	

			// Handle case where the registry has never been initialized by the user
			if (iCount > 0 && (Registry.GetSelectedCamera( m_bIsMainCamera ) == _T("")))
			{
				Registry.SetSelectedCamera(csFirstQSISerial, m_bIsMainCamera);
			}

			// If only one camera, always open it.
			if( iCount == 1 )
			{
				m_USBSerialNumber = csFirstQSISerial;
			}
			else if (iCount > 1)
			{
				// Get serial number of selected camera from registry
				m_USBSerialNumber = Registry.GetSelectedCamera( m_bIsMainCamera );
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////

		csQSI.Lock();
		m_iError = m_QSIInterface.OpenCamera( m_USBSerialNumber );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot open camera connection", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		// Get device details from camera
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_GetDeviceDetails( m_DeviceDetails );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot get device details", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		m_ExposureSettings.ColumnsToRead = m_DeviceDetails.ArrayColumns;
		m_ExposureSettings.RowsToRead = m_DeviceDetails.ArrayRows;
		m_ExposureSettings.BinFactorX = 1;
		m_ExposureSettings.BinFactorY = 1;

		// Get default advanced setup settings
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_GetAdvDefaultSettings( m_AdvDefaultSettings, m_DeviceDetails );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot get advanced default settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
		m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

		// Send advanced settings to camera
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
		
		// we need to call this to get the AdvEnabledOptions;
		// Bug here: Camera always reports fan speed index as 2.
		// So throw away what the camera says.
		QSI_AdvSettings temp;
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_GetCamDefaultAdvDetails( temp, m_AdvEnabledOptions, m_DeviceDetails );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot get advanced settings from camera", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		// Allocate image buffer used during download
		m_pusBuffer = new USHORT [ m_DeviceDetails.ArrayColumns * m_DeviceDetails.ArrayRows ];
		if( !m_pusBuffer ) 
			return Error ( "Out of memory", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOMEMORY) );
		
		m_bIsConnected = true;

		// Get Hardware and Firmware version
		m_iError = m_QSIInterface.GetVersionInfo(m_HWVersion, m_FWVersion);
	}
	else
	{
		m_bIsConnected = false;
		CloseCamera();
	}
	return S_OK;
}

int CCCDCamera::PutFilterConnected(bool bCon)
{
	//Connect the camera to connect the filter wheel
	if (m_DeviceDetails.HasFilter)
		return put_Connected(bCon);
	else
		return Error ( "No filter wheel available", IID_IFilterWheel, MAKE_HRESULT(1,FACILITY_ITF,QSI_NOFILTER ));
}

int CCCDCamera::GetFilterConnected(bool * pVal)
{
	*pVal = false;
	if (m_DeviceDetails.HasFilter)
	{
		// Check camera status
		// It must be connected for filter to be connected
		if (m_bIsConnected)
			*pVal = true;
		else
			*pVal = false;
	}
	else
	{
		// No filter wheel on camera
		*pVal = false;
	}

	return S_OK;
}

void CCCDCamera::CloseCamera ( void )
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// CloseCamera shuts down the link to the camera and deallocates buffer memory
	// Send command
	csQSI.Lock();
	m_QSIInterface.CloseCamera();
	csQSI.Unlock();
  
	// Delete image buffer and reset pointer to null 
	if (m_pusBuffer != NULL)
		delete [] m_pusBuffer;
	m_pusBuffer = NULL;

	return;
}

int CCCDCamera::FillImageBuffer(bool bMakeRequest)
{
	// This is the common code for reading an image from the camera
	// and filling the image buffer
	// The interface methods call this and then transfer the data
	// from the USHORT buffer and convert it into the appropriate
	// format

	int iStride;
	int iRowsRead;
	int iPixelSize = sizeof(USHORT); // Always 16 bit pixels for now
	int	iTotRowsRead;

	if (!m_bIsConnected  || m_pusBuffer == NULL)
		return Error ( "Not connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DownloadPending)
		return S_OK;

	// Surround entire operation with a semaphore so the read data isn;t interrupted by a status request
	csQSI.Lock();
	m_DownloadPending = false;
	// Calculate the amount of pixels to read per block
	if (m_ExposureNumX<=0 || m_ExposureNumY <= 0)
	{
		csQSI.Unlock();
		return Error ( "Image transfer error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_INVALIDIMAGEPARAMETER) );
	}

	if (bMakeRequest)
	{
		// Send transfer image command to camera
		m_iError = m_QSIInterface.CMD_TransferImage();

		if( m_iError )
		{
			csQSI.Unlock();
			return Error ( "Image transfer error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
		}
	}

	// Some callers require each row word aligned, so this may be some number of bytes more that number of columns
	// MaxIm is always row align, so stride is just the size of the row in bytes.
	iStride = m_ExposureSettings.ColumnsToRead * iPixelSize;
	iTotRowsRead = 0;

	while (iTotRowsRead < m_ExposureSettings.RowsToRead)
	{
		// ReadImageByRow may return fewer rows than requested.  It is up to the caller to make additional calls to retreive the entire image.
		m_iError = m_QSIInterface.ReadImageByRow( (BYTE *)m_pusBuffer + (iTotRowsRead * iStride), (m_ExposureSettings.RowsToRead - iTotRowsRead),
													m_ExposureSettings.ColumnsToRead, iStride, iPixelSize, iRowsRead);
		if (m_iError != ALL_OK)
		{
			csQSI.Unlock();
			return Error ( "Image transfer error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
		}
		iTotRowsRead += iRowsRead;  // Update the number of pixels read, ReadImage may return less row that we requested.
	}
	//
	// Image is now in m_pusBuffer
	//
	csQSI.Unlock();
	
	m_iError = GetAutoZeroData( bMakeRequest ); // true == issue autozero request to camera
	if( m_iError != ALL_OK ) 
		return Error ( "Auto zero get data error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Now apply the Hot Pixel map
	m_QSIInterface.HotPixelRemap((BYTE *)m_pusBuffer, 0, m_ExposureSettings, m_DeviceDetails, m_AutoZeroData.zeroLevel);
	m_bImageValid = true;
	return S_OK;
}

int CCCDCamera::GetAutoZeroData(bool bMakeRequest)
{
	int iPixelSize = sizeof(USHORT);
	int iRowsRead;

	csQSI.Lock();
	if (bMakeRequest) // Send AZ request to camera.  Other callers just expect the data to be ready to read (ie FocusImage).
	{
		m_iError = m_QSIInterface.CMD_GetAutoZero( m_AutoZeroData );
		if( m_iError != ALL_OK )
		{
			csQSI.Unlock();
			return m_iError;
		}
	}

	if (m_AutoZeroData.zeroEnable && m_AutoZeroData.pixelCount > 0 && m_AutoZeroData.pixelCount <= 8192)
	{
		m_iError = m_QSIInterface.ReadImageByRow( (BYTE *)m_usOverScanPixels, 1, m_AutoZeroData.pixelCount,
													m_AutoZeroData.pixelCount * 2, iPixelSize, iRowsRead);
		m_QSIInterface.LogWrite(2, _T("AutoZero adjust pixels started."));

		if( m_iError == ALL_OK )
			m_QSIInterface.GetAutoZeroAdjustment(m_AutoZeroData, m_usOverScanPixels, &m_usLastOverscanMean, &m_iOverscanAdjustment, &m_dOverscanAdjustment);

		if (m_iError == ALL_OK)
			m_QSIInterface.LogWrite(2, "AutoZero analyze over-scan completed OK.");
		else
			m_QSIInterface.LogWrite(2, "AutoZero analyze over-scan failed. Error Code: %x", m_iError);
	}
	csQSI.Unlock();
	return S_OK;
}

int  CCCDCamera::get_PowerOfTwoBinning(bool* pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_DeviceDetails.TwoTimesBinning;
	return S_OK;
}

int  CCCDCamera::get_DriverInfo(std::string& pVal)
{
#define XQUOTEME(x) QUOTEME(x)
#define QUOTEME(x) #x
#ifdef PACKAGE_VERSION
	std::stringstream s;
	s << XQUOTEME(PACKAGE_VERSION);

#ifdef USELIBFTDIZERO
	s << " using libftdi";
#elif defined(USELIBFTD2XX)
	s << " using libftd2xx";
#else
	s << " unknown";
#endif
	std::string sVer = s.str();
#else
	std::string sVer( "0.0.0.0" );
#endif

	pVal = sVer;

	return S_OK;
}

int  CCCDCamera::get_Name(std::string& pVal)
{
	std::string bsName = m_DeviceDetails.ModelName;
	pVal = bsName;

	return S_OK;
}

int  CCCDCamera::get_HasFilterWheel(bool* pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_DeviceDetails.HasFilter;

	return S_OK;
}

int  CCCDCamera::get_SerialNumber(std::string& pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	std::string bsSN = m_DeviceDetails.SerialNumber;
	pVal = bsSN;

	return S_OK;
}

int  CCCDCamera::get_ModelNumber(std::string& pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	std::string bsMN = m_DeviceDetails.ModelNumber;
	pVal = bsMN;

	return S_OK;
}

int  CCCDCamera::get_IsMainCamera(bool* pVal)
{
	*pVal = (bool) m_bIsMainCamera;

	return S_OK;
}

int  CCCDCamera::put_IsMainCamera(bool newVal)
{
	if (m_bIsConnected)
		return Error ( "Already connected - Disconnect to change roles.", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_CONNECTED) );
	
	m_bIsMainCamera = newVal;

	return S_OK;
}

bool CCCDCamera::GetCameraRole( void )
{
	return m_bIsMainCamera;
}

int CCCDCamera::put_SelectCamera(std::string serialNum)
{
	if (m_bIsConnected)
		return Error ( "Already connected - Disconnect to change cameras.", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_CONNECTED) );

	m_USBSerialNumber = serialNum;
	QSI_Registry Registry;
	Registry.SetSelectedCamera( serialNum, m_bIsMainCamera );
	return S_OK;
}

int CCCDCamera::get_SelectCamera(std::string& serialNum)
{
	// QSI_Registry Registry;
	// Get serial number of selected camera from registry
	// serialNum = Registry.GetSelectedCamera( m_bIsMainCamera );
	serialNum = m_USBSerialNumber;

	return S_OK;
}

int CCCDCamera::get_AvailableCameras(std::string cameraSerial[], std::string cameraDesc[], int& numFound)
{
	std::vector<CameraID> vID;

	csQSI.Lock();
	m_iError = m_QSIInterface.ListDevices( vID, numFound );
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot list cameras", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	for (int i = 0; i < numFound; i++)
	{
		cameraSerial[i] = vID[i].SerialNumber;
		cameraDesc[i] = vID[i].Description;
	}

	for (int i = numFound; i < MAXCAMERAS; i++)
	{
		cameraSerial[i] = std::string("");
		cameraDesc[i] = std::string("");
	}

	return S_OK;
}

int CCCDCamera::put_UseStructuredExceptions(bool newVal)
{
	m_bStructuredExceptions = newVal;
	return S_OK;
}

	
int CCCDCamera::put_EnableShutterStatusOutput(bool newVal)
{
	unsigned char ucMode;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetAltMode1(ucMode);
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot get AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (newVal)
		ucMode |= EXPOSUREPULSEOUTBIT;
	else
		ucMode &= ~EXPOSUREPULSEOUTBIT;

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetAltMode1(ucMode);
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot set AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_EnableShutterStatusOutput(bool* pVal)
{
	unsigned char ucMode;
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );


	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetAltMode1(ucMode);
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot get AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (ucMode & EXPOSUREPULSEOUTBIT)
		*pVal = true;
	else
		*pVal = false;

	return S_OK;
}

int CCCDCamera::get_ManualShutterMode(bool * pVal)
{
	unsigned char ucMode;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetAltMode1(ucMode);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot get AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (ucMode & MANUALSHUTTERMODE)
		*pVal = true;
	else
		*pVal = false;

	return S_OK;
}

int CCCDCamera::put_ManualShutterMode(bool newVal)
{
	unsigned char ucMode = 0;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	bool hasShutter;
	this->get_HasShutter(&hasShutter);
	if (!hasShutter)
		return Error ( "No Shutter Installed", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetAltMode1(ucMode);
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot get AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (newVal)
		ucMode |= MANUALSHUTTERMODE;
	else
		ucMode &= ~MANUALSHUTTERMODE;

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetAltMode1(ucMode);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot set AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;

}

int CCCDCamera::put_ManualShutterOpen(bool newVal)
{
	unsigned char ucMode = 0;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	bool hasShutter;
	this->get_HasShutter(&hasShutter);
	if (!hasShutter)
		return Error ( "No Shutter Installed", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (newVal)
		ucMode = MANUALSHUTTEROPEN;
	else
		ucMode = MANUALSHUTTERCLOSE;

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetAltMode1(ucMode);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot set AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

// 520 extensions

int CCCDCamera::get_CameraGain(CameraGain * pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	*pVal = (CameraGain)m_AdvSettings.CameraGainIndex;

	return S_OK;
}

int CCCDCamera::put_CameraGain(CameraGain newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.CameraGain)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.CameraGainIndex = newVal;

	Registry.SetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_AntiBlooming(AntiBloom * pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	*pVal = (AntiBloom)m_AdvSettings.AntiBloomingIndex;

	return S_OK;
}
int CCCDCamera::put_AntiBlooming(AntiBloom newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.AntiBlooming)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.AntiBloomingIndex = newVal;

	Registry.SetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}
int CCCDCamera::get_ShutterPriority(ShutterPriority * pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	// KAF firmware reports the wrong value so, override the camera:

	if (m_DeviceDetails.ModelBaseType == _T("503") ||
	m_DeviceDetails.ModelBaseType == _T("504") ||
	m_DeviceDetails.ModelBaseType == _T("516") ||
	m_DeviceDetails.ModelBaseType == _T("532") ||
	m_DeviceDetails.ModelBaseType == _T("583")	)	
	{
		*pVal = ShutterPriorityMechanical;
	}
	else
	{
		*pVal = (ShutterPriority)m_AdvSettings.ShutterPriorityIndex;
	}

	return S_OK;
}
int CCCDCamera::put_ShutterPriority(ShutterPriority newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.ShutterPriority)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.ShutterPriorityIndex = newVal;

	Registry.SetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}
int CCCDCamera::get_PreExposureFlush(PreExposureFlush * pVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	*pVal = (PreExposureFlush)m_AdvSettings.PreExposureFlushIndex;

	return S_OK;
}
int CCCDCamera::put_PreExposureFlush(PreExposureFlush newVal)
{

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_AdvEnabledOptions.PreExposureFlush)
		return Error ( "Option not available on this model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	QSI_Registry Registry;

	// Attempt to get the advanced settings from registry and go to camera default on any setting that can't be retrieved
	m_AdvSettings = Registry.GetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvDefaultSettings );

	m_AdvSettings.PreExposureFlushIndex = newVal;

	Registry.SetAdvancedSetupSettings( m_USBSerialNumber, m_bIsMainCamera, m_AdvSettings );

	// Send advanced settings to camera
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SendAdvSettings( m_AdvSettings );
	csQSI.Unlock();
	if( m_iError ) 
		return Error ( "Cannot set advanced settings", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}


int CCCDCamera::put_HostTimedExposure(bool newVal)
{
	unsigned char ucMode = 0;

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (m_DeviceDetails.ModelBaseType == "520")
		return Error ( "Feature not available on the currect camera model", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_GetAltMode1(ucMode);
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( "Cannot get AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	if (newVal)
		ucMode |= HOSTTIMEDEXPOSURE;
	else
		ucMode &= ~HOSTTIMEDEXPOSURE;

	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_SetAltMode1(ucMode);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot set AltMode1", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;

}

int CCCDCamera::get_LastOverscanMean( unsigned short * pVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_usLastOverscanMean;
	return S_OK;
}

int CCCDCamera::get_MinExposureTime( double * pVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_QSIInterface.m_CCDSpecs.minExp;
	return S_OK;
}

int CCCDCamera::get_MaxExposureTime( double * pVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_QSIInterface.m_CCDSpecs.maxExp;
	return S_OK;
}

//

int CCCDCamera::get_QSIDeviceCount(SHORT * pVal)
{
	//read only
	//Returns the number of camera available for connection (with or without serial numbers assigned), 0 if no camera connected

	csQSI.Lock();
	*pVal = (SHORT)m_QSIInterface.CountDevices( );
	csQSI.Unlock();

	return S_OK;
}
	
int CCCDCamera::get_QSISelectedDevice(std::string& pVal)
{
	//read/write
	//Sets or returns the current selected camera serial number

	pVal = m_USBSerialNumber;

	return S_OK;
}

int CCCDCamera::put_QSISelectedDevice(std::string newVal)
{
	//read/write
	//Sets or returns the current selected camera serial number
	//Must be disconnected to change slected device.

	if (m_bIsConnected)
		return Error ( "Camera already connected.  Set Connected false before changing selected camera.", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_CONNECTED) );

	//QSI_Registry Registry;
	//Registry.SetSelectedCamera( CString(newVal), m_bIsMainCamera  );
	m_USBSerialNumber = newVal;

	return S_OK;
}
		   
int CCCDCamera::get_QSISerialNumbers(std::string pVal[], int * iNumFound)
{
	//read only
	//Returns a string array of available device USB serial numbers

	std::vector<std::string> vSerial;
	std::vector<CameraID> vID;

	csQSI.Lock();
	m_iError = m_QSIInterface.ListDevices( vID,  *iNumFound );
	csQSI.Unlock();

	if (m_iError == 200002) // No cameras connected
	{
		*iNumFound = 0;
	} 
	else if( m_iError )
	{
		return Error ( "Cannot get device list", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
	}
	else
	{
		for (int i = 0; i < *iNumFound; i++)
		{
			pVal[i] = vID[i].SerialNumber;
		}
	}

	for (int i = *iNumFound; i < MAXCAMERAS; i++)
	{
		pVal[i] = std::string("");
	}

	return S_OK;
}

int CCCDCamera::QSIRead( unsigned char * Buffer, int BytesToRead, int * BytesReturned)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError = m_QSIInterface.QSIRead( Buffer, BytesToRead, BytesReturned);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Read Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::QSIWrite( unsigned char * Buffer, int BytesToWrite, int * BytesWritten)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError = m_QSIInterface.QSIWrite( Buffer, BytesToWrite, BytesWritten);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Write Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_QSIReadDataAvailable( int * pVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError =  m_QSIInterface.QSIReadDataAvailable( pVal);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot get read data available", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::get_QSIWriteDataPending( int * pVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError =  m_QSIInterface.QSIWriteDataPending(pVal);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot get write data pending", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::put_QSIReadTimeout( int newVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();	
	m_iError =  m_QSIInterface.QSIReadTimeout(newVal);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot set read timeout", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::put_QSIWriteTimeout( int newVal )
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	csQSI.Lock();
	m_iError =  m_QSIInterface.QSIWriteTimeout(newVal);
	csQSI.Unlock();

	if( this->m_iError ) 
		return Error ( "Cannot set write timeout", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	return S_OK;
}

int CCCDCamera::put_QSIOpen(bool bCon)
{
	// 
	// Connected
	// ---------
	// 
	// Property
	//             CCDCamera.QSIOpen
	// Syntax
	//             CCDCamera.QSIOPen (bool)
	// Exceptions
	//             Must throw exception if unsuccessful.
	// 
	// Remarks
	// 
	// Low Level open for QSI commands.  Does not communicate with camera ala put-Connected.
	//
	m_bIsConnected = false;
	if (bCon)
	{
		QSI_Registry Registry;

		// If we are re-initializing, make sure old storage deleted
		CloseCamera();

		// Make sure we don't get that pesky first connect failure; see function definition for details
		csQSI.Lock();
		m_QSIInterface.Initialize();
		csQSI.Unlock();

		// Determine what role we're in (either main or guider camera)
		m_bIsMainCamera = GetCameraRole();

		//////////////////////////////////////////////////////////////////////////////////////////////
		// Routine for default selection of camera if only one QSI camera is connected
		// Modified to account for other NON-QSI FTDI devices
		// Always use the available camera if there is only one camera and
		// the user has not specified a specific camera
		//
		if ( m_USBSerialNumber == _T("") )
		{		
			std::string csFirstQSISerial;
			CameraID cID;
			
			int iCount;

			csQSI.Lock();		
			iCount = m_QSIInterface.CountDevices();
			csQSI.Unlock();
				
			if (iCount == 0)
				return Error ( "Cannot open camera connection", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

			csQSI.Lock();
			m_iError = m_QSIInterface.GetDeviceInfo(0, cID);
			csQSI.Unlock();
			if( m_iError ) 
				return Error ( "Cannot open camera connection, no device description", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

			csFirstQSISerial = cID.SerialNumber;	

			// Handle case where the registry has never been initialized by the user
			if (iCount > 0 && (Registry.GetSelectedCamera( m_bIsMainCamera ) == _T("")))
			{
				Registry.SetSelectedCamera(csFirstQSISerial, m_bIsMainCamera);
			}

			// If only one camera, always open it.
			if( iCount == 1 )
			{
				m_USBSerialNumber = csFirstQSISerial;
			}
			else if (iCount > 1)
			{
				// Get serial number of selected camera from registry
				m_USBSerialNumber = Registry.GetSelectedCamera( m_bIsMainCamera );
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////

		csQSI.Lock();
		m_iError = m_QSIInterface.OpenCamera( m_USBSerialNumber );
		csQSI.Unlock();
		if( m_iError ) 
			return Error ( "Cannot open camera connection", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

		m_bIsConnected = true;
	}
	else
	{
		m_bIsConnected = false;
		CloseCamera();
	}
	return S_OK;
}

int  CCCDCamera::get_CanSetGain(bool* pVal)
{
	// 
	// CanSetGain
	// ----------------
	// 
	// Syntax
	//             CCDCamera.CanSetGain (read only)
	// Syntax
	//             CCDCamera.CanSetGain(bool *)
	// Exceptions
	//             None
	// 
	// Remarks
	// 
	// Returns True if the camera can set gain; False if not.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	// Check for previous error
	if ( m_iError )
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// KAF firmware reports the wrong value so, override the camera:
	if (m_DeviceDetails.ModelBaseType == "503" ||
		m_DeviceDetails.ModelBaseType == "504" ||
		m_DeviceDetails.ModelBaseType == "516" ||
		m_DeviceDetails.ModelBaseType == "532" 
		)
		*pVal = false;
	else
		*pVal = true;

	return S_OK;
}

int CCCDCamera::put_MaskPixels(bool newVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	m_QSIInterface.m_hpmMap.m_bEnable = newVal;
	m_QSIInterface.m_hpmMap.Save();
	return S_OK;
}

int CCCDCamera::get_MaskPixels(bool* pVal)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	*pVal = m_QSIInterface.m_hpmMap.m_bEnable;
	return S_OK;
}

int CCCDCamera::put_PixelMask(std::vector<Pixel> pixels)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	m_QSIInterface.m_hpmMap.SetPixels(pixels);
	m_QSIInterface.m_hpmMap.Save();
	return S_OK;
}

int CCCDCamera::get_PixelMask(std::vector<Pixel> *pixels)
{
	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	*pixels = m_QSIInterface.m_hpmMap.GetPixels();
	return S_OK;
}

int CCCDCamera::get_FilterPositionTrim( std::vector<short> * pVal )
{
	// 
	// FilterPositionTrim
	// ------------
	// 
	// Property
	//             FilterWheel.FilterPositionTrim
	// Syntax
	//             FilterWheel.FocusFilterPositionTrim(std::vector<short> *)
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the FilterPositionTrim for
	// the given filter position.  
	// 
	//

	if (!m_bIsConnected)
		return Error ( _T("Not connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error ( _T("No filter wheel"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	int iNumFilters = m_AdvSettings.fwWheel.Filters.size();

	pVal->clear();

	for (int i = 0; i <  iNumFilters; i++)
	{
		short iTrim = m_AdvSettings.fwWheel.Filters[i].Trim;
		pVal->push_back(iTrim);
	}

	return S_OK;	
}

int CCCDCamera::put_FilterPositionTrim( std::vector<short> sa)
{
	// 
	// FilterPositionTrim
	// ------------
	// 
	// Property
	//             FilterWheel.FilterPositionTrim
	// Syntax
	//             FilterWheel.FilterPositionTrim(std::vector<short>)
	// Exceptions
	//             Must throw exception if filter number (i) is invalid
	// 
	// Remarks
	// 
	// For each valid slot number (from 0 to N-1), reports the FilterPositionTrim for
	// the given filter position. 
	// 
	//

	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasFilter || m_DeviceDetails.NumFilters < 1)
		return Error  (_T("No Filter Wheel"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOFILTER) );

	short iTrim;

	int iNumFilters = m_AdvSettings.fwWheel.Filters.size() <= sa.size()?m_AdvSettings.fwWheel.Filters.size():sa.size();

	for (int i = 0; i <  iNumFilters; i++)
	{
		iTrim = (short)sa[i];
		m_AdvSettings.fwWheel.Filters[i].Trim = iTrim;
	}

	m_AdvSettings.fwWheel.SaveToRegistry(m_USBSerialNumber);
	return S_OK;
}


int CCCDCamera::get_HasFilterWheelTrim(bool * pVal)
{
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_DeviceDetails.HasFilterTrim;

	return S_OK;
}

int CCCDCamera::get_FilterWheelNames( std::vector<std::string> * pVal)
{
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	std::vector<FilterWheel> Wheels;

	FilterWheel::GetWheels( m_USBSerialNumber, &Wheels, m_DeviceDetails.NumFilters);

	pVal->clear();

	int iNumWheels = Wheels.size();
	for (int i = 0; i < iNumWheels; i++)
	{
		pVal->push_back(Wheels[i].Name);
	}

	return S_OK;
}

int CCCDCamera::get_SelectedFilterWheel(std::string * pVal)
{
	//read/write
	//Sets or returns the current selected filter wheel
	if (!m_bIsConnected )
	return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	*pVal = m_AdvSettings.fwWheel.Name;

	return S_OK;
}

int CCCDCamera::put_SelectedFilterWheel(std::string newVal)
{
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry reg;
	FilterWheel Wheel(m_DeviceDetails.NumFilters);
	std::string strWheelName = newVal;

	Wheel.LoadFromRegistry(m_USBSerialNumber, strWheelName, m_DeviceDetails.NumFilters);
	m_AdvSettings.fwWheel = Wheel;
	reg.SetSelectedFilterWheel( m_USBSerialNumber, m_bIsMainCamera, strWheelName);

	return S_OK;
}

int CCCDCamera::NewFilterWheel(std::string newName)
{
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry reg;
	FilterWheel Wheel(m_DeviceDetails.NumFilters);
	std::string strWheelName = newName;
	Wheel.Name = std::string(strWheelName);
	Wheel.SaveToRegistry(m_USBSerialNumber);
	reg.SetSelectedFilterWheel( m_USBSerialNumber, m_bIsMainCamera, strWheelName);

	return S_OK;
}

int CCCDCamera::DeleteFilterWheel(std::string delName)
{
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	QSI_Registry reg;
	FilterWheel Wheel;
	std::string strWheelName = delName;

	Wheel.LoadFromRegistry(m_USBSerialNumber, strWheelName, m_DeviceDetails.NumFilters);
	Wheel.DeleteFromRegistry(m_USBSerialNumber);

	std::string SetName = m_AdvSettings.fwWheel.Name;
	if (SetName == strWheelName)
	{
		// Just deleted the set wheel
		FilterWheel newWheel(m_DeviceDetails.NumFilters);
		newWheel.Name =  std::string("Default");
		m_AdvSettings.fwWheel = newWheel;
	}

	return S_OK;
}

int  CCCDCamera::get_PCBTemperature(double* pVal)
{
	// 
	// CCDTemperature
	// --------------
	// 
	// Property
	//             CCDCamera.CCDTemperature  (read only)
	// Syntax
	//             CCDCamera.CCDTemperature
	// Exceptions
	//             Must throw exception if data unavailable.
	// 
	// Remarks
	// 
	// Returns the current CCD temperature in degrees Celsius. Only valid if
	// CanControlTemperature is True.
	// 

	if (!m_bIsConnected)
		return Error ( "Not Connected", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	//CoolerState &State,        // Current temperature controller status
	int iState = 0;
	double Temp = 0;             // Temperature of CCD chip (FLT_MAX if not available)
	double TempAmbient = 0;      // Ambient temperature if available (FLT_MAX if not)
	double PCBTemp = 0;
	unsigned short Power = 0;    // Power level in percent (0-100), return > 100 if not available

	if ( m_iError )
	{
		// Once the error is caught, we must clear it to allow another attempt at this command
		int err = m_iError;
		m_iError = 0;
		return Error ( "Camera Error", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, err) );
	}

	if (m_DeviceDetails.HasCMD_GetTemperatureEx)
	{
		// Send command
		csQSI.Lock();
		m_iError = m_QSIInterface.CMD_GetTemperatureEx ( iState, Temp, TempAmbient, Power, PCBTemp, false );
		csQSI.Unlock();
	}
	else
	{
		m_iError = QSI_NOTSUPPORTED;
	}

	if ( m_iError ) 
		return Error ( "Cannot Get PCB Temperature", IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	*pVal = PCBTemp;

	return S_OK;
}
int CCCDCamera::HSRImage(double Duration, USHORT * pImage)
{
	// 
	// HSRImage
	// -------------
	// 
	// Syntax
	//             CCDCamera.HSRImage ( Duration, Image )
	// Parameters
	//             Double Duration - Duration of exposure in seconds
	//             USHORT Image - Returned image from camera
	// Returns
	//             Boolean - True if successful
	// Exceptions
	//             Must throw exception if NumX, NumY, XBin, YBin, StartX, StartY,
	//               or Duration parameters are invalid.
	//             Must throw exception if CanAsymmetricBin is False and
	//               BinX <> BinY
	//             Must throw exception if the exposure cannot be started for
	//               any reason, such as a hardware or communications error
	// 
	// Remarks
	// High Speed Readout Image
	// Starts an exposure and complete it with an image. Must complete in under 5 seconds.
	// Does exposure and transfer in one api call.
	// 
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	// Check for previous error
	if( this->m_iError ) 
		return Error ( _T("Camera Error"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );
	if (!m_DeviceDetails.HasCMD_HSRExposure)
		return Error ( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
	// Make sure image to be exposed (including offsets and binning) isn't greater than the CCD size
	if ((m_ExposureSettings.ColumnOffset + m_ExposureSettings.ColumnsToRead) * m_ExposureSettings.BinFactorX > m_DeviceDetails.ArrayColumns) 
		return Error ( _T("Invalid Column Size"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADROWSIZE) );
	if ((m_ExposureSettings.RowOffset + m_ExposureSettings.RowsToRead) * m_ExposureSettings.BinFactorY > m_DeviceDetails.ArrayRows) 
		return Error ( _T("Invalid Row Size"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADCOLSIZE) );
	// Make sure binning is within boundaries specified by camera
	if (m_ExposureSettings.BinFactorX > m_DeviceDetails.MaxHBinning || m_ExposureSettings.BinFactorY > m_DeviceDetails.MaxVBinning) 
		return Error ( _T("Invalid Binning Mode"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_INVALIDBIN) );
	if (!m_DeviceDetails.AsymBin && (m_ExposureSettings.BinFactorX != m_ExposureSettings.BinFactorY))
		return Error ( _T("Asymetric Binning Not Allowed"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOASYMBIN) );
	if ((Duration < m_QSIInterface.m_CCDSpecs.minExp || Duration > m_QSIInterface.m_CCDSpecs.maxExp) && Duration != 0.0)
		return Error ( _T("Invalid Exposure Duration"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_BADEXPOSURE) );

	// Save image dimensions as per spec
	// ColumnsToRead and RowsToRead could be changed by the user
	// during the exposure
	m_ExposureNumX = m_ExposureSettings.ColumnsToRead;
	m_ExposureNumY = m_ExposureSettings.RowsToRead;
	m_ExposureSettings.OpenShutter      = true;

	// ASCOM requires light parameter to be ignored if no shutter present
	if (!m_DeviceDetails.HasShutter)
		m_ExposureSettings.OpenShutter = true;

	// TODO expose properties for:
	//m_ExposureSettings.FastReadout      = FastReadout;
	//m_ExposureSettings.HoldShutterOpen  = HoldShutterOpen;
	m_ExposureSettings.UseExtTrigger    = false;	//TODO
	m_ExposureSettings.StrobeShutterOutput = false; //TODO
	m_ExposureSettings.ExpRepeatCount   = 0;		//TODO
	m_ExposureSettings.ProbeForImplemented = false;
	m_dLastDuration = Duration;

	// Send command
	double fExposure;
	double fFract;
	double fIntPart;

	fExposure = Duration;					// Float exposure time in second increments
	fExposure = fExposure * 100;			// Float in 10ms increments
	fFract = modf(fExposure, &fIntPart);

	m_ExposureSettings.Duration         = static_cast<UINT>(fIntPart + 0.5);
	m_ExposureSettings.DurationUSec     = static_cast<BYTE>((fFract * 100.0) + 0.5);
	csQSI.Lock();
	m_iError = m_QSIInterface.CMD_HSRExposure( m_ExposureSettings, m_AutoZeroData );
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( _T("Cannot Start HSR Exposure"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	// Record start time
	gettimeofday(&m_stStartExposure, NULL);
	m_DownloadPending = true;
	m_bExposureTaken = true;
	m_bImageValid = false;
	///////////////////////////////////////////////////////////////////
	// Wait for Image Data, it will just start when camera is ready
	// This will also read the autozero pixels after the image
	///////////////////////////////////////////////////////////////////
	FillImageBuffer(false); // False indicates to need to issue CMD to transfer data/autozero
	if ( !m_bImageValid) 
		return Error ( _T("No Image Available"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOIMAGEAVAILABLE) );

	USHORT* pSrc = m_pusBuffer;
	// Adjust zero also copies the data and does any appropriate casting of pixel type.
	m_iError = m_QSIInterface.AdjustZero(pSrc, pImage, m_ExposureSettings.ColumnsToRead, m_ExposureSettings.RowsToRead,  m_iOverscanAdjustment, m_AutoZeroData.zeroEnable);

	return S_OK;
}

int CCCDCamera::put_HSRMode(bool newVal)
{
	if (!m_bIsConnected )
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasCMD_HSRExposure)
		return Error ( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	csQSI.Lock();
	m_QSIInterface.CMD_SetHSRMode( newVal );
	csQSI.Unlock();
	
	return S_OK;
}

int CCCDCamera::Flush(void)
{
	// 
	// Flush
	// -------------
	// 
	// Syntax
	//             CCDCamera.Flush ( void )
	// Parameters
	//             None
	// Returns
	//             Boolean - True if successful
	// Exceptions
	//           
	// 
	// Remarks
	// 
	// Flush the CCD using the Flush Settings and a minimal exposure that does not return data.
	// 
	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
/*
	QSI_ExposureSettings ExposureSettings;
	ExposureSettings.BinFactorX				= 1;
	ExposureSettings.BinFactorY				= 1;
	ExposureSettings.ColumnOffset			= 0;
	ExposureSettings.ColumnsToRead			= 1;
	ExposureSettings.RowOffset				= 0;
	ExposureSettings.RowsToRead				= 1;
	ExposureSettings.Duration				= 0;		// Zero exposure == Use fastest exposure length
	ExposureSettings.DurationUSec			= 0;		// Ignored on KAF cameras
	ExposureSettings.ExpRepeatCount			= 0;		// Ignored by firmware
	ExposureSettings.OpenShutter			= false;
	ExposureSettings.FastReadout			= true;		// Ignored by firmware
	ExposureSettings.HoldShutterOpen		= false;
	ExposureSettings.UseExtTrigger			= false;
	ExposureSettings.StrobeShutterOutput	= false;
	ExposureSettings.ProbeForImplemented	= false;
*/
	csQSI.Lock();
	if (m_DeviceDetails.HasCMD_StartExposureEx)
		m_iError = m_QSIInterface.CMD_StartExposureEx( m_ExposureSettings );
	else
		m_iError = m_QSIInterface.CMD_StartExposure( m_ExposureSettings );
	csQSI.Unlock();
	if( this->m_iError ) 
		return Error ( _T("Error Flushing Camera"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, m_iError) );

	CameraState CameraState;
	do	
	{
		get_CameraState(&CameraState);
	} 
	while (CameraState != CameraIdle && CameraState != CameraError);

	return S_OK;
}

int CCCDCamera::EnableTriggerMode( TriggerModeEnum newVal1, TriggerPolarityEnum newVal2)
{
	// 
	// EnableTriggerMode
	// -------------
	// 
	// Syntax
	//             CCDCamera::EnableTriggerMode(enum TriggerModeEnum newVal1, enum TriggerPolarityEnum newVal2)
	// Parameters
	//             None
	// Returns
	//             0 is successesful, non-zero on error;
	// Exceptions
	//           
	// 
	// Remarks
	// 
	// Enable the external shutter input trigger mode
	// 

	if (!m_bIsConnected)
		return Error( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );

	if (!m_DeviceDetails.HasCMD_BasicHWTrigger)
		return Error( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	csQSI.Lock();
	int iResult = m_QSIInterface.CMD_ExtTrigMode((BYTE)newVal1, (BYTE)newVal2);
	csQSI.Unlock();
	if (iResult == ERR_IFC_NotSupported)
		return Error ( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );
	if (iResult != 0)
		return Error( _T("Enable Trigger Mode failed."), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_UNRECOVERABLE) );

	return S_OK;
}

int CCCDCamera::TerminatePendingTrigger(void)
{
	// 
	// TerminatePendingTrigger
	// -------------
	// 
	// Syntax
	//             CCCDCamera::TerminatePendingTrigger(void)
	// Parameters
	//             None
	// Returns
	//             0 is successesful, non-zero on error;
	// Exceptions
	//           
	// 
	// Remarks
	// 
	// Terminate the wait for a pending external trigger (startexposure is in progress)
	// 

	if (!m_bIsConnected)
		return Error( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	if (!m_DeviceDetails.HasCMD_BasicHWTrigger)
		return Error( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	csQSI.Lock();
	int iResult = m_QSIInterface.CMD_ExtTrigMode(2,0);
	csQSI.Unlock();

	if (iResult != 0)
		return Error( _T("Terminate Pending Trigger failed."), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_UNRECOVERABLE) );

	return S_OK;
}

int CCCDCamera::CancelTriggerMode(void)
{
	// 
	// CancelTriggerMode
	// -------------
	// 
	// Syntax
	//             CCCDCamera::CancelTriggerMode(void)
	// Parameters
	//             None
	// Returns
	//             0 is successesful, non-zero on error;
	// Exceptions
	//           
	// 
	// Remarks
	// 
	// Cancel the wait for external trigger mode
	// 

	if (!m_bIsConnected)
		return Error( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	if (!m_DeviceDetails.HasCMD_BasicHWTrigger)
		return Error( _T("Not Supported On This Model"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTSUPPORTED) );

	csQSI.Lock();
	int iResult = m_QSIInterface.CMD_ExtTrigMode(0,0);
	csQSI.Unlock();

	if (iResult != 0)
		return Error( _T("Cancel External Trigger Mode failed."), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_UNRECOVERABLE) );

	return S_OK;
}

int CCCDCamera::get_ShutterState(ShutterStateEnum * pVal)
{
	int iState = 0;

	if (!m_bIsConnected)
		return Error ( _T("Not Connected"), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_NOTCONNECTED) );
	csQSI.Lock();
	int iResult = m_QSIInterface.CMD_GetShutterState( iState );
	csQSI.Unlock();

	if (iResult != 0)
		return Error ( _T("Get Shutter State failed."), IID_ICamera, MAKE_HRESULT(1,FACILITY_ITF, QSI_UNRECOVERABLE) );

	*pVal = (ShutterStateEnum)(iState & 0x07);

	return S_OK;
}

