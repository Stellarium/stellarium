/* FUNPACK
 * R. Seaman, NOAO
 * uses fits_img_compress by W. Pence, HEASARC
 */

#include "fitsio.h"
#include "fpack.h"

int main (int argc, char *argv[])
{
	fpstate	fpvar;

	if (argc <= 1) { fu_usage (); fu_hint (); exit (-1); }

	fp_init (&fpvar);
	fu_get_param (argc, argv, &fpvar);

	if (fpvar.listonly) {
	    fp_list (argc, argv, fpvar);

	} else {
	    fp_preflight (argc, argv, FUNPACK, &fpvar);
	    fp_loop (argc, argv, FUNPACK, fpvar);
	}

	exit (0);
}

int fu_get_param (int argc, char *argv[], fpstate *fpptr)
{
	int	iarg;
	char	tile[SZ_STR];

        if (fpptr->initialized != FP_INIT_MAGIC) {
            fp_msg ("Error: internal initialization error\n"); exit (-1);
        }

	tile[0] = 0;

        /* by default, .fz suffix characters to be deleted from compressed file */
	fpptr->delete_suffix = 1;

	/* flags must come first and be separately specified
	 */
	for (iarg = 1; iarg < argc; iarg++) {
	    if (argv[iarg][0] == '-' && strlen (argv[iarg]) == 2) {

		if (argv[iarg][1] == 'F') {
		    fpptr->clobber++;
                    fpptr->delete_suffix = 0;  /* no suffix in this case */

		} else if (argv[iarg][1] == 'D') {
		    fpptr->delete_input++;

		} else if (argv[iarg][1] == 'P') {
		    if (++iarg >= argc) {
			fu_usage (); fu_hint (); exit (-1);
		    } else
			strncpy (fpptr->prefix, argv[iarg], SZ_STR);

		} else if (argv[iarg][1] == 'E') {
		    if (++iarg >= argc) {
			fu_usage (); fu_hint (); exit (-1);
		    } else
			strncpy (fpptr->extname, argv[iarg], SZ_STR);

		} else if (argv[iarg][1] == 'S') {
		    fpptr->to_stdout++;

		} else if (argv[iarg][1] == 'L') {
		    fpptr->listonly++;

		} else if (argv[iarg][1] == 'C') {
		    fpptr->do_checksums = 0;

		} else if (argv[iarg][1] == 'H') {
		    fu_help (); exit (0);

		} else if (argv[iarg][1] == 'V') {
		    fp_version (); exit (0);

		} else if (argv[iarg][1] == 'Z') {
		    fpptr->do_gzip_file++;

		} else if (argv[iarg][1] == 'v') {
		    fpptr->verbose = 1;

		} else if (argv[iarg][1] == 'O') {
		    if (++iarg >= argc) {
			fu_usage (); fu_hint (); exit (-1);
		    } else
			strncpy (fpptr->outfile, argv[iarg], SZ_STR);

		} else {
		    fp_msg ("Error: unknown command line flag `");
		    fp_msg (argv[iarg]); fp_msg ("'\n");
		    fu_usage (); fu_hint (); exit (-1);
		}

	    } else
		break;
	}
	
	if (fpptr->extname[0] && (fpptr->clobber || fpptr->delete_input)) {
	    fp_msg ("Error: -E option may not be used with -F or -D\n");
	    fu_usage (); exit (-1);
        }

	if (fpptr->to_stdout && (fpptr->outfile[0] || fpptr->prefix[0]) ) {

	    fp_msg ("Error: -S option may not be used with -P or -O\n");
	    fu_usage (); exit (-1);
        }

	if (fpptr->outfile[0] && fpptr->prefix[0] ) {
	    fp_msg ("Error: -P and -O options may not be used together\n");
	    fu_usage (); exit (-1);
        }

	if (iarg >= argc) {
	    fp_msg ("Error: no FITS files to uncompress\n");
	    fu_usage (); exit (-1);
	} else
	    fpptr->firstfile = iarg;

	return(0);
}

int fu_usage (void)
{
	fp_msg ("usage: funpack [-E <HDUlist>] [-P <pre>] [-O <name>] [-Z] -v <FITS>\n");
        fp_msg ("more:   [-F] [-D] [-S] [-L] [-C] [-H] [-V] \n");
	return(0);
}

int fu_hint (void)
{
	fp_msg ("      `funpack -H' for help\n");
	return(0);
}

int fu_help (void)
{
fp_msg ("funpack, decompress fpacked files.  Version ");
fp_version ();
fu_usage ();
fp_msg ("\n");

fp_msg ("Flags must be separate and appear before filenames:\n");
fp_msg (" -E <HDUlist> Unpack only the list of HDU names or numbers in the file.\n");
fp_msg (" -P <pre>    Prepend <pre> to create new output filenames.\n");
fp_msg (" -O <name>   Specify full output file name.\n");
fp_msg (" -Z          Recompress the output file with host GZIP program.\n");
fp_msg (" -F          Overwrite input file by output file with same name.\n");
fp_msg (" -D          Delete input file after writing output.\n");
fp_msg (" -S          Output uncompressed file to STDOUT file stream.\n");
fp_msg (" -L          List contents, files unchanged.\n");

fp_msg (" -C          Don't update FITS checksum keywords.\n");

fp_msg (" -v          Verbose mode; list each file as it is processed.\n");
fp_msg (" -H          Show this message.\n");
fp_msg (" -V          Show version number.\n");

fp_msg (" \n<FITS>       FITS files to unpack; enter '-' (a hyphen) to read from stdin.\n");
fp_msg (" Refer to the fpack User's Guide for more extensive help.\n");
	return(0);
}
