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


FCALLSCSUB5(ffgextn,FTGEXTN,ftgextn,FITSUNIT,LONG,LONG,BYTEV,PINT)
FCALLSCSUB5(ffpextn,FTPEXTN,ftpextn,FITSUNIT,LONG,LONG,BYTEV,PINT)

/*------------ read primary array or image elements -------------*/
FCALLSCSUB8(ffgpvb,FTGPVB,ftgpvb,FITSUNIT,LONG,LONG,LONG,BYTE,BYTEV,PLOGICAL,PINT)
FCALLSCSUB8(ffgpvi,FTGPVI,ftgpvi,FITSUNIT,LONG,LONG,LONG,SHORT,SHORTV,PLOGICAL,PINT)
FCALLSCSUB8(ffgpvk,FTGPVJ,ftgpvj,FITSUNIT,LONG,LONG,LONG,INT,INTV,PLOGICAL,PINT)
FCALLSCSUB8(ffgpvjj,FTGPVK,ftgpvk,FITSUNIT,LONG,LONG,LONG,LONGLONG,LONGLONGV,PLOGICAL,PINT)
FCALLSCSUB8(ffgpve,FTGPVE,ftgpve,FITSUNIT,LONG,LONG,LONG,FLOAT,FLOATV,PLOGICAL,PINT)
FCALLSCSUB8(ffgpvd,FTGPVD,ftgpvd,FITSUNIT,LONG,LONG,LONG,DOUBLE,DOUBLEV,PLOGICAL,PINT)


#define ftgpfb_LOGV_A6 A4
FCALLSCSUB8(ffgpfb,FTGPFB,ftgpfb,FITSUNIT,LONG,LONG,LONG,BYTEV,LOGICALV,PLOGICAL,PINT)

#define ftgpfi_LOGV_A6 A4
FCALLSCSUB8(ffgpfi,FTGPFI,ftgpfi,FITSUNIT,LONG,LONG,LONG,SHORTV,LOGICALV,PLOGICAL,PINT)

#define ftgpfj_LOGV_A6 A4
FCALLSCSUB8(ffgpfk,FTGPFJ,ftgpfj,FITSUNIT,LONG,LONG,LONG,INTV,LOGICALV,PLOGICAL,PINT)

#define ftgpfk_LOGV_A6 A4
FCALLSCSUB8(ffgpfjj,FTGPFK,ftgpfk,FITSUNIT,LONG,LONG,LONG,LONGLONGV,LOGICALV,PLOGICAL,PINT)

#define ftgpfe_LOGV_A6 A4
FCALLSCSUB8(ffgpfe,FTGPFE,ftgpfe,FITSUNIT,LONG,LONG,LONG,FLOATV,LOGICALV,PLOGICAL,PINT)

#define ftgpfd_LOGV_A6 A4
FCALLSCSUB8(ffgpfd,FTGPFD,ftgpfd,FITSUNIT,LONG,LONG,LONG,DOUBLEV,LOGICALV,PLOGICAL,PINT)

FCALLSCSUB9(ffg2db,FTG2DB,ftg2db,FITSUNIT,LONG,BYTE,LONG,LONG,LONG,BYTEV,PLOGICAL,PINT)
FCALLSCSUB9(ffg2di,FTG2DI,ftg2di,FITSUNIT,LONG,SHORT,LONG,LONG,LONG,SHORTV,PLOGICAL,PINT)
FCALLSCSUB9(ffg2dk,FTG2DJ,ftg2dj,FITSUNIT,LONG,INT,LONG,LONG,LONG,INTV,PLOGICAL,PINT)
FCALLSCSUB9(ffg2djj,FTG2DK,ftg2dk,FITSUNIT,LONG,LONGLONG,LONG,LONG,LONG,LONGLONGV,PLOGICAL,PINT)
FCALLSCSUB9(ffg2de,FTG2DE,ftg2de,FITSUNIT,LONG,FLOAT,LONG,LONG,LONG,FLOATV,PLOGICAL,PINT)
FCALLSCSUB9(ffg2dd,FTG2DD,ftg2dd,FITSUNIT,LONG,DOUBLE,LONG,LONG,LONG,DOUBLEV,PLOGICAL,PINT)

FCALLSCSUB11(ffg3db,FTG3DB,ftg3db,FITSUNIT,LONG,BYTE,LONG,LONG,LONG,LONG,LONG,BYTEV,PLOGICAL,PINT)
FCALLSCSUB11(ffg3di,FTG3DI,ftg3di,FITSUNIT,LONG,SHORT,LONG,LONG,LONG,LONG,LONG,SHORTV,PLOGICAL,PINT)
FCALLSCSUB11(ffg3dk,FTG3DJ,ftg3dj,FITSUNIT,LONG,INT,LONG,LONG,LONG,LONG,LONG,INTV,PLOGICAL,PINT)
FCALLSCSUB11(ffg3djj,FTG3DK,ftg3dk,FITSUNIT,LONG,LONGLONG,LONG,LONG,LONG,LONG,LONG,LONGLONGV,PLOGICAL,PINT)
FCALLSCSUB11(ffg3de,FTG3DE,ftg3de,FITSUNIT,LONG,FLOAT,LONG,LONG,LONG,LONG,LONG,FLOATV,PLOGICAL,PINT)
FCALLSCSUB11(ffg3dd,FTG3DD,ftg3dd,FITSUNIT,LONG,DOUBLE,LONG,LONG,LONG,LONG,LONG,DOUBLEV,PLOGICAL,PINT)

     /*  The follow LONGV definitions have +1 appended because the   */
     /*  routines use of NAXIS+1 elements of the long vectors.       */

