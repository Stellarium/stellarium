
/* png.c - location for general purpose libpng functions
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 */

#define PNG_INTERNAL
#define PNG_NO_EXTERN
#include "png.h"

/* Version information for C files.  This had better match the version
 * string defined in png.h.
 */
char png_libpng_ver[12] = "1.0.2";

/* Place to hold the signature string for a PNG file. */
png_byte FARDATA png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};

/* Constant strings for known chunk types.  If you need to add a chunk,
 * add a string holding the name here.  If you want to make the code
 * portable to EBCDIC machines, use ASCII numbers, not characters.
 */
png_byte FARDATA png_IHDR[5] = { 73,  72,  68,  82, '\0'};
png_byte FARDATA png_IDAT[5] = { 73,  68,  65,  84, '\0'};
png_byte FARDATA png_IEND[5] = { 73,  69,  78,  68, '\0'};
png_byte FARDATA png_PLTE[5] = { 80,  76,  84,  69, '\0'};
png_byte FARDATA png_bKGD[5] = { 98,  75,  71,  68, '\0'};
png_byte FARDATA png_cHRM[5] = { 99,  72,  82,  77, '\0'};
png_byte FARDATA png_gAMA[5] = {103,  65,  77,  65, '\0'};
png_byte FARDATA png_hIST[5] = {104,  73,  83,  84, '\0'};
png_byte FARDATA png_oFFs[5] = {111,  70,  70, 115, '\0'};
png_byte FARDATA png_pCAL[5] = {112,  67,  65,  76, '\0'};
png_byte FARDATA png_pHYs[5] = {112,  72,  89, 115, '\0'};
png_byte FARDATA png_sBIT[5] = {115,  66,  73,  84, '\0'};
png_byte FARDATA png_sRGB[5] = {115,  82,  71,  66, '\0'};
png_byte FARDATA png_tEXt[5] = {116,  69,  88, 116, '\0'};
png_byte FARDATA png_tIME[5] = {116,  73,  77,  69, '\0'};
png_byte FARDATA png_tRNS[5] = {116,  82,  78,  83, '\0'};
png_byte FARDATA png_zTXt[5] = {122,  84,  88, 116, '\0'};

/* arrays to facilitate easy interlacing - use pass (0 - 6) as index */

/* start of interlace block */
int FARDATA png_pass_start[] = {0, 4, 0, 2, 0, 1, 0};

/* offset to next interlace block */
int FARDATA png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1};

/* start of interlace block in the y direction */
int FARDATA png_pass_ystart[] = {0, 0, 4, 0, 2, 0, 1};

/* offset to next interlace block in the y direction */
int FARDATA png_pass_yinc[] = {8, 8, 8, 4, 4, 2, 2};

/* Width of interlace block.  This is not currently used - if you need
 * it, uncomment it here and in png.h
int FARDATA png_pass_width[] = {8, 4, 4, 2, 2, 1, 1};
*/

/* Height of interlace block.  This is not currently used - if you need
 * it, uncomment it here and in png.h
int FARDATA png_pass_height[] = {8, 8, 4, 4, 2, 2, 1};
*/

/* Mask to determine which pixels are valid in a pass */
int FARDATA png_pass_mask[] = {0x80, 0x08, 0x88, 0x22, 0xaa, 0x55, 0xff};

/* Mask to determine which pixels to overwrite while displaying */
int FARDATA png_pass_dsp_mask[] = {0xff, 0x0f, 0xff, 0x33, 0xff, 0x55, 0xff};


/* Tells libpng that we have already handled the first "num_bytes" bytes
 * of the PNG file signature.  If the PNG data is embedded into another
 * stream we can set num_bytes = 8 so that libpng will not attempt to read
 * or write any of the magic bytes before it starts on the IHDR.
 */
