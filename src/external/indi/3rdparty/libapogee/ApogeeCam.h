/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
*
* \class ApogeeCam 
* \brief Base class for apogee cameras 
* 
*/ 


#ifndef APOGEECAM_INCLUDE_H__ 
#define APOGEECAM_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>


#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

#include "CameraStatusRegs.h" 
#include "CameraInfo.h" 
#include "DefDllExport.h"



class PlatformData;
class CApnCamData;
class CameraIo;
class ModeFsm;
class CcdAcqParams;
class ApgTimer;

class DLL_EXPORT ApogeeCam 
{ 
    public: 

        /*! 
         *
         */
        virtual ~ApogeeCam();

        /*! 
         * Resets the camera's internal pixel processing engines, and 
         * then starts the system flushing again.  This function will cancel
         * and pending expsoures.  For more information on when to use 
         * this function please review the \ref exceptionHandling "Exception Handling" 
         * page.
         * \exception std::runtime_error
         */
        void Reset();

        /*! 
         *  \param [in] reg Register
         *  \return Register value
         *  \exception std::runtime_error 
         */
        uint16_t ReadReg( uint16_t reg );

        /*! 
         * \param [in] reg Register
         * \param [in] value Value to write
         * \exception std::runtime_error
         */
        void WriteReg( uint16_t reg,  uint16_t value);
        
        /*! 
         * Sets the number of imaging ROI rows
         * The default value is the GetMaxImgRows().
         * \param [in] rows number of rows, 1 to GetMaxImgRows() is the valid value range.
         * \exception std::runtime_error
         */
        void SetRoiNumRows( uint16_t rows );

        /*! 
         * Sets the number of imaging ROI columns.
         * The default value is the GetMaxImgCols().
         * \param [in] cols 1 to GetMaxImgCols() is the valid value range for normal imaging.
         * For imaging the overscan columns maximum is GetMaxImgCols() + GetNumOverscanCols().
         * \exception std::runtime_error
         */
        void SetRoiNumCols( uint16_t cols );

        /*! 
         * Returns the number of imaging ROI rows
         * \exception std::runtime_error
         */
        uint16_t GetRoiNumRows();

        /*! 
         * Returns the number of imaging ROI columns
         * \exception std::runtime_error
         */
        uint16_t GetRoiNumCols(); 

        /*! 
         * Sets the starting row for the imaging ROI. 0 is the default value.
         * \param [in] row 0 to GetMaxImgRows() -1 is the valid value range.
         * \exception std::runtime_error
         */
        void SetRoiStartRow( uint16_t row );

        /*! 
         * Sets the starting column for the imaging ROI. 0 is the default value.
         * \param [in] col 0 to GetMaxImgCols() -1 is the valid value range.
         * \exception std::runtime_error
         */
        void SetRoiStartCol( uint16_t col );

        /*! 
         * Returns the starting row for the imaging ROI
         * \exception std::runtime_error
         */
        uint16_t GetRoiStartRow();

        /*! 
         * Returns the starting column for the imaging ROI
         * \exception std::runtime_error
         */
        uint16_t GetRoiStartCol(); 

        /*! 
         * Sets the number of rows to bin (vertical binning). Default value is 1.
         * Binning is not allowed in Apg::AdcSpeed_Video mode or for quad readout
         * ccds.
         * \param [in] bin Valid range is 1 - GetMaxBinRows()
         * \exception std::runtime_error
         */
        void SetRoiBinRow( uint16_t bin );

        /*! 
         * Returns the number of binning rows.
         * \exception std::runtime_error
         */
        uint16_t GetRoiBinRow();

        /*! 
         * Sets the number of columns to bin (horizontal binning).  Default value is 1.
         * Binning is not allowed in Apg::AdcSpeed_Video mode or for quad readout
         * ccds.
         * \param [in] bin Valid range is 1 - GetMaxBinCols()
         * \exception std::runtime_error
         */
        void SetRoiBinCol( uint16_t bin );

        /*! 
         * Returns the number of binning columns.
         * \exception std::runtime_error
         */
        uint16_t GetRoiBinCol();

        /*! 
         * Version number of the camera control firmware.
         * \exception std::runtime_error
         */
        uint16_t GetFirmwareRev();

        /*! 
         * Sets the number of images to take in an 
         * image sequence.
         * \param [in] count Number of image in the sequence to acquire.
         * \exception std::runtime_error
         */
        void SetImageCount( uint16_t count );

        /*! 
         * Returns the number of sequence images set by the user.
         * \exception std::runtime_error
         */
        uint16_t GetImageCount();

        /*! 
         * Returns the number of images in a sequence the camera has acquired.
         * The camera updates this value after the StartExposure function has been
         * called.
         * \exception std::runtime_error
         */
        uint16_t GetImgSequenceCount();

        /*! 
         * Time delay between images of the sequence.  Dependent on 
         * SetVariableSequenceDelay( bool variable ). 
         * The default value of this variable after initialization is 327us.
         * \param [in] delay The valid range is from  327us to 21.42s.
         * \exception std::runtime_error
         */
        void SetSequenceDelay( double delay );

        /*! 
         * Returns he amount of time between the close of the 
         * shutter and the beginning of the sensor readout.
         * \exception std::runtime_error
         */
        double GetSequenceDelay();

        /*! 
         * Tells the camera when it should apply the sequeuce delay during an image
         * sequence.  The default value of this variable after initialization is true.
         * \param [in] variable If true, the delay is applied between the end of last 
         * readout to beginning of next exposure. If false, delay is a constant time interval 
         * from the beginning of the last exposure to the beginning of the next exposure.
         * \exception std::runtime_error
         */
        void SetVariableSequenceDelay( bool variable );

