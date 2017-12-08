/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CameraIo 
* \brief Class that contains the camera io interfaces. 
* 
*/ 


#ifndef CAMERAIO_INCLUDE_H__ 
#define CAMERAIO_INCLUDE_H__ 

#include <string>
#include <vector>
#include <map>
#include "CameraStatusRegs.h"
#include "CamCfgMatrix.h"
#include "CameraInfo.h" 
#include "DefDllExport.h"

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

class ICamIo;
class CamRegMirror;


class DLL_EXPORT CameraIo 
{ 
    public: 
       CameraIo( CamModel::InterfaceType type );

        virtual ~CameraIo(); 

        CamModel::InterfaceType GetInterfaceType() const { return m_type; }

        void ClearAllRegisters();
        uint16_t ReadMirrorReg( uint16_t reg ) const;
        uint16_t ReadReg( uint16_t reg ) const;
	    void WriteReg( uint16_t reg, uint16_t val );

        void WriteReg( const std::vector <std::pair<uint16_t,uint16_t> > & RegAndVal );

        void ReadOrWriteReg(uint16_t reg, uint16_t val2Or);
        void ReadAndWriteReg(uint16_t reg, uint16_t val2And);

        void ReadMirrorOrWriteReg(uint16_t reg, uint16_t val2Or);
        void ReadMirrorAndWriteReg(uint16_t reg, uint16_t val2And);

        void GetUsbVendorInfo( uint16_t & VendorId,
            uint16_t & ProductId, uint16_t  & DeviceId);

        void SetupImgXfer(uint16_t Rows, 
            uint16_t Cols, uint16_t NumOfImages, bool IsBulkSeq);

        void CancelImgXfer();
       
        void GetImageData( std::vector<uint16_t> & data );
    
        void GetStatus(CameraStatusRegs::BasicStatus & status);
        void GetStatus(CameraStatusRegs::AdvStatus & status);

        uint16_t GetFirmwareRev();

        void LoadHorizontalPattern(const CamCfg::APN_HPATTERN_FILE & Pattern, 
            const uint16_t MaskingBit, const uint16_t RamReg, 
            const uint16_t Binning );

        void LoadVerticalPattern(const CamCfg::APN_VPATTERN_FILE & Pattern);

        void Reset(bool Flush);

        std::string GetUsbFirmwareVersion();
        std::string GetInfo();

        uint8_t ReadBufConReg( uint16_t reg ) const;
	    void WriteBufConReg( uint16_t reg, uint8_t val );

        uint8_t ReadFx2Reg( uint16_t reg );
        void WriteFx2Reg( uint16_t reg, uint8_t val );

        std::string GetDriverVersion();

        bool IsError();

        std::string GetFirmwareHdr();
        std::string GetSerialNumber();
        void SetSerialNumber( const std::string & num );

        //      PURE      VIRTUAL
        virtual uint16_t GetId() = 0;

    protected:      
        void WriteSRMD( uint16_t reg, const std::vector<uint16_t> & data );
        void WriteMRMD( uint16_t reg, const std::vector<uint16_t> & data );
        uint16_t GetIdFromReg();

        CamModel::InterfaceType m_type;

//this code removes vc++ compiler warning C4251
//from http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
#ifdef WIN_OS
        template class DLL_EXPORT std::shared_ptr<ICamIo>;
        template class DLL_EXPORT std::shared_ptr<CamRegMirror>;
#endif

        std::shared_ptr<ICamIo> m_Interface;
        std::shared_ptr<CamRegMirror> m_RegMirror;

    private:
        std::string m_fileName;

}; 

namespace InterfaceHelper
{
    CamModel::InterfaceType DetermineInterfaceType(const std::string & type);
}


#endif
