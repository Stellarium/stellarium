/*!
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class PromFx2Io 
* \brief helper class for downloading the fx2 romloader and device firmware into the proms 
* 
*/ 

#include "PromFx2Io.h" 
#include "IUsb.h"
#include "ApnUsbSys.h"
#include "apgHelper.h" 


#include <fstream>

namespace
{
    const uint16_t CPUCS_REG_FX2 = 0xE600;
}

//////////////////////////// 
// CTOR 
PromFx2Io::PromFx2Io( std::shared_ptr<IUsb> & usb,
                                            const uint32_t MaxBlocks, 
                                            const uint32_t MaxBanks ): 
                                            m_Usb( usb ),
                                            m_MaxBlocks( MaxBlocks ),
                                            m_MaxBanks( MaxBanks )

{
}

//////////////////////////// 
// DTOR 
PromFx2Io::~PromFx2Io() 
{ 

} 

//////////////////////////// 
// WRITE     FILE    2   EEPROM
void PromFx2Io::WriteFile2Eeprom(const std::string & filename, uint8_t StartBank, 
            uint8_t StartBlock, uint16_t StartAddr, uint32_t & NumBytesWritten)
{
    std::vector<uint8_t> buffer = ReadFirmwareFile( filename );
    BufferWriteEeprom( StartBank, StartBlock, StartAddr, buffer );
    NumBytesWritten = apgHelper::SizeT2Uint32( buffer.size() );
}

//////////////////////////// 
//     FIRMWARE       DOWNLOAD
 void PromFx2Io::FirmwareDownload(const std::vector<UsbFrmwr::IntelHexRec> & Records)
 {
     // from http://www.keil.com/dd/docs/datashts/cypress/fx2_trm.pdf
     //A host loader program will typically write 0x01 to the CPUCS register 
     //to put the FX2s CPU into RESET, load all or part of the FX2�s internal 
     //RAM with code, then reload the CPUCS register with0 to take the CPU 
     //out of RESET.

     //so we put the fx2 cpu reset
     std::vector<uint8_t> enter(1,1);
     m_Usb->UsbRequestOut( VND_ANCHOR_LOAD_INTERNAL,
         0, CPUCS_REG_FX2, &(*enter.begin()), 
         apgHelper::SizeT2Uint32(enter.size()) );

     //dowload the intel hex rec
     std::vector<UsbFrmwr::IntelHexRec>::const_iterator iter;

     for(iter = Records.begin(); iter != Records.end(); ++iter)
     {
         m_Usb->UsbRequestOut( VND_ANCHOR_LOAD_INTERNAL,
             0, (*iter).Address,
             &(*iter).Data.at(0),
             apgHelper::SizeT2Uint32( (*iter).Data.size() ) );
     }

     //and take the cpu out of reset
     std::vector<uint8_t> exit(1,0);
     m_Usb->UsbRequestOut( VND_ANCHOR_LOAD_INTERNAL,
        0, CPUCS_REG_FX2, &(*exit.begin()), 
        apgHelper::SizeT2Uint32(exit.size()) );
 }

//////////////////////////// 
// BUFFER   WRITE     EEPROM
 void PromFx2Io::BufferWriteEeprom(const uint8_t StartBank, const uint8_t StartBlock,
            const uint16_t StartAddr, const std::vector<uint8_t> & Buffer )
 {

	uint16_t Addr = StartAddr;
	uint8_t	Bank = StartBank;
	uint8_t	Block = StartBlock;	
    
    uint32_t numBytesSent = 0;

    // If we're not starting on a ROM_CHUNK_SIZE boundary, then do a 1st
	// "catch-up" beat.
    if ( (Addr & (Eeprom::XFER_SIZE-1)) & ((Buffer.size()+Addr) > Eeprom::XFER_SIZE) ) 
	{
        const uint16_t firstChunk = static_cast<uint16_t>(Eeprom::XFER_SIZE) - Addr;

        WriteEeprom( Addr, Bank, Block, &(*Buffer.begin()), firstChunk );

        numBytesSent += firstChunk;

        IncrEepromAddrBlockBank( firstChunk, Addr, Bank, Block );
    }

    // Mid-Beat. Once we've got to where the address is on a ROM_CHUNK_SIZE
	// boundary, do zero or more beats at the ROM_CHUNK_SIZE.
    const uint32_t numBytes2Send = apgHelper::SizeT2Uint32( Buffer.size() ) - numBytesSent;

    //static casting to a ushort b/c the max size is 4096 and we have to use this size to incr the
    //addr which is a ushort
    //doing this because of the the std::min will truncate the uint32_t
    uint16_t chunk = 0;
    if( numBytes2Send > Eeprom::XFER_SIZE )
    {
        chunk = Eeprom::XFER_SIZE;
    }
    else
    {
        //this is ok, because the value is  < 4096
        chunk = static_cast<uint16_t>(numBytes2Send);
    }
    
    const uint32_t remainder = numBytes2Send % chunk;

    std::vector<uint8_t>::const_iterator iter;

     //setting the start data iterator to the offset position
    for(iter = Buffer.begin()+numBytesSent; iter != Buffer.end() - remainder; iter += chunk)
    {
        WriteEeprom( Addr, Bank, Block, &(*iter), chunk );
	    IncrEepromAddrBlockBank( chunk, Addr, Bank, Block );
    }
  
    // At this point there's less than a ROM_CHUNK_SIZE remaining.
	// Do a final beat to close up.
	 if( remainder )
     {
        //reset the iter to get the last little bit
        iter = Buffer.end() - remainder;
        WriteEeprom( Addr, Bank, Block, &(*iter), remainder );
     }

 }

