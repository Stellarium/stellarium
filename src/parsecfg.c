/**************************************************************************/
/*                                                                        */
/*  parsecfg - a library for parsing a configuration file                 */
/*  Copyright (C) 1999-2002 Yuuki NINOMIYA <gm@debian.or.jp>              */
/*                                                                        */
/*  This program is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU General Public License as published by  */
/*  the Free Software Foundation; either version 2, or (at your option)   */
/*  any later version.                                                    */
/*                                                                        */
/*  This program is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*  GNU General Public License for more details.                          */
/*                                                                        */
/*  You should have received a copy of the GNU General Public License     */
/*  along with this program; if not, write to the                         */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,          */
/*  Boston, MA 02111-1307, USA.                                           */
/*                                                                        */
/**************************************************************************/

/* $Id: parsecfg.c 2 2002-07-12 17:35:08Z xalioth $ */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "intl.h"
#include "parsecfg.h"


/* proto type declaration of private functions */

static void cfgFatalFunc(cfgErrorCode, const char *, int, const char *);
static void (*cfgFatal) (cfgErrorCode, const char *, int, const char *) = cfgFatalFunc;	/* default error handler */

static int parse_simple(const char *file, FILE *fp, char *ptr, cfgStruct cfg[], int *line);
static int parse_values_between_braces(const char *file, FILE *fp, const char *parameter, cfgStruct cfg[], int *line, cfgFileType type, int section, const char *parameter_buf, int parameter_line);
static char *parse_word(char *ptr, char **word, cfgKeywordValue word_type);
static int store_value(cfgStruct cfg[], const char *parameter, const char *value, cfgFileType type, int section);
static int parse_ini(const char *file, FILE *fp, char *ptr, cfgStruct cfg[], int *line, int *section);
static int alloc_for_new_section(cfgStruct cfg[], int *section);
static void dealloc_for_sections(void);

static char *get_single_line_without_first_spaces(FILE *fp, char **gotstr, int *line);
static char *rm_first_spaces(char *ptr);
static char *dynamic_fgets(FILE *fp);

static int dump_simple(FILE *fp, cfgStruct cfg[], cfgFileType type);
static int dump_ini(FILE *fp, cfgStruct cfg[], cfgFileType type, int max);
static void single_or_double_quote(const char *str, char *ret);

static int fetch_simple(const char *file, FILE *fp, char *parameter_name, void *result_value, cfgValueType value_type);
static int fetch_ini(const char *file, FILE *fp, char *parameter_name, void *result_value, cfgValueType value_type, int section_num, const char *section_name);
static int fetch_value(const char *file, int *line, char *line_buf, FILE *fp, int *store_flag, char *ptr, cfgStruct *fetch_cfg);


/* static variables */

static char **parsecfg_section_name = NULL;
static int parsecfg_maximum_section = 0;


/*************************************************************/
/*                      PUBLIC FUCNCTIONS                    */
/*************************************************************/

/* --------------------------------------------------
   NAME       cfgSetFatalFunc
   FUNCTION   handle new error handler
   INPUT      f ... pointer to new error handler
   OUTPUT     none
   -------------------------------------------------- */
void cfgSetFatalFunc(void (*f) (cfgErrorCode, const char *, int, const char *))
{
	cfgFatal = f;
}


/* --------------------------------------------------
   NAME       cfgParse
   FUNCTION   parse a configuration file (main function)
   INPUT      file ... name of the configuration file
   cfg .... array of possible variables
   type ... type of the configuration file
              + CFG_SIMPLE .. simple 1:1
              + CFG_INI ..... Windows INI-like file
   OUTPUT     the maximum number of sections
              (if type==CFG_INI then return 0)
              -1 if an error occured
   -------------------------------------------------- */
int cfgParse(const char *file, cfgStruct cfg[], cfgFileType type)
{
	char *line_buf;
	char *ptr;
	int line = 0;
	FILE *fp;
	int error_code;
	int max_cfg = -1;

	fp = fopen(file, "r");
	if (fp == NULL) {
		cfgFatal(CFG_OPEN_FAIL, file, 0, NULL);
		return (-1);
	}

	while ((ptr = get_single_line_without_first_spaces(fp, &line_buf, &line)) != NULL) {
		switch (type) {
		case CFG_SIMPLE:
#if 0
			/* proceed even if there is unrecognized parameter */
			if ((error_code = parse_simple(file, fp, ptr, cfg, &line)) == CFG_WRONG_PARAMETER) {
				cfgFatal(error_code, file, line, line_buf);
			} else if (error_code != CFG_NO_ERROR) {
				fclose(fp);
				cfgFatal(error_code, file, line, line_buf);
				return (-1);
			}
#endif
			if ((error_code = parse_simple(file, fp, ptr, cfg, &line)) != CFG_NO_ERROR) {
				fclose(fp);
				cfgFatal(error_code, file, line, line_buf);
				return (-1);
			}
			break;
		case CFG_INI:
			if ((error_code = parse_ini(file, fp, ptr, cfg, &line, &max_cfg)) != CFG_NO_ERROR) {
				fclose(fp);
				cfgFatal(error_code, file, line, line_buf);
				return (-1);
			}
			break;
		default:
			fclose(fp);
			cfgFatal(CFG_INTERNAL_ERROR, file, 0, NULL);
			return (-1);
		}
		free(line_buf);
	}
	fclose(fp);
	parsecfg_maximum_section = max_cfg + 1;
	return (parsecfg_maximum_section);
}


/* --------------------------------------------------
   NAME       cfgDump
   FUNCTION   write configuration data to a file
   INPUT      file ... name of the configuration file
              cfg .... array of possible variables
              type ... type of the configuration file
                          + CFG_SIMPLE .. simple 1:1
                          + CFG_INI ..... Windows INI-like file
              max_section ... the maximum number of sections
                              (if type is CFG_INI, this arg is ignored)
   OUTPUT     0 on success and -1 on error
   -------------------------------------------------- */
