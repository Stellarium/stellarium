//////////////////////////////////////////////////////////////////////
//
// ApnCamData.cpp:  Implementation of the CApnCamData class.
//
// Copyright (c) 2003-2007 Apogee Instruments, Inc.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.
//////////////////////////////////////////////////////////////////////


#include "ApnCamData.h"

#include "apgHelper.h" 
#include "helpers.h"
#include "parseCfgTabDelim.h"

#include <sstream>
#include <fstream>

namespace
{
    //////////////////////////// 
    // MK       PATTERN      FILENAME
    std::string MkPatternFileName( const std::string & path, 
                                           const std::string & baseName)
    {
        std::string result = help::FixPath( path ) + baseName + ".txt";
        return result;
    }
}

//////////////////////////// 
// CTOR 
CApnCamData::CApnCamData() : m_FileName( __FILE__ )
{
    Clear();
}


//////////////////////////// 
// CTOR 
CApnCamData::CApnCamData(const CamCfg::APN_CAMERA_METADATA & meta,
        const CamCfg::APN_VPATTERN_FILE & vert,
        const CamCfg::APN_HPATTERN_FILE & clampNorm,
        const CamCfg::APN_HPATTERN_FILE & skipNorm,
        const CamCfg::APN_HPATTERN_FILE & roiNorm,
        const CamCfg::APN_HPATTERN_FILE & clampFast,
        const CamCfg::APN_HPATTERN_FILE & skipFast,
        const CamCfg::APN_HPATTERN_FILE & roiFast,
        const CamCfg::APN_VPATTERN_FILE & vertVideo,
        const CamCfg::APN_HPATTERN_FILE & clampVideo,
        const CamCfg::APN_HPATTERN_FILE & skipVideo,
        const CamCfg::APN_HPATTERN_FILE & roiVideo,
        const CamCfg::APN_HPATTERN_FILE & clampNormDual,
        const CamCfg::APN_HPATTERN_FILE & skipNormDual,
        const CamCfg::APN_HPATTERN_FILE & roiNormDual,
        const CamCfg::APN_HPATTERN_FILE & clampFastDual,
        const CamCfg::APN_HPATTERN_FILE & skipFastDual,
        const CamCfg::APN_HPATTERN_FILE & roiFastDual ) :
        m_MetaData( meta ),
        m_VerticalPattern( vert ),
        m_ClampPatternNormal( clampNorm ),
        m_SkipPatternNormal( skipNorm ),
        m_RoiPatternNormal( roiNorm ),
        m_ClampPatternFast( clampFast ),
        m_SkipPatternFast( skipFast ),
        m_RoiPatternFast( roiFast ),
        m_VerticalPatternVideo( vertVideo ),
        m_ClampPatternVideo( clampVideo ),
        m_SkipPatternVideo( skipVideo ),
        m_RoiPatternVideo( roiVideo ),
        m_ClampPatternNormalDual( clampNormDual ),
        m_SkipPatternNormalDual( skipNormDual ),
        m_RoiPatternNormalDual( roiNormDual ),
        m_ClampPatternFastDual( clampFastDual ),
        m_SkipPatternFastDual( skipFastDual ),
        m_RoiPatternFastDual( roiFastDual ),
        m_FileName( __FILE__ )
{
        
}

