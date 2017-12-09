/*!
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*  
* Copyright (c) 2009 Apogee Instruments, Inc.
* \class helpers
* \brief This is namespace contains a set of helper
*          functions.
*/

#include "helpers.h"
#include <sstream>
#include <algorithm>
#include <functional>

using namespace std;

//----------------------------------------------
//  USHORT      2       STR
std::string help::uShort2Str(const uint16_t val, bool hexOut)
{
    stringstream ss;

    if( hexOut )
    {
        ss <<  "0x" << hex << val;
    }
    else
    {
        ss << val;
    }

    return ss.str();
}

//----------------------------------------------
//  STR      2       USHORT
uint16_t help::Str2uShort(const std::string & str, bool hexIn)
{
    uint16_t val = 0;
    stringstream is( str );

    if(hexIn)
    {
        is >> std::hex >> val;
    }
    else
    {
        is >> val;
    }
    return val;
}


//----------------------------------------------
//  STR      2       DOUBLE
double help::Str2Double(const std::string & str)
{
    double val = 0;
    stringstream is( str );
    is >> val;
    return val;
}


//----------------------------------------------
//  MAKE        TOKENS
std::vector<std::string> help::MakeTokens(const std::string &str,
const std::string &separator)
{
    std::vector<std::string> returnVector;
    std::string::size_type start = 0;
    std::string::size_type end = 0;

    while( (end = str.find (separator, start)) != std::string::npos)
    {
        returnVector.push_back (str.substr (start, end-start));
        start = end + separator.size();
    }

    returnVector.push_back( str.substr(start) );

    return returnVector;
}

//----------------------------------------------
//  FIX  PATH
 std::string help::FixPath(const std::string & inDir)
 {
     std::string result = inDir;

     std::replace_if(result.begin(), result.end(), std::bind2nd(std::equal_to<int8_t>(),'\\'), '/');

    if( 0 != result.compare( result.size()-1, 1, "/" ) )
    {
        result.append("/");
    }

    return result;
 }


//------------------------
//	GET		HIGH		WORD
uint16_t help::GetHighWord( const uint32_t value)
{
	return ( (value >> 16) & 0x0000FFFF );
}

//------------------------
//		GET		LOW		WORD
uint16_t help::GetLowWord( const uint32_t value)
{
	return (value & 0xFFFF);
}

//------------------------
//		GET		LOW		BYTE
uint8_t help::GetLowByte( const uint16_t value )
{
	return (value & 0xFF);
}

//------------------------
//		GET		HIGH		BYTE
uint8_t help::GetHighByte( const uint16_t value )
{
	return( (value >> 8) & 0xFF );
}


//------------------------
//	GET    ITEM    FROM     FIND       STR
std::string help::GetItemFromFindStr( const std::string & msg,
                                     const std::string & item )
{

	//search the single device input string for the requested item
    std::vector<std::string> params =  help::MakeTokens( msg, "," );
	std::vector<std::string>::iterator iter;

	for(iter = params.begin(); iter != params.end(); ++iter)
	{
	   if( std::string::npos != (*iter).find( item ) )
	   {
		 std::string result =  help::MakeTokens( (*iter), "=" ).at(1);
		 
		 return result;
	   }
	} //for

	std::string noOp;
	return noOp;
}

