
/* png.h - header file for PNG reference library
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see the COPYRIGHT NOTICE below.
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998 Glenn Randers-Pehrson
 *
 * Note about libpng version numbers:
 *
 *    Due to various miscommunications, unforeseen code incompatibilities
 *    and occasional factors outside the authors' control, version numbering
 *    on the library has not always been consistent and straightforward.
 *    The following table summarizes matters since version 0.89c, which was
 *    the first widely used release:
 *
 *    source                    png.h    png.h   shared-lib
 *    version                   string     int   version
 *    -------                   ------   -----  ----------
 *    0.89c ("1.0 beta 3")      0.89        89  1.0.89
 *    0.90  ("1.0 beta 4")      0.90        90  0.90  [should have been 2.0.90]
 *    0.95  ("1.0 beta 5")      0.95        95  0.95  [should have been 2.0.95]
 *    0.96  ("1.0 beta 6")      0.96        96  0.96  [should have been 2.0.96]
 *    0.97b ("1.00.97 beta 7")  1.00.97     97  1.0.1 [should have been 2.0.97]
 *    0.97c                     0.97        97  2.0.97
 *    0.98                      0.98        98  2.0.98
 *    0.99                      0.99        98  2.0.99
 *    0.99a-m                   0.99        99  2.0.99
 *    1.00                      1.00       100  2.1.0 [int should be 10000]
 *    1.0.0                     1.0.0      100  2.1.0 [int should be 10000]
 *    1.0.1                     1.0.1    10001  2.1.0
 *    1.0.1a-e                  1.0.1a-e 10002  2.1.0.1a-e
 *
 *    Henceforth the source version will match the shared-library minor
 *    and patch numbers; the shared-library major version number will be
 *    used for changes in backward compatibility, as it is intended.  The
 *    PNG_PNGLIB_VER macro, which is not used within libpng but is available
 *    for applications, is an unsigned integer of the form xyyzz corresponding
 *    to the source version x.y.z (leading zeros in y and z).
 *
 * See libpng.txt or libpng.3 for more information.  The PNG specification
 * is available as RFC 2083 <ftp://ftp.uu.net/graphics/png/documents/>
 * and as a W3C Recommendation <http://www.w3.org/TR/REC.png.html>
 *
 * Contributing Authors:
 *    John Bowler
 *    Kevin Bracey
 *    Sam Bushell
 *    Andreas Dilger
 *    Magnus Holmgren
 *    Tom Lane
 *    Dave Martindale
 *    Glenn Randers-Pehrson
 *    Greg Roelofs
 *    Guy Eric Schalnat
 *    Paul Schmidt
 *    Tom Tanner
 *    Willem van Schaik
 *    Tim Wegner
 *
 * The contributing authors would like to thank all those who helped
 * with testing, bug fixes, and patience.  This wouldn't have been
 * possible without all of you.
 *
 * Thanks to Frank J. T. Wojcik for helping with the documentation.
 *
 * COPYRIGHT NOTICE:
 *
 * The PNG Reference Library is supplied "AS IS".  The Contributing Authors
 * and Group 42, Inc. disclaim all warranties, expressed or implied,
 * including, without limitation, the warranties of merchantability and of
 * fitness for any purpose.  The Contributing Authors and Group 42, Inc.
 * assume no liability for direct, indirect, incidental, special, exemplary,
 * or consequential damages, which may result from the use of the PNG
 * Reference Library, even if advised of the possibility of such damage.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject
 * to the following restrictions:
 * 1. The origin of this source code must not be misrepresented.
 * 2. Altered versions must be plainly marked as such and must not be
 *    misrepresented as being the original source.
 * 3. This Copyright notice may not be removed or altered from any source or
 *    altered source distribution.
 *
 * The Contributing Authors and Group 42, Inc. specifically permit, without
 * fee, and encourage the use of this source code as a component to
 * supporting the PNG file format in commercial products.  If you use this
 * source code in a product, acknowledgment is not required but would be
 * appreciated.
 */

#ifndef _PNG_H
#define _PNG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This is not the place to learn how to use libpng.  The file libpng.txt
 * describes how to use libpng, and the file example.c summarizes it
 * with some code on which to build.  This file is useful for looking
 * at the actual function definitions and structure components.
 */

/* include the compression library's header */
#include "zlib.h"

/* include all user configurable info */
#include "pngconf.h"

/* This file is arranged in several sections.  The first section contains
 * structure and type definitions.  The second section contains the external
 * library functions, while the third has the internal library functions,
 * which applications aren't expected to use directly.
 */

/* Version information for png.h - this should match the version in png.c */
#define PNG_LIBPNG_VER_STRING "1.0.2"

/* Careful here.  At one time, Guy wanted to use 082, but that would be octal.
 * We must not include leading zeros.
 * Versions 0.7 through 1.0.0 were in the range 0 to 100 here (only
 * version 1.0.0 was mis-numbered 100 instead of 10000).  From
 * version 1.0.1 it's    xxyyzz, where x=major, y=minor, z=bugfix */
#define PNG_LIBPNG_VER    10002  /* 1.0.2 */

/* variables declared in png.c - only it needs to define PNG_NO_EXTERN */
#if !defined(PNG_NO_EXTERN) || defined(PNG_ALWAYS_EXTERN)
/* Version information for C files, stored in png.c.  This had better match
 * the version above.
 */
extern char png_libpng_ver[12];   /* need room for 99.99.99aa */

/* Structures to facilitate easy interlacing.  See png.c for more details */
extern int FARDATA png_pass_start[7];
extern int FARDATA png_pass_inc[7];
extern int FARDATA png_pass_ystart[7];
extern int FARDATA png_pass_yinc[7];
extern int FARDATA png_pass_mask[7];
extern int FARDATA png_pass_dsp_mask[7];

#endif /* PNG_NO_EXTERN */

/* Three color definitions.  The order of the red, green, and blue, (and the
 * exact size) is not important, although the size of the fields need to
 * be png_byte or png_uint_16 (as defined below).
 */
typedef struct png_color_struct
{
   png_byte red;
   png_byte green;
   png_byte blue;
} png_color;
typedef png_color FAR * png_colorp;
typedef png_color FAR * FAR * png_colorpp;

typedef struct png_color_16_struct
{
   png_byte index;    /* used for palette files */
   png_uint_16 red;   /* for use in red green blue files */
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 gray;  /* for use in grayscale files */
} png_color_16;
typedef png_color_16 FAR * png_color_16p;
typedef png_color_16 FAR * FAR * png_color_16pp;

typedef struct png_color_8_struct
{
   png_byte red;   /* for use in red green blue files */
   png_byte green;
   png_byte blue;
   png_byte gray;  /* for use in grayscale files */
   png_byte alpha; /* for alpha channel files */
} png_color_8;
typedef png_color_8 FAR * png_color_8p;
typedef png_color_8 FAR * FAR * png_color_8pp;

/* png_text holds the text in a PNG file, and whether they are compressed
   in the PNG file or not.  The "text" field points to a regular C string. */
typedef struct png_text_struct
{
   int compression;        /* compression value, see PNG_TEXT_COMPRESSION_ */
   png_charp key;          /* keyword, 1-79 character description of "text" */
   png_charp text;         /* comment, may be an empty string (ie "") */
   png_size_t text_length; /* length of "text" field */
} png_text;
typedef png_text FAR * png_textp;
typedef png_text FAR * FAR * png_textpp;

/* Supported compression types for text in PNG files (tEXt, and zTXt).
 * The values of the PNG_TEXT_COMPRESSION_ defines should NOT be changed. */
#define PNG_TEXT_COMPRESSION_NONE_WR -3
#define PNG_TEXT_COMPRESSION_zTXt_WR -2
#define PNG_TEXT_COMPRESSION_NONE    -1
#define PNG_TEXT_COMPRESSION_zTXt     0
#define PNG_TEXT_COMPRESSION_LAST     1  /* Not a valid value */

/* png_time is a way to hold the time in an machine independent way.
 * Two conversions are provided, both from time_t and struct tm.  There
 * is no portable way to convert to either of these structures, as far
 * as I know.  If you know of a portable way, send it to me.  As a side
 * note - PNG is Year 2000 compliant!
 */
typedef struct png_time_struct
{
   png_uint_16 year; /* full year, as in, 1995 */
   png_byte month;   /* month of year, 1 - 12 */
   png_byte day;     /* day of month, 1 - 31 */
   png_byte hour;    /* hour of day, 0 - 23 */
   png_byte minute;  /* minute of hour, 0 - 59 */
   png_byte second;  /* second of minute, 0 - 60 (for leap seconds) */
} png_time;
typedef png_time FAR * png_timep;
typedef png_time FAR * FAR * png_timepp;

/* png_info is a structure that holds the information in a PNG file so
 * that the application can find out the characteristics of the image.
 * If you are reading the file, this structure will tell you what is
 * in the PNG file.  If you are writing the file, fill in the information
 * you want to put into the PNG file, then call png_write_info().
 * The names chosen should be very close to the PNG specification, so
 * consult that document for information about the meaning of each field.
 *
 * With libpng < 0.95, it was only possible to directly set and read the
 * the values in the png_info_struct, which meant that the contents and
 * order of the values had to remain fixed.  With libpng 0.95 and later,
 * however, there are now functions that abstract the contents of
 * png_info_struct from the application, so this makes it easier to use
 * libpng with dynamic libraries, and even makes it possible to use
 * libraries that don't have all of the libpng ancillary chunk-handing
 * functionality.
 *
 * In any case, the order of the parameters in png_info_struct should NOT
 * be changed for as long as possible to keep compatibility with applications
 * that use the old direct-access method with png_info_struct.
 */
