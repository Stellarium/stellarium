/*****************************************************************************************
NAME
qsiapi.cpp

DESCRIPTION
 api shim

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2007

REVISION HISTORY
 DRC 03.11.07 Original Version
 *****************************************************************************************/

#include "qsiapi.h"
#include "CCDCamera.h"

QSICamera::QSICamera()
{
	pCam = new CCCDCamera();
}

QSICamera::~QSICamera()
{
	delete (CCCDCamera *)pCam;
}

// ICCDCamera
int QSICamera::get_BinX(short* pVal)
{
	return ((CCCDCamera *)pCam)->get_BinX( pVal);
}

int QSICamera::put_BinX(short newVal)
{
	return ((CCCDCamera *)pCam)->put_BinX( newVal);
}

int QSICamera::get_BinY(short* pVal)
{
	return ((CCCDCamera *)pCam)->get_BinY( pVal);
}

int QSICamera::put_BinY(short newVal)
{
	return ((CCCDCamera *)pCam)->put_BinY( newVal);
}

int QSICamera::get_CameraState(CameraState* pVal)
{
	return ((CCCDCamera *)pCam)->get_CameraState( (CCCDCamera::CameraState *) pVal);
}

int QSICamera::get_CameraXSize(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_CameraXSize(pVal);
}

int QSICamera::get_CameraYSize(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_CameraYSize(pVal);
}

int QSICamera::get_CanAbortExposure(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanAbortExposure(pVal);
}

int QSICamera::get_CanGetCoolerPower(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanGetCoolerPower(pVal);
}

int QSICamera::get_CanAsymmetricBin(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanAsymmetricBin(pVal);
}

int QSICamera::get_CanPulseGuide(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanPulseGuide(pVal);
}

int QSICamera::get_CanSetCCDTemperature(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanSetCCDTemperature(pVal);
}

int QSICamera::get_CanStopExposure(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CanStopExposure(pVal);
}

int QSICamera::get_CCDTemperature(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_CCDTemperature(pVal);
}

int QSICamera::get_Connected(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_Connected(pVal);
}
	
int QSICamera::put_Connected(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_Connected(newVal);
}

int QSICamera::get_CoolerOn(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_CoolerOn(pVal);
}

int QSICamera::put_CoolerOn(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_CoolerOn(newVal);
}

int QSICamera::get_CoolerPower(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_CoolerPower(pVal);
}

int QSICamera::get_Description(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_Description(pVal);
}

int QSICamera::get_DriverInfo(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_DriverInfo(pVal);
}

int QSICamera::get_ElectronsPerADU(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_ElectronsPerADU(pVal);
}

int QSICamera::get_FullWellCapacity(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_FullWellCapacity(pVal);
}

int QSICamera::get_HasFilterWheel(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_HasFilterWheel(pVal);
}

int QSICamera::get_HasShutter(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_HasShutter(pVal);
}

int QSICamera::get_HeatSinkTemperature(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_HeatSinkTemperature(pVal);
}

int QSICamera::get_ImageArraySize(int& xSize, int& ySize, int& elementSize)
{
	return ((CCCDCamera *)pCam)->get_ImageArraySize(xSize, ySize, elementSize);
}

int QSICamera::get_ImageArray(unsigned short* pVal)
{
	return ((CCCDCamera *)pCam)->get_ImageArray(pVal);
}

int QSICamera::get_ImageArray(double * pVal)
{
	return ((CCCDCamera *)pCam)->get_ImageArray(pVal);
}

int QSICamera::get_ImageReady(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_ImageReady(pVal);
}

int QSICamera::put_IsMainCamera(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_IsMainCamera(newVal);
}

int QSICamera::get_IsMainCamera(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_IsMainCamera(pVal);
}

int QSICamera::get_IsPulseGuiding(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_IsPulseGuiding(pVal);
}

int QSICamera::get_LastError(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_LastError(pVal);
}

int QSICamera::get_LastExposureDuration(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_LastExposureDuration(pVal);
}

int QSICamera::get_LastExposureStartTime(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_LastExposureStartTime(pVal);
}

int QSICamera::get_MaxADU(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_MaxADU(pVal);
}

int QSICamera::get_MaxBinX(short* pVal)
{
	return ((CCCDCamera *)pCam)->get_MaxBinX(pVal);
}

int QSICamera::get_MaxBinY(short* pVal)
{
	return ((CCCDCamera *)pCam)->get_MaxBinY(pVal);
}

int QSICamera::get_ModelNumber(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_ModelNumber(pVal);
}

int QSICamera::get_Name(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_Name(pVal);
}

int QSICamera::get_NumX(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_NumX(pVal);
}

int QSICamera::put_NumX(long newVal)
{
	return ((CCCDCamera *)pCam)->put_NumX(newVal);
}

