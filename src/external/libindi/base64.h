#if 0
INDI
Copyright (C) 2003 Elwood C. Downey

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

#endif

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup base64 Base 64 Functions: Convert from and to base64
 */
/*@{*/

/** \brief Convert bytes array to base64.
    \param out output buffer in base64. The buffer size must be at least (4 * inlen / 3 + 4) bytes long.
    \param in input binary buffer
    \param inlen number of bytes to convert
    \return 0 on success, -1 on failure.
 */
extern int to64frombits(unsigned char *out, const unsigned char *in, int inlen);

/** \brief Convert base64 to bytes array.
    \param out output buffer in bytes. The buffer size must be at least (3 * size_of_in_buffer / 4) bytes long.
    \param in input base64 buffer
    \param inlen base64 buffer lenght
    \return 0 on success, -1 on failure.
 */

extern int from64tobits(char *out, const char *in);
extern int from64tobits_fast(char *out, const char *in, int inlen);

/*@}*/

#ifdef __cplusplus
}
#endif
