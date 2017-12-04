/* FPACK utility routines
   R. Seaman, NOAO & W. Pence, NASA/GSFC
*/

#include <time.h>
#include <float.h>
#include <signal.h>
#include <ctype.h>

/* #include  "bzlib.h"  only for experimental purposes */

#if defined(unix) || defined(__unix__)  || defined(__unix)
#include <sys/time.h>
#endif

#include <math.h>
#include "fitsio.h"
#include "fpack.h"

/* these filename buffer are used to delete temporary files */
/* in case the program is aborted */
char tempfilename[SZ_STR];
char tempfilename2[SZ_STR];
char tempfilename3[SZ_STR];

/* nearest integer function */
# define NINT(x)  ((x >= 0.) ? (int) (x + 0.5) : (int) (x - 0.5))
# define NSHRT(x) ((x >= 0.) ? (short) (x + 0.5) : (short) (x - 0.5))

/* define variables for measuring elapsed time */
clock_t scpu, ecpu;
long startsec;   /* start of elapsed time interval */
int startmilli;  /* start of elapsed time interval */

/*  CLOCKS_PER_SEC should be defined by most compilers */
#if defined(CLOCKS_PER_SEC)
#define CLOCKTICKS CLOCKS_PER_SEC
#else
/* on SUN OS machine, CLOCKS_PER_SEC is not defined, so set its value */
#define CLOCKTICKS 1000000
#endif

FILE *outreport;

/* dimension of central image area to be sampled for test statistics */
int XSAMPLE = 4100;
int YSAMPLE = 4100;

