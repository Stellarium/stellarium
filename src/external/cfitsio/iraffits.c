/*------------------------------------------------------------------------*/
/*                                                                        */
/*  These routines have been modified by William Pence for use by CFITSIO */
/*        The original files were provided by Doug Mink                   */
/*------------------------------------------------------------------------*/

/* File imhfile.c
 * August 6, 1998
 * By Doug Mink, based on Mike VanHilst's readiraf.c

 * Module:      imhfile.c (IRAF .imh image file reading and writing)
 * Purpose:     Read and write IRAF image files (and translate headers)
 * Subroutine:  irafrhead (filename, lfhead, fitsheader, lihead)
 *              Read IRAF image header
 * Subroutine:  irafrimage (fitsheader)
 *              Read IRAF image pixels (call after irafrhead)
 * Subroutine:	same_path (pixname, hdrname)
 *		Put filename and header path together
 * Subroutine:	iraf2fits (hdrname, irafheader, nbiraf, nbfits)
 *		Convert IRAF image header to FITS image header
 * Subroutine:  irafgeti4 (irafheader, offset)
 *		Get 4-byte integer from arbitrary part of IRAF header
 * Subroutine:  irafgetc2 (irafheader, offset)
 *		Get character string from arbitrary part of IRAF v.1 header
 * Subroutine:  irafgetc (irafheader, offset)
 *		Get character string from arbitrary part of IRAF header
 * Subroutine:  iraf2str (irafstring, nchar)
 * 		Convert 2-byte/char IRAF string to 1-byte/char string
 * Subroutine:	irafswap (bitpix,string,nbytes)
 *		Swap bytes in string in place, with FITS bits/pixel code
 * Subroutine:	irafswap2 (string,nbytes)
 *		Swap bytes in string in place
 * Subroutine	irafswap4 (string,nbytes)
 *		Reverse bytes of Integer*4 or Real*4 vector in place
 * Subroutine	irafswap8 (string,nbytes)
 *		Reverse bytes of Real*8 vector in place


 * Copyright:   2000 Smithsonian Astrophysical Observatory
 *              You may do anything you like with this file except remove
 *              this copyright.  The Smithsonian Astrophysical Observatory
 *              makes no representations about the suitability of this
 *              software for any purpose.  It is provided "as is" without
 *              express or implied warranty.
 */

#include "fitsio2.h"
#include <stdio.h>		/* define stderr, FD, and NULL */
#include <stdlib.h>
#include <stddef.h>  /* stddef.h is apparently needed to define size_t */
#include <string.h>

#define FILE_NOT_OPENED 104

/* Parameters from iraf/lib/imhdr.h for IRAF version 1 images */
#define SZ_IMPIXFILE	 79		/* name of pixel storage file */
#define SZ_IMHDRFILE	 79   		/* length of header storage file */
#define SZ_IMTITLE	 79		/* image title string */
#define LEN_IMHDR	2052		/* length of std header */

/* Parameters from iraf/lib/imhdr.h for IRAF version 2 images */
#define	SZ_IM2PIXFILE	255		/* name of pixel storage file */
#define	SZ_IM2HDRFILE	255		/* name of header storage file */
#define	SZ_IM2TITLE	383		/* image title string */
#define LEN_IM2HDR	2046		/* length of std header */

/* Offsets into header in bytes for parameters in IRAF version 1 images */
#define IM_HDRLEN	 12		/* Length of header in 4-byte ints */
#define IM_PIXTYPE       16             /* Datatype of the pixels */
#define IM_NDIM          20             /* Number of dimensions */
#define IM_LEN           24             /* Length (as stored) */
#define IM_PHYSLEN       52             /* Physical length (as stored) */
#define IM_PIXOFF        88             /* Offset of the pixels */
#define IM_CTIME        108             /* Time of image creation */
#define IM_MTIME        112             /* Time of last modification */
#define IM_LIMTIME      116             /* Time of min,max computation */
#define IM_MAX          120             /* Maximum pixel value */
#define IM_MIN          124             /* Maximum pixel value */
#define IM_PIXFILE      412             /* Name of pixel storage file */
#define IM_HDRFILE      572             /* Name of header storage file */
#define IM_TITLE        732             /* Image name string */

/* Offsets into header in bytes for parameters in IRAF version 2 images */
#define IM2_HDRLEN	  6		/* Length of header in 4-byte ints */
#define IM2_PIXTYPE      10             /* Datatype of the pixels */
#define IM2_SWAPPED      14             /* Pixels are byte swapped */
#define IM2_NDIM         18             /* Number of dimensions */
#define IM2_LEN          22             /* Length (as stored) */
#define IM2_PHYSLEN      50             /* Physical length (as stored) */
#define IM2_PIXOFF       86             /* Offset of the pixels */
#define IM2_CTIME       106             /* Time of image creation */
#define IM2_MTIME       110             /* Time of last modification */
#define IM2_LIMTIME     114             /* Time of min,max computation */
#define IM2_MAX         118             /* Maximum pixel value */
#define IM2_MIN         122             /* Maximum pixel value */
#define IM2_PIXFILE     126             /* Name of pixel storage file */
#define IM2_HDRFILE     382             /* Name of header storage file */
#define IM2_TITLE       638             /* Image name string */

/* Codes from iraf/unix/hlib/iraf.h */
#define	TY_CHAR		2
#define	TY_SHORT	3
#define	TY_INT		4
#define	TY_LONG		5
#define	TY_REAL		6
#define	TY_DOUBLE	7
#define	TY_COMPLEX	8
#define TY_POINTER      9
#define TY_STRUCT       10
#define TY_USHORT       11
#define TY_UBYTE        12

#define LEN_PIXHDR	1024
#define MAXINT  2147483647 /* Biggest number that can fit in long */

static int isirafswapped(char *irafheader, int offset);
static int irafgeti4(char *irafheader, int offset);
static char *irafgetc2(char *irafheader, int offset, int nc);
static char *irafgetc(char *irafheader,	int offset, int	nc);
static char *iraf2str(char *irafstring, int nchar);
static char *irafrdhead(const char *filename, int *lihead);
static int irafrdimage (char **buffptr, size_t *buffsize,
    size_t *filesize, int *status);
static int iraftofits (char *hdrname, char *irafheader, int nbiraf,
    char **buffptr, size_t *nbfits, size_t *fitssize, int *status);
static char *same_path(char *pixname, const char *hdrname);

static int swaphead=0;	/* =1 to swap data bytes of IRAF header values */
static int swapdata=0;  /* =1 to swap bytes in IRAF data pixels */

static void irafswap(int bitpix, char *string, int nbytes);
static void irafswap2(char *string, int nbytes);
static void irafswap4(char *string, int nbytes);
static void irafswap8(char *string, int nbytes);
static int pix_version (char *irafheader);
static int irafncmp (char *irafheader, char *teststring, int nc);
static int machswap(void);
static int head_version (char *irafheader);
static int hgeti4(char* hstring, char* keyword, int* val);
static int hgets(char* hstring, char* keyword, int lstr, char* string);
static char* hgetc(char* hstring, char* keyword);
static char* ksearch(char* hstring, char* keyword);
static char *blsearch (char* hstring, char* keyword);	
static char *strsrch (char* s1,	char* s2);
static char *strnsrch (	char* s1,char* s2,int ls1);
static void hputi4(char* hstring,char* keyword,	int ival);
static void hputs(char* hstring,char* keyword,char* cval);
static void hputcom(char* hstring,char* keyword,char* comment);
static void hputl(char* hstring,char* keyword,int lval);
static void hputc(char* hstring,char* keyword,char* cval);
static int getirafpixname (const char *hdrname, char *irafheader, char *pixfilename, int *status);
int iraf2mem(char *filename, char **buffptr, size_t *buffsize, 
      size_t *filesize, int *status);

void ffpmsg(const char *err_message);

/* CFITS_API is defined below for use on Windows systems.  */
/* It is used to identify the public functions which should be exported. */
/* This has no effect on non-windows platforms where "WIN32" is not defined */

/* this is only needed to export the "fits_delete_iraf_file" symbol, which */
/* is called in fpackutil.c (and perhaps in other applications programs) */

