
/* pngconf.h - machine configurable file for libpng
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 */

/* Any machine specific code is near the front of this file, so if you
 * are configuring libpng for a machine, you may want to read the section
 * starting here down to where it starts to typedef png_color, png_text,
 * and png_info.
 */

#ifndef PNGCONF_H
#define PNGCONF_H

/* This is the size of the compression buffer, and thus the size of
 * an IDAT chunk.  Make this whatever size you feel is best for your
 * machine.  One of these will be allocated per png_struct.  When this
 * is full, it writes the data to the disk, and does some other
 * calculations.  Making this an extremely small size will slow
 * the library down, but you may want to experiment to determine
 * where it becomes significant, if you are concerned with memory
 * usage.  Note that zlib allocates at least 32Kb also.  For readers,
 * this describes the size of the buffer available to read the data in.
 * Unless this gets smaller than the size of a row (compressed),
 * it should not make much difference how big this is.
 */

#ifndef PNG_ZBUF_SIZE
#define PNG_ZBUF_SIZE 8192
#endif

/* If you are running on a machine where you cannot allocate more
 * than 64K of memory at once, uncomment this.  While libpng will not
 * normally need that much memory in a chunk (unless you load up a very
 * large file), zlib needs to know how big of a chunk it can use, and
 * libpng thus makes sure to check any memory allocation to verify it
 * will fit into memory.
#define PNG_MAX_MALLOC_64K
 */
#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#define PNG_MAX_MALLOC_64K
#endif

/* This protects us against compilers that run on a windowing system
 * and thus don't have or would rather us not use the stdio types:
 * stdin, stdout, and stderr.  The only one currently used is stderr
 * in png_error() and png_warning().  #defining PNG_NO_STDIO will
 * prevent these from being compiled and used.
 * #define PNG_NO_STDIO
 */

//#define PNG_NO_STDIO
#include <stdio.h>

/* This macro protects us against machines that don't have function
 * prototypes (ie K&R style headers).  If your compiler does not handle
 * function prototypes, define this macro and use the included ansi2knr.
 * I've always been able to use _NO_PROTO as the indicator, but you may
 * need to drag the empty declaration out in front of here, or change the
 * ifdef to suit your own needs.
 */
#ifndef PNGARG

#ifdef OF /* zlib prototype munger */
#define PNGARG(arglist) OF(arglist)
#else

#ifdef _NO_PROTO
#define PNGARG(arglist) ()
#else
#define PNGARG(arglist) arglist
#endif /* _NO_PROTO */

#endif /* OF */

#endif /* PNGARG */

/* Try to determine if we are compiling on a Mac.  Note that testing for
 * just __MWERKS__ is not good enough, because the Codewarrior is now used
 * on non-Mac platforms.
 */
#ifndef MACOS
#if (defined(__MWERKS__) && defined(macintosh)) || defined(applec) || \
    defined(THINK_C) || defined(__SC__) || defined(TARGET_OS_MAC)
#define MACOS
#endif
#endif

/* enough people need this for various reasons to include it here */
#if !defined(MACOS) && !defined(RISCOS)
#include <sys/types.h>
#endif

/* This is an attempt to force a single setjmp behaviour on Linux.  If
 * the X config stuff didn't define _BSD_SOURCE we wouldn't need this.
 */
#ifdef __linux__
#ifdef _BSD_SOURCE
#define _PNG_SAVE_BSD_SOURCE
#undef _BSD_SOURCE
#endif
#ifdef _SETJMP_H
__png.h__ already includes setjmp.h
__dont__ include it again
#endif
#endif /* __linux__ */

/* include setjmp.h for error handling */
#include <setjmp.h>

#ifdef __linux__
#ifdef _PNG_SAVE_BSD_SOURCE
#define _BSD_SOURCE
#undef _PNG_SAVE_BSD_SOURCE
#endif
#endif /* __linux__ */

#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

