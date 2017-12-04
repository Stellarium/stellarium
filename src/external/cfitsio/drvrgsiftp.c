
/*  This file, drvrgsiftp.c contains driver routines for gsiftp files. */
/*  Andrea Barisani <lcars@si.inaf.it>                                 */
/* Taffoni Giuliano <taffoni@oats.inaf.it>                             */
#ifdef HAVE_NET_SERVICES
#ifdef HAVE_GSIFTP

#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <setjmp.h>
#include "fitsio2.h"

#include <globus_ftp_client.h>

#define MAXLEN 1200
#define NETTIMEOUT 80
#define MAX_BUFFER_SIZE_R 1024
#define MAX_BUFFER_SIZE_W (64*1024)

static int gsiftpopen = 0;
static int global_offset = 0;
static int gsiftp_get(char *filename, FILE **gsiftpfile, int num_streams);

static globus_mutex_t lock;
static globus_cond_t cond;
static globus_bool_t done;

static char *gsiftp_tmpfile;
static char *gsiftpurl = NULL;
static char gsiftp_tmpdir[MAXLEN];

static jmp_buf env; /* holds the jump buffer for setjmp/longjmp pairs */
static void signal_handler(int sig);

int gsiftp_init(void)
{

  if (getenv("GSIFTP_TMPFILE")) {
    gsiftp_tmpfile = getenv("GSIFTP_TMPFILE");
  } else {
    strncpy(gsiftp_tmpdir, "/tmp/gsiftp_XXXXXX", sizeof gsiftp_tmpdir);
    if (mkdtemp(gsiftp_tmpdir) == NULL) {
        ffpmsg("Cannot create temporary directory!");
        return (FILE_NOT_OPENED);
    }
    gsiftp_tmpfile = malloc(strlen(gsiftp_tmpdir) + strlen("/gsiftp_buffer.tmp"));
    strcat(gsiftp_tmpfile, gsiftp_tmpdir);
    strcat(gsiftp_tmpfile, "/gsiftp_buffer.tmp");
  }

  return file_init();
}

int gsiftp_shutdown(void)
{
  free(gsiftpurl);
  free(gsiftp_tmpfile);
  free(gsiftp_tmpdir);

  return file_shutdown();
}

int gsiftp_setoptions(int options)
{
  return file_setoptions(options);
}

int gsiftp_getoptions(int *options)
{
  return file_getoptions(options);
}

int gsiftp_getversion(int *version)
{
  return file_getversion(version);
}

int gsiftp_checkfile(char *urltype, char *infile, char *outfile)
{
  return file_checkfile(urltype, infile, outfile);
}

int gsiftp_open(char *filename, int rwmode, int *handle)
{
  FILE *gsiftpfile;
  int num_streams;

  if (getenv("GSIFTP_STREAMS")) {
    num_streams = (int)getenv("GSIFTP_STREAMS");
  } else {
    num_streams = 1;
  }
  
  if (rwmode) {
    gsiftpopen = 2;
  } else {
    gsiftpopen = 1;
  }
 
  if (gsiftpurl)
    free(gsiftpurl);
 
  gsiftpurl = strdup(filename);

  if (setjmp(env) != 0) {
    ffpmsg("Timeout (gsiftp_open)");
    goto error;
  }
  
  signal(SIGALRM, signal_handler);
  alarm(NETTIMEOUT);

  if (gsiftp_get(filename,&gsiftpfile,num_streams)) {
    alarm(0);
    ffpmsg("Unable to open gsiftp file (gsiftp_open)");
    ffpmsg(filename);
    goto error;
  } 
  
  fclose(gsiftpfile);
  
  signal(SIGALRM, SIG_DFL);
  alarm(0);

  return file_open(gsiftp_tmpfile, rwmode, handle);

  error:
   alarm(0);
   signal(SIGALRM, SIG_DFL);
   return (FILE_NOT_OPENED);
}

int gsiftp_create(char *filename, int *handle)
{
  if (gsiftpurl)
    free(gsiftpurl);

  gsiftpurl = strdup(filename);

  return file_create(gsiftp_tmpfile, handle);
}

int gsiftp_truncate(int handle, LONGLONG filesize)
{
  return file_truncate(handle, filesize);
}

int gsiftp_size(int handle, LONGLONG *filesize)
{
  return file_size(handle, filesize);
}

int gsiftp_flush(int handle)
{
  FILE *gsiftpfile;
  int num_streams;

  if (getenv("GSIFTP_STREAMS")) {
    num_streams = (int)getenv("GSIFTP_STREAMS");
  } else {
    num_streams = 1;
  }
  
  int rc = file_flush(handle);

  if (gsiftpopen != 1) {
  
    if (setjmp(env) != 0) {
      ffpmsg("Timeout (gsiftp_write)");
      goto error;
    }
  
    signal(SIGALRM, signal_handler);
    alarm(NETTIMEOUT);

    if (gsiftp_put(gsiftpurl,&gsiftpfile,num_streams)) {
      alarm(0);
      ffpmsg("Unable to open gsiftp file (gsiftp_flush)");
      ffpmsg(gsiftpurl);
      goto error;
    } 
  
    fclose(gsiftpfile);
  
    signal(SIGALRM, SIG_DFL);
    alarm(0);
  }
  
  return rc;

  error:
   alarm(0);
   signal(SIGALRM, SIG_DFL);
   return (FILE_NOT_OPENED);
}

