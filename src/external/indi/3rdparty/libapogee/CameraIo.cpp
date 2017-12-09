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

#include "CameraIo.h" 
#include "CamUsbIo.h" 

#include "CamRegMirror.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include <sstream>


//////////////////////////// 
// CTOR 
CameraIo::CameraIo(CamModel::InterfaceType type ) : m_fileName(__FILE__),
                                                                 m_type(type),
                                                                 m_RegMirror( new CamRegMirror() )
{

}

//////////////////////////// 
// DTOR 
CameraIo::~CameraIo() 
{ 

} 

//////////////////////////// 
//      CLEAR        ALL     REGISTERS
void CameraIo::ClearAllRegisters()
{

    // Issue a clear command, so the registers are zeroed out
	// This will put the camera in a known state for us.
    WriteReg( CameraRegs::CMD_B, 
        CameraRegs::CMD_B_CLEAR_ALL_BIT);

    //clearing the mirror registers
    const std::vector<uint16_t> regs = CameraRegs::GetAll();

    std::vector<uint16_t>::const_iterator iter;

    for( iter = regs.begin(); iter != regs.end(); ++iter )
    {
        m_RegMirror->Write( (*iter), 0);
    }
}


//////////////////////////// 
// READ     MIRROR     REG
uint16_t CameraIo::ReadMirrorReg( uint16_t reg ) const
{
     return m_RegMirror->Read( reg );
}

//////////////////////////// 
// READ     REG 
uint16_t CameraIo::ReadReg( uint16_t reg ) const
{
    return m_Interface->ReadReg( reg );
}

//////////////////////////// 
// WRITE        REG 
void CameraIo::WriteReg( uint16_t reg, 
            uint16_t val )
{
    m_Interface->WriteReg( reg, val );
    m_RegMirror->Write( reg, val );
}

//////////////////////////// 
// WRITE        REG 
void CameraIo::WriteReg( const std::vector< std::pair<uint16_t,uint16_t> > & RegAndVal )
{
    std::vector< std::pair<uint16_t,uint16_t> >::const_iterator it;

    for(it = RegAndVal.begin(); it != RegAndVal .end(); ++it)
    {
        WriteReg( it->first, it->second );
    }
}

///////////////////////////////////////
// READ     OR      WRITE       REG
void CameraIo::ReadOrWriteReg(const uint16_t reg, 
                              const uint16_t val2Or)
{
    uint16_t val = ReadReg(reg);

    val |= val2Or;

    WriteReg(reg, val);
}

///////////////////////////////////////
// READ     AND      WRITE       REG
void CameraIo::ReadAndWriteReg(const uint16_t reg, 
                               const uint16_t val2And)
{
    uint16_t val = ReadReg(reg);

    val &= val2And;

    WriteReg(reg, val);
}

///////////////////////////////////////
// READ     MIRROR       OR      WRITE       REG
void CameraIo::ReadMirrorOrWriteReg(const uint16_t reg, 
                              const uint16_t val2Or)
{
    uint16_t val = ReadMirrorReg(reg);

    val |= val2Or;

    WriteReg(reg, val);
}

///////////////////////////////////////
// READ     MIRROR   AND      WRITE       REG
void CameraIo::ReadMirrorAndWriteReg(const uint16_t reg, 
                               const uint16_t val2And)
{
    uint16_t val = ReadMirrorReg(reg);

    val &= val2And;

    WriteReg(reg, val);
}

//////////////////////////// 
// WRITE        SRMD 
void CameraIo::WriteSRMD( uint16_t reg, 
                         const std::vector<uint16_t> & data )
{
    m_Interface->WriteSRMD(reg, data);
}

//////////////////////////// 
// WRITE        MRMD 
void CameraIo::WriteMRMD( uint16_t reg, 
                         const std::vector<uint16_t> & data )
{
    m_Interface->WriteMRMD(reg, data);
}