        /*! 
         * Returns the variable sequence delay state.
         * \exception std::runtime_error
         */
        bool GetVariableSequenceDelay();

        /*! 
         * Rate between TDI rows.  The default value for this variable 
         * after initialization is 0.100s. Modifying this property also changes 
         * the value of the SetKineticsShiftInterval( double  interval ) property.
         * \param [in] TdiRate Range is from 5.12us to 336ms. 
         * \exception std::runtime_error
         */
        void SetTdiRate( double TdiRate );

        /*! 
         * Returns the rate between TDI rows.
         * \exception std::runtime_error
         */
        double GetTdiRate();
        
        /*! 
         * Total number of rows in the TDI image.  The default value 
         * for this variable after initialization is 1. Modifying this property 
         * also changes the value of the 
         * SetKineticsSections( uint16_t sections ) property.
         * \param [in] TdiRows Range is between 1 and 65535.
         * \exception std::runtime_error
         */
        void SetTdiRows( uint16_t TdiRows );

        /*! 
         * Returns the total number of rows in the TDI image.
         * \exception std::runtime_error
         */
        uint16_t GetTdiRows();
        
        /*! 
         * Dynamically incrementing count during a TDI image. 
         * The final value of TDICounter equals TDIRows. Valid 
         * range is between 1 and 65535.
         * \exception std::runtime_error
         */
        uint16_t GetTdiCounter();

        /*! 
         * The row (vertical) binning of a TDI image.
         * The default value for this variable after initialization is 1. 
         * Modifying this property also changes the value of the 
         * SetKineticsSectionHeight( uint16_t height ) property.
         * \param [in] bin  The valid range for this variable is between 1 
         * and the corresponding value of  GetMaxBinRows(). 
         * \exception std::runtime_error
         */
        void SetTdiBinningRows( uint16_t bin );

        /*! 
         * Returns the number TDI binning rows
         * \exception std::runtime_error
         */
        uint16_t GetTdiBinningRows();
        
        /*! 
         * Set the vertical height for a Kinetics Mode section. Modifying this 
         * property also changes the value of the SetTdiBinningRows( uint16_t bin ) 
         * variable. The default value for this variable after initialization is 1.
         * \param [in] height Valid range for this variable is between 1 
         * and the corresponding value of  GetMaxBinRows(). 
         * \exception std::runtime_error
         */
        void SetKineticsSectionHeight( uint16_t height );

        /*! 
         * Returns the vertical height for a Kinetics Mode section.
         * \exception std::runtime_error
         */
        uint16_t GetKineticsSectionHeight();

        /*! 
         * Sets the number of sections in a Kinetics Mode image.
         * Modifying this property  also changes the value of the 
         * SetTdiRows( uint16_t TdiRows )
         * \param [in] sections Valid range is 1 to 65535.
         * \exception std::runtime_error
         */
        void SetKineticsSections( uint16_t sections );

        /*! 
         * Retruns the number of sections in a Kinetics Mode image.
         * \exception std::runtime_error
         */
        uint16_t GetKineticsSections();

        /*! 
         * Sets the incremental rate between Kinetics Mode sections.
         * The default value for this variable 
         * after initialization is 0.100s. Modifying this property also changes 
         * the value of SetTdiRate( double TdiRate ).
         * \param [in] interval The valid range is from 5.12us to 336ms. 
         * \exception std::runtime_error
         */
        void SetKineticsShiftInterval( double  interval );

        /*! 
         * Returns the incremental rate between Kinetics Mode sections.
         * \exception std::runtime_error
         */
        double GetKineticsShiftInterval();

        /*! 
         * Sets the delay from the time the exposure begins to the time the 
         * rising edge of the shutter strobe period appears on a pin at the 
         * experiment interface. The default value of this variable after 
         * initialization is 0.001s (1ms).
         * \param [in] position The minimum valid value is 3.31us and the 
         * maximum value is 167ms (2.56us/bit resolution).
         * \exception std::runtime_error
         */
        void SetShutterStrobePosition( double position );

        /*! 
         * Returns the shutter strobe position.
         * \exception std::runtime_error
         */
        double GetShutterStrobePosition();

        /*! 
         * Sets the period of the shutter strobe appearing on a pin at the 
         * experiment interface.  The default value of this variable after 
         * initialization is 0.001s (1ms).
         * \param [in] period The minimum valid value is 45ns and maximum 
         * value is 2.6ms (40ns/bit resolution).
         * \exception std::runtime_error
         */
        void SetShutterStrobePeriod( double period );

        /*! 
         * Returns ths shutter strobe period.
         * \exception std::runtime_error
         */
        double GetShutterStrobePeriod();

        /*! 
         * Sets the amount of time between the close of the shutter 
         * and the beginning of the sensor readout. The default value of this 
         * variable after initialization is camera dependent. NOTE: This is a 
         * specialized property designed for unique experiments. Applications
         * and users will not normally need to modify this variable from the 
         * default value.
         * \param [in] delay
         * \exception std::runtime_error
         */
        void SetShutterCloseDelay( double delay );

        /*! 
         * Returns the shutter close delay
         * \exception std::runtime_error
         */
        double GetShutterCloseDelay();

