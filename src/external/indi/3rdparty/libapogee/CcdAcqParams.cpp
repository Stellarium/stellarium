/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CcdAcqParams
* \brief base class for managing the camera's ADCs, pattern files, and roi parmeters
* 
*/ 

#include "CcdAcqParams.h" 
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "ApnCamData.h"
#include "apgHelper.h" 
#include "PlatformData.h" 
#include <sstream>

namespace
{
    const uint16_t FASTDUMP_ARRAY_BIT = 0x4000;
    const uint16_t ARRAY_DIGITIZE_BIT = 0x1000;
}

//////////////////////////// 
// CTOR 
CcdAcqParams::CcdAcqParams(std::shared_ptr<CApnCamData> & camData,
                           std::shared_ptr<CameraIo> & camIo,  
                           std::shared_ptr<PlatformData> & platformData) : 
                                                    m_fileName( __FILE__ ),
                                                    m_CamData(camData),
                                                    m_CamIo(camIo),
                                                    m_PlatformData(platformData),
                                                    m_AdcRes(Apg::Resolution_SixteenBit),
                                                    m_speed(Apg::AdcSpeed_Normal),
                                                    m_RoiStartRow(0),
                                                    m_RoiStartCol(0),
                                                    m_RoiNumRows(0),
                                                    m_RoiNumCols(0),
                                                    m_NumCols2Bin( 1 ),
                                                    m_NumRows2Bin( 1 )
{ 
    m_RoiNumRows = m_CamData->m_MetaData.ImagingRows;
    m_RoiNumCols = m_CamData->m_MetaData.ImagingColumns;
    m_RoiStartCol = m_CamData->m_MetaData.PreRoiSkipColumns;
} 

//////////////////////////// 
// DTOR 
CcdAcqParams::~CcdAcqParams() 
{ 

} 

//////////////////////////// 
// LOAD    HORIZONTAL     PATTERNS
void CcdAcqParams::LoadHorizontalPatterns( const Apg::AdcSpeed speed, 
                                          const uint16_t binning )
{
    //load the clamp
    m_CamIo->LoadHorizontalPattern(
        GetHPattern( speed, CcdAcqParams::CLAMP ), 
        CameraRegs::OP_B_HCLAMP_ENABLE_BIT,
        CameraRegs::HCLAMP_INPUT, 
        1);

    //load the skip
    m_CamIo->LoadHorizontalPattern(
        GetHPattern( speed, CcdAcqParams::SKIP ), 
        CameraRegs::OP_B_HSKIP_ENABLE_BIT,
        CameraRegs::HSKIP_INPUT, 
        1);

    //load the ROI
    LoadRoiPattern( speed, binning );
}

//////////////////////////// 
// LOAD      ROI     PATTERNS
void CcdAcqParams::LoadRoiPattern( const Apg::AdcSpeed speed,
                                        const uint16_t binning )
{
    m_CamIo->LoadHorizontalPattern(
        GetHPattern( speed, CcdAcqParams::ROI ), 
        CameraRegs::OP_B_HRAM_ENABLE_BIT,
        CameraRegs::HRAM_INPUT, 
        binning);
}

