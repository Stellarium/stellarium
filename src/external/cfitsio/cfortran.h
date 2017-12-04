/* cfortran.h  4.4 */
/* http://www-zeus.desy.de/~burow/cfortran/                   */
/* Burkhard Burow  burow@desy.de                 1990 - 2002. */

#ifndef __CFORTRAN_LOADED
#define __CFORTRAN_LOADED

/* 
   THIS FILE IS PROPERTY OF BURKHARD BUROW. IF YOU ARE USING THIS FILE YOU
   SHOULD ALSO HAVE ACCESS TO CFORTRAN.DOC WHICH PROVIDES TERMS FOR USING,
   MODIFYING, COPYING AND DISTRIBUTING THE CFORTRAN.H PACKAGE.
*/

/* The following modifications were made by the authors of CFITSIO or by me. 
 * They are flagged below with CFITSIO, the author's initials, or KMCCARTY.
 * PDW = Peter Wilson
 * DM  = Doug Mink
 * LEB = Lee E Brotzman
 * MR  = Martin Reinecke
 * WDP = William D Pence
 * -- Kevin McCarty, for Debian (19 Dec. 2005) */

/*******
   Modifications:
      Oct 1997: Changed symbol name extname to appendus (PDW/HSTX)
                (Conflicted with a common variable name in FTOOLS)
      Nov 1997: If g77Fortran defined, also define f2cFortran (PDW/HSTX)
      Feb 1998: Let VMS see the NUM_ELEMS code. Lets programs treat
                single strings as vectors with single elements
      Nov 1999: If macintoxh defined, also define f2cfortran (for Mac OS-X)
      Apr 2000: If WIN32 defined, also define PowerStationFortran and
                VISUAL_CPLUSPLUS (Visual C++)
      Jun 2000: If __GNUC__ and linux defined, also define f2cFortran
                (linux/gcc environment detection)
      Apr 2002: If __CYGWIN__ is defined, also define f2cFortran
      Nov 2002: If __APPLE__ defined, also define f2cfortran (for Mac OS-X)

      Nov 2003: If __INTEL_COMPILER or INTEL_COMPILER defined, also define
                f2cFortran (KMCCARTY)
      Dec 2005: If f2cFortran is defined, enforce REAL functions in FORTRAN
                returning "double" in C.  This was one of the items on
		Burkhard's TODO list. (KMCCARTY)
      Dec 2005: Modifications to support 8-byte integers. (MR)
		USE AT YOUR OWN RISK!
      Feb 2006  Added logic to typedef the symbol 'LONGLONG' to an appropriate
                intrinsic 8-byte integer datatype  (WDP)
      Apr 2006: Modifications to support gfortran (and g77 with -fno-f2c flag)
                since by default it returns "float" for FORTRAN REAL function.
                (KMCCARTY)
      May 2008: Revert commenting out of "extern" in COMMON_BLOCK_DEF macro.
		Add braces around do-nothing ";" in 3 empty while blocks to
		get rid of compiler warnings.  Thanks to ROOT developers
		Jacek Holeczek and Rene Brun for these suggestions. (KMCCARTY)
      Dec 2008  Added typedef for LONGLONG to support Borland compiler (WDP)
 *******/

/* 
  Avoid symbols already used by compilers and system *.h:
  __ - OSF1 zukal06 V3.0 347 alpha, cc -c -std1 cfortest.c

*/

/* 
   Determine what 8-byte integer data type is available.
  'long long' is now supported by most compilers, but older
  MS Visual C++ compilers before V7.0 use '__int64' instead. (WDP)
*/

#ifndef LONGLONG_TYPE   /* this may have been previously defined */
#if defined(_MSC_VER)   /* Microsoft Visual C++ */

#if (_MSC_VER < 1300)   /* versions earlier than V7.0 do not have 'long long' */
    typedef __int64 LONGLONG;
#else                   /* newer versions do support 'long long' */
    typedef long long LONGLONG; 
#endif

#elif defined( __BORLANDC__)  /* (WDP) for the free Borland compiler, in particular */
    typedef __int64 LONGLONG;
#else
    typedef long long LONGLONG; 
#endif

#define LONGLONG_TYPE
#endif  

/* Microsoft Visual C++ requires alternate form for static inline. */
#if defined(_MSC_VER)   /* Microsoft Visual C++ */
#define STIN static __inline
#else
#define STIN static inline
#endif

/* First prepare for the C compiler. */

#ifndef ANSI_C_preprocessor /* i.e. user can override. */
#ifdef __CF__KnR
#define ANSI_C_preprocessor 0
#else
#ifdef __STDC__
#define ANSI_C_preprocessor 1
#else
#define _cfleft             1
#define _cfright 
#define _cfleft_cfright     0
#define ANSI_C_preprocessor _cfleft/**/_cfright
#endif
#endif
#endif

#if ANSI_C_preprocessor
#define _0(A,B)   A##B
#define  _(A,B)   _0(A,B)  /* see cat,xcat of K&R ANSI C p. 231 */
#define _2(A,B)   A##B     /* K&R ANSI C p.230: .. identifier is not replaced */
#define _3(A,B,C) _(A,_(B,C))
#else                      /* if it turns up again during rescanning.         */
#define  _(A,B)   A/**/B
#define _2(A,B)   A/**/B
#define _3(A,B,C) A/**/B/**/C
#endif

#if (defined(vax)&&defined(unix)) || (defined(__vax__)&&defined(__unix__))
#define VAXUltrix
#endif

#include <stdio.h>     /* NULL [in all machines stdio.h]                      */
#include <string.h>    /* strlen, memset, memcpy, memchr.                     */
#if !( defined(VAXUltrix) || defined(sun) || (defined(apollo)&&!defined(__STDCPP__)) )
#include <stdlib.h>    /* malloc,free                                         */
#else
#include <malloc.h>    /* Had to be removed for DomainOS h105 10.4 sys5.3 425t*/
#ifdef apollo
#define __CF__APOLLO67 /* __STDCPP__ is in Apollo 6.8 (i.e. ANSI) and onwards */
#endif
#endif

#if !defined(__GNUC__) && !defined(__sun) && (defined(sun)||defined(VAXUltrix)||defined(lynx))
#define __CF__KnR     /* Sun, LynxOS and VAX Ultrix cc only supports K&R.     */
                      /* Manually define __CF__KnR for HP if desired/required.*/
#endif                /*       i.e. We will generate Kernighan and Ritchie C. */
/* Note that you may define __CF__KnR before #include cfortran.h, in order to
generate K&R C instead of the default ANSI C. The differences are mainly in the
function prototypes and declarations. All machines, except the Apollo, work
with either style. The Apollo's argument promotion rules require ANSI or use of
the obsolete std_$call which we have not implemented here. Hence on the Apollo,
only C calling FORTRAN subroutines will work using K&R style.*/


/* Remainder of cfortran.h depends on the Fortran compiler. */

/* 11/29/2003 (KMCCARTY): add *INTEL_COMPILER symbols here */
/* 04/05/2006 (KMCCARTY): add gFortran symbol here */
#if defined(CLIPPERFortran) || defined(pgiFortran) || defined(__INTEL_COMPILER) || defined(INTEL_COMPILER) || defined(gFortran)
#define f2cFortran
#endif

/* VAX/VMS does not let us \-split long #if lines. */ 
/* Split #if into 2 because some HP-UX can't handle long #if */
#if !(defined(NAGf90Fortran)||defined(f2cFortran)||defined(hpuxFortran)||defined(apolloFortran)||defined(sunFortran)||defined(IBMR2Fortran)||defined(CRAYFortran))
#if !(defined(mipsFortran)||defined(DECFortran)||defined(vmsFortran)||defined(CONVEXFortran)||defined(PowerStationFortran)||defined(AbsoftUNIXFortran)||defined(AbsoftProFortran)||defined(SXFortran))
/* If no Fortran compiler is given, we choose one for the machines we know.   */
#if defined(lynx) || defined(VAXUltrix)
#define f2cFortran    /* Lynx:      Only support f2c at the moment.
                         VAXUltrix: f77 behaves like f2c.
                           Support f2c or f77 with gcc, vcc with f2c. 
                           f77 with vcc works, missing link magic for f77 I/O.*/
#endif
/* 04/13/00 DM (CFITSIO): Add these lines for NT */
/*   with PowerStationFortran and and Visual C++ */
#if defined(WIN32) && !defined(__CYGWIN__)
#define PowerStationFortran   
#define VISUAL_CPLUSPLUS
#endif
#if defined(g77Fortran)                        /* 11/03/97 PDW (CFITSIO) */
#define f2cFortran
#endif
#if        defined(__CYGWIN__)                 /* 04/11/02 LEB (CFITSIO) */
#define       f2cFortran 
#endif
#if        defined(__GNUC__) && defined(linux) /* 06/21/00 PDW (CFITSIO) */
#define       f2cFortran 
#endif
#if defined(macintosh)                         /* 11/1999 (CFITSIO) */
#define f2cFortran
#endif
#if defined(__APPLE__)                         /* 11/2002 (CFITSIO) */
#define f2cFortran
#endif
#if defined(__hpux)             /* 921107: Use __hpux instead of __hp9000s300 */
#define       hpuxFortran       /*         Should also allow hp9000s7/800 use.*/
#endif
#if       defined(apollo)
#define           apolloFortran /* __CF__APOLLO67 also defines some behavior. */
#endif
#if          defined(sun) || defined(__sun) 
#define              sunFortran
#endif
#if       defined(_IBMR2)
#define            IBMR2Fortran
#endif
#if        defined(_CRAY)
#define             CRAYFortran /*       _CRAYT3E also defines some behavior. */
#endif
#if        defined(_SX)
#define               SXFortran
#endif
#if         defined(mips) || defined(__mips)
#define             mipsFortran
#endif
#if          defined(vms) || defined(__vms)
#define              vmsFortran
#endif
#if      defined(__alpha) && defined(__unix__)
#define              DECFortran
#endif
#if   defined(__convex__)
#define           CONVEXFortran
#endif
#if   defined(VISUAL_CPLUSPLUS)
#define     PowerStationFortran
#endif
#endif /* ...Fortran */
#endif /* ...Fortran */

/* Split #if into 2 because some HP-UX can't handle long #if */
#if !(defined(NAGf90Fortran)||defined(f2cFortran)||defined(hpuxFortran)||defined(apolloFortran)||defined(sunFortran)||defined(IBMR2Fortran)||defined(CRAYFortran))
#if !(defined(mipsFortran)||defined(DECFortran)||defined(vmsFortran)||defined(CONVEXFortran)||defined(PowerStationFortran)||defined(AbsoftUNIXFortran)||defined(AbsoftProFortran)||defined(SXFortran))
/* If your compiler barfs on ' #error', replace # with the trigraph for #     */
 #error "cfortran.h:  Can't find your environment among:\
    - GNU gcc (g77) on Linux.                                            \
    - MIPS cc and f77 2.0. (e.g. Silicon Graphics, DECstations, ...)     \
    - IBM AIX XL C and FORTRAN Compiler/6000 Version 01.01.0000.0000     \
    - VAX   VMS CC 3.1 and FORTRAN 5.4.                                  \
    - Alpha VMS DEC C 1.3 and DEC FORTRAN 6.0.                           \
    - Alpha OSF DEC C and DEC Fortran for OSF/1 AXP Version 1.2          \
    - Apollo DomainOS 10.2 (sys5.3) with f77 10.7 and cc 6.7.            \
    - CRAY                                                               \
    - NEC SX-4 SUPER-UX                                                  \
    - CONVEX                                                             \
    - Sun                                                                \
    - PowerStation Fortran with Visual C++                               \
    - HP9000s300/s700/s800 Latest test with: HP-UX A.08.07 A 9000/730    \
    - LynxOS: cc or gcc with f2c.                                        \
    - VAXUltrix: vcc,cc or gcc with f2c. gcc or cc with f77.             \
    -            f77 with vcc works; but missing link magic for f77 I/O. \
    -            NO fort. None of gcc, cc or vcc generate required names.\
    - f2c/g77:   Use #define    f2cFortran, or cc -Df2cFortran           \
    - gfortran:  Use #define    gFortran,   or cc -DgFortran             \
                 (also necessary for g77 with -fno-f2c option)           \
    - NAG f90: Use #define NAGf90Fortran, or cc -DNAGf90Fortran          \
    - Absoft UNIX F77: Use #define AbsoftUNIXFortran or cc -DAbsoftUNIXFortran \
    - Absoft Pro Fortran: Use #define AbsoftProFortran \
    - Portland Group Fortran: Use #define pgiFortran \
    - Intel Fortran: Use #define INTEL_COMPILER"
/* Compiler must throw us out at this point! */
#endif
#endif


#if defined(VAXC) && !defined(__VAXC)
#define OLD_VAXC
#pragma nostandard                       /* Prevent %CC-I-PARAMNOTUSED.       */
#endif

/* Throughout cfortran.h we use: UN = Uppercase Name.  LN = Lowercase Name.   */

/* "extname" changed to "appendus" below (CFITSIO) */
#if defined(f2cFortran) || defined(NAGf90Fortran) || defined(DECFortran) || defined(mipsFortran) || defined(apolloFortran) || defined(sunFortran) || defined(CONVEXFortran) || defined(SXFortran) || defined(appendus)
#define CFC_(UN,LN)            _(LN,_)      /* Lowercase FORTRAN symbols.     */
#define orig_fcallsc(UN,LN)    CFC_(UN,LN)
#else 
#if defined(CRAYFortran) || defined(PowerStationFortran) || defined(AbsoftProFortran)
#ifdef _CRAY          /* (UN), not UN, circumvents CRAY preprocessor bug.     */
#define CFC_(UN,LN)            (UN)         /* Uppercase FORTRAN symbols.     */
#else                 /* At least VISUAL_CPLUSPLUS barfs on (UN), so need UN. */
#define CFC_(UN,LN)            UN           /* Uppercase FORTRAN symbols.     */
#endif
#define orig_fcallsc(UN,LN)    CFC_(UN,LN)  /* CRAY insists on arg.'s here.   */
#else  /* For following machines one may wish to change the fcallsc default.  */
#define CF_SAME_NAMESPACE
#ifdef vmsFortran
#define CFC_(UN,LN)            LN           /* Either case FORTRAN symbols.   */
     /* BUT we usually use UN for C macro to FORTRAN routines, so use LN here,*/
     /* because VAX/VMS doesn't do recursive macros.                          */
#define orig_fcallsc(UN,LN)    UN
#else      /* HP-UX without +ppu or IBMR2 without -qextname. NOT reccomended. */
#define CFC_(UN,LN)            LN           /* Lowercase FORTRAN symbols.     */
#define orig_fcallsc(UN,LN)    CFC_(UN,LN)
#endif /*  vmsFortran */
#endif /* CRAYFortran PowerStationFortran */
#endif /* ....Fortran */

#define fcallsc(UN,LN)               orig_fcallsc(UN,LN)
#define preface_fcallsc(P,p,UN,LN)   CFC_(_(P,UN),_(p,LN))
#define  append_fcallsc(P,p,UN,LN)   CFC_(_(UN,P),_(LN,p))

#define C_FUNCTION(UN,LN)            fcallsc(UN,LN)      
#define FORTRAN_FUNCTION(UN,LN)      CFC_(UN,LN)

#ifndef COMMON_BLOCK
#ifndef CONVEXFortran
#ifndef CLIPPERFortran
#if     !(defined(AbsoftUNIXFortran)||defined(AbsoftProFortran))
#define COMMON_BLOCK(UN,LN)          CFC_(UN,LN)
#else
#define COMMON_BLOCK(UN,LN)          _(_C,LN)
#endif  /* AbsoftUNIXFortran or AbsoftProFortran */
#else
#define COMMON_BLOCK(UN,LN)          _(LN,__)
#endif  /* CLIPPERFortran */
#else
#define COMMON_BLOCK(UN,LN)          _3(_,LN,_)
#endif  /* CONVEXFortran */
#endif  /* COMMON_BLOCK */

#ifndef DOUBLE_PRECISION
#if defined(CRAYFortran) && !defined(_CRAYT3E)
#define DOUBLE_PRECISION long double
#else
#define DOUBLE_PRECISION double
#endif
#endif

#ifndef FORTRAN_REAL
#if defined(CRAYFortran) &&  defined(_CRAYT3E)
#define FORTRAN_REAL double
#else
#define FORTRAN_REAL float
#endif
#endif

#ifdef CRAYFortran
#ifdef _CRAY
#include <fortran.h>
#else
#include "fortran.h"  /* i.e. if crosscompiling assume user has file. */
#endif
#define FLOATVVVVVVV_cfPP (FORTRAN_REAL *)   /* Used for C calls FORTRAN.     */
/* CRAY's double==float but CRAY says pointers to doubles and floats are diff.*/
#define VOIDP  (void *)  /* When FORTRAN calls C, we don't know if C routine 
                            arg.'s have been declared float *, or double *.   */
#else
#define FLOATVVVVVVV_cfPP
#define VOIDP
#endif

#ifdef vmsFortran
#if    defined(vms) || defined(__vms)
#include <descrip.h>
#else
#include "descrip.h"  /* i.e. if crosscompiling assume user has file. */
#endif
#endif

#ifdef sunFortran
#if defined(sun) || defined(__sun)
#include <math.h>     /* Sun's FLOATFUNCTIONTYPE, ASSIGNFLOAT, RETURNFLOAT.  */
#else
#include "math.h"     /* i.e. if crosscompiling assume user has file. */
#endif
/* At least starting with the default C compiler SC3.0.1 of SunOS 5.3,
 * FLOATFUNCTIONTYPE, ASSIGNFLOAT, RETURNFLOAT are not required and not in
 * <math.h>, since sun C no longer promotes C float return values to doubles.
 * Therefore, only use them if defined.
 * Even if gcc is being used, assume that it exhibits the Sun C compiler
 * behavior in order to be able to use *.o from the Sun C compiler.
 * i.e. If FLOATFUNCTIONTYPE, etc. are in math.h, they required by gcc.
 */
#endif

#ifndef apolloFortran
#define COMMON_BLOCK_DEF(DEFINITION, NAME) extern DEFINITION NAME
#define CF_NULL_PROTO
#else                                         /* HP doesn't understand #elif. */
/* Without ANSI prototyping, Apollo promotes float functions to double.    */
/* Note that VAX/VMS, IBM, Mips choke on 'type function(...);' prototypes. */
#define CF_NULL_PROTO ...
#ifndef __CF__APOLLO67
#define COMMON_BLOCK_DEF(DEFINITION, NAME) \
 DEFINITION NAME __attribute((__section(NAME)))
#else
#define COMMON_BLOCK_DEF(DEFINITION, NAME) \
 DEFINITION NAME #attribute[section(NAME)]
