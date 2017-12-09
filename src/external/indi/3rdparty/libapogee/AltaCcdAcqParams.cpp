/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class AltaCcdAcqParams
* \brief derived class for managing the Alta's ADCs,
*         horizontal pattern files, and roi parameters 
* 
*/ 

#include "AltaCcdAcqParams.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "ApnCamData.h"
#include "CamCfgMatrix.h"

#include <sstream>

namespace
{
    // values for configuring the camera's pixel A to D converter
    const uint16_t AD9842_GAIN_MASK = 0x3FF;
    const uint16_t AD9842_GAIN_BIT = 0x4000;
    const uint16_t AD9842_OFFSET_MASK = 0xFF;
    const uint16_t AD9842_OFFSET_BIT = 0x2000;
    const uint16_t AD9842_RESET = 0x48;
}

//////////////////////////// 
// CTOR 
AltaCcdAcqParams::AltaCcdAcqParams(std::shared_ptr<CApnCamData> & camData,
                           std::shared_ptr<CameraIo> & camIo,  
                           std::shared_ptr<PlatformData> & platformData) : 
                                                                CcdAcqParams(camData,camIo,platformData),
                                                                m_fileName(__FILE__),
                                                                m_Adc12BitGain(0),
                                                                m_Adc12BitOffset(0)
{ 

}

//////////////////////////// 
// DTOR 
AltaCcdAcqParams::~AltaCcdAcqParams() 
{ 

} 

//////////////////////////// 
// INIT
void AltaCcdAcqParams::Init()
{
    if( CamCfg::ApnAdType_Alta_Twelve == m_CamData->m_MetaData.AlternativeADType )
    {
        PrimeAdc();

        Set12BitGain(  
            static_cast<uint16_t>(m_CamData->m_MetaData.DefaultGainLeft) );
        
        Set12BitOffset( 
            static_cast<uint16_t>(m_CamData->m_MetaData.DefaultOffsetLeft) );
    }

    SetSpeed( Apg::AdcSpeed_Normal );
}

//////////////////////////// 
// PRIME    ADC
void AltaCcdAcqParams::PrimeAdc()
{
    //reseting the ADC - john remmington fix
    m_CamIo->WriteReg( CameraRegs::AD_CONFIG_DATA, AD9842_RESET );
	m_CamIo->WriteReg( CameraRegs::CMD_B, CameraRegs::CMD_B_AD_CONFIG_BIT );

    m_CamIo->WriteReg( CameraRegs::AD_CONFIG_DATA,  m_CamData->m_MetaData.AdCfg );
    m_CamIo->WriteReg( CameraRegs::CMD_B, CameraRegs::CMD_B_AD_CONFIG_BIT );
}

