/* connect to an INDI server and show all desired device.property.element
 *   with possible wild card * in any category.
 * All types but BLOBs are handled from their defXXX messages. Receipt of a
 *   defBLOB sends enableBLOB then uses setBLOBVector for the value. BLOBs
 *   are stored in a file dev.nam.elem.format. only .z compression is handled.
 * exit status: 0 at least some found, 1 some not found, 2 real trouble.
 */

#include "base64.h"
#include "indiapi.h"
#include "lilxml.h"
#include "zlib.h"

#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

/* table of INDI definition elements, plus setBLOB.
 * we also look for set* if -m
 */
typedef struct
{
    char *vec; /* vector name */
    char *one; /* one element name */
} INDIDef;
static INDIDef defs[] = {
    { "defTextVector", "defText" },   { "defNumberVector", "defNumber" }, { "defSwitchVector", "defSwitch" },
    { "defLightVector", "defLight" }, { "defBLOBVector", "defBLOB" },     { "setBLOBVector", "oneBLOB" },
    { "setTextVector", "oneText" },   { "setNumberVector", "oneNumber" }, { "setSwitchVector", "oneSwitch" },
    { "setLightVector", "oneLight" },
};
static int ndefs = 6; /* or 10 if -m */

/* table of keyword to use in query vs name of INDI defXXX attribute */
typedef struct
{
    char *keyword;
    char *indiattr;
} INDIkwattr;
static INDIkwattr kwattr[] = {
    { "_LABEL", "label" }, { "_GROUP", "group" }, { "_STATE", "state" },
    { "_PERM", "perm" },   { "_TO", "timeout" },  { "_TS", "timestamp" },
};
#define NKWA (sizeof(kwattr) / sizeof(kwattr[0]))

typedef struct
{
    char *d;    /* device to seek */
    char *p;    /* property to seek */
    char *e;    /* element to seek */
    int wc : 1; /* whether pattern uses wild cards */
    int ok : 1; /* something matched this query */
} SearchDef;
static SearchDef *srchs; /* properties to look for */
static int nsrchs;

static void usage(void);
static void crackDPE(char *spec);
static void addSearchDef(char *dev, char *prop, char *ele);
static void openINDIServer(void);
static void getprops(void);
static void listenINDI(void);
static int finished(void);
static void onAlarm(int dummy);
static int readServerChar(void);
static void findDPE(XMLEle *root);
static void findEle(XMLEle *root, char *dev, char *nam, char *defone, SearchDef *sp);
static void enableBLOBs(char *dev, char *nam);
static void oneBLOB(XMLEle *root, char *dev, char *nam, char *enam, char *p, int plen);

static char *me;                      /* our name for usage() message */
static char host_def[] = "localhost"; /* default host name */
static char *host      = host_def;    /* working host name */
#define INDIPORT 7624                 /* default port */
static int port = INDIPORT;           /* working port number */
#define TIMEOUT 2                     /* default timeout, secs */
static int timeout = TIMEOUT;         /* working timeout, secs */
static int verbose;                   /* report extra info */
static LilXML *lillp;                 /* XML parser context */
#define WILDCARD '*'                  /* show all in this category */
static int onematch;                  /* only one possible match */
static int justvalue;                 /* if just one match show only value */
static int monitor;                   /* keep watching even after seen def */
static int directfd = -1;             /* direct filedes to server, if >= 0 */
static FILE *svrwfp;                  /* FILE * to talk to server */
static FILE *svrrfp;                  /* FILE * to read from server */
static int wflag;                     /* show wo properties too */