#endif
#endif

#ifdef __cplusplus
#undef  CF_NULL_PROTO
#define CF_NULL_PROTO  ...
#endif


#ifndef USE_NEW_DELETE
#ifdef __cplusplus
#define USE_NEW_DELETE 1
#else
#define USE_NEW_DELETE 0
#endif
#endif
#if USE_NEW_DELETE
#define _cf_malloc(N) new char[N]
#define _cf_free(P)   delete[] P
#else
#define _cf_malloc(N) (char *)malloc(N)
#define _cf_free(P)   free(P)
#endif

#ifdef mipsFortran
#define CF_DECLARE_GETARG         int f77argc; char **f77argv
#define CF_SET_GETARG(ARGC,ARGV)  f77argc = ARGC; f77argv = ARGV
#else
#define CF_DECLARE_GETARG
#define CF_SET_GETARG(ARGC,ARGV)
#endif

#ifdef OLD_VAXC                          /* Allow %CC-I-PARAMNOTUSED.         */
#pragma standard                         
#endif

#define AcfCOMMA ,
#define AcfCOLON ;

/*-------------------------------------------------------------------------*/

/*               UTILITIES USED WITHIN CFORTRAN.H                          */

#define _cfMIN(A,B) (A<B?A:B)

/* 970211 - XIX.145:
   firstindexlength  - better name is all_but_last_index_lengths
   secondindexlength - better name is         last_index_length
 */
#define  firstindexlength(A) (sizeof(A[0])==1 ? 1 : (sizeof(A) / sizeof(A[0])) )
#define secondindexlength(A) (sizeof(A[0])==1 ?      sizeof(A) : sizeof(A[0])  )

/* Behavior of FORTRAN LOGICAL. All machines' LOGICAL is same size as C's int.
Conversion is automatic except for arrays which require F2CLOGICALV/C2FLOGICALV.
f2c, MIPS f77 [DECstation, SGI], VAX Ultrix f77,
HP-UX f77                                        : as in C.
VAX/VMS FORTRAN, VAX Ultrix fort,
Absoft Unix Fortran, IBM RS/6000 xlf             : LS Bit = 0/1 = TRUE/FALSE.
Apollo                                           : neg.   = TRUE, else FALSE. 
[Apollo accepts -1 as TRUE for function values, but NOT all other neg. values.]
[DECFortran for Ultrix RISC is also called f77 but is the same as VAX/VMS.]   
[MIPS f77 treats .eqv./.neqv. as .eq./.ne. and hence requires LOGICAL_STRICT.]*/

#if defined(NAGf90Fortran) || defined(f2cFortran) || defined(mipsFortran) || defined(PowerStationFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran) || defined(AbsoftProFortran) || defined(SXFortran)
/* SX/PowerStationFortran have 0 and 1 defined, others are neither T nor F.   */
/* hpuxFortran800 has 0 and 0x01000000 defined. Others are unknown.           */
#define LOGICAL_STRICT      /* Other Fortran have .eqv./.neqv. == .eq./.ne.   */
#endif

#define C2FLOGICALV(A,I) \
 do {int __i; for(__i=0;__i<I;__i++) A[__i]=C2FLOGICAL(A[__i]); } while (0)
#define F2CLOGICALV(A,I) \
 do {int __i; for(__i=0;__i<I;__i++) A[__i]=F2CLOGICAL(A[__i]); } while (0)

#if defined(apolloFortran)
#define C2FLOGICAL(L) ((L)?-1:(L)&~((unsigned)1<<sizeof(int)*8-1))
#define F2CLOGICAL(L) ((L)<0?(L):0) 
#else
#if defined(CRAYFortran)
#define C2FLOGICAL(L) _btol(L)
#define F2CLOGICAL(L) _ltob(&(L))     /* Strangely _ltob() expects a pointer. */
#else
#if defined(IBMR2Fortran) || defined(vmsFortran) || defined(DECFortran) || defined(AbsoftUNIXFortran)
/* How come no AbsoftProFortran ? */
#define C2FLOGICAL(L) ((L)?(L)|1:(L)&~(int)1)
#define F2CLOGICAL(L) ((L)&1?(L):0)
#else
#if defined(CONVEXFortran)
#define C2FLOGICAL(L) ((L) ? ~0 : 0 )
#define F2CLOGICAL(L) (L)
#else   /* others evaluate LOGICALs as for C. */
#define C2FLOGICAL(L) (L)
#define F2CLOGICAL(L) (L)
#ifndef LOGICAL_STRICT
#undef  C2FLOGICALV
#undef  F2CLOGICALV
#define C2FLOGICALV(A,I)
#define F2CLOGICALV(A,I)
#endif  /* LOGICAL_STRICT                     */
#endif  /* CONVEXFortran || All Others        */
#endif  /* IBMR2Fortran vmsFortran DECFortran AbsoftUNIXFortran */
#endif  /* CRAYFortran                        */
#endif  /* apolloFortran                      */

/* 970514 - In addition to CRAY, there may be other machines
            for which LOGICAL_STRICT makes no sense. */
#if defined(LOGICAL_STRICT) && !defined(CRAYFortran)
/* Force C2FLOGICAL to generate only the values for either .TRUE. or .FALSE.
   SX/PowerStationFortran only have 0 and 1 defined.
   Elsewhere, only needed if you want to do:
     logical lvariable
     if (lvariable .eq.  .true.) then       ! (1)
   instead of
     if (lvariable .eqv. .true.) then       ! (2)
   - (1) may not even be FORTRAN/77 and that Apollo's f77 and IBM's xlf
     refuse to compile (1), so you are probably well advised to stay away from 
     (1) and from LOGICAL_STRICT.
   - You pay a (slight) performance penalty for using LOGICAL_STRICT. */
#undef  C2FLOGICAL
#ifdef hpuxFortran800
#define C2FLOGICAL(L) ((L)?0x01000000:0)
#else
#if defined(apolloFortran) || defined(vmsFortran) || defined(DECFortran)
#define C2FLOGICAL(L) ((L)?-1:0) /* These machines use -1/0 for .true./.false.*/
#else
#define C2FLOGICAL(L) ((L)? 1:0) /* All others     use +1/0 for .true./.false.*/
#endif
#endif
#endif /* LOGICAL_STRICT */

/* Convert a vector of C strings into FORTRAN strings. */
#ifndef __CF__KnR
static char *c2fstrv(char* cstr, char *fstr, int elem_len, int sizeofcstr)
#else
static char *c2fstrv(      cstr,       fstr,     elem_len,     sizeofcstr)
                     char* cstr; char *fstr; int elem_len; int sizeofcstr;
#endif
{ int i,j;
/* elem_len includes \0 for C strings. Fortran strings don't have term. \0.
   Useful size of string must be the same in both languages. */
for (i=0; i<sizeofcstr/elem_len; i++) {
  for (j=1; j<elem_len && *cstr; j++) *fstr++ = *cstr++;
  cstr += 1+elem_len-j;
  for (; j<elem_len; j++) *fstr++ = ' ';
} /* 95109 - Seems to be returning the original fstr. */
return fstr-sizeofcstr+sizeofcstr/elem_len; }

/* Convert a vector of FORTRAN strings into C strings. */
#ifndef __CF__KnR
static char *f2cstrv(char *fstr, char* cstr, int elem_len, int sizeofcstr)
#else
static char *f2cstrv(      fstr,       cstr,     elem_len,     sizeofcstr)
                     char *fstr; char* cstr; int elem_len; int sizeofcstr; 
#endif
{ int i,j;
/* elem_len includes \0 for C strings. Fortran strings don't have term. \0.
   Useful size of string must be the same in both languages. */
cstr += sizeofcstr;
fstr += sizeofcstr - sizeofcstr/elem_len;
for (i=0; i<sizeofcstr/elem_len; i++) {
  *--cstr = '\0';
  for (j=1; j<elem_len; j++) *--cstr = *--fstr;
} return cstr; }

/* kill the trailing char t's in string s. */
#ifndef __CF__KnR
static char *kill_trailing(char *s, char t)
#else
static char *kill_trailing(      s,      t) char *s; char t;
#endif
{char *e; 
e = s + strlen(s);
if (e>s) {                           /* Need this to handle NULL string.*/
  while (e>s && *--e==t) {;}         /* Don't follow t's past beginning. */
  e[*e==t?0:1] = '\0';               /* Handle s[0]=t correctly.       */
} return s; }

/* kill_trailingn(s,t,e) will kill the trailing t's in string s. e normally 
points to the terminating '\0' of s, but may actually point to anywhere in s.
s's new '\0' will be placed at e or earlier in order to remove any trailing t's.
If e<s string s is left unchanged. */ 
#ifndef __CF__KnR
static char *kill_trailingn(char *s, char t, char *e)
#else
static char *kill_trailingn(      s,      t,       e) char *s; char t; char *e;
#endif
{ 
if (e==s) *e = '\0';                 /* Kill the string makes sense here.*/
else if (e>s) {                      /* Watch out for neg. length string.*/
  while (e>s && *--e==t){;}          /* Don't follow t's past beginning. */
  e[*e==t?0:1] = '\0';               /* Handle s[0]=t correctly.       */
} return s; }

/* Note the following assumes that any element which has t's to be chopped off,
does indeed fill the entire element. */
#ifndef __CF__KnR
static char *vkill_trailing(char* cstr, int elem_len, int sizeofcstr, char t)
#else
static char *vkill_trailing(      cstr,     elem_len,     sizeofcstr,      t)
                            char* cstr; int elem_len; int sizeofcstr; char t;
#endif
{ int i;
for (i=0; i<sizeofcstr/elem_len; i++) /* elem_len includes \0 for C strings. */
  kill_trailingn(cstr+elem_len*i,t,cstr+elem_len*(i+1)-1);
return cstr; }

#ifdef vmsFortran
typedef struct dsc$descriptor_s fstring;
#define DSC$DESCRIPTOR_A(DIMCT)  		                               \
struct {                                                                       \
  unsigned short dsc$w_length;	        unsigned char	 dsc$b_dtype;	       \
  unsigned char	 dsc$b_class;	                 char	*dsc$a_pointer;	       \
           char	 dsc$b_scale;	        unsigned char	 dsc$b_digits;         \
  struct {                                                                     \
    unsigned		       : 3;	  unsigned dsc$v_fl_binscale : 1;      \
    unsigned dsc$v_fl_redim    : 1;       unsigned dsc$v_fl_column   : 1;      \
    unsigned dsc$v_fl_coeff    : 1;       unsigned dsc$v_fl_bounds   : 1;      \
  } dsc$b_aflags;	                                                       \
  unsigned char	 dsc$b_dimct;	        unsigned long	 dsc$l_arsize;	       \
           char	*dsc$a_a0;	                 long	 dsc$l_m [DIMCT];      \
  struct {                                                                     \
    long dsc$l_l;                         long dsc$l_u;                        \
  } dsc$bounds [DIMCT];                                                        \
}
typedef DSC$DESCRIPTOR_A(1) fstringvector;
/*typedef DSC$DESCRIPTOR_A(2) fstringarrarr;
  typedef DSC$DESCRIPTOR_A(3) fstringarrarrarr;*/
#define initfstr(F,C,ELEMNO,ELEMLEN)                                           \
( (F).dsc$l_arsize=  ( (F).dsc$w_length                        =(ELEMLEN) )    \
                    *( (F).dsc$l_m[0]=(F).dsc$bounds[0].dsc$l_u=(ELEMNO)  ),   \
  (F).dsc$a_a0    =  ( (F).dsc$a_pointer=(C) ) - (F).dsc$w_length          ,(F))

#endif      /* PDW: 2/10/98 (CFITSIO) -- Let VMS see NUM_ELEMS definitions */
#define _NUM_ELEMS      -1
#define _NUM_ELEM_ARG   -2
#define NUM_ELEMS(A)    A,_NUM_ELEMS
#define NUM_ELEM_ARG(B) *_2(A,B),_NUM_ELEM_ARG
#define TERM_CHARS(A,B) A,B
#ifndef __CF__KnR
STIN int num_elem(char *strv, unsigned elem_len, int term_char, int num_term)
#else
STIN int num_elem(      strv,          elem_len,     term_char,     num_term)
                    char *strv; unsigned elem_len; int term_char; int num_term;
#endif
/* elem_len is the number of characters in each element of strv, the FORTRAN
vector of strings. The last element of the vector must begin with at least
num_term term_char characters, so that this routine can determine how 
many elements are in the vector. */
{
unsigned num,i;
if (num_term == _NUM_ELEMS || num_term == _NUM_ELEM_ARG) 
  return term_char;
if (num_term <=0) num_term = (int)elem_len;
for (num=0; ; num++) {
  for (i=0; i<(unsigned)num_term && *strv==term_char; i++,strv++){;}
  if (i==(unsigned)num_term) break;
  else strv += elem_len-i;
}
if (0) {  /* to prevent not used warnings in gcc (added by ROOT) */
   c2fstrv(0, 0, 0, 0); f2cstrv(0, 0, 0, 0); kill_trailing(0, 0);
   vkill_trailing(0, 0, 0, 0); num_elem(0, 0, 0, 0);
}
return (int)num;
}
/* #endif removed 2/10/98 (CFITSIO) */

/*-------------------------------------------------------------------------*/

/*           UTILITIES FOR C TO USE STRINGS IN FORTRAN COMMON BLOCKS       */

/* C string TO Fortran Common Block STRing. */
/* DIM is the number of DIMensions of the array in terms of strings, not
   characters. e.g. char a[12] has DIM = 0, char a[12][4] has DIM = 1, etc. */
#define C2FCBSTR(CSTR,FSTR,DIM)                                                \
 c2fstrv((char *)CSTR, (char *)FSTR, sizeof(FSTR)/cfelementsof(FSTR,DIM)+1,    \
         sizeof(FSTR)+cfelementsof(FSTR,DIM))

/* Fortran Common Block string TO C STRing. */
#define FCB2CSTR(FSTR,CSTR,DIM)                                                \
 vkill_trailing(f2cstrv((char *)FSTR, (char *)CSTR,                            \
                        sizeof(FSTR)/cfelementsof(FSTR,DIM)+1,                 \
                        sizeof(FSTR)+cfelementsof(FSTR,DIM)),                  \
                sizeof(FSTR)/cfelementsof(FSTR,DIM)+1,                         \
                sizeof(FSTR)+cfelementsof(FSTR,DIM), ' ')

#define cfDEREFERENCE0
#define cfDEREFERENCE1 *
#define cfDEREFERENCE2 **
#define cfDEREFERENCE3 ***
#define cfDEREFERENCE4 ****
#define cfDEREFERENCE5 *****
#define cfelementsof(A,D) (sizeof(A)/sizeof(_(cfDEREFERENCE,D)(A)))

/*-------------------------------------------------------------------------*/

/*               UTILITIES FOR C TO CALL FORTRAN SUBROUTINES               */

/* Define lookup tables for how to handle the various types of variables.  */

#ifdef OLD_VAXC                                /* Prevent %CC-I-PARAMNOTUSED. */
#pragma nostandard
#endif

#define ZTRINGV_NUM(I)       I
#define ZTRINGV_ARGFP(I) (*(_2(A,I))) /* Undocumented. For PINT, etc. */
#define ZTRINGV_ARGF(I) _2(A,I)
#ifdef CFSUBASFUN
#define ZTRINGV_ARGS(I) ZTRINGV_ARGF(I)
#else
#define ZTRINGV_ARGS(I) _2(B,I)
#endif

#define    PBYTE_cfVP(A,B) PINT_cfVP(A,B)
#define  PDOUBLE_cfVP(A,B)
#define   PFLOAT_cfVP(A,B)
#ifdef ZTRINGV_ARGS_allows_Pvariables
/* This allows Pvariables for ARGS. ARGF machinery is above ARGFP.
 * B is not needed because the variable may be changed by the Fortran routine,
 * but because B is the only way to access an arbitrary macro argument.       */
#define     PINT_cfVP(A,B) int  B = (int)A;              /* For ZSTRINGV_ARGS */
#else
#define     PINT_cfVP(A,B)
#endif
#define PLOGICAL_cfVP(A,B) int *B;      /* Returning LOGICAL in FUNn and SUBn */
#define    PLONG_cfVP(A,B) PINT_cfVP(A,B)
#define   PSHORT_cfVP(A,B) PINT_cfVP(A,B)

#define        VCF_INT_S(T,A,B) _(T,VVVVVVV_cfTYPE) B = A;
#define        VCF_INT_F(T,A,B) _(T,_cfVCF)(A,B)
/* _cfVCF table is directly mapped to _cfCCC table. */
#define     BYTE_cfVCF(A,B)
#define   DOUBLE_cfVCF(A,B)
#if !defined(__CF__KnR)
#define    FLOAT_cfVCF(A,B)
#else
#define    FLOAT_cfVCF(A,B) FORTRAN_REAL B = A;
#endif
#define      INT_cfVCF(A,B)
#define  LOGICAL_cfVCF(A,B)
#define     LONG_cfVCF(A,B)
#define    SHORT_cfVCF(A,B)

/* 980416
   Cast (void (*)(CF_NULL_PROTO)) causes SunOS CC 4.2 occasionally to barf,
   while the following equivalent typedef is fine.
   For consistency use the typedef on all machines.
 */
typedef void (*cfCAST_FUNCTION)(CF_NULL_PROTO);

#define VCF(TN,I)       _Icf4(4,V,TN,_(A,I),_(B,I),F)
#define VVCF(TN,AI,BI)  _Icf4(4,V,TN,AI,BI,S)
#define        INT_cfV(T,A,B,F) _(VCF_INT_,F)(T,A,B)
#define       INTV_cfV(T,A,B,F)
#define      INTVV_cfV(T,A,B,F)
#define     INTVVV_cfV(T,A,B,F)
#define    INTVVVV_cfV(T,A,B,F)
#define   INTVVVVV_cfV(T,A,B,F)
#define  INTVVVVVV_cfV(T,A,B,F)
#define INTVVVVVVV_cfV(T,A,B,F)
#define PINT_cfV(      T,A,B,F) _(T,_cfVP)(A,B)
#define PVOID_cfV(     T,A,B,F)
#if defined(apolloFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran) || defined(AbsoftProFortran)
#define    ROUTINE_cfV(T,A,B,F) void (*B)(CF_NULL_PROTO) = (cfCAST_FUNCTION)A;
#else
#define    ROUTINE_cfV(T,A,B,F)
#endif
#define     SIMPLE_cfV(T,A,B,F)
#ifdef vmsFortran
#define     STRING_cfV(T,A,B,F) static struct {fstring f; unsigned clen;} B =  \
                                       {{0,DSC$K_DTYPE_T,DSC$K_CLASS_S,NULL},0};
