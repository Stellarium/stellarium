
/* pngerror.c - stub functions for i/o and memory allocation
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 *
 * This file provides a location for all error handling.  Users who
 * need special error handling are expected to write replacement functions
 * and use png_set_error_fn() to use those functions.  See the instructions
 * at each function.
 */

#define PNG_INTERNAL
#include "png.h"

static void png_default_error PNGARG((png_structp png_ptr,
                                      png_const_charp message));
static void png_default_warning PNGARG((png_structp png_ptr,
                                        png_const_charp message));

/* This function is called whenever there is a fatal error.  This function
 * should not be changed.  If there is a need to handle errors differently,
 * you should supply a replacement error function and use png_set_error_fn()
 * to replace the error function at run-time.
 */
void
png_error(png_structp png_ptr, png_const_charp message)
{
   if (png_ptr->error_fn != NULL)
      (*(png_ptr->error_fn))(png_ptr, message);

   /* if the following returns or doesn't exist, use the default function,
      which will not return */
   png_default_error(png_ptr, message);
}

/* This function is called whenever there is a non-fatal error.  This function
 * should not be changed.  If there is a need to handle warnings differently,
 * you should supply a replacement warning function and use
 * png_set_error_fn() to replace the warning function at run-time.
 */
void
png_warning(png_structp png_ptr, png_const_charp message)
{
   if (png_ptr->warning_fn != NULL)
      (*(png_ptr->warning_fn))(png_ptr, message);
   else
      png_default_warning(png_ptr, message);
}

/* These utilities are used internally to build an error message that relates
 * to the current chunk.  The chunk name comes from png_ptr->chunk_name,
 * this is used to prefix the message.  The message is limited in length
 * to 63 bytes, the name characters are output as hex digits wrapped in []
 * if the character is invalid.
 */
#define isnonalpha(c) ((c) < 41 || (c) > 122 || ((c) > 90 && (c) < 97))
static PNG_CONST char png_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static void
png_format_buffer(png_structp png_ptr, png_charp buffer, png_const_charp message)
{
   int iout = 0, iin = 0;

   while (iin < 4) {
      int c = png_ptr->chunk_name[iin++];
      if (isnonalpha(c)) {
         buffer[iout++] = '[';
         buffer[iout++] = png_digit[(c & 0xf0) >> 4];
         buffer[iout++] = png_digit[c & 0xf];
         buffer[iout++] = ']';
      } else {
         buffer[iout++] = c;
      }
   }

   if (message == NULL)
      buffer[iout] = 0;
   else {
      buffer[iout++] = ':';
      buffer[iout++] = ' ';
      png_memcpy(buffer+iout, message, 64);
      buffer[iout+63] = 0;
   }
}

void
png_chunk_error(png_structp png_ptr, png_const_charp message)
{
   char msg[16+64];
   png_format_buffer(png_ptr, msg, message);
   png_error(png_ptr, msg);
}

void
png_chunk_warning(png_structp png_ptr, png_const_charp message)
{
   char msg[16+64];
   png_format_buffer(png_ptr, msg, message);
   png_warning(png_ptr, msg);
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in png_set_error_fn().
 */
static void
png_default_error(png_structp png_ptr, png_const_charp message)
{
#ifndef PNG_NO_STDIO
   fprintf(stderr, "libpng error: %s\n", message);
#endif

#ifdef USE_FAR_KEYWORD
   {
      jmp_buf jmpbuf;
      png_memcpy(jmpbuf,png_ptr->jmpbuf,sizeof(jmp_buf));
      longjmp(jmpbuf, 1);
   }
#else
   longjmp(png_ptr->jmpbuf, 1);
#endif
}

/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want them to.  In the default configuration, png_ptr is
 * not used, but it is passed in case it may be useful.
 */
static void
png_default_warning(png_structp png_ptr, png_const_charp message)
{
   if (png_ptr == NULL)
      return;

#ifndef PNG_NO_STDIO
   fprintf(stderr, "libpng warning: %s\n", message);
#endif
}

/* This function is called when the application wants to use another method
 * of handling errors and warnings.  Note that the error function MUST NOT
 * return to the calling routine or serious problems will occur.  The return
 * method used in the default routine calls longjmp(png_ptr->jmpbuf, 1)
 */
void
png_set_error_fn(png_structp png_ptr, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warning_fn)
{
   png_ptr->error_ptr = error_ptr;
   png_ptr->error_fn = error_fn;
   png_ptr->warning_fn = warning_fn;
}