#if defined (WIN32)
  #if defined(cfitsio_EXPORTS)
    #define CFITS_API __declspec(dllexport)
  #else
    #define CFITS_API //__declspec(dllimport)
  #endif /* CFITS_API */
#else /* defined (WIN32) */
 #define CFITS_API
#endif

int CFITS_API fits_delete_iraf_file(const char *filename, int *status);


/*--------------------------------------------------------------------------*/
int fits_delete_iraf_file(const char *filename,  /* name of input file      */
             int *status)                        /* IO - error status       */

/*
   Delete the iraf .imh header file and the associated .pix data file
*/
{
    char *irafheader;
    int lenirafhead;

    char pixfilename[SZ_IM2PIXFILE+1];

    /* read IRAF header into dynamically created char array (free it later!) */
    irafheader = irafrdhead(filename, &lenirafhead);

    if (!irafheader)
    {
	return(*status = FILE_NOT_OPENED);
    }

    getirafpixname (filename, irafheader, pixfilename, status);

    /* don't need the IRAF header any more */
    free(irafheader);

    if (*status > 0)
       return(*status);

    remove(filename);
    remove(pixfilename);
    
    return(*status);
}

/*--------------------------------------------------------------------------*/
int iraf2mem(char *filename,     /* name of input file                 */
             char **buffptr,     /* O - memory pointer (initially NULL)    */
             size_t *buffsize,   /* O - size of mem buffer, in bytes        */
             size_t *filesize,   /* O - size of FITS file, in bytes         */
             int *status)        /* IO - error status                       */

/*
   Driver routine that reads an IRAF image into memory, also converting
   it into FITS format.
*/
{
    char *irafheader;
    int lenirafhead;

    *buffptr = NULL;
    *buffsize = 0;
    *filesize = 0;

    /* read IRAF header into dynamically created char array (free it later!) */
    irafheader = irafrdhead(filename, &lenirafhead);

    if (!irafheader)
    {
	return(*status = FILE_NOT_OPENED);
    }

    /* convert IRAF header to FITS header in memory */
    iraftofits(filename, irafheader, lenirafhead, buffptr, buffsize, filesize,
               status);

    /* don't need the IRAF header any more */
    free(irafheader);

    if (*status > 0)
       return(*status);

    *filesize = (((*filesize - 1) / 2880 ) + 1 ) * 2880; /* multiple of 2880 */

    /* append the image data onto the FITS header */
    irafrdimage(buffptr, buffsize, filesize, status);

    return(*status);
}

/*--------------------------------------------------------------------------*/
/* Subroutine:	irafrdhead  (was irafrhead in D. Mink's original code)
 * Purpose:	Open and read the iraf .imh file.
 * Returns:	NULL if failure, else pointer to IRAF .imh image header
 * Notes:	The imhdr format is defined in iraf/lib/imhdr.h, some of
 *		which defines or mimicked, above.
 */

static char *irafrdhead (
    const char *filename,  /* Name of IRAF header file */
    int *lihead)           /* Length of IRAF image header in bytes (returned) */
{
    FILE *fd;
    int nbr;
    char *irafheader;
    char errmsg[81];
    long nbhead;
    int nihead;

    *lihead = 0;

    /* open the image header file */
    fd = fopen (filename, "rb");
    if (fd == NULL) {
        ffpmsg("unable to open IRAF header file:");
        ffpmsg(filename);
	return (NULL);
	}

    /* Find size of image header file */
    if (fseek(fd, 0, 2) != 0)  /* move to end of the file */
    {
        ffpmsg("IRAFRHEAD: cannot seek in file:");
        ffpmsg(filename);
        return(NULL);
    }

    nbhead = ftell(fd);     /* position = size of file */
    if (nbhead < 0)
    {
        ffpmsg("IRAFRHEAD: cannot get pos. in file:");
        ffpmsg(filename);
        return(NULL);
    }

    if (fseek(fd, 0, 0) != 0) /* move back to beginning */
    {
        ffpmsg("IRAFRHEAD: cannot seek to beginning of file:");
        ffpmsg(filename);
        return(NULL);
    }

    /* allocate initial sized buffer */
    nihead = nbhead + 5000;
    irafheader = (char *) calloc (1, nihead);
    if (irafheader == NULL) {
	sprintf(errmsg, "IRAFRHEAD Cannot allocate %d-byte header",
		      nihead);
        ffpmsg(errmsg);
        ffpmsg(filename);
	return (NULL);
	}
    *lihead = nihead;

    /* Read IRAF header */
    nbr = fread (irafheader, 1, nbhead, fd);
    fclose (fd);

    /* Reject if header less than minimum length */
    if (nbr < LEN_PIXHDR) {
	sprintf(errmsg, "IRAFRHEAD header file: %d / %d bytes read.",
		      nbr,LEN_PIXHDR);
        ffpmsg(errmsg);
        ffpmsg(filename);
	free (irafheader);
	return (NULL);
	}

    return (irafheader);
}
/*--------------------------------------------------------------------------*/
static int irafrdimage (
    char **buffptr,	/* FITS image header (filled) */
    size_t *buffsize,      /* allocated size of the buffer */
    size_t *filesize,      /* actual size of the FITS file */
    int *status)
{
    FILE *fd;
    char *bang;
    int nax = 1, naxis1 = 1, naxis2 = 1, naxis3 = 1, naxis4 = 1, npaxis1 = 1, npaxis2;
    int bitpix, bytepix, i;
    char *fitsheader, *image;
    int nbr, nbimage, nbaxis, nbl, nbdiff;
    char *pixheader;
    char *linebuff;
    int imhver, lpixhead = 0;
    char pixname[SZ_IM2PIXFILE+1];
    char errmsg[81];
    size_t newfilesize;
 
    fitsheader = *buffptr;           /* pointer to start of header */

    /* Convert pixel file name to character string */
    hgets (fitsheader, "PIXFILE", SZ_IM2PIXFILE, pixname);
    hgeti4 (fitsheader, "PIXOFF", &lpixhead);

    /* Open pixel file, ignoring machine name if present */
    if ((bang = strchr (pixname, '!')) != NULL )
	fd = fopen (bang + 1, "rb");
    else
	fd = fopen (pixname, "rb");

    /* Print error message and exit if pixel file is not found */
    if (!fd) {
        ffpmsg("IRAFRIMAGE: Cannot open IRAF pixel file:");
        ffpmsg(pixname);
	return (*status = FILE_NOT_OPENED);
	}

    /* Read pixel header */
    pixheader = (char *) calloc (lpixhead, 1);
    if (pixheader == NULL) {
            ffpmsg("IRAFRIMAGE: Cannot alloc memory for pixel header");
            ffpmsg(pixname);
            fclose (fd);
	    return (*status = FILE_NOT_OPENED);
	}
    nbr = fread (pixheader, 1, lpixhead, fd);

    /* Check size of pixel header */
    if (nbr < lpixhead) {
	sprintf(errmsg, "IRAF pixel file: %d / %d bytes read.",
		      nbr,LEN_PIXHDR);
        ffpmsg(errmsg);
	free (pixheader);
	fclose (fd);
	return (*status = FILE_NOT_OPENED);
	}

    /* check pixel header magic word */
    imhver = pix_version (pixheader);
    if (imhver < 1) {
        ffpmsg("File not valid IRAF pixel file:");
        ffpmsg(pixname);
	free (pixheader);
	fclose (fd);
	return (*status = FILE_NOT_OPENED);
	}
    free (pixheader);

    /* Find number of bytes to read */
    hgeti4 (fitsheader,"NAXIS",&nax);
    hgeti4 (fitsheader,"NAXIS1",&naxis1);
    hgeti4 (fitsheader,"NPAXIS1",&npaxis1);
    if (nax > 1) {
        hgeti4 (fitsheader,"NAXIS2",&naxis2);
        hgeti4 (fitsheader,"NPAXIS2",&npaxis2);
	}
    if (nax > 2)
        hgeti4 (fitsheader,"NAXIS3",&naxis3);
    if (nax > 3)
        hgeti4 (fitsheader,"NAXIS4",&naxis4);

    hgeti4 (fitsheader,"BITPIX",&bitpix);
    if (bitpix < 0)
	bytepix = -bitpix / 8;
    else
	bytepix = bitpix / 8;

    nbimage = naxis1 * naxis2 * naxis3 * naxis4 * bytepix;
    
    newfilesize = *filesize + nbimage;  /* header + data */
    newfilesize = (((newfilesize - 1) / 2880 ) + 1 ) * 2880;

    if (newfilesize > *buffsize)   /* need to allocate more memory? */
    {
      fitsheader =  (char *) realloc (*buffptr, newfilesize);
      if (fitsheader == NULL) {
	sprintf(errmsg, "IRAFRIMAGE Cannot allocate %d-byte image buffer",
		(int) (*filesize));
        ffpmsg(errmsg);
        ffpmsg(pixname);
	fclose (fd);
	return (*status = FILE_NOT_OPENED);
	}
    }

    *buffptr = fitsheader;
    *buffsize = newfilesize;

    image = fitsheader + *filesize;
    *filesize = newfilesize;

    /* Read IRAF image all at once if physical and image dimensions are the same */
    if (npaxis1 == naxis1)
	nbr = fread (image, 1, nbimage, fd);

    /* Read IRAF image one line at a time if physical and image dimensions differ */
    else {
	nbdiff = (npaxis1 - naxis1) * bytepix;
	nbaxis = naxis1 * bytepix;
	linebuff = image;
	nbr = 0;
	if (naxis2 == 1 && naxis3 > 1)
	    naxis2 = naxis3;
	for (i = 0; i < naxis2; i++) {
	    nbl = fread (linebuff, 1, nbaxis, fd);
	    nbr = nbr + nbl;
	    fseek (fd, nbdiff, 1);
	    linebuff = linebuff + nbaxis;
	    }
	}
    fclose (fd);

    /* Check size of image */
    if (nbr < nbimage) {
	sprintf(errmsg, "IRAF pixel file: %d / %d bytes read.",
		      nbr,nbimage);
        ffpmsg(errmsg);
        ffpmsg(pixname);
	return (*status = FILE_NOT_OPENED);
	}

    /* Byte-reverse image, if necessary */
    if (swapdata)
	irafswap (bitpix, image, nbimage);

    return (*status);
}
/*--------------------------------------------------------------------------*/
/* Return IRAF image format version number from magic word in IRAF header*/