        /*! 
         * Sets the cooler backoff temperature of the cooler subsystem in Celsius. 
         * If the cooler is unable to reach the desired cooler set point, the Backoff Point 
         * is number of degrees up from the lowest point reached. Used to prevent the 
         * cooler from being constant driven with max power to an unreachable temperature. 
         * The default value of this variable after initialization can vary depending on camera 
         * model, but is typically set at 2.0 degrees Celsius.
         * \param [in] point Desired backoff point.  Must be greater than 0.
         * \exception std::runtime_error
         */
        void SetCoolerBackoffPoint( double point );

        /*! 
         * Returns the cooler backoff temperature in Celsius. 
         * \exception std::runtime_error
         */
        double GetCoolerBackoffPoint();

        /*! 
         * Sets the desired cooler temperature in Celsius. If the input set point cannot be 
         * reached, the cooler subsystem will determine a new set point based on 
         * the backoff point and change the cooler status to Apg::CoolerStatus_Revision. 
         * An application should reread this property to see the new set point that 
         * the system is using. Once the application rereads this property, 
         * the status of Apg::CoolerStatus_Revision will be cleared.
         * \param [in] point Desired cooler set point
         * \exception std::runtime_error
         */
        void SetCoolerSetPoint( double point );

        /*! 
         * Returns the desired cooler temperature in Celsius.
         * \exception std::runtime_error
         */
        double GetCoolerSetPoint();
        
        /*! 
         * Returns the camera's operational mode. 
         * \exception std::runtime_error
         */
        Apg::CameraMode GetCameraMode();

        /*! 
         * Returns the camera's operational mode. 
         * The default value for this variable after initialization 
         * is Apg::CameraMode_Normal.
         * \exception std::runtime_error
         */
        void SetCameraMode( Apg::CameraMode mode );

        /*! 
         * Enables/Disables very fast back to back exposures for interline CCDs only.
         * Also referred to as Ratio Mode. The default value for this variable after 
         * initialization is false.  Note that this property cannot be used in conjunction 
         * with the TriggerNormalEach property, since progressive scan is defined 
         * as having the least possible time between exposures. However, the 
         * FastSequence property may be used with a single trigger to start a 
         * series of images (using TriggerNormalGroup).
         * \param [in] TurnOn true enables fast sequences, false disables this feature
         * \exception std::runtime_error
         */
        void SetFastSequence( bool TurnOn );

        /*! 
         * Retruns the state of fast sequences (true = on, false = off)
         * \exception std::runtime_error
         */
        bool IsFastSequenceOn();

        /*! 
         * Enables/Disables how image data will be retrieved from the 
         * camera during a sequence. For USB camera systems, this variable 
         * is used to determine whether the returned data will be downloaded in 
         * bulk, or streamed as it becomes available. By definition, 
         * bulk download must be false when the continuous imaging is enabled, 
         * since setting this variable true assumes that the number of images in 
         * a series is known in advance of starting the exposure. The default value 
         * for this variable after initialization is true.
         * \param [in] TurnOn true enables bulk download, false disables this feature
         * \exception std::runtime_error
         */
        void SetBulkDownload( bool TurnOn );

        /*! 
         * Returns the state of bulk downloads (true = on, false = off)
         * \exception std::runtime_error
         */
        bool IsBulkDownloadOn();

        /*! 
         * For USB camera systems, this variable is used to determine 
         * when ImageReady status is returned. When on, camera will return ImageReady
         * status as soon as there is any data to download. When off, camera will 
         * not return ImageReady status until all data has been digitized and is ready
         * for download. Turn on for faster downloads and increased frame rate in exchange
         * for higher noise. Which option is best depends on your application.
         * The default value for this variable after initialization is true.
         * \param [in] TurnOn true enables pipelined download, false disables this feature
         * \exception std::runtime_error
         */
        void SetPipelineDownload( bool TurnOn );

        /*! 
         * Returns the state of pipelined downloads (true = on, false = off)
         * \exception std::runtime_error
         */
        bool IsPipelineDownloadOn();

        /*! 
         * Defines the signal usage for the I/O port.  This function is only valid on AltaU 
         * systems. 
         * - Bit 0 (I/O Signal 1): A value of zero (0) indicates that the I/O bit is user 
         * defined according to the specified IoPortDirection. A value of one (1) 
         * indicates that this I/O will be used as a trigger input. 
         * - Bit 1 (I/O Signal 2): A value of zero (0) indicates that the I/O bit is 
         * user defined according to the specified IoPortDirection. A value 
         * of one (1) indicates that this I/O will be used as a shutter output. 
         * - Bit 2 (I/O Signal 3): A value of zero (0) indicates that the I/O bit is 
         * user defined according to the specified IoPortDirection. A value of one 
         * (1) indicates that this I/O will be used as a shutter strobe output. 
         * - Bit 3 (I/O Signal 4): A value of zero (0) indicates that the I/O bit is user 
         * defined according to the specified IoPortDirection. A value of one (1) 
         * indicates that this I/O will be used as an external shutter input. 
         * - Bit 4 (I/O Signal 5): A value of zero (0) indicates that the I/O bit is user 
         * defined according to the specified IoPortDirection. A value of one (1) 
         * indicates that this I/O will be used for starting readout via an external signal. 
         * - Bit 5 (I/O Signal 6): A value of zero (0) indicates that the I/O bit is user 
         * defined according to the specified IoPortDirection. A value of one (1) 
         * indicates that this I/O will be used for an input timer pulse. 
         * \param [in] assignment The valid range is for the 6 LSBs, 0x0 to 0x3F.
         * \exception std::runtime_error
         */
        void SetIoPortAssignment( uint16_t assignment );

