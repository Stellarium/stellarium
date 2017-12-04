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

unsigned long gMinStrLen=80L;
fitsfile *gFitsFiles[NMAXFILES]={0};

/*----------------  Fortran Unit Number Allocation -------------*/

void Cffgiou( int *unit, int *status );
void Cffgiou( int *unit, int *status )
{
   int i;

   if( *status>0 ) return;
   for( i=50;i<NMAXFILES;i++ ) /* Using a unit=0 sounds bad, so start at 1 */
      if( gFitsFiles[i]==NULL ) break;
   if( i==NMAXFILES ) {
      *unit = 0;
      *status = TOO_MANY_FILES;
      ffpmsg("Cffgiou has no more available unit numbers.");
   } else {
      *unit=i;
      gFitsFiles[i] = (fitsfile *)1; /*  Flag it as taken until ftopen/init  */
                                     /*  can be called and set a real value  */
   }
}
FCALLSCSUB2(Cffgiou,FTGIOU,ftgiou,PINT,PINT)

void Cfffiou( int unit, int *status );
void Cfffiou( int unit, int *status )
{
   if( *status>0 ) return;
   if( unit == -1 ) {
      int i; for( i=50; i<NMAXFILES; ) gFitsFiles[i++]=NULL;
   } else if( unit<1 || unit>=NMAXFILES ) {
      *status = BAD_FILEPTR;
      ffpmsg("Cfffiou was sent an unacceptable unit number.");
   } else gFitsFiles[unit]=NULL;
}
FCALLSCSUB2(Cfffiou,FTFIOU,ftfiou,INT,PINT)


int CFITS2Unit( fitsfile *fptr )
     /* Utility routine to convert a fitspointer to a Fortran unit number */
     /* for use when a C program is calling a Fortran routine which could */
     /* in turn call CFITSIO... Modelled after code by Ning Gan.          */
{
   static fitsfile *last_fptr = (fitsfile *)NULL; /* Remember last fptr */
   static int last_unit = 0;                      /* Remember last unit */
   int status = 0;

   /*  Test whether we are repeating the last lookup  */

   if( last_unit && fptr==gFitsFiles[last_unit] )
      return( last_unit );

   /*  Check if gFitsFiles has an entry for this fptr.  */
   /*  Allows Fortran to call C to call Fortran to      */
   /*  call CFITSIO... OUCH!!!                          */

   last_fptr = fptr;
   for( last_unit=1; last_unit<NMAXFILES; last_unit++ ) {
      if( fptr == gFitsFiles[last_unit] )
	 return( last_unit );
   }

   /*  Allocate a new unit number for this fptr  */
   Cffgiou( &last_unit, &status );
   if( status )
      last_unit = 0;
   else
      gFitsFiles[last_unit] = fptr;
   return( last_unit );
}


fitsfile* CUnit2FITS(int unit)
{
    if( unit<1 || unit>=NMAXFILES )
        return(0);
	
    return(gFitsFiles[unit]);
}

     /**************************************************/
     /*   Start of wrappers for routines in fitsio.h   */
     /**************************************************/

/*----------------  FITS file URL parsing routines -------------*/

FCALLSCSUB9(ffiurl,FTIURL,ftiurl,STRING,PSTRING,PSTRING,PSTRING,PSTRING,PSTRING,PSTRING,PSTRING,PINT)
FCALLSCSUB3(ffrtnm,FTRTNM,ftrtnm,STRING,PSTRING,PINT)
FCALLSCSUB3(ffexist,FTEXIST,ftexist,STRING,PINT,PINT)
FCALLSCSUB3(ffextn,FTEXTN,ftextn,STRING,PINT,PINT)
FCALLSCSUB7(ffrwrg,FTRWRG,ftrwrg,STRING,LONG,INT,PINT,PLONG,PLONG,PINT)

/*---------------- FITS file I/O routines ---------------*/

void Cffopen( fitsfile **fptr, const char *filename, int iomode, int *blocksize, int *status );
void Cffopen( fitsfile **fptr, const char *filename, int iomode, int *blocksize, int *status )
{
   int hdutype;

   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffopen( fptr, filename, iomode, status );
      ffmahd( *fptr, 1, &hdutype, status );
      *blocksize = 1;
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffopen tried to use an already opened unit.");
   }
}
FCALLSCSUB5(Cffopen,FTOPEN,ftopen,PFITSUNIT,STRING,INT,PINT,PINT)

