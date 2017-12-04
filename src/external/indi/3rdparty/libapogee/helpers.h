/*!
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*  
* Copyright (c) 2009 Apogee Instruments, Inc.
*   \namespace help
*   \brief This is namespace contains a set of helper
*          functions.
*/

#ifndef HELP_INCLUDE_H_
#define HELP_INCLUDE_H_

#include <string>
#include <vector>
#include <stdint.h>

namespace help
{

    /*!
     * Takes ushort as input and returns it as a std::string
     * \param [in] val Value to convert
     * \return std::string
     */
    std::string uShort2Str(const uint16_t val, bool hexOut=false);


    /*!
     * Takes string as input and returns it as a ushort
     * \param [in] str String to convert
     * \return ushort
     */
    uint16_t Str2uShort(const std::string & str, bool hexIn=false);

    double Str2Double(const std::string & str);

    /*!
     * Breaks input string into a vector of strings based on th
     * input separator
     * \param [in] str String to tokenize
     * \param [in] separator Characters to split string
     * \return vector of strings
     */
    std::vector<std::string> MakeTokens(const std::string &str,
        const std::string &separator);

    std::string FixPath(const std::string & inDir);

    uint16_t GetHighWord( uint32_t value);
    uint16_t GetLowWord( uint32_t value);

    uint8_t GetLowByte( uint16_t value );
    uint8_t GetHighByte( uint16_t value );

    std::string GetItemFromFindStr( const std::string & msg,
                                     const std::string & item );

    uint32_t SizeT2Uint32( size_t value );

}

#endif
