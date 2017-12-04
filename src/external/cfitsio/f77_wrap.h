#define UNSIGNED_BYTE

#include "cfortran.h"

/************************************************************************
   Some platforms creates longs as 8-byte integers.  On other machines, ints
   and longs are both 4-bytes, so both are compatible with Fortrans
   default integer which is 4-bytes.  To support 8-byte longs, we must redefine
   LONGs and convert them to 8-bytes when going to C, and restore them
   to 4-bytes when returning to Fortran.  Ugh!!!
*************************************************************************/

#if defined(DECFortran) || (defined(__alpha) && defined(g77Fortran)) \
    || (defined(mipsFortran)  && _MIPS_SZLONG==64) \
    || (defined(IBMR2Fortran) && defined(__64BIT__)) \
    ||  defined(__ia64__)  \
    ||  defined (__sparcv9) || (defined(__sparc__) && defined(__arch64__)) \
    ||  defined (__x86_64__) \
    ||  defined (_SX) \
    ||  defined (__powerpc64__)\
    ||  defined (__s390x__)

#define   LONG8BYTES_INT4BYTES

#undef LONGV_cfSTR
#undef PLONG_cfSTR
#undef LONGVVVVVVV_cfTYPE
#undef PLONG_cfTYPE
#undef LONGV_cfT
#undef PLONG_cfT

#define    LONGV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,LONGV,A,B,C,D,E)
#define    PLONG_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,PLONG,A,B,C,D,E)
#define    LONGVVVVVVV_cfTYPE    int
#define    PLONG_cfTYPE          int
#define    LONGV_cfQ(B)          long *B, _(B,N);
#define    PLONG_cfQ(B)          long B;
#define    LONGV_cfT(M,I,A,B,D)  ( (_(B,N) = * _3(M,_LONGV_A,I)), \
				    B = F2Clongv(_(B,N),A) )
#define    PLONG_cfT(M,I,A,B,D)  ((B=*A),&B)
#define    LONGV_cfR(A,B,D)      C2Flongv(_(B,N),A,B);
#define    PLONG_cfR(A,B,D)      *A=B;
#define    LONGV_cfH(S,U,B)
#define    PLONG_cfH(S,U,B)

static long *F2Clongv(long size, int *A)
{
  long i;
  long *B;

  B=(long *)malloc( size*sizeof(long) );
  for(i=0;i<size;i++) B[i]=A[i];
  return(B);
}

static void C2Flongv(long size, int *A, long *B)
{
  long i;

  for(i=0;i<size;i++) A[i]=B[i];
  free(B);
}

#endif

/************************************************************************
   Modify cfortran.h's handling of strings.  C interprets a "char **"
   parameter as an array of pointers to the strings (or as a handle),
   not as a pointer to a block of contiguous strings.  Also set a
   a minimum length for string allocations, to minimize risk of
   overflow.
*************************************************************************/

extern unsigned long gMinStrLen;

#undef  STRINGV_cfQ
#undef  STRINGV_cfR
#undef  TTSTR
#undef  TTTTSTRV
#undef  RRRRPSTRV

#undef  PPSTRING_cfT

#ifdef vmsFortran
#define       PPSTRING_cfT(M,I,A,B,D)     (unsigned char*)A->dsc$a_pointer

/*  We want single strings to be equivalent to string vectors with  */
/*  a single element, so ignore the number of elements info in the  */
/*  vector structure, and rely on the NUM_ELEM definitions.         */

#undef  STRINGV_cfT
#define STRINGV_cfT(M,I,A,B,D)  TTTTSTRV(A->dsc$a_pointer, B, \
                                         A->dsc$w_length, \
                                         num_elem(A->dsc$a_pointer, \
                                                  A->dsc$w_length, \
                                                  _3(M,_STRV_A,I) ) )
#else
#ifdef CRAYFortran
#define       PPSTRING_cfT(M,I,A,B,D)     (unsigned char*)_fcdtocp(A)
#else
#define       PPSTRING_cfT(M,I,A,B,D)     (unsigned char*)A
#endif
#endif

#define _cfMAX(A,B)  ( (A>B) ? A : B )
#define  STRINGV_cfQ(B)      char **B; unsigned int _(B,N), _(B,M);
#define  STRINGV_cfR(A,B,D)  free(B[0]); free(B);
#define  TTSTR(    A,B,D)  \
            ((B=(char*)malloc(_cfMAX(D,gMinStrLen)+1))[D]='\0',memcpy(B,A,D), \
               kill_trailing(B,' '))