int fp_msg (char *msg)
{
        printf ("%s", msg);
        return(0);
}
/*--------------------------------------------------------------------------*/
int fp_noop (void)
{
        fp_msg ("Input and output files are unchanged.\n");
        return(0);
}
/*--------------------------------------------------------------------------*/
void fp_abort_output (fitsfile *infptr, fitsfile *outfptr, int stat)
{
	int status = 0, hdunum;
	char  msg[SZ_STR];

	fits_file_name(infptr, tempfilename, &status);
	fits_get_hdu_num(infptr, &hdunum);
	
        fits_close_file (infptr, &status);

	sprintf(msg, "Error processing file: %s\n", tempfilename);
	fp_msg (msg);
	sprintf(msg, "  in HDU number %d\n", hdunum);
	fp_msg (msg);

	fits_report_error (stderr, stat);

	if (outfptr) {
	    fits_delete_file(outfptr, &status);
	    fp_msg ("Input file is unchanged.\n");
	}

	exit (stat); 
}
/*--------------------------------------------------------------------------*/
int fp_version (void)
{
        float version;
        char cfitsioversion[40];

        fp_msg (FPACK_VERSION);
        fits_get_version(&version);
        sprintf(cfitsioversion, " CFITSIO version %5.3f", version);
        fp_msg(cfitsioversion);
        fp_msg ("\n");
        return(0);
}
/*--------------------------------------------------------------------------*/
int fp_access (char *filename)
{
	/* test if a file exists */
	
	FILE *diskfile;
	
	diskfile = fopen(filename, "r");

	if (diskfile) {
		fclose(diskfile);
		return(0);
	} else {
	        return(-1);
	}
}
/*--------------------------------------------------------------------------*/
int fp_tmpnam(char *suffix, char *rootname, char *tmpnam)
{
	/* create temporary file name */

	int maxtry = 30, ii;

	if (strlen(suffix) + strlen(rootname) > SZ_STR-5) {
	    fp_msg ("Error: filename is too long to create tempory file\n"); exit (-1);
	}

	strcpy (tmpnam, rootname);  /* start with rootname */
	strcat(tmpnam, suffix);     /* append the suffix */

	maxtry = SZ_STR - strlen(tmpnam) - 1;

        for (ii = 0; ii < maxtry; ii++) {
		if (fp_access(tmpnam)) break;  /* good, the file does not exist */
		strcat(tmpnam, "x");  /* append an x to the name, and try again */
	}

	if (ii == maxtry) {
		fp_msg ("\nCould not create temporary file name:\n");
		fp_msg (tmpnam);
		fp_msg ("\n");
		exit (-1);
	}

        return(0);
}
/*--------------------------------------------------------------------------*/
int fp_init (fpstate *fpptr)
{
	int	ii;

	fpptr->comptype = RICE_1;
	fpptr->quantize_level = DEF_QLEVEL;
        fpptr->no_dither = 0;
        fpptr->dither_method = 1;
        fpptr->dither_offset = 0;
        fpptr->int_to_float = 0;

	/* thresholds when using the -i2f flag */
        fpptr->n3ratio = 2.0;  /* minimum ratio of image noise sigma / q */
        fpptr->n3min = 6.;     /* minimum noise sigma. */

	fpptr->scale = DEF_HCOMP_SCALE;
	fpptr->smooth = DEF_HCOMP_SMOOTH;
	fpptr->rescale_noise = DEF_RESCALE_NOISE;
	fpptr->ntile[0] = (long) -1;	/* -1 means extent of axis */

	for (ii=1; ii < MAX_COMPRESS_DIM; ii++)
	    fpptr->ntile[ii] = (long) 1;

	fpptr->to_stdout = 0;
	fpptr->listonly = 0;
	fpptr->clobber = 0;
	fpptr->delete_input = 0;
	fpptr->do_not_prompt = 0;
	fpptr->do_checksums = 1;
	fpptr->do_gzip_file = 0;
	fpptr->do_tables = 0;  /* this is intended for testing purposes  */
	fpptr->do_images = 1;  /* can be turned off with -tableonly switch */
	fpptr->test_all = 0;
	fpptr->verbose = 0;

	fpptr->prefix[0] = 0;
	fpptr->extname[0] = 0;
	fpptr->delete_suffix = 0;
	fpptr->outfile[0] = 0;

	fpptr->firstfile = 1;

	/* magic number for initialization check, boolean for preflight
	 */
	fpptr->initialized = FP_INIT_MAGIC;
	fpptr->preflight_checked = 0;
	return(0);
}
/*--------------------------------------------------------------------------*/
int fp_list (int argc, char *argv[], fpstate fpvar)
{
	fitsfile *infptr;
	char	infits[SZ_STR], msg[SZ_STR];
	int	hdunum, iarg, stat=0;
	LONGLONG sizell;

	if (fpvar.initialized != FP_INIT_MAGIC) {
	    fp_msg ("Error: internal initialization error\n"); exit (-1);
	}

	for (iarg=fpvar.firstfile; iarg < argc; iarg++) {
	    strncpy (infits, argv[iarg], SZ_STR);

	    if (strchr (infits, '[') || strchr (infits, ']')) {
		fp_msg ("Error: section/extension notation not supported: ");
		fp_msg (infits); fp_msg ("\n"); exit (-1);
	    }

	    if (fp_access (infits) != 0) {
		        fp_msg ("Error: can't find or read input file "); fp_msg (infits);
		        fp_msg ("\n"); fp_noop (); exit (-1);
	    }

	    fits_open_file (&infptr, infits, READONLY, &stat);
	    if (stat) { fits_report_error (stderr, stat); exit (stat); }

	    /* move to the end of file, to get the total size in bytes */
	    fits_get_num_hdus (infptr, &hdunum, &stat);
	    fits_movabs_hdu (infptr, hdunum, NULL, &stat);
	    fits_get_hduaddrll(infptr, NULL, NULL, &sizell, &stat);


	    if (stat) { 
	        fp_abort_output(infptr, NULL, stat);
	    }

            sprintf (msg, "# %s (", infits); fp_msg (msg);

#if defined(_MSC_VER)
    /* Microsoft Visual C++ 6.0 uses '%I64d' syntax  for 8-byte integers */
        sprintf(msg, "%I64d bytes)\n", sizell); fp_msg (msg);
#elif (USE_LL_SUFFIX == 1)
        sprintf(msg, "%lld bytes)\n", sizell); fp_msg (msg);
#else
        sprintf(msg, "%ld bytes)\n", sizell); fp_msg (msg);
#endif
	    fp_info_hdu (infptr);

	    fits_close_file (infptr, &stat);
	    if (stat) { fits_report_error (stderr, stat); exit (stat); }
	}
	return(0);
}
/*--------------------------------------------------------------------------*/
int fp_info_hdu (fitsfile *infptr)
{
	long	naxes[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	char	msg[SZ_STR], val[SZ_CARD], com[SZ_CARD];
        int     naxis=0, hdutype, bitpix, hdupos, stat=0, ii;
        unsigned long   datasum, hdusum;

	fits_movabs_hdu (infptr, 1, NULL, &stat);
	if (stat) { 
	    fp_abort_output(infptr, NULL, stat);
	}

	for (hdupos=1; ! stat; hdupos++) {
	    fits_get_hdu_type (infptr, &hdutype, &stat);
	    if (stat) { 
	        fp_abort_output(infptr, NULL, stat);
	    }

	    /* fits_get_hdu_type calls unknown extensions "IMAGE_HDU"
	     * so consult XTENSION keyword itself
	     */
	    fits_read_keyword (infptr, "XTENSION", val, com, &stat);
	    if (stat == KEY_NO_EXIST) {
		/* in primary HDU which by definition is an "image" */
		stat=0; /* clear for later error handling */

	    } else if (stat) {
	        fp_abort_output(infptr, NULL, stat);

	    } else if (hdutype == IMAGE_HDU) {
		/* that is, if XTENSION != "IMAGE" AND != "BINTABLE" */
		if (strncmp (val+1, "IMAGE", 5) &&
		    strncmp (val+1, "BINTABLE", 5)) {

		     /* assign something other than any of these */
		    hdutype = IMAGE_HDU + ASCII_TBL + BINARY_TBL;
		}
	    }

            fits_get_chksum(infptr, &datasum, &hdusum, &stat);

	    if (hdutype == IMAGE_HDU) {
		sprintf (msg, "  %d IMAGE", hdupos); fp_msg (msg);
                sprintf (msg, " SUMS=%lu/%lu", (unsigned long) (~((int) hdusum)), datasum); fp_msg (msg);

		fits_get_img_param (infptr, 9, &bitpix, &naxis, naxes, &stat);

                sprintf (msg, " BITPIX=%d", bitpix); fp_msg (msg);

		if (naxis == 0) {
                    sprintf (msg, " [no_pixels]"); fp_msg (msg);
		} else if (naxis == 1) {
		    sprintf (msg, " [%ld]", naxes[1]); fp_msg (msg);
		} else {
		    sprintf (msg, " [%ld", naxes[0]); fp_msg (msg);
		    for (ii=1; ii < naxis; ii++) {
			sprintf (msg, "x%ld", naxes[ii]); fp_msg (msg);
		    }
		    fp_msg ("]");
		}

                if (fits_is_compressed_image (infptr, &stat)) {
                    fits_read_keyword (infptr, "ZCMPTYPE", val, com, &stat);

                    /* allow for quote in keyword value */
                    if (! strncmp (val+1, "RICE_1", 6))
                        fp_msg (" tiled_rice\n");
                    else if (! strncmp (val+1, "GZIP_1", 6))
                        fp_msg (" tiled_gzip_1\n");
                    else if (! strncmp (val+1, "GZIP_2", 6))
                        fp_msg (" tiled_gzip_2\n");
                    else if (! strncmp (val+1, "PLIO_1", 6))
                        fp_msg (" tiled_plio\n");
                    else if (! strncmp (val+1, "HCOMPRESS_1", 11))
                        fp_msg (" tiled_hcompress\n");
                    else
                        fp_msg (" unknown\n");

                } else
                    fp_msg (" not_tiled\n");

            } else if (hdutype == ASCII_TBL) {
                sprintf (msg, "  %d ASCII_TBL", hdupos); fp_msg (msg);
                sprintf (msg, " SUMS=%lu/%lu\n", (unsigned long) (~((int) hdusum)), datasum); fp_msg (msg);

            } else if (hdutype == BINARY_TBL) {
                sprintf (msg, "  %d BINARY_TBL", hdupos); fp_msg (msg);
                sprintf (msg, " SUMS=%lu/%lu\n", (unsigned long) (~((int) hdusum)), datasum); fp_msg (msg);

            } else {
                sprintf (msg, "  %d OTHER", hdupos); fp_msg (msg);
                sprintf (msg, " SUMS=%lu/%lu",   (unsigned long) (~((int) hdusum)), datasum); fp_msg (msg);
                sprintf (msg, " %s\n", val); fp_msg (msg);
            }

	    fits_movrel_hdu (infptr, 1, NULL, &stat);
	}
	return(0);
}

/*--------------------------------------------------------------------------*/
int fp_preflight (int argc, char *argv[], int unpack, fpstate *fpptr)
{
	char	infits[SZ_STR], outfits[SZ_STR];
	int	iarg, namelen, nfiles = 0;

	if (fpptr->initialized != FP_INIT_MAGIC) {
	    fp_msg ("Error: internal initialization error\n"); exit (-1);
	}

	for (iarg=fpptr->firstfile; iarg < argc; iarg++) {

            outfits[0] = '\0';
	    
            if (strlen(argv[iarg]) > SZ_STR - 4) {  /* allow for .fz or .gz suffix */
		    fp_msg ("Error: input file name\n   "); fp_msg (argv[iarg]);
		    fp_msg ("\n   is too long\n"); fp_noop (); exit (-1);
	    }

	    strncpy (infits, argv[iarg], SZ_STR);
	    if (infits[0] == '-' && infits[1] != '\0') {  
	         /* don't interpret this as intending to read input file from stdin */
		    fp_msg ("Error: invalid input file name\n   "); fp_msg (argv[iarg]);
		    fp_msg ("\n"); fp_noop (); exit (-1);
	    }

	    if (strchr (infits, '[') || strchr (infits, ']')) {
		fp_msg ("Error: section/extension notation not supported: ");
		fp_msg (infits); fp_msg ("\n"); fp_noop (); exit (-1);
	    }
	    
            if (unpack) {
	  	/* ********** This section applies to funpack ************ */

	      /* check that input file  exists */
	      if (infits[0] != '-') {  /* if not reading from stdin stream */
	         if (fp_access (infits) != 0) {  /* if not, then check if */
		    strcat(infits, ".fz");       /* a .fz version exsits */
	            if (fp_access (infits) != 0) {
                        namelen = strlen(infits);
                        infits[namelen - 3] = '\0';  /* remove the .fz suffix */
		        fp_msg ("Error: can't find or read input file "); fp_msg (infits);
		        fp_msg ("\n"); fp_noop (); exit (-1);
                    }
	         } else {   /* make sure a .fz version of the same file doesn't exist */
                    namelen = strlen(infits);
		    strcat(infits, ".fz");   
	            if (fp_access (infits) == 0) {
                        infits[namelen] = '\0';  /* remove the .fz suffix */
		        fp_msg ("Error: ambiguous input file name.  Which file should be unpacked?:\n  ");
		        fp_msg (infits); fp_msg ("\n  "); 
		        fp_msg (infits); fp_msg (".fz\n"); 
		        fp_noop (); exit (-1);
                    } else {
                        infits[namelen] = '\0';  /* remove the .fz suffix */
		    }
	         }
	      }

              /* if writing to stdout, then we are all done */
	      if (fpptr->to_stdout) {
                      continue;
	      }

              if (fpptr->outfile[0]) {  /* user specified output file name */
	          nfiles++;
		  if (nfiles > 1) {
		      fp_msg ("Error: cannot use same output file name for multiple files:\n   ");
		      fp_msg (fpptr->outfile);
		      fp_msg ("\n"); fp_noop (); exit (-1);
	          }

                  /* check that output file doesn't exist */
	          if (fp_access (fpptr->outfile) == 0) {
		            fp_msg ("Error: output file already exists:\n "); 
			    fp_msg (fpptr->outfile);
		            fp_msg ("\n "); fp_noop (); exit (-1);
		  } 
                  continue;
	      }     

	      /* construct output file name to test */
	      if (fpptr->prefix[0]) {
                  if (strlen(fpptr->prefix) + strlen(infits) > SZ_STR - 1) {
		      fp_msg ("Error: output file name for\n   "); fp_msg (infits);
		      fp_msg ("\n   is too long with the prefix\n"); fp_noop (); exit (-1);
	          }
	          strcat(outfits,fpptr->prefix);
	      }

	      /* construct output file name */
	      if (infits[0] == '-') {
	        strcpy(outfits, "output.fits");
	      } else {
	        strcpy(outfits, infits);
	      }

	      /* remove .gz suffix, if present (output is not gzipped) */
              namelen = strlen(outfits);
	      if ( !strcmp(".gz", outfits + namelen - 3) ) {
                        outfits[namelen - 3] = '\0';
	      }

	      /* check for .fz suffix that is sometimes required */
	      /* and remove it if present */
	      if (infits[0] != '-') {  /* if not reading from stdin stream */
                 namelen = strlen(outfits);
	         if ( !strcmp(".fz", outfits + namelen - 3) ) { /* suffix is present */
                        outfits[namelen - 3] = '\0';
	         } else if (fpptr->delete_suffix) {  /* required suffix is missing */
		    fp_msg ("Error: input compressed file "); fp_msg (infits);
		    fp_msg ("\n does not have the default .fz suffix.\n"); 
		    fp_noop (); exit (-1);
	         }
	      }

	      /* if infits != outfits, make sure outfits doesn't already exist */
              if (strcmp(infits, outfits)) {
	                if (fp_access (outfits) == 0) {
		            fp_msg ("Error: output file already exists:\n "); fp_msg (outfits);
		            fp_msg ("\n "); fp_noop (); exit (-1);
		        }       
	      }

	      /* if gzipping the output, make sure .gz file doesn't exist */
	      if (fpptr->do_gzip_file) {
	                strcat(outfits, ".gz");
	                if (fp_access (outfits) == 0) {
		            fp_msg ("Error: output file already exists:\n "); fp_msg (outfits);
		            fp_msg ("\n "); fp_noop (); exit (-1);
		        }       
              		namelen = strlen(outfits);
                        outfits[namelen - 3] = '\0';  /* remove the .gz suffix again */
	      }
	  } else {
	  	/* ********** This section applies to fpack ************ */

	      /* check that input file  exists */
	      if (infits[0] != '-') {  /* if not reading from stdin stream */
	        if (fp_access (infits) != 0) {  /* if not, then check if */
		    strcat(infits, ".gz");     /* a gzipped version exsits */
	            if (fp_access (infits) != 0) {
                        namelen = strlen(infits);
                        infits[namelen - 3] = '\0';  /* remove the .gz suffix */
		        fp_msg ("Error: can't find or read input file "); fp_msg (infits);
		        fp_msg ("\n"); fp_noop (); exit (-1);
                    }
	        }
	      }

              /* make sure the file to pack does not already have a .fz suffix */
              namelen = strlen(infits);
	      if ( !strcmp(".fz", infits + namelen - 3) ) {
		        fp_msg ("Error: fpack input file already has '.fz' suffix\n" ); fp_msg (infits);
		        fp_msg ("\n"); fp_noop (); exit (-1);
	      }

              /* if writing to stdout, or just testing the files, then we are all done */
	      if (fpptr->to_stdout || fpptr->test_all) {
                        continue;
	      }

	      /* construct output file name */
	      if (infits[0] == '-') {
	        strcpy(outfits, "input.fits");
	      } else {
	        strcpy(outfits, infits);
	      }

	      /* remove .gz suffix, if present (output is not gzipped) */
              namelen = strlen(outfits);
	      if ( !strcmp(".gz", outfits + namelen - 3) ) {
                        outfits[namelen - 3] = '\0';
	      }
	      
	      /* remove .imh suffix (IRAF format image), and replace with .fits */
              namelen = strlen(outfits);
	      if ( !strcmp(".imh", outfits + namelen - 4) ) {
                        outfits[namelen - 4] = '\0';
                        strcat(outfits, ".fits");
	      }

	      /* If not clobbering the input file, add .fz suffix to output name */
	      if (! fpptr->clobber)
		        strcat(outfits, ".fz");
			
	      /* if infits != outfits, make sure outfits doesn't already exist */
              if (strcmp(infits, outfits)) {
	                if (fp_access (outfits) == 0) {
		            fp_msg ("Error: output file already exists:\n "); fp_msg (outfits);
		            fp_msg ("\n "); fp_noop (); exit (-1);
		        }       
	      }
	  }   /* end of fpack section */
	}

	fpptr->preflight_checked++;
	return(0);
}

/*--------------------------------------------------------------------------*/
/* must run fp_preflight() before fp_loop()
 */
int fp_loop (int argc, char *argv[], int unpack, fpstate fpvar)
{
	char	infits[SZ_STR], outfits[SZ_STR];
	char	temp[SZ_STR], answer[30];
	int	iarg, islossless, namelen, iraf_infile = 0, status = 0, ifail;
        
	if (fpvar.initialized != FP_INIT_MAGIC) {
	    fp_msg ("Error: internal initialization error\n"); exit (-1);
	} else if (! fpvar.preflight_checked) {
	    fp_msg ("Error: internal preflight error\n"); exit (-1);
	}

	if (fpvar.test_all && fpvar.outfile[0]) {
	    outreport = fopen(fpvar.outfile, "w");
		fprintf(outreport," Filename Extension BITPIX NAXIS1 NAXIS2 Size N_nulls Minval Maxval Mean Sigm Noise1 Noise2 Noise3 Noise5 T_whole T_rowbyrow ");
		fprintf(outreport,"[Comp_ratio, Pack_cpu, Unpack_cpu, Lossless readtimes] (repeated for Rice, Hcompress, and GZIP)\n");
	}


	tempfilename[0] = '\0';
	tempfilename2[0] = '\0';
	tempfilename3[0] = '\0';

/* set up signal handler to delete temporary file on abort */	    
#ifdef SIGINT
    if (signal(SIGINT, SIG_IGN) != SIG_IGN) {
	(void) signal(SIGINT,  abort_fpack); 
    }
#endif

#ifdef SIGTERM
    if (signal(SIGTERM, SIG_IGN) != SIG_IGN) {
	(void) signal(SIGTERM,  abort_fpack); 
    }
#endif

#ifdef SIGHUP
    if (signal(SIGHUP, SIG_IGN) != SIG_IGN) {
	(void) signal(SIGHUP,  abort_fpack);
    }
#endif

	for (iarg=fpvar.firstfile; iarg < argc; iarg++) {

          temp[0] = '\0';
	  outfits[0] = '\0';
          islossless = 1;

	  strncpy (infits, argv[iarg], SZ_STR - 1);

          if (unpack) {
	  	/* ********** This section applies to funpack ************ */

	      /* find input file */
	      if (infits[0] != '-') {  /* if not reading from stdin stream */
	         if (fp_access (infits) != 0) {  /* if not, then */
		    strcat(infits, ".fz");       /* a .fz version must exsit */
	         }
	      }

	      if (fpvar.to_stdout) {
		strcpy(outfits, "-");

              } else if (fpvar.outfile[0]) {  /* user specified output file name */
	          strcpy(outfits, fpvar.outfile);

	      } else {
	          /* construct output file name */
	          if (fpvar.prefix[0]) {
	              strcat(outfits,fpvar.prefix);
	          }

	          /* construct output file name */
	          if (infits[0] == '-') {
	            strcpy(outfits, "output.fits");
	          } else {
	            strcpy(outfits, infits);
	          }

	          /* remove .gz suffix, if present (output is not gzipped) */
                  namelen = strlen(outfits);
	          if ( !strcmp(".gz", outfits + namelen - 3) ) {
                        outfits[namelen - 3] = '\0';
	          }

	          /* check for .fz suffix that is sometimes required */
	          /* and remove it if present */
                  namelen = strlen(outfits);
	          if ( !strcmp(".fz", outfits + namelen - 3) ) { /* suffix is present */
                        outfits[namelen - 3] = '\0';
	          }
	      }

	  } else {
	  	/* ********** This section applies to fpack ************ */

	      if (fpvar.to_stdout) {
		strcpy(outfits, "-");
	      } else if (! fpvar.test_all) {

	          /* construct output file name */
	          if (infits[0] == '-') {
	            strcpy(outfits, "input.fits");
	          } else {
	            strcpy(outfits, infits);
	          }
	      
	          /* remove .gz suffix, if present (output is not gzipped) */
                  namelen = strlen(outfits);
	          if ( !strcmp(".gz", outfits + namelen - 3) ) {
                        outfits[namelen - 3] = '\0';
	          }
	      
	          /* remove .imh suffix (IRAF format image), and replace with .fits */
                  namelen = strlen(outfits);
	          if ( !strcmp(".imh", outfits + namelen - 4) ) {
                        outfits[namelen - 4] = '\0';
                        strcat(outfits, ".fits");
                        iraf_infile = 1;  /* this is an IRAF format input file */
			           /* change the output name to "NAME.fits.fz" */
	          }

	          /* If not clobbering the input file, add .fz suffix to output name */
	          if (! fpvar.clobber)
		        strcat(outfits, ".fz");
	      }
	  }

          strncpy(temp, outfits, SZ_STR-1);

	  if (infits[0] != '-') {  /* if not reading from stdin stream */
             if (!strcmp(infits, outfits) ) {  /* are input and output names the same? */

                /* clobber the input file with the output file with the same name */
                if (! fpvar.clobber) {
                    fp_msg ("\nError: must use -F flag to clobber input file.\n");
		    exit (-1);
		} 

                /* create temporary file name in the output directory (same as input directory)*/
		fp_tmpnam("Tmp1", infits, outfits);
		
                strcpy(tempfilename, outfits);  /* store temp file name, in case of abort */
	      }
	    }


            /* *************** now do the real work ********************* */
	    
	    if (fpvar.verbose && ! fpvar.to_stdout)
		printf("%s ", infits);
		
	    if (fpvar.test_all) {   /* compare all the algorithms */

                /* create 2 temporary file names, in the CWD */
		fp_tmpnam("Tmpfile1", "", tempfilename);
		fp_tmpnam("Tmpfile2", "", tempfilename2);

		fp_test (infits, tempfilename, tempfilename2, fpvar);

		remove(tempfilename);
                tempfilename[0] = '\0';   /* clear the temp file name */
		remove(tempfilename2);
                tempfilename2[0] = '\0';
                continue;

	    } else if (unpack) {
		if (fpvar.to_stdout) {
			/* unpack the input file to the stdout stream */
			fp_unpack (infits, outfits, fpvar);
		} else {
			/* unpack to temporary file, so other tasks can't open it until it is renamed */

			/* create  temporary file name, in the output directory */
			fp_tmpnam("Tmp2", outfits, tempfilename2);

			/* unpack the input file to the temporary file */
			fp_unpack (infits, tempfilename2, fpvar);

			/* rename the temporary file to it's real name */
			ifail = rename(tempfilename2, outfits);
			if (ifail) {
			    fp_msg("Failed to rename temporary file name:\n  ");
			    fp_msg(tempfilename2);
			    fp_msg(" -> ");
			    fp_msg(outfits);
			    fp_msg("\n");
			    exit (-1);
			} else {
			    tempfilename2[0] = '\0';  /* clear temporary file name */
			}
		}
	    }  else {
		fp_pack (infits, outfits, fpvar, &islossless);
	    }

	    if (fpvar.to_stdout) {
		continue;
	    }

            /* ********** clobber and/or delete files, if needed ************** */

            if (!strcmp(infits, temp) && fpvar.clobber ) {  

		if (!islossless && ! fpvar.do_not_prompt) {
		    fp_msg ("\nFile ");
		    fp_msg (infits); 
		    fp_msg ("\nwas compressed with a LOSSY method.  Overwrite the\n");
		    fp_msg ("original file with the compressed version? (Y/N) ");
		    fgets(answer, 29, stdin);
		    if (answer[0] != 'Y' && answer[0] != 'y') {
		        fp_msg ("\noriginal file NOT overwritten!\n");
			remove(outfits);
                        continue;
		    }
		}

		if (iraf_infile) {  /* special case of deleting an IRAF format header and pixel file */
		   if (fits_delete_iraf_file(infits, &status)) {
		        fp_msg("\nError deleting IRAF .imh and .pix files.\n");
			fp_msg(infits); fp_msg ("\n"); exit (-1);
		    }
		}
				
#if defined(unix) || defined(__unix__)  || defined(__unix)
	        /* rename clobbers input on Unix platforms */
		if (rename (outfits, temp) != 0) {
		        fp_msg ("\nError renaming tmp file to ");
		        fp_msg (temp); fp_msg ("\n"); exit (-1);
		}
#else
	        /* rename DOES NOT clobber existing files on Windows platforms */
                /* so explicitly remove any existing file before renaming the file */
                remove(temp);
		if (rename (outfits, temp) != 0) {
		        fp_msg ("\nError renaming tmp file to ");
		        fp_msg (temp); fp_msg ("\n"); exit (-1);
		}
#endif

		tempfilename[0] = '\0';  /* clear temporary file name */
                strcpy(outfits, temp);

	    } else if (fpvar.clobber || fpvar.delete_input) {      /* delete the input file */
	         if (!islossless && !fpvar.do_not_prompt) {  /* user did not turn off delete prompt */
		    fp_msg ("\nFile ");
		    fp_msg (infits); 
		    fp_msg ("\nwas compressed with a LOSSY method.  \n");
		    fp_msg ("Delete the original file? (Y/N) ");
		    fgets(answer, 29, stdin);
		    if (answer[0] != 'Y' && answer[0] != 'y') {  /* user abort */
		        fp_msg ("\noriginal file NOT deleted!\n");
		    } else {
			if (iraf_infile) {  /* special case of deleting an IRAF format header and pixel file */
		   	    if (fits_delete_iraf_file(infits, &status)) {
		        	fp_msg("\nError deleting IRAF .imh and .pix files.\n");
				fp_msg(infits); fp_msg ("\n"); exit (-1);
			    }
		        }  else if (remove(infits) != 0) {  /* normal case of deleting input FITS file */
		            fp_msg ("\nError deleting input file ");
		            fp_msg (infits); fp_msg ("\n"); exit (-1);
		        }
		    }
		  } else {   /* user said don't prompt, so just delete the input file */
			if (iraf_infile) {  /* special case of deleting an IRAF format header and pixel file */
		   	    if (fits_delete_iraf_file(infits, &status)) {
		        	fp_msg("\nError deleting IRAF .imh and .pix files.\n");
				fp_msg(infits); fp_msg ("\n"); exit (-1);
			    }
		        }  else if (remove(infits) != 0) {  /* normal case of deleting input FITS file */
		            fp_msg ("\nError deleting input file ");
		            fp_msg (infits); fp_msg ("\n"); exit (-1);
		        }
		  }
	    }
            iraf_infile = 0; 

	    if (fpvar.do_gzip_file) {       /* gzip the output file */
		strcpy(temp, "gzip -1 ");
		strcat(temp,outfits);
                system(temp);
	        strcat(outfits, ".gz");    /* only possibible with funpack */
	    }

	    if (fpvar.verbose && ! fpvar.to_stdout)
		printf("-> %s\n", outfits);

	}

	if (fpvar.test_all && fpvar.outfile[0])
	    fclose(outreport);
	return(0);
}

/*--------------------------------------------------------------------------*/
/* fp_pack assumes the output file does not exist (checked by preflight)
 */
int fp_pack (char *infits, char *outfits, fpstate fpvar, int *islossless)
{
	fitsfile *infptr, *outfptr;
	int	stat=0;

	fits_open_file (&infptr, infits, READONLY, &stat);
	if (stat) { fits_report_error (stderr, stat); exit (stat); }

	fits_create_file (&outfptr, outfits, &stat);
	if (stat) { 
	    fp_abort_output(infptr, NULL, stat);
	}


	if (stat) { 
	    fp_abort_output(infptr, outfptr, stat);
	}

	while (! stat) {

	    /*  LOOP OVER EACH HDU */

	    fits_set_lossy_int (outfptr, fpvar.int_to_float, &stat);
	    fits_set_compression_type (outfptr, fpvar.comptype, &stat);
	    fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);

	    if (fpvar.no_dither)
	        fits_set_quantize_method(outfptr, -1, &stat);
	    else
	        fits_set_quantize_method(outfptr, fpvar.dither_method, &stat);

	    fits_set_quantize_level (outfptr, fpvar.quantize_level, &stat);
	    fits_set_dither_offset(outfptr, fpvar.dither_offset, &stat);
	    fits_set_hcomp_scale (outfptr, fpvar.scale, &stat);
	    fits_set_hcomp_smooth (outfptr, fpvar.smooth, &stat);

	    fp_pack_hdu (infptr, outfptr, fpvar, islossless, &stat);

	    if (fpvar.do_checksums) {
	        fits_write_chksum (outfptr, &stat);
	    }

	    fits_movrel_hdu (infptr, 1, NULL, &stat);
	}

	if (stat == END_OF_FILE) stat = 0;

	/* set checksum for case of newly created primary HDU	 */

	if (fpvar.do_checksums) {
	    fits_movabs_hdu (outfptr, 1, NULL, &stat);
	    fits_write_chksum (outfptr, &stat);
	}

	if (stat) { 
	    fp_abort_output(infptr, outfptr, stat);
	}

	fits_close_file (outfptr, &stat);
	fits_close_file (infptr, &stat);

	return(0);
}

