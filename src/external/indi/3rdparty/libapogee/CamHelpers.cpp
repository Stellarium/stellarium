/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc.  
* \brief Namespaces that contain camera information and bootsraping functions to create new camera objects 
* 
*/ 

#include "CamHelpers.h" 
#include "apgHelper.h" 
#include <algorithm>
#include <sstream>

//////////////////////////// 
//      GET    ALL
std::vector<uint16_t> CameraRegs::GetAll()
{
    std::vector<uint16_t> AllCamRegs;
    AllCamRegs.push_back( CMD_A );
    AllCamRegs.push_back( CMD_B );
    AllCamRegs.push_back( OP_A );
    AllCamRegs.push_back( OP_B );
    AllCamRegs.push_back( TIMER_UPPER );
    AllCamRegs.push_back( TIMER_LOWER );
    AllCamRegs.push_back( HRAM_INPUT );
    AllCamRegs.push_back( VRAM_INPUT );
    AllCamRegs.push_back( HRAM_INV_MASK );
    AllCamRegs.push_back( VRAM_INV_MASK );
    AllCamRegs.push_back( HCLAMP_INPUT );
    AllCamRegs.push_back( HSKIP_INPUT );
    AllCamRegs.push_back( OP_D );
    AllCamRegs.push_back( CLAMP_COUNT );
    AllCamRegs.push_back( PREROI_SKIP_COUNT );
    AllCamRegs.push_back( ROI_COUNT );
    AllCamRegs.push_back( POSTROI_SKIP_COUNT );
    AllCamRegs.push_back( OVERSCAN_COUNT );
    AllCamRegs.push_back( IMAGE_COUNT );
    AllCamRegs.push_back( VFLUSH_BINNING );
    AllCamRegs.push_back( SHUTTER_CLOSE_DELAY );
    AllCamRegs.push_back( IO_PORT_BLANKING_BITS );
    AllCamRegs.push_back( SHUTTER_STROBE_POSITION );
    AllCamRegs.push_back( SHUTTER_STROBE_PERIOD );
    AllCamRegs.push_back( FAN_SPEED_CONTROL );
    AllCamRegs.push_back( LED_DRIVE );
    AllCamRegs.push_back( SUBSTRATE_ADJUST );
    AllCamRegs.push_back( TEST_COUNT_UPPER );
    AllCamRegs.push_back( TEST_COUNT_LOWER );
    AllCamRegs.push_back( A1_ROW_COUNT );
    AllCamRegs.push_back( A1_VBINNING );
    AllCamRegs.push_back( A2_ROW_COUNT );
    AllCamRegs.push_back( A2_VBINNING );
    AllCamRegs.push_back( A3_ROW_COUNT );
    AllCamRegs.push_back( A3_VBINNING );
    AllCamRegs.push_back( A4_ROW_COUNT );
    AllCamRegs.push_back( A4_VBINNING );
    AllCamRegs.push_back( A5_ROW_COUNT );
    AllCamRegs.push_back( A5_VBINNING );
    AllCamRegs.push_back( TDI_BINNING );
    AllCamRegs.push_back( ID_FROM_PROM );
    AllCamRegs.push_back( SEQUENCE_DELAY );
    AllCamRegs.push_back( TDI_RATE );
    AllCamRegs.push_back( IO_PORT_DATA_WRITE );
    AllCamRegs.push_back( IO_PORT_DIRECTION );
    AllCamRegs.push_back( IO_PORT_ASSIGNMENT );
    AllCamRegs.push_back( LED );
    AllCamRegs.push_back( SCRATCH );
    AllCamRegs.push_back( TDI_ROWS );
    AllCamRegs.push_back( TEMP_DESIRED );
    AllCamRegs.push_back( TEMP_RAMP_DOWN_A );
    AllCamRegs.push_back( TEMP_RAMP_DOWN_B );
    AllCamRegs.push_back( OP_C );
    AllCamRegs.push_back( TEMP_BACKOFF );
    AllCamRegs.push_back( TEMP_COOLER_OVERRIDE );
    AllCamRegs.push_back( AD_CONFIG_DATA );
    AllCamRegs.push_back( IO_PORT_DATA_READ );
    AllCamRegs.push_back( STATUS );
    AllCamRegs.push_back( TEMP_HEATSINK );
    AllCamRegs.push_back( TEMP_CCD );
    AllCamRegs.push_back( TEMP_DRIVE );
    AllCamRegs.push_back( INPUT_VOLTAGE );
    AllCamRegs.push_back( FIFO_DATA );
    AllCamRegs.push_back( FIFO_STATUS );
    AllCamRegs.push_back( ID );
    AllCamRegs.push_back( FIRMWARE_REV );
    AllCamRegs.push_back( FIFO_FULL_COUNT );
    AllCamRegs.push_back( FIFO_EMPTY_COUNT );
    AllCamRegs.push_back( TDI_COUNTER );
    AllCamRegs.push_back( SEQUENCE_COUNTER );
    AllCamRegs.push_back( TEMP_REVISED );

    return AllCamRegs;
}


