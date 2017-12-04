%{
/************************************************************************/
/*                                                                      */
/*                       CFITSIO Lexical Parser                         */
/*                                                                      */
/* This file is one of 3 files containing code which parses an          */
/* arithmetic expression and evaluates it in the context of an input    */
/* FITS file table extension.  The CFITSIO lexical parser is divided    */
/* into the following 3 parts/files: the CFITSIO "front-end",           */
/* eval_f.c, contains the interface between the user/CFITSIO and the    */
/* real core of the parser; the FLEX interpreter, eval_l.c, takes the   */
/* input string and parses it into tokens and identifies the FITS       */
/* information required to evaluate the expression (ie, keywords and    */
/* columns); and, the BISON grammar and evaluation routines, eval_y.c,  */
/* receives the FLEX output and determines and performs the actual      */
/* operations.  The files eval_l.c and eval_y.c are produced from       */
/* running flex and bison on the files eval.l and eval.y, respectively. */
/* (flex and bison are available from any GNU archive: see www.gnu.org) */
/*                                                                      */
/* The grammar rules, rather than evaluating the expression in situ,    */
/* builds a tree, or Nodal, structure mapping out the order of          */
/* operations and expression dependencies.  This "compilation" process  */
/* allows for much faster processing of multiple rows.  This technique  */
/* was developed by Uwe Lammers of the XMM Science Analysis System,     */
/* although the CFITSIO implementation is entirely code original.       */
/*                                                                      */
/*                                                                      */
/* Modification History:                                                */
/*                                                                      */
/*   Kent Blackburn      c1992  Original parser code developed for the  */
/*                              FTOOLS software package, in particular, */
/*                              the fselect task.                       */
/*   Kent Blackburn      c1995  BIT column support added                */
/*   Peter D Wilson   Feb 1998  Vector column support added             */
/*   Peter D Wilson   May 1998  Ported to CFITSIO library.  User        */
/*                              interface routines written, in essence  */
/*                              making fselect, fcalc, and maketime     */
/*                              capabilities available to all tools     */
/*                              via single function calls.              */
/*   Peter D Wilson   Jun 1998  Major rewrite of parser core, so as to  */
/*                              create a run-time evaluation tree,      */
/*                              inspired by the work of Uwe Lammers,    */
/*                              resulting in a speed increase of        */
/*                              10-100 times.                           */
/*   Peter D Wilson   Jul 1998  gtifilter(a,b,c,d) function added       */
/*   Peter D Wilson   Aug 1998  regfilter(a,b,c,d) function added       */
/*   Peter D Wilson   Jul 1999  Make parser fitsfile-independent,       */
/*                              allowing a purely vector-based usage    */
/*                                                                      */
/************************************************************************/

#include <math.h>
#include <string.h>
#include <ctype.h>
#ifdef sparc
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include "eval_defs.h"

ParseData gParse;     /* Global structure holding all parser information     */

/*****  Internal functions  *****/

       int yyGetVariable( char *varName, YYSTYPE *varVal );

static int find_variable( char *varName );
static int expr_read( char *buf, int nbytes );

/*****  Definitions  *****/

#define YY_NO_UNPUT   /*  Don't include YYUNPUT function  */
#define YY_NEVER_INTERACTIVE 1

#define MAXCHR 256
#define MAXBIT 128

#define OCT_0 "000"
#define OCT_1 "001"
#define OCT_2 "010"
#define OCT_3 "011"
#define OCT_4 "100"
#define OCT_5 "101"
#define OCT_6 "110"
#define OCT_7 "111"
#define OCT_X "xxx"

#define HEX_0 "0000"
#define HEX_1 "0001"
#define HEX_2 "0010"
#define HEX_3 "0011"
#define HEX_4 "0100"
#define HEX_5 "0101"
#define HEX_6 "0110"
#define HEX_7 "0111"
#define HEX_8 "1000"
#define HEX_9 "1001"
#define HEX_A "1010"
#define HEX_B "1011"
#define HEX_C "1100"
#define HEX_D "1101"
#define HEX_E "1110"
#define HEX_F "1111"
#define HEX_X "xxxx"

/* 
   MJT - 13 June 1996
   read from buffer instead of stdin
   (as per old ftools.skel)
*/
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
        if ( (result = expr_read( (char *) buf, max_size )) < 0 ) \
            YY_FATAL_ERROR( "read() in flex scanner failed" );

%}
bit		([bB][01xX]+)
oct		([oO][01234567xX]+)
hex		([hH][0123456789aAbBcCdDeEfFxX]+)
integer		[0-9]+
boolean         (t|f|T|F)
real		([0-9]*"."[0-9]+)|([0-9]*"."*[0-9]+[eEdD][+-]?[0-9]+)|([0-9]*".")
constant        ("#"[a-zA-Z0-9_]+)|("#""$"[^\n]*"$")
string		([\"][^\"\n]*[\"])|([\'][^\'\n]*[\'])
variable	([a-zA-Z_][a-zA-Z0-9_]*)|("$"[^$\n]*"$")
function	[a-zA-Z][a-zA-Z0-9]+"("
intcast		("(int)"|"(INT)")
fltcast		("(float)"|"(FLOAT)"|"(double)"|"(DOUBLE)")
power		("^"|"**")
not             ("!"|".not."|".NOT."|"not."|"NOT.")
or              ("||"|".or."|".OR."|"or."|"OR.")
and             ("&&"|".and."|".AND."|"and."|"AND.")
equal		("=="|".eq."|".EQ."|"eq."|"EQ.")
not_equal	("!="|".ne."|".NE."|"ne."|"NE.")
greater         (">"|".gt."|".GT."|"gt."|"GT.")
lesser          ("<"|".lt."|".LT."|"lt."|"LT.")
greater_eq	(">="|"=>"|".ge."|".GE."|"ge."|"GE.")
lesser_eq	("<="|"=<"|".le."|".LE."|"le."|"LE.")
nl		\n

%%

[ \t]+     ;
{bit}		{
                  int len;
                  len = strlen(yytext);
		  while (yytext[len] == ' ')
			len--;
                  len = len - 1;
		  strncpy(yylval.str,&yytext[1],len);
		  yylval.str[len] = '\0';
		  return( BITSTR );
		}
{oct}		{
                  int len;
                  char tmpstring[256];
                  char bitstring[256];
                  len = strlen(yytext);
		  if (len >= 256) {
		    char errMsg[100];
		    gParse.status = PARSE_SYNTAX_ERR;
		    strcpy (errMsg,"Bit string exceeds maximum length: '");
		    strncat(errMsg, &(yytext[0]), 20);
		    strcat (errMsg,"...'");
		    ffpmsg (errMsg);
		    len = 0;
		  } else {
		    while (yytext[len] == ' ')
		      len--;
		    len = len - 1;
		    strncpy(tmpstring,&yytext[1],len);
		  }
                  tmpstring[len] = '\0';
                  bitstring[0] = '\0';
		  len = 0;
                  while ( tmpstring[len] != '\0')
                       {
			switch ( tmpstring[len] )
			      {
			       case '0':
					strcat(bitstring,OCT_0);
					break;
			       case '1':
					strcat(bitstring,OCT_1);
					break;
			       case '2':
					strcat(bitstring,OCT_2);
					break;
			       case '3':
					strcat(bitstring,OCT_3);
					break;
			       case '4':
					strcat(bitstring,OCT_4);
					break;
			       case '5':
					strcat(bitstring,OCT_5);
					break;
			       case '6':
					strcat(bitstring,OCT_6);
					break;
			       case '7':
					strcat(bitstring,OCT_7);
					break;
			       case 'x':
			       case 'X':
					strcat(bitstring,OCT_X);
					break;
			      }
			len++;
                       }
                  strcpy( yylval.str, bitstring );
		  return( BITSTR );
		}
{hex}		{
                  int len;
                  char tmpstring[256];
                  char bitstring[256];
                  len = strlen(yytext);
		  if (len >= 256) {
		    char errMsg[100];
		    gParse.status = PARSE_SYNTAX_ERR;
		    strcpy (errMsg,"Hex string exceeds maximum length: '");
		    strncat(errMsg, &(yytext[0]), 20);
		    strcat (errMsg,"...'");
		    ffpmsg (errMsg);
		    len = 0;
		  } else {
		    while (yytext[len] == ' ')
		      len--;
		    len = len - 1;
		    strncpy(tmpstring,&yytext[1],len);
		  }
                  tmpstring[len] = '\0';
                  bitstring[0] = '\0';
		  len = 0;
                  while ( tmpstring[len] != '\0')
                       {
			switch ( tmpstring[len] )
			      {
			       case '0':
					strcat(bitstring,HEX_0);
					break;
			       case '1':
					strcat(bitstring,HEX_1);
					break;
			       case '2':
					strcat(bitstring,HEX_2);
					break;
			       case '3':
					strcat(bitstring,HEX_3);
					break;
			       case '4':
					strcat(bitstring,HEX_4);
					break;
			       case '5':
					strcat(bitstring,HEX_5);
					break;
			       case '6':
					strcat(bitstring,HEX_6);
					break;
			       case '7':
					strcat(bitstring,HEX_7);
					break;
			       case '8':
					strcat(bitstring,HEX_8);
					break;
			       case '9':
					strcat(bitstring,HEX_9);
					break;
			       case 'a':
			       case 'A':
					strcat(bitstring,HEX_A);
					break;
			       case 'b':
			       case 'B':
					strcat(bitstring,HEX_B);
					break;
			       case 'c':
			       case 'C':
					strcat(bitstring,HEX_C);
					break;
			       case 'd':
			       case 'D':
					strcat(bitstring,HEX_D);
					break;
			       case 'e':
			       case 'E':
					strcat(bitstring,HEX_E);
					break;
			       case 'f':
			       case 'F':
					strcat(bitstring,HEX_F);
					break;
			       case 'x':
			       case 'X':
					strcat(bitstring,HEX_X);
					break;
			      }
			len++;
                       }

                  strcpy( yylval.str, bitstring );
		  return( BITSTR );
		}
{integer}	{
                  yylval.lng = atol(yytext);
		  return( LONG );
		}
{boolean}	{
                  if ((yytext[0] == 't') || (yytext[0] == 'T'))
		    yylval.log = 1;
		  else
		    yylval.log = 0;
		  return( BOOLEAN );
		}
{real}		{
                  yylval.dbl = atof(yytext);
		  return( DOUBLE );
		}
{constant}	{
                  if(        !fits_strcasecmp(yytext,"#PI") ) {
		     yylval.dbl = (double)(4) * atan((double)(1));
		     return( DOUBLE );
		  } else if( !fits_strcasecmp(yytext,"#E") ) {
		     yylval.dbl = exp((double)(1));
		     return( DOUBLE );
		  } else if( !fits_strcasecmp(yytext,"#DEG") ) {
		     yylval.dbl = ((double)4)*atan((double)1)/((double)180);
		     return( DOUBLE );
		  } else if( !fits_strcasecmp(yytext,"#ROW") ) {
		     return( ROWREF );
		  } else if( !fits_strcasecmp(yytext,"#NULL") ) {
		     return( NULLREF );
		  } else if( !fits_strcasecmp(yytext,"#SNULL") ) {
		     return( SNULLREF );
		  } else {
                     int len; 
                     if (yytext[1] == '$') {
                        len = strlen(yytext) - 3;
                        yylval.str[0]     = '#';
                        strncpy(yylval.str+1,&yytext[2],len);
                        yylval.str[len+1] = '\0';
                        yytext = yylval.str;
		     }
                     return( (*gParse.getData)(yytext, &yylval) );
                  }
                }
{string}	{
                  int len;
                  len = strlen(yytext) - 2;
		  if (len >= MAX_STRLEN) {
		    char errMsg[100];
		    gParse.status = PARSE_SYNTAX_ERR;
		    strcpy (errMsg,"String exceeds maximum length: '");
		    strncat(errMsg, &(yytext[1]), 20);
		    strcat (errMsg,"...'");
		    ffpmsg (errMsg);
		    len = 0;
		  } else {
		    strncpy(yylval.str,&yytext[1],len);
		  }
		  yylval.str[len] = '\0';
		  return( STRING );
		}
{variable}	{
		 int    len,type;

                 if (yytext[0] == '$') {
		    len = strlen(yytext) - 2;
		    strncpy(yylval.str,&yytext[1],len);
		    yylval.str[len] = '\0';
		    yytext = yylval.str;
		 } 
		 type = yyGetVariable(yytext, &yylval);
		 return( type );
		}
{function}	{
                  char *fname;
		  int len=0;
                  fname = &yylval.str[0];
		  while( (fname[len]=toupper(yytext[len])) ) len++;

                  if(      FSTRCMP(fname,"BOX(")==0 
                        || FSTRCMP(fname,"CIRCLE(")==0 
                        || FSTRCMP(fname,"ELLIPSE(")==0 
                        || FSTRCMP(fname,"NEAR(")==0 
                        || FSTRCMP(fname,"ISNULL(")==0 
                         )
                     /* Return type is always boolean  */
		     return( BFUNCTION );

                  else if( FSTRCMP(fname,"GTIFILTER(")==0 )
                     return( GTIFILTER );

                  else if( FSTRCMP(fname,"REGFILTER(")==0 )
                     return( REGFILTER );

                  else if( FSTRCMP(fname,"STRSTR(")==0 )
                     return( IFUNCTION );  /* Returns integer */

                  else 
		     return( FUNCTION  );
		}
{intcast}	{ return( INTCAST ); }
{fltcast}	{ return( FLTCAST ); }
{power}		{ return( POWER   ); }
{not}		{ return( NOT     ); }
{or}		{ return( OR      ); }
{and}		{ return( AND     ); }
{equal}		{ return( EQ      ); }
{not_equal}	{ return( NE      ); }
{greater}	{ return( GT      ); }
{lesser}	{ return( LT      ); }
{greater_eq}	{ return( GTE     ); }
{lesser_eq}	{ return( LTE     ); }
{nl}		{ return( '\n'    ); }
.		{ return( yytext[0] ); }
%%

int yywrap()
{
  /* MJT -- 13 June 1996
     Supplied for compatibility with
     pre-2.5.1 versions of flex which
     do not recognize %option noyywrap 
  */
  return(1);
}

/* 
   expr_read is lifted from old ftools.skel. 
   Now we can use any version of flex with
   no .skel file necessary! MJT - 13 June 1996

   keep a memory of how many bytes have been
   read previously, so that an unlimited-sized
   buffer can be supported. PDW - 28 Feb 1998
*/

static int expr_read(char *buf, int nbytes)
{
 int n;
 
 n = 0;
 if( !gParse.is_eobuf ) {
     do {
        buf[n++] = gParse.expr[gParse.index++];
       } while ((n<nbytes)&&(gParse.expr[gParse.index] != '\0'));
     if( gParse.expr[gParse.index] == '\0' ) gParse.is_eobuf = 1;
 }
 buf[n] = '\0';
 return(n);
}

int yyGetVariable( char *varName, YYSTYPE *thelval )
{
   int varNum, type;
   char errMsg[MAXVARNAME+25];

   varNum = find_variable( varName );
   if( varNum<0 ) {
      if( gParse.getData ) {
	 type = (*gParse.getData)( varName, thelval );
      } else {
	 type = pERROR;
	 gParse.status = PARSE_SYNTAX_ERR;
	 strcpy (errMsg,"Unable to find data: ");
	 strncat(errMsg, varName, MAXVARNAME);
	 ffpmsg (errMsg);
      }
   } else {
      /*  Convert variable type into expression type  */
      switch( gParse.varData[ varNum ].type ) {
      case LONG:
      case DOUBLE:   type =  COLUMN;  break;
      case BOOLEAN:  type = BCOLUMN;  break;
      case STRING:   type = SCOLUMN;  break;
      case BITSTR:   type =  BITCOL;  break;
      default:
	 type = pERROR;
	 gParse.status = PARSE_SYNTAX_ERR;
	 strcpy (errMsg,"Bad datatype for data: ");
	 strncat(errMsg, varName, MAXVARNAME);
	 ffpmsg (errMsg);
	 break;
      }
      thelval->lng = varNum;
   }
   return( type );
}

static int find_variable(char *varName)
{
   int i;
 
   if( gParse.nCols )
      for( i=0; i<gParse.nCols; i++ ) {
         if( ! fits_strncasecmp(gParse.varData[i].name,varName,MAXVARNAME) ) {
            return( i );
         }
      }
   return( -1 );
}
