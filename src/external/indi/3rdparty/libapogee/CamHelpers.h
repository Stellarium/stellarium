/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \brief Namespaces that contain camera information and bootsraping functions to create new camera objects 
* 
*/ 


#ifndef CAMHELPERS_INCLUDE_H__ 
#define CAMHELPERS_INCLUDE_H__ 

#include <vector>
#include <stdint.h>
#include <string>

// NOTE: when added a new register make sure to also add it to 
// std::vector<uint16_t> CameraRegs::GetAll()
namespace CameraRegs
{
    const uint16_t CMD_A                                                = 0;
    const uint16_t CMD_A_EXPOSE_BIT                       = 0x0001;
    const uint16_t CMD_A_DARK_BIT                            = 0x0002;
    const uint16_t CMD_A_TEST_BIT				                = 0x0004;
    const uint16_t CMD_A_TDI_BIT				                = 0x0008;
    const uint16_t CMD_A_FLUSH_BIT			                = 0x0010;
    const uint16_t CMD_A_TRIGGER_EXPOSE_BIT	= 0x0020;
    const uint16_t CMD_A_KINETICS_BIT			            = 0x0040;
    const uint16_t CMD_A_PIPELINE_BIT			            = 0x0080;

    const uint16_t CMD_B						                            = 1;
    const uint16_t CMD_B_RESET_BIT				                = 0x0002;
    const uint16_t CMD_B_CLEAR_ALL_BIT			            = 0x0010;
    const uint16_t CMD_B_END_EXPOSURE_BIT			= 0x0080;
    const uint16_t CMD_B_RAMP_TO_SETPOINT_BIT	= 0x0200;
    const uint16_t CMD_B_RAMP_TO_AMBIENT_BIT	    = 0x0400;
    const uint16_t CMD_B_START_TEMP_READ_BIT	    = 0x2000;
    const uint16_t CMD_B_DAC_LOAD_BIT				        = 0x4000;
    const uint16_t CMD_B_AD_CONFIG_BIT			            = 0x8000;

    const uint16_t OP_A						                                    = 2;
    const uint16_t OP_A_LED_DISABLE_BIT			                = 0x0001;
    const uint16_t OP_A_PAUSE_TIMER_BIT			            = 0x0002;
    const uint16_t OP_A_RATIO_BIT					                    = 0x0004;
    const uint16_t OP_A_DELAY_MODE_BIT				            = 0x0008;
     const uint16_t OP_A_AMP_CUTOFF_DISABLE_BIT          = 0x0010;
    const uint16_t OP_A_LED_EXPOSE_DISABLE_BIT		    = 0x0020;
    const uint16_t OP_A_DISABLE_H_CLK_BIT			            = 0x0040;
    const uint16_t OP_A_SHUTTER_AMP_CONTROL_BIT	= 0x0080;
    const uint16_t OP_A_HALT_DISABLE_BIT			            = 0x0100;
    const uint16_t OP_A_EXTERNAL_READOUT_BIT	        = 0x0200;
    const uint16_t OP_A_DIGITIZATION_RES_BIT		        = 0x0400;
    const uint16_t OP_A_FORCE_SHUTTER_BIT			        = 0x0800;
    const uint16_t OP_A_DISABLE_SHUTTER_BIT		        = 0x1000;
    const uint16_t OP_A_TEMP_SUSPEND_BIT			        = 0x2000;
    const uint16_t OP_A_SHUTTER_SOURCE_BIT			    = 0x4000;
    const uint16_t OP_A_TEST_MODE_BIT				            = 0x8000;

    const uint16_t OP_B							                                        = 3;
    const uint16_t OP_B_AD_AVERAGING_BIT				            = 0x0001;
    const uint16_t OP_B_IR_PREFLASH_ENABLE_BIT		        = 0x0002;
    const uint16_t OP_B_CONT_IMAGE_ENABLE_BIT			    = 0x0004;
    const uint16_t OP_B_HCLAMP_ENABLE_BIT				            = 0x0008;
    const uint16_t OP_B_HSKIP_ENABLE_BIT				                = 0x0010;
    const uint16_t OP_B_HRAM_ENABLE_BIT				                = 0x0020;
    const uint16_t OP_B_VRAM_ENABLE_BIT				                = 0x0040;
    const uint16_t OP_B_DAC_SELECT_ZERO_BIT			            = 0x0080;
    const uint16_t OP_B_DAC_SELECT_ONE_BIT			            = 0x0100;
    const uint16_t OP_B_FIFO_WRITE_BLOCK_BIT			            = 0x0200;
    const uint16_t OP_B_DISABLE_FLUSH_COMMANDS_BIT   = 0x0800;
    const uint16_t OP_B_DISABLE_POST_EXP_FLUSH_BIT	    = 0x1000;
    const uint16_t OP_B_AD_SIM_RATE_ZERO_BIT			        = 0x2000;
    const uint16_t OP_B_AD_SIM_RATE_ONE_BIT			            = 0x4000;
    const uint16_t OP_B_AD_SIMULATION_BIT				            = 0x8000;

