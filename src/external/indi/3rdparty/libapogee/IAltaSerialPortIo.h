/*! 
* 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* 
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* 
* \class IAltaSerialPortIo 
* \brief virtual interface for the AltaU/E serial port access 
* 
*/ 


#ifndef IALTASERIALPORTIO_INCLUDE_H__ 
#define IALTASERIALPORTIO_INCLUDE_H__ 

#include <stdint.h>
#include <string>
#include "CameraInfo.h" 

class IAltaSerialPortIo 
{ 
    public: 
        virtual ~IAltaSerialPortIo() = 0;
        
        virtual void SetSerialBaudRate( uint16_t PortId , uint32_t BaudRate ) = 0;

        virtual uint32_t GetSerialBaudRate(  uint16_t PortId  ) = 0;

        virtual Apg::SerialFC GetSerialFlowControl( uint16_t PortId ) = 0;

        virtual void SetSerialFlowControl( uint16_t PortId, 
            Apg::SerialFC FlowControl ) = 0;

        virtual  Apg::SerialParity GetSerialParity( uint16_t PortId ) = 0;
        
        virtual void SetSerialParity( uint16_t PortId, Apg::SerialParity Parity ) = 0;
        
        virtual void ReadSerial( uint16_t PortId, std::string & buffer ) = 0;
        
        virtual void WriteSerial( uint16_t PortId, const std::string & buffer ) = 0;
}; 

#endif
