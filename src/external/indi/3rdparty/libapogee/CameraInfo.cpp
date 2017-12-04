/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CameraInfo 
* \brief Namespace the support decoding camera model names from raw input data 
* 
*/ 

#include "CameraInfo.h" 

#include <sstream>

#include "apgHelper.h" 
#include "helpers.h" 

#include "parseCfgTabDelim.h"
#include "CamCfgMatrix.h" 

//////////////////////////// 
//      MK   STR        VECT      FROM     STR     DB
std::vector< std::string > CamInfo::MkStrVectFromStrDb( const CamInfo::StrDb & input )
{
    std::vector< std::string > out;
    out.push_back( input.FactorySn );
    out.push_back( input.CustomerSn );
    out.push_back( input.Id );
    out.push_back( input.Platform );
    out.push_back( input.PartNum );
    out.push_back( input.Ccd );
    out.push_back( input.CcdSn );
    out.push_back( input.CcdGrade );
    out.push_back( input.ProcBoardRev );
    out.push_back( input.DriveBoardRev );
    out.push_back( input.Shutter );
    out.push_back( input.WindowType );
    out.push_back( input.MechCfg );
    out.push_back( input.MechRev );
    out.push_back( input.CoolingType );
    out.push_back( input.FinishFront );
    out.push_back( input.FinishBack );
    out.push_back( input.MpiRev );
    out.push_back( input.TestDate );
    out.push_back( input.TestedBy );
    out.push_back( input.TestedDllRev );
    out.push_back( input.TestedFwRev );
    out.push_back( input.Gain );
    out.push_back( input.Noise );
    out.push_back( input.Bias );
    out.push_back( input.TestTemp );
    out.push_back( input.DarkCount );
    out.push_back( input.DarkDuration );
    out.push_back( input.DarkTemp );
    out.push_back( input.CoolingDelta );
    out.push_back( input.Ad1Offset );
    out.push_back( input.Ad1Gain );
    out.push_back( input.Ad2Offset );
    out.push_back( input.Ad2Gain );
    out.push_back( input.Rma1 );
    out.push_back( input.Rma2 );
    out.push_back( input.Comment1 );
    out.push_back( input.Comment2 );
    out.push_back( input.Comment3 );

    return out;
}

//////////////////////////// 
//  MK      STR        DB        FROM     STR     VECT
CamInfo::StrDb CamInfo::MkStrDbFromStrVect( const std::vector< std::string > & input )
{
    if( input.size() != 39 )
    {
        return GetNoOpDb();
    }
    else
    {
        CamInfo::StrDb out;
        out.FactorySn = input.at(0);
        out.CustomerSn = input.at(1);
        out.Id = input.at(2);
        out.Platform = input.at(3);
        out.PartNum = input.at(4);
        out.Ccd = input.at(5);
        out.CcdSn = input.at(6);
        out.CcdGrade = input.at(7);
        out.ProcBoardRev = input.at(8);
        out.DriveBoardRev = input.at(9);
        out.Shutter = input.at(10);
        out.WindowType = input.at(11);
        out.MechCfg = input.at(12);
        out.MechRev = input.at(13);
        out.CoolingType = input.at(14);
        out.FinishFront = input.at(15);
        out.FinishBack = input.at(16);
        out.MpiRev = input.at(17);
        out.TestDate = input.at(18);
        out.TestedBy = input.at(19);
        out.TestedDllRev = input.at(20);
        out.TestedFwRev = input.at(21);
        out.Gain = input.at(22);
        out.Noise = input.at(23);
        out.Bias = input.at(24);
        out.TestTemp = input.at(25);
        out.DarkCount = input.at(26);
        out.DarkDuration = input.at(27);
        out.DarkTemp = input.at(28);
        out.CoolingDelta = input.at(29);
        out.Ad1Offset = input.at(30);
        out.Ad1Gain = input.at(31);
        out.Ad2Offset = input.at(32);
        out.Ad2Gain = input.at(33);
        out.Rma1 = input.at(34);
        out.Rma2 = input.at(35);
        out.Comment1 = input.at(36);
        out.Comment2 = input.at(37);
        out. Comment3 = input.at(38);

        return out;
    }
}

