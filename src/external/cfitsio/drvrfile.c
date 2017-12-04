/*  This file, drvrfile.c contains driver routines for disk files.         */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"
#include "group.h"  /* needed for fits_get_cwd in file_create */

#if defined(unix) || defined(__unix__)  || defined(__unix)
#include <pwd.h>         /* needed in file_openfile */

#ifdef REPLACE_LINKS
#include <sys/types.h>
#include <sys/stat.h>
#endif

#endif

#ifdef HAVE_FTRUNCATE
#if defined(unix) || defined(__unix__)  || defined(__unix) || defined(HAVE_UNISTD_H)
#include <unistd.h>  /* needed for getcwd prototype on unix machines */
#endif
#endif

#define IO_SEEK 0        /* last file I/O operation was a seek */
#define IO_READ 1        /* last file I/O operation was a read */
#define IO_WRITE 2       /* last file I/O operation was a write */

static char file_outfile[FLEN_FILENAME];

typedef struct    /* structure containing disk file structure */ 
{
    FILE *fileptr;
    LONGLONG currentpos;
    int last_io_op;
} diskdriver;

static diskdriver handleTable[NMAXFILES]; /* allocate diskfile handle tables */

/*--------------------------------------------------------------------------*/
int file_init(void)
{
    int ii;

    for (ii = 0; ii < NMAXFILES; ii++) /* initialize all empty slots in table */
    {
       handleTable[ii].fileptr = 0;
    }
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_setoptions(int options)
{
  /* do something with the options argument, to stop compiler warning */
  options = 0;
  return(options);
}
/*--------------------------------------------------------------------------*/
int file_getoptions(int *options)
{
  *options = 0;
  return(0);
}
/*--------------------------------------------------------------------------*/
int file_getversion(int *version)
{
    *version = 10;
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_shutdown(void)
{
  return(0);
}
/*--------------------------------------------------------------------------*/
int file_open(char *filename, int rwmode, int *handle)
{
    FILE *diskfile;
    int copyhandle, ii, status;
    char recbuf[2880];
    size_t nread;

    /*
       if an output filename has been specified as part of the input
       file, as in "inputfile.fits(outputfile.fit)" then we have to
       create the output file, copy the input to it, then reopen the
       the new copy.
    */

    if (*file_outfile)
    {
      /* open the original file, with readonly access */
      status = file_openfile(filename, READONLY, &diskfile);
      if (status) {
        file_outfile[0] = '\0';
        return(status);
      }
      
      /* create the output file */
      status =  file_create(file_outfile,handle);
      if (status)
      {
        ffpmsg("Unable to create output file for copy of input file:");
        ffpmsg(file_outfile);
        file_outfile[0] = '\0';
        return(status);
      }

      /* copy the file from input to output */
      while(0 != (nread = fread(recbuf,1,2880, diskfile)))
      {
        status = file_write(*handle, recbuf, nread);
        if (status) {
	   file_outfile[0] = '\0';
           return(status);
        }
      }

      /* close both files */
      fclose(diskfile);
      copyhandle = *handle;
      file_close(*handle);
      *handle = copyhandle;  /* reuse the old file handle */

      /* reopen the new copy, with correct rwmode */
      status = file_openfile(file_outfile, rwmode, &diskfile);
      file_outfile[0] = '\0';
    }
    else
    {
      *handle = -1;
      for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in table */
      {
        if (handleTable[ii].fileptr == 0)
        {
            *handle = ii;
            break;
        }
      }

      if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

      /*open the file */
      status = file_openfile(filename, rwmode, &diskfile);
    }

    handleTable[*handle].fileptr = diskfile;
    handleTable[*handle].currentpos = 0;
    handleTable[*handle].last_io_op = IO_SEEK;

    return(status);
}
/*--------------------------------------------------------------------------*/
int file_openfile(char *filename, int rwmode, FILE **diskfile)
/*
   lowest level routine to physically open a disk file
*/
{
    char mode[4];

#if defined(unix) || defined(__unix__) || defined(__unix)
    char tempname[1024], *cptr, user[80];
    struct passwd *pwd;
    int ii = 0;

#if defined(REPLACE_LINKS)
    struct stat stbuf;
    int success = 0;
    size_t n;
    FILE *f1, *f2;
    char buf[BUFSIZ];
#endif

#endif

    if (rwmode == READWRITE)
    {
          strcpy(mode, "r+b");    /* open existing file with read-write */
    }
    else
    {
          strcpy(mode, "rb");     /* open existing file readonly */
    }

#if MACHINE == ALPHAVMS || MACHINE == VAXVMS
        /* specify VMS record structure: fixed format, 2880 byte records */
        /* but force stream mode access to enable random I/O access      */
    *diskfile = fopen(filename, mode, "rfm=fix", "mrs=2880", "ctx=stm"); 

#elif defined(unix) || defined(__unix__) || defined(__unix)

    /* support the ~user/file.fits or ~/file.fits filenames in UNIX */

    if (*filename == '~')
    {
        if (filename[1] == '/')
        {
            cptr = getenv("HOME");
            if (cptr)
            {
                 if (strlen(cptr) + strlen(filename+1) > 1023)
		      return(FILE_NOT_OPENED); 

                 strcpy(tempname, cptr);
                 strcat(tempname, filename+1);
            }
            else
            {
                 if (strlen(filename) > 1023)
		      return(FILE_NOT_OPENED); 

                 strcpy(tempname, filename);
            }
        }
        else
        {
            /* copy user name */
            cptr = filename+1;
            while (*cptr && (*cptr != '/'))
            {
                user[ii] = *cptr;
                cptr++;
                ii++;
            }
            user[ii] = '\0';

            /* get structure that includes name of user's home directory */
            pwd = getpwnam(user);

            /* copy user's home directory */
            if (strlen(pwd->pw_dir) + strlen(cptr) > 1023)
		      return(FILE_NOT_OPENED); 

            strcpy(tempname, pwd->pw_dir);
            strcat(tempname, cptr);
        }

        *diskfile = fopen(tempname, mode); 
    }
    else
    {
        /* don't need to expand the input file name */
        *diskfile = fopen(filename, mode); 

#if defined(REPLACE_LINKS)

        if (!(*diskfile) && (rwmode == READWRITE))  
        {
           /* failed to open file with READWRITE privilege.  Test if  */
           /* the file we are trying to open is a soft link to a file that */
           /* doesn't have write privilege.  */

           lstat(filename, &stbuf);
           if ((stbuf.st_mode & S_IFMT) == S_IFLNK) /* is this a soft link? */
           {
              if ((f1 = fopen(filename, "rb")) != 0) /* try opening READONLY */
              {

                 if (strlen(filename) + 7 > 1023)
		      return(FILE_NOT_OPENED); 

                 strcpy(tempname, filename);
                 strcat(tempname, ".TmxFil");
                 if ((f2 = fopen(tempname, "wb")) != 0) /* create temp file */
                 {
                    success = 1;
                    while ((n = fread(buf, 1, BUFSIZ, f1)) > 0)
                    {
                       /* copy linked file to local temporary file */
                       if (fwrite(buf, 1, n, f2) != n) 
                       {
                          success = 0;
                          break;
                       } 
                    }
                    fclose(f2);
                 }
                 fclose(f1);
  
                 if (success)
                 {
                    /* delete link and rename temp file to previous link name */
                    remove(filename);
                    rename(tempname, filename);

                    /* try once again to open the file with write access */
                    *diskfile = fopen(filename, mode); 
                 }
                 else
                    remove(tempname);  /* clean up the failed copy */
              }
           }
        }
#endif

    }

#else

    /* other non-UNIX machines */
    *diskfile = fopen(filename, mode); 

#endif

    if (!(*diskfile))           /* couldn't open file */
    {
            return(FILE_NOT_OPENED); 
    }
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_create(char *filename, int *handle)
{
    FILE *diskfile;
    int ii;
    char mode[4];
 
    int status = 0, rootlen, rootlen2, slen;
    char *cptr, *cpos;
    char cwd[FLEN_FILENAME], absURL[FLEN_FILENAME];
    char rootstring[256], rootstring2[256];
    char username[FLEN_FILENAME], userroot[FLEN_FILENAME], userroot2[FLEN_FILENAME];

    cptr = getenv("HERA_DATA_DIRECTORY");
    if (cptr) {
	/* This environment variable is defined in the Hera data analysis environment. */
	/* It specifies the root directory path to the users data directories.  */
	/* CFITSIO will verify that the path to the file that is to be created */
	/* is within this root directory + the user's home directory name. */

/*
printf("env = %s\n",cptr);
*/	
        if (strlen(cptr) > 200)  /* guard against possible string overflows */
	    return(FILE_NOT_CREATED); 

	/* environment variable has the form "path/one/;/path/two/" where the */
	/* second path is optional */

	strcpy(rootstring, cptr);
	cpos = strchr(rootstring, ';');
	if (cpos) {
	    *cpos = '\0';
	    cpos++;
	    strcpy(rootstring2, cpos);
	} else {
	  *rootstring2 = '\0';
	}
/*
printf("%s, %s\n", rootstring, rootstring2);
printf("CWD = %s\n", cwd); 
printf("rootstring=%s, cwd=%s.\n", rootstring, cwd);
*/
	/* Get the current working directory */
	fits_get_cwd(cwd, &status);  
	slen = strlen(cwd);
	if (cwd[slen-1] != '/') strcat(cwd,"/"); /* make sure the CWD ends with slash */


	/* check that CWD string matches the rootstring */
	rootlen = strlen(rootstring);
	if (strncmp(rootstring, cwd, rootlen)) {
	    ffpmsg("invalid CWD: does not match root data directory");
	    return(FILE_NOT_CREATED); 
	} else {

	    /* get the user name from CWD (it follows the root string) */
	    strncpy(username, cwd+rootlen, 50);  /* limit length of user name */
	    cpos=strchr(username, '/');
	    if (!cpos) {
               ffpmsg("invalid CWD: not equal to root data directory + username");
               return(FILE_NOT_CREATED); 
	    } else {
	        *(cpos+1) = '\0';   /* truncate user name string */

		/* construct full user root name */
		strcpy(userroot, rootstring);
		strcat(userroot, username);
		rootlen = strlen(userroot);

		/* construct alternate full user root name */
		strcpy(userroot2, rootstring2);
		strcat(userroot2, username);
		rootlen2 = strlen(userroot2);

		/* convert the input filename to absolute path relative to the CWD */
		fits_relurl2url(cwd,  filename,  absURL, &status);

/*
printf("username = %s\n", username);
printf("userroot = %s\n", userroot);
printf("userroot2 = %s\n", userroot2);
printf("filename = %s\n", filename);
printf("ABS = %s\n", absURL);
*/
		/* check that CWD string matches the rootstring or alternate root string */

		if ( strncmp(userroot,  absURL, rootlen)  &&
		   strncmp(userroot2, absURL, rootlen2) ) {
		   ffpmsg("invalid filename: path not within user directory");
		   return(FILE_NOT_CREATED); 
		}
	    }
	}
	/* if we got here, then the input filename appears to be valid */
    }
    
    *handle = -1;
    for (ii = 0; ii < NMAXFILES; ii++)  /* find empty slot in table */
    {
        if (handleTable[ii].fileptr == 0)
        {
            *handle = ii;
            break;
        }
    }
    if (*handle == -1)
       return(TOO_MANY_FILES);    /* too many files opened */

    strcpy(mode, "w+b");    /* create new file with read-write */

    diskfile = fopen(filename, "r"); /* does file already exist? */

    if (diskfile)
    {
        fclose(diskfile);         /* close file and exit with error */
        return(FILE_NOT_CREATED); 
    }

#if MACHINE == ALPHAVMS || MACHINE == VAXVMS
        /* specify VMS record structure: fixed format, 2880 byte records */
        /* but force stream mode access to enable random I/O access      */
    diskfile = fopen(filename, mode, "rfm=fix", "mrs=2880", "ctx=stm"); 
#else
    diskfile = fopen(filename, mode); 
#endif

    if (!(diskfile))           /* couldn't create file */
    {
            return(FILE_NOT_CREATED); 
    }

    handleTable[ii].fileptr = diskfile;
    handleTable[ii].currentpos = 0;
    handleTable[ii].last_io_op = IO_SEEK;

    return(0);
}
/*--------------------------------------------------------------------------*/
int file_truncate(int handle, LONGLONG filesize)
/*
  truncate the diskfile to a new smaller size
*/
{

#ifdef HAVE_FTRUNCATE
    int fdesc;

    fdesc = fileno(handleTable[handle].fileptr);
    ftruncate(fdesc, (OFF_T) filesize);
    file_seek(handle, filesize);

    handleTable[handle].currentpos = filesize;
    handleTable[handle].last_io_op = IO_SEEK;

#endif

    return(0);
}
/*--------------------------------------------------------------------------*/
int file_size(int handle, LONGLONG *filesize)
/*
  return the size of the file in bytes
*/
{
    OFF_T position1,position2;
    FILE *diskfile;

    diskfile = handleTable[handle].fileptr;

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
 
/* call the VISUAL C++ version of the routines which support */
/*  Large Files (> 2GB) if they are supported (since VC 8.0)  */

    position1 = _ftelli64(diskfile);   /* save current postion */
    if (position1 < 0)
        return(SEEK_ERROR);

    if (_fseeki64(diskfile, 0, 2) != 0)  /* seek to end of file */
        return(SEEK_ERROR);

    position2 = _ftelli64(diskfile);     /* get file size */
    if (position2 < 0)
        return(SEEK_ERROR);

    if (_fseeki64(diskfile, position1, 0) != 0)  /* seek back to original pos */
        return(SEEK_ERROR);

#elif _FILE_OFFSET_BITS - 0 == 64

/* call the newer ftello and fseeko routines , which support */
/*  Large Files (> 2GB) if they are supported.  */

    position1 = ftello(diskfile);   /* save current postion */
    if (position1 < 0)
        return(SEEK_ERROR);

    if (fseeko(diskfile, 0, 2) != 0)  /* seek to end of file */
        return(SEEK_ERROR);

    position2 = ftello(diskfile);     /* get file size */
    if (position2 < 0)
        return(SEEK_ERROR);

    if (fseeko(diskfile, position1, 0) != 0)  /* seek back to original pos */
        return(SEEK_ERROR);

#else

    position1 = ftell(diskfile);   /* save current postion */
    if (position1 < 0)
        return(SEEK_ERROR);

    if (fseek(diskfile, 0, 2) != 0)  /* seek to end of file */
        return(SEEK_ERROR);

    position2 = ftell(diskfile);     /* get file size */
    if (position2 < 0)
        return(SEEK_ERROR);

    if (fseek(diskfile, position1, 0) != 0)  /* seek back to original pos */
        return(SEEK_ERROR);

#endif

    *filesize = (LONGLONG) position2;
    
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_close(int handle)
/*
  close the file
*/
{
    
    if (fclose(handleTable[handle].fileptr) )
        return(FILE_NOT_CLOSED);

    handleTable[handle].fileptr = 0;
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_remove(char *filename)
/*
  delete the file from disk
*/
{
    remove(filename);
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_flush(int handle)
/*
  flush the file
*/
{
    if (fflush(handleTable[handle].fileptr) )
        return(WRITE_ERROR);

    /* The flush operation is not supposed to move the internal */
    /* file pointer, but it does on some Windows-95 compilers and */
    /* perhaps others, so seek to original position to be sure. */
    /* This seek will do no harm on other systems.   */

#if MACHINE == IBMPC

    if (file_seek(handle, handleTable[handle].currentpos))
            return(SEEK_ERROR);

#endif

    return(0);
}
/*--------------------------------------------------------------------------*/
int file_seek(int handle, LONGLONG offset)
/*
  seek to position relative to start of the file
*/
{

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
    
     /* Microsoft visual studio C++ */
     /* _fseeki64 supported beginning with version 8.0 */
 
    if (_fseeki64(handleTable[handle].fileptr, (OFF_T) offset, 0) != 0)
        return(SEEK_ERROR);
	
#elif _FILE_OFFSET_BITS - 0 == 64

    if (fseeko(handleTable[handle].fileptr, (OFF_T) offset, 0) != 0)
        return(SEEK_ERROR);

#else

    if (fseek(handleTable[handle].fileptr, (OFF_T) offset, 0) != 0)
        return(SEEK_ERROR);

#endif

    handleTable[handle].currentpos = offset;
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_read(int hdl, void *buffer, long nbytes)
/*
  read bytes from the current position in the file
*/
{
    long nread;
    char *cptr;

    if (handleTable[hdl].last_io_op == IO_WRITE)
    {
        if (file_seek(hdl, handleTable[hdl].currentpos))
            return(SEEK_ERROR);
    }
  
    nread = (long) fread(buffer, 1, nbytes, handleTable[hdl].fileptr);

    if (nread == 1)
    {
         cptr = (char *) buffer;

         /* some editors will add a single end-of-file character to a file */
         /* Ignore it if the character is a zero, 10, or 32 */
         if (*cptr == 0 || *cptr == 10 || *cptr == 32)
             return(END_OF_FILE);
         else
             return(READ_ERROR);
    }
    else if (nread != nbytes)
    {
        return(READ_ERROR);
    }

    handleTable[hdl].currentpos += nbytes;
    handleTable[hdl].last_io_op = IO_READ;
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_write(int hdl, void *buffer, long nbytes)
/*
  write bytes at the current position in the file
*/
{
    if (handleTable[hdl].last_io_op == IO_READ) 
    {
        if (file_seek(hdl, handleTable[hdl].currentpos))
            return(SEEK_ERROR);
    }

    if((long) fwrite(buffer, 1, nbytes, handleTable[hdl].fileptr) != nbytes)
        return(WRITE_ERROR);

    handleTable[hdl].currentpos += nbytes;
    handleTable[hdl].last_io_op = IO_WRITE;
    return(0);
}
/*--------------------------------------------------------------------------*/
int file_compress_open(char *filename, int rwmode, int *hdl)
/*
  This routine opens the compressed diskfile by creating a new uncompressed
  file then opening it.  The input file name (the name of the compressed
  file) gets replaced with the name of the uncompressed file, which is
  initially stored in the global file_outfile string.   file_outfile
  then gets set to a null string.
*/
{
    FILE *indiskfile, *outdiskfile;
    int status;
    char *cptr;

    /* open the compressed disk file */
    status = file_openfile(filename, READONLY, &indiskfile);
    if (status)
    {
        ffpmsg("failed to open compressed disk file (file_compress_open)");
        ffpmsg(filename);
        return(status);
    }

    /* name of the output uncompressed file is stored in the */
    /* global variable called 'file_outfile'.                */

    cptr = file_outfile;
    if (*cptr == '!')
    {
        /* clobber any existing file with the same name */
        cptr++;
        remove(cptr);
    }
    else
    {
        outdiskfile = fopen(file_outfile, "r"); /* does file already exist? */

        if (outdiskfile)
        {
          ffpmsg("uncompressed file already exists: (file_compress_open)");
          ffpmsg(file_outfile);
          fclose(outdiskfile);         /* close file and exit with error */
	  file_outfile[0] = '\0';
          return(FILE_NOT_CREATED); 
        }
    }

    outdiskfile = fopen(cptr, "w+b"); /* create new file */
    if (!outdiskfile)
    {
        ffpmsg("could not create uncompressed file: (file_compress_open)");
        ffpmsg(file_outfile);
	file_outfile[0] = '\0';
        return(FILE_NOT_CREATED); 
    }

    /* uncompress file into another file */
    uncompress2file(filename, indiskfile, outdiskfile, &status);
    fclose(indiskfile);
    fclose(outdiskfile);

    if (status)
    {
        ffpmsg("error in file_compress_open: failed to uncompressed file:");
        ffpmsg(filename);
        ffpmsg(" into new output file:");
        ffpmsg(file_outfile);
	file_outfile[0] = '\0';
        return(status);
    }

    strcpy(filename, cptr);  /* switch the names */
    file_outfile[0] = '\0';

    status = file_open(filename, rwmode, hdl);

    return(status);
}
/*--------------------------------------------------------------------------*/
int file_is_compressed(char *filename) /* I - FITS file name          */
/*
  Test if the disk file is compressed.  Returns 1 if compressed, 0 if not.
  This may modify the filename string by appending a compression suffex.
*/
{
    FILE *diskfile;
    unsigned char buffer[2];
    char tmpfilename[FLEN_FILENAME];

    /* Open file.  Try various suffix combinations */  
    if (file_openfile(filename, 0, &diskfile))
    {
      if (strlen(filename) > FLEN_FILENAME - 1)
          return(0);

      strcpy(tmpfilename,filename);
      strcat(filename,".gz");
      if (file_openfile(filename, 0, &diskfile))
      {
#if HAVE_BZIP2
        strcpy(tmpfilename,filename);
        strcat(filename,".bz2");
        if (file_openfile(filename, 0, &diskfile))
        {
#endif
        strcpy(filename, tmpfilename);
        strcat(filename,".Z");
        if (file_openfile(filename, 0, &diskfile))
        {
          strcpy(filename, tmpfilename);
          strcat(filename,".z");   /* it's often lower case on CDROMs */
          if (file_openfile(filename, 0, &diskfile))
          {
            strcpy(filename, tmpfilename);
            strcat(filename,".zip");
            if (file_openfile(filename, 0, &diskfile))
            {
              strcpy(filename, tmpfilename);
              strcat(filename,"-z");      /* VMS suffix */
              if (file_openfile(filename, 0, &diskfile))
              {
                strcpy(filename, tmpfilename);
                strcat(filename,"-gz");    /* VMS suffix */
                if (file_openfile(filename, 0, &diskfile))
                {
                  strcpy(filename,tmpfilename);  /* restore original name */
                  return(0);    /* file not found */
                }
              }
            }
          }
        }
#if HAVE_BZIP2
        }
#endif
      }
    }

    if (fread(buffer, 1, 2, diskfile) != 2)  /* read 2 bytes */
    {
        fclose(diskfile);   /* error reading file so just return */
        return(0);
    }

    fclose(diskfile);

       /* see if the 2 bytes have the magic values for a compressed file */
    if ( (memcmp(buffer, "\037\213", 2) == 0) ||  /* GZIP  */
         (memcmp(buffer, "\120\113", 2) == 0) ||  /* PKZIP */
         (memcmp(buffer, "\037\036", 2) == 0) ||  /* PACK  */
         (memcmp(buffer, "\037\235", 2) == 0) ||  /* LZW   */
#if HAVE_BZIP2
         (memcmp(buffer, "BZ",       2) == 0) ||  /* BZip2 */
#endif
         (memcmp(buffer, "\037\240", 2) == 0))  /* LZH   */
        {
            return(1);  /* this is a compressed file */
        }
    else
        {
            return(0);  /* not a compressed file */
        }
}
/*--------------------------------------------------------------------------*/
int file_checkfile (char *urltype, char *infile, char *outfile) 
{
    /* special case: if file:// driver, check if the file is compressed */
    if ( file_is_compressed(infile) )
    {
      /* if output file has been specified, save the name for future use: */
      /* This is the name of the uncompressed file to be created on disk. */
      if (strlen(outfile))
      {
        if (!strncmp(outfile, "mem:", 4) )
        {
           /* uncompress the file in memory, with READ and WRITE access */
           strcpy(urltype, "compressmem://");  /* use special driver */
           *file_outfile = '\0';  
        }
        else
        {
          strcpy(urltype, "compressfile://");  /* use special driver */

          /* don't copy the "file://" prefix, if present.  */
          if (!strncmp(outfile, "file://", 7) )
             strcpy(file_outfile,outfile+7);
          else
             strcpy(file_outfile,outfile);
        }
      }
      else
      {
        /* uncompress the file in memory */
        strcpy(urltype, "compress://");  /* use special driver */
        *file_outfile = '\0';  /* no output file was specified */
      }
    }
    else  /* an ordinary, uncompressed FITS file on disk */
    {
        /* save the output file name for later use when opening the file. */
        /* In this case, the file to be opened will be opened READONLY,   */
        /* and copied to this newly created output file.  The original file */
        /* will be closed, and the copy will be opened by CFITSIO for     */
        /* subsequent processing (possibly with READWRITE access).        */
        if (strlen(outfile)) {
	    file_outfile[0] = '\0';
            strncat(file_outfile,outfile,FLEN_FILENAME-1);
        }
    }

    return 0;
}
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

/****  driver routines for stream//: device (stdin or stdout)  ********/


/*--------------------------------------------------------------------------*/
int stream_open(char *filename, int rwmode, int *handle)
{
    /*
        read from stdin
    */
    if (filename)
      rwmode = 1;  /* dummy statement to suppress unused parameter compiler warning */

    *handle = 1;     /*  1 = stdin */   

    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_create(char *filename, int *handle)
{
    /*
        write to stdout
    */

    if (filename)  /* dummy statement to suppress unused parameter compiler warning */
       *handle = 2;
    else
       *handle = 2;         /*  2 = stdout */       

    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_size(int handle, LONGLONG *filesize)
/*
  return the size of the file in bytes
*/
{
    handle = 0;  /* suppress unused parameter compiler warning */
    
    /* this operation is not supported in a stream; return large value */
    *filesize = LONG_MAX;
    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_close(int handle)
/*
     don't have to close stdin or stdout 
*/
{
    handle = 0;  /* suppress unused parameter compiler warning */
    
    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_flush(int handle)
/*
  flush the file
*/
{
    if (handle == 2)
       fflush(stdout);  

    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_seek(int handle, LONGLONG offset)
   /* 
      seeking is not allowed in a stream
   */
{
    offset = handle;  /* suppress unused parameter compiler warning */
    return(1);
}
/*--------------------------------------------------------------------------*/
int stream_read(int hdl, void *buffer, long nbytes)
/*
     reading from stdin stream 
*/

{
    long nread;
    
    if (hdl != 1)
       return(1);  /* can only read from stdin */

    nread = (long) fread(buffer, 1, nbytes, stdin);

    if (nread != nbytes)
    {
/*        return(READ_ERROR); */
        return(END_OF_FILE);
    }

    return(0);
}
/*--------------------------------------------------------------------------*/
int stream_write(int hdl, void *buffer, long nbytes)
/*
  write bytes at the current position in the file
*/
{
    if (hdl != 2)
       return(1);  /* can only write to stdout */

    if((long) fwrite(buffer, 1, nbytes, stdout) != nbytes)
        return(WRITE_ERROR);

    return(0);
}




