/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI 2012 <dchallis@qsimaging.com>
 * 
 */
#pragma once

#include <string>
#include "ICameraEeprom.h"

class QSIModelInfo
{

public:
	QSIModelInfo(ICameraEeprom * eeprom);
	~QSIModelInfo(void);
	std::string GetBaseModelNumber(std::string defaultNumber);		// Marketing model number, sans the features.  Just "604" or "RS0.4".
	std::string GetModelNumber(std::string defaultNumber);			// Full Marketing model number
	std::string GetModelName(std::string defaultName );				// Get the Name of the camera (old EE_NAME field);
private:
	ICameraEeprom * eeprom;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Enums
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	enum AddressesEnum
    {
		  ADDR_Family							= 0x0238,	// ADDR_ prefix, like other below, is because enum names in same scope must be unique in C++.
		  ADDR_Origination						= 0x0237,
		  ADDR_HardwareType						= 0x0236,
		  ADDR_CoolingConfiguration				= 0x0235,
		  ADDR_ChamberWindowConfiguration		= 0x0234,
		  ADDR_ShutterConfiguration				= 0x0233,
		  ADDR_FilterWheelConfiguration			= 0x0232,
		  ADDR_CcdSensorManufacturer			= 0x0231,
		  ADDR_CcdSensorManufacturerPartNumber	= 0x0230,
		  //...
		  ADDR_CcdSensorType					= 0x022F,
		  ADDR_CcdSensorColorMask				= 0x022E,
		  ADDR_CcdSensorMicrolens				= 0x022D,
		  ADDR_CcdSensorBlueEnhanced			= 0x022C,
		  ADDR_CcdSensorCoverGlass				= 0x022B,
		  ADDR_CcdSensorClass					= 0x022A,
		  ADDR_CcdSensorBackThinned				= 0x0229,
    };

