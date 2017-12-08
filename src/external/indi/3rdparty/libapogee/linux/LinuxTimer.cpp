/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class LinuxTimer 
* \brief linux implementation of the timmer 
* 
*/ 

#include "LinuxTimer.h" 
#include <cstddef>

//////////////////////////// 
// CTOR 
LinuxTimer::LinuxTimer() 
{ 

} 

//////////////////////////// 
// DTOR 
LinuxTimer::~LinuxTimer() 
{ 

} 


//////////////////////////// 
// START
void LinuxTimer::Start()
{
    gettimeofday( &m_start, NULL);
}

//////////////////////////// 
// STOP
void LinuxTimer::Stop()
{
    gettimeofday( &m_end, NULL);
}

//////////////////////////// 
// GET      TIME      IN     MS
double LinuxTimer::GetTimeInMs()
{
    double seconds  = m_end.tv_sec  - m_start.tv_sec;
    double useconds = m_end.tv_usec - m_start.tv_usec;
    double mtime = ( (seconds * 1000) + useconds/1000.0) + 0.5;
    return mtime;
}

//////////////////////////// 
// GET      TIME      IN     SEC
double LinuxTimer::GetTimeInSec()
{
  return( GetTimeInMs() / 1000.0 );
}
