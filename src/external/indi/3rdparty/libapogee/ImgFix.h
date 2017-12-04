/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \namespace ImgFix 
* \brief namespace for dealing with removing ad latency pixels and re-ordering pixels for dual and quad ouputs 
* 
*/ 


#ifndef IMGFIX_INCLUDE_H__ 
#define IMGFIX_INCLUDE_H__ 

#include <vector>
#include "stdint.h"

namespace ImgFix 
{ 
    void SingleOuputErase( std::vector<uint16_t> & data, int32_t rows,  
        int32_t numImgCols,  int32_t numLatencyPixels );

    void SingleOuputCopy( const std::vector<uint16_t> & data,   
        std::vector<uint16_t> & out, int32_t rows, int32_t numImgCols,  
        int32_t numLatencyPixels );

    void QuadOuputCopy( const std::vector<uint16_t> & data, 
        std::vector<uint16_t> & out, int32_t rows,  
        int32_t cols,  int32_t numLatencyPixels, int32_t outputBuffOffset=0 );

    void QuadOuputFix( const std::vector<uint16_t> & data, 
                                     std::vector<uint16_t> & out,
                                     const int32_t rows,  const int32_t cols,
                                     const int32_t numLatencyPixels );

    void DualOuputFix( const std::vector<uint16_t> & data, 
                                     std::vector<uint16_t> & out,
                                     const int32_t rows,  const int32_t cols,
                                     const int32_t numLatencyPixels );
}; 

#endif
