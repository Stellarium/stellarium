/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \brief this name space defines the structures and enums for configuration meta, vertical and horiztonal patter data, anything from the configuration matrix  
* 
*/ 

#include "CamCfgMatrix.h" 
#include <sstream>
#include <stdexcept>

//////////////////////////// 
//      CLEAR
void CamCfg::Clear( CamCfg::APN_CAMERA_METADATA & data )
{
    data.Sensor.clear();
    data.CameraId = ID_NO_OP;
    data.CameraLine.clear();
    data.CameraModel.clear();
    data.InterlineCCD = false;
    data.SupportsSerialA = false;
    data.SupportsSerialB = false;
    data.SensorTypeCCD = false;
    data.TotalColumns = 0;
    data.ImagingColumns = 0;
    data.ClampColumns = 0;
    data.PreRoiSkipColumns = 0;
    data.PostRoiSkipColumns = 0;
    data.OverscanColumns = 0;
    data.TotalRows = 0;
    data.ImagingRows = 0;
    data.UnderscanRows = 0;
    data.OverscanRows = 0;
    data.VFlushBinning= 0;
    data.EnableSingleRowOffset = 0;
    data.RowOffsetBinning = 0;
    data.HFlushDisable= false;
    data.ShutterCloseDelay = 0;
    data.PixelSizeX = 0;
    data.PixelSizeY = 0;
    data.Color = false;
    data.ReportedGainSixteenBit = 0.0;
    data.MinSuggestedExpTime = 0.0;
    data.CoolingSupported= false;
    data.RegulatedCoolingSupported = false;
    data.TempSetPoint= 0.0;
    data.TempRampRateOne = 0;
    data.TempRampRateTwo = 0;
    data.TempBackoffPoint = 0.0;
    data.PrimaryADType = CamCfg::ApnAdType_None;
    data.AlternativeADType= CamCfg::ApnAdType_None;
    data.PrimaryADLatency = 0;
    data.AlternativeADLatency= 0;
    data.IRPreflashTime= 0.0;
    data.AdCfg= 0;
    data.DefaultGainLeft= 0;
    data.DefaultOffsetLeft= 0;
    data.DefaultGainRight= 0;
    data.DefaultOffsetRight= 0;
    data.DefaultRVoltage= 0;
    data.DefaultDataReduction= false;
    data.VideoSubSample = 0;
	data.AmpCutoffDisable = 0;
    data.NumAdOutputs = 0;
    data.SupportsSingleDualReadoutSwitching = false;
    data.VerticalPattern.clear();
    data.ClampPatternNormal.clear();
    data.SkipPatternNormal.clear();
    data.RoiPatternNormal.clear();
    data.ClampPatternFast.clear();
    data.SkipPatternFast.clear();
    data.RoiPatternFast.clear();
    data.VerticalPatternVideo.clear();
    data.ClampPatternVideo.clear();
    data.SkipPatternVideo.clear();
    data.RoiPatternVideo.clear();
}

//////////////////////////// 
//      CLEAR
void CamCfg::Clear( CamCfg::APN_VPATTERN_FILE & data )
{
    data.Mask = 0;
    data.PatternData.clear();
}

//////////////////////////// 
//      CLEAR
void CamCfg::Clear( CamCfg::APN_HPATTERN_FILE & data )
{
    data.Mask = 0;
    data.RefPatternData.clear();
    data.SigPatternData.clear();
    data.BinPatternData.clear();
}

//////////////////////////// 
//      CONVERT      INT      2          APNADTYPE
CamCfg::ApnAdType CamCfg::ConvertInt2ApnAdType(const int32_t value)
{
    CamCfg::ApnAdType result = CamCfg::ApnAdType_None;

    switch( value )
    {
        case 0:
            result = ApnAdType_None;
        break;

        case 1:
            result = ApnAdType_Alta_Sixteen;
        break;

        case 2:
            result = ApnAdType_Alta_Twelve;
        break;

        case 3:
            result = ApnAdType_Ascent_Sixteen;
        break;

        default:
        {
            std::stringstream msg;
            msg << __FILE__ << "(" << __LINE__ << "):Undefine ApnAdType: " << value;
            std::runtime_error except( msg.str() );
            throw except;
        }
        break;
    }

    return result;
}

//////////////////////////// 
//  APN    AD    TYPE      2      STR
std::string CamCfg::ApnAdType2Str( const CamCfg::ApnAdType in )
{
    switch( in )
    {
        case( CamCfg::ApnAdType_None ):
        default:
            return "CamCfg::ApnAdType_None";
        break;

        case( CamCfg::ApnAdType_Alta_Sixteen ):
            return "CamCfg::ApnAdType_Alta_Sixteen";
        break;

        case( CamCfg::ApnAdType_Alta_Twelve ):
            return "CamCfg::ApnAdType_Alta_Twelve";
        break;

        case( CamCfg::ApnAdType_Ascent_Sixteen ):
            return "CamCfg::ApnAdType_Ascent_Sixteen";
        break;   
    }

}
