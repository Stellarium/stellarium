/************************************************************************

     f77_wrap1.c and f77_wrap2.c have now been split into 4 files to
     prevent compile-time memory errors (from expansion of compiler commands).
     f77_wrap1.c was split into f77_wrap1.c and f77_wrap3.c, and
     f77_wrap2.c was split into f77_wrap2.c and f77_wrap4.c:
       
       f77_wrap1.c contains routines operating on whole files and some 
       utility routines.
     
       f77_wrap2.c contains routines operating on primary array, image, 
       or column elements.

       f77_wrap3.c contains routines operating on headers & keywords.

       f77_wrap4.c contains miscellaneous routines.

     Peter's original comments:

     Together, f77_wrap1.c and f77_wrap2.c contain C wrappers for all
     the CFITSIO routines prototyped in fitsio.h, except for the
     generic datatype routines and features not supported in fortran
     (eg, unsigned integers), a few routines prototyped in fitsio2.h,
     which only a handful of FTOOLS use, plus a few obsolete FITSIO
     routines not present in CFITSIO.  This file allows Fortran code
     to use the CFITSIO library instead of the FITSIO library without
     modification.  It also gives access to new routines not present
     in FITSIO.  Fortran FTOOLS must continue using the old routine
     names from FITSIO (ie, ftxxxx), but most of the C-wrappers simply
     redirect those calls to the corresponding CFITSIO routines (ie,
     ffxxxx), with appropriate parameter massaging where necessary.
     The main exception are read/write routines ending in j (ie, long
     data) which get redirected to C routines ending in k (ie, int
     data). This is more consistent with the default integer type in
     Fortran. f77_wrap1.c primarily holds routines operating on whole
     files and extension headers.  f77_wrap2.c handle routines which
     read and write the data portion, plus miscellaneous extra routines.
     
        File created by Peter Wilson (HSTX), Oct-Dec. 1997
************************************************************************/

#include "fitsio2.h"
#include "f77_wrap.h"

/*********************************************************************/
/*                     Iterator Functions                            */
/*********************************************************************/

/* Use a simple ellipse prototype for Fwork_fn to satisfy finicky compilers */
typedef struct {
   void *userData;
   void (*Fwork_fn)(PLONG_cfTYPE *total_n, ...);
} FtnUserData;

/*        Declare protoypes to make C++ happy       */
int Cwork_fn(long, long, long, long, int, iteratorCol *, void *);
void Cffiter( int n_cols, int *units, int *colnum, char *colname[], 
	      int *datatype, int *iotype,
              long offset, long n_per_loop, void *Fwork_fn,
	      void *userData, int *status);

/******************************************************************/
/*  Cffiter is the wrapper for CFITSIO's ffiter which takes most  */
/*  of its arguments via a structure, iteratorCol.  This routine  */
/*  takes a list of arrays and converts them into a single array  */
/*  of type iteratorCol and passes it to CFITSIO.  Because ffiter */
/*  will be passing control to a Fortran work function, the C     */
/*  wrapper, Cwork_fn, must be passed in its place which then     */
/*  calls the Fortran routine after the necessary data            */
/*  manipulation.  The Fortran routine is passed via the user-    */
/*  supplied parameter pointer.                                   */
/******************************************************************/

void Cffiter( int n_cols, int *units, int *colnum, char *colname[], 
	      int *datatype, int *iotype,
              long offset, long n_per_loop, void *Fwork_fn,
	      void *userData, int *status)
{
   iteratorCol *cols;
   int i;
   FtnUserData FuserData;

   FuserData.Fwork_fn = (void(*)(PLONG_cfTYPE *,...))Fwork_fn;
   FuserData.userData = userData;

   cols = (iteratorCol *)malloc( n_cols*sizeof(iteratorCol) );
   if( cols==NULL ) {
      *status = MEMORY_ALLOCATION;
      return;
   }
   for(i=0;i<n_cols;i++) {
      cols[i].fptr     = gFitsFiles[ units[i] ];
      cols[i].colnum   = colnum[i];
      strncpy(cols[i].colname,colname[i],70);
      cols[i].datatype = datatype[i];
      cols[i].iotype   = iotype[i];
   }

   ffiter( n_cols, cols, offset, n_per_loop, Cwork_fn, 
	   (void*)&FuserData, status );
   free(cols);
}
#define ftiter_STRV_A4 NUM_ELEM_ARG(1)
FCALLSCSUB11(Cffiter,FTITER,ftiter,INT,INTV,INTV,STRINGV,INTV,INTV,LONG,LONG,PVOID,PVOID,PINT)