#define    PSTRING_cfV(T,A,B,F) static fstring B={0,DSC$K_DTYPE_T,DSC$K_CLASS_S,NULL};
#define    STRINGV_cfV(T,A,B,F) static fstringvector B =                       \
  {sizeof(A),DSC$K_DTYPE_T,DSC$K_CLASS_A,NULL,0,0,{0,0,1,1,1},1,0,NULL,0,{1,0}};
#define   PSTRINGV_cfV(T,A,B,F) static fstringvector B =                       \
          {0,DSC$K_DTYPE_T,DSC$K_CLASS_A,NULL,0,0,{0,0,1,1,1},1,0,NULL,0,{1,0}};
#else
#define     STRING_cfV(T,A,B,F) struct {unsigned int clen, flen; char *nombre;} B;
#define    STRINGV_cfV(T,A,B,F) struct {char *s, *fs; unsigned flen; char *nombre;} B;
#define    PSTRING_cfV(T,A,B,F) int     B;
#define   PSTRINGV_cfV(T,A,B,F) struct{char *fs; unsigned int sizeofA,flen;}B;
#endif
#define    ZTRINGV_cfV(T,A,B,F)  STRINGV_cfV(T,A,B,F)
#define   PZTRINGV_cfV(T,A,B,F) PSTRINGV_cfV(T,A,B,F)

/* Note that the actions of the A table were performed inside the AA table.
   VAX Ultrix vcc, and HP-UX cc, didn't evaluate arguments to functions left to
   right, so we had to split the original table into the current robust two. */
#define ACF(NAME,TN,AI,I)      _(TN,_cfSTR)(4,A,NAME,I,AI,_(B,I),0)
#define   DEFAULT_cfA(M,I,A,B)
#define   LOGICAL_cfA(M,I,A,B) B=C2FLOGICAL(B);
#define  PLOGICAL_cfA(M,I,A,B) A=C2FLOGICAL(A);
#define    STRING_cfA(M,I,A,B)  STRING_cfC(M,I,A,B,sizeof(A))
#define   PSTRING_cfA(M,I,A,B) PSTRING_cfC(M,I,A,B,sizeof(A))
#ifdef vmsFortran
#define  AATRINGV_cfA(    A,B, sA,filA,silA)                                   \
 initfstr(B,_cf_malloc((sA)-(filA)),(filA),(silA)-1),                          \
          c2fstrv(A,B.dsc$a_pointer,(silA),(sA));
#define APATRINGV_cfA(    A,B, sA,filA,silA)                                   \
 initfstr(B,A,(filA),(silA)-1),c2fstrv(A,A,(silA),(sA));
#else
#define  AATRINGV_cfA(    A,B, sA,filA,silA)                                   \
     (B.s=_cf_malloc((sA)-(filA)),B.fs=c2fstrv(A,B.s,(B.flen=(silA)-1)+1,(sA)));
#define APATRINGV_cfA(    A,B, sA,filA,silA)                                   \
 B.fs=c2fstrv(A,A,(B.flen=(silA)-1)+1,B.sizeofA=(sA));
#endif
#define   STRINGV_cfA(M,I,A,B)                                                 \
    AATRINGV_cfA((char *)A,B,sizeof(A),firstindexlength(A),secondindexlength(A))
#define  PSTRINGV_cfA(M,I,A,B)                                                 \
   APATRINGV_cfA((char *)A,B,sizeof(A),firstindexlength(A),secondindexlength(A))
#define   ZTRINGV_cfA(M,I,A,B)  AATRINGV_cfA( (char *)A,B,                     \
                    (_3(M,_ELEMS_,I))*(( _3(M,_ELEMLEN_,I))+1),                \
                              (_3(M,_ELEMS_,I)),(_3(M,_ELEMLEN_,I))+1)
#define  PZTRINGV_cfA(M,I,A,B) APATRINGV_cfA( (char *)A,B,                     \
                    (_3(M,_ELEMS_,I))*(( _3(M,_ELEMLEN_,I))+1),                \
                              (_3(M,_ELEMS_,I)),(_3(M,_ELEMLEN_,I))+1)

#define    PBYTE_cfAAP(A,B) &A
#define  PDOUBLE_cfAAP(A,B) &A
#define   PFLOAT_cfAAP(A,B) FLOATVVVVVVV_cfPP &A
#define     PINT_cfAAP(A,B) &A
#define PLOGICAL_cfAAP(A,B) B= &A         /* B used to keep a common W table. */
#define    PLONG_cfAAP(A,B) &A
#define   PSHORT_cfAAP(A,B) &A

#define AACF(TN,AI,I,C) _SEP_(TN,C,cfCOMMA) _Icf(3,AA,TN,AI,_(B,I))
#define        INT_cfAA(T,A,B) &B
#define       INTV_cfAA(T,A,B) _(T,VVVVVV_cfPP) A
#define      INTVV_cfAA(T,A,B) _(T,VVVVV_cfPP)  A[0]
#define     INTVVV_cfAA(T,A,B) _(T,VVVV_cfPP)   A[0][0]
#define    INTVVVV_cfAA(T,A,B) _(T,VVV_cfPP)    A[0][0][0]
#define   INTVVVVV_cfAA(T,A,B) _(T,VV_cfPP)     A[0][0][0][0]
#define  INTVVVVVV_cfAA(T,A,B) _(T,V_cfPP)      A[0][0][0][0][0]
#define INTVVVVVVV_cfAA(T,A,B) _(T,_cfPP)       A[0][0][0][0][0][0]
#define       PINT_cfAA(T,A,B) _(T,_cfAAP)(A,B)
#define      PVOID_cfAA(T,A,B) (void *) A
#if defined(apolloFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran)
#define    ROUTINE_cfAA(T,A,B) &B
#else
#define    ROUTINE_cfAA(T,A,B) (cfCAST_FUNCTION)A
#endif
#define     STRING_cfAA(T,A,B)  STRING_cfCC(T,A,B)
#define    PSTRING_cfAA(T,A,B) PSTRING_cfCC(T,A,B)
#ifdef vmsFortran
#define    STRINGV_cfAA(T,A,B) &B
#else
#ifdef CRAYFortran
#define    STRINGV_cfAA(T,A,B) _cptofcd(B.fs,B.flen)
#else
#define    STRINGV_cfAA(T,A,B) B.fs
#endif
#endif
#define   PSTRINGV_cfAA(T,A,B) STRINGV_cfAA(T,A,B)
#define    ZTRINGV_cfAA(T,A,B) STRINGV_cfAA(T,A,B)
#define   PZTRINGV_cfAA(T,A,B) STRINGV_cfAA(T,A,B)

#if defined(vmsFortran) || defined(CRAYFortran)
#define JCF(TN,I)
#define KCF(TN,I)
#else
#define JCF(TN,I)    _(TN,_cfSTR)(1,J,_(B,I), 0,0,0,0)
#if defined(AbsoftUNIXFortran)
#define  DEFAULT_cfJ(B) ,0
#else
#define  DEFAULT_cfJ(B)
#endif
#define  LOGICAL_cfJ(B) DEFAULT_cfJ(B)
#define PLOGICAL_cfJ(B) DEFAULT_cfJ(B)
#define   STRING_cfJ(B) ,B.flen
#define  PSTRING_cfJ(B) ,B
#define  STRINGV_cfJ(B) STRING_cfJ(B)
#define PSTRINGV_cfJ(B) STRING_cfJ(B)
#define  ZTRINGV_cfJ(B) STRING_cfJ(B)
#define PZTRINGV_cfJ(B) STRING_cfJ(B)

/* KCF is identical to DCF, except that KCF ZTRING is not empty. */
#define KCF(TN,I)    _(TN,_cfSTR)(1,KK,_(B,I), 0,0,0,0)
#if defined(AbsoftUNIXFortran)
#define  DEFAULT_cfKK(B) , unsigned B
#else
#define  DEFAULT_cfKK(B)
#endif
#define  LOGICAL_cfKK(B) DEFAULT_cfKK(B)
#define PLOGICAL_cfKK(B) DEFAULT_cfKK(B)
#define   STRING_cfKK(B) , unsigned B
#define  PSTRING_cfKK(B) STRING_cfKK(B)
#define  STRINGV_cfKK(B) STRING_cfKK(B)
#define PSTRINGV_cfKK(B) STRING_cfKK(B)
#define  ZTRINGV_cfKK(B) STRING_cfKK(B)
#define PZTRINGV_cfKK(B) STRING_cfKK(B)
#endif

#define WCF(TN,AN,I)      _(TN,_cfSTR)(2,W,AN,_(B,I), 0,0,0)
#define  DEFAULT_cfW(A,B)
#define  LOGICAL_cfW(A,B)
#define PLOGICAL_cfW(A,B) *B=F2CLOGICAL(*B);
#define   STRING_cfW(A,B) (B.nombre=A,B.nombre[B.clen]!='\0'?B.nombre[B.clen]='\0':0); /* A?="constnt"*/
#define  PSTRING_cfW(A,B) kill_trailing(A,' ');
#ifdef vmsFortran
#define  STRINGV_cfW(A,B) _cf_free(B.dsc$a_pointer);
#define PSTRINGV_cfW(A,B)                                                      \
  vkill_trailing(f2cstrv((char*)A, (char*)A,                                   \
                           B.dsc$w_length+1, B.dsc$l_arsize+B.dsc$l_m[0]),     \
                   B.dsc$w_length+1, B.dsc$l_arsize+B.dsc$l_m[0], ' ');
#else
#define  STRINGV_cfW(A,B) _cf_free(B.s);
#define PSTRINGV_cfW(A,B) vkill_trailing(                                      \
         f2cstrv((char*)A,(char*)A,B.flen+1,B.sizeofA), B.flen+1,B.sizeofA,' ');
#endif
#define  ZTRINGV_cfW(A,B)      STRINGV_cfW(A,B)
#define PZTRINGV_cfW(A,B)     PSTRINGV_cfW(A,B)

#define   NCF(TN,I,C)       _SEP_(TN,C,cfCOMMA) _Icf(2,N,TN,_(A,I),0) 
#define  NNCF(TN,I,C)        UUCF(TN,I,C)
#define NNNCF(TN,I,C)       _SEP_(TN,C,cfCOLON) _Icf(2,N,TN,_(A,I),0) 
#define        INT_cfN(T,A) _(T,VVVVVVV_cfTYPE) * A
#define       INTV_cfN(T,A) _(T,VVVVVV_cfTYPE)  * A
#define      INTVV_cfN(T,A) _(T,VVVVV_cfTYPE)   * A
#define     INTVVV_cfN(T,A) _(T,VVVV_cfTYPE)    * A
#define    INTVVVV_cfN(T,A) _(T,VVV_cfTYPE)     * A
#define   INTVVVVV_cfN(T,A) _(T,VV_cfTYPE)      * A
#define  INTVVVVVV_cfN(T,A) _(T,V_cfTYPE)       * A
#define INTVVVVVVV_cfN(T,A) _(T,_cfTYPE)        * A
#define       PINT_cfN(T,A) _(T,_cfTYPE)        * A
#define      PVOID_cfN(T,A) void *                A
#if defined(apolloFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran)
#define    ROUTINE_cfN(T,A) void (**A)(CF_NULL_PROTO)
#else
#define    ROUTINE_cfN(T,A) void ( *A)(CF_NULL_PROTO)
#endif
#ifdef vmsFortran
#define     STRING_cfN(T,A) fstring *             A
#define    STRINGV_cfN(T,A) fstringvector *       A
#else
#ifdef CRAYFortran
#define     STRING_cfN(T,A) _fcd                  A
#define    STRINGV_cfN(T,A) _fcd                  A
#else
#define     STRING_cfN(T,A) char *                A
#define    STRINGV_cfN(T,A) char *                A
#endif
#endif
#define    PSTRING_cfN(T,A)   STRING_cfN(T,A) /* CRAY insists on arg.'s here. */
#define   PNSTRING_cfN(T,A)   STRING_cfN(T,A) /* CRAY insists on arg.'s here. */
#define   PPSTRING_cfN(T,A)   STRING_cfN(T,A) /* CRAY insists on arg.'s here. */
#define   PSTRINGV_cfN(T,A)  STRINGV_cfN(T,A)
#define    ZTRINGV_cfN(T,A)  STRINGV_cfN(T,A)
#define   PZTRINGV_cfN(T,A) PSTRINGV_cfN(T,A)


/* Apollo 6.7, CRAY, old Sun, VAX/Ultrix vcc/cc and new ultrix
   can't hack more than 31 arg's.
   e.g. ultrix >= 4.3 gives message:
       zow35> cc -c -DDECFortran cfortest.c
       cfe: Fatal: Out of memory: cfortest.c
       zow35>
   Old __hpux had the problem, but new 'HP-UX A.09.03 A 9000/735' is fine
   if using -Aa, otherwise we have a problem.
 */
#ifndef MAX_PREPRO_ARGS
#if !defined(__GNUC__) && (defined(VAXUltrix) || defined(__CF__APOLLO67) || (defined(sun)&&!defined(__sun)) || defined(_CRAY) || defined(__ultrix__) || (defined(__hpux)&&defined(__CF__KnR)))
#define MAX_PREPRO_ARGS 31
#else
#define MAX_PREPRO_ARGS 99
#endif
#endif

#if defined(AbsoftUNIXFortran) || defined(AbsoftProFortran)
/* In addition to explicit Absoft stuff, only Absoft requires:
   - DEFAULT coming from _cfSTR.
     DEFAULT could have been called e.g. INT, but keep it for clarity.
   - M term in CFARGT14 and CFARGT14FS.
 */
#define ABSOFT_cf1(T0) _(T0,_cfSTR)(0,ABSOFT1,0,0,0,0,0)
#define ABSOFT_cf2(T0) _(T0,_cfSTR)(0,ABSOFT2,0,0,0,0,0)
#define ABSOFT_cf3(T0) _(T0,_cfSTR)(0,ABSOFT3,0,0,0,0,0)
#define DEFAULT_cfABSOFT1
#define LOGICAL_cfABSOFT1
#define  STRING_cfABSOFT1 ,MAX_LEN_FORTRAN_FUNCTION_STRING
#define DEFAULT_cfABSOFT2
#define LOGICAL_cfABSOFT2
#define  STRING_cfABSOFT2 ,unsigned D0
#define DEFAULT_cfABSOFT3
#define LOGICAL_cfABSOFT3
#define  STRING_cfABSOFT3 ,D0
#else
#define ABSOFT_cf1(T0)
#define ABSOFT_cf2(T0)
#define ABSOFT_cf3(T0)
#endif

/* _Z introduced to cicumvent IBM and HP silly preprocessor warning.
   e.g. "Macro CFARGT14 invoked with a null argument."
 */
#define _Z

#define  CFARGT14S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)                \
 S(T1,1)   S(T2,2)   S(T3,3)    S(T4,4)    S(T5,5)    S(T6,6)    S(T7,7)       \
 S(T8,8)   S(T9,9)   S(TA,10)   S(TB,11)   S(TC,12)   S(TD,13)   S(TE,14)
#define  CFARGT27S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
 S(T1,1)   S(T2,2)   S(T3,3)    S(T4,4)    S(T5,5)    S(T6,6)    S(T7,7)       \
 S(T8,8)   S(T9,9)   S(TA,10)   S(TB,11)   S(TC,12)   S(TD,13)   S(TE,14)      \
 S(TF,15)  S(TG,16)  S(TH,17)   S(TI,18)   S(TJ,19)   S(TK,20)   S(TL,21)      \
 S(TM,22)  S(TN,23)  S(TO,24)   S(TP,25)   S(TQ,26)   S(TR,27)

#define  CFARGT14FS(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)           \
 F(T1,1,0) F(T2,2,1) F(T3,3,1)  F(T4,4,1)  F(T5,5,1)  F(T6,6,1)  F(T7,7,1)     \
 F(T8,8,1) F(T9,9,1) F(TA,10,1) F(TB,11,1) F(TC,12,1) F(TD,13,1) F(TE,14,1)    \
 M       CFARGT14S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define  CFARGT27FS(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
 F(T1,1,0)  F(T2,2,1)  F(T3,3,1)  F(T4,4,1)  F(T5,5,1)  F(T6,6,1)  F(T7,7,1)   \
 F(T8,8,1)  F(T9,9,1)  F(TA,10,1) F(TB,11,1) F(TC,12,1) F(TD,13,1) F(TE,14,1)  \
 F(TF,15,1) F(TG,16,1) F(TH,17,1) F(TI,18,1) F(TJ,19,1) F(TK,20,1) F(TL,21,1)  \
 F(TM,22,1) F(TN,23,1) F(TO,24,1) F(TP,25,1) F(TQ,26,1) F(TR,27,1)             \
 M       CFARGT27S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)