static int head_version (
    char *irafheader)	/* IRAF image header from file */

{

    /* Check header file magic word */
    if (irafncmp (irafheader, "imhdr", 5) != 0 ) {
	if (strncmp (irafheader, "imhv2", 5) != 0)
	    return (0);
	else
	    return (2);
	}
    else
	return (1);
}

/*--------------------------------------------------------------------------*/
/* Return IRAF image format version number from magic word in IRAF pixel file */

static int pix_version (
    char *irafheader)   /* IRAF image header from file */
{

    /* Check pixel file header magic word */
    if (irafncmp (irafheader, "impix", 5) != 0) {
	if (strncmp (irafheader, "impv2", 5) != 0)
	    return (0);
	else
	    return (2);
	}
    else
	return (1);
}

/*--------------------------------------------------------------------------*/
/* Verify that file is valid IRAF imhdr or impix by checking first 5 chars
 * Returns:	0 on success, 1 on failure */

static int irafncmp (

char	*irafheader,	/* IRAF image header from file */
char	*teststring,	/* C character string to compare */
int	nc)		/* Number of characters to compate */

{
    char *line;

    if ((line = iraf2str (irafheader, nc)) == NULL)
	return (1);
    if (strncmp (line, teststring, nc) == 0) {
	free (line);
	return (0);
	}
    else {
	free (line);
	return (1);
	}
}
/*--------------------------------------------------------------------------*/

/* Convert IRAF image header to FITS image header, returning FITS header */

