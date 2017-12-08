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

#include "CameraStatusRegs.h" 
#include <sstream>

//////////////////////////// 
// CTOR 
CameraStatusRegs::CameraStatusRegs() : 
                            m_TempHeatSink( 0 ),
                            m_TempCcd( 0 ),
                            m_CoolerDrive( 0 ),
                            m_InputVoltage( 0 ),
                            m_TdiCounter( 0 ),
                            m_SequenceCounter( 0 ),
                            m_Status( 0 ),
                            m_uFrame( 0 ), 
                            m_MostRecentFrame( 0 ),
                            m_ReadyFrame( 0 ),
                            m_CurrentFrame( 0 ),
                            m_FetchCount( 0 ),
                            m_DataAvailFlag( 0 )
{

}
        
//////////////////////////// 
// CTOR 
CameraStatusRegs::CameraStatusRegs(const CameraStatusRegs::AdvStatus & adv) : 
                                   m_TempHeatSink( adv.TempHeatSink ),
                                   m_TempCcd( adv.TempCcd ),
                                   m_CoolerDrive( adv.CoolerDrive ),
                                   m_InputVoltage( adv.InputVoltage ),
                                   m_TdiCounter( adv.TdiCounter ),
                                   m_SequenceCounter( adv.SequenceCounter ),
                                   m_Status( adv.Status ),
                                   m_uFrame( adv.uFrame ), 
                                   m_MostRecentFrame( adv.MostRecentFrame ),
                                   m_ReadyFrame( adv.ReadyFrame ),
                                   m_CurrentFrame( adv.CurrentFrame ),
                                   m_FetchCount( adv.FetchCount ),    
                                   m_DataAvailFlag( adv.DataAvailFlag )
{

}
 
//////////////////////////// 
// CTOR 
CameraStatusRegs::CameraStatusRegs(const CameraStatusRegs::BasicStatus & basic) : 
                                   m_TempHeatSink( basic.TempHeatSink ),
                                   m_TempCcd( basic.TempCcd ),
                                   m_CoolerDrive( basic.CoolerDrive ),
                                   m_InputVoltage( basic.InputVoltage ),
                                   m_TdiCounter( basic.TdiCounter ),
                                   m_SequenceCounter( basic.SequenceCounter ),
                                   m_Status( basic.Status ),
                                   m_uFrame( basic.uFrame ), 
                                   m_MostRecentFrame( 0 ),
                                   m_ReadyFrame( 0 ),
                                   m_CurrentFrame( 0 ),
                                   m_FetchCount( 0 ),    
                                   m_DataAvailFlag( basic.DataAvailFlag )
{

}

//////////////////////////// 
// DTOR 
CameraStatusRegs::~CameraStatusRegs() 
{ 

} 

//////////////////////////// 
// QUERY    STATUS      REGS
void CameraStatusRegs::QueryStatusRegs( uint16_t &	StatusReg,
						          uint16_t &	HeatsinkTempReg,
						          uint16_t &	CcdTempReg,
						          uint16_t &	CoolerDriveReg,
						          uint16_t &	VoltageReg,
						          uint16_t &	TdiCounter,
						          uint16_t &	SequenceCounter,
						          uint16_t &	MostRecentFrame,
						          uint16_t &	ReadyFrame,
						          uint16_t &	CurrentFrame)
{
    StatusReg = m_Status;
    HeatsinkTempReg = m_TempHeatSink;
    CcdTempReg = m_TempCcd;
    CoolerDriveReg = m_CoolerDrive;
    VoltageReg = m_InputVoltage;
    TdiCounter = m_TdiCounter;
    SequenceCounter = m_SequenceCounter;
    MostRecentFrame = m_MostRecentFrame;
    ReadyFrame = m_ReadyFrame;
    CurrentFrame = m_CurrentFrame;
}

//////////////////////////// 
// GET     STATUS      STR
std::string CameraStatusRegs::GetStatusStr() const
{
    std::stringstream result;
    result << "TempHeatSink = " << m_TempHeatSink;
    result << "; TempCcd = " << m_TempCcd;  
    result << "; CoolerDrive = " << m_CoolerDrive;  
    result << "; InputVoltage = " << m_InputVoltage;  
    result << "; TdiCounter = " << m_TdiCounter;  
    result << "; SequenceCounter = " << m_SequenceCounter;  
    result << "; Status [reg91] = " << m_Status;  
    result << "; uFrame = " << m_uFrame;   
    result << "; MostRecentFrame = " << m_MostRecentFrame;  
    result << "; ReadyFrame = " << m_ReadyFrame;  
    result << "; CurrentFrame = " << m_CurrentFrame;
    result << "; FetchCount = " << m_FetchCount;    
    result << "; DataAvailFlag = " << static_cast<int32_t>(m_DataAvailFlag);

    return result.str();
}


void CameraStatusRegs::Update (const CameraStatusRegs::AdvStatus & adv)
{
    m_TempHeatSink = adv.TempHeatSink;
    m_TempCcd = adv.TempCcd;
    m_CoolerDrive = adv.CoolerDrive;
    m_InputVoltage = adv.InputVoltage;
    m_TdiCounter = adv.TdiCounter;
    m_SequenceCounter = adv.SequenceCounter;
    m_Status = adv.Status;
    m_uFrame = adv.uFrame; 
    m_MostRecentFrame = adv.MostRecentFrame;
    m_ReadyFrame = adv.ReadyFrame;
    m_CurrentFrame = adv.CurrentFrame;
    m_FetchCount = adv.FetchCount;
    m_DataAvailFlag = adv.DataAvailFlag;
}

void CameraStatusRegs::Update(const CameraStatusRegs::BasicStatus & basic)
{
    m_TempHeatSink = basic.TempHeatSink;
    m_TempCcd = basic.TempCcd;
    m_CoolerDrive = basic.CoolerDrive;
    m_InputVoltage = basic.InputVoltage;
    m_TdiCounter = basic.TdiCounter;
    m_SequenceCounter = basic.SequenceCounter;
    m_Status = basic.Status;
    m_uFrame = basic.uFrame; 
    m_MostRecentFrame = 0;
    m_ReadyFrame = 0;
    m_CurrentFrame = 0;
    m_FetchCount = 0;    
    m_DataAvailFlag = basic.DataAvailFlag;

}

