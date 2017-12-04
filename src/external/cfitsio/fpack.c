/* FPACK
 * R. Seaman, NOAO, with a few enhancements by W. Pence, HEASARC
 *
 * Calls fits_img_compress in the CFITSIO library by W. Pence, HEASARC
 */

#include <ctype.h>
/* #include <signal.h> */
#include "fitsio.h"
#include "fpack.h"

/* ================================================================== */
int main(int argc, char *argv[])
{
	fpstate	fpvar;

	if (argc <= 1) { fp_usage (); fp_hint (); exit (-1); }

	fp_init (&fpvar);
	fp_get_param (argc, argv, &fpvar);

	if (fpvar.listonly) {
	    fp_list (argc, argv, fpvar);

	} else {
	    fp_preflight (argc, argv, FPACK, &fpvar);
	    fp_loop (argc, argv, FPACK, fpvar);
	}

	exit (0);
}
/* ================================================================== */
int fp_get_param (int argc, char *argv[], fpstate *fpptr)
{
	int	gottype=0, gottile=0, wholetile=0, iarg, len, ndim, ii, doffset;
	char	tmp[SZ_STR], tile[SZ_STR];

        if (fpptr->initialized != FP_INIT_MAGIC) {
            fp_msg ("Error: internal initialization error\n"); exit (-1);
        }

	tile[0] = 0;

	/* flags must come first and be separately specified
	 */
	for (iarg = 1; iarg < argc; iarg++) {
	    if ((argv[iarg][0] == '-' && strlen (argv[iarg]) == 2) ||
	        !strncmp(argv[iarg], "-q", 2) || !strncmp(argv[iarg], "-qz", 3) ||
	        !strncmp(argv[iarg], "-g1", 3) || !strncmp(argv[iarg], "-g2", 3) ||
	        !strncmp(argv[iarg], "-i2f", 4) ||
	        !strncmp(argv[iarg], "-n3ratio", 8) || !strncmp(argv[iarg], "-n3min", 6) ||
	        !strncmp(argv[iarg], "-tableonly", 10) || !strncmp(argv[iarg], "-table", 6) )  
	    {

		/* Rice is the default, so -r is superfluous  */
		if (       argv[iarg][1] == 'r') {
		    fpptr->comptype = RICE_1;
		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;

		} else if (argv[iarg][1] == 'p') {
		    fpptr->comptype = PLIO_1;
		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;

		} else if (argv[iarg][1] == 'g') {
		    /* test for modifiers following the 'g' */
                    if (argv[iarg][2] == '2')
		        fpptr->comptype = GZIP_2;
		    else
		        fpptr->comptype = GZIP_1;

		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;
/*
		} else if (argv[iarg][1] == 'b') {
		    fpptr->comptype = BZIP2_1;
		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;
*/
		} else if (argv[iarg][1] == 'h') {
		    fpptr->comptype = HCOMPRESS_1;
		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;

		} else if (argv[iarg][1] == 'd') {
		    fpptr->comptype = NOCOMPRESS;
		    if (gottype) {
			fp_msg ("Error: multiple compression flags\n");
			fp_usage (); exit (-1);
		    } else
			gottype++;

		} else if (!strcmp(argv[iarg], "-i2f")) {
		    /* this means convert integer images to float, and then */
		    /* quantize and compress the float image.  This lossy */
		    /* compression method may give higher compression than the */
		    /* lossless compression method that is usually applied to */
		    /* integer images. */
		    
		    fpptr->int_to_float = 1;

		} else if (!strcmp(argv[iarg], "-n3ratio")) {
		    /* this is the minimum ratio between the MAD noise sigma */
		    /* and the q parameter value in the case where the integer */
		    /* image is quantized and compressed like a float image. */
		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else {
			fpptr->n3ratio = (float) atof (argv[iarg]);
		    }
		} else if (!strcmp(argv[iarg], "-n3min")) {
		    /* this is the minimum  MAD noise sigma in the case where the */
		    /* integer image is quantized and compressed like a float image. */
		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else {
			fpptr->n3min = (float) atof (argv[iarg]);
		    }
		} else if (argv[iarg][1] == 'q') {
		    /* test for modifiers following the 'q' */

                    if (argv[iarg][2] == 'z') {
		        fpptr->dither_method = 2;  /* preserve zero pixels */

                        if (argv[iarg][3] == 't') {
		            fpptr->dither_offset = -1;  /* dither based on tile checksum */

                        } else if (isdigit(argv[iarg][3])) { /* is a number appended to q? */
		           doffset = atoi(argv[iarg]+3);

                           if (doffset == 0) {
		              fpptr->no_dither = 1;  /* don't dither the quantized values */
		           } else if (doffset > 0 && doffset <= 10000) {
		              fpptr->dither_offset = doffset;
		           } else {
			      fp_msg ("Error: invalid q suffix\n");
			      fp_usage (); exit (-1);
		           }
			}
		    } else {
                        if (argv[iarg][2] == 't') {
		            fpptr->dither_offset = -1;  /* dither based on tile checksum */

                        } else if (isdigit(argv[iarg][2])) { /* is a number appended to q? */
		           doffset = atoi(argv[iarg]+2);

                           if (doffset == 0) {
		              fpptr->no_dither = 1;  /* don't dither the quantized values */
		           } else if (doffset > 0 && doffset <= 10000) {
		              fpptr->dither_offset = doffset;
		           } else {
			      fp_msg ("Error: invalid q suffix\n");
			      fp_usage (); exit (-1);
		           }
			}
		    }

		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else {
			fpptr->quantize_level = (float) atof (argv[iarg]);
		    }
		} else if (argv[iarg][1] == 'n') {
		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else {
			fpptr->rescale_noise = (float) atof (argv[iarg]);
		    }
		} else if (argv[iarg][1] == 's') {
		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else {
			fpptr->scale = (float) atof (argv[iarg]);
		    }
		} else if (!strcmp(argv[iarg], "-tableonly")) {
		    fpptr->do_tables = 1;
		    fpptr->do_images = 0;
		    fp_msg ("Note: -tableonly is intended for feasibility studies, not general use.\n");

		} else if (!strcmp(argv[iarg], "-table")) {
		    fpptr->do_tables = 1;
		    fp_msg ("Note: -table is intended for feasibility studies, not general use.\n");

		} else if (argv[iarg][1] == 't') {
		    if (gottile) {
			fp_msg ("Error: multiple tile specifications\n");
			fp_usage (); exit (-1);
		    } else
			gottile++;

		    if (++iarg >= argc) {
			fp_usage (); exit (-1);
		    } else
			strncpy (tile, argv[iarg], SZ_STR); /* checked below */

		} else if (argv[iarg][1] == 'v') {
		    fpptr->verbose = 1;

		} else if (argv[iarg][1] == 'w') {
		    wholetile++;
		    if (gottile) {
			fp_msg ("Error: multiple tile specifications\n");
			fp_usage (); exit (-1);
		    } else
			gottile++;

		} else if (argv[iarg][1] == 'F') {
		    fpptr->clobber++;       /* overwrite existing file */

		} else if (argv[iarg][1] == 'D') {
		    fpptr->delete_input++;

		} else if (argv[iarg][1] == 'Y') {
		    fpptr->do_not_prompt++;

		} else if (argv[iarg][1] == 'S') {
		    fpptr->to_stdout++;

		} else if (argv[iarg][1] == 'L') {
		    fpptr->listonly++;

		} else if (argv[iarg][1] == 'C') {
		    fpptr->do_checksums = 0;

		} else if (argv[iarg][1] == 'T') {
		    fpptr->test_all = 1;

		} else if (argv[iarg][1] == 'R') {
		    if (++iarg >= argc) {
			fp_usage (); fp_hint (); exit (-1);
		    } else
			strncpy (fpptr->outfile, argv[iarg], SZ_STR);

		} else if (argv[iarg][1] == 'H') {
		    fp_help (); exit (0);

		} else if (argv[iarg][1] == 'V') {
		    fp_version (); exit (0);

		} else {
		    fp_msg ("Error: unknown command line flag `");
		    fp_msg (argv[iarg]); fp_msg ("'\n");
		    fp_usage (); fp_hint (); exit (-1);
		}

	    } else
		break;
	}

	if (fpptr->scale != 0. && 
	         fpptr->comptype != HCOMPRESS_1 && fpptr->test_all != 1) {

	    fp_msg ("Error: `-s' requires `-h or -T'\n"); exit (-1);
	}

	if (fpptr->quantize_level == 0) {
	
	    if ((fpptr->comptype != GZIP_1) && (fpptr->comptype != GZIP_2)) {
	        fp_msg ("Error: `-q 0' only allowed with GZIP\n"); exit (-1);
	    }

            if (fpptr->int_to_float == 1) {
	        fp_msg ("Error: `-q 0' not allowed with -i2f\n"); exit (-1);
	    }
	}

	if (wholetile) {
	    for (ndim=0; ndim < MAX_COMPRESS_DIM; ndim++)
		fpptr->ntile[ndim] = (long) -1;

	} else if (gottile) {
	    len = strlen (tile);
	    for (ii=0, ndim=0; ii < len; ) {
		if (! (isdigit (tile[ii]) || tile[ii] == ',')) {
		    fp_msg ("Error: `-t' requires comma separated tile dims, ");
		    fp_msg ("e.g., `-t 100,100'\n"); exit (-1);
		}

		if (tile[ii] == ',') { ii++; continue; }

		fpptr->ntile[ndim] = atol (&tile[ii]);
		for ( ; isdigit(tile[ii]); ii++);

		if (++ndim > MAX_COMPRESS_DIM) {
		    fp_msg ("Error: too many dimensions for `-t', max=");
		    sprintf (tmp, "%d\n", MAX_COMPRESS_DIM); fp_msg (tmp);
		    exit (-1);
		}
	    }
	}

	if (iarg >= argc) {
	    fp_msg ("Error: no FITS files to compress\n");
	    fp_usage (); exit (-1);
	} else
	    fpptr->firstfile = iarg;

	return(0);
}