static int iraftofits (
    char    *hdrname,  /* IRAF header file name (may be path) */
    char    *irafheader,  /* IRAF image header */
    int	    nbiraf,	  /* Number of bytes in IRAF header */
    char    **buffptr,    /* pointer to the FITS header  */
    size_t  *nbfits,      /* allocated size of the FITS header buffer */
    size_t  *fitssize,  /* Number of bytes in FITS header (returned) */
                        /*  = number of bytes to the end of the END keyword */
    int     *status)
{
    char *objname;	/* object name from FITS file */
    int lstr, i, j, k, ib, nax, nbits;
    char *pixname, *newpixname, *bang, *chead;
    char *fitsheader;
    int nblock, nlines;
    char *fhead, *fhead1, *fp, endline[81];
    char irafchar;
    char fitsline[81];
    int pixtype;
    int imhver, n, imu, pixoff, impixoff;
/*    int immax, immin, imtime;  */
    int imndim, imlen, imphyslen, impixtype;
    char errmsg[81];

    /* Set up last line of FITS header */
    (void)strncpy (endline,"END", 3);
    for (i = 3; i < 80; i++)
	endline[i] = ' ';
    endline[80] = 0;

    /* Check header magic word */
    imhver = head_version (irafheader);
    if (imhver < 1) {
	ffpmsg("File not valid IRAF image header");
        ffpmsg(hdrname);
	return(*status = FILE_NOT_OPENED);
	}
    if (imhver == 2) {
	nlines = 24 + ((nbiraf - LEN_IM2HDR) / 81);
	imndim = IM2_NDIM;
	imlen = IM2_LEN;
	imphyslen = IM2_PHYSLEN;
	impixtype = IM2_PIXTYPE;
	impixoff = IM2_PIXOFF;
/*	imtime = IM2_MTIME; */
/*	immax = IM2_MAX;  */
/*	immin = IM2_MIN; */
	}
    else {
	nlines = 24 + ((nbiraf - LEN_IMHDR) / 162);
	imndim = IM_NDIM;
	imlen = IM_LEN;
	imphyslen = IM_PHYSLEN;
	impixtype = IM_PIXTYPE;
	impixoff = IM_PIXOFF;
/*	imtime = IM_MTIME; */
/*	immax = IM_MAX; */
/*	immin = IM_MIN; */
	}

    /*  Initialize FITS header */
    nblock = (nlines * 80) / 2880;
    *nbfits = (nblock + 5) * 2880 + 4;
    fitsheader = (char *) calloc (*nbfits, 1);
    if (fitsheader == NULL) {
	sprintf(errmsg, "IRAF2FITS Cannot allocate %d-byte FITS header",
		(int) (*nbfits));
        ffpmsg(hdrname);
	return (*status = FILE_NOT_OPENED);
	}

    fhead = fitsheader;
    *buffptr = fitsheader;
    (void)strncpy (fitsheader, endline, 80);
    hputl (fitsheader, "SIMPLE", 1);
    fhead = fhead + 80;

    /*  check if the IRAF file is in big endian (sun) format (= 0) or not. */
    /*  This is done by checking the 4 byte integer in the header that     */
    /*  represents the iraf pixel type.  This 4-byte word is guaranteed to */
    /*  have the least sig byte != 0 and the most sig byte = 0,  so if the */
    /*  first byte of the word != 0, then the file in little endian format */
    /*  like on an Alpha machine.                                          */

    swaphead = isirafswapped(irafheader, impixtype);
    if (imhver == 1)
        swapdata = swaphead; /* vers 1 data has same swapness as header */
    else
        swapdata = irafgeti4 (irafheader, IM2_SWAPPED); 

    /*  Set pixel size in FITS header */
    pixtype = irafgeti4 (irafheader, impixtype);
    switch (pixtype) {
	case TY_CHAR:
	    nbits = 8;
	    break;
	case TY_UBYTE:
	    nbits = 8;
	    break;
	case TY_SHORT:
	    nbits = 16;
	    break;
	case TY_USHORT:
	    nbits = -16;
	    break;
	case TY_INT:
	case TY_LONG:
	    nbits = 32;
	    break;
	case TY_REAL:
	    nbits = -32;
	    break;
	case TY_DOUBLE:
	    nbits = -64;
	    break;
	default:
	    sprintf(errmsg,"Unsupported IRAF data type: %d", pixtype);
            ffpmsg(errmsg);
            ffpmsg(hdrname);
	    return (*status = FILE_NOT_OPENED);
	}
    hputi4 (fitsheader,"BITPIX",nbits);
    hputcom (fitsheader,"BITPIX", "IRAF .imh pixel type");
    fhead = fhead + 80;

    /*  Set image dimensions in FITS header */
    nax = irafgeti4 (irafheader, imndim);
    hputi4 (fitsheader,"NAXIS",nax);
    hputcom (fitsheader,"NAXIS", "IRAF .imh naxis");
    fhead = fhead + 80;

    n = irafgeti4 (irafheader, imlen);
    hputi4 (fitsheader, "NAXIS1", n);
    hputcom (fitsheader,"NAXIS1", "IRAF .imh image naxis[1]");
    fhead = fhead + 80;

    if (nax > 1) {
	n = irafgeti4 (irafheader, imlen+4);
	hputi4 (fitsheader, "NAXIS2", n);
	hputcom (fitsheader,"NAXIS2", "IRAF .imh image naxis[2]");
        fhead = fhead + 80;
	}
    if (nax > 2) {
	n = irafgeti4 (irafheader, imlen+8);
	hputi4 (fitsheader, "NAXIS3", n);
	hputcom (fitsheader,"NAXIS3", "IRAF .imh image naxis[3]");
	fhead = fhead + 80;
	}
    if (nax > 3) {
	n = irafgeti4 (irafheader, imlen+12);
	hputi4 (fitsheader, "NAXIS4", n);
	hputcom (fitsheader,"NAXIS4", "IRAF .imh image naxis[4]");
	fhead = fhead + 80;
	}

    /* Set object name in FITS header */
    if (imhver == 2)
	objname = irafgetc (irafheader, IM2_TITLE, SZ_IM2TITLE);
    else
	objname = irafgetc2 (irafheader, IM_TITLE, SZ_IMTITLE);
    if ((lstr = strlen (objname)) < 8) {
	for (i = lstr; i < 8; i++)
	    objname[i] = ' ';
	objname[8] = 0;
	}
    hputs (fitsheader,"OBJECT",objname);
    hputcom (fitsheader,"OBJECT", "IRAF .imh title");
    free (objname);
    fhead = fhead + 80;

    /* Save physical axis lengths so image file can be read */
    n = irafgeti4 (irafheader, imphyslen);
    hputi4 (fitsheader, "NPAXIS1", n);
    hputcom (fitsheader,"NPAXIS1", "IRAF .imh physical naxis[1]");
    fhead = fhead + 80;
    if (nax > 1) {
	n = irafgeti4 (irafheader, imphyslen+4);
	hputi4 (fitsheader, "NPAXIS2", n);
	hputcom (fitsheader,"NPAXIS2", "IRAF .imh physical naxis[2]");
	fhead = fhead + 80;
	}
    if (nax > 2) {
	n = irafgeti4 (irafheader, imphyslen+8);
	hputi4 (fitsheader, "NPAXIS3", n);
	hputcom (fitsheader,"NPAXIS3", "IRAF .imh physical naxis[3]");
	fhead = fhead + 80;
	}
    if (nax > 3) {
	n = irafgeti4 (irafheader, imphyslen+12);
	hputi4 (fitsheader, "NPAXIS4", n);
	hputcom (fitsheader,"NPAXIS4", "IRAF .imh physical naxis[4]");
	fhead = fhead + 80;
	}

    /* Save image header filename in header */
    hputs (fitsheader,"IMHFILE",hdrname);
    hputcom (fitsheader,"IMHFILE", "IRAF header file name");
    fhead = fhead + 80;

    /* Save image pixel file pathname in header */
    if (imhver == 2)
	pixname = irafgetc (irafheader, IM2_PIXFILE, SZ_IM2PIXFILE);
    else
	pixname = irafgetc2 (irafheader, IM_PIXFILE, SZ_IMPIXFILE);
    if (strncmp(pixname, "HDR", 3) == 0 ) {
	newpixname = same_path (pixname, hdrname);
        if (newpixname) {
          free (pixname);
          pixname = newpixname;
	  }
	}
    if (strchr (pixname, '/') == NULL && strchr (pixname, '$') == NULL) {
	newpixname = same_path (pixname, hdrname);
        if (newpixname) {
          free (pixname);
          pixname = newpixname;
	  }
	}
	
    if ((bang = strchr (pixname, '!')) != NULL )
	hputs (fitsheader,"PIXFILE",bang+1);
    else
	hputs (fitsheader,"PIXFILE",pixname);
    free (pixname);
    hputcom (fitsheader,"PIXFILE", "IRAF .pix pixel file");
    fhead = fhead + 80;

    /* Save image offset from star of pixel file */
    pixoff = irafgeti4 (irafheader, impixoff);
    pixoff = (pixoff - 1) * 2;
    hputi4 (fitsheader, "PIXOFF", pixoff);
    hputcom (fitsheader,"PIXOFF", "IRAF .pix pixel offset (Do not change!)");
    fhead = fhead + 80;

    /* Save IRAF file format version in header */
    hputi4 (fitsheader,"IMHVER",imhver);
    hputcom (fitsheader,"IMHVER", "IRAF .imh format version (1 or 2)");
    fhead = fhead + 80;

    /* Save flag as to whether to swap IRAF data for this file and machine */
    if (swapdata)
	hputl (fitsheader, "PIXSWAP", 1);
    else
	hputl (fitsheader, "PIXSWAP", 0);
    hputcom (fitsheader,"PIXSWAP", "IRAF pixels, FITS byte orders differ if T");
    fhead = fhead + 80;

    /* Add user portion of IRAF header to FITS header */
    fitsline[80] = 0;
    if (imhver == 2) {
	imu = LEN_IM2HDR;
	chead = irafheader;
	j = 0;
	for (k = 0; k < 80; k++)
	    fitsline[k] = ' ';
	for (i = imu; i < nbiraf; i++) {
	    irafchar = chead[i];
	    if (irafchar == 0)
		break;
	    else if (irafchar == 10) {
		(void)strncpy (fhead, fitsline, 80);
		/* fprintf (stderr,"%80s\n",fitsline); */
		if (strncmp (fitsline, "OBJECT ", 7) != 0) {
		    fhead = fhead + 80;
		    }
		for (k = 0; k < 80; k++)
		    fitsline[k] = ' ';
		j = 0;
		}
	    else {
		if (j > 80) {
		    if (strncmp (fitsline, "OBJECT ", 7) != 0) {
			(void)strncpy (fhead, fitsline, 80);
			/* fprintf (stderr,"%80s\n",fitsline); */
			j = 9;
			fhead = fhead + 80;
			}
		    for (k = 0; k < 80; k++)
			fitsline[k] = ' ';
		    }
		if (irafchar > 32 && irafchar < 127)
		    fitsline[j] = irafchar;
		j++;
		}
	    }
	}
    else {
	imu = LEN_IMHDR;
	chead = irafheader;
	if (swaphead == 1)
	    ib = 0;
	else
	    ib = 1;
	for (k = 0; k < 80; k++)
	    fitsline[k] = ' ';
	j = 0;
	for (i = imu; i < nbiraf; i=i+2) {
	    irafchar = chead[i+ib];
	    if (irafchar == 0)
		break;
	    else if (irafchar == 10) {
		if (strncmp (fitsline, "OBJECT ", 7) != 0) {
		    (void)strncpy (fhead, fitsline, 80);
		    fhead = fhead + 80;
		    }
		/* fprintf (stderr,"%80s\n",fitsline); */
		j = 0;
		for (k = 0; k < 80; k++)
		    fitsline[k] = ' ';
		}
	    else {
		if (j > 80) {
		    if (strncmp (fitsline, "OBJECT ", 7) != 0) {
			(void)strncpy (fhead, fitsline, 80);
			j = 9;
			fhead = fhead + 80;
			}
		    /* fprintf (stderr,"%80s\n",fitsline); */
		    for (k = 0; k < 80; k++)
			fitsline[k] = ' ';
		    }
		if (irafchar > 32 && irafchar < 127)
		    fitsline[j] = irafchar;
		j++;
		}
	    }
	}

    /* Add END to last line */
    (void)strncpy (fhead, endline, 80);

    /* Find end of last 2880-byte block of header */
    fhead = ksearch (fitsheader, "END") + 80;
    nblock = *nbfits / 2880;
    fhead1 = fitsheader + (nblock * 2880);
    *fitssize = fhead - fitsheader;  /* no. of bytes to end of END keyword */

    /* Pad rest of header with spaces */
    strncpy (endline,"   ",3);
    for (fp = fhead; fp < fhead1; fp = fp + 80) {
	(void)strncpy (fp, endline,80);
	}

    return (*status);
}
/*--------------------------------------------------------------------------*/