/*--------------------------------------------------------------------------*/
/* fp_unpack assumes the output file does not exist
 */
int fp_unpack (char *infits, char *outfits, fpstate fpvar)
{
        fitsfile *infptr, *outfptr;
        int stat=0, hdutype, extnum, single = 0;
	char *loc, *hduloc, hduname[SZ_STR];

        fits_open_file (&infptr, infits, READONLY, &stat);
        fits_create_file (&outfptr, outfits, &stat);

	if (stat) { 
	    fp_abort_output(infptr, outfptr, stat);
	}

        if (fpvar.extname[0]) {  /* unpack a list of HDUs? */

            /* move to the first HDU in the list */
	    hduloc = fpvar.extname;
	    loc = strchr(hduloc, ','); /* look for 'comma' delimiter between names */
	    
	    if (loc)         
	        *loc = '\0';  /* terminate the first name in the string */

	    strcpy(hduname, hduloc);  /* copy the first name into temporary string */

	    if (loc)        
	        hduloc = loc + 1;  /* advance to the beginning of the next name, if any */
            else {
	        hduloc += strlen(hduname);  /* end of the list */
	        single = 1;  /* only 1 HDU is being unpacked */
            }

            if (isdigit( (int) hduname[0]) ) {
	       extnum = strtol(hduname, &loc, 10); /* read the string as an integer */

               /* check for junk following the integer */
               if (*loc == '\0' )  /* no junk, so move to this HDU number (+1) */
               {	       
                  fits_movabs_hdu(infptr, extnum + 1, &hdutype, &stat);  /* move to HDU number */
                  if (hdutype != IMAGE_HDU)
		     stat = NOT_IMAGE;

               } else {  /* the string is not an integer, so must be the column name */
	          hdutype = IMAGE_HDU;
	          fits_movnam_hdu(infptr, hdutype, hduname, 0, &stat);
 	       }
            }
	    else
	    {
                /* move to the named image extension */
  	        hdutype = IMAGE_HDU;
	        fits_movnam_hdu(infptr, hdutype, hduname, 0, &stat);
            }
        }

        if (stat) {
	  fp_msg ("Unable to find and move to extension '");
          fp_msg(hduname);
	  fp_msg("'\n");
	  fp_abort_output(infptr, outfptr, stat);
        }

        while (! stat) {
	
	    if (single)
	        stat = -1;  /* special status flag to force output primary array */

            fp_unpack_hdu (infptr, outfptr, fpvar, &stat);

	    if (fpvar.do_checksums) {
	        fits_write_chksum (outfptr, &stat);
	    }

	    /* move to the next HDU */
            if (fpvar.extname[0]) {  /* unpack a list of HDUs? */

                if (!(*hduloc)) {
		    stat = END_OF_FILE;  /* we reached the end of the list */
		} else {
		    /* parse the next HDU name and move to it */
	            loc = strchr(hduloc, ',');
	    
	            if (loc)         /* look for 'comma' delimiter between names */
	               *loc = '\0';  /* terminate the first name in the string */

	            strcpy(hduname, hduloc);  /* copy the next name into temporary string */

	            if (loc)         
	                hduloc = loc + 1;  /* advance to the beginning of the next name, if any */
                    else
	               *hduloc = '\0';  /* end of the list */

                    if (isdigit( (int) hduname[0]) ) {
	               extnum = strtol(hduname, &loc, 10); /* read the string as an integer */

                      /* check for junk following the integer */
                      if (*loc == '\0' )   /* no junk, so move to this HDU number (+1) */
		      {	       
                        fits_movabs_hdu(infptr, extnum + 1, &hdutype, &stat);  /* move to HDU number */
                        if (hdutype != IMAGE_HDU)
		        stat = NOT_IMAGE;

                      } else {  /* the string is not an integer, so must be the column name */
	                hdutype = IMAGE_HDU;
	                fits_movnam_hdu(infptr, hdutype, hduname, 0, &stat);
 	              }

                    } else {
                      /* move to the named image extension */
  	              hdutype = IMAGE_HDU;
	              fits_movnam_hdu(infptr, hdutype, hduname, 0, &stat);
                   }

                   if (stat) {
	              fp_msg ("Unable to find and move to extension '");
                      fp_msg(hduname);
	              fp_msg("'\n");
                   }
                }
            } else {
                /* increment to the next HDU */
                fits_movrel_hdu (infptr, 1, NULL, &stat);
            }
        }

        if (stat == END_OF_FILE) stat = 0;

        /* set checksum for case of newly created primary HDU
         */
	if (fpvar.do_checksums) {
	    fits_movabs_hdu (outfptr, 1, NULL, &stat);
	    fits_write_chksum (outfptr, &stat);
	}


	if (stat) { 
	    fp_abort_output(infptr, outfptr, stat);
	}

	fits_close_file (outfptr, &stat);
	fits_close_file (infptr, &stat);

	return(0);
}