//////////////////////////// 
//      DEFAULT       GET        H          PATTERN
CamCfg::APN_HPATTERN_FILE CcdAcqParams::DefaultGetHPattern( const Apg::AdcSpeed speed,
            const CcdAcqParams::HPatternType ptype )
{
    CamCfg::APN_HPATTERN_FILE hpattern;
    switch( speed )
    {
        case Apg::AdcSpeed_Normal:
            switch( ptype )
            {
                case CcdAcqParams::CLAMP:
                    hpattern = m_CamData->m_ClampPatternNormal;
                break;

                case CcdAcqParams::SKIP:
                    hpattern = m_CamData->m_SkipPatternNormal;
                break;

                case CcdAcqParams::ROI:
                    hpattern = m_CamData->m_RoiPatternNormal;
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
                    hpattern = m_CamData->m_ClampPatternFast;
                break;

                case CcdAcqParams::SKIP:
                    hpattern = m_CamData->m_SkipPatternFast;
                break;

                case CcdAcqParams::ROI:
                    hpattern = m_CamData->m_RoiPatternFast;
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

        case Apg::AdcSpeed_Video:
            switch( ptype )
            {
                case CcdAcqParams::CLAMP:
                    hpattern = m_CamData->m_ClampPatternVideo;
                break;

                case CcdAcqParams::SKIP:
                    hpattern = m_CamData->m_SkipPatternVideo;
                break;

                case CcdAcqParams::ROI:
                    hpattern = m_CamData->m_RoiPatternVideo;
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

//////////////////////////// 
//  SET      ROI      PATTERN
void CcdAcqParams::SetRoiPattern( const uint16_t binning )
{
    LoadRoiPattern( m_speed, binning );
}

//////////////////////////// 
//  LOAD      ALL      PATTERN          FILES
void CcdAcqParams::LoadAllPatternFiles( 
            const Apg::AdcSpeed speed,
            const uint16_t binning)
{

    switch( speed )
    {
        case Apg::AdcSpeed_Normal:
             m_CamIo->LoadVerticalPattern( m_CamData->m_VerticalPattern );

             // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternNormal.Mask);

            LoadHorizontalPatterns( speed, binning );
        break;

        case Apg::AdcSpeed_Fast:
            m_CamIo->LoadVerticalPattern( m_CamData->m_VerticalPattern );

             // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternFast.Mask);

            LoadHorizontalPatterns( speed, binning );
        break;

        case Apg::AdcSpeed_Video:
             m_CamIo->LoadVerticalPattern( m_CamData->m_VerticalPatternVideo );

             // Load Inversion Mask
            m_CamIo->WriteReg(CameraRegs::HRAM_INV_MASK,
                m_CamData->m_RoiPatternVideo.Mask);

            LoadHorizontalPatterns( speed, binning );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid adc speed, " << speed;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }
}

//////////////////////////// 
//  SET     ROI      START        ROW
void CcdAcqParams::SetRoiStartRow( const uint16_t row )
{
     if( row > m_CamData->m_MetaData.TotalRows )
    {
        std::stringstream msg;
        msg << "Invalid start roi row " << row;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    m_RoiStartRow = row;
}

//////////////////////////// 
//  SET     ROI      START        COL
void CcdAcqParams::SetRoiStartCol( const uint16_t col )
{
    if( col > m_CamData->m_MetaData.TotalColumns )
    {
        std::stringstream msg;
        msg << "Invalid start roi column " << col;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage);
    }

    m_RoiStartCol = col;
}
 
//////////////////////////// 
//  SET     ROI     NUM    ROWS
void CcdAcqParams::SetRoiNumRows( const uint16_t rows )
{
    //boundary checking
    //total - using the looser restriction for now
    if( 0 == rows || rows > m_CamData->m_MetaData.TotalRows )
    {
        std::stringstream msg;
        msg << "Invalid number of roi rows " << rows;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    m_RoiNumRows = rows;
}

//////////////////////////// 
//  SET     ROI     NUM    COLS
void CcdAcqParams::SetRoiNumCols( const uint16_t cols )
{
    //boundary checking
    //total - using the looser restriction for now
    if( 0 == cols || cols > m_CamData->m_MetaData.TotalColumns )
    {
        std::stringstream msg;
        msg << "Invalid number of roi columns " << cols;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    m_RoiNumCols = cols;
}


//////////////////////////// 
//  GET    MAX    BIN      COLS
uint16_t CcdAcqParams::GetMaxBinCols()
{
    //max number of binning columns is the number
    //of roi binning patterns, this number may differ
    //based on adc speed and ccd readout;
    if( CcdAcqParams::QUAD_READOUT == GetReadoutType() )
    {
        return 1;
    }

    uint16_t  max = 1;
    switch( m_speed )
    {
        case Apg::AdcSpeed_Fast:
            max = GetMaxFastBinCols();
        break;

        case Apg::AdcSpeed_Normal: 
            max = GetMaxNormalBinCols();
        break;

        default:
            max = 1;
        break;
    }

    return max;
}
 
//////////////////////////// 
//  GET    MAX    BIN      ROWS
uint16_t CcdAcqParams::GetMaxBinRows()
{
    if( CcdAcqParams::QUAD_READOUT == GetReadoutType() )
    {
        return 1;
    }

    uint16_t  max = 1;

    if(  Apg::AdcSpeed_Video != m_speed )
    {
        max = m_CamData->m_MetaData.ImagingRows <= m_PlatformData->m_NumRows2BinMax ?
        m_CamData->m_MetaData.ImagingRows : m_PlatformData->m_NumRows2BinMax;
    }

    return max;
        
}

//////////////////////////// 
//  SET     NUM    COLS  2  BIN
void CcdAcqParams::SetNumCols2Bin( const uint16_t bin )
{
    //this guard is important, becuase we don't want to
    //call reset on the camera if we don't have too, especially
    //on ascents...see ticket 94 in the alta project for why
    if( bin == GetNumCols2Bin() )
    {
        //exit, do have to change anything
        return;
    }

    if( 0 == bin )
    {
        std::stringstream msg;
        msg << "Invalid number of columns to bin " << bin;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    if( bin > GetMaxBinCols() )
    {
        std::stringstream msg;
        msg << "Invalid number of columns to bin " << bin;
        msg << " . Maximum value = " << GetMaxBinCols();
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    if(  Apg::AdcSpeed_Video == m_speed )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "Binning not allowed in Video mode", 
            __LINE__, Apg::ErrorType_InvalidMode);
    }

    if( CcdAcqParams::QUAD_READOUT == GetReadoutType() )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "Binning not allowed for quad readout ccds.", 
            __LINE__, Apg::ErrorType_InvalidMode);
    }

    m_CamIo->Reset( false );
    SetRoiPattern( bin );
    m_CamIo->Reset( true );

    m_NumCols2Bin = bin;
}

//////////////////////////// 
//  SET     NUM    ROWS  2  BIN
void CcdAcqParams::SetNumRows2Bin( const uint16_t bin )
{
    //this guard is important, becuase we don't want to
    //call reset on the camera if we don't have too, especially
    //on ascents...see ticket 94 in the alta project for why
     if( bin == GetNumRows2Bin() )
    {
        //exit, do have to change anything
        return;
    }

    if( 0 == bin )
    {
        std::stringstream msg;
        msg << "Invalid number of columns to bin " << bin;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    if( bin > GetMaxBinRows() )
    {
        std::stringstream msg;
        msg << "Invalid number of rows to bin " << bin;
        msg << " . Maximum value = " << GetMaxBinRows();
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    if(  Apg::AdcSpeed_Video == m_speed )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "Binning not allowed in Video mode", 
            __LINE__, Apg::ErrorType_InvalidMode);
    }

    if( CcdAcqParams::QUAD_READOUT == GetReadoutType() )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "Binning not allowed for quad readout ccds.", 
            __LINE__, Apg::ErrorType_InvalidMode);
    }

    //set in camera before exposure
    //just store it for now
    m_NumRows2Bin = bin;
}



//////////////////////////// 
// SET    IMAGING      REGS
void CcdAcqParams::SetImagingRegs(const uint16_t FirmwareVer)
{
    std::vector< std::pair<uint16_t,uint16_t> > roiRegValues;

    if( FirmwareVer < 11)
    {
        GetPreVer11Settings( roiRegValues, GetPixelShift() );
    }
    else
    {
         GetPostVer11Settings( roiRegValues, GetPixelShift() );
    }

    m_CamIo->WriteReg( roiRegValues );
}

//////////////////////////// 
// GET       SETTINGS
void CcdAcqParams::GetPostVer11Settings( 
                                       std::vector< std::pair<uint16_t,uint16_t> > & settings,
                                       const uint16_t pixelShift)
{
    settings.clear();

    AppendCommonHorizontals( settings, pixelShift );
    
    uint16_t A1Rows  = 0, A1BinRows = 0, A2Rows  = 0, A2BinRows  = 0;
    uint16_t A4Rows  = 0, A4BinRows = 0, A5Rows  = 0, A5BinRows  = 0;

   if( m_CamData->m_MetaData.EnableSingleRowOffset )
   {
         CalcVerticalValues( A2Rows, A2BinRows, A5Rows, A5BinRows );
   }
   else
   {
        CalcVerticalValues( A1Rows, A1BinRows, A2Rows, A2BinRows,
                                        A4Rows, A4BinRows, A5Rows, A5BinRows );
   }

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A1_ROW_COUNT, A1Rows) );
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A1_VBINNING, A1BinRows));

     settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A2_ROW_COUNT, A2Rows) );
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A2_VBINNING, A2BinRows));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A3_ROW_COUNT, GetCcdImgRows() ));
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A3_VBINNING,
        (GetCcdImgBinRows() | ARRAY_DIGITIZE_BIT)));
    
      settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A4_ROW_COUNT, A4Rows) );
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A4_VBINNING, A4BinRows));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A5_ROW_COUNT,A5Rows));
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A5_VBINNING,A5BinRows));

}