#if !(defined(PowerStationFortran)||defined(hpuxFortran800))
/*  Old CFARGT14 -> CFARGT14FS as seen below, for Absoft cross-compile yields:
      SunOS> cc -c -Xa -DAbsoftUNIXFortran c.c
      "c.c", line 406: warning: argument mismatch
    Haven't checked if this is ANSI C or a SunOS bug. SunOS -Xs works ok.
    Behavior is most clearly seen in example:
      #define A 1 , 2
      #define  C(X,Y,Z) x=X. y=Y. z=Z.
      #define  D(X,Y,Z) C(X,Y,Z)
      D(x,A,z)
    Output from preprocessor is: x = x . y = 1 . z = 2 .
 #define CFARGT14(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) \
       CFARGT14FS(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
*/
#define  CFARGT14(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)             \
 F(T1,1,0) F(T2,2,1) F(T3,3,1)  F(T4,4,1)  F(T5,5,1)  F(T6,6,1)  F(T7,7,1)     \
 F(T8,8,1) F(T9,9,1) F(TA,10,1) F(TB,11,1) F(TC,12,1) F(TD,13,1) F(TE,14,1)    \
 M       CFARGT14S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define  CFARGT27(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
 F(T1,1,0)  F(T2,2,1)  F(T3,3,1)  F(T4,4,1)  F(T5,5,1)  F(T6,6,1)  F(T7,7,1)   \
 F(T8,8,1)  F(T9,9,1)  F(TA,10,1) F(TB,11,1) F(TC,12,1) F(TD,13,1) F(TE,14,1)  \
 F(TF,15,1) F(TG,16,1) F(TH,17,1) F(TI,18,1) F(TJ,19,1) F(TK,20,1) F(TL,21,1)  \
 F(TM,22,1) F(TN,23,1) F(TO,24,1) F(TP,25,1) F(TQ,26,1) F(TR,27,1)             \
 M       CFARGT27S(S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)

#define  CFARGT20(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
 F(T1,1,0)  F(T2,2,1)  F(T3,3,1)  F(T4,4,1)  F(T5,5,1)  F(T6,6,1)  F(T7,7,1)   \
 F(T8,8,1)  F(T9,9,1)  F(TA,10,1) F(TB,11,1) F(TC,12,1) F(TD,13,1) F(TE,14,1)  \
 F(TF,15,1) F(TG,16,1) F(TH,17,1) F(TI,18,1) F(TJ,19,1) F(TK,20,1)             \
 S(T1,1)    S(T2,2)    S(T3,3)    S(T4,4)    S(T5,5)    S(T6,6)    S(T7,7)     \
 S(T8,8)    S(T9,9)    S(TA,10)   S(TB,11)   S(TC,12)   S(TD,13)   S(TE,14)    \
 S(TF,15)   S(TG,16)   S(TH,17)   S(TI,18)   S(TJ,19)   S(TK,20)
#define CFARGTA14(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE) \
 F(T1,A1,1,0)  F(T2,A2,2,1)  F(T3,A3,3,1) F(T4,A4,4,1)  F(T5,A5,5,1)  F(T6,A6,6,1)  \
 F(T7,A7,7,1)  F(T8,A8,8,1)  F(T9,A9,9,1) F(TA,AA,10,1) F(TB,AB,11,1) F(TC,AC,12,1) \
 F(TD,AD,13,1) F(TE,AE,14,1) S(T1,1)      S(T2,2)       S(T3,3)       S(T4,4)       \
 S(T5,5)       S(T6,6)       S(T7,7)      S(T8,8)       S(T9,9)       S(TA,10)      \
 S(TB,11)      S(TC,12)      S(TD,13)     S(TE,14)
#if MAX_PREPRO_ARGS>31
#define CFARGTA20(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK) \
 F(T1,A1,1,0)  F(T2,A2,2,1)  F(T3,A3,3,1)  F(T4,A4,4,1)  F(T5,A5,5,1)  F(T6,A6,6,1)  \
 F(T7,A7,7,1)  F(T8,A8,8,1)  F(T9,A9,9,1)  F(TA,AA,10,1) F(TB,AB,11,1) F(TC,AC,12,1) \
 F(TD,AD,13,1) F(TE,AE,14,1) F(TF,AF,15,1) F(TG,AG,16,1) F(TH,AH,17,1) F(TI,AI,18,1) \
 F(TJ,AJ,19,1) F(TK,AK,20,1) S(T1,1)       S(T2,2)       S(T3,3)       S(T4,4)       \
 S(T5,5)       S(T6,6)       S(T7,7)       S(T8,8)       S(T9,9)       S(TA,10)      \
 S(TB,11)      S(TC,12)      S(TD,13)      S(TE,14)      S(TF,15)      S(TG,16)      \
 S(TH,17)      S(TI,18)      S(TJ,19)      S(TK,20)
#define CFARGTA27(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR) \
 F(T1,A1,1,0)  F(T2,A2,2,1)  F(T3,A3,3,1)  F(T4,A4,4,1)  F(T5,A5,5,1)  F(T6,A6,6,1)  \
 F(T7,A7,7,1)  F(T8,A8,8,1)  F(T9,A9,9,1)  F(TA,AA,10,1) F(TB,AB,11,1) F(TC,AC,12,1) \
 F(TD,AD,13,1) F(TE,AE,14,1) F(TF,AF,15,1) F(TG,AG,16,1) F(TH,AH,17,1) F(TI,AI,18,1) \
 F(TJ,AJ,19,1) F(TK,AK,20,1) F(TL,AL,21,1) F(TM,AM,22,1) F(TN,AN,23,1) F(TO,AO,24,1) \
 F(TP,AP,25,1) F(TQ,AQ,26,1) F(TR,AR,27,1) S(T1,1)       S(T2,2)       S(T3,3)       \
 S(T4,4)       S(T5,5)       S(T6,6)       S(T7,7)       S(T8,8)       S(T9,9)       \
 S(TA,10)      S(TB,11)      S(TC,12)      S(TD,13)      S(TE,14)      S(TF,15)      \
 S(TG,16)      S(TH,17)      S(TI,18)      S(TJ,19)      S(TK,20)      S(TL,21)      \
 S(TM,22)      S(TN,23)      S(TO,24)      S(TP,25)      S(TQ,26)      S(TR,27)
#endif
#else
#define  CFARGT14(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)             \
 F(T1,1,0) S(T1,1) F(T2,2,1)  S(T2,2)  F(T3,3,1)  S(T3,3)  F(T4,4,1)  S(T4,4)  \
 F(T5,5,1) S(T5,5) F(T6,6,1)  S(T6,6)  F(T7,7,1)  S(T7,7)  F(T8,8,1)  S(T8,8)  \
 F(T9,9,1) S(T9,9) F(TA,10,1) S(TA,10) F(TB,11,1) S(TB,11) F(TC,12,1) S(TC,12) \
 F(TD,13,1) S(TD,13) F(TE,14,1) S(TE,14)
#define  CFARGT27(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
 F(T1,1,0)  S(T1,1)  F(T2,2,1)  S(T2,2)  F(T3,3,1)  S(T3,3)  F(T4,4,1)  S(T4,4)  \
 F(T5,5,1)  S(T5,5)  F(T6,6,1)  S(T6,6)  F(T7,7,1)  S(T7,7)  F(T8,8,1)  S(T8,8)  \
 F(T9,9,1)  S(T9,9)  F(TA,10,1) S(TA,10) F(TB,11,1) S(TB,11) F(TC,12,1) S(TC,12) \
 F(TD,13,1) S(TD,13) F(TE,14,1) S(TE,14) F(TF,15,1) S(TF,15) F(TG,16,1) S(TG,16) \
 F(TH,17,1) S(TH,17) F(TI,18,1) S(TI,18) F(TJ,19,1) S(TJ,19) F(TK,20,1) S(TK,20) \
 F(TL,21,1) S(TL,21) F(TM,22,1) S(TM,22) F(TN,23,1) S(TN,23) F(TO,24,1) S(TO,24) \
 F(TP,25,1) S(TP,25) F(TQ,26,1) S(TQ,26) F(TR,27,1) S(TR,27)

#define  CFARGT20(F,S,M,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
 F(T1,1,0)  S(T1,1)  F(T2,2,1)  S(T2,2)  F(T3,3,1)  S(T3,3)  F(T4,4,1)  S(T4,4)  \
 F(T5,5,1)  S(T5,5)  F(T6,6,1)  S(T6,6)  F(T7,7,1)  S(T7,7)  F(T8,8,1)  S(T8,8)  \
 F(T9,9,1)  S(T9,9)  F(TA,10,1) S(TA,10) F(TB,11,1) S(TB,11) F(TC,12,1) S(TC,12) \
 F(TD,13,1) S(TD,13) F(TE,14,1) S(TE,14) F(TF,15,1) S(TF,15) F(TG,16,1) S(TG,16) \
 F(TH,17,1) S(TH,17) F(TI,18,1) S(TI,18) F(TJ,19,1) S(TJ,19) F(TK,20,1) S(TK,20)
#define CFARGTA14(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE) \
 F(T1,A1,1,0)  S(T1,1)  F(T2,A2,2,1)  S(T2,2)  F(T3,A3,3,1)  S(T3,3)           \
 F(T4,A4,4,1)  S(T4,4)  F(T5,A5,5,1)  S(T5,5)  F(T6,A6,6,1)  S(T6,6)           \
 F(T7,A7,7,1)  S(T7,7)  F(T8,A8,8,1)  S(T8,8)  F(T9,A9,9,1)  S(T9,9)           \
 F(TA,AA,10,1) S(TA,10) F(TB,AB,11,1) S(TB,11) F(TC,AC,12,1) S(TC,12)          \
 F(TD,AD,13,1) S(TD,13) F(TE,AE,14,1) S(TE,14)
#if MAX_PREPRO_ARGS>31
#define CFARGTA20(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK) \
 F(T1,A1,1,0)  S(T1,1)  F(T2,A2,2,1)  S(T2,2)  F(T3,A3,3,1)  S(T3,3)           \
 F(T4,A4,4,1)  S(T4,4)  F(T5,A5,5,1)  S(T5,5)  F(T6,A6,6,1)  S(T6,6)           \
 F(T7,A7,7,1)  S(T7,7)  F(T8,A8,8,1)  S(T8,8)  F(T9,A9,9,1)  S(T9,9)           \
 F(TA,AA,10,1) S(TA,10) F(TB,AB,11,1) S(TB,11) F(TC,AC,12,1) S(TC,12)          \
 F(TD,AD,13,1) S(TD,13) F(TE,AE,14,1) S(TE,14) F(TF,AF,15,1) S(TF,15)          \
 F(TG,AG,16,1) S(TG,16) F(TH,AH,17,1) S(TH,17) F(TI,AI,18,1) S(TI,18)          \
 F(TJ,AJ,19,1) S(TJ,19) F(TK,AK,20,1) S(TK,20)                
#define CFARGTA27(F,S,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR) \
 F(T1,A1,1,0)  S(T1,1)  F(T2,A2,2,1)  S(T2,2)  F(T3,A3,3,1)  S(T3,3)           \
 F(T4,A4,4,1)  S(T4,4)  F(T5,A5,5,1)  S(T5,5)  F(T6,A6,6,1)  S(T6,6)           \
 F(T7,A7,7,1)  S(T7,7)  F(T8,A8,8,1)  S(T8,8)  F(T9,A9,9,1)  S(T9,9)           \
 F(TA,AA,10,1) S(TA,10) F(TB,AB,11,1) S(TB,11) F(TC,AC,12,1) S(TC,12)          \
 F(TD,AD,13,1) S(TD,13) F(TE,AE,14,1) S(TE,14) F(TF,AF,15,1) S(TF,15)          \
 F(TG,AG,16,1) S(TG,16) F(TH,AH,17,1) S(TH,17) F(TI,AI,18,1) S(TI,18)          \
 F(TJ,AJ,19,1) S(TJ,19) F(TK,AK,20,1) S(TK,20) F(TL,AL,21,1) S(TL,21)          \
 F(TM,AM,22,1) S(TM,22) F(TN,AN,23,1) S(TN,23) F(TO,AO,24,1) S(TO,24)          \
 F(TP,AP,25,1) S(TP,25) F(TQ,AQ,26,1) S(TQ,26) F(TR,AR,27,1) S(TR,27)
#endif
#endif


#define PROTOCCALLSFSUB1( UN,LN,T1) \
        PROTOCCALLSFSUB14(UN,LN,T1,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB2( UN,LN,T1,T2) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB3( UN,LN,T1,T2,T3) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB4( UN,LN,T1,T2,T3,T4) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB5( UN,LN,T1,T2,T3,T4,T5) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB6( UN,LN,T1,T2,T3,T4,T5,T6) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB7( UN,LN,T1,T2,T3,T4,T5,T6,T7) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB8( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB9( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB11(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB12(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,CF_0,CF_0)
#define PROTOCCALLSFSUB13(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD) \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,CF_0)


#define PROTOCCALLSFSUB15(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB16(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB17(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB18(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,CF_0,CF_0)
#define PROTOCCALLSFSUB19(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,CF_0)

#define PROTOCCALLSFSUB21(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB22(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB23(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB24(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,CF_0,CF_0,CF_0)
#define PROTOCCALLSFSUB25(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,CF_0,CF_0)
#define PROTOCCALLSFSUB26(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,CF_0)


#ifndef FCALLSC_QUALIFIER
#ifdef VISUAL_CPLUSPLUS
#define FCALLSC_QUALIFIER __stdcall
#else
#define FCALLSC_QUALIFIER
#endif
#endif

#ifdef __cplusplus
#define CFextern extern "C"
#else
#define CFextern extern
#endif


#ifdef CFSUBASFUN
#define PROTOCCALLSFSUB0(UN,LN) \
   PROTOCCALLSFFUN0( VOID,UN,LN)
#define PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) \
   PROTOCCALLSFFUN14(VOID,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)\
   PROTOCCALLSFFUN20(VOID,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)
#define PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)\
   PROTOCCALLSFFUN27(VOID,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)
#else
/* Note: Prevent compiler warnings, null #define PROTOCCALLSFSUB14/20 after 
   #include-ing cfortran.h if calling the FORTRAN wrapper within the same 
   source code where the wrapper is created. */
#define PROTOCCALLSFSUB0(UN,LN)     _(VOID,_cfPU)(CFC_(UN,LN))();
#ifndef __CF__KnR
#define PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) \
 _(VOID,_cfPU)(CFC_(UN,LN))( CFARGT14(NCF,KCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) );
#define PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)\
 _(VOID,_cfPU)(CFC_(UN,LN))( CFARGT20(NCF,KCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) );
#define PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)\
 _(VOID,_cfPU)(CFC_(UN,LN))( CFARGT27(NCF,KCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) );
#else
#define PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)     \
         PROTOCCALLSFSUB0(UN,LN)
#define PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
         PROTOCCALLSFSUB0(UN,LN)
#define PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
         PROTOCCALLSFSUB0(UN,LN)
#endif
#endif


#ifdef OLD_VAXC                                  /* Allow %CC-I-PARAMNOTUSED. */
#pragma standard
#endif


#define CCALLSFSUB1( UN,LN,T1,                        A1)         \
        CCALLSFSUB5 (UN,LN,T1,CF_0,CF_0,CF_0,CF_0,A1,0,0,0,0)
#define CCALLSFSUB2( UN,LN,T1,T2,                     A1,A2)      \
        CCALLSFSUB5 (UN,LN,T1,T2,CF_0,CF_0,CF_0,A1,A2,0,0,0)
#define CCALLSFSUB3( UN,LN,T1,T2,T3,                  A1,A2,A3)   \
        CCALLSFSUB5 (UN,LN,T1,T2,T3,CF_0,CF_0,A1,A2,A3,0,0)
#define CCALLSFSUB4( UN,LN,T1,T2,T3,T4,               A1,A2,A3,A4)\
        CCALLSFSUB5 (UN,LN,T1,T2,T3,T4,CF_0,A1,A2,A3,A4,0)
#define CCALLSFSUB5( UN,LN,T1,T2,T3,T4,T5,            A1,A2,A3,A4,A5)          \
        CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,CF_0,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,0,0,0,0,0)
#define CCALLSFSUB6( UN,LN,T1,T2,T3,T4,T5,T6,         A1,A2,A3,A4,A5,A6)       \
        CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,0,0,0,0)
#define CCALLSFSUB7( UN,LN,T1,T2,T3,T4,T5,T6,T7,      A1,A2,A3,A4,A5,A6,A7)    \
        CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,T7,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,0,0,0)
#define CCALLSFSUB8( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,   A1,A2,A3,A4,A5,A6,A7,A8) \
        CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,0,0)
#define CCALLSFSUB9( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,A1,A2,A3,A4,A5,A6,A7,A8,A9)\
        CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,0)
#define CCALLSFSUB10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA)\
        CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,0,0,0,0)
#define CCALLSFSUB11(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB)\
        CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,0,0,0)
#define CCALLSFSUB12(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC)\
        CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,0,0)
#define CCALLSFSUB13(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD)\
        CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,0)

#ifdef __cplusplus
#define CPPPROTOCLSFSUB0( UN,LN)
#define CPPPROTOCLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define CPPPROTOCLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)
#define CPPPROTOCLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)
#else
#define CPPPROTOCLSFSUB0(UN,LN) \
        PROTOCCALLSFSUB0(UN,LN)
#define CPPPROTOCLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)     \
        PROTOCCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define CPPPROTOCLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
        PROTOCCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)
#define CPPPROTOCLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
        PROTOCCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)
#endif

#ifdef CFSUBASFUN
#define CCALLSFSUB0(UN,LN) CCALLSFFUN0(UN,LN)
#define CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE)\
        CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE)
#else
/* do{...}while(0) allows if(a==b) FORT(); else BORT(); */
#define CCALLSFSUB0( UN,LN) do{CPPPROTOCLSFSUB0(UN,LN) CFC_(UN,LN)();}while(0)
#define CCALLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE)\
do{VVCF(T1,A1,B1) VVCF(T2,A2,B2) VVCF(T3,A3,B3) VVCF(T4,A4,B4) VVCF(T5,A5,B5)  \
   VVCF(T6,A6,B6) VVCF(T7,A7,B7) VVCF(T8,A8,B8) VVCF(T9,A9,B9) VVCF(TA,AA,B10) \
   VVCF(TB,AB,B11) VVCF(TC,AC,B12) VVCF(TD,AD,B13) VVCF(TE,AE,B14)             \
   CPPPROTOCLSFSUB14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)          \
   ACF(LN,T1,A1,1)  ACF(LN,T2,A2,2)  ACF(LN,T3,A3,3)                           \
   ACF(LN,T4,A4,4)  ACF(LN,T5,A5,5)  ACF(LN,T6,A6,6)  ACF(LN,T7,A7,7)          \
   ACF(LN,T8,A8,8)  ACF(LN,T9,A9,9)  ACF(LN,TA,AA,10) ACF(LN,TB,AB,11)         \
   ACF(LN,TC,AC,12) ACF(LN,TD,AD,13) ACF(LN,TE,AE,14)                          \
   CFC_(UN,LN)( CFARGTA14(AACF,JCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE) );\
   WCF(T1,A1,1)  WCF(T2,A2,2)  WCF(T3,A3,3)  WCF(T4,A4,4)  WCF(T5,A5,5)        \
   WCF(T6,A6,6)  WCF(T7,A7,7)  WCF(T8,A8,8)  WCF(T9,A9,9)  WCF(TA,AA,10)       \
   WCF(TB,AB,11) WCF(TC,AC,12) WCF(TD,AD,13) WCF(TE,AE,14)      }while(0)
#endif


#if MAX_PREPRO_ARGS>31
#define CCALLSFSUB15(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF)\
        CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,CF_0,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,0,0,0,0,0)
#define CCALLSFSUB16(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG)\
        CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,0,0,0,0)
#define CCALLSFSUB17(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH)\
        CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,0,0,0)
#define CCALLSFSUB18(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI)\
        CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,0,0)
#define CCALLSFSUB19(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ)\
        CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,0)

#ifdef CFSUBASFUN
#define CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH, \
        TI,TJ,TK, A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK) \
        CCALLSFFUN20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH, \
        TI,TJ,TK, A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK)
#else
#define CCALLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH, \
        TI,TJ,TK, A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK) \