int main(int ac, char *av[])
{
    /* save our name */
    me = av[0];

    /* crack args */
    while (--ac && **++av == '-')
    {
        char *s = *av;
        while (*++s)
        {
            switch (*s)
            {
                case '1': /* just value */
                    justvalue++;
                    break;
                case 'd':
                    if (ac < 2)
                    {
                        fprintf(stderr, "-d requires open fileno\n");
                        usage();
                    }
                    directfd = atoi(*++av);
                    ac--;
                    break;
                case 'h':
                    if (directfd >= 0)
                    {
                        fprintf(stderr, "Can not combine -d and -h\n");
                        usage();
                    }
                    if (ac < 2)
                    {
                        fprintf(stderr, "-h requires host name\n");
                        usage();
                    }
                    host = *++av;
                    ac--;
                    break;
                case 'm':
                    monitor++;
                    ndefs = 10; /* include set*Vectors too */
                    break;
                case 'p':
                    if (directfd >= 0)
                    {
                        fprintf(stderr, "Can not combine -d and -p\n");
                        usage();
                    }
                    if (ac < 2)
                    {
                        fprintf(stderr, "-p requires tcp port number\n");
                        usage();
                    }
                    port = atoi(*++av);
                    ac--;
                    break;
                case 't':
                    if (ac < 2)
                    {
                        fprintf(stderr, "-t requires timeout\n");
                        usage();
                    }
                    timeout = atoi(*++av);
                    ac--;
                    break;
                case 'v': /* verbose */
                    verbose++;
                    break;
                case 'w':
                    wflag++;
                    break;
                default:
                    fprintf(stderr, "Unknown flag: %c\n", *s);
                    usage();
            }
        }
    }

    /* now ac args starting with av[0] */
    if (ac == 0)
        av[ac++] = "*.*.*"; /* default is get everything */

    /* crack each d.p.e */
    while (ac--)
        crackDPE(*av++);
    onematch = nsrchs == 1 && !srchs[0].wc;

    /* open connection */
    if (directfd >= 0)
    {
        svrwfp = fdopen(directfd, "w");
        svrrfp = fdopen(directfd, "r");
        if (!svrwfp || !svrrfp)
        {
            fprintf(stderr, "Direct fd %d: %s\n", directfd, strerror(errno));
            exit(1);
        }
        setbuf(svrrfp, NULL); /* don't absorb next guy's stuff */
        if (verbose)
            fprintf(stderr, "Using direct fd %d\n", directfd);
    }
    else
    {
        openINDIServer();
        if (verbose)
            fprintf(stderr, "Connected to %s on port %d\n", host, port);
    }

    /* build a parser context for cracking XML responses */
    lillp = newLilXML();

    /* issue getProperties */
    getprops();

    /* listen for responses, looking for d.p.e or timeout */
    listenINDI();

    return (0);
}

static void usage()
{
    fprintf(stderr, "Purpose: retrieve readable properties from an INDI server\n");
    fprintf(stderr, "%s\n", "$Revision: 1.11 $");
    fprintf(stderr, "Usage: %s [options] [device.property.element ...]\n", me);
    fprintf(stderr, "  Any component may be \"*\" to match all (beware shell metacharacters).\n");
    fprintf(stderr, "  Reports all properties if none specified.\n");
    fprintf(stderr, "  BLOBs are saved in file named device.property.element.format\n");
    fprintf(stderr, "  In perl try: %s\n", "%props = split (/[=\\n]/, `getINDI`);");
    fprintf(stderr, "  Set element to one of following to return property attribute:\n");
    for (int i = 0; i < (int)NKWA; i++)
        fprintf(stderr, "    %10s to report %s\n", kwattr[i].keyword, kwattr[i].indiattr);
    fprintf(stderr, "Output format: output is fully qualified name=value one per line\n");
    fprintf(stderr, "  or just value if -1 and exactly one query without wildcards.\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -1    : print just value if expecting exactly one response\n");
    fprintf(stderr, "  -d f  : use file descriptor f already open to server\n");
    fprintf(stderr, "  -h h  : alternate host, default is %s\n", host_def);
    fprintf(stderr, "  -m    : keep monitoring for more updates\n");
    fprintf(stderr, "  -p p  : alternate port, default is %d\n", INDIPORT);
    fprintf(stderr, "  -t t  : max time to wait, default is %d secs\n", TIMEOUT);
    fprintf(stderr, "  -v    : verbose (cumulative)\n");
    fprintf(stderr, "  -w    : show write-only properties too\n");
    fprintf(stderr, "Exit status:\n");
    fprintf(stderr, "  0: found at least one match for each query\n");
    fprintf(stderr, "  1: at least one query returned nothing\n");
    fprintf(stderr, "  2: real trouble, try repeating with -v\n");

    exit(2);
}

/* crack spec and add to srchs[], else exit */
static void crackDPE(char *spec)
{
    char d[1024], p[1024], e[1024];

    if (verbose)
        fprintf(stderr, "looking for %s\n", spec);
    if (sscanf(spec, "%[^.].%[^.].%[^.]", d, p, e) != 3)
    {
        fprintf(stderr, "Unknown format for property spec: %s\n", spec);
        usage();
    }

    addSearchDef(d, p, e);
}

/* grow srchs[] with the new search */
static void addSearchDef(char *dev, char *prop, char *ele)
{
    if (!srchs)
        srchs = (SearchDef *)malloc(1); /* realloc seed */
    srchs            = (SearchDef *)realloc(srchs, (nsrchs + 1) * sizeof(SearchDef));
    srchs[nsrchs].d  = strcpy(malloc(strlen(dev) + 1), dev);
    srchs[nsrchs].p  = strcpy(malloc(strlen(prop) + 1), prop);
    srchs[nsrchs].e  = strcpy(malloc(strlen(ele) + 1), ele);
    srchs[nsrchs].wc = *dev == WILDCARD || *prop == WILDCARD || *ele == WILDCARD;
    srchs[nsrchs].ok = 0;
    nsrchs++;
}

