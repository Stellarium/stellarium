/*  This file, drvrhttp.c contains driver routines for http, ftp and root 
    files. */

/* This file was written by Bruce O'Neel at the ISDC, Switzerland          */
/*  The FITSIO software is maintained by William Pence at the High Energy  */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */


/* Notes on the drivers:

   The ftp driver uses passive mode exclusivly.  If your remote system can't 
   deal with passive mode then it'll fail.  Since Netscape Navigator uses 
   passive mode as well there shouldn't be too many ftp servers which have
   problems.


   The http driver works properly with 301 and 302 redirects.  For many more 
   gory details see http://www.w3c.org/Protocols/rfc2068/rfc2068.  The only
   catch to the 301/302 redirects is that they have to redirect to another 
   http:// url.  If not, things would have to change a lot in cfitsio and this
   was thought to be too difficult.
   
   Redirects look like


   <HTML><HEAD>
   <TITLE>301 Moved Permanently</TITLE>
   </HEAD><BODY>
   <H1>Moved Permanently</H1>
   The document has moved <A HREF="http://heasarc.gsfc.nasa.gov/FTP/software/ftools/release/other/image.fits.gz">here</A>.<P>
   </BODY></HTML>

   This redirect was from apache 1.2.5 but most of the other servers produce 
   something very similiar.  The parser for the redirects finds the first 
   anchor <A> tag in the body and goes there.  If that wasn't what was intended
   by the remote system then hopefully the error stack, which includes notes 
   about the redirect will help the user fix the problem.

  ****************************************************************
   Note added in 2017:  
   The redirect format shown above is actually preceded by 2 lines that look like
  
   HTTP/1.1 302 Found
   LOCATION: http://heasarc.gsfc.nasa.gov/FTP/software/ftools/release/other/image.fits.gz

   The CFITSIO parser now looks for the "Location:" string, not the html tag.
  ****************************************************************


   Root protocal doesn't have any real docs, so, the emperical docs are as 
   follows.  

   First, you must use a slightly modified rootd server.  The modifications 
   include implimentation of the stat command which returns the size of the 
   remote file.  Without that it's impossible for cfitsio to work properly
   since fitsfiles don't include any information about the size of the files 
   in the headers.  The rootd server closes the connections on any errors, 
   including reading beyond the end of the file or seeking beyond the end 
   of the file.  The rootd:// driver doesn't reopen a closed connection, if
   the connection is closed you're pretty much done.

   The messages are of the form

   <len><opcode><optional information>

   All binary information is transfered in network format, so use htonl and 
   ntohl to convert back and forth.

   <len> :== 4 byte length, in network format, the len doesn't include the
         length of <len>
   <opcode> :== one of the message opcodes below, 4 bytes, network format
   <optional info> :== depends on opcode

   The response is of the same form with the same opcode sent.  Success is
   indicated by <optional info> being 0.

   Root is a NFSish protocol where each read/write includes the byte
   offset to read or write to.  As a result, seeks will always succeed
   in the driver even if they would cause a fatal error when you try
   to read because you're beyond the end of the file.

   There is file locking on the host such that you need to possibly
   create /usr/tmp/rootdtab on the host system.  There is one file per
   socket connection, though the rootd daemon can support multiple
   files open at once.

   The messages are sent in the following order:

   ROOTD_USER - user name, <optional info> is the user name, trailing
   null is sent though it's not required it seems.  A ROOTD_AUTH
   message is returned with any sort of error meaning that the user
   name is wrong.

   ROOTD_PASS - password, ones complemented, stored in <optional info>. Once
   again the trailing null is sent.  Once again a ROOTD_AUTH message is 
   returned

   ROOTD_OPEN - <optional info> includes filename and one of
     {create|update|read} as the file mode.  ~ seems to be dealt with
     as the username's login directory.  A ROOTD_OPEN message is
     returned.

   Once the file is opened any of the following can be sent:

   ROOTD_STAT - file status and size
   returns a message where <optional info> is the file length in bytes

   ROOTD_FLUSH - flushes the file, not sure this has any real effect
   on the daemon since the daemon uses open/read/write/close rather
   than the buffered fopen/fread/fwrite/fclose.

   ROOTD_GET - on send <optional info> includes a text message of
   offset and length to get.  Return is a status message first with a
   status value, then, the raw bytes for the length that you
   requested.  It's an error to seek or read past the end of the file,
   and, the rootd daemon exits and won't respond anymore.  Ie, don't
   do this.

   ROOTD_PUT - on send <optional info> includes a text message of
   offset and length to put.  Then send the raw bytes you want to
   write.  Then recieve a status message


   When you are finished then you send the message:

   ROOTD_CLOSE - closes the file

   Once the file is closed then the socket is closed.


Revision 1.56  2000/01/04 11:58:31  oneel
Updates so that compressed network files are dealt with regardless of
their file names and/or mime types.

Revision 1.55  2000/01/04 10:52:40  oneel
cfitsio 2.034

Revision 1.51  1999/08/10 12:13:40  oneel
Make the http code a bit less picky about the types of files it
uncompresses.  Now it also uncompresses files which end in .Z or .gz.

Revision 1.50  1999/08/04 12:38:46  oneel
Don's 2.0.32 patch with dal 1.3

Revision 1.39  1998/12/02 15:31:33  oneel
Updates to drvrnet.c so that less compiler warnings would be
generated.  Fixes the signal handling.

Revision 1.38  1998/11/23 10:03:24  oneel
Added in a useragent string, as suggested by:
Tim Kimball   Data Systems Division   kimball@stsci.edu   410-338-4417
Space Telescope Science Institute     http://www.stsci.edu/~kimball/
3700 San Martin Drive                 http://archive.stsci.edu/
Baltimore MD 21218 USA                http://faxafloi.stsci.edu:4547/

   
 */

#ifdef HAVE_NET_SERVICES
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef CFITSIO_HAVE_CURL
#include <curl/curl.h>
#endif

#if defined(unix) || defined(__unix__)  || defined(__unix) || defined(HAVE_UNISTD_H)
#include <unistd.h>  
#endif

#include <signal.h>
#include <setjmp.h>
#include "fitsio2.h"

static jmp_buf env; /* holds the jump buffer for setjmp/longjmp pairs */
static void signal_handler(int sig);

/* Network routine error codes */
#define NET_OK 0
#define NOT_INET_ADDRESS -1000
#define UNKNOWN_INET_HOST -1001
#define CONNECTION_ERROR -1002

/* Network routine constants */
#define NET_DEFAULT 0
#define NET_OOB 1
#define NET_PEEK 2

#define NETTIMEOUT 180 /* in secs */

/* local defines and variables */
#define MAXLEN 1200
#define SHORTLEN 100
static char netoutfile[MAXLEN];


#define ROOTD_USER  2000       /*user id follows */
#define ROOTD_PASS  2001       /*passwd follows */
#define ROOTD_AUTH  2002       /*authorization status (to client) */
#define ROOTD_FSTAT 2003       /*filename follows */
#define ROOTD_OPEN  2004       /*filename follows + mode */
#define ROOTD_PUT   2005       /*offset, number of bytes and buffer */
#define ROOTD_GET   2006       /*offset, number of bytes */
#define ROOTD_FLUSH 2007       /*flush file */
#define ROOTD_CLOSE 2008       /*close file */
#define ROOTD_STAT  2009       /*return rootd statistics */
#define ROOTD_ACK   2010       /*acknowledgement (all OK) */
#define ROOTD_ERR   2011       /*error code and message follow */

typedef struct    /* structure containing disk file structure */ 
{
  int sock;
  LONGLONG currentpos;
} rootdriver;

typedef struct  /* simple mem struct for receiving files from curl */
{
   char *memory;
   size_t size;
} curlmembuf;

static rootdriver handleTable[NMAXFILES];  /* allocate diskfile handle tables */

/* static prototypes */

static int NET_TcpConnect(char *hostname, int port);
static int NET_SendRaw(int sock, const void *buf, int length, int opt);
static int NET_RecvRaw(int sock, void *buffer, int length);
static int NET_ParseUrl(const char *url, char *proto, char *host, int *port, 
		 char *fn);
static int CreateSocketAddress(struct sockaddr_in *sockaddrPtr,
			       char *host,int port);
static int ftp_status(FILE *ftp, char *statusstr);
static int http_open_network(char *url, FILE **httpfile, char *contentencoding,
			  int *contentlength);
static int https_open_network(char *filename, curlmembuf* buffer);
static int ftp_open_network(char *url, FILE **ftpfile, FILE **command, 
			    int *sock);
static int ftp_file_exist(char *url);
static int root_send_buffer(int sock, int op, char *buffer, int buflen);
static int root_recv_buffer(int sock, int *op, char *buffer,int buflen);
static int root_openfile(char *filename, char *rwmode, int *sock);
static int encode64(unsigned s_len, char *src, unsigned d_len, char *dst);
static size_t curlToMemCallback(void *buffer, size_t size, size_t nmemb, void *userp);

/***************************/
/* Static variables */

static int closehttpfile;
static int closememfile;
static int closefdiskfile;
static int closediskfile;
static int closefile;
static int closeoutfile;
static int closecommandfile;
static int closeftpfile;
static FILE *diskfile;
static FILE *outfile;

static int curl_verbose=0;

/*--------------------------------------------------------------------------*/
/* This creates a memory file handle with a copy of the URL in filename. The 
   file is uncompressed if necessary */

int http_open(char *filename, int rwmode, int *handle)
{

  FILE *httpfile;
  char contentencoding[SHORTLEN];
  char errorstr[MAXLEN];
  char recbuf[MAXLEN];
  long len;
  int contentlength;
  int status;
  char firstchar;

  closehttpfile = 0;
  closememfile = 0;

  /* don't do r/w files */
  if (rwmode != 0) {
    ffpmsg("Can't open http:// type file with READWRITE access");
    ffpmsg("  Specify an outfile for r/w access (http_open)");
    goto error;
  }

  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }

  (void) signal(SIGALRM, signal_handler);
  
  /* Open the network connection */

  if (http_open_network(filename,&httpfile,contentencoding,
			       &contentlength)) {
      alarm(0);
      ffpmsg("Unable to open http file (http_open):");
      ffpmsg(filename);
      goto error;
  } 

  closehttpfile++;

  /* Create the memory file */
  if ((status =  mem_create(filename,handle))) {
    ffpmsg("Unable to create memory file (http_open)");
    goto error;
  }

  closememfile++;

  /* Now, what do we do with the file */
  /* Check to see what the first character is */
  firstchar = fgetc(httpfile);
  ungetc(firstchar,httpfile);
  if (!strcmp(contentencoding,"x-gzip") || 
      !strcmp(contentencoding,"x-compress") ||
      strstr(filename,".gz") || 
      strstr(filename,".Z") ||
      ('\037' == firstchar)) {
    /* do the compress dance, which is the same as the gzip dance */
    /* Using the cfitsio routine */

    status = 0;
    /* Ok, this is a tough case, let's be arbritary and say 10*NETTIMEOUT,
       Given the choices for nettimeout above they'll probaby ^C before, but
       it's always worth a shot*/
    
    alarm(NETTIMEOUT*10);
    status = mem_uncompress2mem(filename, httpfile, *handle);
    alarm(0);
    if (status) {
      ffpmsg("Error writing compressed memory file (http_open)");
      ffpmsg(filename);
      goto error;
    }
    
  } else {
    /* It's not compressed, bad choice, but we'll copy it anyway */
    if (contentlength % 2880) {
      sprintf(errorstr,"Content-Length not a multiple of 2880 (http_open) %d",
	      contentlength);
      ffpmsg(errorstr);
    }

    /* write a memory file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,httpfile))) {
      alarm(0); /* cancel alarm */
      status = mem_write(*handle,recbuf,len);
      if (status) {
        ffpmsg("Error copying http file into memory (http_open)");
        ffpmsg(filename);
	goto error;
      }
      alarm(NETTIMEOUT); /* rearm the alarm */
    }
  }
  
  fclose(httpfile);

  signal(SIGALRM, SIG_DFL);
  alarm(0);
  return mem_seek(*handle,0);

 error:
  alarm(0); /* clear it */
  if (closehttpfile) {
    fclose(httpfile);
  }
  if (closememfile) {
    mem_close_free(*handle);
  }
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}

