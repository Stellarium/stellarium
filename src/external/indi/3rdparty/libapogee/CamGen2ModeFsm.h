/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CamGen2ModeFsm 
* \brief "finite state machine' for ascent and alta-g cameras.  use the fms lightly, because this isn't a true state machine. 
* 
*/ 


#ifndef CAMGEN2MODEFSM_INCLUDE_H__ 
#define CAMGEN2MODEFSM_INCLUDE_H__ 

#include "ModeFsm.h"
#include <string> 

class CamGen2ModeFsm : public ModeFsm
{ 
    public: 
         CamGen2ModeFsm( std::shared_ptr<CameraIo> & io,
             std::shared_ptr<CApnCamData> & camData,
             uint16_t rev);
        virtual ~CamGen2ModeFsm(); 

        bool IsTdiAvailable();
        bool IsKineticsAvailable();
        bool IsContinuousImagingAvailable();
        bool IsTriggerNormEachOn();
        bool IsTriggerNormGroupOn();
        bool IsTriggerTdiKinEachOn();
        bool IsTriggerTdiKinGroupOn();
        bool IsTriggerExternalShutterOn();
        bool IsTriggerExternalReadoutOn();

     private:
         bool IsExternalTriggerAvailable( Apg::TriggerMode trigMode );

        std::string m_fileName;

        CamGen2ModeFsm(const CamGen2ModeFsm&);
        CamGen2ModeFsm& operator=(CamGen2ModeFsm&);
}; 

#endif