void Cffdkopn( fitsfile **fptr, const char *filename, int iomode, int *blocksize, int *status );
void Cffdkopn( fitsfile **fptr, const char *filename, int iomode, int *blocksize, int *status )
{
   int hdutype;

   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffdkopn( fptr, filename, iomode, status );
      ffmahd( *fptr, 1, &hdutype, status );
      *blocksize = 1;
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffdkopn tried to use an already opened unit.");
   }
}
FCALLSCSUB5(Cffdkopn,FTDKOPN,ftdkopn,PFITSUNIT,STRING,INT,PINT,PINT)


void Cffnopn( fitsfile **fptr, const char *filename, int iomode, int *status );
void Cffnopn( fitsfile **fptr, const char *filename, int iomode, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffopen( fptr, filename, iomode, status );
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffnopn tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cffnopn,FTNOPN,ftnopn,PFITSUNIT,STRING,INT,PINT)

void Cffdopn( fitsfile **fptr, const char *filename, int iomode, int *status );
void Cffdopn( fitsfile **fptr, const char *filename, int iomode, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffdopn( fptr, filename, iomode, status );
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffdopn tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cffdopn,FTDOPN,ftdopn,PFITSUNIT,STRING,INT,PINT)

void Cfftopn( fitsfile **fptr, const char *filename, int iomode, int *status );
void Cfftopn( fitsfile **fptr, const char *filename, int iomode, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      fftopn( fptr, filename, iomode, status );
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cfftopn tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cfftopn,FTTOPN,fttopn,PFITSUNIT,STRING,INT,PINT)

void Cffiopn( fitsfile **fptr, const char *filename, int iomode, int *status );
void Cffiopn( fitsfile **fptr, const char *filename, int iomode, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffiopn( fptr, filename, iomode, status );
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffiopn tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cffiopn,FTIOPN,ftiopn,PFITSUNIT,STRING,INT,PINT)

void Cffreopen( fitsfile *openfptr, fitsfile **newfptr, int *status );
void Cffreopen( fitsfile *openfptr, fitsfile **newfptr, int *status )
{
   if( *newfptr==NULL || *newfptr==(fitsfile*)1 ) {
      ffreopen( openfptr, newfptr, status );
   } else {
      *status = FILE_NOT_OPENED;
      ffpmsg("Cffreopen tried to use an already opened unit.");
   }
}
FCALLSCSUB3(Cffreopen,FTREOPEN,ftreopen,FITSUNIT,PFITSUNIT,PINT)