#define  TTTTSTRV( A,B,D,E)  ( \
            _(B,N)=_cfMAX(E,1), \
            _(B,M)=_cfMAX(D,gMinStrLen)+1, \
            B=(char**)malloc(_(B,N)*sizeof(char*)), \
            B[0]=(char*)malloc(_(B,N)*_(B,M)), \
            vindex(B,_(B,M),_(B,N),f2cstrv2(A,B[0],D,_(B,M),_(B,N))) \
            )
#define  RRRRPSTRV(A,B,D)    \
            c2fstrv2(B[0],A,_(B,M),D,_(B,N)), \
            free(B[0]), \
            free(B);

static char **vindex(char **B, int elem_len, int nelem, char *B0)
{
   int i;
   if( nelem )
      for( i=0;i<nelem;i++ ) B[i] = B0+i*elem_len;
   return B;
}

static char *c2fstrv2(char* cstr, char *fstr, int celem_len, int felem_len,
               int nelem)
{
   int i,j;

   if( nelem )
      for (i=0; i<nelem; i++) {
	 for (j=0; j<felem_len && *cstr; j++) *fstr++ = *cstr++;
	 cstr += celem_len-j;
	 for (; j<felem_len; j++) *fstr++ = ' ';
      }
   return( fstr-felem_len*nelem );
}

static char *f2cstrv2(char *fstr, char* cstr, int felem_len, int celem_len,
               int nelem)
{
   int i,j;

   if( nelem )
      for (i=0; i<nelem; i++, cstr+=(celem_len-felem_len)) {
	 for (j=0; j<felem_len; j++) *cstr++ = *fstr++;
	 *cstr='\0';
	 kill_trailingn( cstr-felem_len, ' ', cstr );
      }
   return( cstr-celem_len*nelem );
}

/************************************************************************
   The following definitions redefine the BYTE data type to be
   interpretted as a character*1 string instead of an integer*1 which
   is not supported by all compilers.
*************************************************************************/

#undef   BYTE_cfT
#undef   BYTEV_cfT
#undef   BYTE_cfINT
#undef   BYTEV_cfINT
#undef   BYTE_cfSTR
#undef   BYTEV_cfSTR

#define   BYTE_cfINT(N,A,B,X,Y,Z)      _(CFARGS,N)(A,BYTE,B,X,Y,Z,0)
#define   BYTEV_cfINT(N,A,B,X,Y,Z)     _(CFARGS,N)(A,BYTEV,B,X,Y,Z,0)
#define   BYTE_cfSTR(N,T,A,B,C,D,E)    _(CFARGS,N)(T,BYTE,A,B,C,D,E)
#define   BYTEV_cfSTR(N,T,A,B,C,D,E)   _(CFARGS,N)(T,BYTEV,A,B,C,D,E)
#define   BYTE_cfSEP(T,B)              INT_cfSEP(T,B)
#define   BYTEV_cfSEP(T,B)             INT_cfSEP(T,B)
#define   BYTE_cfH(S,U,B)              STRING_cfH(S,U,B)
#define   BYTEV_cfH(S,U,B)             STRING_cfH(S,U,B)
#define   BYTE_cfQ(B)
#define   BYTEV_cfQ(B)
#define   BYTE_cfR(A,B,D)
#define   BYTEV_cfR(A,B,D)

#ifdef vmsFortran
#define   BYTE_cfN(T,A)           fstring * A
#define   BYTEV_cfN(T,A)          fstringvector * A
#define   BYTE_cfT(M,I,A,B,D)     (INTEGER_BYTE)((A->dsc$a_pointer)[0])
#define   BYTEV_cfT(M,I,A,B,D)    (INTEGER_BYTE*)A->dsc$a_pointer
#else
#ifdef CRAYFortran
#define   BYTE_cfN(T,A)           _fcd A
#define   BYTEV_cfN(T,A)          _fcd A
#define   BYTE_cfT(M,I,A,B,D)     (INTEGER_BYTE)((_fcdtocp(A))[0])
#define   BYTEV_cfT(M,I,A,B,D)    (INTEGER_BYTE*)_fcdtocp(A)
#else
#define   BYTE_cfN(T,A)           INTEGER_BYTE * A
#define   BYTEV_cfN(T,A)          INTEGER_BYTE * A
#define   BYTE_cfT(M,I,A,B,D)     A[0]
#define   BYTEV_cfT(M,I,A,B,D)    A
#endif
#endif

/************************************************************************
   The following definitions and functions handle conversions between
   C and Fortran arrays of LOGICALS.  Individually, LOGICALS are
   treated as int's but as char's when in an array.  cfortran defines
   (F2C/C2F)LOGICALV but never uses them, so these routines also
   handle TRUE/FALSE conversions.
*************************************************************************/