int cfgDump(const char *file, cfgStruct cfg[], cfgFileType type, int max_section)
{
	FILE *fp;
	int retcode;

	fp = fopen(file, "w");
	if (fp == NULL) {
		cfgFatal(CFG_CREATE_FAIL, file, 0, NULL);
		return (-1);
	}

	switch (type) {
	case CFG_SIMPLE:
		retcode = dump_simple(fp, cfg, type);
		break;
	case CFG_INI:
		retcode = dump_ini(fp, cfg, type, max_section);
		break;
	default:
		fclose(fp);
		cfgFatal(CFG_INTERNAL_ERROR, file, 0, NULL);
		return (-1);
	}

	fclose(fp);
	return (retcode);
}


/* --------------------------------------------------
   NAME       fetchVarFromCfgFile
   FUNCTION   fetch specified variable from configuration file
   INPUT      file ... name of the configuration file
              parameter_name ... parameter name to fetch
              result_value ... stored result
              value_type ... type of variable
              file_type ... type of the configuration file
                              + CFG_SIMPLE .. simple 1:1
                              + CFG_INI ..... Windows INI-like file
              section_num ... section number (for file_type==CFG_INI)
              section_name ... section name (use this if section_num<1)
   OUTPUT     return 0 if successful, -1 otherwise
   -------------------------------------------------- */
int fetchVarFromCfgFile(const char *file, char *parameter_name, void *result_value, cfgValueType value_type, cfgFileType file_type, int section_num, const char *section_name)
{
	FILE *fp;

	fp = fopen(file, "r");
	if (fp == NULL) {
		cfgFatal(CFG_OPEN_FAIL, file, 0, NULL);
		return (-1);
	}

	switch (file_type) {
	case CFG_SIMPLE:
		if (fetch_simple(file, fp, parameter_name, result_value, value_type) == 0) {
			fclose(fp);
			return (0);
		}
		break;
	case CFG_INI:
		if (fetch_ini(file, fp, parameter_name, result_value, value_type, section_num, section_name) == 0) {
			fclose(fp);
			return (0);
		}
		break;
	default:
		fclose(fp);
		cfgFatal(CFG_INTERNAL_ERROR, file, 0, NULL);
		return (-1);
	}
	fclose(fp);
	return (-1);
}


/* --------------------------------------------------
   NAME       cfgSectionNameToNumber
   FUNCTION   convert section name to number
   INPUT      name ... section name
   OUTPUT     section number (0,1,2,...)
              if no matching, return -1
   -------------------------------------------------- */
int cfgSectionNameToNumber(const char *name)
{
	int i;

	for (i = 0; i < parsecfg_maximum_section; i++) {
		if (strcasecmp(name, parsecfg_section_name[i]) == 0) {
			return (i);
		}
	}
	return (-1);
}


/* --------------------------------------------------
   NAME       cfgSectionNumberToName
   FUNCTION   convert section number to name
   INPUT      num ... section number (0,1,2,...)
   OUTPUT     pointer to section name
              if no section, return NULL
   -------------------------------------------------- */
char *cfgSectionNumberToName(int num)
{
	if (num > parsecfg_maximum_section - 1 || num < 0) {
		return (NULL);
	}
	return (parsecfg_section_name[num]);
}


/* --------------------------------------------------
   NAME       cfgAllocForNewSection
   FUNCTION   allocate memory for new section
   INPUT      cfg ... array of possible variables
              name .. name of new section
   OUTPUT     the maximum number of sections
              -1 if an error occured
   -------------------------------------------------- */
int cfgAllocForNewSection(cfgStruct cfg[], const char *name)
{
	int result;
	int section;

	section = parsecfg_maximum_section - 1;

	result = alloc_for_new_section(cfg, &section);
	if (result != CFG_NO_ERROR) {
		cfgFatalFunc(result, "unknown", 0, "");
		return (-1);
	}
	parsecfg_maximum_section = section + 1;

	parsecfg_section_name = realloc(parsecfg_section_name, sizeof(char *) * parsecfg_maximum_section);
	parsecfg_section_name[parsecfg_maximum_section - 1] = strdup(name);
	return (parsecfg_maximum_section);
}


/* --------------------------------------------------
   NAME       cfgStoreValue
   FUNCTION   store the value according to cfg
   INPUT      cfg ......... array of possible variables
              parameter ... parameter
	      value ....... value
	      type ........ type of the configuration file
	      section ..... section number (0,1,2,...)
   OUTPUT     return 0 if successful, -1 otherwise
   -------------------------------------------------- */
int cfgStoreValue(cfgStruct cfg[], const char *parameter, const char *value, cfgFileType type, int section)
{
	int result;

	result = store_value(cfg, parameter, value, type, section);
	if (result != CFG_NO_ERROR) {
		cfgFatalFunc(result, "unknown", 0, "");
		return (-1);
	}
	return (0);
}


/* --------------------------------------------------
   NAME       cfgStrError
   FUNCTION   convertparsecfg error number to string
   INPUT      cfgErrorCode .. errorcode
   OUTPUT     pointer to non modifiable error string
   -------------------------------------------------- */
