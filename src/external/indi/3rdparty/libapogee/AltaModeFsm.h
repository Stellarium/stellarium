/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
*
* \class AltaModeFsm 
* \brief "finite state machine' for alta cameras.  use the fms lightly, because this isn't a true state machine. 
* 
*/ 


#ifndef ALTAMODEFSM_INCLUDE_H__ 
#define ALTAMODEFSM_INCLUDE_H__ 

#include "ModeFsm.h"
#include <string> 

class AltaModeFsm : public ModeFsm
{ 
    public: 
        AltaModeFsm( std::shared_ptr<CameraIo> & io,
             std::shared_ptr<CApnCamData> & camData,
             uint16_t rev);

        virtual ~AltaModeFsm(); 

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
        std::string m_fileName;

        bool IsExternalTriggerAvailable( Apg::TriggerMode trigMode );

        AltaModeFsm(const AltaModeFsm&);
        AltaModeFsm& operator=(AltaModeFsm&);

}; 

#endif