do{VVCF(T1,A1,B1)  VVCF(T2,A2,B2)  VVCF(T3,A3,B3)  VVCF(T4,A4,B4)  VVCF(T5,A5,B5)   \
   VVCF(T6,A6,B6)  VVCF(T7,A7,B7)  VVCF(T8,A8,B8)  VVCF(T9,A9,B9)  VVCF(TA,AA,B10)  \
   VVCF(TB,AB,B11) VVCF(TC,AC,B12) VVCF(TD,AD,B13) VVCF(TE,AE,B14) VVCF(TF,AF,B15)  \
   VVCF(TG,AG,B16) VVCF(TH,AH,B17) VVCF(TI,AI,B18) VVCF(TJ,AJ,B19) VVCF(TK,AK,B20)  \
   CPPPROTOCLSFSUB20(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)  \
   ACF(LN,T1,A1,1)  ACF(LN,T2,A2,2)  ACF(LN,T3,A3,3)  ACF(LN,T4,A4,4)          \
   ACF(LN,T5,A5,5)  ACF(LN,T6,A6,6)  ACF(LN,T7,A7,7)  ACF(LN,T8,A8,8)          \
   ACF(LN,T9,A9,9)  ACF(LN,TA,AA,10) ACF(LN,TB,AB,11) ACF(LN,TC,AC,12)         \
   ACF(LN,TD,AD,13) ACF(LN,TE,AE,14) ACF(LN,TF,AF,15) ACF(LN,TG,AG,16)         \
   ACF(LN,TH,AH,17) ACF(LN,TI,AI,18) ACF(LN,TJ,AJ,19) ACF(LN,TK,AK,20)         \
   CFC_(UN,LN)( CFARGTA20(AACF,JCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK) ); \
 WCF(T1,A1,1)  WCF(T2,A2,2)  WCF(T3,A3,3)  WCF(T4,A4,4)  WCF(T5,A5,5)  WCF(T6,A6,6)  \
 WCF(T7,A7,7)  WCF(T8,A8,8)  WCF(T9,A9,9)  WCF(TA,AA,10) WCF(TB,AB,11) WCF(TC,AC,12) \
 WCF(TD,AD,13) WCF(TE,AE,14) WCF(TF,AF,15) WCF(TG,AG,16) WCF(TH,AH,17) WCF(TI,AI,18) \
 WCF(TJ,AJ,19) WCF(TK,AK,20) }while(0)
#endif
#endif         /* MAX_PREPRO_ARGS */

#if MAX_PREPRO_ARGS>31
#define CCALLSFSUB21(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,0,0,0,0,0,0)
#define CCALLSFSUB22(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,CF_0,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,0,0,0,0,0)
#define CCALLSFSUB23(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,0,0,0,0)
#define CCALLSFSUB24(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,0,0,0)
#define CCALLSFSUB25(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,0,0)
#define CCALLSFSUB26(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ)\
        CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,0)

#ifdef CFSUBASFUN
#define CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR, \
                           A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR) \
        CCALLSFFUN27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR, \
                           A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR)
#else
#define CCALLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR, \
                           A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR) \
do{VVCF(T1,A1,B1)  VVCF(T2,A2,B2)  VVCF(T3,A3,B3)  VVCF(T4,A4,B4)  VVCF(T5,A5,B5)   \
   VVCF(T6,A6,B6)  VVCF(T7,A7,B7)  VVCF(T8,A8,B8)  VVCF(T9,A9,B9)  VVCF(TA,AA,B10)  \
   VVCF(TB,AB,B11) VVCF(TC,AC,B12) VVCF(TD,AD,B13) VVCF(TE,AE,B14) VVCF(TF,AF,B15)  \
   VVCF(TG,AG,B16) VVCF(TH,AH,B17) VVCF(TI,AI,B18) VVCF(TJ,AJ,B19) VVCF(TK,AK,B20)  \
   VVCF(TL,AL,B21) VVCF(TM,AM,B22) VVCF(TN,AN,B23) VVCF(TO,AO,B24) VVCF(TP,AP,B25)  \
   VVCF(TQ,AQ,B26) VVCF(TR,AR,B27)                                                  \
   CPPPROTOCLSFSUB27(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
   ACF(LN,T1,A1,1)  ACF(LN,T2,A2,2)  ACF(LN,T3,A3,3)  ACF(LN,T4,A4,4)          \
   ACF(LN,T5,A5,5)  ACF(LN,T6,A6,6)  ACF(LN,T7,A7,7)  ACF(LN,T8,A8,8)          \
   ACF(LN,T9,A9,9)  ACF(LN,TA,AA,10) ACF(LN,TB,AB,11) ACF(LN,TC,AC,12)         \
   ACF(LN,TD,AD,13) ACF(LN,TE,AE,14) ACF(LN,TF,AF,15) ACF(LN,TG,AG,16)         \
   ACF(LN,TH,AH,17) ACF(LN,TI,AI,18) ACF(LN,TJ,AJ,19) ACF(LN,TK,AK,20)         \
   ACF(LN,TL,AL,21) ACF(LN,TM,AM,22) ACF(LN,TN,AN,23) ACF(LN,TO,AO,24)         \
   ACF(LN,TP,AP,25) ACF(LN,TQ,AQ,26) ACF(LN,TR,AR,27)                          \
   CFC_(UN,LN)( CFARGTA27(AACF,JCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR,\
                                   A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP,AQ,AR) ); \
 WCF(T1,A1,1)  WCF(T2,A2,2)  WCF(T3,A3,3)  WCF(T4,A4,4)  WCF(T5,A5,5)  WCF(T6,A6,6)  \
 WCF(T7,A7,7)  WCF(T8,A8,8)  WCF(T9,A9,9)  WCF(TA,AA,10) WCF(TB,AB,11) WCF(TC,AC,12) \
 WCF(TD,AD,13) WCF(TE,AE,14) WCF(TF,AF,15) WCF(TG,AG,16) WCF(TH,AH,17) WCF(TI,AI,18) \
 WCF(TJ,AJ,19) WCF(TK,AK,20) WCF(TL,AL,21) WCF(TM,AM,22) WCF(TN,AN,23) WCF(TO,AO,24) \
 WCF(TP,AP,25) WCF(TQ,AQ,26) WCF(TR,AR,27) }while(0)
#endif
#endif         /* MAX_PREPRO_ARGS */

/*-------------------------------------------------------------------------*/

/*               UTILITIES FOR C TO CALL FORTRAN FUNCTIONS                 */

/*N.B. PROTOCCALLSFFUNn(..) generates code, whether or not the FORTRAN
  function is called. Therefore, especially for creator's of C header files
  for large FORTRAN libraries which include many functions, to reduce
  compile time and object code size, it may be desirable to create
  preprocessor directives to allow users to create code for only those
  functions which they use.                                                */

/* The following defines the maximum length string that a function can return.
   Of course it may be undefine-d and re-define-d before individual
   PROTOCCALLSFFUNn(..) as required. It would also be nice to have this derived
   from the individual machines' limits.                                      */
#define MAX_LEN_FORTRAN_FUNCTION_STRING 0x4FE

/* The following defines a character used by CFORTRAN.H to flag the end of a
   string coming out of a FORTRAN routine.                                 */
#define CFORTRAN_NON_CHAR 0x7F

#ifdef OLD_VAXC                                /* Prevent %CC-I-PARAMNOTUSED. */
#pragma nostandard
#endif

#define _SEP_(TN,C,cfCOMMA)     _(__SEP_,C)(TN,cfCOMMA)
#define __SEP_0(TN,cfCOMMA)  
#define __SEP_1(TN,cfCOMMA)     _Icf(2,SEP,TN,cfCOMMA,0)
#define        INT_cfSEP(T,B) _(A,B)
#define       INTV_cfSEP(T,B) INT_cfSEP(T,B)
#define      INTVV_cfSEP(T,B) INT_cfSEP(T,B)
#define     INTVVV_cfSEP(T,B) INT_cfSEP(T,B)
#define    INTVVVV_cfSEP(T,B) INT_cfSEP(T,B)
#define   INTVVVVV_cfSEP(T,B) INT_cfSEP(T,B)
#define  INTVVVVVV_cfSEP(T,B) INT_cfSEP(T,B)
#define INTVVVVVVV_cfSEP(T,B) INT_cfSEP(T,B)
#define       PINT_cfSEP(T,B) INT_cfSEP(T,B)
#define      PVOID_cfSEP(T,B) INT_cfSEP(T,B)
#define    ROUTINE_cfSEP(T,B) INT_cfSEP(T,B)
#define     SIMPLE_cfSEP(T,B) INT_cfSEP(T,B)
#define       VOID_cfSEP(T,B) INT_cfSEP(T,B)    /* For FORTRAN calls C subr.s.*/
#define     STRING_cfSEP(T,B) INT_cfSEP(T,B)
#define    STRINGV_cfSEP(T,B) INT_cfSEP(T,B)
#define    PSTRING_cfSEP(T,B) INT_cfSEP(T,B)
#define   PSTRINGV_cfSEP(T,B) INT_cfSEP(T,B)
#define   PNSTRING_cfSEP(T,B) INT_cfSEP(T,B)
#define   PPSTRING_cfSEP(T,B) INT_cfSEP(T,B)
#define    ZTRINGV_cfSEP(T,B) INT_cfSEP(T,B)
#define   PZTRINGV_cfSEP(T,B) INT_cfSEP(T,B)
                         
#if defined(SIGNED_BYTE) || !defined(UNSIGNED_BYTE)
#ifdef OLD_VAXC
#define INTEGER_BYTE               char    /* Old VAXC barfs on 'signed char' */
#else
#define INTEGER_BYTE        signed char    /* default */
#endif
#else
#define INTEGER_BYTE        unsigned char
#endif
#define    BYTEVVVVVVV_cfTYPE INTEGER_BYTE
#define  DOUBLEVVVVVVV_cfTYPE DOUBLE_PRECISION 
#define   FLOATVVVVVVV_cfTYPE FORTRAN_REAL
#define     INTVVVVVVV_cfTYPE int
#define LOGICALVVVVVVV_cfTYPE int
#define    LONGVVVVVVV_cfTYPE long
#define LONGLONGVVVVVVV_cfTYPE LONGLONG   /* added by MR December 2005 */
#define   SHORTVVVVVVV_cfTYPE short
#define          PBYTE_cfTYPE INTEGER_BYTE
#define        PDOUBLE_cfTYPE DOUBLE_PRECISION 
#define         PFLOAT_cfTYPE FORTRAN_REAL
#define           PINT_cfTYPE int
#define       PLOGICAL_cfTYPE int
#define          PLONG_cfTYPE long
#define      PLONGLONG_cfTYPE LONGLONG  /* added by MR December 2005 */
#define         PSHORT_cfTYPE short

#define CFARGS0(A,T,V,W,X,Y,Z) _3(T,_cf,A)
#define CFARGS1(A,T,V,W,X,Y,Z) _3(T,_cf,A)(V)
#define CFARGS2(A,T,V,W,X,Y,Z) _3(T,_cf,A)(V,W)
#define CFARGS3(A,T,V,W,X,Y,Z) _3(T,_cf,A)(V,W,X)
#define CFARGS4(A,T,V,W,X,Y,Z) _3(T,_cf,A)(V,W,X,Y)
#define CFARGS5(A,T,V,W,X,Y,Z) _3(T,_cf,A)(V,W,X,Y,Z)

#define  _Icf(N,T,I,X,Y)                 _(I,_cfINT)(N,T,I,X,Y,0)
#define _Icf4(N,T,I,X,Y,Z)               _(I,_cfINT)(N,T,I,X,Y,Z)
#define           BYTE_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define         DOUBLE_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INT,B,X,Y,Z,0)
#define          FLOAT_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define            INT_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define        LOGICAL_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define           LONG_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define       LONGLONG_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define          SHORT_cfINT(N,A,B,X,Y,Z)        DOUBLE_cfINT(N,A,B,X,Y,Z)
#define          PBYTE_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define        PDOUBLE_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,PINT,B,X,Y,Z,0)
#define         PFLOAT_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define           PINT_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define       PLOGICAL_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define          PLONG_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define      PLONGLONG_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define         PSHORT_cfINT(N,A,B,X,Y,Z)       PDOUBLE_cfINT(N,A,B,X,Y,Z)
#define          BYTEV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define         BYTEVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define        BYTEVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define       BYTEVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define      BYTEVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define     BYTEVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define    BYTEVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define        DOUBLEV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTV,B,X,Y,Z,0)
#define       DOUBLEVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVV,B,X,Y,Z,0)
#define      DOUBLEVVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVVV,B,X,Y,Z,0)
#define     DOUBLEVVVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVVVV,B,X,Y,Z,0)
#define    DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVVVVV,B,X,Y,Z,0)
#define   DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVVVVVV,B,X,Y,Z,0)
#define  DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,INTVVVVVVV,B,X,Y,Z,0)
#define         FLOATV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define        FLOATVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define       FLOATVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define      FLOATVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define     FLOATVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define    FLOATVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define   FLOATVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define           INTV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define          INTVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define         INTVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define        INTVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define       INTVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define      INTVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define     INTVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define       LOGICALV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define      LOGICALVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define     LOGICALVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define    LOGICALVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define   LOGICALVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define  LOGICALVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define LOGICALVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define          LONGV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define         LONGVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define        LONGVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define       LONGVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define      LONGVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define     LONGVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define    LONGVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define      LONGLONGV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define     LONGLONGVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define    LONGLONGVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define   LONGLONGVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define  LONGLONGVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define LONGLONGVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define LONGLONGVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z) /* added by MR December 2005 */
#define         SHORTV_cfINT(N,A,B,X,Y,Z)       DOUBLEV_cfINT(N,A,B,X,Y,Z)
#define        SHORTVV_cfINT(N,A,B,X,Y,Z)      DOUBLEVV_cfINT(N,A,B,X,Y,Z)
#define       SHORTVVV_cfINT(N,A,B,X,Y,Z)     DOUBLEVVV_cfINT(N,A,B,X,Y,Z)
#define      SHORTVVVV_cfINT(N,A,B,X,Y,Z)    DOUBLEVVVV_cfINT(N,A,B,X,Y,Z)
#define     SHORTVVVVV_cfINT(N,A,B,X,Y,Z)   DOUBLEVVVVV_cfINT(N,A,B,X,Y,Z)
#define    SHORTVVVVVV_cfINT(N,A,B,X,Y,Z)  DOUBLEVVVVVV_cfINT(N,A,B,X,Y,Z)
#define   SHORTVVVVVVV_cfINT(N,A,B,X,Y,Z) DOUBLEVVVVVVV_cfINT(N,A,B,X,Y,Z)
#define          PVOID_cfINT(N,A,B,X,Y,Z) _(CFARGS,N)(A,B,B,X,Y,Z,0)
#define        ROUTINE_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
/*CRAY coughs on the first,
  i.e. the usual trouble of not being able to
  define macros to macros with arguments. 
  New ultrix is worse, it coughs on all such uses.
 */
/*#define       SIMPLE_cfINT                    PVOID_cfINT*/
#define         SIMPLE_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define           VOID_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define         STRING_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define        STRINGV_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define        PSTRING_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define       PSTRINGV_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define       PNSTRING_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define       PPSTRING_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define        ZTRINGV_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define       PZTRINGV_cfINT(N,A,B,X,Y,Z)         PVOID_cfINT(N,A,B,X,Y,Z)
#define           CF_0_cfINT(N,A,B,X,Y,Z)
                         

#define   UCF(TN,I,C)  _SEP_(TN,C,cfCOMMA) _Icf(2,U,TN,_(A,I),0)
#define  UUCF(TN,I,C)  _SEP_(TN,C,cfCOMMA) _SEP_(TN,1,I) 
#define UUUCF(TN,I,C)  _SEP_(TN,C,cfCOLON) _Icf(2,U,TN,_(A,I),0)
#define        INT_cfU(T,A) _(T,VVVVVVV_cfTYPE)   A
#define       INTV_cfU(T,A) _(T,VVVVVV_cfTYPE)  * A
#define      INTVV_cfU(T,A) _(T,VVVVV_cfTYPE)   * A
#define     INTVVV_cfU(T,A) _(T,VVVV_cfTYPE)    * A
#define    INTVVVV_cfU(T,A) _(T,VVV_cfTYPE)     * A
#define   INTVVVVV_cfU(T,A) _(T,VV_cfTYPE)      * A
#define  INTVVVVVV_cfU(T,A) _(T,V_cfTYPE)       * A
#define INTVVVVVVV_cfU(T,A) _(T,_cfTYPE)        * A
#define       PINT_cfU(T,A) _(T,_cfTYPE)        * A
#define      PVOID_cfU(T,A) void  *A 
#define    ROUTINE_cfU(T,A) void (*A)(CF_NULL_PROTO) 
#define       VOID_cfU(T,A) void   A    /* Needed for C calls FORTRAN sub.s.  */
#define     STRING_cfU(T,A) char  *A    /*            via VOID and wrapper.   */
#define    STRINGV_cfU(T,A) char  *A
#define    PSTRING_cfU(T,A) char  *A
#define   PSTRINGV_cfU(T,A) char  *A
#define    ZTRINGV_cfU(T,A) char  *A
#define   PZTRINGV_cfU(T,A) char  *A

/* VOID breaks U into U and UU. */
#define       INT_cfUU(T,A) _(T,VVVVVVV_cfTYPE) A
#define      VOID_cfUU(T,A)             /* Needed for FORTRAN calls C sub.s.  */
#define    STRING_cfUU(T,A) char *A 


#define      BYTE_cfPU(A)   CFextern INTEGER_BYTE      FCALLSC_QUALIFIER A
#define    DOUBLE_cfPU(A)   CFextern DOUBLE_PRECISION  FCALLSC_QUALIFIER A
#if ! (defined(FLOATFUNCTIONTYPE)&&defined(ASSIGNFLOAT)&&defined(RETURNFLOAT))
#if defined (f2cFortran) && ! defined (gFortran)
/* f2c/g77 return double from FORTRAN REAL functions. (KMCCARTY, 2005/12/09) */
#define     FLOAT_cfPU(A)   CFextern DOUBLE_PRECISION  FCALLSC_QUALIFIER A
#else
#define     FLOAT_cfPU(A)   CFextern FORTRAN_REAL      FCALLSC_QUALIFIER A
#endif
#else				   	                   
#define     FLOAT_cfPU(A)   CFextern FLOATFUNCTIONTYPE FCALLSC_QUALIFIER A
#endif				   	                   
#define       INT_cfPU(A)   CFextern int   FCALLSC_QUALIFIER   A
#define   LOGICAL_cfPU(A)   CFextern int   FCALLSC_QUALIFIER   A
#define      LONG_cfPU(A)   CFextern long  FCALLSC_QUALIFIER   A
#define     SHORT_cfPU(A)   CFextern short FCALLSC_QUALIFIER   A
#define    STRING_cfPU(A)   CFextern void  FCALLSC_QUALIFIER   A
#define      VOID_cfPU(A)   CFextern void  FCALLSC_QUALIFIER   A

#define    BYTE_cfE INTEGER_BYTE     A0;
#define  DOUBLE_cfE DOUBLE_PRECISION A0;
#if ! (defined(FLOATFUNCTIONTYPE)&&defined(ASSIGNFLOAT)&&defined(RETURNFLOAT))
#define   FLOAT_cfE FORTRAN_REAL  A0;
#else
#define   FLOAT_cfE FORTRAN_REAL AA0;   FLOATFUNCTIONTYPE A0;
#endif
#define     INT_cfE int    A0;
#define LOGICAL_cfE int    A0;
#define    LONG_cfE long   A0;
#define   SHORT_cfE short  A0;
#define    VOID_cfE
#ifdef vmsFortran
#define  STRING_cfE static char AA0[1+MAX_LEN_FORTRAN_FUNCTION_STRING];        \
                       static fstring A0 =                                     \
             {MAX_LEN_FORTRAN_FUNCTION_STRING,DSC$K_DTYPE_T,DSC$K_CLASS_S,AA0};\
               memset(AA0, CFORTRAN_NON_CHAR, MAX_LEN_FORTRAN_FUNCTION_STRING);\
                                    *(AA0+MAX_LEN_FORTRAN_FUNCTION_STRING)='\0';
#else
#ifdef CRAYFortran
#define  STRING_cfE static char AA0[1+MAX_LEN_FORTRAN_FUNCTION_STRING];        \
                   static _fcd A0; *(AA0+MAX_LEN_FORTRAN_FUNCTION_STRING)='\0';\
                memset(AA0,CFORTRAN_NON_CHAR, MAX_LEN_FORTRAN_FUNCTION_STRING);\
                            A0 = _cptofcd(AA0,MAX_LEN_FORTRAN_FUNCTION_STRING);
#else
/* 'cc: SC3.0.1 13 Jul 1994' barfs on char A0[0x4FE+1]; 
 * char A0[0x4FE +1]; char A0[1+0x4FE]; are both OK.     */
#define STRING_cfE static char A0[1+MAX_LEN_FORTRAN_FUNCTION_STRING];          \
                       memset(A0, CFORTRAN_NON_CHAR,                           \
                              MAX_LEN_FORTRAN_FUNCTION_STRING);                \
                       *(A0+MAX_LEN_FORTRAN_FUNCTION_STRING)='\0';
#endif
#endif
/* ESTRING must use static char. array which is guaranteed to exist after
   function returns.                                                     */

/* N.B.i) The diff. for 0 (Zero) and >=1 arguments.
       ii)That the following create an unmatched bracket, i.e. '(', which
          must of course be matched in the call.
       iii)Commas must be handled very carefully                         */
#define    INT_cfGZ(T,UN,LN) A0=CFC_(UN,LN)(
#define   VOID_cfGZ(T,UN,LN)    CFC_(UN,LN)(
#ifdef vmsFortran
#define STRING_cfGZ(T,UN,LN)    CFC_(UN,LN)(&A0
#else
#if defined(CRAYFortran) || defined(AbsoftUNIXFortran) || defined(AbsoftProFortran)
#define STRING_cfGZ(T,UN,LN)    CFC_(UN,LN)( A0
#else
#define STRING_cfGZ(T,UN,LN)    CFC_(UN,LN)( A0,MAX_LEN_FORTRAN_FUNCTION_STRING
#endif
#endif

#define     INT_cfG(T,UN,LN)    INT_cfGZ(T,UN,LN)
#define    VOID_cfG(T,UN,LN)   VOID_cfGZ(T,UN,LN)
#define  STRING_cfG(T,UN,LN) STRING_cfGZ(T,UN,LN), /*, is only diff. from _cfG*/

#define    BYTEVVVVVVV_cfPP
#define     INTVVVVVVV_cfPP     /* These complement FLOATVVVVVVV_cfPP. */
#define  DOUBLEVVVVVVV_cfPP
#define LOGICALVVVVVVV_cfPP
#define    LONGVVVVVVV_cfPP
#define   SHORTVVVVVVV_cfPP
#define          PBYTE_cfPP
#define           PINT_cfPP
#define        PDOUBLE_cfPP
#define       PLOGICAL_cfPP
#define          PLONG_cfPP
#define         PSHORT_cfPP
#define         PFLOAT_cfPP FLOATVVVVVVV_cfPP

#define BCF(TN,AN,C)        _SEP_(TN,C,cfCOMMA) _Icf(2,B,TN,AN,0)
#define        INT_cfB(T,A) (_(T,VVVVVVV_cfTYPE)) A
#define       INTV_cfB(T,A)            A
#define      INTVV_cfB(T,A)           (A)[0]
#define     INTVVV_cfB(T,A)           (A)[0][0]
#define    INTVVVV_cfB(T,A)           (A)[0][0][0]
#define   INTVVVVV_cfB(T,A)           (A)[0][0][0][0]
#define  INTVVVVVV_cfB(T,A)           (A)[0][0][0][0][0]
#define INTVVVVVVV_cfB(T,A)           (A)[0][0][0][0][0][0]
#define       PINT_cfB(T,A) _(T,_cfPP)&A
#define     STRING_cfB(T,A) (char *)   A
#define    STRINGV_cfB(T,A) (char *)   A
#define    PSTRING_cfB(T,A) (char *)   A
#define   PSTRINGV_cfB(T,A) (char *)   A
#define      PVOID_cfB(T,A) (void *)   A
#define    ROUTINE_cfB(T,A) (cfCAST_FUNCTION)A
#define    ZTRINGV_cfB(T,A) (char *)   A
#define   PZTRINGV_cfB(T,A) (char *)   A
                                                              	
#define SCF(TN,NAME,I,A)    _(TN,_cfSTR)(3,S,NAME,I,A,0,0)
#define  DEFAULT_cfS(M,I,A)
#define  LOGICAL_cfS(M,I,A)
#define PLOGICAL_cfS(M,I,A)
#define   STRING_cfS(M,I,A) ,sizeof(A)
#define  STRINGV_cfS(M,I,A) ,( (unsigned)0xFFFF*firstindexlength(A) \
                              +secondindexlength(A))
