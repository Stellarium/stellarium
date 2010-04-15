/*
 * stat.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Symbolic constants for opening and creating files, also stat, fstat and
 * chmod functions.
 *
 */
#ifndef KDEWIN_SYS_STAT_H
#define KDEWIN_SYS_STAT_H

#include "kdewin32/sys/types.h"
#include <../include/sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _S_IFLNK  0xF000    /* Pretend */
#define S_IFLNK   _S_IFLNK  /* kio/kio/kzip.cpp */ 
#define _S_IFSOCK 0140000   /* socket */

#define S_ISLNK(m)  (((m) & _S_IFMT) == _S_IFLNK) /* Should always be zero.*/
#define S_ISSOCK(m) (((m)&_S_IFMT) == _S_IFSOCK)

#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRGRP 0000040  /* read permission, group */
#define S_IWGRP 0000020  /* write permission, grougroup */
#define S_IXGRP 0000010  /* execute/search permission, group */
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#define S_IROTH 0000004  /* read permission, other */
#define S_IWOTH 0000002  /* write permission, other */
#define S_IXOTH 0000001  /* execute/search permission, other */

#define S_ISUID 0004000  /* set user id on execution */
#define S_ISGID 0002000  /* set group id on execution */
#define S_ISVTX 0001000  /* save swapped text even after use */

#define S_IFSOCK _S_IFSOCK

int lstat( const char *__path, struct stat *__buf);
int fchmod(int __fd, mode_t __mode);

#ifdef __cplusplus
}
#endif

#endif  // KDEWIN_SYS_STAT_H