typedef struct png_info_struct
{
   /* the following are necessary for every PNG file */
   png_uint_32 width;       /* width of image in pixels (from IHDR) */
   png_uint_32 height;      /* height of image in pixels (from IHDR) */
   png_uint_32 valid;       /* valid chunk data (see PNG_INFO_ below) */
   png_uint_32 rowbytes;    /* bytes needed to hold an untransformed row */
   png_colorp palette;      /* array of color values (valid & PNG_INFO_PLTE) */
   png_uint_16 num_palette; /* number of color entries in "palette" (PLTE) */
   png_uint_16 num_trans;   /* number of transparent palette color (tRNS) */
   png_byte bit_depth;      /* 1, 2, 4, 8, or 16 bits/channel (from IHDR) */
   png_byte color_type;     /* see PNG_COLOR_TYPE_ below (from IHDR) */
   png_byte compression_type; /* must be PNG_COMPRESSION_TYPE_BASE (IHDR) */
   png_byte filter_type;    /* must be PNG_FILTER_TYPE_BASE (from IHDR) */
   png_byte interlace_type; /* One of PNG_INTERLACE_NONE, PNG_INTERLACE_ADAM7 */

   /* The following is informational only on read, and not used on writes. */
   png_byte channels;       /* number of data channels per pixel (1, 3, 4)*/
   png_byte pixel_depth;    /* number of bits per pixel */
   png_byte spare_byte;     /* to align the data, and for future use */
   png_byte signature[8];   /* magic bytes read by libpng from start of file */

   /* The rest of the data is optional.  If you are reading, check the
    * valid field to see if the information in these are valid.  If you
    * are writing, set the valid field to those chunks you want written,
    * and initialize the appropriate fields below.
    */

#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_WRITE_gAMA_SUPPORTED) || \
    defined(PNG_READ_GAMMA_SUPPORTED)
   /* The gAMA chunk describes the gamma characteristics of the system
    * on which the image was created, normally in the range [1.0, 2.5].
    * Data is valid if (valid & PNG_INFO_gAMA) is non-zero.
    */
   float gamma; /* gamma value of image, if (valid & PNG_INFO_gAMA) */
#endif /* PNG_READ_gAMA_SUPPORTED || PNG_WRITE_gAMA_SUPPORTED */

#if defined(PNG_READ_sRGB_SUPPORTED) || defined(PNG_WRITE_sRGB_SUPPORTED)
    /* GR-P, 0.96a */
    /* Data valid if (valid & PNG_INFO_sRGB) non-zero. */
   png_byte srgb_intent; /* sRGB rendering intent [0, 1, 2, or 3] */
#endif /* PNG_READ_sRGB_SUPPORTED || PNG_WRITE_sRGB_SUPPORTED */

#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED)
   /* The tEXt and zTXt chunks contain human-readable textual data in
    * uncompressed and compressed forms, respectively.  The data in "text"
    * is an array of pointers to uncompressed, null-terminated C strings.
    * Each chunk has a keyword that describes the textual data contained
    * in that chunk.  Keywords are not required to be unique, and the text
    * string may be empty.  Any number of text chunks may be in an image.
    */
   int num_text; /* number of comments read/to write */
   int max_text; /* current size of text array */
   png_textp text; /* array of comments read/to write */
#endif /* PNG_READ_OR_WRITE_tEXt_OR_zTXt_SUPPORTED */
#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
   /* The tIME chunk holds the last time the displayed image data was
    * modified.  See the png_time struct for the contents of this struct.
    */
   png_time mod_time;
#endif /* PNG_READ_tIME_SUPPORTED || PNG_WRITE_tIME_SUPPORTED */
#if defined(PNG_READ_sBIT_SUPPORTED) || defined(PNG_WRITE_sBIT_SUPPORTED)
   /* The sBIT chunk specifies the number of significant high-order bits
    * in the pixel data.  Values are in the range [1, bit_depth], and are
    * only specified for the channels in the pixel data.  The contents of
    * the low-order bits is not specified.  Data is valid if
    * (valid & PNG_INFO_sBIT) is non-zero.
    */
   png_color_8 sig_bit; /* significant bits in color channels */
#endif /* PNG_READ_sBIT_SUPPORTED || PNG_WRITE_sBIT_SUPPORTED */
#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_WRITE_tRNS_SUPPORTED) || \
    defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   /* The tRNS chunk supplies transparency data for paletted images and
    * other image types that don't need a full alpha channel.  There are
    * "num_trans" transparency values for a paletted image, stored in the
    * same order as the palette colors, starting from index 0.  Values
    * for the data are in the range [0, 255], ranging from fully transparent
    * to fully opaque, respectively.  For non-paletted images, there is a
    * single color specified that should be treated as fully transparent.
    * Data is valid if (valid & PNG_INFO_tRNS) is non-zero.
    */
   png_bytep trans; /* transparent values for paletted image */
   png_color_16 trans_values; /* transparent color for non-palette image */
#endif /* PNG_READ_tRNS_SUPPORTED || PNG_WRITE_tRNS_SUPPORTED */
#if defined(PNG_READ_bKGD_SUPPORTED) || defined(PNG_WRITE_bKGD_SUPPORTED) || \
    defined(PNG_READ_BACKGROUND_SUPPORTED)
   /* The bKGD chunk gives the suggested image background color if the
    * display program does not have its own background color and the image
    * is needs to composited onto a background before display.  The colors
    * in "background" are normally in the same color space/depth as the
    * pixel data.  Data is valid if (valid & PNG_INFO_bKGD) is non-zero.
    */
   png_color_16 background;
#endif /* PNG_READ_bKGD_SUPPORTED || PNG_WRITE_bKGD_SUPPORTED */
#if defined(PNG_READ_oFFs_SUPPORTED) || defined(PNG_WRITE_oFFs_SUPPORTED)
   /* The oFFs chunk gives the offset in "offset_unit_type" units rightwards
    * and downwards from the top-left corner of the display, page, or other
    * application-specific co-ordinate space.  See the PNG_OFFSET_ defines
    * below for the unit types.  Valid if (valid & PNG_INFO_oFFs) non-zero.
    */
   png_uint_32 x_offset; /* x offset on page */
   png_uint_32 y_offset; /* y offset on page */
   png_byte offset_unit_type; /* offset units type */
#endif /* PNG_READ_oFFs_SUPPORTED || PNG_WRITE_oFFs_SUPPORTED */
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   /* The pHYs chunk gives the physical pixel density of the image for
    * display or printing in "phys_unit_type" units (see PNG_RESOLUTION_
    * defines below).  Data is valid if (valid & PNG_INFO_pHYs) is non-zero.
    */
   png_uint_32 x_pixels_per_unit; /* horizontal pixel density */
   png_uint_32 y_pixels_per_unit; /* vertical pixel density */
   png_byte phys_unit_type; /* resolution type (see PNG_RESOLUTION_ below) */
#endif /* PNG_READ_pHYs_SUPPORTED || PNG_WRITE_pHYs_SUPPORTED */
#if defined(PNG_READ_hIST_SUPPORTED) || defined(PNG_WRITE_hIST_SUPPORTED)
   /* The hIST chunk contains the relative frequency or importance of the
    * various palette entries, so that a viewer can intelligently select a
    * reduced-color palette, if required.  Data is an array of "num_palette"
    * values in the range [0,65535]. Data valid if (valid & PNG_INFO_hIST)
    * is non-zero.
    */
   png_uint_16p hist;
#endif /* PNG_READ_hIST_SUPPORTED || PNG_WRITE_hIST_SUPPORTED */
#if defined(PNG_READ_cHRM_SUPPORTED) || defined(PNG_WRITE_cHRM_SUPPORTED)
   /* The cHRM chunk describes the CIE color characteristics of the monitor
    * on which the PNG was created.  This data allows the viewer to do gamut
    * mapping of the input image to ensure that the viewer sees the same
    * colors in the image as the creator.  Values are in the range
    * [0.0, 0.8].  Data valid if (valid & PNG_INFO_cHRM) non-zero.
    */
   float x_white;
   float y_white;
   float x_red;
   float y_red;
   float x_green;
   float y_green;
   float x_blue;
   float y_blue;
#endif /* PNG_READ_cHRM_SUPPORTED || PNG_WRITE_cHRM_SUPPORTED */
#if defined(PNG_READ_pCAL_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED)
   /* The pCAL chunk describes a transformation between the stored pixel
    * values and original physcical data values used to create the image.
    * The integer range [0, 2^bit_depth - 1] maps to the floating-point
    * range given by [pcal_X0, pcal_X1], and are further transformed by a
    * (possibly non-linear) transformation function given by "pcal_type"
    * and "pcal_params" into "pcal_units".  Please see the PNG_EQUATION_
    * defines below, and the PNG-Group's Scientific Visualization extension
    * chunks document png-scivis-19970203 for a complete description of the
    * transformations and how they should be implemented, as well as the
    * png-extensions document for a description of the ASCII parameter
    * strings.  Data values are valid if (valid & PNG_INFO_pCAL) non-zero.
    */
   png_charp pcal_purpose;  /* pCAL chunk description string */
   png_int_32 pcal_X0;      /* minimum value */
   png_int_32 pcal_X1;      /* maximum value */
   png_charp pcal_units;    /* Latin-1 string giving physical units */
   png_charpp pcal_params;  /* ASCII strings containing parameter values */
   png_byte pcal_type;      /* equation type (see PNG_EQUATION_ below) */
   png_byte pcal_nparams;   /* number of parameters given in pcal_params */
#endif /* PNG_READ_pCAL_SUPPORTED || PNG_WRITE_pCAL_SUPPORTED */
} png_info;
typedef png_info FAR * png_infop;
typedef png_info FAR * FAR * png_infopp;

/* These describe the color_type field in png_info. */
/* color type masks */
#define PNG_COLOR_MASK_PALETTE    1
#define PNG_COLOR_MASK_COLOR      2
#define PNG_COLOR_MASK_ALPHA      4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_PALETTE  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB        (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_RGB_ALPHA  (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)

/* This is for compression type. PNG 1.0 only defines the single type. */
#define PNG_COMPRESSION_TYPE_BASE 0 /* Deflate method 8, 32K window */
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE

/* This is for filter type. PNG 1.0 only defines the single type. */
#define PNG_FILTER_TYPE_BASE      0 /* Single row per-byte filtering */
#define PNG_FILTER_TYPE_DEFAULT   PNG_FILTER_TYPE_BASE

/* These are for the interlacing type.  These values should NOT be changed. */
#define PNG_INTERLACE_NONE        0 /* Non-interlaced image */
#define PNG_INTERLACE_ADAM7       1 /* Adam7 interlacing */
#define PNG_INTERLACE_LAST        2 /* Not a valid value */

/* These are for the oFFs chunk.  These values should NOT be changed. */
#define PNG_OFFSET_PIXEL          0 /* Offset in pixels */
#define PNG_OFFSET_MICROMETER     1 /* Offset in micrometers (1/10^6 meter) */
#define PNG_OFFSET_LAST           2 /* Not a valid value */

/* These are for the pCAL chunk.  These values should NOT be changed. */
#define PNG_EQUATION_LINEAR       0 /* Linear transformation */
#define PNG_EQUATION_BASE_E       1 /* Exponential base e transform */
#define PNG_EQUATION_ARBITRARY    2 /* Arbitrary base exponential transform */
#define PNG_EQUATION_HYPERBOLIC   3 /* Hyperbolic sine transformation */
#define PNG_EQUATION_LAST         4 /* Not a valid value */

/* These are for the pHYs chunk.  These values should NOT be changed. */
#define PNG_RESOLUTION_UNKNOWN    0 /* pixels/unknown unit (aspect ratio) */
#define PNG_RESOLUTION_METER      1 /* pixels/meter */
#define PNG_RESOLUTION_LAST       2 /* Not a valid value */

/* These are for the sRGB chunk.  These values should NOT be changed. */
#define PNG_sRGB_INTENT_SATURATION 0
#define PNG_sRGB_INTENT_PERCEPTUAL 1
#define PNG_sRGB_INTENT_ABSOLUTE   2
#define PNG_sRGB_INTENT_RELATIVE   3
#define PNG_sRGB_INTENT_LAST       4 /* Not a valid value */
                        


/* These determine if an ancillary chunk's data has been successfully read
 * from the PNG header, or if the application has filled in the corresponding
 * data in the info_struct to be written into the output file.  The values
 * of the PNG_INFO_<chunk> defines should NOT be changed.
 */