    /// <summary>
    /// This field specifies the general 'Family' of the camera.
    /// </summary>
    enum FamilyEnum
    {
		FAM_NotImplemented = 0,
		FAM_NotSpecified = 1,
		FAM_Series500 = 2,
		FAM_Corona = 3,
		FAM_Series600 = 4,
		FAM_ResearchSpec600 = 5,
		FAM_Series700 = 6,
		//...
		FAM_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies how the camera formally declares its origination, 
    /// either QSI for normal retail cameras or otherwise for specific OEM 
    /// customers.
    /// </summary>
    enum OriginationEnum
    {
		ORIG_NotImplemented = 0,
		ORIG_NotSpecified = 1,
		ORIG_QSI = 2,
		ORIG_Veralight = 3,
		//...
		ORIG_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the type of hardware in the camera.
    /// </summary>
    enum HardwareTypeEnum
    {
		HWT_NotImplemented = 0,
		HWT_NotSpecified = 1,
		HWT_Unused2 = 2,
		HWT_Unused3 = 3,
		HWT_Unused4 = 4,
		HWT_Unused5 = 5,
		HWT_Version05 = 5,
		HWT_Version06 = 6,
		HWT_Version07 = 7,
		HWT_Version08 = 8,
		HWT_Version09 = 9,
		HWT_Version10 = 10,
		HWT_Version12 = 12,
		//...
		HWT_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the type of cooling installed in the camera.
    /// </summary>
    enum CoolingConfigurationEnum
    {
		CC_NotImplemented = 0,
		CC_NotSpecified = 1,
		CC_Standard500 = 2,
		CC_Enhanced500 = 3,
		CC_Standard600 = 4,
		CC_VeralightCorona = 5,
		//...
		CC_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the type of chamber window installed in the camera.
    /// </summary>
    enum ChamberWindowConfigurationEnum
    {
		CWC_NotImplemented = 0,
		CWC_NotSpecified = 1,
		CWC_EO_B270_32951_25mmDia_MgF2 = 2,
		CWC_EO_B270_45254_30mmDia_MgF2 = 3,
		CWC_EO_1WaveFS_45812_25mmDia_UV_AR = 4,
		CWC_EO_1WaveFS_84475_25mmDia_UV_VIS = 5,
		CWC_EO_1WaveFS_49643_25mmDia_NIR_VIS = 6,
		CWC_EO_1WaveFS_45813_30mmDia_UV_AR = 7,
		CWC_EO_1WaveFS_84477_30mmDia_UV_VIS = 8,
		CWC_EO_1WaveFS_49645_30mmDia_NIR_VIS = 9,
		//...
		CWC_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the kind of shutter installed in the camera.
    /// </summary>
    enum ShutterConfigurationEnum
    {
		SC_NotImplemented = 0,
		SC_NotSpecified = 1,
		SC_NoShutter = 2,
		SC_QsiStandardShutter = 3,
		SC_QsiCoronaShutter = 4,
		//...
		SC_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the kind of filter assembly installed in the 
    /// camera.
    /// </summary>
    enum FilterWheelConfigurationEnum
    {
		FWC_NotImplemented = 0,
		FWC_NotSpecified = 1,
		FWC_NoFilterWheel = 2,
		FWC_QsiStandardFilterWheel = 3,
		//...
		FWC_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the manufacturer of the CCD imager installed in the
    /// camera.
    /// </summary>
    enum CcdSensorManufacturerEnum
    {
		MAN_NotImplemented = 0,
		MAN_NotSpecified = 1,
		MAN_Truesense = 2,
		MAN_Sony = 3,
		//...
		MAN_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the exact manufacturers part number. Presumably, 
    /// this part number will explicitly specify every aspect of the CCD imager.
    /// At this point, having only dealt with Kodak CCDs, it isn't clear if this
    /// field will provide more information than the Secondary CCD Image Sensor
    /// Description feature bytes describe further down in this document. In any
    /// case, this is a future issue to be dealt with. This information is 
    /// included in cameras manufactured from here on for maximum flexibility in
    /// the future.
    /// 
    /// FOR MANUFACTURING USE ONLY
    /// </summary>
    enum CcdSensorManufacturerPartNumberEnum
    {
		PN_NotImplemented = 0,
		PN_NotSpecified = 1,
		PN_KAF_0402_ABA_CD_B1 = 10,
		PN_KAF_0402_ABA_CD_B2 = 11,
		PN_KAF_0402_ABA_CP_B1 = 12,
		PN_KAF_0402_ABA_CP_B2 = 13,
		PN_KAF_1603_ABA_CD_B2 = 20,
		PN_KAF_3200_ABA_CD_B2 = 30,
		PN_KAF_3200_ABA_CP_B2 = 31,
		PN_KAF_8300_AXC_CD_AA = 40,
		PN_KAF_8300_CXB_CB_AA = 41,
		PN_KAI_2020_ABA_CD_BA = 50,
		PN_KAI_2020_ABA_CR_BA = 51,
		PN_KAI_2020_CBA_CD_BA = 52,
		PN_KAI_04022_ABA_CD_BA = 60,
		PN_KAI_04022_ABA_CR_BA = 61,
		PN_KAI_04022_CBA_CD_BA = 62,
		PN_ICX674ALG = 70,
		PN_ICX674AQG = 71,
		PN_ICX694ALG = 72,
		PN_ICX694AQG = 73,
		PN_ICX814ALG = 74,
		PN_ICX814AQG = 75,
		//...
		PN_SeeExtensionTable = 254,
		PN_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the shorthand CCD sensor part name.
    /// </summary>
    enum CcdSensorTypeEnum
    {
		CST_NotImplemented = 0,
		CST_NotSpecified = 1,
		CST_KAF_0402 = 2,
		CST_KAF_1603 = 3,
		CST_KAF_3200 = 4,
		CST_KAF_8300 = 5,
		CST_KAI_2020 = 6,
		CST_KAI_04022 = 7,
		CST_ICX674 = 8,
		CST_ICX694 = 9,
		CST_ICX814 = 10,
		//...
		CST_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies if the CCD imager has a color mask.
    /// </summary>
    enum CcdSensorColorMaskEnum
    {
		CSC_NotImplemented = 0,
		CSC_NotSpecified = 1,
		CSC_None = 2,
		CSC_BayerNormal = 3,
		CSC_TruesenseNormal = 4,
		//...
		CSC_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies if the CCD imager has microlens.
    /// </summary>
    enum CcdSensorMicrolensEnum
    {
		CSM_NotImplemented = 0,
		CSM_NotSpecified = 1,
		CSM_None = 2,
		CSM_Yes = 3,
		//...
		CSM_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the whether the CCD imager is Blue Enhanced.
    /// </summary>
    enum CcdSensorBlueEnhancedEnum
    {
		CSB_NotImplemented = 0,
		CSB_NotSpecified = 1,
		CSB_No = 2,
		CSB_Yes = 3,
		//...
		CSB_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the cover glass on the CCD imager.
    /// </summary>
    enum CcdSensorCoverGlassEnum
    {
		CSCG_NotImplemented = 0,
		CSCG_NotSpecified = 1,
		CSCG_AntiReflectiveGlass = 2,
		CSCG_ClearGlass = 3,
		CSCG_NoGlass = 4,
		//...
		CSCG_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies the cover glass on the CCD imager.
    /// </summary>
    enum CcdSensorClassEnum
    {
		CS_NotImplemented = 0,
		CS_NotSpecified = 1,
		CS_NotApplicable = 2,
		CS_TruesenseClass1 = 3,
		CS_TruesenseClass2 = 4,
		CS_TruesenseEngineering = 5,
		//...
		CS_NotInitialized = 255
    };

