/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
*
* \class Alta 
* \brief Derived class for the alta apogee cameras 
* 
*/ 

#ifndef ALTA_INCLUDE_H__ 
#define ALTA_INCLUDE_H__ 

#include <string>
#include <map>
#include "ApogeeCam.h" 


class DLL_EXPORT Alta : public ApogeeCam
{ 
    public: 
        Alta();

        virtual ~Alta(); 

        void OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id );

        void CloseConnection();

        void Init();

        void StartExposure( double Duration, bool IsLight );

        CameraStatusRegs GetStatus();
        Apg::Status GetImagingStatus();
      
        void GetImage( std::vector<uint16_t> & out );

        void StopExposure( bool Digitize );

        uint32_t GetAvailableMemory();

        /*! 
         * Sets the analog to digital converter gain value for the 12 bit ADC.
         * \param [in] gain The new gain value. 0-1023 is a valid range.
         * Calling ApogeeCam::SetAdcGain( gain, 1, 0 ) is equivalent to this function
         * and is recommended for use.
         * \exception std::runtime_error
         */
        void SetCcdAdc12BitGain( uint16_t gain );

         /*! 
         * Sets the analog to digital converter offset vaule for the 12 bit ADC.
         * \param [in] gain The new offset value. 0-255 is a valid range. 
         * Calling ApogeeCam::SetAdcOffset( offset, 1, 0 ) is equivalent to this function
         * and is recommended for use.
         * \exception std::runtime_error
         */
        void SetCcdAdc12BitOffset( uint16_t offset );    

        /*! 
         * Returns the analog to digital converter gain value for the 12 bit ADC.
         * Calling ApogeeCam::GetAdcGain( 1, 0 ) is equivalent to this function
         * and is recommended for use.
         * \exception std::runtime_error
         */
        uint16_t GetCcdAdc12BitGain();

         /*! 
         * Returns the analog to digital converter offset value for the 12 bit ADC.
         * Calling ApogeeCam::GetAdcOffset( 1, 0 ) is equivalent to this function
         * and is recommended for use.
         * \exception std::runtime_error
         */
        uint16_t GetCcdAdc12BitOffset();

        /*! 
         * Returns the analog to digital converter gain value for the 16 bit ADC.
         * Calling ApogeeCam::GetAdcOffset( 0, 0 ) is equivalent to this function
         * and is recommended for use.
         * \exception std::runtime_error
         */
        double GetCcdAdc16BitGain();

        int32_t GetNumAds();
        int32_t GetNumAdChannels();

        double GetCoolerDrive();

        /*! 
         *
         * \exception std::runtime_error
         */
        void SetFanMode( Apg::FanMode mode, bool PreCondCheck = true );

        /*! 
         *
         * \exception std::runtime_error
         */
        Apg::FanMode GetFanMode();

        double GetTempHeatsink();

		/*! 
         * Returns an ethernet's camera MAC address.  Will throw
         * an std::runtime_error exception if the call is made on a
         * USB camera.
         * \exception std::runtime_error
         */
        std::string GetMacAddress();

       /*! 
        * Open the connection to serial port on the AltaU/E camera
        * with a default baud rate of 9600.
        * \param [in] PortId port A = 0, port B = 1
        * \exception std::runtime_error
        */
        void OpenSerial( uint16_t PortId );

       /*! 
        * Closes the connection to serial port on the AltaU/E camera
        * \param [in] PortId port A = 0, port B = 1
        * \exception std::runtime_error
        */
        void CloseSerial( uint16_t PortId );

        /*! 
        * Sets the serial port's baud rate
        * \param [in] PortId port A = 0, port B = 1
        * \param [in] BaudRate Valid values are 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
        * \exception std::runtime_error
        */

        void SetSerialBaudRate( uint16_t PortId , uint32_t BaudRate );

       /*! 
        * Returns the serial port's baud rate
        * \param [in] PortId port A = 0, port B = 1
        * \return baud rate
        * \exception std::runtime_error
        */
        uint32_t GetSerialBaudRate(  uint16_t PortId  );

       /*! 
        * Returns serial port's flow control type
        * \param [in] PortId port A = 0, port B = 1
        * \return current flow control type
        * \exception std::runtime_error
        */
        Apg::SerialFC GetSerialFlowControl( uint16_t PortId );

       /*! 
        * Sets serial port's flow control type
        * \param [in] PortId port A = 0, port B = 1
        * \param [in] FlowControl flow control type to set
        * \exception std::runtime_error
        */
        void SetSerialFlowControl( uint16_t PortId, 
            Apg::SerialFC FlowControl );

       /*! 
        * Get serial port's parity
        * \param [in] PortId port A = 0, port B = 1
        * \return  Input port's current parity type to set
        * \exception std::runtime_error
        */
        Apg::SerialParity GetSerialParity( uint16_t PortId );
        
       /*! 
        * Sets serial port's parity
        * \param [in] PortId port A = 0, port B = 1
        * \param [in]  Parity  parity type to set
        * \exception std::runtime_error
        */
        void SetSerialParity( uint16_t PortId, Apg::SerialParity Parity );
        
       /*!
        * Read data from the camera's serial port
        * \param [in] PortId port A = 0, port B = 1
        * \exception std::runtime_error
        */
        std::string ReadSerial( uint16_t PortId );
        
       /*!
        * Data to sent out of the camera's serial port
        * \param [in] PortId port A = 0, port B = 1
        * \parm[in] buffer data to send out of the serial port
        * \exception std::runtime_error
        */
        void WriteSerial( uint16_t PortId, const std::string & buffer );

    protected:
        Alta(const std::string & ioType,
             const std::string & DeviceAddr);

        void ExposureAndGetImgRC(uint16_t & r, uint16_t & c);
        uint16_t ExposureZ();
        uint16_t GetImageZ();
        uint16_t GetIlluminationMask();
        void CreateCamIo(const std::string & ioType,
            const std::string & DeviceAddr);

        void FixImgFromCamera( const std::vector<uint16_t> & data,
            std::vector<uint16_t> & out,  int32_t rows, int32_t cols);

    private:
        
        void VerifyCamId();
        void CfgCamFromId( uint16_t CameraId );
        uint16_t GetPixelShift();

        void Init12BitCcdAdc();
        void StopExposureImageReady( bool Digitize );
        void StopExposureModeTdiKinetics( bool Digitize );
        
        bool IsSerialPortOpen( uint16_t PortId );
        const std::string m_fileName;


        std::map<uint16_t , bool> m_serialPortOpenStatus;
        
        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        Alta(const Alta&);
        Alta& operator=(Alta&);
}; 

#endif