/*--------------------------------------------------------------------------*/
/* This creates a memory file handle with a copy of the URL in filename.  The
   file must be compressed and is copied (still compressed) to disk first. 
   The compressed disk file is then uncompressed into memory (READONLY).
*/

int http_compress_open(char *url, int rwmode, int *handle)
{
  FILE *httpfile;
  char contentencoding[SHORTLEN];
  char recbuf[MAXLEN];
  long len;
  int contentlength;
  int ii, flen, status;
  char firstchar;

  closehttpfile = 0;
  closediskfile = 0;
  closefdiskfile = 0;
  closememfile = 0;

  flen = strlen(netoutfile);
  if (!flen)  {
     /* cfileio made a mistake, should set the netoufile first otherwise 
        we don't know where to write the output file */
     ffpmsg
	("Output file not set, shouldn't have happened (http_compress_open)");
      goto error;
  }

  if (rwmode != 0) {
    ffpmsg("Can't open compressed http:// type file with READWRITE access");
    ffpmsg("  Specify an UNCOMPRESSED outfile (http_compress_open)");
    goto error;
  }
  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }

  signal(SIGALRM, signal_handler);
  
  /* Open the http connectin */
  alarm(NETTIMEOUT);
  if ((status = http_open_network(url,&httpfile,contentencoding,
			       &contentlength))) {
    alarm(0);
    ffpmsg("Unable to open http file (http_compress_open)");
    ffpmsg(url);
    goto error;
  }

  closehttpfile++;

  /* Better be compressed */

  firstchar = fgetc(httpfile);
  ungetc(firstchar,httpfile);
  if (!strcmp(contentencoding,"x-gzip") || 
      !strcmp(contentencoding,"x-compress") ||
      ('\037' == firstchar)) {

    if (*netoutfile == '!')
    {
       /* user wants to clobber file, if it already exists */
       for (ii = 0; ii < flen; ii++)
           netoutfile[ii] = netoutfile[ii + 1];  /* remove '!' */

       status = file_remove(netoutfile);
    }

    /* Create the new file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output disk file (http_compress_open):");
      ffpmsg(netoutfile);
      goto error;
    }
    
    closediskfile++;

    /* write a file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,httpfile))) {
      alarm(0);
      status = file_write(*handle,recbuf,len);
      if (status) {
	ffpmsg("Error writing disk file (http_compres_open)");
        ffpmsg(netoutfile);
	goto error;
      }
      alarm(NETTIMEOUT);
    }
    file_close(*handle);
    fclose(httpfile);
    closehttpfile--;
    closediskfile--;

    /* File is on disk, let's uncompress it into memory */

    if (NULL == (diskfile = fopen(netoutfile,"r"))) {
      ffpmsg("Unable to reopen disk file (http_compress_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    closefdiskfile++;

    /* Create the memory handle to hold it */
    if ((status =  mem_create(url,handle))) {
      ffpmsg("Unable to create memory file (http_compress_open)");
      goto error;
    }
    closememfile++;

    /* Uncompress it */
    status = 0;
    status = mem_uncompress2mem(url,diskfile,*handle);
    fclose(diskfile);
    closefdiskfile--;
    if (status) {
      ffpmsg("Error uncompressing disk file to memory (http_compress_open)");
      ffpmsg(netoutfile);
      goto error;
    }
      
  } else {
    /* Opps, this should not have happened */
    ffpmsg("Can only have compressed files here (http_compress_open)");
    goto error;
  }    
    
  signal(SIGALRM, SIG_DFL);
  alarm(0);
  return mem_seek(*handle,0);

 error:
  alarm(0); /* clear it */
  if (closehttpfile) {
    fclose(httpfile);
  }
  if (closefdiskfile) {
    fclose(diskfile);
  }
  if (closememfile) {
    mem_close_free(*handle);
  }
  if (closediskfile) {
    file_close(*handle);
  } 
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}

/*--------------------------------------------------------------------------*/
/* This creates a file handle with a copy of the URL in filename.  The http
   file is copied to disk first.  If it's compressed then it is
   uncompressed when copying to the disk */

int http_file_open(char *url, int rwmode, int *handle)
{
  FILE *httpfile;
  char contentencoding[SHORTLEN];
  char errorstr[MAXLEN];
  char recbuf[MAXLEN];
  long len;
  int contentlength;
  int ii, flen, status;
  char firstchar;

  /* Check if output file is actually a memory file */
  if (!strncmp(netoutfile, "mem:", 4) )
  {
     /* allow the memory file to be opened with write access */
     return( http_open(url, READONLY, handle) );
  }     

  closehttpfile = 0;
  closefile = 0;
  closeoutfile = 0;

  flen = strlen(netoutfile);
  if (!flen) {
      /* cfileio made a mistake, we need to know where to write the file */
      ffpmsg("Output file not set, shouldn't have happened (http_file_open)");
      return (FILE_NOT_OPENED);
  }

  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }

  signal(SIGALRM, signal_handler);
  
  /* Open the network connection */
  alarm(NETTIMEOUT);
  if ((status = http_open_network(url,&httpfile,contentencoding,
			       &contentlength))) {
    alarm(0);
    ffpmsg("Unable to open http file (http_file_open)");
    ffpmsg(url);
    goto error;
  }

  closehttpfile++;

  if (*netoutfile == '!')
  {
     /* user wants to clobber disk file, if it already exists */
     for (ii = 0; ii < flen; ii++)
         netoutfile[ii] = netoutfile[ii + 1];  /* remove '!' */

     status = file_remove(netoutfile);
  }

  firstchar = fgetc(httpfile);
  ungetc(firstchar,httpfile);
  if (!strcmp(contentencoding,"x-gzip") || 
      !strcmp(contentencoding,"x-compress") ||
      ('\037' == firstchar)) {

    /* to make this more cfitsioish we use the file driver calls to create
       the disk file */

    /* Create the output file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output file (http_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }

    file_close(*handle);
    if (NULL == (outfile = fopen(netoutfile,"w"))) {
      ffpmsg("Unable to reopen the output file (http_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    closeoutfile++;
    status = 0;

    /* Ok, this is a tough case, let's be arbritary and say 10*NETTIMEOUT,
       Given the choices for nettimeout above they'll probaby ^C before, but
       it's always worth a shot*/

    alarm(NETTIMEOUT*10);
    status = uncompress2file(url,httpfile,outfile,&status);
    alarm(0);
    if (status) {
      ffpmsg("Error uncompressing http file to disk file (http_file_open)");
      ffpmsg(url);
      ffpmsg(netoutfile);
      goto error;
    }
    fclose(outfile);
    closeoutfile--;
  } else {
    
    /* Create the output file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output file (http_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    
    /* Give a warning message.  This could just be bad padding at the end
       so don't treat it like an error. */
    closefile++;
    
    if (contentlength % 2880) {
      sprintf(errorstr,
	      "Content-Length not a multiple of 2880 (http_file_open) %d",
	      contentlength);
      ffpmsg(errorstr);
    }
    
    /* write a file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,httpfile))) {
      alarm(0);
      status = file_write(*handle,recbuf,len);
      if (status) {
	ffpmsg("Error copying http file to disk file (http_file_open)");
        ffpmsg(url);
        ffpmsg(netoutfile);
	goto error;
      }
    }
    file_close(*handle);
    closefile--;
  }
  
  fclose(httpfile);
  closehttpfile--;

  signal(SIGALRM, SIG_DFL);
  alarm(0);

  return file_open(netoutfile,rwmode,handle); 

 error:
  alarm(0); /* clear it */
  if (closehttpfile) {
    fclose(httpfile);
  }
  if (closeoutfile) {
    fclose(outfile);
  }
  if (closefile) {
    file_close(*handle);
  } 
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}

/*--------------------------------------------------------------------------*/
/* This is the guts of the code to get a file via http.  
   url is the input url
   httpfile is set to be the file connected to the socket which you can
     read the file from
   contentencoding is the mime type of the file, returned if the http server
     returns it
   contentlength is the length of the file, returned if the http server returns
     it
*/
static int http_open_network(char *url, FILE **httpfile, char *contentencoding,
			  int *contentlength)
{

  int status;
  int sock;
  int tmpint;
  char recbuf[MAXLEN];
  char tmpstr[MAXLEN];
  char tmpstr1[SHORTLEN];
  char tmpstr2[MAXLEN];
  char errorstr[MAXLEN];
  char proto[SHORTLEN];
  char host[SHORTLEN];
  char userpass[MAXLEN];
  char fn[MAXLEN];
  char turl[MAXLEN];
  char *scratchstr;
  char *scratchstr2;
  char *saveptr;
  int port;
  float version;

  char pproto[SHORTLEN];
  char phost[SHORTLEN]; /* address of the proxy server */
  int  pport;  /* port number of the proxy server */
  char pfn[MAXLEN];
  char *proxy; /* URL of the proxy server */

  /* Parse the URL apart again */
  strcpy(turl,"http://");
  strncat(turl,url,MAXLEN - 8);
  if (NET_ParseUrl(turl,proto,host,&port,fn)) {
    sprintf(errorstr,"URL Parse Error (http_open) %s",url);
    ffpmsg(errorstr);
    return (FILE_NOT_OPENED);
  }

  /* Do we have a user:password combo ? */
    strcpy(userpass, url);
  if ((scratchstr = strchr(userpass, '@')) != NULL) {
    *scratchstr = '\0';
  } else {
    strcpy(userpass, "");
  }

  /* Ph. Prugniel 2003/04/03
     Are we using a proxy?
     
     We use a proxy if the environment variable "http_proxy" is set to an
     address, eg. http://wwwcache.nottingham.ac.uk:3128
     ("http_proxy" is also used by wget)
  */
  proxy = getenv("http_proxy");

  /* Connect to the remote host */
  if (proxy) {
    if (NET_ParseUrl(proxy,pproto,phost,&pport,pfn)) {
      sprintf(errorstr,"URL Parse Error (http_open) %s",proxy);
      ffpmsg(errorstr);
      return (FILE_NOT_OPENED);
    }
    sock = NET_TcpConnect(phost,pport);
  }  else {
    sock = NET_TcpConnect(host,port); 
  }

  if (sock < 0) {
    if (proxy) {
      ffpmsg("Couldn't connect to host via proxy server (http_open_network)");
      ffpmsg(proxy);
    }
    return (FILE_NOT_OPENED);
  }

  /* Make the socket a stdio file */
  if (NULL == (*httpfile = fdopen(sock,"r"))) {
    ffpmsg ("fdopen failed to convert socket to file (http_open_network)");
    close(sock);
    return (FILE_NOT_OPENED);
  }

  /* Send the GET request to the remote server */
  /* Ph. Prugniel 2003/04/03 
     One must add the Host: command because of HTTP 1.1 servers (ie. virtual
     hosts) */

  if (proxy) {
    sprintf(tmpstr,"GET http://%s:%-d%s HTTP/1.0\r\n",host,port,fn);
  } else {
    sprintf(tmpstr,"GET %s HTTP/1.0\r\n",fn);
  }

  if (strcmp(userpass, "")) {
    encode64(strlen(userpass), userpass, MAXLEN, tmpstr2);
    sprintf(tmpstr1, "Authorization: Basic %s\r\n", tmpstr2);

    if (strlen(tmpstr) + strlen(tmpstr1) > MAXLEN - 1)
        return (FILE_NOT_OPENED);

    strcat(tmpstr,tmpstr1);
  }

/*  sprintf(tmpstr1,"User-Agent: HEASARC/CFITSIO/%-8.3f\r\n",ffvers(&version)); */

/*  sprintf(tmpstr1,"User-Agent: CFITSIO/HEASARC/%-8.3f\r\n",ffvers(&version)); */
  sprintf(tmpstr1,"User-Agent: FITSIO/HEASARC/%-8.3f\r\n",ffvers(&version)); 
 
  if (strlen(tmpstr) + strlen(tmpstr1) > MAXLEN - 1)
        return (FILE_NOT_OPENED);

  strcat(tmpstr,tmpstr1);

  /* HTTP 1.1 servers require the following 'Host: ' string */
  sprintf(tmpstr1,"Host: %s:%-d\r\n\r\n",host,port);

  if (strlen(tmpstr) + strlen(tmpstr1) > MAXLEN - 1)
        return (FILE_NOT_OPENED);

  strcat(tmpstr,tmpstr1);

  status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);

  /* read the header */
  if (!(fgets(recbuf,MAXLEN,*httpfile))) {
    sprintf (errorstr,"http header short (http_open_network) %s",recbuf);
    ffpmsg(errorstr);
    fclose(*httpfile);
    return (FILE_NOT_OPENED);
  }

  *contentlength = 0;
  contentencoding[0] = '\0';

  /* Our choices are 200, ok, 302, temporary redirect, or 301 perm redirect */
  sscanf(recbuf,"%s %d",tmpstr,&status);
  if (status != 200){
    if (status == 301 || status == 302) {
      /* got a redirect */

/*
      if (status == 302) {
	ffpmsg("Note: Web server replied with a temporary redirect from");
      } else {
	ffpmsg("Note: Web server replied with a redirect from");
      }
      ffpmsg(turl);
*/
      /* now, let's not write the most sophisticated parser here */

      while (fgets(recbuf,MAXLEN,*httpfile)) {

	scratchstr = strstr(recbuf,"Location: ");
	if (scratchstr != NULL) {

	  /* Ok, we found the Location line which gives the redirected URL */
          /* skip the "Location: "  charactrers */
	  scratchstr += 10; 
             
	  /* strip off any end-of-line characters */
          tmpint = strlen(scratchstr);
	  if (scratchstr[tmpint-1] == '\r') scratchstr[tmpint-1] = '\0';
          tmpint = strlen(scratchstr);
          if (scratchstr[tmpint-1] == '\n') scratchstr[tmpint-1] = '\0';
          tmpint = strlen(scratchstr);
	  if (scratchstr[tmpint-1] == '\r') scratchstr[tmpint-1] = '\0';

/*
	  ffpmsg("to:");
	  ffpmsg(scratchstr);
	  ffpmsg(" ");
*/
	  scratchstr2 = strstr(scratchstr,"http://");
          if (scratchstr2 != NULL) {
	     /* Ok, we found the HTTP redirection is to another HTTP URL. */
	     /* We can handle this case directly, here */
	     /* skip the "http://" characters */
	     scratchstr2 += 7;
	     strcpy(turl, scratchstr2);
	     fclose (*httpfile);

             /* note the recursive call to itself */
	     return 
	        http_open_network(turl,httpfile,contentencoding,contentlength);
          }

          /* It was not a HTTP to HTTP redirection, so see if it HTTP to FTP */
	  scratchstr2 = strstr(scratchstr,"ftp://");
          if (scratchstr2 != NULL) {
	     /* Ok, we found the HTTP redirection is to a FTP URL. */
	     /* skip the "ftp://" characters */
	     scratchstr2 += 6;

             /* return the new URL string, and set contentencoding to "ftp" as
	        a flag to the http_checkfile routine
	     */
	     strcpy(url, scratchstr2);
             strcpy(contentencoding,"ftp://");
	     fclose (*httpfile); 
	     return 0;
          }
          
          /* Now check for HTTP to HTTPS redirection. */
	  scratchstr2 = strstr(scratchstr,"https://");
          if (scratchstr2 != NULL) {
             /* skip the "https://" characters */
             scratchstr2 += 8;
             
             /* return the new URL string, and set contentencoding to "https" as
	        a flag to the http_checkfile routine
	     */
             strcpy(url, scratchstr2);
             strcpy(contentencoding,"https://");
             fclose(*httpfile);
             return 0;
          }
          
	}
      }

      /* if we get here then we couldnt' decide the redirect */
      ffpmsg("but we were unable to find the redirected url in the servers response");
    }

    /* error.  could not open the http file */
    fclose(*httpfile);
    return (FILE_NOT_OPENED);
  }

  /* from here the first word holds the keyword we want */
  /* so, read the rest of the header */
  while (fgets(recbuf,MAXLEN,*httpfile)) {
    /* Blank line ends the header */
    if (*recbuf == '\r') break;
    if (strlen(recbuf) > 3) {
      recbuf[strlen(recbuf)-1] = '\0';
      recbuf[strlen(recbuf)-1] = '\0';
    }
    sscanf(recbuf,"%s %d",tmpstr,&tmpint);
    /* Did we get a content-length header ? */
    if (!strcmp(tmpstr,"Content-Length:")) {
      *contentlength = tmpint;
    }
    /* Did we get the content-encoding header ? */
    if (!strcmp(tmpstr,"Content-Encoding:")) {
      if (NULL != (scratchstr = strstr(recbuf,":"))) {
	/* Found the : */
	scratchstr++; /* skip the : */
	scratchstr++; /* skip the extra space */
	strcpy(contentencoding,scratchstr);
      }
    }
  }
  
  /* we're done, so return */
  return 0;
}

