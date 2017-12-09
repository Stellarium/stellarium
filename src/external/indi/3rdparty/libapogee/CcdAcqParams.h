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


#ifndef CCDACQPARAMS_INCLUDE_H__ 
#define CCDACQPARAMS_INCLUDE_H__ 

#include "CameraInfo.h" 
#include "CamCfgMatrix.h" 
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

class CameraIo;
class CApnCamData;
class PlatformData;

class CcdAcqParams 
{ 
    public:         
        virtual ~CcdAcqParams(); 

        Apg::Resolution GetResolution() { return m_AdcRes; }
        Apg::AdcSpeed GetSpeed() { return m_speed; }

        void SetRoiStartRow( uint16_t row );
        void SetRoiStartCol( uint16_t col );
        void SetRoiNumRows( uint16_t rows ); 
        void SetRoiNumCols( uint16_t cols );

        uint16_t GetRoiStartRow() { return m_RoiStartRow; }
        uint16_t GetRoiStartCol() { return m_RoiStartCol; }
        uint16_t GetRoiNumRows() { return m_RoiNumRows; }
        uint16_t GetRoiNumCols() { return m_RoiNumCols; }
        uint16_t GetNumCols2Bin() { return m_NumCols2Bin; }
        uint16_t GetNumRows2Bin() { return m_NumRows2Bin; }

        uint16_t GetMaxBinCols();
        uint16_t GetMaxBinRows();

        void SetNumCols2Bin( uint16_t bin );
        void SetNumRows2Bin( uint16_t bin );

        void SetImagingRegs(const uint16_t FirmwareVer);

        void UpdateApnCamData( std::shared_ptr<CApnCamData> & newCamData );

        void SetAdsSimMode(bool TurnOn);
        
        bool IsOverscanDigitized() { return m_DigitizeOverScan; }
        void SetDigitizeOverscan( const bool TurnOn ) { m_DigitizeOverScan = TurnOn; }

         // ****** PURE VIRTUAL INTERFACE ********
        virtual void Init() = 0;

        virtual void SetResolution( Apg::Resolution res ) = 0;
        virtual void SetSpeed( Apg::AdcSpeed speed ) = 0;

        virtual void SetAdcGain( uint16_t gain, int32_t ad, int32_t channel ) = 0;
        virtual uint16_t GetAdcGain( int32_t ad, int32_t channel ) = 0;
        virtual void SetAdcOffset( uint16_t offset, int32_t ad, int32_t channel ) = 0;
        virtual uint16_t GetAdcOffset( int32_t ad, int32_t channel ) = 0;
        virtual bool IsAdsSimModeOn() = 0;
        virtual uint16_t GetPixelShift() = 0;

    protected:
        enum CcdReadoutType
        {
            UNKNOWN_READOUT,
            SINGLE_READOUT,
            DUAL_READOUT,
            QUAD_READOUT
        };
        
        enum HPatternType
        {
            CLAMP,
            SKIP,
            ROI
        };

       CcdAcqParams::CcdReadoutType GetReadoutType();

       CcdAcqParams(std::shared_ptr<CApnCamData> & camData,
            std::shared_ptr<CameraIo> & camIo,
            std::shared_ptr<PlatformData> & platformData);

        void SetRoiPattern( uint16_t binning );

         void LoadHorizontalPatterns( Apg::AdcSpeed speed, 
             uint16_t binning );

        void LoadRoiPattern( Apg::AdcSpeed speed, uint16_t binning );

        CamCfg::APN_HPATTERN_FILE DefaultGetHPattern( Apg::AdcSpeed speed,
            CcdAcqParams::HPatternType ptype );
        /*! 
         * Loads all of the camera pattern files vertical, clamp, skip
         * roi based on the input speed and binning values.
         * param [in] speed
         * param [in]  binning
         */
        void LoadAllPatternFiles( Apg::AdcSpeed speed,
            uint16_t binning);

        void GetPostVer11Settings( std::vector <std::pair<uint16_t,uint16_t> > & settings,
            uint16_t pixelShift);

        void GetPreVer11Settings( std::vector< std::pair<uint16_t,uint16_t> > & settings,
            uint16_t pixelShift);

        void AppendCommonHorizontals( 
            std::vector< std::pair<uint16_t,uint16_t> > & settings,
            uint16_t pixelShift);

        void CalcVerticalValues( uint16_t & A2_RoiRows,
                                     uint16_t & A2_RoiBinRows,
                                     uint16_t & A5_RoiRows,
                                     uint16_t & A5_RoiBinRows );

        void CalcVerticalValues( uint16_t & A1_RoiRows,
                                     uint16_t & A1_RoiBinRows,
                                     uint16_t & A2_RoiRows,
                                     uint16_t & A2_RoiBinRows,
                                     uint16_t & A4_RoiRows, 
                                     uint16_t & A4_RoiBinRows,
                                     uint16_t & A5_RoiRows,
                                     uint16_t & A5_RoiBinRows );


        void BalanceSections( const uint16_t BottomMaxRows, 
                                  const uint16_t TopMaxBin,
                                  uint16_t & TopRoiRows, 
                                  uint16_t & TopRoiBinRows,
                                  uint16_t & BottomRoiRows,
                                  uint16_t & BottomRoiBinRows );

        uint16_t GetMaxFastBinCols();
        uint16_t GetMaxNormalBinCols();

         // ****** PURE VIRTUAL INTERFACE ********
        virtual uint16_t GetCcdImgRows() = 0;
        virtual uint16_t GetCcdImgCols() = 0;
        virtual uint16_t GetCcdImgBinRows() = 0;
        virtual uint16_t GetCcdImgBinCols() = 0;
        virtual uint16_t GetTotalCcdCols() = 0;
        virtual uint16_t CalcHPostRoiSkip(uint16_t HPreRoiSkip,
            uint16_t UnbinnedRoiCols ) = 0;
        virtual bool IsColCalcGood( uint16_t UnbinnedRoiCols, uint16_t PreRoiSkip, 
            uint16_t PostRoiSkip) = 0;
        virtual  CamCfg::APN_HPATTERN_FILE GetHPattern( Apg::AdcSpeed speed,
            CcdAcqParams::HPatternType ptype ) = 0;

        std::string m_fileName;
        std::shared_ptr<CApnCamData> m_CamData;
        std::shared_ptr<CameraIo> m_CamIo;
        std::shared_ptr<PlatformData> m_PlatformData;
        Apg::Resolution m_AdcRes;
        Apg::AdcSpeed m_speed;
        uint16_t m_RoiStartRow;
        uint16_t m_RoiStartCol;
        uint16_t m_RoiNumRows;
        uint16_t m_RoiNumCols;
        uint16_t m_NumCols2Bin;
        uint16_t m_NumRows2Bin;
        bool m_DigitizeOverScan;
    private:
        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        CcdAcqParams(const CcdAcqParams&);
        CcdAcqParams& operator=(CcdAcqParams&);

}; 

#endif