//////////////////////////// 
// COPY       CTOR 
CApnCamData::CApnCamData( const CApnCamData &rhs ) :
    m_MetaData( rhs.m_MetaData ),
        m_VerticalPattern( rhs.m_VerticalPattern ),
        m_ClampPatternNormal( rhs.m_ClampPatternNormal ),
        m_SkipPatternNormal( rhs.m_SkipPatternNormal ),
        m_RoiPatternNormal( rhs.m_RoiPatternNormal ),
        m_ClampPatternFast( rhs.m_ClampPatternFast ),
        m_SkipPatternFast( rhs.m_SkipPatternFast ),
        m_RoiPatternFast( rhs.m_RoiPatternFast ),
        m_VerticalPatternVideo( rhs.m_VerticalPatternVideo ),
        m_ClampPatternVideo( rhs.m_ClampPatternVideo ),
        m_SkipPatternVideo( rhs.m_SkipPatternVideo ),
        m_RoiPatternVideo( rhs.m_RoiPatternVideo ),
        m_ClampPatternNormalDual( rhs.m_ClampPatternNormalDual ),
        m_SkipPatternNormalDual( rhs.m_SkipPatternNormalDual ),
        m_RoiPatternNormalDual( rhs.m_RoiPatternNormalDual ),
        m_ClampPatternFastDual( rhs.m_ClampPatternFastDual ),
        m_SkipPatternFastDual( rhs.m_SkipPatternFastDual ),
        m_RoiPatternFastDual( rhs.m_RoiPatternFastDual ),
        m_FileName( __FILE__ )
{
}

//////////////////////////// 
// DTOR 
CApnCamData::~CApnCamData()
{
    Clear();
}

//////////////////////////// 
//      ASSIGNMENT    OPERATOR
CApnCamData& CApnCamData::operator=(CApnCamData const&d)
{
     // Gracefully handle self assignment
    // from http://www.parashift.com/c++-faq-lite/assignment-operators.html
    if (this != &d) 
    {  
        this->m_ClampPatternFast = d.m_ClampPatternFast;
        this->m_ClampPatternNormal = d.m_ClampPatternNormal;
        this->m_ClampPatternVideo = d.m_ClampPatternVideo;
        this->m_MetaData = d.m_MetaData;
        this->m_RoiPatternFast = d.m_RoiPatternFast;
        this->m_RoiPatternNormal = d.m_RoiPatternNormal;
        this->m_RoiPatternVideo = d.m_RoiPatternVideo;
        this->m_SkipPatternFast = d.m_SkipPatternFast;
        this->m_SkipPatternNormal = d.m_SkipPatternNormal;
        this->m_SkipPatternVideo = d.m_SkipPatternVideo;
        this->m_VerticalPattern = d.m_VerticalPattern;
        this->m_VerticalPatternVideo = d.m_VerticalPatternVideo;
        this->m_ClampPatternNormalDual = d.m_ClampPatternNormalDual;
        this->m_SkipPatternNormalDual = d.m_SkipPatternNormalDual;
        this->m_RoiPatternNormalDual = d.m_RoiPatternNormalDual;
        this->m_ClampPatternFastDual = d.m_ClampPatternFastDual;
        this->m_SkipPatternFastDual = d.m_SkipPatternFastDual;
        this->m_RoiPatternFastDual = d.m_RoiPatternFastDual;
    }
   return *this;
}

//////////////////////////// 
// SET 
void CApnCamData::Set( const std::string & path, 
                      const std::string & cfgFile, 
                      const uint16_t CamId )
{

    try
    {
        std::string fixedPath = help::FixPath(path);
        std::string fullFile = fixedPath + cfgFile;
        
        m_MetaData = parseCfgTabDelim::FetchMetaData( fullFile, CamId );

        //use the sensor data to
        //get the pattern data
        m_VerticalPattern = parseCfgTabDelim::FetchVerticalPattern( 
            MkPatternFileName( fixedPath, m_MetaData.VerticalPattern ) );
        
        //normal clamp
        m_ClampPatternNormal = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.ClampPatternNormal )  );
     
        //normal skip
        m_SkipPatternNormal = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.SkipPatternNormal )  );

        //normal roi
        m_RoiPatternNormal = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.RoiPatternNormal )  );

        //fast clamp
        m_ClampPatternFast = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.ClampPatternFast )  );

        //Fast skip
        m_SkipPatternFast = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.SkipPatternFast )  );

        //Fast roi
        m_RoiPatternFast = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.RoiPatternFast )  );

        //veritcal video
        m_VerticalPatternVideo = parseCfgTabDelim::FetchVerticalPattern( 
            MkPatternFileName( fixedPath, m_MetaData.VerticalPatternVideo )  );
        
        //video clamp
        m_ClampPatternVideo  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.ClampPatternVideo )  );
       
        //video skip
        m_SkipPatternVideo = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.SkipPatternVideo )  );

        //video roi
        m_RoiPatternVideo = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.RoiPatternVideo )  );

         //normal clamp dual
        m_ClampPatternNormalDual = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.ClampPatternNormalDual  )  );
     
        //normal skip dual
        m_SkipPatternNormalDual  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.SkipPatternNormalDual )  );

        //normal roi dual
        m_RoiPatternNormalDual  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.RoiPatternNormalDual )  );

        //fast clamp dual
        m_ClampPatternFastDual  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.ClampPatternFastDual )  );

        //Fast skip dual
        m_SkipPatternFastDual  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.SkipPatternFastDual )  );

        //Fast roi dual
        m_RoiPatternFastDual  = parseCfgTabDelim::FetchHorizontalPattern(
            MkPatternFileName( fixedPath, m_MetaData.RoiPatternFastDual )  );
    }
    catch( std::exception & e)
    {
        std::string errStr( e.what() );
        errStr += " exception thrown for camera id = " + help::uShort2Str( CamId );
        apgHelper::throwRuntimeException( __FILE__, errStr, 
            __LINE__, Apg::ErrorType_Configuration );
    }
}

