/*  This file, cfileio.c, contains the low-level file access routines.     */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>  /* apparently needed to define size_t */
#include "fitsio2.h"
#include "group.h"
#ifdef CFITSIO_HAVE_CURL
  #include <curl/curl.h>
#endif

#define MAX_PREFIX_LEN 20  /* max length of file type prefix (e.g. 'http://') */
#define MAX_DRIVERS 27     /* max number of file I/O drivers */

typedef struct    /* structure containing pointers to I/O driver functions */ 
{   char prefix[MAX_PREFIX_LEN];
    int (*init)(void);
    int (*shutdown)(void);
    int (*setoptions)(int option);
    int (*getoptions)(int *options);
    int (*getversion)(int *version);
    int (*checkfile)(char *urltype, char *infile, char *outfile);
    int (*open)(char *filename, int rwmode, int *driverhandle);
    int (*create)(char *filename, int *drivehandle);
    int (*truncate)(int drivehandle, LONGLONG size);
    int (*close)(int drivehandle);
    int (*remove)(char *filename);
    int (*size)(int drivehandle, LONGLONG *size);
    int (*flush)(int drivehandle);
    int (*seek)(int drivehandle, LONGLONG offset);
    int (*read)(int drivehandle, void *buffer, long nbytes);
    int (*write)(int drivehandle, void *buffer, long nbytes);
} fitsdriver;

fitsdriver driverTable[MAX_DRIVERS];  /* allocate driver tables */

FITSfile *FptrTable[NMAXFILES];  /* this table of Fptr pointers is */
                                 /* used by fits_already_open */

int need_to_initialize = 1;    /* true if CFITSIO has not been initialized */
int no_of_drivers = 0;         /* number of currently defined I/O drivers */

static int pixel_filter_helper(fitsfile **fptr, char *outfile,
				char *expr,  int *status);
static int find_quote(char **string);
static int find_doublequote(char **string);
static int find_paren(char **string);
static int find_bracket(char **string);
static int find_curlybracket(char **string);
int comma2semicolon(char *string);

#ifdef _REENTRANT

pthread_mutex_t Fitsio_InitLock = PTHREAD_MUTEX_INITIALIZER;

#endif