/* get the IRAF pixel file name */

static int getirafpixname (
    const char *hdrname,  /* IRAF header file name (may be path) */
    char    *irafheader,  /* IRAF image header */
    char    *pixfilename,     /* IRAF pixel file name */
    int     *status)
{
    int imhver;
    char *pixname, *newpixname, *bang;

    /* Check header magic word */
    imhver = head_version (irafheader);
    if (imhver < 1) {
	ffpmsg("File not valid IRAF image header");
        ffpmsg(hdrname);
	return(*status = FILE_NOT_OPENED);
	}

    /* get image pixel file pathname in header */
    if (imhver == 2)
	pixname = irafgetc (irafheader, IM2_PIXFILE, SZ_IM2PIXFILE);
    else
	pixname = irafgetc2 (irafheader, IM_PIXFILE, SZ_IMPIXFILE);

    if (strncmp(pixname, "HDR", 3) == 0 ) {
	newpixname = same_path (pixname, hdrname);
        if (newpixname) {
          free (pixname);
          pixname = newpixname;
	  }
	}

    if (strchr (pixname, '/') == NULL && strchr (pixname, '$') == NULL) {
	newpixname = same_path (pixname, hdrname);
        if (newpixname) {
          free (pixname);
          pixname = newpixname;
	  }
	}
	
    if ((bang = strchr (pixname, '!')) != NULL )
	strcpy(pixfilename,bang+1);
    else
	strcpy(pixfilename,pixname);

    free (pixname);

    return (*status);
}

/*--------------------------------------------------------------------------*/
/* Put filename and header path together */

static char *same_path (

char	*pixname,	/* IRAF pixel file pathname */
const char	*hdrname)	/* IRAF image header file pathname */

{
    int len;
    char *newpixname;

/*  WDP - 10/16/2007 - increased allocation to avoid possible overflow */
/*    newpixname = (char *) calloc (SZ_IM2PIXFILE, sizeof (char)); */

    newpixname = (char *) calloc (2*SZ_IM2PIXFILE+1, sizeof (char));
    if (newpixname == NULL) {
            ffpmsg("iraffits same_path: Cannot alloc memory for newpixname");
	    return (NULL);
	}

    /* Pixel file is in same directory as header */
    if (strncmp(pixname, "HDR$", 4) == 0 ) {
	(void)strncpy (newpixname, hdrname, SZ_IM2PIXFILE);

	/* find the end of the pathname */
	len = strlen (newpixname);
#ifndef VMS
	while( (len > 0) && (newpixname[len-1] != '/') )
#else
	while( (len > 0) && (newpixname[len-1] != ']') && (newpixname[len-1] != ':') )
#endif
	    len--;

	/* add name */
	newpixname[len] = '\0';
	(void)strncat (newpixname, &pixname[4], SZ_IM2PIXFILE);
	}

    /* Bare pixel file with no path is assumed to be same as HDR$filename */
    else if (strchr (pixname, '/') == NULL && strchr (pixname, '$') == NULL) {
	(void)strncpy (newpixname, hdrname, SZ_IM2PIXFILE);

	/* find the end of the pathname */
	len = strlen (newpixname);
#ifndef VMS
	while( (len > 0) && (newpixname[len-1] != '/') )
#else
	while( (len > 0) && (newpixname[len-1] != ']') && (newpixname[len-1] != ':') )
#endif
	    len--;

	/* add name */
	newpixname[len] = '\0';
	(void)strncat (newpixname, pixname, SZ_IM2PIXFILE);
	}

    /* Pixel file has same name as header file, but with .pix extension */
    else if (strncmp (pixname, "HDR", 3) == 0) {

	/* load entire header name string into name buffer */
	(void)strncpy (newpixname, hdrname, SZ_IM2PIXFILE);
	len = strlen (newpixname);
	newpixname[len-3] = 'p';
	newpixname[len-2] = 'i';
	newpixname[len-1] = 'x';
	}

    return (newpixname);
}

/*--------------------------------------------------------------------------*/
static int isirafswapped (

char	*irafheader,	/* IRAF image header */
int	offset)		/* Number of bytes to skip before number */

    /*  check if the IRAF file is in big endian (sun) format (= 0) or not */
    /*  This is done by checking the 4 byte integer in the header that */
    /*  represents the iraf pixel type.  This 4-byte word is guaranteed to */
    /*  have the least sig byte != 0 and the most sig byte = 0,  so if the */
    /*  first byte of the word != 0, then the file in little endian format */
    /*  like on an Alpha machine.                                          */

{
    int  swapped;

    if (irafheader[offset] != 0)
	swapped = 1;
    else
	swapped = 0;

    return (swapped);
}
/*--------------------------------------------------------------------------*/
static int irafgeti4 (

char	*irafheader,	/* IRAF image header */
int	offset)		/* Number of bytes to skip before number */

{
    char *ctemp, *cheader;
    int  temp;

    cheader = irafheader;
    ctemp = (char *) &temp;

    if (machswap() != swaphead) {
	ctemp[3] = cheader[offset];
	ctemp[2] = cheader[offset+1];
	ctemp[1] = cheader[offset+2];
	ctemp[0] = cheader[offset+3];
	}
    else {
	ctemp[0] = cheader[offset];
	ctemp[1] = cheader[offset+1];
	ctemp[2] = cheader[offset+2];
	ctemp[3] = cheader[offset+3];
	}
    return (temp);
}

/*--------------------------------------------------------------------------*/
/* IRAFGETC2 -- Get character string from arbitrary part of v.1 IRAF header */

static char *irafgetc2 (

char	*irafheader,	/* IRAF image header */
int	offset,		/* Number of bytes to skip before string */
int	nc)		/* Maximum number of characters in string */

{
    char *irafstring, *string;

    irafstring = irafgetc (irafheader, offset, 2*(nc+1));
    string = iraf2str (irafstring, nc);
    free (irafstring);

    return (string);
}

/*--------------------------------------------------------------------------*/
/* IRAFGETC -- Get character string from arbitrary part of IRAF header */

static char *irafgetc (

char	*irafheader,	/* IRAF image header */
int	offset,		/* Number of bytes to skip before string */
int	nc)		/* Maximum number of characters in string */

{
    char *ctemp, *cheader;
    int i;

    cheader = irafheader;
    ctemp = (char *) calloc (nc+1, 1);
    if (ctemp == NULL) {
	ffpmsg("IRAFGETC Cannot allocate memory for string variable");
	return (NULL);
	}
    for (i = 0; i < nc; i++) {
	ctemp[i] = cheader[offset+i];
	if (ctemp[i] > 0 && ctemp[i] < 32)
	    ctemp[i] = ' ';
	}

    return (ctemp);
}

/*--------------------------------------------------------------------------*/
/* Convert IRAF 2-byte/char string to 1-byte/char string */

static char *iraf2str (

char	*irafstring,	/* IRAF 2-byte/character string */
int	nchar)		/* Number of characters in string */
{
    char *string;
    int i, j;

    string = (char *) calloc (nchar+1, 1);
    if (string == NULL) {
	ffpmsg("IRAF2STR Cannot allocate memory for string variable");
	return (NULL);
	}

    /* the chars are in bytes 1, 3, 5, ... if bigendian format (SUN) */
    /* else in bytes 0, 2, 4, ... if little endian format (Alpha)    */

    if (irafstring[0] != 0)
	j = 0;
    else
	j = 1;

    /* Convert appropriate byte of input to output character */
    for (i = 0; i < nchar; i++) {
	string[i] = irafstring[j];
	j = j + 2;
	}

    return (string);
}

/*--------------------------------------------------------------------------*/
/* IRAFSWAP -- Reverse bytes of any type of vector in place */

