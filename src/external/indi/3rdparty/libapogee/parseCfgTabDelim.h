/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \brief This name space takes file names as input an pass out camera data structs 
* 
*/ 


#ifndef PARSECFGTABDELIM_INCLUDE_H__ 
#define PARSECFGTABDELIM_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

#include "CamCfgMatrix.h" 

namespace parseCfgTabDelim
{ 
    
   //------------------------------------------------
   // FUNCTIONS
   bool IsPatternFile( const std::string & fileName );
   bool IsVerticalFile( const std::string & fileName );
   bool IsCfgFile( const std::string & fileName );
 
   CamCfg::APN_HPATTERN_FILE FetchHorizontalPattern(
         const std::string & fileName);

   CamCfg::APN_VPATTERN_FILE FetchVerticalPattern( 
       const std::string & fileName);

    void FetchMetaData(const std::string & fileName,
        std::vector< std::shared_ptr<CamCfg::APN_CAMERA_METADATA> > & out);

    CamCfg::APN_CAMERA_METADATA FetchMetaData(
        const std::string & fileName, uint16_t CamId );
}; 

#endif
