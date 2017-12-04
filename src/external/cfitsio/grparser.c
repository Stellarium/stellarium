/*		T E M P L A T E   P A R S E R
		=============================

		by Jerzy.Borkowski@obs.unige.ch

		Integral Science Data Center
		ch. d'Ecogia 16
		1290 Versoix
		Switzerland

14-Oct-98: initial release
16-Oct-98: code cleanup, #include <string.h> included, now gcc -Wall prints no
		warnings during compilation. Bugfix: now one can specify additional
		columns in group HDU. Autoindexing also works in this situation
		(colunms are number from 7 however).
17-Oct-98: bugfix: complex keywords were incorrectly written (was TCOMPLEX should
		be TDBLCOMPLEX).
20-Oct-98: bugfix: parser was writing EXTNAME twice, when first HDU in template is
		defined with XTENSION IMAGE then parser creates now dummy PHDU,
		SIMPLE T is now allowed only at most once and in first HDU only.
		WARNING: one should not define EXTNAME keyword for GROUP HDUs, as
		they have them already defined by parser (EXTNAME = GROUPING).
		Parser accepts EXTNAME oin GROUP HDU definition, but in this
		case multiple EXTNAME keywords will present in HDU header.
23-Oct-98: bugfix: unnecessary space was written to FITS file for blank
		keywords.
24-Oct-98: syntax change: empty lines and lines with only whitespaces are 
		written to FITS files as blank keywords (if inside group/hdu
		definition). Previously lines had to have at least 8 spaces.
		Please note, that due to pecularities of CFITSIO if the
		last keyword(s) defined for given HDU are blank keywords
		consisting of only 80 spaces, then (some of) those keywords
		may be silently deleted by CFITSIO.
13-Nov-98: bugfix: parser was writing GRPNAME twice. Parser still creates
                GRPNAME keywords for GROUP HDU's which do not specify them.
                However, values (of form DEFAULT_GROUP_XXX) are assigned
                not necessarily in order HDUs appear in template file, but
                rather in order parser completes their creation in FITS
                file. Also, when including files, if fopen fails, parser
                tries to open file with a name = directory_of_top_level
                file + name of file to be included, as long as name
                of file to be included does not specify absolute pathname.
16-Nov-98: bugfix to bugfix from 13-Nov-98
19-Nov-98: EXTVER keyword is now automatically assigned value by parser.
17-Dev-98: 2 new things added: 1st: CFITSIO_INCLUDE_FILES environment
		variable can contain a colon separated list of directories
		to look for when looking for template include files (and master
		template also). 2nd: it is now possible to append template
		to nonempty FITS. file. fitsfile *ff no longer needs to point
		to an empty FITS file with 0 HDUs in it. All data written by
		parser will simple be appended at the end of file.
22-Jan-99: changes to parser: when in append mode parser initially scans all
		existing HDUs to built a list of already used EXTNAME/EXTVERs
22-Jan-99: Bruce O'Neel, bugfix : TLONG should always reference long type
		variable on OSF/Alpha and on 64-bit archs in general
20-Jun-2002 Wm Pence, added support for the HIERARCH keyword convention in
                which keyword names can effectively be longer than 8 characters.
                Example:
                HIERARCH  LongKeywordName = 'value' / comment
30-Jan-2003 Wm Pence, bugfix: ngp_read_xtension was testing for "ASCIITABLE" 
                instead of "TABLE" as the XTENSION value of an ASCII table,
                and it did not allow for optional trailing spaces in the
                "IMAGE" or "TABLE" string. 
16-Dec-2003 James Peachey: ngp_keyword_all_write was modified to apply
                comments from the template file to the output file in
                the case of reserved keywords (e.g. tform#, ttype# etcetera).
*/


#include <stdio.h>
#include <stdlib.h>

#ifdef sparc
#include <malloc.h>
#include <memory.h>
#endif

#include <string.h>
#include "fitsio2.h"
#include "grparser.h"

NGP_RAW_LINE	ngp_curline = { NULL, NULL, NULL, NGP_TTYPE_UNKNOWN, NULL, NGP_FORMAT_OK, 0 };
NGP_RAW_LINE	ngp_prevline = { NULL, NULL, NULL, NGP_TTYPE_UNKNOWN, NULL, NGP_FORMAT_OK, 0 };

int		ngp_inclevel = 0;		/* number of included files, 1 - means mean file */
int		ngp_grplevel = 0;		/* group nesting level, 0 - means no grouping */

FILE		*ngp_fp[NGP_MAX_INCLUDE];	/* stack of included file handles */
int		ngp_keyidx = NGP_TOKEN_UNKNOWN;	/* index of token in current line */
NGP_TOKEN	ngp_linkey;			/* keyword after line analyze */

char            ngp_master_dir[NGP_MAX_FNAME];  /* directory of top level include file */

NGP_TKDEF	ngp_tkdef[] = 			/* tokens recognized by parser */
      { {	"\\INCLUDE",	NGP_TOKEN_INCLUDE },
	{	"\\GROUP",	NGP_TOKEN_GROUP },
	{	"\\END",	NGP_TOKEN_END },
	{	"XTENSION",	NGP_TOKEN_XTENSION },
	{	"SIMPLE",	NGP_TOKEN_SIMPLE },
	{	NULL,		NGP_TOKEN_UNKNOWN }
      };

int	master_grp_idx = 1;			/* current unnamed group in object */

int		ngp_extver_tab_size = 0;
NGP_EXTVER_TAB	*ngp_extver_tab = NULL;


int	ngp_get_extver(char *extname, int *version)
 { NGP_EXTVER_TAB *p;
   char 	*p2;
   int		i;

   if ((NULL == extname) || (NULL == version)) return(NGP_BAD_ARG);
   if ((NULL == ngp_extver_tab) && (ngp_extver_tab_size > 0)) return(NGP_BAD_ARG);
   if ((NULL != ngp_extver_tab) && (ngp_extver_tab_size <= 0)) return(NGP_BAD_ARG);

   for (i=0; i<ngp_extver_tab_size; i++)
    { if (0 == strcmp(extname, ngp_extver_tab[i].extname))
        { *version = (++ngp_extver_tab[i].version);
          return(NGP_OK);
        }
    }

   if (NULL == ngp_extver_tab)
     { p = (NGP_EXTVER_TAB *)ngp_alloc(sizeof(NGP_EXTVER_TAB)); }
   else
     { p = (NGP_EXTVER_TAB *)ngp_realloc(ngp_extver_tab, (ngp_extver_tab_size + 1) * sizeof(NGP_EXTVER_TAB)); }

   if (NULL == p) return(NGP_NO_MEMORY);

   p2 = ngp_alloc(strlen(extname) + 1);
   if (NULL == p2)
     { ngp_free(p);
       return(NGP_NO_MEMORY);
     }

   strcpy(p2, extname);
   ngp_extver_tab = p;
   ngp_extver_tab[ngp_extver_tab_size].extname = p2;
   *version = ngp_extver_tab[ngp_extver_tab_size].version = 1;

   ngp_extver_tab_size++;

   return(NGP_OK);
 }