/*-----------------------------------------------------------------*/
/*  This function is called by CFITSIO's ffiter and serves as the  */
/*  wrapper for the Fortran work function which is passed in the   */
/*  extra user-supplied pointer.  It breaks up C's iteratorCol     */
/*  into several separate arrays.  Because we cannot send an       */
/*  array of pointers for the column data, we instead send *many*  */
/*  arrays as final parameters.                                    */
/*-----------------------------------------------------------------*/

int Cwork_fn( long total_n, long offset,       long first_n,    long n_values,
	      int n_cols,   iteratorCol *cols, void *FuserData )
{
   int  *units, *colnum, *datatype, *iotype, *repeat;
   char **sptr;
   void **ptrs;
   int  i,j,k,nstr,status=0;
   long *slen;

#ifdef vmsFortran
   /*  Passing strings under VMS require a special structure  */
   fstringvector *vmsStrs;
#endif

   /*  Allocate memory for all the arrays.  Grab all the int's  */
   /*  at once and divide up among parameters                   */

   ptrs  = (void**)malloc(2*n_cols*sizeof(void*));
   if( ptrs==NULL )
      return( MEMORY_ALLOCATION );
   units = (int*)malloc(5*n_cols*sizeof(int));
   if( units==NULL ) {
      free(ptrs);
      return( MEMORY_ALLOCATION );
   }
   colnum   = units + 1 * n_cols;
   datatype = units + 2 * n_cols;
   iotype   = units + 3 * n_cols;
   repeat   = units + 4 * n_cols;

   nstr = 0;
   slen = (long*)(ptrs+n_cols);
#ifdef vmsFortran
   vmsStrs = (fstringvector *)calloc(sizeof(fstringvector),n_cols);
   if( vmsStrs==NULL ) {
      free(ptrs);
      free(units);
      return( MEMORY_ALLOCATION );
   }
#endif

   for(i=0;i<n_cols;i++) {
      for(j=0;j<NMAXFILES;j++)
	 if( cols[i].fptr==gFitsFiles[j] )
	    units[i] = j;
      colnum[i]   = cols[i].colnum;
      datatype[i] = cols[i].datatype;
      iotype[i]   = cols[i].iotype;
      repeat[i]   = cols[i].repeat;

      if( datatype[i]==TLOGICAL ) {
	 /*  Don't forget first element is null value  */
	 ptrs[i] = (void *)malloc( (n_values*repeat[i]+1)*4 );
	 if( ptrs[i]==NULL ) {
	    free(ptrs);
	    free(units);
	    return( MEMORY_ALLOCATION );
	 }
	 for( j=0;j<=n_values*repeat[i]; j++ )
	    ((int*)ptrs[i])[j] = C2FLOGICAL( ((char*)cols[i].array)[j]);
      } else if ( datatype[i]==TSTRING ) {
	 sptr = (char**)cols[i].array;
	 slen[nstr] = sptr[1] - sptr[0];
	 for(j=0;j<=n_values;j++)
	    for(k=strlen( sptr[j] );k<slen[nstr];k++)
	       sptr[j][k] = ' ';
#ifdef vmsFortran
	 vmsStrs[nstr].dsc$a_pointer         = sptr[0];
	 vmsStrs[nstr].dsc$w_length          = slen[nstr];
	 vmsStrs[nstr].dsc$l_m[0]            = n_values+1;
	 vmsStrs[nstr].dsc$l_arsize          = slen[nstr] * (n_values+1);
	 vmsStrs[nstr].dsc$bounds[0].dsc$l_u = n_values+1;
	 vmsStrs[nstr].dsc$a_a0              = sptr[0] - slen[nstr];
	 ptrs[i] = (void *)(vmsStrs+nstr);
#else
	 ptrs[i] = (void *)sptr[0];
#endif
	 nstr++;
      } else
	 ptrs[i] = (void *)cols[i].array;
   }

   if(!status) {
              /*  Handle Fortran function call manually...  */
	      /*  cfortran.h cannot handle all the desired  */
              /*  'ptrs' nor the indirect function call.    */

      PLONG_cfTYPE a1,a2,a3,a4;    /* Do this in case longs are */ 
      FtnUserData *f;              /* not the same size as ints */

      a1 = total_n;
      a2 = offset;
      a3 = first_n;
      a4 = n_values;
      f  = (FtnUserData *)FuserData;

      f->Fwork_fn(&a1,&a2,&a3,&a4,&n_cols,units,colnum,datatype,
		  iotype,repeat,&status,f->userData,
		  ptrs[ 0], ptrs[ 1], ptrs[ 2], ptrs[ 3], ptrs[ 4],
		  ptrs[ 5], ptrs[ 6], ptrs[ 7], ptrs[ 8], ptrs[ 9],
		  ptrs[10], ptrs[11], ptrs[12], ptrs[13], ptrs[14],
		  ptrs[15], ptrs[16], ptrs[17], ptrs[18], ptrs[19],
		  ptrs[20], ptrs[21], ptrs[22], ptrs[23], ptrs[24] );
   }

   /*  Check whether there are any LOGICAL or STRING columns being outputted  */
   nstr=0;
   for( i=0;i<n_cols;i++ ) {
      if( iotype[i]!=InputCol ) {
	 if( datatype[i]==TLOGICAL ) {
	    for( j=0;j<=n_values*repeat[i];j++ )
	       ((char*)cols[i].array)[j] = F2CLOGICAL( ((int*)ptrs[i])[j] );
	    free(ptrs[i]);
	 } else if( datatype[i]==TSTRING ) {
	    for( j=0;j<=n_values;j++ )
	       ((char**)cols[i].array)[j][slen[nstr]-1] = '\0';
	 }
      }
      if( datatype[i]==TSTRING ) nstr++;
   }

   free(ptrs);
   free(units);
#ifdef vmsFortran
   free(vmsStrs);
#endif
   return(status);
}