//////////////////////////// 
// GET       SETTINGS
void CcdAcqParams::GetPreVer11Settings( 
                                      std::vector< std::pair<uint16_t,uint16_t> > & settings,
                                      const uint16_t pixelShift)
{
    settings.clear();

    AppendCommonHorizontals( settings, pixelShift );
    
    uint16_t A1Rows  = 0, A1BinRows = 0; 
    uint16_t A3Rows  = 0, A3BinRows  = 0;

    CalcVerticalValues( A1Rows, A1BinRows, 
        A3Rows, A3BinRows );

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A1_ROW_COUNT, A1Rows) );

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A1_VBINNING, A1BinRows));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A2_ROW_COUNT, GetCcdImgRows() ));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A2_VBINNING,
        (GetCcdImgBinRows() | ARRAY_DIGITIZE_BIT)));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A3_ROW_COUNT,A3Rows));

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::A3_VBINNING,A3BinRows));
}


//////////////////////////// 
// APPEND     COMMON      HORIZONTALS
void CcdAcqParams::AppendCommonHorizontals( 
    std::vector< std::pair<uint16_t,uint16_t> > & settings,
    const uint16_t pixelShift)
{

    // Validate the ROI params
    const uint16_t UnbinnedRoiCols	= GetCcdImgCols() * GetCcdImgBinCols();

    const uint16_t PreRoiSkip = m_RoiStartCol;


    const uint16_t PostRoiSkip = CalcHPostRoiSkip( PreRoiSkip, UnbinnedRoiCols );


	if ( !IsColCalcGood( UnbinnedRoiCols, PreRoiSkip, PostRoiSkip) )
	{
		std::stringstream msg;
        msg << "Invalid calculated number of ccd cols ";
        msg << ".  Max number of cols is " << GetTotalCcdCols() << ".";
        apgHelper::throwRuntimeException( m_fileName, 
            msg.str(), __LINE__, Apg::ErrorType_InvalidUsage );
	}
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::PREROI_SKIP_COUNT, PreRoiSkip) );	
    
    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::ROI_COUNT,GetCcdImgCols()+pixelShift) );	

    settings.push_back( 
        std::pair<uint16_t,uint16_t>(
        CameraRegs::POSTROI_SKIP_COUNT, PostRoiSkip) );		
}