//////////////////////////// 
// MAKE      REC        VECT
 std::vector<UsbFrmwr::IntelHexRec> UsbFrmwr::MakeRecVect(
            UsbFrmwr::INTEL_HEX_RECORD * pRec)
 {
    int32_t index = 0;
    std::vector<UsbFrmwr::IntelHexRec> result;

    //fill up the new intel hex record struct with the data
    //from the older one with the flat arrays
    while( 0 == pRec[index].Type )
    {
        UsbFrmwr::IntelHexRec item;
        item.Type = pRec[index].Type;
        item.Address = pRec[index].Address;
        
        for(int32_t i=0; i < pRec[index].Length; ++i)
        {
            item.Data.push_back( pRec[index].Data[i] );
        }

        result.push_back( item );

        ++index;
    }

    return result;
}

////////////////////////////
//	IS		APG		DEVICE
bool UsbFrmwr::IsApgDevice(const uint16_t vid,
						   const uint16_t pid)
{
	if(vid == APOGEE_VID)
	{
		if(pid == ALTA_USB_PID ||
		   pid == ASCENT_USB_PID ||
		   pid == ASPEN_USB_PID ||
		   pid == FILTER_WHEEL_PID)
		{
			return true;
		}
	}

	//fall through, not an apogee device
	return false;
}

//////////////////////////// 
// CALC    HDR    CHECK   SUM
uint8_t Eeprom::CalcHdrCheckSum( const Eeprom::Header & hdr )
{
    uint8_t Check = hdr.Size;
    Check += hdr.Version;
    Check += static_cast<uint8_t>( (hdr.Fields >> 8) & 0xFF );
    Check += static_cast<uint8_t>( hdr.Fields & 0xFF );

    Check += static_cast<uint8_t>( (hdr.BufConSize >> 24) & 0xFF );
    Check += static_cast<uint8_t>( (hdr.BufConSize >> 16) & 0xFF );
    Check += static_cast<uint8_t>( (hdr.BufConSize >> 8)  & 0xFF );
    Check += static_cast<uint8_t>( (hdr.BufConSize & 0xFF) );

    Check += static_cast<uint8_t>( (hdr.CamConSize >> 24) & 0xFF );
    Check += static_cast<uint8_t>( (hdr.CamConSize >> 16) & 0xFF );
    Check += static_cast<uint8_t>( (hdr.CamConSize >> 8)  & 0xFF );
    Check += static_cast<uint8_t>( (hdr.CamConSize & 0xFF) );

    Check += static_cast<uint8_t>( hdr.VendorId & 0xFF );
    Check += static_cast<uint8_t>( (hdr.VendorId >> 8) & 0xFF );
    Check += static_cast<uint8_t>( hdr.ProductId& 0xFF );
    Check += static_cast<uint8_t>( (hdr.ProductId >> 8) & 0xFF );
    Check += static_cast<uint8_t>( hdr.DeviceId & 0xFF );
    Check += static_cast<uint8_t>( (hdr.DeviceId >> 8) & 0xFF );

    Check += hdr.SerialNumIndex;

    return( Check );
}

