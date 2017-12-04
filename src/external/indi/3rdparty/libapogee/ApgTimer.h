/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
*
* \class ApgTimer 
* \brief wrapper for cross platform timing 
* 
*/ 
#ifndef APGTIMER_INCLUDE_H__ 
#define APGTIMER_INCLUDE_H__ 

#include "DefDllExport.h"

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

class ITimer;

class DLL_EXPORT ApgTimer 
{ 
    public: 
        ApgTimer(); 
        virtual ~ApgTimer(); 

        void Start();
        void Stop();

        double GetTimeInMs();
        double GetTimeInSec();

    private:
//this code removes vc++ compiler warning C4251
//from http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
#ifdef WIN_OS
#if _MSC_VER < 1600
        template class DLL_EXPORT std::shared_ptr<ITimer>;
#endif
#endif

        std::shared_ptr<ITimer> m_timer;
}; 

#endif