//////////////////////////// 
//  GET    NO       OP       DB
CamInfo::StrDb CamInfo::GetNoOpDb()
{
    CamInfo::StrDb out;
    out.FactorySn = "Not Set";
    out.CustomerSn = "Not Set";
    out.Id = "Not Set";
    out.Platform = "Not Set";
    out.PartNum = "Not Set";
    out.Ccd = "Not Set";
    out.CcdSn = "Not Set";
    out.CcdGrade = "Not Set";
    out.ProcBoardRev = "Not Set";
    out.DriveBoardRev = "Not Set";
    out.Shutter = "Not Set";
    out.WindowType = "Not Set";
    out.MechCfg = "Not Set";
    out.MechRev = "Not Set";
    out.CoolingType = "Not Set";
    out.FinishFront = "Not Set";
    out.FinishBack = "Not Set";
    out.MpiRev = "Not Set";
    out.TestDate = "Not Set";
    out.TestedBy = "Not Set";
    out.TestedDllRev = "Not Set";
    out.TestedFwRev = "Not Set";
    out.Gain = "Not Set";
    out.Noise = "Not Set";
    out.Bias = "Not Set";
    out.TestTemp = "Not Set";
    out.DarkCount = "Not Set";
    out.DarkDuration = "Not Set";
    out.DarkTemp = "Not Set";
    out.CoolingDelta = "Not Set";
    out.Ad1Offset = "Not Set";
    out.Ad1Gain = "Not Set";
    out.Ad2Offset = "Not Set";
    out.Ad2Gain = "Not Set";
    out.Rma1 = "Not Set";
    out.Rma2 = "Not Set";
    out.Comment1 = "Not Set";
    out.Comment2 = "Not Set";
    out. Comment3 = "Not Set";

    return out;
}


//////////////////////////// 
//      MK   U8        VECT      FROM     NET     DB
std::vector< uint8_t > CamInfo::MkU8VectFromNetDb( const CamInfo::NetDb & input )
{
    std::vector< uint8_t > out;
    out.push_back( input.Magic >> 24 & 0xff );
    out.push_back( input.Magic >> 16 & 0xff );
    out.push_back( input.Magic >> 8 & 0xff );
    out.push_back( input.Magic & 0xff );
	out.push_back( input.IP[0] );
	out.push_back( input.IP[1] );
	out.push_back( input.IP[2] );
	out.push_back( input.IP[3] );
	out.push_back( input.Gateway[0] );
	out.push_back( input.Gateway[1] );
	out.push_back( input.Gateway[2] );
	out.push_back( input.Gateway[3] );
	out.push_back( input.Netmask[0] );
	out.push_back( input.Netmask[1] );
	out.push_back( input.Netmask[2] );
	out.push_back( input.Netmask[3] );
	out.push_back( input.Port[0] );
	out.push_back( input.Port[1] );
	out.push_back( input.Flags[0] );
	out.push_back( input.Flags[1] );
	out.push_back( input.Flags[2] );
	out.push_back( input.Flags[3] );
	out.push_back( input.MAC[0] );
	out.push_back( input.MAC[1] );
	out.push_back( input.MAC[2] );
	out.push_back( input.MAC[3] );
	out.push_back( input.MAC[4] );
	out.push_back( input.MAC[5] );
	out.push_back( input.Timeout >> 24 & 0xff );
	out.push_back( input.Timeout >> 16 & 0xff );
	out.push_back( input.Timeout >> 8 & 0xff );
	out.push_back( input.Timeout & 0xff );

    return out;
}

//////////////////////////// 
//  MK      STR        DB        FROM     STR     VECT
CamInfo::NetDb CamInfo::MkNetDbFromU8Vect( const std::vector< uint8_t > & input )
{
        CamInfo::NetDb out;
        out.Magic = input.at(0) << 24 | input.at(1) << 16 | input.at(2) << 8 | input.at(3);
        out.IP[0] = input.at(4);
        out.IP[1] = input.at(5);
        out.IP[2] = input.at(6);
        out.IP[3] = input.at(7);
        out.Gateway[0] = input.at(8);
        out.Gateway[1] = input.at(9);
        out.Gateway[2] = input.at(10);
        out.Gateway[3] = input.at(11);
        out.Netmask[0] = input.at(12);
        out.Netmask[1] = input.at(13);
        out.Netmask[2] = input.at(14);
        out.Netmask[3] = input.at(15);
        out.Port[0] = input.at(16);
        out.Port[1] = input.at(17);
        out.Flags[0] = input.at(18);
        out.Flags[1] = input.at(19);
        out.Flags[2] = input.at(20);
        out.Flags[3] = input.at(21);
        out.MAC[0] = input.at(22);
        out.MAC[1] = input.at(23);
        out.MAC[2] = input.at(24);
        out.MAC[3] = input.at(25);
        out.MAC[4] = input.at(26);
        out.MAC[5] = input.at(27);
        out.Timeout = input.at(28) << 24 | input.at(29) << 16 | input.at(30) << 8 | input.at(31);

        return out;
    
}

//////////////////////////// 
// GET     PLATFORM
CamModel::PlatformType CamModel::GetPlatformType( const uint16_t FixedId, bool IsEthernet )
{
    std::string result = CamModel::GetPlatformStr( FixedId, IsEthernet );
    return  CamModel::GetPlatformType( result );
}

