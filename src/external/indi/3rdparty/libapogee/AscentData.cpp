/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class AscentData 
* \brief Ascent platform constants 
* 
*/ 

#include "AscentData.h" 

namespace
{
    const uint16_t APN_HBINNING_MAX_ASCENT      = 20;
    const uint16_t APN_VBINNING_MAX_ASCENT      = 4095;	
                                                        
    const double APN_TIMER_RESOLUTION_ASCENT          = 0.00000133;	
    const double APN_PERIOD_TIMER_RESOLUTION_ASCENT   = 0.00000002078;
                                                        
    const long APN_TIMER_OFFSET_COUNT_ASCENT          = 3;
                                                        
    const double APN_SEQUENCE_DELAY_RESOLUTION_ASCENT = 0.00037547;
    const double APN_SEQUENCE_DELAY_MAXIMUM_ASCENT    = 24.606426;	
    const double APN_SEQUENCE_DELAY_MINIMUM_ASCENT    = 0.00037547;	
                                                        
    const double APN_EXPOSURE_TIME_MIN_ASCENT           = 0.00001;
    const double APN_EXPOSURE_TIME_MAX_ASCENT          = 5712.3;
                                                        
    const double APN_TDI_RATE_RESOLUTION_ASCENT        = 0.00000533;
    const double APN_TDI_RATE_MIN_ASCENT                         = 0.00000533;	
    const double APN_TDI_RATE_MAX_ASCENT                        = 0.349;
    const double APN_TDI_RATE_DEFAULT_ASCENT               = 0.100;
                                                        
    const double APN_VOLTAGE_RESOLUTION_ASCENT        = 0.00439453;
                                                        
    const double APN_SHUTTER_CLOSE_DIFF_ASCENT          = 0.00000533;
                                                        
    const double APN_STROBE_POSITION_MIN_ASCENT                   = 0.00000533;
    const double APN_STROBE_POSITION_MAX_ASCENT                  = 0.3493;
    const double APN_STROBE_POSITION_DEFAULT_ASCENT         = 0.001;
                                                        
    const double APN_STROBE_TIMER_RESOLUTION_ASCENT          = 0.00001067;	
    const double APN_STROBE_PERIOD_MIN_ASCENT                = 0.000000026;
    const double APN_STROBE_PERIOD_MAX_ASCENT              = 0.00136;
    const double APN_STROBE_PERIOD_DEFAULT_ASCENT     = 0.001;
                                                        
    const long APN_TEMP_COUNTS_ASCENT                 = 4096;		
    const double APN_TEMP_KELVIN_SCALE_OFFSET_ASCENT  = 273.16;
                                                 
    //documented with register 55, desired temp, in firmware doc
    const double APN_TEMP_SETPOINT_MIN_ASCENT         = -60.0; // ~213 Kelvin
    const double APN_TEMP_SETPOINT_MAX_ASCENT         = 39.0; // ~313 Kelvin

    const double APN_TEMP_BACKOFFPOINT_MIN_ASCENT         = 0.1;
    //per discussion with wayne 16 march 2010, the maximum input value into the
    //backoff register should be 1000.  1000*0.024414 ~ 24.0
    const double APN_TEMP_BACKOFFPOINT_MAX_ASCENT        = 24.0;
                                                        
    const double APN_TEMP_HEATSINK_MIN_ASCENT         = 240;
    const double APN_TEMP_HEATSINK_MAX_ASCENT         = 240;
                                                        
    const long APN_TEMP_SETPOINT_ZERO_POINT_ASCENT    = 2458;  // emperically determined zero celuius point
    const long APN_TEMP_HEATSINK_ZERO_POINT_ASCENT    = 2458;
                                                        
    const double APN_TEMP_DEGREES_PER_BIT_ASCENT      = 0.025146;
                                                        