#define PNG_INFO_gAMA 0x0001
#define PNG_INFO_sBIT 0x0002
#define PNG_INFO_cHRM 0x0004
#define PNG_INFO_PLTE 0x0008
#define PNG_INFO_tRNS 0x0010
#define PNG_INFO_bKGD 0x0020
#define PNG_INFO_hIST 0x0040
#define PNG_INFO_pHYs 0x0080
#define PNG_INFO_oFFs 0x0100
#define PNG_INFO_tIME 0x0200
#define PNG_INFO_pCAL 0x0400
#define PNG_INFO_sRGB 0x0800   /* GR-P, 0.96a */

/* This is used for the transformation routines, as some of them
 * change these values for the row.  It also should enable using
 * the routines for other purposes.
 */
typedef struct png_row_info_struct
{
   png_uint_32 width; /* width of row */
   png_uint_32 rowbytes; /* number of bytes in row */
   png_byte color_type; /* color type of row */
   png_byte bit_depth; /* bit depth of row */
   png_byte channels; /* number of channels (1, 2, 3, or 4) */
   png_byte pixel_depth; /* bits per pixel (depth * channels) */
} png_row_info;

typedef png_row_info FAR * png_row_infop;
typedef png_row_info FAR * FAR * png_row_infopp;

/* These are the function types for the I/O functions and for the functions
 * that allow the user to override the default I/O functions with his or her
 * own.  The png_error_ptr type should match that of user-supplied warning
 * and error functions, while the png_rw_ptr type should match that of the
 * user read/write data functions.
 */
typedef struct png_struct_def png_struct;
typedef png_struct FAR * png_structp;

typedef void (*png_error_ptr) PNGARG((png_structp, png_const_charp));
typedef void (*png_rw_ptr) PNGARG((png_structp, png_bytep, png_size_t));
typedef void (*png_flush_ptr) PNGARG((png_structp));
typedef void (*png_read_status_ptr) PNGARG((png_structp, png_uint_32, int));
typedef void (*png_write_status_ptr) PNGARG((png_structp, png_uint_32, int));
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
typedef void (*png_progressive_info_ptr) PNGARG((png_structp, png_infop));
typedef void (*png_progressive_end_ptr) PNGARG((png_structp, png_infop));
typedef void (*png_progressive_row_ptr) PNGARG((png_structp, png_bytep,
   png_uint_32, int));
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
typedef void (*png_user_transform_ptr) PNGARG((png_structp,
    png_row_infop, png_bytep));
#endif /* PNG_READ|WRITE_USER_TRANSFORM_SUPPORTED */

typedef png_voidp (*png_malloc_ptr) PNGARG((png_structp, png_size_t));
typedef void (*png_free_ptr) PNGARG((png_structp, png_structp));

/* The structure that holds the information to read and write PNG files.
 * The only people who need to care about what is inside of this are the
 * people who will be modifying the library for their own special needs.
 * It should NOT be accessed directly by an application, except to store
 * the jmp_buf.
 */

struct png_struct_def
{
   jmp_buf jmpbuf;            /* used in png_error */

   png_error_ptr error_fn;    /* function for printing errors and aborting */
   png_error_ptr warning_fn;  /* function for printing warnings */
   png_voidp error_ptr;       /* user supplied struct for error functions */
   png_rw_ptr write_data_fn;  /* function for writing output data */
   png_rw_ptr read_data_fn;   /* function for reading input data */
#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
   png_user_transform_ptr read_user_transform_fn; /* user read transform */
   png_user_transform_ptr write_user_transform_fn; /* user write transform */
#endif
   png_voidp io_ptr;          /* ptr to application struct for I/O functions*/

   png_uint_32 mode;          /* tells us where we are in the PNG file */
   png_uint_32 flags;         /* flags indicating various things to libpng */
   png_uint_32 transformations; /* which transformations to perform */

   z_stream zstream;          /* pointer to decompression structure (below) */
   png_bytep zbuf;            /* buffer for zlib */
   png_size_t zbuf_size;      /* size of zbuf */
   int zlib_level;            /* holds zlib compression level */
   int zlib_method;           /* holds zlib compression method */
   int zlib_window_bits;      /* holds zlib compression window bits */
   int zlib_mem_level;        /* holds zlib compression memory level */
   int zlib_strategy;         /* holds zlib compression strategy */

   png_uint_32 width;         /* width of image in pixels */
   png_uint_32 height;        /* height of image in pixels */
   png_uint_32 num_rows;      /* number of rows in current pass */
   png_uint_32 usr_width;     /* width of row at start of write */
   png_uint_32 rowbytes;      /* size of row in bytes */
   png_uint_32 irowbytes;     /* size of current interlaced row in bytes */
   png_uint_32 iwidth;        /* width of current interlaced row in pixels */
   png_uint_32 row_number;    /* current row in interlace pass */
   png_bytep prev_row;        /* buffer to save previous (unfiltered) row */
   png_bytep row_buf;         /* buffer to save current (unfiltered) row */
   png_bytep sub_row;         /* buffer to save "sub" row when filtering */
   png_bytep up_row;          /* buffer to save "up" row when filtering */
   png_bytep avg_row;         /* buffer to save "avg" row when filtering */
   png_bytep paeth_row;       /* buffer to save "Paeth" row when filtering */
   png_row_info row_info;     /* used for transformation routines */

   png_uint_32 idat_size;     /* current IDAT size for read */
   png_uint_32 crc;           /* current chunk CRC value */
   png_colorp palette;        /* palette from the input file */
   png_uint_16 num_palette;   /* number of color entries in palette */
   png_uint_16 num_trans;     /* number of transparency values */
   png_byte chunk_name[5];    /* null-terminated name of current chunk */
   png_byte compression;      /* file compression type (always 0) */
   png_byte filter;           /* file filter type (always 0) */
   png_byte interlaced;       /* PNG_INTERLACE_NONE, PNG_INTERLACE_ADAM7 */
   png_byte pass;             /* current interlace pass (0 - 6) */
   png_byte do_filter;        /* row filter flags (see PNG_FILTER_ below ) */
   png_byte color_type;       /* color type of file */
   png_byte bit_depth;        /* bit depth of file */
   png_byte usr_bit_depth;    /* bit depth of users row */
   png_byte pixel_depth;      /* number of bits per pixel */
   png_byte channels;         /* number of channels in file */
   png_byte usr_channels;     /* channels at start of write */
   png_byte sig_bytes;        /* magic bytes read/written from start of file */

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
   png_uint_16 filler;           /* filler bytes for pixel expansion */
#endif /* PNG_READ_FILLER_SUPPORTED */
#if defined(PNG_READ_bKGD_SUPPORTED)
   png_byte background_gamma_type;
   float background_gamma;
   png_color_16 background;   /* background color in screen gamma space */
#if defined(PNG_READ_GAMMA_SUPPORTED)
   png_color_16 background_1; /* background normalized to gamma 1.0 */
#endif /* PNG_READ_GAMMA && PNG_READ_bKGD_SUPPORTED */
#endif /* PNG_READ_bKGD_SUPPORTED */
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_flush_ptr output_flush_fn;/* Function for flushing output */
   png_uint_32 flush_dist;    /* how many rows apart to flush, 0 - no flush */
   png_uint_32 flush_rows;    /* number of rows written since last flush */
#endif /* PNG_WRITE_FLUSH_SUPPORTED */
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   int gamma_shift;      /* number of "insignificant" bits 16-bit gamma */
   float gamma;          /* file gamma value */
   float screen_gamma;   /* screen gamma value (display_gamma/viewing_gamma */
#endif /* PNG_READ_GAMMA_SUPPORTED */
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep gamma_table;     /* gamma table for 8 bit depth files */
   png_bytep gamma_from_1;    /* converts from 1.0 to screen */
   png_bytep gamma_to_1;      /* converts from file to 1.0 */
   png_uint_16pp gamma_16_table; /* gamma table for 16 bit depth files */
   png_uint_16pp gamma_16_from_1; /* converts from 1.0 to screen */
   png_uint_16pp gamma_16_to_1; /* converts from file to 1.0 */
#endif /* PNG_READ_GAMMA_SUPPORTED || PNG_WRITE_GAMMA_SUPPORTED */
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined (PNG_READ_sBIT_SUPPORTED)
   png_color_8 sig_bit;       /* significant bits in each available channel */
#endif /* PNG_READ_GAMMA_SUPPORTED || PNG_READ_sBIT_SUPPORTED */
#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
   png_color_8 shift;         /* shift for significant bit tranformation */
#endif /* PNG_READ_SHIFT_SUPPORTED || PNG_WRITE_SHIFT_SUPPORTED */
#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED) \
 || defined(PNG_READ_EXPAND_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep trans;           /* transparency values for paletted files */
   png_color_16 trans_values; /* transparency values for non-paletted files */
#endif /* PNG_READ|WRITE_tRNS_SUPPORTED||PNG_READ_EXPAND|BACKGROUND_SUPPORTED */
   png_read_status_ptr read_row_fn;   /* called after each row is decoded */
   png_write_status_ptr write_row_fn; /* called after each row is encoded */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_progressive_info_ptr info_fn; /* called after header data fully read */
   png_progressive_row_ptr row_fn;   /* called after each prog. row is decoded */
   png_progressive_end_ptr end_fn;   /* called after image is complete */
   png_bytep save_buffer_ptr;        /* current location in save_buffer */
   png_bytep save_buffer;            /* buffer for previously read data */
   png_bytep current_buffer_ptr;     /* current location in current_buffer */
   png_bytep current_buffer;         /* buffer for recently used data */
   png_uint_32 push_length;          /* size of current input chunk */
   png_uint_32 skip_length;          /* bytes to skip in input data */
   png_size_t save_buffer_size;      /* amount of data now in save_buffer */
   png_size_t save_buffer_max;       /* total size of save_buffer */
   png_size_t buffer_size;           /* total amount of available input data */
   png_size_t current_buffer_size;   /* amount of data now in current_buffer */
   int process_mode;                 /* what push library is currently doing */
   int cur_palette;                  /* current push library palette index */
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_READ_zTXt_SUPPORTED)
   png_size_t current_text_size;     /* current size of text input data */
   png_size_t current_text_left;     /* how much text left to read in input */
   png_charp current_text;           /* current text chunk buffer */
   png_charp current_text_ptr;       /* current location in current_text */
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED && PNG_READ_tEXt/zTXt_SUPPORTED */
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */
#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)
/* for the Borland special 64K segment handler */
   png_bytepp offset_table_ptr;
   png_bytep offset_table;
   png_uint_16 offset_table_number;
   png_uint_16 offset_table_count;
   png_uint_16 offset_table_count_free;
#endif /* __TURBOC__&&!_Windows&&!__FLAT__ */
#if defined(PNG_READ_DITHER_SUPPORTED)
   png_bytep palette_lookup;         /* lookup table for dithering */
   png_bytep dither_index;           /* index translation for palette files */
#endif /* PNG_READ_DITHER_SUPPORTED */
#if defined(PNG_READ_DITHER_SUPPORTED) || defined(PNG_READ_hIST_SUPPORTED)
   png_uint_16p hist;                /* histogram */
#endif
#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED)
   png_byte heuristic_method;        /* heuristic for row filter selection */
   png_byte num_prev_filters;        /* number of weights for previous rows */
   png_bytep prev_filters;           /* filter type(s) of previous row(s) */
   png_uint_16p filter_weights;      /* weight(s) for previous line(s) */
   png_uint_16p inv_filter_weights;  /* 1/weight(s) for previous line(s) */
   png_uint_16p filter_costs;        /* relative filter calculation cost */
   png_uint_16p inv_filter_costs;    /* 1/relative filter calculation cost */
#endif /* PNG_WRITE_WEIGHTED_FILTER_SUPPORTED */
#if defined(PNG_TIME_RFC1123_SUPPORTED)
   png_charp time_buffer;            /* String to hold RFC 1123 time text */
#endif /* PNG_TIME_RFC1123_SUPPORTED */
#ifdef PNG_USER_MEM_SUPPORTED
   png_voidp mem_ptr;                /* user supplied struct for mem functions */
   png_malloc_ptr malloc_fn;         /* function for allocating memory */
   png_free_ptr free_fn;             /* function for freeing memory */
#endif /* PNG_USER_MEM_SUPPORTED */
};