int QSICamera::get_NumY(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_NumY(pVal);
}

int QSICamera::put_NumY(long newVal)
{
	return ((CCCDCamera *)pCam)->put_NumY(newVal);
}

int QSICamera::get_PixelSizeX(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_PixelSizeX(pVal);
}

int QSICamera::get_PixelSizeY(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_PixelSizeY(pVal);
}

int QSICamera::get_PowerOfTwoBinning(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_PowerOfTwoBinning(pVal);
}

int QSICamera::get_SerialNumber(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_SerialNumber(pVal);
}

int QSICamera::get_SetCCDTemperature(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_SetCCDTemperature(pVal);
}

int QSICamera::put_SetCCDTemperature(double newVal)
{
	return ((CCCDCamera *)pCam)->put_SetCCDTemperature(newVal);
}

int QSICamera::get_StartX(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_StartX(pVal);
}

int QSICamera::put_StartX(long newVal)
{
	return ((CCCDCamera *)pCam)->put_StartX(newVal);
}

int QSICamera::get_StartY(long* pVal)
{
	return ((CCCDCamera *)pCam)->get_StartY(pVal);
}

int QSICamera::put_StartY(long newVal)
{
	return ((CCCDCamera *)pCam)->put_StartY(newVal);
}

int QSICamera::AbortExposure(void)
{
	return ((CCCDCamera *)pCam)->AbortExposure();
}

int QSICamera::PulseGuide(GuideDirections Direction, long Duration)
{
	return ((CCCDCamera *)pCam)->PulseGuide((CCCDCamera::GuideDirections) Direction, Duration);
}

int QSICamera::StartExposure(double Duration, bool Light)
{
	return ((CCCDCamera *)pCam)->StartExposure(Duration, Light);
}

int QSICamera::StopExposure(void)
{
	return ((CCCDCamera *)pCam)->StopExposure();
}


int QSICamera::put_SelectCamera(std::string serialNum)
{
	return ((CCCDCamera *)pCam)->put_SelectCamera(serialNum);
}

int QSICamera::get_SelectCamera(std::string& serialNum)
{
	return ((CCCDCamera *)pCam)->get_SelectCamera(serialNum);
}

int QSICamera::get_AvailableCameras(std::string cameraSerial[], std::string cameraDesc[], int& numFound)
{
	return ((CCCDCamera *)pCam)->get_AvailableCameras( cameraSerial, cameraDesc, numFound);
}


//Advanced settings
int QSICamera::get_SoundEnabled(bool& pVal)
{
	return ((CCCDCamera *)pCam)->get_SoundEnabled(pVal);
}

int QSICamera::put_SoundEnabled(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_SoundEnabled(newVal);
}

int QSICamera::get_LEDEnabled(bool& pVal)
{
	return ((CCCDCamera *)pCam)->get_LEDEnabled(pVal);
}

int QSICamera::put_LEDEnabled(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_LEDEnabled(newVal);
}

int QSICamera::get_FanMode(FanMode& pVal)
{
	return ((CCCDCamera *)pCam)->get_FanMode((CCCDCamera::FanMode&) pVal);
}

int QSICamera::put_FanMode(FanMode newVal)
{
	return ((CCCDCamera *)pCam)->put_FanMode((CCCDCamera::FanMode) newVal);
}

int QSICamera::get_FlushCycles(FlushCycles& pVal)
{
	return ((CCCDCamera *)pCam)->get_FlushCycles((CCCDCamera::FlushCycles&) pVal);
}

int QSICamera::put_FlushCycles(FlushCycles newVal)
{
	return ((CCCDCamera *)pCam)->put_FlushCycles((CCCDCamera::FlushCycles) newVal);
}

int QSICamera::get_ShutterMode(ShutterMode& pVal)
{
	return ((CCCDCamera *)pCam)->get_ShutterMode((CCCDCamera::ShutterMode&) pVal);
}

int QSICamera::put_ShutterMode(ShutterMode newVal)
{
	return ((CCCDCamera *)pCam)->put_ShutterMode((CCCDCamera::ShutterMode) newVal);
}

int QSICamera::put_UseStructuredExceptions(bool newVal)
{
	return ((CCCDCamera*)pCam)->put_UseStructuredExceptions(newVal);
}

int QSICamera::get_ReadoutSpeed(ReadoutSpeed& pVal)
{
	return ((CCCDCamera *)pCam)->get_ReadoutSpeed((CCCDCamera::ReadoutSpeed&)pVal);
}

int QSICamera::put_ReadoutSpeed(ReadoutSpeed newVal)
{
	return ((CCCDCamera *)pCam)->put_ReadoutSpeed((CCCDCamera::ReadoutSpeed)newVal);
}

// IFilterWheel Methods
int QSICamera::get_FilterCount(int& count)
{
	return ((CCCDCamera *)pCam)->get_FilterCount(count);
}