void
png_set_sig_bytes(png_structp png_ptr, int num_bytes)
{
   png_debug(1, "in png_set_sig_bytes\n");
   if (num_bytes > 8)
      png_error(png_ptr, "Too many bytes for PNG signature.");

   png_ptr->sig_bytes = num_bytes < 0 ? 0 : num_bytes;
}

/* Checks whether the supplied bytes match the PNG signature.  We allow
 * checking less than the full 8-byte signature so that those apps that
 * already read the first few bytes of a file to determine the file type
 * can simply check the remaining bytes for extra assurance.  Returns
 * an integer less than, equal to, or greater than zero if sig is found,
 * respectively, to be less than, to match, or be greater than the correct
 * PNG signature (this is the same behaviour as strcmp, memcmp, etc).
 */
int
png_sig_cmp(png_bytep sig, png_size_t start, png_size_t num_to_check)
{
   if (num_to_check > 8)
      num_to_check = 8;
   else if (num_to_check < 1)
      return (0);

   if (start > 7)
      return (0);

   if (start + num_to_check > 8)
      num_to_check = 8 - start;

   return ((int)(png_memcmp(&sig[start], &png_sig[start], num_to_check)));
}

/* (Obsolete) function to check signature bytes.  It does not allow one
 * to check a partial signature.  This function will be removed in the
 * future - use png_sig_cmp().
 */
int
png_check_sig(png_bytep sig, int num)
{
  return ((int)!png_sig_cmp(sig, (png_size_t)0, (png_size_t)num));
}

/* Function to allocate memory for zlib. */
voidpf
png_zalloc(voidpf png_ptr, uInt items, uInt size)
{
   png_uint_32 num_bytes = (png_uint_32)items * size;
   png_voidp ptr = (png_voidp)png_malloc((png_structp)png_ptr, num_bytes);

   if (num_bytes > (png_uint_32)0x8000L)
   {
      png_memset(ptr, 0, (png_size_t)0x8000L);
      png_memset((png_bytep)ptr + (png_size_t)0x8000L, 0,
         (png_size_t)(num_bytes - (png_uint_32)0x8000L));
   }
   else
   {
      png_memset(ptr, 0, (png_size_t)num_bytes);
   }
   return ((voidpf)ptr);
}

/* function to free memory for zlib */
void
png_zfree(voidpf png_ptr, voidpf ptr)
{
   png_free((png_structp)png_ptr, (png_voidp)ptr);
}

/* Reset the CRC variable to 32 bits of 1's.  Care must be taken
 * in case CRC is > 32 bits to leave the top bits 0.
 */
void
png_reset_crc(png_structp png_ptr)
{
   png_ptr->crc = crc32(0, Z_NULL, 0);
}

/* Calculate the CRC over a section of data.  We can only pass as
 * much data to this routine as the largest single buffer size.  We
 * also check that this data will actually be used before going to the
 * trouble of calculating it.
 */
void
png_calculate_crc(png_structp png_ptr, png_bytep ptr, png_size_t length)
{
   int need_crc = 1;

   if (png_ptr->chunk_name[0] & 0x20)                     /* ancillary */
   {
      if ((png_ptr->flags & PNG_FLAG_CRC_ANCILLARY_MASK) ==
          (PNG_FLAG_CRC_ANCILLARY_USE | PNG_FLAG_CRC_ANCILLARY_NOWARN))
         need_crc = 0;
   }
   else                                                    /* critical */
   {
      if (png_ptr->flags & PNG_FLAG_CRC_CRITICAL_IGNORE)
         need_crc = 0;
   }

   if (need_crc)
      png_ptr->crc = crc32(png_ptr->crc, ptr, (uInt)length);
}

/* Allocate the memory for an info_struct for the application.  We don't
 * really need the png_ptr, but it could potentially be useful in the
 * future.  This should be used in favour of malloc(sizeof(png_info))
 * and png_info_init() so that applications that want to use a shared
 * libpng don't have to be recompiled if png_info changes size.
 */