//////////////////////////// 
// WRITE     EEPROM
 void PromFx2Io::WriteEeprom(const uint16_t Addr, 
     const uint8_t Bank, const uint8_t Block, 
     const uint8_t * data, const uint32_t DataSzInBytes)
{
    const uint16_t Value = static_cast<uint16_t>( (Bank<<8) | Block );
    
    m_Usb->UsbRequestOut(VND_APOGEE_EEPROM, Addr, Value,
            data, DataSzInBytes);
}

 //////////////////////////// 
// BUFFER   READ     EEPROM
 void PromFx2Io::BufferReadEeprom(const uint8_t StartBank, const uint8_t StartBlock,
            const uint16_t StartAddr, std::vector<uint8_t> & Buffer )
 {

	uint16_t	Addr = StartAddr;
	uint8_t	Bank = StartBank;
	uint8_t	Block = StartBlock;	
    
    uint32_t numBytesRead = 0;

    // If we're not starting on a ROM_CHUNK_SIZE boundary, then do a 1st
	// "catch-up" beat.
    if ( (Addr & (Eeprom::XFER_SIZE-1)) & ((Buffer.size()+Addr) > Eeprom::XFER_SIZE) ) 
	{
        const uint16_t firstChunk = static_cast<uint16_t>(Eeprom::XFER_SIZE) - Addr;

        ReadEeprom( Addr, Bank, Block, &(*Buffer.begin()), firstChunk );

        numBytesRead+= firstChunk;

        IncrEepromAddrBlockBank( firstChunk, Addr, Bank, Block );
    }

    // Mid-Beat. Once we've got to where the address is on a ROM_CHUNK_SIZE
	// boundary, do zero or more beats at the ROM_CHUNK_SIZE.
    const uint32_t numBytes2Read = apgHelper::SizeT2Uint32( Buffer.size() ) - numBytesRead;

    //static casting to a ushort b/c the max size is 4096 and we have to use this size to incr the
    //addr which is a ushort
    //doing this because of the the std::min will truncate the uint32_t
    uint16_t chunk = 0;
    if( numBytes2Read > Eeprom::XFER_SIZE )
    {
        chunk = Eeprom::XFER_SIZE;
    }
    else
    {
        //this is ok, because the value is  < 4096
        chunk = static_cast<uint16_t>(numBytes2Read);
    }
    
    const uint32_t remainder = numBytes2Read% chunk;

    std::vector<uint8_t>::iterator iter;

     //setting the start data iterator to the offset position
    for(iter = Buffer.begin()+numBytesRead; iter != Buffer.end() - remainder; iter += chunk)
    {
        ReadEeprom( Addr, Bank, Block, &(*iter), chunk );
	    IncrEepromAddrBlockBank( chunk, Addr, Bank, Block );
    }
  
    // At this point there's less than a ROM_CHUNK_SIZE remaining.
	// Do a final beat to close up.
	 if( remainder )
     {
        //reset the iter to get the last little bit
        iter = Buffer.end() - remainder;
        ReadEeprom( Addr, Bank, Block, &(*iter), remainder );
     }

 }

 //////////////////////////// 
// USB     READ     EEPROM
 void PromFx2Io::ReadEeprom(const uint16_t Addr, 
     const uint8_t Bank, const uint8_t Block, 
     uint8_t * data, const uint32_t DataSzInBytes)
{
    const uint16_t Value = static_cast<uint16_t>( (Bank<<8) | Block );
    
    m_Usb->UsbRequestIn(VND_APOGEE_EEPROM, Addr, Value,
            data, DataSzInBytes);
}