/* ================================================================== */
int fp_usage (void)
{
fp_msg ("usage: fpack ");
fp_msg (
"[-r|-h|-g|-p] [-w|-t <axes>] [-q <level>] [-s <scale>] [-n <noise>] -v <FITS>\n");
fp_msg ("more:   [-T] [-R] [-F] [-D] [-Y] [-S] [-L] [-C] [-H] [-V] [-i2f]\n");
return(0);
}

/* ================================================================== */
int fp_hint (void) 
{ fp_msg ("      `fpack -H' for help\n"); 
return(0);
}

/* ================================================================== */
int fp_help (void)
{
fp_msg ("fpack, a FITS image compression program.  Version ");
fp_version ();
fp_usage ();
fp_msg ("\n");
fp_msg ("NOTE: the compression parameters specified on the fpack command line may\n");
fp_msg ("be over-ridden by compression directive keywords in the header of each HDU\n");
fp_msg ("of the input file(s). See the fpack User's Guide for more details\n");
fp_msg ("\n");

fp_msg ("Flags must be separate and appear before filenames:\n");
fp_msg (" -r          Rice compression [default], or\n");
fp_msg (" -h          Hcompress compression, or\n");
fp_msg (" -g  or -g1  GZIP_1 (per-tile) compression, or\n");
fp_msg (" -g2         GZIP_2 (per-tile) compression (with byte shuffling), or\n");
/*
fp_msg (" -b          BZIP2 (per-tile) compression, or\n");
*/
fp_msg (" -p          PLIO compression (only for positive 8 or 16-bit integer images).\n");
fp_msg (" -d          Tile the image without compression (debugging mode).\n");

fp_msg (" -w          Compress the whole image as a single large tile.\n");
fp_msg (" -t <axes>   Comma separated list of tile dimensions [default is row by row].\n");

fp_msg (" -q <level>  Quantized level spacing when converting floating point images to\n");
fp_msg ("             scaled integers. (+value relative to sigma of background noise;\n");
fp_msg ("             -value is absolute). Default q value of 4 gives a compression ratio\n");
fp_msg ("             of about 6 with very high fidelity (only 0.26% increase in noise).\n");
fp_msg ("             Using q values of  2, or 1 will give compression ratios of\n");
fp_msg ("             about 8, or 10, respectively (with 1.0% or 4.1% noise increase).\n");
fp_msg ("             The scaled quantized values are randomly dithered using a seed \n");
fp_msg ("             value determined from the system clock at run time.\n");
fp_msg ("             Use -q0 instead of -q to suppress random dithering.\n");
fp_msg ("             Use -qz instead of -q to not dither zero-valued pixels.\n");
fp_msg ("             Use -qt or -qzt to compute random dithering seed from first tile checksum.\n");
fp_msg ("             Use -qN or -qzN, (N in range 1 to 10000) to use a specific dithering seed.\n");
fp_msg ("             Floating-point images can be losslessly compressed by selecting\n");
fp_msg ("             the GZIP algorithm and specifying -q 0, but this is slower and often\n");
fp_msg ("             produces much less compression than the default quantization method.\n");
fp_msg (" -i2f        Convert integer images to floating point, then quantize and compress\n");
fp_msg ("             using the specified q level.  When used appropriately, this lossy\n");
fp_msg ("             compression method can give much better compression than the normal\n");
fp_msg ("             lossless compression methods without significant loss of information.\n");
fp_msg ("             The -n3ratio and -n3min flags control the minimum noise thresholds;\n");
fp_msg ("             Images below these thresholds will be losslessly compressed.\n");
fp_msg (" -n3ratio    Minimum ratio of background noise sigma divided by q.  Default = 2.0.\n");
fp_msg (" -n3min      Minimum background noise sigma. Default = 6. The -i2f flag will be ignored\n");
fp_msg ("             if the noise level in the image does not exceed both thresholds.\n");
fp_msg (" -s <scale>  Scale factor for lossy Hcompress [default = 0 = lossless]\n");
fp_msg ("             (+values relative to RMS noise; -value is absolute)\n");
fp_msg (" -n <noise>  Rescale scaled-integer images to reduce noise and improve compression.\n");
fp_msg (" -v          Verbose mode; list each file as it is processed.\n");
fp_msg (" -T          Show compression algorithm comparison test statistics; files unchanged.\n");
fp_msg (" -R <file>   Write the comparison test report (above) to a text file.\n");
fp_msg (" -table      Compress FITS binary tables as well as compress any image HDUs.\n");
fp_msg (" -tableonly  Compress only FITS binary tables; do not compress any image HDUs.\n");
fp_msg ("             \n");

fp_msg ("\nkeywords shared with funpack:\n");

fp_msg (" -F          Overwrite input file by output file with same name.\n");
fp_msg (" -D          Delete input file after writing output.\n");
fp_msg (" -Y          Suppress prompts to confirm -F or -D options.\n");

fp_msg (" -S          Output compressed FITS files to STDOUT.\n");
fp_msg (" -L          List contents; files unchanged.\n");

fp_msg (" -C          Don't update FITS checksum keywords.\n");

fp_msg (" -H          Show this message.\n");
fp_msg (" -V          Show version number.\n");

fp_msg ("\n <FITS>      FITS files to pack; enter '-' (a hyphen) to read input from stdin stream.\n");
fp_msg (" Refer to the fpack User's Guide for more extensive help.\n");
return(0);
}