void Cffinit( fitsfile **fptr, const char *filename, int blocksize, int *status );
void Cffinit( fitsfile **fptr, const char *filename, int blocksize, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffinit( fptr, filename, status );
   } else {
      *status = FILE_NOT_CREATED;
      ffpmsg("Cffinit tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cffinit,FTINIT,ftinit,PFITSUNIT,STRING,INT,PINT)

void Cffdkinit( fitsfile **fptr, const char *filename, int blocksize, int *status );
void Cffdkinit( fitsfile **fptr, const char *filename, int blocksize, int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      ffdkinit( fptr, filename, status );
   } else {
      *status = FILE_NOT_CREATED;
      ffpmsg("Cffdkinit tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cffdkinit,FTDKINIT,ftdkinit,PFITSUNIT,STRING,INT,PINT)

void Cfftplt( fitsfile **fptr, const char *filename, const char *tempname,
	      int *status );
void Cfftplt( fitsfile **fptr, const char *filename, const char *tempname, 
	      int *status )
{
   if( *fptr==NULL || *fptr==(fitsfile*)1 ) {
      fftplt( fptr, filename, tempname, status );
   } else {
      *status = FILE_NOT_CREATED;
      ffpmsg("Cfftplt tried to use an already opened unit.");
   }
}
FCALLSCSUB4(Cfftplt,FTTPLT,fttplt,PFITSUNIT,STRING,STRING,PINT)

FCALLSCSUB2(ffflus,FTFLUS,ftflus,FITSUNIT,PINT)
FCALLSCSUB3(ffflsh,FTFLSH,ftflsh,FITSUNIT, INT, PINT)

void Cffclos( int unit, int *status );
void Cffclos( int unit, int *status )
{
   if( gFitsFiles[unit]!=NULL && gFitsFiles[unit]!=(void*)1 ) {
      ffclos( gFitsFiles[unit], status );  /* Flag unit number as unavailable */
      gFitsFiles[unit]=(fitsfile*)1;       /* in case want to reuse it        */
   }
}
FCALLSCSUB2(Cffclos,FTCLOS,ftclos,INT,PINT)

void Cffdelt( int unit, int *status );
void Cffdelt( int unit, int *status )
{
   if( gFitsFiles[unit]!=NULL && gFitsFiles[unit]!=(void*)1 ) {
      ffdelt( gFitsFiles[unit], status );  /* Flag unit number as unavailable */
      gFitsFiles[unit]=(fitsfile*)1;       /* in case want to reuse it        */
   }
}
FCALLSCSUB2(Cffdelt,FTDELT,ftdelt,INT,PINT)

FCALLSCSUB3(ffflnm,FTFLNM,ftflnm,FITSUNIT,PSTRING,PINT)
FCALLSCSUB3(ffflmd,FTFLMD,ftflmd,FITSUNIT,PINT,PINT)

/*--------------- utility routines ---------------*/
FCALLSCSUB1(ffvers,FTVERS,ftvers,PFLOAT)
FCALLSCSUB1(ffupch,FTUPCH,ftupch,PSTRING)
FCALLSCSUB2(ffgerr,FTGERR,ftgerr,INT,PSTRING)
FCALLSCSUB1(ffpmsg,FTPMSG,ftpmsg,STRING)
FCALLSCSUB1(ffgmsg,FTGMSG,ftgmsg,PSTRING)
FCALLSCSUB0(ffcmsg,FTCMSG,ftcmsg)
FCALLSCSUB0(ffpmrk,FTPMRK,ftpmrk)
FCALLSCSUB0(ffcmrk,FTCMRK,ftcmrk)

void Cffrprt( char *fname, int status );
void Cffrprt( char *fname, int status )
{
   if( !strcmp(fname,"STDOUT") || !strcmp(fname,"stdout") )
      ffrprt( stdout, status );
   else if( !strcmp(fname,"STDERR") || !strcmp(fname,"stderr") )
      ffrprt( stderr, status );
   else {
      FILE *fptr;

      fptr = fopen(fname, "a");
      if (fptr==NULL)
	 printf("file pointer is null.\n");
      else {
	 ffrprt(fptr,status);
	 fclose(fptr);
      }
   }
}
FCALLSCSUB2(Cffrprt,FTRPRT,ftrprt,STRING,INT)

FCALLSCSUB5(ffcmps,FTCMPS,ftcmps,STRING,STRING,LOGICAL,PLOGICAL,PLOGICAL)
FCALLSCSUB2(fftkey,FTTKEY,fttkey,STRING,PINT)
FCALLSCSUB2(fftrec,FTTREC,fttrec,STRING,PINT)
FCALLSCSUB2(ffnchk,FTNCHK,ftnchk,FITSUNIT,PINT)
FCALLSCSUB4(ffkeyn,FTKEYN,ftkeyn,STRING,INT,PSTRING,PINT)
FCALLSCSUB4(ffgknm,FTGKNM,ftgknm,STRING,PSTRING, PINT, PINT)
FCALLSCSUB4(ffnkey,FTNKEY,ftnkey,INT,STRING,PSTRING,PINT)
FCALLSCSUB3(ffdtyp,FTDTYP,ftdtyp,STRING,PSTRING,PINT)
FCALLSCFUN1(INT,ffgkcl,FTGKCL,ftgkcl,STRING)
FCALLSCSUB5(ffmkky,FTMKKY,ftmkky,STRING,STRING,STRING,PSTRING,PINT)
FCALLSCSUB4(ffpsvc,FTPSVC,ftpsvc,STRING,PSTRING,PSTRING,PINT)
FCALLSCSUB4(ffgthd,FTGTHD,ftgthd,STRING,PSTRING,PINT,PINT)
FCALLSCSUB5(ffasfm,FTASFM,ftasfm,STRING,PINT,PLONG,PINT,PINT)
FCALLSCSUB5(ffbnfm,FTBNFM,ftbnfm,STRING,PINT,PLONG,PLONG,PINT)

#define ftgabc_STRV_A2 NUM_ELEM_ARG(1)
#define ftgabc_LONGV_A5 A1
FCALLSCSUB6(ffgabc,FTGABC,ftgabc,INT,STRINGV,INT,PLONG,LONGV,PINT)

