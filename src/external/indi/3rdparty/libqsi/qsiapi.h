/*****************************************************************************************
NAME
 qsiapi.h

DESCRIPTION
headerfile for qsi camera api

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2007

REVISION HISTORY
 DRC 12.19.06 Original Version
*****************************************************************************************/
#ifndef qsiapi_H
#define qsiapi_H

#include <string>
#include <stdexcept>
#include <vector>

class Pixel
{
public:
	Pixel(int x, int y){ this->x = x; this->y = y;}
	~Pixel(void) { }

	int x;
	int y;
};
//
// QSICamera
//
class QSICamera
{
public:

	//Enums
	enum CameraState
	{
		CameraIdle 		= 0,	//At idle state, available to start exposure
		CameraWaiting 	= 1,	//Exposure started but waiting (for shutter, trigger, filter wheel, etc.)
		CameraExposing 	= 2,	//Exposure currently in progress
		CameraReading	= 3,	//CCD array is being read out (digitized)
		CameraDownload 	= 4,	//Downloading data to PC
		CameraError 	= 5		//Camera error condition serious enough to prevent further operations (link fail, etc.).
	};
	
	enum GuideDirections
	{
		guideNorth 		= 0,
		guideSouth 		= 1,
		guideEast 		= 2,
		guideWest 		= 3
	};

	enum FanMode
	{
		fanOff			= 0,
		fanQuiet		= 1,
		fanFull			= 2
	};

	enum FlushCycles
	{
		FlushZero		= 0,
		FlushOne		= 1,
		FlushTwo		= 2,
		FlushFour		= 3,
		FlushEight		= 4
	};

	enum ShutterMode
	{
		ShutterMechanical = 0,
		ShutterElectronic = 1
	};

	enum PreExposureFlush
	{
		FlushNone = 0,		
		FlushModest = 1,
		FlushNormal = 2,	
		FlushAggressive = 3,
		FlushVeryAggressive = 4
	};

	enum ShutterPriority
	{
		ShutterPriorityMechanical = 0,
		ShutterPriorityElectronic = 1
	};

	enum CameraGain 
	{
		CameraGainHigh = 0,
		CameraGainLow = 1,
		CameraGainAuto = 2
	};

	enum AntiBloom 
	{
		AntiBloomNormal = 0,
		AntiBloomHigh = 1
	};
	
	enum ReadoutSpeed
	{
		HighImageQuality = 0,
		FastReadout = 1
	};

	enum TriggerModeEnum
	{
		ShortWait = 4,
		LongWait = 6
	};

	enum TriggerPolarityEnum
	{
		HighToLow = 0,
		LowToHigh = 1
	};

	enum ShutterStateEnum
	{
		RightSlitCW		= 0,
		LeftSlitCW		= 1,
		RightGateCW		= 2,
		LeftGateCW		= 3,
		RightSlitCCW	= 4,
		LeftSlitCCW		= 5,
		RightGateCCW	= 6,
		LeftGateCCW		= 7
	};

	static const int MAXCAMERAS = 128;

	QSICamera();
	~QSICamera();
	// to disallow copying, declare copy constructor and assignment
	// operators priavte
private:
	QSICamera(const QSICamera& cam);
	QSICamera& operator=(QSICamera that);
public:

	// Camera
	int get_BinX(short* pVal);
	int put_BinX(short newVal);
	int get_BinY(short* pVal);
	int put_BinY(short newVal);
	int get_CameraState(CameraState* pVal);
	int get_CameraXSize(long* pVal);
	int get_CameraYSize(long* pVal);
	int get_CanAbortExposure(bool* pVal);
	int get_CanAsymmetricBin(bool* pVal);
	int get_CanGetCoolerPower(bool* pVal);
	int get_CanPulseGuide(bool* pVal);
	int get_CanSetCCDTemperature(bool* pVal);
	int get_CanStopExposure(bool* pVal);
	int get_CCDTemperature(double* pVal);
	int get_Connected(bool* pVal);	
	int put_Connected(bool newVal);
	int get_CoolerOn(bool* pVal);
	int put_CoolerOn(bool newVal);
	int get_CoolerPower(double* pVal);
	int get_Description(std::string& pVal);
	int get_DriverInfo(std::string& pVal);
	int get_ElectronsPerADU(double* pVal);
	int get_FullWellCapacity(double* pVal);
	int get_HasFilterWheel(bool* pVal);
	int get_HasShutter(bool* pVal);
	int get_HeatSinkTemperature(double* pVal);
	int get_ImageArraySize(int& xSize, int& ySize, int& elementSize);
	int get_ImageArray(unsigned short* pVal);
	int get_ImageArray(double * pVal);
	int get_ImageReady(bool* pVal);
	int get_IsMainCamera(bool* pVal);
	int put_IsMainCamera(bool newVal);
	int get_IsPulseGuiding(bool* pVal);
	int get_LastError(std::string& pVal);
	int get_LastExposureDuration(double* pVal);
	int get_LastExposureStartTime(std::string& pVal);
	int get_MaxADU(long* pVal);
	int get_MaxBinX(short* pVal);
	int get_MaxBinY(short* pVal);
	int get_ModelNumber(std::string& pVal);
	int get_Name(std::string& pVal);
	int get_NumX(long* pVal);
	int put_NumX(long newVal);
	int get_NumY(long* pVal);
	int put_NumY(long newVal);
	int get_PixelSizeX(double* pVal);
	int get_PixelSizeY(double* pVal);
	int get_PowerOfTwoBinning(bool* pVal);
	int get_SerialNumber(std::string& pVal);
	int get_SetCCDTemperature(double* pVal);
	int put_SetCCDTemperature(double newVal);
	int get_StartX(long* pVal);
	int put_StartX(long newVal);
	int get_StartY(long* pVal);
	int put_StartY(long newVal);
	int AbortExposure(void);
	int PulseGuide(GuideDirections Direction, long Duration);
	int StartExposure(double Duration, bool Light);
	int StopExposure(void);