const char *cfgStrError(cfgErrorCode error_code)
{
	const char *retMsg;

	switch (error_code) {
	case CFG_OPEN_FAIL:
		retMsg = _("Cannot open configuration file");
		break;
	case CFG_CREATE_FAIL:
		retMsg = _("Cannot create configuration file");
		break;
	case CFG_SYNTAX_ERROR:
		retMsg = _("Syntax error");
		break;
	case CFG_WRONG_PARAMETER:
		retMsg = _("Unrecognized parameter");
		break;
	case CFG_INTERNAL_ERROR:
		retMsg = _("Internal error");
		break;
	case CFG_INVALID_NUMBER:
		retMsg = _("Invalid number");
		break;
	case CFG_OUT_OF_RANGE:
		retMsg = _("Out of range");
		break;
	case CFG_MEM_ALLOC_FAIL:
		retMsg = _("Cannot allocate memory");
		break;
	case CFG_BOOL_ERROR:
		retMsg = _("It must be specified TRUE or FALSE");
		break;
	case CFG_USED_SECTION:
		retMsg = _("The section name is already used");
		break;
	case CFG_NO_CLOSING_BRACE:
		retMsg = _("There is no closing brace");
		break;
	case CFG_JUST_RETURN_WITHOUT_MSG:
		retMsg = _("Unspecified error");
		break;
	default:
		retMsg = _("Unknown error");
		break;
	}

	return (retMsg);
}

/* --------------------------------------------------
   NAME       cfgFree
   FUNCTION   store the value according to cfg
   INPUT      cfg .......... array of possible variables
	      type ......... type of the configuration file
	      numSections .. Number of sections (1,2,...)
   OUTPUT     void
   -------------------------------------------------- */
void cfgFree(cfgStruct cfg[], cfgFileType type, int numSections)
{
	int num, section;
	cfgList *listptr;
	cfgList *next;

	switch(type) {
	case CFG_INI:
		for (num = 0; cfg[num].type != CFG_END; num++) {
			if ((*(void **) cfg[num].value) != NULL) {
				for (section = 0; section < numSections; section++) {
					switch (cfg[num].type) {
					case CFG_STRING:
						if (*((*(char ***) (cfg[num].value)) + section) != NULL) {
							free(*((*(char ***) (cfg[num].value)) + section));
						}
						break;
					case CFG_STRING_LIST:
						for (listptr = *(*(cfgList ***) (cfg[num].value) + section); listptr != NULL;) {
							free(listptr->str);
							next = listptr->next;
							free(listptr);
							listptr = next;
						}
					default:
						break;
					}
				}
				free(*(void **)(cfg[num].value));
				*(void **)(cfg[num].value) = NULL;
			}
		}
		dealloc_for_sections();
		break;
	case CFG_SIMPLE:
		/* TODO: support this (seems difficult ...) */
		break;
	}
}


/*************************************************************/
/*                     PRIVATE FUCNCTIONS                    */
/*************************************************************/

/* --------------------------------------------------
   NAME       cfgFatalFunc
   FUNCTION   error handler
   INPUT      error_code ... reason of error
              file ......... the configuration file
              line ......... line number causing error
              str .......... strings at above line
   OUTPUT     none
   -------------------------------------------------- */
static void cfgFatalFunc(cfgErrorCode error_code, const char *file, int line, const char *str)
{
	switch (error_code) {
	case CFG_OPEN_FAIL:
	case CFG_CREATE_FAIL:
		fprintf(stderr, "%s : %s\n", cfgStrError(error_code), file);
		break;
	case CFG_NO_CLOSING_BRACE:
		fprintf(stderr, "%s(%d)\n%s\n", file, line, cfgStrError(error_code));
		break;
	default:
		fprintf(stderr, "%s(%d): %s\n%s\n", file, line, str, cfgStrError(error_code));
		break;
	}
}


/* --------------------------------------------------
   NAME       parse_simple
   FUNCTION   parse as simple 1:1 file
   INPUT      file .. name of the configuration file
              fp .... file pointer to the configuration file
              ptr ... pointer of current parsing
              cfg ... array of possible variables
              line .. pointer to current working line
   OUTPUT     error code (no error is CFG_NO_ERROR)
   -------------------------------------------------- */