#define ftgsvb_LONGV_A4 A3+1
#define ftgsvb_LONGV_A5 A3+1
#define ftgsvb_LONGV_A6 A3+1
#define ftgsvb_LONGV_A7 A3+1
FCALLSCSUB11(ffgsvb,FTGSVB,ftgsvb,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,BYTE,BYTEV,PLOGICAL,PINT)

#define ftgsvi_LONGV_A4 A3+1
#define ftgsvi_LONGV_A5 A3+1
#define ftgsvi_LONGV_A6 A3+1
#define ftgsvi_LONGV_A7 A3+1
FCALLSCSUB11(ffgsvi,FTGSVI,ftgsvi,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,SHORT,SHORTV,PLOGICAL,PINT)

#define ftgsvj_LONGV_A4 A3+1
#define ftgsvj_LONGV_A5 A3+1
#define ftgsvj_LONGV_A6 A3+1
#define ftgsvj_LONGV_A7 A3+1
FCALLSCSUB11(ffgsvk,FTGSVJ,ftgsvj,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,INT,INTV,PLOGICAL,PINT)

#define ftgsvk_LONGV_A4 A3+1
#define ftgsvk_LONGV_A5 A3+1
#define ftgsvk_LONGV_A6 A3+1
#define ftgsvk_LONGV_A7 A3+1
FCALLSCSUB11(ffgsvjj,FTGSVK,ftgsvk,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,LONGLONG,LONGLONGV,PLOGICAL,PINT)

#define ftgsve_LONGV_A4 A3+1
#define ftgsve_LONGV_A5 A3+1
#define ftgsve_LONGV_A6 A3+1
#define ftgsve_LONGV_A7 A3+1
FCALLSCSUB11(ffgsve,FTGSVE,ftgsve,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,FLOAT,FLOATV,PLOGICAL,PINT)

#define ftgsvd_LONGV_A4 A3+1
#define ftgsvd_LONGV_A5 A3+1
#define ftgsvd_LONGV_A6 A3+1
#define ftgsvd_LONGV_A7 A3+1
FCALLSCSUB11(ffgsvd,FTGSVD,ftgsvd,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,DOUBLE,DOUBLEV,PLOGICAL,PINT)


