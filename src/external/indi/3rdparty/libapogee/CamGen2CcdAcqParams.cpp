/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CamGen2CcdAcqParams 
* \brief derived class for managing the Ascent and Aspen camera's ADCs, horizontal pattern files, and roi parameters
* 
*/ 

#include "CamGen2CcdAcqParams.h" 
#include "apgHelper.h" 
#include "ApnCamData.h"
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "ApgLogger.h" 
#include <sstream>

namespace
{
    // values for configuring the camera's pixel A to D converter
    const uint16_t AD9826_SINGLE_MUX = 0x10C0;
    const uint16_t AD9826_QUAD_MUX = 0x10D0;
    const uint16_t AD9826_GAIN_MAX = 0x003F;
    const uint16_t AD9826_RED_GAIN_ADDR = 0x2000;
    const uint16_t AD9826_GREEN_GAIN_ADDR = 0x3000;
    const uint16_t AD9826_BLUE_GAIN_ADDR = 0x4000;

    const uint16_t AD9826_OFFEST_MAX = 0x01FF;
    const uint16_t AD9826_RED_OFFSET_ADDR = 0x5000;
    const uint16_t AD9826_GREEN_OFFSET_ADDR  = 0x6000;
    const uint16_t AD9826_BLUE_OFFSET_ADDR = 0x7000;

}

//////////////////////////// 
// CTOR 
CamGen2CcdAcqParams::CamGen2CcdAcqParams(std::shared_ptr<CApnCamData> & camData,
                           std::shared_ptr<CameraIo> & camIo,  
                           std::shared_ptr<PlatformData> & platformData) : 
                                            CcdAcqParams(camData,camIo,platformData),       
                                            m_fileName(__FILE__) 
{ 
    //fill up the vector with default values
    AdcParams ad_0_ch_0 = {CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                 AD9826_RED_GAIN_ADDR,
                                 AD9826_RED_OFFSET_ADDR,
                                 0, 
                                 0 };

    m_adcParamMap[ std::pair<int32_t, int32_t>(0,0) ] = ad_0_ch_0;

    AdcParams ad_0_ch_1 = {CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                 AD9826_BLUE_GAIN_ADDR,
                                 AD9826_BLUE_OFFSET_ADDR,
                                 0, 
                                 0 };

    m_adcParamMap[ std::pair<int32_t, int32_t>(0,1) ] = ad_0_ch_1;

    AdcParams ad_0_ch_2 = {CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                 AD9826_GREEN_GAIN_ADDR,
                                 AD9826_GREEN_OFFSET_ADDR,
                                 0, 
                                 0 };

    m_adcParamMap[ std::pair<int32_t, int32_t>(0,2) ] = ad_0_ch_2;

    AdcParams ad_1_ch_0 = { CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                    AD9826_RED_GAIN_ADDR,
                                    AD9826_RED_OFFSET_ADDR,
                                    0, 
                                    0 };

     m_adcParamMap[ std::pair<int32_t, int32_t>(1,0) ] = ad_1_ch_0;

     AdcParams ad_1_ch_1 = { CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                    AD9826_BLUE_GAIN_ADDR,
                                    AD9826_BLUE_OFFSET_ADDR,
                                    0, 
                                    0 };

     m_adcParamMap[ std::pair<int32_t, int32_t>(1,1) ] = ad_1_ch_1;

     AdcParams ad_1_ch_2 = { CameraRegs::OP_D_AD_LOAD_SELECT_BIT,
                                    AD9826_GREEN_GAIN_ADDR,
                                    AD9826_GREEN_OFFSET_ADDR,                           
                                    0, 
                                    0 };

     m_adcParamMap[ std::pair<int32_t, int32_t>(1,2) ] = ad_1_ch_2;
} 