static int parse_simple(const char *file, FILE *fp, char *ptr, cfgStruct cfg[], int *line)
{
	char *parameter;
	char *parameter_buf;
	int parameter_line;
	char *value;
	int error_code;

	parameter_buf = ptr;
	parameter_line = *line;

	if ((ptr = parse_word(ptr, &parameter, CFG_PARAMETER)) == NULL) {	/* malloc parameter */
		return (CFG_SYNTAX_ERROR);
	}
	if (*ptr == '{') {
		ptr = rm_first_spaces(ptr + 1);
		if (*ptr != '\0' && *ptr != '#') {
			free(parameter);
			return (CFG_SYNTAX_ERROR);
		}
		if (parse_values_between_braces(file, fp, parameter, cfg, line, CFG_SIMPLE, 0, parameter_buf, parameter_line) != CFG_NO_ERROR) {
			free(parameter);
			return (CFG_JUST_RETURN_WITHOUT_MSG);	/* error handling has already done */
		}
	} else {
		if ((ptr = parse_word(ptr, &value, CFG_VALUE)) == NULL) {	/* malloc value */
			free(parameter);
			return (CFG_SYNTAX_ERROR);
		}
		if ((error_code = store_value(cfg, parameter, value, CFG_SIMPLE, 0)) != CFG_NO_ERROR) {
			free(parameter);
			free(value);
			return (error_code);
		}
		free(value);
	}
	free(parameter);
	return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   NAME       parse_word
   FUNCTION   parse a word
   INPUT      ptr ... pointer of current parsing
              word .. pointer of pointer for parsed word
                      (dynamic allocating)
              word_type ... what word type parse as
                            + CFG_PARAMETER ... parameter
                            + CFG_VALUE ....... value 
                            + CFG_SECTION ..... section
   OUTPUT     new current pointer (on error, return NULL)
-------------------------------------------------- */
static char *parse_word(char *ptr, char **word, cfgKeywordValue word_type)
{
	int len = 0;
	cfgQuote quote_flag;

	switch (*ptr) {
	case '\"':
		quote_flag = CFG_DOUBLE_QUOTE;
		ptr++;
		break;
	case '\'':
		quote_flag = CFG_SINGLE_QUOTE;
		ptr++;
		break;
	default:
		quote_flag = CFG_NO_QUOTE;
	}

	for (;;) {
		if (quote_flag == CFG_NO_QUOTE) {
			if (*(ptr + len) == ' ' || *(ptr + len) == '\t' || *(ptr + len) == '\0' || *(ptr + len) == '#' || (*(ptr + len) == '=' && word_type == CFG_PARAMETER) || (*(ptr + len) == ']' && word_type == CFG_SECTION)) {
				break;
			}
		} else if (quote_flag == CFG_DOUBLE_QUOTE) {
			if (*(ptr + len) == '\"') {
				break;
			}
		} else if (quote_flag == CFG_SINGLE_QUOTE) {
			if (*(ptr + len) == '\'') {
				break;
			}
		}
		if (*(ptr + len) == '\0') {
			return (NULL);
		}
		len++;
	}
	if ((*word = malloc(len + 1)) == NULL) {
		cfgFatalFunc(CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
		return (NULL);
	}
	strncpy(*word, ptr, len);
	*(*word + len) = '\0';

	ptr += (len + (quote_flag == CFG_NO_QUOTE ? 0 : 1));

	ptr = rm_first_spaces(ptr);

	switch (word_type) {
	case CFG_PARAMETER:
		if (*ptr != '=') {
			free(*word);
			return (NULL);
		}
		ptr++;
		ptr = rm_first_spaces(ptr);
		break;
	case CFG_VALUE:
		if (*ptr != '\0' && *ptr != '#') {
			free(*word);
			return (NULL);
		}
		break;
	case CFG_SECTION:
		if (*ptr != ']') {
			free(*word);
			return (NULL);
		}
		break;
	default:
		free(*word);
		return (NULL);
	}
	return (ptr);
}


/* --------------------------------------------------
   NAME       store_value
   FUNCTION   store the value according to cfg
   INPUT      cfg ......... array of possible variables
              parameter ... parameter
	      value ....... value
	      type ........ type of the configuration file
	      section ..... section number
   OUTPUT     error code
   -------------------------------------------------- */
static int store_value(cfgStruct cfg[], const char *parameter, const char *value, cfgFileType type, int section)
{
	int num;
	long tmp;
	unsigned long utmp;
	float ftmp;
	double dtmp;
	char *endptr;
	char *strptr;
	cfgList *listptr;


	for (num = 0; cfg[num].type != CFG_END; num++) {
		if (strcasecmp(parameter, cfg[num].parameterName) == 0) {
			errno = 0;
			switch (cfg[num].type) {
			case CFG_BOOL:
				if (strcasecmp(value, "TRUE") == 0 || strcasecmp(value, "YES") == 0 || strcasecmp(value, "T") == 0 || strcasecmp(value, "Y") == 0 || strcasecmp(value, "1") == 0) {
					if (type == CFG_INI) {
						*(*(int **) (cfg[num].value) + section) = 1;
					} else {
						*(int *) (cfg[num].value) = 1;
					}
					return (CFG_NO_ERROR);
				} else if (strcasecmp(value, "FALSE") == 0 || strcasecmp(value, "NO") == 0 || strcasecmp(value, "F") == 0 || strcasecmp(value, "N") == 0 || strcasecmp(value, "0") == 0) {
					if (type == CFG_INI) {
						*(*(int **) (cfg[num].value) + section) = 0;
					} else {
						*(int *) (cfg[num].value) = 0;
					}
					return (CFG_NO_ERROR);
				}
				return (CFG_BOOL_ERROR);

			case CFG_STRING:
				if ((strptr = malloc(strlen(value) + 1)) == NULL) {
					return (CFG_MEM_ALLOC_FAIL);
				}
				strcpy(strptr, value);
				if (type == CFG_INI) {
					*(*(char ***) (cfg[num].value) + section) = strptr;
				} else {
					*(char **) (cfg[num].value) = strptr;
				}
				return (CFG_NO_ERROR);

			case CFG_INT:
				tmp = strtol(value, &endptr, 0);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE || tmp > INT_MAX || tmp < INT_MIN) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(int **) (cfg[num].value) + section) = tmp;
				} else {
					*(int *) (cfg[num].value) = tmp;
				}
				return (CFG_NO_ERROR);

			case CFG_UINT:
				utmp = strtoul(value, &endptr, 0);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE || utmp > UINT_MAX) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(unsigned int **) (cfg[num].value) + section) = utmp;
				} else {
					*(unsigned int *) (cfg[num].value) = utmp;
				}
				return (CFG_NO_ERROR);

			case CFG_LONG:
				tmp = strtol(value, &endptr, 0);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(long **) (cfg[num].value) + section) = tmp;
				} else {
					*(long *) (cfg[num].value) = tmp;
				}
				return (CFG_NO_ERROR);

			case CFG_ULONG:
				utmp = strtoul(value, &endptr, 0);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(unsigned long **) (cfg[num].value) + section) = utmp;
				} else {
					*(unsigned long *) (cfg[num].value) = utmp;
				}
				return (CFG_NO_ERROR);

			case CFG_STRING_LIST:
				if (type == CFG_INI) {
					listptr = *(*(cfgList ***) (cfg[num].value) + section);
				} else {
					listptr = *(cfgList **) (cfg[num].value);
				}
				if (listptr != NULL) {
					while (listptr->next != NULL) {
						listptr = listptr->next;
					}
					if ((listptr = listptr->next = malloc(sizeof(cfgList))) == NULL) {
						return (CFG_MEM_ALLOC_FAIL);
					}
				} else {
					if ((listptr = malloc(sizeof(cfgList))) == NULL) {
						return (CFG_MEM_ALLOC_FAIL);
					}
					if (type == CFG_INI) {
						*(*(cfgList ***) (cfg[num].value) + section) = listptr;
					} else {
						*(cfgList **) (cfg[num].value) = listptr;
					}
				}
				if ((strptr = malloc(strlen(value) + 1)) == NULL) {
					return (CFG_MEM_ALLOC_FAIL);
				}
				strcpy(strptr, value);
				listptr->str = strptr;
				listptr->next = NULL;
				return (CFG_NO_ERROR);

			case CFG_FLOAT:
				ftmp = (float)strtod(value, &endptr);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(float **) (cfg[num].value) + section) = ftmp;
				} else {
					*(float *) (cfg[num].value) = ftmp;
				}
				return (CFG_NO_ERROR);

			case CFG_DOUBLE:
				dtmp = strtod(value, &endptr);
				if (*endptr) {
					return (CFG_INVALID_NUMBER);
				}
				if (errno == ERANGE) {
					return (CFG_OUT_OF_RANGE);
				}
				if (type == CFG_INI) {
					*(*(double **) (cfg[num].value) + section) = dtmp;
				} else {
					*(double *) (cfg[num].value) = dtmp;
				}
				return (CFG_NO_ERROR);

			default:
				return (CFG_INTERNAL_ERROR);
			}
		}
	}
	return (CFG_WRONG_PARAMETER);
}