//////////////////////////// 
// CLEAR
void CApnCamData::Clear()
{
    CamCfg::Clear( m_MetaData );
    CamCfg::Clear( m_VerticalPattern );
    CamCfg::Clear( m_ClampPatternNormal );
    CamCfg::Clear( m_SkipPatternNormal );
    CamCfg::Clear( m_RoiPatternNormal );
    CamCfg::Clear( m_ClampPatternFast);
    CamCfg::Clear( m_SkipPatternFast );
    CamCfg::Clear( m_RoiPatternFast );
    CamCfg::Clear( m_VerticalPatternVideo );
    CamCfg::Clear( m_ClampPatternVideo );
    CamCfg::Clear( m_SkipPatternVideo );
    CamCfg::Clear( m_RoiPatternVideo );
    CamCfg::Clear( m_ClampPatternNormalDual  );
    CamCfg::Clear( m_SkipPatternNormalDual  );
    CamCfg::Clear( m_RoiPatternNormalDual  );
    CamCfg::Clear( m_ClampPatternFastDual  );
    CamCfg::Clear( m_SkipPatternFastDual  );
    CamCfg::Clear( m_RoiPatternFastDual  );
}

void CApnCamData::Write2File( const std::string & fname )
{
    std::ofstream f( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "cfg data" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteMeta(  fname );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Vertical" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteVPattern( fname, m_VerticalPattern );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Clamp Normal" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_ClampPatternNormal );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Skip Normal" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_SkipPatternNormal );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Roi Normal" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_RoiPatternNormal );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Clamp Fast" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_ClampPatternFast );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Skip Fast" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_SkipPatternFast );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Roi Fast" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_RoiPatternFast );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Vertical Video" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteVPattern( fname, m_VerticalPatternVideo );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Clamp Video" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_ClampPatternVideo );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Skip Video" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_SkipPatternVideo );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Roi Video" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_RoiPatternVideo );

     f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Clamp Normal Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_ClampPatternNormalDual );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Skip Normal Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_SkipPatternNormalDual );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Roi Normal Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_RoiPatternNormalDual );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Clamp Fast Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_ClampPatternFastDual );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Skip Fast Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_SkipPatternFastDual );

    f.open( fname.c_str(), std::ios::out | std::ios::app );
    f << "---------------------------------------" << std::endl;
    f << "Roi Fast Dual" << std::endl;
    f << "---------------------------------------" << std::endl;
    f.close();

    WriteHPattern( fname, m_RoiPatternFastDual );
}

