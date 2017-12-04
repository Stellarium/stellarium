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


#ifndef WINTIMER_INCLUDE_H__ 
#define WINTIMER_INCLUDE_H__ 

#include <windows.h>
#include "ITimer.h"

class WinTimer : public ITimer
{ 
    public: 
        WinTimer(); 
        virtual ~WinTimer(); 

        void Start();
        void Stop();

        double GetTimeInMs();
        double GetTimeInSec();

    private:
        LARGE_INTEGER	m_Start;
        LARGE_INTEGER	m_Finish;
	    LARGE_INTEGER	m_Frequency;

}; 

#endif