/*--------------------------------------------------------------------------*/
/* This creates a memory file handle with a copy of the URL in filename. The 
   curl library called from https_open_network will perform file uncompression
   if necessary. */
int https_open(char *filename, int rwmode, int *handle)
{
  curlmembuf inmem;
  char errStr[MAXLEN];
  int status=0;
    
  /* don't do r/w files */
  if (rwmode != 0) {
    ffpmsg("Can't open https:// type file with READWRITE access");
    ffpmsg("  Specify an outfile for r/w access (https_open)");
    return (FILE_NOT_OPENED);
  }

  inmem.memory=0;
  inmem.size=0;
  if (setjmp(env) != 0)
  {
    alarm(0);
    signal(SIGALRM, SIG_DFL);
    ffpmsg("Timeout (https_open)");
    free(inmem.memory);
    return (FILE_NOT_OPENED);
  }

  signal(SIGALRM, signal_handler);
  alarm(NETTIMEOUT);

  if (https_open_network(filename, &inmem))
  {
     alarm(0);
     signal(SIGALRM, SIG_DFL);
     ffpmsg("Unable to read https file into memory (https_open)");
     free(inmem.memory);
     return (FILE_NOT_OPENED);  
  }
  alarm(0);
  signal(SIGALRM, SIG_DFL);
  /* We now have the file transfered from the https server into the
     inmem.memory buffer.  Now transfer that into a FITS memory file. */
  if ((status = mem_create(filename, handle)))
  {
     ffpmsg("Unable to create memory file (https_open)");
     free(inmem.memory);
     return (FILE_NOT_OPENED);
  }
  
  if (inmem.size % 2880)
  {
     sprintf(errStr,"Content-Length not a multiple of 2880 (https_open) %u",
         inmem.size);
     ffpmsg(errStr);
  }
  status = mem_write(*handle, inmem.memory, inmem.size);
  if (status)
  {
     ffpmsg("Error copying https file into memory (https_open)");
     ffpmsg(filename);
     free(inmem.memory);
     mem_close_free(*handle);
     return (FILE_NOT_OPENED);
  }
  free(inmem.memory);
  return mem_seek(*handle, 0);
   
}

/*--------------------------------------------------------------------------*/
int https_file_open(char *filename, int rwmode, int *handle)
{
  int ii, flen;
  char errStr[MAXLEN];
  curlmembuf inmem;
  
  /* Check if output file is actually a memory file */
  if (!strncmp(netoutfile, "mem:", 4) )
  {
     /* allow the memory file to be opened with write access */
     return( https_open(filename, READONLY, handle) );
  }     

  flen = strlen(netoutfile);
  if (!flen)
  {
      /* cfileio made a mistake, we need to know where to write the file */
      ffpmsg("Output file not set, shouldn't have happened (https_file_open)");
      return (FILE_NOT_OPENED);
  }
  
  inmem.memory=0;
  inmem.size=0;
  if (setjmp(env) != 0)
  {
     alarm(0);
     signal(SIGALRM, SIG_DFL);
     ffpmsg("Timeout (https_file_open)");
     free(inmem.memory);
     return (FILE_NOT_OPENED);
  }
  signal(SIGALRM, signal_handler);
  alarm(NETTIMEOUT);
  if (https_open_network(filename, &inmem))
  {
     alarm(0);
     signal(SIGALRM, SIG_DFL);
     ffpmsg("Unable to read https file into memory (https_file_open)");
     free(inmem.memory);
     return (FILE_NOT_OPENED);  
  }
  alarm(0);
  signal(SIGALRM, SIG_DFL);
  
  if (*netoutfile == '!')
  {
     /* user wants to clobber disk file, if it already exists */
     for (ii = 0; ii < flen; ii++)
         netoutfile[ii] = netoutfile[ii + 1];  /* remove '!' */

     file_remove(netoutfile);
  }

  /* Create the output file */
  if (file_create(netoutfile,handle)) 
  {
    ffpmsg("Unable to create output file (https_file_open)");
    ffpmsg(netoutfile);
    free(inmem.memory);
    return (FILE_NOT_OPENED);
  }
    
  if (inmem.size % 2880)
  {
    sprintf(errStr,
	    "Content-Length not a multiple of 2880 (https_file_open) %d",
	    inmem.size);
    ffpmsg(errStr);
  }
   
  if (file_write(*handle, inmem.memory, inmem.size))
  {
     ffpmsg("Error copying https file to disk file (https_file_open)");
     ffpmsg(filename);
     ffpmsg(netoutfile);
     free(inmem.memory);
     file_close(*handle);
     return (FILE_NOT_OPENED);
  }
  free(inmem.memory); 
  file_close(*handle);
     
  return file_open(netoutfile, rwmode, handle);
}

/*--------------------------------------------------------------------------*/
/* Callback function curl library uses during https connection to transfer
   server file into memory */
size_t curlToMemCallback(void *buffer, size_t size, size_t nmemb, void *userp)
{
   curlmembuf* inmem = (curlmembuf* )userp;
   size_t transferSize = size*nmemb;
   if (!inmem->size)
   {
      /* First time through - initialize with malloc */
      inmem->memory = (char *)malloc(transferSize); 
   }
   else
      inmem->memory = realloc(inmem->memory, inmem->size+transferSize);
   if (inmem->memory == NULL)
   {
      ffpmsg("realloc error - not enough memory (curlToMemCallback)\n");
      return 0;
   }
   memcpy(&(inmem->memory[inmem->size]), buffer, transferSize);
   inmem->size += transferSize;
   
   return transferSize;
}

