/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class AltaData 
* \brief Alta platform data 
* 
*/ 

#include "AltaData.h" 

namespace
{
    const uint16_t APN_HBINNING_MAX_ALTA      = 10;
    const uint16_t APN_VBINNING_MAX_ALTA      = 2048;

    const double APN_TIMER_RESOLUTION_ALTA          = 0.00000256;
    const double APN_PERIOD_TIMER_RESOLUTION_ALTA   = 0.000000040;

    const long APN_TIMER_OFFSET_COUNT_ALTA          = 3;

    const double APN_SEQUENCE_DELAY_RESOLUTION_ALTA = 0.000327;
    const double APN_SEQUENCE_DELAY_MAXIMUM_ALTA    = 21.429945;
    const double APN_SEQUENCE_DELAY_MINIMUM_ALTA    = 0.000327;

    const double APN_EXPOSURE_TIME_MIN_ALTA         = 0.00001;		// 10us is the defined min.
    const double APN_EXPOSURE_TIME_MAX_ALTA         = 10990.0;		// seconds

    const double APN_TDI_RATE_RESOLUTION_ALTA       = 0.00000512;
    const double APN_TDI_RATE_MIN_ALTA              = 0.00000512;
    const double APN_TDI_RATE_MAX_ALTA              = 0.335;
    const double APN_TDI_RATE_DEFAULT_ALTA          = 0.100;

    const double APN_VOLTAGE_RESOLUTION_ALTA        = 0.00439453;

    const double APN_SHUTTER_CLOSE_DIFF_ALTA        = 0.00001024;

    const double APN_STROBE_TIMER_RESOLUTION_ALTA  = APN_TIMER_RESOLUTION_ALTA;
    const double APN_STROBE_POSITION_MIN_ALTA       = 0.00000331;
    const double APN_STROBE_POSITION_MAX_ALTA       = 0.1677;
    const double APN_STROBE_POSITION_DEFAULT_ALTA   = 0.001;

    const double APN_STROBE_PERIOD_MIN_ALTA         = 0.000000045;
    const double APN_STROBE_PERIOD_MAX_ALTA         = 0.0026;
    const double APN_STROBE_PERIOD_DEFAULT_ALTA     = 0.001;

    const long APN_TEMP_COUNTS_ALTA                 = 4096;
    const double APN_TEMP_KELVIN_SCALE_OFFSET_ALTA  = 273.16;

    //documented with register 55, desired temp, in firmware doc
    const double APN_TEMP_SETPOINT_MIN_ALTA          = -60.0; // ~213 Kelvin
    const double APN_TEMP_SETPOINT_MAX_ALTA         = 39.0; // ~313 Kelvin

    const double APN_TEMP_BACKOFFPOINT_MIN_ALTA = 0.1;

    //per discussion with wayne 16 march 2010, the maximum input value into the
    //backoff register should be 1000.  1000*0.024414 ~ 24.0
    const double APN_TEMP_BACKOFFPOINT_MAX_ALTA        = 24.0;

    const double APN_TEMP_HEATSINK_MIN_ALTA         = 240;
    const double APN_TEMP_HEATSINK_MAX_ALTA         = 340;

    const long APN_TEMP_SETPOINT_ZERO_POINT_ALTA    = 2458;  // emperically determined zero celuius point
    const long APN_TEMP_HEATSINK_ZERO_POINT_ALTA    = 1351;

    const double APN_TEMP_DEGREES_PER_BIT_ALTA      = 0.025146;

    const uint16_t APN_FAN_SPEED_OFF_ALTA     = 0;
    const uint16_t APN_FAN_SPEED_LOW_ALTA     = 3300;
    const uint16_t APN_FAN_SPEED_MEDIUM_ALTA  = 3660;
    const uint16_t APN_FAN_SPEED_HIGH_ALTA    = 4095;

    const double APN_PREFLASH_DURATION_ALTA         = 0.160;
    const uint16_t COOLER_DRIVE_MAX = 3200;
    const double COOLER_DRIVE_OFFSET = 600.0;
    const double COOLER_DRIVE_DIVISOR = 2600.0;

}

//////////////////////////// 
// CTOR 
AltaData::AltaData() : PlatformData(
                                    APN_HBINNING_MAX_ALTA,      
                                    APN_VBINNING_MAX_ALTA,      
                                    APN_TIMER_RESOLUTION_ALTA,
                                    APN_PERIOD_TIMER_RESOLUTION_ALTA,
                                    APN_TIMER_OFFSET_COUNT_ALTA,    
                                    APN_SEQUENCE_DELAY_RESOLUTION_ALTA,
                                    APN_SEQUENCE_DELAY_MAXIMUM_ALTA,
                                    APN_SEQUENCE_DELAY_MINIMUM_ALTA,   
                                    APN_EXPOSURE_TIME_MIN_ALTA,      
                                    APN_EXPOSURE_TIME_MAX_ALTA,        
                                    APN_TDI_RATE_RESOLUTION_ALTA,
                                    APN_TDI_RATE_MIN_ALTA,      
                                    APN_TDI_RATE_MAX_ALTA,             
                                    APN_TDI_RATE_DEFAULT_ALTA,         
                                    APN_VOLTAGE_RESOLUTION_ALTA,
                                    APN_SHUTTER_CLOSE_DIFF_ALTA,       
                                    APN_STROBE_TIMER_RESOLUTION_ALTA,
                                    APN_STROBE_POSITION_MIN_ALTA,
                                    APN_STROBE_POSITION_MAX_ALTA,      
                                    APN_STROBE_POSITION_DEFAULT_ALTA,
                                    APN_STROBE_PERIOD_MIN_ALTA,  
                                    APN_STROBE_PERIOD_MAX_ALTA,        
                                    APN_STROBE_PERIOD_DEFAULT_ALTA,
                                    APN_TEMP_COUNTS_ALTA,      
                                    APN_TEMP_KELVIN_SCALE_OFFSET_ALTA,
                                    APN_TEMP_SETPOINT_MIN_ALTA, 
                                    APN_TEMP_SETPOINT_MAX_ALTA, 
                                    APN_TEMP_BACKOFFPOINT_MIN_ALTA,
                                    APN_TEMP_BACKOFFPOINT_MAX_ALTA,
                                    APN_TEMP_HEATSINK_MIN_ALTA,        
                                    APN_TEMP_HEATSINK_MAX_ALTA,        
                                    APN_TEMP_SETPOINT_ZERO_POINT_ALTA,
                                    APN_TEMP_HEATSINK_ZERO_POINT_ALTA,   
                                    APN_TEMP_DEGREES_PER_BIT_ALTA,     
                                    APN_FAN_SPEED_OFF_ALTA,
                                    APN_FAN_SPEED_LOW_ALTA,    
                                    APN_FAN_SPEED_MEDIUM_ALTA,
                                    APN_FAN_SPEED_HIGH_ALTA, 
                                    APN_PREFLASH_DURATION_ALTA,
                                    COOLER_DRIVE_MAX,
                                    COOLER_DRIVE_OFFSET,
                                    COOLER_DRIVE_DIVISOR)       
{ 

} 

//////////////////////////// 
// DTOR 
AltaData::~AltaData() 
{ 

}
