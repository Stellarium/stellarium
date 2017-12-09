//*****************************************************************************************
//NAME
// Registry / SimpleIni Wrapper Class
//
//DESCRIPTION
// Contains class prototype and definition for a registry wrapper/helper class to allow easy
// access to the registry ro SimpleIni for setting and retrieving the advanced settings.
//
//COPYRIGHT (C)
// QSI (Quantum Scientific Imaging) 2005-2006
//
//REVISION HISTORY
// MWB 04.16.06 Original Version
// MWB 04.27.06 Completely rewritten; now handles all registry aspects and stores/retrieves
//              settings for multiple cameras
// DRC 01.01.07 Added SimpleIni support for linux api
//*****************************************************************************************/

#pragma once

#include "QSI_Global.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

#define SI_CONVERT_GENERIC
#include "SimpleIni.h"

#define KEY_QSI std::string("SOFTWARE/QSI/")
#define KEY_Base std::string("SOFTWARE/QSI/API/")
#define KEY_GuiderCamera std::string("SOFTWARE/QSI/API/Guider/")
#define KEY_MainCamera std::string("SOFTWARE/QSI/API/Main/")
#define KEY_Filter std::string("Filter/")

#define SUBKEY_IPAddress std::string("IPAddress")

#define SUBKEY_SelectedMain std::string("SelectedMainCamera")
#define SUBKEY_SelectedGuider std::string("SelectedGuiderCamera")
#define SUBKEY_SelectedMainFW std::string("SelectedMainFilterWheel")
#define SUBKEY_SelectedGuiderFW std::string("SelectedGuiderFilterWheel")

#define SUBKEY_SelectedMainInterface std::string("SelectedMainInterface")
#define SUBKEY_SelectedGuiderInterface std::string("SelectedGuiderInterface")
#define SUBKEY_MainIPv4Address std::string("MainIPv4Address")
#define SUBKEY_GuiderIPv4Address std::string("GuiderIPv4Address")

#define SUBKEY_FanModeIndex std::string("FanModeIndex")
#define SUBKEY_CameraGainIndex std::string("CameraGainIndex")
#define SUBKEY_ShutterPriorityIndex std::string("ShutterPriorityIndex")
#define SUBKEY_AntiBloomingIndex std::string("AntiBloomingIndex")
#define SUBKEY_PreExposureFlushIndex std::string("PreExposureFlushIndex")
#define SUBKEY_LEDIndicatorOnCheck std::string("LEDIndicatorOn")
#define SUBKEY_SoundOnCheck std::string("SoundOn")
#define SUBKEY_ShowDLProgressCheck std::string("ShowDLProgress")
#define SUBKEY_OptimizeReadoutSpeedCheck std::string("OptimizeReadoutSpeed")
#define SUBKEY_NumFilters std::string("NumFilters")
#define SUBKEY_FilterName std::string("FilterName")
#define SUBKEY_FilterFocus std::string("FilterFocusOffset")
#define SUBKEY_FilterTrim std::string("FilterTrim")