typedef png_struct FAR * FAR * png_structpp;

/* Here are the function definitions most commonly used.  This is not
 * the place to find out how to use libpng.  See libpng.txt for the
 * full explanation, see example.c for the summary.  This just provides
 * a simple one line of the use of each function.
 */

/* Tell lib we have already handled the first <num_bytes> magic bytes.
 * Handling more than 8 bytes from the beginning of the file is an error.
 */
extern PNG_EXPORT(void,png_set_sig_bytes) PNGARG((png_structp png_ptr,
   int num_bytes));

/* Check sig[start] through sig[start + num_to_check - 1] to see if it's a
 * PNG file.  Returns zero if the supplied bytes match the 8-byte PNG
 * signature, and non-zero otherwise.  Having num_to_check == 0 or
 * start > 7 will always fail (ie return non-zero).
 */
extern PNG_EXPORT(int,png_sig_cmp) PNGARG((png_bytep sig, png_size_t start,
   png_size_t num_to_check));

/* Simple signature checking function.  This is the same as calling
 * png_check_sig(sig, n) := !png_sig_cmp(sig, 0, n).
 */
extern PNG_EXPORT(int,png_check_sig) PNGARG((png_bytep sig, int num));

/* Allocate and initialize png_ptr struct for reading, and any other memory. */
extern PNG_EXPORT(png_structp,png_create_read_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

/* Allocate and initialize png_ptr struct for writing, and any other memory */
extern PNG_EXPORT(png_structp,png_create_write_struct)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn));

#ifdef PNG_USER_MEM_SUPPORTED
extern PNG_EXPORT(png_structp,png_create_read_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr,
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));
extern PNG_EXPORT(png_structp,png_create_write_struct_2)
   PNGARG((png_const_charp user_png_ver, png_voidp error_ptr, 
   png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
   png_malloc_ptr malloc_fn, png_free_ptr free_fn));
#endif

/* Write a PNG chunk - size, type, (optional) data, CRC. */
extern PNG_EXPORT(void,png_write_chunk) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_bytep data, png_size_t length));

/* Write the start of a PNG chunk - length and chunk name. */
extern PNG_EXPORT(void,png_write_chunk_start) PNGARG((png_structp png_ptr,
   png_bytep chunk_name, png_uint_32 length));

/* Write the data of a PNG chunk started with png_write_chunk_start(). */
extern PNG_EXPORT(void,png_write_chunk_data) PNGARG((png_structp png_ptr,
   png_bytep data, png_size_t length));

/* Finish a chunk started with png_write_chunk_start() (includes CRC). */
extern PNG_EXPORT(void,png_write_chunk_end) PNGARG((png_structp png_ptr));

/* Allocate and initialize the info structure */
extern PNG_EXPORT(png_infop,png_create_info_struct)
   PNGARG((png_structp png_ptr));

/* Initialize the info structure (old interface - NOT DLL EXPORTED) */
extern void png_info_init PNGARG((png_infop info_ptr));

/* Writes all the PNG information before the image. */
extern PNG_EXPORT(void,png_write_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* read the information before the actual image data. */
extern PNG_EXPORT(void,png_read_info) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

#if defined(PNG_TIME_RFC1123_SUPPORTED)
extern PNG_EXPORT(png_charp,png_convert_to_rfc1123)
   PNGARG((png_structp png_ptr, png_timep ptime));
#endif /* PNG_TIME_RFC1123_SUPPORTED */

#if defined(PNG_WRITE_tIME_SUPPORTED)
/* convert from a struct tm to png_time */
extern PNG_EXPORT(void,png_convert_from_struct_tm) PNGARG((png_timep ptime,
   struct tm FAR * ttime));

/* convert from time_t to png_time.  Uses gmtime() */
extern PNG_EXPORT(void,png_convert_from_time_t) PNGARG((png_timep ptime,
   time_t ttime));
#endif /* PNG_WRITE_tIME_SUPPORTED */

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* Expand data to 24 bit RGB, or 8 bit grayscale, with alpha if available. */
extern PNG_EXPORT(void,png_set_expand) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_EXPAND_SUPPORTED */

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
/* Use blue, green, red order for pixels. */
extern PNG_EXPORT(void,png_set_bgr) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_BGR_SUPPORTED || PNG_WRITE_BGR_SUPPORTED */

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
/* Expand the grayscale to 24 bit RGB if necessary. */
extern PNG_EXPORT(void,png_set_gray_to_rgb) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_GRAY_TO_RGB_SUPPORTED */

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
/* Reduce RGB to grayscale. (Not yet implemented) */
extern PNG_EXPORT(void,png_set_rgb_to_gray) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_RGB_TO_GRAY_SUPPORTED */

#if defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_strip_alpha) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_STRIP_ALPHA_SUPPORTED */

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_swap_alpha) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_SWAP_ALPHA_SUPPORTED || PNG_WRITE_SWAP_ALPHA_SUPPORTED */

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED) || \
    defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
extern PNG_EXPORT(void,png_set_invert_alpha) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_INVERT_ALPHA_SUPPORTED || PNG_WRITE_INVERT_ALPHA_SUPPORTED */

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
/* Add a filler byte to 24-bit RGB images. */
extern PNG_EXPORT(void,png_set_filler) PNGARG((png_structp png_ptr,
   png_uint_32 filler, int flags));

/* The values of the PNG_FILLER_ defines should NOT be changed */
#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1
#endif /* PNG_READ_FILLER_SUPPORTED || PNG_WRITE_FILLER_SUPPORTED */

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
/* Swap bytes in 16 bit depth files. */
extern PNG_EXPORT(void,png_set_swap) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_SWAP_SUPPORTED || PNG_WRITE_SWAP_SUPPORTED */

#if defined(PNG_READ_PACK_SUPPORTED) || defined(PNG_WRITE_PACK_SUPPORTED)
/* Use 1 byte per pixel in 1, 2, or 4 bit depth files. */
extern PNG_EXPORT(void,png_set_packing) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_PACK_SUPPORTED || PNG_WRITE_PACK_SUPPORTED */

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || defined(PNG_WRITE_PACKSWAP_SUPPORTED)
/* Swap packing order of pixels in bytes. */
extern PNG_EXPORT(void,png_set_packswap) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_PACKSWAP_SUPPORTED || PNG_WRITE_PACKSWAP_SUPPOR */

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
/* Converts files to legal bit depths. */
extern PNG_EXPORT(void,png_set_shift) PNGARG((png_structp png_ptr,
   png_color_8p true_bits));
#endif /* PNG_READ_SHIFT_SUPPORTED || PNG_WRITE_SHIFT_SUPPORTED */

#if defined(PNG_READ_INTERLACING_SUPPORTED) || \
    defined(PNG_WRITE_INTERLACING_SUPPORTED)
/* Have the code handle the interlacing.  Returns the number of passes. */
extern PNG_EXPORT(int,png_set_interlace_handling) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_INTERLACING_SUPPORTED || PNG_WRITE_INTERLACING_SUPPORTED */

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
/* Invert monocrome files */
extern PNG_EXPORT(void,png_set_invert_mono) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_INVERT_SUPPORTED || PNG_WRITE_INVERT_SUPPORTED */

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
/* Handle alpha and tRNS by replacing with a background color. */
extern PNG_EXPORT(void,png_set_background) PNGARG((png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma));
#define PNG_BACKGROUND_GAMMA_UNKNOWN 0
#define PNG_BACKGROUND_GAMMA_SCREEN  1
#define PNG_BACKGROUND_GAMMA_FILE    2
#define PNG_BACKGROUND_GAMMA_UNIQUE  3
#endif /* PNG_READ_BACKGROUND_SUPPORTED */

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* strip the second byte of information from a 16 bit depth file. */
extern PNG_EXPORT(void,png_set_strip_16) PNGARG((png_structp png_ptr));
#endif /* PNG_READ_16_TO_8_SUPPORTED */

#if defined(PNG_READ_DITHER_SUPPORTED)
/* Turn on dithering, and reduce the palette to the number of colors available. */
extern PNG_EXPORT(void,png_set_dither) PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette, int maximum_colors,
   png_uint_16p histogram, int full_dither));
#endif /* PNG_READ_DITHER_SUPPORTED */

#if defined(PNG_READ_GAMMA_SUPPORTED)
/* Handle gamma correction. Screen_gamma=(display_gamma/viewing_gamma) */
extern PNG_EXPORT(void,png_set_gamma) PNGARG((png_structp png_ptr,
   double screen_gamma, double default_file_gamma));
#endif /* PNG_READ_GAMMA_SUPPORTED */

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
/* Set how many lines between output flushes - 0 for no flushing */
extern PNG_EXPORT(void,png_set_flush) PNGARG((png_structp png_ptr, int nrows));

/* Flush the current PNG output buffer */
extern PNG_EXPORT(void,png_write_flush) PNGARG((png_structp png_ptr));
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

/* optional call to update the users info structure */
extern PNG_EXPORT(void,png_read_update_info) PNGARG((png_structp png_ptr, png_infop info_ptr));

/* read a row of data.*/
extern PNG_EXPORT(void,png_read_row) PNGARG((png_structp png_ptr,
   png_bytep row,
   png_bytep display_row));

/* read the whole image into memory at once. */
extern PNG_EXPORT(void,png_read_image) PNGARG((png_structp png_ptr,
   png_bytepp image));

/* read the end of the PNG file. */
extern PNG_EXPORT(void,png_read_end) PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* free any memory associated with the png_struct and the png_info_structs */
extern PNG_EXPORT(void,png_destroy_read_struct) PNGARG((png_structpp
   png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr));

/* free all memory used by the read (old method - NOT DLL EXPORTED) */
extern void png_read_destroy PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_infop end_info_ptr));

/* Values for png_set_crc_action() to say how to handle CRC errors in
 * ancillary and critical chunks, and whether to use the data contained
 * therein.  Note that it is impossible to "discard" data in a critical
 * chunk.  For versions prior to 0.90, the action was always error/quit,
 * whereas in version 0.90 and later, the action for CRC errors in ancillary
 * chunks is warn/discard.  These values should NOT be changed.
 *
 *      value                       action:critical     action:ancillary
 */