    const uint16_t TIMER_UPPER					    = 4;
    const uint16_t TIMER_LOWER					    = 5;
    const uint16_t HRAM_INPUT						    = 6;
    const uint16_t VRAM_INPUT						    = 7;
    const uint16_t HRAM_INV_MASK					= 8;
    const uint16_t VRAM_INV_MASK					= 9;
    const uint16_t HCLAMP_INPUT					    = 10;
    const uint16_t HSKIP_INPUT					        = 11;

    // this register is write only
    const uint16_t OP_D							                                = 12;
    const uint16_t OP_D_AD_LOAD_SELECT_BIT			    = 0x0002; // 0 = left, 1=right
    const uint16_t OP_D_DUALREADOUT_BIT	                    = 0x0008;
    const uint16_t OP_D_STOP_INTERLINE_FLUSH_BIT	= 0x0020;
    const uint16_t OP_D_STOP_TEMP_REVISIONS_BIT 	= 0x0040;
   

    const uint16_t CLAMP_COUNT					    = 13;
    const uint16_t PREROI_SKIP_COUNT		    = 14;
    const uint16_t ROI_COUNT						    = 15;
    const uint16_t POSTROI_SKIP_COUNT	    = 16;
    const uint16_t OVERSCAN_COUNT			    = 17;
    const uint16_t IMAGE_COUNT				        = 18;
    const uint16_t VFLUSH_BINNING				    = 19;
    const uint16_t SHUTTER_CLOSE_DELAY	= 20;

    const uint16_t IO_PORT_BLANKING_BITS			    = 21;
    const uint16_t IO_PORT_BLANKING_BITS_MASK	= 0x003F;

    const uint16_t SHUTTER_STROBE_POSITION		= 23;
    const uint16_t SHUTTER_STROBE_PERIOD			= 24;

    const uint16_t FAN_SPEED_CONTROL				  = 25;

	// Never actually implemented
    const uint16_t LED_DRIVE						  = 26;

	// For Aspen systems, these default values are loaded
	// into registers on powerup by microblaze processor
	// so that the fpga doesn't have to be reset
	// to access the string database. Valid bit will be
	// high if default values were programmed in the db.
    const uint16_t AD1_DEFAULT_VALUES                 = 26;
    const uint16_t AD2_DEFAULT_VALUES                 = 46;
    const uint16_t AD_DEFAULT_GAIN_BITS               = 0xFC00;
    const uint16_t AD_DEFAULT_OFFSET_BITS             = 0x03FE;
    const uint16_t AD_DEFAULT_VALID_BIT               = 0x0001;
    const uint16_t AD_DEFAULT_GAIN_SHIFT              = 10;
    const uint16_t AD_DEFAULT_OFFSET_SHIFT            = 1;


    const uint16_t SUBSTRATE_ADJUST				                    = 27;
    const uint16_t MASK_FAN_SPEED_CONTROL_ALTA		= 0x0FFF;
    const uint16_t MASK_LED_ILLUMINATION_ALTA			    = 0x0FFF;
    const uint16_t MASK_LED_ILLUMINATION_ASCENT		= 0xFFFF;
    const uint16_t MASK_SUBSTRATE_ADJUST_ALTA		    = 0x0FFF;

    const uint16_t TEST_COUNT_UPPER		= 28;
    const uint16_t TEST_COUNT_LOWER		= 29;

    const uint16_t A1_ROW_COUNT				= 30;
    const uint16_t A1_VBINNING					    = 31;
    const uint16_t A2_ROW_COUNT				= 32;
    const uint16_t A2_VBINNING					    = 33;
    const uint16_t A3_ROW_COUNT				= 34;
    const uint16_t A3_VBINNING					    = 35;
    const uint16_t A4_ROW_COUNT				= 36;
    const uint16_t A4_VBINNING					    = 37;
    const uint16_t A5_ROW_COUNT				= 38;
    const uint16_t A5_VBINNING					    = 39;

