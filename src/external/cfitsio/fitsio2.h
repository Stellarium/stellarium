#ifndef _FITSIO2_H
#define _FITSIO2_H
 
#include "fitsio.h"

/* 
    Threading support using POSIX threads programming interface
    (supplied by Bruce O'Neel) 

    All threaded programs MUST have the 

    -D_REENTRANT

    on the compile line and must link with -lpthread.  This means that
    when one builds cfitsio for threads you must have -D_REENTRANT on the
    gcc or cc command line.
*/

#ifdef _REENTRANT
#include <pthread.h>
/*  #include <assert.h>  not needed any more */
extern pthread_mutex_t Fitsio_Lock;
extern int Fitsio_Pthread_Status;

#define FFLOCK1(lockname)   (Fitsio_Pthread_Status = pthread_mutex_lock(&lockname))
#define FFUNLOCK1(lockname) (Fitsio_Pthread_Status = pthread_mutex_unlock(&lockname))
#define FFLOCK   FFLOCK1(Fitsio_Lock)
#define FFUNLOCK FFUNLOCK1(Fitsio_Lock)
#define ffstrtok(str, tok, save) strtok_r(str, tok, save)

#else
#define FFLOCK
#define FFUNLOCK
#define ffstrtok(str, tok, save) strtok(str, tok)
#endif

/*
  If REPLACE_LINKS is defined, then whenever CFITSIO fails to open
  a file with write access because it is a soft link to a file that
  only has read access, then CFITSIO will attempt to replace
  the link with a local copy of the file, with write access.  This
  feature was originally added to support the ftools in the Hera
  environment, where many of the user's data file are soft links.
*/
#if defined(BUILD_HERA)
#define REPLACE_LINKS 1
#endif

#define USE_LARGE_VALUE -99  /* flag used when writing images */

#define DBUFFSIZE 28800 /* size of data buffer in bytes */

#define NMAXFILES  1000   /* maximum number of FITS files that can be opened */
        /* CFITSIO will allocate (NMAXFILES * 80) bytes of memory */
	/* plus each file that is opened will use NIOBUF * 2880 bytes of memeory */
	/* where NIOBUF is defined in fitio.h and has a default value of 40 */

#define MINDIRECT 8640   /* minimum size for direct reads and writes */
                         /* MINDIRECT must have a value >= 8640 */

/*   it is useful to identify certain specific types of machines   */
#define NATIVE             0 /* machine that uses non-byteswapped IEEE formats */
#define OTHERTYPE          1  /* any other type of machine */
#define VAXVMS             3  /* uses an odd floating point format */
#define ALPHAVMS           4  /* uses an odd floating point format */
#define IBMPC              5  /* used in drvrfile.c to work around a bug on PCs */
#define CRAY               6  /* requires a special NaN test algorithm */

#define GFLOAT             1  /* used for VMS */
#define IEEEFLOAT          2  /* used for VMS */

/* ======================================================================= */
/* The following logic is used to determine the type machine,              */
/*  whether the bytes are swapped, and the number of bits in a long value  */
/* ======================================================================= */

/*   The following platforms have sizeof(long) == 8               */
/*   This block of code should match a similar block in fitsio.h  */
/*   and the block of code at the beginning of f77_wrap.h         */

#if defined(__alpha) && ( defined(__unix__) || defined(__NetBSD__) )
                                  /* old Dec Alpha platforms running OSF */
#define BYTESWAPPED TRUE
#define LONGSIZE 64

#elif defined(__sparcv9) || (defined(__sparc__) && defined(__arch64__))
                               /*  SUN Solaris7 in 64-bit mode */
#define BYTESWAPPED FALSE
#define MACHINE NATIVE
#define LONGSIZE 64   

                            /* IBM System z mainframe support */ 
#elif defined(__s390x__)
#define BYTESWAPPED FALSE
#define LONGSIZE 64

#elif defined(__s390__)
#define BYTESWAPPED FALSE
#define LONGSIZE 32

#elif defined(__ia64__)  || defined(__x86_64__) || defined(__AARCH64EL__)
                  /*  Intel itanium 64-bit PC, or AMD opteron 64-bit PC */
#define BYTESWAPPED TRUE
#define LONGSIZE 64   

#elif defined(_SX)             /* Nec SuperUx */

#define BYTESWAPPED FALSE
#define MACHINE NATIVE
#define LONGSIZE 64

#elif defined(__powerpc64__) || defined(__64BIT__) || defined(__AARCH64EB__)  /* IBM 64-bit AIX powerpc*/
                              /* could also test for __ppc64__ or __PPC64 */

#  if defined(__LITTLE_ENDIAN__)
#   define BYTESWAPPED TRUE
#  else
#   define BYTESWAPPED FALSE
#   define MACHINE NATIVE
#  endif
#  define LONGSIZE 64

#elif defined(_MIPS_SZLONG)

#  if defined(MIPSEL)
#    define BYTESWAPPED TRUE
#  else
#    define BYTESWAPPED FALSE
#    define MACHINE NATIVE
#  endif

#  if _MIPS_SZLONG == 32
#    define LONGSIZE 32
#  elif _MIPS_SZLONG == 64
#    define LONGSIZE 64
#  else
#    error "can't handle long size given by _MIPS_SZLONG"
#  endif

/* ============================================================== */
/*  the following are all 32-bit byteswapped platforms            */

#elif defined(vax) && defined(VMS)
 
#define MACHINE VAXVMS
#define BYTESWAPPED TRUE
 
#elif defined(__alpha) && defined(__VMS)

#if (__D_FLOAT == TRUE)

/* this float option is the same as for VAX/VMS machines. */
#define MACHINE VAXVMS
#define BYTESWAPPED TRUE
 
#elif  (__G_FLOAT == TRUE)
 