png_infop
png_create_info_struct(png_structp png_ptr)
{
   png_infop info_ptr;

   png_debug(1, "in png_create_info_struct\n");
   if(png_ptr == NULL) return (NULL);
#ifdef PNG_USER_MEM_SUPPORTED
   if ((info_ptr = (png_infop)png_create_struct_2(PNG_STRUCT_INFO,
      png_ptr->malloc_fn)) != NULL)
#else
   if ((info_ptr = (png_infop)png_create_struct(PNG_STRUCT_INFO)) != NULL)
#endif
   {
      png_info_init(info_ptr);
   }

   return (info_ptr);
}

/* Initialize the info structure.  This is now an internal function (0.89)
 * and applications using it are urged to use png_create_info_struct()
 * instead.
 */
void
png_info_init(png_infop info_ptr)
{
   png_debug(1, "in png_info_init\n");
   /* set everything to 0 */
   png_memset(info_ptr, 0, sizeof (png_info));
}

/* This is an internal routine to free any memory that the info struct is
 * pointing to before re-using it or freeing the struct itself.  Recall
 * that png_free() checks for NULL pointers for us.
 */
void
png_info_destroy(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_READ_zTXt_SUPPORTED)
   png_debug(1, "in png_info_destroy\n");
   if (info_ptr->text != NULL)
   {
      int i;
      for (i = 0; i < info_ptr->num_text; i++)
      {
         png_free(png_ptr, info_ptr->text[i].key);
      }
      png_free(png_ptr, info_ptr->text);
   }
#endif
#if defined(PNG_READ_pCAL_SUPPORTED)
   png_free(png_ptr, info_ptr->pcal_purpose);
   png_free(png_ptr, info_ptr->pcal_units);
   if (info_ptr->pcal_params != NULL)
   {
      int i;
      for (i = 0; i < (int)info_ptr->pcal_nparams; i++)
      {
         png_free(png_ptr, info_ptr->pcal_params[i]);
      }
      png_free(png_ptr, info_ptr->pcal_params);
   }
#endif

   png_info_init(info_ptr);
}

/* Initialize the default input/output functions for the PNG file.  If you
 * use your own read or write routines, you can call either png_set_read_fn()
 * or png_set_write_fn() instead of png_init_io().
 */
void
png_init_io(png_structp png_ptr, FILE *fp)
{
   png_debug(1, "in png_init_io\n");
   png_ptr->io_ptr = (png_voidp)fp;
}

#if defined(PNG_TIME_RFC1123_SUPPORTED)
/* Convert the supplied time into an RFC 1123 string suitable for use in
 * a "Creation Time" or other text-based time string.
 */
png_charp
png_convert_to_rfc1123(png_structp png_ptr, png_timep ptime)
{
   static PNG_CONST char short_months[12][4] =
	{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

   if (png_ptr->time_buffer == NULL)
   {
      png_ptr->time_buffer = (png_charp)png_malloc(png_ptr, (png_uint_32)(29*
         sizeof(char)));
   }

#ifdef USE_FAR_KEYWORD
   {
      char near_time_buf[29];
      sprintf(near_time_buf, "%d %s %d %02d:%02d:%02d +0000",
               ptime->day % 32, short_months[(ptime->month - 1) % 12],
               ptime->year, ptime->hour % 24, ptime->minute % 60,
               ptime->second % 61);
      png_memcpy(png_ptr->time_buffer, near_time_buf,
      29*sizeof(char));
   }
#else
   sprintf(png_ptr->time_buffer, "%d %s %d %02d:%02d:%02d +0000",
               ptime->day % 32, short_months[(ptime->month - 1) % 12],
               ptime->year, ptime->hour % 24, ptime->minute % 60,
               ptime->second % 61);
#endif
   return ((png_charp)png_ptr->time_buffer);
}
#endif /* PNG_TIME_RFC1123_SUPPORTED */
