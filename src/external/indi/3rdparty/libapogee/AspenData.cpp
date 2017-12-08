/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2013 Apogee Instruments, Inc. 
* \class AspenData 
* \brief Aspen platform constants 
* 
*/ 

#include "AspenData.h" 

namespace
{
    const uint16_t APN_HBINNING_MAX_ASPEN      = 20;
    const uint16_t APN_VBINNING_MAX_ASPEN      = 4095;	
                                                        
    const double APN_TIMER_RESOLUTION_ASPEN          = 0.00000133;	
    const double APN_PERIOD_TIMER_RESOLUTION_ASPEN   = 0.00000002078;
                                                        
    const long APN_TIMER_OFFSET_COUNT_ASPEN          = 3;
                                                        
    const double APN_SEQUENCE_DELAY_RESOLUTION_ASPEN = 0.00037547;
    const double APN_SEQUENCE_DELAY_MAXIMUM_ASPEN    = 24.606426;	
    const double APN_SEQUENCE_DELAY_MINIMUM_ASPEN    = 0.00037547;	
                                                        
    const double APN_EXPOSURE_TIME_MIN_ASPEN           = 0.00001;
    const double APN_EXPOSURE_TIME_MAX_ASPEN          = 5712.3;
                                                        
    const double APN_TDI_RATE_RESOLUTION_ASPEN        = 0.00000533;
    const double APN_TDI_RATE_MIN_ASPEN                         = 0.00000533;	
    const double APN_TDI_RATE_MAX_ASPEN                        = 0.349;
    const double APN_TDI_RATE_DEFAULT_ASPEN               = 0.100;
                                                        
    const double APN_VOLTAGE_RESOLUTION_ASPEN        = 0.00439453;
                                                        
    const double APN_SHUTTER_CLOSE_DIFF_ASPEN          = 0.00000533;
                                                        
    const double APN_STROBE_TIMER_RESOLUTION_ASPEN          = 0.00001067;	
    const double APN_STROBE_POSITION_MIN_ASPEN                   = 0.00000533;
    const double APN_STROBE_POSITION_MAX_ASPEN                  = 0.3493;
    const double APN_STROBE_POSITION_DEFAULT_ASPEN         = 0.001;
                                                        
    const double APN_STROBE_PERIOD_MIN_ASPEN                = 0.000000026;
    const double APN_STROBE_PERIOD_MAX_ASPEN              = 0.00136;
    const double APN_STROBE_PERIOD_DEFAULT_ASPEN     = 0.001;
                                                        
    const long APN_TEMP_COUNTS_ASPEN                 = 4096;		
    const double APN_TEMP_KELVIN_SCALE_OFFSET_ASPEN  = 273.16;
                                                 
    //documented with register 55, desired temp, in firmware doc
    const double APN_TEMP_SETPOINT_MIN_ASPEN         = -60.0; // ~213 Kelvin
    const double APN_TEMP_SETPOINT_MAX_ASPEN         = 39.0; // ~313 Kelvin

    const double APN_TEMP_BACKOFFPOINT_MIN_ASPEN         = 0.1;
    //per discussion with wayne 16 march 2010, the maximum input value into the
    //backoff register should be 1000.  1000*0.024414 ~ 24.0
    const double APN_TEMP_BACKOFFPOINT_MAX_ASPEN        = 24.0;
                                                        
    const double APN_TEMP_HEATSINK_MIN_ASPEN         = 240;
    const double APN_TEMP_HEATSINK_MAX_ASPEN         = 240;
                                                        
    const long APN_TEMP_SETPOINT_ZERO_POINT_ASPEN    = 2518;  // emperically determined zero celuius point
    const long APN_TEMP_HEATSINK_ZERO_POINT_ASPEN    = 2518;
                                                        
    const double APN_TEMP_DEGREES_PER_BIT_ASPEN      = 0.025146;
                                                        