//////////////////////////// 
// VERIFY        HDR        CHECKSUM
bool Eeprom::VerifyHdrCheckSum( const Eeprom::Header & hdr )
{
    return( Eeprom::CalcHdrCheckSum(hdr) == hdr.CheckSum ? true : false );
}


//////////////////////////// 
//      PACK      STRINGS
std::vector<uint8_t> CamStrDb::PackStrings( const std::vector<std::string> & info )
{
     //pack the strings - from tim
    // The first two bytes get the total length, little endian.
    // We fill that in later.
    std::vector<uint8_t> buffer;
    buffer.resize( 2 );

    //check number of strings
    if( info.size() > CamStrDb::MAX_NUM_STR )
    {
        std::stringstream ss;
        ss << "Too many input strings (" << info.size() << " ).  ";
        ss << "Maximum number of strings is " << CamStrDb::MAX_NUM_STR << ".";
        apgHelper::throwRuntimeException( __FILE__, ss.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    buffer.push_back( 
         apgHelper::SizeT2Uint8( info.size() ) );

    // Copy each string individually.
    std::vector<std::string>::const_iterator iter;
    for( iter = info.begin(); iter != info.end(); ++iter )
    { 
        if( (*iter).size() > CamStrDb::MAX_STR_SIZE )
        {
            std::stringstream ss;
            ss << "Input string, " << (*iter) << ", too long (" << info.size() << " ).  ";
            ss << "Maximum string length is " << CamStrDb::MAX_STR_SIZE << ".";
            apgHelper::throwRuntimeException( __FILE__, ss.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }

        buffer.push_back( 
            apgHelper::SizeT2Uint8( (*iter).size() ) );
        buffer.insert( buffer.end(), (*iter).begin(), (*iter).end() );
    }

    //save the total size of the strings to store in flash memory
    buffer.at(0) = static_cast<uint8_t>( buffer.size() & 0xFF);
    buffer.at(1) = static_cast<uint8_t>( buffer.size() >> 8 );

    //make sure the total size is less than the max bytes set aside in
    //the flash memory
    if( buffer.size() >= CamStrDb::MAX_STR_DB_BYTES )
    {
        std::stringstream ss;
        ss << "Total buffer size too large (" << buffer.size() << " ).  ";
        ss << "Maximum string database size is ";
        ss << CamStrDb::MAX_STR_DB_BYTES << ".";
        apgHelper::throwRuntimeException( __FILE__, ss.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }
   
    return buffer;
}

//////////////////////////// 
//      UNPACK      STRINGS
std::vector<std::string> CamStrDb::UnpackStrings( const std::vector<uint8_t> & data )
{
    
    const uint16_t totalSize = data.at(1) << 8 | data.at(0);

    if( CamStrDb::MAX_STR_DB_BYTES < totalSize )
    {
        //log error and return an empty vector
        std::stringstream ss;
        ss << "Input buffer size too large (" << totalSize << " ).  ";
        ss << "Maximum string database size is ";
        ss << CamStrDb::MAX_STR_DB_BYTES << ".";
        apgHelper::LogErrorMsg( __FILE__, ss.str(), __LINE__ );

        std::vector<std::string> out;
        return out;
    }
   
    uint32_t numStrs = data.at(2);
   
   uint32_t start = 3;
   std::vector<std::string> out;

    for( uint32_t i = 0; i < numStrs; ++i )
    {
        uint32_t sz = data.at(start);
        ++start;
        if( sz > 0 )
        {
            std::string ss( reinterpret_cast<const char*>( &data.at(0)+ start), sz );
            out.push_back( ss );
            start += sz;
        }
       
    }

    return out;
}