void CApnCamData::WriteMeta( const std::string & fname )
{
    std::ofstream f( fname.c_str(), std::ios::out | std::ios::app );

    f << "Sensor: " << m_MetaData.Sensor << std::endl;
    f << "CameraId: " << m_MetaData.CameraId << std::endl;
    f << "CameraLine: " << m_MetaData.CameraLine << std::endl;
    f << "CameraModel: " << m_MetaData.CameraModel << std::endl;
    f << "InterlineCCD: " << m_MetaData.InterlineCCD << std::endl;
    f << "SupportsSerialA: " << m_MetaData.SupportsSerialA << std::endl;
    f << "SupportsSerialB: " << m_MetaData.SupportsSerialB << std::endl;
    f << "SensorTypeCCD: " << m_MetaData.SensorTypeCCD << std::endl;
    f << "TotalColumns: " << m_MetaData.TotalColumns << std::endl;
    f << "ImagingColumns: " << m_MetaData.ImagingColumns << std::endl;
    f << "ClampColumns: " << m_MetaData.ClampColumns << std::endl;
    f << "PreRoiSkipColumns: " << m_MetaData.PreRoiSkipColumns << std::endl;
    f << "PostRoiSkipColumns: " << m_MetaData.PostRoiSkipColumns << std::endl;
    f << "OverscanColumns: " << m_MetaData.OverscanColumns << std::endl;
    f << "TotalRows: " << m_MetaData.TotalRows << std::endl;
    f << "ImagingRows: " << m_MetaData.ImagingRows << std::endl;
    f << "UnderscanRows: " << m_MetaData.UnderscanRows << std::endl;
    f << "OverscanRows: " << m_MetaData.OverscanRows << std::endl;
    f << "VFlushBinning: " << m_MetaData.VFlushBinning << std::endl;
    f << "EnableSingleRowOffset: " << m_MetaData.EnableSingleRowOffset << std::endl;
    f << "RowOffsetBinning: " << m_MetaData.RowOffsetBinning << std::endl;
    f << "HFlushDisable: " << m_MetaData.HFlushDisable << std::endl;
    f << "ShutterCloseDelay: " << m_MetaData.ShutterCloseDelay << std::endl;
    f << "PixelSizeX: " << m_MetaData.PixelSizeX << std::endl;
    f << "PixelSizeY: " << m_MetaData.PixelSizeY << std::endl;
    f << "Color: " << m_MetaData.Color << std::endl;
    f << "ReportedGainSixteenBit: " << m_MetaData.ReportedGainSixteenBit << std::endl;
    f << "MinSuggestedExpTime: " << m_MetaData.MinSuggestedExpTime << std::endl;
    f << "CoolingSupported: " << m_MetaData.CoolingSupported << std::endl;
    f << "RegulatedCoolingSupported: " << m_MetaData.RegulatedCoolingSupported << std::endl;
    f << "TempSetPoint: " << m_MetaData.TempSetPoint << std::endl;
    f << "TempRampRateOne: " << m_MetaData.TempRampRateOne << std::endl;
    f << "TempRampRateTwo: " << m_MetaData.TempRampRateTwo << std::endl;
    f << "TempBackoffPoint: " << m_MetaData.TempBackoffPoint << std::endl;
    f << "PrimaryADType: " << m_MetaData.PrimaryADType << std::endl;
    f << "AlternativeADType: " << m_MetaData.AlternativeADType << std::endl;
    f << "PrimaryADLatency: " << m_MetaData.PrimaryADLatency << std::endl;
    f << "AlternativeADLatency: " << m_MetaData.AlternativeADLatency << std::endl;
    f << "IRPreflashTime: " << m_MetaData.IRPreflashTime << std::endl;
    f << "AdCfg: " << m_MetaData.AdCfg << std::endl;
    f << "DefaultGainLeft: " << m_MetaData.DefaultGainLeft << std::endl;
    f << "DefaultOffsetLeft: " << m_MetaData.DefaultOffsetLeft << std::endl;
    f << "DefaultGainRight: " << m_MetaData.DefaultGainRight << std::endl;
    f << "DefaultOffsetRight: " << m_MetaData.DefaultOffsetRight << std::endl;
    f << "DefaultRVoltage: " << m_MetaData.DefaultRVoltage << std::endl;
    f << "DefaultDataReduction: " << m_MetaData.DefaultDataReduction << std::endl;
    f << "VideoSubSample: " << m_MetaData.VideoSubSample << std::endl;
    f << "AmpCutoffDisable: " << m_MetaData.AmpCutoffDisable << std::endl;
    f << "NumAdOutputs: " << m_MetaData.NumAdOutputs << std::endl;
    f << "SupportsSingleDualReadoutSwitching: " << m_MetaData.SupportsSingleDualReadoutSwitching << std::endl;
    f << "VerticalPattern: " << m_MetaData.VerticalPattern << std::endl;
    f << "SkipPatternNormal: " << m_MetaData.SkipPatternNormal << std::endl;
    f << "ClampPatternNormal: " << m_MetaData.ClampPatternNormal << std::endl;
    f << "RoiPatternNormal: " << m_MetaData.RoiPatternNormal << std::endl;
    f << "SkipPatternFast: " << m_MetaData.SkipPatternFast << std::endl;
    f << "ClampPatternFast: " << m_MetaData.ClampPatternFast << std::endl;
    f << "RoiPatternFast: " << m_MetaData.RoiPatternFast << std::endl;
    f << "VerticalPatternVideo: " << m_MetaData.VerticalPatternVideo << std::endl;
    f << "SkipPatternVideo: " << m_MetaData.SkipPatternVideo << std::endl;
    f << "ClampPatternVideo: " << m_MetaData.ClampPatternVideo << std::endl;
    f << "RoiPatternVideo: " << m_MetaData.RoiPatternVideo << std::endl;
    f << "SkipPatternNormalDual: " << m_MetaData.SkipPatternNormalDual << std::endl;
    f << "ClampPatternNormalDual: " << m_MetaData.ClampPatternNormalDual << std::endl;
    f << "RoiPatternNormalDual: " << m_MetaData.RoiPatternNormalDual << std::endl;
    f << "SkipPatternFastDual: " << m_MetaData.SkipPatternFastDual << std::endl;
    f << "ClampPatternFastDual: " << m_MetaData.ClampPatternFastDual << std::endl;
    f << "RoiPatternFastDual: " << m_MetaData.RoiPatternFastDual << std::endl;

    f.close();
}