/* Other defines for things like memory and the like can go here.  */
#ifdef PNG_INTERNAL
#include <stdlib.h>

/* The functions exported by PNG_EXTERN are PNG_INTERNAL functions, which
 * aren't usually used outside the library (as far as I know), so it is
 * debatable if they should be exported at all.  In the future, when it is
 * possible to have run-time registry of chunk-handling functions, some of
 * these will be made available again.
#define PNG_EXTERN extern
 */
#define PNG_EXTERN

/* Other defines specific to compilers can go here.  Try to keep
 * them inside an appropriate ifdef/endif pair for portability.
 */

#if defined(MACOS)
/* We need to check that <math.h> hasn't already been included earlier
 * as it seems it doesn't agree with <fp.h>, yet we should really use
 * <fp.h> if possible.
 */
#if !defined(__MATH_H__) && !defined(__MATH_H) && !defined(__cmath__)
#include <fp.h>
#endif
#else
#include <math.h>
#endif

/* Codewarrior on NT has linking problems without this. */
#if defined(__MWERKS__) && defined(WIN32)
#define PNG_ALWAYS_EXTERN
#endif

/* For some reason, Borland C++ defines memcmp, etc. in mem.h, not
 * stdlib.h like it should (I think).  Or perhaps this is a C++
 * "feature"?
 */
#ifdef __TURBOC__
#include <mem.h>
#include "alloc.h"
#endif

#ifdef _MSC_VER
#include <malloc.h>
#endif

/* This controls how fine the dithering gets.  As this allocates
 * a largish chunk of memory (32K), those who are not as concerned
 * with dithering quality can decrease some or all of these.
 */
#ifndef PNG_DITHER_RED_BITS
#define PNG_DITHER_RED_BITS 5
#endif
#ifndef PNG_DITHER_GREEN_BITS
#define PNG_DITHER_GREEN_BITS 5
#endif
#ifndef PNG_DITHER_BLUE_BITS
#define PNG_DITHER_BLUE_BITS 5
#endif

/* This controls how fine the gamma correction becomes when you
 * are only interested in 8 bits anyway.  Increasing this value
 * results in more memory being used, and more pow() functions
 * being called to fill in the gamma tables.  Don't set this value
 * less then 8, and even that may not work (I haven't tested it).
 */

#ifndef PNG_MAX_GAMMA_8
#define PNG_MAX_GAMMA_8 11
#endif

/* This controls how much a difference in gamma we can tolerate before
 * we actually start doing gamma conversion.
 */
#ifndef PNG_GAMMA_THRESHOLD
#define PNG_GAMMA_THRESHOLD 0.05
#endif

#endif /* PNG_INTERNAL */

/* The following uses const char * instead of char * for error
 * and warning message functions, so some compilers won't complain.
 * If you do not want to use const, define PNG_NO_CONST here.
 */

#ifndef PNG_NO_CONST
#  define PNG_CONST const
#else
#  define PNG_CONST
#endif

/* The following defines give you the ability to remove code from the
 * library that you will not be using.  I wish I could figure out how to
 * automate this, but I can't do that without making it seriously hard
 * on the users.  So if you are not using an ability, change the #define
 * to and #undef, and that part of the library will not be compiled.  If
 * your linker can't find a function, you may want to make sure the
 * ability is defined here.  Some of these depend upon some others being
 * defined.  I haven't figured out all the interactions here, so you may
 * have to experiment awhile to get everything to compile.  If you are
 * creating or using a shared library, you probably shouldn't touch this,
 * as it will affect the size of the structures, and this will cause bad
 * things to happen if the library and/or application ever change.
 */

/* Any transformations you will not be using can be undef'ed here */

