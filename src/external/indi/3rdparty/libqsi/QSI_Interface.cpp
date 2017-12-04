/*****************************************************************************************
NAME
 Interface

DESCRIPTION
 Camera Interface

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 MWB 04.28.06 Original Version
 *****************************************************************************************/

#include "QSI_Interface.h"
#include "IHostIO.h"
#include "QSIError.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Constructor
QSI_Interface::QSI_Interface ( void )
{
	m_iError = 0;
#ifdef LOG
	this->m_log = new QSILog("QSIINTERFACELOG.TXT", "LOGINTERFACETOFILE", "INT");
	this->m_log->TestForLogging();
#endif
	QSI_Registry reg;	
	
	// Check to see if we are re-mapping Bayer color for factor calibration
	m_bColorProfiling = false;
	m_bTestBayerImage = false;
	m_bCameraStateCacheInvalid = true;
	//
	m_bAutoZeroEnable = true;
	m_dwAutoZeroSatThreshold = AUTOZEROSATTHRESHOLD;
	m_dwAutoZeroMaxADU = AUTOZEROMAXADU;
	//
	m_dwAutoZeroSkipStartPixels = AUTOZEROSKIPSTARTPIXELS;
	m_dwAutoZeroSkipEndPixels = AUTOZEROSKIPENDPIXELS;
	m_bAutoZeroMedianNotMean = false;

	if (reg.GetNumber( "SOFTWARE/QSI", "COLORPROFILING", 0 ) > 0)
	{
		m_bColorProfiling = true;
		m_log->Write(2, "COLORPROFILING enabled");
	}

	if (reg.GetNumber( "SOFTWARE/QSI", "TESTBAYERIMAGE", 0 ) > 0)
	{
		m_bTestBayerImage = true;
		m_log->Write(2, "TESTBAYERIMAGE enabled");
	}
	//
	m_bAutoZeroEnable = reg.GetNumber( "SOFTWARE/QSI", "AUTOZEROENABLE", 1 ) == 1 ? true:false;
	if (m_bAutoZeroEnable == false)
	{
		m_log->Write(2, "AUTOZEROENABLE set to %d", m_bAutoZeroEnable);
	}

	m_dwAutoZeroSatThreshold = reg.GetNumber( "SOFTWARE/QSI", "AUTOZEROSATTHRESHOLD", AUTOZEROSATTHRESHOLD );
	if (m_dwAutoZeroSatThreshold != AUTOZEROSATTHRESHOLD)
	{
		m_log->Write(2, "AUTOZEROSATTHRESHOLD set to %d", m_dwAutoZeroSatThreshold);
	}

	m_dwAutoZeroMaxADU = reg.GetNumber( "SOFTWARE/QSI", "AUTOZEROMAXADU", AUTOZEROMAXADU );
	if (m_dwAutoZeroMaxADU != AUTOZEROMAXADU)
	{
		m_log->Write(2, "AUTOZEROMAXADU set to %d", m_dwAutoZeroMaxADU);
	}
	// default to mean, skip 32 start and end
	m_bAutoZeroMedianNotMean = reg.GetNumber( "SOFTWARE/QSI", "AUTOZEROMEDIANNOTMEAN", 0 ) == 1 ? true:false;
	if (m_bAutoZeroMedianNotMean == true)
	{
		m_log->Write(2, "AUTOZEROMEDIANNOTMEAN set to %d", m_bAutoZeroMedianNotMean);
	}

	m_dwAutoZeroSkipStartPixels = reg.GetNumber( "SOFTWARE/QSI", "AUTOZEROSKIPSTARTPIXELS", AUTOZEROSKIPSTARTPIXELS );
	if (m_dwAutoZeroSkipStartPixels != AUTOZEROSKIPSTARTPIXELS)
	{
		m_log->Write(2, "AUTOZEROSKIPSTARTPIXELS set to %d", m_dwAutoZeroSkipStartPixels);
	}

	m_dwAutoZeroSkipEndPixels = reg.GetNumber( "SOFTWARE/QSI", "AUTOSKIPENDPIXELS", AUTOZEROSKIPENDPIXELS );
	if (m_dwAutoZeroSkipEndPixels != AUTOZEROSKIPENDPIXELS)
	{
		m_log->Write(2, "AUTOZEROSKIPENDPIXELS set to %d", m_dwAutoZeroSkipEndPixels);
	}

	m_bHighGainOverride = false;
	m_bLowGainOverride = false;
	m_dHighGainOverride = 0.0;
	m_dLowGainOverride = 0.0;

	// Command sensed by sending commands to camera and looking for a timeout
	// Do this one at initial connection time.	
	m_bHasCMD_GetTemperatureEx = false;
	m_bHasCMD_StartExposureEx = false;
	m_bHasCMD_SetFilterTrim = false;
	m_bHasCMD_GetFeatures = false;
	
	m_TriggerMode = TRIG_DISABLE;

	// Set some defaults
	m_CCDSpecs.EADUHigh = 1.0;
	m_CCDSpecs.EADULow  = 1.0;
	m_CCDSpecs.EFull	= 25500.0;
	m_CCDSpecs.MaxADU	= 0xffff;
	m_CCDSpecs.maxExp	= 14400;
	m_CCDSpecs.minExp   = 0.03;

	m_MaxBytesPerReadBlock = 65536;
	
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Destructor
QSI_Interface::~QSI_Interface ( void )
{
	this->m_log->Close();			// flush to log file
	this->m_log->TestForLogging();	// turn off logging is appropriate
	delete this->m_log;
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// This function solves some issue with the FTDI driver failing to send/receive data when
// a camera is freshly connected to the USB and a connection attempt is made. To see this
// issue, comment out the body of this function, unplug and replug in the camera, and 
// attempt to make a connection in MaxIm DL. If you delay any sending/receiving of data 
// to the camera after the connect, there will not be any error. To see this, put a 
// breakpoint right after the OpenCamera call, stop there for a few seconds, and then 
// continue. The delay will prevent there from being any error.
void QSI_Interface::Initialize ( void )
{
	CameraID cID;
	this->GetDeviceInfo( 0, cID );
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CountDevices( void )
{
	int iNumDevices = 0;


	m_log->Write(2, "CountDevices started");

	std::vector<CameraID> vID;
	m_iError = this->ListDevices(vID, iNumDevices); 

	m_log->Write(2, "CountDevice complete. Devices: %x , Error Code: %x", iNumDevices, m_iError);

	if( m_iError ) 
		iNumDevices = 0;

	return iNumDevices;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::GetDeviceInfo( int iIndex, CameraID & cID )
{
	int iNumFound;

	m_log->Write(2, "GetDeviceInfo Description started");

	std::vector<CameraID> vID;
	m_iError = this->ListDevices(vID, iNumFound); 

	if( iIndex >= (int)vID.size() || m_iError != 0 ) 
	{
		m_iError += ERR_IFC_GetDeviceInfo;
		m_log->Write(2, "GetDeviceInfo Description failed. iIndex: %d, iNumFound: %d, Error Code: %x", iIndex, iNumFound, m_iError);
	}
	else
	{
		cID = vID[iIndex];
		m_log->Write(2, "GetDeviceInfo Description complete. Serial number: %s, Desc: %s, Error Code: %x", cID.SerialNumber.c_str(), cID.Description.c_str(), m_iError);
	}
	
	return m_iError;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::ListDevices(std::vector <CameraID> & vID, int & iNumFound)
{
	return ListDevices( vID, CameraID::CP_All, iNumFound );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
int QSI_Interface::ListDevices( std::vector <CameraID> & vID, CameraID::ConnProto_t proto, int & iNumFound )
{
	m_log->Write(2, _T("ListDevices started"));
	// Get serial numbers
	m_iError = m_HostCon.ListDevices( vID , proto);  
	if( m_iError )
	{
		m_log->Write(2, _T("ListDevices failed. Error Code: %I32x"), m_iError);
		return m_iError + ERR_IFC_ListSerial;
	}

	iNumFound = vID.size();

	m_log->Write(2, _T("ListDevices completed ok.  Num found: %I32x"), iNumFound);
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Opens the connection to the camera
int QSI_Interface::OpenCamera ( std::string strSerialNumber )
{
	int iError = 0;
	int iNumFound;
	std::vector<CameraID> vID;
	CameraID cID;

	m_log->TestForLogging();
	m_log->Write(2, "OpenCamera by serial number started.");

	iError = ListDevices(vID, iNumFound);
	if (iError == 0)
	{
		// Find the CameraID for this serial number
		iError = ERR_PKT_OpenFailed;
		for (int i = 0; i < (int)vID.size(); i++)
		{
			if (vID[i].SerialNumber == std::string(strSerialNumber))
			{
				cID = vID[i];
				iError = 0;
				break;
			}
		}
	}
	else
	{
		// Handle case where list devices failed.
		cID = CameraID(std::string(strSerialNumber), std::string(strSerialNumber), "Unknown", 0x403, 0xeb48, CameraID::CP_USB);
		iError = 0;
	}

	// If match found, open the device
	if (iError == 0)
	{
		iError = OpenCamera( cID );
	}
	m_log->Write(2, "OpenCamera by serial completed. Error Code: %d", m_iError);
	return iError;
}

int QSI_Interface::OpenCamera( CameraID cID )
{
	QSI_Registry reg;
	int iError = 0;
	int iRetVal;

	m_log->TestForLogging();
	m_log->Write(2, "OpenCamera by CameraID number started.");
	
	iError = m_HostCon.Open( cID );
	if (iError != 0)
		return iError;	

	m_MaxBytesPerReadBlock = reg.GetMaxPixelsPerBlock( (m_HostCon.m_HostIO) ? m_HostCon.m_HostIO->MaxBytesPerReadBlock() : 65536 );
	if (m_MaxBytesPerReadBlock < 1) m_MaxBytesPerReadBlock = 1;	
	m_log->Write(2, _T("Open: MaxBytesPerReadBlock: %d for this connection."), m_MaxBytesPerReadBlock);

	m_hpmMap = HotPixelMap(cID.SerialNumber);
	m_log->Write(2, "OpenCamera completed. Error Code: %x", m_iError);
	
	m_iError = CMD_InitCamera();
	if (m_iError != 0)
		return m_iError;
	
	std::string strKeyPath = "Software\\QSI\\"+ cID.SerialNumber;
	m_bHighGainOverride = reg.GetInt (strKeyPath, _T("HIGHGAIN"), 1000, iRetVal);
	m_dHighGainOverride = iRetVal/1000.0;
	if(m_bHighGainOverride ) m_log->Write(2, _T("High gain override: %f"), m_dHighGainOverride);

	m_bLowGainOverride = reg.GetInt (strKeyPath, _T("LOWGAIN"), 1000, iRetVal);
	m_dLowGainOverride = iRetVal/1000.0;
	if(m_bLowGainOverride ) m_log->Write(2, _T("Low gain override: %f"), m_dLowGainOverride);

	// Sense extended GetTemp command in 600 series
	int iTemp1;
	double dTemp2, dTemp3, dTemp5;
	unsigned short usTemp4;

	// Get special camera configuration bytes to controlled various features in plugins
	BYTE pFeatures[QSIFeatures::IMAXFEATURES];
	int iFeaturesReturned = 0;
	m_iError = CMD_GetFeatures(pFeatures, QSIFeatures::IMAXFEATURES, iFeaturesReturned);
	if (m_iError == 0)
	{
		m_bHasCMD_GetFeatures = true;
		m_Features.GetFeatures(pFeatures, iFeaturesReturned);
		m_log->Write(2, _T("GetFeatures completed OK."));
		m_log->WriteBuffer(3, pFeatures, QSIFeatures::IMAXFEATURES, iFeaturesReturned, 256);
	}
	else
	{
		m_bHasCMD_GetFeatures = false;
		m_Features.GetFeatures(pFeatures, 0);	// Clear all features
		m_log->Write(2, _T("GetFeatures failed. Error Code: %I32x"), m_iError);
		m_iError = 0;
	}

	// Reset any non-sticky soft modes
	// Basic external strigger is reset to disable on each new connection.
	if (m_Features.HasBasicHWTrigger())
	{
		CMD_ExtTrigMode( TRIG_DISABLE, 0);
	}

	m_bHasCMD_GetTemperatureEx = CMD_GetTemperatureEx(iTemp1, dTemp2, dTemp3, usTemp4, dTemp5, true)==0?true:false;
	HasFastExposure(m_bHasCMD_StartExposureEx);
	m_bHasCMD_SetFilterTrim = HasFilterWheelTrim();

	return iError;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Closes the connection to the camera
int QSI_Interface::CloseCamera ( void )
{
	int iError;

	m_log->Write(2, "CloseCamera started");

	iError = m_HostCon.Close();
	m_log->Write(2, "CloseCamera completed. Error Code: %x", m_iError);

	return iError;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
int  QSI_Interface::ReadImageByRow(PVOID pvRxBuffer, int iRowsRequested, int iColumnsRequested, int iStride, int iPixelSize, int & iRowsRead)
{
	int iError;
	int iRowsToRead;
	int iBytesReturned;

	// ReadImageByRow may return fewer rows than requested.  It is up to the caller to make additional calls to retreive the entire image.
	// iStride is in BYTES per horizontal row.

	m_bCameraStateCacheInvalid = true;
	m_log->Write(2, _T("ReadImageByRow started. Rows requested to read: %d"), iRowsRequested);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}

	// Calculate number of rows to read, based on connection type. Some protocols are limited to one row at a time
	int iMaxRows = m_MaxBytesPerReadBlock / (iColumnsRequested * iPixelSize);
	if (iMaxRows == 0)
		iMaxRows = 1;
	
	if (m_HostCon.m_HostIO->GetTransferType() == IOType_SingleRow)
		iRowsToRead = 1;
	else if (iColumnsRequested * iPixelSize != iStride) // Non-contiguous rows, so read one row at a time
		iRowsToRead = 1;
	else if (iRowsRequested < iMaxRows)
		iRowsToRead = iRowsRequested;
	else
		iRowsToRead = iMaxRows;

	iError = m_HostCon.m_HostIO->Read((unsigned char *)pvRxBuffer, iRowsToRead * iColumnsRequested * iPixelSize, &iBytesReturned);

	iRowsRead = (iBytesReturned / iPixelSize) / iColumnsRequested;

	if (iRowsRead != iRowsToRead)
	{
		iError = ERR_IFC_TransferImage + ERR_PKT_BlockRxTooLittle;
		m_log->Write(2, _T("ReadImageByRow completed with Error Code: %d"), iError);
		return iError;
	}

	m_log->Write(2, _T("ReadImageByRow completed."));
	return iError;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
int QSI_Interface::CMD_InitCamera ( void )
{
	m_log->Write(2, "InitCamera started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	//
	//	Send the Init Request.  This may take up to 600ms to run.
	//
	// Define command packet structure
	m_log->Write(2, "Send InitCamera packet.");
	
	const int CMD_Command      = 0;
	const int CMD_Length       = 1;

	// Define response packet structure
	const int RSP_DeviceError  = 2;

	// Construct command packet header
	Cmd_Pkt[CMD_Command] = CMD_INIT;
	Cmd_Pkt[CMD_Length] = 0;
	int retry = 2;
	do
	{
		m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO,Cmd_Pkt,Rsp_Pkt, true);
	}
	while (m_iError != 0 && retry-- > 0);
			
	if( m_iError )
	{
		m_log->Write(2, "InitCamera failed. Error Code: %x", m_iError);
		return m_iError;
	}
		
	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "InitCamera failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_InitCamera;
	}
	m_log->Write(2, "InitCamera completed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_GetDeviceDetails ( QSI_DeviceDetails & DeviceDetails )
{
	m_log->Write(2, "GetDeviceDetails started");
	
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_HasCamera        = 2;
	const int RSP_HasShutter       = 3;
	const int RSP_HasFilter        = 4;
	const int RSP_HasRelays        = 5;
	const int RSP_HasTempReg       = 6;
	const int RSP_ArrayColumns     = 7;
	const int RSP_ArrayRows        = 9;
	const int RSP_XAspect          = 11;
	const int RSP_YAspect          = 13;
	const int RSP_MaxHBinning      = 15;
	const int RSP_MaxVBinning      = 16;
	const int RSP_AsymBin          = 17;
	const int RSP_TwoTimesBinning  = 18;
	const int RSP_NumRowsPerBlock  = 19;
	const int RSP_ControlEachBlock = 21;
	const int RSP_NumFilters       = 22;
	const int RSP_ModelNumber      = 23;
	const int RSP_ModelName        = 55;
	const int RSP_SerialNumber     = 87;
	const int RSP_DeviceError      = 103;

	// Construct command packet header
	Cmd_Pkt[CMD_Command] = CMD_GETDEVICEDETAILS;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt,true);
	if( m_iError )
	{
		m_log->Write(2, "GetDeviceDetails failed. Error Code: %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetDeviceDetails failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_GetDeviceDetails;
	}

	// Retrieve each parameter from response packet
	DeviceDetails.HasCamera         = GetBoolean(Rsp_Pkt[RSP_HasCamera]);
	DeviceDetails.HasShutter        = GetBoolean(Rsp_Pkt[RSP_HasShutter]);
	DeviceDetails.HasFilter         = GetBoolean(Rsp_Pkt[RSP_HasFilter]);
	DeviceDetails.HasRelays         = GetBoolean(Rsp_Pkt[RSP_HasRelays]);
	DeviceDetails.HasTempReg        = GetBoolean(Rsp_Pkt[RSP_HasTempReg]);
	DeviceDetails.ArrayColumns      = Get2Bytes(&Rsp_Pkt[RSP_ArrayColumns]);
	DeviceDetails.ArrayRows         = Get2Bytes(&Rsp_Pkt[RSP_ArrayRows]);
	DeviceDetails.XAspect           = Get2Bytes(&Rsp_Pkt[RSP_XAspect]);
	DeviceDetails.YAspect           = Get2Bytes(&Rsp_Pkt[RSP_YAspect]);
	DeviceDetails.MaxHBinning       = Rsp_Pkt[RSP_MaxHBinning];
	DeviceDetails.MaxVBinning       = Rsp_Pkt[RSP_MaxVBinning];
	DeviceDetails.AsymBin           = GetBoolean(Rsp_Pkt[RSP_AsymBin]);
	DeviceDetails.TwoTimesBinning   = GetBoolean(Rsp_Pkt[RSP_TwoTimesBinning]);
	DeviceDetails.NumRowsPerBlock   = Get2Bytes(&Rsp_Pkt[RSP_NumRowsPerBlock]);
	DeviceDetails.ControlEachBlock  = GetBoolean(Rsp_Pkt[RSP_ControlEachBlock]);
	DeviceDetails.NumFilters        = Rsp_Pkt[RSP_NumFilters];

	// Extract model name/type/number/serialnumber info
	const int iModelNumberLen = RSP_ModelName - RSP_ModelNumber;
	const int iModelNameLen	= RSP_SerialNumber - RSP_ModelName;
	const int iSerialNumberLen = RSP_DeviceError - RSP_SerialNumber;
	// from low eeprom config memory:
	DeviceDetails.ModelType		= GetStdString( &Rsp_Pkt[RSP_ModelNumber],  iModelNumberLen);		// "504ws" Full model type number field
	DeviceDetails.ModelName		= GetStdString( &Rsp_Pkt[RSP_ModelName],    iModelNameLen);			// "QSI 500 Series Camera"
	DeviceDetails.SerialNumber	= GetStdString( &Rsp_Pkt[RSP_SerialNumber], iSerialNumberLen);		// "05001234"
	// Model Base Type
	// "504" Model number, leading numeric digits for matching hex files
	size_t nonNum = DeviceDetails.ModelType.find_first_not_of("0123456789");
	if (nonNum != std::string::npos)
	{
		DeviceDetails.ModelBaseType = DeviceDetails.ModelType.substr(0, nonNum);
	}
	else
	{
		DeviceDetails.ModelBaseType	= DeviceDetails.ModelType;	
	}
	
	// From Feature bytes
	QSIModelInfo modelInfo(this);
	DeviceDetails.ModelBaseNumber = modelInfo.GetBaseModelNumber(DeviceDetails.ModelBaseType);	// "683" or "RS8.3"
	DeviceDetails.ModelNumber = modelInfo.GetModelNumber(DeviceDetails.ModelType);				// "683ws" or "RS8.3ws" for user display+
	DeviceDetails.ModelName = modelInfo.GetModelName(DeviceDetails.ModelName);					// "QSI 683 Series Camera"
	//
	DeviceDetails.HasFilterTrim = m_bHasCMD_SetFilterTrim;

	// Parse feature array to produce device details
	DeviceDetails.HasCMD_GetTemperatureEx =		m_bHasCMD_GetTemperatureEx;
	DeviceDetails.HasCMD_SetFilterTrim =		m_bHasCMD_SetFilterTrim;
	DeviceDetails.HasCMD_StartExposureEx =		m_bHasCMD_StartExposureEx;
	DeviceDetails.HasCMD_HSRExposure = 			m_Features.HasHSRExposure();
	DeviceDetails.HasCMD_PVIMode	 = 			m_Features.HasPVIMode();
	DeviceDetails.HasCMD_LockCamera = 			m_Features.HasLockCamera();
	DeviceDetails.HasCMD_BasicHWTrigger = 		m_Features.HasBasicHWTrigger();
	
	// Log results
	m_log->Write(2, _T("GetDeviceDetails: Has Camera: %d"), DeviceDetails.HasCamera?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Shutter: %d"), DeviceDetails.HasShutter?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Filter: %d"), DeviceDetails.HasFilter?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Relays: %d"), DeviceDetails.HasRelays?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has TempReg: %d"), DeviceDetails.HasTempReg?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Columns: %d"), DeviceDetails.ArrayColumns);
	m_log->Write(2, _T("GetDeviceDetails: Rows: %d"), DeviceDetails.ArrayRows);
	m_log->Write(2, _T("GetDeviceDetails: XAspect: %f"), DeviceDetails.XAspect);
	m_log->Write(2, _T("GetDeviceDetails: YAspect: %f"), DeviceDetails.YAspect);
	m_log->Write(2, _T("GetDeviceDetails: Max H Bin: %d"), DeviceDetails.MaxHBinning);
	m_log->Write(2, _T("GetDeviceDetails: Max V Bin: %d"), DeviceDetails.MaxVBinning);
	m_log->Write(2, _T("GetDeviceDetails: Asym Binnning: %d"), DeviceDetails.AsymBin?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Two Times Binnning: %d"), DeviceDetails.TwoTimesBinning?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Num Rows per Block: %d"), DeviceDetails.NumRowsPerBlock);
	m_log->Write(2, _T("GetDeviceDetails: ControlEachBlock: %d"), DeviceDetails.ControlEachBlock?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Num Filters: %d"), DeviceDetails.NumFilters);
	m_log->Write(2, _T("GetDeviceDetails: Model Number: %s"), DeviceDetails.ModelNumber.c_str());
	m_log->Write(2, _T("GetDeviceDetails: Model Name: %s"), DeviceDetails.ModelName.c_str());
	m_log->Write(2, _T("GetDeviceDetails: Model Serial Number: %s"), DeviceDetails.SerialNumber.c_str());
	m_log->Write(2, _T("GetDeviceDetails: Filter Wheel Trim: %d"), DeviceDetails.HasFilterTrim?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has HSRExposure: %d"), DeviceDetails.HasCMD_HSRExposure?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has GetTemperatureEx: %d"), DeviceDetails.HasCMD_GetTemperatureEx?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Set FilterWheel Trim: %d"), DeviceDetails.HasCMD_SetFilterTrim?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has StartExposureEx: %d"), DeviceDetails.HasCMD_StartExposureEx?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has HSR Exposure: %d"), DeviceDetails.HasCMD_HSRExposure?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has PVI Mode: %d"), DeviceDetails.HasCMD_PVIMode?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Lock Camera: %d"), DeviceDetails.HasCMD_LockCamera?1:0);
	m_log->Write(2, _T("GetDeviceDetails: Has Basic HW Trigger: %d"), DeviceDetails.HasCMD_BasicHWTrigger?1:0);
	m_log->Write(2, _T("GetDeviceDetails completed OK."));

	m_DeviceDetails = DeviceDetails;
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// AutoGainAdjust - Adjust gain based on binning settings for a given exposure
int QSI_Interface::AutoGainAdjust(QSI_ExposureSettings ExpSettings, QSI_AdvSettings AdvSettings)
{
	m_log->Write(3, _T("Checking for Automatic Gain Selection"));

	QSI_AdvSettings newAdvSettings = AdvSettings;

	// Adjust for Auto gain if set by request AdvSettings from user
	int gainRequired = AdvSettings.CameraGainIndex;
	if (gainRequired == GAIN_AUTO)
	{
		if (ExpSettings.BinFactorY == 1 && ExpSettings.BinFactorX == 1)
		{
			gainRequired = GAIN_HIGH; // High Gain
			m_log->Write(3, _T("AutoGain has selected is High Gain"));
		}
		else
		{
			gainRequired = GAIN_LOW; // Low Gain
			m_log->Write(3, _T("AutoGain has selected is Low Gain"));
		}
	}
	else
	{
		m_log->Write(3, _T("Not Autogain. No gain adjusted."));
	}

	// Now check the cached value on the camera.
	// We don't change gain on every exposure, as that would slow the frame rate considerably.
	if (gainRequired != m_CameraAdvSettingsCache.CameraGainIndex)
	{
		// Update Camera and cache settings
		m_log->Write(3, _T("AutoGain cache invalid.  Update Gain setting camera to gain index: %d"), gainRequired);
		newAdvSettings.CameraGainIndex = gainRequired;
		UpdateAdvSettings(newAdvSettings);
	}
	else
	{
		m_log->Write(3, _T("Using AutoGain cache setting on camera. Gain index: %d"), m_CameraAdvSettingsCache.CameraGainIndex);
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_StartExposure ( QSI_ExposureSettings ExposureSettings )
{
	m_log->Write(2,  "StartExposure Started %d milliseconds, %d x, %d y",ExposureSettings.Duration * 10, ExposureSettings.ColumnsToRead, ExposureSettings.RowsToRead);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	AutoGainAdjust( ExposureSettings, m_UserRequestedAdvSettings);
	
	// Define command packet structure
	const int CMD_Command          = 0;
	const int CMD_Length           = 1;
	const int CMD_Duration         = 2;
	const int CMD_ColumnOffset     = 5;
	const int CMD_RowOffset        = 7;
	const int CMD_ColumnsToRead    = 9;
	const int CMD_RowsToRead       = 11;
	const int CMD_BinY             = 13;
	const int CMD_BinX             = 14;
	const int CMD_Light            = 15;
	const int CMD_FastReadout      = 16;
	const int CMD_HoldShutterOpen  = 17;

	// Define response packet structure
	const int RSP_DeviceError  = 2;
	
	// Construct command packet header
	Cmd_Pkt[CMD_Command] = CMD_STARTEXPOSURE;
	Cmd_Pkt[CMD_Length] = 16;

	// Construct command packet body
	Put3Bytes(&Cmd_Pkt[CMD_Duration],       ExposureSettings.Duration );
	Put2Bytes(&Cmd_Pkt[CMD_ColumnOffset],   ExposureSettings.ColumnOffset);
	Put2Bytes(&Cmd_Pkt[CMD_RowOffset],      ExposureSettings.RowOffset);
	Put2Bytes(&Cmd_Pkt[CMD_ColumnsToRead],  ExposureSettings.ColumnsToRead);
	Put2Bytes(&Cmd_Pkt[CMD_RowsToRead],     ExposureSettings.RowsToRead);
	Cmd_Pkt[CMD_BinY] =                     ExposureSettings.BinFactorY;
	Cmd_Pkt[CMD_BinX] =                     ExposureSettings.BinFactorX;
	PutBool(&Cmd_Pkt[CMD_Light],            ExposureSettings.OpenShutter);
	PutBool(&Cmd_Pkt[CMD_FastReadout],      ExposureSettings.FastReadout);
	PutBool(&Cmd_Pkt[CMD_HoldShutterOpen],  ExposureSettings.HoldShutterOpen);

	m_log->Write(2, _T("Duration: %d"),ExposureSettings.Duration);
	m_log->Write(2, _T("Column Offset: %d"),ExposureSettings.ColumnOffset);
	m_log->Write(2, _T("Row Offset: %d"),ExposureSettings.RowOffset);
	m_log->Write(2, _T("Columns: %d"),ExposureSettings.ColumnsToRead);
	m_log->Write(2, _T("Rows: %d"),ExposureSettings.RowsToRead);
	m_log->Write(2, _T("Bin Y: %d"),ExposureSettings.BinFactorY);
	m_log->Write(2, _T("Bin X: %d"),ExposureSettings.BinFactorX);
	m_log->Write(2, _T("Open Shutter: %d"),ExposureSettings.OpenShutter);
	m_log->Write(2, _T("Fast Readout: %d"),ExposureSettings.FastReadout);
	m_log->Write(2, _T("Hold Shutter Open: %d"),ExposureSettings.HoldShutterOpen);

	
	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, IOTimeout_Long);
	if( m_iError ) 
	{
		m_log->Write(2, "StartExposure failed. Error Code: %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "StartExposure failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_StartExposure;
	}

	m_log->Write(2, "StartExposure completed OK");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_StartExposureEx( QSI_ExposureSettings ExposureSettings )
{
	m_log->Write(2, "StartExposureEx started. Duration: %d, DurationUSec: %d.", ExposureSettings.Duration, ExposureSettings.DurationUSec);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	if (ExposureSettings.OpenShutter)
		m_log->Write(2, "StartExposureEx Light = true.");
	else
		m_log->Write(2, "StartExposureEx Light = false.");

	AutoGainAdjust (ExposureSettings, m_UserRequestedAdvSettings);
	
	// Define command packet structure
	const int CMD_Command          = 0;
	const int CMD_Length           = 1;
	const int CMD_Duration         = 2; // 3 byte duration in 10ms increments
	const int CMD_DurationUSec	   = 5; // remaining dureation in 100uSec increments
	const int CMD_ColumnOffset     = 6;
	const int CMD_RowOffset        = 8;
	const int CMD_ColumnsToRead    = 10;
	const int CMD_RowsToRead       = 12;
	const int CMD_BinY             = 14;
	const int CMD_BinX             = 16;
	const int CMD_Light            = 18;
	const int CMD_FastReadout      = 19;
	const int CMD_HoldShutterOpen  = 20;
	const int CMD_UseExtTrigger    = 21;
	const int CMD_StrobeShutterOutput = 22;
	const int CMD_ExpRepeatCount   = 23;
	const int CMD_Implemented      = 25;
	const int CMD_END			   = 26;

	// Define response packet structure
	const int RSP_DeviceError  = 2;
	m_bCameraStateCacheInvalid = true;

	// Construct command packet header
	Cmd_Pkt[CMD_Command] = CMD_STARTEXPOSUREEX;
	Cmd_Pkt[CMD_Length] = CMD_END - 2;	// Don't count the first two bytes

	// Construct command packet body
	Put3Bytes(&Cmd_Pkt[CMD_Duration],       ExposureSettings.Duration );
	Cmd_Pkt[CMD_DurationUSec] =				ExposureSettings.DurationUSec;
	Put2Bytes(&Cmd_Pkt[CMD_ColumnOffset],   ExposureSettings.ColumnOffset);
	Put2Bytes(&Cmd_Pkt[CMD_RowOffset],      ExposureSettings.RowOffset);
	Put2Bytes(&Cmd_Pkt[CMD_ColumnsToRead],  ExposureSettings.ColumnsToRead);
	Put2Bytes(&Cmd_Pkt[CMD_RowsToRead],     ExposureSettings.RowsToRead);
	Put2Bytes(&Cmd_Pkt[CMD_BinY],           ExposureSettings.BinFactorY);
	Put2Bytes(&Cmd_Pkt[CMD_BinX],           ExposureSettings.BinFactorX);
	PutBool(&Cmd_Pkt[CMD_Light],            ExposureSettings.OpenShutter);
	PutBool(&Cmd_Pkt[CMD_FastReadout],      ExposureSettings.FastReadout);
	PutBool(&Cmd_Pkt[CMD_HoldShutterOpen],  ExposureSettings.HoldShutterOpen);
	PutBool(&Cmd_Pkt[CMD_UseExtTrigger],    ExposureSettings.UseExtTrigger);
	PutBool(&Cmd_Pkt[CMD_StrobeShutterOutput], ExposureSettings.StrobeShutterOutput);
	Put2Bytes(&Cmd_Pkt[CMD_ExpRepeatCount],	ExposureSettings.ExpRepeatCount);
	PutBool(&Cmd_Pkt[CMD_Implemented],		ExposureSettings.ProbeForImplemented);

	m_log->Write(2, _T("Duration: %d"),ExposureSettings.Duration);
	m_log->Write(2, _T("DurationUSec: %d"),ExposureSettings.DurationUSec);
	m_log->Write(2, _T("Column Offset: %d"),ExposureSettings.ColumnOffset);
	m_log->Write(2, _T("Row Offset: %d"),ExposureSettings.RowOffset);
	m_log->Write(2, _T("Columns: %d"),ExposureSettings.ColumnsToRead);
	m_log->Write(2, _T("Rows: %d"),ExposureSettings.RowsToRead);
	m_log->Write(2, _T("Bin Y: %d"),ExposureSettings.BinFactorY);
	m_log->Write(2, _T("Bin X: %d"),ExposureSettings.BinFactorX);
	m_log->Write(2, _T("Open Shutter: %d"),ExposureSettings.OpenShutter?1:0);
	m_log->Write(2, _T("Fast Readout: %d"),ExposureSettings.FastReadout?1:0);
	m_log->Write(2, _T("Hold Shutter Open: %d"),ExposureSettings.HoldShutterOpen?1:0);
	m_log->Write(2, _T("Ext Trigger Output: %d"),ExposureSettings.UseExtTrigger?1:0);
	m_log->Write(2, _T("Strobe Shutter Output: %d"),ExposureSettings.StrobeShutterOutput?1:0);
	m_log->Write(2, _T("Exp Repeat Count: %d"),ExposureSettings.ExpRepeatCount);
	m_log->Write(2, _T("Ext Trigger Input Mode: %d"), m_TriggerMode);
	m_log->Write(2, _T("Implemented: %d"),ExposureSettings.ProbeForImplemented?1:0);

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, ExposureSettings.ProbeForImplemented?IOTimeout_Short:IOTimeout_Long);
	if( m_iError ) 
	{
		m_log->Write(2, "StartExposureEx failed. Error Code: %I32x", m_iError);
		return m_iError  + ERR_PKT_SetTimeOutFailed;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "StartExposureEx failed. Error Code: %I32x", m_iError);
		return m_iError + ERR_IFC_StartExposure;
	}

	m_log->Write(2, "StartExposureEx completed OK");

	return ALL_OK;

}

//////////////////////////////////////////////////////////////////////////////////////////
// Probe for fast exposure command
int QSI_Interface::HasFastExposure( bool & bFast )
{
	m_log->Write(2, "Probe for StartExposureEx started.");
	QSI_ExposureSettings ExposureSettings;

	ExposureSettings.ProbeForImplemented = true;

	// Send command
	int iError;
	iError = CMD_StartExposureEx( ExposureSettings );
	if (iError)
		bFast = false;
	else
		bFast = true;

	m_iError = 0;

	m_log->Write(2, "Probe for StartExposureEx completed OK Result: %d", bFast?1:0);

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Sends command to camera to stop current exposure
int QSI_Interface::CMD_AbortExposure ( void )
{
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_DeviceError = 2;
	m_log->Write(2, "AbortExposure started");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_ABORTEXPOSURE;
	Cmd_Pkt[CMD_Length] = 2;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "AbortExposure failed. Error Code: %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "AbortExposure failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_AbortExposure;
	}
	m_log->Write(2, "AbortExposure completed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Sends command to camera to begin transfer of 1 block
int QSI_Interface::CMD_TransferImage ( void )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_DeviceError = 2;
	m_log->Write(2, "TransferImage started");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_TRANSFERIMAGE;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, false);

	// Check for errors (receive queue should be dirty so we ignore that)
	if( m_iError != ALL_OK)
	{
		m_log->Write(2, "TransferImage failed. Error Code: %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "TransferImage failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_TransferImage;
	}
	m_log->Write(2, "TransferImage completed OK");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Sends command to camera to get AutoZero parameters and start transfer of auto zero data
int QSI_Interface::CMD_GetAutoZero( QSI_AutoZeroData & AutoZeroData )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_ZeroEnable	   = 2;
	const int RSP_ZeroLevel      = 3;
	const int RSP_PixelCount     = 5;
	const int RSP_DeviceError    = 7;
	m_log->Write(2, "GetAutoZero started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETAUTOZERO;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt,false);

	// On a slow system, the data could start coming back before we get here
	// so check for rxDirtyQueue, which is ok 
	// Check for errors
	if( m_iError != ALL_OK)
	{
		m_log->Write(2, "GetAutoZero failed. Error Code: %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetAutoZero failed. Error Code: %x", m_iError);
		return m_iError + ERR_IFC_TransferImage;
	}

	AutoZeroData.zeroEnable = GetBoolean(Rsp_Pkt[RSP_ZeroEnable]);
	AutoZeroData.zeroLevel = Get2Bytes(&Rsp_Pkt[RSP_ZeroLevel]);
	AutoZeroData.pixelCount = Get2Bytes(&Rsp_Pkt[RSP_PixelCount]);

	m_log->Write(2, "GetAutoZero completed OK. Enable: %s Level: %x Count: %x", AutoZeroData.pixelCount?"T":"F", AutoZeroData.zeroLevel, AutoZeroData.pixelCount);
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// This function returns an error code and the device's state (byref parameter)
int QSI_Interface::CMD_GetDeviceState ( int & iCameraState, bool & bShutterOpen, bool & bFilterState )
{
	// Handle previous error
	if( m_iError )
	{
		iCameraState = -1;
		return m_iError;
	}

	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_CameraState  = 2;
	const int RSP_ShutterState = 3;
	const int RSP_FilterState  = 4;
	const int RSP_DeviceError  = 5;

	m_log->Write(2, "GetDeviceState started");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETDEVICESTATE;
	Cmd_Pkt[CMD_Length] = 0;

	int iRetries = 2;
	do
	{
		// Send/receive packets
		m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
		if (m_iError)
		{
			m_log->Write(2, "GetDeviceState Send/Rec Packet Error %x, retries left: %x", m_iError, iRetries);
			usleep(INTERFACERETRYMS * 1000L);
		}

	} while (m_iError != 0 && iRetries-- > 0);

	if( m_iError ) 
	{
		m_log->Write(2, "GetDeviceState Send/Receive Packet Error %x, failed", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];

	if( m_iError ) 
	{
		m_log->Write(2, "GetDeviceState CMD Response Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetDeviceState;
	}

	// Retrieve each parameter from response packet
	iCameraState = Rsp_Pkt[RSP_CameraState];
	bShutterOpen = GetBoolean(Rsp_Pkt[RSP_ShutterState]);
	bFilterState = GetBoolean(Rsp_Pkt[RSP_FilterState]);
	
	if (m_TriggerMode != TRIG_DISABLE  && iCameraState == CCD_ERROR)
	{
		// Issue a trigger command reset
		CMD_ExtTrigMode( TRIG_CLEARERROR, 0);
	}

	m_log->Write(2, _T("GetDeviceState completed OK. Camera: %d Shutter: %d Filter: %d"), iCameraState, bShutterOpen, bFilterState);
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
//
int QSI_Interface::CMD_SetTemperature ( bool bCoolerOn, bool bGoToAmbient, double dSetPoint)
{
	// Define command packet structure
	const int CMD_Command      = 0;
	const int CMD_Length       = 1;
	const int CMD_CoolerOn     = 2;
	const int CMD_GotoAmbient  = 3;
	const int CMD_SetPoint     = 4;

	// Define response packet structure
	const int RSP_DeviceError  = 2;

	m_log->Write(2, "SetTemperature started Cooler: %s, Set point: %f", bCoolerOn?"On":"Off", dSetPoint);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_SETTEMPERATURE;
	Cmd_Pkt[CMD_Length]  = 4;

	short tmp = 0;
	Put2Bytes( &tmp, (short) (dSetPoint * 100) );

	// Construct transmit packet body
	PutBool(&Cmd_Pkt[CMD_CoolerOn],    bCoolerOn);
	PutBool(&Cmd_Pkt[CMD_GotoAmbient], bGoToAmbient);
	Put2Bytes(&Cmd_Pkt[CMD_SetPoint],  (short) (dSetPoint * 100) );

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2,  "SetTemperature failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "SetTemperature failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_SetTemperature;
	}

	m_log->Write(2, "SetTemperature completed OK.");
	return ALL_OK;

}

//////////////////////////////////////////////////////////////////////////////////////////
//
int QSI_Interface::CMD_GetTemperature ( int & iCoolerState, double & dCoolerTemp, double & dTempAmbient, USHORT & usCoolerPower )
{
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_CoolerState  = 2;
	const int RSP_CoolerTemp   = 3;
	const int RSP_TempAmbient  = 5;
	const int RSP_CoolerPower  = 7;
	const int RSP_DeviceError  = 9;
	m_log->Write(2, "GetTemperature started");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETTEMPERATURE;
	Cmd_Pkt[CMD_Length]  = 0;

	int iRetries = 2;
	do
	{
		// Send/receive packets
		m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
		if (m_iError)
		{
			m_log->Write(2, "GetTemperature Send/Rec Packet Error %x, retries left: %x", m_iError, iRetries);
			usleep(INTERFACERETRYMS * 1000L);
		}

	} while (m_iError != 0 && iRetries-- > 0);

	if( m_iError ) 
	{
		m_log->Write(2, "GetTemperature Send/Rec Packet Error %x, failed", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];

	if( m_iError )
	{
		m_log->Write(2, "GetTemperature CMD Response Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetTemperature;
	}

	// Retrieve each parameter from response packet
	iCoolerState  = Rsp_Pkt[RSP_CoolerState];

	//short tmp = Get2Bytes(&Rsp_Pkt[RSP_CoolerTemp]);
	//double a = tmp / 100.0;

	dCoolerTemp   = (short) Get2Bytes(&Rsp_Pkt[RSP_CoolerTemp]) / 100.0;
	dTempAmbient  = (short) Get2Bytes(&Rsp_Pkt[RSP_TempAmbient]) / 100.0;
	usCoolerPower = (short) ( Get2Bytes(&Rsp_Pkt[RSP_CoolerPower]) / 100.0 );
	m_log->Write(2, "GetTemperature completed OK. Cooler power: %d, Temp: %f Camera Body Temp: %f", usCoolerPower, dCoolerTemp, dTempAmbient);
	return ALL_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
int QSI_Interface::CMD_GetTemperatureEx( int & iCoolerState, double & dCoolerTemp, double & dHotsideTemp, USHORT & usCoolerPower, double & dPCBTemp, bool bProbe )
{
	if (!(m_DeviceDetails.HasCMD_GetTemperatureEx || bProbe))
	{
		dPCBTemp = 0.0;
		return CMD_GetTemperature(iCoolerState, dCoolerTemp, dHotsideTemp, usCoolerPower);
	}

	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_CoolerState  = 2;
	const int RSP_CoolerTemp   = 3;	 // Cold finger temp.
	const int RSP_HotsideTemp  = 5;  // AKA body temp.
	const int RSP_AmbientTemp  = 7;  // PCBTemp. Called Ambient temp in firmware
	const int RSP_CoolerPower  = 9;
	const int RSP_DeviceError  = 11;

	m_log->Write(2, _T("GetTemperatureEx started"));

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}

	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETTEMPERATUREEX;
	Cmd_Pkt[CMD_Length]  = 0;

	int iRetries = 2;
	do
	{
		// Send/receive packets
		m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, bProbe?IOTimeout_Short:IOTimeout_Normal);
		if (bProbe)
		{
			m_log->Write(2, _T("GetTemperatureEx Probe returning with status %d"), m_iError);
			return m_iError;
		}

		if (m_iError)
		{
			m_log->Write(2, _T("GetTemperatureEx Send/Rec Packet Error %I32x, retries left: %I32x"), m_iError, iRetries);
			usleep(INTERFACERETRYMS * 1000L);
		}

	} while (m_iError != 0 && iRetries-- > 0);

	if( m_iError )
	{
		m_log->Write(2, _T("GetTemperatureEx Send/Rec Packet Error %I32x, failed"), m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];

	if( m_iError )
	{
		m_log->Write(2, _T("GetTemperatureEx CMD Response Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_GetTemperature;
	}

	// Retrieve each parameter from response packet
	iCoolerState  = Rsp_Pkt[RSP_CoolerState];

	dCoolerTemp   = (short) Get2Bytes(&Rsp_Pkt[RSP_CoolerTemp]) / 100.0;
	dHotsideTemp  = (short) Get2Bytes(&Rsp_Pkt[RSP_HotsideTemp]) / 100.0;
	usCoolerPower = (short) ( Get2Bytes(&Rsp_Pkt[RSP_CoolerPower]) / 100.0 );
	dPCBTemp = (short) Get2Bytes(&Rsp_Pkt[RSP_AmbientTemp]) / 100.0 ;
	m_log->Write(2, _T("GetTemperatureEx completed OK. Cooler power: %d, cold side temp: %f, Hotside temp: %f, PCB Temp: %f"), usCoolerPower, dCoolerTemp, dHotsideTemp, dPCBTemp);

	return ALL_OK;

}
/////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_ActivateRelay ( int iXRelay, int iYRelay )
{
	// If both relays are to be moved, only move the X-axis
	if( iXRelay && iYRelay ) iYRelay = 0;

	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1; 
	const int CMD_XRelay   = 2;
	const int CMD_YRelay   = 4;

	// Define response packet structure
	const int RSP_DeviceError  = 2;
	m_log->Write(2, "ActivateRelay started. X: %x Y: %x", iXRelay, iYRelay);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_ACTIVATERELAY; 
	Cmd_Pkt[CMD_Length]  = 4;

	// Construct transmit packet body
	Put2Bytes( &Cmd_Pkt[CMD_XRelay], (short) iXRelay );
	Put2Bytes( &Cmd_Pkt[CMD_YRelay], (short) iYRelay );

	// Send/receive packets 
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "ActivateRelay failed. Error Code %x", m_iError);
		return m_iError;
	}
	  
	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "ActivateRelay failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_ActivateRelay; 
	}
	m_log->Write(2, "ActivateRelay completed OK");
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_IsRelayDone ( bool & bGuiderRelayState )
{
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_GuiderRelayState = 2;
	const int RSP_DeviceError   = 3;
	m_log->Write(2, "IsRelayDone started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_ISRELAYDONE;
	Cmd_Pkt[CMD_Length]  = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "IsRelayDone failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "IsRelayDone failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_IsRelayDone;
	}

	// Retrieve each parameter from response packet
	bGuiderRelayState = GetBoolean(Rsp_Pkt[RSP_GuiderRelayState]);
	m_log->Write(2, "IsRelayDone completed OK. State %s", bGuiderRelayState?"T":"F");
	return ALL_OK;
}
/////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_SetFilterWheel ( int iFilterPosition )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;
	const int CMD_FilterPosition = 2;

	// Define response packet structure
	const int RSP_DeviceError = 2;
	m_log->Write(2, "SetFilterWheel started. Pos: %x", iFilterPosition);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_SETFILTERWHEEL;
	Cmd_Pkt[CMD_Length]  = 1;

	// Construct transmit packet body
	Cmd_Pkt[CMD_FilterPosition] = iFilterPosition;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, IOTimeout_Long);
	if( m_iError )
	{
		m_log->Write(2, "SetFilterWheel failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_SetFilterWheel;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "SetFilterWheel failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_SetFilterWheel;
	}
		m_log->Write(2, "SetFilterWheel completed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Get the advanced default settings and enabled options
int QSI_Interface::CMD_GetCamDefaultAdvDetails( QSI_AdvSettings & AdvDefaultSettings, QSI_AdvEnabledOptions & AdvEnabledOptions,QSI_DeviceDetails DeviceDetails )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_LEDIndicatorEnabled   = 2;
	const int RSP_LEDIndicatorDefault   = 3;
	const int RSP_SoundOnEnabled        = 4;
	const int RSP_SoundOnDefault        = 5;
	const int RSP_FanModeEnabled        = 6;
	const int RSP_FanModeDefault        = 7;
	const int RSP_CameraGainEnabled     = 8;
	const int RSP_CameraGainDefault     = 9;
	const int RSP_ShutterPriorityEnabled    = 10;
	const int RSP_ShutterPriorityDefault    = 11;
	const int RSP_AntiBloomingEnabled   = 12;
	const int RSP_AntiBloomingDefault   = 13;
	const int RSP_PreExposureFlushEnabled    = 14;
	const int RSP_PreExposureFlushDefault    = 15;
	const int RSP_ShowDLProgressEnabled = 16;
	const int RSP_ShowDLProgressDefault = 17;
	const int RSP_OptimizationsEnabled  = 18;
	const int RSP_OptimizeReadoutDefault= 19;
	const int RSP_DeviceError           = 20;
	m_log->Write(2, "GetAdvDetails started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETDEFAULTADVDETAILS;
	Cmd_Pkt[CMD_Length]  = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetAdvDetails failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetAdvDetails failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetAdvDetails;
	}

	// Retrieve each parameter from response packet concerning advanced window options
	AdvEnabledOptions.LEDIndicatorOn  = this->GetBoolean( Rsp_Pkt[RSP_LEDIndicatorEnabled] );
	AdvEnabledOptions.SoundOn         = this->GetBoolean( Rsp_Pkt[RSP_SoundOnEnabled] );
	AdvEnabledOptions.FanMode         = this->GetBoolean( Rsp_Pkt[RSP_FanModeEnabled] );
	AdvEnabledOptions.CameraGain      = this->GetBoolean( Rsp_Pkt[RSP_CameraGainEnabled] );
	AdvEnabledOptions.ShutterPriority = this->GetBoolean( Rsp_Pkt[RSP_ShutterPriorityEnabled] );
	AdvEnabledOptions.AntiBlooming    = this->GetBoolean( Rsp_Pkt[RSP_AntiBloomingEnabled] );
	AdvEnabledOptions.PreExposureFlush = this->GetBoolean( Rsp_Pkt[RSP_PreExposureFlushEnabled] );
	AdvEnabledOptions.ShowDLProgress  = this->GetBoolean( Rsp_Pkt[RSP_ShowDLProgressEnabled] );
	AdvEnabledOptions.Optimizations   = this->GetBoolean( Rsp_Pkt[RSP_OptimizationsEnabled] );

	// Retrieve each parameter from response packet concerning advanced window defaults
	AdvDefaultSettings.LEDIndicatorOn       = this->GetBoolean( Rsp_Pkt[RSP_LEDIndicatorDefault] );
	AdvDefaultSettings.SoundOn              = this->GetBoolean( Rsp_Pkt[RSP_SoundOnDefault] );
	AdvDefaultSettings.FanModeIndex         = Rsp_Pkt[RSP_FanModeDefault];
	AdvDefaultSettings.CameraGainIndex      = Rsp_Pkt[RSP_CameraGainDefault];
	AdvDefaultSettings.ShutterPriorityIndex     = Rsp_Pkt[RSP_ShutterPriorityDefault];
	AdvDefaultSettings.AntiBloomingIndex    = Rsp_Pkt[RSP_AntiBloomingDefault];
	AdvDefaultSettings.PreExposureFlushIndex     = Rsp_Pkt[RSP_PreExposureFlushDefault];
	AdvDefaultSettings.ShowDLProgress       = this->GetBoolean( Rsp_Pkt[RSP_ShowDLProgressDefault] );
	AdvDefaultSettings.OptimizeReadoutSpeed = this->GetBoolean( Rsp_Pkt[RSP_OptimizeReadoutDefault] );

	AdvDefaultSettings.fwWheel = FilterWheel(DeviceDetails.NumFilters);

	m_log->Write(2, "GetAdvDetails completed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Get just the advanced default settings and NOT the enabled options
int QSI_Interface::CMD_GetAdvDefaultSettings( QSI_AdvSettings & AdvDefaultSettings, QSI_DeviceDetails DeviceDetails)
{
	QSI_AdvEnabledOptions tmp;
	return this->CMD_GetCamDefaultAdvDetails( AdvDefaultSettings, tmp, DeviceDetails );
}
//////////////////////////////////////////////////////////////////////////////////////////
// Set advanced settings
int QSI_Interface::CMD_SendAdvSettings ( QSI_AdvSettings AdvSettings )
{
	// Auto Gain Mapping
	// If GainIndex == 2, the gain is in Auto Mode based on binning (not available here).
	// When called from Open(), the gain index can could be a 2 (auto),
	// (from the Adv Settings Dialog / Registry), if so map it to default 0 (high),
	// as the camera does not accept a value of 2,
	// and then cache the current setting SENT TO THE CAMERA that is set here.  
	// StartExposure(Ex) will test the current cached value,
	// and based on the binning mode, change the gain to 0 or 1, and call here to set it.
	// On that call the gain index will be only 0 (high) or 1 (low), never 2 (auto).

	// Save settings now to check for autogain later
	m_UserRequestedAdvSettings = AdvSettings;
	if (AdvSettings.CameraGainIndex == GAIN_AUTO)
	{
		AdvSettings.CameraGainIndex = GAIN_LOW;
		m_log->Write(2, _T("Autogain setting detected, camera set to default high gain."));
	}
	
	return UpdateAdvSettings(AdvSettings);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Set advanced settings
int QSI_Interface::UpdateAdvSettings ( QSI_AdvSettings AdvSettings )
{
	// This just sends the requested settings to the camera, ignoring any auto gain issues.
	m_CameraAdvSettingsCache = AdvSettings;
	// Define command packet structure
	const int CMD_Command         = 0;
	const int CMD_Length          = 1;
	const int CMD_LEDIndicatorOn  = 2;
	const int CMD_SoundOn         = 3;
	const int CMD_FanMode         = 4;
	const int CMD_CameraGain      = 5;
	const int CMD_ShutterPriority = 6;
	const int CMD_AntiBlooming    = 7;
	const int CMD_PreExposureFlush = 8;
	const int CMD_ShowDLProgress  = 9;
	const int CMD_OptimizeReadout = 10;

	// Define response packet structure
	const int RSP_DeviceError = 2;

	m_fwWheel = AdvSettings.fwWheel;  // assign current filter wheel.
	
	m_log->Write(2, _T("SendAdvSettings started."));
	m_log->Write(2, _T("SendAdvSettings: LED Enabled %d"), AdvSettings.LEDIndicatorOn?1:0);
	m_log->Write(2, _T("SendAdvSettings: Sound Enabled %d"), AdvSettings.SoundOn?1:0);
	m_log->Write(2, _T("SendAdvSettings: Fan index %d"), AdvSettings.FanModeIndex);
	m_log->Write(2, _T("SendAdvSettings: Gain index %d"), AdvSettings.CameraGainIndex);
	m_log->Write(2, _T("SendAdvSettings: Shutter Priority index %d"), AdvSettings.ShutterPriorityIndex);
	m_log->Write(2, _T("SendAdvSettings: AntiBloom index %d"), AdvSettings.AntiBloomingIndex);
	m_log->Write(2, _T("SendAdvSettings: Flush index %d"), AdvSettings.PreExposureFlushIndex);
	m_log->Write(2, _T("SendAdvSettings: Show progress %d"), AdvSettings.ShowDLProgress?1:0);
	m_log->Write(2, _T("SendAdvSettings: Optimize readout speed %d"), AdvSettings.OptimizeReadoutSpeed?1:0);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command]  = CMD_SETADVSETTINGS;
	Cmd_Pkt[CMD_Length]   = 9;

	// Construct transmit packet body
	Cmd_Pkt[CMD_LEDIndicatorOn]   = AdvSettings.LEDIndicatorOn;
	Cmd_Pkt[CMD_SoundOn]          = AdvSettings.SoundOn;
	Cmd_Pkt[CMD_FanMode]          = AdvSettings.FanModeIndex;
	Cmd_Pkt[CMD_CameraGain  ]     = AdvSettings.CameraGainIndex;
	Cmd_Pkt[CMD_ShutterPriority]  = AdvSettings.ShutterPriorityIndex;
	Cmd_Pkt[CMD_AntiBlooming]     = AdvSettings.AntiBloomingIndex;
	Cmd_Pkt[CMD_PreExposureFlush] = AdvSettings.PreExposureFlushIndex;
	Cmd_Pkt[CMD_ShowDLProgress]   = AdvSettings.ShowDLProgress;
	Cmd_Pkt[CMD_OptimizeReadout]  = AdvSettings.OptimizeReadoutSpeed;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "SendAdvSettings failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "SendAdvSettings failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_SendAdvSettings;
	}

	// 600 Series Update.  Refresh the CCD Settings in Device Details.  These change based on the camera settings for Gain and Readout Mode
	// Get CCD and other camera parameters
	m_iError = CMD_GetCCDSpecs ( m_CCDSpecs );
	if( m_iError )
	{
		m_log->Write(2, "SendAdvSettings, GetCCDSPecs failed. Error Code %I32x", m_iError);
		return m_iError + ERR_IFC_SendAdvSettings;
	}

	m_log->Write(2, "SendAdvSettings completed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Get the current temp regulation set point
int QSI_Interface::CMD_GetSetPoint( double & dSetPoint)
{
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;

	// Define response packet structure
	const int RSP_SetPoint  = 2;
	const int RSP_DeviceError = 4;
	m_log->Write(2, "GetSetPoint started");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETSETPOINT;
	Cmd_Pkt[CMD_Length]  = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetSetPoint failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetSetPoint failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetTemperature;
	}

	dSetPoint   = (short) Get2Bytes(&Rsp_Pkt[RSP_SetPoint]) / 100.0;
	m_log->Write(2, "GetSetPoint completed OK. Set point %f", dSetPoint);
	return ALL_OK;	
}

//////////////////////////////////////////////////////////////////////////////////////////
// Open or closes the shutter independent of the exposure command
int QSI_Interface::CMD_SetShutter( bool bOpen )
{
	// Define command packet structure
	const int CMD_Command  = 0;
	const int CMD_Length   = 1;
	const int CMD_Open     = 2;

	// Define response packet structure
	const int RSP_DeviceError   = 2;
	m_log->Write(2, "SetShutter started. Shutter Open: %s", bOpen?"T":"F");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}

	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_SETSHUTTER;
	Cmd_Pkt[CMD_Length]  = 1;

	PutBool(&Cmd_Pkt[CMD_Open],bOpen);

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "SetShutter failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "SetShutter failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_IsRelayDone;
	}
	m_log->Write(2, "SetShutter competed OK.");
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Aborts any relay operation
int QSI_Interface::CMD_AbortRelays( void )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_DeviceError = 2;
	m_log->Write(2, "AbortRelays started.");
	
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}

	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_ABORTRELAYS;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "AbortRelays failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "AbortRelays failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_AbortRelays;
	}
	m_log->Write(2, "AbortRelays completed OK");
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_GetLastExposure( double & dExposure )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_ExpTime = 2;
	const int RSP_DeviceError = 5;
	m_log->Write(2, "GetLastExposureTime started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETLASTEXPOSURETIME;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetLastExposureTime failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetLastExposureTime failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetLastExposure;
	}

	dExposure = Get3Bytes(&Rsp_Pkt[RSP_ExpTime]) / 100.0;

	if (Rsp_Pkt[RSP_ExpTime] == 0xff && Rsp_Pkt[RSP_ExpTime + 1] == 0xff && Rsp_Pkt[RSP_ExpTime + 2] == 0xff)
		dExposure = -1;
	m_log->Write(2, "GetLastExposureTime completed. Exp: %f", dExposure);
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_CanAbortExposure( bool & bCanAbort )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_CanAbort		= 2;
	const int RSP_DeviceError	= 3;
	m_log->Write(2, "CanAbortExposure started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_CANABORTEXPOSURE;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "CanAbortExposure failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "CanAbortExposure failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_CanAbortExposure;
	}

	bCanAbort = GetBoolean(Rsp_Pkt[RSP_CanAbort]);
	m_log->Write(2, "CanAbortExposure completed ok. Can abort %s", bCanAbort?"T":"F");
		
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_CanStopExposure( bool & bCanStop )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_CanStop = 2;
	const int RSP_DeviceError = 3;
	m_log->Write(2, "CanStopExposure started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_CANSTOPEXPOSURE;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "CanStopExposure failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2,  "CanStopExposure failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_CanStopExposure;
	}

	bCanStop = GetBoolean(Rsp_Pkt[RSP_CanStop]);
	m_log->Write(2, "CanStopExposure completed ok. Can stop %s", bCanStop?"T":"F");
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_GetFilterPosition( int & iPosition )
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_Position = 2;
	const int RSP_DeviceError = 3;
	m_log->Write(2, "GetFilterPosition started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETFILTERPOSITION;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetFilterPosition failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetFilterPosition failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetFilterPosition;
	}

	iPosition = Rsp_Pkt[RSP_Position];

	m_log->Write(2, "GetFilterPosition completed OK. Pos %x", iPosition);
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_GetCCDSpecs( QSI_CCDSpecs & CCDSpecs)
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_MaxADU = 2;
	const int RSP_EADU = 4;
	const int RSP_EFull = 6;
	const int RSP_MinExp = 8;
	const int RSP_MaxExp = 10;
	const int RSP_DeviceError = 12;
	m_log->Write(2, "GetCCDSpecs started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETCCDSPECS;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetCCDSpecs failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetCCDSpecs failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}

	//
	// minExp scale factor depends on StartExposureEx availability
	//
	double dMinScale;
	if (m_DeviceDetails.HasCMD_StartExposureEx)
		dMinScale = 10000.0; // New start exposure units are in 100us
	else
		dMinScale = 1000.0;  // Old start exposure unit are in 1 millisecond ticks.

	int		iMaxADU		= Get2Bytes(&Rsp_Pkt[RSP_MaxADU]);
	double	dEFullWell	= Get2Bytes(&Rsp_Pkt[RSP_EFull]) * 100.0;
	double	dMinExp		= Get2Bytes(&Rsp_Pkt[RSP_MinExp]) / dMinScale;
	double 	dMaxExp		= Get2Bytes(&Rsp_Pkt[RSP_MaxExp]);
	double	dEPerADUHigh	= Get2Bytes(&Rsp_Pkt[RSP_EADU]) / 1000.0;
	double	dEPerADULow 	= dEPerADUHigh;
	
	// Now adjust gain based on current gain control setting.
	if (m_DeviceDetails.ModelBaseType==_T("503"))
	{
		dEPerADUHigh = 2.6;
		dEPerADULow = 2.6;
	}
	else if (m_DeviceDetails.ModelBaseType==_T("504"))
	{
		dEPerADUHigh = 2.6;
		dEPerADULow = 2.6;
	}
	else if (m_DeviceDetails.ModelBaseType== _T("516"))
	{
		dEPerADUHigh = 2.6;
		dEPerADULow = 2.6;
	}
	else if (m_DeviceDetails.ModelBaseType==_T("520"))
	{
		dEPerADUHigh = 0.8;
		dEPerADULow = 1.9;
	}
	else if (m_DeviceDetails.ModelBaseType==_T("532"))
	{
		dEPerADUHigh = 1.3;
		dEPerADULow = 1.3;
	}
	else if (m_DeviceDetails.ModelBaseType==_T("540"))
	{
		dEPerADUHigh = 0.8;
		dEPerADULow = 1.9;
	}
	else if (m_DeviceDetails.ModelBaseType==_T("583"))
	{
		dEPerADUHigh = 0.5;
		dEPerADULow = 1.1;
	}
	
	if (m_bHighGainOverride)
	{
		dEPerADUHigh = m_dHighGainOverride;
	}

	if (m_bLowGainOverride)
	{
		dEPerADULow = m_dLowGainOverride;
	}

	CCDSpecs.EADUHigh = dEPerADUHigh;
	CCDSpecs.EADULow = dEPerADULow;
	CCDSpecs.EFull = dEFullWell;
	CCDSpecs.MaxADU = iMaxADU;
	CCDSpecs.maxExp = dMaxExp;
	CCDSpecs.minExp = dMinExp;	

	m_log->Write(2, "GetCCDSpecs completed ok. MaxADU: %x E/ADU High: %f E/ADU Low: %f Full: %f Min: %f Max %f",iMaxADU, dEPerADUHigh, dEPerADULow, dEFullWell, dMinExp, dMaxExp);
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_GetAltMode1( unsigned char & ucMode)
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_AltMode1 		 = 2;
	const int RSP_DeviceError    = 3;

	m_log->Write(2, "GetAltMode1 started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}

	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETALTMODE1;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "GetAltMode1 failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetAltMode failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}

	ucMode		= Rsp_Pkt[RSP_AltMode1];

	m_log->Write(2, "GetAltMode1 completed ok. Altmode1: %x ", ucMode);
	return ALL_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////
// 
int QSI_Interface::CMD_SetAltMode1( unsigned char ucMode)
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;
	const int CMD_AltMode1		 = 2;
	// Define response packet structure
	const int RSP_DeviceError    = 2;

	m_log->Write(2, "SetAltMode1 started. Altmode1: %x ", ucMode);

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_SETALTMODE1;
	Cmd_Pkt[CMD_Length] = 1;
	Cmd_Pkt[CMD_AltMode1] = ucMode;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, "SetAltMode1 failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "SetAltMode failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}

	m_log->Write(2, "SetAltMode1 completed ok.");
	return ALL_OK;
}

int QSI_Interface::CMD_GetFeatures(BYTE* pMem, int iFeatureArraySize, int & iCountReturned)
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;

	// Define response packet structure
	const int RSP_Length		= 1;
	const int RSP_FeatureStart 	= 2;

	m_log->Write(2, "GetFeatures started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	iCountReturned = 0;

	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETFEATURES;
	Cmd_Pkt[CMD_Length] = 0;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, IOTimeout_Short);
	
	// 600 firmware prior to 6.1.8 returns the incorrect amount of data (one extra byte).
	// The resulting error is a dirty queue.
	// Purge the extra byte and return without error.
	if (m_iError == ERR_PKT_RxQueueDirty)
	{
		m_iError = m_HostCon.m_HostIO->Purge();
		m_log->Write(2, "GetFeatures - Too much Rx data.  Please upgrade camera firmware to version 6.1.8 or later");
		return ALL_OK;
	}
	
	if( m_iError )
	{
		m_log->Write(2, "GetFeatures failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}
	
	// A packet of 5C 02 00 00 is a "no feature packet".
	// The third byte, if zero. means no feature content.

	// Check that we have some status data and maybe feature data
	// Example Response Packet, %C command, GetFeatures.
	// 00 01 02 03 04 05 06
	// 5C 05 XX XX XX XX SS
	// Above is 4 features with a status of 'SS'

	int iPacketLen = (int)Rsp_Pkt[ RSP_Length ];

	// Check valid packet length
	if ( (iPacketLen < 2) || (iPacketLen == 2 && (Rsp_Pkt[RSP_FeatureStart] == 0xFF ))	)// Must be at least one feature, not FF, if the command is supported
	{
		m_log->Write(2, _T("GetFeature failed. Invalid Feature Count %d. Error Code %x"), iPacketLen, m_iError);
		return ERR_IFC_GetCCDSpecs;
	}	

	// Check for device error
	m_iError = Rsp_Pkt[iPacketLen + RSP_Length];						// Last byte is a status
	if( m_iError )
	{
		m_log->Write(2, _T("GetFeature failed. Bad Status Code.  Error Code %x"), m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}

	iCountReturned = iPacketLen - 1;									// Last byte is a status, so number of features is one less then length.

	// Now copy the features into the returned array
	for (int i = 0; i < iFeatureArraySize; i++)
	{
		pMem[i] = i < iCountReturned? Rsp_Pkt[RSP_FeatureStart+i] : 0; // Feature bytes start on after cmd and status bytes
	}

	m_log->Write(2, _T("GetFeatures completed ok. %d features returned"), iCountReturned);
	return ALL_OK;
}

int QSI_Interface::CMD_GetEEPROM(USHORT usAddress, BYTE & bData)
{
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;
	const int CMD_MSB_Addr	     = 2;
	const int CMD_LSB_Addr       = 3;

	// Define response packet structure
	const int RSP_Data			 = 2;
	const int RSP_DeviceError    = 3;

	m_log->Write(2, "GetEEPROM started.");

	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	
	// Construct command packet
	Cmd_Pkt[CMD_Command] = CMD_GETEEPROM;
	Cmd_Pkt[CMD_Length] = 2;
	Cmd_Pkt[CMD_MSB_Addr] = usAddress >> 8;
	Cmd_Pkt[CMD_LSB_Addr] = usAddress & 0x00ff;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, IOTimeout_Short);
	if( m_iError )
	{
		m_log->Write(2, "GetEEPROM failed. Error Code %x", m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, "GetEEPROM failed. Error Code %x", m_iError);
		return m_iError + ERR_IFC_GetCCDSpecs;
	}

	bData = Rsp_Pkt[RSP_Data];
	m_log->Write(2, "GetEEPROM1 completed ok. Address: %x, Data: %x ", usAddress, bData);
	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Get the hardware (main board) version and firmware version from EEPROM space.
int QSI_Interface::GetVersionInfo(char tszHWVersion[], char tszFWVersion[])
{
	// Camera EEPROM addresses for public data
	
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
	const USHORT EE_MODEL	= 0x0000;
	const USHORT EE_MFGDATE	= 0x0008;
	const USHORT EE_HWSN	= 0x0010;
	const USHORT EE_HWREV	= 0x0018;
	const USHORT EE_FWREV	= 0x0020;
	const USHORT EE_EE_NAME	= 0x0028;
	const USHORT EE_SHUT	= 0x004C;
	const USHORT EE_FIL		= 0x004D;
	const USHORT EE_TCOLS	= 0x0057;
	const USHORT EE_CLOFF	= 0x0059;
	const USHORT EE_COLS	= 0x005B;

	
	BYTE data;
	m_log->Write(2, "GetVersionInfo started.");

	try 
	{
		for (int i = 0; i < 8; i++)
		{
			m_iError = CMD_GetEEPROM( EE_HWREV + i, data);
			if (m_iError != 0) {
				throw QSIException("EEPROM Read Failed",
					QSI_EEPROMREADERROR);
			}
			tszHWVersion[i] = (char) data;
		}

		for (int i = 0; i < 8; i++)
		{
			m_iError = CMD_GetEEPROM( EE_FWREV + i, data);
			if (m_iError != 0) {
				throw QSIException("EEPROM Read Failed",
					QSI_EEPROMREADERROR);
			}
			tszFWVersion[i] = (char) data;
		}
	}
	catch (char* ErrStr)
	{
		m_log->Write(2, ErrStr);
		strncpy(tszHWVersion, "00.00.00", 8);
		strncpy(tszFWVersion, "00.00.00", 8);
	}

	tszHWVersion[8] = 0;
	tszFWVersion[8] = 0;

	m_log->Write(2, _T("GetVersionInfo completed. HW %s FW %s"), tszHWVersion, tszFWVersion);
	return ALL_OK;
}
#pragma GCC diagnostic pop

//////////////////////////////////////////////////////////////////////////////////////////
// GetBoolean converts a 1 byte value to either true or false (0 = false, 1-255 = true)
bool QSI_Interface::GetBoolean(UCHAR ucValue)
{
	if (ucValue != 0) 
		return true;
	else 
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Get2Bytes returns the value of 2 bytes in their reversed order
USHORT QSI_Interface::Get2Bytes(PVOID pvValue)
{
	return *((UCHAR*)pvValue)*256 + *((UCHAR*)pvValue+1);
}
//////////////////////////////////////////////////////////////////////////////////////////
// Get3Bytes returns the value of 3 bytes in their reversed order
UINT QSI_Interface::Get3Bytes(PVOID pvValue)
{
	return *((UCHAR*)pvValue)*256*256 + *((UCHAR*)pvValue+1)*256 + *((UCHAR*)pvValue+2);
}

//////////////////////////////////////////////////////////////////////////////////////////
// GetString takes specified number of bytes from 'Source', copies them to 'Destination',
// and adds proper string termination (\0)
// Destination must be one byte longer than the source to convert to zero terminated string
void QSI_Interface::GetString ( PVOID pvSource, PVOID pvDestination, int iSourceLength )
{
	if (iSourceLength <= 0)
		return;
	// Copy each character
	for (int iCount = 0; iCount < iSourceLength; iCount++)
	*((UCHAR*)pvDestination+iCount) = *((UCHAR*)pvSource+iCount);

	// Add null terminating character
	*((UCHAR*)pvDestination+iSourceLength) = '\0';

	return;
}

std::string QSI_Interface::GetStdString( BYTE * pSrc, int iLen)
{
	std::string out("");
	out.append((char *)pSrc, iLen);
	return out;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Put2Bytes takes the two bytes from 'Value', reverses their order, and puts them in
// 'Destination'
void QSI_Interface::Put2Bytes ( PVOID pvDestination, USHORT usValue )
{
	*((UCHAR*)pvDestination+0) = *((UCHAR*)&usValue+1);
	*((UCHAR*)pvDestination+1) = *((UCHAR*)&usValue+0);
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Put3Bytes does the same as Put2Bytes but with three bytes
void QSI_Interface::Put3Bytes ( PVOID pvDestination, uint32_t ulValue )
{
	*((UCHAR*)pvDestination+0) = *((UCHAR*)&ulValue+2);
	*((UCHAR*)pvDestination+1) = *((UCHAR*)&ulValue+1);
	*((UCHAR*)pvDestination+2) = *((UCHAR*)&ulValue+0);
	return;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PutBool converts a C++ bool (0 or 1) to the type of bool used in the camera (0 or 255)
void QSI_Interface::PutBool(PVOID pvDestination, bool bValue)
{
	if (bValue == true) *((UCHAR*)pvDestination) = 255;
	else *((UCHAR*)pvDestination) = 0;
	return;
}

void QSI_Interface::LogWrite(int iLevel, const char * msg, ... )
{
	std::va_list args;
	va_start(args, msg);
	m_log->Write(iLevel, msg, args);
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Calculate AutoZero Parameters at exposure download time.
void QSI_Interface::GetAutoZeroAdjustment(QSI_AutoZeroData autoZeroData, USHORT * zeroPixels, USHORT * usLastMean, int * usAdjust, double * dAdjust)
{
	int iMedian = 0;
	int iMean = 0;
	int iTotal = 0;

	double dMedian = 0;
	double dMean = 0;
	double dTotal = 0;

	double val = 0;
	int size = 0;
	int iAvg = 0;
	double dAvg = 0;

	*usAdjust = 0;
	*dAdjust = 0;

	if (m_bAutoZeroEnable == false)
	{
		m_log->Write(2, _T("WARNING: AutoZero disabled via user setting."));
		return;
	}

	//
	// First sort the pixels to optionally remove the start/end pixels
	//
	size = autoZeroData.pixelCount;
	qsort(zeroPixels, size, sizeof(USHORT), compareUSHORT);

	//Calc net size of over-scan array to use
	size = size - (m_dwAutoZeroSkipStartPixels + m_dwAutoZeroSkipEndPixels);
	if ( size <= 0)
	{
		m_log->Write(2, _T("WARNING: AutoZero net overscan size less than or equal to zero.  AutoZero skipped."));
		return;
	}

	if (m_dwAutoZeroSkipStartPixels != 0)
	{
		// Throw away start pixels to be skipped
		for (int i = 0; i < size; i++)
		{
			zeroPixels[i] = zeroPixels[i + m_dwAutoZeroSkipStartPixels ];
		}
	}

	// Get the median and mean value

	// Find the Median
	if ( size % 2 == 1)
	{
		//odd number of array elements, so just select the middle element
		iMedian = zeroPixels[(size/2)];
		dMedian = zeroPixels[(size/2)];
	}
	else
	{
		// Even number of elements, so take the mean of the two in the middle
		val = (zeroPixels[(size/2) - 1] + zeroPixels[size/2]) / 2.0;
		dMedian = val;
		//The following two lines work together to round at .5 rather than truncate
		val = val < 0 ? val - 0.5 : val + 0.5;
		iMedian = (int)val;
	}

	// Find the mean
	iTotal = 0;
	dTotal = 0;
	for (int i = 0; i < size; i++)
	{
		iTotal = iTotal + (int)zeroPixels[i];
		dTotal = dTotal + (double)zeroPixels[i];
	}
	iMean = iTotal / size;
	dMean = dTotal / (double)size;

	iAvg = m_bAutoZeroMedianNotMean ? iMedian:iMean;
	dAvg = m_bAutoZeroMedianNotMean ? dMedian:dMean;

	*usLastMean = iAvg;

	// If the camera is saturated, don't try to pull down to the zero target
	if (iAvg > (int)m_dwAutoZeroSatThreshold)
	{
		m_log->Write(2, _T("WARNING: AutoZero median/mean, %d, exceeds saturation threshold."), iAvg);
		m_log->Write(2, _T("         CCD is most likely saturated."));
		m_log->Write(2, _T("         Pixels forced to Max ADU."));

		*usAdjust = 65535;	// Force pixels to saturation
		*dAdjust = 65535;
	}
	else if (iAvg == 0)
	{
		m_log->Write(2, _T("WARNING: AutoZero median/mean, %d, is zero."), iAvg);
		m_log->Write(2, _T("         CCD is most likely is saturated. "));
		m_log->Write(2, _T("         No Autozero adjustment performed (but pixels still limited to Max ADU)."));

		*usAdjust = 0;	// No adjust to pixels if overscan average is 0
		*dAdjust = 0;
	}
	else
	{
		*usAdjust = (int)autoZeroData.zeroLevel - iAvg;	// Normal autozero
		*dAdjust = (double)autoZeroData.zeroLevel - dAvg;
	}

	if (m_log->LoggingEnabled(6))
	{
		m_log->Write(6, _T("AutoZero Data:"));
		snprintf(m_log->m_Message, MSGSIZE, _T("Target: %d, Median: % d, Mean: %d, Adjust By: %d"),
											 (int)autoZeroData.zeroLevel, iMedian, iMean, (int)(*usAdjust) );
		m_log->Write(6);
		m_log->Write(6, _T("AutoZero Float (double) Data:"));
		snprintf(m_log->m_Message, MSGSIZE, _T("Target: %f, Median: % f, Mean: %f, Adjust By: %f"),
											 (double)autoZeroData.zeroLevel, dMedian, dMean, (double)(*usAdjust) );
		m_log->Write(6);
	
		m_log->Write(6, _T("Overscan Pixels values:"));
		int iLines = (size / 16);
		if (size % 16 > 0)
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			for (int j = 0; j < 16; j++)
			{
				snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5u "), (unsigned int)zeroPixels[(i*16)+j]);
			}
			m_log->Write(6);
		}
	}

	return;

}

//////////////////////////////////////////////////////////////////////////////////////////
// AutoZero (drift adjust) the image using the median value of the zero data
int QSI_Interface::AdjustZero(USHORT* pSrc, USHORT* pDst, int iPixelsPerRow, int iRowsLeft, int usAdjust, bool bAdjust)
{
	int pixel;
	int result;
	int iNegPixelCount;
	int iLowPixel;
	int iSatPixelCount;

	m_log->Write(2, _T("AutoZero adjust pixels (unsigned short) started."));

	result = 0;

	if (m_bAutoZeroEnable == false)
	{
		m_log->Write(2, _T("WARNING: AutoZero disabled via user setting."));
		bAdjust = false;
	}

	m_log->Write(6, _T("First row of un-adjusted image data (up to the first 512 bytes):"));

	int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;
	int iLines = (iSampleSize / 16);
	if (iSampleSize % 16 > 0)
		iLines++;

	for (int i = 0; i < iLines; i++)
	{
		for (int j = 0; j < 16 && iSampleSize > 0; j++)
		{
			snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5u "), ((unsigned short*)pSrc)[(i*16)+j]);
			iSampleSize--;
		}
		m_log->Write(6);
	}

	//
	// Now drift adjust the image
	//
	iSatPixelCount = 0;
	iNegPixelCount = 0;
	iLowPixel = 65535;
	//
	// Break up adjustment into rows with pad
	//

	USHORT* psrc = pSrc;
	USHORT* pdst = pDst;
	while (iRowsLeft-- > 0)
	{
		for (int i = 0; i < iPixelsPerRow; i++)
		{
			pixel = *psrc++;
			if (bAdjust) pixel = pixel + (int)usAdjust;
			if (pixel < 0) 
			{
				pixel = 0; 				//result = ERR_IFC_NegAutoZero;
				iNegPixelCount++;
			}
			if (pixel < iLowPixel) 
				iLowPixel = pixel;
			if (pixel > (int)m_dwAutoZeroMaxADU)
			{
				pixel = (int)m_dwAutoZeroMaxADU;
				iSatPixelCount++;
			}
			*pdst++ = (USHORT)pixel;
		}
	}

	if (m_log->LoggingEnabled(6) || (m_log->LoggingEnabled(1) && iNegPixelCount > 0) )
	{
		m_log->Write(6, _T("AutoZero Data:"));
		snprintf(m_log->m_Message, MSGSIZE, _T("NegPixels: %d, Lowest Net Pixel: %d, Pixels Exceeding Sat Threshold : %d"),
											 iNegPixelCount, iLowPixel, iSatPixelCount );
		m_log->Write(6);
	}

	if (m_log->LoggingEnabled(6))
	{
		m_log->Write(6, _T("First row of adjusted image data (up to the first 512 bytes):"));

		int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;

		int iLines = (iSampleSize / 16);
		if (iSampleSize % 16 > 0)
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			for (int j = 0; j < 16 && iSampleSize > 0; j++)
			{
				snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5u "), ((unsigned short*)pDst)[(i*16)+j]);
				iSampleSize--;
			}
			m_log->Write(6);
		}
	}
	m_log->Write(2, _T("AutoZero adjust pixels (unsigned short) complete."));
	return result;
}

int QSI_Interface::AdjustZero(USHORT* pSrc, double * pDst, int iPixelsPerRow, int iRowsLeft, double dAdjust, bool bAdjust)
{
	double pixel;
	int result;
	int iNegPixelCount;
	double dLowPixel;
	int iSatPixelCount;

	m_log->Write(2, _T("AutoZero adjust pixels (double) started."));

	result = 0;

	if (m_bAutoZeroEnable == false)
	{
		m_log->Write(2, _T("WARNING: AutoZero disabled via user setting."));
		bAdjust = false;
	}

	if (m_log->LoggingEnabled(6))
	{
		m_log->Write(6, _T("First row of un-adjusted image data (up to the first 512 bytes):"));

		int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;

		int iLines = (iSampleSize / 16);
		if (iSampleSize % 16 > 0)
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			for (int j = 0; j < 16 && iSampleSize > 0; j++)
			{
				snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5u "), ((unsigned short*)pSrc)[(i*16)+j]);
				iSampleSize--;
			}
			m_log->Write(6);
		}
	}

	//
	// Now drift adjust the image
	//
	iSatPixelCount = 0;
	iNegPixelCount = 0;
	dLowPixel = 65535;
	//
	// Break up adjustment into rows with pad
	//

	USHORT* psrc = pSrc;
	double* pdst = pDst;
	while (iRowsLeft-- > 0)
	{
		for (int i = 0; i < iPixelsPerRow; i++)
		{
			pixel = (double)(*psrc++);
			if (bAdjust) pixel = pixel + dAdjust;
			if (pixel < 0) 
			{
				pixel = 0; //result = ERR_IFC_NegAutoZero;
				iNegPixelCount++;
			}
			if (pixel < dLowPixel) 
				dLowPixel = pixel;
			if (pixel > (double)m_dwAutoZeroMaxADU)
			{
				pixel = (double)m_dwAutoZeroMaxADU;
				iSatPixelCount++;
			}
			*pdst++ = pixel;
		}
	}

	if (m_log->LoggingEnabled(6) || (m_log->LoggingEnabled(1) && iNegPixelCount > 0) )
	{
		m_log->Write(6, _T("AutoZero Data:"));
		snprintf(m_log->m_Message, MSGSIZE, _T("NegPixels: %d, Lowest Net Pixel: %f, Pixels Exceeding Sat Threshold : %d"),
											 iNegPixelCount, dLowPixel, iSatPixelCount );
		m_log->Write(6);
	}

	if (m_log->LoggingEnabled(6))
	{
		m_log->Write(6, _T("First row of adjusted image data (up to the first 512 bytes):"));

		int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;

		int iLines = (iSampleSize / 16);
		if (iSampleSize % 16 > 0)
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			for (int j = 0; j < 16 && iSampleSize > 0; j++)
			{
				snprintf(m_log->m_Message+(j*8), MSGSIZE, _T("%7.2f "), pDst[(i*16)+j]);
				iSampleSize--;
			}
			m_log->Write(6);
		}
	}
	m_log->Write(2, _T("AutoZero adjust pixels (double) complete."));
	return result;
}

int QSI_Interface::AdjustZero(USHORT* pSrc, long* pDst, int iPixelsPerRow, int iRowsLeft, int usAdjust, bool bAdjust)
{
	int pixel;
	int result;
	int iNegPixelCount;
	int iLowPixel;
	int iSatPixelCount;

	m_log->Write(2, _T("AutoZero adjust pixels (unsigned short) started."));

	result = 0;

	if (m_bAutoZeroEnable == false)
	{
		m_log->Write(2, _T("WARNING: AutoZero disabled via user setting."));
		bAdjust = false;
	}

	m_log->Write(6, _T("First row of un-adjusted image data (up to the first 512 bytes):"));

	int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;
	int iLines = (iSampleSize / 16);
	if (iSampleSize % 16 > 0)
		iLines++;

	for (int i = 0; i < iLines; i++)
	{
		for (int j = 0; j < 16 && iSampleSize > 0; j++)
		{
			snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5u "), ((unsigned short*)pSrc)[(i*16)+j]);
			iSampleSize--;
		}
		m_log->Write(6);
	}

	//
	// Now drift adjust the image
	//
	iSatPixelCount = 0;
	iNegPixelCount = 0;
	iLowPixel = 65535;
	//
	// Break up adjustment into rows with pad
	//

	USHORT* psrc = pSrc;
	long* pdst = pDst;
	while (iRowsLeft-- > 0)
	{
		for (int i = 0; i < iPixelsPerRow; i++)
		{
			pixel = *psrc++;
			if (bAdjust) pixel = pixel + (int)usAdjust;
			if (pixel < 0) 
			{
				pixel = 0; 				//result = ERR_IFC_NegAutoZero;
				iNegPixelCount++;
			}
			if (pixel < iLowPixel) 
				iLowPixel = pixel;
			if (pixel > (int)m_dwAutoZeroMaxADU)
			{
				pixel = (int)m_dwAutoZeroMaxADU;
				iSatPixelCount++;
			}
			*pdst++ = (long)pixel;
		}
	}

	if (m_log->LoggingEnabled(6) || (m_log->LoggingEnabled(1) && iNegPixelCount > 0) )
	{
		m_log->Write(6, _T("AutoZero Data:"));
		snprintf(m_log->m_Message, MSGSIZE, _T("NegPixels: %d, Lowest Net Pixel: %d, Pixels Exceeding Sat Threshold : %d"),
											 iNegPixelCount, iLowPixel, iSatPixelCount );
		m_log->Write(6);
	}

	if (m_log->LoggingEnabled(6))
	{
		m_log->Write(6, _T("First row of adjusted image data (up to the first 512 bytes):"));

		int iSampleSize = iPixelsPerRow  > 512 ? 512 : iPixelsPerRow;

		int iLines = (iSampleSize / 16);
		if (iSampleSize % 16 > 0)
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			for (int j = 0; j < 16 && iSampleSize > 0; j++)
			{
				snprintf(m_log->m_Message+(j*6), MSGSIZE, _T("%5ld "), ((long*)pDst)[(i*16)+j]);
				iSampleSize--;
			}
			m_log->Write(6);
		}
	}
	m_log->Write(2, _T("AutoZero adjust pixels (unsigned short) complete."));
	return result;
}

int compareUSHORT( const void *val1, const void *val2)
{
	if (*(USHORT*)val1 == *(USHORT*)val2)
		return 0;
	if (*(USHORT*)val1 < *(USHORT*)val2)
		return -1;
	return 1;
}

int QSI_Interface::QSIRead(unsigned char * Buffer, int BytesToRead, int * BytesReturned)
{
	m_log->Write(2, _T("QSIRead started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	m_iError = m_HostCon.m_HostIO->Read(Buffer, BytesToRead, BytesReturned);
	m_log->Write(2, _T("QSIRead finished. Error Code: %I32X"), m_iError);
	
	return m_iError;
}

int QSI_Interface::QSIWrite(unsigned char * Buffer, int BytesToWrite, int * BytesWritten)
{
	m_log->Write(2, _T("QSIWrite started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	m_iError = m_HostCon.m_HostIO->Write(Buffer, BytesToWrite, BytesWritten);
	m_log->Write(2, _T("QSIWrite finished. Error Code: %I32X"), m_iError);
	
	return m_iError;
}

int QSI_Interface::QSIReadDataAvailable(int * count)
{	
	int temp;
	m_log->Write(2, _T("QSIReadDataAvailable started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	m_iError = m_HostCon.m_HostIO->GetReadWriteQueueStatus(count, &temp);
	m_log->Write(2, _T("QSIReadDataAvailable finished. Error Code: %I32X"), m_iError);

	return m_iError;
}

int QSI_Interface::QSIWriteDataPending(int * count)
{
	int temp;
	m_log->Write(2, _T("QSIWriteDataPending started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	m_iError = m_HostCon.m_HostIO->GetReadWriteQueueStatus(&temp, count);
	m_log->Write(2, _T("QSIWriteDataAvailable finished. Error Code: %I32X"), m_iError);

	return m_iError;
}

int QSI_Interface::QSIReadTimeout(int timeout)
{
	m_log->Write(2, _T("QSIReadTimeout started."));
	m_iError = m_HostCon.m_HostIO->SetStandardReadTimeout((ULONG)timeout);
	m_log->Write(2, _T("QSIReadTimeout finished. Error Code: %I32X"), m_iError);

	return m_iError;
}

int QSI_Interface::QSIWriteTimeout(int timeout)
{
	m_log->Write(2, _T("QSIWriteTimeout started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	m_iError = m_HostCon.m_HostIO->SetStandardWriteTimeout((ULONG)timeout);
	m_log->Write(2, _T("QSIWriteTimeout finished. Error Code: %I32X"), m_iError);

	return m_iError;
}

void QSI_Interface::HotPixelRemap(	BYTE * Image, int RowPad, QSI_ExposureSettings Exposure,
							QSI_DeviceDetails Details, USHORT ZeroPixel)
{
	m_log->Write(2, _T("Hot Pixel Remap started."));
	m_hpmMap.Remap(Image, RowPad, Exposure, Details, ZeroPixel, m_log);
	m_log->Write(2, _T("Hot Pixel Remap complete."));
}

int QSI_Interface::CMD_SetFilterTrim(int pos, bool probe)
{
	m_log->Write(2, _T("SetFilterTrim started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	if (!m_DeviceDetails.HasFilter)
	{
		m_log->Write(2, _T("SetFilterTrim: No filter wheel configured."));
		return ERR_IFC_SetFilterWheel;
	}
	
	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;
	const int CMD_FilterTrim     = 2;

	// Define response packet structure
	const int RSP_DeviceError = 2;
	m_bCameraStateCacheInvalid = true;
	
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_SETFILTERTRIM;
	Cmd_Pkt[CMD_Length]  = 2;

	if (!probe)
	{
		// pos ranges from 0 to n-1, size() ranges from 0 to n.
		if ( pos+1 > (int)m_fwWheel.Filters.size() )
		{
			m_log->Write(2, _T("SetFilterTrim Invalid position : %d"), pos);
			return ERR_IFC_SetFilterWheel;
		}
		m_log->Write(2, _T("SetFilterTrim started. Pos: %I32x, Trim: %d"), pos, m_fwWheel.Filters[pos].Trim);
		Put2Bytes( &Cmd_Pkt[CMD_FilterTrim], m_fwWheel.Filters[pos].Trim );	             
	}
	else
	{
		m_log->Write(2, _T("SetFilterTrim probe started."));
		Put2Bytes( &Cmd_Pkt[CMD_FilterTrim], 0 );
	}

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true, IOTimeout_Short);
	if( m_iError )
	{
		m_log->Write(2, _T("SetFilterTrim failed. Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_SetFilterWheel;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, _T("SetFilterTrim failed. Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_SetFilterWheel;
	}

	m_log->Write(2, _T("SetFilterTrim completed OK."));

	return ALL_OK;
}

bool QSI_Interface::HasFilterWheelTrim(void)
{
	m_bCameraStateCacheInvalid = true;

	m_log->Write(2, _T("HasFilterTrim started."));

	// Set short command timeout to catch the unimplemented case
	int result = CMD_SetFilterTrim(0, true);
	
	if (result != 0)
	{
		m_log->Write(2, _T("HasFilterTrim failed. Error Code %I32x"), result);
	}

	bool bHasTrim = result==0?true:false;

	if( m_log->LoggingEnabled(2))
		m_log->Write(2, _T("HasFilterTrim completed OK."));

	return bHasTrim;
}

int QSI_Interface::CMD_BurstBlock(int Count, BYTE * Buffer, int * Status)
{
	if (Count < 1 || Count > 254)
		return -1;

	// Define command packet structure
	const int CMD_Command        = 0;
	const int CMD_Length         = 1;
	const int CMD_ReturnCount    = 2;

	// Define response packet structure
	const int RSP_BurstData = 2;
	int RSP_DeviceError = 2 + Count;

	m_bCameraStateCacheInvalid = true;
	m_log->Write(2, _T("BurstBlock started. Count: %d"), Count);
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_BURSTBLOCK;
	Cmd_Pkt[CMD_Length]  = 1;
	Cmd_Pkt[CMD_ReturnCount] = (BYTE) Count;

	// Send/receive packets
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, _T("BurstBlock failed. Error Code %I32x"), m_iError);
		return m_iError;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, _T("BurstBlock failed. Error Code %I32x"), m_iError);
		return m_iError;
	}

	// Copy BurstData to Buffer
	*Status = -1;
	for (int i = 0; i < Count; i++)
	{
		Buffer[i] = Rsp_Pkt[RSP_BurstData + i];
		if (Buffer[i] != i && *Status == 0)
			*Status = i;
	}
	m_log->Write(6, _T("BurstBlock Data"));
	m_log->WriteBuffer(6, Buffer, Count, Count, 256);

	m_log->Write(2, _T("BurstBlock completed. Status Code %d."), *Status);
	return  ALL_OK;
}

int QSI_Interface::CMD_HSRExposure( QSI_ExposureSettings ExposureSettings, QSI_AutoZeroData& AutoZeroData)
{

	// Define command packet structure
	const int CMD_Command          = 0;
	const int CMD_Length           = 1;
	const int CMD_Duration         = 2; // 3 byte duration in 10ms increments
	const int CMD_DurationUSec	   = 5; // remaining duration in 100uSec increments
	const int CMD_ColumnOffset     = 6;
	const int CMD_RowOffset        = 8;
	const int CMD_ColumnsToRead    = 10;
	const int CMD_RowsToRead       = 12;
	const int CMD_BinY             = 14;
	const int CMD_BinX             = 16;
	const int CMD_Light            = 18;
	const int CMD_FastReadout      = 19;
	const int CMD_HoldShutterOpen  = 20;
	const int CMD_UseExtTrigger    = 21;
	const int CMD_StrobeShutterOutput = 22;
	const int CMD_ExpRepeatCount   = 23;
	const int CMD_Implemented      = 25;
	const int CMD_END			   = 26;

	// Define response packet structure
	//const int RSP_Command			= 0;
	//const int RSP_Length			= 1;
	const int RSP_AZEnable			= 2;
	const int RSP_AZLevel			= 3;
	const int RSP_AZPixelCount		= 5;
	const int RSP_DeviceError		= 7;
	m_bCameraStateCacheInvalid = true;

	m_log->Write(2, _T("HSRExposure started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	// Construct command packet header
	Cmd_Pkt[CMD_Command] = CMD_HSREXPOSURE;
	Cmd_Pkt[CMD_Length] = CMD_END - 2;	// Don't count the first two bytes

	// Construct command packet body
	Put3Bytes(&Cmd_Pkt[CMD_Duration],       ExposureSettings.Duration );
	Cmd_Pkt[CMD_DurationUSec] =				ExposureSettings.DurationUSec;
	Put2Bytes(&Cmd_Pkt[CMD_ColumnOffset],   ExposureSettings.ColumnOffset);
	Put2Bytes(&Cmd_Pkt[CMD_RowOffset],      ExposureSettings.RowOffset);
	Put2Bytes(&Cmd_Pkt[CMD_ColumnsToRead],  ExposureSettings.ColumnsToRead);
	Put2Bytes(&Cmd_Pkt[CMD_RowsToRead],     ExposureSettings.RowsToRead);
	Put2Bytes(&Cmd_Pkt[CMD_BinY],           ExposureSettings.BinFactorY);
	Put2Bytes(&Cmd_Pkt[CMD_BinX],           ExposureSettings.BinFactorX);
	PutBool(&Cmd_Pkt[CMD_Light],            ExposureSettings.OpenShutter);
	PutBool(&Cmd_Pkt[CMD_FastReadout],      ExposureSettings.FastReadout);
	PutBool(&Cmd_Pkt[CMD_HoldShutterOpen],  ExposureSettings.HoldShutterOpen);
	PutBool(&Cmd_Pkt[CMD_UseExtTrigger],    ExposureSettings.UseExtTrigger);
	PutBool(&Cmd_Pkt[CMD_StrobeShutterOutput], ExposureSettings.StrobeShutterOutput);
	Put2Bytes(&Cmd_Pkt[CMD_ExpRepeatCount],	ExposureSettings.ExpRepeatCount);
	PutBool(&Cmd_Pkt[CMD_Implemented],		ExposureSettings.ProbeForImplemented);

	m_log->Write(2, _T("Duration: %d"),ExposureSettings.Duration);
	m_log->Write(2, _T("DurationUSec: %d"),ExposureSettings.DurationUSec);
	m_log->Write(2, _T("Column Offset: %d"),ExposureSettings.ColumnOffset);
	m_log->Write(2, _T("Row Offset: %d"),ExposureSettings.RowOffset);
	m_log->Write(2, _T("Columns: %d"),ExposureSettings.ColumnsToRead);
	m_log->Write(2, _T("Rows: %d"),ExposureSettings.RowsToRead);
	m_log->Write(2, _T("Bin Y: %d"),ExposureSettings.BinFactorY);
	m_log->Write(2, _T("Bin X: %d"),ExposureSettings.BinFactorX);
	m_log->Write(2, _T("Open Shutter: %d"),ExposureSettings.OpenShutter?1:0);
	m_log->Write(2, _T("Fast Readout: %d"),ExposureSettings.FastReadout?1:0);
	m_log->Write(2, _T("Hold Shutter Open: %d"),ExposureSettings.HoldShutterOpen?1:0);
	m_log->Write(2, _T("Ext Trigger: %d"),ExposureSettings.UseExtTrigger?1:0);
	m_log->Write(2, _T("Strobe Shutter Output: %d"),ExposureSettings.StrobeShutterOutput?1:0);
	m_log->Write(2, _T("Exp Repeat Count: %d"),ExposureSettings.ExpRepeatCount);
	m_log->Write(2, _T("Implemented: %d"),ExposureSettings.ProbeForImplemented?1:0);

	// Send/receive packets
	// Don't check queues, as image data, and autozero data is closely following,
	// And will be transfered by the caller of this routine.
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, false);
	if( m_iError ) 
	{
		m_log->Write(2, _T("HSRExposure failed. Error Code: %I32x"), m_iError);
		return m_iError + ERR_PKT_SetTimeOutFailed;
	}

	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, _T("HSRExposure failed. Error Code: %I32x"), m_iError);
		return m_iError + ERR_IFC_StartExposure;
	}

	AutoZeroData.zeroEnable = Rsp_Pkt[RSP_AZEnable] == 0?false:true;
	AutoZeroData.zeroLevel = Get2Bytes(&Rsp_Pkt[RSP_AZLevel]);
	AutoZeroData.pixelCount = Get2Bytes(&Rsp_Pkt[RSP_AZPixelCount]);

	m_log->Write(2, _T("HSRExposure completed OK"));
	return ALL_OK;
}

int QSI_Interface::CMD_SetHSRMode( bool enable)
{
	// Define command packet structure
	const int CMD_Command	= 0;
	const int CMD_Length	= 1; 
	const int CMD_State		= 2;

	// Define response packet structure
	const int RSP_DeviceError	= 2;

	m_log->Write(2, _T("SetHSRMode started. : %d"), enable?1:0);
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_SETHSRMODE; 
	Cmd_Pkt[CMD_Length]  = 1;

	// Construct transmit packet body
	PutBool( &Cmd_Pkt[CMD_State], enable );
	
	// Send/receive packets 
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, _T("SetHSRMode failed. Error Code %I32x"), m_iError);
		return m_iError;
	}
	  
	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if( m_iError )
	{
		m_log->Write(2, _T("SetHSRMode failed. Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_ActivateRelay; 
	}

	m_log->Write(2, _T("SetHSRMode completed OK"));
	return ALL_OK;
}

int QSI_Interface::CMD_ExtTrigMode( BYTE action, BYTE polarity)
{
	// Define command packet structure
	const int CMD_Command	= 0;
	const int CMD_Length	= 1; 
	const int CMD_Mode		= 2;

	// Define response packet structure
	const int RSP_DeviceError	= 2;

	m_log->Write(2, _T("ExtTrigMode started. : %d, %d"), action, polarity);
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_BASICHWTRIGGER; 
	Cmd_Pkt[CMD_Length]  = 1;

	// Construct transmit packet body
	if (action == TRIG_SHORTWAIT || action == TRIG_LONGWAIT)
	{
		Cmd_Pkt[CMD_Mode] = action | polarity;
	}
	else
	{
		Cmd_Pkt[CMD_Mode] = action;
	}
	
	// Cache the current trigger mode
	if (action == TRIG_DISABLE || action == TRIG_SHORTWAIT || action == TRIG_LONGWAIT)
		m_TriggerMode = action;
	
	// Send/receive packets 
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, _T("ExtTrigMode failed. Error Code %I32x"), m_iError);
		return m_iError;
	}
	  
	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];
	if (m_iError == 0x80)
	{
		m_log->Write(2, _T("Selected trigger mode not supported on this model"));
		return ERR_IFC_NotSupported;
	}
	
	if( m_iError )
	{
		m_log->Write(2, _T("ExtTrigMode failed. Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_TriggerCCDError; 
	}

	m_log->Write(2, _T("ExtTrigMode completed OK"));
	return ALL_OK;
}


BYTE QSI_Interface::EepromRead( USHORT address )
{
	BYTE data;
	return CMD_GetEEPROM(address, data)==0?data:0xFF;
}

int QSI_Interface::CMD_GetShutterState( int & iState )
{
	// Define command packet structure
	const int CMD_Command	= 0;
	const int CMD_Length	= 1; 

	// Define response packet structure
	const int RSP_ShutterState = 2;
	const int RSP_DeviceError	= 3;

	m_log->Write(2, _T("Get Shutter State started."));
	if (m_HostCon.m_HostIO == NULL)
	{
		m_log->Write(2, _T("NULL m_HostIO pointer"));
		return ERR_PKT_NoConnection;
	}
	// Construct transmit packet header
	Cmd_Pkt[CMD_Command] = CMD_GETSHUTTERSTATE; 
	Cmd_Pkt[CMD_Length]  = 0;

	// Send/receive packets 
	m_iError = m_PacketWrapper.PKT_SendPacket(m_HostCon.m_HostIO, Cmd_Pkt, Rsp_Pkt, true);
	if( m_iError )
	{
		m_log->Write(2, _T("Get Shutter State failed. Error Code %I32x"), m_iError);
		return m_iError;
	}
	  
	// Check for device error
	m_iError = Rsp_Pkt[RSP_DeviceError];

	if( m_iError )
	{
		m_log->Write(2, _T("Get Shutter State failed. Error Code %I32x"), m_iError);
		return m_iError + ERR_IFC_GetShutterStateError; 
	}

	iState = Rsp_Pkt[RSP_ShutterState];
	m_log->Write(2, _T("Get Shutter State completed OK, State: %d"), iState);
	return ALL_OK;
}

int QSI_Interface::GetMaxBytesPerReadBlock(void)
{
	return m_MaxBytesPerReadBlock;
}
