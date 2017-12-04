/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AltaIo 
* \brief derived class for alta u and e camera io 
* 
*/ 


#ifndef ALTAIO_INCLUDE_H__ 
#define ALTAIO_INCLUDE_H__ 

#include "CameraIo.h" 
#include <string>

class DLL_EXPORT AltaIo : public CameraIo
{ 
    public: 

        AltaIo( CamModel::InterfaceType type, 
               const std::string & deviceAddr );
        virtual ~AltaIo(); 

        void Program( const std::string & FilenameCamCon,
            const std::string & FilenameBufCon, const std::string & FilenameFx2,
            const std::string & FilenameGpifCamCon,const std::string & FilenameGpifBufCon,
            const std::string & FilenameGpifFifo, bool Print2StdOut=false );

        uint16_t GetId();

        std::string GetMacAddress();

        void SetSerialBaudRate( uint16_t PortId , uint32_t BaudRate );

        uint32_t GetSerialBaudRate(  uint16_t PortId  );

        Apg::SerialFC GetSerialFlowControl( uint16_t PortId );

        void SetSerialFlowControl( uint16_t PortId, 
            Apg::SerialFC FlowControl );

         Apg::SerialParity GetSerialParity( uint16_t PortId );
        
        void SetSerialParity( uint16_t PortId, Apg::SerialParity Parity );
        
        void ReadSerial( uint16_t PortId, std::string & buffer );
        
        void WriteSerial( uint16_t PortId, const std::string & buffer );

    private:
        void VerifyPortIdGood(  uint16_t PortId );

        std::string m_fileName;
}; 

#endif