//////////////////////////// 
// GET     PLATFORM
CamModel::PlatformType CamModel::GetPlatformType( const std::string & cameraLine )
{
    std::string altaU("AltaU");
    if( 0 == cameraLine.compare( 0, altaU.size(), altaU ) )
    {
        return CamModel::ALTAU;
    }

    std::string altaE("AltaE");
    if( 0 == cameraLine.compare( 0, altaE.size(), altaE ) )
    {
        return CamModel::ALTAE;
    }

    std::string ascent("Ascent");
    if( 0 == cameraLine.compare( 0, ascent.size(), ascent ) )
    {
        return CamModel::ASCENT;
    }

    std::string g("Aspen");
    if( 0 == cameraLine.compare( 0, g.size(), g ) )
    {
        return CamModel::ASPEN;
    }

    std::string h("HiC");
    if( 0 == cameraLine.compare( 0, h.size(), h ) )
    {
        return CamModel::HIC;
    }

    std::string i("AltaF");
    if( 0 == cameraLine.compare( 0, i.size(), i ) )
    {
        return CamModel::ALTAF;
    }

    std::string j("Quad");
    if( 0 == cameraLine.compare( 0, j.size(), j ) )
    {
        return CamModel::QUAD;
    }

    return CamModel::UNKNOWN_PLATFORM;
}

//////////////////////////// 
// GET     PLATFORM    STR
std::string CamModel::GetPlatformStr(const uint16_t FixedId, bool IsEthernet)
{
    // TODO - remove when get a better solution
    const uint16_t PMO_ID = 412;
    if( PMO_ID == FixedId )
    {
        return "Quad";
    }

    std::string cfgName = apgHelper::GetCamCfgDir() + apgHelper::GetCfgFileName();
    CamCfg::APN_CAMERA_METADATA meta =
    parseCfgTabDelim::FetchMetaData( cfgName, FixedId );
    
    std::string result = meta.CameraLine;
    
    //if old alta append with U or E
    std::string alta("Alta");
    if( result.size() == alta.size() )
    {
        if( 0 == result.compare( 0, alta.size(), alta ) )
        {
            if( IsEthernet )
            {
                result.append("E");
            }
            else
            {
                result.append( "U" );
            }
        }
    }

    return result;
}


//////////////////////////// 
// GET  NOOP     FIRMWARE    REV
std::string CamModel::GetNoOpFirmwareRev()
{
    //want something other than FFFF or 0000
    //for no op, because the we could just error to
    //those extremes and I want to see that we set this
    //on purpose
    std::stringstream msg;
    msg << std::hex << NO_OP_FRMWR_REV; 
    return msg.str();
}

//////////////////////////// 
// IS   ALTA 
bool CamModel::IsAlta(const uint16_t FirmwareRev)
{
    return(  FirmwareRev > 0 && FirmwareRev < MAX_ALTA_FIRMWARE_REV ? true : false);
}

    
//////////////////////////// 
// IS      GEN        2      PLATFORM 
bool CamModel::IsGen2Platform(const uint16_t FirmwareRev)
{
    if( MIN_GEN2_FIRMWARE  <= FirmwareRev && 
        FirmwareRev <= MAX_GEN2_FIRMWARE)
    {
        return true;
    }

    return false;
}


//////////////////////////// 
// MASK      RAW       ID  
uint16_t CamModel::MaskRawId( const uint16_t FirmwareRev,
        const uint16_t CamId)
{
    if( !CamModel::IsFirmwareRevGood(FirmwareRev) )
    {
        std::string errStr = "Invalid camera firmware revision = " + 
             help::uShort2Str(FirmwareRev);
        apgHelper::throwRuntimeException( __FILE__, errStr, __LINE__,
            Apg::ErrorType_Configuration );
    }


    uint16_t result = CamCfg::ID_NO_OP;
    if( IsAlta( FirmwareRev ) )
    {
        result = CamId & ALTA_CAMERA_ID_MASK;
    }

    if( IsGen2Platform( FirmwareRev ) )
    {
        result = CamId & GEN2_CAMERA_ID_MASK;
    }

    if( CamCfg::ID_NO_OP == result )
     {
        std::string errStr = "Error determining platform type, firmware revision = " + 
             help::uShort2Str(FirmwareRev);
        apgHelper::throwRuntimeException( __FILE__, errStr, __LINE__,
            Apg::ErrorType_Configuration);
    }

    return result;
}

//////////////////////////// 
// IS      FIRMWARE    REV        GOOD
bool CamModel::IsFirmwareRevGood( uint16_t FirmwareRev )
{
    int32_t TotalHits = 0;

    if( CamModel::IsAlta( FirmwareRev ) )
    {
        ++TotalHits;
    }

    if( CamModel::IsGen2Platform( FirmwareRev ) )
    {
        ++TotalHits;
    }


    return( 1 == TotalHits ? true : false );
}


//////////////////////////// 
// GET     MODEL   STR
std::string CamModel::GetModelStr( const uint16_t CamId )
{
    std::string cfgName = apgHelper::GetCamCfgDir() + apgHelper::GetCfgFileName();
    CamCfg::APN_CAMERA_METADATA meta =
    parseCfgTabDelim::FetchMetaData( cfgName, CamId );
    
    return meta.CameraModel;
}