class QSI_Registry
{
public:
	////////////////////////////////////////////////////////////////////////////////////////
	QSI_Registry( void )
	{
		char *pTmp = NULL;
		uid_t me;
		struct passwd *my_passwd;
		me = getuid();
		my_passwd = getpwuid(me);
		pTmp = my_passwd->pw_dir;

		if (pTmp == NULL)
		{
			strncpy(m_szPath, "/tmp/.QSIConfig", MAX_PATH);
		}
		else
		{
			strncpy(m_szPath, pTmp, MAX_PATH);
			strncat(m_szPath, "/.QSIConfig", MAX_PATH);
		}

		m_rc = SI_OK;

		m_iError = 0;
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	~QSI_Registry( void )
	{
		return;
	}
	
	void AddIPAddress(std::string IPAddr)
	{
		m_ini.SetValue( (std::string(KEY_QSI) + SUBKEY_IPAddress).c_str(), IPAddr.c_str(), NULL, NULL );
	}

	void GetIPAdressList(std::vector<std::string> * vIPAddresses)
	{
		GetAllKeys(std::string(KEY_QSI) + SUBKEY_IPAddress, vIPAddresses);
		return;
	}

	void SetSelectedInterface( int iInterface, bool bIsMainCamera)
	{
		if( bIsMainCamera )
		  SetNumber( KEY_Base, SUBKEY_SelectedMainInterface, iInterface );
		else
		  SetNumber( KEY_Base, SUBKEY_SelectedGuiderInterface, iInterface );
	}

	int GetSelectedInterface(bool bIsMainCamera, int iDefaultInterface)
	{
		if( bIsMainCamera )
			return GetNumber( KEY_Base, SUBKEY_SelectedMainInterface, iDefaultInterface);
		else
			return GetNumber( KEY_Base, SUBKEY_SelectedGuiderInterface, iDefaultInterface);
	}

	void SetIPv4Address(int iIPv4Addr, bool bIsMainCamera)
	{
		if( bIsMainCamera )
		  SetNumber( KEY_Base, SUBKEY_MainIPv4Address, iIPv4Addr );
		else
		  SetNumber( KEY_Base, SUBKEY_GuiderIPv4Address, iIPv4Addr );
	}

	int GetIPv4Addresss(bool bIsMainCamera, int iDefaultAddress)
	{
		if( bIsMainCamera )
			return GetNumber( KEY_Base, SUBKEY_MainIPv4Address, iDefaultAddress);
		else
			return GetNumber( KEY_Base, SUBKEY_GuiderIPv4Address, iDefaultAddress);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	bool RecordExists( std::string strSerialNumber, bool bIsMainCamera )
	{
		if( bIsMainCamera )
		  return KeyExists( KEY_MainCamera + strSerialNumber );
		else
		  return KeyExists( KEY_GuiderCamera + strSerialNumber );
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void SetSelectedFilterWheel( std::string strSerialNumber, bool bIsMainCamera, std::string strFilterName )
	{
		if( bIsMainCamera )
		  SetString( KEY_QSI + strSerialNumber, SUBKEY_SelectedMainFW, strFilterName );
		else
		  SetString( KEY_QSI + strSerialNumber, SUBKEY_SelectedGuiderFW, strFilterName );

		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	std::string GetSelectedFilterWheel( std::string strSerialNumber, bool bIsMainCamera )
	{
		if( bIsMainCamera )
		  return GetString( KEY_QSI + strSerialNumber, SUBKEY_SelectedMainFW, std::string("Default") );
		else
		  return GetString( KEY_QSI + strSerialNumber, SUBKEY_SelectedGuiderFW, std::string("Default") );
	}


	////////////////////////////////////////////////////////////////////////////////////////
	void SetSelectedCamera( std::string strSerialNumber, bool bIsMainCamera )
	{
		if( bIsMainCamera )
		  SetString( KEY_Base, SUBKEY_SelectedMain, strSerialNumber );
		else
		  SetString( KEY_Base, SUBKEY_SelectedGuider, strSerialNumber );

		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	std::string GetSelectedCamera( bool bIsMainCamera )
	{
		if( bIsMainCamera )
		  return GetString( KEY_Base, SUBKEY_SelectedMain, std::string("") );
		else
		  return GetString( KEY_Base, SUBKEY_SelectedGuider, std::string("") );
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void SetAdvancedSetupSettings( std::string strSerialNumber, bool bIsMainCamera, QSI_AdvSettings AdvSettings )
	{
		std::string KeyPath = "/";
		std::string FilterPath = "/";
		std::string FilterNum = "0";

		if( bIsMainCamera )
		  KeyPath = KEY_MainCamera + strSerialNumber + "/";
		else
		  KeyPath = KEY_GuiderCamera + strSerialNumber + "/";

		SetBoolean( KeyPath, SUBKEY_LEDIndicatorOnCheck, AdvSettings.LEDIndicatorOn );
		SetBoolean( KeyPath, SUBKEY_SoundOnCheck, AdvSettings.SoundOn );
		SetBoolean( KeyPath, SUBKEY_ShowDLProgressCheck, AdvSettings.ShowDLProgress );
		SetBoolean( KeyPath, SUBKEY_OptimizeReadoutSpeedCheck, AdvSettings.OptimizeReadoutSpeed );
		SetNumber( KeyPath, SUBKEY_FanModeIndex, AdvSettings.FanModeIndex );
		SetNumber( KeyPath, SUBKEY_CameraGainIndex, AdvSettings.CameraGainIndex );
		SetNumber( KeyPath, SUBKEY_ShutterPriorityIndex, AdvSettings.ShutterPriorityIndex );
		SetNumber( KeyPath, SUBKEY_AntiBloomingIndex, AdvSettings.AntiBloomingIndex );
		SetNumber( KeyPath, SUBKEY_PreExposureFlushIndex, AdvSettings.PreExposureFlushIndex );
		// Filter Wheel settings
		AdvSettings.fwWheel.SaveToRegistry(strSerialNumber);

		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Attempts to get the advanced settings from registry and goes to defaults on any setting that can't be retrieved from registry
	QSI_AdvSettings GetAdvancedSetupSettings( std::string strSerialNumber, bool bIsMainCamera, QSI_AdvSettings DefaultSettings )
	{
		QSI_AdvSettings AdvSettings;
		std::string KeyPath = "/";
		std::string FilterPath = "/";
		std::string FilterNum = "0";
		std::string strValue = "0";

		if( bIsMainCamera )
		  KeyPath = KEY_MainCamera + strSerialNumber + "/";
		else
		  KeyPath = KEY_GuiderCamera + strSerialNumber + "/";

		AdvSettings.LEDIndicatorOn       = GetBoolean( KeyPath, SUBKEY_LEDIndicatorOnCheck      , DefaultSettings.LEDIndicatorOn );
		AdvSettings.SoundOn              = GetBoolean( KeyPath, SUBKEY_SoundOnCheck             , DefaultSettings.SoundOn );
		AdvSettings.ShowDLProgress       = GetBoolean( KeyPath, SUBKEY_ShowDLProgressCheck      , DefaultSettings.ShowDLProgress );
		AdvSettings.OptimizeReadoutSpeed = GetBoolean( KeyPath, SUBKEY_OptimizeReadoutSpeedCheck, DefaultSettings.OptimizeReadoutSpeed );
		AdvSettings.FanModeIndex         = GetNumber( KeyPath, SUBKEY_FanModeIndex              , DefaultSettings.FanModeIndex );
		AdvSettings.CameraGainIndex      = GetNumber( KeyPath, SUBKEY_CameraGainIndex           , DefaultSettings.CameraGainIndex );
		AdvSettings.ShutterPriorityIndex = GetNumber( KeyPath, SUBKEY_ShutterPriorityIndex      , DefaultSettings.ShutterPriorityIndex );
		AdvSettings.AntiBloomingIndex    = GetNumber( KeyPath, SUBKEY_AntiBloomingIndex         , DefaultSettings.AntiBloomingIndex );
		AdvSettings.PreExposureFlushIndex = GetNumber( KeyPath, SUBKEY_PreExposureFlushIndex    , DefaultSettings.PreExposureFlushIndex );
		// Filter Wheel settings
		std::string strWheelName = GetSelectedFilterWheel( strSerialNumber, bIsMainCamera);
		FilterWheel CurrentWheel;
		if (CurrentWheel.LoadFromRegistry(strSerialNumber, strWheelName, DefaultSettings.fwWheel.m_iNumFilters))
			AdvSettings.fwWheel = CurrentWheel;
		else
		{
			// Remove missing filter wheel setting
			SetSelectedFilterWheel( strSerialNumber, bIsMainCamera, DefaultSettings.fwWheel.Name);
			AdvSettings.fwWheel = DefaultSettings.fwWheel;
		}

		return AdvSettings;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetMaxPixelsPerBlock( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "MaxPixelsPerBlock", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBInSize( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBInSize", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBOutSize( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBOutSize", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBReadTimeout( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBReadTimeout", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBWriteTimeout( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBWriteTimeout", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBExReadTimeout( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBExtendedReadTimeout", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetUSBExWriteTimeout( int iDefaultValue )
	{
		return GetNumber(KEY_Base, "USBExtendedWriteTimeout", iDefaultValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////
	bool KeyExists( std::string strKeyPath )
	{
		// m_iError = RegOpenKeyEx( HKEY_CURRENT_USER, strKeyPath, 0, KEY_READ, &hKey );
		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0 ) return false;
		return m_ini.GetSection(strKeyPath.c_str());
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void SetBoolean( std::string strKeyPath, std::string strSubKeyName, bool bValue )
	{
		if( bValue == true )
		  SetNumber( strKeyPath, strSubKeyName, 1 );
		else
		  SetNumber( strKeyPath, strSubKeyName, 0 );
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void SetString( std::string strKeyPath, std::string strSubKeyName, std::string strValue )
	{
		m_rc = m_ini.LoadFile(m_szPath);
		m_rc = m_ini.SetValue(strKeyPath.c_str(), strSubKeyName.c_str(), strValue.c_str(), NULL);
		m_rc = m_ini.SaveFile(m_szPath);

		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	void SetNumber( std::string strKeyPath, std::string strSubKeyName, int iValue )
	{
		char buff[256];
		m_rc = m_ini.LoadFile(m_szPath);
		sprintf(buff, "%d", iValue);
		m_rc = m_ini.SetValue(strKeyPath.c_str(), strSubKeyName.c_str(), buff, NULL);
		m_rc = m_ini.SaveFile(m_szPath, false);

		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	int GetNumber( std::string strKeyPath, std::string strSubKeyName, int iDefaultValue )
	{
		int iValue = iDefaultValue;
		GetInt(strKeyPath, strSubKeyName, iDefaultValue, iValue);

		return iValue;
	}

	bool GetInt (std::string strKeyPath, std::string strSubKeyName, int iDefaultValue, int & iRetVal)
	{
		// Declare variables
		int iValue = iDefaultValue;

		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
		{
			iRetVal = iDefaultValue;
			return false;
		}

		const char * pszValue = m_ini.GetValue(strKeyPath.c_str(), strSubKeyName.c_str(), NULL, NULL);
		if (pszValue == NULL)
		{
			iRetVal = iDefaultValue;
			return false;
		}
		sscanf(pszValue, "%d", &iValue);
		iRetVal = iValue;

		return true;
	}


	int RegQueryValueEx(std::string strKeyPath, std::string strSubKeyName, int n0, int n1, int * piData, int dSize)
	{
		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
			return -1;
		const char * pszValue = m_ini.GetValue(strKeyPath.c_str(), strSubKeyName.c_str(), NULL, NULL);
		if (pszValue == NULL)
			return -2;
		sscanf(pszValue, "%d", piData);
		return 0;
	}

	int RegDelnode (std::string Root)
	{
		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
			return -1;
		m_ini.Delete( Root.c_str(), NULL, true );
		m_rc = m_ini.SaveFile(m_szPath, false);
		return 0;
	}

	int RegDelKey(std::string Section, std::string Key)
	{
		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
			return -1;
		m_ini.Delete( Section.c_str(), Key.c_str(), false/*don't remove partent if empty*/ );
		m_rc = m_ini.SaveFile(m_szPath, false);
		return 0;
	}

	int GetAllKeys(std::string strKeyPath, std::vector<std::string> * vKeys)
	{
		CSimpleIniCaseA::TNamesDepend Names;

		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
			return -1;

		m_ini.GetAllKeys(strKeyPath.c_str(), Names);

		vKeys->clear();

		CSimpleIniCaseA::TNamesDepend::iterator i = Names.begin();
        for (; i != Names.end(); ++i) 
		{
            vKeys->push_back(std::string( const_cast<const char *>(i->pItem)));
        }
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	std::string GetString( std::string strKeyPath, std::string strSubKeyName, std::string strDefaultValue )
	{
		m_rc = m_ini.LoadFile(m_szPath);
		if (m_rc < 0)
			return strDefaultValue;
		const char * pszValue = m_ini.GetValue(strKeyPath.c_str(), strSubKeyName.c_str(), NULL, NULL);
		if (pszValue == NULL)
			return strDefaultValue;
		std::string strValue(pszValue);
		return strValue;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	bool GetBoolean( std::string strKeyPath, std::string strSubKeyName, bool bDefaultValue )
	{
		int iValue = GetNumber( strKeyPath, strSubKeyName, bDefaultValue );

		if( iValue == 0 )
		  return false;
		else
		  return true;
	}
	
private:
	int m_iError;   		// Holds last error... declared here so it doesn't have to be declared each function
	CSimpleIniCaseA m_ini;	// config file class
	SI_Error m_rc;
	char m_szPath[MAX_PATH+1];

};