/*--------------------------------------------------------------------------*/
int fitsio_init_lock(void)
{
  int status = 0;
  
#ifdef _REENTRANT

  static int need_to_init = 1;

  pthread_mutexattr_t mutex_init;

  FFLOCK1(Fitsio_InitLock);

  if (need_to_init) {

    /* Init the main fitsio lock here since we need a a recursive lock */

    status = pthread_mutexattr_init(&mutex_init);
    if (status) {
        ffpmsg("pthread_mutexattr_init failed (fitsio_init_lock)");
        return(status);
    }

#ifdef __GLIBC__
    status = pthread_mutexattr_settype(&mutex_init,
				     PTHREAD_MUTEX_RECURSIVE_NP);
#else
    status = pthread_mutexattr_settype(&mutex_init,
				     PTHREAD_MUTEX_RECURSIVE);
#endif
    if (status) {
        ffpmsg("pthread_mutexattr_settype failed (fitsio_init_lock)");
        return(status);
    }

    status = pthread_mutex_init(&Fitsio_Lock,&mutex_init);
    if (status) {
        ffpmsg("pthread_mutex_init failed (fitsio_init_lock)");
        return(status);
    }

    need_to_init = 0;
  }

  FFUNLOCK1(Fitsio_InitLock);

#endif

    return(status);
}
/*--------------------------------------------------------------------------*/
int ffomem(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - name of file to open                */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           void **buffptr,       /* I - address of memory pointer           */
           size_t *buffsize,     /* I - size of buffer, in bytes            */
           size_t deltasize,     /* I - increment for future realloc's      */
           void *(*mem_realloc)(void *p, size_t newsize), /* function       */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file in core memory.  This is a specialized version
  of ffopen.
*/
{
    int ii, driver, handle, hdutyp, slen, movetotype, extvers, extnum;
    char extname[FLEN_VALUE];
    LONGLONG filesize;
    char urltype[MAX_PREFIX_LEN], infile[FLEN_FILENAME], outfile[FLEN_FILENAME];
    char extspec[FLEN_FILENAME], rowfilter[FLEN_FILENAME];
    char binspec[FLEN_FILENAME], colspec[FLEN_FILENAME];
    char imagecolname[FLEN_VALUE], rowexpress[FLEN_FILENAME];
    char *url, errmsg[FLEN_ERRMSG];
    char *hdtype[3] = {"IMAGE", "TABLE", "BINTABLE"};

    if (*status > 0)
        return(*status);

    *fptr = 0;                   /* initialize null file pointer */

    if (need_to_initialize)           /* this is called only once */
    {
        *status = fits_init_cfitsio();

        if (*status > 0)
            return(*status);
    }

    url = (char *) name;
    while (*url == ' ')  /* ignore leading spaces in the file spec */
        url++;

        /* parse the input file specification */
    fits_parse_input_url(url, urltype, infile, outfile, extspec,
              rowfilter, binspec, colspec, status);

    strcpy(urltype, "memkeep://");   /* URL type for pre-existing memory file */

    *status = urltype2driver(urltype, &driver);

    if (*status > 0)
    {
        ffpmsg("could not find driver for pre-existing memory file: (ffomem)");
        return(*status);
    }

    /* call driver routine to open the memory file */
    FFLOCK;  /* lock this while searching for vacant handle */
    *status =   mem_openmem( buffptr, buffsize,deltasize,
                            mem_realloc,  &handle);
    FFUNLOCK;

    if (*status > 0)
    {
         ffpmsg("failed to open pre-existing memory file: (ffomem)");
         return(*status);
    }

        /* get initial file size */
    *status = (*driverTable[driver].size)(handle, &filesize);

    if (*status > 0)
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed get the size of the memory file: (ffomem)");
        return(*status);
    }

        /* allocate fitsfile structure and initialize = 0 */
    *fptr = (fitsfile *) calloc(1, sizeof(fitsfile));

    if (!(*fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffomem)");
        ffpmsg(url);
        return(*status = MEMORY_ALLOCATION);
    }

        /* allocate FITSfile structure and initialize = 0 */
    (*fptr)->Fptr = (FITSfile *) calloc(1, sizeof(FITSfile));

    if (!((*fptr)->Fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffomem)");
        ffpmsg(url);
        free(*fptr);
        *fptr = 0;       
        return(*status = MEMORY_ALLOCATION);
    }

    slen = strlen(url) + 1;
    slen = maxvalue(slen, 32); /* reserve at least 32 chars */ 
    ((*fptr)->Fptr)->filename = (char *) malloc(slen); /* mem for file name */

    if ( !(((*fptr)->Fptr)->filename) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for filename: (ffomem)");
        ffpmsg(url);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for headstart array */
    ((*fptr)->Fptr)->headstart = (LONGLONG *) calloc(1001, sizeof(LONGLONG)); 

    if ( !(((*fptr)->Fptr)->headstart) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for headstart array: (ffomem)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for file I/O buffers */
    ((*fptr)->Fptr)->iobuffer = (char *) calloc(NIOBUF, IOBUFLEN);

    if ( !(((*fptr)->Fptr)->iobuffer) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for iobuffer array: (ffomem)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->headstart);    /* free memory for headstart array */
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* initialize the ageindex array (relative age of the I/O buffers) */
    /* and initialize the bufrecnum array as being empty */
    for (ii = 0; ii < NIOBUF; ii++)  {
        ((*fptr)->Fptr)->ageindex[ii] = ii;
        ((*fptr)->Fptr)->bufrecnum[ii] = -1;
    }

        /* store the parameters describing the file */
    ((*fptr)->Fptr)->MAXHDU = 1000;              /* initial size of headstart */
    ((*fptr)->Fptr)->filehandle = handle;        /* file handle */
    ((*fptr)->Fptr)->driver = driver;            /* driver number */
    strcpy(((*fptr)->Fptr)->filename, url);      /* full input filename */
    ((*fptr)->Fptr)->filesize = filesize;        /* physical file size */
    ((*fptr)->Fptr)->logfilesize = filesize;     /* logical file size */
    ((*fptr)->Fptr)->writemode = mode;      /* read-write mode    */
    ((*fptr)->Fptr)->datastart = DATA_UNDEFINED; /* unknown start of data */
    ((*fptr)->Fptr)->curbuf = -1;             /* undefined current IO buffer */
    ((*fptr)->Fptr)->open_count = 1;     /* structure is currently used once */
    ((*fptr)->Fptr)->validcode = VALIDSTRUC; /* flag denoting valid structure */

    ffldrc(*fptr, 0, REPORT_EOF, status);     /* load first record */

    fits_store_Fptr( (*fptr)->Fptr, status);  /* store Fptr address */

    if (ffrhdu(*fptr, &hdutyp, status) > 0)  /* determine HDU structure */
    {
        ffpmsg(
          "ffomem could not interpret primary array header of file: (ffomem)");
        ffpmsg(url);

        if (*status == UNKNOWN_REC)
           ffpmsg("This does not look like a FITS file.");

        ffclos(*fptr, status);
        *fptr = 0;              /* return null file pointer */
    }

    /* ---------------------------------------------------------- */
    /* move to desired extension, if specified as part of the URL */
    /* ---------------------------------------------------------- */

    imagecolname[0] = '\0';
    rowexpress[0] = '\0';

    if (*extspec)
    {
       /* parse the extension specifier into individual parameters */
       ffexts(extspec, &extnum, 
         extname, &extvers, &movetotype, imagecolname, rowexpress, status);


      if (*status > 0)
          return(*status);

      if (extnum)
      {
        ffmahd(*fptr, extnum + 1, &hdutyp, status);
      }
      else if (*extname) /* move to named extension, if specified */
      {
        ffmnhd(*fptr, movetotype, extname, extvers, status);
      }

      if (*status > 0)
      {
        ffpmsg("ffomem could not move to the specified extension:");
        if (extnum > 0)
        {
          sprintf(errmsg,
          " extension number %d doesn't exist or couldn't be opened.",extnum);
          ffpmsg(errmsg);
        }
        else
        {
          sprintf(errmsg,
          " extension with EXTNAME = %s,", extname);
          ffpmsg(errmsg);

          if (extvers)
          {
             sprintf(errmsg,
             "           and with EXTVERS = %d,", extvers);
             ffpmsg(errmsg);
          }

          if (movetotype != ANY_HDU)
          {
             sprintf(errmsg,
             "           and with XTENSION = %s,", hdtype[movetotype]);
             ffpmsg(errmsg);
          }

          ffpmsg(" doesn't exist or couldn't be opened.");
        }
        return(*status);
      }
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdkopn(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file on magnetic disk with either readonly or 
  read/write access.  The routine does not support CFITSIO's extended
  filename syntax and simply uses the entire input 'name' string as
  the name of the file.
*/
{
    if (*status > 0)
        return(*status);

    *status = OPEN_DISK_FILE;

    ffopen(fptr, name, mode, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdopn(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access. and
  move to the first HDU that contains 'interesting' data, if the primary
  array contains a null image (i.e., NAXIS = 0). 
*/
{
    if (*status > 0)
        return(*status);

    *status = SKIP_NULL_PRIMARY;

    ffopen(fptr, name, mode, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffeopn(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           char *extlist,        /* I - list of 'good' extensions to move to */
           int *hdutype,         /* O - type of extension that is moved to  */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access. and
  if the primary array contains a null image (i.e., NAXIS = 0) then attempt to
  move to the first extension named in the extlist of extension names. If
  none are found, then simply move to the 2nd extension.
*/
{
    int hdunum, naxis, thdutype, gotext=0;
    char *ext, *textlist;
    char *saveptr;
  
    if (*status > 0)
        return(*status);

    if (ffopen(fptr, name, mode, status) > 0)
        return(*status);

    fits_get_hdu_num(*fptr, &hdunum); 
    fits_get_img_dim(*fptr, &naxis, status);

    if( (hdunum == 1) && (naxis == 0) ){ 
      /* look through the extension list */
      if( extlist ){
        gotext = 0;
	textlist = malloc(strlen(extlist) + 1);
	if (!textlist) {
	    *status = MEMORY_ALLOCATION;
	    return(*status);
	}

        strcpy(textlist, extlist);
        for(ext=(char *)ffstrtok(textlist, " ",&saveptr); ext != NULL; 
	    ext=(char *)ffstrtok(NULL," ",&saveptr)){
	    fits_movnam_hdu(*fptr, ANY_HDU, ext, 0, status);
	    if( *status == 0 ){
	      gotext = 1;
	      break;
	    } else {
	      *status = 0;
	    }
        }
        free(textlist);      
      }
      if( !gotext ){
        /* if all else fails, move to extension #2 and hope for the best */
        fits_movabs_hdu(*fptr, 2, &thdutype, status);
      }
    }
    fits_get_hdu_type(*fptr, hdutype, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftopn(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access. and
  move to the first HDU that contains 'interesting' table (not an image). 
*/
{
    int hdutype;

    if (*status > 0)
        return(*status);

    *status = SKIP_IMAGE;

    ffopen(fptr, name, mode, status);

    if (ffghdt(*fptr, &hdutype, status) <= 0) {
        if (hdutype == IMAGE_HDU)
            *status = NOT_TABLE;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffiopn(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access. and
  move to the first HDU that contains 'interesting' image (not an table). 
*/
{
    int hdutype;

    if (*status > 0)
        return(*status);

    *status = SKIP_TABLE;

    ffopen(fptr, name, mode, status);

    if (ffghdt(*fptr, &hdutype, status) <= 0) {
        if (hdutype != IMAGE_HDU)
            *status = NOT_IMAGE;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffopentest(int soname,       /* I - CFITSIO shared library version     */
                                 /*     application program (fitsio.h file) */
           fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access.
  First test that the SONAME of fitsio.h used to build the CFITSIO library
  is the same as was used in compiling the application program that
  links to the library.
*/
{ 
    if (soname != CFITSIO_SONAME)
    {
        printf("\nERROR: Mismatch in the CFITSIO_SONAME value in the fitsio.h include file\n");
	printf("that was used to build the CFITSIO library, and the value in the include file\n");
	printf("that was used when compiling the application program:\n");
	printf("   Version used to build the CFITSIO library   = %d\n",CFITSIO_SONAME);
	printf("   Version included by the application program = %d\n",soname);
	printf("\nFix this by recompiling and then relinking this application program \n");
	printf("with the CFITSIO library.\n");

        *status = FILE_NOT_OPENED;
	return(*status);
    }

    /* now call the normal file open routine */
    ffopen(fptr, name, mode, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffopen(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           const char *name,     /* I - full name of file to open           */
           int mode,             /* I - 0 = open readonly; 1 = read/write   */
           int *status)          /* IO - error status                       */
/*
  Open an existing FITS file with either readonly or read/write access.
*/
{
    fitsfile *newptr;
    int  ii, driver, hdutyp, hdunum, slen, writecopy, isopen;
    LONGLONG filesize;
    long rownum, nrows, goodrows;
    int extnum, extvers, handle, movetotype, tstatus = 0, only_one = 0;
    char urltype[MAX_PREFIX_LEN], infile[FLEN_FILENAME], outfile[FLEN_FILENAME];
    char origurltype[MAX_PREFIX_LEN], extspec[FLEN_FILENAME];
    char extname[FLEN_VALUE], rowfilter[FLEN_FILENAME], tblname[FLEN_VALUE];
    char imagecolname[FLEN_VALUE], rowexpress[FLEN_FILENAME];
    char binspec[FLEN_FILENAME], colspec[FLEN_FILENAME], pixfilter[FLEN_FILENAME];
    char histfilename[FLEN_FILENAME];
    char filtfilename[FLEN_FILENAME], compspec[FLEN_FILENAME];
    char wtcol[FLEN_VALUE];
    char minname[4][FLEN_VALUE], maxname[4][FLEN_VALUE];
    char binname[4][FLEN_VALUE];

    char *url;
    double minin[4], maxin[4], binsizein[4], weight;
    int imagetype, naxis = 1, haxis, recip;
    int skip_null = 0, skip_image = 0, skip_table = 0, open_disk_file = 0;
    char colname[4][FLEN_VALUE];
    char errmsg[FLEN_ERRMSG];
    char *hdtype[3] = {"IMAGE", "TABLE", "BINTABLE"};
    char *rowselect = 0;

    if (*status > 0)
        return(*status);

    if (*status == SKIP_NULL_PRIMARY)
    {
      /* this special status value is used as a flag by ffdopn to tell */
      /* ffopen to skip over a null primary array when opening the file. */

       skip_null = 1;
       *status = 0;
    }
    else if (*status == SKIP_IMAGE)
    {
      /* this special status value is used as a flag by fftopn to tell */
      /* ffopen to move to 1st significant table when opening the file. */

       skip_image = 1;
       *status = 0;
    }
    else if (*status == SKIP_TABLE)
    {
      /* this special status value is used as a flag by ffiopn to tell */
      /* ffopen to move to 1st significant image when opening the file. */

       skip_table = 1;
       *status = 0;
    }
    else if (*status == OPEN_DISK_FILE)
    {
      /* this special status value is used as a flag by ffdkopn to tell */
      /* ffopen to not interpret the input filename using CFITSIO's    */
      /* extended filename syntax, and simply open the specified disk file */

       open_disk_file = 1;
       *status = 0;
    }
    
    *fptr = 0;              /* initialize null file pointer */
    writecopy = 0;  /* have we made a write-able copy of the input file? */

    if (need_to_initialize) {          /* this is called only once */
       *status = fits_init_cfitsio();
    }
    
    if (*status > 0)
        return(*status);

    url = (char *) name;
    while (*url == ' ')  /* ignore leading spaces in the filename */
        url++;

    if (*url == '\0')
    {
        ffpmsg("Name of file to open is blank. (ffopen)");
        return(*status = FILE_NOT_OPENED);
    }

    if (open_disk_file)
    {
      /* treat the input URL literally as the name of the file to open */
      /* and don't try to parse the URL using the extended filename syntax */
      
        if (strlen(url) > FLEN_FILENAME - 1) {
            ffpmsg("Name of file to open is too long. (ffopen)");
            return(*status = FILE_NOT_OPENED);
        }

        strcpy(infile,url);
        strcpy(urltype, "file://");
        outfile[0] = '\0';
        extspec[0] = '\0';
        binspec[0] = '\0';
        colspec[0] = '\0';
        rowfilter[0] = '\0';
        pixfilter[0] = '\0';
        compspec[0] = '\0';
    }
    else
    {
        /* parse the input file specification */

        /* NOTE: This routine tests that all the strings do not */
	/* overflow the standard buffer sizes (FLEN_FILENAME, etc.) */
	/* therefore in general we do not have to worry about buffer */
	/* overflow of any of the returned strings. */
	
        /* call the newer version of this parsing routine that supports 'compspec' */
        ffifile2(url, urltype, infile, outfile, extspec,
              rowfilter, binspec, colspec, pixfilter, compspec, status);
    }
    
    if (*status > 0)
    {
        ffpmsg("could not parse the input filename: (ffopen)");
        ffpmsg(url);
        return(*status);
    }

    imagecolname[0] = '\0';
    rowexpress[0] = '\0';

    if (*extspec)
    {
       slen = strlen(extspec);
       if (extspec[slen - 1] == '#') {  /* special symbol to mean only copy this extension */
           extspec[slen - 1] = '\0';
	   only_one = 1;
       }

       /* parse the extension specifier into individual parameters */
       ffexts(extspec, &extnum, 
         extname, &extvers, &movetotype, imagecolname, rowexpress, status);

      if (*status > 0)
          return(*status);
    }

    /*-------------------------------------------------------------------*/
    /* special cases:                                                    */
    /*-------------------------------------------------------------------*/

    histfilename[0] = '\0';
    filtfilename[0] = '\0';
    if (*outfile && (*binspec || *imagecolname || *pixfilter))
    {
        /* if binspec or imagecolumn are specified, then the  */
        /* output file name is intended for the final image,  */
        /* and not a copy of the input file.                  */

        strcpy(histfilename, outfile);
        outfile[0] = '\0';
    }
    else if (*outfile && (*rowfilter || *colspec))
    {
        /* if rowfilter or colspece are specified, then the    */
        /* output file name is intended for the filtered file  */
        /* and not a copy of the input file.                   */

        strcpy(filtfilename, outfile);
        outfile[0] = '\0';
    }

    /*-------------------------------------------------------------------*/
    /* check if this same file is already open, and if so, attach to it  */
    /*-------------------------------------------------------------------*/

    FFLOCK;
    if (fits_already_open(fptr, url, urltype, infile, extspec, rowfilter,
            binspec, colspec, mode, &isopen, status) > 0)
    {
        FFUNLOCK;
        return(*status);
    }
    FFUNLOCK;

    if (isopen) {
       goto move2hdu;  
    }

    /* get the driver number corresponding to this urltype */
    *status = urltype2driver(urltype, &driver);

    if (*status > 0)
    {
        ffpmsg("could not find driver for this file: (ffopen)");
        ffpmsg(urltype);
        ffpmsg(url);
        return(*status);
    }

    /*-------------------------------------------------------------------
        deal with all those messy special cases which may require that
        a different driver be used:
            - is disk file compressed?
            - are ftp:, gsiftp:, or http: files compressed?
            - has user requested that a local copy be made of
              the ftp or http file?
      -------------------------------------------------------------------*/

    if (driverTable[driver].checkfile)
    {
        strcpy(origurltype,urltype);  /* Save the urltype */

        /* 'checkfile' may modify the urltype, infile and outfile strings */
        *status =  (*driverTable[driver].checkfile)(urltype, infile, outfile);

        if (*status)
        {
            ffpmsg("checkfile failed for this file: (ffopen)");
            ffpmsg(url);
            return(*status);
        }

        if (strcmp(origurltype, urltype))  /* did driver changed on us? */
        {
            *status = urltype2driver(urltype, &driver);
            if (*status > 0)
            {
                ffpmsg("could not change driver for this file: (ffopen)");
                ffpmsg(url);
                ffpmsg(urltype);
                return(*status);
            }
        }
    }

    /* call appropriate driver to open the file */
    if (driverTable[driver].open)
    {
        FFLOCK;  /* lock this while searching for vacant handle */
        *status =  (*driverTable[driver].open)(infile, mode, &handle);
        FFUNLOCK;
        if (*status > 0)
        {
            ffpmsg("failed to find or open the following file: (ffopen)");
            ffpmsg(url);
            return(*status);
       }
    }
    else
    {
        ffpmsg("cannot open an existing file of this type: (ffopen)");
        ffpmsg(url);
        return(*status = FILE_NOT_OPENED);
    }

        /* get initial file size */
    *status = (*driverTable[driver].size)(handle, &filesize);
    if (*status > 0)
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed get the size of the following file: (ffopen)");
        ffpmsg(url);
        return(*status);
    }

        /* allocate fitsfile structure and initialize = 0 */
    *fptr = (fitsfile *) calloc(1, sizeof(fitsfile));

    if (!(*fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffopen)");
        ffpmsg(url);
        return(*status = MEMORY_ALLOCATION);
    }

        /* allocate FITSfile structure and initialize = 0 */
    (*fptr)->Fptr = (FITSfile *) calloc(1, sizeof(FITSfile));

    if (!((*fptr)->Fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffopen)");
        ffpmsg(url);
        free(*fptr);
        *fptr = 0;       
        return(*status = MEMORY_ALLOCATION);
    }

    slen = strlen(url) + 1;
    slen = maxvalue(slen, 32); /* reserve at least 32 chars */ 
    ((*fptr)->Fptr)->filename = (char *) malloc(slen); /* mem for file name */

    if ( !(((*fptr)->Fptr)->filename) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for filename: (ffopen)");
        ffpmsg(url);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for headstart array */
    ((*fptr)->Fptr)->headstart = (LONGLONG *) calloc(1001, sizeof(LONGLONG));

    if ( !(((*fptr)->Fptr)->headstart) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for headstart array: (ffopen)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for file I/O buffers */
    ((*fptr)->Fptr)->iobuffer = (char *) calloc(NIOBUF, IOBUFLEN);

    if ( !(((*fptr)->Fptr)->iobuffer) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for iobuffer array: (ffopen)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->headstart);    /* free memory for headstart array */
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* initialize the ageindex array (relative age of the I/O buffers) */
    /* and initialize the bufrecnum array as being empty */
    for (ii = 0; ii < NIOBUF; ii++)  {
        ((*fptr)->Fptr)->ageindex[ii] = ii;
        ((*fptr)->Fptr)->bufrecnum[ii] = -1;
    }

        /* store the parameters describing the file */
    ((*fptr)->Fptr)->MAXHDU = 1000;              /* initial size of headstart */
    ((*fptr)->Fptr)->filehandle = handle;        /* file handle */
    ((*fptr)->Fptr)->driver = driver;            /* driver number */
    strcpy(((*fptr)->Fptr)->filename, url);      /* full input filename */
    ((*fptr)->Fptr)->filesize = filesize;        /* physical file size */
    ((*fptr)->Fptr)->logfilesize = filesize;     /* logical file size */
    ((*fptr)->Fptr)->writemode = mode;           /* read-write mode    */
    ((*fptr)->Fptr)->datastart = DATA_UNDEFINED; /* unknown start of data */
    ((*fptr)->Fptr)->curbuf = -1;            /* undefined current IO buffer */
    ((*fptr)->Fptr)->open_count = 1;      /* structure is currently used once */
    ((*fptr)->Fptr)->validcode = VALIDSTRUC; /* flag denoting valid structure */
    ((*fptr)->Fptr)->only_one = only_one; /* flag denoting only copy single extension */

    ffldrc(*fptr, 0, REPORT_EOF, status);     /* load first record */

    fits_store_Fptr( (*fptr)->Fptr, status);  /* store Fptr address */

    if (ffrhdu(*fptr, &hdutyp, status) > 0)  /* determine HDU structure */
    {
        ffpmsg(
          "ffopen could not interpret primary array header of file: ");
        ffpmsg(url);

        if (*status == UNKNOWN_REC)
           ffpmsg("This does not look like a FITS file.");

        ffclos(*fptr, status);
        *fptr = 0;              /* return null file pointer */
        return(*status);
    }

    /* ------------------------------------------------------------- */
    /* At this point, the input file has been opened. If outfile was */
    /* specified, then we have opened a copy of the file, not the    */
    /* original file so it is safe to modify it if necessary         */
    /* ------------------------------------------------------------- */

    if (*outfile)
        writecopy = 1;  

move2hdu:

    /* ---------------------------------------------------------- */
    /* move to desired extension, if specified as part of the URL */
    /* ---------------------------------------------------------- */

    if (*extspec)
    {
      if (extnum)  /* extension number was specified */
      {
        ffmahd(*fptr, extnum + 1, &hdutyp, status);
      }
      else if (*extname) /* move to named extension, if specified */
      {
        ffmnhd(*fptr, movetotype, extname, extvers, status);
      }

      if (*status > 0)  /* clean up after error */
      {
        ffpmsg("ffopen could not move to the specified extension:");
        if (extnum > 0)
        {
          sprintf(errmsg,
          " extension number %d doesn't exist or couldn't be opened.",extnum);
          ffpmsg(errmsg);
        }
        else
        {
          sprintf(errmsg,
          " extension with EXTNAME = %s,", extname);
          ffpmsg(errmsg);

          if (extvers)
          {
             sprintf(errmsg,
             "           and with EXTVERS = %d,", extvers);
             ffpmsg(errmsg);
          }

          if (movetotype != ANY_HDU)
          {
             sprintf(errmsg,
             "           and with XTENSION = %s,", hdtype[movetotype]);
             ffpmsg(errmsg);
          }

          ffpmsg(" doesn't exist or couldn't be opened.");
        }

        ffclos(*fptr, status);
        *fptr = 0;              /* return null file pointer */
        return(*status);
      }
    }
    else if (skip_null || skip_image || skip_table ||
            (*imagecolname || *colspec || *rowfilter || *binspec))
    {
      /* ------------------------------------------------------------------

      If no explicit extension specifier is given as part of the file
      name, and, if a) skip_null is true (set if ffopen is called by
      ffdopn) or b) skip_image or skip_table is true (set if ffopen is
      called by fftopn or ffdopn) or c) other file filters are
      specified, then CFITSIO will attempt to move to the first
      'interesting' HDU after opening an existing FITS file (or to
      first interesting table HDU if skip_image is true);

      An 'interesting' HDU is defined to be either an image with NAXIS
      > 0 (i.e., not a null array) or a table which has an EXTNAME
      value which does not contain any of the following strings:
         'GTI'  - Good Time Interval extension
         'OBSTABLE'  - used in Beppo SAX data files

      The main purpose for this is to allow CFITSIO to skip over a null
      primary and other non-interesting HDUs when opening an existing
      file, and move directly to the first extension that contains
      significant data.
      ------------------------------------------------------------------ */

      fits_get_hdu_num(*fptr, &hdunum);
      if (hdunum == 1) {

        fits_get_img_dim(*fptr, &naxis, status);

        if (naxis == 0 || skip_image) /* skip primary array */
        {
          while(1) 
          {
            /* see if the next HDU is 'interesting' */
            if (fits_movrel_hdu(*fptr, 1, &hdutyp, status))
            {
               if (*status == END_OF_FILE)
                  *status = 0;  /* reset expected error */

               /* didn't find an interesting HDU so move back to beginning */
               fits_movabs_hdu(*fptr, 1, &hdutyp, status);
               break;
            }

            if (hdutyp == IMAGE_HDU && skip_image) {

                continue;   /* skip images */

            } else if (hdutyp != IMAGE_HDU && skip_table) {

                continue;   /* skip tables */

            } else if (hdutyp == IMAGE_HDU) {

               fits_get_img_dim(*fptr, &naxis, status);
               if (naxis > 0)
                  break;  /* found a non-null image */

            } else {

               tstatus = 0;
               tblname[0] = '\0';
               fits_read_key(*fptr, TSTRING, "EXTNAME", tblname, NULL,&tstatus);

               if ( (!strstr(tblname, "GTI") && !strstr(tblname, "gti")) &&
                    fits_strncasecmp(tblname, "OBSTABLE", 8) )
                  break;  /* found an interesting table */
            }
          }  /* end while */
        }
      } /* end if (hdunum==1) */
    }

    if (*imagecolname)
    {
       /* ----------------------------------------------------------------- */
       /* we need to open an image contained in a single table cell         */
       /* First, determine which row of the table to use.                   */
       /* ----------------------------------------------------------------- */

       if (isdigit((int) *rowexpress))  /* is the row specification a number? */
       {
          sscanf(rowexpress, "%ld", &rownum);
          if (rownum < 1)
          {
             ffpmsg("illegal rownum for image cell:");
             ffpmsg(rowexpress);
             ffpmsg("Could not open the following image in a table cell:");
             ffpmsg(extspec);
             ffclos(*fptr, status);
             *fptr = 0;              /* return null file pointer */
             return(*status = BAD_ROW_NUM);
          }
       }
       else if (fits_find_first_row(*fptr, rowexpress, &rownum, status) > 0)
       {
          ffpmsg("Failed to find row matching this expression:");
          ffpmsg(rowexpress);
          ffpmsg("Could not open the following image in a table cell:");
          ffpmsg(extspec);
          ffclos(*fptr, status);
          *fptr = 0;              /* return null file pointer */
          return(*status);
       }

       if (rownum == 0)
       {
          ffpmsg("row statisfying this expression doesn't exist::");
          ffpmsg(rowexpress);
          ffpmsg("Could not open the following image in a table cell:");
          ffpmsg(extspec);
          ffclos(*fptr, status);
          *fptr = 0;              /* return null file pointer */
          return(*status = BAD_ROW_NUM);
       }

       /* determine the name of the new file to contain copy of the image */
       if (*histfilename && !(*pixfilter) )
           strcpy(outfile, histfilename); /* the original outfile name */
       else
           strcpy(outfile, "mem://_1");  /* create image file in memory */

       /* Copy the image into new primary array and open it as the current */
       /* fptr.  This will close the table that contains the original image. */

       /* create new empty file to hold copy of the image */
       if (ffinit(&newptr, outfile, status) > 0)
       {
          ffpmsg("failed to create file for copy of image in table cell:");
          ffpmsg(outfile);
          return(*status);
       }
      
       if (fits_copy_cell2image(*fptr, newptr, imagecolname, rownum,
                                status) > 0)
       {
          ffpmsg("Failed to copy table cell to new primary array:");
          ffpmsg(extspec);
          ffclos(*fptr, status);
          *fptr = 0;              /* return null file pointer */
          return(*status);
       }

       /* close the original file and set fptr to the new image */
       ffclos(*fptr, status);

       *fptr = newptr; /* reset the pointer to the new table */

       writecopy = 1;  /* we are now dealing with a copy of the original file */

       /* add some HISTORY; fits_copy_image_cell also wrote HISTORY keywords */
       
/*  disable this; leave it up to calling routine to write any HISTORY keywords
       if (*extname)
        sprintf(card,"HISTORY  in HDU '%.16s' of file '%.36s'",extname,infile);
       else
        sprintf(card,"HISTORY  in HDU %d of file '%.45s'", extnum, infile);

       ffprec(*fptr, card, status);
*/
    }

    /* --------------------------------------------------------------------- */
    /* edit columns (and/or keywords) in the table, if specified in the URL  */
    /* --------------------------------------------------------------------- */
 
    if (*colspec)
    {
       /* the column specifier will modify the file, so make sure */
       /* we are already dealing with a copy, or else make a new copy */

       if (!writecopy)  /* Is the current file already a copy? */
           writecopy = fits_is_this_a_copy(urltype);

       if (!writecopy)
       {
           if (*filtfilename && *outfile == '\0')
               strcpy(outfile, filtfilename); /* the original outfile name */
           else
               strcpy(outfile, "mem://_1");   /* will create copy in memory */

           writecopy = 1;
       }
       else
       {
           ((*fptr)->Fptr)->writemode = READWRITE; /* we have write access */
           outfile[0] = '\0';
       }

       if (ffedit_columns(fptr, outfile, colspec, status) > 0)
       {
           ffpmsg("editing columns in input table failed (ffopen)");
           ffpmsg(" while trying to perform the following operation:");
           ffpmsg(colspec);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status);
       }
    }

    /* ------------------------------------------------------------------- */
    /* select rows from the table, if specified in the URL                 */
    /* or select a subimage (if this is an image HDU and not a table)      */
    /* ------------------------------------------------------------------- */
 
    if (*rowfilter)
    {
     fits_get_hdu_type(*fptr, &hdutyp, status);  /* get type of HDU */
     if (hdutyp == IMAGE_HDU)
     {
        /* this is an image so 'rowfilter' is an image section specification */

        if (*filtfilename && *outfile == '\0')
            strcpy(outfile, filtfilename); /* the original outfile name */
        else if (*outfile == '\0') /* output file name not already defined? */
            strcpy(outfile, "mem://_2");  /* will create file in memory */

        /* create new file containing the image section, plus a copy of */
        /* any other HDUs that exist in the input file.  This routine   */
        /* will close the original image file and return a pointer      */
        /* to the new file. */

        if (fits_select_image_section(fptr, outfile, rowfilter, status) > 0)
        {
           ffpmsg("on-the-fly selection of image section failed (ffopen)");
           ffpmsg(" while trying to use the following section filter:");
           ffpmsg(rowfilter);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status);
        }
     }
     else
     {
       /* this is a table HDU, so the rowfilter is really a row filter */

      if (*binspec)
      {
        /*  since we are going to make a histogram of the selected rows,   */
        /*  it would be a waste of time and memory to make a whole copy of */
        /*  the selected rows.  Instead, just construct an array of TRUE   */
        /*  or FALSE values that indicate which rows are to be included    */
        /*  in the histogram and pass that to the histogram generating     */
        /*  routine                                                        */

        fits_get_num_rows(*fptr, &nrows, status);  /* get no. of rows */

        rowselect = (char *) calloc(nrows, 1);
        if (!rowselect)
        {
           ffpmsg(
           "failed to allocate memory for selected columns array (ffopen)");
           ffpmsg(" while trying to select rows with the following filter:");
           ffpmsg(rowfilter);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status = MEMORY_ALLOCATION);
        }

        if (fits_find_rows(*fptr, rowfilter, 1L, nrows, &goodrows,
            rowselect, status) > 0)
        {
           ffpmsg("selection of rows in input table failed (ffopen)");
           ffpmsg(" while trying to select rows with the following filter:");
           ffpmsg(rowfilter);
           free(rowselect);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status);
        }
      }
      else
      {
        if (!writecopy)  /* Is the current file already a copy? */
           writecopy = fits_is_this_a_copy(urltype);

        if (!writecopy)
        {
           if (*filtfilename && *outfile == '\0')
               strcpy(outfile, filtfilename); /* the original outfile name */
           else if (*outfile == '\0') /* output filename not already defined? */
               strcpy(outfile, "mem://_2");  /* will create copy in memory */
        }
        else
        {
           ((*fptr)->Fptr)->writemode = READWRITE; /* we have write access */
           outfile[0] = '\0';
        }

        /* select rows in the table.  If a copy of the input file has */
        /* not already been made, then this routine will make a copy */
        /* and then close the input file, so that the modifications will */
        /* only be made on the copy, not the original */

        if (ffselect_table(fptr, outfile, rowfilter, status) > 0)
        {
          ffpmsg("on-the-fly selection of rows in input table failed (ffopen)");
           ffpmsg(" while trying to select rows with the following filter:");
           ffpmsg(rowfilter);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status);
        }

        /* write history records */
        ffphis(*fptr, 
        "CFITSIO used the following filtering expression to create this table:",
        status);
        ffphis(*fptr, name, status);

      }   /* end of no binspec case */
     }   /* end of table HDU case */
    }  /* end of rowfilter exists case */

    /* ------------------------------------------------------------------- */
    /* make an image histogram by binning columns, if specified in the URL */
    /* ------------------------------------------------------------------- */
 
    if (*binspec)
    {
       if (*histfilename  && !(*pixfilter) )
           strcpy(outfile, histfilename); /* the original outfile name */
       else
           strcpy(outfile, "mem://_3");  /* create histogram in memory */
                                         /* if not already copied the file */ 

       /* parse the binning specifier into individual parameters */
       ffbins(binspec, &imagetype, &haxis, colname, 
                          minin, maxin, binsizein, 
                          minname, maxname, binname,
                          &weight, wtcol, &recip, status);

       /* Create the histogram primary array and open it as the current fptr */
       /* This will close the table that was used to create the histogram. */
       ffhist2(fptr, outfile, imagetype, haxis, colname, minin, maxin,
              binsizein, minname, maxname, binname,
              weight, wtcol, recip, rowselect, status);

       if (rowselect)
          free(rowselect);

       if (*status > 0)
       {
      ffpmsg("on-the-fly histogramming of input table failed (ffopen)");
      ffpmsg(" while trying to execute the following histogram specification:");
      ffpmsg(binspec);
           ffclos(*fptr, status);
           *fptr = 0;              /* return null file pointer */
           return(*status);
       }

        /* write history records */
        ffphis(*fptr,
        "CFITSIO used the following expression to create this histogram:", 
        status);
        ffphis(*fptr, name, status);
    }

    if (*pixfilter)
    {
       if (*histfilename)
           strcpy(outfile, histfilename); /* the original outfile name */
       else
           strcpy(outfile, "mem://_4");  /* create in memory */
                                         /* if not already copied the file */ 

       /* Ensure type of HDU is consistent with pixel filtering */
       fits_get_hdu_type(*fptr, &hdutyp, status);  /* get type of HDU */
       if (hdutyp == IMAGE_HDU) {

          pixel_filter_helper(fptr, outfile, pixfilter, status);

          if (*status > 0) {
             ffpmsg("pixel filtering of input image failed (ffopen)");
             ffpmsg(" while trying to execute the following:");
             ffpmsg(pixfilter);
             ffclos(*fptr, status);
             *fptr = 0;              /* return null file pointer */
             return(*status);
          }

          /* write history records */
          ffphis(*fptr,
          "CFITSIO used the following expression to create this image:",
          status);
          ffphis(*fptr, name, status);
       }
       else
       {
          ffpmsg("cannot use pixel filter on non-IMAGE HDU");
          ffpmsg(pixfilter);
          ffclos(*fptr, status);
          *fptr = 0;              /* return null file pointer */
          *status = NOT_IMAGE;
          return(*status);
       }
    }

   /* parse and save image compression specification, if given */
   if (*compspec) {
      ffparsecompspec(*fptr, compspec, status);
   }
 
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffreopen(fitsfile *openfptr, /* I - FITS file pointer to open file  */ 
             fitsfile **newfptr,  /* O - pointer to new re opened file   */
             int *status)        /* IO - error status                   */
/*
  Reopen an existing FITS file with either readonly or read/write access.
  The reopened file shares the same FITSfile structure but may point to a
  different HDU within the file.
*/
{
    if (*status > 0)
        return(*status);

    /* check that the open file pointer is valid */
    if (!openfptr)
        return(*status = NULL_INPUT_PTR);
    else if ((openfptr->Fptr)->validcode != VALIDSTRUC) /* check magic value */
        return(*status = BAD_FILEPTR); 

        /* allocate fitsfile structure and initialize = 0 */
    *newfptr = (fitsfile *) calloc(1, sizeof(fitsfile));

    (*newfptr)->Fptr = openfptr->Fptr; /* both point to the same structure */
    (*newfptr)->HDUposition = 0;  /* set initial position to primary array */
    (((*newfptr)->Fptr)->open_count)++;   /* increment the file usage counter */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_store_Fptr(FITSfile *Fptr,  /* O - FITS file pointer               */ 
           int *status)              /* IO - error status                   */
/*
   store the new Fptr address for future use by fits_already_open 
*/
{
    int ii;

    if (*status > 0)
        return(*status);

    FFLOCK;
    for (ii = 0; ii < NMAXFILES; ii++) {
        if (FptrTable[ii] == 0) {
            FptrTable[ii] = Fptr;
            break;
        }
    }
    FFUNLOCK;
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_clear_Fptr(FITSfile *Fptr,  /* O - FITS file pointer               */ 
           int *status)              /* IO - error status                   */
/*
   clear the Fptr address from the Fptr Table  
*/
{
    int ii;

    FFLOCK;
    for (ii = 0; ii < NMAXFILES; ii++) {
        if (FptrTable[ii] == Fptr) {
            FptrTable[ii] = 0;
            break;
        }
    }
    FFUNLOCK;
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_already_open(fitsfile **fptr, /* I/O - FITS file pointer       */ 
           char *url, 
           char *urltype, 
           char *infile, 
           char *extspec, 
           char *rowfilter,
           char *binspec, 
           char *colspec, 
           int  mode,             /* I - 0 = open readonly; 1 = read/write   */
           int  *isopen,          /* O - 1 = file is already open            */
           int  *status)          /* IO - error status                       */
/*
  Check if the file to be opened is already open.  If so, then attach to it.
*/

   /* the input strings must not exceed the standard lengths */
   /* of FLEN_FILENAME, MAX_PREFIX_LEN, etc. */

     /*
       this function was changed so that for files of access method FILE://
       the file paths are compared using standard URL syntax and absolute
       paths (as opposed to relative paths). This eliminates some instances
       where a file is already opened but it is not realized because it
       was opened with another file path. For instance, if the CWD is
       /a/b/c and I open /a/b/c/foo.fits then open ./foo.fits the previous
       version of this function would not have reconized that the two files
       were the same. This version does recognize that the two files are
       the same.
     */
{
    FITSfile *oldFptr;
    int ii;
    char oldurltype[MAX_PREFIX_LEN], oldinfile[FLEN_FILENAME];
    char oldextspec[FLEN_FILENAME], oldoutfile[FLEN_FILENAME];
    char oldrowfilter[FLEN_FILENAME];
    char oldbinspec[FLEN_FILENAME], oldcolspec[FLEN_FILENAME];
    char cwd[FLEN_FILENAME];
    char tmpStr[FLEN_FILENAME];
    char tmpinfile[FLEN_FILENAME];

    *isopen = 0;

/*  When opening a file with readonly access then we simply let
    the operating system open the file again, instead of using the CFITSIO
    trick of attaching to the previously opened file.  This is required
    if CFITSIO is running in a multi-threaded environment, because 2 different
    threads cannot share the same FITSfile pointer.
    
    If the file is opened/reopened with write access, then the file MUST
    only be physically opened once..
*/ 
    if (mode == 0)
        return(*status);

    if(fits_strcasecmp(urltype,"FILE://") == 0)
      {
        fits_path2url(infile,tmpinfile,status);

        if(tmpinfile[0] != '/')
          {
            fits_get_cwd(cwd,status);
            strcat(cwd,"/");
 
            if (strlen(cwd) + strlen(tmpinfile) > FLEN_FILENAME-1) {
		  ffpmsg("File name is too long. (fits_already_open)");
                  return(*status = FILE_NOT_OPENED);
            }

            strcat(cwd,tmpinfile);
            fits_clean_url(cwd,tmpinfile,status);
          }
      }
    else
      strcpy(tmpinfile,infile);

    for (ii = 0; ii < NMAXFILES; ii++)   /* check every buffer */
    {
        if (FptrTable[ii] != 0)
        {
          oldFptr = FptrTable[ii];

          fits_parse_input_url(oldFptr->filename, oldurltype, 
                    oldinfile, oldoutfile, oldextspec, oldrowfilter, 
                    oldbinspec, oldcolspec, status);

          if (*status > 0)
          {
            ffpmsg("could not parse the previously opened filename: (ffopen)");
            ffpmsg(oldFptr->filename);
            return(*status);
          }

          if(fits_strcasecmp(oldurltype,"FILE://") == 0)
            {
              fits_path2url(oldinfile,tmpStr,status);
              
              if(tmpStr[0] != '/')
                {
                  fits_get_cwd(cwd,status);
                  strcat(cwd,"/");


                  strcat(cwd,tmpStr);
                  fits_clean_url(cwd,tmpStr,status);
                }

              strcpy(oldinfile,tmpStr);
            }

          if (!strcmp(urltype, oldurltype) && !strcmp(tmpinfile, oldinfile) )
          {
              /* identical type of file and root file name */

              if ( (!rowfilter[0] && !oldrowfilter[0] &&
                    !binspec[0]   && !oldbinspec[0] &&
                    !colspec[0]   && !oldcolspec[0])

                  /* no filtering or binning specs for either file, so */
                  /* this is a case where the same file is being reopened. */
                  /* It doesn't matter if the extensions are different */

                      ||   /* or */

                  (!strcmp(rowfilter, oldrowfilter) &&
                   !strcmp(binspec, oldbinspec)     &&
                   !strcmp(colspec, oldcolspec)     &&
                   !strcmp(extspec, oldextspec) ) )

                  /* filtering specs are given and are identical, and */
                  /* the same extension is specified */

              {
                  if (mode == READWRITE && oldFptr->writemode == READONLY)
                  {
                    /*
                      cannot assume that a file previously opened with READONLY
                      can now be written to (e.g., files on CDROM, or over the
                      the network, or STDIN), so return with an error.
                    */

                    ffpmsg(
                "cannot reopen file READWRITE when previously opened READONLY");
                    ffpmsg(url);
                    return(*status = FILE_NOT_OPENED);
                  }

                  *fptr = (fitsfile *) calloc(1, sizeof(fitsfile));

                  if (!(*fptr))
                  {
                     ffpmsg(
                   "failed to allocate structure for following file: (ffopen)");
                     ffpmsg(url);
                     return(*status = MEMORY_ALLOCATION);
                  }

                  (*fptr)->Fptr = oldFptr; /* point to the structure */
                  (*fptr)->HDUposition = 0;     /* set initial position */
                (((*fptr)->Fptr)->open_count)++;  /* increment usage counter */

                  if (binspec[0])  /* if binning specified, don't move */
                      extspec[0] = '\0';

                  /* all the filtering has already been applied, so ignore */
                  rowfilter[0] = '\0';
                  binspec[0] = '\0';
                  colspec[0] = '\0';

                  *isopen = 1;
              }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_is_this_a_copy(char *urltype) /* I - type of file */
/*
  specialized routine that returns 1 if the file is known to be a temporary
  copy of the originally opened file.  Otherwise it returns 0.
*/
{
  int iscopy;

  if (!strncmp(urltype, "mem", 3) )
     iscopy = 1;    /* file copy is in memory */
  else if (!strncmp(urltype, "compress", 8) )
     iscopy = 1;    /* compressed diskfile that is uncompressed in memory */
  else if (!strncmp(urltype, "http", 4) )
     iscopy = 1;    /* copied file using http protocol */
  else if (!strncmp(urltype, "ftp", 3) )
     iscopy = 1;    /* copied file using ftp protocol */
  else if (!strncmp(urltype, "gsiftp", 6) )
     iscopy = 1;    /* copied file using gsiftp protocol */
  else if (!strncpy(urltype, "stdin", 5) )
     iscopy = 1;    /* piped stdin has been copied to memory */
  else
     iscopy = 0;    /* file is not known to be a copy */
 
    return(iscopy);
}
/*--------------------------------------------------------------------------*/
static int find_quote(char **string)

/*  
    look for the closing single quote character in the input string
*/
{
    char *tstr;

    tstr = *string;

    while (*tstr) {
        if (*tstr == '\'') { /* found the closing quote */
           *string = tstr + 1;  /* set pointer to next char */
           return(0); 
        } else {  /* skip over any other character */
           tstr++;
        }
    }
    return(1);  /* opps, didn't find the closing character */
}
/*--------------------------------------------------------------------------*/
static int find_doublequote(char **string)

/*  
    look for the closing double quote character in the input string
*/
{
    char *tstr;

    tstr = *string;

    while (*tstr) {
        if (*tstr == '"') { /* found the closing quote */
           *string = tstr + 1;  /* set pointer to next char */
           return(0); 
        } else {  /* skip over any other character */
           tstr++;
        }
    }
    return(1);  /* opps, didn't find the closing character */
}

/*--------------------------------------------------------------------------*/
static int find_paren(char **string)

/*  
    look for the closing parenthesis character in the input string
*/
{
    char *tstr;

    tstr = *string;

    while (*tstr) {

        if (*tstr == ')') { /* found the closing parens */
           *string = tstr + 1;  /* set pointer to next char */
           return(0); 
        } else if (*tstr == '(') { /* found another level of parens */
           tstr++;
           if (find_paren(&tstr)) return(1); 
        } else if (*tstr == '[') { 
           tstr++;
           if (find_bracket(&tstr)) return(1);
        } else if (*tstr == '{') { 
           tstr++;
           if (find_curlybracket(&tstr)) return(1);
        } else if (*tstr == '"') { 
           tstr++;
           if (find_doublequote(&tstr)) return(1);
        } else if (*tstr == '\'') { 
           tstr++;
           if (find_quote(&tstr)) return(1);
        } else { 
           tstr++;
        }
    }
    return(1);  /* opps, didn't find the closing character */
}
/*--------------------------------------------------------------------------*/
static int find_bracket(char **string)

/*  
    look for the closing bracket character in the input string
*/
{
    char *tstr;

    tstr = *string;

    while (*tstr) {
        if (*tstr == ']') { /* found the closing bracket */
           *string = tstr + 1;  /* set pointer to next char */
           return(0); 
        } else if (*tstr == '(') { /* found another level of parens */
           tstr++;
           if (find_paren(&tstr)) return(1); 
        } else if (*tstr == '[') { 
           tstr++;
           if (find_bracket(&tstr)) return(1);
        } else if (*tstr == '{') { 
           tstr++;
           if (find_curlybracket(&tstr)) return(1);
        } else if (*tstr == '"') { 
           tstr++;
           if (find_doublequote(&tstr)) return(1);
        } else if (*tstr == '\'') { 
           tstr++;
           if (find_quote(&tstr)) return(1);
        } else { 
           tstr++;
        }
    }
    return(1);  /* opps, didn't find the closing character */
}
/*--------------------------------------------------------------------------*/
static int find_curlybracket(char **string)

/*  
    look for the closing curly bracket character in the input string
*/
{
    char *tstr;

    tstr = *string;

    while (*tstr) {
        if (*tstr == '}') { /* found the closing curly bracket */
           *string = tstr + 1;  /* set pointer to next char */
           return(0); 
        } else if (*tstr == '(') { /* found another level of parens */
           tstr++;
           if (find_paren(&tstr)) return(1); 
        } else if (*tstr == '[') { 
           tstr++;
           if (find_bracket(&tstr)) return(1);
        } else if (*tstr == '{') { 
           tstr++;
           if (find_curlybracket(&tstr)) return(1);
        } else if (*tstr == '"') { 
           tstr++;
           if (find_doublequote(&tstr)) return(1);
        } else if (*tstr == '\'') { 
           tstr++;
           if (find_quote(&tstr)) return(1);
        } else { 
           tstr++;
        }
    }
    return(1);  /* opps, didn't find the closing character */
}
/*--------------------------------------------------------------------------*/
int comma2semicolon(char *string)

/*  
    replace commas with semicolons, unless the comma is within a quoted or bracketed expression 
*/
{
    char *tstr;

    tstr = string;

    while (*tstr) {

        if (*tstr == ',') { /* found a comma */
           *tstr = ';';
           tstr++;
        } else if (*tstr == '(') { /* found another level of parens */
           tstr++;
           if (find_paren(&tstr)) return(1); 
        } else if (*tstr == '[') { 
           tstr++;
           if (find_bracket(&tstr)) return(1);
        } else if (*tstr == '{') { 
           tstr++;
           if (find_curlybracket(&tstr)) return(1);
        } else if (*tstr == '"') { 
           tstr++;
           if (find_doublequote(&tstr)) return(1);
        } else if (*tstr == '\'') { 
           tstr++;
           if (find_quote(&tstr)) return(1);
        } else { 
           tstr++;
        }
    }
    return(0);  /* reached end of string */
}
/*--------------------------------------------------------------------------*/
int ffedit_columns(
           fitsfile **fptr,  /* IO - pointer to input table; on output it  */
                             /*      points to the new selected rows table */
           char *outfile,    /* I - name for output file */
           char *expr,       /* I - column edit expression    */
           int *status)
/*
   modify columns in a table and/or header keywords in the HDU
*/
{
    fitsfile *newptr;
    int ii, hdunum, slen, colnum = -1, testnum, deletecol = 0, savecol = 0;
    int numcols = 0, *colindex = 0, tstatus = 0;
    char *cptr, *cptr2, *cptr3, *clause = NULL, keyname[FLEN_KEYWORD];
    char colname[FLEN_VALUE], oldname[FLEN_VALUE], colformat[FLEN_VALUE];
    char *file_expr = NULL, testname[FLEN_VALUE], card[FLEN_CARD];

    if (*outfile)
    {
      /* create new empty file in to hold the selected rows */
      if (ffinit(&newptr, outfile, status) > 0)
      {
        ffpmsg("failed to create file for copy (ffedit_columns)");
        return(*status);
      }

      fits_get_hdu_num(*fptr, &hdunum);  /* current HDU number in input file */

      /* copy all HDUs to the output copy, if the 'only_one' flag is not set */
      if (!((*fptr)->Fptr)->only_one) {
        for (ii = 1; 1; ii++)
        {
          if (fits_movabs_hdu(*fptr, ii, NULL, status) > 0)
            break;

          fits_copy_hdu(*fptr, newptr, 0, status);
        }

        if (*status == END_OF_FILE)
        {
          *status = 0;              /* got the expected EOF error; reset = 0  */
        }
        else if (*status > 0)
        {
          ffclos(newptr, status);
          ffpmsg("failed to copy all HDUs from input file (ffedit_columns)");
          return(*status);
        }


      } else {
        /* only copy the primary array and the designated table extension */
	fits_movabs_hdu(*fptr, 1, NULL, status);
	fits_copy_hdu(*fptr, newptr, 0, status);
	fits_movabs_hdu(*fptr, hdunum, NULL, status);
	fits_copy_hdu(*fptr, newptr, 0, status);
        if (*status > 0)
        {
          ffclos(newptr, status);
          ffpmsg("failed to copy all HDUs from input file (ffedit_columns)");
          return(*status);
        }
        hdunum = 2;
      }

      /* close the original file and return ptr to the new image */
      ffclos(*fptr, status);

      *fptr = newptr; /* reset the pointer to the new table */

      /* move back to the selected table HDU */
      if (fits_movabs_hdu(*fptr, hdunum, NULL, status) > 0)
      {
         ffpmsg("failed to copy the input file (ffedit_columns)");
         return(*status);
      }
    }

    /* remove the "col " from the beginning of the column edit expression */
    cptr = expr + 4;

    while (*cptr == ' ')
         cptr++;         /* skip leading white space */
   
    /* Check if need to import expression from a file */

    if( *cptr=='@' ) {
       if( ffimport_file( cptr+1, &file_expr, status ) ) return(*status);
       cptr = file_expr;
       while (*cptr == ' ')
          cptr++;         /* skip leading white space... again */
    }

    tstatus = 0;
    ffgncl(*fptr, &numcols, &tstatus);  /* get initial # of cols */

    /* as of July 2012, the CFITSIO column filter syntax was modified */
    /* so that commas may be used to separate clauses, as well as semi-colons. */
    /* This was done because users cannot enter the semi-colon in the HEASARC's */
    /* Hera on-line data processing system for computer security reasons.  */
    /* Therefore, we must convert those commas back to semi-colons here, but we */
    /* must not convert any columns that occur within parenthesies.  */

    if (comma2semicolon(cptr)) {
         ffpmsg("parsing error in column filter expression");
         ffpmsg(cptr);
         if( file_expr ) free( file_expr );
         *status = PARSE_SYNTAX_ERR;
         return(*status);
    }

    /* parse expression and get first clause, if more than 1 */
    while ((slen = fits_get_token2(&cptr, ";", &clause, NULL, status)) > 0 )
    {
        if( *cptr==';' ) cptr++;
        clause[slen] = '\0';

        if (clause[0] == '!' || clause[0] == '-')
        {
            /* ===================================== */
            /* Case I. delete this column or keyword */
            /* ===================================== */

            if (ffgcno(*fptr, CASEINSEN, &clause[1], &colnum, status) <= 0)
            {
                /* a column with this name exists, so try to delete it */
                if (ffdcol(*fptr, colnum, status) > 0)
                {
                    ffpmsg("failed to delete column in input file:");
                    ffpmsg(clause);
                    if( colindex ) free( colindex );
                    if( file_expr ) free( file_expr );
		    if( clause ) free(clause);
                    return(*status);
                }
                deletecol = 1; /* set flag that at least one col was deleted */
                numcols--;
                colnum = -1;
            }
            else
            {
	        ffcmsg();   /* clear previous error message from ffgcno */
                /* try deleting a keyword with this name */
                *status = 0;
                if (ffdkey(*fptr, &clause[1], status) > 0)
                {
                    ffpmsg("column or keyword to be deleted does not exist:");
                    ffpmsg(clause);
                    if( colindex ) free( colindex );
                    if( file_expr ) free( file_expr );
		    if( clause ) free(clause);
                    return(*status);
                }
            }
        }
        else
        {
            /* ===================================================== */
            /* Case II:
	       this is either a column name, (case 1) 

               or a new column name followed by double = ("==") followed
               by the old name which is to be renamed. (case 2A)

               or a column or keyword name followed by a single "=" and a
	       calculation expression (case 2B) */
            /* ===================================================== */
            cptr2 = clause;
            slen = fits_get_token(&cptr2, "( =", colname, NULL);

            if (slen == 0)
            {
                ffpmsg("error: column or keyword name is blank:");
                ffpmsg(clause);
                if( colindex ) free( colindex );
                if( file_expr ) free( file_expr );
		if (clause) free(clause);
                return(*status= URL_PARSE_ERROR);
            }

	    /* If this is a keyword of the form 
	         #KEYWORD# 
	       then transform to the form
	         #KEYWORDn
	       where n is the previously used column number 
	    */
	    if (colname[0] == '#' &&
		strstr(colname+1, "#") == (colname + strlen(colname) - 1)) 
	    {
		if (colnum <= 0) 
		  {
		    ffpmsg("The keyword name:");
		    ffpmsg(colname);
		    ffpmsg("is invalid unless a column has been previously");
		    ffpmsg("created or editted by a calculator command");
                    if( file_expr ) free( file_expr );
		    if (clause) free(clause);
		    return(*status = URL_PARSE_ERROR);
		  }
		colname[strlen(colname)-1] = '\0';
		/* Make keyword name and put it in oldname */
		ffkeyn(colname+1, colnum, oldname, status);
		if (*status) return (*status);
		/* Re-copy back into colname */
		strcpy(colname+1,oldname);
	    }
            else if  (strstr(colname, "#") == (colname + strlen(colname) - 1)) 
	    {
	        /*  colname is of the form "NAME#";  if
		      a) colnum is defined, and
		      b) a column with literal name "NAME#" does not exist, and
		      c) a keyword with name "NAMEn" (where n=colnum) exists, then
		    transfrom the colname string to "NAMEn", otherwise
		    do nothing.
		*/
		if (colnum > 0) {  /* colnum must be defined */
		  tstatus = 0;
                  ffgcno(*fptr, CASEINSEN, colname, &testnum, &tstatus);
		  if (tstatus != 0 && tstatus != COL_NOT_UNIQUE) 
		  {  
		    /* OK, column doesn't exist, now see if keyword exists */
		    ffcmsg();   /* clear previous error message from ffgcno */
		    strcpy(testname, colname);
 		    testname[strlen(testname)-1] = '\0';
		    /* Make keyword name and put it in oldname */
		    ffkeyn(testname, colnum, oldname, status);
		    if (*status) {
                      if( file_expr ) free( file_expr );
		      if (clause) free(clause);
		      return (*status);
		    }

		    tstatus = 0;
		    if (!fits_read_card(*fptr, oldname, card, &tstatus)) {
		      /* Keyword does exist; copy real name back into colname */
		      strcpy(colname,oldname);
		    }
		  }
                }
	    }

            /* if we encountered an opening parenthesis, then we need to */
            /* find the closing parenthesis, and concatinate the 2 strings */
            /* This supports expressions like:
                [col #EXTNAME(Extension name)="GTI"]
            */
            if (*cptr2  == '(')
            {
                fits_get_token(&cptr2, ")", oldname, NULL);
                strcat(colname, oldname);
                strcat(colname, ")");
                cptr2++;
            }

            while (*cptr2 == ' ')
                 cptr2++;         /* skip white space */

            if (*cptr2 != '=')
            {
              /* ------------------------------------ */
              /* case 1 - simply the name of a column */
              /* ------------------------------------ */

              /* look for matching column */
              ffgcno(*fptr, CASEINSEN, colname, &testnum, status);

              while (*status == COL_NOT_UNIQUE) 
              {
                 /* the column name contained wild cards, and it */
                 /* matches more than one column in the table. */
		 
		 colnum = testnum;

                 /* keep this column in the output file */
                 savecol = 1;

                 if (!colindex)
                    colindex = (int *) calloc(999, sizeof(int));

                 colindex[colnum - 1] = 1;  /* flag this column number */

                 /* look for other matching column names */
                 ffgcno(*fptr, CASEINSEN, colname, &testnum, status);

                 if (*status == COL_NOT_FOUND)
                    *status = 999;  /* temporary status flag value */
              }

              if (*status <= 0)
              {
	         colnum = testnum;
		 
                 /* keep this column in the output file */
                 savecol = 1;

                 if (!colindex)
                    colindex = (int *) calloc(999, sizeof(int));

                 colindex[colnum - 1] = 1;  /* flag this column number */
              }
              else if (*status == 999)
              {
                  /* this special flag value does not represent an error */
                  *status = 0;  
              }
              else
              {
               ffpmsg("Syntax error in columns specifier in input URL:");
               ffpmsg(cptr2);
               if( colindex ) free( colindex );
               if( file_expr ) free( file_expr );
	       if (clause) free(clause);
               return(*status = URL_PARSE_ERROR);
              }
            }
            else
            {
              /* ----------------------------------------------- */
              /* case 2 where the token ends with an equals sign */
              /* ----------------------------------------------- */

              cptr2++;   /* skip over the first '=' */

              if (*cptr2 == '=')
              {
                /*................................................. */
                /*  Case A:  rename a column or keyword;  syntax is
                    "new_name == old_name"  */
                /*................................................. */

                cptr2++;  /* skip the 2nd '=' */
                while (*cptr2 == ' ')
                      cptr2++;       /* skip white space */

                fits_get_token(&cptr2, " ", oldname, NULL);

                /* get column number of the existing column */
                if (ffgcno(*fptr, CASEINSEN, oldname, &colnum, status) <= 0)
                {
                    /* modify the TTYPEn keyword value with the new name */
                    ffkeyn("TTYPE", colnum, keyname, status);

                    if (ffmkys(*fptr, keyname, colname, NULL, status) > 0)
                    {
                      ffpmsg("failed to rename column in input file");
                      ffpmsg(" oldname =");
                      ffpmsg(oldname);
                      ffpmsg(" newname =");
                      ffpmsg(colname);
                      if( colindex ) free( colindex );
                      if( file_expr ) free( file_expr );
	              if (clause) free(clause);
                      return(*status);
                    }
                    /* keep this column in the output file */
                    savecol = 1;
                    if (!colindex)
                       colindex = (int *) calloc(999, sizeof(int));

                    colindex[colnum - 1] = 1;  /* flag this column number */
                }
                else
                {
                    /* try renaming a keyword */
		    ffcmsg();   /* clear error message stack */
                    *status = 0;
                    if (ffmnam(*fptr, oldname, colname, status) > 0)
                    {
                      ffpmsg("column or keyword to be renamed does not exist:");
                        ffpmsg(clause);
                        if( colindex ) free( colindex );
                        if( file_expr ) free( file_expr );
			if (clause) free(clause);
                        return(*status);
                    }
                }
              }  
              else
              {
                /*...................................................... */
                /* Case B: */
                /* this must be a general column/keyword calc expression */
                /* "name = expression" or "colname(TFORM) = expression" */
                /*...................................................... */

                /* parse the name and TFORM values, if present */
                colformat[0] = '\0';
                cptr3 = colname;

                fits_get_token(&cptr3, "(", oldname, NULL);

                if (cptr3[0] == '(' )
                {
                   cptr3++;  /* skip the '(' */
                   fits_get_token(&cptr3, ")", colformat, NULL);
                }

                /* calculate values for the column or keyword */
                /*   cptr2 = the expression to be calculated */
                /*   oldname = name of the column or keyword */
                /*   colformat = column format, or keyword comment string */
                if (fits_calculator(*fptr, cptr2, *fptr, oldname, colformat,
       	                        status) > 0) {
				
                        ffpmsg("Unable to calculate expression");
                        if( colindex ) free( colindex );
                        if( file_expr ) free( file_expr );
			if (clause) free(clause);
                         return(*status);
                }

                /* test if this is a column and not a keyword */
                tstatus = 0;
                ffgcno(*fptr, CASEINSEN, oldname, &testnum, &tstatus);
                if (tstatus == 0)
                {
                    /* keep this column in the output file */
		    colnum = testnum;
                    savecol = 1;

                    if (!colindex)
                      colindex = (int *) calloc(999, sizeof(int));

                    colindex[colnum - 1] = 1;
                    if (colnum > numcols)numcols++;
                }
		else
		{
		   ffcmsg();  /* clear the error message stack */
		}
              }
            }
        }
	if (clause) free(clause);  /* free old clause before getting new one */
        clause = NULL;
    }

    if (savecol && !deletecol)
    {
       /* need to delete all but the specified columns */
       for (ii = numcols; ii > 0; ii--)
       {
         if (!colindex[ii-1])  /* delete this column */
         {
           if (ffdcol(*fptr, ii, status) > 0)
           {
             ffpmsg("failed to delete column in input file:");
             ffpmsg(clause);
             if( colindex ) free( colindex );
             if( file_expr ) free( file_expr );
	     if (clause) free(clause);
             return(*status);
           }
         }
       }
    }

    if( colindex ) free( colindex );
    if( file_expr ) free( file_expr );
    if (clause) free(clause);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_copy_cell2image(
	   fitsfile *fptr,   /* I - point to input table */
	   fitsfile *newptr, /* O - existing output file; new image HDU
				    will be appended to it */
           char *colname,    /* I - column name / number containing the image*/
           long rownum,      /* I - number of the row containing the image */
           int *status)      /* IO - error status */

/*
  Copy a table cell of a given row and column into an image extension.
  The output file must already have been created.  A new image
  extension will be created in that file.
  
  This routine was written by Craig Markwardt, GSFC
*/

{
    unsigned char buffer[30000];
    int hdutype, colnum, typecode, bitpix, naxis, maxelem, tstatus;
    LONGLONG naxes[9], nbytes, firstbyte, ntodo;
    LONGLONG repeat, startpos, elemnum, rowlen, tnull;
    long twidth, incre;
    double scale, zero;
    char tform[20];
    char card[FLEN_CARD];
    char templt[FLEN_CARD] = "";

    /* Table-to-image keyword translation table  */
    /*                        INPUT      OUTPUT  */
    /*                       01234567   01234567 */
    char *patterns[][2] = {{"TSCALn",  "BSCALE"  },  /* Standard FITS keywords */
			   {"TZEROn",  "BZERO"   },
			   {"TUNITn",  "BUNIT"   },
			   {"TNULLn",  "BLANK"   },
			   {"TDMINn",  "DATAMIN" },
			   {"TDMAXn",  "DATAMAX" },
			   {"iCTYPn",  "CTYPEi"  },  /* Coordinate labels */
			   {"iCTYna",  "CTYPEia" },
			   {"iCUNIn",  "CUNITi"  },  /* Coordinate units */
			   {"iCUNna",  "CUNITia" },
			   {"iCRVLn",  "CRVALi"  },  /* WCS keywords */
			   {"iCRVna",  "CRVALia" },
			   {"iCDLTn",  "CDELTi"  },
			   {"iCDEna",  "CDELTia" },
			   {"iCRPXn",  "CRPIXi"  },
			   {"iCRPna",  "CRPIXia" },
			   {"ijPCna",  "PCi_ja"  },
			   {"ijCDna",  "CDi_ja"  },
			   {"iVn_ma",  "PVi_ma"  },
			   {"iSn_ma",  "PSi_ma"  },
			   {"iCRDna",  "CRDERia" },
			   {"iCSYna",  "CSYERia" },
			   {"iCROTn",  "CROTAi"  },
			   {"WCAXna",  "WCSAXESa"},
			   {"WCSNna",  "WCSNAMEa"},

			   {"LONPna",  "LONPOLEa"},
			   {"LATPna",  "LATPOLEa"},
			   {"EQUIna",  "EQUINOXa"},
			   {"MJDOBn",  "MJD-OBS" },
			   {"MJDAn",   "MJD-AVG" },
			   {"RADEna",  "RADESYSa"},
			   {"iCNAna",  "CNAMEia" },
			   {"DAVGn",   "DATE-AVG"},

                           /* Delete table keywords related to other columns */
			   {"T????#a", "-"       }, 
 			   {"TC??#a",  "-"       },
 			   {"TWCS#a",  "-"       },
			   {"TDIM#",   "-"       }, 
			   {"iCTYPm",  "-"       },
			   {"iCUNIm",  "-"       },
			   {"iCRVLm",  "-"       },
			   {"iCDLTm",  "-"       },
			   {"iCRPXm",  "-"       },
			   {"iCTYma",  "-"       },
			   {"iCUNma",  "-"       },
			   {"iCRVma",  "-"       },
			   {"iCDEma",  "-"       },
			   {"iCRPma",  "-"       },
			   {"ijPCma",  "-"       },
			   {"ijCDma",  "-"       },
			   {"iVm_ma",  "-"       },
			   {"iSm_ma",  "-"       },
			   {"iCRDma",  "-"       },
			   {"iCSYma",  "-"       },
			   {"iCROTm",  "-"       },
			   {"WCAXma",  "-"       },
			   {"WCSNma",  "-"       },

			   {"LONPma",  "-"       },
			   {"LATPma",  "-"       },
			   {"EQUIma",  "-"       },
			   {"MJDOBm",  "-"       },
			   {"MJDAm",   "-"       },
			   {"RADEma",  "-"       },
			   {"iCNAma",  "-"       },
			   {"DAVGm",   "-"       },

			   {"EXTNAME", "-"       },  /* Remove structural keywords*/
			   {"EXTVER",  "-"       },
			   {"EXTLEVEL","-"       },
			   {"CHECKSUM","-"       },
			   {"DATASUM", "-"       },
			   
			   {"*",       "+"       }}; /* copy all other keywords */
    int npat;

    if (*status > 0)
        return(*status);

    /* get column number */
    if (ffgcno(fptr, CASEINSEN, colname, &colnum, status) > 0)
    {
        ffpmsg("column containing image in table cell does not exist:");
        ffpmsg(colname);
        return(*status);
    }

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if ( ffgcprll(fptr, colnum, rownum, 1L, 1L, 0, &scale, &zero,
         tform, &twidth, &typecode, &maxelem, &startpos, &elemnum, &incre,
         &repeat, &rowlen, &hdutype, &tnull, (char *) buffer, status) > 0 )
         return(*status);

     /* get the actual column name, in case a column number was given */
    ffkeyn("", colnum, templt, &tstatus);
    ffgcnn(fptr, CASEINSEN, templt, colname, &colnum, &tstatus);

    if (hdutype != BINARY_TBL)
    {
        ffpmsg("This extension is not a binary table.");
        ffpmsg(" Cannot open the image in a binary table cell.");
        return(*status = NOT_BTABLE);
    }

    if (typecode < 0)
    {
        /* variable length array */
        typecode *= -1;  

        /* variable length arrays are 1-dimensional by default */
        naxis = 1;
        naxes[0] = repeat;
    }
    else
    {
        /* get the dimensions of the image */
        ffgtdmll(fptr, colnum, 9, &naxis, naxes, status);
    }

    if (*status > 0)
    {
        ffpmsg("Error getting the dimensions of the image");
        return(*status);
    }

    /* determine BITPIX value for the image */
    if (typecode == TBYTE)
    {
        bitpix = BYTE_IMG;
        nbytes = repeat;
    }
    else if (typecode == TSHORT)
    {
        bitpix = SHORT_IMG;
        nbytes = repeat * 2;
    }
    else if (typecode == TLONG)
    {
        bitpix = LONG_IMG;
        nbytes = repeat * 4;
    }
    else if (typecode == TFLOAT)
    {
        bitpix = FLOAT_IMG;
        nbytes = repeat * 4;
    }
    else if (typecode == TDOUBLE)
    {
        bitpix = DOUBLE_IMG;
        nbytes = repeat * 8;
    }
    else if (typecode == TLONGLONG)
    {
        bitpix = LONGLONG_IMG;
        nbytes = repeat * 8;
    }
    else if (typecode == TLOGICAL)
    {
        bitpix = BYTE_IMG;
        nbytes = repeat;
    }
    else
    {
        ffpmsg("Error: the following image column has invalid datatype:");
        ffpmsg(colname);
        ffpmsg(tform);
        ffpmsg("Cannot open an image in a single row of this column.");
        return(*status = BAD_TFORM);
    }

    /* create new image in output file */
    if (ffcrimll(newptr, bitpix, naxis, naxes, status) > 0)
    {
        ffpmsg("failed to write required primary array keywords in the output file");
        return(*status);
    }

    npat = sizeof(patterns)/sizeof(patterns[0][0])/2;
    
    /* skip over the first 8 keywords, starting just after TFIELDS */
    fits_translate_keywords(fptr, newptr, 9, patterns, npat,
			    colnum, 0, 0, status);

    /* add some HISTORY  */
    sprintf(card,"HISTORY  This image was copied from row %ld of column '%s',",
            rownum, colname);
/* disable this; leave it up to the caller to write history if needed.    
    ffprec(newptr, card, status);
*/
    /* the use of ffread routine, below, requires that any 'dirty' */
    /* buffers in memory be flushed back to the file first */
    
    ffflsh(fptr, FALSE, status);

    /* finally, copy the data, one buffer size at a time */
    ffmbyt(fptr, startpos, TRUE, status);
    firstbyte = 1; 

    /* the upper limit on the number of bytes must match the declaration */
    /* read up to the first 30000 bytes in the normal way with ffgbyt */

    ntodo = minvalue(30000, nbytes);
    ffgbyt(fptr, ntodo, buffer, status);
    ffptbb(newptr, 1, firstbyte, ntodo, buffer, status);

    nbytes    -= ntodo;
    firstbyte += ntodo;

    /* read any additional bytes with low-level ffread routine, for speed */
    while (nbytes && (*status <= 0) )
    {
        ntodo = minvalue(30000, nbytes);
        ffread((fptr)->Fptr, (long) ntodo, buffer, status);
        ffptbb(newptr, 1, firstbyte, ntodo, buffer, status);
        nbytes    -= ntodo;
        firstbyte += ntodo;
    }

    /* Re-scan the header so that CFITSIO knows about all the new keywords */
    ffrdef(newptr,status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_copy_image2cell(
	   fitsfile *fptr,   /* I - pointer to input image extension */
	   fitsfile *newptr, /* I - pointer to output table */
           char *colname,    /* I - name of column containing the image    */
           long rownum,      /* I - number of the row containing the image */
           int copykeyflag,  /* I - controls which keywords to copy */
           int *status)      /* IO - error status */

/* 
   Copy an image extension into a table cell at a given row and
   column.  The table must have already been created.  If the "colname"
   column exists, it will be used, otherwise a new column will be created
   in the table.

   The "copykeyflag" parameter controls which keywords to copy from the 
   input image to the output table header (with any appropriate translation).
 
   copykeyflag = 0  -- no keywords will be copied
   copykeyflag = 1  -- essentially all keywords will be copied
   copykeyflag = 2  -- copy only the WCS related keywords 
   
  This routine was written by Craig Markwardt, GSFC

*/
{
    tcolumn *colptr;
    unsigned char buffer[30000];
    int ii, hdutype, colnum, typecode, bitpix, naxis, ncols, hdunum;
    char tformchar, tform[20], card[FLEN_CARD];
    LONGLONG imgstart, naxes[9], nbytes, repeat, ntodo,firstbyte;
    char filename[FLEN_FILENAME+20];

    int npat;

    int naxis1;
    LONGLONG naxes1[9] = {0,0,0,0,0,0,0,0,0}, repeat1, width1;
    int typecode1;
    unsigned char dummy = 0;

    LONGLONG headstart, datastart, dataend;

    /* Image-to-table keyword translation table  */
    /*                        INPUT      OUTPUT  */
    /*                       01234567   01234567 */
    char *patterns[][2] = {{"BSCALE",  "TSCALn"  },  /* Standard FITS keywords */
			   {"BZERO",   "TZEROn"  },
			   {"BUNIT",   "TUNITn"  },
			   {"BLANK",   "TNULLn"  },
			   {"DATAMIN", "TDMINn"  },
			   {"DATAMAX", "TDMAXn"  },
			   {"CTYPEi",  "iCTYPn"  },  /* Coordinate labels */
			   {"CTYPEia", "iCTYna"  },
			   {"CUNITi",  "iCUNIn"  },  /* Coordinate units */
			   {"CUNITia", "iCUNna"  },
			   {"CRVALi",  "iCRVLn"  },  /* WCS keywords */
			   {"CRVALia", "iCRVna"  },
			   {"CDELTi",  "iCDLTn"  },
			   {"CDELTia", "iCDEna"  },
			   {"CRPIXj",  "jCRPXn"  },
			   {"CRPIXja", "jCRPna"  },
			   {"PCi_ja",  "ijPCna"  },
			   {"CDi_ja",  "ijCDna"  },
			   {"PVi_ma",  "iVn_ma"  },
			   {"PSi_ma",  "iSn_ma"  },
			   {"WCSAXESa","WCAXna"  },
			   {"WCSNAMEa","WCSNna"  },
			   {"CRDERia", "iCRDna"  },
			   {"CSYERia", "iCSYna"  },
			   {"CROTAi",  "iCROTn"  },

			   {"LONPOLEa","LONPna"},
			   {"LATPOLEa","LATPna"},
			   {"EQUINOXa","EQUIna"},
			   {"MJD-OBS", "MJDOBn" },
			   {"MJD-AVG", "MJDAn" },
			   {"RADESYSa","RADEna"},
			   {"CNAMEia", "iCNAna"  },
			   {"DATE-AVG","DAVGn"},

			   {"NAXISi",  "-"       },  /* Remove structural keywords*/
			   {"PCOUNT",  "-"       },
			   {"GCOUNT",  "-"       },
			   {"EXTEND",  "-"       },
			   {"EXTNAME", "-"       },
			   {"EXTVER",  "-"       },
			   {"EXTLEVEL","-"       },
			   {"CHECKSUM","-"       },
			   {"DATASUM", "-"       },
			   {"*",       "+"       }}; /* copy all other keywords */

    
    if (*status > 0)
        return(*status);

    if (fptr == 0 || newptr == 0) return (*status = NULL_INPUT_PTR);

    if (ffghdt(fptr, &hdutype, status) > 0) {
      ffpmsg("could not get input HDU type");
      return (*status);
    }

    if (hdutype != IMAGE_HDU) {
        ffpmsg("The input extension is not an image.");
        ffpmsg(" Cannot open the image.");
        return(*status = NOT_IMAGE);
    }

    if (ffghdt(newptr, &hdutype, status) > 0) {
      ffpmsg("could not get output HDU type");
      return (*status);
    }

    if (hdutype != BINARY_TBL) {
        ffpmsg("The output extension is not a table.");
        return(*status = NOT_BTABLE);
    }


    if (ffgiprll(fptr, 9, &bitpix, &naxis, naxes, status) > 0) {
      ffpmsg("Could not read image parameters.");
      return (*status);
    }

    /* Determine total number of pixels in the image */
    repeat = 1;
    for (ii = 0; ii < naxis; ii++) repeat *= naxes[ii];

    /* Determine the TFORM value for the table cell */
    if (bitpix == BYTE_IMG) {
      typecode = TBYTE;
      tformchar = 'B';
      nbytes = repeat;
    } else if (bitpix == SHORT_IMG) {
      typecode = TSHORT;
      tformchar = 'I';
      nbytes = repeat*2;
    } else if (bitpix == LONG_IMG) {
      typecode = TLONG;
      tformchar = 'J';
      nbytes = repeat*4;
    } else if (bitpix == FLOAT_IMG) {
      typecode = TFLOAT;
      tformchar = 'E';
      nbytes = repeat*4;
    } else if (bitpix == DOUBLE_IMG) {
      typecode = TDOUBLE;
      tformchar = 'D';
      nbytes = repeat*8;
    } else if (bitpix == LONGLONG_IMG) {
      typecode = TLONGLONG;
      tformchar = 'K';
      nbytes = repeat*8;
    } else {
      ffpmsg("Error: the image has an invalid datatype.");
      return (*status = BAD_BITPIX);
    }

    /* get column number */
    ffpmrk();
    ffgcno(newptr, CASEINSEN, colname, &colnum, status);
    ffcmrk();

    /* Column does not exist; create it */
    if (*status) {

      *status = 0;
      sprintf(tform, "%.0f%c", (double) repeat, tformchar);
      ffgncl(newptr, &ncols, status);
      colnum = ncols+1;
      fficol(newptr, colnum, colname, tform, status);
      ffptdmll(newptr, colnum, naxis, naxes, status);
      
      if (*status) {
	ffpmsg("Could not insert new column into output table.");
	return *status;
      }

    } else {

      ffgtdmll(newptr, colnum, 9, &naxis1, naxes1, status);
      if (*status > 0 || naxis != naxis1) {
	ffpmsg("Input image dimensions and output table cell dimensions do not match.");
	return (*status = BAD_DIMEN);
      }
      for (ii=0; ii<naxis; ii++) if (naxes[ii] != naxes1[ii]) {
	ffpmsg("Input image dimensions and output table cell dimensions do not match.");
	return (*status = BAD_DIMEN);
      }

      ffgtclll(newptr, colnum, &typecode1, &repeat1, &width1, status);
      if ((*status > 0) || (typecode1 != typecode) || (repeat1 != repeat)) {
	ffpmsg("Input image data type does not match output table cell type.");
	return (*status = BAD_TFORM);
      }
    }

    /* copy keywords from input image to output table, if required */
    
    if (copykeyflag) {
    
      npat = sizeof(patterns)/sizeof(patterns[0][0])/2;

      if (copykeyflag == 2) {   /* copy only the WCS-related keywords */
	patterns[npat-1][1] = "-";
      }

      /* The 3rd parameter value = 5 means skip the first 4 keywords in the image */
      fits_translate_keywords(fptr, newptr, 5, patterns, npat,
			      colnum, 0, 0, status);
    }

    /* Here is all the code to compute offsets:
     *     * byte offset from start of row to column (dest table)
     *     * byte offset from start of file to image data (source image)
     */   
 
    /* Force the writing of the row of the table by writing the last byte of
        the array, which grows the table, and/or shifts following extensions */
    ffpcl(newptr, TBYTE, colnum, rownum, repeat, 1, &dummy, status);

    /* byte offset within the row to the start of the image column */
    colptr  = (newptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */
    firstbyte = colptr->tbcol + 1; 

    /* get starting address of input image to be read */
    ffghadll(fptr, &headstart, &datastart, &dataend, status);
    imgstart = datastart;

    sprintf(card, "HISTORY  Table column '%s' row %ld copied from image",
	    colname, rownum);
/*
  Don't automatically write History keywords; leave this up to the caller. 
    ffprec(newptr, card, status);
*/

    /* write HISTORY keyword with the file name (this is now disabled)*/

    filename[0] = '\0'; hdunum = 0;
    strcpy(filename, "HISTORY   ");
    ffflnm(fptr, filename+strlen(filename), status);
    ffghdn(fptr, &hdunum);
    sprintf(filename+strlen(filename),"[%d]", hdunum-1);
/*
    ffprec(newptr, filename, status);
*/

    /* the use of ffread routine, below, requires that any 'dirty' */
    /* buffers in memory be flushed back to the file first */
    
    ffflsh(fptr, FALSE, status);

    /* move to the first byte of the input image */
    ffmbyt(fptr, imgstart, TRUE, status);

    ntodo = minvalue(30000L, nbytes);
    ffgbyt(fptr, ntodo, buffer, status);  /* read input image */
    ffptbb(newptr, rownum, firstbyte, ntodo, buffer, status); /* write to table */

    nbytes    -= ntodo;
    firstbyte += ntodo;


    /* read any additional bytes with low-level ffread routine, for speed */
    while (nbytes && (*status <= 0) )
    {
        ntodo = minvalue(30000L, nbytes);
        ffread(fptr->Fptr, (long) ntodo, buffer, status);
        ffptbb(newptr, rownum, firstbyte, ntodo, buffer, status);
        nbytes    -= ntodo;
        firstbyte += ntodo;
    }

    /* Re-scan the header so that CFITSIO knows about all the new keywords */
    ffrdef(newptr,status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_select_image_section(
           fitsfile **fptr,  /* IO - pointer to input image; on output it  */
                             /*      points to the new subimage */
           char *outfile,    /* I - name for output file        */
           char *expr,       /* I - Image section expression    */
           int *status)
{
  /*
     copies an image section from the input file to a new output file.
     Any HDUs preceding or following the image are also copied to the
     output file.
  */

    fitsfile *newptr;
    int ii, hdunum;

    /* create new empty file to hold the image section */
    if (ffinit(&newptr, outfile, status) > 0)
    {
        ffpmsg(
         "failed to create output file for image section:");
        ffpmsg(outfile);
        return(*status);
    }

    fits_get_hdu_num(*fptr, &hdunum);  /* current HDU number in input file */

    /* copy all preceding extensions to the output file, if 'only_one' flag not set */
    if (!(((*fptr)->Fptr)->only_one)) {
      for (ii = 1; ii < hdunum; ii++)
      {
        fits_movabs_hdu(*fptr, ii, NULL, status);
        if (fits_copy_hdu(*fptr, newptr, 0, status) > 0)
        {
            ffclos(newptr, status);
            return(*status);
        }
      }

      /* move back to the original HDU position */
      fits_movabs_hdu(*fptr, hdunum, NULL, status);
    }

    if (fits_copy_image_section(*fptr, newptr, expr, status) > 0)
    {
        ffclos(newptr, status);
        return(*status);
    }

    /* copy any remaining HDUs to the output file, if 'only_one' flag not set */

    if (!(((*fptr)->Fptr)->only_one)) {
      for (ii = hdunum + 1; 1; ii++)
      {
        if (fits_movabs_hdu(*fptr, ii, NULL, status) > 0)
            break;

        fits_copy_hdu(*fptr, newptr, 0, status);
      }

      if (*status == END_OF_FILE)   
        *status = 0;              /* got the expected EOF error; reset = 0  */
      else if (*status > 0)
      {
        ffclos(newptr, status);
        return(*status);
      }
    } else {
      ii = hdunum + 1;  /* this value of ii is required below */
    }

    /* close the original file and return ptr to the new image */
    ffclos(*fptr, status);

    *fptr = newptr; /* reset the pointer to the new table */

    /* move back to the image subsection */
    if (ii - 1 != hdunum)
        fits_movabs_hdu(*fptr, hdunum, NULL, status);
    else
    {
        /* may have to reset BSCALE and BZERO pixel scaling, */
        /* since the keywords were previously turned off */

        if (ffrdef(*fptr, status) > 0)  
        {
            ffclos(*fptr, status);
            return(*status);
        }

    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_copy_image_section(
           fitsfile *fptr,  /* I - pointer to input image */
           fitsfile *newptr,  /* I - pointer to output image */
           char *expr,       /* I - Image section expression    */
           int *status)
{
  /*
     copies an image section from the input file to a new output HDU
  */

    int bitpix, naxis, numkeys, nkey;
    long naxes[] = {1,1,1,1,1,1,1,1,1}, smin, smax, sinc;
    long fpixels[] = {1,1,1,1,1,1,1,1,1};
    long lpixels[] = {1,1,1,1,1,1,1,1,1};
    long incs[] = {1,1,1,1,1,1,1,1,1};
    char *cptr, keyname[FLEN_KEYWORD], card[FLEN_CARD];
    int ii, tstatus, anynull;
    long minrow, maxrow, minslice, maxslice, mincube, maxcube;
    long firstpix;
    long ncubeiter, nsliceiter, nrowiter, kiter, jiter, iiter;
    int klen, kk, jj;
    long outnaxes[9], outsize, buffsize;
    double *buffer, crpix, cdelt;

    if (*status > 0)
        return(*status);

    /* get the size of the input image */
    fits_get_img_type(fptr, &bitpix, status);
    fits_get_img_dim(fptr, &naxis, status);
    if (fits_get_img_size(fptr, naxis, naxes, status) > 0)
        return(*status);

    if (naxis < 1 || naxis > 4)
    {
        ffpmsg(
        "Input image either had NAXIS = 0 (NULL image) or has > 4 dimensions");
        return(*status = BAD_NAXIS);
    }

    /* create output image with same size and type as the input image */
    /*  Will update the size later */
    fits_create_img(newptr, bitpix, naxis, naxes, status);

    /* copy all other non-structural keywords from the input to output file */
    fits_get_hdrspace(fptr, &numkeys, NULL, status);

    for (nkey = 4; nkey <= numkeys; nkey++) /* skip the first few keywords */
    {
        fits_read_record(fptr, nkey, card, status);

        if (fits_get_keyclass(card) > TYP_CMPRS_KEY)
        {
            /* write the record to the output file */
            fits_write_record(newptr, card, status);
        }
    }

    if (*status > 0)
    {
         ffpmsg("error copying header from input image to output image");
         return(*status);
    }

    /* parse the section specifier to get min, max, and inc for each axis */
    /* and the size of each output image axis */

    cptr = expr;
    for (ii=0; ii < naxis; ii++)
    {
       if (fits_get_section_range(&cptr, &smin, &smax, &sinc, status) > 0)
       {
          ffpmsg("error parsing the following image section specifier:");
          ffpmsg(expr);
          return(*status);
       }

       if (smax == 0)
          smax = naxes[ii];   /* use whole axis  by default */
       else if (smin == 0)
          smin = naxes[ii];   /* use inverted whole axis */

       if (smin > naxes[ii] || smax > naxes[ii])
       {
          ffpmsg("image section exceeds dimensions of input image:");
          ffpmsg(expr);
          return(*status = BAD_NAXIS);
       }

       fpixels[ii] = smin;
       lpixels[ii] = smax;
       incs[ii] = sinc;

       if (smin <= smax)
           outnaxes[ii] = (smax - smin + sinc) / sinc;
       else
           outnaxes[ii] = (smin - smax + sinc) / sinc;

       /* modify the NAXISn keyword */
       fits_make_keyn("NAXIS", ii + 1, keyname, status);
       fits_modify_key_lng(newptr, keyname, outnaxes[ii], NULL, status);

       /* modify the WCS keywords if necessary */

       if (fpixels[ii] != 1 || incs[ii] != 1)
       {
            for (kk=-1;kk<26; kk++)  /* modify any alternate WCS keywords */
	{
         /* read the CRPIXn keyword if it exists in the input file */
         fits_make_keyn("CRPIX", ii + 1, keyname, status);
	 
         if (kk != -1) {
	   klen = strlen(keyname);
	   keyname[klen]='A' + kk;
	   keyname[klen + 1] = '\0';
	 }

         tstatus = 0;
         if (fits_read_key(fptr, TDOUBLE, keyname, 
             &crpix, NULL, &tstatus) == 0)
         {
           /* calculate the new CRPIXn value */
           if (fpixels[ii] <= lpixels[ii]) {
             crpix = (crpix - (fpixels[ii])) / incs[ii] + 1.0;
              /*  crpix = (crpix - (fpixels[ii] - 1.0) - .5) / incs[ii] + 0.5; */
           } else {
             crpix = (fpixels[ii] - crpix)  / incs[ii] + 1.0;
             /* crpix = (fpixels[ii] - (crpix - 1.0) - .5) / incs[ii] + 0.5; */
           }

           /* modify the value in the output file */
           fits_modify_key_dbl(newptr, keyname, crpix, 15, NULL, status);

           if (incs[ii] != 1 || fpixels[ii] > lpixels[ii])
           {
             /* read the CDELTn keyword if it exists in the input file */
             fits_make_keyn("CDELT", ii + 1, keyname, status);

             if (kk != -1) {
	       klen = strlen(keyname);
	       keyname[klen]='A' + kk;
	       keyname[klen + 1] = '\0';
	     }

             tstatus = 0;
             if (fits_read_key(fptr, TDOUBLE, keyname, 
                 &cdelt, NULL, &tstatus) == 0)
             {
               /* calculate the new CDELTn value */
               if (fpixels[ii] <= lpixels[ii])
                 cdelt = cdelt * incs[ii];
               else
                 cdelt = cdelt * (-incs[ii]);
              
               /* modify the value in the output file */
               fits_modify_key_dbl(newptr, keyname, cdelt, 15, NULL, status);
             }

             /* modify the CDi_j keywords if they exist in the input file */

             fits_make_keyn("CD1_", ii + 1, keyname, status);

             if (kk != -1) {
	       klen = strlen(keyname);
	       keyname[klen]='A' + kk;
	       keyname[klen + 1] = '\0';
	     }

             for (jj=0; jj < 9; jj++)   /* look for up to 9 dimensions */
	     {
	       keyname[2] = '1' + jj;
	       
               tstatus = 0;
               if (fits_read_key(fptr, TDOUBLE, keyname, 
                 &cdelt, NULL, &tstatus) == 0)
               {
                 /* calculate the new CDi_j value */
                 if (fpixels[ii] <= lpixels[ii])
                   cdelt = cdelt * incs[ii];
                 else
                   cdelt = cdelt * (-incs[ii]);
              
                 /* modify the value in the output file */
                 fits_modify_key_dbl(newptr, keyname, cdelt, 15, NULL, status);
               }
	     }
	     
           } /* end of if (incs[ii]... loop */
         }   /* end of fits_read_key loop */
	}    /* end of for (kk  loop */
       }
    }  /* end of main NAXIS loop */

    if (ffrdef(newptr, status) > 0)  /* force the header to be scanned */
    {
        return(*status);
    }

    /* turn off any scaling of the pixel values */
    fits_set_bscale(fptr,  1.0, 0.0, status);
    fits_set_bscale(newptr, 1.0, 0.0, status);

    /* to reduce memory foot print, just read/write image 1 row at a time */

    outsize = outnaxes[0];
    buffsize = (abs(bitpix) / 8) * outsize;

    buffer = (double *) malloc(buffsize); /* allocate memory for the image row */
    if (!buffer)
    {
        ffpmsg("fits_copy_image_section: no memory for image section");
        return(*status = MEMORY_ALLOCATION);
    }

    /* read the image section then write it to the output file */

    minrow = fpixels[1];
    maxrow = lpixels[1];
    if (minrow > maxrow) {
        nrowiter = (minrow - maxrow + incs[1]) / incs[1];
    } else {
        nrowiter = (maxrow - minrow + incs[1]) / incs[1];
    }

    minslice = fpixels[2];
    maxslice = lpixels[2];
    if (minslice > maxslice) {
        nsliceiter = (minslice - maxslice + incs[2]) / incs[2];
    } else {
        nsliceiter = (maxslice - minslice + incs[2]) / incs[2];
    }

    mincube = fpixels[3];
    maxcube = lpixels[3];
    if (mincube > maxcube) {
        ncubeiter = (mincube - maxcube + incs[3]) / incs[3];
    } else {
        ncubeiter = (maxcube - mincube + incs[3]) / incs[3];
    }

    firstpix = 1;
    for (kiter = 0; kiter < ncubeiter; kiter++)
    {
      if (mincube > maxcube) {
	 fpixels[3] = mincube - (kiter * incs[3]);
      } else {
	 fpixels[3] = mincube + (kiter * incs[3]);
      }
      
      lpixels[3] = fpixels[3];

      for (jiter = 0; jiter < nsliceiter; jiter++)
      {
        if (minslice > maxslice) {
	    fpixels[2] = minslice - (jiter * incs[2]);
        } else {
	    fpixels[2] = minslice + (jiter * incs[2]);
        }

	lpixels[2] = fpixels[2];

        for (iiter = 0; iiter < nrowiter; iiter++)
        {
            if (minrow > maxrow) {
	       fpixels[1] = minrow - (iiter * incs[1]);
	    } else {
	       fpixels[1] = minrow + (iiter * incs[1]);
            }

	    lpixels[1] = fpixels[1];

	    if (bitpix == 8)
	    {
	        ffgsvb(fptr, 1, naxis, naxes, fpixels, lpixels, incs, 0,
	            (unsigned char *) buffer, &anynull, status);

	        ffpprb(newptr, 1, firstpix, outsize, (unsigned char *) buffer, status);
	    }
	    else if (bitpix == 16)
	    {
	        ffgsvi(fptr, 1, naxis, naxes, fpixels, lpixels, incs, 0,
	            (short *) buffer, &anynull, status);

	        ffppri(newptr, 1, firstpix, outsize, (short *) buffer, status);
	    }
	    else if (bitpix == 32)
	    {
	        ffgsvk(fptr, 1, naxis, naxes, fpixels, lpixels, incs, 0,
	            (int *) buffer, &anynull, status);

	        ffpprk(newptr, 1, firstpix, outsize, (int *) buffer, status);
	    }
	    else if (bitpix == -32)
	    {
	        ffgsve(fptr, 1, naxis, naxes, fpixels, lpixels, incs, FLOATNULLVALUE,
	            (float *) buffer, &anynull, status);

	        ffppne(newptr, 1, firstpix, outsize, (float *) buffer, FLOATNULLVALUE, status);
	    }
	    else if (bitpix == -64)
	    {
	        ffgsvd(fptr, 1, naxis, naxes, fpixels, lpixels, incs, DOUBLENULLVALUE,
	             buffer, &anynull, status);

	        ffppnd(newptr, 1, firstpix, outsize, buffer, DOUBLENULLVALUE,
	               status);
	    }
	    else if (bitpix == 64)
	    {
	        ffgsvjj(fptr, 1, naxis, naxes, fpixels, lpixels, incs, 0,
	            (LONGLONG *) buffer, &anynull, status);

	        ffpprjj(newptr, 1, firstpix, outsize, (LONGLONG *) buffer, status);
	    }

            firstpix += outsize;
        }
      }
    }

    free(buffer);  /* finished with the memory */

    if (*status > 0)
    {
        ffpmsg("fits_copy_image_section: error copying image section");
        return(*status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_get_section_range(char **ptr, 
                   long *secmin,
                   long *secmax, 
                   long *incre,
                   int *status)
/*
   Parse the input image section specification string, returning 
   the  min, max and increment values.
   Typical string =   "1:512:2"  or "1:512"
*/
{
    int slen, isanumber;
    char token[FLEN_VALUE];

    if (*status > 0)
        return(*status);

    slen = fits_get_token(ptr, " ,:", token, &isanumber); /* get 1st token */

    /* support [:2,:2] type syntax, where the leading * is implied */
    if (slen==0) strcpy(token,"*");

    if (*token == '*')  /* wild card means to use the whole range */
    {
       *secmin = 1;
       *secmax = 0;
    }
    else if (*token == '-' && *(token+1) == '*' )  /* invert the whole range */
    {
       *secmin = 0;
       *secmax = 1;
    }
    else
    {
      if (slen == 0 || !isanumber || **ptr != ':')
        return(*status = URL_PARSE_ERROR);   

      /* the token contains the min value */
      *secmin = atol(token);

      (*ptr)++;  /* skip the colon between the min and max values */
      slen = fits_get_token(ptr, " ,:", token, &isanumber); /* get token */

      if (slen == 0 || !isanumber)
        return(*status = URL_PARSE_ERROR);   

      /* the token contains the max value */
      *secmax = atol(token);
    }

    if (**ptr == ':')
    {
        (*ptr)++;  /* skip the colon between the max and incre values */
        slen = fits_get_token(ptr, " ,", token, &isanumber); /* get token */

        if (slen == 0 || !isanumber)
            return(*status = URL_PARSE_ERROR);   

        *incre = atol(token);
    }
    else
        *incre = 1;  /* default increment if none is supplied */

    if (**ptr == ',')
        (*ptr)++;

    while (**ptr == ' ')   /* skip any trailing blanks */
         (*ptr)++;

    if (*secmin < 0 || *secmax < 0 || *incre < 1)
        *status = URL_PARSE_ERROR;

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffselect_table(
           fitsfile **fptr,  /* IO - pointer to input table; on output it  */
                             /*      points to the new selected rows table */
           char *outfile,    /* I - name for output file */
           char *expr,       /* I - Boolean expression    */
           int *status)
{
    fitsfile *newptr;
    int ii, hdunum;

    if (*outfile)
    {
      /* create new empty file in to hold the selected rows */
      if (ffinit(&newptr, outfile, status) > 0)
      {
        ffpmsg(
         "failed to create file for selected rows from input table");
        ffpmsg(outfile);
        return(*status);
      }

      fits_get_hdu_num(*fptr, &hdunum);  /* current HDU number in input file */

      /* copy all preceding extensions to the output file, if the 'only_one' flag is not set */
      if (!((*fptr)->Fptr)->only_one) {
        for (ii = 1; ii < hdunum; ii++)
        {
          fits_movabs_hdu(*fptr, ii, NULL, status);
          if (fits_copy_hdu(*fptr, newptr, 0, status) > 0)
          {
            ffclos(newptr, status);
            return(*status);
          }
        }
      } else {
          /* just copy the primary array */
          fits_movabs_hdu(*fptr, 1, NULL, status);
          if (fits_copy_hdu(*fptr, newptr, 0, status) > 0)
          {
            ffclos(newptr, status);
            return(*status);
          }
      }
      
      fits_movabs_hdu(*fptr, hdunum, NULL, status);

      /* copy all the header keywords from the input to output file */
      if (fits_copy_header(*fptr, newptr, status) > 0)
      {
        ffclos(newptr, status);
        return(*status);
      }

      /* set number of rows = 0 */
      fits_modify_key_lng(newptr, "NAXIS2", 0, NULL,status);
      (newptr->Fptr)->numrows = 0;
      (newptr->Fptr)->origrows = 0;

      if (ffrdef(newptr, status) > 0)  /* force the header to be scanned */
      {
        ffclos(newptr, status);
        return(*status);
      }
    }
    else
        newptr = *fptr;  /* will delete rows in place in the table */

    /* copy rows which satisfy the selection expression to the output table */
    /* or delete the nonqualifying rows if *fptr = newptr.   */
    if (fits_select_rows(*fptr, newptr, expr, status) > 0)
    {
        if (*outfile)
            ffclos(newptr, status);

        return(*status);
    }

    if (*outfile)
    {
      /* copy any remaining HDUs to the output copy */

      if (!((*fptr)->Fptr)->only_one) {
        for (ii = hdunum + 1; 1; ii++)
        {
          if (fits_movabs_hdu(*fptr, ii, NULL, status) > 0)
            break;

          fits_copy_hdu(*fptr, newptr, 0, status);
        }

        if (*status == END_OF_FILE)   
          *status = 0;              /* got the expected EOF error; reset = 0  */
        else if (*status > 0)
        {
          ffclos(newptr, status);
          return(*status);
        }
      } else {
        hdunum = 2;
      }

      /* close the original file and return ptr to the new image */
      ffclos(*fptr, status);

      *fptr = newptr; /* reset the pointer to the new table */

      /* move back to the selected table HDU */
      fits_movabs_hdu(*fptr, hdunum, NULL, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffparsecompspec(fitsfile *fptr,  /* I - FITS file pointer               */
           char *compspec,     /* I - image compression specification */
           int *status)          /* IO - error status                       */
/*
  Parse the image compression specification that was give in square brackets
  following the output FITS file name, as in these examples:

    myfile.fits[compress]  - default Rice compression, row by row 
    myfile.fits[compress TYPE] -  the first letter of TYPE defines the
                                  compression algorithm:
                                   R = Rice
                                   G = GZIP
                                   H = HCOMPRESS
                                   HS = HCOMPRESS (with smoothing)
				   B - BZIP2
                                   P = PLIO

    myfile.fits[compress TYPE 100,100] - the numbers give the dimensions
                                         of the compression tiles.  Default
                                         is NAXIS1, 1, 1, ...

       other optional parameters may be specified following a semi-colon 
       
    myfile.fits[compress; q 8.0]          q specifies the floating point 
    mufile.fits[compress TYPE; q -.0002]        quantization level;
    myfile.fits[compress TYPE 100,100; q 10, s 25]  s specifies the HCOMPRESS
                                                     integer scaling parameter

The compression parameters are saved in the fptr->Fptr structure for use
when writing FITS images.

*/
{
    char *ptr1;

    /* initialize with default values */
    int ii, compresstype = RICE_1, smooth = 0;
    int quantize_method = SUBTRACTIVE_DITHER_1;
    long tilesize[MAX_COMPRESS_DIM] = {0,0,0,0,0,0};
    float qlevel = -99., scale = 0.;
    
    ptr1 = compspec;
    while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;

    if (strncmp(ptr1, "compress", 8) && strncmp(ptr1, "COMPRESS", 8) )
    {
       /* apparently this string does not specify compression parameters */
       return(*status = URL_PARSE_ERROR);
    }

    ptr1 += 8;
    while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;

    /* ========================= */
    /* look for compression type */
    /* ========================= */

    if (*ptr1 == 'r' || *ptr1 == 'R')
    {
        compresstype = RICE_1;
        while (*ptr1 != ' ' && *ptr1 != ';' && *ptr1 != '\0') 
           ptr1++;
    }
    else if (*ptr1 == 'g' || *ptr1 == 'G')
    {
        compresstype = GZIP_1;
        while (*ptr1 != ' ' && *ptr1 != ';' && *ptr1 != '\0') 
           ptr1++;

    }
/*
    else if (*ptr1 == 'b' || *ptr1 == 'B')
    {
        compresstype = BZIP2_1;
        while (*ptr1 != ' ' && *ptr1 != ';' && *ptr1 != '\0') 
           ptr1++;

    }
*/
    else if (*ptr1 == 'p' || *ptr1 == 'P')
    {
        compresstype = PLIO_1;
        while (*ptr1 != ' ' && *ptr1 != ';' && *ptr1 != '\0') 
           ptr1++;
    }
    else if (*ptr1 == 'h' || *ptr1 == 'H')
    {
        compresstype = HCOMPRESS_1;
        ptr1++;
        if (*ptr1 == 's' || *ptr1 == 'S')
           smooth = 1;  /* apply smoothing when uncompressing HCOMPRESSed image */

        while (*ptr1 != ' ' && *ptr1 != ';' && *ptr1 != '\0') 
           ptr1++;
    }

    /* ======================== */
    /* look for tile dimensions */
    /* ======================== */

    while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;

    ii = 0;
    while (isdigit( (int) *ptr1) && ii < 9)
    {
       tilesize[ii] = atol(ptr1);  /* read the integer value */
       ii++;

       while (isdigit((int) *ptr1))    /* skip over the integer */
           ptr1++;

       if (*ptr1 == ',')
           ptr1++;   /* skip over the comma */
          
       while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;
    }

    /* ========================================================= */
    /* look for semi-colon, followed by other optional parameters */
    /* ========================================================= */

    if (*ptr1 == ';') {
        ptr1++;
        while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;

          while (*ptr1 != 0) {  /* haven't reached end of string yet */

              if (*ptr1 == 's' || *ptr1 == 'S') {
                  /* this should be the HCOMPRESS "scale" parameter; default = 1 */
	   
                  ptr1++;
                  while (*ptr1 == ' ')    /* ignore leading blanks */
                      ptr1++;

                  scale = (float) strtod(ptr1, &ptr1);

                  while (*ptr1 == ' ' || *ptr1 == ',') /* skip over blanks or comma */
                     ptr1++;

            } else if (*ptr1 == 'q' || *ptr1 == 'Q') {
                /* this should be the floating point quantization parameter */

                  ptr1++;
                  if (*ptr1 == 'z' || *ptr1 == 'Z') {
                      /* use the subtractive_dither_2 option */
                      quantize_method = SUBTRACTIVE_DITHER_2;
                      ptr1++;
		  } else if (*ptr1 == '0') {
                      /* do not dither */
                      quantize_method = NO_DITHER;
                      ptr1++;
		  }

                  while (*ptr1 == ' ')    /* ignore leading blanks */
                      ptr1++;

                  qlevel = (float) strtod(ptr1, &ptr1);

                  while (*ptr1 == ' ' || *ptr1 == ',') /* skip over blanks or comma */
                     ptr1++;

            } else {
                return(*status = URL_PARSE_ERROR);
            }
        }
    }

    /* ================================= */
    /* finished parsing; save the values */
    /* ================================= */

    fits_set_compression_type(fptr, compresstype, status);
    fits_set_tile_dim(fptr, MAX_COMPRESS_DIM, tilesize, status);
 
    if (compresstype == HCOMPRESS_1) {
        fits_set_hcomp_scale (fptr, scale,  status);
        fits_set_hcomp_smooth(fptr, smooth, status);
    }

    if (qlevel != -99.) {
        fits_set_quantize_level(fptr, qlevel, status);
        fits_set_quantize_method(fptr, quantize_method, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdkinit(fitsfile **fptr,      /* O - FITS file pointer                   */
           const char *name,     /* I - name of file to create              */
           int *status)          /* IO - error status                       */
/*
  Create and initialize a new FITS file on disk.  This routine differs
  from ffinit in that the input 'name' is literally taken as the name
  of the disk file to be created, and it does not support CFITSIO's 
  extended filename syntax.
*/
{
    *fptr = 0;              /* initialize null file pointer, */
                            /* regardless of the value of *status */
    if (*status > 0)
        return(*status);

    *status = CREATE_DISK_FILE;

    ffinit(fptr, name,status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffinit(fitsfile **fptr,      /* O - FITS file pointer                   */
           const char *name,     /* I - name of file to create              */
           int *status)          /* IO - error status                       */
/*
  Create and initialize a new FITS file.
*/
{
    int ii, driver, slen, clobber = 0;
    char *url;
    char urltype[MAX_PREFIX_LEN], outfile[FLEN_FILENAME];
    char tmplfile[FLEN_FILENAME], compspec[80];
    int handle, create_disk_file = 0;

    *fptr = 0;              /* initialize null file pointer, */
                            /* regardless of the value of *status */
    if (*status > 0)
        return(*status);

    if (*status == CREATE_DISK_FILE)
    {
       create_disk_file = 1;
       *status = 0;
    }

    if (need_to_initialize)  {          /* this is called only once */
        *status = fits_init_cfitsio();
    }

    if (*status > 0)
        return(*status);

    url = (char *) name;
    while (*url == ' ')  /* ignore leading spaces in the filename */
        url++;

    if (*url == '\0')
    {
        ffpmsg("Name of file to create is blank. (ffinit)");
        return(*status = FILE_NOT_CREATED);
    }

    if (create_disk_file)
    {
       if (strlen(url) > FLEN_FILENAME - 1)
       {
           ffpmsg("Filename is too long. (ffinit)");
           return(*status = FILE_NOT_CREATED);
       }

       strcpy(outfile, url);
       strcpy(urltype, "file://");
       tmplfile[0] = '\0';
       compspec[0] = '\0';
    }
    else
    {
       
      /* check for clobber symbol, i.e,  overwrite existing file */
      if (*url == '!')
      {
          clobber = TRUE;
          url++;
      }
      else
          clobber = FALSE;

        /* parse the output file specification */
	/* this routine checks that the strings will not overflow */
      ffourl(url, urltype, outfile, tmplfile, compspec, status);

      if (*status > 0)
      {
        ffpmsg("could not parse the output filename: (ffinit)");
        ffpmsg(url);
        return(*status);
      }
    }
    
        /* find which driver corresponds to the urltype */
    *status = urltype2driver(urltype, &driver);

    if (*status)
    {
        ffpmsg("could not find driver for this file: (ffinit)");
        ffpmsg(url);
        return(*status);
    }

        /* delete pre-existing file, if asked to do so */
    if (clobber)
    {
        if (driverTable[driver].remove)
             (*driverTable[driver].remove)(outfile);
    }

        /* call appropriate driver to create the file */
    if (driverTable[driver].create)
    {

        FFLOCK;  /* lock this while searching for vacant handle */
        *status = (*driverTable[driver].create)(outfile, &handle);
        FFUNLOCK;

        if (*status)
        {
            ffpmsg("failed to create new file (already exists?):");
            ffpmsg(url);
            return(*status);
       }
    }
    else
    {
        ffpmsg("cannot create a new file of this type: (ffinit)");
        ffpmsg(url);
        return(*status = FILE_NOT_CREATED);
    }

        /* allocate fitsfile structure and initialize = 0 */
    *fptr = (fitsfile *) calloc(1, sizeof(fitsfile));

    if (!(*fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffopen)");
        ffpmsg(url);
        return(*status = MEMORY_ALLOCATION);
    }

        /* allocate FITSfile structure and initialize = 0 */
    (*fptr)->Fptr = (FITSfile *) calloc(1, sizeof(FITSfile));

    if (!((*fptr)->Fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for following file: (ffopen)");
        ffpmsg(url);
        free(*fptr);
        *fptr = 0;       
        return(*status = MEMORY_ALLOCATION);
    }

    slen = strlen(url) + 1;
    slen = maxvalue(slen, 32); /* reserve at least 32 chars */ 
    ((*fptr)->Fptr)->filename = (char *) malloc(slen); /* mem for file name */

    if ( !(((*fptr)->Fptr)->filename) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for filename: (ffinit)");
        ffpmsg(url);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = FILE_NOT_CREATED);
    }

    /* mem for headstart array */
    ((*fptr)->Fptr)->headstart = (LONGLONG *) calloc(1001, sizeof(LONGLONG)); 

    if ( !(((*fptr)->Fptr)->headstart) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for headstart array: (ffinit)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for file I/O buffers */
    ((*fptr)->Fptr)->iobuffer = (char *) calloc(NIOBUF, IOBUFLEN);

    if ( !(((*fptr)->Fptr)->iobuffer) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for iobuffer array: (ffinit)");
        ffpmsg(url);
        free( ((*fptr)->Fptr)->headstart);    /* free memory for headstart array */
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* initialize the ageindex array (relative age of the I/O buffers) */
    /* and initialize the bufrecnum array as being empty */
    for (ii = 0; ii < NIOBUF; ii++)  {
        ((*fptr)->Fptr)->ageindex[ii] = ii;
        ((*fptr)->Fptr)->bufrecnum[ii] = -1;
    }

        /* store the parameters describing the file */
    ((*fptr)->Fptr)->MAXHDU = 1000;              /* initial size of headstart */
    ((*fptr)->Fptr)->filehandle = handle;        /* store the file pointer */
    ((*fptr)->Fptr)->driver = driver;            /*  driver number         */
    strcpy(((*fptr)->Fptr)->filename, url);      /* full input filename    */
    ((*fptr)->Fptr)->filesize = 0;               /* physical file size     */
    ((*fptr)->Fptr)->logfilesize = 0;            /* logical file size      */
    ((*fptr)->Fptr)->writemode = 1;              /* read-write mode        */
    ((*fptr)->Fptr)->datastart = DATA_UNDEFINED; /* unknown start of data  */
    ((*fptr)->Fptr)->curbuf = -1;         /* undefined current IO buffer   */
    ((*fptr)->Fptr)->open_count = 1;      /* structure is currently used once */
    ((*fptr)->Fptr)->validcode = VALIDSTRUC; /* flag denoting valid structure */

    ffldrc(*fptr, 0, IGNORE_EOF, status);     /* initialize first record */

    fits_store_Fptr( (*fptr)->Fptr, status);  /* store Fptr address */

    /* if template file was given, use it to define structure of new file */

    if (tmplfile[0])
        ffoptplt(*fptr, tmplfile, status);

    /* parse and save image compression specification, if given */
    if (compspec[0])
        ffparsecompspec(*fptr, compspec, status);

    return(*status);                       /* successful return */
}
/*--------------------------------------------------------------------------*/
/* ffimem == fits_create_memfile */

int ffimem(fitsfile **fptr,      /* O - FITS file pointer                   */ 
           void **buffptr,       /* I - address of memory pointer           */
           size_t *buffsize,     /* I - size of buffer, in bytes            */
           size_t deltasize,     /* I - increment for future realloc's      */
           void *(*mem_realloc)(void *p, size_t newsize), /* function       */
           int *status)          /* IO - error status                       */

/*
  Create and initialize a new FITS file in memory
*/
{
    int ii, driver, slen;
    char urltype[MAX_PREFIX_LEN];
    int handle;

    if (*status > 0)
        return(*status);

    *fptr = 0;              /* initialize null file pointer */

    if (need_to_initialize)    {        /* this is called only once */
       *status = fits_init_cfitsio();
    }
    
    if (*status > 0)
        return(*status);

    strcpy(urltype, "memkeep://"); /* URL type for pre-existing memory file */

    *status = urltype2driver(urltype, &driver);

    if (*status > 0)
    {
        ffpmsg("could not find driver for pre-existing memory file: (ffimem)");
        return(*status);
    }

    /* call driver routine to "open" the memory file */
    FFLOCK;  /* lock this while searching for vacant handle */
    *status =   mem_openmem( buffptr, buffsize, deltasize,
                            mem_realloc,  &handle);
    FFUNLOCK;

    if (*status > 0)
    {
         ffpmsg("failed to open pre-existing memory file: (ffimem)");
         return(*status);
    }

        /* allocate fitsfile structure and initialize = 0 */
    *fptr = (fitsfile *) calloc(1, sizeof(fitsfile));

    if (!(*fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for memory file: (ffimem)");
        return(*status = MEMORY_ALLOCATION);
    }

        /* allocate FITSfile structure and initialize = 0 */
    (*fptr)->Fptr = (FITSfile *) calloc(1, sizeof(FITSfile));

    if (!((*fptr)->Fptr))
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate structure for memory file: (ffimem)");
        free(*fptr);
        *fptr = 0;       
        return(*status = MEMORY_ALLOCATION);
    }

    slen = 32; /* reserve at least 32 chars */ 
    ((*fptr)->Fptr)->filename = (char *) malloc(slen); /* mem for file name */

    if ( !(((*fptr)->Fptr)->filename) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for filename: (ffimem)");
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for headstart array */
    ((*fptr)->Fptr)->headstart = (LONGLONG *) calloc(1001, sizeof(LONGLONG)); 

    if ( !(((*fptr)->Fptr)->headstart) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for headstart array: (ffimem)");
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* mem for file I/O buffers */
    ((*fptr)->Fptr)->iobuffer = (char *) calloc(NIOBUF, IOBUFLEN);

    if ( !(((*fptr)->Fptr)->iobuffer) )
    {
        (*driverTable[driver].close)(handle);  /* close the file */
        ffpmsg("failed to allocate memory for iobuffer array: (ffimem)");
        free( ((*fptr)->Fptr)->headstart);    /* free memory for headstart array */
        free( ((*fptr)->Fptr)->filename);
        free((*fptr)->Fptr);
        free(*fptr);
        *fptr = 0;              /* return null file pointer */
        return(*status = MEMORY_ALLOCATION);
    }

    /* initialize the ageindex array (relative age of the I/O buffers) */
    /* and initialize the bufrecnum array as being empty */
    for (ii = 0; ii < NIOBUF; ii++)  {
        ((*fptr)->Fptr)->ageindex[ii] = ii;
        ((*fptr)->Fptr)->bufrecnum[ii] = -1;
    }

        /* store the parameters describing the file */
    ((*fptr)->Fptr)->MAXHDU = 1000;              /* initial size of headstart */
    ((*fptr)->Fptr)->filehandle = handle;        /* file handle */
    ((*fptr)->Fptr)->driver = driver;            /* driver number */
    strcpy(((*fptr)->Fptr)->filename, "memfile"); /* dummy filename */
    ((*fptr)->Fptr)->filesize = *buffsize;        /* physical file size */
    ((*fptr)->Fptr)->logfilesize = *buffsize;     /* logical file size */
    ((*fptr)->Fptr)->writemode = 1;               /* read-write mode    */
    ((*fptr)->Fptr)->datastart = DATA_UNDEFINED;  /* unknown start of data */
    ((*fptr)->Fptr)->curbuf = -1;             /* undefined current IO buffer */
    ((*fptr)->Fptr)->open_count = 1;     /* structure is currently used once */
    ((*fptr)->Fptr)->validcode = VALIDSTRUC; /* flag denoting valid structure */

    ffldrc(*fptr, 0, IGNORE_EOF, status);     /* initialize first record */
    fits_store_Fptr( (*fptr)->Fptr, status);  /* store Fptr address */
    return(*status); 
}
/*--------------------------------------------------------------------------*/
int fits_init_cfitsio(void)
/*
  initialize anything that is required before using the CFITSIO routines
*/
{
    int status;

    union u_tag {
      short ival;
      char cval[2];
    } u;

    fitsio_init_lock();

    FFLOCK;   /* lockout other threads while executing this critical */
              /* section of code  */

    if (need_to_initialize == 0) { /* already initialized? */
      FFUNLOCK;
      return(0);
    }

    /*   test for correct byteswapping.   */

    u.ival = 1;
    if  ((BYTESWAPPED && u.cval[0] != 1) ||
         (BYTESWAPPED == FALSE && u.cval[1] != 1) )
    {
      printf ("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf(" Byteswapping is not being done correctly on this system.\n");
      printf(" Check the MACHINE and BYTESWAPPED definitions in fitsio2.h\n");
      printf(" Please report this problem to the CFITSIO developers.\n");
      printf(  "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      FFUNLOCK;
      return(1);
    }
    
    
    /*  test that LONGLONG is an 8 byte integer */
    
    if (sizeof(LONGLONG) != 8)
    {
      printf ("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf(" CFITSIO did not find an 8-byte long integer data type.\n");
      printf("   sizeof(LONGLONG) = %d\n",(int)sizeof(LONGLONG));
      printf(" Please report this problem to the CFITSIO developers.\n");
      printf(  "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
      FFUNLOCK;
      return(1);
    }

    /* register the standard I/O drivers that are always available */

    /* 1--------------------disk file driver-----------------------*/
    status = fits_register_driver("file://", 
            file_init,
            file_shutdown,
            file_setoptions,
            file_getoptions, 
            file_getversion,
	    file_checkfile,
            file_open,
            file_create,
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the file:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 2------------ output temporary memory file driver ----------------*/
    status = fits_register_driver("mem://", 
            mem_init,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */
            NULL,            /* open function not allowed */
            mem_create, 
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);


    if (status)
    {
        ffpmsg("failed to register the mem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 3--------------input pre-existing memory file driver----------------*/
    status = fits_register_driver("memkeep://", 
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */
            NULL,            /* file open driver function is not used */
            NULL,            /* create function not allowed */
            mem_truncate,
            mem_close_keep,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);


    if (status)
    {
        ffpmsg("failed to register the memkeep:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

   /* 4-------------------stdin stream driver----------------------*/
   /*  the stdin stream is copied to memory then opened in memory */

    status = fits_register_driver("stdin://", 
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            stdin_checkfile, 
            stdin_open,
            NULL,            /* create function not allowed */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the stdin:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

   /* 5-------------------stdin file stream driver----------------------*/
   /*  the stdin stream is copied to a disk file then the disk file is opened */

    status = fits_register_driver("stdinfile://", 
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            stdin_open,
            NULL,            /* create function not allowed */
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the stdinfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }


    /* 6-----------------------stdout stream driver------------------*/
    status = fits_register_driver("stdout://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            NULL,            /* open function not required */
            mem_create, 
            mem_truncate,
            stdout_close,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the stdout:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 7------------------iraf disk file to memory driver -----------*/
    status = fits_register_driver("irafmem://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            mem_iraf_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the irafmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 8------------------raw binary file to memory driver -----------*/
    status = fits_register_driver("rawfile://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            mem_rawfile_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the rawfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 9------------------compressed disk file to memory driver -----------*/
    status = fits_register_driver("compress://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            mem_compress_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the compress:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 10------------------compressed disk file to memory driver -----------*/
    /*  Identical to compress://, except it allows READWRITE access      */

    status = fits_register_driver("compressmem://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            mem_compress_openrw,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the compressmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 11------------------compressed disk file to disk file driver -------*/
    status = fits_register_driver("compressfile://",
            NULL,
            file_shutdown,
            file_setoptions,
            file_getoptions, 
            file_getversion,
            NULL,            /* checkfile not needed */ 
            file_compress_open,
            file_create,
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the compressfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 12---create file in memory, then compress it to disk file on close--*/
    status = fits_register_driver("compressoutfile://", 
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */
            NULL,            /* open function not allowed */
            mem_create_comp, 
            mem_truncate,
            mem_close_comp,
            file_remove,     /* delete existing compressed disk file */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);


    if (status)
    {
        ffpmsg(
        "failed to register the compressoutfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* Register Optional drivers */

#ifdef HAVE_NET_SERVICES

    /* 13--------------------root driver-----------------------*/

    status = fits_register_driver("root://",
				  root_init,
				  root_shutdown,
				  root_setoptions,
				  root_getoptions, 
				  root_getversion,
				  NULL,            /* checkfile not needed */ 
				  root_open,
				  root_create,
				  NULL,  /* No truncate possible */
				  root_close,
				  NULL,  /* No remove possible */
				  root_size,  /* no size possible */
				  root_flush,
				  root_seek, /* Though will always succeed */
				  root_read,
				  root_write);

    if (status)
    {
        ffpmsg("failed to register the root:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 14--------------------http  driver-----------------------*/
    status = fits_register_driver("http://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            http_checkfile,
            http_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the http:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 15--------------------http file driver-----------------------*/

    status = fits_register_driver("httpfile://",
            NULL,
            file_shutdown,
            file_setoptions,
            file_getoptions, 
            file_getversion,
            NULL,            /* checkfile not needed */ 
            http_file_open,
            file_create,
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the httpfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 16--------------------http memory driver-----------------------*/
    /*  same as http:// driver, except memory file can be opened READWRITE */
    status = fits_register_driver("httpmem://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            http_checkfile,
            http_file_open,  /* this will simply call http_open */
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the httpmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 17--------------------httpcompress file driver-----------------------*/

    status = fits_register_driver("httpcompress://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            http_compress_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the httpcompress:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }


    /* 18--------------------ftp driver-----------------------*/
    status = fits_register_driver("ftp://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            ftp_checkfile,
            ftp_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the ftp:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 19--------------------ftp file driver-----------------------*/
    status = fits_register_driver("ftpfile://",
            NULL,
            file_shutdown,
            file_setoptions,
            file_getoptions, 
            file_getversion,
            NULL,            /* checkfile not needed */ 
            ftp_file_open,
            file_create,
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the ftpfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 20--------------------ftp mem driver-----------------------*/
    /*  same as ftp:// driver, except memory file can be opened READWRITE */
    status = fits_register_driver("ftpmem://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            ftp_checkfile,
            ftp_file_open,   /* this will simply call ftp_open */
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the ftpmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 21--------------------ftp compressed file driver------------------*/
    status = fits_register_driver("ftpcompress://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            NULL,            /* checkfile not needed */ 
            ftp_compress_open,
            0,            /* create function not required */
            mem_truncate,
            mem_close_free,
            0,            /* remove function not required */
            mem_size,
            0,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the ftpcompress:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }
      /* === End of net drivers section === */  
#endif

/* ==================== SHARED MEMORY DRIVER SECTION ======================= */

#ifdef HAVE_SHMEM_SERVICES

    /* 22--------------------shared memory driver-----------------------*/
    status = fits_register_driver("shmem://", 
            smem_init,
            smem_shutdown,
            smem_setoptions,
            smem_getoptions, 
            smem_getversion,
            NULL,            /* checkfile not needed */ 
            smem_open,
            smem_create,
            NULL,            /* truncate file not supported yet */ 
            smem_close,
            smem_remove,
            smem_size,
            smem_flush,
            smem_seek,
            smem_read,
            smem_write );

    if (status)
    {
        ffpmsg("failed to register the shmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

#endif
/* ==================== END OF SHARED MEMORY DRIVER SECTION ================ */


#ifdef HAVE_GSIFTP
    /* 23--------------------gsiftp driver-----------------------*/
    status = fits_register_driver("gsiftp://",
            gsiftp_init,
            gsiftp_shutdown,
            gsiftp_setoptions,
            gsiftp_getoptions, 
            gsiftp_getversion,
            gsiftp_checkfile,
            gsiftp_open,
            gsiftp_create,
#ifdef HAVE_FTRUNCATE
            gsiftp_truncate,
#else
            NULL,
#endif
            gsiftp_close,
            NULL,            /* remove function not yet implemented */
            gsiftp_size,
            gsiftp_flush,
            gsiftp_seek,
            gsiftp_read,
            gsiftp_write);

    if (status)
    {
        ffpmsg("failed to register the gsiftp:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

#endif

    /* 24---------------stdin and stdout stream driver-------------------*/
    status = fits_register_driver("stream://", 
            NULL,
            NULL,
            NULL,
            NULL, 
            NULL,
	    NULL,
            stream_open,
            stream_create,
            NULL,   /* no stream truncate function */
            stream_close,
            NULL,   /* no stream remove */
            stream_size,
            stream_flush,
            stream_seek,
            stream_read,
            stream_write);

    if (status)
    {
        ffpmsg("failed to register the stream:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 25--------------------https  driver-----------------------*/
    status = fits_register_driver("https://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            https_checkfile,
            https_open,
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the https:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 26--------------------https file driver-----------------------*/

    status = fits_register_driver("httpsfile://",
            NULL,
            file_shutdown,
            file_setoptions,
            file_getoptions, 
            file_getversion,
            NULL,            /* checkfile not needed */ 
            https_file_open,
            file_create,
#ifdef HAVE_FTRUNCATE
            file_truncate,
#else
            NULL,   /* no file truncate function */
#endif
            file_close,
            file_remove,
            file_size,
            file_flush,
            file_seek,
            file_read,
            file_write);

    if (status)
    {
        ffpmsg("failed to register the httpsfile:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }

    /* 27--------------------https memory driver-----------------------*/
    /*  same as https:// driver, except memory file can be opened READWRITE */
    status = fits_register_driver("httpsmem://",
            NULL,
            mem_shutdown,
            mem_setoptions,
            mem_getoptions, 
            mem_getversion,
            https_checkfile,
            https_file_open,  /* this will simply call https_open */
            NULL,            /* create function not required */
            mem_truncate,
            mem_close_free,
            NULL,            /* remove function not required */
            mem_size,
            NULL,            /* flush function not required */
            mem_seek,
            mem_read,
            mem_write);

    if (status)
    {
        ffpmsg("failed to register the httpsmem:// driver (init_cfitsio)");
        FFUNLOCK;
        return(status);
    }


    /* reset flag.  Any other threads will now not need to call this routine */
    need_to_initialize = 0;

    FFUNLOCK;
    return(status);
}
/*--------------------------------------------------------------------------*/
int fits_register_driver(char *prefix,
	int (*init)(void),
	int (*shutdown)(void),
	int (*setoptions)(int option),
	int (*getoptions)(int *options),
	int (*getversion)(int *version),
	int (*checkfile) (char *urltype, char *infile, char *outfile),
	int (*open)(char *filename, int rwmode, int *driverhandle),
	int (*create)(char *filename, int *driverhandle),
	int (*truncate)(int driverhandle, LONGLONG filesize),
	int (*close)(int driverhandle),
	int (*fremove)(char *filename),
        int (*size)(int driverhandle, LONGLONG *sizex),
	int (*flush)(int driverhandle),
	int (*seek)(int driverhandle, LONGLONG offset),
	int (*read) (int driverhandle, void *buffer, long nbytes),
	int (*write)(int driverhandle, void *buffer, long nbytes) )
/*
  register all the functions needed to support an I/O driver
*/
{
    int status;
 
    if (no_of_drivers < 0 ) {
	  /* This is bad. looks like memory has been corrupted. */
	  ffpmsg("Vital CFITSIO parameters held in memory have been corrupted!!");
	  ffpmsg("Fatal condition detected in fits_register_driver.");
	  return(TOO_MANY_DRIVERS);
    }

    if (no_of_drivers + 1 > MAX_DRIVERS)
        return(TOO_MANY_DRIVERS);

    if (prefix  == NULL)
        return(BAD_URL_PREFIX);
   

    if (init != NULL)		
    { 
        status = (*init)();  /* initialize the driver */
        if (status)
            return(status);
    }

    	/*  fill in data in table */
    strncpy(driverTable[no_of_drivers].prefix, prefix, MAX_PREFIX_LEN);
    driverTable[no_of_drivers].prefix[MAX_PREFIX_LEN - 1] = 0;
    driverTable[no_of_drivers].init = init;
    driverTable[no_of_drivers].shutdown = shutdown;
    driverTable[no_of_drivers].setoptions = setoptions;
    driverTable[no_of_drivers].getoptions = getoptions;
    driverTable[no_of_drivers].getversion = getversion;
    driverTable[no_of_drivers].checkfile = checkfile;
    driverTable[no_of_drivers].open = open;
    driverTable[no_of_drivers].create = create;
    driverTable[no_of_drivers].truncate = truncate;
    driverTable[no_of_drivers].close = close;
    driverTable[no_of_drivers].remove = fremove;
    driverTable[no_of_drivers].size = size;
    driverTable[no_of_drivers].flush = flush;
    driverTable[no_of_drivers].seek = seek;
    driverTable[no_of_drivers].read = read;
    driverTable[no_of_drivers].write = write;

    no_of_drivers++;      /* increment the number of drivers */
    return(0);
 }
/*--------------------------------------------------------------------------*/
/* fits_parse_input_url */
int ffiurl(char *url,               /* input filename */
           char *urltype,    /* e.g., 'file://', 'http://', 'mem://' */
           char *infilex,    /* root filename (may be complete path) */
           char *outfile,    /* optional output file name            */
           char *extspec,    /* extension spec: +n or [extname, extver]  */
           char *rowfilterx, /* boolean row filter expression */
           char *binspec,    /* histogram binning specifier   */
           char *colspec,    /* column or keyword modifier expression */
           int *status)
/*
   parse the input URL into its basic components.
   This routine does not support the pixfilter or compspec components.
*/
{
	return ffifile2(url, urltype, infilex, outfile,
               extspec, rowfilterx, binspec, colspec, 0, 0, status);
}
/*--------------------------------------------------------------------------*/
/* fits_parse_input_file */
int ffifile(char *url,       /* input filename */
           char *urltype,    /* e.g., 'file://', 'http://', 'mem://' */
           char *infilex,    /* root filename (may be complete path) */
           char *outfile,    /* optional output file name            */
           char *extspec,    /* extension spec: +n or [extname, extver]  */
           char *rowfilterx, /* boolean row filter expression */
           char *binspec,    /* histogram binning specifier   */
           char *colspec,    /* column or keyword modifier expression */
           char *pixfilter,  /* pixel filter expression */
           int *status)
/*
   fits_parse_input_filename
   parse the input URL into its basic components.
   This routine does not support the compspec component.
*/
{
	return ffifile2(url, urltype, infilex, outfile,
               extspec, rowfilterx, binspec, colspec, pixfilter, 0, status);

} 
/*--------------------------------------------------------------------------*/
int ffifile2(char *url,       /* input filename */
           char *urltype,    /* e.g., 'file://', 'http://', 'mem://' */
           char *infilex,    /* root filename (may be complete path) */
           char *outfile,    /* optional output file name            */
           char *extspec,    /* extension spec: +n or [extname, extver]  */
           char *rowfilterx, /* boolean row filter expression */
           char *binspec,    /* histogram binning specifier   */
           char *colspec,    /* column or keyword modifier expression */
           char *pixfilter,  /* pixel filter expression */
           char *compspec,   /* image compression specification */
           int *status)
/*
   fits_parse_input_filename
   parse the input URL into its basic components.
   This routine is big and ugly and should be redesigned someday!
*/
{ 
    int ii, jj, slen, infilelen, plus_ext = 0, collen;
    char *ptr1, *ptr2, *ptr3, *ptr4, *tmptr;
    int hasAt, hasDot, hasOper, followingOper, spaceTerm, rowFilter;
    int colStart, binStart, pixStart, compStart;


    /* must have temporary variable for these, in case inputs are NULL */
    char *infile;
    char *rowfilter;
    char *tmpstr;

    if (*status > 0)
        return(*status);

    /* Initialize null strings */
    if (infilex) *infilex  = '\0';
    if (urltype) *urltype = '\0';
    if (outfile) *outfile = '\0';
    if (extspec) *extspec = '\0';
    if (binspec) *binspec = '\0';
    if (colspec) *colspec = '\0';
    if (rowfilterx) *rowfilterx = '\0';
    if (pixfilter) *pixfilter = '\0';
    if (compspec) *compspec = '\0';
    slen = strlen(url);

    if (slen == 0)       /* blank filename ?? */
        return(*status);

    /* allocate memory for 3 strings, each as long as the input url */
    infile = (char *) calloc(3,  slen + 1);
    if (!infile)
       return(*status = MEMORY_ALLOCATION);

    rowfilter = &infile[slen + 1];
    tmpstr = &rowfilter[slen + 1];

    ptr1 = url;

    /* -------------------------------------------------------- */
    /*  get urltype (e.g., file://, ftp://, http://, etc.)  */
    /* --------------------------------------------------------- */

    if (*ptr1 == '-' && ( *(ptr1 +1) ==  0   || *(ptr1 +1) == ' '  || 
                          *(ptr1 +1) == '['  || *(ptr1 +1) == '(' ) )
    {
        /* "-" means read file from stdin. Also support "- ",        */
        /* "-[extname]" and '-(outfile.fits)" but exclude disk file  */
        /* names that begin with a minus sign, e.g., "-55d33m.fits"  */

        if (urltype)
            strcat(urltype, "stdin://");
        ptr1++;
    }
    else if (!fits_strncasecmp(ptr1, "stdin", 5))
    {
        if (urltype)
            strcat(urltype, "stdin://");
        ptr1 = ptr1 + 5;
    }
    else
    {
        ptr2 = strstr(ptr1, "://");
        ptr3 = strstr(ptr1, "(" );

        if (ptr3 && (ptr3 < ptr2) )
        {
           /* the urltype follows a '(' character, so it must apply */
           /* to the output file, and is not the urltype of the input file */
           ptr2 = 0;   /* so reset pointer to zero */
        }

        if (ptr2)            /* copy the explicit urltype string */ 
        {
            if (urltype)
                 strncat(urltype, ptr1, ptr2 - ptr1 + 3);
            ptr1 = ptr2 + 3;
        }
        else if (!strncmp(ptr1, "ftp:", 4) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "ftp://");
            ptr1 += 4;
        }
        else if (!strncmp(ptr1, "gsiftp:", 7) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "gsiftp://");
            ptr1 += 7;
        }
        else if (!strncmp(ptr1, "http:", 5) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "http://");
            ptr1 += 5;
        }
        else if (!strncmp(ptr1, "mem:", 4) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "mem://");
            ptr1 += 4;
        }
        else if (!strncmp(ptr1, "shmem:", 6) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "shmem://");
            ptr1 += 6;
        }
        else if (!strncmp(ptr1, "file:", 5) )
        {                              /* the 2 //'s are optional */
            if (urltype)
                strcat(urltype, "file://");
            ptr1 += 5;
        }
        else                       /* assume file driver    */
        {
            if (urltype)
                strcat(urltype, "file://");
        }
    }

    /* ----------------------------------------------------------    
       If this is a http:// type file, then the cgi file name could
       include the '[' character, which should not be interpreted
       as part of CFITSIO's Extended File Name Syntax.  Test for this
       case by seeing if the last character is a ']' or ')'.  If it 
       is not, then just treat the whole input string as the file name
       and do not attempt to interprete the name using the extended
       filename syntax.
     ----------------------------------------------------------- */

    if (urltype && !strncmp(urltype, "http://", 7) )
    {
        /* test for opening parenthesis or bracket in the file name */
        if( strchr(ptr1, '(' ) || strchr(ptr1, '[' ) )
        {
            slen = strlen(ptr1);
            ptr3 = ptr1 + slen - 1;
            while (*ptr3 == ' ')    /* ignore trailing blanks */
                ptr3--;

            if (*ptr3 != ']' && *ptr3 != ')' )
            {
                /* name doesn't end with a ']' or ')' so don't try */
                /* to parse this unusual string (may be cgi string)  */
                if (infilex) {

                    if (strlen(ptr1) > FLEN_FILENAME - 1) {
                        ffpmsg("Name of file is too long.");
                        return(*status = URL_PARSE_ERROR);
                    }
		    
                    strcpy(infilex, ptr1);
                }

                free(infile);
                return(*status);
            }
        }
    }

    /* ----------------------------------------------------------    
       Look for VMS style filenames like: 
            disk:[directory.subdirectory]filename.ext, or
                 [directory.subdirectory]filename.ext

       Check if the first character is a '[' and urltype != stdin
       or if there is a ':[' string in the remaining url string. If
       so, then need to move past this bracket character before
       search for the opening bracket of a filter specification.
     ----------------------------------------------------------- */

    tmptr = ptr1;
    if (*ptr1 == '[')
    {
      if (*url != '-') 
        tmptr = ptr1 + 1; /* this bracket encloses a VMS directory name */
    }
    else
    {
       tmptr = strstr(ptr1, ":[");
       if (tmptr) /* these 2 chars are part of the VMS disk and directory */
          tmptr += 2; 
       else
          tmptr = ptr1;
    }

    /* ------------------------ */
    /*  get the input file name */
    /* ------------------------ */

    ptr2 = strchr(tmptr, '(');   /* search for opening parenthesis ( */
    ptr3 = strchr(tmptr, '[');   /* search for opening bracket [ */

    if (ptr2 == ptr3)  /* simple case: no [ or ( in the file name */
    {
        strcat(infile, ptr1);
    }
    else if (!ptr3 ||         /* no bracket, so () enclose output file name */
         (ptr2 && (ptr2 < ptr3)) ) /* () enclose output name before bracket */
    {
        strncat(infile, ptr1, ptr2 - ptr1);
        ptr2++;

        ptr1 = strchr(ptr2, ')' );   /* search for closing ) */
        if (!ptr1)
        {
            free(infile);
            return(*status = URL_PARSE_ERROR);  /* error, no closing ) */
        }

        if (outfile) {
	
	    if (ptr1 - ptr2 > FLEN_FILENAME - 1)
	    {
                 free(infile);
                 return(*status = URL_PARSE_ERROR);
            }

            strncat(outfile, ptr2, ptr1 - ptr2);
        }
	
        /* the opening [ could have been part of output name,    */
        /*      e.g., file(out[compress])[3][#row > 5]           */
        /* so search again for opening bracket following the closing ) */
        ptr3 = strchr(ptr1, '[');

    }
    else    /*   bracket comes first, so there is no output name */
    {
        strncat(infile, ptr1, ptr3 - ptr1);
    }

   /* strip off any trailing blanks in the names */

    slen = strlen(infile);
    while ( (--slen) > 0  && infile[slen] == ' ') 
         infile[slen] = '\0';

    if (outfile)
    {
        slen = strlen(outfile);
        while ( (--slen) > 0  && outfile[slen] == ' ') 
            outfile[slen] = '\0';
    }

    /* --------------------------------------------- */
    /* check if this is an IRAF file (.imh extension */
    /* --------------------------------------------- */

    ptr4 = strstr(infile, ".imh");

    /* did the infile name end with ".imh" ? */
    if (ptr4 && (*(ptr4 + 4) == '\0'))
    {
        if (urltype)
            strcpy(urltype, "irafmem://");
    }

    /* --------------------------------------------- */
    /* check if the 'filename+n' convention has been */
    /* used to specifiy which HDU number to open     */ 
    /* --------------------------------------------- */

    jj = strlen(infile);

    for (ii = jj - 1; ii >= 0; ii--)
    {
        if (infile[ii] == '+')    /* search backwards for '+' sign */
            break;
    }

    if (ii > 0 && (jj - ii) < 7)  /* limit extension numbers to 5 digits */
    {
        infilelen = ii;
        ii++;
        ptr1 = infile+ii;   /* pointer to start of sequence */

        for (; ii < jj; ii++)
        {
            if (!isdigit((int) infile[ii] ) ) /* are all the chars digits? */
                break;
        }

        if (ii == jj)      
        {
             /* yes, the '+n' convention was used.  Copy */
             /* the digits to the output extspec string. */
             plus_ext = 1;

             if (extspec) {
	         if (jj - infilelen > FLEN_FILENAME - 1)
	         {
                     free(infile);
                     return(*status = URL_PARSE_ERROR);
                 }

                 strncpy(extspec, ptr1, jj - infilelen);
             }
	     
             infile[infilelen] = '\0'; /* delete the extension number */
        }
    }

    /* -------------------------------------------------------------------- */
    /* if '*' was given for the output name expand it to the root file name */
    /* -------------------------------------------------------------------- */

    if (outfile && outfile[0] == '*')
    {
        /* scan input name backwards to the first '/' character */
        for (ii = jj - 1; ii >= 0; ii--)
        {
            if (infile[ii] == '/' || ii == 0)
            {
	      if (strlen(&infile[ii + 1]) > FLEN_FILENAME - 1)
	      {
                 free(infile);
                 return(*status = URL_PARSE_ERROR);
              }

                strcpy(outfile, &infile[ii + 1]);
                break;
            }
        }
    }

    /* ------------------------------------------ */
    /* copy strings from local copy to the output */
    /* ------------------------------------------ */
    if (infilex) {
	if (strlen(infile) > FLEN_FILENAME - 1)
	{
                 free(infile);
                 return(*status = URL_PARSE_ERROR);
        }

        strcpy(infilex, infile);
    }
    /* ---------------------------------------------------------- */
    /* if no '[' character in the input string, then we are done. */
    /* ---------------------------------------------------------- */
    if (!ptr3) 
    {
        free(infile);
        return(*status);
    }

    /* ------------------------------------------- */
    /* see if [ extension specification ] is given */
    /* ------------------------------------------- */

    if (!plus_ext) /* extension no. not already specified?  Then      */
                   /* first brackets must enclose extension name or # */
                   /* or it encloses a image subsection specification */
                   /* or a raw binary image specifier */
                   /* or a image compression specifier */

                   /* Or, the extension specification may have been */
                   /* omitted and we have to guess what the user intended */
    {
       ptr1 = ptr3 + 1;    /* pointer to first char after the [ */

       ptr2 = strchr(ptr1, ']' );   /* search for closing ] */
       if (!ptr2)
       {
            ffpmsg("input file URL is missing closing bracket ']'");
            free(infile);
            return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
       }

       /* ---------------------------------------------- */
       /* First, test if this is a rawfile specifier     */
       /* which looks something like: '[ib512,512:2880]' */
       /* Test if first character is b,i,j,d,r,f, or u,  */
       /* and optional second character is b or l,       */
       /* followed by one or more digits,                */
       /* finally followed by a ',', ':', or ']'         */
       /* ---------------------------------------------- */

       if (*ptr1 == 'b' || *ptr1 == 'B' || *ptr1 == 'i' || *ptr1 == 'I' ||
           *ptr1 == 'j' || *ptr1 == 'J' || *ptr1 == 'd' || *ptr1 == 'D' ||
           *ptr1 == 'r' || *ptr1 == 'R' || *ptr1 == 'f' || *ptr1 == 'F' ||
           *ptr1 == 'u' || *ptr1 == 'U')
       {
           /* next optional character may be a b or l (for Big or Little) */
           ptr1++;
           if (*ptr1 == 'b' || *ptr1 == 'B' || *ptr1 == 'l' || *ptr1 == 'L')
              ptr1++;

           if (isdigit((int) *ptr1))  /* must have at least 1 digit */
           {
             while (isdigit((int) *ptr1))
              ptr1++;             /* skip over digits */

             if (*ptr1 == ',' || *ptr1 == ':' || *ptr1 == ']' )
             {
               /* OK, this looks like a rawfile specifier */

               if (urltype)
               {
                 if (strstr(urltype, "stdin") )
                   strcpy(urltype, "rawstdin://");
                 else
                   strcpy(urltype, "rawfile://");
               }

               /* append the raw array specifier to infilex */
               if (infilex)
               {

	         if (strlen(infilex) + strlen(ptr3) > FLEN_FILENAME - 1)
	         {
                    free(infile);
                    return(*status = URL_PARSE_ERROR);
                 }

                 strcat(infilex, ptr3);
                 ptr1 = strchr(infilex, ']'); /* find the closing ] char */
                 if (ptr1)
                   *(ptr1 + 1) = '\0';  /* terminate string after the ] */
               }

               if (extspec)
                  strcpy(extspec, "0"); /* the 0 ext number is implicit */

               tmptr = strchr(ptr2 + 1, '[' ); /* search for another [ char */ 

               /* copy any remaining characters into rowfilterx  */
               if (tmptr && rowfilterx)
               {


	         if (strlen(rowfilterx) + strlen(tmptr + 1) > FLEN_FILENAME -1)
	         {
                    free(infile);
                    return(*status = URL_PARSE_ERROR);
                 }

                 strcat(rowfilterx, tmptr + 1);

                 tmptr = strchr(rowfilterx, ']' );   /* search for closing ] */
                 if (tmptr)
                   *tmptr = '\0'; /* overwrite the ] with null terminator */
               }

               free(infile);        /* finished parsing, so return */
               return(*status);
             }
           }   
       }        /* end of rawfile specifier test */

       /* -------------------------------------------------------- */
       /* Not a rawfile, so next, test if this is an image section */
       /* i.e., an integer followed by a ':' or a '*' or '-*'      */
       /* -------------------------------------------------------- */
 
       ptr1 = ptr3 + 1;    /* reset pointer to first char after the [ */
       tmptr = ptr1;

       while (*tmptr == ' ')
          tmptr++;   /* skip leading blanks */

       while (isdigit((int) *tmptr))
          tmptr++;             /* skip over leading digits */

       if (*tmptr == ':' || *tmptr == '*' || *tmptr == '-')
       {
           /* this is an image section specifier */
           strcat(rowfilter, ptr3);
/*
  don't want to assume 0 extension any more; may imply an image extension.
           if (extspec)
              strcpy(extspec, "0");
*/
       }
       else
       {
       /* ----------------------------------------------------------------- 
         Not an image section or rawfile spec so may be an extension spec. 

         Examples of valid extension specifiers:
            [3]                - 3rd extension; 0 = primary array
            [events]           - events extension
            [events, 2]        - events extension, with EXTVER = 2
            [events,2]         - spaces are optional
            [events, 3, b]     - same as above, plus XTENSION = 'BINTABLE'
            [PICS; colName(12)] - an image in row 12 of the colName column
                                      in the PICS table extension             
            [PICS; colName(exposure > 1000)] - as above, but find image in
                          first row with with exposure column value > 1000.
            [Rate Table] - extension name can contain spaces!
            [Rate Table;colName(exposure>1000)]

         Examples of other types of specifiers (Not extension specifiers)

            [bin]  !!! this is ambiguous, and can't be distinguished from
                       a valid extension specifier
            [bini X=1:512:16]  (also binb, binj, binr, and bind are allowed)
            [binr (X,Y) = 5]
            [bin @binfilter.txt]

            [col Time;rate]
            [col PI=PHA * 1.1]
            [col -Time; status]

            [X > 5]
            [X>5]
            [@filter.txt]
            [StatusCol]  !!! this is ambiguous, and can't be distinguished
                       from a valid extension specifier
            [StatusCol==0]
            [StatusCol || x>6]
            [gtifilter()]
            [regfilter("region.reg")]

            [compress Rice]

         There will always be some ambiguity between an extension name and 
         a boolean row filtering expression, (as in a couple of the above
         examples).  If there is any doubt, the expression should be treated
         as an extension specification;  The user can always add an explicit
         expression specifier to override this interpretation.

         The following decision logic will be used:

         1) locate the first token, terminated with a space, comma, 
            semi-colon, or closing bracket.

         2) the token is not part of an extension specifier if any of
            the following is true:

            - if the token begins with '@' and contains a '.'
            - if the token contains an operator: = > < || && 
            - if the token begins with "gtifilter(" or "regfilter(" 
            - if the token is terminated by a space and is followed by
               additional characters (not a ']')  AND any of the following:
                 - the token is 'col'
                 - the token is 3 or 4 chars long and begins with 'bin'
                 - the second token begins with an operator:
                     ! = < > | & + - * / %
                 

         3) otherwise, the string is assumed to be an extension specifier

         ----------------------------------------------------------------- */

           tmptr = ptr1;
           while(*tmptr == ' ')
               tmptr++;

           hasAt = 0;
           hasDot = 0;
           hasOper = 0;
           followingOper = 0;
           spaceTerm = 0;
           rowFilter = 0;
           colStart = 0;
           binStart = 0;
           pixStart = 0;
           compStart = 0;

           if (*tmptr == '@')  /* test for leading @ symbol */
               hasAt = 1;

           if ( !fits_strncasecmp(tmptr, "col ", 4) )
              colStart = 1;

           if ( !fits_strncasecmp(tmptr, "bin", 3) )
              binStart = 1;

           if ( !fits_strncasecmp(tmptr, "pix", 3) )
              pixStart = 1;

           if ( !fits_strncasecmp(tmptr, "compress ", 9) ||
                !fits_strncasecmp(tmptr, "compress]", 9) )
              compStart = 1;

           if ( !fits_strncasecmp(tmptr, "gtifilter(", 10) ||
                !fits_strncasecmp(tmptr, "regfilter(", 10) )
           {
               rowFilter = 1;
           }
           else
           {
             /* parse the first token of the expression */
             for (ii = 0; ii < ptr2 - ptr1 + 1; ii++, tmptr++)
             {
               if (*tmptr == '.')
                   hasDot = 1;
               else if (*tmptr == '=' || *tmptr == '>' || *tmptr == '<' ||
                   (*tmptr == '|' && *(tmptr+1) == '|') ||
                   (*tmptr == '&' && *(tmptr+1) == '&') )
                   hasOper = 1;

               else if (*tmptr == ',' || *tmptr == ';' || *tmptr == ']')
               {
                  break;
               }
               else if (*tmptr == ' ')   /* a space char? */
               {
                  while(*tmptr == ' ')  /* skip spaces */
                    tmptr++;

                  if (*tmptr == ']') /* is this the end? */
                     break;  

                  spaceTerm = 1; /* 1st token is terminated by space */

                  /* test if this is a column or binning specifier */
                  if (colStart || (ii <= 4 && (binStart || pixStart)) )
                     rowFilter = 1;
                  else
                  {
  
                    /* check if next character is an operator */
                    if (*tmptr == '=' || *tmptr == '>' || *tmptr == '<' ||
                      *tmptr == '|' || *tmptr == '&' || *tmptr == '!' ||
                      *tmptr == '+' || *tmptr == '-' || *tmptr == '*' ||
                      *tmptr == '/' || *tmptr == '%')
                       followingOper = 1;
                  }
                  break;
               }
             }
           }

           /* test if this is NOT an extension specifier */
           if ( rowFilter || (pixStart && spaceTerm) ||
                (hasAt && hasDot) ||
                hasOper ||
                compStart ||
                (spaceTerm && followingOper) )
           {
               /* this is (probably) not an extension specifier */
               /* so copy all chars to filter spec string */
               strcat(rowfilter, ptr3);
           }
           else
           {
               /* this appears to be a legit extension specifier */
               /* copy the extension specification */
               if (extspec) {
                   if (ptr2 - ptr1 > FLEN_FILENAME - 1) {
                       free(infile);
                       return(*status = URL_PARSE_ERROR);
		   }
                   strncat(extspec, ptr1, ptr2 - ptr1);
               }

               /* copy any remaining chars to filter spec string */
               strcat(rowfilter, ptr2 + 1);
           }
       }
    }      /* end of  if (!plus_ext)     */
    else   
    {
      /* ------------------------------------------------------------------ */
      /* already have extension, so this must be a filter spec of some sort */
      /* ------------------------------------------------------------------ */

        strcat(rowfilter, ptr3);
    }

    /* strip off any trailing blanks from filter */
    slen = strlen(rowfilter);
    while ( (--slen) >= 0  && rowfilter[slen] == ' ') 
         rowfilter[slen] = '\0';

    if (!rowfilter[0])
    {
        free(infile);
        return(*status);      /* nothing left to parse */
    }

    /* ------------------------------------------------ */
    /* does the filter contain a binning specification? */
    /* ------------------------------------------------ */

    ptr1 = strstr(rowfilter, "[bin");      /* search for "[bin" */
    if (!ptr1)
        ptr1 = strstr(rowfilter, "[BIN");      /* search for "[BIN" */
    if (!ptr1)
        ptr1 = strstr(rowfilter, "[Bin");      /* search for "[Bin" */

    if (ptr1)
    {
      ptr2 = ptr1 + 4;     /* end of the '[bin' string */
      if (*ptr2 == 'b' || *ptr2 == 'i' || *ptr2 == 'j' ||
          *ptr2 == 'r' || *ptr2 == 'd')
         ptr2++;  /* skip the datatype code letter */


      if ( *ptr2 != ' ' && *ptr2 != ']')
        ptr1 = NULL;   /* bin string must be followed by space or ] */
    }

    if (ptr1)
    {
        /* found the binning string */
        if (binspec)
        {
	    if (strlen(ptr1 +1) > FLEN_FILENAME - 1)
	    {
                    free(infile);
                    return(*status = URL_PARSE_ERROR);
            }

            strcpy(binspec, ptr1 + 1);       
            ptr2 = strchr(binspec, ']');

            if (ptr2)      /* terminate the binning filter */
            {
                *ptr2 = '\0';

                if ( *(--ptr2) == ' ')  /* delete trailing spaces */
                    *ptr2 = '\0';
            }
            else
            {
                ffpmsg("input file URL is missing closing bracket ']'");
                ffpmsg(rowfilter);
                free(infile);
                return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
            }
        }

        /* delete the binning spec from the row filter string */
        ptr2 = strchr(ptr1, ']');
        strcpy(tmpstr, ptr2+1);  /* copy any chars after the binspec */
        strcpy(ptr1, tmpstr);    /* overwrite binspec */
    }

    /* --------------------------------------------------------- */
    /* does the filter contain a column selection specification? */
    /* --------------------------------------------------------- */

    ptr1 = strstr(rowfilter, "[col ");
    if (!ptr1)
    {
        ptr1 = strstr(rowfilter, "[COL ");

        if (!ptr1)
            ptr1 = strstr(rowfilter, "[Col ");
    }

    if (ptr1)
    {           /* find the end of the column specifier */
        ptr2 = ptr1 + 5;
        while (*ptr2 != ']')
        {
            if (*ptr2 == '\0')
            {
                ffpmsg("input file URL is missing closing bracket ']'");
                free(infile);
                return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
            }

            if (*ptr2 == '\'')  /* start of a literal string */
            {
                ptr2 = strchr(ptr2 + 1, '\'');  /* find closing quote */
                if (!ptr2)
                {
                  ffpmsg
          ("literal string in input file URL is missing closing single quote");
                  free(infile);
                  return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
                }
            }

            if (*ptr2 == '[')  /* set of nested square brackets */
            {
                ptr2 = strchr(ptr2 + 1, ']');  /* find closing bracket */
                if (!ptr2)
                {
                  ffpmsg
          ("nested brackets in input file URL is missing closing bracket");
                  free(infile);
                  return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
                }
            }

            ptr2++;  /* continue search for the closing bracket character */
        } 

        collen = ptr2 - ptr1 - 1;

        if (colspec)    /* copy the column specifier to output string */
        {
            if (collen > FLEN_FILENAME - 1) {
                       free(infile);
                       return(*status = URL_PARSE_ERROR);
            }

            strncpy(colspec, ptr1 + 1, collen);       
            colspec[collen] = '\0';
 
            while (colspec[--collen] == ' ')
                colspec[collen] = '\0';  /* strip trailing blanks */
        }

        /* delete the column selection spec from the row filter string */
        strcpy(tmpstr, ptr2 + 1);  /* copy any chars after the colspec */
        strcpy(ptr1, tmpstr);      /* overwrite binspec */
    }

    /* --------------------------------------------------------- */
    /* does the filter contain a pixel filter specification?     */
    /* --------------------------------------------------------- */

    ptr1 = strstr(rowfilter, "[pix");
    if (!ptr1)
    {
        ptr1 = strstr(rowfilter, "[PIX");

        if (!ptr1)
            ptr1 = strstr(rowfilter, "[Pix");
    }

    if (ptr1)
    {
      ptr2 = ptr1 + 4;     /* end of the '[pix' string */
      if (*ptr2 == 'b' || *ptr2 == 'i' || *ptr2 == 'j' || *ptr2 == 'B' ||
          *ptr2 == 'I' || *ptr2 == 'J' || *ptr2 == 'r' || *ptr2 == 'd' ||
           *ptr2 == 'R' || *ptr2 == 'D')
         ptr2++;  /* skip the datatype code letter */

      if (*ptr2 == '1')
         ptr2++;   /* skip the single HDU indicator */

      if ( *ptr2 != ' ')
        ptr1 = NULL;   /* pix string must be followed by space */
    }

    if (ptr1)
    {           /* find the end of the pixel filter */
        while (*ptr2 != ']')
        {
            if (*ptr2 == '\0')
            {
                ffpmsg("input file URL is missing closing bracket ']'");
                free(infile);
                return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
            }

            if (*ptr2 == '\'')  /* start of a literal string */
            {
                ptr2 = strchr(ptr2 + 1, '\'');  /* find closing quote */
                if (!ptr2)
                {
                  ffpmsg
          ("literal string in input file URL is missing closing single quote");
                  free(infile);
                  return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
                }
            }

            if (*ptr2 == '[')  /* set of nested square brackets */
            {
                ptr2 = strchr(ptr2 + 1, ']');  /* find closing bracket */
                if (!ptr2)
                {
                  ffpmsg
          ("nested brackets in input file URL is missing closing bracket");
                  free(infile);
                  return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
                }
            }

            ptr2++;  /* continue search for the closing bracket character */
        } 

        collen = ptr2 - ptr1 - 1;

        if (pixfilter)    /* copy the column specifier to output string */
        {
            if (collen > FLEN_FILENAME - 1) {
                       free(infile);
                       return(*status = URL_PARSE_ERROR);
            }

            strncpy(pixfilter, ptr1 + 1, collen);       
            pixfilter[collen] = '\0';
 
            while (pixfilter[--collen] == ' ')
                pixfilter[collen] = '\0';  /* strip trailing blanks */
        }

        /* delete the pixel filter from the row filter string */
        strcpy(tmpstr, ptr2 + 1);  /* copy any chars after the pixel filter */
        strcpy(ptr1, tmpstr);      /* overwrite binspec */
    }

    /* ------------------------------------------------------------ */
    /* does the filter contain an image compression specification?  */
    /* ------------------------------------------------------------ */

    ptr1 = strstr(rowfilter, "[compress");

    if (ptr1)
    {
      ptr2 = ptr1 + 9;     /* end of the '[compress' string */

      if ( *ptr2 != ' ' && *ptr2 != ']')
        ptr1 = NULL;   /* compress string must be followed by space or ] */
    }

    if (ptr1)
    {
        /* found the compress string */
        if (compspec)
        {
	    if (strlen(ptr1 +1) > FLEN_FILENAME - 1)
	    {
                    free(infile);
                    return(*status = URL_PARSE_ERROR);
            }

            strcpy(compspec, ptr1 + 1);       
            ptr2 = strchr(compspec, ']');

            if (ptr2)      /* terminate the binning filter */
            {
                *ptr2 = '\0';

                if ( *(--ptr2) == ' ')  /* delete trailing spaces */
                    *ptr2 = '\0';
            }
            else
            {
                ffpmsg("input file URL is missing closing bracket ']'");
                ffpmsg(rowfilter);
                free(infile);
                return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
            }
        }

        /* delete the compression spec from the row filter string */
        ptr2 = strchr(ptr1, ']');
        strcpy(tmpstr, ptr2+1);  /* copy any chars after the binspec */
        strcpy(ptr1, tmpstr);    /* overwrite binspec */
    }
   
    /* copy the remaining string to the rowfilter output... should only */
    /* contain a rowfilter expression of the form "[expr]"              */

    if (rowfilterx && rowfilter[0]) {
       ptr2 = rowfilter + strlen(rowfilter) - 1;
       if( rowfilter[0]=='[' && *ptr2==']' ) {
          *ptr2 = '\0';

	   if (strlen(rowfilter + 1)  > FLEN_FILENAME - 1)
	   {
                    free(infile);
                    return(*status = URL_PARSE_ERROR);
           }

          strcpy(rowfilterx, rowfilter+1);
       } else {
          ffpmsg("input file URL lacks valid row filter expression");
          *status = URL_PARSE_ERROR;
       }
    }

    free(infile);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffexist(const char *infile, /* I - input filename or URL */
            int *exists,        /* O -  2 = a compressed version of file exists */
	                        /*      1 = yes, disk file exists               */
	                        /*      0 = no, disk file could not be found    */
				/*     -1 = infile is not a disk file (could    */
				/*   be a http, ftp, gsiftp, smem, or stdin file) */
            int *status)        /* I/O  status  */

/*
   test if the input file specifier is an existing file on disk
   If the specified file can't be found, it then searches for a 
   compressed version of the file.
*/
{
    FILE *diskfile;
    char rootname[FLEN_FILENAME];
    char *ptr1;
    
    if (*status > 0)
        return(*status);

    /* strip off any extname or filters from the name */
    ffrtnm( (char *)infile, rootname, status);

    ptr1 = strstr(rootname, "://");
    
    if (ptr1 || *rootname == '-') {
        if (!strncmp(rootname, "file", 4) ) {
	    ptr1 = ptr1 + 3;   /* pointer to start of the disk file name */
	} else {
	    *exists = -1;   /* this is not a disk file */
	    return (*status);
	}
    } else {
        ptr1 = rootname;
    }
    
    /* see if the disk file exists */
    if (file_openfile(ptr1, 0, &diskfile)) {
    
        /* no, couldn't open file, so see if there is a compressed version */
        if (file_is_compressed(ptr1) ) {
           *exists = 2;  /* a compressed version of the file exists */
        } else {
	   *exists = 0;  /* neither file nor compressed version exist */
	}
	
    } else {
    
        /* yes, file exists */
        *exists = 1; 
	fclose(diskfile);
    }
    	   
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffrtnm(char *url, 
           char *rootname,
           int *status)
/*
   parse the input URL, returning the root name (filetype://basename).
*/

{ 
    int ii, jj, slen, infilelen;
    char *ptr1, *ptr2, *ptr3;
    char urltype[MAX_PREFIX_LEN];
    char infile[FLEN_FILENAME];

    if (*status > 0)
        return(*status);

    ptr1 = url;
    *rootname = '\0';
    *urltype = '\0';
    *infile  = '\0';

    /*  get urltype (e.g., file://, ftp://, http://, etc.)  */
    if (*ptr1 == '-')        /* "-" means read file from stdin */
    {
        strcat(urltype, "-");
        ptr1++;
    }
    else if (!strncmp(ptr1, "stdin", 5) || !strncmp(ptr1, "STDIN", 5))
    {
        strcat(urltype, "-");
        ptr1 = ptr1 + 5;
    }
    else
    {
        ptr2 = strstr(ptr1, "://");
        ptr3 = strstr(ptr1, "(" );

        if (ptr3 && (ptr3 < ptr2) )
        {
           /* the urltype follows a '(' character, so it must apply */
           /* to the output file, and is not the urltype of the input file */
           ptr2 = 0;   /* so reset pointer to zero */
        }


        if (ptr2)                  /* copy the explicit urltype string */ 
        {

	   if (ptr2 - ptr1 + 3 > MAX_PREFIX_LEN - 1)
	   {
               return(*status = URL_PARSE_ERROR);
           }
            strncat(urltype, ptr1, ptr2 - ptr1 + 3);
            ptr1 = ptr2 + 3;
        }
        else if (!strncmp(ptr1, "ftp:", 4) )
        {                              /* the 2 //'s are optional */
            strcat(urltype, "ftp://");
            ptr1 += 4;
        }
        else if (!strncmp(ptr1, "gsiftp:", 7) )
        {                              /* the 2 //'s are optional */
            strcat(urltype, "gsiftp://");
            ptr1 += 7;
        }
        else if (!strncmp(ptr1, "http:", 5) )
        {                              /* the 2 //'s are optional */
            strcat(urltype, "http://");
            ptr1 += 5;
        }
        else if (!strncmp(ptr1, "mem:", 4) )
        {                              /* the 2 //'s are optional */
            strcat(urltype, "mem://");
            ptr1 += 4;
        }
        else if (!strncmp(ptr1, "shmem:", 6) )
        {                              /* the 2 //'s are optional */
            strcat(urltype, "shmem://");
            ptr1 += 6;
        }
        else if (!strncmp(ptr1, "file:", 5) )
        {                              /* the 2 //'s are optional */
            ptr1 += 5;
        }

        /* else assume file driver    */
    }
 
       /*  get the input file name  */
    ptr2 = strchr(ptr1, '(');   /* search for opening parenthesis ( */
    ptr3 = strchr(ptr1, '[');   /* search for opening bracket [ */

    if (ptr2 == ptr3)  /* simple case: no [ or ( in the file name */
    {

	if (strlen(ptr1) > FLEN_FILENAME - 1)
        {
            return(*status = URL_PARSE_ERROR);
        }

        strcat(infile, ptr1);
    }
    else if (!ptr3)     /* no bracket, so () enclose output file name */
    {

	if (ptr2 - ptr1 > FLEN_FILENAME - 1)
        {
            return(*status = URL_PARSE_ERROR);
        }

        strncat(infile, ptr1, ptr2 - ptr1);
        ptr2++;

        ptr1 = strchr(ptr2, ')' );   /* search for closing ) */
        if (!ptr1)
            return(*status = URL_PARSE_ERROR);  /* error, no closing ) */

    }
    else if (ptr2 && (ptr2 < ptr3)) /* () enclose output name before bracket */
    {

	if (ptr2 - ptr1 > FLEN_FILENAME - 1)
        {
            return(*status = URL_PARSE_ERROR); 
        }

        strncat(infile, ptr1, ptr2 - ptr1);
        ptr2++;

        ptr1 = strchr(ptr2, ')' );   /* search for closing ) */
        if (!ptr1)
            return(*status = URL_PARSE_ERROR);  /* error, no closing ) */
    }
    else    /*   bracket comes first, so there is no output name */
    {
	if (ptr3 - ptr1 > FLEN_FILENAME - 1)
        {
            return(*status = URL_PARSE_ERROR); 
        }

        strncat(infile, ptr1, ptr3 - ptr1);
    }

       /* strip off any trailing blanks in the names */
    slen = strlen(infile);
    for (ii = slen - 1; ii > 0; ii--)   
    {
        if (infile[ii] == ' ')
            infile[ii] = '\0';
        else
            break;
    }

    /* --------------------------------------------- */
    /* check if the 'filename+n' convention has been */
    /* used to specifiy which HDU number to open     */ 
    /* --------------------------------------------- */

    jj = strlen(infile);

    for (ii = jj - 1; ii >= 0; ii--)
    {
        if (infile[ii] == '+')    /* search backwards for '+' sign */
            break;
    }

    if (ii > 0 && (jj - ii) < 5)  /* limit extension numbers to 4 digits */
    {
        infilelen = ii;
        ii++;


        for (; ii < jj; ii++)
        {
            if (!isdigit((int) infile[ii] ) ) /* are all the chars digits? */
                break;
        }

        if (ii == jj)      
        {
             /* yes, the '+n' convention was used.  */

             infile[infilelen] = '\0'; /* delete the extension number */
        }
    }

    if (strlen(urltype) + strlen(infile) > FLEN_FILENAME - 1)
    {
            return(*status = URL_PARSE_ERROR); 
    }

    strcat(rootname, urltype);  /* construct the root name */
    strcat(rootname, infile);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffourl(char *url,             /* I - full input URL   */
           char *urltype,          /* O - url type         */
           char *outfile,          /* O - base file name   */
           char *tpltfile,         /* O - template file name, if any */
           char *compspec,         /* O - compression specification, if any */
           int *status)
/*
   parse the output URL into its basic components.
*/

{ 
    char *ptr1, *ptr2, *ptr3;

    if (*status > 0)
        return(*status);

    if (urltype)
      *urltype = '\0';
    if (outfile)
      *outfile = '\0';
    if (tpltfile)
      *tpltfile = '\0';
    if (compspec)
      *compspec = '\0';

    ptr1 = url;
    while (*ptr1 == ' ')    /* ignore leading blanks */
           ptr1++;

    if ( ( (*ptr1 == '-') &&  ( *(ptr1 +1) ==  0   || *(ptr1 +1) == ' ' ) )
         ||  !strcmp(ptr1, "stdout")
         ||  !strcmp(ptr1, "STDOUT"))

         /* "-" means write to stdout;  also support "- "            */
         /* but exclude disk file names that begin with a minus sign */
         /* e.g., "-55d33m.fits"   */
    {
      if (urltype)
        strcpy(urltype, "stdout://");
    }
    else
    {
        /* not writing to stdout */
        /*  get urltype (e.g., file://, ftp://, http://, etc.)  */

        ptr2 = strstr(ptr1, "://");
        if (ptr2)                  /* copy the explicit urltype string */ 
        {
          if (urltype) {
	    if (ptr2 - ptr1 + 3 > MAX_PREFIX_LEN - 1)
	    {
                return(*status = URL_PARSE_ERROR); 
            }

            strncat(urltype, ptr1, ptr2 - ptr1 + 3);
          }

          ptr1 = ptr2 + 3;
        }
        else                       /* assume file driver    */
        {
          if (urltype)
             strcat(urltype, "file://");
        }

        /* look for template file name, enclosed in parenthesis */
        ptr2 = strchr(ptr1, '('); 

        /* look for image compression parameters, enclosed in sq. brackets */
        ptr3 = strchr(ptr1, '['); 

        if (outfile)
        {
          if (ptr2) {  /* template file was specified  */
	     if (ptr2 - ptr1 > FLEN_FILENAME - 1)
	     {
                return(*status = URL_PARSE_ERROR); 
             }
 
             strncat(outfile, ptr1, ptr2 - ptr1);
          } else if (ptr3) {  /* compression was specified  */
	     if (ptr3 - ptr1 > FLEN_FILENAME - 1)
	     {
                return(*status = URL_PARSE_ERROR); 
             }
             strncat(outfile, ptr1, ptr3 - ptr1);

          } else { /* no template file or compression */
	     if (strlen(ptr1) > FLEN_FILENAME - 1)
	     {
                return(*status = URL_PARSE_ERROR); 
             }
             strcpy(outfile, ptr1);
          }
        }


        if (ptr2)   /* template file was specified  */
        {
            ptr2++;

            ptr1 = strchr(ptr2, ')' );   /* search for closing ) */

            if (!ptr1)
            {
                return(*status = URL_PARSE_ERROR);  /* error, no closing ) */
            }

            if (tpltfile) {
	        if (ptr1 - ptr2 > FLEN_FILENAME - 1)
	        {
                   return(*status = URL_PARSE_ERROR); 
                }
                 strncat(tpltfile, ptr2, ptr1 - ptr2);
            }
        }
        
        if (ptr3)   /* compression was specified  */
        {
            ptr3++;

            ptr1 = strchr(ptr3, ']' );   /* search for closing ] */

            if (!ptr1)
            {
                return(*status = URL_PARSE_ERROR);  /* error, no closing ] */
            }

            if (compspec) {

	        if (ptr1 - ptr3 > FLEN_FILENAME - 1)
	        {
                   return(*status = URL_PARSE_ERROR); 
                }
 
                strncat(compspec, ptr3, ptr1 - ptr3);
            }
        }

        /* check if a .gz compressed output file is to be created */
        /* by seeing if the filename ends in '.gz'   */
        if (urltype && outfile)
        {
            if (!strcmp(urltype, "file://") )
            {
                ptr1 = strstr(outfile, ".gz");
                if (ptr1)
                {    /* make sure the ".gz" is at the end of the file name */
                   ptr1 += 3;
                   if (*ptr1 ==  0  || *ptr1 == ' '  )
                      strcpy(urltype, "compressoutfile://");
                }
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffexts(char *extspec, 
                       int *extnum, 
                       char *extname,
                       int *extvers,
                       int *hdutype,
                       char *imagecolname,
                       char *rowexpress,
                       int *status)
{
/*
   Parse the input extension specification string, returning either the
   extension number or the values of the EXTNAME, EXTVERS, and XTENSION
   keywords in desired extension. Also return the name of the column containing
   an image, and an expression to be used to determine which row to use,
   if present.
*/
    char *ptr1, *ptr2;
    int slen, nvals;
    int notint = 1; /* initially assume specified extname is not an integer */
    char tmpname[FLEN_VALUE], *loc;

    *extnum = 0;
    *extname = '\0';
    *extvers = 0;
    *hdutype = ANY_HDU;
    *imagecolname = '\0';
    *rowexpress = '\0';

    if (*status > 0)
        return(*status);

    ptr1 = extspec;       /* pointer to first char */

    while (*ptr1 == ' ')  /* skip over any leading blanks */
        ptr1++;

    if (isdigit((int) *ptr1))  /* is the extension specification a number? */
    {
        notint = 0;  /* looks like extname may actually be the ext. number */
        errno = 0;  /* reset this prior to calling strtol */
        *extnum = strtol(ptr1, &loc, 10);  /* read the string as an integer */

        while (*loc == ' ')  /* skip over trailing blanks */
           loc++;

        /* check for read error, or junk following the integer */
        if ((*loc != '\0' && *loc != ';' ) || (errno == ERANGE) )
        {
           *extnum = 0;
           notint = 1;  /* no, extname was not a simple integer after all */
           errno = 0;  /* reset error condition flag if it was set */
        }

        if ( *extnum < 0 || *extnum > 99999)
        {
            *extnum = 0;   /* this is not a reasonable extension number */
            ffpmsg("specified extension number is out of range:");
            ffpmsg(extspec);
            return(*status = URL_PARSE_ERROR); 
        }
    }


/*  This logic was too simple, and failed on extnames like '1000TEMP' 
    where it would try to move to the 1000th extension

    if (isdigit((int) *ptr1))  
    {
        sscanf(ptr1, "%d", extnum);
        if (*extnum < 0 || *extnum > 9999)
        {
            *extnum = 0;   
            ffpmsg("specified extension number is out of range:");
            ffpmsg(extspec);
            return(*status = URL_PARSE_ERROR); 
        }
    }
*/

    if (notint)
    {
           /* not a number, so EXTNAME must be specified, followed by */
           /* optional EXTVERS and XTENSION  values */

           /* don't use space char as end indicator, because there */
           /* may be imbedded spaces in the EXTNAME value */
           slen = strcspn(ptr1, ",:;");   /* length of EXTNAME */

	   if (slen > FLEN_VALUE - 1)
	   {
                return(*status = URL_PARSE_ERROR); 
           }
 
           strncat(extname, ptr1, slen);  /* EXTNAME value */

           /* now remove any trailing blanks */
           while (slen > 0 && *(extname + slen -1) == ' ')
           {
               *(extname + slen -1) = '\0';
               slen--;
           }

           ptr1 += slen;
           slen = strspn(ptr1, " ,:");  /* skip delimiter characters */
           ptr1 += slen;

           slen = strcspn(ptr1, " ,:;");   /* length of EXTVERS */
           if (slen)
           {
               nvals = sscanf(ptr1, "%d", extvers);  /* EXTVERS value */
               if (nvals != 1)
               {
                   ffpmsg("illegal EXTVER value in input URL:");
                   ffpmsg(extspec);
                   return(*status = URL_PARSE_ERROR);
               }

               ptr1 += slen;
               slen = strspn(ptr1, " ,:");  /* skip delimiter characters */
               ptr1 += slen;

               slen = strcspn(ptr1, ";");   /* length of HDUTYPE */
               if (slen)
               {
                 if (*ptr1 == 'b' || *ptr1 == 'B')
                     *hdutype = BINARY_TBL;  
                 else if (*ptr1 == 't' || *ptr1 == 'T' ||
                          *ptr1 == 'a' || *ptr1 == 'A')
                     *hdutype = ASCII_TBL;
                 else if (*ptr1 == 'i' || *ptr1 == 'I')
                     *hdutype = IMAGE_HDU;
                 else
                 {
                     ffpmsg("unknown type of HDU in input URL:");
                     ffpmsg(extspec);
                     return(*status = URL_PARSE_ERROR);
                 }
               }
           }
           else
           {
                strcpy(tmpname, extname);
                ffupch(tmpname);
                if (!strcmp(tmpname, "PRIMARY") || !strcmp(tmpname, "P") )
                    *extname = '\0';  /* return extnum = 0 */
           }
    }

    ptr1 = strchr(ptr1, ';');
    if (ptr1)
    {
        /* an image is to be opened; the image is contained in a single */
        /* cell of a binary table.  A column name and an expression to  */
        /* determine which row to use has been entered.                 */

        ptr1++;  /* skip over the ';' delimiter */
        while (*ptr1 == ' ')  /* skip over any leading blanks */
            ptr1++;

        ptr2 = strchr(ptr1, '(');
        if (!ptr2)
        {
            ffpmsg("illegal specification of image in table cell in input URL:");
            ffpmsg(" did not find a row expression enclosed in ( )");
            ffpmsg(extspec);
            return(*status = URL_PARSE_ERROR);
        }

	if (ptr2 - ptr1 > FLEN_FILENAME - 1)
	{
            return(*status = URL_PARSE_ERROR); 
        }

        strncat(imagecolname, ptr1, ptr2 - ptr1); /* copy column name */

        ptr2++;  /* skip over the '(' delimiter */
        while (*ptr2 == ' ')  /* skip over any leading blanks */
            ptr2++;


        ptr1 = strchr(ptr2, ')');
        if (!ptr2)
        {
            ffpmsg("illegal specification of image in table cell in input URL:");
            ffpmsg(" missing closing ')' character in row expression");
            ffpmsg(extspec);
            return(*status = URL_PARSE_ERROR);
        }

	if (ptr1 - ptr2 > FLEN_FILENAME - 1)
        {
                return(*status = URL_PARSE_ERROR); 
        }
 
        strncat(rowexpress, ptr2, ptr1 - ptr2); /* row expression */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffextn(char *url,           /* I - input filename/URL  */
           int *extension_num,  /* O - returned extension number */
           int *status)
{
/*
   Parse the input url string and return the number of the extension that
   CFITSIO would automatically move to if CFITSIO were to open this input URL.
   The extension numbers are one's based, so 1 = the primary array, 2 = the
   first extension, etc.

   The extension number that gets returned is determined by the following 
   algorithm:

   1. If the input URL includes a binning specification (e.g.
   'myfile.fits[3][bin X,Y]') then the returned extension number
   will always = 1, since CFITSIO would create a temporary primary
   image on the fly in this case.  The same is true if an image
   within a single cell of a binary table is opened.

   2.  Else if the input URL specifies an extension number (e.g.,
   'myfile.fits[3]' or 'myfile.fits+3') then the specified extension
   number (+ 1) is returned.  

   3.  Else if the extension name is specified in brackets
   (e.g., this 'myfile.fits[EVENTS]') then the file will be opened and searched
   for the extension number.  If the input URL is '-'  (reading from the stdin
   file stream) this is not possible and an error will be returned.

   4.  Else if the URL does not specify an extension (e.g. 'myfile.fits') then
   a special extension number = -99 will be returned to signal that no
   extension was specified.  This feature is mainly for compatibility with
   existing FTOOLS software.  CFITSIO would open the primary array by default
   (extension_num = 1) in this case.

*/
    fitsfile *fptr;
    char urltype[20];
    char infile[FLEN_FILENAME];
    char outfile[FLEN_FILENAME]; 
    char extspec[FLEN_FILENAME];
    char extname[FLEN_FILENAME];
    char rowfilter[FLEN_FILENAME];
    char binspec[FLEN_FILENAME];
    char colspec[FLEN_FILENAME];
    char imagecolname[FLEN_VALUE], rowexpress[FLEN_FILENAME];
    char *cptr;
    int extnum, extvers, hdutype, tstatus = 0;

    if (*status > 0)
        return(*status);

    /*  parse the input URL into its basic components  */
    fits_parse_input_url(url, urltype, infile, outfile,
             extspec, rowfilter,binspec, colspec, status);

    if (*status > 0)
        return(*status);

    if (*binspec)   /* is there a binning specification? */
    {
       *extension_num = 1; /* a temporary primary array image is created */
       return(*status);
    }

    if (*extspec)   /* is an extension specified? */
    {
       ffexts(extspec, &extnum, 
         extname, &extvers, &hdutype, imagecolname, rowexpress, status);

      if (*status > 0)
        return(*status);

      if (*imagecolname)   /* is an image within a table cell being opened? */
      {
         *extension_num = 1; /* a temporary primary array image is created */
         return(*status);
      }

      if (*extname)
      {
         /* have to open the file to search for the extension name (curses!) */

         if (!strcmp(urltype, "stdin://"))
            /* opening stdin would destroying it! */
            return(*status = URL_PARSE_ERROR); 

         /* First, strip off any filtering specification */
         infile[0] = '\0';
	 strncat(infile, url, FLEN_FILENAME -1);
	 
         cptr = strchr(infile, ']');  /* locate the closing bracket */
         if (!cptr)
         {
             return(*status = URL_PARSE_ERROR);
         }
         else
         {
             cptr++;
             *cptr = '\0'; /* terminate URl after the extension spec */
         }

         if (ffopen(&fptr, infile, READONLY, status) > 0) /* open the file */
         {
            ffclos(fptr, &tstatus);
            return(*status);
         }

         ffghdn(fptr, &extnum);    /* where am I in the file? */
         *extension_num = extnum;
         ffclos(fptr, status);

         return(*status);
      }
      else
      {
         *extension_num = extnum + 1;  /* return the specified number (+ 1) */
         return(*status);
      }
    }
    else
    {
         *extension_num = -99;  /* no specific extension was specified */
                                /* defaults to primary array */
         return(*status);
    }
}
/*--------------------------------------------------------------------------*/

int ffurlt(fitsfile *fptr, char *urlType, int *status)
/*
   return the prefix string associated with the driver in use by the
   fitsfile pointer fptr
*/

{ 
  strcpy(urlType, driverTable[fptr->Fptr->driver].prefix);
  return(*status);
}

/*--------------------------------------------------------------------------*/
int ffimport_file( char *filename,   /* Text file to read                   */
                   char **contents,  /* Pointer to pointer to hold file     */
                   int *status )     /* CFITSIO error code                  */
/*
   Read and concatenate all the lines from the given text file.  User
   must free the pointer returned in contents.  Pointer is guaranteed
   to hold 2 characters more than the length of the text... allows the
   calling routine to append (or prepend) a newline (or quotes?) without
   reallocating memory.
*/
{
   int allocLen, totalLen, llen, eoline = 1;
   char *lines,line[256];
   FILE *aFile;

   if( *status > 0 ) return( *status );

   totalLen =    0;
   allocLen = 1024;
   lines    = (char *)malloc( allocLen * sizeof(char) );
   if( !lines ) {
      ffpmsg("Couldn't allocate memory to hold ASCII file contents.");
      return(*status = MEMORY_ALLOCATION );
   }
   lines[0] = '\0';

   if( (aFile = fopen( filename, "r" ))==NULL ) {
      sprintf(line,"Could not open ASCII file %s.",filename);
      ffpmsg(line);
      free( lines );
      return(*status = FILE_NOT_OPENED);
   }

   while( fgets(line,256,aFile)!=NULL ) {
      llen = strlen(line);
      if ( eoline && (llen > 1) && (line[0] == '/' && line[1] == '/'))
          continue;       /* skip comment lines begging with // */

      eoline = 0;

      /* replace CR and newline chars at end of line with nulls */
      if ((llen > 0) && (line[llen-1]=='\n' || line[llen-1] == '\r')) {
          line[--llen] = '\0';
          eoline = 1;   /* found an end of line character */

          if ((llen > 0) && (line[llen-1]=='\n' || line[llen-1] == '\r')) {
                 line[--llen] = '\0';
          }
      }

      if( totalLen + llen + 3 >= allocLen ) {
         allocLen += 256;
         lines = (char *)realloc(lines, allocLen * sizeof(char) );
         if( ! lines ) {
            ffpmsg("Couldn't allocate memory to hold ASCII file contents.");
            *status = MEMORY_ALLOCATION;
            break;
         }
      }
      strcpy( lines+totalLen, line );
      totalLen += llen;

      if (eoline) {
         strcpy( lines+totalLen, " "); /* add a space between lines */
         totalLen += 1;
      }
   }
   fclose(aFile);

   *contents = lines;
   return( *status );
}

/*--------------------------------------------------------------------------*/
int fits_get_token(char **ptr, 
                   char *delimiter,
                   char *token,
                   int *isanumber)   /* O - is this token a number? */
/*
   parse off the next token, delimited by a character in 'delimiter',
   from the input ptr string;  increment *ptr to the end of the token.
   Returns the length of the token, not including the delimiter char;
*/
{
    char *loc, tval[73];
    int slen;
    double dval;
    
    *token = '\0';

    while (**ptr == ' ')  /* skip over leading blanks */
        (*ptr)++;

    slen = strcspn(*ptr, delimiter);  /* length of next token */
    if (slen)
    {
        strncat(token, *ptr, slen);       /* copy token */

        (*ptr) += slen;                   /* skip over the token */

        if (isanumber)  /* check if token is a number */
        {
            *isanumber = 1;

	    if (strchr(token, 'D'))  {
	        strncpy(tval, token, 72);
		tval[72] = '\0';

	        /*  The C language does not support a 'D'; replace with 'E' */
	        if ((loc = strchr(tval, 'D'))) *loc = 'E';

	        dval =  strtod(tval, &loc);
	    } else {
	        dval =  strtod(token, &loc);
 	    }

	    /* check for read error, or junk following the value */
	    if (*loc != '\0' && *loc != ' ' ) *isanumber = 0;
	    if (errno == ERANGE) *isanumber = 0;
        }
    }

    return(slen);
}
/*--------------------------------------------------------------------------*/
int fits_get_token2(char **ptr, 
                   char *delimiter,
                   char **token,
                   int *isanumber,  /* O - is this token a number? */
		   int *status)

/*
   parse off the next token, delimited by a character in 'delimiter',
   from the input ptr string;  increment *ptr to the end of the token.
   Returns the length of the token, not including the delimiter char;

   This routine allocates the *token string;  the calling routine must free it 
*/
{
    char *loc, tval[73];
    int slen;
    double dval;
    
    if (*status)
        return(0);
	
    while (**ptr == ' ')  /* skip over leading blanks */
        (*ptr)++;

    slen = strcspn(*ptr, delimiter);  /* length of next token */
    if (slen)
    {
	*token = (char *) calloc(slen + 1, 1); 
	if (!(*token)) {
          ffpmsg("Couldn't allocate memory to hold token string (fits_get_token2).");
          *status = MEMORY_ALLOCATION ;
	  return(0);
        }
 
        strncat(*token, *ptr, slen);       /* copy token */
        (*ptr) += slen;                   /* skip over the token */

        if (isanumber)  /* check if token is a number */
        {
            *isanumber = 1;

	    if (strchr(*token, 'D'))  {
	        strncpy(tval, *token, 72);
		tval[72] = '\0';

	        /*  The C language does not support a 'D'; replace with 'E' */
	        if ((loc = strchr(tval, 'D'))) *loc = 'E';

	        dval =  strtod(tval, &loc);
	    } else {
	        dval =  strtod(*token, &loc);
 	    }

	    /* check for read error, or junk following the value */
	    if (*loc != '\0' && *loc != ' ' ) *isanumber = 0;
	    if (errno == ERANGE) *isanumber = 0;
        }
    }

    return(slen);
}
/*---------------------------------------------------------------------------*/
char *fits_split_names(
   char *list)   /* I   - input list of names */
{
/*  
   A sequence of calls to fits_split_names will split the input string
   into name tokens.  The string typically contains a list of file or
   column names.  The names must be delimited by a comma and/or spaces.
   This routine ignores spaces and commas that occur within parentheses,
   brackets, or curly brackets.  It also strips any leading and trailing
   blanks from the returned name.

   This routine is similar to the ANSI C 'strtok' function:

   The first call to fits_split_names has a non-null input string.
   It finds the first name in the string and terminates it by
   overwriting the next character of the string with a '\0' and returns
   a pointer to the name.  Each subsequent call, indicated by a NULL
   value of the input string, returns the next name, searching from
   just past the end of the previous name.  It returns NULL when no
   further names are found.

   The following line illustrates how a string would be split into 3 names:
    myfile[1][bin (x,y)=4], file2.fits  file3.fits
    ^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^  ^^^^^^^^^^
      1st name               2nd name    3rd name


NOTE:  This routine is not thread-safe.  
This routine is simply provided as a utility routine for other external
software. It is not used by any CFITSIO routine.

*/
    int depth = 0;
    char *start;
    static char *ptr;

    if (list)  /* reset ptr if a string is given */
        ptr = list;

    while (*ptr == ' ')ptr++;  /* skip leading white space */

    if (*ptr == '\0')return(0);  /* no remaining file names */

    start = ptr;

    while (*ptr != '\0') {
       if ((*ptr == '[') || (*ptr == '(') || (*ptr == '{')) depth ++;
       else if ((*ptr == '}') || (*ptr == ')') || (*ptr == ']')) depth --;
       else if ((depth == 0) && (*ptr == ','  || *ptr == ' ')) {
          *ptr = '\0';  /* terminate the filename here */
          ptr++;  /* save pointer to start of next filename */
          break;  
       }
       ptr++;
    }
    
    return(start);
}
/*--------------------------------------------------------------------------*/
int urltype2driver(char *urltype, int *driver)
/*
   compare input URL with list of known drivers, returning the
   matching driver numberL.
*/

{ 
    int ii;

       /* find matching driver; search most recent drivers first */

    for (ii=no_of_drivers - 1; ii >= 0; ii--)
    {
        if (0 == strcmp(driverTable[ii].prefix, urltype))
        { 
             *driver = ii;
             return(0);
        }
    }

    return(NO_MATCHING_DRIVER);   
}
/*--------------------------------------------------------------------------*/
int ffclos(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  close the FITS file by completing the current HDU, flushing it to disk,
  then calling the system dependent routine to physically close the FITS file
*/   
{
    int tstatus = NO_CLOSE_ERROR, zerostatus = 0;

    if (!fptr)
        return(*status = NULL_INPUT_PTR);
    else if ((fptr->Fptr)->validcode != VALIDSTRUC) /* check for magic value */
        return(*status = BAD_FILEPTR); 

    /* close and flush the current HDU */
    if (*status > 0)
       ffchdu(fptr, &tstatus);  /* turn off the error message from ffchdu */
    else
       ffchdu(fptr, status);         

    ((fptr->Fptr)->open_count)--;           /* decrement usage counter */

    if ((fptr->Fptr)->open_count == 0)  /* if no other files use structure */
    {
        ffflsh(fptr, TRUE, status);   /* flush and disassociate IO buffers */

        /* call driver function to actually close the file */
        if ((*driverTable[(fptr->Fptr)->driver].close)((fptr->Fptr)->filehandle))
        {
            if (*status <= 0)
            {
              *status = FILE_NOT_CLOSED;  /* report if no previous error */

              ffpmsg("failed to close the following file: (ffclos)");
              ffpmsg((fptr->Fptr)->filename);
            }
        }

        fits_clear_Fptr( fptr->Fptr, status);  /* clear Fptr address */
        free((fptr->Fptr)->iobuffer);    /* free memory for I/O buffers */
        free((fptr->Fptr)->headstart);    /* free memory for headstart array */
        free((fptr->Fptr)->filename);     /* free memory for the filename */
        (fptr->Fptr)->filename = 0;
        (fptr->Fptr)->validcode = 0; /* magic value to indicate invalid fptr */
        free(fptr->Fptr);         /* free memory for the FITS file structure */
        free(fptr);               /* free memory for the FITS file structure */
    }
    else
    {
        /*
           to minimize the fallout from any previous error (e.g., trying to 
           open a non-existent extension in a already opened file), 
           always call ffflsh with status = 0.
        */
        /* just flush the buffers, don't disassociate them */
        if (*status > 0)
            ffflsh(fptr, FALSE, &zerostatus); 
        else
            ffflsh(fptr, FALSE, status); 

        free(fptr);               /* free memory for the FITS file structure */
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffdelt(fitsfile *fptr,      /* I - FITS file pointer */
           int *status)         /* IO - error status     */
/*
  close and DELETE the FITS file. 
*/
{
    char *basename;
    int slen, tstatus = NO_CLOSE_ERROR, zerostatus = 0;

    if (!fptr)
        return(*status = NULL_INPUT_PTR);
    else if ((fptr->Fptr)->validcode != VALIDSTRUC) /* check for magic value */
        return(*status = BAD_FILEPTR); 

    if (*status > 0)
       ffchdu(fptr, &tstatus);  /* turn off the error message from ffchdu */
    else
        ffchdu(fptr, status);  

    ffflsh(fptr, TRUE, status);     /* flush and disassociate IO buffers */

        /* call driver function to actually close the file */
    if ( (*driverTable[(fptr->Fptr)->driver].close)((fptr->Fptr)->filehandle) )
    {
        if (*status <= 0)
        {
            *status = FILE_NOT_CLOSED;  /* report error if no previous error */

            ffpmsg("failed to close the following file: (ffdelt)");
            ffpmsg((fptr->Fptr)->filename);
        }
    }

    /* call driver function to actually delete the file */
    if ( (driverTable[(fptr->Fptr)->driver].remove) )
    {
        /* parse the input URL to get the base filename */
        slen = strlen((fptr->Fptr)->filename);
        basename = (char *) malloc(slen +1);
        if (!basename)
            return(*status = MEMORY_ALLOCATION);
    
        fits_parse_input_url((fptr->Fptr)->filename, NULL, basename, NULL, NULL, NULL, NULL,
               NULL, &zerostatus);

       if ((*driverTable[(fptr->Fptr)->driver].remove)(basename))
        {
            ffpmsg("failed to delete the following file: (ffdelt)");
            ffpmsg((fptr->Fptr)->filename);
            if (!(*status))
                *status = FILE_NOT_CLOSED;
        }
        free(basename);
    }

    fits_clear_Fptr( fptr->Fptr, status);  /* clear Fptr address */
    free((fptr->Fptr)->iobuffer);    /* free memory for I/O buffers */
    free((fptr->Fptr)->headstart);    /* free memory for headstart array */
    free((fptr->Fptr)->filename);     /* free memory for the filename */
    (fptr->Fptr)->filename = 0;
    (fptr->Fptr)->validcode = 0;      /* magic value to indicate invalid fptr */
    free(fptr->Fptr);              /* free memory for the FITS file structure */
    free(fptr);                    /* free memory for the FITS file structure */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftrun( fitsfile *fptr,    /* I - FITS file pointer           */
             LONGLONG filesize,   /* I - size to truncate the file   */
             int *status)      /* O - error status                */
/*
  low level routine to truncate a file to a new smaller size.
*/
{
  if (driverTable[(fptr->Fptr)->driver].truncate)
  {
    ffflsh(fptr, FALSE, status);  /* flush all the buffers first */
    (fptr->Fptr)->filesize = filesize;
    (fptr->Fptr)->io_pos = filesize;
    (fptr->Fptr)->logfilesize = filesize;
    (fptr->Fptr)->bytepos = filesize;
    ffbfeof(fptr, status);   /* eliminate any buffers beyond current EOF */
    return (*status = 
     (*driverTable[(fptr->Fptr)->driver].truncate)((fptr->Fptr)->filehandle,
     filesize) );
  }
  else
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffflushx( FITSfile *fptr)     /* I - FITS file pointer                  */
/*
  low level routine to flush internal file buffers to the file.
*/
{
    if (driverTable[fptr->driver].flush)
        return ( (*driverTable[fptr->driver].flush)(fptr->filehandle) );
    else
        return(0);    /* no flush function defined for this driver */
}
/*--------------------------------------------------------------------------*/
int ffseek( FITSfile *fptr,   /* I - FITS file pointer              */
            LONGLONG position)   /* I - byte position to seek to       */
/*
  low level routine to seek to a position in a file.
*/
{
    return( (*driverTable[fptr->driver].seek)(fptr->filehandle, position) );
}
/*--------------------------------------------------------------------------*/
int ffwrite( FITSfile *fptr,   /* I - FITS file pointer              */
             long nbytes,      /* I - number of bytes to write       */
             void *buffer,     /* I - buffer to write                */
             int *status)      /* O - error status                   */
/*
  low level routine to write bytes to a file.
*/
{
    if ( (*driverTable[fptr->driver].write)(fptr->filehandle, buffer, nbytes) )
    {
        ffpmsg("Error writing data buffer to file:");
	ffpmsg(fptr->filename);

        *status = WRITE_ERROR;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffread( FITSfile *fptr,   /* I - FITS file pointer              */
            long nbytes,      /* I - number of bytes to read        */
            void *buffer,     /* O - buffer to read into            */
            int *status)      /* O - error status                   */
/*
  low level routine to read bytes from a file.
*/
{
    int readstatus;

    readstatus = (*driverTable[fptr->driver].read)(fptr->filehandle, 
        buffer, nbytes);

    if (readstatus == END_OF_FILE)
        *status = END_OF_FILE;
    else if (readstatus > 0)
    {
        ffpmsg("Error reading data buffer from file:");
	ffpmsg(fptr->filename);

        *status = READ_ERROR;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int fftplt(fitsfile **fptr,      /* O - FITS file pointer                   */
           const char *filename, /* I - name of file to create              */
           const char *tempname, /* I - name of template file               */
           int *status)          /* IO - error status                       */
/*
  Create and initialize a new FITS file  based on a template file.
  Uses C fopen and fgets functions.
*/
{
    *fptr = 0;              /* initialize null file pointer, */
                            /* regardless of the value of *status */
    if (*status > 0)
        return(*status);

    if ( ffinit(fptr, filename, status) )  /* create empty file */
        return(*status);

    ffoptplt(*fptr, tempname, status);  /* open and use template */

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffoptplt(fitsfile *fptr,      /* O - FITS file pointer                   */
            const char *tempname, /* I - name of template file               */
            int *status)          /* IO - error status                       */
/*
  open template file and use it to create new file
*/
{
    fitsfile *tptr;
    int tstatus = 0, nkeys, nadd, ii;
    char card[FLEN_CARD];

    if (*status > 0)
        return(*status);

    if (tempname == NULL || *tempname == '\0')     /* no template file? */
        return(*status);

    /* try opening template */
    ffopen(&tptr, (char *) tempname, READONLY, &tstatus); 

    if (tstatus)  /* not a FITS file, so treat it as an ASCII template */
    {
        ffxmsg(2, card);  /* clear the  error message */
        fits_execute_template(fptr, (char *) tempname, status);

        ffmahd(fptr, 1, 0, status);   /* move back to the primary array */
        return(*status);
    }
    else  /* template is a valid FITS file */
    {
        ffmahd(tptr, 1, NULL, status); /* make sure we are at the beginning */
        while (*status <= 0)
        {
           ffghsp(tptr, &nkeys, &nadd, status); /* get no. of keywords */

           for (ii = 1; ii <= nkeys; ii++)   /* copy keywords */
           {
              ffgrec(tptr,  ii, card, status);

              /* must reset the PCOUNT keyword to zero in the new output file */
              if (strncmp(card, "PCOUNT  ",8) == 0) { /* the PCOUNT keyword? */
	         if (strncmp(card+25, "    0", 5)) {  /* non-zero value? */
		    strncpy(card, "PCOUNT  =                    0", 30);
		 }
	      }   
 
              ffprec(fptr, card, status);
           }

           ffmrhd(tptr, 1, 0, status); /* move to next HDU until error */
           ffcrhd(fptr, status);  /* create empty new HDU in output file */
        }

        if (*status == END_OF_FILE)
        {
           *status = 0;              /* expected error condition */
        }
        ffclos(tptr, status);       /* close the template file */
    }

    ffmahd(fptr, 1, 0, status);   /* move to the primary array */
    return(*status);
}
/*--------------------------------------------------------------------------*/
void ffrprt( FILE *stream, int status)
/* 
   Print out report of cfitsio error status and messages on the error stack.
   Uses C FILE stream.
*/
{
    char status_str[FLEN_STATUS], errmsg[FLEN_ERRMSG];
  
    if (status)
    {

      fits_get_errstatus(status, status_str);  /* get the error description */
      fprintf(stream, "\nFITSIO status = %d: %s\n", status, status_str);

      while ( fits_read_errmsg(errmsg) )  /* get error stack messages */
             fprintf(stream, "%s\n", errmsg);
    }
    return; 
}
/*--------------------------------------------------------------------------*/
int pixel_filter_helper(
           fitsfile **fptr,  /* IO - pointer to input image; on output it  */
                             /*      points to the new image */
           char *outfile,    /* I - name for output file        */
           char *expr,       /* I - Image filter expression    */
           int *status)
{
	PixelFilter filter = { 0 };
	char * DEFAULT_TAG = "X";
	int ii, hdunum;
        int singleHDU = 0;

	filter.count = 1;
	filter.ifptr = fptr;
	filter.tag = &DEFAULT_TAG;

    /* create new empty file for result */
    if (ffinit(&filter.ofptr, outfile, status) > 0)
    {
        ffpmsg("failed to create output file for pixel filter:");
        ffpmsg(outfile);
        return(*status);
    }

    fits_get_hdu_num(*fptr, &hdunum);  /* current HDU number in input file */

    expr += 3; /* skip 'pix' */
    switch (expr[0]) {
       case 'b': 
       case 'B': filter.bitpix = BYTE_IMG; break;
       case 'i':
       case 'I': filter.bitpix = SHORT_IMG; break;
       case 'j':
       case 'J': filter.bitpix = LONG_IMG; break;
       case 'r':
       case 'R': filter.bitpix = FLOAT_IMG; break;
       case 'd':
       case 'D': filter.bitpix = DOUBLE_IMG; break;
    }
    if (filter.bitpix) /* skip bitpix indicator */
       ++expr;

    if (*expr == '1') {
       ++expr;
       singleHDU = 1;
    }

    if (((*fptr)->Fptr)->only_one)
       singleHDU = 1;

    if (*expr != ' ') {
       ffpmsg("pixel filtering expression not space separated:");
       ffpmsg(expr);
    }
    while (*expr == ' ')
       ++expr;

    /* copy all preceding extensions to the output file */
    for (ii = 1; !singleHDU && ii < hdunum; ii++)
    {
        fits_movabs_hdu(*fptr, ii, NULL, status);
        if (fits_copy_hdu(*fptr, filter.ofptr, 0, status) > 0)
        {
            ffclos(filter.ofptr, status);
            return(*status);
        }
    }

    /* move back to the original HDU position */
    fits_movabs_hdu(*fptr, hdunum, NULL, status);

	filter.expression = expr;
    if (fits_pixel_filter(&filter, status)) {
        ffpmsg("failed to execute image filter:");
        ffpmsg(expr);
        ffclos(filter.ofptr, status);
        return(*status);
    }


    /* copy any remaining HDUs to the output file */

    for (ii = hdunum + 1; !singleHDU; ii++)
    {
        if (fits_movabs_hdu(*fptr, ii, NULL, status) > 0)
            break;

        fits_copy_hdu(*fptr, filter.ofptr, 0, status);
    }

    if (*status == END_OF_FILE)   
        *status = 0;              /* got the expected EOF error; reset = 0  */
    else if (*status > 0)
    {
        ffclos(filter.ofptr, status);
        return(*status);
    }

    /* close the original file and return ptr to the new image */
    ffclos(*fptr, status);

    *fptr = filter.ofptr; /* reset the pointer to the new table */

    /* move back to the image subsection */
    if (ii - 1 != hdunum)
        fits_movabs_hdu(*fptr, hdunum, NULL, status);

    return(*status);
}

/*-------------------------------------------------------------------*/
int ffihtps()
{
   /* Wrapper function for global initialization of curl library.
      This is NOT THREAD-SAFE */
   int status=0;
#ifdef CFITSIO_HAVE_CURL
   if (curl_global_init(CURL_GLOBAL_ALL))
      /* Do we want to define a new CFITSIO error code for this? */
      status = -1;
#endif
   return status;
}

/*-------------------------------------------------------------------*/
int ffchtps()
{
   /* Wrapper function for global cleanup of curl library.
      This is NOT THREAD-SAFE */
#ifdef CFITSIO_HAVE_CURL
   curl_global_cleanup();
#endif
   return 0;
}

/*-------------------------------------------------------------------*/
void ffvhtps(int flag)
{
   /* Turn libcurl's verbose output on (1) or off (0). 
      This is NOT THREAD-SAFE */
   https_set_verbose(flag);
}