//////////////////////////// 
// DTOR 
CamGen2CcdAcqParams::~CamGen2CcdAcqParams() 
{ 

} 
//////////////////////////// 
// INIT  ADC
void CamGen2CcdAcqParams::Init()
{
    //setup left side adc
    SetAdcCfgAndMux( 0, 0 );
    SetAdcGain(m_CamData->m_MetaData.DefaultGainLeft, 0, 0);
    SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetLeft, 0, 0);
    SetAdcGain(m_CamData->m_MetaData.DefaultGainLeft, 0, 1);
    SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetLeft, 0, 1);
    SetAdcGain(m_CamData->m_MetaData.DefaultGainLeft, 0, 2);
    SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetLeft, 0, 2);

    //setup right side adc
   SetAdcCfgAndMux( 1, 0 );
   SetAdcGain(m_CamData->m_MetaData.DefaultGainRight, 1, 0);
   SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetRight, 1, 0);
   SetAdcGain(m_CamData->m_MetaData.DefaultGainRight, 1, 1);
   SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetRight, 1, 1);
   SetAdcGain(m_CamData->m_MetaData.DefaultGainRight, 1, 2);
   SetAdcOffset(m_CamData->m_MetaData.DefaultOffsetRight, 1, 2);

    // 3-13-2012 ge reported images filled with zeros
    // (ticket #125) with the fix for ticker #114 in alta project
    // taking out the roi only loads
   SetSpeed( Apg::AdcSpeed_Normal );
}

//////////////////////////// 
// SET      ADC            CFG        AND        MUX           
void CamGen2CcdAcqParams::SetAdcCfgAndMux( const int32_t ad, const int32_t channel )
{
    const uint16_t restore = SelectAd( ad, channel );

    //see page 14 of the AD9826 spec sheet for a description of what the configuration (adCfg) 
    // and mux register bits mean
    // these values only need to be set once at start up
   Write2AdcReg( m_CamData->m_MetaData.AdCfg );

    if( CamGen2CcdAcqParams::QUAD_READOUT == GetReadoutType() )
    {
        Write2AdcReg(  AD9826_QUAD_MUX );
    }
    else
    {
        Write2AdcReg(  AD9826_SINGLE_MUX );
    }

   RestoreAdSelect( restore  );
}


//////////////////////////// 
//      SET         ADC        GAIN
void CamGen2CcdAcqParams::SetAdcGain( const uint16_t gain, const int32_t ad, const int32_t channel )
{
    const uint16_t restore = SelectAd( ad, channel );

    //based on the ad and channel get the adc params
    CamGen2CcdAcqParams::AdcParams params = GetAdcParams( ad, channel );

    // capping the upper limit on the gain
    uint16_t gainIn = 0;
    
    if( gain > AD9826_GAIN_MAX )
    {
        gainIn = AD9826_GAIN_MAX ;
    }
    else
    {
         gainIn = gain;
    }

    //setting bits to tell the adc what channel (R0, G1, B2) the gain is for
    const uint16_t gain2Write = gainIn | params.channelGainAddr;

    Write2AdcReg( gain2Write );

    //if we are here the write was good, now save the info so
    //it can be fecthed later
    params.gain = gainIn;

    SetAdcParams( ad, channel, params );

    RestoreAdSelect( restore );
}

//////////////////////////// 
//  GET    ADC         GAIN
uint16_t CamGen2CcdAcqParams::GetAdcGain( int32_t ad, int32_t channel )
{
     //based on the ad and channel get the adc params
    CamGen2CcdAcqParams::AdcParams params = GetAdcParams( ad, channel );
    return params.gain;

}

//////////////////////////// 
//  SET             ADC       OFFSET   
void CamGen2CcdAcqParams::SetAdcOffset( const uint16_t offset, 
                                       const int32_t ad, const int32_t channel )
{
    const uint16_t restore = SelectAd( ad, channel );

    CamGen2CcdAcqParams::AdcParams params = GetAdcParams( ad, channel );

   // capping the upper limit on the offset
   uint16_t offsetIn = 0;
   if( offset > AD9826_OFFEST_MAX )
   {
       offsetIn = AD9826_OFFEST_MAX;
   }
   else
   {
       offsetIn = offset;
   }

    //setting bits to tell the adc what channel (R0, G1, B2) the offset is for
    const uint16_t offset2Write = offsetIn | params.channelOffsetAddr;

    Write2AdcReg( offset2Write );

    //if we are here the write was good, now save the info so
    //it can be fecthed later
    params.offset = offsetIn;

    SetAdcParams( ad, channel, params );

    RestoreAdSelect( restore  );
}

//////////////////////////// 
//  GET             ADC       OFFSET   
uint16_t CamGen2CcdAcqParams::GetAdcOffset( int32_t ad, int32_t channel )
{
    CamGen2CcdAcqParams::AdcParams params = GetAdcParams( ad, channel );
    return params.offset;
}