        /*! 
         * Returns the I/O port's signal usage.
         * \exception std::runtime_error
         */
        uint16_t GetIoPortAssignment();

        /*! 
         *
         * \exception std::runtime_error
         */
        void SetIoPortBlankingBits( uint16_t blankingBits );

        /*! 
         *
         * \exception std::runtime_error
         */
        uint16_t GetIoPortBlankingBits();

        /*! 
         * This function is only valid on AltaU camera systems. \n
         * On Ascent camera systems, the direction cannot be configured.  
         * Pins 1, 4, and 5 are always input and pins 2, 3, and 6 are always output. \n
         * - Bit 0: I/O Signal 1 (0=IN and 1=OUT)
         * - Bit 1: I/O Signal 2 (0=IN and 1=OUT)
         * - Bit 2: I/O Signal 3 (0=IN and 1=OUT)
         * - Bit 3: I/O Signal 4 (0=IN and 1=OUT)
         * - Bit 4: I/O Signal 5 (0=IN and 1=OUT)
         * - Bit 5: I/O Signal 6 (0=IN and 1=OUT)
         * \param [in] direction Valid range is for the 6 LSBs, 0x0 to 0x3F.
         * \exception std::runtime_error
         */
        void SetIoPortDirection( uint16_t direction );

        /*! 
         * Returns the IO Port Direction.
         * \exception std::runtime_error
         */
        uint16_t GetIoPortDirection();

        /*! 
         * Send data to the I/O port. Applications are responsible for 
         * toggling bits, i.e., if Bit 2 of the I/O port is specified as an 
         * OUT signal, and a 0x1 is written to this bit, it will remain 0x1 
         * until 0x0 is written to the same bit.
         * \param [in] data Valid range of this property is for the 6 LSBs, 0x0 to 0x3F.
         * \exception std::runtime_error
         */
        void SetIoPortData( uint16_t data );

        /*! 
         * Returns the I/O port data.
         * \exception std::runtime_error
         */
        uint16_t GetIoPortData();

        /*! 
         * Toggles IR pre-flash control.  IR pre-flash 
         * normalizes the sensor before an image is 
         * taken with a flash of IR.  false is the default camera state.
         * \ param [in] TurnOn, true turn IR pre-flash on, false turns pre-flash off.
         * \exception std::runtime_error
         */
        void SetPreFlash( bool TurnOn ) { m_IsPreFlashOn = TurnOn; }

        /*! 
         * Returns IR pre-flash state ( true = turned on, false = turned off )
         * \exception std::runtime_error
         */
        bool GetPreFlash() { return m_IsPreFlashOn; }

        /*!
         * Sets the external trigger mode and type.  
         * For more information see \ref hwTrig "Hardware Trigger" page
         * \param [in] TurnOn true activates the trigger type and mode, false deactivates it
         * \param [in] trigMode Trigger mode
         * \param [in] trigType Trigger Type
         * \exception std::runtime_error
         */
        void SetExternalTrigger( bool TurnOn, Apg::TriggerMode trigMode,
            Apg::TriggerType trigType );

        /*! 
         * Returns true if normal each trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerNormEachOn();

        /*! 
         * Returns true if normal group trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerNormGroupOn();

        /*! 
         * Returns true if TDI-Kinetrics each trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerTdiKinEachOn();

        /*! 
         * Returns true if TDI-Kinetrics group trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerTdiKinGroupOn();

        /*! 
         * Returns true if external shutter trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerExternalShutterOn();

        /*! 
         * Returns true if external readout trigger is on, false if off.
         * \exception std::runtime_error
         */
        bool IsTriggerExternalReadoutOn();

        /*! 
         * Set the shutter state.
         * \param [in] state Desired shutter state
         * \exception std::runtime_error
         */
        void SetShutterState( Apg::ShutterState state );

        /*! 
         * Returns the current shutter state.
         * \exception std::runtime_error
         */
        Apg::ShutterState GetShutterState();

        /*! 
         * Returns if the shutter is in the Apg::ShutterState_ForceOpen
         * state (true = yes, false = no)
         * \exception std::runtime_error
         */
        bool IsShutterForcedOpen();

        /*! 
         * Returns if the shutter is in the Apg::ShutterState_ForceClosed
         * state (true = yes, false = no)
         * \exception std::runtime_error
         */
        bool IsShutterForcedClosed();

        /*! 
         * Returns shutter state (true = open, false = closed)
         * \exception std::runtime_error
         */
        bool IsShutterOpen();

        /*! 
         * Enables/disables the CCD voltage while the shutter strobe is high.
         * The default value of this variable after initialization is FALSE.
         * \param [in] TurnOn true disables the CCD voltage, false enables voltage
         * \exception std::runtime_error
         */
        void SetShutterAmpCtrl( bool TurnOn );

        /*! 
         * Returns shutter amp control state (true = CCD voltage disabled, 
         * false = CCD voltage enabled)
         * \exception std::runtime_error
         */
        bool IsShutterAmpCtrlOn();
 
        /*! 
         * Sets the state of the camera's cooler
         * \param [in] TurnOn true = on, false = off
         * \exception std::runtime_error
         */
        void SetCooler( bool TurnOn );

        /*! 
         * Returns the current cooler status
         * \exception std::runtime_error
         */
        Apg::CoolerStatus GetCoolerStatus();

        /*! 
         * Returns the state of the camera's cooler (true = on, false = off);
         * \exception std::runtime_error
         */
        bool IsCoolerOn();
        