#define  PSTRING_cfS(M,I,A) ,sizeof(A)
#define PSTRINGV_cfS(M,I,A) STRINGV_cfS(M,I,A)
#define  ZTRINGV_cfS(M,I,A)
#define PZTRINGV_cfS(M,I,A)

#define   HCF(TN,I)         _(TN,_cfSTR)(3,H,cfCOMMA, H,_(C,I),0,0)
#define  HHCF(TN,I)         _(TN,_cfSTR)(3,H,cfCOMMA,HH,_(C,I),0,0)
#define HHHCF(TN,I)         _(TN,_cfSTR)(3,H,cfCOLON, H,_(C,I),0,0)
#define  H_CF_SPECIAL       unsigned
#define HH_CF_SPECIAL
#define  DEFAULT_cfH(M,I,A)
#define  LOGICAL_cfH(S,U,B)
#define PLOGICAL_cfH(S,U,B)
#define   STRING_cfH(S,U,B) _(A,S) _(U,_CF_SPECIAL) B
#define  STRINGV_cfH(S,U,B) STRING_cfH(S,U,B)
#define  PSTRING_cfH(S,U,B) STRING_cfH(S,U,B)
#define PSTRINGV_cfH(S,U,B) STRING_cfH(S,U,B)
#define PNSTRING_cfH(S,U,B) STRING_cfH(S,U,B)
#define PPSTRING_cfH(S,U,B) STRING_cfH(S,U,B)
#define  ZTRINGV_cfH(S,U,B)
#define PZTRINGV_cfH(S,U,B)

/* Need VOID_cfSTR because Absoft forced function types go through _cfSTR. */
/* No spaces inside expansion. They screws up macro catenation kludge.     */
#define           VOID_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define           BYTE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         DOUBLE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define          FLOAT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define            INT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        LOGICAL_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,LOGICAL,A,B,C,D,E)
#define           LONG_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       LONGLONG_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define          SHORT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define          BYTEV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         BYTEVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        BYTEVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       BYTEVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      BYTEVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     BYTEVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    BYTEVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        DOUBLEV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       DOUBLEVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      DOUBLEVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     DOUBLEVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    DOUBLEVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define   DOUBLEVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define  DOUBLEVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         FLOATV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        FLOATVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       FLOATVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      FLOATVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     FLOATVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    FLOATVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define   FLOATVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define           INTV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define          INTVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         INTVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        INTVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       INTVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      INTVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     INTVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       LOGICALV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      LOGICALVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     LOGICALVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    LOGICALVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define   LOGICALVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define  LOGICALVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define LOGICALVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define          LONGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         LONGVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        LONGVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       LONGVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      LONGVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     LONGVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    LONGVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      LONGLONGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define     LONGLONGVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define    LONGLONGVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define   LONGLONGVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define  LONGLONGVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define LONGLONGVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define LONGLONGVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define         SHORTV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        SHORTVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       SHORTVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      SHORTVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define     SHORTVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define    SHORTVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define   SHORTVVVVVVV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define          PBYTE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        PDOUBLE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         PFLOAT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define           PINT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define       PLOGICAL_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PLOGICAL,A,B,C,D,E)
#define          PLONG_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define      PLONGLONG_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E) /* added by MR December 2005 */
#define         PSHORT_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         STRING_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,STRING,A,B,C,D,E)
#define        PSTRING_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PSTRING,A,B,C,D,E)
#define        STRINGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,STRINGV,A,B,C,D,E)
#define       PSTRINGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PSTRINGV,A,B,C,D,E)
#define       PNSTRING_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PNSTRING,A,B,C,D,E)
#define       PPSTRING_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PPSTRING,A,B,C,D,E)
#define          PVOID_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        ROUTINE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define         SIMPLE_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,DEFAULT,A,B,C,D,E)
#define        ZTRINGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,ZTRINGV,A,B,C,D,E)
#define       PZTRINGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PZTRINGV,A,B,C,D,E)
#define           CF_0_cfSTR(N,T,A,B,C,D,E)

/* See ACF table comments, which explain why CCF was split into two. */
#define CCF(NAME,TN,I)     _(TN,_cfSTR)(5,C,NAME,I,_(A,I),_(B,I),_(C,I))
#define  DEFAULT_cfC(M,I,A,B,C)
#define  LOGICAL_cfC(M,I,A,B,C)  A=C2FLOGICAL( A);
#define PLOGICAL_cfC(M,I,A,B,C) *A=C2FLOGICAL(*A);
#ifdef vmsFortran
#define   STRING_cfC(M,I,A,B,C) (B.clen=strlen(A),B.f.dsc$a_pointer=A,         \
        C==sizeof(char*)||C==(unsigned)(B.clen+1)?B.f.dsc$w_length=B.clen:     \
          (memset((A)+B.clen,' ',C-B.clen-1),A[B.f.dsc$w_length=C-1]='\0'));
      /* PSTRING_cfC to beware of array A which does not contain any \0.      */
#define  PSTRING_cfC(M,I,A,B,C) (B.dsc$a_pointer=A, C==sizeof(char*) ?         \
             B.dsc$w_length=strlen(A):  (A[C-1]='\0',B.dsc$w_length=strlen(A), \
       memset((A)+B.dsc$w_length,' ',C-B.dsc$w_length-1), B.dsc$w_length=C-1));
#else
#define   STRING_cfC(M,I,A,B,C) (B.nombre=A,B.clen=strlen(A),                             \
                C==sizeof(char*)||C==(unsigned)(B.clen+1)?B.flen=B.clen:       \
                        (memset(B.nombre+B.clen,' ',C-B.clen-1),B.nombre[B.flen=C-1]='\0'));
#define  PSTRING_cfC(M,I,A,B,C) (C==sizeof(char*)? B=strlen(A):                \
                    (A[C-1]='\0',B=strlen(A),memset((A)+B,' ',C-B-1),B=C-1));
#endif
          /* For CRAYFortran for (P)STRINGV_cfC, B.fs is set, but irrelevant. */
#define  STRINGV_cfC(M,I,A,B,C) \
        AATRINGV_cfA(    A,B,(C/0xFFFF)*(C%0xFFFF),C/0xFFFF,C%0xFFFF)
#define PSTRINGV_cfC(M,I,A,B,C) \
       APATRINGV_cfA(    A,B,(C/0xFFFF)*(C%0xFFFF),C/0xFFFF,C%0xFFFF)
#define  ZTRINGV_cfC(M,I,A,B,C) \
        AATRINGV_cfA(    A,B, (_3(M,_ELEMS_,I))*((_3(M,_ELEMLEN_,I))+1),       \
                              (_3(M,_ELEMS_,I)), (_3(M,_ELEMLEN_,I))+1   )
#define PZTRINGV_cfC(M,I,A,B,C) \
       APATRINGV_cfA(    A,B, (_3(M,_ELEMS_,I))*((_3(M,_ELEMLEN_,I))+1),       \
                              (_3(M,_ELEMS_,I)), (_3(M,_ELEMLEN_,I))+1   )

#define     BYTE_cfCCC(A,B) &A
#define   DOUBLE_cfCCC(A,B) &A
#if !defined(__CF__KnR)
#define    FLOAT_cfCCC(A,B) &A
                               /* Although the VAX doesn't, at least the      */
#else                          /* HP and K&R mips promote float arg.'s of     */
#define    FLOAT_cfCCC(A,B) &B /* unprototyped functions to double. Cannot    */
#endif                         /* use A here to pass the argument to FORTRAN. */
#define      INT_cfCCC(A,B) &A
#define  LOGICAL_cfCCC(A,B) &A
#define     LONG_cfCCC(A,B) &A
#define    SHORT_cfCCC(A,B) &A
#define    PBYTE_cfCCC(A,B)  A
#define  PDOUBLE_cfCCC(A,B)  A
#define   PFLOAT_cfCCC(A,B)  A
#define     PINT_cfCCC(A,B)  A
#define PLOGICAL_cfCCC(A,B)  B=A       /* B used to keep a common W table. */
#define    PLONG_cfCCC(A,B)  A
#define   PSHORT_cfCCC(A,B)  A

#define CCCF(TN,I,M)           _SEP_(TN,M,cfCOMMA) _Icf(3,CC,TN,_(A,I),_(B,I))
#define        INT_cfCC(T,A,B) _(T,_cfCCC)(A,B) 
#define       INTV_cfCC(T,A,B)  A
#define      INTVV_cfCC(T,A,B)  A
#define     INTVVV_cfCC(T,A,B)  A
#define    INTVVVV_cfCC(T,A,B)  A
#define   INTVVVVV_cfCC(T,A,B)  A
#define  INTVVVVVV_cfCC(T,A,B)  A
#define INTVVVVVVV_cfCC(T,A,B)  A
#define       PINT_cfCC(T,A,B) _(T,_cfCCC)(A,B) 
#define      PVOID_cfCC(T,A,B)  A
#if defined(apolloFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran)
#define    ROUTINE_cfCC(T,A,B) &A
#else
#define    ROUTINE_cfCC(T,A,B)  A
#endif
#define     SIMPLE_cfCC(T,A,B)  A
#ifdef vmsFortran
#define     STRING_cfCC(T,A,B) &B.f
#define    STRINGV_cfCC(T,A,B) &B
#define    PSTRING_cfCC(T,A,B) &B
#define   PSTRINGV_cfCC(T,A,B) &B
#else
#ifdef CRAYFortran
#define     STRING_cfCC(T,A,B) _cptofcd(A,B.flen)
#define    STRINGV_cfCC(T,A,B) _cptofcd(B.s,B.flen)
#define    PSTRING_cfCC(T,A,B) _cptofcd(A,B)
#define   PSTRINGV_cfCC(T,A,B) _cptofcd(A,B.flen)
#else
#define     STRING_cfCC(T,A,B)  A
#define    STRINGV_cfCC(T,A,B)  B.fs
#define    PSTRING_cfCC(T,A,B)  A
#define   PSTRINGV_cfCC(T,A,B)  B.fs
#endif
#endif
#define    ZTRINGV_cfCC(T,A,B)   STRINGV_cfCC(T,A,B)
#define   PZTRINGV_cfCC(T,A,B)  PSTRINGV_cfCC(T,A,B)

#define    BYTE_cfX  return A0;
#define  DOUBLE_cfX  return A0;
#if ! (defined(FLOATFUNCTIONTYPE)&&defined(ASSIGNFLOAT)&&defined(RETURNFLOAT))
#define   FLOAT_cfX  return A0;
#else
#define   FLOAT_cfX  ASSIGNFLOAT(AA0,A0); return AA0;
#endif
#define     INT_cfX  return A0;
#define LOGICAL_cfX  return F2CLOGICAL(A0);
#define    LONG_cfX  return A0;
#define   SHORT_cfX  return A0;
#define    VOID_cfX  return   ;
#if defined(vmsFortran) || defined(CRAYFortran)
#define  STRING_cfX  return kill_trailing(                                     \
                                      kill_trailing(AA0,CFORTRAN_NON_CHAR),' ');
#else
#define  STRING_cfX  return kill_trailing(                                     \
                                      kill_trailing( A0,CFORTRAN_NON_CHAR),' ');
#endif

#define CFFUN(NAME) _(__cf__,NAME)

/* Note that we don't use LN here, but we keep it for consistency. */
#define CCALLSFFUN0(UN,LN) CFFUN(UN)()

#ifdef OLD_VAXC                                  /* Allow %CC-I-PARAMNOTUSED. */
#pragma standard
#endif

#define CCALLSFFUN1( UN,LN,T1,                        A1)         \
        CCALLSFFUN5 (UN,LN,T1,CF_0,CF_0,CF_0,CF_0,A1,0,0,0,0)
#define CCALLSFFUN2( UN,LN,T1,T2,                     A1,A2)      \
        CCALLSFFUN5 (UN,LN,T1,T2,CF_0,CF_0,CF_0,A1,A2,0,0,0)
#define CCALLSFFUN3( UN,LN,T1,T2,T3,                  A1,A2,A3)   \
        CCALLSFFUN5 (UN,LN,T1,T2,T3,CF_0,CF_0,A1,A2,A3,0,0)
#define CCALLSFFUN4( UN,LN,T1,T2,T3,T4,               A1,A2,A3,A4)\
        CCALLSFFUN5 (UN,LN,T1,T2,T3,T4,CF_0,A1,A2,A3,A4,0)
#define CCALLSFFUN5( UN,LN,T1,T2,T3,T4,T5,            A1,A2,A3,A4,A5)          \
        CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,CF_0,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,0,0,0,0,0)
#define CCALLSFFUN6( UN,LN,T1,T2,T3,T4,T5,T6,         A1,A2,A3,A4,A5,A6)       \
        CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,T6,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,0,0,0,0)
#define CCALLSFFUN7( UN,LN,T1,T2,T3,T4,T5,T6,T7,      A1,A2,A3,A4,A5,A6,A7)    \
        CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,T6,T7,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,0,0,0)