static void irafswap (

int	bitpix,		/* Number of bits per pixel */
			/*  16 = short, -16 = unsigned short, 32 = int */
			/* -32 = float, -64 = double */
char	*string,	/* Address of starting point of bytes to swap */
int	nbytes)		/* Number of bytes to swap */

{
    switch (bitpix) {

	case 16:
	    if (nbytes < 2) return;
	    irafswap2 (string,nbytes);
	    break;

	case 32:
	    if (nbytes < 4) return;
	    irafswap4 (string,nbytes);
	    break;

	case -16:
	    if (nbytes < 2) return;
	    irafswap2 (string,nbytes);
	    break;

	case -32:
	    if (nbytes < 4) return;
	    irafswap4 (string,nbytes);
	    break;

	case -64:
	    if (nbytes < 8) return;
	    irafswap8 (string,nbytes);
	    break;

	}
    return;
}

/*--------------------------------------------------------------------------*/
/* IRAFSWAP2 -- Swap bytes in string in place */

static void irafswap2 (

char *string,	/* Address of starting point of bytes to swap */
int nbytes)	/* Number of bytes to swap */

{
    char *sbyte, temp, *slast;

    slast = string + nbytes;
    sbyte = string;
    while (sbyte < slast) {
	temp = sbyte[0];
	sbyte[0] = sbyte[1];
	sbyte[1] = temp;
	sbyte= sbyte + 2;
	}
    return;
}

/*--------------------------------------------------------------------------*/
/* IRAFSWAP4 -- Reverse bytes of Integer*4 or Real*4 vector in place */

static void irafswap4 (

char *string,	/* Address of Integer*4 or Real*4 vector */
int nbytes)	/* Number of bytes to reverse */

{
    char *sbyte, *slast;
    char temp0, temp1, temp2, temp3;

    slast = string + nbytes;
    sbyte = string;
    while (sbyte < slast) {
	temp3 = sbyte[0];
	temp2 = sbyte[1];
	temp1 = sbyte[2];
	temp0 = sbyte[3];
	sbyte[0] = temp0;
	sbyte[1] = temp1;
	sbyte[2] = temp2;
	sbyte[3] = temp3;
	sbyte = sbyte + 4;
	}

    return;
}

/*--------------------------------------------------------------------------*/
/* IRAFSWAP8 -- Reverse bytes of Real*8 vector in place */

static void irafswap8 (

char *string,	/* Address of Real*8 vector */
int nbytes)	/* Number of bytes to reverse */

{
    char *sbyte, *slast;
    char temp[8];

    slast = string + nbytes;
    sbyte = string;
    while (sbyte < slast) {
	temp[7] = sbyte[0];
	temp[6] = sbyte[1];
	temp[5] = sbyte[2];
	temp[4] = sbyte[3];
	temp[3] = sbyte[4];
	temp[2] = sbyte[5];
	temp[1] = sbyte[6];
	temp[0] = sbyte[7];
	sbyte[0] = temp[0];
	sbyte[1] = temp[1];
	sbyte[2] = temp[2];
	sbyte[3] = temp[3];
	sbyte[4] = temp[4];
	sbyte[5] = temp[5];
	sbyte[6] = temp[6];
	sbyte[7] = temp[7];
	sbyte = sbyte + 8;
	}
    return;
}

/*--------------------------------------------------------------------------*/
static int
machswap (void)

{
    char *ctest;
    int itest;

    itest = 1;
    ctest = (char *)&itest;
    if (*ctest)
	return (1);
    else
	return (0);
}

/*--------------------------------------------------------------------------*/
/*             the following routines were originally in hget.c             */
/*--------------------------------------------------------------------------*/


static int lhead0 = 0;

/*--------------------------------------------------------------------------*/

/* Extract long value for variable from FITS header string */

static int
hgeti4 (hstring,keyword,ival)

char *hstring;	/* character string containing FITS header information
		   in the format <keyword>= <value> {/ <comment>} */
char *keyword;	/* character string containing the name of the keyword
		   the value of which is returned.  hget searches for a
		   line beginning with this string.  if "[n]" is present,
		   the n'th token in the value is returned.
		   (the first 8 characters must be unique) */
int *ival;
{
char *value;
double dval;
int minint;
char val[30]; 

/* Get value and comment from header string */
	value = hgetc (hstring,keyword);

/* Translate value from ASCII to binary */
	if (value != NULL) {
	    minint = -MAXINT - 1;
	    strcpy (val, value);
	    dval = atof (val);
	    if (dval+0.001 > MAXINT)
		*ival = MAXINT;
	    else if (dval >= 0)
		*ival = (int) (dval + 0.001);
	    else if (dval-0.001 < minint)
		*ival = minint;
	    else
		*ival = (int) (dval - 0.001);
	    return (1);
	    }
	else {
	    return (0);
	    }
}

/*-------------------------------------------------------------------*/
/* Extract string value for variable from FITS header string */

static int
hgets (hstring, keyword, lstr, str)

char *hstring;	/* character string containing FITS header information
		   in the format <keyword>= <value> {/ <comment>} */
char *keyword;	/* character string containing the name of the keyword
		   the value of which is returned.  hget searches for a
		   line beginning with this string.  if "[n]" is present,
		   the n'th token in the value is returned.
		   (the first 8 characters must be unique) */
int lstr;	/* Size of str in characters */
char *str;	/* String (returned) */
{
	char *value;
	int lval;

/* Get value and comment from header string */
	value = hgetc (hstring,keyword);

	if (value != NULL) {
	    lval = strlen (value);
	    if (lval < lstr)
		strcpy (str, value);
	    else if (lstr > 1)
		strncpy (str, value, lstr-1);
	    else
		str[0] = value[0];
	    return (1);
	    }
	else
	    return (0);
}

/*-------------------------------------------------------------------*/
/* Extract character value for variable from FITS header string */

static char *
hgetc (hstring,keyword0)

char *hstring;	/* character string containing FITS header information
		   in the format <keyword>= <value> {/ <comment>} */
char *keyword0;	/* character string containing the name of the keyword
		   the value of which is returned.  hget searches for a
		   line beginning with this string.  if "[n]" is present,
		   the n'th token in the value is returned.
		   (the first 8 characters must be unique) */
{
	static char cval[80];
	char *value;
	char cwhite[2];
	char squot[2], dquot[2], lbracket[2], rbracket[2], slash[2], comma[2];
	char keyword[81]; /* large for ESO hierarchical keywords */
	char line[100];
	char *vpos, *cpar = NULL;
	char *q1, *q2 = NULL, *v1, *v2, *c1, *brack1, *brack2;
        char *saveptr;
	int ipar, i;

	squot[0] = 39;
	squot[1] = 0;
	dquot[0] = 34;
	dquot[1] = 0;
	lbracket[0] = 91;
	lbracket[1] = 0;
	comma[0] = 44;
	comma[1] = 0;
	rbracket[0] = 93;
	rbracket[1] = 0;
	slash[0] = 47;
	slash[1] = 0;

/* Find length of variable name */
	strncpy (keyword,keyword0, sizeof(keyword)-1);
	brack1 = strsrch (keyword,lbracket);
	if (brack1 == NULL)
	    brack1 = strsrch (keyword,comma);
	if (brack1 != NULL) {
	    *brack1 = '\0';
	    brack1++;
	    }

/* Search header string for variable name */
	vpos = ksearch (hstring,keyword);

/* Exit if not found */
	if (vpos == NULL) {
	    return (NULL);
	    }

/* Initialize line to nulls */
	 for (i = 0; i < 100; i++)
	    line[i] = 0;

/* In standard FITS, data lasts until 80th character */

/* Extract entry for this variable from the header */
	strncpy (line,vpos,80);

/* check for quoted value */
	q1 = strsrch (line,squot);
	c1 = strsrch (line,slash);
	if (q1 != NULL) {
	    if (c1 != NULL && q1 < c1)
		q2 = strsrch (q1+1,squot);
	    else if (c1 == NULL)
		q2 = strsrch (q1+1,squot);
	    else
		q1 = NULL;
	    }
	else {
	    q1 = strsrch (line,dquot);
	    if (q1 != NULL) {
		if (c1 != NULL && q1 < c1)
		    q2 = strsrch (q1+1,dquot);
		else if (c1 == NULL)
		    q2 = strsrch (q1+1,dquot);
		else
		    q1 = NULL;
		}
	    else {
		q1 = NULL;
		q2 = line + 10;
		}
	    }

/* Extract value and remove excess spaces */
	if (q1 != NULL) {
	    v1 = q1 + 1;
	    v2 = q2;
	    c1 = strsrch (q2,"/");
	    }
	else {
	    v1 = strsrch (line,"=") + 1;
	    c1 = strsrch (line,"/");
	    if (c1 != NULL)
		v2 = c1;
	    else
		v2 = line + 79;
	    }

/* Ignore leading spaces */
	while (*v1 == ' ' && v1 < v2) {
	    v1++;
	    }

/* Drop trailing spaces */
	*v2 = '\0';
	v2--;
	while (*v2 == ' ' && v2 > v1) {
	    *v2 = '\0';
	    v2--;
	    }

	if (!strcmp (v1, "-0"))
	    v1++;
	strcpy (cval,v1);
	value = cval;

/* If keyword has brackets, extract appropriate token from value */
	if (brack1 != NULL) {
	    brack2 = strsrch (brack1,rbracket);
	    if (brack2 != NULL)
		*brack2 = '\0';
	    ipar = atoi (brack1);
	    if (ipar > 0) {
		cwhite[0] = ' ';
		cwhite[1] = '\0';
		for (i = 1; i <= ipar; i++) {
		    cpar = ffstrtok (v1,cwhite,&saveptr);
		    v1 = NULL;
		    }
		if (cpar != NULL) {
		    strcpy (cval,cpar);
		    }
		else
		    value = NULL;
		}
	    }

	return (value);
}