//////////////////////////// 
// CALC      VERTICAL      VALUES
void CcdAcqParams::CalcVerticalValues( uint16_t & A2_RoiRows,
                                    uint16_t & A2_RoiBinRows,
                                    uint16_t & A5_RoiRows, 
                                    uint16_t & A5_RoiBinRows )
{
	uint16_t UnbinnedRoiRows	= GetCcdImgRows() * GetCcdImgBinRows();

	A2_RoiRows =  m_CamData->m_MetaData.UnderscanRows + m_RoiStartRow;

	A5_RoiRows = m_CamData->m_MetaData.TotalRows - A2_RoiRows - UnbinnedRoiRows;

	const uint16_t TotalNumRows = A5_RoiRows + A2_RoiRows + UnbinnedRoiRows;

	if ( TotalNumRows != m_CamData->m_MetaData.TotalRows )
	{
        std::stringstream msg;
        msg << "Invalid calculated number of ccd rows " <<  TotalNumRows;
        msg << ".  Max number of rows is " << m_CamData->m_MetaData.TotalRows << ".";
        apgHelper::throwRuntimeException( m_fileName, 
            msg.str(), __LINE__, Apg::ErrorType_InvalidUsage );
	}

    A2_RoiBinRows = m_CamData->m_MetaData.RowOffsetBinning;
    A5_RoiBinRows = 1;
    
    if( m_CamData->m_MetaData.EnableSingleRowOffset )
    {
        A2_RoiBinRows = A2_RoiRows |  FASTDUMP_ARRAY_BIT;
        A5_RoiBinRows = A5_RoiRows |  FASTDUMP_ARRAY_BIT;

        A2_RoiRows = 1;
        A5_RoiRows= 1;
    }

}

