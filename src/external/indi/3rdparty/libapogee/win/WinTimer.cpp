/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class WinTimer 
* \brief windows implementation of the timmer 
* 
*/ 

#include "WinTimer.h" 


//////////////////////////// 
// CTOR 
WinTimer::WinTimer() 
{ 
    QueryPerformanceFrequency(&m_Frequency);
} 

//////////////////////////// 
// DTOR 
WinTimer::~WinTimer() 
{ 

} 

//////////////////////////// 
// START
void WinTimer::Start()
{
    QueryPerformanceCounter( &m_Start );
}

//////////////////////////// 
// STOP
void WinTimer::Stop()
{
    QueryPerformanceCounter( &m_Finish );
}

//////////////////////////// 
// GET      TIME      IN     MS
double WinTimer::GetTimeInMs()
{
    return ( GetTimeInSec() * 1000.0 );
}

//////////////////////////// 
// GET      TIME      IN     SEC
double WinTimer::GetTimeInSec()
{
    LARGE_INTEGER Total;
    Total.QuadPart = (m_Finish.QuadPart - m_Start.QuadPart);
	const double result = (static_cast<double>(Total.QuadPart)/ static_cast<double>(m_Frequency.QuadPart) );
    return result;
}