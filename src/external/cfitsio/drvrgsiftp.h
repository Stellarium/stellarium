#ifndef _GSIFTP_H
#define _GSIFTP_H

int gsiftp_init(void);
int gsiftp_setoptions(int options);
int gsiftp_getoptions(int *options);
int gsiftp_getversion(int *version);
int gsiftp_shutdown(void);
int gsiftp_checkfile(char *urltype, char *infile, char *outfile);
int gsiftp_open(char *filename, int rwmode, int *driverhandle);
int gsiftp_create(char *filename, int *driverhandle);
int gsiftp_truncate(int driverhandle, LONGLONG filesize);
int gsiftp_size(int driverhandle, LONGLONG *filesize);
int gsiftp_close(int driverhandle);
int gsiftp_remove(char *filename);
int gsiftp_flush(int driverhandle);
int gsiftp_seek(int driverhandle, LONGLONG offset);
int gsiftp_read (int driverhandle, void *buffer, long nbytes);
int gsiftp_write(int driverhandle, void *buffer, long nbytes);

#endif
