/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
*
* \brief this name space defines the structures and enums for configuration meta, vertical and horiztonal patter data, anything from the configuration matrix  
* 
*/ 


#ifndef CAMCFGMATRIX_INCLUDE_H__ 
#define CAMCFGMATRIX_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>

namespace CamCfg
{ 
    const uint16_t ID_NO_OP = 60000;

    enum ApnAdType 
    {
	    ApnAdType_None,
	    ApnAdType_Alta_Sixteen,
	    ApnAdType_Alta_Twelve,
	    ApnAdType_Ascent_Sixteen
    };

    struct APN_CAMERA_METADATA 
    {
        std::string Sensor;
        uint16_t CameraId;
        std::string CameraLine;
        std::string CameraModel;
        bool InterlineCCD;
        bool SupportsSerialA;
        bool SupportsSerialB;
        bool SensorTypeCCD;
        uint16_t TotalColumns;
        uint16_t ImagingColumns;
        uint16_t ClampColumns;
	    uint16_t PreRoiSkipColumns;
	    uint16_t PostRoiSkipColumns;
	    uint16_t OverscanColumns;
	    uint16_t TotalRows;
	    uint16_t ImagingRows;
	    uint16_t UnderscanRows;
	    uint16_t OverscanRows;
	    uint16_t VFlushBinning;
	    bool EnableSingleRowOffset;
	    uint16_t RowOffsetBinning;
	    bool HFlushDisable;
	    double ShutterCloseDelay;
	    double PixelSizeX;
	    double PixelSizeY;
	    bool Color;
	    double ReportedGainSixteenBit;
	    double MinSuggestedExpTime;
	    bool CoolingSupported;
	    bool RegulatedCoolingSupported;
	    double TempSetPoint;
	    uint16_t TempRampRateOne;
	    uint16_t TempRampRateTwo;
	    double TempBackoffPoint;
	    ApnAdType	 PrimaryADType;
	    ApnAdType	 AlternativeADType;
	    uint16_t PrimaryADLatency;
	    uint16_t AlternativeADLatency;
	    double IRPreflashTime;
        uint16_t AdCfg;
	    uint16_t DefaultGainLeft;
	    uint16_t DefaultOffsetLeft;
	    uint16_t DefaultGainRight;
	    uint16_t DefaultOffsetRight;
	    uint16_t DefaultRVoltage;
	    bool DefaultDataReduction;
        uint16_t VideoSubSample;
		uint16_t AmpCutoffDisable;
        uint16_t NumAdOutputs;
        bool  SupportsSingleDualReadoutSwitching;
        std::string VerticalPattern;
        std::string ClampPatternNormal;
        std::string SkipPatternNormal;
        std::string RoiPatternNormal;
        std::string ClampPatternFast;
        std::string SkipPatternFast;
        std::string RoiPatternFast;
        std::string VerticalPatternVideo;
        std::string ClampPatternVideo;
        std::string SkipPatternVideo;
        std::string RoiPatternVideo;
        std::string ClampPatternNormalDual;
        std::string SkipPatternNormalDual;
        std::string RoiPatternNormalDual;
        std::string ClampPatternFastDual;
        std::string SkipPatternFastDual;
        std::string RoiPatternFastDual;
    };

   struct APN_VPATTERN_FILE {
	    uint16_t Mask;
	    std::vector<uint16_t> PatternData;
    };

   struct APN_HPATTERN_FILE {
	    uint16_t Mask;
	    std::vector<uint16_t> RefPatternData;
	    std::vector< std::vector<uint16_t> > BinPatternData;
	    std::vector<uint16_t> SigPatternData;
    };

      
   void Clear( CamCfg::APN_CAMERA_METADATA & data );
   void Clear( CamCfg::APN_VPATTERN_FILE & data );
   void Clear( CamCfg::APN_HPATTERN_FILE & data );

   //type safe enum conversion function
   CamCfg::ApnAdType ConvertInt2ApnAdType(const int32_t value);
   std::string ApnAdType2Str( const CamCfg::ApnAdType in );

}

#endif