/*  G_FLOAT is the default for ALPHA VMS systems */
#define MACHINE ALPHAVMS
#define BYTESWAPPED TRUE
#define FLOATTYPE GFLOAT
 
#elif  (__IEEE_FLOAT == TRUE)
 
#define MACHINE ALPHAVMS
#define BYTESWAPPED TRUE
#define FLOATTYPE IEEEFLOAT

#endif  /* end of alpha VMS case */

#elif defined(ultrix) && defined(unix)
 /* old Dec ultrix machines */
#define BYTESWAPPED TRUE
 
#elif defined(__i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) \
  || defined(_MSC_VER) || defined(__BORLANDC__) || defined(__TURBOC__) \
  || defined(_NI_mswin_) || defined(__EMX__)

/*  generic 32-bit IBM PC */
#define MACHINE IBMPC
#define BYTESWAPPED TRUE

#elif defined(__arm__)

/* This assumes all ARM are little endian.  In the future, it might be  */
/* necessary to use  "if defined(__ARMEL__)"  to distinguish little from big. */
/* (__ARMEL__ would be defined on little-endian, but not on big-endian). */

#define BYTESWAPPED TRUE
 
#elif defined(__tile__)

/*  64-core 8x8-architecture Tile64 platform */

#define BYTESWAPPED TRUE

#elif defined(__sh__)

/* SuperH CPU can be used in both little and big endian modes */

#if defined(__LITTLE_ENDIAN__)
#define BYTESWAPPED TRUE
#else
#define BYTESWAPPED FALSE
#endif
 
#else

/*  assume all other machine uses the same IEEE formats as used in FITS files */
/*  e.g., Macs fall into this category  */

#define MACHINE NATIVE
#define BYTESWAPPED FALSE
 
#endif

#ifndef MACHINE
#define MACHINE  OTHERTYPE
#endif

/*  assume longs are 4 bytes long, unless previously set otherwise */
#ifndef LONGSIZE
#define LONGSIZE 32
#endif

/*       end of block that determine long size and byte swapping        */ 
/* ==================================================================== */
 
#define IGNORE_EOF 1
#define REPORT_EOF 0
#define DATA_UNDEFINED -1
#define NULL_UNDEFINED 1234554321
#define ASCII_NULL_UNDEFINED 1   /* indicate no defined null value */
 
#define maxvalue(A,B) ((A) > (B) ? (A) : (B))
#define minvalue(A,B) ((A) < (B) ? (A) : (B))

/* faster string comparison macros */
#define FSTRCMP(a,b)     ((a)[0]<(b)[0]? -1:(a)[0]>(b)[0]?1:strcmp((a),(b)))
#define FSTRNCMP(a,b,n)  ((a)[0]<(b)[0]?-1:(a)[0]>(b)[0]?1:strncmp((a),(b),(n)))

#if defined(__VMS) || defined(VMS)
 
#define FNANMASK   0xFFFF /* mask all bits  */
#define DNANMASK   0xFFFF /* mask all bits  */
 
#else
 
#define FNANMASK   0x7F80 /* mask bits 1 - 8; all set on NaNs */
                                     /* all 0 on underflow  or 0. */
 
#define DNANMASK   0x7FF0 /* mask bits 1 - 11; all set on NaNs */
                                     /* all 0 on underflow  or 0. */
 
#endif
 
#if MACHINE == CRAY
    /*
      Cray machines:   the large negative integer corresponds
      to the 3 most sig digits set to 1.   If these
      3 bits are set in a floating point number (64 bits), then it represents
      a reserved value (i.e., a NaN)
    */
#define fnan(L) ( (L) >= 0xE000000000000000 ? 1 : 0) )
 
#else
    /* these functions work for both big and little endian machines */
    /* that use the IEEE floating point format for internal numbers */
 
   /* These functions tests whether the float value is a reserved IEEE     */
   /* value such as a Not-a-Number (NaN), or underflow, overflow, or       */
   /* infinity.   The functions returns 1 if the value is a NaN, overflow  */
   /* or infinity; it returns 2 if the value is an denormalized underflow  */
   /* value; otherwise it returns 0. fnan tests floats, dnan tests doubles */
 
#define fnan(L) \
      ( (L & FNANMASK) == FNANMASK ?  1 : (L & FNANMASK) == 0 ? 2 : 0)
 
#define dnan(L) \
      ( (L & DNANMASK) == DNANMASK ?  1 : (L & DNANMASK) == 0 ? 2 : 0)
 
#endif

#define DSCHAR_MAX  127.49 /* max double value that fits in an signed char */
#define DSCHAR_MIN -128.49 /* min double value that fits in an signed char */
#define DUCHAR_MAX  255.49 /* max double value that fits in an unsigned char */
#define DUCHAR_MIN -0.49   /* min double value that fits in an unsigned char */
#define DUSHRT_MAX  65535.49 /* max double value that fits in a unsigned short*/
#define DUSHRT_MIN -0.49   /* min double value that fits in an unsigned short */
#define DSHRT_MAX  32767.49 /* max double value that fits in a short */
#define DSHRT_MIN -32768.49 /* min double value that fits in a short */

#if LONGSIZE == 32
#  define DLONG_MAX  2147483647.49 /* max double value that fits in a long */
#  define DLONG_MIN -2147483648.49 /* min double value that fits in a long */
#  define DULONG_MAX 4294967295.49 /* max double that fits in a unsigned long */
#else
#  define DLONG_MAX   9.2233720368547752E18 /* max double value  long */
#  define DLONG_MIN  -9.2233720368547752E18 /* min double value  long */
#  define DULONG_MAX 1.84467440737095504E19 /* max double value  ulong */
#endif