/* GR-P, 0.96a: Set "*TRANSFORMS_SUPPORTED as default but allow user
   to turn it off with "*TRANSFORMS_NOT_SUPPORTED" or *PNG_NO_*_TRANSFORMS
   on the compile line, then pick and choose which ones to define without
   having to edit this file. It is safe to use the *TRANSFORMS_NOT_SUPPORTED
   if you only want to have a png-compliant reader/writer but don't need
   any of the extra transformations.  This saves about 80 kbytes in a
   typical installation of the library. (PNG_NO_* form added in version
   1.0.1c, for consistency)
 */


#if !defined(PNG_READ_TRANSFORMS_NOT_SUPPORTED) && \
    !defined(PNG_NO_READ_TRANSFORMS)
#define PNG_READ_TRANSFORMS_SUPPORTED
#endif
#ifdef PNG_READ_TRANSFORMS_SUPPORTED
#ifndef PNG_NO_READ_EXPAND
#define PNG_READ_EXPAND_SUPPORTED
#endif
#ifndef PNG_NO_READ_GAMMA
#define PNG_READ_GAMMA_SUPPORTED
#endif
#ifndef PNG_NO_READ_GRAY_TO_RGB
#define PNG_READ_GRAY_TO_RGB_SUPPORTED
#endif
#ifndef PNG_NO_READ_STRIP_ALPHA
#define PNG_READ_STRIP_ALPHA_SUPPORTED
#endif
#endif /* PNG_READ_TRANSFORMS_SUPPORTED */
#ifndef PNG_NO_READ_COMPOSITED_NODIV
#define PNG_READ_COMPOSITE_NODIV_SUPPORTED    /* well tested on Intel and SGI */
#endif
#define PNG_WRITE_INTERLACING_SUPPORTED

#ifndef PNG_NO_STDIO
#define PNG_TIME_RFC1123_SUPPORTED
#endif

/* Any chunks you are not interested in, you can undef here.  The
 * ones that allocate memory may be expecially important (hIST,
 * tEXt, zTXt, tRNS, pCAL).  Others will just save time and make png_info
 * a bit smaller.
 */

#if !defined(PNG_READ_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_READ_ANCILLARY_CHUNKS)
#define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#endif
#ifdef PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#ifndef PNG_NO_READ_gAMA
#define PNG_READ_gAMA_SUPPORTED
#endif
#ifndef PNG_NO_READ_zTXt
#define PNG_READ_zTXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_OPT_PLTE
#define PNG_READ_OPT_PLTE_SUPPORTED /* only affects support of the optional */
#endif                              /* PLTE chunk in RGB and RGBA images */
#endif /* PNG_READ_ANCILLARY_CHUNKS_SUPPORTED */

/* need the time information for reading tIME chunks */
#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
#include <time.h>
#endif

/* Some typedefs to get us started.  These should be safe on most of the
 * common platforms.  The typedefs should be at least as large as the
 * numbers suggest (a png_uint_32 must be at least 32 bits long), but they
 * don't have to be exactly that size.  Some compilers dislike passing
 * unsigned shorts as function parameters, so you may be better off using
 * unsigned int for png_uint_16.  Likewise, for 64-bit systems, you may
 * want to have unsigned int for png_uint_32 instead of unsigned long.
 */

typedef unsigned long png_uint_32;
typedef long png_int_32;
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

/* This is usually size_t.  It is typedef'ed just in case you need it to
   change (I'm not sure if you will or not, so I thought I'd be safe) */
typedef size_t png_size_t;

/* The following is needed for medium model support.  It cannot be in the
 * PNG_INTERNAL section.  Needs modification for other compilers besides
 * MSC.  Model independent support declares all arrays and pointers to be
 * large using the far keyword.  The zlib version used must also support
 * model independent data.  As of version zlib 1.0.4, the necessary changes
 * have been made in zlib.  The USE_FAR_KEYWORD define triggers other
 * changes that are needed. (Tim Wegner)
 */

/* Separate compiler dependencies (problem here is that zlib.h always
   defines FAR. (SJT) */
#ifdef __BORLANDC__
#if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#define LDATA 1
#else
#define LDATA 0
#endif