    const uint16_t APN_FAN_SPEED_OFF_ASCENT     = 0;	
    const uint16_t APN_FAN_SPEED_LOW_ASCENT     = 49611;		
    const uint16_t APN_FAN_SPEED_MEDIUM_ASCENT  = 56206;	
    const uint16_t APN_FAN_SPEED_HIGH_ASCENT    = 63731;		
                                                        
    const double APN_PREFLASH_DURATION_ASCENT         = 0.160;
    const uint16_t COOLER_DRIVE_MAX = 60000;
    const double COOLER_DRIVE_OFFSET = 15000.0;
    const double COOLER_DRIVE_DIVISOR = 45000.0;
}

////////////////////////////                            
// CTOR                                                 
AscentData::AscentData() : PlatformData(                
                                    APN_HBINNING_MAX_ASCENT,      
                                    APN_VBINNING_MAX_ASCENT,      
                                    APN_TIMER_RESOLUTION_ASCENT,
                                    APN_PERIOD_TIMER_RESOLUTION_ASCENT,
                                    APN_TIMER_OFFSET_COUNT_ASCENT,    
                                    APN_SEQUENCE_DELAY_RESOLUTION_ASCENT,
                                    APN_SEQUENCE_DELAY_MAXIMUM_ASCENT,
                                    APN_SEQUENCE_DELAY_MINIMUM_ASCENT,   
                                    APN_EXPOSURE_TIME_MIN_ASCENT,      
                                    APN_EXPOSURE_TIME_MAX_ASCENT,        
                                    APN_TDI_RATE_RESOLUTION_ASCENT,
                                    APN_TDI_RATE_MIN_ASCENT,      
                                    APN_TDI_RATE_MAX_ASCENT,             
                                    APN_TDI_RATE_DEFAULT_ASCENT,         
                                    APN_VOLTAGE_RESOLUTION_ASCENT,
                                    APN_SHUTTER_CLOSE_DIFF_ASCENT,       
                                    APN_STROBE_TIMER_RESOLUTION_ASCENT,
                                    APN_STROBE_POSITION_MIN_ASCENT,
                                    APN_STROBE_POSITION_MAX_ASCENT,      
                                    APN_STROBE_POSITION_DEFAULT_ASCENT,
                                    APN_STROBE_PERIOD_MIN_ASCENT,  
                                    APN_STROBE_PERIOD_MAX_ASCENT,        
                                    APN_STROBE_PERIOD_DEFAULT_ASCENT,
                                    APN_TEMP_COUNTS_ASCENT,      
                                    APN_TEMP_KELVIN_SCALE_OFFSET_ASCENT,
                                    APN_TEMP_SETPOINT_MIN_ASCENT, 
                                    APN_TEMP_SETPOINT_MAX_ASCENT,   
                                    APN_TEMP_BACKOFFPOINT_MIN_ASCENT,
                                    APN_TEMP_BACKOFFPOINT_MAX_ASCENT,
                                    APN_TEMP_HEATSINK_MIN_ASCENT,        
                                    APN_TEMP_HEATSINK_MAX_ASCENT,        
                                    APN_TEMP_SETPOINT_ZERO_POINT_ASCENT,
                                    APN_TEMP_HEATSINK_ZERO_POINT_ASCENT,   
                                    APN_TEMP_DEGREES_PER_BIT_ASCENT,     
                                    APN_FAN_SPEED_OFF_ASCENT,
                                    APN_FAN_SPEED_LOW_ASCENT,    
                                    APN_FAN_SPEED_MEDIUM_ASCENT,
                                    APN_FAN_SPEED_HIGH_ASCENT, 
                                    APN_PREFLASH_DURATION_ASCENT,
                                    COOLER_DRIVE_MAX,
                                    COOLER_DRIVE_OFFSET,
                                    COOLER_DRIVE_DIVISOR)        
{ 

} 
//////////////////////////// 
// DTOR 
AscentData::~AscentData() 
{ 

}