//////////////////////////// 
// IS      ADS        SIM     MODE         ON   
bool CamGen2CcdAcqParams::IsAdsSimModeOn()
{
    return( (m_CamIo->ReadReg(CameraRegs::OP_B) & CameraRegs::OP_B_AD_SIMULATION_BIT) ? true : false);
}

//////////////////////////// 
//      GET     ADC         PARAMS
CamGen2CcdAcqParams::AdcParams CamGen2CcdAcqParams::GetAdcParams( int32_t ad, int32_t channel )
{
    std::pair<int32_t, int32_t> val(ad,channel) ;

     std::map< std::pair<int32_t, int32_t>, 
         CamGen2CcdAcqParams::AdcParams >::iterator iter = m_adcParamMap.find( val );

     CamGen2CcdAcqParams::AdcParams result = {0,0,0,0,0};
     if( iter != m_adcParamMap.end() )
     {
         result = (*iter).second;
     }
     else
     {
        std::stringstream msg;
        msg << "Invalid input ad ( " << ad << " ) or channel ( " << channel << " ) ";
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
     }

     return result;
}

//////////////////////////// 
//      SET     ADC         PARAMS
void CamGen2CcdAcqParams::SetAdcParams( const int32_t ad, const int32_t channel, 
        const CamGen2CcdAcqParams::AdcParams & params )
{
    std::pair<int32_t, int32_t> val(ad,channel) ;

     std::map< std::pair<int32_t, int32_t>, 
         CamGen2CcdAcqParams::AdcParams >::iterator iter = m_adcParamMap.find( val );

     if( iter != m_adcParamMap.end() )
     {
         m_adcParamMap[ val ] = params;
     }
     else
     {
        std::stringstream msg;
        msg << "Invalid input ad ( " << ad << " ) or channel ( " << channel << " ) ";
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
     }

}

//////////////////////////// 
//      WRITE        2      ADC     REG
void CamGen2CcdAcqParams::Write2AdcReg(  const uint16_t value2Write )
{   
    // write the adc value to camcon register
    m_CamIo->WriteReg( CameraRegs::AD_CONFIG_DATA,  value2Write );

    //tell camcon to put the adc register data in the adc
	m_CamIo->WriteReg( CameraRegs::CMD_B, CameraRegs::CMD_B_AD_CONFIG_BIT );
}

//////////////////////////// 
//  SET   ADC      RESOLUTION
void CamGen2CcdAcqParams::SetResolution( 
            const Apg::Resolution res )
{
    NO_OP_PARAMETER( res );
    std::string errStr("cannot set CCD adc resolution on ascent/Aspencameras");
    apgHelper::throwRuntimeException( m_fileName, errStr, 
        __LINE__, Apg::ErrorType_InvalidOperation );
}