/*-------------------------------------------------------------------*/
/* Find beginning of fillable blank line before FITS header keyword line */

static char *
blsearch (hstring,keyword)

/* Find entry for keyword keyword in FITS header string hstring.
   (the keyword may have a maximum of eight letters)
   NULL is returned if the keyword is not found */

char *hstring;	/* character string containing fits-style header
		information in the format <keyword>= <value> {/ <comment>}
		the default is that each entry is 80 characters long;
		however, lines may be of arbitrary length terminated by
		nulls, carriage returns or linefeeds, if packed is true.  */
char *keyword;	/* character string containing the name of the variable
		to be returned.  ksearch searches for a line beginning
		with this string.  The string may be a character
		literal or a character variable terminated by a null
		or '$'.  it is truncated to 8 characters. */
{
    char *loc, *headnext, *headlast, *pval, *lc, *line;
    char *bval;
    int icol, nextchar, lkey, nleft, lhstr;

    pval = 0;

    /* Search header string for variable name */
    if (lhead0)
	lhstr = lhead0;
    else {
	lhstr = 0;
	while (lhstr < 57600 && hstring[lhstr] != 0)
	    lhstr++;
	}
    headlast = hstring + lhstr;
    headnext = hstring;
    pval = NULL;
    while (headnext < headlast) {
	nleft = headlast - headnext;
	loc = strnsrch (headnext, keyword, nleft);

	/* Exit if keyword is not found */
	if (loc == NULL) {
	    break;
	    }

	icol = (loc - hstring) % 80;
	lkey = strlen (keyword);
	nextchar = (int) *(loc + lkey);

	/* If this is not in the first 8 characters of a line, keep searching */
	if (icol > 7)
	    headnext = loc + 1;

	/* If parameter name in header is longer, keep searching */
	else if (nextchar != 61 && nextchar > 32 && nextchar < 127)
	    headnext = loc + 1;

	/* If preceeding characters in line are not blanks, keep searching */
	else {
	    line = loc - icol;
	    for (lc = line; lc < loc; lc++) {
		if (*lc != ' ')
		    headnext = loc + 1;
		}

	/* Return pointer to start of line if match */
	    if (loc >= headnext) {
		pval = line;
		break;
		}
	    }
	}

    /* Return NULL if keyword is found at start of FITS header string */
    if (pval == NULL)
	return (pval);

    /* Return NULL if  found the first keyword in the header */
    if (pval == hstring)
        return (NULL);

    /* Find last nonblank line before requested keyword */
    bval = pval - 80;
    while (!strncmp (bval,"        ",8))
	bval = bval - 80;
    bval = bval + 80;

    /* Return pointer to calling program if blank lines found */
    if (bval < pval)
	return (bval);
    else
	return (NULL);
}


/*-------------------------------------------------------------------*/
/* Find FITS header line containing specified keyword */

static char *ksearch (hstring,keyword)

/* Find entry for keyword keyword in FITS header string hstring.
   (the keyword may have a maximum of eight letters)
   NULL is returned if the keyword is not found */

char *hstring;	/* character string containing fits-style header
		information in the format <keyword>= <value> {/ <comment>}
		the default is that each entry is 80 characters long;
		however, lines may be of arbitrary length terminated by
		nulls, carriage returns or linefeeds, if packed is true.  */
char *keyword;	/* character string containing the name of the variable
		to be returned.  ksearch searches for a line beginning
		with this string.  The string may be a character
		literal or a character variable terminated by a null
		or '$'.  it is truncated to 8 characters. */
{
    char *loc, *headnext, *headlast, *pval, *lc, *line;
    int icol, nextchar, lkey, nleft, lhstr;

    pval = 0;

/* Search header string for variable name */
    if (lhead0)
	lhstr = lhead0;
    else {
	lhstr = 0;
	while (lhstr < 57600 && hstring[lhstr] != 0)
	    lhstr++;
	}
    headlast = hstring + lhstr;
    headnext = hstring;
    pval = NULL;
    while (headnext < headlast) {
	nleft = headlast - headnext;
	loc = strnsrch (headnext, keyword, nleft);

	/* Exit if keyword is not found */
	if (loc == NULL) {
	    break;
	    }

	icol = (loc - hstring) % 80;
	lkey = strlen (keyword);
	nextchar = (int) *(loc + lkey);

	/* If this is not in the first 8 characters of a line, keep searching */
	if (icol > 7)
	    headnext = loc + 1;

	/* If parameter name in header is longer, keep searching */
	else if (nextchar != 61 && nextchar > 32 && nextchar < 127)
	    headnext = loc + 1;

	/* If preceeding characters in line are not blanks, keep searching */
	else {
	    line = loc - icol;
	    for (lc = line; lc < loc; lc++) {
		if (*lc != ' ')
		    headnext = loc + 1;
		}

	/* Return pointer to start of line if match */
	    if (loc >= headnext) {
		pval = line;
		break;
		}
	    }
	}

/* Return pointer to calling program */
	return (pval);

}

/*-------------------------------------------------------------------*/
/* Find string s2 within null-terminated string s1 */

static char *
strsrch (s1, s2)

char *s1;	/* String to search */
char *s2;	/* String to look for */

{
    int ls1;
    ls1 = strlen (s1);
    return (strnsrch (s1, s2, ls1));
}

/*-------------------------------------------------------------------*/
/* Find string s2 within string s1 */

static char *
strnsrch (s1, s2, ls1)

char	*s1;	/* String to search */
char	*s2;	/* String to look for */
int	ls1;	/* Length of string being searched */

{
    char *s,*s1e;
    char cfirst,clast;
    int i,ls2;

    /* Return null string if either pointer is NULL */
    if (s1 == NULL || s2 == NULL)
	return (NULL);

    /* A zero-length pattern is found in any string */
    ls2 = strlen (s2);
    if (ls2 ==0)
	return (s1);

    /* Only a zero-length string can be found in a zero-length string */
    if (ls1 ==0)
	return (NULL);

    cfirst = s2[0];
    clast = s2[ls2-1];
    s1e = s1 + ls1 - ls2 + 1;
    s = s1;
    while (s < s1e) { 

	/* Search for first character in pattern string */
	if (*s == cfirst) {

	    /* If single character search, return */
	    if (ls2 == 1)
		return (s);

	    /* Search for last character in pattern string if first found */
	    if (s[ls2-1] == clast) {

		/* If two-character search, return */
		if (ls2 == 2)
		    return (s);

		/* If 3 or more characters, check for rest of search string */
		i = 1;
		while (i < ls2 && s[i] == s2[i])
		    i++;

		/* If entire string matches, return */
		if (i >= ls2)
		    return (s);
		}
	    }
	s++;
	}
    return (NULL);
}

