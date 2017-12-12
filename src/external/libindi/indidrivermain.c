#if 0
    INDI
    Copyright (C) 2003-2006 Elwood C. Downey

                        Updated by Jasem Mutlaq (2003-2010)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#endif

/* main() for one INDI driver process.
 * Drivers define IS*() functions we call to deliver INDI XML arriving on stdin.
 * Drivers call ID*() functions to send INDI XML commands to stdout.
 * Drivers call IE*() functions to build an event-driver program.
 * Drivers call IU*() functions to perform various common utility tasks.
 * Troubles are reported on stderr then we exit.
 *
 * This requires liblilxml.
 */

#include "base64.h"
#include "eventloop.h"
#include "indicom.h"
#include "indidevapi.h"
#include "indidriver.h"
#include "lilxml.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXRBUF 2048

ROSC *propCache;
int nPropCache; /* # of elements in roCheck */

int verbose;    /* chatty */
char *me;       /* a.out name */
LilXML *clixml; /* XML parser context */

static void usage(void);

int main(int ac, char *av[])
{
#ifndef _WIN32
    int ret = setgid(getgid());

    ret = setuid(getuid());

    if (geteuid() != getuid())
        exit(255);
#endif

    /* save handy pointer to our base name */
    for (me = av[0]; av[0][0]; av[0]++)
        if (av[0][0] == '/')
            me = &av[0][1];

    /* crack args */
    while (--ac && (*++av)[0] == '-')
        while (*++(*av))
            switch (*(*av))
            {
                case 'v': /* verbose */
                    verbose++;
                    break;
                default:
                    usage();
            }

    /* ac remaining args starting at av[0] */
    if (ac > 0)
        usage();

    /* init */
    clixml = newLilXML();
    addCallback(0, clientMsgCB, NULL);

    /* service client */
    eventLoop();

    /* eh?? */
    fprintf(stderr, "%s: inf loop ended\n", me);
    return (1);
}

/* print usage message and exit (1) */
static void usage(void)
{
    fprintf(stderr, "Usage: %s [options]\n", me);
    fprintf(stderr, "Purpose: INDI Device driver framework.\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " -v    : more verbose to stderr\n");

    exit(1);
}