/*   Must handle LOGICALV conversion manually   */
void Cffgsfb( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, unsigned char *array, int *flagval, int *anynul, int *status );
void Cffgsfb( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, unsigned char *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfb( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfb_LONGV_A4 A3+1
#define ftgsfb_LONGV_A5 A3+1
#define ftgsfb_LONGV_A6 A3+1
#define ftgsfb_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfb,FTGSFB,ftgsfb,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,BYTEV,INTV,PLOGICAL,PINT)

/*   Must handle LOGICALV conversion manually   */
void Cffgsfi( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, short *array, int *flagval, int *anynul, int *status );
void Cffgsfi( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, short *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfi( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfi_LONGV_A4 A3+1
#define ftgsfi_LONGV_A5 A3+1
#define ftgsfi_LONGV_A6 A3+1
#define ftgsfi_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfi,FTGSFI,ftgsfi,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,SHORTV,INTV,PLOGICAL,PINT)

/*   Must handle LOGICALV conversion manually   */
void Cffgsfk( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, int *array, int *flagval, int *anynul, int *status );
void Cffgsfk( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, int *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfk( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfj_LONGV_A4 A3+1
#define ftgsfj_LONGV_A5 A3+1
#define ftgsfj_LONGV_A6 A3+1
#define ftgsfj_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfk,FTGSFJ,ftgsfj,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,INTV,INTV,PLOGICAL,PINT)


/*   Must handle LOGICALV conversion manually   */
void Cffgsfjj( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, LONGLONG *array, int *flagval, int *anynul, int *status );
void Cffgsfjj( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, LONGLONG *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfjj( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfk_LONGV_A4 A3+1
#define ftgsfk_LONGV_A5 A3+1
#define ftgsfk_LONGV_A6 A3+1
#define ftgsfk_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfjj,FTGSFK,ftgsfk,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,LONGLONGV,INTV,PLOGICAL,PINT)


/*   Must handle LOGICALV conversion manually   */
void Cffgsfe( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, float *array, int *flagval, int *anynul, int *status );
void Cffgsfe( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, float *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfe( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfe_LONGV_A4 A3+1
#define ftgsfe_LONGV_A5 A3+1
#define ftgsfe_LONGV_A6 A3+1
#define ftgsfe_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfe,FTGSFE,ftgsfe,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,FLOATV,INTV,PLOGICAL,PINT)

/*   Must handle LOGICALV conversion manually   */
void Cffgsfd( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, double *array, int *flagval, int *anynul, int *status );
void Cffgsfd( fitsfile *fptr, int colnum, int naxis, long *naxes, long *blc, long *trc, long *inc, double *array, int *flagval, int *anynul, int *status )
{
   char *Cflagval;
   long nflagval;
   int i;
 
   for( nflagval=1, i=0; i<naxis; i++ )
      nflagval *= (trc[i]-blc[i])/inc[i]+1;
   Cflagval = F2CcopyLogVect(nflagval, flagval );
   ffgsfd( fptr, colnum, naxis, naxes, blc, trc, inc, array, Cflagval, anynul, status );   
   C2FcopyLogVect(nflagval, flagval, Cflagval);
}
#define ftgsfd_LONGV_A4 A3+1
#define ftgsfd_LONGV_A5 A3+1
#define ftgsfd_LONGV_A6 A3+1
#define ftgsfd_LONGV_A7 A3+1
FCALLSCSUB11(Cffgsfd,FTGSFD,ftgsfd,FITSUNIT,INT,INT,LONGV,LONGV,LONGV,LONGV,DOUBLEV,INTV,PLOGICAL,PINT)

FCALLSCSUB6(ffggpb,FTGGPB,ftggpb,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB6(ffggpi,FTGGPI,ftggpi,FITSUNIT,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB6(ffggpk,FTGGPJ,ftggpj,FITSUNIT,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB6(ffggpjj,FTGGPK,ftggpk,FITSUNIT,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB6(ffggpe,FTGGPE,ftggpe,FITSUNIT,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB6(ffggpd,FTGGPD,ftggpd,FITSUNIT,LONG,LONG,LONG,DOUBLEV,PINT)

/*--------------------- read column elements -------------*/
/*   To guarantee that we allocate enough memory to hold strings within
     a table, call FFGTCL first to obtain width of the unique string
     and use it as the minimum string width.  Also test whether column
     has a variable width in which case a single string is read
     containing all its characters, so only declare a string vector
     with 1 element.                                                     */

#define ftgcvs_STRV_A7 NUM_ELEMS(velem)
CFextern VOID_cfF(FTGCVS,ftgcvs)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONG,LONG,LONG,STRING,PSTRINGV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTGCVS,ftgcvs)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONG,LONG,LONG,STRING,PSTRINGV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(FITSUNIT,1)
   QCF(INT,2)
   QCF(LONG,3)
   QCF(LONG,4)
   QCF(LONG,5)
   QCF(STRING,6)
   QCF(PSTRINGV,7)
   QCF(PLOGICAL,8)
   QCF(PINT,9)

   fitsfile *fptr;
   int colnum, *anynul, *status, velem, type;
   long firstrow, firstelem, nelem;
   long repeat;
   unsigned long gMinStrLen=80L;  /* gMin = width */
   char *nulval, **array;

   fptr =      TCF(ftgcvs,FITSUNIT,1,0);
   colnum =    TCF(ftgcvs,INT,2,0);
   firstrow =  TCF(ftgcvs,LONG,3,0);
   firstelem = TCF(ftgcvs,LONG,4,0);
   nelem =     TCF(ftgcvs,LONG,5,0);
   nulval =    TCF(ftgcvs,STRING,6,0);
   /*  put off variable 7 (array) until column type is learned  */
   anynul =    TCF(ftgcvs,PLOGICAL,8,0);
   status =    TCF(ftgcvs,PINT,9,0);
   
   ffgtcl( fptr, colnum, &type, &repeat, (long *)&gMinStrLen, status );
   if( type<0 ) velem = 1;   /*  Variable length column  */
   else velem = nelem;

   array = TCF(ftgcvs,PSTRINGV,7,0);

   ffgcvs( fptr, colnum, firstrow, firstelem, nelem, nulval, array,
           anynul, status );

   RCF(FITSUNIT,1)
   RCF(INT,2)
   RCF(LONG,3)
   RCF(LONG,4)
   RCF(LONG,5)
   RCF(STRING,6)
   RCF(PSTRINGV,7)
   RCF(PLOGICAL,8)
   RCF(PINT,9)
}

#define ftgcvsll_STRV_A7 NUM_ELEMS(velem)
CFextern VOID_cfF(FTGCVSLL,ftgcvsll)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONGLONG,LONGLONG,LONG,STRING,PSTRINGV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTGCVSLL,ftgcvsll)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONGLONG,LONGLONG,LONG,STRING,PSTRINGV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(FITSUNIT,1)
   QCF(INT,2)
   QCF(LONGLONG,3)
   QCF(LONGLONG,4)
   QCF(LONG,5)
   QCF(STRING,6)
   QCF(PSTRINGV,7)
   QCF(PLOGICAL,8)
   QCF(PINT,9)

   fitsfile *fptr;
   int colnum, *anynul, *status, velem, type;
   LONGLONG firstrow, firstelem;
   long nelem;
   long repeat;
   unsigned long gMinStrLen=80L;  /* gMin = width */
   char *nulval, **array;

   fptr =      TCF(ftgcvsll,FITSUNIT,1,0);
   colnum =    TCF(ftgcvsll,INT,2,0);
   firstrow =  TCF(ftgcvsll,LONGLONG,3,0);
   firstelem = TCF(ftgcvsll,LONGLONG,4,0);
   nelem =     TCF(ftgcvsll,LONG,5,0);
   nulval =    TCF(ftgcvsll,STRING,6,0);
   /*  put off variable 7 (array) until column type is learned  */
   anynul =    TCF(ftgcvsll,PLOGICAL,8,0);
   status =    TCF(ftgcvsll,PINT,9,0);
   
   ffgtcl( fptr, colnum, &type, &repeat, (long *)&gMinStrLen, status );
   if( type<0 ) velem = 1;   /*  Variable length column  */
   else velem = nelem;

   array = TCF(ftgcvsll,PSTRINGV,7,0);

   ffgcvs( fptr, colnum, firstrow, firstelem, nelem, nulval, array,
           anynul, status );

   RCF(FITSUNIT,1)
   RCF(INT,2)
   RCF(LONGLONG,3)
   RCF(LONGLONG,4)
   RCF(LONG,5)
   RCF(STRING,6)
   RCF(PSTRINGV,7)
   RCF(PLOGICAL,8)
   RCF(PINT,9)
}


#define ftgcl_LOGV_A6 A5
FCALLSCSUB7(ffgcl,FTGCL,ftgcl,FITSUNIT,INT,LONG,LONG,LONG,LOGICALV,PINT)

#define ftgcvl_LOGV_A7 A5
FCALLSCSUB9(ffgcvl,FTGCVL,ftgcvl,FITSUNIT,INT,LONG,LONG,LONG,LOGICAL,LOGICALV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvb,FTGCVB,ftgcvb,FITSUNIT,INT,LONG,LONG,LONG,BYTE,BYTEV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvi,FTGCVI,ftgcvi,FITSUNIT,INT,LONG,LONG,LONG,SHORT,SHORTV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvk,FTGCVJ,ftgcvj,FITSUNIT,INT,LONG,LONG,LONG,INT,INTV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvjj,FTGCVK,ftgcvk,FITSUNIT,INT,LONG,LONG,LONG,LONGLONG,LONGLONGV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcve,FTGCVE,ftgcve,FITSUNIT,INT,LONG,LONG,LONG,FLOAT,FLOATV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvd,FTGCVD,ftgcvd,FITSUNIT,INT,LONG,LONG,LONG,DOUBLE,DOUBLEV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvc,FTGCVC,ftgcvc,FITSUNIT,INT,LONG,LONG,LONG,FLOAT,FLOATV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvm,FTGCVM,ftgcvm,FITSUNIT,INT,LONG,LONG,LONG,DOUBLE,DOUBLEV,PLOGICAL,PINT)

#define ftgcvlll_LOGV_A7 A5
FCALLSCSUB9(ffgcvl,FTGCVLLL,ftgcvlll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LOGICAL,LOGICALV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvb,FTGCVBLL,ftgcvbll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,BYTE,BYTEV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvi,FTGCVILL,ftgcvill,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,SHORT,SHORTV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvk,FTGCVJLL,ftgcvjll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,INT,INTV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvjj,FTGCVKLL,ftgcvkll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LONGLONG,LONGLONGV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcve,FTGCVELL,ftgcvell,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOAT,FLOATV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvd,FTGCVDLL,ftgcvdll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLE,DOUBLEV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvc,FTGCVCLL,ftgcvcll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOAT,FLOATV,PLOGICAL,PINT)
FCALLSCSUB9(ffgcvm,FTGCVMLL,ftgcvmll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLE,DOUBLEV,PLOGICAL,PINT)

#define ftgcx_LOGV_A6 A5
FCALLSCSUB7(ffgcx,FTGCX,ftgcx,FITSUNIT,INT,LONG,LONG,LONG,LOGICALV,PINT)

/*   We need to worry about unsigned vs signed pointers in the following  */
/*   two routines, so use a pair of C wrappers which cast the pointers    */
/*   before passing them to CFITSIO.                                      */

void Cffgcxui(fitsfile *fptr, int colnum, long firstrow, long nrows,
              long firstbit, int nbits, short *array, int *status);
void Cffgcxui(fitsfile *fptr, int colnum, long firstrow, long nrows,
              long firstbit, int nbits, short *array, int *status)
{
   ffgcxui( fptr, colnum, firstrow, nrows, firstbit, nbits,
	    (unsigned short *)array, status );
}
FCALLSCSUB8(Cffgcxui,FTGCXI,ftgcxi,FITSUNIT,INT,LONG,LONG,LONG,INT,SHORTV,PINT)

void Cffgcxuk(fitsfile *fptr, int colnum, long firstrow, long nrows,
              long firstbit, int nbits, int *array, int *status);
void Cffgcxuk(fitsfile *fptr, int colnum, long firstrow, long nrows,
              long firstbit, int nbits, int *array, int *status)
{
   ffgcxuk( fptr, colnum, firstrow, nrows, firstbit, nbits,
	    (unsigned int *)array, status );
}
FCALLSCSUB8(Cffgcxuk,FTGCXJ,ftgcxj,FITSUNIT,INT,LONG,LONG,LONG,INT,INTV,PINT)

/*   To guarantee that we allocate enough memory to hold strings within
     a table, call FFGTCL first to obtain width of the unique string
     and use it as the minimum string width.  Also test whether column
     has a variable width in which case a single string is read
     containing all its characters, so only declare a string vector
     with 1 element.                                                     */

#define ftgcfs_STRV_A6 NUM_ELEMS(velem)
#define ftgcfs_LOGV_A7 A5
CFextern VOID_cfF(FTGCFS,ftgcfs)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONG,LONG,LONG,PSTRINGV,LOGICALV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0));
CFextern VOID_cfF(FTGCFS,ftgcfs)
CFARGT14(NCF,DCF,ABSOFT_cf2(VOID),FITSUNIT,INT,LONG,LONG,LONG,PSTRINGV,LOGICALV,PLOGICAL,PINT,CF_0,CF_0,CF_0,CF_0,CF_0))
{
   QCF(FITSUNIT,1)
   QCF(INT,2)
   QCF(LONG,3)
   QCF(LONG,4)
   QCF(LONG,5)
   QCF(PSTRINGV,6)
   QCF(LOGICALV,7)
   QCF(PLOGICAL,8)
   QCF(PINT,9)

   fitsfile *fptr;
   int colnum, *anynul, *status, velem, type;
   long firstrow, firstelem, nelem;
   long repeat;
   unsigned long gMinStrLen=80L;  /* gMin = width */
   char **array, *nularray;
 
   fptr =      TCF(ftgcfs,FITSUNIT,1,0);
   colnum =    TCF(ftgcfs,INT,2,0);
   firstrow =  TCF(ftgcfs,LONG,3,0);
   firstelem = TCF(ftgcfs,LONG,4,0);
   nelem =     TCF(ftgcfs,LONG,5,0);
   /*  put off variable 6 (array) until column type is learned  */
   nularray =  TCF(ftgcfs,LOGICALV,7,0);
   anynul =    TCF(ftgcfs,PLOGICAL,8,0);
   status =    TCF(ftgcfs,PINT,9,0);
   
   ffgtcl( fptr, colnum, &type, &repeat, (long*)&gMinStrLen, status );
   if( type<0 ) velem = 1;   /*  Variable length column  */
   else velem = nelem;

   array = TCF(ftgcfs,PSTRINGV,6,0);

   ffgcfs( fptr, colnum, firstrow, firstelem, nelem, array, nularray,
           anynul, status);

   RCF(FITSUNIT,1)
   RCF(INT,2)
   RCF(LONG,3)
   RCF(LONG,4)
   RCF(LONG,5)
   RCF(PSTRINGV,6)
   RCF(LOGICALV,7)
   RCF(PLOGICAL,8)
   RCF(PINT,9)
}

#define ftgcfl_LOGV_A6 A5
#define ftgcfl_LOGV_A7 A5
FCALLSCSUB9(ffgcfl,FTGCFL,ftgcfl,FITSUNIT,INT,LONG,LONG,LONG,LOGICALV,LOGICALV,PLOGICAL,PINT)

#define ftgcfb_LOGV_A7 A5
FCALLSCSUB9(ffgcfb,FTGCFB,ftgcfb,FITSUNIT,INT,LONG,LONG,LONG,BYTEV,LOGICALV,PLOGICAL,PINT)

#define ftgcfi_LOGV_A7 A5
FCALLSCSUB9(ffgcfi,FTGCFI,ftgcfi,FITSUNIT,INT,LONG,LONG,LONG,SHORTV,LOGICALV,PLOGICAL,PINT)

#define ftgcfj_LOGV_A7 A5
FCALLSCSUB9(ffgcfk,FTGCFJ,ftgcfj,FITSUNIT,INT,LONG,LONG,LONG,INTV,LOGICALV,PLOGICAL,PINT)

#define ftgcfk_LOGV_A7 A5
FCALLSCSUB9(ffgcfjj,FTGCFK,ftgcfk,FITSUNIT,INT,LONG,LONG,LONG,LONGLONGV,LOGICALV,PLOGICAL,PINT)

#define ftgcfe_LOGV_A7 A5
FCALLSCSUB9(ffgcfe,FTGCFE,ftgcfe,FITSUNIT,INT,LONG,LONG,LONG,FLOATV,LOGICALV,PLOGICAL,PINT)

#define ftgcfd_LOGV_A7 A5
FCALLSCSUB9(ffgcfd,FTGCFD,ftgcfd,FITSUNIT,INT,LONG,LONG,LONG,DOUBLEV,LOGICALV,PLOGICAL,PINT)

/*   Must handle LOGICALV conversion manually   */
void Cffgcfc( fitsfile *fptr, int colnum, long firstrow, long firstelem, long nelem, float *array, int *nularray, int *anynul, int *status );
void Cffgcfc( fitsfile *fptr, int colnum, long firstrow, long firstelem, long nelem, float *array, int *nularray, int *anynul, int *status )
{
   char *Cnularray;
 
   Cnularray = F2CcopyLogVect(nelem*2, nularray );
   ffgcfc( fptr, colnum, firstrow, firstelem, nelem, array, Cnularray, anynul, status );   
   C2FcopyLogVect(nelem*2, nularray, Cnularray );
}
FCALLSCSUB9(Cffgcfc,FTGCFC,ftgcfc,FITSUNIT,INT,LONG,LONG,LONG,FLOATV,INTV,PLOGICAL,PINT)

/*   Must handle LOGICALV conversion manually   */
void Cffgcfm( fitsfile *fptr, int colnum, long firstrow, long firstelem, long nelem, double *array, int *nularray, int *anynul, int *status );
void Cffgcfm( fitsfile *fptr, int colnum, long firstrow, long firstelem, long nelem, double *array, int *nularray, int *anynul, int *status )
{
   char *Cnularray;
 
   Cnularray = F2CcopyLogVect(nelem*2, nularray );
   ffgcfm( fptr, colnum, firstrow, firstelem, nelem, array, Cnularray, anynul, status );   
   C2FcopyLogVect(nelem*2, nularray, Cnularray );
}
FCALLSCSUB9(Cffgcfm,FTGCFM,ftgcfm,FITSUNIT,INT,LONG,LONG,LONG,DOUBLEV,INTV,PLOGICAL,PINT)


#define ftgcfbll_LOGV_A7 A5
FCALLSCSUB9(ffgcfb,FTGCFBLL,ftgcfbll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,BYTEV,LOGICALV,PLOGICAL,PINT)

#define ftgcfill_LOGV_A7 A5
FCALLSCSUB9(ffgcfi,FTGCFILL,ftgcfill,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,SHORTV,LOGICALV,PLOGICAL,PINT)

#define ftgcfjll_LOGV_A7 A5
FCALLSCSUB9(ffgcfk,FTGCFJLL,ftgcfjll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,INTV,LOGICALV,PLOGICAL,PINT)

#define ftgcfkll_LOGV_A7 A5
FCALLSCSUB9(ffgcfjj,FTGCFKLL,ftgcfkll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LONGLONGV,LOGICALV,PLOGICAL,PINT)

#define ftgcfell_LOGV_A7 A5
FCALLSCSUB9(ffgcfe,FTGCFELL,ftgcfell,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOATV,LOGICALV,PLOGICAL,PINT)

#define ftgcfdll_LOGV_A7 A5
FCALLSCSUB9(ffgcfd,FTGCFDLL,ftgcfdll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLEV,LOGICALV,PLOGICAL,PINT)

FCALLSCSUB6(ffgdes,FTGDES,ftgdes,FITSUNIT,INT,LONG,PLONG,PLONG,PINT)
FCALLSCSUB6(ffgdesll,FTGDESLL,ftgdesll,FITSUNIT,INT,LONG,PLONGLONG,PLONGLONG,PINT)

#define ftgdess_LONGV_A5 A4
#define ftgdess_LONGV_A6 A4
FCALLSCSUB7(ffgdess,FTGDESS,ftgdess,FITSUNIT,INT,LONG,LONG,LONGV,LONGV,PINT)
#define ftgdessll_LONGV_A5 A4
#define ftgdessll_LONGV_A6 A4FCALLSCSUB7(ffgdessll,FTGDESSLL,ftgdessll,FITSUNIT,INT,LONG,LONG,LONGLONGV,LONGLONGV,PINT)
FCALLSCSUB7(ffgdessll,FTGDESSLL,ftgdessll,FITSUNIT,INT,LONG,LONG,LONGLONGV,LONGLONGV,PINT)

FCALLSCSUB6(ffgtbb,FTGTBB,ftgtbb,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB6(ffgtbb,FTGTBS,ftgtbs,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)

/*------------ write primary array or image elements -------------*/
FCALLSCSUB6(ffpprb,FTPPRB,ftpprb,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB6(ffppri,FTPPRI,ftppri,FITSUNIT,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB6(ffpprk,FTPPRJ,ftpprj,FITSUNIT,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB6(ffpprjj,FTPPRK,ftpprk,FITSUNIT,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB6(ffppre,FTPPRE,ftppre,FITSUNIT,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB6(ffpprd,FTPPRD,ftpprd,FITSUNIT,LONG,LONG,LONG,DOUBLEV,PINT)

FCALLSCSUB7(ffppnb,FTPPNB,ftppnb,FITSUNIT,LONG,LONG,LONG,BYTEV,BYTE,PINT)
FCALLSCSUB7(ffppni,FTPPNI,ftppni,FITSUNIT,LONG,LONG,LONG,SHORTV,SHORT,PINT)
FCALLSCSUB7(ffppnk,FTPPNJ,ftppnj,FITSUNIT,LONG,LONG,LONG,INTV,INT,PINT)
FCALLSCSUB7(ffppnjj,FTPPNK,ftppnk,FITSUNIT,LONG,LONG,LONG,LONGLONGV,LONGLONG,PINT)
FCALLSCSUB7(ffppne,FTPPNE,ftppne,FITSUNIT,LONG,LONG,LONG,FLOATV,FLOAT,PINT)
FCALLSCSUB7(ffppnd,FTPPND,ftppnd,FITSUNIT,LONG,LONG,LONG,DOUBLEV,DOUBLE,PINT)

FCALLSCSUB7(ffp2db,FTP2DB,ftp2db,FITSUNIT,LONG,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB7(ffp2di,FTP2DI,ftp2di,FITSUNIT,LONG,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB7(ffp2dk,FTP2DJ,ftp2dj,FITSUNIT,LONG,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB7(ffp2djj,FTP2DK,ftp2dk,FITSUNIT,LONG,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB7(ffp2de,FTP2DE,ftp2de,FITSUNIT,LONG,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB7(ffp2dd,FTP2DD,ftp2dd,FITSUNIT,LONG,LONG,LONG,LONG,DOUBLEV,PINT)

FCALLSCSUB9(ffp3db,FTP3DB,ftp3db,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB9(ffp3di,FTP3DI,ftp3di,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB9(ffp3dk,FTP3DJ,ftp3dj,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB9(ffp3djj,FTP3DK,ftp3dk,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB9(ffp3de,FTP3DE,ftp3de,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB9(ffp3dd,FTP3DD,ftp3dd,FITSUNIT,LONG,LONG,LONG,LONG,LONG,LONG,DOUBLEV,PINT)

#define ftpssb_LONGV_A4 A3
#define ftpssb_LONGV_A5 A3
#define ftpssb_LONGV_A6 A3
FCALLSCSUB8(ffpssb,FTPSSB,ftpssb,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,BYTEV,PINT)

#define ftpssi_LONGV_A4 A3
#define ftpssi_LONGV_A5 A3
#define ftpssi_LONGV_A6 A3
FCALLSCSUB8(ffpssi,FTPSSI,ftpssi,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,SHORTV,PINT)

#define ftpssj_LONGV_A4 A3
#define ftpssj_LONGV_A5 A3
#define ftpssj_LONGV_A6 A3
FCALLSCSUB8(ffpssk,FTPSSJ,ftpssj,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,INTV,PINT)

#define ftpssk_LONGV_A4 A3
#define ftpssk_LONGV_A5 A3
#define ftpssk_LONGV_A6 A3
FCALLSCSUB8(ffpssjj,FTPSSK,ftpssk,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,LONGLONGV,PINT)

#define ftpsse_LONGV_A4 A3
#define ftpsse_LONGV_A5 A3
#define ftpsse_LONGV_A6 A3
FCALLSCSUB8(ffpsse,FTPSSE,ftpsse,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,FLOATV,PINT)

#define ftpssd_LONGV_A4 A3
#define ftpssd_LONGV_A5 A3
#define ftpssd_LONGV_A6 A3
FCALLSCSUB8(ffpssd,FTPSSD,ftpssd,FITSUNIT,LONG,LONG,LONGV,LONGV,LONGV,DOUBLEV,PINT)

FCALLSCSUB6(ffpgpb,FTPGPB,ftpgpb,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB6(ffpgpi,FTPGPI,ftpgpi,FITSUNIT,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB6(ffpgpk,FTPGPJ,ftpgpj,FITSUNIT,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB6(ffpgpjj,FTPGPK,ftpgpk,FITSUNIT,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB6(ffpgpe,FTPGPE,ftpgpe,FITSUNIT,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB6(ffpgpd,FTPGPD,ftpgpd,FITSUNIT,LONG,LONG,LONG,DOUBLEV,PINT)

FCALLSCSUB5(ffppru,FTPPRU,ftppru,FITSUNIT,LONG,LONG,LONG,PINT)
FCALLSCSUB4(ffpprn,FTPPRN,ftpprn,FITSUNIT,LONG,LONG,PINT)

/*--------------------- write column elements -------------*/
#define ftpcls_STRV_A6 NUM_ELEM_ARG(5)
FCALLSCSUB7(ffpcls,FTPCLS,ftpcls,FITSUNIT,INT,LONG,LONG,LONG,STRINGV,PINT)

#define ftpcll_LOGV_A6 A5
FCALLSCSUB7(ffpcll,FTPCLL,ftpcll,FITSUNIT,INT,LONG,LONG,LONG,LOGICALV,PINT)
FCALLSCSUB7(ffpclb,FTPCLB,ftpclb,FITSUNIT,INT,LONG,LONG,LONG,BYTEV,PINT)
FCALLSCSUB7(ffpcli,FTPCLI,ftpcli,FITSUNIT,INT,LONG,LONG,LONG,SHORTV,PINT)
FCALLSCSUB7(ffpclk,FTPCLJ,ftpclj,FITSUNIT,INT,LONG,LONG,LONG,INTV,PINT)
FCALLSCSUB7(ffpcljj,FTPCLK,ftpclk,FITSUNIT,INT,LONG,LONG,LONG,LONGLONGV,PINT)
FCALLSCSUB7(ffpcle,FTPCLE,ftpcle,FITSUNIT,INT,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB7(ffpcld,FTPCLD,ftpcld,FITSUNIT,INT,LONG,LONG,LONG,DOUBLEV,PINT)
FCALLSCSUB7(ffpclc,FTPCLC,ftpclc,FITSUNIT,INT,LONG,LONG,LONG,FLOATV,PINT)
FCALLSCSUB7(ffpclm,FTPCLM,ftpclm,FITSUNIT,INT,LONG,LONG,LONG,DOUBLEV,PINT)
FCALLSCSUB6(ffpclu,FTPCLU,ftpclu,FITSUNIT,INT,LONG,LONG,LONG,PINT)
FCALLSCSUB4(ffprwu,FTPRWU,ftprwu,FITSUNIT,LONG,LONG,PINT)

#define ftpclsll_STRV_A6 NUM_ELEM_ARG(5)
FCALLSCSUB7(ffpcls,FTPCLSLL,ftpclsll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,STRINGV,PINT)

#define ftpcllll_LOGV_A6 A5
FCALLSCSUB7(ffpcll,FTPCLLLL,ftpcllll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LOGICALV,PINT)
FCALLSCSUB7(ffpclb,FTPCLBLL,ftpclbll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,BYTEV,PINT)
FCALLSCSUB7(ffpcli,FTPCLILL,ftpclill,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,SHORTV,PINT)
FCALLSCSUB7(ffpclk,FTPCLJLL,ftpcljll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,INTV,PINT)
FCALLSCSUB7(ffpcljj,FTPCLKLL,ftpclkll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LONGLONGV,PINT)
FCALLSCSUB7(ffpcle,FTPCLELL,ftpclell,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOATV,PINT)
FCALLSCSUB7(ffpcld,FTPCLDLL,ftpcldll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLEV,PINT)
FCALLSCSUB7(ffpclc,FTPCLCLL,ftpclcll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOATV,PINT)
FCALLSCSUB7(ffpclm,FTPCLMLL,ftpclmll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLEV,PINT)
FCALLSCSUB6(ffpclu,FTPCLULL,ftpclull,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,PINT)

#define ftpclx_LOGV_A6 A5
FCALLSCSUB7(ffpclx,FTPCLX,ftpclx,FITSUNIT,INT,LONG,LONG,LONG,LOGICALV,PINT)

#define ftpcns_STRV_A6 NUM_ELEM_ARG(5)
FCALLSCSUB8(ffpcns,FTPCNS,ftpcns,FITSUNIT,INT,LONG,LONG,LONG,STRINGV,STRING,PINT)

FCALLSCSUB8(ffpcnb,FTPCNB,ftpcnb,FITSUNIT,INT,LONG,LONG,LONG,BYTEV,BYTE,PINT)
FCALLSCSUB8(ffpcni,FTPCNI,ftpcni,FITSUNIT,INT,LONG,LONG,LONG,SHORTV,SHORT,PINT)
FCALLSCSUB8(ffpcnk,FTPCNJ,ftpcnj,FITSUNIT,INT,LONG,LONG,LONG,INTV,INT,PINT)
FCALLSCSUB8(ffpcnjj,FTPCNK,ftpcnk,FITSUNIT,INT,LONG,LONG,LONG,LONGLONGV,LONGLONG,PINT)
FCALLSCSUB8(ffpcne,FTPCNE,ftpcne,FITSUNIT,INT,LONG,LONG,LONG,FLOATV,FLOAT,PINT)
FCALLSCSUB8(ffpcnd,FTPCND,ftpcnd,FITSUNIT,INT,LONG,LONG,LONG,DOUBLEV,DOUBLE,PINT)

#define ftpcnsll_STRV_A6 NUM_ELEM_ARG(5)
FCALLSCSUB8(ffpcns,FTPCNSLL,ftpcnsll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,STRINGV,STRING,PINT)

FCALLSCSUB8(ffpcnb,FTPCNBLL,ftpcnbll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,BYTEV,BYTE,PINT)
FCALLSCSUB8(ffpcni,FTPCNILL,ftpcnill,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,SHORTV,SHORT,PINT)
FCALLSCSUB8(ffpcnk,FTPCNJLL,ftpcnjll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,INTV,INT,PINT)
FCALLSCSUB8(ffpcnjj,FTPCNKLL,ftpcnkll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,LONGLONGV,LONGLONG,PINT)
FCALLSCSUB8(ffpcne,FTPCNELL,ftpcnell,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,FLOATV,FLOAT,PINT)
FCALLSCSUB8(ffpcnd,FTPCNDLL,ftpcndll,FITSUNIT,INT,LONGLONG,LONGLONG,LONG,DOUBLEV,DOUBLE,PINT)

FCALLSCSUB6(ffpdes,FTPDES,ftpdes,FITSUNIT,INT,LONG,LONG,LONG,PINT)
FCALLSCSUB6(ffpdes,FTPDESLL,ftpdesll,FITSUNIT,INT,LONG,LONGLONG,LONGLONG,PINT)

FCALLSCSUB6(ffptbb,FTPTBB,ftptbb,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)
   /*  Add extra entry point to ffptbb... ftptbs obsolete  */
FCALLSCSUB6(ffptbb,FTPTBS,ftptbs,FITSUNIT,LONG,LONG,LONG,BYTEV,PINT)

FCALLSCSUB4(ffirow,FTIROW,ftirow,FITSUNIT,LONG,LONG,PINT)
FCALLSCSUB4(ffirow,FTIROWLL,ftirowll,FITSUNIT,LONGLONG,LONGLONG,PINT)
FCALLSCSUB4(ffdrow,FTDROW,ftdrow,FITSUNIT,LONG,LONG,PINT)
FCALLSCSUB4(ffdrow,FTDROWLL,ftdrowll,FITSUNIT,LONGLONG,LONGLONG,PINT)
FCALLSCSUB3(ffdrrg,FTDRRG,ftdrrg,FITSUNIT,STRING,PINT)
#define ftdrws_LONGV_A2 A3
FCALLSCSUB4(ffdrws,FTDRWS,ftdrws,FITSUNIT,LONGV,LONG,PINT)
FCALLSCSUB5(fficol,FTICOL,fticol,FITSUNIT,INT,STRING,STRING,PINT)

#define fticls_STRV_A4 NUM_ELEM_ARG(3)
#define fticls_STRV_A5 NUM_ELEM_ARG(3)
FCALLSCSUB6(fficls,FTICLS,fticls,FITSUNIT,INT,INT,STRINGV,STRINGV,PINT)
FCALLSCSUB4(ffmvec,FTMVEC,ftmvec,FITSUNIT,INT,LONG,PINT)
FCALLSCSUB3(ffdcol,FTDCOL,ftdcol,FITSUNIT,INT,PINT)
FCALLSCSUB6(ffcpcl,FTCPCL,ftcpcl,FITSUNIT,FITSUNIT,INT,INT,INT,PINT)