int QSICamera::get_Names( std::string names[])
{
	return ((CCCDCamera *)pCam)->get_Names(names);
}

int QSICamera::put_Names( std::string names[])
{
	return ((CCCDCamera *)pCam)->put_Names(names);
}

int QSICamera::get_Position( short* pVal )
{
	return ((CCCDCamera *)pCam)->get_Position(pVal);
}

int QSICamera::put_Position( short newVal )
{
	return ((CCCDCamera *)pCam)->put_Position(newVal);
}

int QSICamera::get_FocusOffset( long pVal[] )
{
	return ((CCCDCamera *)pCam)->get_FocusOffset(pVal);
}

int QSICamera::put_FocusOffset( long newVal[] )
{
	return ((CCCDCamera *)pCam)->put_FocusOffset(newVal);
}

int QSICamera::get_FilterWheelConnected(bool * pVal)
{
	return ((CCCDCamera *)pCam)->get_FilterWheelConnected(pVal);
}

int QSICamera::put_FilterWheelConnected(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_FilterWheelConnected(newVal);
}
	
int QSICamera::put_EnableShutterStatusOutput(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_EnableShutterStatusOutput(newVal);
}

int QSICamera::get_EnableShutterStatusOutput(bool* pVal)
{
	return ((CCCDCamera *)pCam)->get_EnableShutterStatusOutput(pVal);
}

int QSICamera::get_ManualShutterMode(bool * pVal)
{
	return ((CCCDCamera *)pCam)->get_ManualShutterMode(pVal);
}

int QSICamera::put_ManualShutterMode(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_ManualShutterMode(newVal);
}

int QSICamera::put_ManualShutterOpen(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_ManualShutterOpen(newVal);
}

// 520 features

int QSICamera::get_CameraGain(QSICamera::CameraGain * pVal)
{
	return ((CCCDCamera *)pCam)->get_CameraGain( (CCCDCamera::CameraGain *) pVal);
}

int QSICamera::put_CameraGain(QSICamera::CameraGain newVal)
{
	return ((CCCDCamera *)pCam)->put_CameraGain((CCCDCamera::CameraGain)newVal);
}

int QSICamera::get_AntiBlooming(QSICamera::AntiBloom * pVal)
{
	return ((CCCDCamera *)pCam)->get_AntiBlooming( (CCCDCamera::AntiBloom *) pVal);
}

int QSICamera::put_AntiBlooming(QSICamera::AntiBloom newVal)
{
	return ((CCCDCamera *)pCam)->put_AntiBlooming((CCCDCamera::AntiBloom) newVal);
}

int QSICamera::get_ShutterPriority(QSICamera::ShutterPriority * pVal)
{
	return ((CCCDCamera *)pCam)->get_ShutterPriority( (CCCDCamera::ShutterPriority *) pVal);
}

int QSICamera::put_ShutterPriority(QSICamera::ShutterPriority newVal)
{
	return ((CCCDCamera *)pCam)->put_ShutterPriority( (CCCDCamera::ShutterPriority) newVal);
}

int QSICamera::get_PreExposureFlush(QSICamera::PreExposureFlush * pVal)
{
	return ((CCCDCamera *)pCam)->get_PreExposureFlush( (CCCDCamera::PreExposureFlush *) pVal);
}

int QSICamera::put_PreExposureFlush(QSICamera::PreExposureFlush newVal)
{
	return ((CCCDCamera *)pCam)->put_PreExposureFlush( (CCCDCamera::PreExposureFlush) newVal);
}

int QSICamera::put_HostTimedExposure(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_HostTimedExposure(newVal);
}

int QSICamera::get_LastOverscanMean( unsigned short * pVal )
{
	return ((CCCDCamera *)pCam)->get_LastOverscanMean( pVal );
}

// Extensions

int QSICamera::get_MinExposureTime( double * pVal )
{
	return ((CCCDCamera *)pCam)->get_MinExposureTime( pVal );
}

int QSICamera::get_MaxExposureTime( double * pVal )
{
	return ((CCCDCamera *)pCam)->get_MaxExposureTime( pVal );
}

int QSICamera::get_QSIDeviceCount(short * pVal)
{
	return ((CCCDCamera *)pCam)->get_QSIDeviceCount( pVal );
}

int QSICamera::get_QSISelectedDevice(std::string& pVal)
{
	return ((CCCDCamera *)pCam)->get_QSISelectedDevice( pVal );
}

int QSICamera::put_QSISelectedDevice(std::string newVal)
{
	return ((CCCDCamera *)pCam)->put_QSISelectedDevice( newVal );
}

int QSICamera::get_QSISerialNumbers(std::string pVal[], int * iNumFound)
{
	return ((CCCDCamera *)pCam)->get_QSISerialNumbers( pVal, iNumFound );
}

