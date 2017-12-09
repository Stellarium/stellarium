/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI 2012 <dchallis@qsimaging.com>
 * 
 */

#include "QSIModelInfo.h"

QSIModelInfo::QSIModelInfo(ICameraEeprom * eeprom)
{
	this->eeprom = eeprom;
}

QSIModelInfo::~QSIModelInfo(void)
{
}

QSIModelInfo::FamilyEnum QSIModelInfo::Family()
{
    return GetFeature<FamilyEnum>( ADDR_Family );
}

QSIModelInfo::OriginationEnum QSIModelInfo::Origination()
{
    return GetFeature<OriginationEnum>( ADDR_Origination );
}

QSIModelInfo::HardwareTypeEnum QSIModelInfo::HardwareType()
{
    return GetFeature<HardwareTypeEnum>( ADDR_HardwareType ); 
}

QSIModelInfo::CoolingConfigurationEnum QSIModelInfo::CoolingConfiguration()
{
    return GetFeature<CoolingConfigurationEnum>( ADDR_CoolingConfiguration );
}

QSIModelInfo::ChamberWindowConfigurationEnum QSIModelInfo::ChamberWindowConfiguration()
{
    return GetFeature<ChamberWindowConfigurationEnum>( ADDR_ChamberWindowConfiguration );
}

QSIModelInfo::ShutterConfigurationEnum QSIModelInfo::ShutterConfiguration()
{
    return GetFeature<ShutterConfigurationEnum>( ADDR_ShutterConfiguration );
}

QSIModelInfo::FilterWheelConfigurationEnum QSIModelInfo::FilterWheelConfiguration()
{
    return GetFeature<FilterWheelConfigurationEnum>( ADDR_FilterWheelConfiguration );
}

QSIModelInfo::CcdSensorManufacturerEnum QSIModelInfo::CcdSensorManufacturer()
{
    return GetFeature<CcdSensorManufacturerEnum>( ADDR_CcdSensorManufacturer );
}

QSIModelInfo::CcdSensorManufacturerPartNumberEnum QSIModelInfo::CcdSensorManufacturerPartNumber()
{
    return GetFeature<CcdSensorManufacturerPartNumberEnum>( ADDR_CcdSensorManufacturerPartNumber );
}

QSIModelInfo::CcdSensorTypeEnum QSIModelInfo::CcdSensorType()
{
    return GetFeature<CcdSensorTypeEnum>( ADDR_CcdSensorType );
}
QSIModelInfo::CcdSensorColorMaskEnum QSIModelInfo::CcdSensorColorMask()
{
    return GetFeature<CcdSensorColorMaskEnum>( ADDR_CcdSensorColorMask );
}

QSIModelInfo::CcdSensorMicrolensEnum QSIModelInfo::CcdSensorMicrolens()
{
    return GetFeature<CcdSensorMicrolensEnum>( ADDR_CcdSensorMicrolens );
}

QSIModelInfo::CcdSensorBlueEnhancedEnum QSIModelInfo::CcdSensorBlueEnhanced()
{
    return GetFeature<CcdSensorBlueEnhancedEnum>( ADDR_CcdSensorBlueEnhanced );
}

QSIModelInfo::CcdSensorCoverGlassEnum QSIModelInfo::CcdSensorCoverGlass()
{
    return GetFeature<CcdSensorCoverGlassEnum>( ADDR_CcdSensorCoverGlass );
}

QSIModelInfo::CcdSensorClassEnum QSIModelInfo::CcdSensorClass()
{
    return GetFeature<CcdSensorClassEnum>( ADDR_CcdSensorClass );
}

QSIModelInfo::CcdSensorBackThinnedEnum QSIModelInfo::CcdSensorBackThinned()
{
    return GetFeature<CcdSensorBackThinnedEnum>( ADDR_CcdSensorBackThinned );
}

std::string QSIModelInfo::GetModelName(std::string defaultName )
{
	// if there are no model feature bytes, just return the old string, else constuct a new model name string
	std::string modelName = GetBaseModelNumber("");
	if (modelName == "")
		return defaultName;
	return "QSI " + modelName + " Series Camera";
}