/* --------------------------------------------------
   NAME       parse_values_between_braces
   FUNCTION   parse values between braces
   INPUT      file ....... file name
              fp ......... file pointer to the configuration file
	      parameter .. parameter
	      cfg ........ array of possible variables
	      line ....... pointer to current working line
	      type ....... file type
	      section .... section number
	      parameter_buf ... for error handling 
	      parameter_line .. for error handling
   OUTPUT     CFG_NO_ERROR on success, CFG_JUST_RETURN_WITHOUT_MSG otherwise
   -------------------------------------------------- */
static int parse_values_between_braces(const char *file, FILE *fp, const char *parameter, cfgStruct cfg[], int *line, cfgFileType type, int section, const char *parameter_buf, int parameter_line)
{
	char *line_buf;
	char *value;
	char *ptr;
	int error_code;

	while ((ptr = get_single_line_without_first_spaces(fp, &line_buf, line)) != NULL) {
		if (*ptr == '}') {
			ptr = rm_first_spaces(ptr + 1);
			if (*ptr != '\0' && *ptr != '#') {
				cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
				return (CFG_JUST_RETURN_WITHOUT_MSG);
			}
			free(line_buf);
			return (CFG_NO_ERROR);
		}
		if (parse_word(ptr, &value, CFG_VALUE) == NULL) {
			cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
			return (CFG_JUST_RETURN_WITHOUT_MSG);
		}
		if ((error_code = store_value(cfg, parameter, value, type, section)) != CFG_NO_ERROR) {
			if (error_code == CFG_WRONG_PARAMETER) {
				cfgFatal(error_code, file, parameter_line, parameter_buf);
				return (CFG_JUST_RETURN_WITHOUT_MSG);
			}
			cfgFatal(error_code, file, *line, line_buf);
			return (CFG_JUST_RETURN_WITHOUT_MSG);
		}
		free(line_buf);
		free(value);
	}
	cfgFatal(CFG_NO_CLOSING_BRACE, file, *line, NULL);
	return (CFG_JUST_RETURN_WITHOUT_MSG);
}


/* --------------------------------------------------
   NAME       parse_ini
   FUNCTION   parse the configuration file as Windows INI-like file
   INPUT      file .. name of the configuration file
              fp .... file pointer to the configuration file
	      ptr ... pointer of current parsing strings
	      cfg ... array of possible variables
	      line .. pointer to current working line number
	      section ... pointer to current section number
   OUTPUT     error code (no error is CFG_NO_ERROR)
   -------------------------------------------------- */