int QSICamera::QSIRead( unsigned char * Buffer, int BytesToRead, int * BytesReturned)
{
	return ((CCCDCamera *)pCam)->QSIRead( Buffer, BytesToRead, BytesReturned);
}

int QSICamera::QSIWrite( unsigned char * Buffer, int BytesToWrite, int * BytesWritten)
{
	return ((CCCDCamera *)pCam)->QSIWrite(Buffer, BytesToWrite, BytesWritten);
}

int QSICamera::get_QSIReadDataAvailable(int * pVal )
{
	return ((CCCDCamera *)pCam)->get_QSIReadDataAvailable( pVal );
}

int QSICamera::get_QSIWriteDataPending( int * pVal )
{
	return ((CCCDCamera *)pCam)->get_QSIWriteDataPending( pVal );
}

int QSICamera::put_QSIReadTimeout( int newVal )
{
	return ((CCCDCamera *)pCam)->put_QSIReadTimeout( newVal );
}

int QSICamera::put_QSIWriteTimeout( int newVal )
{
	return ((CCCDCamera *)pCam)->put_QSIWriteTimeout( newVal );
}

int QSICamera::put_QSIOpen( bool newVal )
{
	return ((CCCDCamera *)pCam)->put_QSIOpen( newVal );
}

int QSICamera::get_CanSetGain(bool* pVal)
{
	return ((CCCDCamera*)pCam)->get_CanSetGain(pVal);
}

int QSICamera::put_MaskPixels(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_MaskPixels( newVal );
}

int QSICamera::get_MaskPixels(bool* pVal)
{
	return ((CCCDCamera*)pCam)->get_MaskPixels(pVal);
}

int QSICamera::put_PixelMask(std::vector<Pixel> pixels )
{
	return ((CCCDCamera *)pCam)->put_PixelMask( pixels );
}

int QSICamera::get_PixelMask(std::vector<Pixel> *pixels )
{
	return ((CCCDCamera *)pCam)->get_PixelMask( pixels );
}

int QSICamera::get_FilterPositionTrim( std::vector<short> * pVal)
{
	return ((CCCDCamera *)pCam)->get_FilterPositionTrim( pVal );
}

int QSICamera::put_FilterPositionTrim( std::vector<short> newVal)
{
	return ((CCCDCamera *)pCam)->put_FilterPositionTrim( newVal );
}

int QSICamera::get_HasFilterWheelTrim(bool * pVal)
{
	return ((CCCDCamera *)pCam)->get_HasFilterWheelTrim( pVal );
}

int QSICamera::get_FilterWheelNames( std::vector<std::string> * pVal)
{
	return ((CCCDCamera *)pCam)->get_FilterWheelNames( pVal );
}

int QSICamera::get_SelectedFilterWheel(std::string * pVal)
{
	return ((CCCDCamera *)pCam)->get_SelectedFilterWheel( pVal);
}

int QSICamera::put_SelectedFilterWheel(std::string newVal)
{
	return ((CCCDCamera *)pCam)->put_SelectedFilterWheel( newVal );
}

int QSICamera::NewFilterWheel(std::string newVal)
{
	return ((CCCDCamera *)pCam)->NewFilterWheel( newVal );
}

int QSICamera::DeleteFilterWheel(std::string newVal)
{
	return ((CCCDCamera *)pCam)->DeleteFilterWheel( newVal );
}

int QSICamera::get_PCBTemperature(double* pVal)
{
	return ((CCCDCamera *)pCam)->get_PCBTemperature(pVal);
}

int QSICamera::HSRImage(double Duration, unsigned short * Image)
{
	return ((CCCDCamera *)pCam)->HSRImage(Duration, Image);
}

int QSICamera::put_HSRMode(bool newVal)
{
	return ((CCCDCamera *)pCam)->put_HSRMode(newVal);
}

int QSICamera::Flush(void)
{
	return ((CCCDCamera *)pCam)->Flush();
}

int QSICamera::EnableTriggerMode(TriggerModeEnum newVal1, TriggerPolarityEnum newVal2)
{
	return ((CCCDCamera *)pCam)->EnableTriggerMode((CCCDCamera::TriggerModeEnum)newVal1, (CCCDCamera::TriggerPolarityEnum)newVal2);
}

int QSICamera::TerminatePendingTrigger(void)
{
	return ((CCCDCamera *)pCam)->TerminatePendingTrigger();
}

int QSICamera::CancelTriggerMode(void)
{
	return ((CCCDCamera *)pCam)->CancelTriggerMode();
}

int QSICamera::get_ShutterState( ShutterStateEnum * pVal)
{
	return ((CCCDCamera *)pCam)->get_ShutterState((CCCDCamera::ShutterStateEnum *)pVal);
}