int	ngp_set_extver(char *extname, int version)
 { NGP_EXTVER_TAB *p;
   char 	*p2;
   int		i;

   if (NULL == extname) return(NGP_BAD_ARG);
   if ((NULL == ngp_extver_tab) && (ngp_extver_tab_size > 0)) return(NGP_BAD_ARG);
   if ((NULL != ngp_extver_tab) && (ngp_extver_tab_size <= 0)) return(NGP_BAD_ARG);

   for (i=0; i<ngp_extver_tab_size; i++)
    { if (0 == strcmp(extname, ngp_extver_tab[i].extname))
        { if (version > ngp_extver_tab[i].version)  ngp_extver_tab[i].version = version;
          return(NGP_OK);
        }
    }

   if (NULL == ngp_extver_tab)
     { p = (NGP_EXTVER_TAB *)ngp_alloc(sizeof(NGP_EXTVER_TAB)); }
   else
     { p = (NGP_EXTVER_TAB *)ngp_realloc(ngp_extver_tab, (ngp_extver_tab_size + 1) * sizeof(NGP_EXTVER_TAB)); }

   if (NULL == p) return(NGP_NO_MEMORY);

   p2 = ngp_alloc(strlen(extname) + 1);
   if (NULL == p2)
     { ngp_free(p);
       return(NGP_NO_MEMORY);
     }

   strcpy(p2, extname);
   ngp_extver_tab = p;
   ngp_extver_tab[ngp_extver_tab_size].extname = p2;
   ngp_extver_tab[ngp_extver_tab_size].version = version;

   ngp_extver_tab_size++;

   return(NGP_OK);
 }


int	ngp_delete_extver_tab(void)
 { int i;

   if ((NULL == ngp_extver_tab) && (ngp_extver_tab_size > 0)) return(NGP_BAD_ARG);
   if ((NULL != ngp_extver_tab) && (ngp_extver_tab_size <= 0)) return(NGP_BAD_ARG);
   if ((NULL == ngp_extver_tab) && (0 == ngp_extver_tab_size)) return(NGP_OK);

   for (i=0; i<ngp_extver_tab_size; i++)
    { if (NULL != ngp_extver_tab[i].extname)
        { ngp_free(ngp_extver_tab[i].extname);
          ngp_extver_tab[i].extname = NULL;
        }
      ngp_extver_tab[i].version = 0;
    }
   ngp_free(ngp_extver_tab);
   ngp_extver_tab = NULL;
   ngp_extver_tab_size = 0;
   return(NGP_OK);
 }

	/* read one line from file */

int	ngp_line_from_file(FILE *fp, char **p)
 { int	c, r, llen, allocsize, alen;
   char	*p2;

   if (NULL == fp) return(NGP_NUL_PTR);		/* check for stupid args */
   if (NULL == p) return(NGP_NUL_PTR);		/* more foolproof checks */
   
   r = NGP_OK;					/* initialize stuff, reset err code */
   llen = 0;					/* 0 characters read so far */
   *p = (char *)ngp_alloc(1);			/* preallocate 1 byte */
   allocsize = 1;				/* signal that we have allocated 1 byte */
   if (NULL == *p) return(NGP_NO_MEMORY);	/* if this failed, system is in dire straits */

   for (;;)
    { c = getc(fp);				/* get next character */
      if ('\r' == c) continue;			/* carriage return character ?  Just ignore it */
      if (EOF == c)				/* EOF signalled ? */
        { 
          if (ferror(fp)) r = NGP_READ_ERR;	/* was it real error or simply EOF ? */
	  if (0 == llen) return(NGP_EOF);	/* signal EOF only if 0 characters read so far */
          break;
        }
      if ('\n' == c) break;			/* end of line character ? */
      
      llen++;					/* we have new character, make room for it */
      alen = ((llen + NGP_ALLOCCHUNK) / NGP_ALLOCCHUNK) * NGP_ALLOCCHUNK;
      if (alen > allocsize)
        { p2 = (char *)ngp_realloc(*p, alen);	/* realloc buffer, if there is need */
          if (NULL == p2)
            { r = NGP_NO_MEMORY;
              break;
            }
	  *p = p2;
          allocsize = alen;
        }
      (*p)[llen - 1] = c;			/* copy character to buffer */
    }

   llen++;					/* place for terminating \0 */
   if (llen != allocsize)
     { p2 = (char *)ngp_realloc(*p, llen);
       if (NULL == p2) r = NGP_NO_MEMORY;
       else
         { *p = p2;
           (*p)[llen - 1] = 0;			/* copy \0 to buffer */
         }         
     }
   else
     { (*p)[llen - 1] = 0;			/* necessary when line read was empty */
     }

   if ((NGP_EOF != r) && (NGP_OK != r))		/* in case of errors free resources */
     { ngp_free(*p);
       *p = NULL;
     }
   
   return(r);					/* return  status code */
 }

	/* free current line structure */

int	ngp_free_line(void)
 {
   if (NULL != ngp_curline.line)
     { ngp_free(ngp_curline.line);
       ngp_curline.line = NULL;
       ngp_curline.name = NULL;
       ngp_curline.value = NULL;
       ngp_curline.comment = NULL;
       ngp_curline.type = NGP_TTYPE_UNKNOWN;
       ngp_curline.format = NGP_FORMAT_OK;
       ngp_curline.flags = 0;
     }
   return(NGP_OK);
 }

	/* free cached line structure */

int	ngp_free_prevline(void)
 {
   if (NULL != ngp_prevline.line)
     { ngp_free(ngp_prevline.line);
       ngp_prevline.line = NULL;
       ngp_prevline.name = NULL;
       ngp_prevline.value = NULL;
       ngp_prevline.comment = NULL;
       ngp_prevline.type = NGP_TTYPE_UNKNOWN;
       ngp_prevline.format = NGP_FORMAT_OK;
       ngp_prevline.flags = 0;
     }
   return(NGP_OK);
 }

	/* read one line */

int	ngp_read_line_buffered(FILE *fp)
 {
   ngp_free_line();				/* first free current line (if any) */
   
   if (NULL != ngp_prevline.line)		/* if cached, return cached line */
     { ngp_curline = ngp_prevline;
       ngp_prevline.line = NULL;
       ngp_prevline.name = NULL;
       ngp_prevline.value = NULL;
       ngp_prevline.comment = NULL;
       ngp_prevline.type = NGP_TTYPE_UNKNOWN;
       ngp_prevline.format = NGP_FORMAT_OK;
       ngp_prevline.flags = 0;
       ngp_curline.flags = NGP_LINE_REREAD;
       return(NGP_OK);
     }

   ngp_curline.flags = 0;   			/* if not cached really read line from file */
   return(ngp_line_from_file(fp, &(ngp_curline.line)));
 }

	/* unread line */

int	ngp_unread_line(void)
 {
   if (NULL == ngp_curline.line)		/* nothing to unread */
     return(NGP_EMPTY_CURLINE);

   if (NULL != ngp_prevline.line)		/* we cannot unread line twice */
     return(NGP_UNREAD_QUEUE_FULL);

   ngp_prevline = ngp_curline;
   ngp_curline.line = NULL;
   return(NGP_OK);
 }

	/* a first guess line decomposition */

