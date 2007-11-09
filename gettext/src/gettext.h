
/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
//# define DCIGETTEXT libintl_dcigettext


#ifdef __cplusplus
extern "C" {
#endif

extern char* DCIGETTEXT(const char *domainname, const char *msgid1, const char *msgid2, int plural, unsigned long int n, int category);
extern char* BINDTEXTDOMAIN(const char *domainname, const char *dirname);
extern char* BIND_TEXTDOMAIN_CODESET(const char *domainname, const char *codeset);
extern char* TEXTDOMAIN(const char *domainname);

#ifdef __cplusplus
}
#endif