/*-------------------------------------------------------------------*/
/*             the following routines were originally in hget.c      */
/*-------------------------------------------------------------------*/
/*  HPUTI4 - Set int keyword = ival in FITS header string */

static void
hputi4 (hstring,keyword,ival)

  char *hstring;	/* character string containing FITS-style header
			   information in the format
			   <keyword>= <value> {/ <comment>}
			   each entry is padded with spaces to 80 characters */

  char *keyword;		/* character string containing the name of the variable
			   to be returned.  hput searches for a line beginning
			   with this string, and if there isn't one, creates one.
		   	   The first 8 characters of keyword must be unique. */
  int ival;		/* int number */
{
    char value[30];

    /* Translate value from binary to ASCII */
    sprintf (value,"%d",ival);

    /* Put value into header string */
    hputc (hstring,keyword,value);

    /* Return to calling program */
    return;
}

/*-------------------------------------------------------------------*/

/*  HPUTL - Set keyword = F if lval=0, else T, in FITS header string */

static void
hputl (hstring, keyword,lval)

char *hstring;		/* FITS header */
char *keyword;		/* Keyword name */
int lval;		/* logical variable (0=false, else true) */
{
    char value[8];

    /* Translate value from binary to ASCII */
    if (lval)
	strcpy (value, "T");
    else
	strcpy (value, "F");

    /* Put value into header string */
    hputc (hstring,keyword,value);

    /* Return to calling program */
    return;
}

/*-------------------------------------------------------------------*/

/*  HPUTS - Set character string keyword = 'cval' in FITS header string */

static void
hputs (hstring,keyword,cval)

char *hstring;	/* FITS header */
char *keyword;	/* Keyword name */
char *cval;	/* character string containing the value for variable
		   keyword.  trailing and leading blanks are removed.  */
{
    char squot = 39;
    char value[70];
    int lcval;

    /*  find length of variable string */

    lcval = strlen (cval);
    if (lcval > 67)
	lcval = 67;

    /* Put quotes around string */
    value[0] = squot;
    strncpy (&value[1],cval,lcval);
    value[lcval+1] = squot;
    value[lcval+2] = 0;

    /* Put value into header string */
    hputc (hstring,keyword,value);

    /* Return to calling program */
    return;
}

/*---------------------------------------------------------------------*/
/*  HPUTC - Set character string keyword = value in FITS header string */

static void
hputc (hstring,keyword,value)

char *hstring;
char *keyword;
char *value;	/* character string containing the value for variable
		   keyword.  trailing and leading blanks are removed.  */
{
    char squot = 39;
    char line[100];
    char newcom[50];
    char blank[80];
    char *v, *vp, *v1, *v2, *q1, *q2, *c1, *ve;
    int lkeyword, lcom, lval, lc, i;

    for (i = 0; i < 80; i++)
	blank[i] = ' ';

    /*  find length of keyword and value */
    lkeyword = strlen (keyword);
    lval = strlen (value);

    /*  If COMMENT or HISTORY, always add it just before the END */
    if (lkeyword == 7 && (strncmp (keyword,"COMMENT",7) == 0 ||
	strncmp (keyword,"HISTORY",7) == 0)) {

	/* Find end of header */
	v1 = ksearch (hstring,"END");
	v2 = v1 + 80;

	/* Move END down one line */
	strncpy (v2, v1, 80);

	/* Insert keyword */
	strncpy (v1,keyword,7);

	/* Pad with spaces */
	for (vp = v1+lkeyword; vp < v2; vp++)
	    *vp = ' ';

	/* Insert comment */
	strncpy (v1+9,value,lval);
	return;
	}

    /* Otherwise search for keyword */
    else
	v1 = ksearch (hstring,keyword);

    /*  If parameter is not found, find a place to put it */
    if (v1 == NULL) {
	
	/* First look for blank lines before END */
        v1 = blsearch (hstring, "END");
    
	/*  Otherwise, create a space for it at the end of the header */
	if (v1 == NULL) {
	    ve = ksearch (hstring,"END");
	    v1 = ve;
	    v2 = v1 + 80;
	    strncpy (v2, ve, 80);
	    }
	else
	    v2 = v1 + 80;
	lcom = 0;
	newcom[0] = 0;
	}

    /*  Otherwise, extract the entry for this keyword from the header */
    else {
	strncpy (line, v1, 80);
	line[80] = 0;
	v2 = v1 + 80;

	/*  check for quoted value */
	q1 = strchr (line, squot);
	if (q1 != NULL)
	    q2 = strchr (q1+1,squot);
	else
	    q2 = line;

	/*  extract comment and remove trailing spaces */

	c1 = strchr (q2,'/');
	if (c1 != NULL) {
	    lcom = 80 - (c1 - line);
	    strncpy (newcom, c1+1, lcom);
	    vp = newcom + lcom - 1;
	    while (vp-- > newcom && *vp == ' ')
		*vp = 0;
	    lcom = strlen (newcom);
	    }
	else {
	    newcom[0] = 0;
	    lcom = 0;
	    }
	}

    /* Fill new entry with spaces */
    for (vp = v1; vp < v2; vp++)
	*vp = ' ';

    /*  Copy keyword to new entry */
    strncpy (v1, keyword, lkeyword);

    /*  Add parameter value in the appropriate place */
    vp = v1 + 8;
    *vp = '=';
    vp = v1 + 9;
    *vp = ' ';
    vp = vp + 1;
    if (*value == squot) {
	strncpy (vp, value, lval);
	if (lval+12 > 31)
	    lc = lval + 12;
	else
	    lc = 30;
	}
    else {
	vp = v1 + 30 - lval;
	strncpy (vp, value, lval);
	lc = 30;
	}

    /* Add comment in the appropriate place */
	if (lcom > 0) {
	    if (lc+2+lcom > 80)
		lcom = 78 - lc;
	    vp = v1 + lc + 2;     /* Jul 16 1997: was vp = v1 + lc * 2 */
	    *vp = '/';
	    vp = vp + 1;
	    strncpy (vp, newcom, lcom);
	    for (v = vp + lcom; v < v2; v++)
		*v = ' ';
	    }

	return;
}

/*-------------------------------------------------------------------*/
/*  HPUTCOM - Set comment for keyword or on line in FITS header string */

static void
hputcom (hstring,keyword,comment)

  char *hstring;
  char *keyword;
  char *comment;
{
	char squot;
	char line[100];
	int lkeyword, lcom;
	char *vp, *v1, *v2, *c0 = NULL, *c1, *q1, *q2;

	squot = 39;

/*  Find length of variable name */
	lkeyword = strlen (keyword);

/*  If COMMENT or HISTORY, always add it just before the END */
	if (lkeyword == 7 && (strncmp (keyword,"COMMENT",7) == 0 ||
	    strncmp (keyword,"HISTORY",7) == 0)) {

	/* Find end of header */
	    v1 = ksearch (hstring,"END");
	    v2 = v1 + 80;
	    strncpy (v2, v1, 80);

	/*  blank out new line and insert keyword */
	    for (vp = v1; vp < v2; vp++)
		*vp = ' ';
	    strncpy (v1, keyword, lkeyword);
	    }

/* search header string for variable name */
	else {
	    v1 = ksearch (hstring,keyword);
	    v2 = v1 + 80;

	/* if parameter is not found, return without doing anything */
	    if (v1 == NULL) {
		return;
		}

	/* otherwise, extract entry for this variable from the header */
	    strncpy (line, v1, 80);

	/* check for quoted value */
	    q1 = strchr (line,squot);
	    if (q1 != NULL)
		q2 = strchr (q1+1,squot);
	    else
		q2 = NULL;

	    if (q2 == NULL || q2-line < 31)
		c0 = v1 + 31;
	    else
		c0 = v1 + (q2-line) + 2; /* allan: 1997-09-30, was c0=q2+2 */

	    strncpy (c0, "/ ",2);
	    }

/* create new entry */
	lcom = strlen (comment);

	if (lcom > 0) {
	    c1 = c0 + 2;
	    if (c1+lcom > v2)
		lcom = v2 - c1;
	    strncpy (c1, comment, lcom);
	    }

}