/*--------------------------------------------------------------------------*/
int https_open_network(char *filename, curlmembuf* buffer)
{
  char *urlname=0;
  char errStr[MAXLEN];
  char agentStr[MAXLEN];
  float version=0.0;
  char *verify=0;
  /* These settings will force libcurl to perform host and peer authentication.
     If it fails, this routine will try again without authentication (unless
     user forbids this via CFITSIO_VERIFY_HTTPS environment variable).
  */
  long verifyPeer = 1;
  long verifyHost = 2;
#ifdef CFITSIO_HAVE_CURL
  CURL *curl=0;
  CURLcode res;
  char curlErrBuf[CURL_ERROR_SIZE];
  
  if (strstr(filename,".Z"))
  {
     ffpmsg("x-compress .Z format not currently supported with https transfers");
     return(FILE_NOT_OPENED);
  }

  
  /* Will ASSUME curl_global_init has been called by this point.
     It is not thread-safe to call it here. */
  curl = curl_easy_init();
   
  res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifyPeer);
  if (res != CURLE_OK)
  {
     ffpmsg("ERROR: CFITSIO was built with a libcurl library that ");
     ffpmsg("does not have SSL support, and therefore can't perform https transfers.");
     return (FILE_NOT_OPENED);    
  }
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyHost);
  
  curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)curl_verbose);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlToMemCallback);
  sprintf(agentStr,"User-Agent: FITSIO/HEASARC/%-8.3f",ffvers(&version)); 
  curl_easy_setopt(curl, CURLOPT_USERAGENT,agentStr);
  
  buffer->memory = 0; /* malloc/realloc will grow this in the callback function */
  buffer->size = 0;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)buffer);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlErrBuf);
  curlErrBuf[0]=0;
  /* This is needed for easy_perform to return an error whenever http server
      returns an error >= 400, ie. if it can't find the requested file. */
  curl_easy_setopt(curl, CURLOPT_FAILONERROR,  1L);
  /* This turns on automatic decompression for all recognized types. */
  curl_easy_setopt(curl, CURLOPT_ENCODING, "");
  
  /* urlname should be large enough to accomodate "https://"+filename+".gz". */
  urlname = (char *)malloc(strlen(filename)+12);
  strcpy(urlname, "https://");
  strcat(urlname, filename);
  /* Unless filename already contains a .gz or '?' (probably from a cgi script),
     first try with .gz appended. */
  if (!strstr(filename,".gz") && !strstr(filename,"?"))
     strcat(urlname, ".gz");
  
  /* First attempt: verification on */
  curl_easy_setopt(curl, CURLOPT_URL, urlname);
  res = curl_easy_perform(curl);
  if (res != CURLE_OK && res != CURLE_HTTP_RETURNED_ERROR)
  {
     /*   CURLE_HTTP_RETURNED_ERROR is what gets returned if HTTP server
        returns an error code >= 400. If that's not causing this error, assume
        it is a verification issue. 
          Try again with verification removed, unless user disallowed it
        via environment variable. */
     verify = getenv("CFITSIO_VERIFY_HTTPS");
     if (verify)
     {
        if (verify[0] == 'T' || verify[0] == 't')
        {
           sprintf(errStr,"libcurl error: %d",res);
           ffpmsg(errStr);
           if (strlen(curlErrBuf))
              ffpmsg(curlErrBuf);     
           curl_easy_cleanup(curl);  
           free(urlname);
           return (FILE_NOT_OPENED);
        }
     }
     verifyPeer = 0;
     verifyHost = 0;
     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifyPeer);
     curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyHost);
     res = curl_easy_perform(curl);
     if (res != CURLE_OK)
     {
        /* Unless original filename already contains a .gz or '?' (probably from a cgi script),
           try again without.gz appended. */
        if (!strstr(filename,".gz") && !strstr(filename,"?"))
        {
           strcpy(urlname, "https://");
           strcat(urlname, filename);        
           curl_easy_setopt(curl, CURLOPT_URL, urlname); 
           res = curl_easy_perform(curl);
           if (res != CURLE_OK)
           {
              sprintf(errStr,"libcurl error: %d",res);
              ffpmsg(errStr);
              if (strlen(curlErrBuf))
                 ffpmsg(curlErrBuf);     
              curl_easy_cleanup(curl);  
              free(urlname);
              return (FILE_NOT_OPENED);
           }
           else
              fprintf(stderr, "Warning: Unable to perform SSL verification on https transfer from: %s\n",
                   urlname);           
        }
        else
        {
           sprintf(errStr,"libcurl error: %d",res);
           ffpmsg(errStr);
           if (strlen(curlErrBuf))
              ffpmsg(curlErrBuf);     
           curl_easy_cleanup(curl);  
           free(urlname);
           return (FILE_NOT_OPENED);
        }        
     }
     else
        fprintf(stderr, "Warning: Unable to perform SSL verification on https transfer from: %s\n",
             urlname);

  }
  else if (res == CURLE_HTTP_RETURNED_ERROR)
  {
     /* Verification isn't the problem.  No need to relax peer/host checking */
     /* Unless filename already contains a .gz or '?' (probably from a cgi script),
        try again with original filename unappended */
     if (!strstr(filename,".gz") && !strstr(filename,"?"))
     {
        strcpy(urlname, "https://");
        strcat(urlname, filename);        
        curl_easy_setopt(curl, CURLOPT_URL, urlname); 
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
           sprintf(errStr,"libcurl error: %d",res);
           ffpmsg(errStr);
           if (strlen(curlErrBuf))
              ffpmsg(curlErrBuf);     
           curl_easy_cleanup(curl);  
           free(urlname);
           return (FILE_NOT_OPENED);
        }
     }
     else
     {
        sprintf(errStr,"libcurl error: %d",res);
        ffpmsg(errStr);
        if (strlen(curlErrBuf))
           ffpmsg(curlErrBuf);     
        curl_easy_cleanup(curl);  
        free(urlname);
        return (FILE_NOT_OPENED);
     }
  }
  
  free(urlname);
  curl_easy_cleanup(curl);  
  
   return 0;

#else
   ffpmsg("ERROR: This CFITSIO build was not compiled with the libcurl library package ");
   ffpmsg("and therefore it cannot perform HTTPS connections.");   
#endif
  
  return (FILE_NOT_OPENED);
}

void https_set_verbose(int flag)
{
   if (!flag)
      curl_verbose = 0;
   else
      curl_verbose = 1;
}

/*--------------------------------------------------------------------------*/
/* This creates a memory file handle with a copy of the URL in filename. The 
   file is uncompressed if necessary */

int ftp_open(char *filename, int rwmode, int *handle)
{
  FILE *ftpfile;
  FILE *command;
  int sock;
  char recbuf[MAXLEN];
  long len;
  int status;
  char firstchar;

  closememfile = 0;
  closecommandfile = 0;
  closeftpfile = 0;

  /* don't do r/w files */
  if (rwmode != 0) {
    ffpmsg("Can't open ftp:// type file with READWRITE access");
    ffpmsg("Specify an outfile for r/w access (ftp_open)");
    return (FILE_NOT_OPENED);
  }

  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }

  signal(SIGALRM, signal_handler);
  
  /* Open the ftp connetion.  ftpfile is connected to the file port, 
     command is connected to port 21.  sock is the socket on port 21 */

  if (strlen(filename) > MAXLEN - 4) {
      ffpmsg("filename too long (ftp_open)");
      ffpmsg(filename);
      goto error;
  } 

  alarm(NETTIMEOUT);
  if (ftp_open_network(filename,&ftpfile,&command,&sock)) {

      alarm(0);
      ffpmsg("Unable to open following ftp file (ftp_open):");
      ffpmsg(filename);
      goto error;
  } 

  closeftpfile++;
  closecommandfile++;

  /* create the memory file */
  if ((status = mem_create(filename,handle))) {
    ffpmsg ("Could not create memory file to passive port (ftp_open)");
    ffpmsg(filename);
    goto error;
  }
  closememfile++;
  /* This isn't quite right, it'll fail if the file has .gzabc at the end
     for instance */

  /* Decide if the file is compressed */
  firstchar = fgetc(ftpfile);
  ungetc(firstchar,ftpfile);

  if (strstr(filename,".gz") || 
      strstr(filename,".Z") ||
      ('\037' == firstchar)) {
    
    status = 0;
    /* A bit arbritary really, the user will probably hit ^C */
    alarm(NETTIMEOUT*10);
    status = mem_uncompress2mem(filename, ftpfile, *handle);
    alarm(0);
    if (status) {
      ffpmsg("Error writing compressed memory file (ftp_open)");
      ffpmsg(filename);
      goto error;
    }
  } else {
    /* write a memory file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,ftpfile))) {
      alarm(0);
      status = mem_write(*handle,recbuf,len);
      if (status) {
	ffpmsg("Error writing memory file (http_open)");
        ffpmsg(filename);
	goto error;
      }
      alarm(NETTIMEOUT);
    }
  }

  /* close and clean up */
  fclose(ftpfile);
  closeftpfile--;

  fclose(command);
  NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  closecommandfile--;

  signal(SIGALRM, SIG_DFL);
  alarm(0);

  return mem_seek(*handle,0);

 error:
  alarm(0); /* clear it */
  if (closecommandfile) {
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  }
  if (closeftpfile) {
    fclose(ftpfile);
  }
  if (closememfile) {
    mem_close_free(*handle);
  }
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}
/*--------------------------------------------------------------------------*/
/* This creates a file handle with a copy of the URL in filename. The 
   file must be  uncompressed and is copied to disk first */

int ftp_file_open(char *url, int rwmode, int *handle)
{
  FILE *ftpfile;
  FILE *command;
  char recbuf[MAXLEN];
  long len;
  int sock;
  int ii, flen, status;
  char firstchar;

  /* Check if output file is actually a memory file */
  if (!strncmp(netoutfile, "mem:", 4) )
  {
     /* allow the memory file to be opened with write access */
     return( ftp_open(url, READONLY, handle) );
  }     

  closeftpfile = 0;
  closecommandfile = 0;
  closefile = 0;
  closeoutfile = 0;
  
  /* cfileio made a mistake, need to know where to write the output file */
  flen = strlen(netoutfile);
  if (!flen) 
    {
      ffpmsg("Output file not set, shouldn't have happened (ftp_file_open)");
      return (FILE_NOT_OPENED);
    }

  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }

  signal(SIGALRM, signal_handler);
  
  /* open the network connection to url. ftpfile holds the connection to
     the input file, command holds the connection to port 21, and sock is 
     the socket connected to port 21 */

  alarm(NETTIMEOUT);
  if ((status = ftp_open_network(url,&ftpfile,&command,&sock))) {
    alarm(0);
    ffpmsg("Unable to open http file (ftp_file_open)");
    ffpmsg(url);
    goto error;
  }
  closeftpfile++;
  closecommandfile++;

  if (*netoutfile == '!')
  {
     /* user wants to clobber file, if it already exists */
     for (ii = 0; ii < flen; ii++)
         netoutfile[ii] = netoutfile[ii + 1];  /* remove '!' */

     status = file_remove(netoutfile);
  }

  /* Now, what do we do with the file */
  firstchar = fgetc(ftpfile);
  ungetc(firstchar,ftpfile);

  if (strstr(url,".gz") || 
      strstr(url,".Z") ||
      ('\037' == firstchar)) {

    /* to make this more cfitsioish we use the file driver calls to create
       the file */
    /* Create the output file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output file (ftp_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }

    file_close(*handle);
    if (NULL == (outfile = fopen(netoutfile,"w"))) {
      ffpmsg("Unable to reopen the output file (ftp_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    closeoutfile++;
    status = 0;

    /* Ok, this is a tough case, let's be arbritary and say 10*NETTIMEOUT,
       Given the choices for nettimeout above they'll probaby ^C before, but
       it's always worth a shot*/

    alarm(NETTIMEOUT*10);
    status = uncompress2file(url,ftpfile,outfile,&status);
    alarm(0);
    if (status) {
      ffpmsg("Unable to uncompress the output file (ftp_file_open)");
      ffpmsg(url);
      ffpmsg(netoutfile);
      goto error;
    }
    fclose(outfile);
    closeoutfile--;

  } else {
    
    /* Create the output file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output file (ftp_file_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    closefile++;
    
    /* write a file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,ftpfile))) {
      alarm(0);
      status = file_write(*handle,recbuf,len);
      if (status) {
	ffpmsg("Error writing file (ftp_file_open)");
        ffpmsg(url);
        ffpmsg(netoutfile);
	goto error;
      }
      alarm(NETTIMEOUT);
    }
    file_close(*handle);
  }
  fclose(ftpfile);
  closeftpfile--;
  
  fclose(command);
  NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  closecommandfile--;

  signal(SIGALRM, SIG_DFL);
  alarm(0);

  return file_open(netoutfile,rwmode,handle);

 error:
  alarm(0); /* clear it */
  if (closeftpfile) {
    fclose(ftpfile);
  }
  if (closecommandfile) {
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  }
  if (closeoutfile) {
    fclose(outfile);
  }
  if (closefile) {
    file_close(*handle);
  } 
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}

/*--------------------------------------------------------------------------*/
/* This creates a memory  handle with a copy of the URL in filename. The 
   file must be compressed and is copied to disk first */

