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

#include "ImgFix.h" 
#include <algorithm>

//////////////////////////// 
//      SINGLE       OUPUT       ERASE
void ImgFix::SingleOuputErase( std::vector<uint16_t> & data, const int32_t rows,  
        const int32_t numImgCols,  const int32_t numLatencyPixels )
{
        for(int32_t r = 0, i=numLatencyPixels; r < rows; i += numImgCols, ++r)
        {
            std::vector<uint16_t>::iterator start = data.begin()+i;
            std::vector<uint16_t>::iterator end = start+numLatencyPixels;
            data.erase( start, end );
        }
}

    
//////////////////////////// 
//      SINGLE       OUPUT       COPY
void ImgFix::SingleOuputCopy( const std::vector<uint16_t> & data, 
      std::vector<uint16_t> & out, const int32_t rows,  const int32_t numImgCols,  
      const int32_t numLatencyPixels )
{

    // in testing found that this function is much faster than the erase function
    const int32_t actNumCols = numImgCols + numLatencyPixels;

    for(int32_t r = 0, actColsOffset=numLatencyPixels, outColsOffset=0; r < rows;
		    actColsOffset += actNumCols, outColsOffset += numImgCols, ++r)
    {
        std::vector<uint16_t>::const_iterator start = data.begin()+actColsOffset;
        std::vector<uint16_t>::const_iterator end = start + numImgCols;
        std::vector<uint16_t>::iterator outStart = out.begin() + outColsOffset;
        std::copy( start, end, outStart );
    }
}


//////////////////////////// 
//      QUAD      OUPUT       COPY
void ImgFix::QuadOuputCopy( const std::vector<uint16_t> & data, 
      std::vector<uint16_t> & out, const int32_t rows,  const int32_t cols,  
      const int32_t numLatencyPixels, const int32_t outputBuffOffset )
{
    int32_t numGood =  ( cols / 2 ) * 4;
    int32_t numBad = numLatencyPixels*2;

    int32_t down = rows*cols;
    
    int32_t goodStart = 0;
    int32_t badStart = numLatencyPixels*2;

    while( down > 0 )
    {
         int32_t len = std::min<int32_t>( down, numGood );

        std::vector<uint16_t>::const_iterator start = data.begin()+badStart;
        std::vector<uint16_t>::const_iterator end = start + len;
        std::vector<uint16_t>::iterator outStart = out.begin() + outputBuffOffset + goodStart;
        std::copy( start, end, outStart );

         goodStart += len;
         badStart += (len + numBad);
         down -= len;
    }
}

//////////////////////////// 
//      QUAD       OUPUT       FIX
void ImgFix::QuadOuputFix( const std::vector<uint16_t> & data, 
                                             std::vector<uint16_t> & out,
                                             const int32_t rows,  const int32_t cols,
                                             const int32_t numLatencyPixels)
{
    const int32_t HALF_COLS = cols / 2;
    const int32_t HALF_ROWS = rows / 2;
    
    int32_t index = numLatencyPixels*2;
  
    for( int32_t r=0; r < HALF_ROWS; ++r )
    {
        int32_t topOffset = cols*r;
        int32_t bottomOffset = (cols*(rows-(r+1)));

        for( int32_t c=0; c < HALF_COLS; ++c)
        {
            int32_t ul = topOffset + c;
            out[ul] = data[index];

            int32_t ur =  topOffset + (cols-(c+1) );
            ++index;
            out[ur] = data[index];
            
            int32_t lr = bottomOffset + (cols-(c+1) );
            ++index;
            out[lr] = data[index];

            int32_t ll = bottomOffset+c;
            ++index;
            out[ll] = data[index];

            ++index;
        }

        //skip the latency pixels
        index += numLatencyPixels*2;
    }
}

//////////////////////////// 
//      DUAL       OUPUT       FIX
void ImgFix::DualOuputFix( const std::vector<uint16_t> & data, 
                                             std::vector<uint16_t> & out,
                                             const int32_t rows,  const int32_t cols,
                                             const int32_t numLatencyPixels)
{
   
    const int32_t HALF_COLS = cols / 2;

     //account for the odd no op col
    const int32_t oddAdjust = ( cols % 2 ) ? 1 : 0;
    const int32_t START_UR_COL = cols;

    int32_t index = numLatencyPixels;
  
    for( int32_t r=0; r < rows; ++r )
    {
        int32_t topOffset = cols*r;

        for( int32_t c=0; c < HALF_COLS; ++c)
        {
            // skip odd col if need with oddAdjust
            int32_t ur =  topOffset + (START_UR_COL-(c+1) ) - oddAdjust;
            out[ur] = data[index];

           int32_t ul = topOffset + c;
            ++index;
            out[ul] = data[index];
            
            ++index;
        }

        //skip the latency pixels
        index += numLatencyPixels;
    }
}