        /*! 
         * Returns the current CCD temperature in degrees Celsius.
         * \exception std::runtime_error
         */
        double GetTempCcd();

        /*! 
         * Sets the camera's digitization resolution. 
         * NOTE: This feature is scheduled for deprecation 
         * and only functional on AltaU cameras.
         * Please use the SetCcdAdcSpeed(Apg::AdcSpeed speed) function; 
         * 16bit = Normal, 12bit = Fast.
         * \exception std::runtime_error
         */
        void SetCcdAdcResolution(Apg::Resolution res);

        /*! 
         * Returns the camera's digitization resolution. 
         * \exception std::runtime_error
         */
        Apg::Resolution GetCcdAdcResolution();

        /*! 
         * Sets the camera's acquisition speed. The default value after initialization
         * is Apg::AdcSpeed_Normal.  
         * For the AltaU setting the camera's speed to normal/fast is
         * the equivalant of SetCcdAdcResolution( 16bit/12bit ).  Calling 
         * GetCcdAdcResolution() after setting the ADC speed will return 16bit for
         * normal speed and 12bit for fast.
         */
        void SetCcdAdcSpeed(Apg::AdcSpeed speed);

        /*! 
         * Returns the camera's acquisition speed.
         * \exception std::runtime_error
         */
        Apg::AdcSpeed GetCcdAdcSpeed();

        /*! 
         * Returns the maximum number of binning columns (horizontal) for the camera.
         * \exception std::runtime_error
         */
        uint16_t GetMaxBinCols();

        /*! 
         * Returns the maximum number of binning rows (vertical) for the camera.
         * \exception std::runtime_error
         */
        uint16_t GetMaxBinRows();

        /*! 
         * Returns number of imaging columns in terms of unbinned pixels.
         * This value depends on the camera's sensor geometry.
         * \exception std::runtime_error
         */
        uint16_t GetMaxImgCols();

        /*! 
         * Returns number of imaging rows in terms of unbinned pixels.
         * This value depends on the camera's sensor geometry.
         * \exception std::runtime_error
         */
        uint16_t GetMaxImgRows();

        /*! 
         * Returns the total number of physical rows on the CCD. 
         * This value depends on the camera's sensor geometry.
         * \exception std::runtime_error
         */
        uint16_t GetTotalRows();

        /*! 
         * Returns the total number of physical columns on the CCD. 
         * This value depends on the camera's sensor geometry.
         * \exception std::runtime_error
         */
        uint16_t GetTotalCols();

        /*! 
         * Returns the number of overscan columns in terms 
         * of unbinned pixels. This variable depends upon the 
         * particular sensor used within the camera.
         * \exception std::runtime_error
         */
        uint16_t GetNumOverscanCols();

        /*! 
         * Returns true if the sensor is an Interline CCD and false otherwise.
         * \exception std::runtime_error
         */
        bool IsInterline();
        
        /*! 
         * Returns camera platform type.
         * \exception std::runtime_error
         */
        CamModel::PlatformType GetPlatformType();

        /*! 
         * Sets the state of LED light A.
         * \param [in] state Desired Apg::LedState
         * \exception std::runtime_error
         */
        void SetLedAState( Apg::LedState state );

        /*! 
         * Returns the state of LED light A.
         * \exception std::runtime_error
         */
        Apg::LedState GetLedAState();

        /*! 
         * Sets the state of LED light B.
         * \param [in] state Desired Apg::LedState
         * \exception std::runtime_error
         */
        void SetLedBState( Apg::LedState state );

        /*! 
         * Returns the state of LED light B.
         * \exception std::runtime_error
         */
        Apg::LedState GetLedBState();

        /*! 
         * Sets the mode of LED status lights.
         * \param [in] mode Desired Apg::LedMode
         * \exception std::runtime_error
         */
        void SetLedMode( Apg::LedMode mode );

        /*! 
         * Returns the mode of LED status lights.
         * \exception std::runtime_error
         */
        Apg::LedMode GetLedMode();

        /*! 
         * Returns information on the camera such as
         * model, sensor, ids, etc.
         * \exception std::runtime_error
         */
        std::string GetInfo();

        /*! 
         * Returns a camera model.
         * \exception std::runtime_error
         */
        std::string GetModel();

        /*! 
         * Returns the sensor model installed in the camera
         * \exception std::runtime_error
         */
        std::string GetSensor();

        /*! 
         * Enables/Disables any flushing command sent by the software 
         * to be recognized or unrecognized by the camera control firmware. 
         * This property may be used with SetPostExposeFlushing( bool Disable )
         * to completely stop all flushing operations within the camera. The 
         * default value of this variable after initialization is false. WARNING: This 
         * is a highly specialized property designed for very unique experiments. 
         * Applications and users will not normally need to modify this variable 
         * from the default value.
         * \param [in] Disable True disables flushing, false allows flushing after commands
         * \exception std::runtime_error
         */
        void SetFlushCommands( bool Disable );

        /*! 
         * Retruns if flushing commands have been disabled.
         * \exception std::runtime_error
         */
        bool AreFlushCmdsDisabled();

        /*! 
         * Enables/Disables the camera control firmware to/from immediately 
         * beginning an internal flush cycle after an exposure. This property may 
         * be used with SetFlushCommands( bool Disable ) to completely stop all flushing 
         * operations within the camera. The default value of this variable after 
         * initialization is false.  WARNING: This is a highly specialized property 
         * designed for very unique experiments. Applications and users will not 
         * normally need to modify this variable from the default value.
         * \param [in] Disable True disables flushing, false allows flushing after exposures
         * \exception std::runtime_error
         */
        void SetPostExposeFlushing( bool Disable );