int gsiftp_seek(int handle, LONGLONG offset)
{
  return file_seek(handle, offset);
}

int gsiftp_read(int hdl, void *buffer, long nbytes)
{
  return file_read(hdl, buffer, nbytes);
}

int gsiftp_write(int hdl, void *buffer, long nbytes)
{
  return file_write(hdl, buffer, nbytes);
}

int gsiftp_close(int handle)
{
    unlink(gsiftp_tmpfile);
    
    if (gsiftp_tmpdir)
        rmdir(gsiftp_tmpdir);

    return file_close(handle);
}

static void done_cb( void * 				user_arg,
                     globus_ftp_client_handle_t * 	handle,
                     globus_object_t * 			err)
{

    if(err){
        fprintf(stderr, "%s", globus_object_printable_to_string(err));
    }
    
    globus_mutex_lock(&lock);
    done = GLOBUS_TRUE;
    globus_cond_signal(&cond);
    globus_mutex_unlock(&lock);
    return;
}

static void data_cb_read( void * 			user_arg,
                          globus_ftp_client_handle_t * 	handle,
                          globus_object_t * 		err,
                          globus_byte_t * 		buffer,
                          globus_size_t 		length,
                          globus_off_t 			offset,
                          globus_bool_t 		eof)
{
    if(err) {
        fprintf(stderr, "%s", globus_object_printable_to_string(err));
    }
    else {
        FILE* fd = (FILE*) user_arg;
        int rc = fwrite(buffer, 1, length, fd);
        if (ferror(fd)) {
            printf("Read error in function data_cb_read; errno = %d\n", errno);
            return;
        }

        if (!eof) {
            globus_ftp_client_register_read(handle,
                                            buffer,
                                            MAX_BUFFER_SIZE_R,
                                            data_cb_read,
                                            (void*) fd);
        }
    }
    return;
}

static void data_cb_write( void * 			user_arg,
                           globus_ftp_client_handle_t * handle,
                           globus_object_t * 		err,
                           globus_byte_t * 		buffer,
                           globus_size_t 		length,
                           globus_off_t 		offset,
                           globus_bool_t 		eof)
{
    int curr_offset;
    if(err) {
        fprintf(stderr, "%s", globus_object_printable_to_string(err));
    }
    else {
        if (!eof) {
            FILE* fd = (FILE*) user_arg;
            int rc;
            globus_mutex_lock(&lock);
            curr_offset = global_offset;
            rc = fread(buffer, 1, MAX_BUFFER_SIZE_W, fd);
            global_offset += rc;
            globus_mutex_unlock(&lock);
            if (ferror(fd)) {
                printf("Read error in function data_cb_write; errno = %d\n", errno);
                return;
            }

            globus_ftp_client_register_write(handle,
                                             buffer,
					     rc,
					     curr_offset,
                                             feof(fd) != 0,
                                             data_cb_write,
                                             (void*) fd);
        } else {
            globus_libc_free(buffer);
        }
    }
    return;
}

