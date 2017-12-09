/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class ApgTimer 
* \brief wrapper for cross platform timing 
* 
*/ 

#include "ApgTimer.h" 

#include "ITimer.h"

#ifdef WIN_OS
#include "WinTimer.h"
#else
#include "LinuxTimer.h"
#endif

//////////////////////////// 
// CTOR 
ApgTimer::ApgTimer() 
{ 

#ifdef WIN_OS
    m_timer = std::shared_ptr<ITimer>( new WinTimer );
#else
    m_timer = std::shared_ptr<ITimer>( new LinuxTimer );
#endif

} 

//////////////////////////// 
// DTOR 
ApgTimer::~ApgTimer() 
{ 

} 

//////////////////////////// 
// START
void ApgTimer::Start()
{
    m_timer->Start();
}

//////////////////////////// 
// STOP
void ApgTimer::Stop()
{
    m_timer->Stop();
}

//////////////////////////// 
// GET      TIME      IN     MS
double ApgTimer::GetTimeInMs()
{
    return m_timer->GetTimeInMs();
}

//////////////////////////// 
// GET      TIME      IN     SEC
double ApgTimer::GetTimeInSec()
{
    return m_timer->GetTimeInSec();
}