    /// <summary>
    /// This field specifies that the CCD imager is back thinned.
    /// </summary>
    enum CcdSensorBackThinnedEnum
    {
		CBT_NotImplemented = 0,
		CBT_NotSpecified = 1,
		CBT_NotApplicable = 2,
		//...
		CBT_NotInitialized = 255
    };

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Properties
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
    /// <summary>
    /// Gets the feature.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="address">The address.</param>
    /// <returns></returns>
    template <typename T> T GetFeature( AddressesEnum address )
    {
		return (T) eeprom->EepromRead( (USHORT) address );
    };

    /// <summary>
    /// Gets or sets the family.
    /// </summary>
    /// <value>
    /// The family.
    /// </value>
    FamilyEnum Family(void);

    /// <summary>
    /// Gets or sets the origination.
    /// </summary>
    /// <value>
    /// The origination.
    /// </value>
    OriginationEnum Origination(void);

    /// <summary>
    /// Gets or sets the type of the hardware.
    /// </summary>
    /// <value>
    /// The type of the hardware.
    /// </value>
    HardwareTypeEnum HardwareType(void);

    /// <summary>
    /// Gets or sets the cooling configuration.
    /// </summary>
    /// <value>
    /// The cooling configuration.
    /// </value>
    CoolingConfigurationEnum CoolingConfiguration(void);

    /// <summary>
    /// Gets or sets the chamber window configuration.
    /// </summary>
    /// <value>
    /// The chamber window configuration.
    /// </value>
    ChamberWindowConfigurationEnum ChamberWindowConfiguration(void);

    /// <summary>
    /// Gets or sets the shutter configuration.
    /// </summary>
    /// <value>
    /// The shutter configuration.
    /// </value>
    ShutterConfigurationEnum ShutterConfiguration(void);

    /// <summary>
    /// Gets or sets the filter wheel configuration.
    /// </summary>
    /// <value>
    /// The filter wheel configuration.
    /// </value>
    FilterWheelConfigurationEnum FilterWheelConfiguration(void);

    /// <summary>
    /// Gets or sets the CCD sensor manufacturer.
    /// </summary>
    /// <value>
    /// The CCD sensor manufacturer.
    /// </value>
    CcdSensorManufacturerEnum CcdSensorManufacturer(void);

    /// <summary>
    /// Gets or sets the CCD sensor manufacturer part number.
    /// </summary>
    /// <value>
    /// The CCD sensor manufacturer part number.
    /// </value>
    CcdSensorManufacturerPartNumberEnum CcdSensorManufacturerPartNumber(void);

    /// <summary>
    /// Gets or sets the type of the CCD sensor.
    /// </summary>
    /// <value>
    /// The type of the CCD sensor.
    /// </value>
    CcdSensorTypeEnum CcdSensorType(void);

    /// <summary>
    /// Gets or sets the CCD sensor color mask.
    /// </summary>
    /// <value>
    /// The CCD sensor color mask.
    /// </value>
    CcdSensorColorMaskEnum CcdSensorColorMask(void);

    /// <summary>
    /// Gets or sets the CCD sensor microlens.
    /// </summary>
    /// <value>
    /// The CCD sensor microlens.
    /// </value>
    CcdSensorMicrolensEnum CcdSensorMicrolens(void);

    /// <summary>
    /// Gets or sets the CCD sensor blue enhanced.
    /// </summary>
    /// <value>
    /// The CCD sensor blue enhanced.
    /// </value>
    CcdSensorBlueEnhancedEnum CcdSensorBlueEnhanced(void);

    /// <summary>
    /// Gets or sets the CCD sensor cover glass.
    /// </summary>
    /// <value>
    /// The CCD sensor cover glass.
    /// </value>
    CcdSensorCoverGlassEnum CcdSensorCoverGlass(void);

    /// <summary>
    /// Gets or sets the CCD sensor class.
    /// </summary>
    /// <value>
    /// The CCD sensor class.
    /// </value>
    CcdSensorClassEnum CcdSensorClass(void);

    /// <summary>
    /// Gets or sets the CCD sensor back thinned.
    /// </summary>
    /// <value>
    /// The CCD sensor back thinned.
    /// </value>
    CcdSensorBackThinnedEnum CcdSensorBackThinned(void);

};