//////////////////////////// 
// GET      USB     VENDOR      INFO
void CameraIo::GetUsbVendorInfo( uint16_t & VendorId,
    uint16_t & ProductId, uint16_t  & DeviceId)
{
    if( CamModel::USB != m_type )
    {
         std::string errStr("error cannot get Usb vendor info via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

     std::dynamic_pointer_cast<CamUsbIo>(
            m_Interface)->GetUsbVendorInfo( VendorId, ProductId, DeviceId );
}

//////////////////////////// 
// SETUP     IMG     XFER
void CameraIo::SetupImgXfer(const uint16_t Rows,
            const uint16_t Cols, 
            const uint16_t NumOfImages, const bool IsBulkSeq)
{
    //checking pre-condition
    if(Rows == 0 ||
       Cols == 0 ||
       NumOfImages == 0)
    {
        std::stringstream msg;
        msg <<  "Invalid image size r = " << Rows;
        msg <<  ", c = " << Cols << " , # imgs = " << NumOfImages;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

   
     m_Interface->SetupImgXfer( Rows, Cols, NumOfImages, IsBulkSeq );
}

//////////////////////////// 
//  CANCEL     IMG     XFER
void CameraIo::CancelImgXfer()
{
    m_Interface->CancelImgXfer();
}

//////////////////////////// 
// GET  IMAGE   DATA
void CameraIo::GetImageData( std::vector<uint16_t> & data )
{
    if( 0 == data.size() )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "input vector size to GetImageData must not be zero", 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    try
    {
       m_Interface->GetImageData( data );
    }
    catch( std::exception & err )
    {
        //log that we are trying to reset the camera
        std::string msg( "Exception caught trying to fetch the image.  Canceling exposure and resetting the camera." );
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
        apgHelper::mkMsg( m_fileName, msg, __LINE__) );

        //try to get the USB interface and camera
        //in a state to image again
        Reset( true );
        CancelImgXfer();
        
        //rethrow current exception
        throw;
    }
}

//////////////////////////// 
// GET  STATUS
void CameraIo::GetStatus(CameraStatusRegs::BasicStatus & status)
{
    m_Interface->GetStatus( status );
}

//////////////////////////// 
// GET  STATUS
void CameraIo::GetStatus(CameraStatusRegs::AdvStatus & status)
{
    m_Interface->GetStatus( status );
}

//////////////////////////// 
// GET       FIRMWARE    REV
uint16_t CameraIo::GetFirmwareRev()
{
    return m_Interface->GetFirmwareRev();
}


//////////////////////////// 
// LOAD      HORIZONTAL        PATTERN
void CameraIo::LoadHorizontalPattern(const CamCfg::APN_HPATTERN_FILE & Pattern, 
            const uint16_t MaskingBit, const uint16_t RamReg, 
            const uint16_t Binning )
{

    //check the pattern file size before trying to load it
    if( 0 == Pattern.BinPatternData.size() )
    {
         apgHelper::throwRuntimeException( m_fileName, "horizontal bin pattern size of zero", 
             __LINE__, Apg::ErrorType_Configuration );
    }

    //fill up data vector to send to srmd
    std::vector<uint16_t> data(Pattern.RefPatternData); 

    //binning is the number of columns 1, 2, 3, etc, but we
    //store the data in vectors that count from 0
    const int32_t vectIndex = Binning - 1;

     data.insert( data.end(), Pattern.BinPatternData.at(vectIndex ).begin(), 
        Pattern.BinPatternData.at(vectIndex ).end() );

    data.insert( data.end(), Pattern.SigPatternData.begin(), 
        Pattern.SigPatternData.end() );

    // Prime the RAM (Enable)
    ReadOrWriteReg( CameraRegs::OP_B, 
        MaskingBit);

    //sent the data to the card
    WriteSRMD( RamReg, data );

     // RAM is now loaded (Disable)
    ReadAndWriteReg( CameraRegs::OP_B, 
        ~MaskingBit);
}


//////////////////////////// 
// LOAD      VERTICAL        PATTERN
void CameraIo::LoadVerticalPattern(const CamCfg::APN_VPATTERN_FILE & Pattern)
{
     //check the pattern file size before trying to load it
    if( 0 == Pattern.PatternData.size() )
    {
         apgHelper::throwRuntimeException( m_fileName, "vertical pattern size of zero ", 
             __LINE__, Apg::ErrorType_Configuration );
    }

  	// Prime the RAM (Enable)
    ReadOrWriteReg( CameraRegs::OP_B, 
        CameraRegs::OP_B_VRAM_ENABLE_BIT);

					
    //write the data to the camera block memory
    WriteSRMD( CameraRegs::VRAM_INPUT, Pattern.PatternData );

	// RAM is now loaded (Disable)
    ReadAndWriteReg( CameraRegs::OP_B, 
        static_cast<uint16_t>(~CameraRegs::OP_B_VRAM_ENABLE_BIT) );

}


//////////////////////////// 
// RESET 
void CameraIo::Reset(const bool Flush)
{

    //reset all counters and fifo to inital values
    WriteReg(CameraRegs::CMD_B, CameraRegs::CMD_B_RESET_BIT);

    // A little delay before we start flushing
    //very important and found by trial and error
    //by mark
	WriteReg( CameraRegs::SCRATCH, 0x8086 );
	WriteReg( CameraRegs::SCRATCH, 0x8088 );
	WriteReg( CameraRegs::SCRATCH, 0x8086 );
	WriteReg( CameraRegs::SCRATCH, 0x8088 );
	WriteReg( CameraRegs::SCRATCH, 0x8086 );
	WriteReg( CameraRegs::SCRATCH, 0x8088 );

    if( Flush )
    {
        //start the flushing process
        WriteReg(CameraRegs::CMD_A, CameraRegs::CMD_A_FLUSH_BIT);

        //same as above
        WriteReg( CameraRegs::SCRATCH, 0x8086 );
	    WriteReg( CameraRegs::SCRATCH, 0x8088 );
    }
}


//////////////////////////// 
// GET     USB        FIRMWARE        VERSION
std::string CameraIo::GetUsbFirmwareVersion()
{

    if( CamModel::USB != m_type )
    {
        std::string errStr("error cannot get Usb firwmare version via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    return std::dynamic_pointer_cast<CamUsbIo>(
            m_Interface)->GetUsbFirmwareVersion();
}

//////////////////////////// 
// GET  INFO
std::string CameraIo::GetInfo()
{
     return m_Interface->GetInfo();
}


//////////////////////////// 
// READ       BUFCON          REG 
uint8_t CameraIo::ReadBufConReg( uint16_t reg ) const
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error ReadBufConReg not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
            __LINE__, Apg::ErrorType_InvalidOperation );
    }

    return std::dynamic_pointer_cast<CamUsbIo>(
        m_Interface)->ReadBufConReg( reg ) ;
}

