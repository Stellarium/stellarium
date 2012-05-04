/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KDEWIN_UNISTD_H
#define _KDEWIN_UNISTD_H

#include "kdewin32/sys/types.h"
#include <../include/unistd.h>

#include <winsock2.h>

#define environ _environ

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define	F_OK	0
#define	R_OK	4
#define	W_OK	2
#define	X_OK	1 

/* + from <sys/stat.h>: */
#define	_IFMT		0170000	/* type of file */
#define		_IFDIR	0040000	/* directory */
#define		_IFCHR	0020000	/* character special */
#define		_IFBLK	0060000	/* block special */
#define		_IFREG	0100000	/* regular */
#define		_IFLNK	0120000	/* symbolic link */
#define		_IFSOCK	0140000	/* socket */
#define		_IFIFO	0010000	/* fifo */

#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)
#endif 

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

int chown(const char *__path, uid_t __owner, gid_t __group);

int fchown(int __fd, uid_t __owner, gid_t __group );

/* Get the real user ID of the calling process.  */
uid_t getuid (void);

/* Get the effective user ID of the calling process.  */
uid_t geteuid (void);

/* Get the real group ID of the calling process.  */
gid_t getgid (void);

/* Get the effective group ID of the calling process.  */
gid_t getegid (void);

int getgroups(int size, gid_t _list[]);

/* On win32 we do not have fs-links, so simply 0 (success) is returned
   when __path is accessible. It is then just copied to __buf.
*/
int readlink(const char *__path, char *__buf, int __buflen);

/* just copies __name1 to __name2 */
int symlink(const char *__name1, const char *__name2);

/* just copies __name1 to __name2 */
int link(const char *__name1, const char *__name2);

int pipe(int *fd);

#ifndef __MINGW64__
pid_t fork(void);
#endif

pid_t setsid(void);

// using winsock gethostname() requires WSAStartup called before :-( 
// which will not be done in every case, so uses this one 
//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/gethostname_2.asp
#undef gethostname
#define gethostname kde_gethostname
int kde_gethostname(char *__name, size_t __len);

unsigned alarm(unsigned __secs ); 

char* getlogin();

int fsync (int fd);

/*
//BM: Both of these functions cause a previous declaration error
//when building on Windows/MinGW.
void usleep(unsigned int usec);

void sleep(unsigned int sec);
*/

int setreuid(uid_t ruid, uid_t euid);

int mkstemps(char* _template, int suffix_len);

int initgroups(const char *name, int basegid);

// from kdecore/fakes.c

int seteuid(uid_t euid);

int mkstemp (char* _template);

char* mkdtemp (char* _template);

int revoke(const char *tty);

long getpagesize (void);

#ifdef __cplusplus
}
#endif

#endif /* _KDEWIN_UNISTD_H */