#undef  LOGICALV_cfSTR
#undef  LOGICALV_cfT
#define LOGICALV_cfSTR(N,T,A,B,C,D,E) _(CFARGS,N)(T,LOGICALV,A,B,C,D,E)
#define LOGICALV_cfQ(B)               char *B; unsigned int _(B,N);
#define LOGICALV_cfT(M,I,A,B,D)       (_(B,N)= * _3(M,_LOGV_A,I), \
                                            B=F2CcopyLogVect(_(B,N),A))
#define LOGICALV_cfR(A,B,D)           C2FcopyLogVect(_(B,N),A,B);
#define LOGICALV_cfH(S,U,B)

static char *F2CcopyLogVect(long size, int *A)
{
   long i;
   char *B;

   B=(char *)malloc(size*sizeof(char));
   for( i=0; i<size; i++ ) B[i]=F2CLOGICAL(A[i]);
   return(B);
}

static void C2FcopyLogVect(long size, int *A, char *B)
{
   long i;

   for( i=0; i<size; i++ ) A[i]=C2FLOGICAL(B[i]);
   free(B);
}

/*------------------  Fortran File Handling  ----------------------*/
/*  Fortran uses unit numbers, whereas C uses file pointers, so    */
/*  a global array of file pointers is setup in which Fortran's    */
/*  unit number serves as the index.  Two FITSIO routines are      */
/*  the integer unit number and the fitsfile file pointer.         */
/*-----------------------------------------------------------------*/

extern fitsfile *gFitsFiles[];       /*    by Fortran unit numbers       */

#define  FITSUNIT_cfINT(N,A,B,X,Y,Z)   INT_cfINT(N,A,B,X,Y,Z)
#define  FITSUNIT_cfSTR(N,T,A,B,C,D,E) INT_cfSTR(N,T,A,B,C,D,E)
#define  FITSUNIT_cfT(M,I,A,B,D)       gFitsFiles[*A]
#define  FITSUNITVVVVVVV_cfTYPE        int
#define PFITSUNIT_cfINT(N,A,B,X,Y,Z)   PINT_cfINT(N,A,B,X,Y,Z)
#define PFITSUNIT_cfSTR(N,T,A,B,C,D,E) PINT_cfSTR(N,T,A,B,C,D,E)
#define PFITSUNIT_cfT(M,I,A,B,D)       (gFitsFiles + *A)
#define PFITSUNIT_cfTYPE               int


/*---------------------- Make C++ Happy -----------------------------*/
/* Redefine FCALLSCFUNn so that they create prototypes of themselves */
/*   and change TTTTSTR to use (char *)0 instead of NULL             */
/*-------------------------------------------------------------------*/

#undef FCALLSCFUN0
#undef FCALLSCFUN14
#undef TTTTSTR

#define TTTTSTR(A,B,D)   ( !(D<4||A[0]||A[1]||A[2]||A[3]) ) ? ((char*)0) :     \
                             memchr(A,'\0',D) ? A : TTSTR(A,B,D)

#define FCALLSCFUN0(T0,CN,UN,LN) \
  CFextern _(T0,_cfFZ)(UN,LN) void ABSOFT_cf2(T0)); \
  CFextern _(T0,_cfFZ)(UN,LN) void ABSOFT_cf2(T0))  \
  {_Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0) CN(); _Icf(0,K,T0,0,0) _(T0,_cfI)}

#define FCALLSCFUN14(T0,CN,UN,LN,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)    \
  CFextern _(T0,_cfF)(UN,LN)                                                   \
  CFARGT14(NCF,DCF,ABSOFT_cf2(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)); \
  CFextern _(T0,_cfF)(UN,LN)                                                   \
  CFARGT14(NCF,DCF,ABSOFT_cf2(T0),T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE))  \
  {                 CFARGT14S(QCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)   \
  _Icf(2,UU,T0,A0,0); _Icf(0,L,T0,0,0)      CN(  TCF(LN,T1,1,0) TCF(LN,T2,2,1) \
    TCF(LN,T3,3,1) TCF(LN,T4,4,1) TCF(LN,T5,5,1) TCF(LN,T6,6,1) TCF(LN,T7,7,1) \
    TCF(LN,T8,8,1) TCF(LN,T9,9,1) TCF(LN,TA,10,1) TCF(LN,TB,11,1) TCF(LN,TC,12,1) \
    TCF(LN,TD,13,1) TCF(LN,TE,14,1) );                          _Icf(0,K,T0,0,0) \
    CFARGT14S(RCF,T1,T2,T3,T4,T5,T6,T7,T8,T9,TA,TB,TC,TD,TE)        _(T0,_cfI) \
  }