#define PNG_CRC_DEFAULT       0  /* error/quit          warn/discard data */
#define PNG_CRC_ERROR_QUIT    1  /* error/quit          error/quit        */
#define PNG_CRC_WARN_DISCARD  2  /* (INVALID)           warn/discard data */
#define PNG_CRC_WARN_USE      3  /* warn/use data       warn/use data     */
#define PNG_CRC_QUIET_USE     4  /* quiet/use data      quiet/use data    */
#define PNG_CRC_NO_CHANGE     5  /* use current value   use current value */

/* These functions give the user control over the scan-line filtering in
 * libpng and the compression methods used by zlib.  These functions are
 * mainly useful for testing, as the defaults should work with most users.
 * Those users who are tight on memory or want faster performance at the
 * expense of compression can modify them.  See the compression library
 * header file (zlib.h) for an explination of the compression functions.
 */

/* set the filtering method(s) used by libpng.  Currently, the only valid
 * value for "method" is 0.
 */
extern PNG_EXPORT(void,png_set_filter) PNGARG((png_structp png_ptr, int method,
   int filters));

/* Flags for png_set_filter() to say which filters to use.  The flags
 * are chosen so that they don't conflict with real filter types
 * below, in case they are supplied instead of the #defined constants.
 * These values should NOT be changed.
 */
#define PNG_NO_FILTERS     0x00
#define PNG_FILTER_NONE    0x08
#define PNG_FILTER_SUB     0x10
#define PNG_FILTER_UP      0x20
#define PNG_FILTER_AVG     0x40
#define PNG_FILTER_PAETH   0x80
#define PNG_ALL_FILTERS (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | \
                         PNG_FILTER_AVG | PNG_FILTER_PAETH)

/* Filter values (not flags) - used in pngwrite.c, pngwutil.c for now.
 * These defines should NOT be changed.
 */
#define PNG_FILTER_VALUE_NONE  0
#define PNG_FILTER_VALUE_SUB   1
#define PNG_FILTER_VALUE_UP    2
#define PNG_FILTER_VALUE_AVG   3
#define PNG_FILTER_VALUE_PAETH 4
#define PNG_FILTER_VALUE_LAST  5

#if defined(PNG_WRITE_WEIGHTED_FILTER_SUPPORTED) /* EXPERIMENTAL */
/* The "heuristic_method" is given by one of the PNG_FILTER_HEURISTIC_
 * defines, either the default (minimum-sum-of-absolute-differences), or
 * the experimental method (weighted-minimum-sum-of-absolute-differences).
 *
 * Weights are factors >= 1.0, indicating how important it is to keep the
 * filter type consistent between rows.  Larger numbers mean the current
 * filter is that many times as likely to be the same as the "num_weights"
 * previous filters.  This is cumulative for each previous row with a weight.
 * There needs to be "num_weights" values in "filter_weights", or it can be
 * NULL if the weights aren't being specified.  Weights have no influence on
 * the selection of the first row filter.  Well chosen weights can (in theory)
 * improve the compression for a given image.
 *
 * Costs are factors >= 1.0 indicating the relative decoding costs of a
 * filter type.  Higher costs indicate more decoding expense, and are
 * therefore less likely to be selected over a filter with lower computational
 * costs.  There needs to be a value in "filter_costs" for each valid filter
 * type (given by PNG_FILTER_VALUE_LAST), or it can be NULL if you aren't
 * setting the costs.  Costs try to improve the speed of decompression without
 * unduly increasing the compressed image size.
 *
 * A negative weight or cost indicates the default value is to be used, and
 * values in the range [0.0, 1.0) indicate the value is to remain unchanged.
 * The default values for both weights and costs are currently 1.0, but may
 * change if good general weighting/cost heuristics can be found.  If both
 * the weights and costs are set to 1.0, this degenerates the WEIGHTED method
 * to the UNWEIGHTED method, but with added encoding time/computation.
 */
extern PNG_EXPORT(void,png_set_filter_heuristics) PNGARG((png_structp png_ptr,
   int heuristic_method, int num_weights, png_doublep filter_weights,
   png_doublep filter_costs));
#endif /*  PNG_WRITE_WEIGHTED_FILTER_SUPPORTED */

/* Heuristic used for row filter selection.  These defines should NOT be
 * changed.
 */
#define PNG_FILTER_HEURISTIC_DEFAULT    0  /* Currently "UNWEIGHTED" */
#define PNG_FILTER_HEURISTIC_UNWEIGHTED 1  /* Used by libpng < 0.95 */
#define PNG_FILTER_HEURISTIC_WEIGHTED   2  /* Experimental feature */
#define PNG_FILTER_HEURISTIC_LAST       3  /* Not a valid value */

/* Set the library compression level.  Currently, valid values range from
 * 0 - 9, corresponding directly to the zlib compression levels 0 - 9
 * (0 - no compression, 9 - "maximal" compression).  Note that tests have
 * shown that zlib compression levels 3-6 usually perform as well as level 9
 * for PNG images, and do considerably fewer caclulations.  In the future,
 * these values may not correspond directly to the zlib compression levels.
 */
extern PNG_EXPORT(void,png_set_compression_level) PNGARG((png_structp png_ptr,
   int level));

extern PNG_EXPORT(void,png_set_compression_mem_level)
   PNGARG((png_structp png_ptr, int mem_level));

extern PNG_EXPORT(void,png_set_compression_strategy)
   PNGARG((png_structp png_ptr, int strategy));

extern PNG_EXPORT(void,png_set_compression_window_bits)
   PNGARG((png_structp png_ptr, int window_bits));

extern PNG_EXPORT(void,png_set_compression_method) PNGARG((png_structp png_ptr,
   int method));

/* These next functions are called for input/output, memory, and error
 * handling.  They are in the file pngrio.c, pngwio.c, and pngerror.c,
 * and call standard C I/O routines such as fread(), fwrite(), and
 * fprintf().  These functions can be made to use other I/O routines
 * at run time for those applications that need to handle I/O in a
 * different manner by calling png_set_???_fn().  See libpng.txt for
 * more information.
 */

/* Initialize the input/output for the PNG file to the default functions. */
extern PNG_EXPORT(void,png_init_io) PNGARG((png_structp png_ptr, FILE *fp));

/* Replace the (error and abort), and warning functions with user
 * supplied functions.  If no messages are to be printed you must still
 * write and use replacement functions. The replacement error_fn should
 * still do a longjmp to the last setjmp location if you are using this
 * method of error handling.  If error_fn or warning_fn is NULL, the
 * default function will be used.
 */
extern PNG_EXPORT(void,png_set_error_fn) PNGARG((png_structp png_ptr,
   png_voidp error_ptr, png_error_ptr error_fn, png_error_ptr warning_fn));

/* Replace the default data output functions with a user supplied one(s).
 * If buffered output is not used, then output_flush_fn can be set to NULL.
 * If PNG_WRITE_FLUSH_SUPPORTED is not defined at libpng compile time
 * output_flush_fn will be ignored (and thus can be NULL).
 */
extern PNG_EXPORT(void,png_set_write_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));

/* Replace the default data input function with a user supplied one. */
extern PNG_EXPORT(void,png_set_read_fn) PNGARG((png_structp png_ptr,
   png_voidp io_ptr, png_rw_ptr read_data_fn));

extern PNG_EXPORT(void,png_set_write_status_fn) PNGARG((png_structp png_ptr,
   png_write_status_ptr write_row_fn));

#ifdef PNG_USER_MEM_SUPPORTED
/* Replace the default memory allocation functions with user supplied one(s). */
extern PNG_EXPORT(void,png_set_mem_fn) PNGARG((png_structp png_ptr,
   png_voidp mem_ptr, png_malloc_ptr malloc_fn, png_free_ptr free_fn));

/* Return the user pointer associated with the memory functions */
extern PNG_EXPORT(png_voidp,png_get_mem_ptr) PNGARG((png_structp png_ptr));
#endif /* PNG_USER_MEM_SUPPORTED */

#ifdef PNG_READ_USER_TRANSFORM_SUPPORTED
extern PNG_EXPORT(void,png_set_read_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr read_user_transform_fn));
#endif

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
extern PNG_EXPORT(void,png_set_write_user_transform_fn) PNGARG((png_structp
   png_ptr, png_user_transform_ptr write_user_transform_fn));
#endif

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
/* Sets the function callbacks for the push reader, and a pointer to a
 * user-defined structure available to the callback functions.
 */
extern PNG_EXPORT(void,png_set_progressive_read_fn) PNGARG((png_structp png_ptr,
   png_voidp progressive_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn));

/* returns the user pointer associated with the push read functions */
extern PNG_EXPORT(png_voidp,png_get_progressive_ptr)
   PNGARG((png_structp png_ptr));

/* function to be called when data becomes available */
extern PNG_EXPORT(void,png_process_data) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep buffer, png_size_t buffer_size));

/* function that combines rows.  Not very much different than the
 * png_combine_row() call.  Is this even used?????
 */
extern PNG_EXPORT(void,png_progressive_combine_row) PNGARG((png_structp png_ptr,
   png_bytep old_row, png_bytep new_row));
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

extern PNG_EXPORT(png_voidp,png_malloc) PNGARG((png_structp png_ptr,
   png_uint_32 size));

/* frees a pointer allocated by png_malloc() */
extern PNG_EXPORT(void,png_free) PNGARG((png_structp png_ptr, png_voidp ptr));

#ifdef PNG_USER_MEM_SUPPORTED
extern PNG_EXPORT(png_voidp,png_malloc_default) PNGARG((png_structp png_ptr,
   png_uint_32 size));
extern PNG_EXPORT(void,png_free_default) PNGARG((png_structp png_ptr,
   png_voidp ptr));
#endif /* PNG_USER_MEM_SUPPORTED */

extern PNG_EXPORT(png_voidp,png_memcpy_check) PNGARG((png_structp png_ptr,
   png_voidp s1, png_voidp s2, png_uint_32 size));

extern PNG_EXPORT(png_voidp,png_memset_check) PNGARG((png_structp png_ptr,
   png_voidp s1, int value, png_uint_32 size));

#if defined(USE_FAR_KEYWORD)  /* memory model conversion function */
extern void *png_far_to_near PNGARG((png_structp png_ptr,png_voidp ptr,
   int check));
#endif /* USE_FAR_KEYWORD */

/* Fatal error in PNG image of libpng - can't continue */
extern PNG_EXPORT(void,png_error) PNGARG((png_structp png_ptr,
   png_const_charp error));

/* The same, but the chunk name is prepended to the error string. */
extern PNG_EXPORT(void,png_chunk_error) PNGARG((png_structp png_ptr,
   png_const_charp error));

/* Non-fatal error in libpng.  Can continue, but may have a problem. */
extern PNG_EXPORT(void,png_warning) PNGARG((png_structp png_ptr,
   png_const_charp message));

/* Non-fatal error in libpng, chunk name is prepended to message. */
extern PNG_EXPORT(void,png_chunk_warning) PNGARG((png_structp png_ptr,
   png_const_charp message));