void CApnCamData::WriteHPattern( const std::string & fname,
                                const CamCfg::APN_HPATTERN_FILE & horiztonal)
{
     std::ofstream f( fname.c_str(), std::ios::out | std::ios::app );

    f << "Mask: " << horiztonal.Mask << std::endl;

    f << "Ref: ";

    std::vector<uint16_t>::const_iterator iter;

    for( iter = horiztonal.RefPatternData.begin(); 
        iter != horiztonal.RefPatternData.end(); ++iter )
    {
          f << (*iter) << " ";
    }

    f << std::endl;

    f << "Sig: ";

    for( iter = horiztonal.SigPatternData.begin(); 
        iter != horiztonal.SigPatternData.end(); ++iter )
    {
          f << (*iter) << " ";
    }

    f << std::endl;

    int c = 0;
    std::vector< std::vector<uint16_t> >::const_iterator jj;
    for( jj = horiztonal.BinPatternData.begin(); 
        jj != horiztonal.BinPatternData.end(); ++jj, ++c )
    {
        f << "Bin " << c << ": ";
        std::vector<uint16_t>::const_iterator ii;
        for( ii = (*jj).begin(); ii != (*jj).end(); ++ii )
        {
            f << (*ii) << " ";
        }

        f << std::endl;
    }

    f << std::endl;

    f.close();

}

void CApnCamData::WriteVPattern( const std::string & fname, 
                                const CamCfg::APN_VPATTERN_FILE & vert )
{
    std::ofstream f( fname.c_str(), std::ios::out | std::ios::app );

     f << "Mask: " << vert.Mask << std::endl;

    f << "Pattern: ";

    std::vector<uint16_t>::const_iterator iter;

    for( iter = vert.PatternData.begin(); iter != vert.PatternData.end(); ++iter )
    {
          f << (*iter) << " ";
    }

    f << std::endl;
    f.close();
}