//////////////////////////// 
//  SET   ADC      RESOLUTION
void AltaCcdAcqParams::SetResolution( 
            const Apg::Resolution res )
{
    switch( res )
    {
        case Apg::Resolution_SixteenBit:
            SetSpeed( Apg::AdcSpeed_Normal );
        break;

        case Apg::Resolution_TwelveBit:
          SetSpeed( Apg::AdcSpeed_Fast );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid adc resolution, " << res;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

}

//////////////////////////// 
//  SET   ADC      SPEED
void AltaCcdAcqParams::SetSpeed( const Apg::AdcSpeed speed )
{
    // Reset the camera
    m_CamIo->Reset( false );

    switch( speed )
    {
        case Apg::AdcSpeed_Fast:
            //checking pre-conditions
            if( CamCfg::ApnAdType_Alta_Twelve != m_CamData->m_MetaData.AlternativeADType)
            {
                std::stringstream msg;
                msg << "Invalid adc type " << m_CamData->m_MetaData.AlternativeADType;
                msg << " for 12bit conversion.";
                apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                    __LINE__, Apg::ErrorType_InvalidOperation );
            }

            if( CamModel::ETHERNET == m_CamIo->GetInterfaceType() )
            {
                 std::string vinfo = apgHelper::mkMsg( m_fileName, 
                    "Apg::Resolution_TwelveBit not supported on alta ethernet interface.", 
                    __LINE__);
                ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

                //exit here...
                return;
            }

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

            m_CamIo->ReadOrWriteReg(CameraRegs::OP_A, 
                CameraRegs::OP_A_DIGITIZATION_RES_BIT);
        break;

        case Apg::AdcSpeed_Normal:
            //checking pre-conditions
            if( CamCfg::ApnAdType_Alta_Sixteen != m_CamData->m_MetaData.PrimaryADType )
            {
                std::stringstream msg;
                msg << "Invalid adc type " << m_CamData->m_MetaData.PrimaryADType;
                msg << " for 16bit conversion.";
                apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                    __LINE__, Apg::ErrorType_InvalidOperation );
            }

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

            m_CamIo->ReadAndWriteReg( CameraRegs::OP_A, 
                    static_cast<uint16_t>(~CameraRegs::OP_A_DIGITIZATION_RES_BIT) );
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

    // reload all of the pattern files for given speed and binning
    // not 100% sure if we still need to this, because we are only
    // loading the roi for the given speed in gen2 cameras (Ascent, AltaF, AltaG)
    // but this was what was done before so not going to change it now
    CcdAcqParams::LoadAllPatternFiles( speed, GetNumCols2Bin() );

     // Reset the camera and start flushing
    m_CamIo->Reset( true );

    //store the speed value
    m_speed = speed;

    //resoultion and speed are tied together
    m_AdcRes = ( Apg::AdcSpeed_Fast == speed ?  
        Apg::Resolution_TwelveBit : Apg::Resolution_SixteenBit);
}

//////////////////////////// 
// SET   12BIT   GAIN
void AltaCcdAcqParams::Set12BitGain( const uint16_t gain ) 
{
    uint16_t NewVal = 0x0;
    uint16_t StartVal = gain & AD9842_GAIN_MASK;
    uint16_t FirstBit = 0;

    for ( int32_t i=0; i<10; ++i )
    {
	    FirstBit = ( StartVal & 0x0001 );
	    NewVal |= ( FirstBit << (10-i) );
	    StartVal = StartVal >> 1;
    }

    NewVal |= AD9842_GAIN_BIT;

    m_CamIo->WriteReg( CameraRegs::AD_CONFIG_DATA, NewVal );
    m_CamIo->WriteReg( CameraRegs::CMD_B, 0x8000 );

    m_Adc12BitGain = gain & AD9842_GAIN_MASK;
}


//////////////////////////// 
// SET   12BIT   OFFSET
void AltaCcdAcqParams::Set12BitOffset( const uint16_t offset )
{
    uint16_t NewVal = 0x0;
    uint16_t StartVal = offset & AD9842_OFFSET_MASK;
    uint16_t FirstBit = 0;

    for ( int32_t i=0; i<8; i++ )
    {
	    FirstBit = ( StartVal & 0x0001 );
	    NewVal |= ( FirstBit << (10-i) );
	    StartVal = StartVal >> 1;
    }

    NewVal |= AD9842_OFFSET_BIT;

    m_CamIo->WriteReg( CameraRegs::AD_CONFIG_DATA, NewVal );
    m_CamIo->WriteReg( CameraRegs::CMD_B, CameraRegs::CMD_B_AD_CONFIG_BIT);

    m_Adc12BitOffset = offset;
}

//////////////////////////// 
// GET  16BIT      GAIN
double AltaCcdAcqParams::Get16bitGain()
{
    return m_CamData->m_MetaData.ReportedGainSixteenBit;
}

//////////////////////////// 
// IS      ADS        SIM     MODE         ON    
bool AltaCcdAcqParams::IsAdsSimModeOn()
{
    //reading from the mirror, b/c the registers cannot always be read
    return( (m_CamIo->ReadMirrorReg(CameraRegs::OP_B) & CameraRegs::OP_B_AD_SIMULATION_BIT) ? true : false);
}