/* The png_set_<chunk> functions are for storing values in the png_info_struct.
 * Similarly, the png_get_<chunk> calls are used to read values from the
 * png_info_struct, either storing the parameters in the passed variables, or
 * setting pointers into the png_info_struct where the data is stored.  The
 * png_get_<chunk> functions return a non-zero value if the data was available
 * in info_ptr, or return zero and do not change any of the parameters if the
 * data was not available.
 *
 * These functions should be used instead of directly accessing png_info
 * to avoid problems with future changes in the size and internal layout of
 * png_info_struct.
 */

/* Returns number of bytes needed to hold a transformed row. */
extern PNG_EXPORT(png_uint_32,png_get_rowbytes) PNGARG((png_structp png_ptr,
png_infop info_ptr));

#ifdef PNG_EASY_ACCESS_SUPPORTED
/* Returns image width in pixels. */
extern PNG_EXPORT(png_uint_32, png_get_image_width) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image height in pixels. */
extern PNG_EXPORT(png_uint_32, png_get_image_height) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image bit_depth. */
extern PNG_EXPORT(png_byte, png_get_bit_depth) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image color_type. */
extern PNG_EXPORT(png_byte, png_get_color_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image filter_type. */
extern PNG_EXPORT(png_byte, png_get_filter_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image interlace_type. */
extern PNG_EXPORT(png_byte, png_get_interlace_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image compression_type. */
extern PNG_EXPORT(png_byte, png_get_compression_type) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns image resolution in pixels per meter, from pHYs chunk data. */
extern PNG_EXPORT(png_uint_32, png_get_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_x_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));
extern PNG_EXPORT(png_uint_32, png_get_y_pixels_per_meter) PNGARG((png_structp
png_ptr, png_infop info_ptr));

/* Returns pixel aspect ratio, computed from pHYs chunk data.  */
extern PNG_EXPORT(float, png_get_pixel_aspect_ratio) PNGARG((png_structp
png_ptr, png_infop info_ptr));

#endif /* PNG_EASY_ACCESS_SUPPORTED */

#if defined(PNG_READ_bKGD_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p *background));
#endif /* PNG_READ_bKGD_SUPPORTED */

#if defined(PNG_READ_bKGD_SUPPORTED) || defined(PNG_WRITE_bKGD_SUPPORTED)
extern PNG_EXPORT(void,png_set_bKGD) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_16p background));
#endif /* PNG_READ_bKGD_SUPPORTED || PNG_WRITE_bKGD_SUPPORTED */

#if defined(PNG_READ_cHRM_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *white_x, double *white_y, double *red_x,
   double *red_y, double *green_x, double *green_y, double *blue_x,
   double *blue_y));
#endif /* PNG_READ_cHRM_SUPPORTED */

#if defined(PNG_READ_cHRM_SUPPORTED) || defined(PNG_WRITE_cHRM_SUPPORTED)
extern PNG_EXPORT(void,png_set_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double white_x, double white_y, double red_x,
   double red_y, double green_x, double green_y, double blue_x, double blue_y));
#endif /* PNG_READ_cHRM_SUPPORTED || PNG_WRITE_cHRM_SUPPORTED */

#if defined(PNG_READ_gAMA_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double *file_gamma));
#endif /* PNG_READ_gAMA_SUPPORTED */

#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_WRITE_gAMA_SUPPORTED)
extern PNG_EXPORT(void,png_set_gAMA) PNGARG((png_structp png_ptr,
   png_infop info_ptr, double file_gamma));
#endif /* PNG_READ_gAMA_SUPPORTED || PNG_WRITE_gAMA_SUPPORTED */

#if defined(PNG_READ_hIST_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p *hist));
#endif /* PNG_READ_hIST_SUPPORTED */

#if defined(PNG_READ_hIST_SUPPORTED) || defined(PNG_WRITE_hIST_SUPPORTED)
extern PNG_EXPORT(void,png_set_hIST) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_16p hist));
#endif /* PNG_READ_hIST_SUPPORTED || PNG_WRITE_hIST_SUPPORTED */

extern PNG_EXPORT(png_uint_32,png_get_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *width, png_uint_32 *height,
   int *bit_depth, int *color_type, int *interlace_type,
   int *compression_type, int *filter_type));
  
extern PNG_EXPORT(void,png_set_IHDR) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type, int filter_type));

#if defined(PNG_READ_oFFs_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *offset_x, png_uint_32 *offset_y,
   int *unit_type));
#endif /* PNG_READ_oFFs_SUPPORTED */

#if defined(PNG_READ_oFFs_SUPPORTED) || defined(PNG_WRITE_oFFs_SUPPORTED)
extern PNG_EXPORT(void,png_set_oFFs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 offset_x, png_uint_32 offset_y,
   int unit_type));
#endif /* PNG_READ_oFFs_SUPPORTED || PNG_WRITE_oFFs_SUPPORTED */

#if defined(PNG_READ_pCAL_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp *purpose, png_int_32 *X0, png_int_32 *X1,
   int *type, int *nparams, png_charp *units, png_charpp *params));
#endif /* PNG_READ_pCAL_SUPPORTED */

#if defined(PNG_READ_pCAL_SUPPORTED) || defined(PNG_WRITE_pCAL_SUPPORTED)
extern PNG_EXPORT(void,png_set_pCAL) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_charp purpose, png_int_32 X0, png_int_32 X1,
   int type, int nparams, png_charp units, png_charpp params));
#endif /* PNG_READ_pCAL_SUPPORTED || PNG_WRITE_pCAL_SUPPORTED */

#if defined(PNG_READ_pHYs_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type));
#endif /* PNG_READ_pHYs_SUPPORTED */

#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
extern PNG_EXPORT(void,png_set_pHYs) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 res_x, png_uint_32 res_y, int unit_type));
#endif /* PNG_READ_pHYs_SUPPORTED || PNG_WRITE_pHYs_SUPPORTED */

extern PNG_EXPORT(png_uint_32,png_get_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp *palette, int *num_palette));

extern PNG_EXPORT(void,png_set_PLTE) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_colorp palette, int num_palette));

#if defined(PNG_READ_sBIT_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p *sig_bit));
#endif /* PNG_READ_sBIT_SUPPORTED */

#if defined(PNG_READ_sBIT_SUPPORTED) || defined(PNG_WRITE_sBIT_SUPPORTED)
extern PNG_EXPORT(void,png_set_sBIT) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_color_8p sig_bit));
#endif /* PNG_READ_sBIT_SUPPORTED || PNG_WRITE_sBIT_SUPPORTED */

#if defined(PNG_READ_sRGB_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int *intent));
#endif /* PNG_READ_sRGB_SUPPORTED */

#if defined(PNG_READ_sRGB_SUPPORTED) || defined(PNG_WRITE_sRGB_SUPPORTED)
extern PNG_EXPORT(void,png_set_sRGB) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
extern PNG_EXPORT(void,png_set_sRGB_gAMA_and_cHRM) PNGARG((png_structp png_ptr,
   png_infop info_ptr, int intent));
#endif /* PNG_READ_sRGB_SUPPORTED || PNG_WRITE_sRGB_SUPPORTED */

#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_READ_zTXt_SUPPORTED)
/* png_get_text also returns the number of text chunks in text_ptr */
extern PNG_EXPORT(png_uint_32,png_get_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp *text_ptr, int *num_text));
#endif /* PNG_READ_tEXt_SUPPORTED || PNG_READ_zTXt_SUPPORTED */

#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED)
extern PNG_EXPORT(void,png_set_text) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_textp text_ptr, int num_text));
#endif /* PNG_READ_OR_WRITE_tEXt_OR_zTXt_SUPPORTED */

#if defined(PNG_READ_tIME_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep *mod_time));
#endif /* PNG_READ_tIME_SUPPORTED */

#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
extern PNG_EXPORT(void,png_set_tIME) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_timep mod_time));
#endif /* PNG_READ_tIME_SUPPORTED || PNG_WRITE_tIME_SUPPORTED */

#if defined(PNG_READ_tRNS_SUPPORTED)
extern PNG_EXPORT(png_uint_32,png_get_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep *trans, int *num_trans,
   png_color_16p *trans_values));
#endif /* PNG_READ_tRNS_SUPPORTED */

#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_WRITE_tRNS_SUPPORTED)
extern PNG_EXPORT(void,png_set_tRNS) PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_bytep trans, int num_trans,
   png_color_16p trans_values));
#endif /* PNG_READ_tRNS_SUPPORTED || PNG_WRITE_tRNS_SUPPORTED */

/* Define PNG_DEBUG at compile time for debugging information.  Higher
 * numbers for PNG_DEBUG mean more debugging information.  This has
 * only been added since version 0.95 so it is not implemented throughout
 * libpng yet, but more support will be added as needed.
 */
#if (PNG_DEBUG > 0)
#ifdef PNG_NO_STDIO
#include <stdio.h>
#endif
#ifndef PNG_DEBUG_FILE
#define PNG_DEBUG_FILE stderr
#endif /* PNG_DEBUG_FILE */

#define png_debug(l,m)        if (PNG_DEBUG > l) \
                                 fprintf(PNG_DEBUG_FILE,"%s"m,(l==1 ? "\t" : \
                                    (l==2 ? "\t\t":(l==3 ? "\t\t\t":""))))
#define png_debug1(l,m,p1)    if (PNG_DEBUG > l) \
                                 fprintf(PNG_DEBUG_FILE,"%s"m,(l==1 ? "\t" : \
                                    (l==2 ? "\t\t":(l==3 ? "\t\t\t":""))),p1)
#define png_debug2(l,m,p1,p2) if (PNG_DEBUG > l) \
                                 fprintf(PNG_DEBUG_FILE,"%s"m,(l==1 ? "\t" : \
                                    (l==2 ? "\t\t":(l==3 ? "\t\t\t":""))),p1,p2)
#else
#define png_debug(l, m)
#define png_debug1(l, m, p1)
#define png_debug2(l, m, p1, p2)
#endif /* (PNG_DEBUG > 0) */

#ifdef PNG_READ_COMPOSITE_NODIV_SUPPORTED
/* With these routines we avoid an integer divide, which will be slower on
 * most machines.  However, it does take more operations than the corresponding
 * divide method, so it may be slower on a few RISC systems.  There are two
 * shifts (by 8 or 16 bits) and an addition, versus a single integer divide.
 *
 * Note that the rounding factors are NOT supposed to be the same!  128 and
 * 32768 are correct for the NODIV code; 127 and 32767 are correct for the
 * standard method.
 *
 * [Optimized code by Greg Roelofs and Mark Adler...blame us for bugs. :-) ]
 */

   /* fg and bg should be in `gamma 1.0' space; alpha is the opacity */
#  define png_composite(composite, fg, alpha, bg) \
     { png_uint_16 temp = ((png_uint_16)(fg) * (png_uint_16)(alpha) + \
                        (png_uint_16)(bg)*(png_uint_16)(255 - \
                        (png_uint_16)(alpha)) + (png_uint_16)128); \
       (composite) = (png_byte)((temp + (temp >> 8)) >> 8); }