//////////////////////////// 
//  SET   ADC      SPEED
void CamGen2CcdAcqParams::SetSpeed( const Apg::AdcSpeed speed )
{
    // Reset the camera
    m_CamIo->Reset( false );

    // only changing the ROI pattern on speed switch
    // to prevent half image issue in ticket #114 in alta
    // project
    switch( speed )
    {
        case Apg::AdcSpeed_Fast:

            //ensure the number of cols to bin are ok
            if( GetNumCols2Bin() > GetMaxFastBinCols() )
            {
                std::stringstream msg;
                msg << "Reseting imaging columns to " << GetMaxFastBinCols();
                msg << " because current bin value of " << GetNumCols2Bin();
                msg << " is not supported in fast mode.";
                std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
                ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

                SetNumCols2Bin( GetMaxFastBinCols() );
            }

            // LT: 2012-06-28
            // this bit is not used in alta f, aspen, and new ascents
            // left here because of the v108 ascents may still need it
             m_CamIo->ReadOrWriteReg(CameraRegs::OP_A, 
                CameraRegs::OP_A_DIGITIZATION_RES_BIT);

             // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternFast.Mask);
        break;

        
        case Apg::AdcSpeed_Normal:

             //ensure the number of cols to bin are ok
            if( GetNumCols2Bin() > GetMaxNormalBinCols() )
            {
                std::stringstream msg;
                msg << "Reseting imaging columns to " << GetMaxNormalBinCols();
                msg << " because current bin value of " << GetNumCols2Bin();
                msg << " is not supported in nomral mode.";
                std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
                ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

                SetNumCols2Bin( GetMaxNormalBinCols() );
            }

            // LT: 2012-06-28
            // this bit is not used in alta f, aspen, and new ascents
            // left here because of the v108 ascents may still need it
             m_CamIo->ReadAndWriteReg( CameraRegs::OP_A, 
                    static_cast<uint16_t>(~CameraRegs::OP_A_DIGITIZATION_RES_BIT) );

             // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternNormal.Mask);
        break;

        case Apg::AdcSpeed_Video:
            if( 1 != GetNumCols2Bin() )
            {
                apgHelper::throwRuntimeException( m_fileName, 
                    "Video mode not allowed when column binning is enabled.", 
                    __LINE__, Apg::ErrorType_InvalidMode );
            }

            if( 1 != this->GetNumRows2Bin() )
            {
                 apgHelper::throwRuntimeException( m_fileName, 
                    "Video mode not allowed when row binning is enabled.", 
                    __LINE__, Apg::ErrorType_InvalidMode );
            }

            m_CamIo->ReadAndWriteReg( CameraRegs::OP_A, 
                    static_cast<uint16_t>(~CameraRegs::OP_A_DIGITIZATION_RES_BIT) );

            // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternVideo.Mask);
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid adc speed " << speed;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }


    // 3-13-2012 ge reported images filled with zeros
    // (ticket #125) with the fix for ticker #114 in alta project
    // taking out the roi only loads
    CcdAcqParams::LoadAllPatternFiles(speed, GetNumCols2Bin() );

     // Reset the camera and start flushing
    m_CamIo->Reset( true );

    //store the speed value
    m_speed = speed;
}

//////////////////////////// 
//  SELECT      AD
uint16_t CamGen2CcdAcqParams::SelectAd( int32_t ad, int32_t channel )
{
    CamGen2CcdAcqParams::AdcParams params = GetAdcParams( ad, channel );

    uint16_t orgReg = m_CamIo->ReadMirrorReg( CameraRegs::OP_D );
    uint16_t selectValue = 0;

     switch( ad )
     {
        //channel 1 is the right ad
        default:
        case 1:
            selectValue = orgReg | params.selectMask;
        break;

        //channel 0 is the left ad
        case 0:
            selectValue = orgReg & ~params.selectMask;
        break;
     }

     m_CamIo->WriteReg( CameraRegs::OP_D, selectValue );

     return orgReg;
     
}

//////////////////////////// 
//      RESTORE      AD
void CamGen2CcdAcqParams::RestoreAdSelect( const uint16_t value )
{
    m_CamIo->WriteReg( CameraRegs::OP_D, value );
}

//////////////////////////// 
//      GET    CCD    IMG     ROWS
uint16_t CamGen2CcdAcqParams::GetCcdImgRows()
{
    switch( GetReadoutType() )
    {
        case CamGen2CcdAcqParams::QUAD_READOUT:      
            return m_RoiNumRows / 2;
        break;

        default:
            return m_RoiNumRows;
        break;
    }

}
   
//////////////////////////// 
//      GET    CCD    IMG     COLS
uint16_t CamGen2CcdAcqParams::GetCcdImgCols()
{
    switch( GetReadoutType() )
    {
        case CamGen2CcdAcqParams::DUAL_READOUT:
            return (m_RoiNumCols - GetOddColsAdjust() ) / 2;
        break;

        case CamGen2CcdAcqParams::QUAD_READOUT:      
            return m_RoiNumCols / 2;
        break;

        default:
            return m_RoiNumCols;
        break;
    }
}

//////////////////////////// 
//      GET    CCD    IMG     BIN   ROWS
uint16_t CamGen2CcdAcqParams::GetCcdImgBinRows()
{
    return m_NumRows2Bin;
}

//////////////////////////// 
//      GET    CCD    IMG     BIN COLS
uint16_t CamGen2CcdAcqParams::GetCcdImgBinCols()
{
    return m_NumCols2Bin;
}

//////////////////////////// 
//          GET    TOTAL    CCD    COLS
uint16_t CamGen2CcdAcqParams::GetTotalCcdCols()
{
    switch( GetReadoutType() )
    {
        case CamGen2CcdAcqParams::DUAL_READOUT:
        case CamGen2CcdAcqParams::QUAD_READOUT:      
            return m_CamData->m_MetaData.TotalColumns / 2;
        break;

        default:
            return m_CamData->m_MetaData.TotalColumns;
        break;
    }
}

//////////////////////////// 
//      CALC   H     POST  ROI     SKIP
uint16_t CamGen2CcdAcqParams::CalcHPostRoiSkip(const uint16_t HPreRoiSkip,
                                               const uint16_t UnbinnedRoiCols )
{
    switch( GetReadoutType() )
    {
        case CamGen2CcdAcqParams::DUAL_READOUT:
        {
            const int32_t diff = (m_CamData->m_MetaData.ImagingColumns / 2) - UnbinnedRoiCols;
            return ( diff > 0 ? static_cast<uint16_t>(diff) : 0 );
        }
        break;

        case CamGen2CcdAcqParams::QUAD_READOUT:      
            return 0;
        break;

        default:
            return (m_CamData->m_MetaData.TotalColumns -
					  m_CamData->m_MetaData.ClampColumns - 
                      HPreRoiSkip - 
                      UnbinnedRoiCols);
        break;
    }

}

//////////////////////////// 
//      GET    ODD    COLS      ADJUST
uint16_t CamGen2CcdAcqParams::GetOddColsAdjust()
{
    return (m_RoiNumCols % 2) ? 1 : 0;
}

//////////////////////////// 
//      IS     COL        CALC   GOOD
bool CamGen2CcdAcqParams::IsColCalcGood( const uint16_t UnbinnedRoiCols,
                                     const uint16_t PreRoiSkip, const uint16_t PostRoiSkip)
{
    uint16_t TotalCols = UnbinnedRoiCols + 
        PreRoiSkip + PostRoiSkip + m_CamData->m_MetaData.ClampColumns;

    return ( TotalCols == GetTotalCcdCols() ? true : false );
}

//////////////////////////// 
// GET     PIXEL      SHIFT
uint16_t CamGen2CcdAcqParams::GetPixelShift()
{
    if( IsAdsSimModeOn() )
    {
        return 0;
    }
    else
    {
        return( Apg::AdcSpeed_Fast == m_speed ? 
            m_CamData->m_MetaData.AlternativeADLatency : 
            m_CamData->m_MetaData.PrimaryADLatency );
    }
}

//////////////////////////// 
//       GET        H          PATTERN
CamCfg::APN_HPATTERN_FILE CamGen2CcdAcqParams::GetHPattern( const Apg::AdcSpeed speed,
            const CcdAcqParams::HPatternType ptype )
{
    // if the camera is not an ascent then use the default get pattern file function
    if( CamModel::ASCENT != 
        CamModel::GetPlatformType( m_CamData->m_MetaData.CameraLine ) )
    {
        return DefaultGetHPattern( speed, ptype );
    }

    // if the camera is not in a dual reaout mode then use 
    // the default get pattern file function
    if( CamGen2CcdAcqParams::DUAL_READOUT != GetReadoutType() )
    {
        return DefaultGetHPattern( speed, ptype );
    }

    // get the dual pattern files
    CamCfg::APN_HPATTERN_FILE hpattern;
    switch( speed )
    {
        case Apg::AdcSpeed_Normal:
            switch( ptype )
            {
                case CcdAcqParams::CLAMP:
                    hpattern = m_CamData->m_ClampPatternNormalDual;
                break;

                case CcdAcqParams::SKIP:
                    hpattern = m_CamData->m_SkipPatternNormalDual;
                break;

                case CcdAcqParams::ROI:
                    hpattern = m_CamData->m_RoiPatternNormalDual;
                break;

                default:
                {
                    std::stringstream msg;
                    msg << "Invalid h pattern type " << ptype << " cannot fetch pattern file" ;
                    apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                        __LINE__, Apg::ErrorType_InvalidUsage);
                }
                break;
            }
        break;

        case Apg::AdcSpeed_Fast:
            switch( ptype )
            {
                case CcdAcqParams::CLAMP:
                    hpattern = m_CamData->m_ClampPatternFastDual;
                break;

                case CcdAcqParams::SKIP:
                    hpattern = m_CamData->m_SkipPatternFastDual;
                break;

                case CcdAcqParams::ROI:
                    hpattern = m_CamData->m_RoiPatternFastDual;
                break;

                default:
                {
                    std::stringstream msg;
                    msg << "Invalid h pattern type " << ptype << " cannot fetch pattern file" ;
                    apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                        __LINE__, Apg::ErrorType_InvalidUsage);
                }
                break;
            }
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid adc speed, " << speed << " cannot fetch pattern file" ;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage);
        }
        break;
    }

    return hpattern;

}