static int parse_ini(const char *file, FILE *fp, char *ptr, cfgStruct cfg[], int *line, int *section)
{
	char *parameter;
	char *parameter_buf;
	int parameter_line;
	char *value;
	int error_code;
	int i;

	if (*ptr == '[') {
		if ((error_code = alloc_for_new_section(cfg, section)) != CFG_NO_ERROR) {
			return (error_code);
		}
		ptr = rm_first_spaces(ptr + 1);

		parsecfg_section_name = realloc(parsecfg_section_name, sizeof(char *) * (*section + 1));

		if ((ptr = parse_word(ptr, &parsecfg_section_name[*section], CFG_SECTION)) == NULL) {
			return (CFG_SYNTAX_ERROR);
		}
		for (i = 0; i < *section; i++) {
			if (strcasecmp(parsecfg_section_name[*section], parsecfg_section_name[i]) == 0) {
				return (CFG_USED_SECTION);
			}
		}
		ptr = rm_first_spaces(ptr + 1);
		if (*ptr != '\0' && *ptr != '#') {
			return (CFG_SYNTAX_ERROR);
		}
		return (CFG_NO_ERROR);
	} else if (*section == -1) {
		return (CFG_SYNTAX_ERROR);
	}

	parameter_buf = ptr;
	parameter_line = *line;

	if ((ptr = parse_word(ptr, &parameter, CFG_PARAMETER)) == NULL) {
		return (CFG_SYNTAX_ERROR);
	}
	if (*ptr == '{') {
		ptr = rm_first_spaces(ptr + 1);
		if (*ptr != '\0' && *ptr != '#') {
			return (CFG_SYNTAX_ERROR);
		}
		if (parse_values_between_braces(file, fp, parameter, cfg, line, CFG_INI, *section, parameter_buf, parameter_line) != CFG_NO_ERROR) {
			return (CFG_JUST_RETURN_WITHOUT_MSG);	/* error handling has already done */
		}
		free(parameter);
	} else {
		if ((ptr = parse_word(ptr, &value, CFG_VALUE)) == NULL) {
			return (CFG_SYNTAX_ERROR);
		}
		if ((error_code = store_value(cfg, parameter, value, CFG_INI, *section)) != CFG_NO_ERROR) {
			return (error_code);
		}
		free(parameter);
		free(value);
	}
	return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   NAME       alloc_for_new_section
   FUNCTION   allocate memory for new section
   INPUT      cfg ....... array of possible variables
              section ... pointer to current section number
   OUTPUT     error code
   -------------------------------------------------- */
static int alloc_for_new_section(cfgStruct cfg[], int *section)
{
	int num;
	void *ptr;

	(*section)++;
	for (num = 0; cfg[num].type != CFG_END; num++) {
		switch (cfg[num].type) {
		case CFG_BOOL:
		case CFG_INT:
		case CFG_UINT:
			if (*section == 0) {
				*(int **) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(int **) (cfg[num].value), sizeof(int) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(int **) (cfg[num].value) = ptr;
			if (cfg[num].type == CFG_BOOL) {
				*(*((int **) (cfg[num].value)) + *section) = -1;
			} else {
				*(*((int **) (cfg[num].value)) + *section) = 0;
			}
			break;

		case CFG_LONG:
		case CFG_ULONG:
			if (*section == 0) {
				*(long **) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(long **) (cfg[num].value), sizeof(long) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(long **) (cfg[num].value) = ptr;
			*(*((long **) (cfg[num].value)) + *section) = 0;
			break;

		case CFG_STRING:
			if (*section == 0) {
				*(char ***) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(char ***) (cfg[num].value), sizeof(char *) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(char ***) (cfg[num].value) = ptr;
			*(*(char ***) (cfg[num].value) + *section) = NULL;
			break;

		case CFG_STRING_LIST:
			if (*section == 0) {
				*(cfgList ***) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(cfgList ***) (cfg[num].value), sizeof(cfgList *) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(cfgList ***) (cfg[num].value) = ptr;
			*(*(cfgList ***) (cfg[num].value) + *section) = NULL;
			break;

		case CFG_FLOAT:
			if (*section == 0) {
				*(float **) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(float **) (cfg[num].value), sizeof(float) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(float **) (cfg[num].value) = ptr;
			*(*((float **) (cfg[num].value)) + *section) = 0;
			break;

		case CFG_DOUBLE:
			if (*section == 0) {
				*(double **) (cfg[num].value) = NULL;
			}
			if ((ptr = realloc(*(double **) (cfg[num].value), sizeof(double) * (*section + 1))) == NULL) {
				return (CFG_MEM_ALLOC_FAIL);
			}
			*(double **) (cfg[num].value) = ptr;
			*(*((double **) (cfg[num].value)) + *section) = 0;
			break;

		default:
			return (CFG_INTERNAL_ERROR);
		}
	}
	return (CFG_NO_ERROR);
}


/* --------------------------------------------------
   FUNCTION   Free all memory allocated for section names
              so that an application can parse an ini file
	      multiple times, or parse multiple ini files.
   INPUT      void
   OUTPUT     void
   -------------------------------------------------- */
void dealloc_for_sections(void)
{
	int section;

	if (parsecfg_section_name != NULL) {
		for (section = 0; section < parsecfg_maximum_section; section++) {
			free(parsecfg_section_name[section]);
		}
		free(parsecfg_section_name);
		parsecfg_section_name = NULL;
		parsecfg_maximum_section = 0;
	}
}


/* --------------------------------------------------
   NAME       rm_first_spaces
   FUNCTION   remove lead-off spaces and tabs in the string
   INPUT      ptr ... pointer to string
   OUTPUT     new poniter after removing
   -------------------------------------------------- */
static char *rm_first_spaces(char *ptr)
{
	while (*ptr == ' ' || *ptr == '\t') {
		ptr++;
	}
	return (ptr);
}


/* --------------------------------------------------
   NAME       get_single_line_without_first_spaces
   FUNCTION   get a single line without lead-off spaces
              and tabs from file
   INPUT      fp ....... file pointer
              gotptr ... pointer stored got string
	      line ..... pointer to line number
   OUTPUT     new pointer after removing
   -------------------------------------------------- */
static char *get_single_line_without_first_spaces(FILE *fp, char **gotstr, int *line)
{
	char *ptr;

	for (;;) {
		if ((*gotstr = dynamic_fgets(fp)) == NULL) {
			return (NULL);
		}
		(*line)++;
		ptr = rm_first_spaces(*gotstr);
		if (*ptr != '#' && *ptr != '\0') {
			return (ptr);
		}
		free(*gotstr);
	}
}


/* --------------------------------------------------
   NAME       dynamic_fgets
   FUNCTION   fgets with dynamic allocated memory
   INPUT      fp ... file pointer
   OUTPUT     pointer to got strings
              NULL on error
   -------------------------------------------------- */
static char *dynamic_fgets(FILE *fp)
{
	char *ptr;
	char temp[128];
	int i;

	ptr = malloc(1);
	if (ptr == NULL) {
		cfgFatalFunc(CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
		return (NULL);
	}
	*ptr = '\0';
	for (i = 0;; i++) {
		if (fgets(temp, 128, fp) == NULL) {
			if (ferror(fp) != 0 || i == 0) {
				free(ptr);
				return (NULL);
			}
			return (ptr);
		}
		ptr = realloc(ptr, 127 * (i + 1) + 1);
		if (ptr == NULL) {
			cfgFatalFunc(CFG_MEM_ALLOC_FAIL, "unknown", 0, "");
			return (NULL);
		}
		strcat(ptr, temp);
		if (strchr(temp, '\n') != NULL) {
		char* end = strchr(ptr, '\n');
        *end = '\0';
        if (end[-1] == '\r') end[-1] = '\0';
			return (ptr);
		}
	}
}


/* --------------------------------------------------
   NAME       dump_simple
   FUNCTION   
   INPUT      
   OUTPUT     0 on success, -1 on error
   -------------------------------------------------- */
static int dump_simple(FILE *fp, cfgStruct cfg[], cfgFileType type)
{
	int i;
	char c[2];
	cfgList *l;

	for (i = 0; cfg[i].type != CFG_END; i++) {
		switch (cfg[i].type) {
		case CFG_BOOL:
			fprintf(fp, "%s\t= %s\n", cfg[i].parameterName, (*(int *) (cfg[i].value)) ? "True" : "False");
			break;
		case CFG_INT:
			fprintf(fp, "%s\t= %d\n", cfg[i].parameterName, *(int *) (cfg[i].value));
			break;
		case CFG_UINT:
			fprintf(fp, "%s\t= %u\n", cfg[i].parameterName, *(unsigned int *) (cfg[i].value));
			break;
		case CFG_LONG:
			fprintf(fp, "%s\t= %ld\n", cfg[i].parameterName, *(long *) (cfg[i].value));
			break;
		case CFG_ULONG:
			fprintf(fp, "%s\t= %lu\n", cfg[i].parameterName, *(unsigned long *) (cfg[i].value));
			break;
		case CFG_STRING:
			if (*(char **) (cfg[i].value) == NULL) {
				break;
			}
			single_or_double_quote(*(char **) (cfg[i].value), c);
			fprintf(fp, "%s\t= %s%s%s\n", cfg[i].parameterName, c, *(char **) (cfg[i].value), c);
			break;
		case CFG_STRING_LIST:
			for (l = *(cfgList **) (cfg[i].value); l != NULL; l = l->next) {
				single_or_double_quote(l->str, c);
				fprintf(fp, "%s\t= %s%s%s\n", cfg[i].parameterName, c, l->str, c);
			}
			break;
		case CFG_FLOAT:
			fprintf(fp, "%s\t= %f\n", cfg[i].parameterName, *(float *) (cfg[i].value));
			break;
		case CFG_DOUBLE:
			fprintf(fp, "%s\t= %f\n", cfg[i].parameterName, *(double *) (cfg[i].value));
			break;
		default:
			cfgFatal(CFG_INTERNAL_ERROR, "?", 0, NULL);
			return (-1);
		}
	}
	return (0);
}


/* --------------------------------------------------
   NAME       dump_ini
   FUNCTION   
   INPUT      
   OUTPUT     0 on success, -1 on error
   -------------------------------------------------- */
static int dump_ini(FILE *fp, cfgStruct cfg[], cfgFileType type, int max)
{
	int i, j;
	char c[2];
	cfgList *l;

	for (j = 0; j < max; j++) {
		single_or_double_quote(cfgSectionNumberToName(j), c);
		fprintf(fp, "[%s%s%s]\n", c, cfgSectionNumberToName(j), c);

		for (i = 0; cfg[i].type != CFG_END; i++) {
			switch (cfg[i].type) {
			case CFG_BOOL:
				fprintf(fp, "%s\t= %s\n", cfg[i].parameterName, (*(int **) (cfg[i].value))[j] ? "True" : "False");
				break;
			case CFG_INT:
				fprintf(fp, "%s\t= %d\n", cfg[i].parameterName, (*(int **) (cfg[i].value))[j]);
				break;
			case CFG_UINT:
				fprintf(fp, "%s\t= %u\n", cfg[i].parameterName, (*(unsigned int **) (cfg[i].value))[j]);
				break;
			case CFG_LONG:
				fprintf(fp, "%s\t= %ld\n", cfg[i].parameterName, (*(long **) (cfg[i].value))[j]);
				break;
			case CFG_ULONG:
				fprintf(fp, "%s\t= %lu\n", cfg[i].parameterName, (*(unsigned long **) (cfg[i].value))[j]);
				break;
			case CFG_STRING:
				if ((*(char ***) (cfg[i].value))[j] == NULL) {
					break;
				}
				single_or_double_quote((*(char ***) (cfg[i].value))[j], c);
				fprintf(fp, "%s\t= %s%s%s\n", cfg[i].parameterName, c, (*(char ***) (cfg[i].value))[j], c);
				break;
			case CFG_STRING_LIST:
				for (l = (*(cfgList ***) (cfg[i].value))[j]; l != NULL; l = l->next) {
					single_or_double_quote(l->str, c);
					fprintf(fp, "%s\t= %s%s%s\n", cfg[i].parameterName, c, l->str, c);
				}
				break;
			case CFG_FLOAT:
				fprintf(fp, "%s\t= %f\n", cfg[i].parameterName, (*(float **) (cfg[i].value))[j]);
				break;
			case CFG_DOUBLE:
				fprintf(fp, "%s\t= %f\n", cfg[i].parameterName, (*(double **) (cfg[i].value))[j]);
				break;
			default:
				cfgFatal(CFG_INTERNAL_ERROR, "?", 0, NULL);
				return (-1);
			}
		}
		fprintf(fp, "\n");
	}
	return (0);
}


/* --------------------------------------------------
   NAME       single_or_double_quote
   FUNCTION   
   INPUT      
   OUTPUT     none
   -------------------------------------------------- */
static void single_or_double_quote(const char *str, char *ret)
{
	ret[1] = '\0';

	if (strchr(str, '\"') != NULL) {
		ret[0] = '\'';
	} else if (strchr(str, '\'') != NULL || strchr(str, '#') != NULL || strchr(str, '\t') != NULL || strchr(str, ' ') != NULL) {
		ret[0] = '\"';
	} else {
		ret[0] = '\0';
	}
}


/* --------------------------------------------------
   NAME       fetch_simple
   FUNCTION   
   INPUT      
   OUTPUT     0 on success, -1 on error
   -------------------------------------------------- */
static int fetch_simple(const char *file, FILE *fp, char *parameter_name, void *result_value, cfgValueType value_type)
{
	int store_flag = -1;
	int line;
	char *line_buf;
	char *ptr;

#if 0
	/* for GNU extensions */
	cfgStruct fetch_cfg[] = {
		{parameter_name, value_type, result_value},
		{NULL, CFG_END, NULL}
	};
#endif

	cfgStruct fetch_cfg[] = {
		{NULL, CFG_END, NULL},
		{NULL, CFG_END, NULL}
	};
	fetch_cfg[0].parameterName = parameter_name;
	fetch_cfg[0].type = value_type;
	fetch_cfg[0].value = result_value;

	while ((ptr = get_single_line_without_first_spaces(fp, &line_buf, &line)) != NULL) {	/* malloc line_buf */
		if (fetch_value(file, &line, line_buf, fp, &store_flag, ptr, fetch_cfg) == -1) {
			return (-1);
		}
	}
	return (store_flag);
}


/* --------------------------------------------------
   NAME       fetch_ini
   FUNCTION   
   INPUT      
   OUTPUT     0 on success, -1 on error
   -------------------------------------------------- */
static int fetch_ini(const char *file, FILE *fp, char *parameter_name, void *result_value, cfgValueType value_type, int section_num, const char *section_name)
{
	int store_flag = -1;
	int section_flag = -1;
	int line;
	char *line_buf;
	char *ptr;
	char *read_section_name;
	int current_section_number = 0;

#if 0
	/* for GNU extensions */
	cfgStruct fetch_cfg[] = {
		{parameter_name, value_type, result_value},
		{NULL, CFG_END, NULL}
	};
#endif

	cfgStruct fetch_cfg[] = {
		{NULL, CFG_END, NULL},
		{NULL, CFG_END, NULL}
	};
	fetch_cfg[0].parameterName = parameter_name;
	fetch_cfg[0].type = value_type;
	fetch_cfg[0].value = result_value;

	while ((ptr = get_single_line_without_first_spaces(fp, &line_buf, &line)) != NULL) {	/* malloc line_buf */
		if (*ptr == '[') {
			if (section_flag == 0) {
				free(line_buf);
				return (store_flag);
			}
			ptr = rm_first_spaces(ptr + 1);
			if ((ptr = parse_word(ptr, &read_section_name, CFG_SECTION)) == NULL) {	/* malloc read_section_name */
				free(line_buf);
				return (CFG_SYNTAX_ERROR);
			}
			ptr = rm_first_spaces(ptr + 1);
			if (*ptr != '\0' && *ptr != '#') {
				free(line_buf);
				free(read_section_name);
				return (CFG_SYNTAX_ERROR);
			}
			current_section_number++;
			if ((section_num > 0 && current_section_number == section_num) || (section_num <= 0 && strcasecmp(read_section_name, section_name) == 0)) {
				section_flag = 0;
				free(line_buf);
				free(read_section_name);
				continue;
			}
			free(read_section_name);
		}
		if (section_flag == -1) {
			free(line_buf);
			continue;
		}

		if (fetch_value(file, &line, line_buf, fp, &store_flag, ptr, fetch_cfg) == -1) {
			return (-1);
		}
	}
	return (store_flag);
}


/* --------------------------------------------------
   NAME       fetch_value
   FUNCTION   
   INPUT      
   OUTPUT     0 on success, -1 on error
   -------------------------------------------------- */
static int fetch_value(const char *file, int *line, char *line_buf, FILE *fp, int *store_flag, char *ptr, cfgStruct *fetch_cfg)
{
	int error_code;
	char *read_parameter;
	char *read_value;


	if ((ptr = parse_word(ptr, &read_parameter, CFG_PARAMETER)) == NULL) {	/* malloc read_parameter */
		cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
		free(line_buf);
		return (-1);
	}
	if (strcasecmp(read_parameter, fetch_cfg[0].parameterName) == 0) {
		if (*ptr == '{') {
			ptr = rm_first_spaces(ptr + 1);
			if (*ptr != '\0' && *ptr != '#') {
				cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
				free(line_buf);
				free(read_parameter);
				return (-1);
			}
			if (parse_values_between_braces(file, fp, fetch_cfg[0].parameterName, fetch_cfg, line, CFG_SIMPLE, 0, line_buf, *line) != CFG_NO_ERROR) {
				free(line_buf);
				free(read_parameter);
				return (-1);
			}
		} else {
			if ((ptr = parse_word(ptr, &read_value, CFG_VALUE)) == NULL) {	/* malloc read_value */
				cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
				free(line_buf);
				free(read_parameter);
				return (-1);
			}
			if ((error_code = store_value(fetch_cfg, fetch_cfg[0].parameterName, read_value, CFG_SIMPLE, 0)) != CFG_NO_ERROR) {
				cfgFatal(error_code, file, *line, line_buf);
				free(line_buf);
				free(read_parameter);
				free(read_value);
				return (-1);
			}
			free(read_value);
		}
		*store_flag = 0;
		free(line_buf);
	} else {
		if (*ptr == '{') {
			ptr = rm_first_spaces(ptr + 1);
			if (*ptr != '\0' && *ptr != '#') {
				cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
				free(line_buf);
				free(read_parameter);
				return (-1);
			}
			free(line_buf);
			while ((ptr = get_single_line_without_first_spaces(fp, &line_buf, line)) != NULL) { /* malloc line_buf */
				if (*ptr == '}') {
					ptr = rm_first_spaces(ptr + 1);
					if (*ptr != '\0' && *ptr != '#') {
						cfgFatal(CFG_SYNTAX_ERROR, file, *line, line_buf);
						free(line_buf);
						free(read_parameter);
						return (-1);
					}
					free(line_buf);
					break;
				}
				free(line_buf);
			}
		} else {
			free(line_buf);
		}
	}
	free(read_parameter);
	return (0);
}
