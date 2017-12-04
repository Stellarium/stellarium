/*		T E M P L A T E   P A R S E R   H E A D E R   F I L E
		=====================================================

		by Jerzy.Borkowski@obs.unige.ch

		Integral Science Data Center
		ch. d'Ecogia 16
		1290 Versoix
		Switzerland

14-Oct-98: initial release
16-Oct-98: reference to fitsio.h removed, also removed strings after #endif
		directives to make gcc -Wall not to complain
20-Oct-98: added declarations NGP_XTENSION_SIMPLE and NGP_XTENSION_FIRST
24-Oct-98: prototype of ngp_read_line() function updated.
22-Jan-99: prototype for ngp_set_extver() function added.
20-Jun-2002 Wm Pence, added support for the HIERARCH keyword convention
            (changed NGP_MAX_NAME from (20) to FLEN_KEYWORD)
*/

#ifndef	GRPARSER_H_INCLUDED
#define	GRPARSER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

	/* error codes  - now defined in fitsio.h */

	/* common constants definitions */

#define	NGP_ALLOCCHUNK		(1000)
#define	NGP_MAX_INCLUDE		(10)			/* include file nesting limit */
#define	NGP_MAX_COMMENT		(80)			/* max size for comment */
#define	NGP_MAX_NAME		FLEN_KEYWORD		/* max size for KEYWORD (FITS limits it to 8 chars) */
                                                        /* except HIERARCH can have longer effective keyword names */
#define	NGP_MAX_STRING		(80)			/* max size for various strings */
#define	NGP_MAX_ARRAY_DIM	(999)			/* max. number of dimensions in array */
#define NGP_MAX_FNAME           (1000)                  /* max size of combined path+fname */
#define	NGP_MAX_ENVFILES	(10000)			/* max size of CFITSIO_INCLUDE_FILES env. variable */

#define	NGP_TOKEN_UNKNOWN	(-1)			/* token type unknown */
#define	NGP_TOKEN_INCLUDE	(0)			/* \INCLUDE token */
#define	NGP_TOKEN_GROUP		(1)			/* \GROUP token */
#define	NGP_TOKEN_END		(2)			/* \END token */
#define	NGP_TOKEN_XTENSION	(3)			/* XTENSION token */
#define	NGP_TOKEN_SIMPLE	(4)			/* SIMPLE token */
#define	NGP_TOKEN_EOF		(5)			/* End Of File pseudo token */

#define	NGP_TTYPE_UNKNOWN	(0)			/* undef (yet) token type - invalid to print/write to disk */
#define	NGP_TTYPE_BOOL		(1)			/* boolean, it is 'T' or 'F' */
#define	NGP_TTYPE_STRING	(2)			/* something withing "" or starting with letter */
#define	NGP_TTYPE_INT		(3)			/* starting with digit and not with '.' */
#define	NGP_TTYPE_REAL		(4)			/* digits + '.' */
#define	NGP_TTYPE_COMPLEX	(5)			/* 2 reals, separated with ',' */
#define	NGP_TTYPE_NULL		(6)			/* NULL token, format is : NAME = / comment */
#define	NGP_TTYPE_RAW		(7)			/* HISTORY/COMMENT/8SPACES + comment string without / */

#define	NGP_FOUND_EQUAL_SIGN	(1)			/* line contains '=' after keyword name */

#define	NGP_FORMAT_OK		(0)			/* line format OK */
#define	NGP_FORMAT_ERROR	(1)			/* line format error */

#define	NGP_NODE_INVALID	(0)			/* default node type - invalid (to catch errors) */
#define	NGP_NODE_IMAGE		(1)			/* IMAGE type */
#define	NGP_NODE_ATABLE		(2)			/* ASCII table type */
#define	NGP_NODE_BTABLE		(3)			/* BINARY table type */

#define	NGP_NON_SYSTEM_ONLY	(0)			/* save all keywords except NAXIS,BITPIX,etc.. */
#define	NGP_REALLY_ALL		(1)			/* save really all keywords */

#define	NGP_XTENSION_SIMPLE	(1)			/* HDU defined with SIMPLE T */
#define	NGP_XTENSION_FIRST	(2)			/* this is first extension in template */

#define	NGP_LINE_REREAD		(1)			/* reread line */

#define	NGP_BITPIX_INVALID	(-12345)		/* default BITPIX (to catch errors) */

	/* common macro definitions */

#ifdef	NGP_PARSER_DEBUG_MALLOC

#define	ngp_alloc(x)		dal_malloc(x)
#define	ngp_free(x)		dal_free(x)
#define	ngp_realloc(x,y)	dal_realloc(x,y)

#else

#define	ngp_alloc(x)		malloc(x)
#define	ngp_free(x)		free(x)
#define	ngp_realloc(x,y)	realloc(x,y)

#endif

	/* type definitions */

typedef struct NGP_RAW_LINE_STRUCT
      {	char	*line;
	char	*name;
	char	*value;
	int	type;
	char	*comment;
	int	format;
	int	flags;
      } NGP_RAW_LINE;


typedef union NGP_TOKVAL_UNION
      {	char	*s;		/* space allocated separately, be careful !!! */
	char	b;
	int	i;
	double	d;
	struct NGP_COMPLEX_STRUCT
	 { double re;
	   double im;
	 } c;			/* complex value */
      } NGP_TOKVAL;


typedef struct NGP_TOKEN_STRUCT
      { int		type;
        char		name[NGP_MAX_NAME];
        NGP_TOKVAL	value;
        char		comment[NGP_MAX_COMMENT];
      } NGP_TOKEN;


typedef struct NGP_HDU_STRUCT
      {	int		tokcnt;
        NGP_TOKEN	*tok;
      } NGP_HDU;


typedef struct NGP_TKDEF_STRUCT
      {	char	*name;
	int	code;
      } NGP_TKDEF;


typedef struct NGP_EXTVER_TAB_STRUCT
      {	char	*extname;
	int	version;
      } NGP_EXTVER_TAB;


	/* globally visible variables declarations */

extern	NGP_RAW_LINE	ngp_curline;
extern	NGP_RAW_LINE	ngp_prevline;

extern	int		ngp_extver_tab_size;
extern	NGP_EXTVER_TAB	*ngp_extver_tab;


	/* globally visible functions declarations */

int	ngp_get_extver(char *extname, int *version);
int	ngp_set_extver(char *extname, int version);
int	ngp_delete_extver_tab(void);
int	ngp_line_from_file(FILE *fp, char **p);
int	ngp_free_line(void);
int	ngp_free_prevline(void);
int	ngp_read_line_buffered(FILE *fp);
int	ngp_unread_line(void);
int	ngp_extract_tokens(NGP_RAW_LINE *cl);
int	ngp_include_file(char *fname);
int	ngp_read_line(int ignore_blank_lines);
int	ngp_keyword_is_write(NGP_TOKEN *ngp_tok);
int     ngp_keyword_all_write(NGP_HDU *ngph, fitsfile *ffp, int mode);
int	ngp_hdu_init(NGP_HDU *ngph);
int	ngp_hdu_clear(NGP_HDU *ngph);
int	ngp_hdu_insert_token(NGP_HDU *ngph, NGP_TOKEN *newtok);
int	ngp_append_columns(fitsfile *ff, NGP_HDU *ngph, int aftercol);
int	ngp_read_xtension(fitsfile *ff, int parent_hn, int simple_mode);
int	ngp_read_group(fitsfile *ff, char *grpname, int parent_hn);

		/* top level API function - now defined in fitsio.h */

#ifdef __cplusplus
}
#endif

#endif