#  define png_composite_16(composite, fg, alpha, bg) \
     { png_uint_32 temp = ((png_uint_32)(fg) * (png_uint_32)(alpha) + \
                        (png_uint_32)(bg)*(png_uint_32)(65535L - \
                        (png_uint_32)(alpha)) + (png_uint_32)32768L); \
       (composite) = (png_uint_16)((temp + (temp >> 16)) >> 16); }

#else  /* standard method using integer division */

   /* fg and bg should be in `gamma 1.0' space; alpha is the opacity */
#  define png_composite(composite, fg, alpha, bg) \
     (composite) = (png_byte)(((png_uint_16)(fg) * (png_uint_16)(alpha) + \
       (png_uint_16)(bg) * (png_uint_16)(255 - (png_uint_16)(alpha)) + \
       (png_uint_16)127) / 255)
#  define png_composite_16(composite, fg, alpha, bg) \
     (composite) = (png_uint_16)(((png_uint_32)(fg) * (png_uint_32)(alpha) + \
       (png_uint_32)(bg)*(png_uint_32)(65535L - (png_uint_32)(alpha)) + \
       (png_uint_32)32767) / (png_uint_32)65535L)

#endif /* PNG_READ_COMPOSITE_NODIV_SUPPORTED */

/* These next functions are used internally in the code.  They generally
 * shouldn't be used unless you are writing code to add or replace some
 * functionality in libpng.  More information about most functions can
 * be found in the files where the functions are located.
 */

#if defined(PNG_INTERNAL)

/* Various modes of operation.  Note that after an init, mode is set to
 * zero automatically when the structure is created.
 */
#define PNG_BEFORE_IHDR       0x00
#define PNG_HAVE_IHDR         0x01
#define PNG_HAVE_PLTE         0x02
#define PNG_HAVE_IDAT         0x04
#define PNG_AFTER_IDAT        0x08
#define PNG_HAVE_IEND         0x10
#define PNG_HAVE_gAMA         0x20
#define PNG_HAVE_cHRM         0x40
#define PNG_HAVE_sRGB         0x80

/* push model modes */
#define PNG_READ_SIG_MODE   0
#define PNG_READ_CHUNK_MODE 1
#define PNG_READ_IDAT_MODE  2
#define PNG_SKIP_MODE       3
#define PNG_READ_tEXt_MODE  4
#define PNG_READ_zTXt_MODE  5
#define PNG_READ_DONE_MODE  6
#define PNG_ERROR_MODE      7

/* flags for the transformations the PNG library does on the image data */
#define PNG_BGR                0x0001
#define PNG_INTERLACE          0x0002
#define PNG_PACK               0x0004
#define PNG_SHIFT              0x0008
#define PNG_SWAP_BYTES         0x0010
#define PNG_INVERT_MONO        0x0020
#define PNG_DITHER             0x0040
#define PNG_BACKGROUND         0x0080
#define PNG_BACKGROUND_EXPAND  0x0100
#define PNG_RGB_TO_GRAY        0x0200 /* Not currently implemented */
#define PNG_16_TO_8            0x0400
#define PNG_RGBA               0x0800
#define PNG_EXPAND             0x1000
#define PNG_GAMMA              0x2000
#define PNG_GRAY_TO_RGB        0x4000
#define PNG_FILLER             0x8000
#define PNG_PACKSWAP          0x10000L
#define PNG_SWAP_ALPHA        0x20000L
#define PNG_STRIP_ALPHA       0x40000L
#define PNG_INVERT_ALPHA      0x80000L
#define PNG_USER_TRANSFORM   0x100000L

/* flags for png_create_struct */
#define PNG_STRUCT_PNG   0x0001
#define PNG_STRUCT_INFO  0x0002

/* Scaling factor for filter heuristic weighting calculations */
#define PNG_WEIGHT_SHIFT 8
#define PNG_WEIGHT_FACTOR (1<<(PNG_WEIGHT_SHIFT))
#define PNG_COST_SHIFT 3
#define PNG_COST_FACTOR (1<<(PNG_COST_SHIFT))

/* flags for the png_ptr->flags rather than declaring a byte for each one */
#define PNG_FLAG_ZLIB_CUSTOM_STRATEGY     0x0001
#define PNG_FLAG_ZLIB_CUSTOM_LEVEL        0x0002
#define PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL    0x0004
#define PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS  0x0008
#define PNG_FLAG_ZLIB_CUSTOM_METHOD       0x0010
#define PNG_FLAG_ZLIB_FINISHED            0x0020
#define PNG_FLAG_ROW_INIT                 0x0040
#define PNG_FLAG_FILLER_AFTER             0x0080
#define PNG_FLAG_CRC_ANCILLARY_USE        0x0100
#define PNG_FLAG_CRC_ANCILLARY_NOWARN     0x0200
#define PNG_FLAG_CRC_CRITICAL_USE         0x0400
#define PNG_FLAG_CRC_CRITICAL_IGNORE      0x0800
#define PNG_FLAG_FREE_PALETTE             0x1000
#define PNG_FLAG_FREE_TRANS               0x2000
#define PNG_FLAG_FREE_HIST                0x4000
#define PNG_FLAG_HAVE_CHUNK_HEADER        0x8000L
#define PNG_FLAG_WROTE_tIME              0x10000L
#define PNG_FLAG_BACKGROUND_IS_GRAY      0x20000L

#define PNG_FLAG_CRC_ANCILLARY_MASK (PNG_FLAG_CRC_ANCILLARY_USE | \
                                     PNG_FLAG_CRC_ANCILLARY_NOWARN)

#define PNG_FLAG_CRC_CRITICAL_MASK  (PNG_FLAG_CRC_CRITICAL_USE | \
                                     PNG_FLAG_CRC_CRITICAL_IGNORE)

#define PNG_FLAG_CRC_MASK           (PNG_FLAG_CRC_ANCILLARY_MASK | \
                                     PNG_FLAG_CRC_CRITICAL_MASK)

/* save typing and make code easier to understand */
#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))

/* variables declared in png.c - only it needs to define PNG_NO_EXTERN */
#if !defined(PNG_NO_EXTERN) || defined(PNG_ALWAYS_EXTERN)
/* place to hold the signature string for a PNG file. */
extern png_byte FARDATA png_sig[8];

/* Constant strings for known chunk types.  If you need to add a chunk,
 * add a string holding the name here.  See png.c for more details.  We
 * can't selectively include these, since we still check for chunk in the
 * wrong locations with these labels.
 */
extern png_byte FARDATA png_IHDR[5];
extern png_byte FARDATA png_IDAT[5];
extern png_byte FARDATA png_IEND[5];
extern png_byte FARDATA png_PLTE[5];
extern png_byte FARDATA png_bKGD[5];
extern png_byte FARDATA png_cHRM[5];
extern png_byte FARDATA png_gAMA[5];
extern png_byte FARDATA png_hIST[5];
extern png_byte FARDATA png_oFFs[5];
extern png_byte FARDATA png_pCAL[5];
extern png_byte FARDATA png_pHYs[5];
extern png_byte FARDATA png_sBIT[5];
extern png_byte FARDATA png_sRGB[5];
extern png_byte FARDATA png_tEXt[5];
extern png_byte FARDATA png_tIME[5];
extern png_byte FARDATA png_tRNS[5];
extern png_byte FARDATA png_zTXt[5];

#endif /* PNG_NO_EXTERN */

/* Inline macros to do direct reads of bytes from the input buffer.  These
 * require that you are using an architecture that uses PNG byte ordering
 * (MSB first) and supports unaligned data storage.  I think that PowerPC
 * in big-endian mode and 680x0 are the only ones that will support this.
 * The x86 line of processors definitely do not.  The png_get_int_32()
 * routine also assumes we are using two's complement format for negative
 * values, which is almost certainly true.
 */
#if defined(PNG_READ_BIG_ENDIAN_SUPPORTED)
#if defined(PNG_READ_pCAL_SUPPORTED)
#define png_get_int_32(buf) ( *((png_int_32p) (buf)))
#endif /* PNG_READ_pCAL_SUPPORTED */
#define png_get_uint_32(buf) ( *((png_uint_32p) (buf)))
#define png_get_uint_16(buf) ( *((png_uint_16p) (buf)))
#else
#if defined(PNG_READ_pCAL_SUPPORTED)
PNG_EXTERN png_int_32 png_get_int_32 PNGARG((png_bytep buf));
#endif /* PNG_READ_pCAL_SUPPORTED */
PNG_EXTERN png_uint_32 png_get_uint_32 PNGARG((png_bytep buf));
PNG_EXTERN png_uint_16 png_get_uint_16 PNGARG((png_bytep buf));
#endif /* PNG_READ_BIG_ENDIAN_SUPPORTED */

/* Initialize png_ptr struct for writing, and allocate any other memory.
 * (old interface - NOT DLL EXPORTED).
 */
extern void png_write_init PNGARG((png_structp png_ptr));

/* allocate memory for an internal libpng struct */
PNG_EXTERN png_voidp png_create_struct PNGARG((int type));

/* free memory from internal libpng struct */
PNG_EXTERN void png_destroy_struct PNGARG((png_voidp struct_ptr));

PNG_EXTERN png_voidp png_create_struct_2 PNGARG((int type, png_malloc_ptr
  malloc_fn));
PNG_EXTERN void png_destroy_struct_2 PNGARG((png_voidp struct_ptr,
   png_free_ptr free_fn));

/* free any memory that info_ptr points to and reset struct. */
PNG_EXTERN void png_info_destroy PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* Function to allocate memory for zlib. */
PNG_EXTERN voidpf png_zalloc PNGARG((voidpf png_ptr, uInt items, uInt size));

/* function to free memory for zlib */
PNG_EXTERN void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));

/* reset the CRC variable */
PNG_EXTERN void png_reset_crc PNGARG((png_structp png_ptr));

/* Write the "data" buffer to whatever output you are using. */
PNG_EXTERN void png_write_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

/* Read data from whatever input you are using into the "data" buffer */
PNG_EXTERN void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

/* read bytes into buf, and update png_ptr->crc */
PNG_EXTERN void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
   png_size_t length));

/* read "skip" bytes, read the file crc, and (optionally) verify png_ptr->crc */
PNG_EXTERN int png_crc_finish PNGARG((png_structp png_ptr, png_uint_32 skip));

/* read the CRC from the file and compare it to the libpng calculated CRC */
PNG_EXTERN int png_crc_error PNGARG((png_structp png_ptr));

/* Calculate the CRC over a section of data.  Note that we are only
 * passing a maximum of 64K on systems that have this as a memory limit,
 * since this is the maximum buffer size we can specify.
 */
PNG_EXTERN void png_calculate_crc PNGARG((png_structp png_ptr, png_bytep ptr,
   png_size_t length));

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
PNG_EXTERN void png_flush PNGARG((png_structp png_ptr));
#endif

/* Place a 32-bit number into a buffer in PNG byte order (big-endian).
 * The only currently known PNG chunk that uses signed numbers is
 * the ancillary extension chunk, pCAL.
 */
PNG_EXTERN void png_save_uint_32 PNGARG((png_bytep buf, png_uint_32 i));