int	ngp_extract_tokens(NGP_RAW_LINE *cl)
 { char *p, *s;
   int	cl_flags, i;

   p = cl->line;				/* start from beginning of line */
   if (NULL == p) return(NGP_NUL_PTR);

   cl->name = cl->value = cl->comment = NULL;
   cl->type = NGP_TTYPE_UNKNOWN;
   cl->format = NGP_FORMAT_OK;

   cl_flags = 0;

   for (i=0;; i++)				/* if 8 spaces at beginning then line is comment */
    { if ((0 == *p) || ('\n' == *p))
        {					/* if line has only blanks -> write blank keyword */
          cl->line[0] = 0;			/* create empty name (0 length string) */
          cl->comment = cl->name = cl->line;
	  cl->type = NGP_TTYPE_RAW;		/* signal write unformatted to FITS file */
          return(NGP_OK);
        }
      if ((' ' != *p) && ('\t' != *p)) break;
      if (i >= 7)
        { 
          cl->comment = p + 1;
          for (s = cl->comment;; s++)		/* filter out any EOS characters in comment */
           { if ('\n' == *s) *s = 0;
	     if (0 == *s) break;
           }
          cl->line[0] = 0;			/* create empty name (0 length string) */
          cl->name = cl->line;
	  cl->type = NGP_TTYPE_RAW;
          return(NGP_OK);
        }
      p++;
    }

   cl->name = p;

   for (;;)					/* we need to find 1st whitespace */
    { if ((0 == *p) || ('\n' == *p))
        { *p = 0;
          break;
        }

      /*
        from Richard Mathar, 2002-05-03, add 10 lines:
        if upper/lowercase HIERARCH followed also by an equal sign...
      */
      if( fits_strncasecmp("HIERARCH",p,strlen("HIERARCH")) == 0 )
      {
           char * const eqsi=strchr(p,'=') ;
           if( eqsi )
           {
              cl_flags |= NGP_FOUND_EQUAL_SIGN ;
              p=eqsi ;
              break ;
           }
      }

      if ((' ' == *p) || ('\t' == *p)) break;
      if ('=' == *p)
        { cl_flags |= NGP_FOUND_EQUAL_SIGN;
          break;
        }

      p++;
    }

   if (*p) *(p++) = 0;				/* found end of keyname so terminate string with zero */

   if ((!fits_strcasecmp("HISTORY", cl->name))
    || (!fits_strcasecmp("COMMENT", cl->name))
    || (!fits_strcasecmp("CONTINUE", cl->name)))
     { cl->comment = p;
       for (s = cl->comment;; s++)		/* filter out any EOS characters in comment */
        { if ('\n' == *s) *s = 0;
	  if (0 == *s) break;
        }
       cl->type = NGP_TTYPE_RAW;
       return(NGP_OK);
     }

   if (!fits_strcasecmp("\\INCLUDE", cl->name))
     {
       for (;; p++)  if ((' ' != *p) && ('\t' != *p)) break; /* skip whitespace */

       cl->value = p;
       for (s = cl->value;; s++)		/* filter out any EOS characters */
        { if ('\n' == *s) *s = 0;
	  if (0 == *s) break;
        }
       cl->type = NGP_TTYPE_UNKNOWN;
       return(NGP_OK);
     }
       
   for (;; p++)
    { if ((0 == *p) || ('\n' == *p))  return(NGP_OK);	/* test if at end of string */
      if ((' ' == *p) || ('\t' == *p)) continue; /* skip whitespace */
      if (cl_flags & NGP_FOUND_EQUAL_SIGN) break;
      if ('=' != *p) break;			/* ignore initial equal sign */
      cl_flags |= NGP_FOUND_EQUAL_SIGN;
    }
      
   if ('/' == *p)				/* no value specified, comment only */
     { p++;
       if ((' ' == *p) || ('\t' == *p)) p++;
       cl->comment = p;
       for (s = cl->comment;; s++)		/* filter out any EOS characters in comment */
        { if ('\n' == *s) *s = 0;
	  if (0 == *s) break;
        }
       return(NGP_OK);
     }

   if ('\'' == *p)				/* we have found string within quotes */
     { cl->value = s = ++p;			/* set pointer to beginning of that string */
       cl->type = NGP_TTYPE_STRING;		/* signal that it is of string type */

       for (;;)					/* analyze it */
        { if ((0 == *p) || ('\n' == *p))	/* end of line -> end of string */
            { *s = 0; return(NGP_OK); }

          if ('\'' == *p)			/* we have found doublequote */
            { if ((0 == p[1]) || ('\n' == p[1]))/* doublequote is the last character in line */
                { *s = 0; return(NGP_OK); }
              if (('\t' == p[1]) || (' ' == p[1])) /* duoblequote was string terminator */
                { *s = 0; p++; break; }
              if ('\'' == p[1]) p++;		/* doublequote is inside string, convert "" -> " */ 
            }

          *(s++) = *(p++);			/* compact string in place, necess. by "" -> " conversion */
        }
     }
   else						/* regular token */
     { 
       cl->value = p;				/* set pointer to token */
       cl->type = NGP_TTYPE_UNKNOWN;		/* we dont know type at the moment */
       for (;; p++)				/* we need to find 1st whitespace */
        { if ((0 == *p) || ('\n' == *p))
            { *p = 0; return(NGP_OK); }
          if ((' ' == *p) || ('\t' == *p)) break;
        }
       if (*p)  *(p++) = 0;			/* found so terminate string with zero */
     }
       
   for (;; p++)
    { if ((0 == *p) || ('\n' == *p))  return(NGP_OK);	/* test if at end of string */
      if ((' ' != *p) && ('\t' != *p)) break;	/* skip whitespace */
    }
      
   if ('/' == *p)				/* no value specified, comment only */
     { p++;
       if ((' ' == *p) || ('\t' == *p)) p++;
       cl->comment = p;
       for (s = cl->comment;; s++)		/* filter out any EOS characters in comment */
        { if ('\n' == *s) *s = 0;
	  if (0 == *s) break;
        }
       return(NGP_OK);
     }

   cl->format = NGP_FORMAT_ERROR;
   return(NGP_OK);				/* too many tokens ... */
 }

/*      try to open include file. If open fails and fname
        does not specify absolute pathname, try to open fname
        in any directory specified in CFITSIO_INCLUDE_FILES
        environment variable. Finally try to open fname
        relative to ngp_master_dir, which is directory of top
        level include file
*/