	int put_SelectCamera(std::string serialNum);
	int get_SelectCamera(std::string& serialNum);
	int get_AvailableCameras(std::string cameraSerial[], std::string cameraDesc[], int& numFound);


	int put_UseStructuredExceptions(bool newVal);

	//Advanced settings
	int get_SoundEnabled(bool& pVal);
	int put_SoundEnabled(bool newVal);
	int get_LEDEnabled(bool& pVal);
	int put_LEDEnabled(bool newVal);
	int get_FanMode(FanMode& pVal);
	int put_FanMode(FanMode newVal);
	int get_FlushCycles(FlushCycles& pVal);
	int put_FlushCycles(FlushCycles newVal);
	int get_ShutterMode(ShutterMode& pVal);
	int put_ShutterMode(ShutterMode newVal);
	int get_ReadoutSpeed(ReadoutSpeed& pVal);
	int put_ReadoutSpeed(ReadoutSpeed newVal);
	
	// FilterWheel Methods
	int get_FilterCount(int& count);
	int get_Names( std::string names[]);
	int put_Names( std::string names[]);
	int get_Position( short* pVal );
	int put_Position( short newVal );
	int get_FocusOffset( long pVal[] );
	int put_FocusOffset( long newVal[] );
	int get_FilterWheelConnected(bool * pVal);
	int put_FilterWheelConnected(bool newVal);

	// Custom Controls
	int put_EnableShutterStatusOutput(bool newVal);
	int get_EnableShutterStatusOutput(bool* pVal);	
	int get_ManualShutterMode(bool * pVal);
	int put_ManualShutterMode(bool newVal);
	int put_ManualShutterOpen(bool newVal);

	// 520 extensions
	int get_CameraGain(CameraGain * pVal);
	int put_CameraGain(CameraGain newVal);
	int get_AntiBlooming(AntiBloom * pVal);
	int put_AntiBlooming(AntiBloom newVal);
	int get_ShutterPriority(ShutterPriority * pVal);
	int put_ShutterPriority(ShutterPriority newVal);
	int get_PreExposureFlush(PreExposureFlush * pVal);
	int put_PreExposureFlush(PreExposureFlush newVal);
	int put_HostTimedExposure(bool newVal);
	//
	// Diagnostics
	//
	int get_LastOverscanMean( unsigned short * pVal );
	//
	// QSI Extensions
	//
	int get_MinExposureTime( double * pVal );
	int get_MaxExposureTime( double * pVal );
	int get_QSIDeviceCount(short * pVal);
	int get_QSISelectedDevice(std::string& pVal);
	int put_QSISelectedDevice(std::string newVal);
	int get_QSISerialNumbers(std::string pVal[], int * iNumFound);
	int QSIRead( unsigned char * Buffer, int BytesToRead, int * BytesReturned);
	int QSIWrite( unsigned char * Buffer, int BytesToWrite, int * BytesWritten);
	int get_QSIReadDataAvailable(int * pVal );
	int get_QSIWriteDataPending( int * pVal );
	int put_QSIReadTimeout( int newVal );
	int put_QSIWriteTimeout( int newVal );
	int put_QSIOpen( bool newVal );
	int get_CanSetGain(bool* pVal);
	int put_MaskPixels(bool newVal);
	int get_MaskPixels(bool* pVal);
	int put_PixelMask(std::vector<Pixel> pixels);
	int get_PixelMask(std::vector<Pixel> *pixels);
	int get_FilterPositionTrim( std::vector<short> * pVal);
	int put_FilterPositionTrim( std::vector<short>);
	int get_HasFilterWheelTrim(bool* pVal);
	int get_FilterWheelNames( std::vector<std::string> * pVal);
	int get_SelectedFilterWheel(std::string * pVal);
	int put_SelectedFilterWheel(std::string newVal);
	int NewFilterWheel(std::string Name);
	int DeleteFilterWheel(std::string Name);
	int get_PCBTemperature(double* pVal);
	int HSRImage(double Duration, unsigned short * Image);
	int put_HSRMode(bool newVal);
	int Flush(void);
	int EnableTriggerMode(TriggerModeEnum newVal1, TriggerPolarityEnum newVal2);
	int TerminatePendingTrigger(void);
	int CancelTriggerMode(void);
	int get_ShutterState( ShutterStateEnum * pVal);
	
private:
	//////////////////////////////////////////////////////////////////////////////////////
	// Private methods
	//

	//////////////////////////////////////////////////////////////////////////////////////
	// Private members
	//
	void * pCam;
};

#endif