#define CCALLSFFUN8( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,   A1,A2,A3,A4,A5,A6,A7,A8) \
        CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,0,0)
#define CCALLSFFUN9( UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,A1,A2,A3,A4,A5,A6,A7,A8,A9)\
        CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,0)
#define CCALLSFFUN10(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA)\
        CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,CF_0,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,0,0,0,0)
#define CCALLSFFUN11(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB)\
        CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,CF_0,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,0,0,0)
#define CCALLSFFUN12(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC)\
        CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,CF_0,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,0,0)
#define CCALLSFFUN13(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD)\
        CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,CF_0,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,0)

#define CCALLSFFUN14(UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE)\
((CFFUN(UN)(  BCF(T1,A1,0) BCF(T2,A2,1) BCF(T3,A3,1) BCF(T4,A4,1) BCF(T5,A5,1) \
              BCF(T6,A6,1) BCF(T7,A7,1) BCF(T8,A8,1) BCF(T9,A9,1) BCF(TA,AA,1) \
              BCF(TB,AB,1) BCF(TC,AC,1) BCF(TD,AD,1) BCF(TE,AE,1)              \
           SCF(T1,LN,1,A1)  SCF(T2,LN,2,A2)  SCF(T3,LN,3,A3)  SCF(T4,LN,4,A4)  \
           SCF(T5,LN,5,A5)  SCF(T6,LN,6,A6)  SCF(T7,LN,7,A7)  SCF(T8,LN,8,A8)  \
           SCF(T9,LN,9,A9)  SCF(TA,LN,10,AA) SCF(TB,LN,11,AB) SCF(TC,LN,12,AC) \
           SCF(TD,LN,13,AD) SCF(TE,LN,14,AE))))

/*  N.B. Create a separate function instead of using (call function, function
value here) because in order to create the variables needed for the input
arg.'s which may be const.'s one has to do the creation within {}, but these
can never be placed within ()'s. Therefore one must create wrapper functions.
gcc, on the other hand may be able to avoid the wrapper functions. */

/* Prototypes are needed to correctly handle the value returned correctly. N.B.
Can only have prototype arg.'s with difficulty, a la G... table since FORTRAN
functions returning strings have extra arg.'s. Don't bother, since this only
causes a compiler warning to come up when one uses FCALLSCFUNn and CCALLSFFUNn
for the same function in the same source code. Something done by the experts in
debugging only.*/    

#define PROTOCCALLSFFUN0(F,UN,LN)                                              \
_(F,_cfPU)( CFC_(UN,LN))(CF_NULL_PROTO);                                       \
static _Icf(2,U,F,CFFUN(UN),0)() {_(F,_cfE) _Icf(3,GZ,F,UN,LN) ABSOFT_cf1(F));_(F,_cfX)}

#define PROTOCCALLSFFUN1( T0,UN,LN,T1)                                         \
        PROTOCCALLSFFUN5 (T0,UN,LN,T1,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN2( T0,UN,LN,T1,T2)                                      \
        PROTOCCALLSFFUN5 (T0,UN,LN,T1,T2,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN3( T0,UN,LN,T1,T2,T3)                                   \
        PROTOCCALLSFFUN5 (T0,UN,LN,T1,T2,T3,CF_0,CF_0)
#define PROTOCCALLSFFUN4( T0,UN,LN,T1,T2,T3,T4)                                \
        PROTOCCALLSFFUN5 (T0,UN,LN,T1,T2,T3,T4,CF_0)
#define PROTOCCALLSFFUN5( T0,UN,LN,T1,T2,T3,T4,T5)                             \
        PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,CF_0,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN6( T0,UN,LN,T1,T2,T3,T4,T5,T6)                          \
        PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,T6,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN7( T0,UN,LN,T1,T2,T3,T4,T5,T6,T7)                       \
        PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN8( T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8)                    \
        PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,CF_0,CF_0)
#define PROTOCCALLSFFUN9( T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9)                 \
        PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,CF_0)
#define PROTOCCALLSFFUN10(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA)              \
        PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,CF_0,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN11(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB)           \
        PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,CF_0,CF_0,CF_0)
#define PROTOCCALLSFFUN12(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC)        \
        PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,CF_0,CF_0)
#define PROTOCCALLSFFUN13(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD)     \
        PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,CF_0)

/* HP/UX 9.01 cc requires the blank between '_Icf(3,G,T0,UN,LN) CCCF(T1,1,0)' */

#ifndef __CF__KnR
#define PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)  \
 _(T0,_cfPU)(CFC_(UN,LN))(CF_NULL_PROTO); static _Icf(2,U,T0,CFFUN(UN),0)(     \
   CFARGT14FS(UCF,HCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) )          \
{       CFARGT14S(VCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    _(T0,_cfE) \
 CCF(LN,T1,1)  CCF(LN,T2,2)  CCF(LN,T3,3)  CCF(LN,T4,4)  CCF(LN,T5,5)          \
 CCF(LN,T6,6)  CCF(LN,T7,7)  CCF(LN,T8,8)  CCF(LN,T9,9)  CCF(LN,TA,10)         \
 CCF(LN,TB,11) CCF(LN,TC,12) CCF(LN,TD,13) CCF(LN,TE,14)    _Icf(3,G,T0,UN,LN) \
 CFARGT14(CCCF,JCF,ABSOFT_cf1(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)); \
 WCF(T1,A1,1)   WCF(T2,A2,2)   WCF(T3,A3,3)   WCF(T4,A4,4)  WCF(T5,A5,5)       \
 WCF(T6,A6,6)   WCF(T7,A7,7)   WCF(T8,A8,8)   WCF(T9,A9,9)  WCF(TA,A10,10)     \
 WCF(TB,A11,11) WCF(TC,A12,12) WCF(TD,A13,13) WCF(TE,A14,14) _(T0,_cfX)}
#else
#define PROTOCCALLSFFUN14(T0,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)  \
 _(T0,_cfPU)(CFC_(UN,LN))(CF_NULL_PROTO); static _Icf(2,U,T0,CFFUN(UN),0)(     \
   CFARGT14FS(UUCF,HHCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) )        \
 CFARGT14FS(UUUCF,HHHCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) ;        \
{       CFARGT14S(VCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    _(T0,_cfE) \
 CCF(LN,T1,1)  CCF(LN,T2,2)  CCF(LN,T3,3)  CCF(LN,T4,4)  CCF(LN,T5,5)          \
 CCF(LN,T6,6)  CCF(LN,T7,7)  CCF(LN,T8,8)  CCF(LN,T9,9)  CCF(LN,TA,10)         \
 CCF(LN,TB,11) CCF(LN,TC,12) CCF(LN,TD,13) CCF(LN,TE,14)    _Icf(3,G,T0,UN,LN) \
 CFARGT14(CCCF,JCF,ABSOFT_cf1(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)); \
 WCF(T1,A1,1)   WCF(T2,A2,2)   WCF(T3,A3,3)   WCF(T4,A4,4)   WCF(T5,A5,5)      \
 WCF(T6,A6,6)   WCF(T7,A7,7)   WCF(T8,A8,8)   WCF(T9,A9,9)   WCF(TA,A10,10)    \
 WCF(TB,A11,11) WCF(TC,A12,12) WCF(TD,A13,13) WCF(TE,A14,14) _(T0,_cfX)}
#endif

/*-------------------------------------------------------------------------*/

/*               UTILITIES FOR FORTRAN TO CALL C ROUTINES                  */

#ifdef OLD_VAXC                                /* Prevent %CC-I-PARAMNOTUSED. */
#pragma nostandard
#endif

#if defined(vmsFortran) || defined(CRAYFortran)
#define   DCF(TN,I)
#define  DDCF(TN,I)
#define DDDCF(TN,I)
#else
#define   DCF(TN,I)          HCF(TN,I)
#define  DDCF(TN,I)         HHCF(TN,I)
#define DDDCF(TN,I)        HHHCF(TN,I)
#endif

#define QCF(TN,I)       _(TN,_cfSTR)(1,Q,_(B,I), 0,0,0,0)
#define  DEFAULT_cfQ(B)
#define  LOGICAL_cfQ(B)
#define PLOGICAL_cfQ(B)
#define  STRINGV_cfQ(B) char *B; unsigned int _(B,N);
#define   STRING_cfQ(B) char *B=NULL;
#define  PSTRING_cfQ(B) char *B=NULL;
#define PSTRINGV_cfQ(B) STRINGV_cfQ(B)
#define PNSTRING_cfQ(B) char *B=NULL;
#define PPSTRING_cfQ(B)

#ifdef     __sgi   /* Else SGI gives warning 182 contrary to its C LRM A.17.7 */
#define ROUTINE_orig    *(void**)& 
#else
#define ROUTINE_orig     (void *)  
#endif

#define ROUTINE_1     ROUTINE_orig   
#define ROUTINE_2     ROUTINE_orig   
#define ROUTINE_3     ROUTINE_orig   
#define ROUTINE_4     ROUTINE_orig   
#define ROUTINE_5     ROUTINE_orig   
#define ROUTINE_6     ROUTINE_orig   
#define ROUTINE_7     ROUTINE_orig   
#define ROUTINE_8     ROUTINE_orig   
#define ROUTINE_9     ROUTINE_orig   
#define ROUTINE_10    ROUTINE_orig   
#define ROUTINE_11    ROUTINE_orig   
#define ROUTINE_12    ROUTINE_orig   
#define ROUTINE_13    ROUTINE_orig   
#define ROUTINE_14    ROUTINE_orig   
#define ROUTINE_15    ROUTINE_orig   
#define ROUTINE_16    ROUTINE_orig   
#define ROUTINE_17    ROUTINE_orig   
#define ROUTINE_18    ROUTINE_orig   
#define ROUTINE_19    ROUTINE_orig   
#define ROUTINE_20    ROUTINE_orig   
#define ROUTINE_21    ROUTINE_orig   
#define ROUTINE_22    ROUTINE_orig   
#define ROUTINE_23    ROUTINE_orig   
#define ROUTINE_24    ROUTINE_orig   
#define ROUTINE_25    ROUTINE_orig   
#define ROUTINE_26    ROUTINE_orig   
#define ROUTINE_27    ROUTINE_orig   

#define TCF(NAME,TN,I,M)              _SEP_(TN,M,cfCOMMA) _(TN,_cfT)(NAME,I,_(A,I),_(B,I),_(C,I))
#define           BYTE_cfT(M,I,A,B,D) *A
#define         DOUBLE_cfT(M,I,A,B,D) *A
#define          FLOAT_cfT(M,I,A,B,D) *A
#define            INT_cfT(M,I,A,B,D) *A
#define        LOGICAL_cfT(M,I,A,B,D)  F2CLOGICAL(*A)
#define           LONG_cfT(M,I,A,B,D) *A
#define       LONGLONG_cfT(M,I,A,B,D) *A /* added by MR December 2005 */
#define          SHORT_cfT(M,I,A,B,D) *A
#define          BYTEV_cfT(M,I,A,B,D)  A
#define        DOUBLEV_cfT(M,I,A,B,D)  A
#define         FLOATV_cfT(M,I,A,B,D)  VOIDP A
#define           INTV_cfT(M,I,A,B,D)  A
#define       LOGICALV_cfT(M,I,A,B,D)  A
#define          LONGV_cfT(M,I,A,B,D)  A
#define      LONGLONGV_cfT(M,I,A,B,D)  A /* added by MR December 2005 */
#define         SHORTV_cfT(M,I,A,B,D)  A
#define         BYTEVV_cfT(M,I,A,B,D)  (void *)A /* We have to cast to void *,*/
#define        BYTEVVV_cfT(M,I,A,B,D)  (void *)A /* since we don't know the   */
#define       BYTEVVVV_cfT(M,I,A,B,D)  (void *)A /* dimensions of the array.  */
#define      BYTEVVVVV_cfT(M,I,A,B,D)  (void *)A /* i.e. Unfortunately, can't */
#define     BYTEVVVVVV_cfT(M,I,A,B,D)  (void *)A /* check that the type       */
#define    BYTEVVVVVVV_cfT(M,I,A,B,D)  (void *)A /* matches the prototype.    */
#define       DOUBLEVV_cfT(M,I,A,B,D)  (void *)A
#define      DOUBLEVVV_cfT(M,I,A,B,D)  (void *)A
#define     DOUBLEVVVV_cfT(M,I,A,B,D)  (void *)A
#define    DOUBLEVVVVV_cfT(M,I,A,B,D)  (void *)A
#define   DOUBLEVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define  DOUBLEVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define        FLOATVV_cfT(M,I,A,B,D)  (void *)A
#define       FLOATVVV_cfT(M,I,A,B,D)  (void *)A
#define      FLOATVVVV_cfT(M,I,A,B,D)  (void *)A
#define     FLOATVVVVV_cfT(M,I,A,B,D)  (void *)A
#define    FLOATVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define   FLOATVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define          INTVV_cfT(M,I,A,B,D)  (void *)A  
#define         INTVVV_cfT(M,I,A,B,D)  (void *)A  
#define        INTVVVV_cfT(M,I,A,B,D)  (void *)A  
#define       INTVVVVV_cfT(M,I,A,B,D)  (void *)A
#define      INTVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define     INTVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define      LOGICALVV_cfT(M,I,A,B,D)  (void *)A
#define     LOGICALVVV_cfT(M,I,A,B,D)  (void *)A
#define    LOGICALVVVV_cfT(M,I,A,B,D)  (void *)A
#define   LOGICALVVVVV_cfT(M,I,A,B,D)  (void *)A
#define  LOGICALVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define LOGICALVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define         LONGVV_cfT(M,I,A,B,D)  (void *)A
#define        LONGVVV_cfT(M,I,A,B,D)  (void *)A
#define       LONGVVVV_cfT(M,I,A,B,D)  (void *)A
#define      LONGVVVVV_cfT(M,I,A,B,D)  (void *)A
#define     LONGVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define    LONGVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define     LONGLONGVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define    LONGLONGVVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define   LONGLONGVVVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define  LONGLONGVVVVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define LONGLONGVVVVVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define LONGLONGVVVVVVV_cfT(M,I,A,B,D)  (void *)A /* added by MR December 2005 */
#define        SHORTVV_cfT(M,I,A,B,D)  (void *)A
#define       SHORTVVV_cfT(M,I,A,B,D)  (void *)A
#define      SHORTVVVV_cfT(M,I,A,B,D)  (void *)A
#define     SHORTVVVVV_cfT(M,I,A,B,D)  (void *)A
#define    SHORTVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define   SHORTVVVVVVV_cfT(M,I,A,B,D)  (void *)A
#define          PBYTE_cfT(M,I,A,B,D)  A
#define        PDOUBLE_cfT(M,I,A,B,D)  A
#define         PFLOAT_cfT(M,I,A,B,D)  VOIDP A
#define           PINT_cfT(M,I,A,B,D)  A
#define       PLOGICAL_cfT(M,I,A,B,D)  ((*A=F2CLOGICAL(*A)),A)
#define          PLONG_cfT(M,I,A,B,D)  A
#define      PLONGLONG_cfT(M,I,A,B,D)  A /* added by MR December 2005 */
#define         PSHORT_cfT(M,I,A,B,D)  A
#define          PVOID_cfT(M,I,A,B,D)  A
#if defined(apolloFortran) || defined(hpuxFortran800) || defined(AbsoftUNIXFortran)
#define        ROUTINE_cfT(M,I,A,B,D)  _(ROUTINE_,I)  (*A)
#else
#define        ROUTINE_cfT(M,I,A,B,D)  _(ROUTINE_,I)    A
#endif
/* A == pointer to the characters
   D == length of the string, or of an element in an array of strings
   E == number of elements in an array of strings                             */
#define TTSTR(    A,B,D)                                                       \
           ((B=_cf_malloc(D+1))[D]='\0', memcpy(B,A,D), kill_trailing(B,' '))
#define TTTTSTR(  A,B,D)   (!(D<4||A[0]||A[1]||A[2]||A[3]))?NULL:              \
                            memchr(A,'\0',D)                 ?A   : TTSTR(A,B,D)
#define TTTTSTRV( A,B,D,E) (_(B,N)=E,B=_cf_malloc(_(B,N)*(D+1)), (void *)      \
  vkill_trailing(f2cstrv(A,B,D+1, _(B,N)*(D+1)), D+1,_(B,N)*(D+1),' '))
#ifdef vmsFortran
#define         STRING_cfT(M,I,A,B,D)  TTTTSTR( A->dsc$a_pointer,B,A->dsc$w_length)
#define        STRINGV_cfT(M,I,A,B,D)  TTTTSTRV(A->dsc$a_pointer, B,           \
                                             A->dsc$w_length , A->dsc$l_m[0])
#define        PSTRING_cfT(M,I,A,B,D)    TTSTR( A->dsc$a_pointer,B,A->dsc$w_length)
#define       PPSTRING_cfT(M,I,A,B,D)           A->dsc$a_pointer
#else
#ifdef CRAYFortran
#define         STRING_cfT(M,I,A,B,D)  TTTTSTR( _fcdtocp(A),B,_fcdlen(A))
#define        STRINGV_cfT(M,I,A,B,D)  TTTTSTRV(_fcdtocp(A),B,_fcdlen(A),      \
                              num_elem(_fcdtocp(A),_fcdlen(A),_3(M,_STRV_A,I)))
#define        PSTRING_cfT(M,I,A,B,D)    TTSTR( _fcdtocp(A),B,_fcdlen(A))
#define       PPSTRING_cfT(M,I,A,B,D)           _fcdtocp(A)
#else
#define         STRING_cfT(M,I,A,B,D)  TTTTSTR( A,B,D)
#define        STRINGV_cfT(M,I,A,B,D)  TTTTSTRV(A,B,D, num_elem(A,D,_3(M,_STRV_A,I)))
#define        PSTRING_cfT(M,I,A,B,D)    TTSTR( A,B,D)
#define       PPSTRING_cfT(M,I,A,B,D)           A
#endif
#endif
#define       PNSTRING_cfT(M,I,A,B,D)    STRING_cfT(M,I,A,B,D)
#define       PSTRINGV_cfT(M,I,A,B,D)   STRINGV_cfT(M,I,A,B,D)
#define           CF_0_cfT(M,I,A,B,D)

#define RCF(TN,I)           _(TN,_cfSTR)(3,R,_(A,I),_(B,I),_(C,I),0,0)
#define  DEFAULT_cfR(A,B,D)
#define  LOGICAL_cfR(A,B,D)
#define PLOGICAL_cfR(A,B,D) *A=C2FLOGICAL(*A);
#define   STRING_cfR(A,B,D) if (B) _cf_free(B);
#define  STRINGV_cfR(A,B,D) _cf_free(B);
/* A and D as defined above for TSTRING(V) */
#define RRRRPSTR( A,B,D)    if (B) memcpy(A,B, _cfMIN(strlen(B),D)),           \
                  (D>strlen(B)?memset(A+strlen(B),' ', D-strlen(B)):0), _cf_free(B);