int	ngp_include_file(char *fname)		/* try to open include file */
 { char *p, *p2, *cp, *envar, envfiles[NGP_MAX_ENVFILES];
   char *saveptr;

   if (NULL == fname) return(NGP_NUL_PTR);

   if (ngp_inclevel >= NGP_MAX_INCLUDE)		/* too many include files */
     return(NGP_INC_NESTING);

   if (NULL == (ngp_fp[ngp_inclevel] = fopen(fname, "r")))
     {                                          /* if simple open failed .. */
       envar = getenv("CFITSIO_INCLUDE_FILES");	/* scan env. variable, and retry to open */

       if (NULL != envar)			/* is env. variable defined ? */
         { strncpy(envfiles, envar, NGP_MAX_ENVFILES - 1);
           envfiles[NGP_MAX_ENVFILES - 1] = 0;	/* copy search path to local variable, env. is fragile */

           for (p2 = ffstrtok(envfiles, ":",&saveptr); NULL != p2; p2 = ffstrtok(NULL, ":",&saveptr))
            {
	      cp = (char *)ngp_alloc(strlen(fname) + strlen(p2) + 2);
	      if (NULL == cp) return(NGP_NO_MEMORY);

	      strcpy(cp, p2);
#ifdef  MSDOS
              strcat(cp, "\\");			/* abs. pathname for MSDOS */
               
#else
              strcat(cp, "/");			/* and for unix */
#endif
	      strcat(cp, fname);
	  
	      ngp_fp[ngp_inclevel] = fopen(cp, "r");
	      ngp_free(cp);

	      if (NULL != ngp_fp[ngp_inclevel]) break;
	    }
        }
                                      
       if (NULL == ngp_fp[ngp_inclevel])	/* finally try to open relative to top level */
         {
#ifdef  MSDOS
           if ('\\' == fname[0]) return(NGP_ERR_FOPEN); /* abs. pathname for MSDOS, does not support C:\\PATH */
#else
           if ('/' == fname[0]) return(NGP_ERR_FOPEN); /* and for unix */
#endif
           if (0 == ngp_master_dir[0]) return(NGP_ERR_FOPEN);

	   p = ngp_alloc(strlen(fname) + strlen(ngp_master_dir) + 1);
           if (NULL == p) return(NGP_NO_MEMORY);

           strcpy(p, ngp_master_dir);		/* construct composite pathname */
           strcat(p, fname);			/* comp = master + fname */

           ngp_fp[ngp_inclevel] = fopen(p, "r");/* try to open composite */
           ngp_free(p);				/* we don't need buffer anymore */

           if (NULL == ngp_fp[ngp_inclevel])
             return(NGP_ERR_FOPEN);		/* fail if error */
         }
     }

   ngp_inclevel++;
   return(NGP_OK);
 }


/* read line in the intelligent way. All \INCLUDE directives are handled,
   empty and comment line skipped. If this function returns NGP_OK, than
   decomposed line (name, type, value in proper type and comment) are
   stored in ngp_linkey structure. ignore_blank_lines parameter is zero
   when parser is inside GROUP or HDU definition. Nonzero otherwise.
*/

int	ngp_read_line(int ignore_blank_lines)
 { int r, nc, savec;
   unsigned k;

   if (ngp_inclevel <= 0)		/* do some sanity checking first */
     { ngp_keyidx = NGP_TOKEN_EOF;	/* no parents, so report error */
       return(NGP_OK);	
     }
   if (ngp_inclevel > NGP_MAX_INCLUDE)  return(NGP_INC_NESTING);
   if (NULL == ngp_fp[ngp_inclevel - 1]) return(NGP_NUL_PTR);

   for (;;)
    { switch (r = ngp_read_line_buffered(ngp_fp[ngp_inclevel - 1]))
       { case NGP_EOF:
		ngp_inclevel--;			/* end of file, revert to parent */
		if (ngp_fp[ngp_inclevel])	/* we can close old file */
		  fclose(ngp_fp[ngp_inclevel]);

		ngp_fp[ngp_inclevel] = NULL;
		if (ngp_inclevel <= 0)
		  { ngp_keyidx = NGP_TOKEN_EOF;	/* no parents, so report error */
		    return(NGP_OK);	
		  }
		continue;

	 case NGP_OK:
		if (ngp_curline.flags & NGP_LINE_REREAD) return(r);
		break;
	 default:
		return(r);
       }
      
      switch (ngp_curline.line[0])
       { case 0: if (0 == ignore_blank_lines) break; /* ignore empty lines if told so */
         case '#': continue;			/* ignore comment lines */
       }
      
      r = ngp_extract_tokens(&ngp_curline);	/* analyse line, extract tokens and comment */
      if (NGP_OK != r) return(r);

      if (NULL == ngp_curline.name)  continue;	/* skip lines consisting only of whitespaces */

      for (k = 0; k < strlen(ngp_curline.name); k++)
       { if ((ngp_curline.name[k] >= 'a') && (ngp_curline.name[k] <= 'z')) 
           ngp_curline.name[k] += 'A' - 'a';	/* force keyword to be upper case */
         if (k == 7) break;  /* only first 8 chars are required to be upper case */
       }

      for (k=0;; k++)				/* find index of keyword in keyword table */
       { if (NGP_TOKEN_UNKNOWN == ngp_tkdef[k].code) break;
         if (0 == strcmp(ngp_curline.name, ngp_tkdef[k].name)) break;
       }

      ngp_keyidx = ngp_tkdef[k].code;		/* save this index, grammar parser will need this */

      if (NGP_TOKEN_INCLUDE == ngp_keyidx)	/* if this is \INCLUDE keyword, try to include file */
        { if (NGP_OK != (r = ngp_include_file(ngp_curline.value))) return(r);
	  continue;				/* and read next line */
        }

      ngp_linkey.type = NGP_TTYPE_UNKNOWN;	/* now, get the keyword type, it's a long story ... */

      if (NULL != ngp_curline.value)		/* if no value given signal it */
        { if (NGP_TTYPE_STRING == ngp_curline.type)  /* string type test */
            { ngp_linkey.type = NGP_TTYPE_STRING;
              ngp_linkey.value.s = ngp_curline.value;
            }
          if (NGP_TTYPE_UNKNOWN == ngp_linkey.type) /* bool type test */
            { if ((!fits_strcasecmp("T", ngp_curline.value)) || (!fits_strcasecmp("F", ngp_curline.value)))
                { ngp_linkey.type = NGP_TTYPE_BOOL;
                  ngp_linkey.value.b = (fits_strcasecmp("T", ngp_curline.value) ? 0 : 1);
                }
            }
          if (NGP_TTYPE_UNKNOWN == ngp_linkey.type) /* complex type test */
            { if (2 == sscanf(ngp_curline.value, "(%lg,%lg)%n", &(ngp_linkey.value.c.re), &(ngp_linkey.value.c.im), &nc))
                { if ((' ' == ngp_curline.value[nc]) || ('\t' == ngp_curline.value[nc])
                   || ('\n' == ngp_curline.value[nc]) || (0 == ngp_curline.value[nc]))
                    { ngp_linkey.type = NGP_TTYPE_COMPLEX;
                    }
                }
            }
          if (NGP_TTYPE_UNKNOWN == ngp_linkey.type) /* real type test */
            { if (strchr(ngp_curline.value, '.') && (1 == sscanf(ngp_curline.value, "%lg%n", &(ngp_linkey.value.d), &nc)))
                {
		 if ('D' == ngp_curline.value[nc]) {
		   /* test if template used a 'D' rather than an 'E' as the exponent character (added by WDP in 12/2010) */
                   savec = nc;
		   ngp_curline.value[nc] = 'E';
		   sscanf(ngp_curline.value, "%lg%n", &(ngp_linkey.value.d), &nc);
		   if ((' ' == ngp_curline.value[nc]) || ('\t' == ngp_curline.value[nc])
                    || ('\n' == ngp_curline.value[nc]) || (0 == ngp_curline.value[nc]))  {
                       ngp_linkey.type = NGP_TTYPE_REAL;
                     } else {  /* no, this is not a real value */
		       ngp_curline.value[savec] = 'D';  /* restore the original D character */
 		     }
		 } else {
		  if ((' ' == ngp_curline.value[nc]) || ('\t' == ngp_curline.value[nc])
                   || ('\n' == ngp_curline.value[nc]) || (0 == ngp_curline.value[nc]))
                    { ngp_linkey.type = NGP_TTYPE_REAL;
                    }
                 } 
                }
            }
          if (NGP_TTYPE_UNKNOWN == ngp_linkey.type) /* integer type test */
            { if (1 == sscanf(ngp_curline.value, "%d%n", &(ngp_linkey.value.i), &nc))
                { if ((' ' == ngp_curline.value[nc]) || ('\t' == ngp_curline.value[nc])
                   || ('\n' == ngp_curline.value[nc]) || (0 == ngp_curline.value[nc]))
                    { ngp_linkey.type = NGP_TTYPE_INT;
                    }
                }
            }
          if (NGP_TTYPE_UNKNOWN == ngp_linkey.type) /* force string type */
            { ngp_linkey.type = NGP_TTYPE_STRING;
              ngp_linkey.value.s = ngp_curline.value;
            }
        }
      else
        { if (NGP_TTYPE_RAW == ngp_curline.type) ngp_linkey.type = NGP_TTYPE_RAW;
	  else ngp_linkey.type = NGP_TTYPE_NULL;
	}

      if (NULL != ngp_curline.comment)
        { strncpy(ngp_linkey.comment, ngp_curline.comment, NGP_MAX_COMMENT); /* store comment */
	  ngp_linkey.comment[NGP_MAX_COMMENT - 1] = 0;
	}
      else
        { ngp_linkey.comment[0] = 0;
        }

      strncpy(ngp_linkey.name, ngp_curline.name, NGP_MAX_NAME); /* and keyword's name */
      ngp_linkey.name[NGP_MAX_NAME - 1] = 0;

      if (strlen(ngp_linkey.name) > FLEN_KEYWORD)  /* WDP: 20-Jun-2002:  mod to support HIERARCH */
        { 
           return(NGP_BAD_ARG);		/* cfitsio does not allow names > 8 chars */
        }
      
      return(NGP_OK);			/* we have valid non empty line, so return success */
    }
 }

	/* check whether keyword can be written as is */

