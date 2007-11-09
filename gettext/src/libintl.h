
#ifndef _LIBINTL_H
#define _LIBINTL_H	1

#include "gettext.h"

#if !defined LC_MESSAGES && !(defined __LOCALE_H || (defined _LOCALE_H && defined __sun))
# define LC_MESSAGES 1729
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline char *gettext (const char *__msgid)
{
  return DCIGETTEXT(0, __msgid, 0, 0, 0, LC_MESSAGES) ;
}

static inline char *dgettext (const char *__domainname, const char *__msgid)
{
  //return libintl_dgettext (__domainname, __msgid);
  return NULL;
}

static inline char *dcgettext (const char *__domainname, const char *__msgid, int __category)
{
  //return libintl_dcgettext (__domainname, __msgid, __category);
  return NULL;
}

static inline char *ngettext (const char *__msgid1, const char *__msgid2, unsigned long int __n)
{
  //return libintl_ngettext (__msgid1, __msgid2, __n);
  return NULL;
}

static inline char *dngettext (const char *__domainname, const char *__msgid1, const char *__msgid2, unsigned long int __n)
{
  //return libintl_dngettext (__domainname, __msgid1, __msgid2, __n);
  return NULL;
}

static inline char *dcngettext (const char *__domainname,
				const char *__msgid1, const char *__msgid2,
				unsigned long int __n, int __category)
{
  //return libintl_dcngettext (__domainname, __msgid1, __msgid2, __n, __category);
  return NULL;
}

static inline char *textdomain (const char *__domainname)
{
  return TEXTDOMAIN(__domainname);
}

static inline char *bindtextdomain (const char *__domainname, const char *__dirname)
{
  return BINDTEXTDOMAIN(__domainname, __dirname);
}

static inline char *bind_textdomain_codeset (const char *__domainname, const char *__codeset)
{
  return BIND_TEXTDOMAIN_CODESET(__domainname, __codeset);
}

#ifdef __cplusplus
}
#endif

#endif