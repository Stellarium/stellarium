/* used by FPACK and FUNPACK
 * R. Seaman, NOAO
 * W. Pence, NASA/GSFC
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* not needed any more */
/* #include <unistd.h> */
/* #include <sys/stat.h> */
/* #include <sys/types.h> */

#define	FPACK_VERSION	"1.7.0 (Dec 2013)"
/*
VERSION	History

1.7.0 (Dec 2013)
    - extensive changes to the binary table compression method.  All types
      of binary table columns, including variable length array columns are
      now supported.  The command line table compression flag has been changed
      to "-table" instead of "-BETAtable", and a new "-tableonly" flag has
      been introduced to only compress the binary tables in the input files(s)
      and not the image HDUs.
1.6.1 (Mar 2013)
    - numerous changes to the BETAtable compression method used to compress
      binary tables
    - added support for compression 'steering' keywords that specify the
      desired compression parameters that should be used when compressing
      that particular HDU, thus overriding the fpack command line parameter
      values.
    
1.6.0 (June 2012)
    - Fixed behavior of the "rename" function on Windows platforms so that
      it will clobber/delete an existing file before renaming a file to 
      that name (the rename command behaves differently on POSIX and non-POSIX
      environments).

1.6.0 (February 2011)
    - Added full support for compressing and uncompressing FITS binary tables
      using a newly proposed format convention.  This is intended only for
      further feasibility studies, and is not recommended for use with publicly
      distributed FITS files.
    - Use the minimum of the MAD 2nd, 3rd, and 5th order values as a more
      conservative extimate of the noise when quantizing floating point images.
    - Enhanced the tile compression routines so that a tile that contains all
      NaN pixel values will be compressed.
    - When uncompressing an image that was originally in a FITS primary array,
      funpack will also append any new keywords that were written into the 
      primary array of the compressed FITS file after the file was compressed.
    - Added support for the GZIP_2 algorithm, which shuffles the bytes in the
      pixel values prior to compressing them with gzip.      
1.5.1 (December 2010) Added prototype, mainly hidden, support for compressing 
      binary tables.  
1.5.0 (August 2010) Added the -i2f option to lossy compress integer images.
1.4.0 (Jan 2010) Reduced the default value for the q floating point image 
      quantization parameter from 16 to 4.  This results in about 50% better
      compression (from about 4.6x to 6.4) with no lost of significant information 
      (with the new subtractive dithering enhancement).  Replaced the code for
      generating temporary filenames to make the code more portable (to Windows). 
      Replaced calls to the unix 'access' and 'stat' functions with more portable
      code.   When unpacking a file, write it first to a temporary file, then
      rename it when finished, so that other tasks cannot try to read the file
      before it is complete.
1.3.0 (Oct 2009) added randomization to the dithering pattern so that
      the same pattern is not used for every image; also added an option
      for losslessly compressing floating point images with GZIP for test 
      purposes (not recommended for general use).  Also added support for
      reading the input FITS file from the stdin file streams.
1.2.0 (Sept 2009) added subtractive dithering feature (in CFITSIO) when
      quantizing floating point images; When packing an IRAF .imh + .pix image,
      the file name is changed to FILE.fits.fz, and if the original file is
      deleted, then both the .imh and .pix files are deleted.
1.1.4 (May 2009) added -E option to funpack to unpack a list of HDUs
1.1.3 (March 2009)  minor modifications to the content and format of the -T report
1.1.2 (September 2008)
*/

#define	FP_INIT_MAGIC	42
#define	FPACK		0
#define	FUNPACK		1

/* changed from 16 in Jan. 2010 */
#define	DEF_QLEVEL	4.  

#define	DEF_HCOMP_SCALE	 0.
#define	DEF_HCOMP_SMOOTH 0
#define	DEF_RESCALE_NOISE 0

#define	SZ_STR		513
#define	SZ_CARD		81


typedef struct
{
	int	comptype;
	float	quantize_level;
	int     no_dither;
	int     dither_offset;
	int     dither_method;
	float	scale;
	float	rescale_noise;
	int	smooth;
	int	int_to_float;
	float	n3ratio;
	float	n3min;
	long	ntile[MAX_COMPRESS_DIM];

	int	to_stdout;
	int	listonly;
	int	clobber;
	int	delete_input;
	int	do_not_prompt;
	int	do_checksums;
	int	do_gzip_file;
	int     do_images;  
	int     do_tables;  
	int	test_all;
	int	verbose;

	char	prefix[SZ_STR];
	char	extname[SZ_STR];
	int	delete_suffix;
	char	outfile[SZ_STR];
	int	firstfile;

	int	initialized;
	int	preflight_checked;
} fpstate;

typedef struct
{
	int	n_nulls;
	double	minval;
	double 	maxval;
	double	mean;
	double	sigma;
	double	noise1;
	double	noise2;
	double	noise3;
	double	noise5;
} imgstats;

int fp_get_param (int argc, char *argv[], fpstate *fpptr);
void abort_fpack(int sig);
void fp_abort_output (fitsfile *infptr, fitsfile *outfptr, int stat); 
int fp_usage (void);
int fp_help (void);
int fp_hint (void); 
int fp_init (fpstate *fpptr);
int fp_list (int argc, char *argv[], fpstate fpvar);
int fp_info (char *infits);
int fp_info_hdu (fitsfile *infptr);
int fp_preflight (int argc, char *argv[], int unpack, fpstate *fpptr);
int fp_loop (int argc, char *argv[], int unpack, fpstate fpvar);
int fp_pack (char *infits, char *outfits, fpstate fpvar, int *islossless);
int fp_unpack (char *infits, char *outfits, fpstate fpvar);
int fp_test (char *infits, char *outfits, char *outfits2, fpstate fpvar);
int fp_pack_hdu (fitsfile *infptr, fitsfile *outfptr, fpstate fpvar, 
    int *islossless, int *status);
int fp_unpack_hdu (fitsfile *infptr, fitsfile *outfptr, fpstate fpvar, int *status);
int fits_read_image_speed (fitsfile *infptr, float *whole_elapse, 
    float *whole_cpu, float *row_elapse, float *row_cpu, int *status);
int fp_test_hdu (fitsfile *infptr, fitsfile *outfptr, fitsfile *outfptr2, 
	fpstate fpvar, int *status);
int fp_test_table (fitsfile *infptr, fitsfile *outfptr, fitsfile *outfptr2, 
	fpstate fpvar, int *status);
int marktime(int *status);
int gettime(float *elapse, float *elapscpu, int *status);
int fits_read_image_speed (fitsfile *infptr, float *whole_elapse, 
    float *whole_cpu, float *row_elapse, float *row_cpu, int *status);

int fp_i2stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status);
int fp_i4stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status);
int fp_r4stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status);
int fp_i2rescale(fitsfile *infptr, int naxis, long *naxes, double rescale,
    fitsfile *outfptr, int *status);
int fp_i4rescale(fitsfile *infptr, int naxis, long *naxes, double rescale,
    fitsfile *outfptr, int *status);
    
int fp_msg (char *msg);
int fp_version (void);
int fp_noop (void);

int fu_get_param (int argc, char *argv[], fpstate *fpptr);
int fu_usage (void);
int fu_hint (void);
int fu_help (void);