/*--------------------- WCS Utilities ----------------------------*/
FCALLSCSUB10(ffgics, FTGICS, ftgics, FITSUNIT,     PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PSTRING,PINT)
FCALLSCSUB11(ffgicsa,FTGICSA,ftgicsa,FITSUNIT,BYTE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PSTRING,PINT)
FCALLSCSUB12(ffgtcs,FTGTCS,ftgtcs,FITSUNIT,INT,INT,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PDOUBLE,PSTRING,PINT)
FCALLSCSUB13(ffwldp,FTWLDP,ftwldp,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,STRING,PDOUBLE,PDOUBLE,PINT)
FCALLSCSUB13(ffxypx,FTXYPX,ftxypx,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,DOUBLE,STRING,PDOUBLE,PDOUBLE,PINT)

/*------------------- Conversion Utilities -----------------*/
/*                 (prototyped in fitsio2.h)                */
/*----------------------------------------------------------*/

CFextern VOID_cfF(FTI2C,fti2c)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),LONG,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTI2C,fti2c)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),LONG,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(LONG,1)
   QCF(PSTRING,2)
   QCF(PINT,3)
   char str[21];

   ffi2c( TCF(fti2c,LONG,1,0) 
          TCF(fti2c,PSTRING,2,1) 
          TCF(fti2c,PINT,3,1) );

   sprintf(str,"%20s",B2);
   strcpy(B2,str);

   RCF(LONG,1)
   RCF(PSTRING,2)
   RCF(PINT,3)
}

CFextern VOID_cfF(FTL2C,ftl2c)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),LOGICAL,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTL2C,ftl2c)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),LOGICAL,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(LOGICAL,1)
   QCF(PSTRING,2)
   QCF(PINT,3)
   char str[21];

   ffl2c( TCF(ftl2c,LOGICAL,1,0) 
          TCF(ftl2c,PSTRING,2,1) 
          TCF(ftl2c,PINT,3,1) );

   sprintf(str,"%20s",B2);
   strcpy(B2,str);

   RCF(LOGICAL,1)
   RCF(PSTRING,2)
   RCF(PINT,3)
}