    const uint16_t APN_FAN_SPEED_OFF_ASPEN     = 0;	
    const uint16_t APN_FAN_SPEED_LOW_ASPEN     = 18000;		
    const uint16_t APN_FAN_SPEED_MEDIUM_ASPEN  = 25000;	
    const uint16_t APN_FAN_SPEED_HIGH_ASPEN    = 35000;		
                                                        
    const double APN_PREFLASH_DURATION_ASPEN         = 0.160;
    const uint16_t COOLER_DRIVE_MAX = 55000;
    const double COOLER_DRIVE_OFFSET = 10000.0;
    const double COOLER_DRIVE_DIVISOR = 45000.0;
}

////////////////////////////                            
// CTOR                                                 
AspenData::AspenData() : PlatformData(                
                                    APN_HBINNING_MAX_ASPEN,      
                                    APN_VBINNING_MAX_ASPEN,      
                                    APN_TIMER_RESOLUTION_ASPEN,
                                    APN_PERIOD_TIMER_RESOLUTION_ASPEN,
                                    APN_TIMER_OFFSET_COUNT_ASPEN,    
                                    APN_SEQUENCE_DELAY_RESOLUTION_ASPEN,
                                    APN_SEQUENCE_DELAY_MAXIMUM_ASPEN,
                                    APN_SEQUENCE_DELAY_MINIMUM_ASPEN,   
                                    APN_EXPOSURE_TIME_MIN_ASPEN,      
                                    APN_EXPOSURE_TIME_MAX_ASPEN,        
                                    APN_TDI_RATE_RESOLUTION_ASPEN,
                                    APN_TDI_RATE_MIN_ASPEN,      
                                    APN_TDI_RATE_MAX_ASPEN,             
                                    APN_TDI_RATE_DEFAULT_ASPEN,         
                                    APN_VOLTAGE_RESOLUTION_ASPEN,
                                    APN_SHUTTER_CLOSE_DIFF_ASPEN,       
                                    APN_STROBE_TIMER_RESOLUTION_ASPEN,
                                    APN_STROBE_POSITION_MIN_ASPEN,
                                    APN_STROBE_POSITION_MAX_ASPEN,      
                                    APN_STROBE_POSITION_DEFAULT_ASPEN,
                                    APN_STROBE_PERIOD_MIN_ASPEN,  
                                    APN_STROBE_PERIOD_MAX_ASPEN,        
                                    APN_STROBE_PERIOD_DEFAULT_ASPEN,
                                    APN_TEMP_COUNTS_ASPEN,      
                                    APN_TEMP_KELVIN_SCALE_OFFSET_ASPEN,
                                    APN_TEMP_SETPOINT_MIN_ASPEN, 
                                    APN_TEMP_SETPOINT_MAX_ASPEN,   
                                    APN_TEMP_BACKOFFPOINT_MIN_ASPEN,
                                    APN_TEMP_BACKOFFPOINT_MAX_ASPEN,
                                    APN_TEMP_HEATSINK_MIN_ASPEN,        
                                    APN_TEMP_HEATSINK_MAX_ASPEN,        
                                    APN_TEMP_SETPOINT_ZERO_POINT_ASPEN,
                                    APN_TEMP_HEATSINK_ZERO_POINT_ASPEN,   
                                    APN_TEMP_DEGREES_PER_BIT_ASPEN,     
                                    APN_FAN_SPEED_OFF_ASPEN,
                                    APN_FAN_SPEED_LOW_ASPEN,    
                                    APN_FAN_SPEED_MEDIUM_ASPEN,
                                    APN_FAN_SPEED_HIGH_ASPEN, 
                                    APN_PREFLASH_DURATION_ASPEN,
                                    COOLER_DRIVE_MAX,
                                    COOLER_DRIVE_OFFSET,
                                    COOLER_DRIVE_DIVISOR)        
{ 

} 
//////////////////////////// 
// DTOR 
AspenData::~AspenData() 
{ 

}