    const uint16_t TDI_BINNING					    = 44;
    const uint16_t ID_FROM_PROM			    = 45;
    const uint16_t SEQUENCE_DELAY		    = 47;
    const uint16_t TDI_RATE						    = 48;

    const uint16_t IO_PORT_DATA_WRITE	                = 49;
    const uint16_t IO_PORT_DATA_MASK                    = 0x003F;
    const uint16_t IO_PORT_DIRECTION				        = 50;
    const uint16_t IO_PORT_DIRECTION_MASK          = 0x003F;

    const uint16_t IO_PORT_ASSIGNMENT                                 = 51;
    const uint16_t IO_PORT_ASSIGNMENT_MASK                     = 0x003F;
    const uint16_t IO_PORT_ASSIGNMENT_TRIG_IN_BIT         = 0x0001;

    const uint16_t LED                      = 52;
    const uint16_t LED_A_MASK     = 0x7;
    const uint16_t LED_B_MASK     = 0x70;
    const int32_t LED_BIT_SHIFT    = 4;

    const uint16_t SCRATCH					        = 53;
    const uint16_t TDI_ROWS						    = 54;  //called tdi counts on bit map sheet
    const uint16_t TEMP_DESIRED				= 55;


    const uint16_t TEMP_RAMP_DOWN_A   = 57;
    const uint16_t TEMP_RAMP_DOWN_B   = 58;

    const uint16_t OP_C							                                = 59;
    const uint16_t OP_C_TDI_TRIGGER_GROUP_BIT		    = 0x0001;
    const uint16_t OP_C_TDI_TRIGGER_EACH_BIT		    = 0x0002;
    const uint16_t OP_C_IMAGE_TRIGGER_EACH_BIT	    = 0x0004;
    const uint16_t OP_C_IMAGE_TRIGGER_GROUP_BIT	= 0x0008;
    const uint16_t OP_C_IS_ASCENT_BIT 	                        = 0x0010;  // for v111 ascent firmware and greater
    const uint16_t OP_C_IS_INTERLINE_BIT			                = 0x0020;  // for v111 ascent firmware and greater
    const uint16_t OP_C_IS_QUAD_BIT					                = 0x0040;  // for v125 F/ascent firmware and greater
    const uint16_t OP_C_FILTER_1_BIT				                    = 0x0100;
    const uint16_t OP_C_FILTER_2_BIT				                    = 0x0200;
    const uint16_t OP_C_FILTER_3_BIT				                    = 0x0400;
    const uint16_t OP_C_FILTER_POSITION_MASK		    = 0x0700;

    const uint16_t TEMP_BACKOFF					        = 60;
    const uint16_t TEMP_COOLER_OVERRIDE		= 61;
    const uint16_t MASK_TEMP_PARAMS				= 0x0FFF;		// 12 bits

    const uint16_t AD_CONFIG_DATA					    = 62;

    const uint16_t IO_PORT_DATA_READ	                        = 90;
    const uint16_t STATUS					                                = 91;
    const uint16_t STATUS_IMAGE_EXPOSING_BIT	    = 0x0001;
    const uint16_t STATUS_IMAGING_ACTIVE_BIT		    = 0x0002;
    const uint16_t STATUS_DATA_HALTED_BIT			    = 0x0004;
    const uint16_t STATUS_IMAGE_DONE_BIT			    = 0x0008;
    const uint16_t STATUS_FLUSHING_BIT			            = 0x0010;
    const uint16_t STATUS_WAITING_TRIGGER_BIT	    = 0x0020;
    const uint16_t STATUS_SHUTTER_OPEN_BIT		    = 0x0040;
    const uint16_t STATUS_PATTERN_ERROR_BIT		= 0x0080;
    const uint16_t STATUS_TEMP_SUSPEND_ACK_BIT	= 0x0100;
    const uint16_t STATUS_FILTER_SENSE_BIT	            = 0x0200;
    const uint16_t STATUS_TEMP_REVISION_BIT		    = 0x2000;
    const uint16_t STATUS_TEMP_AT_TEMP_BIT		    = 0x4000;
    const uint16_t STATUS_TEMP_ACTIVE_BIT			    = 0x8000;

    const uint16_t TEMP_HEATSINK					        = 93;
    const uint16_t TEMP_CCD						                = 94;
    const uint16_t TEMP_DRIVE						            = 95;