#if !defined(__WIN32__) && !defined(__FLAT__)
#define PNG_MAX_MALLOC_64K
#if (LDATA != 1)
#ifndef FAR
#define FAR __far
#endif
#define USE_FAR_KEYWORD
#endif   /* LDATA != 1 */

/* Possibly useful for moving data out of default segment.
 * Uncomment it if you want. Could also define FARDATA as
 * const if your compiler supports it. (SJT)
#  define FARDATA FAR
 */
#endif  /* __WIN32__, __FLAT__ */

#endif   /* __BORLANDC__ */


/* Suggest testing for specific compiler first before testing for
 * FAR.  The Watcom compiler defines both __MEDIUM__ and M_I86MM,
 * making reliance oncertain keywords suspect. (SJT)
 */

/* MSC Medium model */
#if defined(FAR)
#  if defined(M_I86MM)
#     define USE_FAR_KEYWORD
#     define FARDATA FAR
#     include <dos.h>
#  endif
#endif

/* SJT: default case */
#ifndef FAR
#   define FAR
#endif

/* At this point FAR is always defined */
#ifndef FARDATA
#define FARDATA
#endif

/* Add typedefs for pointers */
typedef void            FAR * png_voidp;
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;
typedef double          FAR * png_doublep;

/* Pointers to pointers; i.e. arrays */
typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef double          FAR * FAR * png_doublepp;

/* Pointers to pointers to pointers; i.e. pointer to array */
typedef char            FAR * FAR * FAR * png_charppp;

/* libpng typedefs for types in zlib. If zlib changes
 * or another compression library is used, then change these.
 * Eliminates need to change all the source files.
 */
typedef charf *         png_zcharp;
typedef charf * FAR *   png_zcharpp;
typedef z_stream FAR *  png_zstreamp; 

/* allow for compilation as dll under MS Windows */
#ifdef __WIN32DLL__
#define PNG_EXPORT(type,symbol) __declspec(dllexport) type symbol
#endif

/* allow for compilation as dll with BORLAND C++ 5.0 */
#if defined(__BORLANDC__) && defined(_Windows) && defined(__DLL__)
#   define PNG_EXPORT(type,symbol) type _export symbol
#endif

/* allow for compilation as shared lib under BeOS */
#ifdef __BEOSDLL__
#define PNG_EXPORT(type,symbol) __declspec(export) type symbol
#endif

#ifndef PNG_EXPORT
#define PNG_EXPORT(type,symbol) type symbol
#endif


/* User may want to use these so not in PNG_INTERNAL. Any library functions
 * that are passed far data must be model independent.
 */

#if defined(USE_FAR_KEYWORD)  /* memory model independent fns */
/* use this to make far-to-near assignments */
#   define CHECK   1
#   define NOCHECK 0
#   define CVT_PTR(ptr) (png_far_to_near(png_ptr,ptr,CHECK))
#   define CVT_PTR_NOCHECK(ptr) (png_far_to_near(png_ptr,ptr,NOCHECK))
#   define png_strcpy _fstrcpy
#   define png_strlen _fstrlen
#   define png_memcmp _fmemcmp      /* SJT: added */
#   define png_memcpy _fmemcpy
#   define png_memset _fmemset
#else /* use the usual functions */
#   define CVT_PTR(ptr)         (ptr)
#   define CVT_PTR_NOCHECK(ptr) (ptr)
#   define png_strcpy strcpy
#   define png_strlen strlen
#   define png_memcmp memcmp     /* SJT: added */
#   define png_memcpy memcpy
#   define png_memset memset
#endif
/* End of memory model independent support */

/* Just a double check that someone hasn't tried to define something
 * contradictory. 
 */
#if (PNG_ZBUF_SIZE > 65536) && defined(PNG_MAX_MALLOC_64K)
#undef PNG_ZBUF_SIZE
#define PNG_ZBUF_SIZE 65536
#endif

#endif /* PNGCONF_H */

