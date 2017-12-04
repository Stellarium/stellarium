/*
 * Copyright (c) 2009, Roland Roberts
 */

#include "Util.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

/// \brief convert input string into vector of string tokens
///
/// \note consecutive delimiters will be treated as single delimiter
/// \note delimiters are _not_ included in return data
///
/// \param input string to be parsed
/// \param delims list of delimiters.
///
/// \see http://www.rosettacode.org/wiki/Tokenizing_A_String
std::vector<std::string> tokenize_str(const std::string &str, const std::string &delims)
{
    using namespace std;
    // Skip delims at beginning, find start of first token
    string::size_type lastPos = str.find_first_not_of(delims, 0);
    // Find next delimiter @ end of token
    string::size_type pos = str.find_first_of(delims, lastPos);

    // output vector
    vector<string> tokens;

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delims.  Note the "not_of". this is beginning of token
        lastPos = str.find_first_not_of(delims, pos);
        // Find next delimiter at end of token.
        pos = str.find_first_of(delims, lastPos);
    }

    return tokens;
}