        /*! 
         * Retruns if post exposure flushing has been disabled.
         * \exception std::runtime_error
         */
        bool IsPostExposeFlushingDisabled();

        /*! 
         * Returns sensor's pixels width in micrometers.
         * \exception std::runtime_error
         */
        double GetPixelWidth();

        /*! 
         * Returns sensor's pixels height in micrometers.
         * \exception std::runtime_error
         */
        double GetPixelHeight();

        /*! 
         * Returns the suggested minimum exposure duration, based on the camera's configuration.
         * \exception std::runtime_error
         */
        double GetMinExposureTime();

        /*! 
         * Returns the maximum exposure duration.
         * \exception std::runtime_error
         */
        double GetMaxExposureTime();

        /*! 
         * Returns true is CCD sensor has color dyes, and false otherwise
         * \exception std::runtime_error
         */
        bool IsColor();

        /*! 
         * Returns true if the camera supports cooling, false if no cooling is available.
         * \exception std::runtime_error
         */
        bool IsCoolingSupported();

        /*! 
         * Returns true if the camera supports regulated cooling, false if regulated cooling is not available.
         * \exception std::runtime_error
         */
        bool IsCoolingRegulated();

        /*! 
         * Returns the operating input voltage to the camera.
         * \exception std::runtime_error
         */
        double GetInputVoltage();

        /*! 
         * Retruns the camera interface type.
         * \exception std::runtime_error
         */
        CamModel::InterfaceType GetInterfaceType();

        /*! 
         * Returns the USB ids associated with the camera system.
         * \param [out] VendorId USB vendor id
         * \param [out] ProductId USB product id
         * \param [out] DeviceId USB device id
         * \exception std::runtime_error
         */
        void GetUsbVendorInfo( uint16_t & VendorId,
            uint16_t & ProductId, uint16_t  & DeviceId);

        /*! 
         * Returns true if the sensor is a CCD; false if CMOS
         * \exception std::runtime_error
         */
        bool IsCCD();

        /*! 
         * Pauses the current exposure by closing the shutter and pausing the exposure timer.
         * \param [in] TurnOn A state variable that controls the pausing of the exposure timer. 
         * A value of true will issue a command to pause the timer. A value offalse will issue 
         * a command to unpause the timer. Multiple calls with this parameter set consistently 
         * to either state (i.e. back-to-back true states) have no effect.
         * \exception std::runtime_error
         */
        void PauseTimer( bool TurnOn );

        /*! 
         * Returns whether the camera supports Serial Port A. 
         * NOTE: Ascent cameras do not have serial ports.
         * \exception std::runtime_error
         */
        bool IsSerialASupported();

        /*! 
         * Returns whether the camera supports Serial Port B. 
         * NOTE: Ascent cameras do not have serial ports.
         * \exception std::runtime_error
         */
        bool IsSerialBSupported();

        /*! 
         * Sets the row (vertical) binning value used during flushing operations. 
         * The default value after camera initialization is sensor-specific. 
         * NOTE: This is a specialized property designed for unique experiments. 
         * Applications and users will not normally need to modify this variable from 
         * the default value.
         * \param [in] bin The valid range for this property is between 1 and GetMaxBinRows(). 
         * \exception std::runtime_error
         */
        void SetFlushBinningRows( uint16_t bin );

        /*! 
         * Returns the row (vertical) binning value used during flushing operations. 
         * \exception std::runtime_error
         */
        uint16_t GetFlushBinningRows();

        /*! 
         * \return The digitize overscan state (on = true, off = false)
         * \exception std::runtime_error
         */
        bool IsOverscanDigitized();

        /*! 
         * Function is a no op.  It is in this interface for backwards
         * compatibility purposes.  The way to get the overscan data
         * is to set the ROI to capture it.
         * \param [in] TurnOn Toggling the digitize overscan variable
         * \exception std::runtime_error
         */
        void SetDigitizeOverscan( const bool TurnOn );

        /*! 
         * Sets the analog to digital converter gain value for the given ad and channel.
         * \param [in] gain The new gain value. 0-1023 is a valid range for Alta cameras. 0-63 is a valid range for Ascent cameras.
         * \param [in] ad The analog to digital convert to set. Valid range is 0 to 1 - GetNumAds().
         * \param [in] channel Channel on the ADC to set.  Valid range is 0 to 1 - GetNumAdChannels().
         * \exception std::runtime_error
         */
        void SetAdcGain( uint16_t gain, int32_t ad, int32_t channel );

        /*! 
         * This function returns the analog to digital converter gain value for the given ad and channel.
         * \param [in] ad The analog to digital convert to query. Valid range is 0 to 1 - GetNumAds().
         * \param [in] channel Channel on the ADC to query.  Valid range is 0 to 1 - GetNumAdChannels().
         * \exception std::runtime_error
         */
        uint16_t GetAdcGain( int32_t ad, int32_t channel );

        /*! 
         * Sets the analog to digital converter offset value for the given ad and channel.
         * \param [in] offset The new offset value. 0-255 is a valid range for Alta cameras. 0-511 is a valid range for Ascent cameras.
         * \param [in] ad The analog to digital convert to set. Valid range is 0 to 1 - GetNumAds().
         * \param [in] channel Channel on the ADC to set.  Valid range is 0 to 1 - GetNumAdChannels().
         * \exception std::runtime_error
         */
        void SetAdcOffset( uint16_t offset, int32_t ad, int32_t channel );