#define DULONG_MIN -0.49   /* min double value that fits in an unsigned long */
#define DLONGLONG_MAX  9.2233720368547755807E18 /* max double value  longlong */
#define DLONGLONG_MIN -9.2233720368547755808E18 /* min double value  longlong */
#define DUINT_MAX 4294967295.49 /* max dbl that fits in a unsigned 4-byte int */
#define DUINT_MIN -0.49   /* min dbl that fits in an unsigned 4-byte int */
#define DINT_MAX  2147483647.49 /* max double value that fits in a 4-byte int */
#define DINT_MIN -2147483648.49 /* min double value that fits in a 4-byte int */

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U /* max unsigned 32-bit integer */
#endif
#ifndef INT32_MAX
#define INT32_MAX  2147483647 /* max 32-bit integer */
#endif
#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX -1) /* min 32-bit integer */
#endif


#define COMPRESS_NULL_VALUE -2147483647
#define N_RANDOM 10000  /* DO NOT CHANGE THIS;  used when quantizing real numbers */

int ffgnky(fitsfile *fptr, char *card, int *status);
void ffcfmt(char *tform, char *cform);
void ffcdsp(char *tform, char *cform);
void ffswap2(short *values, long nvalues);
void ffswap4(INT32BIT *values, long nvalues);
void ffswap8(double *values, long nvalues);
int ffi2c(LONGLONG ival, char *cval, int *status);
int ffl2c(int lval, char *cval, int *status);
int ffs2c(const char *instr, char *outstr, int *status);
int ffr2f(float fval, int decim, char *cval, int *status);
int ffr2e(float fval, int decim, char *cval, int *status);
int ffd2f(double dval, int decim, char *cval, int *status);
int ffd2e(double dval, int decim, char *cval, int *status);
int ffc2ii(const char *cval, long *ival, int *status);
int ffc2jj(const char *cval, LONGLONG *ival, int *status);
int ffc2ll(const char *cval, int *lval, int *status);
int ffc2rr(const char *cval, float *fval, int *status);
int ffc2dd(const char *cval, double *dval, int *status);
int ffc2x(const char *cval, char *dtype, long *ival, int *lval, char *sval,
          double *dval, int *status);
int ffc2xx(const char *cval, char *dtype, LONGLONG *ival, int *lval, char *sval,
          double *dval, int *status);
int ffc2s(const char *instr, char *outstr, int *status);
int ffc2i(const char *cval, long *ival, int *status);
int ffc2j(const char *cval, LONGLONG *ival, int *status);
int ffc2r(const char *cval, float *fval, int *status);
int ffc2d(const char *cval, double *dval, int *status);
int ffc2l(const char *cval, int *lval, int *status);
void ffxmsg(int action, char *err_message);
int ffgcnt(fitsfile *fptr, char *value, char *comm, int *status);
int ffgtkn(fitsfile *fptr, int numkey, char *keyname, long *value, int *status);
int ffgtknjj(fitsfile *fptr, int numkey, char *keyname, LONGLONG *value, int *status);
int fftkyn(fitsfile *fptr, int numkey, char *keyname, char *value, int *status);
int ffgphd(fitsfile *fptr, int maxdim, int *simple, int *bitpix, int *naxis,
        LONGLONG naxes[], long *pcount, long *gcount, int *extend, double *bscale,
          double *bzero, LONGLONG *blank, int *nspace, int *status);
int ffgttb(fitsfile *fptr, LONGLONG *rowlen, LONGLONG *nrows, LONGLONG *pcount,
          long *tfield, int *status);
 
int ffmkey(fitsfile *fptr, const char *card, int *status);
 
/*  ffmbyt has been moved to fitsio.h */
int ffgbyt(fitsfile *fptr, LONGLONG nbytes, void *buffer, int *status);
int ffpbyt(fitsfile *fptr, LONGLONG nbytes, void *buffer, int *status);
int ffgbytoff(fitsfile *fptr, long gsize, long ngroups, long offset, 
           void *buffer, int *status);
int ffpbytoff(fitsfile *fptr, long gsize, long ngroups, long offset,
           void *buffer, int *status);
int ffldrc(fitsfile *fptr, long record, int err_mode, int *status);
int ffwhbf(fitsfile *fptr, int *nbuff);
int ffbfeof(fitsfile *fptr, int *status);
int ffbfwt(FITSfile *Fptr, int nbuff, int *status);
int ffpxsz(int datatype);

int ffourl(char *url, char *urltype, char *outfile, char *tmplfile,
            char *compspec, int *status);
int ffparsecompspec(fitsfile *fptr, char *compspec, int *status);
int ffoptplt(fitsfile *fptr, const char *tempname, int *status);
int fits_is_this_a_copy(char *urltype);
int fits_store_Fptr(FITSfile *Fptr, int *status);
int fits_clear_Fptr(FITSfile *Fptr, int *status);
int fits_already_open(fitsfile **fptr, char *url, 
    char *urltype, char *infile, char *extspec, char *rowfilter,
    char *binspec, char *colspec, int  mode,int  *isopen, int  *status);
int ffedit_columns(fitsfile **fptr, char *outfile, char *expr, int *status);
int fits_get_col_minmax(fitsfile *fptr, int colnum, float *datamin, 
                     float *datamax, int *status);
int ffwritehisto(long totaln, long offset, long firstn, long nvalues,
             int narrays, iteratorCol *imagepars, void *userPointer);
int ffcalchist(long totalrows, long offset, long firstrow, long nrows,
             int ncols, iteratorCol *colpars, void *userPointer);
int ffpinit(fitsfile *fptr, int *status);
int ffainit(fitsfile *fptr, int *status);
int ffbinit(fitsfile *fptr, int *status);
int ffchdu(fitsfile *fptr, int *status);
int ffwend(fitsfile *fptr, int *status);
int ffpdfl(fitsfile *fptr, int *status);
int ffuptf(fitsfile *fptr, int *status);