FCALLSCSUB3(ffs2c,FTS2C,fts2c,STRING,PSTRING,PINT)

CFextern VOID_cfF(FTR2F,ftr2f)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FLOAT,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTR2F,ftr2f)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FLOAT,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(FLOAT,1)
   QCF(INT,2)
   QCF(PSTRING,3)
   QCF(PINT,4)
   char str[21];

   ffr2f( TCF(ftr2f,FLOAT,1,0) 
          TCF(ftr2f,INT,2,1) 
          TCF(ftr2f,PSTRING,3,1) 
          TCF(ftr2f,PINT,4,1) );

   sprintf(str,"%20s",B3);
   strcpy(B3,str);

   RCF(FLOAT,1)
   RCF(INT,2)
   RCF(PSTRING,3)
   RCF(PINT,4)
}

CFextern VOID_cfF(FTR2E,ftr2e)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FLOAT,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTR2E,ftr2e)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FLOAT,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(FLOAT,1)
   QCF(INT,2)
   QCF(PSTRING,3)
   QCF(PINT,4)
   char str[21];

   ffr2e( TCF(ftr2e,FLOAT,1,0) 
          TCF(ftr2e,INT,2,1) 
          TCF(ftr2e,PSTRING,3,1) 
          TCF(ftr2e,PINT,4,1) );

   sprintf(str,"%20s",B3);
   strcpy(B3,str);

   RCF(FLOAT,1)
   RCF(INT,2)
   RCF(PSTRING,3)
   RCF(PINT,4)
}

CFextern VOID_cfF(FTD2F,ftd2f)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),DOUBLE,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTD2F,ftd2f)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),DOUBLE,INT,PSTRING,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(DOUBLE,1)
   QCF(INT,2)
   QCF(PSTRING,3)
   QCF(PINT,4)
   char str[21];

   ffd2f( TCF(ftd2f,DOUBLE,1,0) 
          TCF(ftd2f,INT,2,1) 
          TCF(ftd2f,PSTRING,3,1) 
          TCF(ftd2f,PINT,4,1) );

   sprintf(str,"%20s",B3);
   strcpy(B3,str);

   RCF(DOUBLE,1)
   RCF(INT,2)
   RCF(PSTRING,3)
   RCF(PINT,4)
}

CFextern VOID_cfF(FTD2E,ftd2e)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),DOUBLE,INT,PSTRING,PINT,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTD2E,ftd2e)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),DOUBLE,INT,PSTRING,PINT,PINT,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(DOUBLE,1)
   QCF(INT,2)
   QCF(PSTRING,3)
   QCF(PINT,4)
   QCF(PINT,5)
   char str[21];
   int *vlen;

   vlen = TCF(ftd2e,PINT,4,0);

   /*  C version of routine doesn't use the 4th parameter, vlen  */
   ffd2e( TCF(ftd2e,DOUBLE,1,0) 
          TCF(ftd2e,INT,2,1) 
          TCF(ftd2e,PSTRING,3,1) 
          TCF(ftd2e,PINT,5,1) );

   *vlen = strlen(B3);
   if ( *vlen<20 ) {
      sprintf(str,"%20s",B3);  /* right justify if vlen<20 characters */
      strcpy(B3,str);
      *vlen = 20;
   }

   RCF(DOUBLE,1)
   RCF(INT,2)
   RCF(PSTRING,3)
   RCF(PINT,4)
   RCF(PINT,5)
}

FCALLSCSUB3(ffc2ii,FTC2II,ftc2ii,STRING,PLONG,PINT)
FCALLSCSUB3(ffc2ll,FTC2LL,ftc2ll,STRING,PINT,PINT)
FCALLSCSUB3(ffc2rr,FTC2RR,ftc2rr,STRING,PFLOAT,PINT)
FCALLSCSUB3(ffc2dd,FTC2DD,ftc2dd,STRING,PDOUBLE,PINT)
FCALLSCSUB7(ffc2x,FTC2X,ftc2x,STRING,PSTRING,PLONG,PINT,PSTRING,PDOUBLE,PINT)
FCALLSCSUB3(ffc2s,FTC2S,ftc2s,STRING,PSTRING,PINT)
FCALLSCSUB3(ffc2i,FTC2I,ftc2i,STRING,PLONG,PINT)
FCALLSCSUB3(ffc2r,FTC2R,ftc2r,STRING,PFLOAT,PINT)
FCALLSCSUB3(ffc2d,FTC2D,ftc2d,STRING,PDOUBLE,PINT)
FCALLSCSUB3(ffc2l,FTC2L,ftc2l,STRING,PINT,PINT)