        /*! 
         * This function returns the analog to digital converter offset value for the given ad and channel.
         * \param [in] ad The analog to digital convert to query. Valid range is 0 to 1 - GetNumAds().
         * \param [in] channel Channel on the ADC to query.  Valid range is 0 to 1 - GetNumAdChannels().
         * \exception std::runtime_error
         */
        uint16_t GetAdcOffset( int32_t ad, int32_t channel );

        /*! 
         * Returns if the Init() function has been called.
         */
        bool IsInitialized() { return m_IsInitialized; }

        /*! 
         * Returns if the host is connected to a camera
         */
        bool IsConnected() { return m_IsConnected; }

        /*! 
         * Specifies that the camera operation should be defined 
         * using simulated data for image parameters.
         * param [in] TurnOn True turns on simulator mode.  False turns of the simulator.
         * \exception std::runtime_error
         */
        void SetAdSimMode( bool TurnOn );

        /*! 
         * Status of camera simulator mode
         * \exception std::runtime_error
         */
        bool IsAdSimModeOn();

        /*! 
         * Sets brightness/intensity level of the LED light within the 
         * cap of the camera head. The default value of this variable 
         * after initialization is 0%.
         * \param [in] PercentIntensity Valid range is 0-100.
         * \exception std::runtime_error
         */
        void SetLedBrightness( double PercentIntensity );

        /*! 
         * Returns brightness/intensity level of the LED light within the 
         * cap of the camera head. Expressed as a percentage from 0% to 
         * 100%. 
         * \exception std::runtime_error
         */
        double GetLedBrightness();

        /*! 
         * On Windows returns device driver version
         * \exception std::runtime_error
         */
        std::string GetDriverVersion();

        /*! 
         * Returns USB firmware version.
         * \exception std::runtime_error
         */
        std::string GetUsbFirmwareVersion();

        /*! 
         * Returns a special OEM-specific serial number.
         * \exception std::runtime_error
         */
        std::string GetSerialNumber();


        CamInfo::StrDb ReadStrDatabase();
        void WriteStrDatabase(CamInfo::StrDb &info);

        // ****** PURE VIRTUAL INTERFACE ********

        /*! 
         * Opens a connection from the PC to the camera.  The results strings from
         * the FindDeviceUsb::Find() and the FindDeviceEthernet::Find() functions.
         * provide the input into this function.
         * \param [in] ioType specifies camera IO interface 'usb' or 'ethernet'
         * \param [in] DeviceAddr specifies the address of the camera on the interface
         * \param [in] FirmwareRev Camera's firmware revision.  Used to verify interface connection.
         * \param [in] Id Camera's ID.  Used to verify interface connection and setup 
         * camera specfic parameters.
         * \exception std::runtime_error
         */
        virtual void OpenConnection( const std::string & ioType,
            const std::string & DeviceAddr,
            const uint16_t FirmwareRev,
            const uint16_t Id ) = 0;

        /*! 
         * Closes the IO connection to the camera.  IMPORTANT: If this call is made
         * if camera is in an error condition, then the function will try to reset the interface.
         * Thus it is not guaranteed that the address for the camera will be the same
         * after this function is called.
         * \exception std::runtime_error
         */
        virtual void CloseConnection() = 0;

         /*! 
         * Method for initializing the Apogee camera system.  Must be called
         * once before image acquisition.
         * \exception std::runtime_error
         */
        virtual void Init() = 0;

        /*! 
         * This method begins the imaging process.  The type of exposure taken is 
         * depends on various state variables including the CameraMode and TriggerMode.
         * \param [in] Duration Length of the exposure(s), in seconds. The valid range 
         * for this parameter is GetMinExposureTime() to GetMaxExposureTime().
         * \param [in] Determines whether the exposure is a light or dark/bias frame. 
         * A light frame requires this parameter to be set to true, while a dark frame 
         * requires this parameter to be false.
         * \exception std::runtime_error
         */
        virtual void StartExposure( double Duration, bool IsLight ) = 0;

        /*! 
         * Returns the camera's status registers as a CameraStatusRegs class.
         * \exception std::runtime_error
         */
        virtual CameraStatusRegs GetStatus() = 0;

        /*! 
         * Returns the current imaging state of the camera.
         * \exception std::runtime_error
         */
        virtual Apg::Status GetImagingStatus() = 0;

        /*! 
         * Downloads the image data from the camera.  
         * \param [out] out Vector that will recieve the image data
         * \exception std::runtime_error
         */
        virtual void GetImage( std::vector<uint16_t> & out ) = 0;

        /*! 
         * This method halts an in progress exposure. If this method is called 
         * and there is no exposure in progress a std::runtime_error exception is thrown.
         * \param [in] Digitize  If set to true, then the application must call 
         * GetImage() to retrieve the image data and to put the camera
         * in a good state for the next exposure. If set to false, then an application 
         * should not  call GetImage().  
         * \exception std::runtime_error
         */
        virtual void StopExposure( bool Digitize ) = 0;

        /*! 
         * Returns the amount of available memory for storing images in terms of kilobytes (KB).
         * \exception std::runtime_error
         */
        virtual uint32_t GetAvailableMemory() = 0;

        /*! 
         * Returns the number of analog to digital (AD) converters on the camera.
         * \exception std::runtime_error
         */
        virtual int32_t GetNumAds() = 0;

        /*! 
         * Returns the number of channels on the camera's AD converters.
         * \exception std::runtime_error
         */
        virtual int32_t GetNumAdChannels() = 0;

