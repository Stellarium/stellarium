/* This file is part of the KDE project
   Copyright (C) 2003-2004 Werner Almesberger <werner@almesberger.net>

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

#include <windows.h>

#include "kdewin32/errno.h"
#include <limits.h>
#include "kdewin32/stdlib.h"
#include "kdewin32/string.h"
#include "kdewin32/sys/stat.h"
#include "kdewin32/unistd.h"
#include <direct.h>

/**
 * @internal Canonical name: never ends with a slash
 */
static int resolve_path(char *path,char *result,char *pos)
{
	if (*path == '/') {
		*result = '/';
		pos = result+1;
		path++;
	}
	*pos = 0;
	if (!*path)
		return 0;
	while (1) {
		char *slash;
		struct stat st;

		slash = *path ? strchr(path,'/') : NULL;
		if (slash)
			*slash = 0;
		if (!path[0] || (path[0] == '.' &&
		  (!path[1] || (path[1] == '.' && !path[2])))) {
			pos--;
			if (pos != result && path[0] && path[1])
			while (*--pos != '/');
		}
		else {
			strcpy(pos,path);
			if (lstat(result,&st) < 0)
				return -1;
			if (S_ISLNK(st.st_mode)) {
				char buf[PATH_MAX];

				if (readlink(result,buf,sizeof(buf)) < 0)
					return -1;
				*pos = 0;
				if (slash) {
					*slash = '/';
					strcat(buf,slash);
				}
				strcpy(path,buf);
				if (*path == '/')
					result[1] = 0;
				pos = strchr(result,0);
				continue;
			}
			pos = strchr(result,0);
		}
		if (slash) {
			*pos++ = '/';
			path = slash+1;
		}
		*pos = 0;
		if (!slash)
			break;
	}
	return 0;
}

/** @return the canonicalized absolute pathname */
char *realpath(const char *path,char *resolved_path)
{
	char cwd[PATH_MAX];
	char *path_copy;
	int res;

	if (!*path) {
		errno = ENOENT;
		return NULL;
	}
	if (!getcwd(cwd,sizeof(cwd)))
		return NULL;
	strcpy(resolved_path,"/");
	if (resolve_path(cwd,resolved_path,resolved_path))
		return NULL;
	strcat(resolved_path,"/");
	path_copy = strdup(path);
	if (!path_copy)
		return NULL;
	res = resolve_path(path_copy,resolved_path,strchr(resolved_path,0));
	free(path_copy);
	if (res)
		return NULL;
	return resolved_path;
}