/*--------------------------------------------------------------------------*/
/* fp_test assumes the output files do not exist
 */
int fp_test (char *infits, char *outfits, char *outfits2, fpstate fpvar)
{
	fitsfile *inputfptr, *infptr, *outfptr, *outfptr2, *tempfile;

	long	naxes[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int	stat=0, totpix=0, naxis=0, ii, hdutype, bitpix = 0, extnum = 0, len;
	int     tstatus = 0, hdunum, rescale_flag, bpix, ncols;
	char	dtype[8], dimen[100];
	double  bscale, rescale, noisemin;
	long headstart, datastart, dataend;
	float origdata = 0., whole_cpu, whole_elapse, row_elapse, row_cpu, xbits;

	LONGLONG nrows;
	/* structure to hold image statistics (defined in fpack.h) */
	imgstats imagestats;

	fits_open_file (&inputfptr, infits, READONLY, &stat);
	fits_create_file (&outfptr, outfits, &stat);
	fits_create_file (&outfptr2, outfits2, &stat);

	if (stat) { fits_report_error (stderr, stat); exit (stat); }

	while (! stat) {
	    
	    /*  LOOP OVER EACH HDU */
	    rescale_flag = 0;
	    fits_get_hdu_type (inputfptr, &hdutype, &stat);

	    if (hdutype == IMAGE_HDU) {
	        fits_get_img_param (inputfptr, 9, &bitpix, &naxis, naxes, &stat);
	        for (totpix=1, ii=0; ii < 9; ii++) totpix *= naxes[ii];
	    }

	    if (!fits_is_compressed_image (inputfptr,  &stat) && hdutype == IMAGE_HDU &&
		naxis != 0 && totpix != 0 && fpvar.do_images) {

		/* rescale a scaled integer image to reduce noise? */
		if (fpvar.rescale_noise != 0. && bitpix > 0 && bitpix < LONGLONG_IMG) {

		   tstatus = 0;
		   fits_read_key(inputfptr, TDOUBLE, "BSCALE", &bscale, 0, &tstatus);

		   if (tstatus == 0 && bscale != 1.0) {  /* image must be scaled */

			if (bitpix == LONG_IMG)
			  fp_i4stat(inputfptr, naxis, naxes, &imagestats, &stat);
			else
			  fp_i2stat(inputfptr, naxis, naxes, &imagestats, &stat);

			/* use the minimum of the MAD 2nd, 3rd, and 5th order noise estimates */
			noisemin = imagestats.noise3;
			if (imagestats.noise2 != 0. && imagestats.noise2 < noisemin) noisemin = imagestats.noise2;
			if (imagestats.noise5 != 0. && imagestats.noise5 < noisemin) noisemin = imagestats.noise5;

			rescale = noisemin / fpvar.rescale_noise;
			if (rescale > 1.0) {
			  
			  /* all the criteria are met, so create a temporary file that */
			  /* contains a rescaled version of the image, in CWD */
			  
                	  /* create temporary file name */
			  fp_tmpnam("Tmpfile3", "", tempfilename3);

			  fits_create_file(&tempfile, tempfilename3, &stat);

			  fits_get_hdu_num(inputfptr, &hdunum);
			  if (hdunum != 1) {

			     /* the input hdu is an image extension, so create dummy primary */
			     fits_create_img(tempfile, 8, 0, naxes, &stat);
			  }

			  fits_copy_header(inputfptr, tempfile, &stat); /* copy the header */
			  
			  /* rescale the data, so that it will compress more efficiently */
			  if (bitpix == LONG_IMG)
			    fp_i4rescale(inputfptr, naxis, naxes, rescale, tempfile, &stat);
			  else
			    fp_i2rescale(inputfptr, naxis, naxes, rescale, tempfile, &stat);

			  /* scale the BSCALE keyword by the inverse factor */

			  bscale = bscale * rescale;  
			  fits_update_key(tempfile, TDOUBLE, "BSCALE", &bscale, 0, &stat);

			  /* rescan the header, to reset the actual scaling parameters */
			  fits_set_hdustruc(tempfile, &stat);

			  infptr = tempfile;
			  rescale_flag = 1;
			}
		   }
		 }

		if (!rescale_flag)   /* just compress the input file, without rescaling */
		   infptr = inputfptr;

		/* compute basic statistics about the input image */
                if (bitpix == BYTE_IMG) {
		   bpix = 8;
		   strcpy(dtype, "8  ");
		   fp_i2stat(infptr, naxis, naxes, &imagestats, &stat);
		} else if (bitpix == SHORT_IMG) {
		   bpix = 16;
		   strcpy(dtype, "16 ");
		   fp_i2stat(infptr, naxis, naxes, &imagestats, &stat);
		} else if (bitpix == LONG_IMG) {
		   bpix = 32;
		   strcpy(dtype, "32 ");
		   fp_i4stat(infptr, naxis, naxes, &imagestats, &stat);
		} else if (bitpix == LONGLONG_IMG) {
		   bpix = 64;
		   strcpy(dtype, "64 ");
		} else if (bitpix == FLOAT_IMG)   {
		   bpix = 32;
		   strcpy(dtype, "-32");
		   fp_r4stat(infptr, naxis, naxes, &imagestats, &stat);
		} else if (bitpix == DOUBLE_IMG)  {
		   bpix = 64;
		   strcpy(dtype, "-64");
		   fp_r4stat(infptr, naxis, naxes, &imagestats, &stat);
		}

		/* use the minimum of the MAD 2nd, 3rd, and 5th order noise estimates */
		noisemin = imagestats.noise3;
		if (imagestats.noise2 != 0. && imagestats.noise2 < noisemin) noisemin = imagestats.noise2;
		if (imagestats.noise5 != 0. && imagestats.noise5 < noisemin) noisemin = imagestats.noise5;

                xbits = (float) (log10(noisemin)/.301 + 1.792);

		printf("\n File: %s\n", infits);
		printf("  Ext BITPIX Dimens.   Nulls    Min    Max     Mean    Sigma  Noise2  Noise3  Noise5  Nbits   MaxR\n");

		printf("  %3d  %s", extnum, dtype);
		sprintf(dimen," (%ld", naxes[0]);
		len =strlen(dimen);
		for (ii = 1; ii < naxis; ii++) {
		    sprintf(dimen+len,",%ld", naxes[ii]);
		    len =strlen(dimen);
		}
		strcat(dimen, ")");
		printf("%-12s",dimen);

		fits_get_hduaddr(inputfptr, &headstart, &datastart, &dataend, &stat);
		origdata = (float) ((dataend - datastart)/1000000.);

		/* get elapsed and cpu times need to read the uncompressed image */
		fits_read_image_speed (infptr, &whole_elapse, &whole_cpu,
		               &row_elapse, &row_cpu, &stat);

		printf(" %5d %6.0f %6.0f %8.1f %#8.2g %#7.3g %#7.3g %#7.3g %#5.1f %#6.2f\n",
		        imagestats.n_nulls, imagestats.minval, imagestats.maxval, 
		      imagestats.mean, imagestats.sigma, 
		      imagestats.noise2, imagestats.noise3, imagestats.noise5, xbits, bpix/xbits);

		printf("\n       Type   Ratio       Size (MB)     Pk (Sec) UnPk Exact ElpN CPUN  Elp1  CPU1\n");

		printf("       Native                                             %5.3f %5.3f %5.3f %5.3f\n",
		        whole_elapse, whole_cpu, row_elapse, row_cpu);

		if (fpvar.outfile[0]) {
		    fprintf(outreport,
	" %s  %d %d %ld %ld %#10.4g %d %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g %#10.4g",
		      infits, extnum, bitpix, naxes[0], naxes[1], origdata, imagestats.n_nulls, imagestats.minval, 
		      imagestats.maxval, imagestats.mean, imagestats.sigma, 
		      imagestats.noise1, imagestats.noise2, imagestats.noise3, imagestats.noise5, whole_elapse, whole_cpu, row_elapse, row_cpu);
		}

		fits_set_lossy_int (outfptr, fpvar.int_to_float, &stat);
		if ( (bitpix > 0) && (fpvar.int_to_float != 0) ) {

			if ( (noisemin < (fpvar.n3ratio * fpvar.quantize_level) ) ||
			    (noisemin < fpvar.n3min)) {
			
			    /* image contains too little noise to quantize effectively */
			    fits_set_lossy_int (outfptr, 0, &stat);
			    fits_get_hdu_num(infptr, &hdunum);

printf("    HDU %d does not meet noise criteria to be quantized, so losslessly compressed.\n", hdunum);
			} 
		}

		/* test compression ratio and speed for each algorithm */

		if (fpvar.quantize_level != 0) {

		  fits_set_compression_type (outfptr, RICE_1, &stat);
		  fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);
		  if (fpvar.no_dither)
	    	    fits_set_quantize_method(outfptr, -1, &stat);
		  else
	    	    fits_set_quantize_method(outfptr, fpvar.dither_method, &stat);

		  fits_set_quantize_level (outfptr, fpvar.quantize_level, &stat);
		  fits_set_dither_offset(outfptr, fpvar.dither_offset, &stat);
		  fits_set_hcomp_scale (outfptr, fpvar.scale, &stat);
		  fits_set_hcomp_smooth (outfptr, fpvar.smooth, &stat);

		  fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);
		}

		if (fpvar.quantize_level != 0) {
\
  		  fits_set_compression_type (outfptr, HCOMPRESS_1, &stat);
		  fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);

		  if (fpvar.no_dither)
	    	    fits_set_quantize_method(outfptr, -1, &stat);
		  else
	    	    fits_set_quantize_method(outfptr, fpvar.dither_method, &stat);

		  fits_set_quantize_level (outfptr, fpvar.quantize_level, &stat);
		  fits_set_dither_offset(outfptr, fpvar.dither_offset, &stat);
		  fits_set_hcomp_scale (outfptr, fpvar.scale, &stat);
		  fits_set_hcomp_smooth (outfptr, fpvar.smooth, &stat);

		  fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);
		}

		if (fpvar.comptype == GZIP_2) {
		    fits_set_compression_type (outfptr, GZIP_2, &stat);
		} else {
		    fits_set_compression_type (outfptr, GZIP_1, &stat);
		}

		fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);

		if (fpvar.no_dither)
	    	    fits_set_quantize_method(outfptr, -1, &stat);
		else
	    	    fits_set_quantize_method(outfptr, fpvar.dither_method, &stat);

		fits_set_quantize_level (outfptr, fpvar.quantize_level, &stat);
		fits_set_dither_offset(outfptr, fpvar.dither_offset, &stat);
		fits_set_hcomp_scale (outfptr, fpvar.scale, &stat);
		fits_set_hcomp_smooth (outfptr, fpvar.smooth, &stat);

		fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);