    const uint16_t INPUT_VOLTAGE					        = 96;
    const uint16_t INPUT_VOLTAGE_MASK				= 0x0FFF;

    const uint16_t FIFO_DATA					        = 98;
    const uint16_t FIFO_STATUS				    = 99;
    const uint16_t ID                                         = 100;
    const uint16_t FIRMWARE_REV               = 101;
    const uint16_t FIFO_FULL_COUNT	        = 102;
    const uint16_t FIFO_EMPTY_COUNT	    = 103;
    const uint16_t TDI_COUNTER					= 104;
    const uint16_t SEQUENCE_COUNTER	= 105;
    const uint16_t TEMP_REVISED				= 106;

    // returns a vector of all of the camera's registers
    std::vector<uint16_t> GetAll();
}


///////////////////////////////////////////////////////////////////////////
namespace UsbFrmwr
{
    const uint16_t CYPRESS_VID = 0x04B4;
    const uint16_t APOGEE_VID = 0x125C;
    const uint16_t ALTA_USB_PID = 0x0010;
    const uint16_t ALTA_USB_DID = 0x0011;
    const uint16_t FILTER_WHEEL_PID = 0x0100;
    const uint16_t ASCENT_USB_PID = 0x0020;
     const uint16_t ASPEN_USB_PID = 0x0030;
    const int32_t MAX_INTEL_HEX_RECORD_LENGTH = 16;
    const int32_t REV_LENGTH = 8;

    //from the Apogee CCD USB 2.0 interface firmware user guide
    const uint8_t END_POINT = 0x86; 

    struct INTEL_HEX_RECORD
    {
	    uint8_t	Length;
	    uint16_t	Address;
	    uint8_t	Type;
	    uint8_t	Data[MAX_INTEL_HEX_RECORD_LENGTH];
    }; 

    struct IntelHexRec
    {
	    uint16_t	Address;
	    uint8_t	Type;
        std::vector<uint8_t>	Data;
    };

    std::vector<UsbFrmwr::IntelHexRec> MakeRecVect(
            UsbFrmwr::INTEL_HEX_RECORD * pRec);

    bool IsApgDevice(uint16_t vid,
    		uint16_t pid);
}

///////////////////////////////////////////////////////////////////////////
namespace Eeprom
{
    const uint16_t BLOCK_SIZE = 32768;
    const uint16_t XFER_SIZE = 4096;
    const int32_t MAX_SERIAL_NUM_BYTES = 64; 

    // This header written to the eeprom and
    // used by the fx2 firmware to 
    // load the BufCon and CamCon FPGA images. 
    #pragma pack(push,1)
    struct Header
    { 
        uint8_t CheckSum;
        uint8_t Size;
        uint8_t Version;
        uint16_t Fields;
        uint32_t BufConSize;
        uint32_t CamConSize;
        uint16_t VendorId;
        uint16_t ProductId;
        uint16_t DeviceId;
        uint8_t SerialNumIndex;
    };
     #pragma pack(pop)
    
    const uint8_t HEADER_VERSION = 1;
    const uint16_t HEADER_BUFCON_VALID_BIT = 0x1;
    const uint16_t HEADER_CAMCON_VALID_BIT = 0x2;
    const uint16_t HEADER_BOOTROM_VALID_BIT = 0x4;
    const uint16_t HEADER_VID_VALID = 0x8;
    const uint16_t HEADER_PID_VALID = 0x010;
    const uint16_t HEADER_DID_VALID = 0x020;
    const uint16_t HEADER_GPIF_VALID_BIT = 0x040;
    const uint16_t HEADER_DESCRIPTOR_VALID_BIT = 0x100;


    uint8_t CalcHdrCheckSum( const Eeprom::Header & hdr );
    bool VerifyHdrCheckSum( const Eeprom::Header & hdr );
}

namespace CamStrDb
{
    const size_t MAX_STR_DB_BYTES = 8192;
    const size_t MAX_NUM_STR = 256;
    const size_t MAX_STR_SIZE = 256;
    std::vector<uint8_t> PackStrings( const std::vector<std::string> & info );
    std::vector<std::string> UnpackStrings( const std::vector<uint8_t> & data );
}

namespace CamconFrmwr
{
    // this is the inital release version of the Ascent based camera firmware
    // the v108 verison uses the AltaU style patterns, no dual support, dip switches
    // for camera id (not EEPROM), etc.  Some AltaF were also shipped with this
    // revision.
    const uint16_t ASC_BASED_BASIC_FEATURES = 108;
}
#endif