//////////////////////////// 
//      SET     ADC        GAIN 
void AltaCcdAcqParams::SetAdcGain( uint16_t gain, int32_t ad, int32_t channel )
{
    //channel is a no op on altas
    switch( ad )
    {
        case 1:
            return this->Set12BitGain( gain );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Cannot SetAdcGain invalid adc value " << ad;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }    
        break;
    }
}

//////////////////////////// 
//      GET     ADC        GAIN 
uint16_t AltaCcdAcqParams::GetAdcGain( const int32_t ad, const  int32_t channel )
{
    //channel is a no op on altas
    switch( ad )
    {
        case 0:
            return static_cast<uint16_t>( Get16bitGain() );
        break;

        case 1:
            return Get12BitGain();
        break;

        default:
        {
            std::stringstream msg;
            msg << "Cannot GetAdcGain invalid adc value " << ad;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }    
        break;
    }

    //should never reach here
    return 0;
}

//////////////////////////// 
//      SET     ADC        OFFSET
void AltaCcdAcqParams::SetAdcOffset( uint16_t offset, int32_t ad, int32_t channel )
{
    //channel is a no op on altas
    switch( ad )
    {
        case 1:
            return Set12BitOffset( offset );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Cannot SetAdcOffset invalid adc value " << ad;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }    
        break;
    }
}

//////////////////////////// 
//      GET     ADC        OFFSET
uint16_t AltaCcdAcqParams::GetAdcOffset( int32_t ad, int32_t channel )
{
    //channel is a no op on altas
    switch( ad )
    {
        case 1:
            return Get12BitOffset();
        break;

        default:
        {
            std::stringstream msg;
            msg << "Cannot GetAdcOffset invalid adc value " << ad;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }    
        break;
    }

    //should never reach here
    return 0;
}

//////////////////////////// 
//          GET    TOTAL    CCD    COLS
uint16_t AltaCcdAcqParams::GetTotalCcdCols()
{
    return m_CamData->m_MetaData.TotalColumns;
}

//////////////////////////// 
//      CALC   H     POST  ROI     SKIP
uint16_t AltaCcdAcqParams::CalcHPostRoiSkip(const uint16_t HPreRoiSkip,
                                               const uint16_t UnbinnedRoiCols )
{
    return (m_CamData->m_MetaData.TotalColumns -
		      m_CamData->m_MetaData.ClampColumns - 
              HPreRoiSkip - 
              UnbinnedRoiCols);
}

//////////////////////////// 
//      IS     COL        CALC   GOOD
bool AltaCcdAcqParams::IsColCalcGood( const uint16_t UnbinnedRoiCols,
                                     const uint16_t PreRoiSkip, const uint16_t PostRoiSkip)
{
    const uint16_t TotalCols = UnbinnedRoiCols + 
        PreRoiSkip + PostRoiSkip + m_CamData->m_MetaData.ClampColumns;

    return ( TotalCols == GetTotalCcdCols() ? true : false );
}

//////////////////////////// 
// GET     PIXEL      SHIFT
uint16_t AltaCcdAcqParams::GetPixelShift()
{
    if( IsAdsSimModeOn() )
    {
        return 0;
    }
    else
    {
        return( Apg::Resolution_TwelveBit == m_AdcRes ? 
            m_CamData->m_MetaData.AlternativeADLatency : 
            m_CamData->m_MetaData.PrimaryADLatency);
    }
}

//////////////////////////// 
//       GET        H          PATTERN
CamCfg::APN_HPATTERN_FILE AltaCcdAcqParams::GetHPattern( const Apg::AdcSpeed speed,
            const CcdAcqParams::HPatternType ptype )
{
    return DefaultGetHPattern( speed, ptype );
}