std::string QSIModelInfo::GetBaseModelNumber(std::string defaultNumber)	// Marketing model number, sans the features.  Just "604" or "RS0.4".
{
	std::string sb;
    bool researchSpec = false;

    switch( Family() )
    {
		case FAM_Series500:
			sb.append( "5" );
			break;
		case FAM_Series600:
			sb.append( "6" );
			break;
		case FAM_ResearchSpec600:
			sb.append( "RS" );
			researchSpec = true;
			break;
		default:
			return defaultNumber;
    }

    switch( CcdSensorType() )
    {
		case CST_KAF_0402:
			sb.append( researchSpec ? "0.4" : "04" );
			break;
		case CST_KAF_1603:
			sb.append( researchSpec ? "1.6" : "16" );
			break;
		case CST_KAF_3200:
			sb.append( researchSpec ? "3.2" : "32" );
			break;
		case CST_KAF_8300:
			sb.append( researchSpec ? "8.3" : "83" );
			break;
		case CST_KAI_2020:
			sb.append( researchSpec ? "2.0" : "20" );
			break;
		case CST_KAI_04022:
			sb.append( researchSpec ? "4.0" : "40" );
			break;
		case CST_ICX674:
			sb.append( researchSpec ? "2.8" : "28" );
			break;
		case CST_ICX694:
			sb.append( researchSpec ? "6.0" : "60" );
			break;
		case CST_ICX814:
			sb.append( researchSpec ? "9.0" : "90" );
			break;			
		default:
			return defaultNumber;
    }

	return sb;
}

std::string QSIModelInfo::GetModelNumber(std::string defaultNumber)		// Full Marketing model number
{
    std::string sb;
	bool hasFilterWheel = false;
	bool hasShutter = false;
	bool hasMicrolens = false;

	sb = GetBaseModelNumber(defaultNumber);

	// Any NotImplemented/NotSpecified, just ignore feature
	// Any NotInitialized indicates the usage of the defaultNumber, as feature data is invalid.

    switch( CcdSensorColorMask() )
    {
		case CSC_BayerNormal:
		case CSC_TruesenseNormal:
			sb.append( "c" );
			break;
		case CSC_None:
			break;
		case CSC_NotImplemented:
		case CSC_NotSpecified:
			break;
		case CSC_NotInitialized:
		default:
			return defaultNumber;
    }

    switch( FilterWheelConfiguration() )
    {
		case FWC_NoFilterWheel:
			hasFilterWheel = false;
			break;
		case FWC_QsiStandardFilterWheel:
			hasFilterWheel = true;
			sb.append( "w" );
			break;
		case FWC_NotImplemented:
		case FWC_NotSpecified:
			break;
		case FWC_NotInitialized:
		default:
			return defaultNumber;
    }

    switch( ShutterConfiguration() )
    {
		case SC_NoShutter:
			hasShutter = false;
			break;
		case SC_QsiStandardShutter:
			hasShutter = true;
			sb.append( "s" );
			break;
		case CS_NotImplemented:
		case CS_NotSpecified:
			break;
		case CS_NotInitialized:
		default:
			return defaultNumber;
    }

    if( !hasFilterWheel && !hasShutter )
    {
		if( CcdSensorType() == CST_KAI_04022 || CcdSensorType() == CST_KAI_2020 )
			sb.append( "i" );
		else
			sb.append( "x" );
    }

    switch( CcdSensorMicrolens() )
    {
		case CSM_None:
			hasMicrolens = false;
			break;
		case CSM_Yes:
			hasMicrolens = true;
			break;
		case CSM_NotImplemented:
		case CSM_NotSpecified:
			hasMicrolens = false;
			break;
		case CSM_NotInitialized:
		default:
			return defaultNumber;
    }

    switch( CcdSensorClass() )
    {
		case CS_TruesenseClass1:
			sb.append( hasMicrolens ? "-M1" : "-1" );
			break;
		case CS_TruesenseClass2:
			sb.append( hasMicrolens ? "-M2" : "-2" );
			break;
		case CS_TruesenseEngineering:
			sb.append( hasMicrolens ? "-ME" : "-E" );
			break;
		case CS_NotImplemented:
		case CS_NotSpecified:
		case CS_NotApplicable:
			break;
		case CS_NotInitialized:
		default:
			return defaultNumber;
    }

    return sb;
}