//////////////////////////// 
// WRITE       BUFCON          REG 
void CameraIo::WriteBufConReg( uint16_t reg, uint8_t val )
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error WriteBufConReg not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<CamUsbIo>(m_Interface)->WriteBufConReg( reg, val );
}


//////////////////////////// 
// READ       FX2          REG 
uint8_t CameraIo::ReadFx2Reg( uint16_t reg )
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error ReadFx2Reg not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    return std::dynamic_pointer_cast<CamUsbIo>(
        m_Interface)->ReadFx2Reg( reg );
}

//////////////////////////// 
// WRITE       FX2         REG 
void CameraIo::WriteFx2Reg( uint16_t reg, uint8_t val )
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error WriteFx2Reg not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<CamUsbIo>(m_Interface)->WriteFx2Reg( reg, val );
}

//////////////////////////// 
//      GET    FIRMWARE     HDR
std::string CameraIo::GetFirmwareHdr()
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error GetFirmwareHdr not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    Eeprom::Header hdr;
    std::dynamic_pointer_cast<CamUsbIo>(
        m_Interface)->ReadHeader( hdr );

    std::stringstream strm;
    strm << "BufCon Size = " << hdr.BufConSize << "\n";
    strm << "CamCon Size = " << hdr.CamConSize << "\n";
    strm << "CheckSum = "  << static_cast<int32_t>(hdr.CheckSum)  << "\n";
    strm << "DeviceId = " << hdr.DeviceId << "\n";
    strm << "Fields = "  << hdr.Fields << "\n";
    strm << "ProductId = "  << hdr.ProductId << "\n";
    strm << "Serial Number Index = "  << static_cast<int32_t>(hdr.SerialNumIndex) << "\n";
    strm << "Size ="  << static_cast<int32_t>(hdr.Size) << "\n";
    strm << "VendorId = "  << hdr.VendorId << "\n";
    strm << "Version = "  << static_cast<int32_t>(hdr.Version) << std::endl;

    return strm.str();
}

//////////////////////////// 
//      GET    SERIAL    NUMBER
std::string CameraIo::GetSerialNumber()
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error GetSerialNumber not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    return std::dynamic_pointer_cast<CamUsbIo>(
        m_Interface)->GetSerialNumber();
}

//////////////////////////// 
//      SET     SERIAL       NUMBER
void CameraIo::SetSerialNumber( const std::string & num )
{
    if( CamModel::USB != m_type )
    {
        std::string errStr("error SetSerialNumber not supported via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<CamUsbIo>(
        m_Interface)->SetSerialNumber( num );
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string CameraIo::GetDriverVersion()
{
    return m_Interface->GetDriverVersion();
}

//////////////////////////// 
//      IS     ERROR
bool CameraIo::IsError()
{
    return m_Interface->IsError();
}


//////////////////////////// 
// GET       ID       FROM     REG
uint16_t CameraIo::GetIdFromReg()
{
    return ReadReg( CameraRegs::ID );
}
   

// END       OF            CLASS
/////////////////////////////////////////////////////////////////////////////////

//////////////////////////// 
// DETERMINE    INTERFACE   TYPE
CamModel::InterfaceType InterfaceHelper::DetermineInterfaceType(const std::string & type)
{
    if( 0 == type.compare("ethernet") )
    {
        return CamModel::ETHERNET;
    }

    if( 0 == type.compare("usb") )
    {
        return CamModel::USB;
    }

    //fall through
    return CamModel::UNKNOWN_INTERFACE;
}
