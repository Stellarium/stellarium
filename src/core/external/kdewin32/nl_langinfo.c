#include "kdewin32/langinfo.h"
#include "kdewin32/stdlib.h"

static __inline char* get_codeset(void) {
  /* this is normally only used to look for "UTF-8" */
  char* s=getenv("LC_CTYPE");
  if (!s) s=getenv("LC_ALL");
  if (!s) s="ANSI_X3.4-1968";	/* it's what glibc does */
  return s;
}

static const char   sweekdays [7] [4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char   weekdays [7] [10] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

static const char   smonths [12] [4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char*  months [12] = { 
    "January", "February", "March", "April", smonths[5-1], "June",
    "July", "August", "September", "October", "November", "December"
};

char* nl_langinfo(nl_item x) {
  if (x>=DAY_1 && x<=DAY_7) return (char*)weekdays[x-DAY_1];
  if (x>=ABDAY_1 && x<=ABDAY_7) return (char*)sweekdays[x-ABDAY_1];
  if (x>=MON_1 && x<=MON_12) return (char*)months[x-MON_1];
  if (x>=ABMON_1 && x<=ABMON_12) return (char*)smonths[x-ABMON_1];
  switch (x) {
  case CODESET: return get_codeset();
  case D_T_FMT: return "%b %a %d %k:%M:%S %Z %Y";
  case D_FMT: return "%b %a %d";
  case T_FMT: return "%H:%M";
  case T_FMT_AMPM: return "%I:%M:%S %p";
  case AM_STR: return "am";
  case PM_STR: return "pm";
  case ERA: return 0;
  case ERA_D_FMT: case ERA_D_T_FMT: case ERA_T_FMT: case ALT_DIGITS: return "";
  case RADIXCHAR: return ".";
  case THOUSEP: return "";
  case YESEXPR: return "^[yY]";
  case NOEXPR: return "^[nN]";
  case CRNCYSTR: return "$";
  default: return 0;
  }
}