        /*! 
         * Drive level applied to the temp controller. Expressed
         * as a percentage from 0% to 100%.
         * \exception std::runtime_error
         */
        virtual double GetCoolerDrive() = 0;

        /*! 
         * Sets the current fan speed. The default value of this variable 
         * after initialization is Apg::FanMode_Low.  Ascent cameras do not
         * support programmable fan speed, thus writes using this property have no effect.
         * \param [in] mode Desired fan mode
         * \param [in] PreCondCheck Setting PreCondCheck to
         * false results in the pre-condition checking to be skipped.  
         * PreCondCheck should ALWAYS be set to true.
         * \exception std::runtime_error
         */
        virtual void SetFanMode( Apg::FanMode mode, bool PreCondCheck = true ) = 0;

        /*! 
         * Retruns current fan mode.  Ascents always return Apg::FanMode_Off.
         * \exception std::runtime_error
         */
        virtual Apg::FanMode GetFanMode() = 0;

        /*! 
         * Returns the current Heatsink temperature in degrees Celsius. 
         * The Ascent camera platform does not support reading the heatsink 
         * temperature, and this property will return -255.
         * \exception std::runtime_error
         */
        virtual double GetTempHeatsink() = 0;

        void UpdateAlta(const std::string FilenameCamCon, const std::string FilenameBufCon, const std::string FilenameFx2, const std::string FilenameGpifCamCon, const std::string FilenameGpifBufCon, const std::string FilenameGpifFifo);
        void UpdateAscentOrAltaF(const std::string FilenameFpga, const std::string FilenameFx2, const std::string FilenameDescriptor);
        void UpdateAspen(const std::string FilenameFpga, const std::string FilenameFx2, const std::string FilenameDescriptor, const std::string FilenameWebPage, const std::string FilenameWebServer, const std::string FilenameWebCfg);

        
    protected:
        ApogeeCam(CamModel::PlatformType platform) ;
        
        void VerifyFrmwrRev();
        void LogConnectAndDisconnect( bool Connect );

        void ExectuePreFlash();
        void SetExpsoureTime( double Duration );
        void IssueExposeCmd(  bool IsLight );

        void IsThereAStatusError( uint16_t statusReg );

        bool IsImgDone( const CameraStatusRegs & statusObj);
        Apg::Status LogAndReturnStatus( Apg::Status status,
            const CameraStatusRegs & statusObj);

        void SupsendCooler( bool & resume );
        void ResumeCooler();
        void WaitForCoolerSuspendBit( const uint16_t mask, const bool IsHigh );

        void InitShutterCloseDelay();

        void StopExposureModeNorm( bool Digitize );
        void Reset(bool Flush);

        void HardStopExposure( const std::string & msg );
        void GrabImageAndThrowItAway();

        void AdcParamCheck( const int32_t ad, 
                              const int32_t channel, const std::string & fxName );

        void SetNumAdOutputs( const uint16_t num );

        bool CheckAndWaitForStatus( Apg::Status desired, Apg::Status & acutal );
        void CancelExposureNoThrow();
        double DefaultGetTempHeatsink();

        void DefaultInit();
        void ClearAllRegisters();
        void DefaultCfgCamFromId( uint16_t CameraId );
        void DefaultSetFanMode( Apg::FanMode mode, bool PreCondCheck );
        Apg::FanMode DefaultGetFanMode();
        void DefaultCloseConnection();

         // ****** PURE VIRTUAL INTERFACE ********
        virtual void CfgCamFromId( uint16_t CameraId ) = 0;
        virtual void ExposureAndGetImgRC(uint16_t & r, uint16_t & c) = 0;
        virtual uint16_t ExposureZ() = 0;
        virtual uint16_t GetImageZ() = 0;
        virtual uint16_t GetIlluminationMask() = 0;
        virtual void FixImgFromCamera( const std::vector<uint16_t> & data,
            std::vector<uint16_t> & out,  int32_t rows, int32_t cols) = 0;
                
//this code removes vc++ compiler warning C4251
//from http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
#ifdef WIN_OS
#if _MSC_VER < 1600
        template class DLL_EXPORT std::shared_ptr<CameraIo>;
        template class DLL_EXPORT std::shared_ptr<PlatformData>;
        template class DLL_EXPORT std::shared_ptr<CApnCamData>;
        template class DLL_EXPORT std::shared_ptr<ModeFsm>;
        template class DLL_EXPORT std::shared_ptr<CcdAcqParams>;
        template class DLL_EXPORT std::shared_ptr<ApgTimer>;
#endif
#endif

        std::shared_ptr<CameraIo> m_CamIo;
        std::shared_ptr<PlatformData> m_CameraConsts;
        std::shared_ptr<CApnCamData> m_CamCfgData;
        std::shared_ptr<ModeFsm> m_CamMode;
        std::shared_ptr<CcdAcqParams> m_CcdAcqSettings;
        std::shared_ptr<ApgTimer> m_ExposureTimer;

        CamModel::PlatformType m_PlatformType;
        const std::string m_fileName;
        uint16_t m_FirmwareVersion;
        uint16_t m_Id;
        uint16_t m_NumImgsDownloaded;
        bool m_ImageInProgress;
        bool m_IsPreFlashOn;
        bool m_IsInitialized;
        bool m_IsConnected;
		double m_LastExposureTime;
     
    private:

        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        ApogeeCam(const ApogeeCam&);
        ApogeeCam& operator=(ApogeeCam&);
}; 

#endif
