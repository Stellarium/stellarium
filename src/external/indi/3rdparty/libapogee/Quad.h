/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class Quad
* \brief camera class for f4320 for Quad
* 
*/ 


#ifndef QUAD_INCLUDE_H__ 
#define QUAD_INCLUDE_H__ 


#include "CamGen2Base.h" 
#include "CameraInfo.h" 
#include <string>

class DLL_EXPORT Quad: public CamGen2Base
{ 
    public: 
        Quad();

        virtual ~Quad(); 

        void OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id );

        void CloseConnection();

        void StartExposure( double Duration, bool IsLight );

        bool IsPixelReorderOn() { return m_DoPixelReorder; }

        void SetPixelReorder( const bool TurnOn ) { m_DoPixelReorder = TurnOn; }

        int32_t GetNumAdChannels();
		
		void SetIsQuadBit();

        void Init();

        Apg::FanMode GetFanMode();
        void SetFanMode( Apg::FanMode mode, bool PreCondCheck = true );

    protected:
        Quad(const std::string & ioType,
             const std::string & DeviceAddr);
        
        void FixImgFromCamera( const std::vector<uint16_t> & data,
            std::vector<uint16_t> & out,  int32_t rows, int32_t cols );

        void CreateCamIo(const std::string & ioType,
            const std::string & DeviceAddr);

        bool IsRoiCenteredAndSymmetric(uint16_t ccdLen, uint16_t startingPos,  uint16_t roiLen );

        void ExposureAndGetImgRC(uint16_t & r, uint16_t & c);

    private:
        void UpdateCfgWithStrDbInfo();
        void FullCtorInit( const std::string & ioType,
                const std::string & DeviceAddr );

        void CfgCamFromId( uint16_t CameraId );
        void VerifyCamId();

        const std::string m_fileName;
        bool m_DoPixelReorder;

        Quad(const Quad&);
        Quad& operator=(Quad&);
}; 

#endif