//////////////////////////// 
// CALC      VERTICAL      VALUES
void CcdAcqParams::CalcVerticalValues( uint16_t & A1_RoiRows,
                                     uint16_t & A1_RoiBinRows,
                                     uint16_t & A2_RoiRows,
                                     uint16_t & A2_RoiBinRows,
                                     uint16_t & A4_RoiRows, 
                                     uint16_t & A4_RoiBinRows,
                                     uint16_t & A5_RoiRows,
                                     uint16_t & A5_RoiBinRows )
{
    
    CalcVerticalValues( A2_RoiRows, A2_RoiBinRows,
                             A5_RoiRows, A5_RoiBinRows );
    A1_RoiRows = 0;
    A1_RoiBinRows = 0;
    A4_RoiRows = 0;
    A4_RoiBinRows = 0;

    const uint16_t MAX_A2_A5_ROI_ROWS = 70;

    if ( A2_RoiRows > MAX_A2_A5_ROI_ROWS  )
	{
        // updated this algorithm for ticket #87
        // the sum total of A1 and A2 rows need to equal the PreRoiRows value
        // ( A1_Rows * A1_Num_Bins ) + ( A2_Rows * A2_Num_Bins ) = PreRoiRows
        // Because we don't want to dump a lot of charge in to the area before the
        // true image roi (A3) the bin value of A2 will always equal 1.
        // thus ( A1_Rows * A1_Num_Bins ) + A2_Rows = PreRoiRows
        const uint16_t A1_ROI_BIN_MAX = 50;
        BalanceSections( MAX_A2_A5_ROI_ROWS, 
                         A1_ROI_BIN_MAX, 
                         A1_RoiRows, 
                         A1_RoiBinRows, 
                         A2_RoiRows, 
                         A2_RoiBinRows );       
    }

    if ( A5_RoiRows > MAX_A2_A5_ROI_ROWS )
    {

        //this max determined via trial and error
        //for ticket #107
        const uint16_t A4_ROI_BIN_MAX = 2048;

        BalanceSections( MAX_A2_A5_ROI_ROWS, 
                         A4_ROI_BIN_MAX, 
                         A4_RoiRows, 
                         A4_RoiBinRows, 
                         A5_RoiRows, 
                         A5_RoiBinRows );
    }

}

//////////////////////////// 
//      BALANCE       SECTIONS
void CcdAcqParams::BalanceSections( const uint16_t BottomMaxRows, 
                                  const uint16_t TopMaxBin,
                                  uint16_t & TopRoiRows, 
                                  uint16_t & TopRoiBinRows,
                                  uint16_t & BottomRoiRows,
                                  uint16_t & BottomRoiBinRows )
{
    const uint16_t NUM_TOP_ROWS = (BottomRoiRows - BottomMaxRows);

    if( NUM_TOP_ROWS < TopMaxBin )
    {
        TopRoiRows = 1;
        TopRoiBinRows = NUM_TOP_ROWS;
        BottomRoiRows = BottomMaxRows;
        BottomRoiBinRows = 1;
    }
    else
    {
        TopRoiRows = NUM_TOP_ROWS / TopMaxBin;
        TopRoiBinRows = TopMaxBin;
        BottomRoiRows = BottomMaxRows + 
            ( NUM_TOP_ROWS % TopMaxBin );  // sop up extra rows in the bottom section 
        BottomRoiBinRows = 1;
    }

}

//////////////////////////// 
// UPDATE      APNCAM        DATA
void CcdAcqParams::UpdateApnCamData( std::shared_ptr<CApnCamData> & newCamData )
{
    m_CamData = newCamData;
}


//////////////////////////// 
// SET      ADS        SIM     MODE
void CcdAcqParams::SetAdsSimMode(const bool TurnOn)
{
    if( TurnOn )
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
            CameraRegs::OP_B_AD_SIMULATION_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
            static_cast<uint16_t>(~CameraRegs::OP_B_AD_SIMULATION_BIT) );
    }

}

//////////////////////////// 
//  GET    READOUT   TYPE
CcdAcqParams::CcdReadoutType CcdAcqParams::GetReadoutType()
{
    CcdAcqParams::CcdReadoutType type = CcdAcqParams::UNKNOWN_READOUT;
    switch( m_CamData->m_MetaData.NumAdOutputs )
    {
        case 1:
            type = CcdAcqParams::SINGLE_READOUT;
        break;

        case 2:
            type = CcdAcqParams::DUAL_READOUT;
        break;

        case 4:
            type = CcdAcqParams::QUAD_READOUT;
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid num ccd outputs " << m_CamData->m_MetaData.NumAdOutputs;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    return type;
}

//////////////////////////// 
//  GET     MAX    FAST       BIN      COLS
uint16_t CcdAcqParams::GetMaxFastBinCols()
{
    return static_cast<uint16_t>( m_CamData->m_RoiPatternFast.BinPatternData.size() );
}

//////////////////////////// 
//  GET     MAX    NORMAL       BIN      COLS
uint16_t CcdAcqParams::GetMaxNormalBinCols()
{
    return static_cast<uint16_t>( m_CamData->m_RoiPatternNormal.BinPatternData.size() );
}