int ffdblk(fitsfile *fptr, long nblocks, int *status);
int ffgext(fitsfile *fptr, int moveto, int *exttype, int *status);
int ffgtbc(fitsfile *fptr, LONGLONG *totalwidth, int *status);
int ffgtbp(fitsfile *fptr, char *name, char *value, int *status);
int ffiblk(fitsfile *fptr, long nblock, int headdata, int *status);
int ffshft(fitsfile *fptr, LONGLONG firstbyte, LONGLONG nbytes, LONGLONG nshift,
    int *status);
 
 int ffgcprll(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, int writemode, double *scale, double *zero, char *tform,
           long *twidth, int *tcode, int *maxelem, LONGLONG *startpos,
           LONGLONG *elemnum, long *incre, LONGLONG *repeat, LONGLONG *rowlen,
           int *hdutype, LONGLONG *tnull, char *snull, int *status);
	   
int ffflushx(FITSfile *fptr);
int ffseek(FITSfile *fptr, LONGLONG position);
int ffread(FITSfile *fptr, long nbytes, void *buffer,
            int *status);
int ffwrite(FITSfile *fptr, long nbytes, void *buffer,
            int *status);
int fftrun(fitsfile *fptr, LONGLONG filesize, int *status);

int ffpcluc(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, int *status);
	   
int ffgcll(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, int nultyp, char nulval, char *array, char *nularray,
           int *anynul, int *status);
int ffgcls(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, int nultyp, char *nulval,
           char **array, char *nularray, int *anynul, int  *status);
int ffgcls2(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, int nultyp, char *nulval,
           char **array, char *nularray, int *anynul, int  *status);
int ffgclb(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long  elemincre, int nultyp, unsigned char nulval,
           unsigned char *array, char *nularray, int *anynul, int  *status);
int ffgclsb(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long  elemincre, int nultyp, signed char nulval,
           signed char *array, char *nularray, int *anynul, int  *status);
int ffgclui(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long  elemincre, int nultyp, unsigned short nulval,
           unsigned short *array, char *nularray, int *anynul, int  *status);
int ffgcli(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long  elemincre, int nultyp, short nulval,
           short *array, char *nularray, int *anynul, int  *status);
int ffgcluj(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, unsigned long nulval,
           unsigned long *array, char *nularray, int *anynul, int  *status);
int ffgcljj(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, LONGLONG nulval, 
           LONGLONG *array, char *nularray, int *anynul, int  *status);
int ffgclj(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, long nulval, long *array,
           char *nularray, int *anynul, int  *status);
int ffgcluk(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, unsigned int nulval,
           unsigned int *array, char *nularray, int *anynul, int  *status);
int ffgclk(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, int nulval, int *array,
           char *nularray, int *anynul, int  *status);
int ffgcle(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp,  float nulval, float *array,
           char *nularray, int *anynul, int  *status);
int ffgcld(fitsfile *fptr, int colnum, LONGLONG firstrow, LONGLONG firstelem,
           LONGLONG nelem, long elemincre, int nultyp, double nulval,
           double *array, char *nularray, int *anynul, int  *status);
 
int ffpi1b(fitsfile *fptr, long nelem, long incre, unsigned char *buffer,
           int *status);
int ffpi2b(fitsfile *fptr, long nelem, long incre, short *buffer, int *status);
int ffpi4b(fitsfile *fptr, long nelem, long incre, INT32BIT *buffer,
           int *status);
int ffpi8b(fitsfile *fptr, long nelem, long incre, long *buffer, int *status);
int ffpr4b(fitsfile *fptr, long nelem, long incre, float *buffer, int *status);
int ffpr8b(fitsfile *fptr, long nelem, long incre, double *buffer, int *status);
 
int ffgi1b(fitsfile *fptr, LONGLONG pos, long nelem, long incre,
          unsigned char *buffer, int *status);
int ffgi2b(fitsfile *fptr, LONGLONG pos, long nelem, long incre, short *buffer,
          int *status);
int ffgi4b(fitsfile *fptr, LONGLONG pos, long nelem, long incre, INT32BIT *buffer,
          int *status);
int ffgi8b(fitsfile *fptr, LONGLONG pos, long nelem, long incre, long *buffer,
          int *status);
int ffgr4b(fitsfile *fptr, LONGLONG pos, long nelem, long incre, float *buffer,
          int *status);
int ffgr8b(fitsfile *fptr, LONGLONG pos, long nelem, long incre, double *buffer,
          int *status);
 
int ffcins(fitsfile *fptr, LONGLONG naxis1, LONGLONG naxis2, LONGLONG nbytes,
           LONGLONG bytepos, int *status);
int ffcdel(fitsfile *fptr, LONGLONG naxis1, LONGLONG naxis2, LONGLONG nbytes,
           LONGLONG bytepos, int *status);
int ffkshf(fitsfile *fptr, int firstcol, int tfields, int nshift, int *status);
 
int fffi1i1(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, unsigned char nullval, char
             *nullarray, int *anynull, unsigned char *output, int *status);
int fffi2i1(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, unsigned char nullval, char *nullarray,
            int *anynull, unsigned char *output, int *status);
int fffi4i1(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, unsigned char nullval, char *nullarray,
            int *anynull, unsigned char *output, int *status);
int fffi8i1(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, unsigned char nullval, char *nullarray,
            int *anynull, unsigned char *output, int *status);
int fffr4i1(float *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char nullval, char *nullarray,
            int *anynull, unsigned char *output, int *status);
int fffr8i1(double *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char nullval, char *nullarray,
            int *anynull, unsigned char *output, int *status);
int fffstri1(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            unsigned char nullval, char *nullarray, int *anynull,
            unsigned char *output, int *status);
 
int fffi1s1(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, signed char nullval, char
             *nullarray, int *anynull, signed char *output, int *status);
int fffi2s1(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, signed char nullval, char *nullarray,
            int *anynull, signed char *output, int *status);
int fffi4s1(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, signed char nullval, char *nullarray,
            int *anynull, signed char *output, int *status);
