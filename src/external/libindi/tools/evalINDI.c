/* evaluate an expression of INDI operands
 */

/* Overall design:
 * compile expression, building operand table, if trouble exit 2
 * open INDI connection, if trouble exit 2
 * send getProperties as required to get operands flowing
 * watch for messages until get initial values of each operand
 * evaluate expression, repeat if -w each time an op arrives until true
 * exit val==0
 */

#include "indiapi.h"
#include "indidevapi.h"
#include "lilxml.h"

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

extern int compileExpr(char *expr, char *errmsg);
extern int evalExpr(double *vp, char *errmsg);
extern int allOperandsSet();
extern int getAllOperands(char ***ops);
extern int getSetOperands(char ***ops);
extern int getUnsetOperands(char ***ops);
extern int setOperand(char *name, double valu);

static void usage();
static void compileINDI(char *expr);
static FILE *openINDIServer();
static void getProps(FILE *fp);
static void initProps(FILE *fp);
static int pstatestr(char *state);
static time_t timestampINDI(char *ts);
static int devcmp(char *op1, char *op2);
static int runEval(FILE *fp);
static int setOp(XMLEle *root);
static XMLEle *nxtEle(FILE *fp);
static int readServerChar(FILE *fp);
static void onAlarm(int dummy);

static char *me;
static char host_def[] = "localhost"; /* default host name */
static char *host      = host_def;    /* working host name */
#define INDIPORT 7624                 /* default port */
static int port = INDIPORT;           /* working port number */
#define TIMEOUT 2                     /* default timeout, secs */
static int timeout = TIMEOUT;         /* working timeout, secs */
static LilXML *lillp;                 /* XML parser context */
static int directfd = -1;             /* direct filedes to server, if >= 0 */
static int verbose;                   /* more tracing */
static int eflag;                     /* print each updated expression value*/
static int fflag;                     /* print final expression value */
static int iflag;                     /* read expresion from stdin */
static int oflag;                     /* print operands as they change */
static int wflag;                     /* wait for expression to be true */
static int bflag;                     /* beep when true */

int main(int ac, char *av[])
{
    FILE *fp;

    /* save our name for usage() */
    me = av[0];

    /* crack args */
    while (--ac && **++av == '-')
    {
        char *s = *av;
        while (*++s)
        {
            switch (*s)
            {
                case 'b': /* beep when true */
                    bflag++;
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
                case 'e': /* print each updated expression value */
                    eflag++;
                    break;
                case 'f': /* print final expression value */
                    fflag++;
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
                case 'i': /* read expression from stdin */
                    iflag++;
                    break;
                case 'o': /* print operands as they change */
                    oflag++;
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
                case 'w': /* wait for expression to be true */
                    wflag++;
                    break;
                default:
                    fprintf(stderr, "Unknown flag: %c\n", *s);
                    usage();
            }
        }
    }

    /* now there are ac args starting with av[0] */

    /* compile expression from av[0] or stdin */
    if (ac == 0)
        compileINDI(NULL);
    else if (ac == 1)
        compileINDI(av[0]);
    else
        usage();

    /* open connection */
    if (directfd >= 0)
    {
        fp = fdopen(directfd, "r+");
        setbuf(fp, NULL); /* don't absorb next guy's stuff */
        if (!fp)
        {
            fprintf(stderr, "Direct fd %d: %s\n", directfd, strerror(errno));
            exit(1);
        }
        if (verbose)
            fprintf(stderr, "Using direct fd %d\n", directfd);
    }
    else
    {
        fp = openINDIServer();
        if (verbose)
            fprintf(stderr, "Connected to %s on port %d\n", host, port);
    }

    /* build a parser context for cracking XML responses */
    lillp = newLilXML();

    /* set up to catch an io timeout function */
    signal(SIGALRM, onAlarm);

    /* send getProperties */
    getProps(fp);

    /* initialize all properties */
    initProps(fp);

    /* evaluate expression, return depending on flags */
    return (runEval(fp));
}

