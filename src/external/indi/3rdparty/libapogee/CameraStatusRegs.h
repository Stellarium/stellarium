/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class CameraStatusRegs 
* \brief Class that wrapps the basic and advanced status structs 
* 
*/ 


#ifndef CAMERASTATUSREGS_INCLUDE_H__ 
#define CAMERASTATUSREGS_INCLUDE_H__ 

#include <string>
#include <stdint.h>
#include "DefDllExport.h"

class DLL_EXPORT CameraStatusRegs 
{ 
    public: 
#pragma pack( push, 1 )
   /*! \struct BasicStatus */
    struct BasicStatus
    {
        /*! register 93 */
        uint16_t TempHeatSink;
        /*! register 94 */
        uint16_t TempCcd;
        /*! register 95*/
        uint16_t CoolerDrive;
        /*! register 96 */
        uint16_t InputVoltage;
        /*! register 104 */
        uint16_t TdiCounter;
        /*! register 105 */
        uint16_t SequenceCounter;
        /*! register 91 */
        uint16_t Status;
        /*! USB frame this data was captured on (units of 125uS). */
        uint16_t uFrame;  
        /*! Number of times the PC has fetched this data from the camera*/
        uint32_t FetchCount;    
        /*! Is the data ready for transfer */
        uint8_t  DataAvailFlag;
    };
#pragma pack( pop )

#pragma pack( push, 1 )
    /*! \struct AdvStatus */
    struct AdvStatus
    {
        /*! register 93 */
        uint16_t TempHeatSink;
        /*! register 94 */
        uint16_t TempCcd;
        /*! register 95*/
        uint16_t CoolerDrive;
        /*! register 96 */
        uint16_t InputVoltage;
        /*! register 104 */
        uint16_t TdiCounter;
        /*! register 105 */
        uint16_t SequenceCounter;
        /*! register 91 */
        uint16_t Status;
        /*! USB frame this data was captured on (units of 125uS). */
        uint16_t uFrame; 
        /*! Last frame # put into SDRAM (completely) */
        uint16_t MostRecentFrame; 
        /*! Current frame ready for DL (but not into SDRAM). */
        uint16_t ReadyFrame;
        /*! Current frame in progress. */
        uint16_t CurrentFrame;
        /*! Number of times the PC has fetched this data from the camera*/
        uint32_t  FetchCount;
        /*! Is the data ready for transfer */
        uint8_t  DataAvailFlag;

    };
#pragma pack( pop )

        CameraStatusRegs();
        CameraStatusRegs(const CameraStatusRegs::AdvStatus & adv);
        CameraStatusRegs(const CameraStatusRegs::BasicStatus & basic);
        virtual ~CameraStatusRegs(); 

        uint16_t GetTempHeatSink() { return m_TempHeatSink; }
        uint16_t GetTempCcd() { return m_TempCcd; }
        uint16_t GetCoolerDrive() { return m_CoolerDrive; }
        uint16_t GetInputVoltage() { return m_InputVoltage; }
        uint16_t GetTdiCounter() { return m_TdiCounter; }
        uint16_t GetSequenceCounter() { return m_SequenceCounter; }
        uint16_t GetStatus() const { return m_Status; }
        uint16_t GetuFrame() { return m_uFrame; } 
        uint16_t GetMostRecentFrame() { return m_MostRecentFrame; }
        uint16_t GetReadyFrame() { return m_ReadyFrame; }
        uint16_t GetCurrentFrame() { return m_CurrentFrame ; }
        uint32_t  GetFetchCount() { return m_FetchCount; }   
        bool  GetDataAvailFlag() const { return ( (m_DataAvailFlag & 0x1) ? true:false); }

        void QueryStatusRegs( uint16_t &	StatusReg,
						      uint16_t &	HeatsinkTempReg,
						      uint16_t &	CcdTempReg,
						      uint16_t &	CoolerDriveReg,
						      uint16_t &	VoltageReg,
						      uint16_t &	TdiCounter,
						      uint16_t &	SequenceCounter,
						      uint16_t &	MostRecentFrame,
						      uint16_t &	ReadyFrame,
						      uint16_t &	CurrentFrame);

        std::string GetStatusStr() const;

        void Update(const CameraStatusRegs::AdvStatus & adv);
        void Update(const CameraStatusRegs::BasicStatus & basic);

    private:
        uint16_t m_TempHeatSink;
        uint16_t m_TempCcd;
        uint16_t m_CoolerDrive;
        uint16_t m_InputVoltage;
        uint16_t m_TdiCounter;
        uint16_t m_SequenceCounter;
        uint16_t m_Status;
        uint16_t m_uFrame; 
        uint16_t m_MostRecentFrame; 
        uint16_t m_ReadyFrame;
        uint16_t m_CurrentFrame;
        uint32_t  m_FetchCount;    
        uint8_t  m_DataAvailFlag;

}; 

#endif