int ftp_compress_open(char *url, int rwmode, int *handle)
{
  FILE *ftpfile;
  FILE *command;
  char recbuf[MAXLEN];
  long len;
  int ii, flen, status;
  int sock;
  char firstchar;

  closeftpfile = 0;
  closecommandfile = 0;
  closememfile = 0;
  closefdiskfile = 0;
  closediskfile = 0;

  /* don't do r/w files */
  if (rwmode != 0) {
    ffpmsg("Compressed files must be r/o");
    return (FILE_NOT_OPENED);
  }
  
  /* Need to know where to write the output file */
  flen = strlen(netoutfile);
  if (!flen) 
    {
      ffpmsg(
	"Output file not set, shouldn't have happened (ftp_compress_open)");
      return (FILE_NOT_OPENED);
    }
  
  /* do the signal handler bits */
  if (setjmp(env) != 0) {
    /* feels like the second time */
    /* this means something bad happened */
    ffpmsg("Timeout (http_open)");
    goto error;
  }
  
  signal(SIGALRM, signal_handler);
  
  /* Open the network connection to url, ftpfile is connected to the file 
     port, command is connected to port 21.  sock is for writing to port 21 */
  alarm(NETTIMEOUT);

  if ((status = ftp_open_network(url,&ftpfile,&command,&sock))) {
    alarm(0);
    ffpmsg("Unable to open ftp file (ftp_compress_open)");
    ffpmsg(url);
    goto error;
  }
  closeftpfile++;
  closecommandfile++;

  /* Now, what do we do with the file */
  firstchar = fgetc(ftpfile);
  ungetc(firstchar,ftpfile);

  if (strstr(url,".gz") || 
      strstr(url,".Z") ||
      ('\037' == firstchar)) {
  
    if (*netoutfile == '!')
    {
       /* user wants to clobber file, if it already exists */
       for (ii = 0; ii < flen; ii++)
          netoutfile[ii] = netoutfile[ii + 1];  /* remove '!' */

       status = file_remove(netoutfile);
    }

    /* Create the output file */
    if ((status =  file_create(netoutfile,handle))) {
      ffpmsg("Unable to create output file (ftp_compress_open)");
      ffpmsg(netoutfile);
      goto error;
    }
    closediskfile++;
    
    /* write a file */
    alarm(NETTIMEOUT);
    while(0 != (len = fread(recbuf,1,MAXLEN,ftpfile))) {
      alarm(0);
      status = file_write(*handle,recbuf,len);
      if (status) {
	ffpmsg("Error writing file (ftp_compres_open)");
        ffpmsg(url);
        ffpmsg(netoutfile);
	goto error;
      }
      alarm(NETTIMEOUT);
    }

    file_close(*handle);
    closediskfile--;
    fclose(ftpfile);
    closeftpfile--;
    /* Close down the ftp connection */
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    closecommandfile--;

    /* File is on disk, let's uncompress it into memory */

    if (NULL == (diskfile = fopen(netoutfile,"r"))) {
      ffpmsg("Unable to reopen disk file (ftp_compress_open)");
      ffpmsg(netoutfile);
      return (FILE_NOT_OPENED);
    }
    closefdiskfile++;
  
    if ((status =  mem_create(url,handle))) {
      ffpmsg("Unable to create memory file (ftp_compress_open)");
      ffpmsg(url);
      goto error;
    }
    closememfile++;

    status = 0;
    status = mem_uncompress2mem(url,diskfile,*handle);
    fclose(diskfile);
    closefdiskfile--;

    if (status) {
      ffpmsg("Error writing compressed memory file (ftp_compress_open)");
      goto error;
    }
      
  } else {
    /* Opps, this should not have happened */
    ffpmsg("Can only compressed files here (ftp_compress_open)");
    goto error;
  }    
    

  signal(SIGALRM, SIG_DFL);
  alarm(0);
  return mem_seek(*handle,0);

 error:
  alarm(0); /* clear it */
  if (closeftpfile) {
    fclose(ftpfile);
  }
  if (closecommandfile) {
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  }
  if (closefdiskfile) {
    fclose(diskfile);
  }
  if (closememfile) {
    mem_close_free(*handle);
  }
  if (closediskfile) {
    file_close(*handle);
  } 
  
  signal(SIGALRM, SIG_DFL);
  return (FILE_NOT_OPENED);
}

/*--------------------------------------------------------------------------*/
/* Open a ftp connection to filename (really a URL), return ftpfile set to 
   the file connection, and command set to the control connection, with sock
   also set to the control connection */

