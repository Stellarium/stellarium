/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
*
* \class Ascent 
* \brief Implementation of the ascent camera 
* 
*/ 


#ifndef ASCENT_INCLUDE_H__ 
#define ASCENT_INCLUDE_H__ 

#include "CamGen2Base.h" 
#include <string>
#include "ApogeeFilterWheel.h" 

class ApgTimer;

class DLL_EXPORT Ascent : public CamGen2Base
{ 
    public:
        /*! */
        enum FilterWheelType
        {
            /*! */
            FW_UNKNOWN_TYPE = 0,
            /*! */
            CFW25_6R	= 7,
            /*! */
            CFW31_8R	= 8
        };

        struct FilterWheelInfo
        {
            Ascent::FilterWheelType type;
            std::string name;
            uint16_t maxPositions;
        };

        Ascent();

        void OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id );

        void CloseConnection();

        virtual ~Ascent();

         /*! 
         *  Opens a connection to the Ascent filter wheel
         *  \param [in] type Filter wheel type attached to the Ascent camera
         *  \exception std::runtime_error 
         */
        void FilterWheelOpen( Ascent::FilterWheelType type );

        /*! 
         *  Closes the connection to the Ascent filter wheel
         *  \exception std::runtime_error 
         */
	    void FilterWheelClose();

        /*! 
         *  Sets the filter position to the input value
         *  \param [in] pos Desired position. Valid range is 0 to GetFilterWheelMaxPositions() - 1.
         *  \exception std::runtime_error 
         */
        void SetFilterWheelPos( uint16_t pos );

        /*! 
         *  Returns the current filter wheel position.
         *  \exception std::runtime_error 
         */
        uint16_t GetFilterWheelPos();

        /*! 
         *  Returns the current status of the filter wheel
         *  \exception std::runtime_error 
         */
        ApogeeFilterWheel::Status GetFilterWheelStatus();

         /*! 
         * Returns Current filter wheel type
         */
        Ascent::FilterWheelType GetFilterWheelType() { return m_filterWheelType; }

         /*! 
         * Returns Current filter wheel name
         * \exception std::runtime_error
         */
        std::string GetFilterWheelName();

         /*! 
         * Returns The maximum number of filter wheel position
         * \exception std::runtime_error
         */
        uint16_t GetFilterWheelMaxPositions();

        void StartExposure( double Duration, bool IsLight );

        int32_t GetNumAdChannels();


         void Init();

         /*! 
         * Returns true if dual readout is support on this
         * camera model
         */
         bool IsDualReadoutSupported();

         /*! 
         * Toggles dual readout (data digitzation via 2 ADC's)
         * mode for cameras that support this feature.
         * This function will throw an exception if the calling
         * program tries to activiate this feature on a camera that
         * does not support this mode
         *  \param [in] TurnOn true to activate, false to deactivate (single readout mode)
         * \exception std::runtime_error
         */
         void SetDualReadout( bool TurnOn );

         /*! 
         * \return true if dual readout is on, false if it is off (single readout mode)
         */
         bool GetDualReadout();

        Apg::FanMode GetFanMode();
        void SetFanMode( Apg::FanMode mode, bool PreCondCheck = true );

    protected:
        Ascent(const std::string & ioType,
             const std::string & DeviceAddr);

        void FixImgFromCamera( const std::vector<uint16_t> & data,
            std::vector<uint16_t> & out,  int32_t rows, int32_t cols );

        void CreateCamIo(const std::string & ioType,
            const std::string & DeviceAddr);

        void ExposureAndGetImgRC(uint16_t & r, uint16_t & c);

        void UpdateCamRegIfNeeded();

        void SetIsInterlineBit();
        
        void SetIsAscentBit();

    private:
        void StartFwTimer( uint16_t pos );
        ApogeeFilterWheel::Status FwStatusFromTimer();
        ApogeeFilterWheel::Status FwStatusFromCamera();

        void CfgCamFromId( uint16_t CameraId );

        void VerifyCamId();

        void UpdateCfgWithStrDbInfo();

        bool AreColsCentered();

        const std::string m_fileName;
        Ascent::FilterWheelType m_filterWheelType;
        double m_FwDiffTime;
      
//this code removes vc++ compiler warning C4251
//from http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
#ifdef WIN_OS
#if _MSC_VER < 1600
        template class DLL_EXPORT std::shared_ptr<ApgTimer>;
#endif
#endif
		
        std::shared_ptr<ApgTimer> m_FwTimer;

        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        Ascent(const Ascent&);
        Ascent& operator=(Ascent&);
}; 

#endif