int	ngp_keyword_is_write(NGP_TOKEN *ngp_tok)
 { int i, j, l, spc;
                        /* indexed variables not to write */

   static char *nm[] = { "NAXIS", "TFORM", "TTYPE", NULL } ;

                        /* non indexed variables not allowed to write */
  
   static char *nmni[] = { "SIMPLE", "XTENSION", "BITPIX", "NAXIS", "PCOUNT",
                           "GCOUNT", "TFIELDS", "THEAP", "EXTEND", "EXTVER",
                           NULL } ;

   if (NULL == ngp_tok) return(NGP_NUL_PTR);

   for (j = 0; ; j++)           /* first check non indexed */
    { if (NULL == nmni[j]) break;
      if (0 == strcmp(nmni[j], ngp_tok->name)) return(NGP_BAD_ARG);
    } 

   for (j = 0; ; j++)           /* now check indexed */
    { if (NULL == nm[j]) return(NGP_OK);
      l = strlen(nm[j]);
      if ((l < 1) || (l > 5)) continue;
      if (0 == strncmp(nm[j], ngp_tok->name, l)) break;
    } 

   if ((ngp_tok->name[l] < '1') || (ngp_tok->name[l] > '9')) return(NGP_OK);
   spc = 0;
   for (i = l + 1; i < 8; i++)
    { if (spc) { if (' ' != ngp_tok->name[i]) return(NGP_OK); }
      else
       { if ((ngp_tok->name[i] >= '0') && (ngp_tok->name[i] <= '9')) continue;
         if (' ' == ngp_tok->name[i]) { spc = 1; continue; }
         if (0 == ngp_tok->name[i]) break;
         return(NGP_OK);
       }
    }
   return(NGP_BAD_ARG);
 }

	/* write (almost) all keywords from given HDU to disk */