#define RRRRPSTRV(A,B,D)    c2fstrv(B,A,D+1,(D+1)*_(B,N)), _cf_free(B);
#ifdef vmsFortran
#define  PSTRING_cfR(A,B,D) RRRRPSTR( A->dsc$a_pointer,B,A->dsc$w_length)
#define PSTRINGV_cfR(A,B,D) RRRRPSTRV(A->dsc$a_pointer,B,A->dsc$w_length)
#else
#ifdef CRAYFortran
#define  PSTRING_cfR(A,B,D) RRRRPSTR( _fcdtocp(A),B,_fcdlen(A))
#define PSTRINGV_cfR(A,B,D) RRRRPSTRV(_fcdtocp(A),B,_fcdlen(A))
#else
#define  PSTRING_cfR(A,B,D) RRRRPSTR( A,B,D)
#define PSTRINGV_cfR(A,B,D) RRRRPSTRV(A,B,D)
#endif
#endif
#define PNSTRING_cfR(A,B,D) PSTRING_cfR(A,B,D)
#define PPSTRING_cfR(A,B,D)

#define    BYTE_cfFZ(UN,LN) INTEGER_BYTE     FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define  DOUBLE_cfFZ(UN,LN) DOUBLE_PRECISION FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define     INT_cfFZ(UN,LN) int   FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define LOGICAL_cfFZ(UN,LN) int   FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define    LONG_cfFZ(UN,LN) long  FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define LONGLONG_cfFZ(UN,LN) LONGLONG FCALLSC_QUALIFIER fcallsc(UN,LN)( /* added by MR December 2005 */
#define   SHORT_cfFZ(UN,LN) short FCALLSC_QUALIFIER fcallsc(UN,LN)(
#define    VOID_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(
#ifndef __CF__KnR
/* The void is req'd by the Apollo, to make this an ANSI function declaration.
   The Apollo promotes K&R float functions to double. */
#if defined (f2cFortran) && ! defined (gFortran)
/* f2c/g77 return double from FORTRAN REAL functions. (KMCCARTY, 2005/12/09) */
#define FLOAT_cfFZ(UN,LN) DOUBLE_PRECISION FCALLSC_QUALIFIER fcallsc(UN,LN)(void
#else
#define FLOAT_cfFZ(UN,LN) FORTRAN_REAL FCALLSC_QUALIFIER fcallsc(UN,LN)(void
#endif
#ifdef vmsFortran
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(fstring *AS
#else
#ifdef CRAYFortran
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(_fcd     AS
#else
#if  defined(AbsoftUNIXFortran) || defined(AbsoftProFortran)
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(char    *AS
#else
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(char    *AS, unsigned D0
#endif
#endif
#endif
#else
#if ! (defined(FLOATFUNCTIONTYPE)&&defined(ASSIGNFLOAT)&&defined(RETURNFLOAT))
#if defined (f2cFortran) && ! defined (gFortran)
/* f2c/g77 return double from FORTRAN REAL functions. (KMCCARTY, 2005/12/09) */
#define   FLOAT_cfFZ(UN,LN) DOUBLE_PRECISION  FCALLSC_QUALIFIER fcallsc(UN,LN)(
#else
#define   FLOAT_cfFZ(UN,LN) FORTRAN_REAL      FCALLSC_QUALIFIER fcallsc(UN,LN)(
#endif
#else
#define   FLOAT_cfFZ(UN,LN) FLOATFUNCTIONTYPE FCALLSC_QUALIFIER fcallsc(UN,LN)(
#endif
#if defined(vmsFortran) || defined(CRAYFortran) || defined(AbsoftUNIXFortran)
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(AS
#else
#define  STRING_cfFZ(UN,LN) void  FCALLSC_QUALIFIER fcallsc(UN,LN)(AS, D0
#endif
#endif

#define    BYTE_cfF(UN,LN)     BYTE_cfFZ(UN,LN)
#define  DOUBLE_cfF(UN,LN)   DOUBLE_cfFZ(UN,LN)
#ifndef __CF_KnR
#if defined (f2cFortran) && ! defined (gFortran)
/* f2c/g77 return double from FORTRAN REAL functions. (KMCCARTY, 2005/12/09) */
#define   FLOAT_cfF(UN,LN)  DOUBLE_PRECISION FCALLSC_QUALIFIER fcallsc(UN,LN)(
#else
#define   FLOAT_cfF(UN,LN)  FORTRAN_REAL FCALLSC_QUALIFIER fcallsc(UN,LN)(
#endif
#else
#define   FLOAT_cfF(UN,LN)    FLOAT_cfFZ(UN,LN)
#endif
#define     INT_cfF(UN,LN)      INT_cfFZ(UN,LN)
#define LOGICAL_cfF(UN,LN)  LOGICAL_cfFZ(UN,LN)
#define    LONG_cfF(UN,LN)     LONG_cfFZ(UN,LN)
#define LONGLONG_cfF(UN,LN) LONGLONG_cfFZ(UN,LN) /* added by MR December 2005 */
#define   SHORT_cfF(UN,LN)    SHORT_cfFZ(UN,LN)
#define    VOID_cfF(UN,LN)     VOID_cfFZ(UN,LN)
#define  STRING_cfF(UN,LN)   STRING_cfFZ(UN,LN),

#define     INT_cfFF
#define    VOID_cfFF
#ifdef vmsFortran
#define  STRING_cfFF           fstring *AS; 
#else
#ifdef CRAYFortran
#define  STRING_cfFF           _fcd     AS;
#else
#define  STRING_cfFF           char    *AS; unsigned D0;
#endif
#endif

#define     INT_cfL            A0=
#define  STRING_cfL            A0=
#define    VOID_cfL                        

#define    INT_cfK
#define   VOID_cfK
/* KSTRING copies the string into the position provided by the caller. */
#ifdef vmsFortran
#define STRING_cfK                                                             \
 memcpy(AS->dsc$a_pointer,A0,_cfMIN(AS->dsc$w_length,(A0==NULL?0:strlen(A0))));\
 AS->dsc$w_length>(A0==NULL?0:strlen(A0))?                                     \
  memset(AS->dsc$a_pointer+(A0==NULL?0:strlen(A0)),' ',                        \
         AS->dsc$w_length-(A0==NULL?0:strlen(A0))):0;
#else
#ifdef CRAYFortran
#define STRING_cfK                                                             \
 memcpy(_fcdtocp(AS),A0, _cfMIN(_fcdlen(AS),(A0==NULL?0:strlen(A0))) );        \
 _fcdlen(AS)>(A0==NULL?0:strlen(A0))?                                          \
  memset(_fcdtocp(AS)+(A0==NULL?0:strlen(A0)),' ',                             \
         _fcdlen(AS)-(A0==NULL?0:strlen(A0))):0;
#else
#define STRING_cfK         memcpy(AS,A0, _cfMIN(D0,(A0==NULL?0:strlen(A0))) ); \
                 D0>(A0==NULL?0:strlen(A0))?memset(AS+(A0==NULL?0:strlen(A0)), \
                                            ' ', D0-(A0==NULL?0:strlen(A0))):0;
#endif
#endif

/* Note that K.. and I.. can't be combined since K.. has to access data before
R.., in order for functions returning strings which are also passed in as
arguments to work correctly. Note that R.. frees and hence may corrupt the
string. */
#define    BYTE_cfI  return A0;
#define  DOUBLE_cfI  return A0;
#if ! (defined(FLOATFUNCTIONTYPE)&&defined(ASSIGNFLOAT)&&defined(RETURNFLOAT))
#define   FLOAT_cfI  return A0;
#else
#define   FLOAT_cfI  RETURNFLOAT(A0);
#endif
#define     INT_cfI  return A0;
#ifdef hpuxFortran800
/* Incredibly, functions must return true as 1, elsewhere .true.==0x01000000. */
#define LOGICAL_cfI  return ((A0)?1:0);
#else
#define LOGICAL_cfI  return C2FLOGICAL(A0);
#endif
#define    LONG_cfI  return A0;
#define LONGLONG_cfI  return A0; /* added by MR December 2005 */
#define   SHORT_cfI  return A0;
#define  STRING_cfI  return   ;
#define    VOID_cfI  return   ;

#ifdef OLD_VAXC                                  /* Allow %CC-I-PARAMNOTUSED. */
#pragma standard
#endif

#define FCALLSCSUB0( CN,UN,LN)             FCALLSCFUN0(VOID,CN,UN,LN)
#define FCALLSCSUB1( CN,UN,LN,T1)          FCALLSCFUN1(VOID,CN,UN,LN,T1)
#define FCALLSCSUB2( CN,UN,LN,T1,T2)       FCALLSCFUN2(VOID,CN,UN,LN,T1,T2)
#define FCALLSCSUB3( CN,UN,LN,T1,T2,T3)    FCALLSCFUN3(VOID,CN,UN,LN,T1,T2,T3)
#define FCALLSCSUB4( CN,UN,LN,T1,T2,T3,T4) \
    FCALLSCFUN4(VOID,CN,UN,LN,T1,T2,T3,T4)
#define FCALLSCSUB5( CN,UN,LN,T1,T2,T3,T4,T5) \
    FCALLSCFUN5(VOID,CN,UN,LN,T1,T2,T3,T4,T5)
#define FCALLSCSUB6( CN,UN,LN,T1,T2,T3,T4,T5,T6) \
    FCALLSCFUN6(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6)       
#define FCALLSCSUB7( CN,UN,LN,T1,T2,T3,T4,T5,T6,T7) \
    FCALLSCFUN7(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7)
#define FCALLSCSUB8( CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8) \
    FCALLSCFUN8(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8)
#define FCALLSCSUB9( CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9) \
    FCALLSCFUN9(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9)
#define FCALLSCSUB10(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA) \
   FCALLSCFUN10(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA)
#define FCALLSCSUB11(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB) \
   FCALLSCFUN11(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB)
#define FCALLSCSUB12(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC) \
   FCALLSCFUN12(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC)
#define FCALLSCSUB13(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD) \
   FCALLSCFUN13(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD)
#define FCALLSCSUB14(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) \
   FCALLSCFUN14(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)
#define FCALLSCSUB15(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF) \
   FCALLSCFUN15(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF)
#define FCALLSCSUB16(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG) \
   FCALLSCFUN16(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG)
#define FCALLSCSUB17(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH) \
   FCALLSCFUN17(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH)
#define FCALLSCSUB18(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI) \
   FCALLSCFUN18(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI)
#define FCALLSCSUB19(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ) \
   FCALLSCFUN19(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ)
#define FCALLSCSUB20(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
   FCALLSCFUN20(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK)
#define FCALLSCSUB21(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL) \
   FCALLSCFUN21(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL)
#define FCALLSCSUB22(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM) \
   FCALLSCFUN22(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM)
#define FCALLSCSUB23(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN) \
   FCALLSCFUN23(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN)
#define FCALLSCSUB24(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO) \
   FCALLSCFUN24(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO)
#define FCALLSCSUB25(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP) \
   FCALLSCFUN25(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP)
#define FCALLSCSUB26(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ) \
   FCALLSCFUN26(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ)
#define FCALLSCSUB27(CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) \
   FCALLSCFUN27(VOID,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)


#define FCALLSCFUN1( T0,CN,UN,LN,T1) \
        FCALLSCFUN5 (T0,CN,UN,LN,T1,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN2( T0,CN,UN,LN,T1,T2) \
        FCALLSCFUN5 (T0,CN,UN,LN,T1,T2,CF_0,CF_0,CF_0)
#define FCALLSCFUN3( T0,CN,UN,LN,T1,T2,T3) \
        FCALLSCFUN5 (T0,CN,UN,LN,T1,T2,T3,CF_0,CF_0)
#define FCALLSCFUN4( T0,CN,UN,LN,T1,T2,T3,T4) \
        FCALLSCFUN5 (T0,CN,UN,LN,T1,T2,T3,T4,CF_0)
#define FCALLSCFUN5( T0,CN,UN,LN,T1,T2,T3,T4,T5) \
        FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,CF_0,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN6( T0,CN,UN,LN,T1,T2,T3,T4,T5,T6) \
        FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN7( T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7) \
        FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,CF_0,CF_0,CF_0)
#define FCALLSCFUN8( T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8) \
        FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,CF_0,CF_0)
#define FCALLSCFUN9( T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9) \
        FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,CF_0)
#define FCALLSCFUN10(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA) \
        FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN11(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB) \
        FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,CF_0,CF_0,CF_0)
#define FCALLSCFUN12(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC) \
        FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,CF_0,CF_0)
#define FCALLSCFUN13(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD) \
        FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,CF_0)


#define FCALLSCFUN15(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF) \
        FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,CF_0,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN16(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG) \
        FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN17(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH) \
        FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,CF_0,CF_0,CF_0)
#define FCALLSCFUN18(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI) \
        FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,CF_0,CF_0)
#define FCALLSCFUN19(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ) \
        FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,CF_0)
#define FCALLSCFUN20(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN21(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN22(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,CF_0,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN23(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,CF_0,CF_0,CF_0,CF_0)
#define FCALLSCFUN24(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,CF_0,CF_0,CF_0)
#define FCALLSCFUN25(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,CF_0,CF_0)
#define FCALLSCFUN26(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ) \
        FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,CF_0)


#ifndef __CF__KnR
#define FCALLSCFUN0(T0,CN,UN,LN) CFextern _(T0,_cfFZ)(UN,LN) ABSOFT_cf2(T0))   \
        {_Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0) CN(); _Icf(0,K,T0,0,0) _(T0,_cfI)}

#define FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    \
                                 CFextern _(T0,_cfF)(UN,LN)                    \
 CFARGT14(NCF,DCF,ABSOFT_cf2(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE) )  \
 {                 CFARGT14S(QCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    \
  _Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0)      CN(    TCF(LN,T1,1,0)  TCF(LN,T2,2,1) \
    TCF(LN,T3,3,1)  TCF(LN,T4,4,1) TCF(LN,T5,5,1)  TCF(LN,T6,6,1)  TCF(LN,T7,7,1) \
    TCF(LN,T8,8,1)  TCF(LN,T9,9,1) TCF(LN,TA,10,1) TCF(LN,TB,11,1) TCF(LN,TC,12,1) \
    TCF(LN,TD,13,1) TCF(LN,TE,14,1) );                          _Icf(0,K,T0,0,0) \
                   CFARGT14S(RCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)  _(T0,_cfI) }

#define FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)   \
                                 CFextern _(T0,_cfF)(UN,LN)                    \
 CFARGT27(NCF,DCF,ABSOFT_cf2(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR) ) \
 {                 CFARGT27S(QCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)   \
  _Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0)      CN(     TCF(LN,T1,1,0)  TCF(LN,T2,2,1)  \
    TCF(LN,T3,3,1)  TCF(LN,T4,4,1)  TCF(LN,T5,5,1)  TCF(LN,T6,6,1)  TCF(LN,T7,7,1)  \
    TCF(LN,T8,8,1)  TCF(LN,T9,9,1)  TCF(LN,TA,10,1) TCF(LN,TB,11,1) TCF(LN,TC,12,1) \
    TCF(LN,TD,13,1) TCF(LN,TE,14,1) TCF(LN,TF,15,1) TCF(LN,TG,16,1) TCF(LN,TH,17,1) \
    TCF(LN,TI,18,1) TCF(LN,TJ,19,1) TCF(LN,TK,20,1) TCF(LN,TL,21,1) TCF(LN,TM,22,1) \
    TCF(LN,TN,23,1) TCF(LN,TO,24,1) TCF(LN,TP,25,1) TCF(LN,TQ,26,1) TCF(LN,TR,27,1) ); _Icf(0,K,T0,0,0) \
                   CFARGT27S(RCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)  _(T0,_cfI) }

#else
#define FCALLSCFUN0(T0,CN,UN,LN) CFextern _(T0,_cfFZ)(UN,LN) ABSOFT_cf3(T0)) _Icf(0,FF,T0,0,0)\
        {_Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0) CN(); _Icf(0,K,T0,0,0) _(T0,_cfI)}

#define FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    \
                                 CFextern _(T0,_cfF)(UN,LN)                    \
 CFARGT14(NNCF,DDCF,ABSOFT_cf3(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)) _Icf(0,FF,T0,0,0) \
       CFARGT14FS(NNNCF,DDDCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE);   \
 {                 CFARGT14S(QCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    \
  _Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0)      CN(  TCF(LN,T1,1,0) TCF(LN,T2,2,1) \
    TCF(LN,T3,3,1) TCF(LN,T4,4,1) TCF(LN,T5,5,1) TCF(LN,T6,6,1) TCF(LN,T7,7,1) \
    TCF(LN,T8,8,1) TCF(LN,T9,9,1) TCF(LN,TA,10,1) TCF(LN,TB,11,1) TCF(LN,TC,12,1) \
    TCF(LN,TD,13,1) TCF(LN,TE,14,1) );                          _Icf(0,K,T0,0,0) \
                   CFARGT14S(RCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)  _(T0,_cfI)}

#define FCALLSCFUN27(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)  \
                                 CFextern _(T0,_cfF)(UN,LN)                    \
 CFARGT27(NNCF,DDCF,ABSOFT_cf3(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)) _Icf(0,FF,T0,0,0) \
       CFARGT27FS(NNNCF,DDDCF,_Z,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR); \
 {                 CFARGT27S(QCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)  \
  _Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0)      CN(     TCF(LN,T1,1,0)  TCF(LN,T2,2,1)  \
    TCF(LN,T3,3,1)  TCF(LN,T4,4,1)  TCF(LN,T5,5,1)  TCF(LN,T6,6,1)  TCF(LN,T7,7,1)  \
    TCF(LN,T8,8,1)  TCF(LN,T9,9,1)  TCF(LN,TA,10,1) TCF(LN,TB,11,1) TCF(LN,TC,12,1) \
    TCF(LN,TD,13,1) TCF(LN,TE,14,1) TCF(LN,TF,15,1) TCF(LN,TG,16,1) TCF(LN,TH,17,1) \
    TCF(LN,TI,18,1) TCF(LN,TJ,19,1) TCF(LN,TK,20,1) TCF(LN,TL,21,1) TCF(LN,TM,22,1) \
    TCF(LN,TN,23,1) TCF(LN,TO,24,1) TCF(LN,TP,25,1) TCF(LN,TQ,26,1) TCF(LN,TR,27,1) ); _Icf(0,K,T0,0,0) \
                   CFARGT27S(RCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE,TF,TG,TH,TI,TJ,TK,TL,TM,TN,TO,TP,TQ,TR)  _(T0,_cfI)}

#endif


#endif	 /* __CFORTRAN_LOADED */