int fffi8s1(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, signed char nullval, char *nullarray,
            int *anynull, signed char *output, int *status);
int fffr4s1(float *input, long ntodo, double scale, double zero,
            int nullcheck, signed char nullval, char *nullarray,
            int *anynull, signed char *output, int *status);
int fffr8s1(double *input, long ntodo, double scale, double zero,
            int nullcheck, signed char nullval, char *nullarray,
            int *anynull, signed char *output, int *status);
int fffstrs1(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            signed char nullval, char *nullarray, int *anynull,
            signed char *output, int *status);

int fffi1u2(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, unsigned short nullval, 
            char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffi2u2(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, unsigned short nullval, char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffi4u2(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, unsigned short nullval, char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffi8u2(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, unsigned short nullval, char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffr4u2(float *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned short nullval, char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffr8u2(double *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned short nullval, char *nullarray,
            int *anynull, unsigned short *output, int *status);
int fffstru2(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            unsigned short nullval, char *nullarray, int  *anynull, 
            unsigned short *output, int *status);

int fffi1i2(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffi2i2(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffi4i2(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffi8i2(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffr4i2(float *input, long ntodo, double scale, double zero,
            int nullcheck, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffr8i2(double *input, long ntodo, double scale, double zero,
            int nullcheck, short nullval, char *nullarray,
            int *anynull, short *output, int *status);
int fffstri2(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            short nullval, char *nullarray, int  *anynull, short *output,
            int *status);

int fffi1u4(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, unsigned long nullval,
            char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffi2u4(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, unsigned long nullval, char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffi4u4(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, unsigned long nullval, char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffi8u4(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, unsigned long nullval, char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffr4u4(float *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned long nullval, char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffr8u4(double *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned long nullval, char *nullarray,
            int *anynull, unsigned long *output, int *status);
int fffstru4(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            unsigned long nullval, char *nullarray, int *anynull,
            unsigned long *output, int *status);
 
int fffi1i4(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffi2i4(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffi4i4(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffi8i4(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffr4i4(float *input, long ntodo, double scale, double zero,
            int nullcheck, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffr8i4(double *input, long ntodo, double scale, double zero,
            int nullcheck, long nullval, char *nullarray,
            int *anynull, long *output, int *status);
int fffstri4(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            long nullval, char *nullarray, int *anynull, long *output,
            int *status);
 
int fffi1int(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffi2int(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffi4int(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffi8int(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffr4int(float *input, long ntodo, double scale, double zero,
            int nullcheck, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffr8int(double *input, long ntodo, double scale, double zero,
            int nullcheck, int nullval, char *nullarray,
            int *anynull, int *output, int *status);
int fffstrint(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            int nullval, char *nullarray, int *anynull, int *output,
            int *status);
 
int fffi1uint(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, unsigned int nullval,
            char *nullarray, int *anynull, unsigned int *output, int *status);
int fffi2uint(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, unsigned int nullval, char *nullarray,
            int *anynull, unsigned int *output, int *status);
int fffi4uint(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, unsigned int nullval, char *nullarray,
            int *anynull, unsigned int *output, int *status);
int fffi8uint(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, unsigned int nullval, char *nullarray,
            int *anynull, unsigned int *output, int *status);
int fffr4uint(float *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned int nullval, char *nullarray,
            int *anynull, unsigned int *output, int *status);
int fffr8uint(double *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned int nullval, char *nullarray,
            int *anynull, unsigned int *output, int *status);
int fffstruint(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            unsigned int nullval, char *nullarray, int *anynull,
            unsigned int *output, int *status);
 
int fffi1i8(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, LONGLONG nullval, 
            char *nullarray, int *anynull, LONGLONG *output, int *status);
int fffi2i8(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, LONGLONG nullval, char *nullarray,
            int *anynull, LONGLONG *output, int *status);
int fffi4i8(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, LONGLONG nullval, char *nullarray,
            int *anynull, LONGLONG *output, int *status);
int fffi8i8(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, LONGLONG nullval, char *nullarray,
            int *anynull, LONGLONG *output, int *status);
int fffr4i8(float *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG nullval, char *nullarray,
            int *anynull, LONGLONG *output, int *status);
int fffr8i8(double *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG nullval, char *nullarray,
            int *anynull, LONGLONG *output, int *status);
int fffstri8(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            LONGLONG nullval, char *nullarray, int *anynull, LONGLONG *output,
            int *status);

int fffi1r4(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffi2r4(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffi4r4(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffi8r4(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffr4r4(float *input, long ntodo, double scale, double zero,
            int nullcheck, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffr8r4(double *input, long ntodo, double scale, double zero,
            int nullcheck, float nullval, char *nullarray,
            int *anynull, float *output, int *status);
int fffstrr4(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            float nullval, char *nullarray, int *anynull, float *output,
            int *status);
 
int fffi1r8(unsigned char *input, long ntodo, double scale, double zero,
            int nullcheck, unsigned char tnull, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffi2r8(short *input, long ntodo, double scale, double zero,
            int nullcheck, short tnull, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffi4r8(INT32BIT *input, long ntodo, double scale, double zero,
            int nullcheck, INT32BIT tnull, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffi8r8(LONGLONG *input, long ntodo, double scale, double zero,
            int nullcheck, LONGLONG tnull, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffr4r8(float *input, long ntodo, double scale, double zero,
            int nullcheck, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffr8r8(double *input, long ntodo, double scale, double zero,
            int nullcheck, double nullval, char *nullarray,
            int *anynull, double *output, int *status);
int fffstrr8(char *input, long ntodo, double scale, double zero,
            long twidth, double power, int nullcheck, char *snull,
            double nullval, char *nullarray, int *anynull, double *output,
            int *status);
 
int ffi1fi1(unsigned char *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffs1fi1(signed char *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffu2fi1(unsigned short *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffi2fi1(short *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffu4fi1(unsigned long *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffi4fi1(long *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffi8fi1(LONGLONG *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffuintfi1(unsigned int *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffintfi1(int *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffr4fi1(float *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
int ffr8fi1(double *array, long ntodo, double scale, double zero,
            unsigned char *buffer, int *status);
 
int ffi1fi2(unsigned char *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffs1fi2(signed char *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffu2fi2(unsigned short *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffi2fi2(short *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffu4fi2(unsigned long *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffi4fi2(long *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffi8fi2(LONGLONG *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffuintfi2(unsigned int *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffintfi2(int *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffr4fi2(float *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
int ffr8fi2(double *array, long ntodo, double scale, double zero,
            short *buffer, int *status);
 
int ffi1fi4(unsigned char *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffs1fi4(signed char *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffu2fi4(unsigned short *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffi2fi4(short *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffu4fi4(unsigned long *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffi4fi4(long *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffi8fi4(LONGLONG *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffuintfi4(unsigned int *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffintfi4(int *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffr4fi4(float *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);
int ffr8fi4(double *array, long ntodo, double scale, double zero,
            INT32BIT *buffer, int *status);

int fflongfi8(long *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffi8fi8(LONGLONG *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffi2fi8(short *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffi1fi8(unsigned char *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffs1fi8(signed char *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffr4fi8(float *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffr8fi8(double *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffintfi8(int *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffu2fi8(unsigned short *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffu4fi8(unsigned long *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);
int ffuintfi8(unsigned int *array, long ntodo, double scale, double zero,
            LONGLONG *buffer, int *status);

int ffi1fr4(unsigned char *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffs1fr4(signed char *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffu2fr4(unsigned short *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffi2fr4(short *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffu4fr4(unsigned long *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffi4fr4(long *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffi8fr4(LONGLONG *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffuintfr4(unsigned int *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffintfr4(int *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffr4fr4(float *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
int ffr8fr4(double *array, long ntodo, double scale, double zero,
            float *buffer, int *status);
 
int ffi1fr8(unsigned char *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffs1fr8(signed char *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffu2fr8(unsigned short *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffi2fr8(short *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffu4fr8(unsigned long *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffi4fr8(long *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffi8fr8(LONGLONG *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffuintfr8(unsigned int *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffintfr8(int *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffr4fr8(float *array, long ntodo, double scale, double zero,
            double *buffer, int *status);
int ffr8fr8(double *array, long ntodo, double scale, double zero,
            double *buffer, int *status);

int ffi1fstr(unsigned char *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffs1fstr(signed char *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffu2fstr(unsigned short *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffi2fstr(short *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffu4fstr(unsigned long *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffi4fstr(long *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffi8fstr(LONGLONG *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffintfstr(int *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffuintfstr(unsigned int *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffr4fstr(float *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);
int ffr8fstr(double *input, long ntodo, double scale, double zero,
            char *cform, long twidth, char *output, int *status);

/*  the following 4 routines are VMS macros used on VAX or Alpha VMS */
void ieevpd(double *inarray, double *outarray, long *nvals);
void ieevud(double *inarray, double *outarray, long *nvals);
void ieevpr(float *inarray, float *outarray, long *nvals);
void ieevur(float *inarray, float *outarray, long *nvals);

/*  routines related to the lexical parser  */
int  ffselect_table(fitsfile **fptr, char *outfile, char *expr,  int *status);
int  ffiprs( fitsfile *fptr, int compressed, char *expr, int maxdim,
	     int *datatype, long *nelem, int *naxis, long *naxes,
	     int *status );
void ffcprs( void );
int  ffcvtn( int inputType, void *input, char *undef, long ntodo,
	     int outputType, void *nulval, void *output,
	     int *anynull, int *status );
int  parse_data( long totalrows, long offset, long firstrow,
                 long nrows, int nCols, iteratorCol *colData,
                 void *userPtr );
int  uncompress_hkdata( fitsfile *fptr, long ntimes, 
                        double *times, int *status );
int  ffffrw_work( long totalrows, long offset, long firstrow,
                  long nrows, int nCols, iteratorCol *colData,
                  void *userPtr );

int fits_translate_pixkeyword(char *inrec, char *outrec,char *patterns[][2],
    int npat, int naxis, int *colnum, int *pat_num, int *i,
      int *j, int *n, int *m, int *l, int *status);

/*  image compression routines */
int fits_write_compressed_img(fitsfile *fptr, 
            int  datatype, long  *fpixel, long *lpixel,   
            int nullcheck, void *array,  void *nulval,
            int  *status);
int fits_write_compressed_pixels(fitsfile *fptr, 
            int  datatype, LONGLONG  fpixel, LONGLONG npixels,   
            int nullcheck,  void *array, void *nulval,
            int  *status);
int fits_write_compressed_img_plane(fitsfile *fptr, int  datatype, 
      int  bytesperpixel,  long   nplane, long *firstcoord, long *lastcoord, 
      long *naxes,  int  nullcheck, 
      void *array,  void *nullval, long *nread, int  *status);

int imcomp_init_table(fitsfile *outfptr,
        int bitpix, int naxis,long *naxes, int writebitpix, int *status);
int imcomp_calc_max_elem (int comptype, int nx, int zbitpix, int blocksize);
int imcomp_copy_imheader(fitsfile *infptr, fitsfile *outfptr,
                int *status);
int imcomp_copy_img2comp(fitsfile *infptr, fitsfile *outfptr, int *status);
int imcomp_copy_comp2img(fitsfile *infptr, fitsfile *outfptr, 
                          int norec, int *status);
int imcomp_copy_prime2img(fitsfile *infptr, fitsfile *outfptr, int *status);
int imcomp_compress_image (fitsfile *infptr, fitsfile *outfptr,
                 int *status);
int imcomp_compress_tile (fitsfile *outfptr, long row, 
    int datatype,  void *tiledata, long tilelen, long nx, long ny,
    int nullcheck, void *nullval, int *status);
int imcomp_nullscale(int *idata, long tilelen, int nullflagval, int nullval,
     double scale, double zero, int * status);
int imcomp_nullvalues(int *idata, long tilelen, int nullflagval, int nullval,
     int * status);
int imcomp_scalevalues(int *idata, long tilelen, double scale, double zero,
     int * status);
int imcomp_nullscalefloats(float *fdata, long tilelen, int *idata, 
    double scale, double zero, int nullcheck, float nullflagval, int nullval,
    int *status);
int imcomp_nullfloats(float *fdata, long tilelen, int *idata, int nullcheck,
    float nullflagval, int nullval, int *status);
int imcomp_nullscaledoubles(double *fdata, long tilelen, int *idata, 
    double scale, double zero, int nullcheck, double nullflagval, int nullval,
    int *status);
int imcomp_nulldoubles(double *fdata, long tilelen, int *idata, int nullcheck,
    double nullflagval, int nullval, int *status);
    
 
/*  image decompression routines */
int fits_read_compressed_img(fitsfile *fptr, 
            int  datatype, LONGLONG  *fpixel,LONGLONG  *lpixel,long *inc,   
            int nullcheck, void *nulval,  void *array, char *nullarray,
            int  *anynul, int  *status);
int fits_read_compressed_pixels(fitsfile *fptr, 
            int  datatype, LONGLONG  fpixel, LONGLONG npixels,   
            int nullcheck, void *nulval,  void *array, char *nullarray,
            int  *anynul, int  *status);
int fits_read_compressed_img_plane(fitsfile *fptr, int  datatype, 
      int  bytesperpixel,  long   nplane, LONGLONG *firstcoord, LONGLONG *lastcoord, 
      long *inc,  long *naxes,  int  nullcheck,  void *nullval, 
      void *array, char *nullarray, int  *anynul, long *nread, int  *status);

int imcomp_get_compressed_image_par(fitsfile *infptr, int *status);
int imcomp_decompress_tile (fitsfile *infptr,
          int nrow, int tilesize, int datatype, int nullcheck,
          void *nulval, void *buffer, char *bnullarray, int *anynul,
          int *status);
int imcomp_copy_overlap (char *tile, int pixlen, int ndim,
         long *tfpixel, long *tlpixel, char *bnullarray, char *image,
         long *fpixel, long *lpixel, long *inc, int nullcheck, char *nullarray,
         int *status);
int imcomp_test_overlap (int ndim, long *tfpixel, long *tlpixel, 
         long *fpixel, long *lpixel, long *inc, int *status);
int imcomp_merge_overlap (char *tile, int pixlen, int ndim,
         long *tfpixel, long *tlpixel, char *bnullarray, char *image,
         long *fpixel, long *lpixel, int nullcheck, int *status);
int imcomp_decompress_img(fitsfile *infptr, fitsfile *outfptr, int datatype,
         int  *status);
int fits_quantize_float (long row, float fdata[], long nx, long ny, int nullcheck,
         float in_null_value, float quantize_level, 
           int dither_method, int idata[], double *bscale, double *bzero,
           int *iminval, int *imaxval);
int fits_quantize_double (long row, double fdata[], long nx, long ny, int nullcheck,
         double in_null_value, float quantize_level,
           int dither_method, int idata[], double *bscale, double *bzero,
           int *iminval, int *imaxval);
int fits_rcomp(int a[], int nx, unsigned char *c, int clen,int nblock);
int fits_rcomp_short(short a[], int nx, unsigned char *c, int clen,int nblock);
int fits_rcomp_byte(signed char a[], int nx, unsigned char *c, int clen,int nblock);
int fits_rdecomp (unsigned char *c, int clen, unsigned int array[], int nx,
             int nblock);
int fits_rdecomp_short (unsigned char *c, int clen, unsigned short array[], int nx,
             int nblock);
int fits_rdecomp_byte (unsigned char *c, int clen, unsigned char array[], int nx,
             int nblock);
int pl_p2li (int *pxsrc, int xs, short *lldst, int npix);
int pl_l2pi (short *ll_src, int xs, int *px_dst, int npix);
int fits_init_randoms(void);
int fits_unset_compression_param( fitsfile *fptr, int *status);
int fits_unset_compression_request( fitsfile *fptr, int *status);
int fitsio_init_lock(void);

/* general driver routines */

int urltype2driver(char *urltype, int *driver);

int fits_register_driver( char *prefix,
	int (*init)(void),
	int (*fitsshutdown)(void),
	int (*setoptions)(int option),
	int (*getoptions)(int *options),
	int (*getversion)(int *version),
	int (*checkfile) (char *urltype, char *infile, char *outfile),
	int (*fitsopen)(char *filename, int rwmode, int *driverhandle),
	int (*fitscreate)(char *filename, int *driverhandle),
	int (*fitstruncate)(int driverhandle, LONGLONG filesize),
	int (*fitsclose)(int driverhandle),
	int (*fremove)(char *filename),
        int (*size)(int driverhandle, LONGLONG *sizex),
	int (*flush)(int driverhandle),
	int (*seek)(int driverhandle, LONGLONG offset),
	int (*fitsread) (int driverhandle, void *buffer, long nbytes),
	int (*fitswrite)(int driverhandle, void *buffer, long nbytes));

/* file driver I/O routines */

int file_init(void);
int file_setoptions(int options);
int file_getoptions(int *options);
int file_getversion(int *version);
int file_shutdown(void);
int file_checkfile(char *urltype, char *infile, char *outfile);
int file_open(char *filename, int rwmode, int *driverhandle);
int file_compress_open(char *filename, int rwmode, int *hdl);
int file_openfile(char *filename, int rwmode, FILE **diskfile);
int file_create(char *filename, int *driverhandle);
int file_truncate(int driverhandle, LONGLONG filesize);
int file_size(int driverhandle, LONGLONG *filesize);
int file_close(int driverhandle);
int file_remove(char *filename);
int file_flush(int driverhandle);
int file_seek(int driverhandle, LONGLONG offset);
int file_read (int driverhandle, void *buffer, long nbytes);
int file_write(int driverhandle, void *buffer, long nbytes);
int file_is_compressed(char *filename);

/* stream driver I/O routines */

int stream_open(char *filename, int rwmode, int *driverhandle);
int stream_create(char *filename, int *driverhandle);
int stream_size(int driverhandle, LONGLONG *filesize);
int stream_close(int driverhandle);
int stream_flush(int driverhandle);
int stream_seek(int driverhandle, LONGLONG offset);
int stream_read (int driverhandle, void *buffer, long nbytes);
int stream_write(int driverhandle, void *buffer, long nbytes);

/* memory driver I/O routines */

int mem_init(void);
int mem_setoptions(int options);
int mem_getoptions(int *options);
int mem_getversion(int *version);
int mem_shutdown(void);
int mem_create(char *filename, int *handle);
int mem_create_comp(char *filename, int *handle);
int mem_openmem(void **buffptr, size_t *buffsize, size_t deltasize,
                void *(*memrealloc)(void *p, size_t newsize), int *handle);
int mem_createmem(size_t memsize, int *handle);
int stdin_checkfile(char *urltype, char *infile, char *outfile);
int stdin_open(char *filename, int rwmode, int *handle);
int stdin2mem(int hd);
int stdin2file(int hd);
int stdout_close(int handle);
int mem_compress_openrw(char *filename, int rwmode, int *hdl);
int mem_compress_open(char *filename, int rwmode, int *hdl);
int mem_compress_stdin_open(char *filename, int rwmode, int *hdl);
int mem_iraf_open(char *filename, int rwmode, int *hdl);
int mem_rawfile_open(char *filename, int rwmode, int *hdl);
int mem_size(int handle, LONGLONG *filesize);
int mem_truncate(int handle, LONGLONG filesize);
int mem_close_free(int handle);
int mem_close_keep(int handle);
int mem_close_comp(int handle);
int mem_seek(int handle, LONGLONG offset);
int mem_read(int hdl, void *buffer, long nbytes);
int mem_write(int hdl, void *buffer, long nbytes);
int mem_uncompress2mem(char *filename, FILE *diskfile, int hdl);

int iraf2mem(char *filename, char **buffptr, size_t *buffsize, 
      size_t *filesize, int *status);

/* root driver I/O routines */

int root_init(void);
int root_setoptions(int options);
int root_getoptions(int *options);
int root_getversion(int *version);
int root_shutdown(void);
int root_open(char *filename, int rwmode, int *driverhandle);
int root_create(char *filename, int *driverhandle);
int root_close(int driverhandle);
int root_flush(int driverhandle);
int root_seek(int driverhandle, LONGLONG offset);
int root_read (int driverhandle, void *buffer, long nbytes);
int root_write(int driverhandle, void *buffer, long nbytes);
int root_size(int handle, LONGLONG *filesize);

/* http driver I/O routines */

int http_checkfile(char *urltype, char *infile, char *outfile);
int http_open(char *filename, int rwmode, int *driverhandle);
int http_file_open(char *filename, int rwmode, int *driverhandle);
int http_compress_open(char *filename, int rwmode, int *driverhandle);

/* https driver I/O routines */
int https_checkfile(char* urltype, char *infile, char *outfile);
int https_open(char *filename, int rwmode, int *driverhandle);
int https_file_open(char *filename, int rwmode, int *driverhandle);
void https_set_verbose(int flag);

/* ftp driver I/O routines */

int ftp_checkfile(char *urltype, char *infile, char *outfile);
int ftp_open(char *filename, int rwmode, int *driverhandle);
int ftp_file_open(char *filename, int rwmode, int *driverhandle);
int ftp_compress_open(char *filename, int rwmode, int *driverhandle);

int uncompress2mem(char *filename, FILE *diskfile,
             char **buffptr, size_t *buffsize,
             void *(*mem_realloc)(void *p, size_t newsize),
             size_t *filesize, int *status);

int uncompress2mem_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             char **buffptr,  
             size_t *buffsize,  
             void *(*mem_realloc)(void *p, size_t newsize), 
             size_t *filesize,  
             int *status);

int uncompress2file(char *filename, 
             FILE *indiskfile, 
             FILE *outdiskfile, 
             int *status);

int compress2mem_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             char **buffptr,  
             size_t *buffsize,  
             void *(*mem_realloc)(void *p, size_t newsize), 
             size_t *filesize,  
             int *status);

int compress2file_from_mem(                                                
             char *inmemptr,     
             size_t inmemsize, 
             FILE *outdiskfile, 
             size_t *filesize,   /* O - size of file, in bytes              */
             int *status);


#ifdef HAVE_GSIFTP
/* prototypes for gsiftp driver I/O routines */
#include "drvrgsiftp.h"
#endif

#ifdef HAVE_SHMEM_SERVICES
/* prototypes for shared memory driver I/O routines  */
#include "drvrsmem.h"
#endif

/* A hack for nonunix machines, which lack strcasecmp and strncasecmp */
/* these functions are in fitscore.c */
int fits_strcasecmp (const char *s1, const char *s2       );
int fits_strncasecmp(const char *s1, const char *s2, size_t n);

/* end of the entire "ifndef _FITSIO2_H" block */
#endif
