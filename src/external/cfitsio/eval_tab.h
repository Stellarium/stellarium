
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef FFTOKENTYPE
# define FFTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum fftokentype {
     BOOLEAN = 258,
     LONG = 259,
     DOUBLE = 260,
     STRING = 261,
     BITSTR = 262,
     FUNCTION = 263,
     BFUNCTION = 264,
     IFUNCTION = 265,
     GTIFILTER = 266,
     REGFILTER = 267,
     COLUMN = 268,
     BCOLUMN = 269,
     SCOLUMN = 270,
     BITCOL = 271,
     ROWREF = 272,
     NULLREF = 273,
     SNULLREF = 274,
     OR = 275,
     AND = 276,
     NE = 277,
     EQ = 278,
     GTE = 279,
     LTE = 280,
     LT = 281,
     GT = 282,
     POWER = 283,
     NOT = 284,
     FLTCAST = 285,
     INTCAST = 286,
     UMINUS = 287,
     DIFF = 288,
     ACCUM = 289
   };
#endif
/* Tokens.  */
#define BOOLEAN 258
#define LONG 259
#define DOUBLE 260
#define STRING 261
#define BITSTR 262
#define FUNCTION 263
#define BFUNCTION 264
#define IFUNCTION 265
#define GTIFILTER 266
#define REGFILTER 267
#define COLUMN 268
#define BCOLUMN 269
#define SCOLUMN 270
#define BITCOL 271
#define ROWREF 272
#define NULLREF 273
#define SNULLREF 274
#define OR 275
#define AND 276
#define NE 277
#define EQ 278
#define GTE 279
#define LTE 280
#define LT 281
#define GT 282
#define POWER 283
#define NOT 284
#define FLTCAST 285
#define INTCAST 286
#define UMINUS 287
#define DIFF 288
#define ACCUM 289




#if ! defined FFSTYPE && ! defined FFSTYPE_IS_DECLARED
typedef union FFSTYPE
{

/* Line 1676 of yacc.c  */
#line 192 "eval.y"

    int    Node;        /* Index of Node */
    double dbl;         /* real value    */
    long   lng;         /* integer value */
    char   log;         /* logical value */
    char   str[MAX_STRLEN];    /* string value  */



/* Line 1676 of yacc.c  */
#line 130 "y.tab.h"
} FFSTYPE;
# define FFSTYPE_IS_TRIVIAL 1
# define ffstype FFSTYPE /* obsolescent; will be withdrawn */
# define FFSTYPE_IS_DECLARED 1
#endif

extern FFSTYPE fflval;