int     ngp_keyword_all_write(NGP_HDU *ngph, fitsfile *ffp, int mode)
 { int		i, r, ib;
   char		buf[200];
   long		l;


   if (NULL == ngph) return(NGP_NUL_PTR);
   if (NULL == ffp) return(NGP_NUL_PTR);
   r = NGP_OK;
   
   for (i=0; i<ngph->tokcnt; i++)
    { r = ngp_keyword_is_write(&(ngph->tok[i]));
      if ((NGP_REALLY_ALL & mode) || (NGP_OK == r))
        { switch (ngph->tok[i].type)
           { case NGP_TTYPE_BOOL:
			ib = ngph->tok[i].value.b;
			fits_write_key(ffp, TLOGICAL, ngph->tok[i].name, &ib, ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_STRING:
			fits_write_key_longstr(ffp, ngph->tok[i].name, ngph->tok[i].value.s, ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_INT:
			l = ngph->tok[i].value.i;	/* bugfix - 22-Jan-99, BO - nonalignment of OSF/Alpha */
			fits_write_key(ffp, TLONG, ngph->tok[i].name, &l, ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_REAL:
			fits_write_key(ffp, TDOUBLE, ngph->tok[i].name, &(ngph->tok[i].value.d), ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_COMPLEX:
			fits_write_key(ffp, TDBLCOMPLEX, ngph->tok[i].name, &(ngph->tok[i].value.c), ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_NULL:
			fits_write_key_null(ffp, ngph->tok[i].name, ngph->tok[i].comment, &r);
			break;
             case NGP_TTYPE_RAW:
			if (0 == strcmp("HISTORY", ngph->tok[i].name))
			  { fits_write_history(ffp, ngph->tok[i].comment, &r);
			    break;
			  }
			if (0 == strcmp("COMMENT", ngph->tok[i].name))
			  { fits_write_comment(ffp, ngph->tok[i].comment, &r);
			    break;
			  }
			sprintf(buf, "%-8.8s%s", ngph->tok[i].name, ngph->tok[i].comment);
			fits_write_record(ffp, buf, &r);
                        break;
           }
        }
      else if (NGP_BAD_ARG == r) /* enhancement 10 dec 2003, James Peachey: template comments replace defaults */
        { r = NGP_OK;						/* update comments of special keywords like TFORM */
          if (ngph->tok[i].comment && *ngph->tok[i].comment)	/* do not update with a blank comment */
            { fits_modify_comment(ffp, ngph->tok[i].name, ngph->tok[i].comment, &r);
            }
        }
      else /* other problem, typically a blank token */
        { r = NGP_OK;						/* skip this token, but continue */
        }
      if (r) return(r);
    }
     
   fits_set_hdustruc(ffp, &r);				/* resync cfitsio */
   return(r);
 }

	/* init HDU structure */

int	ngp_hdu_init(NGP_HDU *ngph)
 { if (NULL == ngph) return(NGP_NUL_PTR);
   ngph->tok = NULL;
   ngph->tokcnt = 0;
   return(NGP_OK);
 }

	/* clear HDU structure */

int	ngp_hdu_clear(NGP_HDU *ngph)
 { int i;

   if (NULL == ngph) return(NGP_NUL_PTR);

   for (i=0; i<ngph->tokcnt; i++)
    { if (NGP_TTYPE_STRING == ngph->tok[i].type)
        if (NULL != ngph->tok[i].value.s)
          { ngp_free(ngph->tok[i].value.s);
            ngph->tok[i].value.s = NULL;
          }
    }

   if (NULL != ngph->tok) ngp_free(ngph->tok);

   ngph->tok = NULL;
   ngph->tokcnt = 0;

   return(NGP_OK);
 }

	/* insert new token to HDU structure */

int	ngp_hdu_insert_token(NGP_HDU *ngph, NGP_TOKEN *newtok)
 { NGP_TOKEN *tkp;
   
   if (NULL == ngph) return(NGP_NUL_PTR);
   if (NULL == newtok) return(NGP_NUL_PTR);

   if (0 == ngph->tokcnt)
     tkp = (NGP_TOKEN *)ngp_alloc((ngph->tokcnt + 1) * sizeof(NGP_TOKEN));
   else
     tkp = (NGP_TOKEN *)ngp_realloc(ngph->tok, (ngph->tokcnt + 1) * sizeof(NGP_TOKEN));

   if (NULL == tkp) return(NGP_NO_MEMORY);
       
   ngph->tok = tkp;
   ngph->tok[ngph->tokcnt] = *newtok;

   if (NGP_TTYPE_STRING == newtok->type)
     { if (NULL != newtok->value.s)
         { ngph->tok[ngph->tokcnt].value.s = (char *)ngp_alloc(1 + strlen(newtok->value.s));
           if (NULL == ngph->tok[ngph->tokcnt].value.s) return(NGP_NO_MEMORY);
           strcpy(ngph->tok[ngph->tokcnt].value.s, newtok->value.s);
         }
     }

   ngph->tokcnt++;
   return(NGP_OK);
 }


int	ngp_append_columns(fitsfile *ff, NGP_HDU *ngph, int aftercol)
 { int		r, i, j, exitflg, ngph_i;
   char 	*my_tform, *my_ttype;
   char		ngph_ctmp;


   if (NULL == ff) return(NGP_NUL_PTR);
   if (NULL == ngph) return(NGP_NUL_PTR);
   if (0 == ngph->tokcnt) return(NGP_OK);	/* nothing to do ! */

   r = NGP_OK;
   exitflg = 0;

   for (j=aftercol; j<NGP_MAX_ARRAY_DIM; j++)	/* 0 for table, 6 for group */
    { 
      my_tform = NULL;
      my_ttype = "";
    
      for (i=0; ; i++)
       { if (1 == sscanf(ngph->tok[i].name, "TFORM%d%c", &ngph_i, &ngph_ctmp))
           { if ((NGP_TTYPE_STRING == ngph->tok[i].type) && (ngph_i == (j + 1)))
   	    { my_tform = ngph->tok[i].value.s;
   	    }
                }
         else if (1 == sscanf(ngph->tok[i].name, "TTYPE%d%c", &ngph_i, &ngph_ctmp))
           { if ((NGP_TTYPE_STRING == ngph->tok[i].type) && (ngph_i == (j + 1)))
               { my_ttype = ngph->tok[i].value.s;
               }
           }
         
         if ((NULL != my_tform) && (my_ttype[0])) break;
         
         if (i < (ngph->tokcnt - 1)) continue;
         exitflg = 1;
         break;
       }
      if ((NGP_OK == r) && (NULL != my_tform))
        fits_insert_col(ff, j + 1, my_ttype, my_tform, &r);

      if ((NGP_OK != r) || exitflg) break;
    }
   return(r);
 }

	/* read complete HDU */

int	ngp_read_xtension(fitsfile *ff, int parent_hn, int simple_mode)
 { int		r, exflg, l, my_hn, tmp0, incrementor_index, i, j;
   int		ngph_dim, ngph_bitpix, ngph_node_type, my_version;
   char		incrementor_name[NGP_MAX_STRING], ngph_ctmp;
   char 	*ngph_extname = 0;
   long		ngph_size[NGP_MAX_ARRAY_DIM];
   NGP_HDU	ngph;
   long		lv;

   incrementor_name[0] = 0;			/* signal no keyword+'#' found yet */
   incrementor_index = 0;

   if (NGP_OK != (r = ngp_hdu_init(&ngph))) return(r);

   if (NGP_OK != (r = ngp_read_line(0))) return(r);	/* EOF always means error here */
   switch (NGP_XTENSION_SIMPLE & simple_mode)
     {
       case 0:  if (NGP_TOKEN_XTENSION != ngp_keyidx) return(NGP_TOKEN_NOT_EXPECT);
		break;
       default:	if (NGP_TOKEN_SIMPLE != ngp_keyidx) return(NGP_TOKEN_NOT_EXPECT);
		break;
     }
       	
   if (NGP_OK != (r = ngp_hdu_insert_token(&ngph, &ngp_linkey))) return(r);

   for (;;)
    { if (NGP_OK != (r = ngp_read_line(0))) return(r);	/* EOF always means error here */
      exflg = 0;
      switch (ngp_keyidx)
       { 
	 case NGP_TOKEN_SIMPLE:
	 		r = NGP_TOKEN_NOT_EXPECT;
			break;
	 		                        
	 case NGP_TOKEN_END:
         case NGP_TOKEN_XTENSION:
         case NGP_TOKEN_GROUP:
         		r = ngp_unread_line();	/* WARNING - not break here .... */
         case NGP_TOKEN_EOF:
			exflg = 1;
 			break;

         default:	l = strlen(ngp_linkey.name);
			if ((l >= 2) && (l <= 6))
			  { if ('#' == ngp_linkey.name[l - 1])
			      { if (0 == incrementor_name[0])
			          { memcpy(incrementor_name, ngp_linkey.name, l - 1);
			            incrementor_name[l - 1] = 0;
			          }
			        if (((l - 1) == (int)strlen(incrementor_name)) && (0 == memcmp(incrementor_name, ngp_linkey.name, l - 1)))
			          { incrementor_index++;
			          }
			        sprintf(ngp_linkey.name + l - 1, "%d", incrementor_index);
			      }
			  }
			r = ngp_hdu_insert_token(&ngph, &ngp_linkey);
 			break;
       }
      if ((NGP_OK != r) || exflg) break;
    }

   if (NGP_OK == r)
     { 				/* we should scan keywords, and calculate HDU's */
				/* structure ourselves .... */

       ngph_node_type = NGP_NODE_INVALID;	/* init variables */
       ngph_bitpix = 0;
       ngph_extname = NULL;
       for (i=0; i<NGP_MAX_ARRAY_DIM; i++) ngph_size[i] = 0;
       ngph_dim = 0;

       for (i=0; i<ngph.tokcnt; i++)
        { if (!strcmp("XTENSION", ngph.tok[i].name))
            { if (NGP_TTYPE_STRING == ngph.tok[i].type)
                { if (!fits_strncasecmp("BINTABLE", ngph.tok[i].value.s,8)) ngph_node_type = NGP_NODE_BTABLE;
                  if (!fits_strncasecmp("TABLE", ngph.tok[i].value.s,5)) ngph_node_type = NGP_NODE_ATABLE;
                  if (!fits_strncasecmp("IMAGE", ngph.tok[i].value.s,5)) ngph_node_type = NGP_NODE_IMAGE;
                }
            }
          else if (!strcmp("SIMPLE", ngph.tok[i].name))
            { if (NGP_TTYPE_BOOL == ngph.tok[i].type)
                { if (ngph.tok[i].value.b) ngph_node_type = NGP_NODE_IMAGE;
                }
            }
          else if (!strcmp("BITPIX", ngph.tok[i].name))
            { if (NGP_TTYPE_INT == ngph.tok[i].type)  ngph_bitpix = ngph.tok[i].value.i;
            }
          else if (!strcmp("NAXIS", ngph.tok[i].name))
            { if (NGP_TTYPE_INT == ngph.tok[i].type)  ngph_dim = ngph.tok[i].value.i;
            }
          else if (!strcmp("EXTNAME", ngph.tok[i].name))	/* assign EXTNAME, I hope struct does not move */
            { if (NGP_TTYPE_STRING == ngph.tok[i].type)  ngph_extname = ngph.tok[i].value.s;
            }
          else if (1 == sscanf(ngph.tok[i].name, "NAXIS%d%c", &j, &ngph_ctmp))
            { if (NGP_TTYPE_INT == ngph.tok[i].type)
		if ((j>=1) && (j <= NGP_MAX_ARRAY_DIM))
		  { ngph_size[j - 1] = ngph.tok[i].value.i;
		  }
            }
        }

       switch (ngph_node_type)
        { case NGP_NODE_IMAGE:
			if (NGP_XTENSION_FIRST == ((NGP_XTENSION_FIRST | NGP_XTENSION_SIMPLE) & simple_mode))
			  { 		/* if caller signals that this is 1st HDU in file */
					/* and it is IMAGE defined with XTENSION, then we */
					/* need create dummy Primary HDU */			  
			    fits_create_img(ff, 16, 0, NULL, &r);
			  }
					/* create image */
			fits_create_img(ff, ngph_bitpix, ngph_dim, ngph_size, &r);

					/* update keywords */
			if (NGP_OK == r)  r = ngp_keyword_all_write(&ngph, ff, NGP_NON_SYSTEM_ONLY);
			break;

          case NGP_NODE_ATABLE:
          case NGP_NODE_BTABLE:
					/* create table, 0 rows and 0 columns for the moment */
			fits_create_tbl(ff, ((NGP_NODE_ATABLE == ngph_node_type)
					     ? ASCII_TBL : BINARY_TBL),
					0, 0, NULL, NULL, NULL, NULL, &r);
			if (NGP_OK != r) break;

					/* add columns ... */
			r = ngp_append_columns(ff, &ngph, 0);
			if (NGP_OK != r) break;

					/* add remaining keywords */
			r = ngp_keyword_all_write(&ngph, ff, NGP_NON_SYSTEM_ONLY);
			if (NGP_OK != r) break;

					/* if requested add rows */
			if (ngph_size[1] > 0) fits_insert_rows(ff, 0, ngph_size[1], &r);
			break;

	  default:	r = NGP_BAD_ARG;
	  		break;
	}

     }

   if ((NGP_OK == r) && (NULL != ngph_extname))
     { r = ngp_get_extver(ngph_extname, &my_version);	/* write correct ext version number */
       lv = my_version;		/* bugfix - 22-Jan-99, BO - nonalignment of OSF/Alpha */
       fits_write_key(ff, TLONG, "EXTVER", &lv, "auto assigned by template parser", &r); 
     }

   if (NGP_OK == r)
     { if (parent_hn > 0)
         { fits_get_hdu_num(ff, &my_hn);
           fits_movabs_hdu(ff, parent_hn, &tmp0, &r);	/* link us to parent */
           fits_add_group_member(ff, NULL, my_hn, &r);
           fits_movabs_hdu(ff, my_hn, &tmp0, &r);
           if (NGP_OK != r) return(r);
         }
     }

   if (NGP_OK != r)					/* in case of error - delete hdu */
     { tmp0 = 0;
       fits_delete_hdu(ff, NULL, &tmp0);
     }

   ngp_hdu_clear(&ngph);
   return(r);
 }

	/* read complete GROUP */

int	ngp_read_group(fitsfile *ff, char *grpname, int parent_hn)
 { int		r, exitflg, l, my_hn, tmp0, incrementor_index;
   char		grnm[NGP_MAX_STRING];			/* keyword holding group name */
   char		incrementor_name[NGP_MAX_STRING];
   NGP_HDU	ngph;

   incrementor_name[0] = 0;			/* signal no keyword+'#' found yet */
   incrementor_index = 6;			/* first 6 cols are used by group */

   ngp_grplevel++;
   if (NGP_OK != (r = ngp_hdu_init(&ngph))) return(r);

   r = NGP_OK;
   if (NGP_OK != (r = fits_create_group(ff, grpname, GT_ID_ALL_URI, &r))) return(r);
   fits_get_hdu_num(ff, &my_hn);
   if (parent_hn > 0)
     { fits_movabs_hdu(ff, parent_hn, &tmp0, &r);	/* link us to parent */
       fits_add_group_member(ff, NULL, my_hn, &r);
       fits_movabs_hdu(ff, my_hn, &tmp0, &r);
       if (NGP_OK != r) return(r);
     }

   for (exitflg = 0; 0 == exitflg;)
    { if (NGP_OK != (r = ngp_read_line(0))) break;	/* EOF always means error here */
      switch (ngp_keyidx)
       {
	 case NGP_TOKEN_SIMPLE:
	 case NGP_TOKEN_EOF:
			r = NGP_TOKEN_NOT_EXPECT;
			break;

         case NGP_TOKEN_END:
         		ngp_grplevel--;
			exitflg = 1;
			break;

         case NGP_TOKEN_GROUP:
			if (NGP_TTYPE_STRING == ngp_linkey.type)
			  { strncpy(grnm, ngp_linkey.value.s, NGP_MAX_STRING);
			  }
			else
			  { sprintf(grnm, "DEFAULT_GROUP_%d", master_grp_idx++);
			  }
			grnm[NGP_MAX_STRING - 1] = 0;
			r = ngp_read_group(ff, grnm, my_hn);
			break;			/* we can have many subsequent GROUP defs */

         case NGP_TOKEN_XTENSION:
         		r = ngp_unread_line();
         		if (NGP_OK != r) break;
         		r = ngp_read_xtension(ff, my_hn, 0);
			break;			/* we can have many subsequent HDU defs */

         default:	l = strlen(ngp_linkey.name);
			if ((l >= 2) && (l <= 6))
			  { if ('#' == ngp_linkey.name[l - 1])
			      { if (0 == incrementor_name[0])
			          { memcpy(incrementor_name, ngp_linkey.name, l - 1);
			            incrementor_name[l - 1] = 0;
			          }
			        if (((l - 1) == (int)strlen(incrementor_name)) && (0 == memcmp(incrementor_name, ngp_linkey.name, l - 1)))
			          { incrementor_index++;
			          }
			        sprintf(ngp_linkey.name + l - 1, "%d", incrementor_index);
			      }
			  }
         		r = ngp_hdu_insert_token(&ngph, &ngp_linkey); 
			break;			/* here we can add keyword */
       }
      if (NGP_OK != r) break;
    }

   fits_movabs_hdu(ff, my_hn, &tmp0, &r);	/* back to our HDU */

   if (NGP_OK == r)				/* create additional columns, if requested */
     r = ngp_append_columns(ff, &ngph, 6);

   if (NGP_OK == r)				/* and write keywords */
     r = ngp_keyword_all_write(&ngph, ff, NGP_NON_SYSTEM_ONLY);

   if (NGP_OK != r)			/* delete group in case of error */
     { tmp0 = 0;
       fits_remove_group(ff, OPT_RM_GPT, &tmp0);
     }

   ngp_hdu_clear(&ngph);		/* we are done with this HDU, so delete it */
   return(r);
 }

		/* top level API functions */

/* read whole template. ff should point to the opened empty fits file. */

int	fits_execute_template(fitsfile *ff, char *ngp_template, int *status)
 { int		r, exit_flg, first_extension, i, my_hn, tmp0, keys_exist, more_keys, used_ver;
   char		grnm[NGP_MAX_STRING], used_name[NGP_MAX_STRING];
   long		luv;

   if (NULL == status) return(NGP_NUL_PTR);
   if (NGP_OK != *status) return(*status);

   if ((NULL == ff) || (NULL == ngp_template))
     { *status = NGP_NUL_PTR;
       return(*status);
     }

   ngp_inclevel = 0;				/* initialize things, not all should be zero */
   ngp_grplevel = 0;
   master_grp_idx = 1;
   exit_flg = 0;
   ngp_master_dir[0] = 0;			/* this should be before 1st call to ngp_include_file */
   first_extension = 1;				/* we need to create PHDU */

   if (NGP_OK != (r = ngp_delete_extver_tab()))
     { *status = r;
       return(r);
     }

   fits_get_hdu_num(ff, &my_hn);		/* our HDU position */
   if (my_hn <= 1)				/* check whether we really need to create PHDU */
     { fits_movabs_hdu(ff, 1, &tmp0, status);
       fits_get_hdrspace(ff, &keys_exist, &more_keys, status);
       fits_movabs_hdu(ff, my_hn, &tmp0, status);
       if (NGP_OK != *status) return(*status);	/* error here means file is corrupted */
       if (keys_exist > 0) first_extension = 0;	/* if keywords exist assume PHDU already exist */
     }
   else
     { first_extension = 0;			/* PHDU (followed by 1+ extensions) exist */

       for (i = 2; i<= my_hn; i++)
        { *status = NGP_OK;
          fits_movabs_hdu(ff, 1, &tmp0, status);
          if (NGP_OK != *status) break;

          fits_read_key(ff, TSTRING, "EXTNAME", used_name, NULL, status);
          if (NGP_OK != *status)  continue;

          fits_read_key(ff, TLONG, "EXTVER", &luv, NULL, status);
          used_ver = luv;			/* bugfix - 22-Jan-99, BO - nonalignment of OSF/Alpha */
          if (VALUE_UNDEFINED == *status)
            { used_ver = 1;
              *status = NGP_OK;
            }

          if (NGP_OK == *status) *status = ngp_set_extver(used_name, used_ver);
        }

       fits_movabs_hdu(ff, my_hn, &tmp0, status);
     }
   if (NGP_OK != *status) return(*status);
                                                                          
   if (NGP_OK != (*status = ngp_include_file(ngp_template))) return(*status);

   for (i = strlen(ngp_template) - 1; i >= 0; i--) /* strlen is > 0, otherwise fopen failed */
    { 
#ifdef MSDOS
      if ('\\' == ngp_template[i]) break;
#else
      if ('/' == ngp_template[i]) break;
#endif
    } 
      
   i++;
   if (i > (NGP_MAX_FNAME - 1)) i = NGP_MAX_FNAME - 1;

   if (i > 0)
     { memcpy(ngp_master_dir, ngp_template, i);
       ngp_master_dir[i] = 0;
     }


   for (;;)
    { if (NGP_OK != (r = ngp_read_line(1))) break;	/* EOF always means error here */
      switch (ngp_keyidx)
       {
         case NGP_TOKEN_SIMPLE:
			if (0 == first_extension)	/* simple only allowed in first HDU */
			  { r = NGP_TOKEN_NOT_EXPECT;
			    break;
			  }
			if (NGP_OK != (r = ngp_unread_line())) break;
			r = ngp_read_xtension(ff, 0, NGP_XTENSION_SIMPLE | NGP_XTENSION_FIRST);
			first_extension = 0;
			break;

         case NGP_TOKEN_XTENSION:
			if (NGP_OK != (r = ngp_unread_line())) break;
			r = ngp_read_xtension(ff, 0, (first_extension ? NGP_XTENSION_FIRST : 0));
			first_extension = 0;
			break;

         case NGP_TOKEN_GROUP:
			if (NGP_TTYPE_STRING == ngp_linkey.type)
			  { strncpy(grnm, ngp_linkey.value.s, NGP_MAX_STRING); }
			else
			  { sprintf(grnm, "DEFAULT_GROUP_%d", master_grp_idx++); }
			grnm[NGP_MAX_STRING - 1] = 0;
			r = ngp_read_group(ff, grnm, 0);
			first_extension = 0;
			break;

	 case NGP_TOKEN_EOF:
			exit_flg = 1;
			break;

         default:	r = NGP_TOKEN_NOT_EXPECT;
			break;
       }
      if (exit_flg || (NGP_OK != r)) break;
    }

/* all top level HDUs up to faulty one are left intact in case of i/o error. It is up
   to the caller to call fits_close_file or fits_delete_file when this function returns
   error. */

   ngp_free_line();		/* deallocate last line (if any) */
   ngp_free_prevline();		/* deallocate cached line (if any) */
   ngp_delete_extver_tab();	/* delete extver table (if present), error ignored */
   
   *status = r;
   return(r);
 }