static int ftp_open_network(char *filename, FILE **ftpfile, FILE **command, int *sock)
{
  int status;
  int sock1;
  int tmpint;
  char recbuf[MAXLEN];
  char errorstr[MAXLEN];
  char tmpstr[MAXLEN];
  char proto[SHORTLEN];
  char host[SHORTLEN];
  char *newhost;
  char *username;
  char *password;
  char fn[MAXLEN];
  char *newfn;
  char *passive;
  char *tstr;
  char *saveptr;
  char ip[SHORTLEN];
  char turl[MAXLEN];
  int port;
  int ii,tryingtologin = 1;

  /* parse the URL */
  if (strlen(filename) > MAXLEN - 7) {
    ffpmsg("ftp filename is too long (ftp_open_network)");
    return (FILE_NOT_OPENED);
  }

  strcpy(turl,"ftp://");
  strcat(turl,filename);
  if (NET_ParseUrl(turl,proto,host,&port,fn)) {
    sprintf(errorstr,"URL Parse Error (ftp_open) %s",filename);
    ffpmsg(errorstr);
    return (FILE_NOT_OPENED);
  }
  
  port = 21;
  /* we might have a user name */
  username = "anonymous";
  password = "user@host.com";
  /* is there an @ sign */
  if (NULL != (newhost = strrchr(host,'@'))) {
    *newhost = '\0'; /* make it a null, */
    newhost++; /* Now newhost points to the host name and host points to the 
		  user name, password combo */
    username = host;
    /* is there a : for a password */
    if (NULL != strchr(username,':')) {
      password = strchr(username,':');
      *password = '\0';
      password++;
    }
  } else {
    newhost = host;
  }

  for (ii = 0; ii < 10; ii++) {  /* make up to 10 attempts to log in */
  
    /* Connect to the host on the required port */
    *sock = NET_TcpConnect(newhost,port);
    /* convert it to a stdio file */
    if (NULL == (*command = fdopen(*sock,"r"))) {
      ffpmsg ("fdopen failed to convert socket to stdio file (ftp_open_netowrk)");
      return (FILE_NOT_OPENED);
    }

    /* Wait for the 220 response */
    if (ftp_status(*command,"220 ")) {
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);

/*      ffpmsg("sleeping for 5 in ftp_open_network, then try again"); */

      sleep (5);  /* take a nap and hope ftp server sorts itself out in the meantime */

    } else {
      tryingtologin = 0;
      break;
    }
  }

  if (tryingtologin) { /* the 10 attempts were not successful */
     ffpmsg ("error connecting to remote server, no 220 seen (ftp_open_network)");
     return (FILE_NOT_OPENED);
  }

  /* Send the user name and wait for the right response */
  sprintf(tmpstr,"USER %s\r\n",username);

  status = NET_SendRaw(*sock,tmpstr,strlen(tmpstr),NET_DEFAULT);

  if (ftp_status(*command,"331 ")) {
    ffpmsg ("USER error no 331 seen (ftp_open_network)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }
  
  /* Send the password and wait for the right response */
  sprintf(tmpstr,"PASS %s\r\n",password);
  status = NET_SendRaw(*sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(*command,"230 ")) {
    ffpmsg ("PASS error, no 230 seen (ftp_open_network)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }

  /* now do the cwd command */
  newfn = strrchr(fn,'/');
  if (newfn == NULL) {
    strcpy(tmpstr,"CWD /\r\n");
    newfn = fn;
  } else {
    *newfn = '\0';
    newfn++;
    if (strlen(fn) == 0) {
      strcpy(tmpstr,"CWD /\r\n");
    } else {
      /* remove the leading slash */
      if (fn[0] == '/') {
	sprintf(tmpstr,"CWD %s\r\n",&fn[1]);
      } else {
	sprintf(tmpstr,"CWD %s\r\n",fn);
      } 
    }
  }
  
  status = NET_SendRaw(*sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(*command,"250 ")) {
    ffpmsg ("CWD error, no 250 seen (ftp_open_network)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }
  
  if (!strlen(newfn)) {
    ffpmsg("Null file name (ftp_open)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }

  /* Always use binary mode */
  sprintf(tmpstr,"TYPE I\r\n");
  status = NET_SendRaw(*sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(*command,"200 ")) {
    ffpmsg ("TYPE I error, 200 not seen (ftp_open_network)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }
 
  status = NET_SendRaw(*sock,"PASV\r\n",6,NET_DEFAULT);

  if (!(fgets(recbuf,MAXLEN,*command))) {
    ffpmsg ("PASV error (ftp_open)");
    fclose(*command);
    NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
    return (FILE_NOT_OPENED);
  }
  
  /*  Passive mode response looks like
      227 Entering Passive Mode (129,194,67,8,210,80) */
  if (recbuf[0] == '2' && recbuf[1] == '2' && recbuf[2] == '7') {
    /* got a good passive mode response, find the opening ( */
    
    if (!(passive = strchr(recbuf,'('))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    
    *passive = '\0';
    passive++;
    ip[0] = '\0';
      
    /* Messy parsing of response from PASV *command */
    
    if (!(tstr = ffstrtok(passive,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    strcpy(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    strcat(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    strcat(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    strcat(ip,tstr);
    
    /* Done the ip number, now do the port # */
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    sscanf(tstr,"%d",&port);
    port *= 256;
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    sscanf(tstr,"%d",&tmpint);
    port += tmpint;

    if (!strlen(newfn)) {
      ffpmsg("Null file name (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    
    /* Connect to the data port */
    sock1 = NET_TcpConnect(ip,port);
    if (NULL == (*ftpfile = fdopen(sock1,"r"))) {
      ffpmsg ("Could not connect to passive port (ftp_open_network)");
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }

    /* Send the retrieve command */
    sprintf(tmpstr,"RETR %s\r\n",newfn);
    status = NET_SendRaw(*sock,tmpstr,strlen(tmpstr),NET_DEFAULT);

    if (ftp_status(*command,"150 ")) {
      fclose(*ftpfile);
      NET_SendRaw(sock1,"QUIT\r\n",6,NET_DEFAULT);
      fclose(*command);
      NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
      return (FILE_NOT_OPENED);
    }
    return 0;    /* successfully opened the ftp file */
  }
  
  /* no passive mode */

  fclose(*command);
  NET_SendRaw(*sock,"QUIT\r\n",6,NET_DEFAULT);
  return (FILE_NOT_OPENED);
}
/*--------------------------------------------------------------------------*/
/* Open a ftp connection to see if the file exists (return 1) or not (return 0) */

int ftp_file_exist(char *filename)
{
  FILE *ftpfile;
  FILE *command;
  int sock;
  int status;
  int sock1;
  int tmpint;
  char recbuf[MAXLEN];
  char errorstr[MAXLEN];
  char tmpstr[MAXLEN];
  char proto[SHORTLEN];
  char host[SHORTLEN];
  char *newhost;
  char *username;
  char *password;
  char fn[MAXLEN];
  char *newfn;
  char *passive;
  char *tstr;
  char *saveptr;
  char ip[SHORTLEN];
  char turl[MAXLEN];
  int port;
  int ii, tryingtologin = 1;

  /* parse the URL */
  if (strlen(filename) > MAXLEN - 7) {
    ffpmsg("ftp filename is too long (ftp_file_exist)");
    return 0;
  }

  strcpy(turl,"ftp://");
  strcat(turl,filename);
  if (NET_ParseUrl(turl,proto,host,&port,fn)) {
    sprintf(errorstr,"URL Parse Error (ftp_file_exist) %s",filename);
    ffpmsg(errorstr);
    return 0;
  }

  port = 21;
  /* we might have a user name */
  username = "anonymous";
  password = "user@host.com";
  /* is there an @ sign */
  if (NULL != (newhost = strrchr(host,'@'))) {
    *newhost = '\0'; /* make it a null, */
    newhost++; /* Now newhost points to the host name and host points to the 
		  user name, password combo */
    username = host;
    /* is there a : for a password */
    if (NULL != strchr(username,':')) {
      password = strchr(username,':');
      *password = '\0';
      password++;
    }
  } else {
    newhost = host;
  }

  for (ii = 0; ii < 10; ii++) {  /* make up to 10 attempts to log in */
  
  /* Connect to the host on the required port */
  sock = NET_TcpConnect(newhost,port);
  /* convert it to a stdio file */
  if (NULL == (command = fdopen(sock,"r"))) {
    ffpmsg ("Failed to convert socket to stdio file (ftp_file_exist)");
    return 0;
  }

  /* Wait for the 220 response */
  if (ftp_status(command,"220 ")) {
    ffpmsg ("error connecting to remote server, no 220 seen (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);

/*    ffpmsg("sleeping for 5 in ftp_file_exist, then try again"); */

    sleep (5);  /* take a nap and hope ftp server sorts itself out in the meantime */

  } else {
    tryingtologin = 0;
    break;
  }
  
  }  

  if (tryingtologin) { /* the 10 attempts were not successful */
     ffpmsg ("error connecting to remote server, no 220 seen (ftp_open_network)");
     return (0);
  }
 
  /* Send the user name and wait for the right response */
  sprintf(tmpstr,"USER %s\r\n",username);

  status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);

  if (ftp_status(command,"331 ")) {
    ffpmsg ("USER error no 331 seen (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }
  
  /* Send the password and wait for the right response */
  sprintf(tmpstr,"PASS %s\r\n",password);
  status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(command,"230 ")) {
    ffpmsg ("PASS error, no 230 seen (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }

  /* now do the cwd command */
  newfn = strrchr(fn,'/');
  if (newfn == NULL) {
    strcpy(tmpstr,"CWD /\r\n");
    newfn = fn;
  } else {
    *newfn = '\0';
    newfn++;
    if (strlen(fn) == 0) {
      strcpy(tmpstr,"CWD /\r\n");
    } else {
      /* remove the leading slash */
      if (fn[0] == '/') {
	sprintf(tmpstr,"CWD %s\r\n",&fn[1]);
      } else {
	sprintf(tmpstr,"CWD %s\r\n",fn);
      } 
    }
  }

  status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(command,"250 ")) {
    ffpmsg ("CWD error, no 250 seen (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }
  
  if (!strlen(newfn)) {
    ffpmsg("Null file name (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }

  /* Always use binary mode */
  sprintf(tmpstr,"TYPE I\r\n");
  status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);
  
  if (ftp_status(command,"200 ")) {
    ffpmsg ("TYPE I error, 200 not seen (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }

  status = NET_SendRaw(sock,"PASV\r\n",6,NET_DEFAULT);

  if (!(fgets(recbuf,MAXLEN,command))) {
    ffpmsg ("PASV error (ftp_file_exist)");
    fclose(command);
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 0;
  }
  
  /*  Passive mode response looks like
      227 Entering Passive Mode (129,194,67,8,210,80) */
  if (recbuf[0] == '2' && recbuf[1] == '2' && recbuf[2] == '7') {
    /* got a good passive mode response, find the opening ( */
    
    if (!(passive = strchr(recbuf,'('))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    
    *passive = '\0';
    passive++;
    ip[0] = '\0';
      
    /* Messy parsing of response from PASV command */
    
    if (!(tstr = ffstrtok(passive,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    strcpy(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    strcat(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    strcat(ip,tstr);
    strcat(ip,".");
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    strcat(ip,tstr);
    
    /* Done the ip number, now do the port # */
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    sscanf(tstr,"%d",&port);
    port *= 256;
    
    if (!(tstr = ffstrtok(NULL,",)",&saveptr))) {
      ffpmsg ("PASV error (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    sscanf(tstr,"%d",&tmpint);
    port += tmpint;

    if (!strlen(newfn)) {
      ffpmsg("Null file name (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }

    /* Connect to the data port */
    sock1 = NET_TcpConnect(ip,port);
    if (NULL == (ftpfile = fdopen(sock1,"r"))) {
      ffpmsg ("Could not connect to passive port (ftp_file_exist)");
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }

    /* Send the retrieve command */
    sprintf(tmpstr,"RETR %s\r\n",newfn);
    status = NET_SendRaw(sock,tmpstr,strlen(tmpstr),NET_DEFAULT);

    if (ftp_status(command,"150 ")) {
      fclose(ftpfile); 
      NET_SendRaw(sock1,"QUIT\r\n",6,NET_DEFAULT);
      fclose(command);
      NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
      return 0;
    }
    
    /* if we got here then the file probably exists */

    fclose(ftpfile); 
    NET_SendRaw(sock1,"QUIT\r\n",6,NET_DEFAULT);
    fclose(command); 
    NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
    return 1;
  }
  
  /* no passive mode */

  fclose(command);
  NET_SendRaw(sock,"QUIT\r\n",6,NET_DEFAULT);
  return 0;
}

/*--------------------------------------------------------------------------*/
/* return a socket which results from connection to hostname on port port */
int NET_TcpConnect(char *hostname, int port)
{
  /* Connect to hostname on port */
 
   struct sockaddr_in sockaddr;
   int sock;
   int stat;
   int val = 1;
 
   CreateSocketAddress(&sockaddr,hostname,port);
   /* Create socket */
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
     ffpmsg("ERROR: NET_TcpConnect can't create socket");
     return CONNECTION_ERROR;
   }
 
   if ((stat = connect(sock, (struct sockaddr*) &sockaddr, 
		       sizeof(sockaddr))) 
       < 0) {
     close(sock);
/*
     perror("NET_Tcpconnect - Connection error");
     ffpmsg("Can't connect to host, connection error");
*/
     return CONNECTION_ERROR;
   }
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
   setsockopt(sock, SOL_SOCKET,  SO_KEEPALIVE, (char *)&val, sizeof(val));

   val = 65536;
   setsockopt(sock, SOL_SOCKET,  SO_SNDBUF,    (char *)&val, sizeof(val));
   setsockopt(sock, SOL_SOCKET,  SO_RCVBUF,    (char *)&val, sizeof(val));
   return sock;
}

/*--------------------------------------------------------------------------*/
/* Write len bytes from buffer to socket sock */
static int NET_SendRaw(int sock, const void *buffer, int length, int opt)
{

  char * buf = (char *) buffer;
 
   int flag;
   int n, nsent = 0;
 
   switch (opt) {
   case NET_DEFAULT:
     flag = 0;
     break;
   case NET_OOB:
     flag = MSG_OOB;
     break;
   case NET_PEEK:            
   default:
     flag = 0;
     break;
   }
 
   if (sock < 0) return -1;
   
   for (n = 0; n < length; n += nsent) {
     if ((nsent = send(sock, buf+n, length-n, flag)) <= 0) {
       return nsent;
     }
   }

   return n;
}

/*--------------------------------------------------------------------------*/

static int NET_RecvRaw(int sock, void *buffer, int length)
{
  /* Receive exactly length bytes into buffer. Returns number of bytes */
  /* received. Returns -1 in case of error. */


   int nrecv, n;
   char *buf = (char *)buffer;

   if (sock < 0) return -1;
   for (n = 0; n < length; n += nrecv) {
      while ((nrecv = recv(sock, buf+n, length-n, 0)) == -1 && errno == EINTR)
	errno = 0;     /* probably a SIGCLD that was caught */
      if (nrecv < 0)
         return nrecv;
      else if (nrecv == 0)
	break;        /*/ EOF */
   }

   return n;
}
 
/*--------------------------------------------------------------------------*/
/* Yet Another URL Parser 
   url - input url
   proto - input protocol
   host - output host
   port - output port
   fn - output filename
*/

static int NET_ParseUrl(const char *url, char *proto, char *host, int *port, 
		 char *fn)
{
  /* parses urls into their bits */
  /* returns 1 if error, else 0 */

  char *urlcopy, *urlcopyorig;
  char *ptrstr;
  char *thost;
  int isftp = 0;

  /* figure out if there is a http: or  ftp: */

  urlcopyorig = urlcopy = (char *) malloc(strlen(url)+1);
  strcpy(urlcopy,url);

  /* set some defaults */
  *port = 80;
  strcpy(proto,"http:");
  strcpy(host,"localhost");
  strcpy(fn,"/");
  
  ptrstr = strstr(urlcopy,"http:");
  if (ptrstr == NULL) {
    /* Nope, not http: */
    ptrstr = strstr(urlcopy,"root:");
    if (ptrstr == NULL) {
      /* Nope, not root either */
      ptrstr = strstr(urlcopy,"ftp:");
      if (ptrstr != NULL) {
	if (ptrstr == urlcopy) {
	  strcpy(proto,"ftp:");
	  *port = 21;
	  isftp++;
	  urlcopy += 4; /* move past ftp: */
	} else {
	  /* not at the beginning, bad url */
	  free(urlcopyorig);
	  return 1;
	}
      }
    } else {
      if (ptrstr == urlcopy) {
	urlcopy += 5; /* move past root: */
      } else {
	/* not at the beginning, bad url */
	free(urlcopyorig);
	return 1;
      }
    }
  } else {
    if (ptrstr == urlcopy) {
      urlcopy += 5; /* move past http: */
    } else {
      free(urlcopyorig);
      return 1;
    }
  }

  /* got the protocol */
  /* get the hostname */
  if (urlcopy[0] == '/' && urlcopy[1] == '/') {
    /* we have a hostname */
    urlcopy += 2; /* move past the // */
  }
  /* do this only if http */
  if (!strcmp(proto,"http:")) {

    /* Move past any user:password */
    if ((thost = strchr(urlcopy, '@')) != NULL)
      urlcopy = thost+1;

    strcpy(host,urlcopy);
    thost = host;
    while (*urlcopy != '/' && *urlcopy != ':' && *urlcopy) {
      thost++;
      urlcopy++;
    }
    /* we should either be at the end of the string, have a /, or have a : */
    *thost = '\0';
    if (*urlcopy == ':') {
      /* follows a port number */
      urlcopy++;
      sscanf(urlcopy,"%d",port);
      while (*urlcopy != '/' && *urlcopy) urlcopy++; /* step to the */
    }
  } else {
    /* do this for ftp */
    strcpy(host,urlcopy);
    thost = host;
    while (*urlcopy != '/' && *urlcopy) {
      thost++;
      urlcopy++; 
    }
    *thost = '\0';
    /* Now, we should either be at the end of the string, or have a / */
    
  }
  /* Now the rest is a fn */

  if (*urlcopy) {
    strcpy(fn,urlcopy);
  }
  free(urlcopyorig);
  return 0;
}

/*--------------------------------------------------------------------------*/
int http_checkfile (char *urltype, char *infile, char *outfile1)
{

/* Small helper functions to set the netoutfile static string */
/* Called by cfileio after parsing the output file off of the input file url */

  char newinfile[MAXLEN];
  FILE *httpfile;
  char contentencoding[MAXLEN];
  int contentlength;
  int foundfile = 0;

  /* set defaults  */
  strcpy(urltype,"http://");

  if (strlen(outfile1)) {
    /* don't copy the "file://" prefix, if present.  */
    if (!strncmp(outfile1, "file://", 7) ) {
      strcpy(netoutfile,outfile1+7);
    } else {
      strcpy(netoutfile,outfile1);
    }
  }

  if (strstr(infile, "?")) {
      /* Special case where infile name contains a "?". */
      /* This is probably a CGI string; no point in testing if it exists */
      /*  so just set urltype and netoutfile if necessary, then return */
      
      if (strlen(outfile1)) {   /* was an outfile specified? */
          strcpy(urltype,"httpfile://");  

          /* don't copy the "file://" prefix, if present.  */
          if (!strncmp(outfile1, "file://", 7) ) {
             strcpy(netoutfile,outfile1+7);
          } else {
             strcpy(netoutfile,outfile1);
          }
      }
      return 0;  /* case where infile name contains "?" */
  }

  /*
     If the specified infile file name does not contain a .gz or .Z suffix,
     then first test if a .gz compressed version of the file exists, and if not
     then test if a .Z version of the file exists. (because it will be much
     faster to read the compressed file).  If the compressed files do not exist,
     then finally just open the infile name exactly as specified.
  */

  if (!strstr(infile,".gz") && (!strstr(infile,".Z"))) {
    /* The infile string does not contain the name of a compressed file.  */
    /* Fisrt, look for a .gz compressed version of the file. */
      
    strcpy(newinfile,infile);
    strcat(newinfile,".gz");

    if (!http_open_network(newinfile,&httpfile,contentencoding,
			   &contentlength)) {
      if (!strcmp(contentencoding, "ftp://")) {
          /* this is a signal from http_open_network that indicates that */
          /* the http server returned a 301 or 302 redirect to a FTP URL. */
          /* Check that the file exists, because redirect many not be reliable */
	   
          if (ftp_file_exist(newinfile)) { 
              /* The ftp .gz compressed file is there, all is good!  */
              strcpy(urltype, "ftp://");
              strcpy(infile,newinfile);

              if (strlen(outfile1)) {
                /* there is an output file;  might need to modify the urltype */

                if (!strncmp(outfile1, "mem:", 4) )  {
                     /* copy the file to memory, with READ and WRITE access 
                     In this case, it makes no difference whether the ftp file
                     and or the output file are compressed or not.   */

                     strcpy(urltype, "ftpmem://");  /* use special driver */
                } else {
        	    /* input file is compressed */
		    if (strstr(outfile1,".gz") || (strstr(outfile1,".Z"))) {
		      strcpy(urltype,"ftpcompress://");
		    } else {
		      strcpy(urltype,"ftpfile://");
		    }
                } 
              }

              return 0;   /* found the .gz compressed ftp file */
	    }
            /* fall through to here if ftp redirect does not exist */
      } else if (!strcmp(contentencoding, "https://")) {
          /* the http server returned a 301 or 302 redirect to an HTTPS URL. */
          https_checkfile(urltype, infile, outfile1);
          /* For https we're not testing for compressed extensions at 
             this stage.  It will all be done in https_open_network.  Therefore
             leave infile alone and do immediate return. */
          return 0;
      } else {
          /* found the http .gz compressed file */
          fclose(httpfile);
          foundfile = 1;
          strcpy(infile,newinfile);
      }
    }

   if (!foundfile) {
    /* did not find .gz compressed version of the file, so look for .Z file. */
      
    strcpy(newinfile,infile);
    strcat(newinfile,".Z");
    if (!http_open_network(newinfile,&httpfile,contentencoding,
			   &contentlength)) {

      if (!strcmp(contentencoding, "ftp://")) {
          /* this is a signal from http_open_network that indicates that */
          /* the http server returned a 301 or 302 redirect to a FTP URL. */
          /* Check that the file exists, because redirect many not be reliable */
	   
          if (ftp_file_exist(newinfile)) { 
              /* The ftp .Z compressed file is there, all is good!  */
              strcpy(urltype, "ftp://");
              strcpy(infile,newinfile);

              if (strlen(outfile1)) {
                /* there is an output file;  might need to modify the urltype */

                if (!strncmp(outfile1, "mem:", 4) )  {
                     /* copy the file to memory, with READ and WRITE access 
                     In this case, it makes no difference whether the ftp file
                     and or the output file are compressed or not.   */

                     strcpy(urltype, "ftpmem://");  /* use special driver */
                } else {
        	    /* input file is compressed */
		    if (strstr(outfile1,".gz") || (strstr(outfile1,".Z"))) {
		      strcpy(urltype,"ftpcompress://");
		    } else {
		      strcpy(urltype,"ftpfile://");
		    }
                } 
            }
            return 0;   /* found the .gz compressed ftp file */
          }
          /* fall through to here if ftp redirect does not exist */
        }  else {
           /* found the http .Z compressed file */
           fclose(httpfile);
           foundfile = 1;
           strcpy(infile,newinfile);
        }
      }
    }
  }  /* end of case where infile does not contain .gz or .Z */

  if (!foundfile) {
    /* look for the base file.name */
      
    strcpy(newinfile,infile);
    if (!http_open_network(newinfile,&httpfile,contentencoding,
			   &contentlength)) {

      if (!strcmp(contentencoding, "ftp://")) {
          /* this is a signal from http_open_network that indicates that */
          /* the http server returned a 301 or 302 redirect to a FTP URL. */
          /* Check that the file exists, because redirect many not be reliable */
	   
          if (ftp_file_exist(newinfile)) { 
              /* The ftp file is there, all is good!  */
              strcpy(urltype, "ftp://");
              strcpy(infile,newinfile);

              if (strlen(outfile1)) {
                /* there is an output file;  might need to modify the urltype */

                if (!strncmp(outfile1, "mem:", 4) )  {
                     /* copy the file to memory, with READ and WRITE access 
                     In this case, it makes no difference whether the ftp file
                     and or the output file are compressed or not.   */

                     strcpy(urltype, "ftpmem://");  /* use special driver */
                     return 0;
                } else {

        	  /* input file is not compressed */
		   strcpy(urltype,"ftpfile://");
                } 
              } 
              return 0;   /* found the ftp file */
            }
            /* fall through to here if ftp redirect does not exist */
      } else if (!strcmp(contentencoding, "https://")) {
          /* the http server returned a 301 or 302 redirect to an HTTPS URL. */
          https_checkfile(urltype, infile, outfile1);
          /* For https we're not testing for compressed extensions at 
             this stage.  It will all be done in https_open_network.  Therefore
             leave infile alone and do immediate return. */
          return 0;
      }  else {
          /* found the http .Z compressed file */
          fclose(httpfile);
          foundfile = 1;
          strcpy(infile,newinfile);
      }

    }
  }

  if (!foundfile) {
     return (FILE_NOT_OPENED);
  }

  if (strlen(outfile1)) {
    /* there is an output file */

    if (!strncmp(outfile1, "mem:", 4) )  {
       /* copy the file to memory, with READ and WRITE access 
          In this case, it makes no difference whether the http file
          and or the output file are compressed or not.   */

       strcpy(urltype, "httpmem://");  /* use special driver */
       return 0;
    }

    if (strstr(infile, "?")) {
      /* file name contains a '?' so probably a cgi string;  */
      strcpy(urltype,"httpfile://");
      return 0;
    }

    if (strstr(infile,".gz") || (strstr(infile,".Z"))) {
	/* It's compressed */
	if (strstr(outfile1,".gz") || (strstr(outfile1,".Z"))) {
	  strcpy(urltype,"httpcompress://");
	} else {
	  strcpy(urltype,"httpfile://");
	}
    } else {
	strcpy(urltype,"httpfile://");
    }
  } 
  return 0;
}

/*--------------------------------------------------------------------------*/
int https_checkfile (char *urltype, char *infile, char *outfile1)
{
  /* set default  */
  strcpy(urltype,"https://");
  
  if (strlen(outfile1))
  {
    /* don't copy the "file://" prefix, if present.  */
    if (!strncmp(outfile1, "file://", 7) ) {
      strcpy(netoutfile,outfile1+7);
    } else {
      strcpy(netoutfile,outfile1);
    }
    
    if (!strncmp(outfile1, "mem:", 4))
       strcpy(urltype,"httpsmem://");
    else       
       strcpy(urltype,"httpsfile://");
  }

   return 0;
}

/*--------------------------------------------------------------------------*/
int ftp_checkfile (char *urltype, char *infile, char *outfile1)
{
  char newinfile[MAXLEN];
  FILE *ftpfile;
  FILE *command;
  int sock;
  int foundfile = 0;

 /* Small helper functions to set the netoutfile static string */

  /* default to ftp://  if no outfile specified */
  strcpy(urltype,"ftp://"); 

 if (!strstr(infile,".gz") && (!strstr(infile,".Z"))) {
    /* The infile string does not contain the name of a compressed file.  */
    /* Fisrt, look for a .gz compressed version of the file. */
      
    strcpy(newinfile,infile);
    strcat(newinfile,".gz");
 
    /* look for .gz version of the file */
    if (ftp_file_exist(newinfile)) {
      foundfile = 1;
      strcpy(infile,newinfile);
    }

    if (!foundfile) {
      strcpy(newinfile,infile);
      strcat(newinfile,".Z");
 
    /* look for .Z version of the file */
      if (ftp_file_exist(newinfile)) {
        foundfile = 1;
        strcpy(infile,newinfile);
      }
    }
  }

  if (!foundfile) {
      strcpy(newinfile,infile);
 
      /* look for the base file */
      if (ftp_file_exist(newinfile)) {
        foundfile = 1;
        strcpy(infile,newinfile);
      }
  }

  if (!foundfile) {
     return (FILE_NOT_OPENED);
  }

  if (strlen(outfile1)) {
    /* there is an output file;  might need to modify the urltype */

    /* don't copy the "file://" prefix, if present.  */
    if (!strncmp(outfile1, "file://", 7) )
       strcpy(netoutfile,outfile1+7);
    else
       strcpy(netoutfile,outfile1);

    if (!strncmp(outfile1, "mem:", 4) )  {
       /* copy the file to memory, with READ and WRITE access 
          In this case, it makes no difference whether the ftp file
          and or the output file are compressed or not.   */

       strcpy(urltype, "ftpmem://");  /* use special driver */
       return 0;
    }
 
    if (strstr(infile,".gz") || (strstr(infile,".Z"))) {
	/* input file is compressed */
	if (strstr(outfile1,".gz") || (strstr(outfile1,".Z"))) {
	  strcpy(urltype,"ftpcompress://");
	} else {
	  strcpy(urltype,"ftpfile://");
	}
    } else {
	strcpy(urltype,"ftpfile://");
    } 
  } 
  return 0;
}
/*--------------------------------------------------------------------------*/
/* A small helper function to wait for a particular status on the ftp 
   connectino */
static int ftp_status(FILE *ftp, char *statusstr)
{
  /* read through until we find a string beginning with statusstr */
  /* This needs a timeout */

  char recbuf[MAXLEN], errorstr[SHORTLEN];
  int len;

  len = strlen(statusstr);
  while (1) {

    if (!(fgets(recbuf,MAXLEN,ftp))) {
      sprintf(errorstr,"ERROR: ftp_status wants %s but fgets returned 0",statusstr);
      ffpmsg(errorstr);
      return 1; /* error reading */
    }

    recbuf[len] = '\0'; /* make it short */
    if (!strcmp(recbuf,statusstr)) {
      return 0; /* we're ok */
    }
    if (recbuf[0] > '3') {
      /* oh well, some sort of error */
      sprintf(errorstr,"ERROR ftp_status wants %s but got %s", statusstr, recbuf);
      ffpmsg(errorstr);
     return 1; 
    }
    sprintf(errorstr,"ERROR ftp_status wants %s but got unexpected %s", statusstr, recbuf);
    ffpmsg(errorstr);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateSocketAddress --
 *
 *	This function initializes a sockaddr structure for a host and port.
 *
 * Results:
 *	1 if the host was valid, 0 if the host could not be converted to
 *	an IP address.
 *
 * Side effects:
 *	Fills in the *sockaddrPtr structure.
 *
 *----------------------------------------------------------------------
 */

static int
CreateSocketAddress(
    struct sockaddr_in *sockaddrPtr,	/* Socket address */
    char *host,				/* Host.  NULL implies INADDR_ANY */
    int port)				/* Port number */
{
    struct hostent *hostent;		/* Host database entry */
    struct in_addr addr;		/* For 64/32 bit madness */
    char localhost[MAXLEN];

    strcpy(localhost,host);

    memset((void *) sockaddrPtr, '\0', sizeof(struct sockaddr_in));
    sockaddrPtr->sin_family = AF_INET;
    sockaddrPtr->sin_port = htons((unsigned short) (port & 0xFFFF));
    if (host == NULL) {
	addr.s_addr = INADDR_ANY;
    } else {
        addr.s_addr = inet_addr(localhost);
        if (addr.s_addr == 0xFFFFFFFF) {
            hostent = gethostbyname(localhost);
            if (hostent != NULL) {
                memcpy((void *) &addr,
                        (void *) hostent->h_addr_list[0],
                        (size_t) hostent->h_length);
            } else {
#ifdef	EHOSTUNREACH
                errno = EHOSTUNREACH;
#else
#ifdef ENXIO
                errno = ENXIO;
#endif
#endif
                return 0;	/* error */
            }
        }
    }
        
    /*
     * NOTE: On 64 bit machines the assignment below is rumored to not
     * do the right thing. Please report errors related to this if you
     * observe incorrect behavior on 64 bit machines such as DEC Alphas.
     * Should we modify this code to do an explicit memcpy?
     */

    sockaddrPtr->sin_addr.s_addr = addr.s_addr;
    return 1;	/* Success. */
}

/* Signal handler for timeouts */

static void signal_handler(int sig) {

  switch (sig) {
  case SIGALRM:    /* process for alarm */
    longjmp(env,sig);
    
  default: {
      /* Hmm, shouldn't have happend */
      exit(sig);
    }
  }
}

/**************************************************************/

/* Root driver */

/*--------------------------------------------------------------------------*/
int root_init(void)
{
    int ii;

    for (ii = 0; ii < NMAXFILES; ii++) /* initialize all empty slots in table */
    {
       handleTable[ii].sock = 0;
       handleTable[ii].currentpos = 0;
    }
    return(0);
}
/*--------------------------------------------------------------------------*/
int root_setoptions(int options)
{
  /* do something with the options argument, to stop compiler warning */
  options = 0;
  return(options);
}
/*--------------------------------------------------------------------------*/
int root_getoptions(int *options)
{
  *options = 0;
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_getversion(int *version)
{
    *version = 10;
    return(0);
}
/*--------------------------------------------------------------------------*/
int root_shutdown(void)
{
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_open(char *url, int rwmode, int *handle)
{
    int ii, status;
    int sock;

    *handle = -1;
    for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in table */
    {
        if (handleTable[ii].sock == 0)
        {
            *handle = ii;
            break;
        }
    }

    if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

    /*open the file */
    if (rwmode) {
      status = root_openfile(url, "update", &sock);
    } else {
      status = root_openfile(url, "read", &sock);
    }
    if (status)
      return(status);
    
    handleTable[ii].sock = sock;
    handleTable[ii].currentpos = 0;
    
    return(0);
}
/*--------------------------------------------------------------------------*/
int root_create(char *filename, int *handle)
{
    int ii, status;
    int sock;

    *handle = -1;
    for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in table */
    {
        if (handleTable[ii].sock == 0)
        {
            *handle = ii;
            break;
        }
    }

    if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

    /*open the file */
    status = root_openfile(filename, "create", &sock);

    if (status) {
      ffpmsg("Unable to create file");
      return(status);
    }
    
    handleTable[ii].sock = sock;
    handleTable[ii].currentpos = 0;
    
    return(0);
}
/*--------------------------------------------------------------------------*/
int root_size(int handle, LONGLONG *filesize)
/*
  return the size of the file in bytes
*/
{

  int sock;
  int offset;
  int status;
  int op;

  sock = handleTable[handle].sock;

  status = root_send_buffer(sock,ROOTD_STAT,NULL,0);
  status = root_recv_buffer(sock,&op,(char *)&offset, 4);
  *filesize = (LONGLONG) ntohl(offset);
  
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_close(int handle)
/*
  close the file
*/
{

  int status;
  int sock;

  sock = handleTable[handle].sock;
  status = root_send_buffer(sock,ROOTD_CLOSE,NULL,0);
  close(sock);
  handleTable[handle].sock = 0;
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_flush(int handle)
/*
  flush the file
*/
{
  int status;
  int sock;

  sock = handleTable[handle].sock;
  status = root_send_buffer(sock,ROOTD_FLUSH,NULL,0);
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_seek(int handle, LONGLONG offset)
/*
  seek to position relative to start of the file
*/
{
  handleTable[handle].currentpos = offset;
  return(0);
}
/*--------------------------------------------------------------------------*/
int root_read(int hdl, void *buffer, long nbytes)
/*
  read bytes from the current position in the file
*/
{
  char msg[SHORTLEN];
  int op;
  int status;
  int astat;

  /* we presume here that the file position will never be > 2**31 = 2.1GB */
  sprintf(msg,"%ld %ld ",(long) handleTable[hdl].currentpos,nbytes);
  status = root_send_buffer(handleTable[hdl].sock,ROOTD_GET,msg,strlen(msg));
  if ((unsigned) status != strlen(msg)) {
    return (READ_ERROR);
  }
  astat = 0;
  status = root_recv_buffer(handleTable[hdl].sock,&op,(char *) &astat,4);
  if (astat != 0) {
    return (READ_ERROR);
  }

  status = NET_RecvRaw(handleTable[hdl].sock,buffer,nbytes);
  if (status != nbytes) {
    return (READ_ERROR);
  }
  handleTable[hdl].currentpos += nbytes;

  return(0);
}
/*--------------------------------------------------------------------------*/
int root_write(int hdl, void *buffer, long nbytes)
/*
  write bytes at the current position in the file
*/
{

  char msg[SHORTLEN];
  int len;
  int sock;
  int status;
  int astat;
  int op;

  sock = handleTable[hdl].sock;
  /* we presume here that the file position will never be > 2**31 = 2.1GB */
  sprintf(msg,"%ld %ld ",(long) handleTable[hdl].currentpos,nbytes);

  len = strlen(msg);
  status = root_send_buffer(sock,ROOTD_PUT,msg,len+1);
  if (status != len+1) {
    return (WRITE_ERROR);
  }
  status = NET_SendRaw(sock,buffer,nbytes,NET_DEFAULT);
  if (status != nbytes) {
    return (WRITE_ERROR);
  }
  astat = 0;
  status = root_recv_buffer(handleTable[hdl].sock,&op,(char *) &astat,4);

  if (astat != 0) {
    return (WRITE_ERROR);
  }
  handleTable[hdl].currentpos += nbytes;
  return(0);
}

/*--------------------------------------------------------------------------*/
int root_openfile(char *url, char *rwmode, int *sock)
     /*
       lowest level routine to physically open a root file
     */
{
  
  int status;
  char recbuf[MAXLEN];
  char errorstr[MAXLEN];
  char proto[SHORTLEN];
  char host[SHORTLEN];
  char fn[MAXLEN];
  char turl[MAXLEN];
  int port;
  int op;
  int ii;
  int authstat;
  
  
  /* Parse the URL apart again */
  strcpy(turl,"root://");
  strcat(turl,url);
  if (NET_ParseUrl(turl,proto,host,&port,fn)) {
    sprintf(errorstr,"URL Parse Error (root_open) %s",url);
    ffpmsg(errorstr);
    return (FILE_NOT_OPENED);
  }
  
  /* Connect to the remote host */
  *sock = NET_TcpConnect(host,port);
  if (*sock < 0) {
    ffpmsg("Couldn't connect to host (root_openfile)");
    return (FILE_NOT_OPENED);
  }
  
  /* get the username */
  if (NULL != getenv("ROOTUSERNAME")) {
    strcpy(recbuf,getenv("ROOTUSERNAME"));
  } else {
    printf("Username: ");
    fgets(recbuf,MAXLEN,stdin);
    recbuf[strlen(recbuf)-1] = '\0';
  }
  
  status = root_send_buffer(*sock, ROOTD_USER, recbuf,strlen(recbuf));
  if (status < 0) {
    ffpmsg("error talking to remote system on username ");
    return (FILE_NOT_OPENED);
  }
  
  status = root_recv_buffer(*sock,&op,(char *)&authstat,4);
  if (!status) {
    ffpmsg("error talking to remote system on username");
    return (FILE_NOT_OPENED);
  }
  
  if (op != ROOTD_AUTH) {
    ffpmsg("ERROR on ROOTD_USER");
    ffpmsg(recbuf);
    return (FILE_NOT_OPENED);
  }
  

  /* now the password */
  if (NULL != getenv("ROOTPASSWORD")) {
    strcpy(recbuf,getenv("ROOTPASSWORD"));
  } else {
    printf("Password: ");
    fgets(recbuf,MAXLEN,stdin);
    recbuf[strlen(recbuf)-1] = '\0';
  }
  /* ones complement the password */
  for (ii=0;(unsigned) ii<strlen(recbuf);ii++) {
    recbuf[ii] = ~recbuf[ii];
  }
  
  status = root_send_buffer(*sock, ROOTD_PASS, recbuf, strlen(recbuf));
  if (status < 0) {
    ffpmsg("error talking to remote system sending password");
    return (FILE_NOT_OPENED);
  }
  
  status = root_recv_buffer(*sock,&op,(char *)&authstat,4);
  if (status < 0) {
    ffpmsg("error talking to remote system acking password");
    return (FILE_NOT_OPENED);
  }
  
  if (op != ROOTD_AUTH) {
    ffpmsg("ERROR on ROOTD_PASS");
    ffpmsg(recbuf);
    return (FILE_NOT_OPENED);
  }
  
  /* now the file open request */
  strcpy(recbuf,fn);
  strcat(recbuf," ");
  strcat(recbuf,rwmode);

  status = root_send_buffer(*sock, ROOTD_OPEN, recbuf, strlen(recbuf));
  if (status < 0) {
    ffpmsg("error talking to remote system on open ");
    return (FILE_NOT_OPENED);
  }

  status = root_recv_buffer(*sock,&op,(char *)&authstat,4);
  if (status < 0) {
    ffpmsg("error talking to remote system on open");
    return (FILE_NOT_OPENED);
  }
  
  if ((op != ROOTD_OPEN) && (authstat != 0)) {
    ffpmsg("ERROR on ROOTD_OPEN");
    ffpmsg(recbuf);
    return (FILE_NOT_OPENED);
  }

  return 0;

}

static int root_send_buffer(int sock, int op, char *buffer, int buflen)
{
  /* send a buffer, the form is
     <len>
     <op>
     <buffer>

     <len> includes the 4 bytes for the op, the length bytes (4) are implicit


     if buffer is null don't send it, not everything needs something sent */

  int len;
  int status;

  int hdr[2];

  len = 4;

  if (buffer != NULL) {
    len += buflen;
  }
  
  hdr[0] = htonl(len);
  hdr[1] = htonl(op);

  status = NET_SendRaw(sock,hdr,sizeof(hdr),NET_DEFAULT);
  if (status < 0) {
    return status;
  }
  if (buffer != NULL) {
    status = NET_SendRaw(sock,buffer,buflen,NET_DEFAULT);
  }
  return status;
}
  
static int root_recv_buffer(int sock, int *op, char *buffer, int buflen)
{
  /* recv a buffer, the form is
     <len>
     <op>
     <buffer>
  */

  int recv1 = 0;
  int len;
  int status;
  char recbuf[MAXLEN];

  status = NET_RecvRaw(sock,&len,4);

  if (status < 0) {
    return status;
  }
  recv1 += status;

  len = ntohl(len);

  /* ok, have the length, recive the operation */
  len -= 4;
  status = NET_RecvRaw(sock,op,4);
  if (status < 0) {
    return status;
  }

  recv1 += status;

  *op = ntohl(*op);
  
  if (len > MAXLEN) {
    len = MAXLEN;
  }

  if (len > 0) { /* Get the rest of the message */
    status = NET_RecvRaw(sock,recbuf,len);
    if (len > buflen) {
      len = buflen;
    }
    memcpy(buffer,recbuf,len);
    if (status < 0) {
      return status;
    }
  } 

  recv1 += status;
  return recv1;

}

/*****************************************************************************/
/*
  Encode a string into MIME Base64 format string
*/


static int encode64(unsigned s_len, char *src, unsigned d_len, char *dst) {

  static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789"
"+/";

  unsigned triad;


  for (triad = 0; triad < s_len; triad += 3) {
    unsigned long int sr;
    unsigned byte;

    for (byte = 0; (byte<3) && (triad+byte<s_len); ++byte) {
      sr <<= 8;
      sr |= (*(src+triad+byte) & 0xff);
    }

    /* shift left to next 6 bit alignment*/
    sr <<= (6-((8*byte)%6))%6;

    if (d_len < 4)
      return 1;

    *(dst+0) = *(dst+1) = *(dst+2) = *(dst+3) = '=';
    switch(byte) {
    case 3:
      *(dst+3) = base64[sr&0x3f];
      sr >>= 6;
    case 2:
      *(dst+2) = base64[sr&0x3f];
      sr >>= 6;
    case 1:
      *(dst+1) = base64[sr&0x3f];
      sr >>= 6;
      *(dst+0) = base64[sr&0x3f];
    }
    dst += 4;
    d_len -= 4;
  }

  *dst = '\0';
  return 0;
}

#endif