static void usage()
{
    fprintf(stderr, "Usage: %s [options] [exp]\n", me);
    fprintf(stderr, "Purpose: evaluate an expression of INDI operands\n");
    fprintf(stderr, "Version: $Revision: 1.5 $\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "   -b   : beep when expression evaluates as true\n");
    fprintf(stderr, "   -d f : use file descriptor f already open to server\n");

    fprintf(stderr, "   -e   : print each updated expression value\n");
    fprintf(stderr, "   -f   : print final expression value\n");
    fprintf(stderr, "   -h h : alternate host, default is %s\n", host_def);
    fprintf(stderr, "   -i   : read expression from stdin\n");
    fprintf(stderr, "   -o   : print operands as they change\n");
    fprintf(stderr, "   -p p : alternate port, default is %d\n", INDIPORT);
    fprintf(stderr, "   -t t : max secs to wait, 0 is forever, default is %d\n", TIMEOUT);
    fprintf(stderr, "   -v   : verbose (cummulative)\n");
    fprintf(stderr, "   -w   : wait for expression to evaluate as true\n");
    fprintf(stderr, "[exp] is an arith expression built from the following operators and functons:\n");
    fprintf(stderr, "     ! + - * / && || > >= == != < <=\n");
    fprintf(stderr, "     pi sin(rad) cos(rad) tan(rad) asin(x) acos(x) atan(x) atan2(y,x) abs(x)\n");
    fprintf(stderr, "     degrad(deg) raddeg(rad) floor(x) log(x) log10(x) exp(x) sqrt(x) pow(x,exp)\n");
    fprintf(stderr, "   operands are of the form \"device.name.element\" (including quotes), where\n");
    fprintf(stderr, "   element may be:\n");
    fprintf(stderr, "     _STATE evaluated to 0,1,2,3 from Idle,Ok,Busy,Alert.\n");
    fprintf(stderr, "     _TS evaluated to UNIX seconds from epoch.\n");
    fprintf(stderr, "   Switch vectors are evaluated to 0,1 from Off,On.\n");
    fprintf(stderr, "   Light vectors are evaluated to 0-3 as per _STATE.\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "   To print 0/1 whether Security.Doors.Front or .Rear are in Alert:\n");
    fprintf(stderr, "     evalINDI -f '\"Security.Doors.Front\"==3 || \"Security.Doors.Rear\"==3'\n");
    fprintf(stderr, "   To exit 0 if the Security property as a whole is in a state of Ok:\n");
    fprintf(stderr, "     evalINDI '\"Security.Security._STATE\"==1'\n");
    fprintf(stderr, "   To wait for RA and Dec to be near zero and watch their values as they change:\n");
    fprintf(stderr, "     evalINDI -t 0 -wo 'abs(\"Mount.EqJ2K.RA\")<.01 && abs(\"Mount.EqJ2K.Dec\")<.01'\n");
    fprintf(stderr, "Exit 0 if expression evaluates to non-0, 1 if 0, else 2\n");

    exit(1);
}

/* compile the given expression else read from stdin.
 * exit(2) if trouble.
 */
static void compileINDI(char *expr)
{
    char errmsg[1024];
    char *exp = expr;

    if (!exp)
    {
        /* read expression from stdin */
        int nr, nexp = 0;
        exp = malloc(1024);
        while ((nr = fread(exp + nexp, 1, 1024, stdin)) > 0)
            exp = realloc(exp, (nexp += nr) + 1024);
        exp[nexp] = '\0';
    }

    if (verbose)
        fprintf(stderr, "Compiling: %s\n", exp);
    if (compileExpr(exp, errmsg) < 0)
    {
        fprintf(stderr, "Compile err: %s\n", errmsg);
        exit(2);
    }

    if (exp != expr)
        free(exp);
}

/* open a connection to the given host and port or die.
 * return FILE pointer to socket.
 */
static FILE *openINDIServer()
{
    struct sockaddr_in serv_addr;
    struct hostent *hp;
    int sockfd;

    /* lookup host address */
    hp = gethostbyname(host);
    if (!hp)
    {
        perror("gethostbyname");
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
    return (fdopen(sockfd, "r+"));
}

/* invite each device referenced in the expression to report its properties.
 */
static void getProps(FILE *fp)
{
    char **ops;
    int nops;
    int i, j;

    /* get each operand used in the expression */
    nops = getAllOperands(&ops);

    /* send getProperties for each unique device referenced */
    for (i = 0; i < nops; i++)
    {
        for (j = 0; j < i; j++)
            if (devcmp(ops[i], ops[j]) == 0)
                break;
        if (j < i)
            continue;
        if (verbose)
            fprintf(stderr, "sending getProperties for %.*s\n", (int)(strchr(ops[i], '.') - ops[i]), ops[i]);
        fprintf(fp, "<getProperties version='%g' device='%.*s'/>\n", INDIV, (int)(strchr(ops[i], '.') - ops[i]),
                ops[i]);
    }
}

/* wait for defXXX or setXXX for each property in the expression.
 * return when find all operands are found or
 * exit(2) if time out waiting for all known operands.
 */
static void initProps(FILE *fp)
{
    alarm(timeout);
    while (allOperandsSet() < 0)
    {
        if (setOp(nxtEle(fp)) == 0)
            alarm(timeout);
    }
    alarm(0);
}

/* pull apart the name and value from the given message, and set operand value.
 * ignore any other messages.
 * return 0 if found a recognized operand else -1
 */
static int setOp(XMLEle *root)
{
    char *t       = tagXMLEle(root);
    const char *d = findXMLAttValu(root, "device");
    const char *n = findXMLAttValu(root, "name");
    int nset      = 0;
    double v;
    char prop[1024];
    XMLEle *ep;

    /* check values */
    if (!strcmp(t, "defNumberVector") || !strcmp(t, "setNumberVector"))
    {
        for (ep = nextXMLEle(root, 1); ep; ep = nextXMLEle(root, 0))
        {
            char *et = tagXMLEle(ep);
            if (!strcmp(et, "defNumber") || !strcmp(et, "oneNumber"))
            {
                sprintf(prop, "%s.%s.%s", d, n, findXMLAttValu(ep, "name"));
                v = atof(pcdataXMLEle(ep));
                if (setOperand(prop, v) == 0)
                {
                    nset++;
                    if (oflag)
                        fprintf(stderr, "%s=%g\n", prop, v);
                }
            }
        }
    }
    else if (!strcmp(t, "defSwitchVector") || !strcmp(t, "setSwitchVector"))
    {
        for (ep = nextXMLEle(root, 1); ep; ep = nextXMLEle(root, 0))
        {
            char *et = tagXMLEle(ep);

            if (!strcmp(et, "defSwitch") || !strcmp(et, "oneSwitch"))
            {
                sprintf(prop, "%s.%s.%s", d, n, findXMLAttValu(ep, "name"));
                v = (double)!strncmp(pcdataXMLEle(ep), "On", 2);
                if (setOperand(prop, v) == 0)
                {
                    nset++;
                    if (oflag)
                        fprintf(stderr, "%s=%g\n", prop, v);
                }
            }
        }
    }
    else if (!strcmp(t, "defLightVector") || !strcmp(t, "setLightVector"))
    {
        for (ep = nextXMLEle(root, 1); ep; ep = nextXMLEle(root, 0))
        {
            char *et = tagXMLEle(ep);
            if (!strcmp(et, "defLight") || !strcmp(et, "oneLight"))
            {
                sprintf(prop, "%s.%s.%s", d, n, findXMLAttValu(ep, "name"));
                v = (double)pstatestr(pcdataXMLEle(ep));
                if (setOperand(prop, v) == 0)
                {
                    nset++;
                    if (oflag)
                        fprintf(stderr, "%s=%g\n", prop, v);
                }
            }
        }
    }

    /* check special elements */
    t = (char *)findXMLAttValu(root, "state");
    if (t[0])
    {
        sprintf(prop, "%s.%s._STATE", d, n);
        v = (double)pstatestr(t);
        if (setOperand(prop, v) == 0)
        {
            nset++;
            if (oflag)
                fprintf(stderr, "%s=%g\n", prop, v);
        }
    }
    t = (char *)findXMLAttValu(root, "timestamp");
    if (t[0])
    {
        sprintf(prop, "%s.%s._TS", d, n);
        v = (double)timestampINDI(t);
        if (setOperand(prop, v) == 0)
        {
            nset++;
            if (oflag)
                fprintf(stderr, "%s=%g\n", prop, v);
        }
    }

    /* return whether any were set */
    return (nset > 0 ? 0 : -1);
}

/* evaluate the expression after seeing any operand change.
 * return whether expression evaluated to 0.
 * exit(2) is trouble or timeout waiting for operands we expect.
 */
static int runEval(FILE *fp)
{
    char errmsg[1024];
    double v;

    alarm(timeout);
    while (1)
    {
        if (evalExpr(&v, errmsg) < 0)
        {
            fprintf(stderr, "Eval: %s\n", errmsg);
            exit(2);
        }
        if (bflag && v)
            fprintf(stderr, "\a");
        if (eflag)
            fprintf(stderr, "%g\n", v);
        if (!wflag || v != 0)
            break;
        while (setOp(nxtEle(fp)) < 0)
            continue;
        alarm(timeout);
    }
    alarm(0);

    if (!eflag && fflag)
        fprintf(stderr, "%g\n", v);

    return (v == 0);
}

/* return 0|1|2|3 depending on whether state is Idle|Ok|Busy|<other>.
 */
static int pstatestr(char *state)
{
    if (!strcmp(state, "Idle"))
        return (0);
    if (!strncmp(state, "Ok", 2))
        return (1);
    if (!strcmp(state, "Busy"))
        return (2);
    return (3);
}

/* return UNIX time for the given ISO 8601 time string
 */
static time_t timestampINDI(char *ts)
{
    struct tm tm;

    if (6 == sscanf(ts, "%d-%d-%dT%d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec))
    {
        tm.tm_mon -= 1;     /* want 0..11 */
        tm.tm_year -= 1900; /* want years since 1900 */
        return (mktime(&tm));
    }
    else
        return ((time_t)-1);
}

/* return 0 if the device portion of the two given property specs match, else 1
 */
static int devcmp(char *op1, char *op2)
{
    int n1 = strchr(op1, '.') - op1;
    int n2 = strchr(op2, '.') - op2;
    return (n1 != n2 || strncmp(op1, op2, n1));
}

/* monitor server and return the next complete XML message.
 * exit(2) if time out.
 * N.B. caller must call delXMLEle()
 */
static XMLEle *nxtEle(FILE *fp)
{
    char msg[1024];

    /* read from server, exit if trouble or see malformed XML */
    while (1)
    {
        XMLEle *root = readXMLEle(lillp, readServerChar(fp), msg);
        if (root)
        {
            /* found a complete XML element */
            if (verbose > 1)
                prXMLEle(stderr, root, 0);
            return (root);
        }
        else if (msg[0])
        {
            fprintf(stderr, "Bad XML from %s/%d: %s\n", host, port, msg);
            exit(2);
        }
    }
}

/* read next char from the INDI server connected by fp */
static int readServerChar(FILE *fp)
{
    int c = fgetc(fp);

    if (c == EOF)
    {
        if (ferror(fp))
            perror("read");
        else
            fprintf(stderr, "INDI server %s/%d disconnected\n", host, port);
        exit(2);
    }

    if (verbose > 2)
        fprintf(stderr, "Read %c\n", c);

    return (c);
}

/* called after timeout seconds waiting to hear from server.
 * print reason for trouble and exit(2).
 */
static void onAlarm(int dummy)
{
    char **ops;
    int nops;

    INDI_UNUSED(dummy);

    /* report any unseen operands if any, else just say timed out */
    if ((nops = getUnsetOperands(&ops)) > 0)
    {
        fprintf(stderr, "No values seen for");
        while (nops-- > 0)
            fprintf(stderr, " %s", ops[nops]);
        fprintf(stderr, "\n");
    }
    else
        fprintf(stderr, "Timed out waiting for new values\n");

    exit(2);
}
