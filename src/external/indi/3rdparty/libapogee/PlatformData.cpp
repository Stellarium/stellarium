/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class PlatformData 
* \brief  
* 
*/ 

#include "PlatformData.h" 

//////////////////////////// 
// CTOR 
PlatformData::PlatformData( const uint16_t HBinningMax,
                            const uint16_t VBinningMax,
                            const double TimerResolution,
                            const double PeriodTimerResolution,
                            const long TimerOffsetCount,
                            const double SequenceDelayResolution,
                            const double SequenceDelayMaximum,
                            const double SequenceDelayMinimum,
                            const double ExposureTimeMin,
                            const double ExposureTimeMax,
                            const double TdiRateResolution,
                            const double TdiRateMin,
                            const double TdiRateMax,
                            const double TdiRateDefault,
                            const double VoltageResolution,
                            const double ShutterCloseDiff,
                            const double StrobeTimerResolution,
                            const double StrobePositionMin,
                            const double StrobePositionMax,
                            const double StrobePositionDefault,
                            const double StrobePeriodMin,
                            const double StrobePeriodMax,
                            const double StrobePeriodDefault,
                            const long TempCounts,
                            const double TempKelvinScaleOffset,                     
                            const double TempSetpointMin,
                            const double TempSetpointMax,
                            const double TempBackoffpointMin,
                            const double TempBackoffpointMax,
                            const double TempHeatsinkMin,
                            const double TempHeatsinkMax,
                            const long TempSetpointZeroPoint,
                            const long TempHeatsinkZeroPoint,
                            const double TempDegreesPerBit,
                            const uint16_t FanSpeedOff,
                            const uint16_t FanSpeedLow,
                            const uint16_t FanSpeedMedium,
                            const uint16_t FanSpeedHigh,
                            const double PreflashDuration,
                            const uint16_t CoolerDriveMax,
                            const double CoolerDriveOffset,
                            const double CoolerDriveDivisor) :
                            m_NumCols2BinMax(HBinningMax),
                            m_NumRows2BinMax(VBinningMax),
                            m_TimerResolution(TimerResolution),
                            m_PeriodTimerResolution(PeriodTimerResolution),
                            m_TimerOffsetCount(TimerOffsetCount),
                            m_SequenceDelayResolution(SequenceDelayResolution),
                            m_SequenceDelayMaximum(SequenceDelayMaximum),
                            m_SequenceDelayMinimum(SequenceDelayMinimum),
                            m_ExposureTimeMin(ExposureTimeMin),
                            m_ExposureTimeMax(ExposureTimeMax),
                            m_TdiRateResolution(TdiRateResolution),
                            m_TdiRateMin(TdiRateMin),
                            m_TdiRateMax(TdiRateMax),
                            m_TdiRateDefault(TdiRateDefault),
                            m_VoltageResolution(VoltageResolution),
                            m_ShutterCloseDiff(ShutterCloseDiff),
                            m_StrobeTimerResolution(StrobeTimerResolution),
                            m_StrobePositionMin(StrobePositionMin),
                            m_StrobePositionMax(StrobePositionMax),
                            m_StrobePositionDefault(StrobePositionDefault),
                            m_StrobePeriodMin(StrobePeriodMin),
                            m_StrobePeriodMax(StrobePeriodMax),
                            m_StrobePeriodDefault(StrobePeriodDefault),
                            m_TempCounts(TempCounts),
                            m_TempKelvinScaleOffset(TempKelvinScaleOffset),
                            m_TempSetpointMin(TempSetpointMin),
                            m_TempSetpointMax(TempSetpointMax),
                            m_TempBackoffpointMin(TempBackoffpointMin),
                            m_TempBackoffpointMax(TempBackoffpointMax),
                            m_TempHeatsinkMin(TempHeatsinkMin),
                            m_TempHeatsinkMax(TempHeatsinkMax),
                            m_TempSetpointZeroPoint(TempSetpointZeroPoint),
                            m_TempHeatsinkZeroPoint(TempHeatsinkZeroPoint),
                            m_TempDegreesPerBit(TempDegreesPerBit),
                            m_FanSpeedOff(FanSpeedOff),
                            m_FanSpeedLow(FanSpeedLow),
                            m_FanSpeedMedium(FanSpeedMedium),
                            m_FanSpeedHigh(FanSpeedHigh),
                            m_PreflashDuration(PreflashDuration),
                            m_CoolerDriveMax(CoolerDriveMax),
                            m_CoolerDriveOffset(CoolerDriveOffset),
                            m_CoolerDriveDivisor(CoolerDriveDivisor)
{
}

//////////////////////////// 
// DTOR 
PlatformData::~PlatformData() 
{ 

}