//////////////////////////// 
// READ       FIRMWARE        FILE
std::vector<uint8_t> PromFx2Io::ReadFirmwareFile( const std::string & filename )
{
    std::ifstream file(filename.c_str(), std::ios::in|std::ios::binary);

    if( !file.is_open() )
    {
        std::string msg("Error: opening file  ");
        msg.append( filename );
        apgHelper::throwRuntimeException( __FILE__, msg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //get the file size and create the input buffer
    file.seekg(0, std::ios::end);
    const int32_t length = apgHelper::OsInt2Int32( file.tellg() );
 
    if( 0 == length )
    {
        std::string msg("Error: zero file length for file ");
        msg.append( filename );
        apgHelper::throwRuntimeException( __FILE__, msg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::vector<uint8_t> buffer( length );

    //reset file point
    file.seekg(0, std::ios::beg);

    file.read( reinterpret_cast<char*>(&buffer.at(0)), length);
    file.close();

    return buffer;
}

//////////////////////////// 
// INCR        PROM    ADDR     BLOCK       BANK
 void PromFx2Io::IncrEepromAddrBlockBank(const uint16_t IncrSize,
            uint16_t & Addr, uint8_t & Bank, 
            uint8_t & Block)
 {
    Addr	 += IncrSize;

    if ( Addr >= Eeprom::BLOCK_SIZE )
    {
        Addr = 0;
        ++Block;

        if( m_MaxBlocks <= Block)
        {                 
            Block = 0;
        
            ++Bank;

            if( m_MaxBanks <= Bank )
            {
                apgHelper::throwRuntimeException( m_fileName, 
                    "Invalid number of EEPROM banks", __LINE__,
                    Apg::ErrorType_InvalidUsage );
            }
        }
    }
 }

 //////////////////////////// 
// READ       EEPROM        HDR
void PromFx2Io::ReadEepromHdr( Eeprom::Header & hdr,
                        uint8_t StartBank, 
                        uint8_t StartBlock,
                        uint16_t StartAddr)
{
    const int32_t BufSize = sizeof( Eeprom::Header );
    std::vector<uint8_t> Buf(BufSize);

    BufferReadEeprom(StartBank, StartBlock, StartAddr, Buf );
   
    hdr.CheckSum = Buf.at(0);
    hdr.Size = Buf.at(1);
    hdr.Version = Buf.at(2);
    hdr.Fields = (Buf.at(3) << 8) | Buf.at(4);
    //big/little???
    hdr.BufConSize = (Buf.at(5) << 24) | (Buf.at(6) << 16) | (Buf.at(7) << 8)  |  Buf.at(8);
    hdr.CamConSize = (Buf.at(9) << 24) | (Buf.at(10) << 16) | (Buf.at(11) << 8)  |  Buf.at(12);
    //big/little???
    hdr.VendorId =  (Buf.at(14) << 8) | Buf.at(13);
    hdr.ProductId = (Buf.at(16) << 8) | Buf.at(15);
    hdr.DeviceId = (Buf.at(18) << 8) | Buf.at(17);
    hdr.SerialNumIndex = Buf.at(19);
}

//////////////////////////// 
// WRITE     EEPROM        HDR
void PromFx2Io::WriteEepromHdr( const Eeprom::Header & hdr,
                         uint8_t StartBank, 
                         uint8_t StartBlock,
                         uint16_t StartAddr)
{

    //have to move th header into a raw buffer because a difference in
    //endianess...yuck.
    const int32_t BufSize = sizeof( Eeprom::Header );
    std::vector<uint8_t> Buf(BufSize);

    Buf.at(0)	= hdr.CheckSum;
    Buf.at(1)	= hdr.Size;
    Buf.at(2)	= hdr.Version;
    Buf.at(3)	= static_cast<uint8_t>( (hdr.Fields >> 8) & 0xFF );
    Buf.at(4)	= static_cast<uint8_t>( hdr.Fields & 0xFF );
    Buf.at(5)	= static_cast<uint8_t>( (hdr.BufConSize >> 24) & 0xFF );
    Buf.at(6)	= static_cast<uint8_t>( (hdr.BufConSize >> 16) & 0xFF );
    Buf.at(7)	= static_cast<uint8_t>( (hdr.BufConSize >> 8)  & 0xFF );
    Buf.at(8)	= static_cast<uint8_t>( (hdr.BufConSize & 0xFF) );
    Buf.at(9)	= static_cast<uint8_t>( (hdr.CamConSize >> 24) & 0xFF );
    Buf.at(10)	= static_cast<uint8_t>( (hdr.CamConSize >> 16) & 0xFF );
    Buf.at(11)	= static_cast<uint8_t>( (hdr.CamConSize >> 8)  & 0xFF );
    Buf.at(12)	= static_cast<uint8_t>( (hdr.CamConSize & 0xFF) );
    Buf.at(13)	= static_cast<uint8_t>( hdr.VendorId & 0xFF );
    Buf.at(14)	= static_cast<uint8_t>( (hdr.VendorId >> 8) & 0xFF );
    Buf.at(15)	= static_cast<uint8_t>( hdr.ProductId & 0xFF );
    Buf.at(16)	= static_cast<uint8_t>( (hdr.ProductId >> 8) & 0xFF );
    Buf.at(17)	= static_cast<uint8_t>( hdr.DeviceId & 0xFF );
    Buf.at(18)	= static_cast<uint8_t>( (hdr.DeviceId >> 8) & 0xFF );
    Buf.at(19)	= hdr.SerialNumIndex;

     BufferWriteEeprom(StartBank, StartBlock, StartAddr, Buf );
}