/* open a connection to the given host and port.
 * set svrwfp and svrrfp or die.
 */
static void openINDIServer(void)
{
    struct sockaddr_in serv_addr;
    struct hostent *hp;
    int sockfd;

    /* lookup host address */
    hp = gethostbyname(host);
    if (!hp)
    {
        herror("gethostbyname");
        exit(2);
    }

    /* create a socket to the INDI server */
    (void)memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
    serv_addr.sin_port        = htons(port);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(2);
    }

    /* connect */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    /* prepare for line-oriented i/o with client */
    svrwfp = fdopen(sockfd, "w");
    svrrfp = fdopen(sockfd, "r");
}

/* issue getProperties to svrwfp, possibly constrained to one device */
static void getprops()
{
    char *onedev = NULL;
    int i;

    /* find if only need one device */
    for (i = 0; i < nsrchs; i++)
    {
        char *d = srchs[i].d;
        if (*d == WILDCARD || (onedev && strcmp(d, onedev)))
        {
            onedev = NULL;
            break;
        }
        else
            onedev = d;
    }

    if (onedev)
        fprintf(svrwfp, "<getProperties version='%g' device='%s'/>\n", INDIV, onedev);
    else
        fprintf(svrwfp, "<getProperties version='%g'/>\n", INDIV);
    fflush(svrwfp);

    if (verbose)
        fprintf(stderr, "Queried properties from %s\n", onedev ? onedev : "*");
}

/* listen for INDI traffic on svrrfp.
 * print matching srchs[] and return when see all.
 * timeout and exit if any trouble.
 */
static void listenINDI()
{
    char msg[1024];

    /* arrange to call onAlarm() if not seeing any more defXXX */
    signal(SIGALRM, onAlarm);
    alarm(timeout);

    /* read from server, exit if find all requested properties */
    while (1)
    {
        XMLEle *root = readXMLEle(lillp, readServerChar(), msg);
        if (root)
        {
            /* found a complete XML element */
            if (verbose > 1)
                prXMLEle(stderr, root, 0);
            findDPE(root);
            if (finished() == 0)
                exit(0);     /* found all we want */
            delXMLEle(root); /* not yet, delete and continue */
        }
        else if (msg[0])
        {
            fprintf(stderr, "Bad XML from %s/%d: %s\n", host, port, msg);
            exit(2);
        }
    }
}

/* return 0 if we are sure we have everything we are looking for, else -1 */
static int finished()
{
    int i;

    if (monitor)
        return (-1);

    for (i = 0; i < nsrchs; i++)
        if (srchs[i].wc || !srchs[i].ok)
            return (-1);
    return (0);
}

/* called after timeout seconds either because we are matching wild cards or
 * there is something still not found
 */
static void onAlarm(int dummy)
{
    (void)dummy;
    int trouble = 0;

    for (int i = 0; i < nsrchs; i++)
    {
        if (!srchs[i].ok)
        {
            trouble = 1;
            fprintf(stderr, "No %s.%s.%s from %s:%d\n", srchs[i].d, srchs[i].p, srchs[i].e, host, port);
        }
    }

    exit(trouble ? 1 : 0);
}

/* read one char from svrrfp */
static int readServerChar()
{
    int c = fgetc(svrrfp);

    if (c == EOF)
    {
        if (ferror(svrrfp))
            perror("read");
        else
            fprintf(stderr, "INDI server %s/%d disconnected\n", host, port);
        exit(2);
    }

    if (verbose > 2)
        fprintf(stderr, "Read %c\n", c);

    return (c);
}

/* print value if root is any srchs[] we are looking for*/
static void findDPE(XMLEle *root)
{
    int i, j;

    for (i = 0; i < nsrchs; i++)
    {
        /* for each property we are looking for */
        for (j = 0; j < ndefs; j++)
        {
            /* for each possible type */
            if (strcmp(tagXMLEle(root), defs[j].vec) == 0)
            {
                /* legal defXXXVector, check device */
                char *dev  = (char *)findXMLAttValu(root, "device");
                char *idev = srchs[i].d;
                if (idev[0] == WILDCARD || !strcmp(dev, idev))
                {
                    /* found device, check name */
                    char *nam   = (char *)findXMLAttValu(root, "name");
                    char *iprop = srchs[i].p;
                    if (iprop[0] == WILDCARD || !strcmp(nam, iprop))
                    {
                        /* found device and name, check perm */
                        char *perm = (char *)findXMLAttValu(root, "perm");
                        if (!wflag && perm[0] && !strchr(perm, 'r'))
                        {
                            if (verbose)
                                fprintf(stderr, "%s.%s is write-only\n", dev, nam);
                        }
                        else
                        {
                            /* check elements or attr keywords */
                            if (!strcmp(defs[j].vec, "defBLOBVector"))
                                enableBLOBs(dev, nam);
                            else
                                findEle(root, dev, nam, defs[j].one, &srchs[i]);
                            if (onematch)
                                return; /* only one can match */
                            if (!strncmp(defs[j].vec, "def", 3))
                                alarm(timeout); /* reset timer if def */
                        }
                    }
                }
            }
        }
    }
}