#if defined(PNG_WRITE_pCAL_SUPPORTED)
PNG_EXTERN void png_save_int_32 PNGARG((png_bytep buf, png_int_32 i));
#endif

/* Place a 16 bit number into a buffer in PNG byte order.
 * The parameter is declared unsigned int, not png_uint_16,
 * just to avoid potential problems on pre-ANSI C compilers.
 */
PNG_EXTERN void png_save_uint_16 PNGARG((png_bytep buf, unsigned int i));

/* simple function to write the signature */
PNG_EXTERN void png_write_sig PNGARG((png_structp png_ptr));

/* write various chunks */

/* Write the IHDR chunk, and update the png_struct with the necessary
 * information.
 */
PNG_EXTERN void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
   png_uint_32 height,
   int bit_depth, int color_type, int compression_type, int filter_type,
   int interlace_type));

PNG_EXTERN void png_write_PLTE PNGARG((png_structp png_ptr, png_colorp palette,
   png_uint_32 num_pal));

PNG_EXTERN void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
   png_size_t length));

PNG_EXTERN void png_write_IEND PNGARG((png_structp png_ptr));

#if defined(PNG_WRITE_gAMA_SUPPORTED)
PNG_EXTERN void png_write_gAMA PNGARG((png_structp png_ptr, double file_gamma));
#endif

#if defined(PNG_WRITE_sBIT_SUPPORTED)
PNG_EXTERN void png_write_sBIT PNGARG((png_structp png_ptr, png_color_8p sbit,
   int color_type));
#endif

#if defined(PNG_WRITE_cHRM_SUPPORTED)
PNG_EXTERN void png_write_cHRM PNGARG((png_structp png_ptr,
   double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y));
#endif

#if defined(PNG_WRITE_sRGB_SUPPORTED)
PNG_EXTERN void png_write_sRGB PNGARG((png_structp png_ptr,
   int intent));
#endif

#if defined(PNG_WRITE_tRNS_SUPPORTED)
PNG_EXTERN void png_write_tRNS PNGARG((png_structp png_ptr, png_bytep trans,
   png_color_16p values, int number, int color_type));
#endif

#if defined(PNG_WRITE_bKGD_SUPPORTED)
PNG_EXTERN void png_write_bKGD PNGARG((png_structp png_ptr,
   png_color_16p values, int color_type));
#endif

#if defined(PNG_WRITE_hIST_SUPPORTED)
PNG_EXTERN void png_write_hIST PNGARG((png_structp png_ptr, png_uint_16p hist,
   int num_hist));
#endif

#if defined(PNG_WRITE_tEXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED) || \
    defined(PNG_WRITE_pCAL_SUPPORTED)
PNG_EXTERN png_size_t png_check_keyword PNGARG((png_structp png_ptr,
   png_charp key, png_charpp new_key));
#endif

#if defined(PNG_WRITE_tEXt_SUPPORTED)
PNG_EXTERN void png_write_tEXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len));
#endif

#if defined(PNG_WRITE_zTXt_SUPPORTED)
PNG_EXTERN void png_write_zTXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_size_t text_len, int compression));
#endif

#if defined(PNG_WRITE_oFFs_SUPPORTED)
PNG_EXTERN void png_write_oFFs PNGARG((png_structp png_ptr,
   png_uint_32 x_offset, png_uint_32 y_offset, int unit_type));
#endif

#if defined(PNG_WRITE_pCAL_SUPPORTED)
PNG_EXTERN void png_write_pCAL PNGARG((png_structp png_ptr, png_charp purpose,
   png_int_32 X0, png_int_32 X1, int type, int nparams,
   png_charp units, png_charpp params));
#endif

#if defined(PNG_WRITE_pHYs_SUPPORTED)
PNG_EXTERN void png_write_pHYs PNGARG((png_structp png_ptr,
   png_uint_32 x_pixels_per_unit, png_uint_32 y_pixels_per_unit,
   int unit_type));
#endif

#if defined(PNG_WRITE_tIME_SUPPORTED)
PNG_EXTERN void png_write_tIME PNGARG((png_structp png_ptr,
   png_timep mod_time));
#endif

/* Called when finished processing a row of data */
PNG_EXTERN void png_write_finish_row PNGARG((png_structp png_ptr));

/* Internal use only.   Called before first row of data */
PNG_EXTERN void png_write_start_row PNGARG((png_structp png_ptr));

#if defined(PNG_READ_GAMMA_SUPPORTED)
PNG_EXTERN void png_build_gamma_table PNGARG((png_structp png_ptr));
#endif

/* combine a row of data, dealing with alpha, etc. if requested */
PNG_EXTERN void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
   int mask));

#if defined(PNG_READ_INTERLACING_SUPPORTED)
/* expand an interlaced row */
PNG_EXTERN void png_do_read_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass, png_uint_32 transformations));
#endif

#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
/* grab pixels out of a row for an interlaced pass */
PNG_EXTERN void png_do_write_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass));
#endif

/* unfilter a row */
PNG_EXTERN void png_read_filter_row PNGARG((png_structp png_ptr,
   png_row_infop row_info, png_bytep row, png_bytep prev_row, int filter));

/* Choose the best filter to use and filter the row data */
PNG_EXTERN void png_write_find_filter PNGARG((png_structp png_ptr,
   png_row_infop row_info));

/* Write out the filtered row. */
PNG_EXTERN void png_write_filtered_row PNGARG((png_structp png_ptr,
   png_bytep filtered_row));
/* finish a row while reading, dealing with interlacing passes, etc. */
PNG_EXTERN void png_read_finish_row PNGARG((png_structp png_ptr));

/* initialize the row buffers, etc. */
PNG_EXTERN void png_read_start_row PNGARG((png_structp png_ptr));
/* optional call to update the users info structure */
PNG_EXTERN void png_read_transform_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* these are the functions that do the transformations */
#if defined(PNG_READ_FILLER_SUPPORTED)
PNG_EXTERN void png_do_read_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 filler, png_uint_32 flags));
#endif

#if defined(PNG_READ_SWAP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_read_swap_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_SWAP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_write_swap_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_INVERT_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_read_invert_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_INVERT_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_write_invert_alpha PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED) || \
    defined(PNG_READ_STRIP_ALPHA_SUPPORTED)
PNG_EXTERN void png_do_strip_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 flags));
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
PNG_EXTERN void png_do_swap PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_PACKSWAP_SUPPORTED) || defined(PNG_WRITE_PACKSWAP_SUPPORTED)
PNG_EXTERN void png_do_packswap PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_RGB_TO_GRAY_SUPPORTED)
PNG_EXTERN void png_do_rgb_to_gray PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
PNG_EXTERN void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
PNG_EXTERN void png_do_unpack PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
PNG_EXTERN void png_do_unshift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p sig_bits));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
PNG_EXTERN void png_do_invert PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
PNG_EXTERN void png_do_chop PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
PNG_EXTERN void png_do_dither PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep palette_lookup, png_bytep dither_lookup));

#  if defined(PNG_CORRECT_PALETTE_SUPPORTED)
PNG_EXTERN void png_correct_palette PNGARG((png_structp png_ptr,
   png_colorp palette, int num_palette));
#  endif
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
PNG_EXTERN void png_do_bgr PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED)
PNG_EXTERN void png_do_pack PNGARG((png_row_infop row_info,
   png_bytep row, png_uint_32 bit_depth));
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED)
PNG_EXTERN void png_do_shift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p bit_depth));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
PNG_EXTERN void png_do_background PNGARG((png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background,
   png_color_16p background_1,
   png_bytep gamma_table, png_bytep gamma_from_1, png_bytep gamma_to_1,
   png_uint_16pp gamma_16, png_uint_16pp gamma_16_from_1,
   png_uint_16pp gamma_16_to_1, int gamma_shift));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
PNG_EXTERN void png_do_gamma PNGARG((png_row_infop row_info, png_bytep row,
   png_bytep gamma_table, png_uint_16pp gamma_16_table,
   int gamma_shift));
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
PNG_EXTERN void png_do_expand_palette PNGARG((png_row_infop row_info,
   png_bytep row, png_colorp palette, png_bytep trans, int num_trans));
PNG_EXTERN void png_do_expand PNGARG((png_row_infop row_info,
   png_bytep row, png_color_16p trans_value));
#endif

/* The following decodes the appropriate chunks, and does error correction,
 * then calls the appropriate callback for the chunk if it is valid.
 */

/* decode the IHDR chunk */
PNG_EXTERN void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
PNG_EXTERN void png_handle_IEND PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));

#if defined(PNG_READ_gAMA_SUPPORTED)
PNG_EXTERN void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
PNG_EXTERN void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
PNG_EXTERN void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sRGB_SUPPORTED)
PNG_EXTERN void png_handle_sRGB PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
PNG_EXTERN void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
PNG_EXTERN void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
PNG_EXTERN void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
PNG_EXTERN void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_pCAL_SUPPORTED)
PNG_EXTERN void png_handle_pCAL PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
PNG_EXTERN void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
PNG_EXTERN void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
PNG_EXTERN void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
PNG_EXTERN void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info_ptr,
   png_uint_32 length));
#endif

PNG_EXTERN void png_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));

PNG_EXTERN void png_check_chunk_name PNGARG((png_structp png_ptr,
   png_bytep chunk_name));

/* handle the transformations for reading and writing */
PNG_EXTERN void png_do_read_transformations PNGARG((png_structp png_ptr));
PNG_EXTERN void png_do_write_transformations PNGARG((png_structp png_ptr));

PNG_EXTERN void png_init_read_transformations PNGARG((png_structp png_ptr));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
PNG_EXTERN void png_push_read_chunk PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_read_sig PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_check_crc PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_crc_skip PNGARG((png_structp png_ptr,
   png_uint_32 length));
PNG_EXTERN void png_push_crc_finish PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_fill_buffer PNGARG((png_structp png_ptr,
   png_bytep buffer, png_size_t length));
PNG_EXTERN void png_push_save_buffer PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_restore_buffer PNGARG((png_structp png_ptr,
   png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_read_IDAT PNGARG((png_structp png_ptr));
PNG_EXTERN void png_process_IDAT_data PNGARG((png_structp png_ptr,
   png_bytep buffer, png_size_t buffer_length));
PNG_EXTERN void png_push_process_row PNGARG((png_structp png_ptr));
PNG_EXTERN void png_push_handle_unknown PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_have_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_end PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_push_have_row PNGARG((png_structp png_ptr, png_bytep row));
PNG_EXTERN void png_push_read_end PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_process_some_data PNGARG((png_structp png_ptr,
   png_infop info_ptr));
PNG_EXTERN void png_read_push_finish_row PNGARG((png_structp png_ptr));
#if defined(PNG_READ_tEXt_SUPPORTED)
PNG_EXTERN void png_push_handle_tEXt PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_tEXt PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
PNG_EXTERN void png_push_handle_zTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr, png_uint_32 length));
PNG_EXTERN void png_push_read_zTXt PNGARG((png_structp png_ptr,
   png_infop info_ptr));
#endif

#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

#endif /* PNG_INTERNAL */

#ifdef __cplusplus
}
#endif

/* do not put anything past this line */
#endif /* _PNG_H */