/*------------------ Byte-level read/seek/write -----------------*/
/*                   (prototyped in fitsio2.h)                   */
/*---------------------------------------------------------------*/

/*
  ffmbyt should not be called by any application programs, so
  the wrapper should not need to be defined.  If it is needed then
  the second parameter (LONG) will need to be changed to the
  equivalent of the C 'off_t' type, which may be 32 or 64 bits long
  depending on the compiler.
  -W.Pence (7/21/00)
  
FCALLSCSUB4(ffmbyt,FTMBYT,ftmbyt,FITSUNIT,LONG,LOGICAL,PINT)
*/

FCALLSCSUB4(ffgbyt,FTGCBF,ftgcbf,FITSUNIT,LONG,PVOID,PINT)
FCALLSCSUB4(ffgbyt,FTGBYT,ftgbyt,FITSUNIT,LONG,PVOID,PINT)

FCALLSCSUB4(ffpbyt,FTPCBF,ftpcbf,FITSUNIT,LONG,PVOID,PINT)
FCALLSCSUB4(ffpbyt,FTPBYT,ftpbyt,FITSUNIT,LONG,PVOID,PINT)


/*-------------- Additional missing FITSIO routines -------------*/
/*                   (abandoned in CFITSIO)                      */
/*---------------------------------------------------------------*/

void Cffcrep( char *comm, char *comm1, int *repeat );
void Cffcrep( char *comm, char *comm1, int *repeat )
{
/*
   check if the first comment string is to be repeated for all keywords
   (if the last non-blank character is '&', then it is to be repeated)

   comm    input comment string
   OUTPUT PARAMETERS:
   comm1   output comment string, = COMM minus the last '&' character
   repeat  TRUE if the last character of COMM was the '&' character

   written by Wm Pence, HEASARC/GSFC, June 1991
   translated to C by Peter Wilson, HSTX/GSFC, Oct 1997
*/

   int len;

   *repeat=FALSE;
   len=strlen(comm);
       /* cfortran strips trailing spaces so only check last character  */
   if( len && comm[ len-1 ]=='&' ) {
      strncpy(comm1,comm,len-1);  /*  Don't copy '&'  */
      comm1[len-1]='\0';
      *repeat=TRUE;
   }
   return;
}
FCALLSCSUB3(Cffcrep,FTCREP,ftcrep,STRING,PSTRING,PLOGICAL)


/*------------------ Test floats for NAN values -----------------*/
/*                     (defined in fitsio2.h)                    */
/*---------------------------------------------------------------*/

int Cfnan( float *val );
int Cfnan( float *val )
{
   int code;

#if BYTESWAPPED
   short *sptr = (short*)val + 1;
#else
   short *sptr = (short*)val;
#endif

   code = fnan(*sptr);
   if( code==2 ) *val = 0.0;   /* Underflow */

   return( code!=0 );
}
FCALLSCFUN1(LOGICAL,Cfnan,FTTRNN,fttrnn,PFLOAT)


int Cdnan( double *val );
int Cdnan( double *val )
{
   int code;

#if BYTESWAPPED
   short *sptr = (short*)val + 3;
#else
   short *sptr = (short*)val;
#endif

   code = dnan(*sptr);
   if( code==2 ) *val = 0.0;   /* Underflow */

   return( code!=0 );
}
FCALLSCFUN1(LOGICAL,Cdnan,FTTDNN,fttdnn,PDOUBLE)

/*-------- Functions no longer supported... normally redundant -----------*/
/*                Included only to support older code                     */
/*------------------------------------------------------------------------*/