/* print elements in root speced in sp (known to be of type defone).
 * print just value if onematch and justvalue else fully qualified name.
 */
static void findEle(XMLEle *root, char *dev, char *nam, char *defone, SearchDef *sp)
{
    char *iele = sp->e;
    XMLEle *ep = NULL;

    /* check for attr keyword */
    for (int i = 0; i < (int)NKWA; i++)
    {
        if (strcmp(iele, kwattr[i].keyword) == 0)
        {
            /* just print the property state, not the element values */
            char *s = (char *)findXMLAttValu(root, kwattr[i].indiattr);
            sp->ok  = 1; /* progress */
            if (onematch && justvalue)
                printf("%s\n", s);
            else
                printf("%s.%s.%s=%s\n", dev, nam, kwattr[i].keyword, s);
            return;
        }
    }

    /* no special attr, look for specific element name */
    for (ep = nextXMLEle(root, 1); ep; ep = nextXMLEle(root, 0))
    {
        if (!strcmp(tagXMLEle(ep), defone))
        {
            /* legal defXXX, check deeper */
            char *enam = (char *)findXMLAttValu(ep, "name");
            if (iele[0] == WILDCARD || !strcmp(enam, iele))
            {
                /* found it! */
                char *p = pcdataXMLEle(ep);
                sp->ok  = 1; /* progress */
                if (!strcmp(defone, "oneBLOB"))
                    oneBLOB(ep, dev, nam, enam, p, pcdatalenXMLEle(ep));
                else if (onematch && justvalue)
                    printf("%s\n", p);
                else
                    printf("%s.%s.%s=%s\n", dev, nam, enam, p);
                if (onematch)
                    return; /* only one can match*/
            }
        }
    }
}

/* send server command to svrwfp that enables blobs for the given dev nam
 */
static void enableBLOBs(char *dev, char *nam)
{
    if (verbose)
        fprintf(stderr, "sending enableBLOB %s.%s\n", dev, nam);
    fprintf(svrwfp, "<enableBLOB device='%s' name='%s'>Also</enableBLOB>\n", dev, nam);
    fflush(svrwfp);
}

/* given a oneBLOB, save
 */
static void oneBLOB(XMLEle *root, char *dev, char *nam, char *enam, char *p, int plen)
{
    char *format;
    FILE *fp;
    int bloblen;
    unsigned char *blob;
    int ucs;
    int isz;
    char fn[128];
    int i;

    /* get uncompressed size */
    ucs = atoi(findXMLAttValu(root, "size"));
    if (verbose)
        fprintf(stderr, "%s.%s.%s reports uncompressed size as %d\n", dev, nam, enam, ucs);

    /* get format and length */
    format = (char *)findXMLAttValu(root, "format");
    isz    = !strncmp(&format[strlen(format) - 2], ".z", 2);

    /* decode blob from base64 in p */
    blob    = malloc(3 * plen / 4);
    bloblen = from64tobits_fast((char *)blob, p, plen);
    if (bloblen < 0)
    {
        fprintf(stderr, "%s.%s.%s bad base64\n", dev, nam, enam);
        exit(2);
    }

    /* uncompress effectively in place if z */
    if (isz)
    {
        uLong nuncomp         = ucs;
        unsigned char *uncomp = malloc(ucs);
        int ok                = uncompress(uncomp, &nuncomp, blob, bloblen);
        if (ok != Z_OK)
        {
            fprintf(stderr, "%s.%s.%s uncompress error %d\n", dev, nam, enam, ok);
            exit(2);
        }
        free(blob);
        blob    = uncomp;
        bloblen = nuncomp;
    }

    /* rig up a file name from property name */
    i = sprintf(fn, "%s.%s.%s%s", dev, nam, enam, format);
    if (isz)
        fn[i - 2] = '\0'; /* chop off .z */

    /* save */
    fp = fopen(fn, "w");
    if (fp)
    {
        if (verbose)
            fprintf(stderr, "Wrote %s\n", fn);
        fwrite(blob, bloblen, 1, fp);
        fclose(fp);
    }
    else
    {
        fprintf(stderr, "%s: %s\n", fn, strerror(errno));
    }

    /* clean up */
    free(blob);
}

/* For RCS Only -- Do Not Edit */
static char *rcsid[2] = { (char *)rcsid,
                          "@(#) $RCSfile: getINDI.c,v $ $Date: 2007/10/11 20:11:23 $ $Revision: 1.11 $ $Name:  $" };