/*
		fits_set_compression_type (outfptr, BZIP2_1, &stat);
		fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);
		fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);
*/
/*
		fits_set_compression_type (outfptr, PLIO_1, &stat);
		fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);
		fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);
*/
/*
                if (bitpix == SHORT_IMG || bitpix == LONG_IMG) {
		  fits_set_compression_type (outfptr, NOCOMPRESS, &stat);
		  fits_set_tile_dim (outfptr, 6, fpvar.ntile, &stat);
		  fp_test_hdu(infptr, outfptr, outfptr2, fpvar, &stat);	  
		}
*/
		if (fpvar.outfile[0])
		    fprintf(outreport,"\n");

		/* delete the temporary file */
		if (rescale_flag)  { 
		    fits_delete_file (infptr, &stat);
		    tempfilename3[0] = '\0';   /* clear the temp filename */ 
                }
	    } else if ( (hdutype == BINARY_TBL) && fpvar.do_tables) {

    		fits_get_num_rowsll(inputfptr, &nrows, &stat);
    		fits_get_num_cols(inputfptr, &ncols, &stat);
#if defined(_MSC_VER)
                /* Microsoft Visual C++ 6.0 uses '%I64d' syntax  for 8-byte integers */
 		printf("\n File: %s, HDU %d,  %d cols X %I64d rows\n", infits, extnum, ncols, nrows);
#elif (USE_LL_SUFFIX == 1)
 		printf("\n File: %s, HDU %d,  %d cols X %lld rows\n", infits, extnum, ncols, nrows);
#else
 		printf("\n File: %s, HDU %d,  %d cols X %ld rows\n", infits, extnum, ncols, nrows);
#endif
		fp_test_table(inputfptr, outfptr, outfptr2, fpvar, &stat);	  

	    } else {
		fits_copy_hdu (inputfptr, outfptr, 0, &stat);
		fits_copy_hdu (inputfptr, outfptr2, 0, &stat);
	    }

	    fits_movrel_hdu (inputfptr, 1, NULL, &stat);
            extnum++;
	}


	if (stat == END_OF_FILE) stat = 0;

	fits_close_file (outfptr2, &stat);
	fits_close_file (outfptr, &stat);
	fits_close_file (inputfptr, &stat);

	if (stat) {
	  fits_report_error (stderr, stat);
	}
	return(0);
}
/*--------------------------------------------------------------------------*/
int fp_pack_hdu (fitsfile *infptr, fitsfile *outfptr, fpstate fpvar,
   int *islossless, int *status)
{
	fitsfile *tempfile;
	long	naxes[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	int	stat=0, totpix=0, naxis=0, ii, hdutype, bitpix;
	int	tstatus, hdunum;
	double  bscale, rescale;

	char	outfits[SZ_STR], fzalgor[FLEN_VALUE];
	long 	headstart, datastart, dataend, datasize;
	double  noisemin;
	/* structure to hold image statistics (defined in fpack.h) */
	imgstats imagestats;

	if (*status) return(0);

	fits_get_hdu_type (infptr, &hdutype, &stat);

	if (hdutype == IMAGE_HDU) {
	    fits_get_img_param (infptr, 9, &bitpix, &naxis, naxes, &stat);
	    for (totpix=1, ii=0; ii < 9; ii++) totpix *= naxes[ii];
	}

	/* check directive keyword to see if this HDU should not be compressed */
        tstatus = 0;
        if (!fits_read_key(infptr, TSTRING, "FZALGOR", fzalgor, NULL, &tstatus) ) {
	    if (!strcmp(fzalgor, "NONE") || !strcmp(fzalgor, "none") ) {
 	        fits_copy_hdu (infptr, outfptr, 0, &stat);

	        *status = stat;
	        return(0);
            }
	}

        /* =============================================================== */
        /* This block is only for  binary table compression */
	if (hdutype == BINARY_TBL && fpvar.do_tables) { 

	    fits_get_hduaddr(infptr, &headstart, &datastart, &dataend, status); 
	    datasize = dataend - datastart;

	    if (datasize <= 2880) {
		/* data is less than 1 FITS block in size, so don't compress */
	        fits_copy_hdu (infptr, outfptr, 0, &stat);
	    } else {
		    fits_compress_table (infptr, outfptr, &stat);
	    }

	    *status = stat;
	    return(0);
	}
        /* =============================================================== */

        /* If this is not a non-null image HDU, just copy it verbatim */
	if (fits_is_compressed_image (infptr,  &stat) || hdutype != IMAGE_HDU ||
	    naxis == 0 || totpix == 0 || !fpvar.do_images) {
	        fits_copy_hdu (infptr, outfptr, 0, &stat);

	} else {  /* remaining code deals only with IMAGE HDUs */

		/* special case: rescale a scaled integer image to reduce noise? */
		if (fpvar.rescale_noise != 0. && bitpix > 0 && bitpix < LONGLONG_IMG) {

		   tstatus = 0;
		   fits_read_key(infptr, TDOUBLE, "BSCALE", &bscale, 0, &tstatus);
		   if (tstatus == 0 && bscale != 1.0) {  /* image must be scaled */

			if (bitpix == LONG_IMG)
			  fp_i4stat(infptr, naxis, naxes, &imagestats, &stat);
			else
			  fp_i2stat(infptr, naxis, naxes, &imagestats, &stat);

			/* use the minimum of the MAD 2nd, 3rd, and 5th order noise estimates */
			noisemin = imagestats.noise3;
			if (imagestats.noise2 != 0. && imagestats.noise2 < noisemin) noisemin = imagestats.noise2;
			if (imagestats.noise5 != 0. && imagestats.noise5 < noisemin) noisemin = imagestats.noise5;

			rescale = noisemin / fpvar.rescale_noise;
			if (rescale > 1.0) {
			  
			  /* all the criteria are met, so create a temporary file that */
			  /* contains a rescaled version of the image, in output directory */
			  
			  /* create temporary file name */
			  fits_file_name(outfptr, outfits, &stat);  /* get the output file name */
			  fp_tmpnam("Tmp3", outfits, tempfilename3);

			  fits_create_file(&tempfile, tempfilename3, &stat);

			  fits_get_hdu_num(infptr, &hdunum);
			  if (hdunum != 1) {

			     /* the input hdu is an image extension, so create dummy primary */
			     fits_create_img(tempfile, 8, 0, naxes, &stat);
			  }

			  fits_copy_header(infptr, tempfile, &stat); /* copy the header */
			  
			  /* rescale the data, so that it will compress more efficiently */
			  if (bitpix == LONG_IMG)
			    fp_i4rescale(infptr, naxis, naxes, rescale, tempfile, &stat);
			  else
			    fp_i2rescale(infptr, naxis, naxes, rescale, tempfile, &stat);


			  /* scale the BSCALE keyword by the inverse factor */

			  bscale = bscale * rescale;  
			  fits_update_key(tempfile, TDOUBLE, "BSCALE", &bscale, 0, &stat);

			  /* rescan the header, to reset the actual scaling parameters */
			  fits_set_hdustruc(tempfile, &stat);

		          fits_img_compress (tempfile, outfptr, &stat);
		          fits_delete_file  (tempfile, &stat);
		          tempfilename3[0] = '\0';   /* clear the temp filename */
		          *islossless = 0;  /* used a lossy compression method */

	                  *status = stat;
	                  return(0);
			}
		   }
		}

		/* if requested to do lossy compression of integer images (by */
		/* converting to float), then check if this HDU qualifies */
		if ( (bitpix > 0) && (fpvar.int_to_float != 0) ) {
		    
			if (bitpix >= LONG_IMG)
			  fp_i4stat(infptr, naxis, naxes, &imagestats, &stat);
			else
			  fp_i2stat(infptr, naxis, naxes, &imagestats, &stat);

			/* rescan the image header to reset scaling values (changed by fp_iNstat) */
			ffrhdu(infptr, &hdutype, &stat);

			/* use the minimum of the MAD 2nd, 3rd, and 5th order noise estimates */
			noisemin = imagestats.noise3;
			if (imagestats.noise2 != 0. && imagestats.noise2 < noisemin) noisemin = imagestats.noise2;
			if (imagestats.noise5 != 0. && imagestats.noise5 < noisemin) noisemin = imagestats.noise5;

			if ( (noisemin < (fpvar.n3ratio * fpvar.quantize_level) ) ||
			    (imagestats.noise3 < fpvar.n3min)) {
			
			    /* image contains too little noise to quantize effectively */
			    fits_set_lossy_int (outfptr, 0, &stat);

			    fits_get_hdu_num(infptr, &hdunum);

printf("    HDU %d does not meet noise criteria to be quantized, so losslessly compressed.\n", hdunum);

			} else {
			    /* compressed image is not identical to original */
			    *islossless = 0;
			}  
		}

                /* finally, do the actual image compression */
		fits_img_compress (infptr, outfptr, &stat);

		if (bitpix < 0 || 
		    (fpvar.comptype == HCOMPRESS_1 && fpvar.scale != 0.)) {

		    /* compressed image is not identical to original */
		    *islossless = 0;  
		}
	}

	*status = stat;
	return(0);
}

/*--------------------------------------------------------------------------*/
int fp_unpack_hdu (fitsfile *infptr, fitsfile *outfptr, fpstate fpvar, int *status)
{
	int hdutype, lval;

        if (*status > 0) return(0);

	fits_get_hdu_type (infptr, &hdutype, status);

        /* =============================================================== */
        /* This block is only for beta testing of binary table compression */
	if (hdutype == BINARY_TBL) { 

	    fits_read_key(infptr, TLOGICAL, "ZTABLE", &lval, NULL, status);
	    
	    if (*status == 0 && lval != 0) {
	        /*  uncompress the table */
	        fits_uncompress_table (infptr, outfptr, status);
	    } else {
	        if (*status == KEY_NO_EXIST)  /* table is not compressed */
		    *status = 0;
                fits_copy_hdu (infptr, outfptr, 0, status);
            }

	    return(0);
        /* =============================================================== */

	} else if (fits_is_compressed_image (infptr,  status)) {
            /* uncompress the compressed image HDU */
            fits_img_decompress (infptr, outfptr, status);
        } else {
            /* not a compressed image HDU, so just copy it to the output */
            fits_copy_hdu (infptr, outfptr, 0, status);
        }

	return(0);
}
/*--------------------------------------------------------------------------*/
int fits_read_image_speed (fitsfile *infptr, float *whole_elapse, 
    float *whole_cpu, float *row_elapse, float *row_cpu, int *status)
{
        unsigned char *carray, cnull = 0;
	short *sarray, snull=0;
	int bitpix, naxis, anynull, *iarray, inull = 0;
	long ii, naxes[9], fpixel[9]={1,1,1,1,1,1,1,1,1}, lpixel[9]={1,1,1,1,1,1,1,1,1};
	long inc[9]={1,1,1,1,1,1,1,1,1} ;
	float *earray, enull = 0, filesize;
	double *darray, dnull = 0;
	
	if (*status) return(*status);

	fits_get_img_param (infptr, 9, &bitpix, &naxis, naxes, status);

	if (naxis != 2)return(*status);
	
	lpixel[0] = naxes[0];
	lpixel[1] = naxes[1];
	
        /* filesize in MB */
	filesize = (float) (naxes[0] * abs(bitpix) / 8000000. * naxes[1]);

	/* measure time required to read the raw image */
	fits_set_bscale(infptr, 1.0, 0.0, status);
	*whole_elapse = 0.;
	*whole_cpu = 0;

        if (bitpix == BYTE_IMG) {
		carray = calloc(naxes[1]*naxes[0], sizeof(char));

                /* remove any cached uncompressed tile 
		  (dangerous to directly modify the structure!) */
         /*       (infptr->Fptr)->tilerow = 0; */

		marktime(status);
		fits_read_subset(infptr, TBYTE, fpixel, lpixel, inc, &cnull, 
		      carray, &anynull, status);
		
		/* get elapsped times */
		gettime(whole_elapse, whole_cpu, status);

		/* now read the image again, row by row */
		if (row_elapse) {

                  /* remove any cached uncompressed tile 
	  	    (dangerous to directly modify the structure!) */
          /*        (infptr->Fptr)->tilerow = 0; */

		  marktime(status);
		  for (ii = 0; ii < naxes[1]; ii++) {
		   fpixel[1] = ii+1;
		   fits_read_pix(infptr, TBYTE, fpixel, naxes[0], &cnull, 
		      carray, &anynull, status);
		   }
		   /* get elapsped times */
		   gettime(row_elapse, row_cpu, status);
		}
 		free(carray);
 
	} else if (bitpix == SHORT_IMG) {
		sarray = calloc(naxes[0]*naxes[1], sizeof(short));

		marktime(status);
		fits_read_subset(infptr, TSHORT, fpixel, lpixel, inc, &snull, 
		      sarray, &anynull, status);

		gettime(whole_elapse, whole_cpu, status);   /* get elapsped times */

		/* now read the image again, row by row */
		if (row_elapse) {
		  marktime(status);
		  for (ii = 0; ii < naxes[1]; ii++) {

		   fpixel[1] = ii+1;
		   fits_read_pix(infptr, TSHORT, fpixel, naxes[0], &snull, 
		      sarray, &anynull, status);
		  }
		  /* get elapsped times */
		  gettime(row_elapse, row_cpu, status);
		}

		free(sarray);	

	} else if (bitpix == LONG_IMG) {
		iarray = calloc(naxes[0]*naxes[1], sizeof(int));

		marktime(status);

		fits_read_subset(infptr, TINT, fpixel, lpixel, inc, &inull, 
		      iarray, &anynull, status);
		
		/* get elapsped times */
		gettime(whole_elapse, whole_cpu, status);


		/* now read the image again, row by row */
		if (row_elapse) {
		  marktime(status);
		  for (ii = 0; ii < naxes[1]; ii++) {
		   fpixel[1] = ii+1;
		   fits_read_pix(infptr, TINT, fpixel, naxes[0], &inull, 
		      iarray, &anynull, status);
		  }
		  /* get elapsped times */
		  gettime(row_elapse, row_cpu, status);
		}


 		free(iarray);	

	} else if (bitpix == FLOAT_IMG)   {
		earray = calloc(naxes[1]*naxes[0], sizeof(float));

		marktime(status);

		fits_read_subset(infptr, TFLOAT, fpixel, lpixel, inc, &enull, 
		      earray, &anynull, status);
		
		/* get elapsped times */
		gettime(whole_elapse, whole_cpu, status);

		/* now read the image again, row by row */
		if (row_elapse) {
		  marktime(status);
		  for (ii = 0; ii < naxes[1]; ii++) {
		   fpixel[1] = ii+1;
		   fits_read_pix(infptr, TFLOAT, fpixel, naxes[0], &enull, 
		      earray, &anynull, status);
		  }
		  /* get elapsped times */
		  gettime(row_elapse, row_cpu, status);
		}

 		free(earray);	

	} else if (bitpix == DOUBLE_IMG)  {
		darray = calloc(naxes[1]*naxes[0], sizeof(double));

		marktime(status);

		fits_read_subset(infptr, TDOUBLE, fpixel, lpixel, inc, &dnull, 
		      darray, &anynull, status);
		
		/* get elapsped times */
		gettime(whole_elapse, whole_cpu, status);

		/* now read the image again, row by row */
		if (row_elapse) {
		  marktime(status);
		  for (ii = 0; ii < naxes[1]; ii++) {
		   fpixel[1] = ii+1;
		   fits_read_pix(infptr, TDOUBLE, fpixel, naxes[0], &dnull, 
		      darray, &anynull, status);
		  }
		  /* get elapsped times */
		  gettime(row_elapse, row_cpu, status);
		}

		free(darray);
	}

        if (whole_elapse) *whole_elapse = *whole_elapse / filesize;
        if (row_elapse)   *row_elapse   = *row_elapse / filesize;
        if (whole_cpu)    *whole_cpu    = *whole_cpu / filesize;
        if (row_cpu)      *row_cpu      = *row_cpu / filesize;

	return(*status);
}
/*--------------------------------------------------------------------------*/
int fp_test_hdu (fitsfile *infptr, fitsfile *outfptr, fitsfile *outfptr2, 
	fpstate fpvar, int *status)
{
   /*   This routine is only used for performance testing of image HDUs. */
   /*   Use fp_test_table for testing binary table HDUs.    */

	int stat = 0, hdutype, comptype;
        char ctype[20], lossless[4];
	long headstart, datastart, dataend;
	float origdata = 0., compressdata = 0.;
	float compratio = 0., packcpu = 0., unpackcpu = 0.;
	float elapse, whole_elapse, row_elapse, whole_cpu, row_cpu;
	unsigned long datasum1, datasum2, hdusum;

	if (*status) return(0);

	origdata = 0;
	compressdata = 0;
	compratio = 0.;
	lossless[0] = '\0';

	fits_get_compression_type(outfptr, &comptype, &stat);
	if (comptype == RICE_1)
		strcpy(ctype, "RICE");
	else if (comptype == GZIP_1)
		strcpy(ctype, "GZIP1");
	else if (comptype == GZIP_2)
		strcpy(ctype, "GZIP2");/*
	else if (comptype == BZIP2_1)
		strcpy(ctype, "BZIP2");
*/
	else if (comptype == PLIO_1)
		strcpy(ctype, "PLIO");
	else if (comptype == HCOMPRESS_1)
		strcpy(ctype, "HCOMP");
	else if (comptype == NOCOMPRESS)
		strcpy(ctype, "NONE");
	else {
	      fp_msg ("Error: unsupported image compression type ");
	      *status = DATA_COMPRESSION_ERR;
	      return(0);
	}

	/* -------------- COMPRESS the image ------------------ */

	marktime(&stat);
	
	fits_img_compress (infptr, outfptr, &stat);

	/* get elapsped times */
	gettime(&elapse, &packcpu, &stat);

	/* get elapsed and cpu times need to read the compressed image */
	fits_read_image_speed (outfptr, &whole_elapse, &whole_cpu, 
	   &row_elapse, &row_cpu, &stat);

        if (!stat) {

		/* -------------- UNCOMPRESS the image ------------------ */

                /* remove any cached uncompressed tile 
		  (dangerous to directly modify the structure!) */
        /*        (outfptr->Fptr)->tilerow = 0; */
		marktime(&stat);

 	       fits_img_decompress (outfptr, outfptr2, &stat);

		/* get elapsped times */
		gettime(&elapse, &unpackcpu, &stat);

		/* ----------------------------------------------------- */


		/* get sizes of original and compressed images */

		fits_get_hduaddr(infptr, &headstart, &datastart, &dataend, &stat);
		origdata = (float) ((dataend - datastart)/1000000.);
		
		fits_get_hduaddr(outfptr, &headstart, &datastart, &dataend, &stat);
		compressdata = (float) ((dataend - datastart)/1000000.);

		if (compressdata != 0)
			compratio = (float) origdata / (float) compressdata;

		/* is this uncompressed image identical to the original? */

		fits_get_chksum(infptr, &datasum1, &hdusum, &stat);	    
		fits_get_chksum(outfptr2, &datasum2, &hdusum, &stat);

		if ( datasum1 == datasum2) {
			strcpy(lossless, "Yes");
		} else {
			strcpy(lossless, "No");
		}

		printf("       %-5s %6.2f %7.2f ->%7.2f %7.2f %7.2f %s %5.3f %5.3f %5.3f %5.3f\n", 
			ctype, compratio, origdata, compressdata, 
			packcpu, unpackcpu, lossless, whole_elapse, whole_cpu,
			row_elapse, row_cpu);


		if (fpvar.outfile[0]) {
		    fprintf(outreport," %6.3f %5.2f %5.2f %s %7.3f %7.3f %7.3f %7.3f", 
		       compratio, packcpu, unpackcpu, lossless,  whole_elapse, whole_cpu,
		       row_elapse, row_cpu);
		}

 	       /* delete the output HDUs to concerve disk space */

		fits_delete_hdu(outfptr, &hdutype, &stat);
		fits_delete_hdu(outfptr2, &hdutype, &stat);

	} else {
		printf("       %-5s     (unable to compress image)\n", ctype);
	}

	/* try to recover from any compression errors */
	if (stat == DATA_COMPRESSION_ERR) stat = 0;

	*status = stat;
        return(0);
}
/*--------------------------------------------------------------------------*/
int fp_test_table (fitsfile *infptr, fitsfile *outfptr, fitsfile *outfptr2, 
	fpstate fpvar, int *status)
{
/* this routine is for performance testing of the table compression methods */

	int stat = 0, hdutype, tstatus = 0;
        char fzalgor[FLEN_VALUE];
	LONGLONG headstart, datastart, dataend;
	float elapse, cpu;

	if (*status) return(0);

	/* check directive keyword to see if this HDU should not be compressed */
        if (!fits_read_key(infptr, TSTRING, "FZALGOR", fzalgor, NULL, &tstatus) ) {
	    if (!strcmp(fzalgor, "NONE")  || !strcmp(fzalgor, "none")) {
		return(0);
            }
	}

        fits_get_hduaddrll(infptr, &headstart, &datastart, &dataend, status); 

	/* can't compress small tables with less than 2880 bytes of data */
        if (dataend - datastart <= 2880) {
		return(0);
	}

 	marktime(&stat);
        stat= -999;  /* set special flag value */
	fits_compress_table (infptr, outfptr,  &stat);

	/* get elapsped times */
	gettime(&elapse, &cpu, &stat);

	fits_delete_hdu(outfptr, &hdutype, &stat);

	printf("\nElapsed time = %f, cpu = %f\n", elapse, cpu);

	fits_report_error (stderr, stat);

        return(0);
}
/*--------------------------------------------------------------------------*/
int marktime(int *status)
{
#if defined(unix) || defined(__unix__)  || defined(__unix)
        struct  timeval tv;
/*        struct  timezone tz; */

/*        gettimeofday (&tv, &tz); */
        gettimeofday (&tv, NULL);

	startsec = tv.tv_sec;
        startmilli = tv.tv_usec/1000;

        scpu = clock();
#else
/* don't support high timing precision on Windows machines */
     	startsec = 0;
        startmilli = 0;

        scpu = clock();
#endif
	return( *status );
}
/*--------------------------------------------------------------------------*/
int gettime(float *elapse, float *elapscpu, int *status)
{
#if defined(unix) || defined(__unix__)  || defined(__unix)
        struct  timeval tv;
/*        struct  timezone tz; */
	int stopmilli;
	long stopsec;

/*        gettimeofday (&tv, &tz); */
        gettimeofday (&tv, NULL);
	ecpu = clock();

        stopmilli = tv.tv_usec/1000;
	stopsec = tv.tv_sec;
	
	*elapse = (stopsec - startsec) + (stopmilli - startmilli)/1000.;
	*elapscpu = (ecpu - scpu) * 1.0 / CLOCKTICKS;
/*
printf(" (start: %ld + %d), stop: (%ld + %d) elapse: %f\n ",
startsec,startmilli,stopsec, stopmilli, *elapse);
*/
#else
/* set the elapsed time the same as the CPU time on Windows machines */
	*elapscpu = (float) ((ecpu - scpu) * 1.0 / CLOCKTICKS);
	*elapse = *elapscpu;  
#endif
	return( *status );
}
/*--------------------------------------------------------------------------*/
int fp_i2stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status)
{
/*
    read the central XSAMPLE by YSAMPLE region of pixels in the int*2 image, 
    and then compute basic statistics: min, max, mean, sigma, mean diff, etc.
*/

	long fpixel[9] = {1,1,1,1,1,1,1,1,1};
	long lpixel[9] = {1,1,1,1,1,1,1,1,1};
	long inc[9]    = {1,1,1,1,1,1,1,1,1};
	long i1, i2, npix, ngood, nx, ny;
	short *intarray, minvalue, maxvalue, nullvalue;
	int anynul, tstatus, checknull = 1;
	double mean, sigma, noise1, noise2, noise3, noise5;
	
         /* select the middle XSAMPLE by YSAMPLE area of the image */
	i1 = naxes[0]/2 - (XSAMPLE/2 - 1);
	i2 = naxes[0]/2 + (XSAMPLE/2);
	if (i1 < 1) i1 = 1;
	if (i2 > naxes[0]) i2 = naxes[0];
	fpixel[0] = i1;
	lpixel[0] = i2;
	nx = i2 - i1 +1;

        if (naxis > 1) {
	    i1 = naxes[1]/2 - (YSAMPLE/2 - 1);
	    i2 = naxes[1]/2 + (YSAMPLE/2);
	    if (i1 < 1) i1 = 1;
	    if (i2 > naxes[1]) i2 = naxes[1];
	    fpixel[1] = i1;
	    lpixel[1] = i2;
	}
	ny = i2 - i1 +1;

	npix = nx * ny;

        /* if there are higher dimensions, read the middle plane of the cube */
	if (naxis > 2) {
	    fpixel[2] = naxes[2]/2 + 1;
	    lpixel[2] = naxes[2]/2 + 1;
	}

	intarray = calloc(npix, sizeof(short));
	if (!intarray) {
		*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* turn off any scaling of the integer pixel values */
	fits_set_bscale(infptr, 1.0, 0.0, status);

	fits_read_subset_sht(infptr, 0, naxis, naxes, fpixel, lpixel, inc,
	    0, intarray, &anynul, status);

	/* read the null value keyword (BLANK) if present */
	tstatus = 0;
	fits_read_key(infptr, TSHORT, "BLANK", &nullvalue, 0, &tstatus);
	if (tstatus) {
	   nullvalue = 0;
	   checknull = 0;
	}

	/* compute statistics of the image */

        fits_img_stats_short(intarray, nx, ny, checknull, nullvalue, 
	&ngood, &minvalue, &maxvalue, &mean, &sigma, &noise1, &noise2, &noise3, &noise5, status);

	imagestats->n_nulls = npix - ngood;
	imagestats->minval = minvalue;
	imagestats->maxval = maxvalue; 
	imagestats->mean = mean; 
	imagestats->sigma = sigma; 
	imagestats->noise1 = noise1; 
	imagestats->noise2 = noise2; 
	imagestats->noise3 = noise3; 
	imagestats->noise5 = noise5; 
    
	free(intarray);
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fp_i4stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status)
{
/*
    read the central XSAMPLE by YSAMPLE region of pixels in the int*2 image, 
    and then compute basic statistics: min, max, mean, sigma, mean diff, etc.
*/

	long fpixel[9] = {1,1,1,1,1,1,1,1,1};
	long lpixel[9] = {1,1,1,1,1,1,1,1,1};
	long inc[9]    = {1,1,1,1,1,1,1,1,1};
	long i1, i2, npix, ngood, nx, ny;
	int *intarray, minvalue, maxvalue, nullvalue;
	int anynul, tstatus, checknull = 1;
	double mean, sigma, noise1, noise2, noise3, noise5;
	
         /* select the middle XSAMPLE by YSAMPLE area of the image */
	i1 = naxes[0]/2 - (XSAMPLE/2 - 1);
	i2 = naxes[0]/2 + (XSAMPLE/2);
	if (i1 < 1) i1 = 1;
	if (i2 > naxes[0]) i2 = naxes[0];
	fpixel[0] = i1;
	lpixel[0] = i2;
	nx = i2 - i1 +1;

        if (naxis > 1) {
	    i1 = naxes[1]/2 - (YSAMPLE/2 - 1);
	    i2 = naxes[1]/2 + (YSAMPLE/2);
	    if (i1 < 1) i1 = 1;
	    if (i2 > naxes[1]) i2 = naxes[1];
	    fpixel[1] = i1;
	    lpixel[1] = i2;
	}
	ny = i2 - i1 +1;

	npix = nx * ny;

        /* if there are higher dimensions, read the middle plane of the cube */
	if (naxis > 2) {
	    fpixel[2] = naxes[2]/2 + 1;
	    lpixel[2] = naxes[2]/2 + 1;
	}

	intarray = calloc(npix, sizeof(int));
	if (!intarray) {
		*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* turn off any scaling of the integer pixel values */
	fits_set_bscale(infptr, 1.0, 0.0, status);

	fits_read_subset_int(infptr, 0, naxis, naxes, fpixel, lpixel, inc,
	    0, intarray, &anynul, status);

	/* read the null value keyword (BLANK) if present */
	tstatus = 0;
	fits_read_key(infptr, TINT, "BLANK", &nullvalue, 0, &tstatus);
	if (tstatus) {
	   nullvalue = 0;
	   checknull = 0;
	}

	/* compute statistics of the image */

        fits_img_stats_int(intarray, nx, ny, checknull, nullvalue, 
	&ngood, &minvalue, &maxvalue, &mean, &sigma, &noise1, &noise2, &noise3, &noise5, status);

	imagestats->n_nulls = npix - ngood;
	imagestats->minval = minvalue;
	imagestats->maxval = maxvalue; 
	imagestats->mean = mean; 
	imagestats->sigma = sigma; 
	imagestats->noise1 = noise1; 
	imagestats->noise2 = noise2; 
	imagestats->noise3 = noise3; 
	imagestats->noise5 = noise5; 
    
	free(intarray);
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fp_r4stat(fitsfile *infptr, int naxis, long *naxes, imgstats *imagestats, int *status)
{
/*
    read the central XSAMPLE by YSAMPLE region of pixels in the int*2 image, 
    and then compute basic statistics: min, max, mean, sigma, mean diff, etc.
*/

	long fpixel[9] = {1,1,1,1,1,1,1,1,1};
	long lpixel[9] = {1,1,1,1,1,1,1,1,1};
	long inc[9]    = {1,1,1,1,1,1,1,1,1};
	long i1, i2, npix, ngood, nx, ny;
	float *array, minvalue, maxvalue, nullvalue = FLOATNULLVALUE;
	int anynul,checknull = 1;
	double mean, sigma, noise1, noise2, noise3, noise5;
	
         /* select the middle XSAMPLE by YSAMPLE area of the image */
	i1 = naxes[0]/2 - (XSAMPLE/2 - 1);
	i2 = naxes[0]/2 + (XSAMPLE/2);
	if (i1 < 1) i1 = 1;
	if (i2 > naxes[0]) i2 = naxes[0];
	fpixel[0] = i1;
	lpixel[0] = i2;
	nx = i2 - i1 +1;

        if (naxis > 1) {
	    i1 = naxes[1]/2 - (YSAMPLE/2 - 1);
	    i2 = naxes[1]/2 + (YSAMPLE/2);
	    if (i1 < 1) i1 = 1;
	    if (i2 > naxes[1]) i2 = naxes[1];
	    fpixel[1] = i1;
	    lpixel[1] = i2;
	}
	ny = i2 - i1 +1;

	npix = nx * ny;

        /* if there are higher dimensions, read the middle plane of the cube */
	if (naxis > 2) {
	    fpixel[2] = naxes[2]/2 + 1;
	    lpixel[2] = naxes[2]/2 + 1;
	}

	array = calloc(npix, sizeof(float));
	if (!array) {
		*status = MEMORY_ALLOCATION;
		return(*status);
	}

	fits_read_subset_flt(infptr, 0, naxis, naxes, fpixel, lpixel, inc,
	    nullvalue, array, &anynul, status);

	/* are there any null values in the array? */
	if (!anynul) {
	   nullvalue = 0.;
	   checknull = 0;
	}

	/* compute statistics of the image */

        fits_img_stats_float(array, nx, ny, checknull, nullvalue, 
	&ngood, &minvalue, &maxvalue, &mean, &sigma, &noise1, &noise2, &noise3, &noise5, status);

	imagestats->n_nulls = npix - ngood;
	imagestats->minval = minvalue;
	imagestats->maxval = maxvalue; 
	imagestats->mean = mean; 
	imagestats->sigma = sigma; 
	imagestats->noise1 = noise1; 
	imagestats->noise2 = noise2; 
	imagestats->noise3 = noise3; 
	imagestats->noise5 = noise5; 
    
	free(array);
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fp_i2rescale(fitsfile *infptr, int naxis, long *naxes, double rescale,
    fitsfile *outfptr, int *status)
{
/*
    divide the integer pixel values in the input file by rescale,
    and write back out to the output file..
*/

	long ii, jj, nelem = 1, nx, ny;
	short *intarray, nullvalue;
	int anynul, tstatus, checknull = 1;
	
	nx = naxes[0];
	ny = 1;
	
	for (ii = 1; ii < naxis; ii++) {
	    ny = ny * naxes[ii];
	}

	intarray = calloc(nx, sizeof(short));
	if (!intarray) {
		*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* read the null value keyword (BLANK) if present */
	tstatus = 0;
	fits_read_key(infptr, TSHORT, "BLANK", &nullvalue, 0, &tstatus);
	if (tstatus) {
	   checknull = 0;
	}

	/* turn off any scaling of the integer pixel values */
	fits_set_bscale(infptr,  1.0, 0.0, status);
	fits_set_bscale(outfptr, 1.0, 0.0, status);

	for (ii = 0; ii < ny; ii++) {

	    fits_read_img_sht(infptr, 1, nelem, nx,
	        0, intarray, &anynul, status);

	    if (checknull) {
	        for (jj = 0; jj < nx; jj++) {
	            if (intarray[jj] != nullvalue)
		        intarray[jj] = NSHRT( (intarray[jj] / rescale) );
		}
	    } else {
	        for (jj = 0; jj < nx; jj++)
		        intarray[jj] = NSHRT( (intarray[jj] / rescale) );
	    }

	    fits_write_img_sht(outfptr, 1, nelem, nx, intarray, status);
	      
	    nelem += nx;
	}

	free(intarray);
	return(*status);
}
/*--------------------------------------------------------------------------*/
int fp_i4rescale(fitsfile *infptr, int naxis, long *naxes, double rescale,
    fitsfile *outfptr, int *status)
{
/*
    divide the integer pixel values in the input file by rescale,
    and write back out to the output file..
*/

	long ii, jj, nelem = 1, nx, ny;
	int *intarray, nullvalue;
	int anynul, tstatus, checknull = 1;
	
	nx = naxes[0];
	ny = 1;
	
	for (ii = 1; ii < naxis; ii++) {
	    ny = ny * naxes[ii];
	}

	intarray = calloc(nx, sizeof(int));
	if (!intarray) {
		*status = MEMORY_ALLOCATION;
		return(*status);
	}

	/* read the null value keyword (BLANK) if present */
	tstatus = 0;
	fits_read_key(infptr, TINT, "BLANK", &nullvalue, 0, &tstatus);
	if (tstatus) {
	   checknull = 0;
	}

	/* turn off any scaling of the integer pixel values */
	fits_set_bscale(infptr,  1.0, 0.0, status);
	fits_set_bscale(outfptr, 1.0, 0.0, status);

	for (ii = 0; ii < ny; ii++) {

	    fits_read_img_int(infptr, 1, nelem, nx,
	        0, intarray, &anynul, status);

	    if (checknull) {
	        for (jj = 0; jj < nx; jj++) {
	            if (intarray[jj] != nullvalue)
		        intarray[jj] = NINT( (intarray[jj] / rescale) );
		}
	    } else {
	        for (jj = 0; jj < nx; jj++)
		        intarray[jj] = NINT( (intarray[jj] / rescale) );
	    }

	    fits_write_img_int(outfptr, 1, nelem, nx, intarray, status);
	      
	    nelem += nx;
	}

	free(intarray);
	return(*status);
}
/* ========================================================================
 * Signal and error handler.
 */
void abort_fpack(int sig)
{
     /* clean up by deleting temporary files */
     
      if (tempfilename[0]) {
         remove(tempfilename);
      }
      if (tempfilename2[0]) {
         remove(tempfilename2);
      }
      if (tempfilename3[0]) {
         remove(tempfilename3);
      }
      exit(-1);
}