void Cffempty(void);
void Cffempty(void)
{ return; }
FCALLSCSUB0(Cffempty,FTPDEF,ftpdef)
FCALLSCSUB0(Cffempty,FTBDEF,ftbdef)
FCALLSCSUB0(Cffempty,FTADEF,ftadef)
FCALLSCSUB0(Cffempty,FTDDEF,ftddef)


/*-------- Functions which use the lex and yacc/bison parser code -----------*/
/*---------------------------------------------------------------------------*/

#define fttexp_LONGV_A7 A3
FCALLSCSUB8(fftexp,FTTEXP,fttexp,FITSUNIT,STRING,INT,PINT,PLONG,PINT,LONGV,PINT)

#define ftfrow_LOGV_A6 A4
FCALLSCSUB7(fffrow,FTFROW,ftfrow,FITSUNIT,STRING,LONG,LONG,PLONG,LOGICALV,PINT)

#define ftfrwc_LOGV_A8 A6
FCALLSCSUB9(fffrwc,FTFRWC,ftfrwc,FITSUNIT,STRING,STRING,STRING,STRING,LONG,DOUBLEV,LOGICALV,PINT)
FCALLSCSUB4(ffffrw,FTFFRW,ftffrw,FITSUNIT,STRING,PLONG,PINT)

FCALLSCSUB4(ffsrow,FTSROW,ftsrow,FITSUNIT,FITSUNIT,STRING,PINT)
FCALLSCSUB9(ffcrow,FTCROW,ftcrow,FITSUNIT,INT,STRING,LONG,LONG,PVOID,PVOID,PLOGICAL,PINT)
FCALLSCSUB6(ffcalc,FTCALC,ftcalc,FITSUNIT,STRING,FITSUNIT,STRING,STRING,PINT)

#define ftcalc_rng_LONGV_A7 A6
#define ftcalc_rng_LONGV_A8 A6
FCALLSCSUB9(ffcalc_rng,FTCALC_RNG,ftcalc_rng,FITSUNIT,STRING,FITSUNIT,STRING,STRING,INT,LONGV,LONGV,PINT)

/*--------------------- grouping routines ------------------*/

FCALLSCSUB4(ffgtcr,FTGTCR,ftgtcr,FITSUNIT,STRING,INT,PINT)
FCALLSCSUB4(ffgtis,FTGTIS,ftgtis,FITSUNIT,STRING,INT,PINT)
FCALLSCSUB3(ffgtch,FTGTCH,ftgtch,FITSUNIT,INT,PINT)
FCALLSCSUB3(ffgtrm,FTGTRM,ftgtrm,FITSUNIT,INT,PINT)
FCALLSCSUB4(ffgtcp,FTGTCP,ftgtcp,FITSUNIT,FITSUNIT,INT,PINT)
FCALLSCSUB4(ffgtmg,FTGTMG,ftgtmg,FITSUNIT,FITSUNIT,INT,PINT)
FCALLSCSUB3(ffgtcm,FTGTCM,ftgtcm,FITSUNIT,INT,PINT)
FCALLSCSUB3(ffgtvf,FTGTVF,ftgtvf,FITSUNIT,PLONG,PINT)
FCALLSCSUB4(ffgtop,FTGTOP,ftgtop,FITSUNIT,INT,PFITSUNIT,PINT)
FCALLSCSUB4(ffgtam,FTGTAM,ftgtam,FITSUNIT,FITSUNIT,INT,PINT)
FCALLSCSUB3(ffgtnm,FTGTNM,ftgtnm,FITSUNIT,PLONG,PINT)
FCALLSCSUB3(ffgmng,FTGMNG,ftgmng,FITSUNIT,PLONG,PINT)
FCALLSCSUB4(ffgmop,FTGMOP,ftgmop,FITSUNIT,LONG,PFITSUNIT,PINT)
FCALLSCSUB5(ffgmcp,FTGMCP,ftgmcp,FITSUNIT,FITSUNIT,LONG,INT,PINT)
FCALLSCSUB5(ffgmtf,FTGMTF,ftgmtf,FITSUNIT,FITSUNIT,LONG,INT,PINT)
FCALLSCSUB4(ffgmrm,FTGMRM,ftgmrm,FITSUNIT,LONG,INT,PINT)