int gsiftp_get(char *filename, FILE **gsiftpfile, int num_streams)
{
    char gsiurl[MAXLEN];

    globus_ftp_client_handle_t 		handle;
    globus_ftp_client_operationattr_t 	attr;
    globus_ftp_client_handleattr_t 	handle_attr;
    globus_ftp_control_parallelism_t   	parallelism;
    globus_ftp_control_layout_t		layout;
    globus_byte_t 			buffer[MAX_BUFFER_SIZE_R];
    globus_size_t buffer_length = sizeof(buffer);
    globus_result_t 			result;
    globus_ftp_client_restart_marker_t	restart;
    globus_ftp_control_type_t 		filetype;
   
    globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    globus_mutex_init(&lock, GLOBUS_NULL);
    globus_cond_init(&cond, GLOBUS_NULL);
    globus_ftp_client_handle_init(&handle,  GLOBUS_NULL);
    globus_ftp_client_handleattr_init(&handle_attr);
    globus_ftp_client_operationattr_init(&attr);
    layout.mode = GLOBUS_FTP_CONTROL_STRIPING_NONE;
    globus_ftp_client_restart_marker_init(&restart);
    globus_ftp_client_operationattr_set_mode(
            &attr,
            GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
   
    if (num_streams >= 1)
    {
        parallelism.mode = GLOBUS_FTP_CONTROL_PARALLELISM_FIXED;
        parallelism.fixed.size = num_streams;
       
        globus_ftp_client_operationattr_set_parallelism(
            &attr,
            &parallelism);
    }
   
    globus_ftp_client_operationattr_set_layout(&attr,
                                               &layout);
   
    filetype = GLOBUS_FTP_CONTROL_TYPE_IMAGE;
    globus_ftp_client_operationattr_set_type (&attr,
                                              filetype);
   
    globus_ftp_client_handle_init(&handle, &handle_attr);
   
    done = GLOBUS_FALSE;

    strcpy(gsiurl,"gsiftp://");
    strcat(gsiurl,filename);

    *gsiftpfile = fopen(gsiftp_tmpfile,"w+");
    
    if (!*gsiftpfile) {
        ffpmsg("Unable to open temporary file!");
        return (FILE_NOT_OPENED);
    }
   
    result = globus_ftp_client_get(&handle,
                                   gsiurl,
                                   &attr,
                                   &restart,
                                   done_cb,
                                   0);
    if(result != GLOBUS_SUCCESS) {
        globus_object_t * err;
        err = globus_error_get(result);
        fprintf(stderr, "%s", globus_object_printable_to_string(err));
        done = GLOBUS_TRUE;
    }
    else {
        globus_ftp_client_register_read(&handle,
                                        buffer,
                                        buffer_length,
                                        data_cb_read,
                                        (void*) *gsiftpfile);
    }
   
    globus_mutex_lock(&lock);

    while(!done) {
        globus_cond_wait(&cond, &lock);
    }

    globus_mutex_unlock(&lock);
    globus_ftp_client_handle_destroy(&handle);
    globus_module_deactivate_all();
   
    return 0;
}

int gsiftp_put(char *filename, FILE **gsiftpfile, int num_streams)
{
    int i;
    char gsiurl[MAXLEN];

    globus_ftp_client_handle_t 		handle;
    globus_ftp_client_operationattr_t 	attr;
    globus_ftp_client_handleattr_t 	handle_attr;
    globus_ftp_control_parallelism_t   	parallelism;
    globus_ftp_control_layout_t		layout;
    globus_byte_t * 			buffer;
    globus_size_t buffer_length = sizeof(buffer);
    globus_result_t 			result;
    globus_ftp_client_restart_marker_t	restart;
    globus_ftp_control_type_t 		filetype;
   
    globus_module_activate(GLOBUS_FTP_CLIENT_MODULE);
    globus_mutex_init(&lock, GLOBUS_NULL);
    globus_cond_init(&cond, GLOBUS_NULL);
    globus_ftp_client_handle_init(&handle,  GLOBUS_NULL);
    globus_ftp_client_handleattr_init(&handle_attr);
    globus_ftp_client_operationattr_init(&attr);
    layout.mode = GLOBUS_FTP_CONTROL_STRIPING_NONE;
    globus_ftp_client_restart_marker_init(&restart);
    globus_ftp_client_operationattr_set_mode(
            &attr,
            GLOBUS_FTP_CONTROL_MODE_EXTENDED_BLOCK);
   
    if (num_streams >= 1)
    {
        parallelism.mode = GLOBUS_FTP_CONTROL_PARALLELISM_FIXED;
        parallelism.fixed.size = num_streams;
       
        globus_ftp_client_operationattr_set_parallelism(
            &attr,
            &parallelism);
    }
   
    globus_ftp_client_operationattr_set_layout(&attr,
                                               &layout);
   
    filetype = GLOBUS_FTP_CONTROL_TYPE_IMAGE;
    globus_ftp_client_operationattr_set_type (&attr,
                                              filetype);
   
    globus_ftp_client_handle_init(&handle, &handle_attr);
   
    done = GLOBUS_FALSE;
    
    strcpy(gsiurl,"gsiftp://");
    strcat(gsiurl,filename);

    *gsiftpfile = fopen(gsiftp_tmpfile,"r");

    if (!*gsiftpfile) {
        ffpmsg("Unable to open temporary file!");
        return (FILE_NOT_OPENED);
    }
   
    result = globus_ftp_client_put(&handle,
                                   gsiurl,
                                   &attr,
                                   &restart,
                                   done_cb,
                                   0);
    if(result != GLOBUS_SUCCESS) {
        globus_object_t * err;
        err = globus_error_get(result);
        fprintf(stderr, "%s", globus_object_printable_to_string(err));
        done = GLOBUS_TRUE;
    }
    else {
        int rc;
        int curr_offset;

	for (i = 0; i< 2 * num_streams && feof(*gsiftpfile) == 0; i++)
        {
            buffer = malloc(MAX_BUFFER_SIZE_W);
            globus_mutex_lock(&lock);
            curr_offset = global_offset;
            rc = fread(buffer, 1, MAX_BUFFER_SIZE_W, *gsiftpfile);
            global_offset += rc;
            globus_mutex_unlock(&lock);
            globus_ftp_client_register_write(
                &handle,
                buffer,
                rc,
                curr_offset,
                feof(*gsiftpfile) != 0,
                data_cb_write,
                (void*) *gsiftpfile);
        }
    }
   
    globus_mutex_lock(&lock);

    while(!done) {
        globus_cond_wait(&cond, &lock);
    }

    globus_mutex_unlock(&lock);
    globus_ftp_client_handle_destroy(&handle);
    globus_module_deactivate_all();
   
    return 0;
}

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

#endif
#endif
