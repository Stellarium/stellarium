#if 0
    STV Low Level Driver
    Copyright (C) 2006 Markus Wildi, markus.wildi@datacomm.ch 
    The initial work is based on the program STVremote by Shashikiran Ganesh.
    email: gshashikiran_AT_linuxmail_dot_org

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

#define _GNU_SOURCE 1

#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <stdio.h>
#include <ctime>
#include <stdlib.h>

#include "config.h"

#include "stvdriver.h"

#include <libnova.h>

#ifndef _WIN32
#include <termios.h>
#endif

/* INDI Common Routines/RS232 */

#include "indicom.h"

extern int fd;
extern char tracking_buf[];
struct termios orig_tty_setting; /* old serial port setting to restore on close */
struct termios tty_setting;      /* new serial port setting */

#define FALSE 0
#define TRUE  1

/*int tty_read( int fd, char *buf, int nbytes, int timeout, int *nbytes_read) ; */
/*int tty_write( int fd, const char *buffer, int *nbytes_written) ; */
void ISUpdateDisplay(int buffer, int line);

int STV_portWrite(char *buf, int nbytes);
int STV_TXDisplay(void);
int STV_TerminateTXDisplay(void);
int STV_FileStatus(int);
int STV_DownloadComplete(void);
int STV_RequestImage(int compression, int buffer, int x_offset, int y_offset, int *length, int *lines, int image[][320],
                     IMAGE_INFO *image_info);
int STV_RequestImageData(int compression, int *data, int j, int length, int *values);
int STV_Download(void);
int STV_DownloadAll(void);
int STV_RequestAck(void);
int STV_CheckHeaderSum(unsigned char *buf);
int STV_CheckDataSum(unsigned char *data);
int STV_PrintBuffer(unsigned char *buf, int n);
int STV_PrintBufferAsText(unsigned char *buf, int n);
int STV_CheckAck(unsigned char *buf);
int STV_SendPacket(int cmd, int *data, int n);
int STV_ReceivePacket(unsigned char *buf, int mode);
int STV_DecompressData(unsigned char *data, int *values, int length, int expected_n_values);
int STV_BufferStatus(int buffer);
void STV_PrintBits(unsigned int x, int n);

unsigned int STV_RecombineInt(unsigned char low_byte, unsigned char high_byte);
unsigned int STV_GetBits(unsigned int x, int p, int n);

int STV_MenueSetup(int delay);
int STV_MenueDateTime(int delay);
int STV_MenueCCDTemperature(int delay);

typedef struct
{
    double ccd_temperature;

} DISPLAY_INFO;

DISPLAY_INFO di;

/* STV Buttons */

int STV_LRRotaryDecrease(void)
{
    int data[] = { LR_ROTARY_DECREASE_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_LRRotaryIncrease(void)
{
    int data[] = { LR_ROTARY_INCREASE_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_UDRotaryDecrease(void)
{
    int data[] = { UD_ROTARY_DECREASE_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_UDRotaryIncrease(void)
{
    int data[] = { UD_ROTARY_INCREASE_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_AKey(void)
{ /* Parameter button */
    int data[] = { A_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_BKey(void)
{ /* Value Button */
    int data[] = { B_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Setup(void)
{
    int data[] = { SETUP_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Interrupt(void)
{
    int data[] = { INT_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Focus(void)
{
    int data[] = { FOCUS_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Image(void)
{
    int data[] = { IMAGE_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Monitor(void)
{
    int data[] = { MONITOR_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Calibrate(void)
{
    int data[] = { CAL_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Track(void)
{
    int data[] = { TRACK_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_Display(void)
{
    int data[] = { DISPLAY_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_FileOps(void)
{
    int data[] = { FILEOPS_KEY_PATTERN };
    return STV_SendPacket(SEND_KEY_PATTERN, data, 1);
}

int STV_RequestImageInfo(int buffer, IMAGE_INFO *image_info)
{
    int i;
    int length;
    int res;
    int data[] = { buffer };
    unsigned char buf[1024];
    unsigned int value[21];

    if ((res = STV_BufferStatus(buffer)) != 0)
    {
        if (res == 1)
        {
            /*fprintf( stderr," STV_RequestImageInfo buffer empty %d\n", buffer) ; */
            return res; /* Buffer is empty */
        }
        else
        {
            fprintf(stderr, " STV_RequestImageInfo error %d\n", res);
            return res;
        }
    }

    res = STV_SendPacket(REQUEST_IMAGE_INFO, data, 1);
    usleep(50000);
    res = STV_ReceivePacket(buf, 0);

    if (buf[1] != REQUEST_IMAGE_INFO)
    {
    /*length= buf[3] * 0x100 + buf[2] ; */
    /*res= STV_PrintBuffer(buf, 6+ length+ 2) ; */

    AGAINI:
        if ((res = STV_ReceivePacket(buf, 0)) < 0)
        { /* STV answers with a packet 0x03 sometimes, should be 0x04 */
            return -1;
            ;
        }
        if (buf[1] != REQUEST_IMAGE_INFO)
        { /* Second try */
            fprintf(stderr, "STV_RequestImageInfo: expected REQUEST_IMAGE_INFO, received %d, try again\n", buf[1]);
            goto AGAINI;
        }
    }

    length = buf[3] * 0x100 + buf[2];

    /* DECODE it */

    for (i = 6; i < length; i += 2)
    {
        value[(i - 6) / 2] = STV_RecombineInt(buf[i], buf[i + 1]);
    }

    image_info->descriptor = value[0];

    /* Decode the descriptor */

    /*STV_PrintBits(( unsigned int)value[0], 16) ; */

    if ((image_info->descriptor & ID_BITS_MASK) == ID_BITS_10)
    {
        /*    fprintf( stderr, "ADC Resolution: 10 bits\n") ; */
    }
    else if ((image_info->descriptor & ID_BITS_MASK) == ID_BITS_8)
    {
        /*fprintf( stderr, "Resolution: 8 bits (focus mode only)\n") ; */
    }

    if ((image_info->descriptor & ID_UNITS_MASK) == ID_UNITS_INCHES)
    {
        /*fprintf( stderr, "Units: inch\n") ; */
    }
    else if ((image_info->descriptor & ID_UNITS_MASK) == ID_UNITS_CM)
    {
        /*fprintf( stderr, "Unis: cm\n") ;  */
    }

    if ((image_info->descriptor & ID_SCOPE_MASK) == ID_SCOPE_REFRACTOR)
    {
        /*fprintf( stderr, "Refractor\n") ;  */
    }
    else if ((image_info->descriptor & ID_SCOPE_MASK) == ID_SCOPE_REFLECTOR)
    {
        /*fprintf( stderr, "Reflector\n") ;  */
    }

    if ((image_info->descriptor & ID_DATETIME_MASK) == ID_DATETIME_VALID)
    {
        /*fprintf( stderr, "Date and time valid\n") ;  */
    }
    else if ((image_info->descriptor & ID_DATETIME_MASK) == ID_DATETIME_INVALID)
    {
        /*fprintf( stderr, "Date and time invalid\n") ;  */
    }

    if ((image_info->descriptor & ID_BIN_MASK) == ID_BIN_1X1)
    {
        image_info->binning = 1;
    }
    else if ((image_info->descriptor & ID_BIN_MASK) == ID_BIN_2X2)
    {
        image_info->binning = 2;
    }
    else if ((image_info->descriptor & ID_BIN_MASK) == ID_BIN_3X3)
    {
        image_info->binning = 3;
    }

    if ((image_info->descriptor & ID_PM_MASK) == ID_PM_PM)
    {
        /*fprintf( stderr, "Time: PM\n") ;  */
    }
    else if ((image_info->descriptor & ID_PM_MASK) == ID_PM_AM)
    {
        /*fprintf( stderr, "Time: AM\n") ;  */
    }

    /* ToDo: Include that to the fits header */
    if ((image_info->descriptor & ID_FILTER_MASK) == ID_FILTER_LUNAR)
    {
        /*fprintf( stderr, "Filter: Lunar\n") ;  */
    }
    else if ((image_info->descriptor & ID_FILTER_MASK) == ID_FILTER_NP)
    {
        /*fprintf( stderr, "Filter: clear\n") ;  */
    }

    /* ToDo: Include that to the fits header */
    if ((image_info->descriptor & ID_DARKSUB_MASK) == ID_DARKSUB_YES)
    {
        /*fprintf( stderr, "Drak sub yes\n") ;  */
    }
    else if ((image_info->descriptor & ID_DARKSUB_MASK) == ID_DARKSUB_NO)
    {
        /*fprintf( stderr, "Dark sub no\n") ;  */
    }

    if ((image_info->descriptor & ID_MOSAIC_MASK) == ID_MOSAIC_NONE)
    {
        /*fprintf( stderr, "Is NOT a mosaic\n") ;  */
    }
    else if ((image_info->descriptor & ID_MOSAIC_MASK) == ID_MOSAIC_SMALL)
    {
        /*fprintf( stderr, "Is a small mosaic\n") ;  */
    }
    else if ((image_info->descriptor & ID_MOSAIC_MASK) == ID_MOSAIC_LARGE)
    {
        /*fprintf( stderr, "Is a large mosaic\n") ;  */
    }

    image_info->height = value[1];
    image_info->width  = value[2];
    image_info->top    = value[3];
    image_info->left   = value[4];

    /* Exposure time */

    if ((value[5] >= 100) && (value[5] <= 60000))
    {
        image_info->exposure = ((float)value[5]) * 0.01;
    }
    else if ((value[5] >= 60001) && (value[5] <= 60999))
    {
        image_info->exposure = ((float)value[5] - 60000.) * .001;
    }
    else
    {
        fprintf(stderr, "Error Exposure time %d\n", value[5]);
        image_info->exposure = -1.;
    }

    image_info->noExposure  = value[6];
    image_info->analogGain  = value[7];
    image_info->digitalGain = value[8];
    image_info->focalLength = value[9];
    image_info->aperture    = value[10];

    image_info->packedDate = value[11];
    image_info->year       = STV_GetBits(value[11], 6, 7) + 1999;
    image_info->day        = STV_GetBits(value[11], 11, 5);
    image_info->month      = STV_GetBits(value[11], 15, 4);

    image_info->packedTime = value[12];
    image_info->seconds    = STV_GetBits(value[12], 5, 6);
    image_info->minutes    = STV_GetBits(value[12], 11, 6);
    image_info->hours      = STV_GetBits(value[12], 15, 4);

    if ((image_info->descriptor & ID_PM_PM) > 0)
    {
        image_info->hours += 12;
    }

    if ((value[13] & 0x8000) == 0x8000)
    {
        image_info->ccdTemp = (float)(0xffff - value[13]) / 100.;
    }
    else
    {
        image_info->ccdTemp = (float)value[13] / 100.;
    }

    image_info->siteID     = value[14];
    image_info->eGain      = value[15];
    image_info->background = value[16];
    image_info->range      = value[17];
    image_info->pedestal   = value[18];
    image_info->ccdTop     = value[19];
    image_info->ccdLeft    = value[20];

    /*   fprintf( stderr, "descriptor:%d 0x%2x 0x%2x\n", image_info->descriptor, buf[6], buf[6+1]) ; */
    /*   fprintf( stderr, "height:%d\n", image_info->height) ; */
    /*   fprintf( stderr, "width:%d\n", image_info->width) ; */
    /*   fprintf( stderr, "top:%d\n", image_info->top) ; */
    /*   fprintf( stderr, "left:%d\n", image_info->left) ; */
    /*   fprintf( stderr, "exposure:%f\n", image_info->exposure) ; */
    /*   fprintf( stderr, "noExposure:%d\n", image_info->noExposure) ; */
    /*   fprintf( stderr, "analogGain:%d\n", image_info->analogGain) ; */
    /*   fprintf( stderr, "digitalGain:%d\n", image_info->digitalGain) ; */
    /*   fprintf( stderr, "focalLength:%d\n", image_info->focalLength) ; */
    /*   fprintf( stderr, "aperture:%d\n", image_info->aperture) ; */
    /*   fprintf( stderr, "packedDate:%d\n", image_info->packedDate) ; */
    /*   fprintf( stderr, "Year:%d\n", image_info->year) ; */
    /*   fprintf( stderr, "Day:%d\n", image_info->day) ; */
    /*   fprintf( stderr, "Month:%d\n", image_info->month) ; */
    /*   fprintf( stderr, "packedTime:%d\n", image_info->packedTime) ; */
    /*   fprintf( stderr, "Seconds:%d\n", image_info->seconds) ; */
    /*   fprintf( stderr, "minutes:%d\n", image_info->minutes) ; */
    /*   fprintf( stderr, "hours:%d\n", image_info->hours) ; */
    /*   fprintf( stderr, "ccdTemp:%f\n", image_info->ccdTemp) ; */
    /*   fprintf( stderr, "siteID:%d\n", image_info->siteID) ; */
    /*   fprintf( stderr, "eGain:%d\n", image_info->eGain) ; */
    /*   fprintf( stderr, "background:%d\n", image_info->background) ; */
    /*   fprintf( stderr, "range :%d\n", image_info->range ) ; */
    /*   fprintf( stderr, "pedestal:%d\n", image_info->pedestal) ; */
    /*   fprintf( stderr, "ccdTop  :%d\n", image_info->ccdTop) ; */
    /*   fprintf( stderr, "ccdLeft:%d\n", image_info->ccdLeft) ; */

    return 0;
}

int STV_RequestImage(int compression, int buffer, int x_offset, int y_offset, int *length, int *lines, int image[][320],
                     IMAGE_INFO *image_info)
{
    int i, j;
    int res;
    int n_values;
    unsigned char buf[0xffff];
    int values[1024];
    int data[] = { 0, y_offset, *length, buffer }; /* Offset row, Offset line, length, buffer number */
    int XOFF;

    /* fprintf(stderr, "STV_RequestImage: --------------------------------buffer= %d, %d\n", buffer, lines) ; */

    if ((res = STV_RequestImageInfo(buffer, image_info)) != 0)
    { /* Trigger for download */
        /* buffer empty */
        return res;
    }
    res = STV_BKey(); /* "press" the STV Value button */
    usleep(50000);
    res = STV_ReceivePacket(buf, 0);

    /* Check the boundaries obtained from the image data versus windowing */
    /* Take the smaller boundaries in each direction */

    if (x_offset > image_info->height)
    {
        x_offset = image_info->height;
    }
    XOFF = image_info->top + x_offset;

    if (y_offset > image_info->width)
    {
        y_offset = image_info->width;
    }
    data[1] = image_info->left + y_offset;

    if (*length > image_info->width - y_offset)
    {
        *length = image_info->width - y_offset;
    }
    data[2] = *length;

    if (*lines > image_info->height - x_offset)
    {
        *lines = image_info->height - x_offset;
    }

    /*fprintf(stderr, "STV_RequestImage: DATA 0=%d, 1=%d, 2=%d, 3=%d, length=%d, lines=%d\n", data[0], data[1], data[2], data[3], *length, *lines) ; */

    for (j = 0; j < *lines; j++)
    {
        data[0] = j + XOFF; /*line */

        if ((n_values = STV_RequestImageData(compression, data, j, *length, values)) < 0)
        {
            fprintf(stderr, "STV_RequestImage: Calling STV_RequestImageData failed %d\n", n_values);
            return n_values;
        }
        else
        {
            for (i = 0; i < n_values; i++)
            {
                image[j][i] = values[i];
            }
            ISUpdateDisplay(buffer, j);
        }
    }
    /* Read the 201th line, see mosaic mode */

    /* ToDo: analyze/display the data or use them in the fits header(s) */
    data[0] = 200; /*line */

    if ((res = STV_RequestImageData(1, data, 200, *length, values)) < 0)
    {
        return res;
    }
    else
    {
        for (i = 0; i < n_values; i++)
        {
            /* fprintf( stderr, "%d: %d ", i, values[i]) ; */
        }
        /* fprintf( stderr, "\n") ; */
    }
    res = STV_DownloadComplete();
    ISUpdateDisplay(buffer, -j);
    return 0;
}

int STV_RequestImageData(int compression, int *data, int j, int length, int *values)
{
    int i;
    int res;
    int data_length;
    int n_values;
    unsigned char buf[0xffff];

    data_length = -1;

    if (compression == ON)
    { /* compressed download */

        if ((res = STV_SendPacket(REQUEST_COMPRESSED_IMAGE_DATA, data, 4)) != 0)
        {
            fprintf(stderr, "STV_RequestImageData: could not write\n");
            return -1;
        }

    AGAINC:
        if ((res = STV_ReceivePacket(buf, 0)) > 0)
        {
            if (buf[1] != REQUEST_COMPRESSED_IMAGE_DATA)
            {
                if (buf[1] != DISPLAY_ECHO)
                {
                    if (buf[1] != ACK)
                    {
                        fprintf(stderr, "STV_RequestImageData: expected REQUEST_COMPRESSED_IMAGE_DATA, received %2x\n",
                                buf[1]);
                    }
                }
                goto AGAINC;
            }

            data_length = (int)buf[3] * 0x100 + (int)buf[2];

            if ((n_values = STV_DecompressData((buf + 6), values, data_length, length)) < 0)
            {
                n_values = -n_values;
                fprintf(stderr, "SEVERE ERROR on Line %d, pixel position=%d\n", j, n_values);
                _exit(-1);
            }

            if (n_values == length)
            {
                return n_values;
            }
            else
            {
                fprintf(stderr, "SEVERE Error: Length not equal, Line: %d, %d != %d, data %2x %2x, values %d, %d\n", j,
                        n_values, length, buf[6 + length - 2], buf[6 + length - 1], values[n_values - 2],
                        values[n_values - 1]);
                _exit(1);
            }
        }
        else
        {
            fprintf(stderr, "Error: waiting for data on the serial port at line %d\n", j);
            return -1;
        }
    }
    else
    { /* uncompressed download */

        if ((res = STV_SendPacket(REQUEST_IMAGE_DATA, data, 4)) == 0)
        {
        AGAINU:
            if ((res = STV_ReceivePacket(buf, 0)) > 0)
            {
                if (buf[1] != REQUEST_IMAGE_DATA)
                {
                    if (buf[1] != DISPLAY_ECHO)
                    {
                        if (buf[1] != ACK)
                        {
                            fprintf(stderr, "STV_RequestImageData: expected REQUEST_IMAGE_DATA, received %2x\n",
                                    buf[1]);
                        }
                    }
                    goto AGAINU;
                }

                data_length = (int)buf[3] * 0x100 + (int)buf[2];

                for (i = 0; i < data_length; i += 2)
                {
                    values[i / 2] = STV_RecombineInt(buf[6 + i], buf[7 + i]);
                }
                return data_length / 2;
                /*ISUpdateDisplay( buffer, j) ; */ /* Update the display of the INDI client */
            }
            else
            {
                fprintf(stderr, "Error: waiting for data on the serial port at line %d\n", j);
                return -1;
            }
        }
        else
        {
            fprintf(stderr, "STV_RequestImageData: error writing %d\n", res);
        }
    }
    return 0;
}
int STV_DecompressData(unsigned char *data, int *values, int length, int expected_n_values)
{
    int i, n_values;
    int value;
    int base;

    base = STV_RecombineInt(data[0], data[1]); /*STV Manual says: MSB then LSB! */

    values[0] = base;
    i         = 2;
    n_values  = 1;

    while (i < length)
    {
        if (((data[i] & 0xc0)) == 0x80)
        { /*two bytes, first byte: bit 7 set, 6 cleared, 14 bits data */

            if ((data[i] & 0x20) == 0x20)
            { /* minus sign set? */

                value =
                    -((int)(((~data[i] & 0x1f)) * 0x100) + (((int)(~data[i + 1])) & 0xff)) - 1; /* value without sign */
            }
            else
            {
                value = (int)((data[i] & 0x1f) * 0x100) + (int)(data[i + 1]);
            }

            base               = value + base; /* new base value */
            values[n_values++] = base;         /*pixel data */
            i += 2;
        }
        else if (((data[i] & 0x80)) == 0)
        { /* one byte: bit 7 clear (7 bits data) */

            if ((data[i] & 0x40) == 0x40)
            { /* minus sign set? */

                value = -(int)((~(data[i])) & 0x3f) - 1; /* value without sign */
            }
            else
            {
                value = (int)(data[i] & 0x3f);
                /*fprintf( stderr, "Value 7P: %d, pixel value: %d, length %d, pix=%d\n", value, value+ base, length, n_values) ; */
            }
            /* Sometimes the STV send a 0 at the end, thats not very likely a pixel to pixel variation */
            /* I checked the last decompressed pixel value against the last uncompressed pixel value */
            /* with different images - it seems to be ok.    */
            if ((value == 0) && (n_values == expected_n_values))
            {
                /*fprintf( stderr, "Ignoring byte %d, Zero difference obtained values: %d\n", i, n_values) ; */
            }
            else
            {
                base               = value + base;
                values[n_values++] = base;
            }
            i++;
        }
        else if (((data[i] & 0xc0)) == 0xc0)
        { /*two bytes, values= pixel_n/4  */

            /*STV_PrintBits(  data[ i], 8) ; */
            /*STV_PrintBits(  data[ i+ 1], 8) ; */

            value = 4 * ((int)((data[i] & 0x3f) * 0x100) + (int)(data[i + 1]));
            /*fprintf( stderr, "Value 14P: %d, pixel value: %d, length %d, pix=%d\n", value, value+ base, length, n_values) ; */

            base               = value;
            values[n_values++] = value;

            i += 2;
            /*return -n_values ; */
        }
        else
        {
            fprintf(stderr, "Unknown compression case: %2x, length %d, i=%d\n", data[i], length, i);
            return -n_values; /* exit */
        }
    }
    return n_values;
}

int STV_TXDisplay(void)
{
    int data[] = { TRUE };
    return STV_SendPacket(DISPLAY_ECHO, data, 1);
}

int STV_TerminateTXDisplay(void)
{
    int res;
    int data[] = { FALSE };

    res = STV_SendPacket(DISPLAY_ECHO, data, 1);

    /* Despite the manual says so, I not always see an ACK packet */
    /* So i flush it for the moment */

    tcflush(fd, TCIOFLUSH);
    return res;
}

int STV_FileStatus(int status)
{
    int data[] = { status };
    return STV_SendPacket(FILE_STATUS, data, 1);
}

int STV_DownloadComplete(void)
{
    return STV_SendPacket(DOWNLOAD_COMPLETE, NULL, 0);
}

int STV_Download(void)
{
    return STV_SendPacket(REQUEST_DOWNLOAD, NULL, 0);
}

int STV_DownloadAll(void)
{
    return STV_SendPacket(REQUEST_DOWNLOAD_ALL, NULL, 0);
}

int STV_RequestAck(void)
{
    int data[] = { 0x06, 0x06, 0x06 };
    /* SBIG manual says contains data, which? */
    return STV_SendPacket(REQUEST_ACK, data, 3);
}

int STV_BufferStatus(int buffer)
{
    unsigned char buf[1024];
    int buffers;
    int res;
    int val;

    /*fprintf(stderr, "STV_BufferStatus entering\n") ; */
    usleep(50000);
    if ((res = STV_SendPacket(REQUEST_BUFFER_STATUS, NULL, 0)) < 0)
    {
        fprintf(stderr, "STV_BufferStatus: Error requesting buffer status: %d\n", buffer);
        return -1;
    }
    else
    {
        /*fprintf( stderr, "STV_BufferStatus %2d\n", buffer) ; */
    }

AGAIN:
    if ((res = STV_ReceivePacket(buf, 0)) < 0)
    {
        fprintf(stderr, "STV_BufferStatus: Error reading: %d\n", res);
        return -1;
        ;
    }

    if (buf[1] == REQUEST_BUFFER_STATUS)
    {
        buffers = STV_RecombineInt(buf[6], buf[7]) * 0x10000 + STV_RecombineInt(buf[8], buf[9]);

        if ((val = STV_GetBits(buffers, buffer, 1)) == 1)
        {
            res = 0; /* image present */
        }
        else
        {
            /*fprintf( stderr, "STV_BufferStatus %2d is empty\n", buffer) ; */
            res = 1; /* empty */
        }
    }
    else
    {
        /* The SBIG manual does not specify an ACK, but it is there (at least sometimes) */
        if ((val = STV_CheckAck(buf)) == 0)
        {
            /*fprintf( stderr, "STV_BufferStatus SAW ACK, reading again\n") ; */
            goto AGAIN;
        }
        /* The SBIG manual does not specify the cmd in the answer */
        if (buf[1] != DISPLAY_ECHO)
        {
            fprintf(stderr, "STV_BufferStatus: unexpected cmd byte received %d\n", buf[1]);
            val = STV_RecombineInt(buf[2], buf[3]);
            res = STV_PrintBuffer(buf, 6 + val + 2);
            return -1;
        }
        else
        {
            /* a display packet is silently ignored, there are many of this kind */
            fprintf(stderr, "STV_BufferStatus  DISPLAY_ECHO received, try again\n");
            return -1;
        }
    }
    /* fprintf(stderr, "STV_BufferStatus leaving\n") ; */
    return res;
}
/* Low level communication */
/*  STV_ReceivePacket n_bytes read */
int STV_ReceivePacket(unsigned char *buf, int mode)
{
    int i, j, k;
    int n_bytes = 0;
    int length  = 0;
    int res;
    int trb = 0;
    int pos = 0;
    char display1[25];
    char display2[25];

    j               = 0;
    tracking_buf[0] = 0;

    /*fprintf( stderr,"R") ; */

    while (pos < 6)
    {
        /* Read the header first, calculate length of data */
        /* At higher speeds the data arrives in smaller chunks, assembling the packet */
        if ((trb = read(fd, (char *)(buf + pos), 1)) == -1)
        {
            fprintf(stderr, "Error, %s\n", strerror(errno));
        }

        if (buf[0] == 0xa5)
        {
            if (pos == 5)
            {
                pos++;
                break;
            }
            else
            {
                pos += trb; /* could be zero */
            }
        }
        else
        {
            /* In tracking mode the STV sends raw data (time, brightnes, centroid x,y). */
            /* Under normal operation here appear pieces of a packet. This could happen */
            /* on start up or if something goes wrong. */

            tracking_buf[j++] = buf[pos];
            /*fprintf(stderr, "READ: %d, read %d, pos %d,  0x%2x< %c\n", j, trb, pos, buf[pos], buf[pos]) ;  */
        }
    }

    if (j > 0)
    {
        /* Sometimes the packets are corrupt, e.g. at the very beginning */
        /* Or it is tracking information */

        tracking_buf[j] = 0;

        if (mode == ON)
        { /* Tracking mode */

            fprintf(stderr, "%s\n", tracking_buf);
        }
        else
        {
            for (k = 0; k < j; k++)
            {
                if (!(tracking_buf[k] > 29 && tracking_buf[k] < 127))
                {
                    tracking_buf[k] = 0x20; /* simply */
                }
            }
            fprintf(stderr, "Not a packet: length: %d >%s<\n", j, tracking_buf);
        }
    }

    n_bytes = pos;

    if ((buf[0] == 0xa5) && (pos == 6))
    { /* Check the sanity of the header part */
        if ((res = STV_CheckHeaderSum(buf)) == 0)
        {
            length = (int)buf[3] * 0x100 + (int)buf[2];
            ;

            if (length > 0)
            {
                trb = 0;
                while (pos < 6 + length + 2)
                { /* header, data, check sum */

                    /* At higher speeds the data arrives in smaller chunks, assembling the packet */

                    if ((trb = read(fd, (char *)(buf + pos), (6 + length + 2) - pos)) == -1)
                    {
                        fprintf(stderr, "STV_ReceivePacket: Error reading at serial port, %s\n", strerror(errno));
                        return -1;
                    }
                    pos += trb;
                }
                n_bytes += pos;
                /*fprintf(stderr, "STV_ReceivePacket: LEAVING LOOP length %d, to read %d, read %d, pos %d\n",  length, (6 + length + 2)- pos, trb, pos) ; */
            }
        }
        else
        {
            fprintf(stderr, "STV_ReceivePacket: Header check failed\n");
            return -1;
        }
    }
    else if (pos > 0)
    {
        for (i = 0; i < 6; i++)
        {
            if (buf[i] == 0xa5)
            {
                fprintf(stderr, "STV_ReceivePacket: pos= %d, saw 0xa5 at %d\n", pos, i);
            }
        }
        return -1;
    }
    else
    {
        fprintf(stderr, "STV_ReceivePacket: NO 0xa5 until pos= %d\n", pos);
        return -1;
    }

    if (length > 0)
    { /* Check the sanity of the data */

        if ((res = STV_CheckDataSum(buf)) == -1)
        {
            return -1;
        }
    }
    /* Analyse the display and retrieve the values */
    if (buf[1] == DISPLAY_ECHO)
    {
        for (i = 0; i < 24; i++)
        {
            display1[i] = buf[i + 6];
            if (display1[i] == 0)
            {
                display1[i] = 0x32;
            }
            display2[i] = buf[i + 30];
            if (display2[i] == 0)
            {
                display2[i] = 0x32;
            }
        }
        display1[24] = 0;
        display2[24] = 0;

        /*fprintf(stderr, "STV_ReceivePacket: DISPLAY1 %s<\n", display1) ; */
        /*fprintf(stderr, "STV_ReceivePacket: DISPLAY2 %s<\n", display2) ; */

        /* CCD temperature */
        if ((res = strncmp("CCD Temp.", display2, 9)) == 0)
        {
            float t;
            res                = sscanf(display2, "CCD Temp.          %f", &t);
            di.ccd_temperature = (double)t;
            /*fprintf(stderr, "STV_ReceivePacket: Read from DISPLAY2 %g<\n", di.ccd_temperature ) ; */
        } /* further values can be stored here */
    }
    return n_bytes;
}

int STV_CheckHeaderSum(unsigned char *buf)
{
    int sum    = buf[0] + buf[1] + buf[2] + buf[3]; /* Calculated header sum */
    int sumbuf = STV_RecombineInt(buf[4], buf[5]);  /* STV packet header sum */

    if (buf[0] != 0xa5)
    {
        fprintf(stderr, "STV_CheckHeaderSum:  Wrong start byte, skipping\n");
        return -1;
    }

    if (sum != sumbuf)
    {
        fprintf(stderr, "STV_CheckHeaderSum: NOK: %d==%d\n", sum, sumbuf);
        return -1;
    }
    return 0;
}

int STV_CheckDataSum(unsigned char *buf)
{
    /* *buf points to the beginning of the packet */

    int j, n;
    int sum    = 0;
    int sumbuf = 0;

    n = STV_RecombineInt(buf[2], buf[3]);

    if (n == 0)
    {
        fprintf(stderr, "STV_CheckDataSum: No data present\n");
        return 0;
    }
    sumbuf = STV_RecombineInt(buf[6 + n], buf[6 + n + 1]);

    for (j = 0; j < n; j++)
    {
        sum += (int)buf[6 + j];
    }
    sum = sum & 0xffff;

    if (sum != sumbuf)
    {
        fprintf(stderr, "DATA SUM NOK: %d !=%d\n", sum, sumbuf);
        return -1;
    }
    return 0;
}

int STV_PrintBuffer(unsigned char *buf, int n)
{
    /* For debugging purposes only */
    int i;

    fprintf(stderr, "\nHEADER: %d bytes ", n);
    for (i = 0; i < n; i++)
    {
        if (i == 6)
        {
            fprintf(stderr, "\nDATA  : ");
        }
        fprintf(stderr, "%d:0x%2x<>%c<  >>", i, (unsigned char)buf[i], (unsigned char)buf[i]);
        /*STV_PrintBits((unsigned int) buf[i], 8) ; */
    }
    fprintf(stderr, "\n");

    return 0;
}
int STV_PrintBufferAsText(unsigned char *buf, int n)
{
    /* For debugging purposes only */
    int i;

    fprintf(stderr, "\nHEADER: %d bytes ", n);
    for (i = 0; i < n; i++)
    {
        if (i == 6)
        {
            fprintf(stderr, "\nDATA  : ");
        }
        fprintf(stderr, "%c", (unsigned char)buf[i]);
    }
    fprintf(stderr, "\n");

    return 0;
}
/* Aggregates a STV command packet */

int STV_SendPacket(int cmd, int *data, int n)
{
    char buf[1024];

    int j, l;
    int res;
    int sum; /* check sum */

    /* Header section */
    buf[0] = (unsigned char)0xa5;  /* start byte */
    buf[1] = (unsigned char)cmd;   /* command byte */
    buf[2] = (unsigned char)2 * n; /* data length N (low byte) */
    buf[3] = (unsigned char)0x00;  /* data length N (high byte) */

    sum    = buf[0] + buf[1] + buf[2] + buf[3]; /* header checksum */
    buf[4] = (unsigned char)sum % 0x100;        /* header checksum low byte */
    buf[5] = (unsigned char)(sum / 0x100);      /* header checksum high byte */

    /* DATA section */
    l = 0;
    if (n > 0)
    {
        l = 2; /* Two bytes per value are sent to STV */
        for (j = 0; j < 2 * n; j += 2)
        {
            buf[6 + j] = (unsigned char)(data[j / 2] % 0x100); /* data low byte */
            buf[7 + j] = (unsigned char)(data[j / 2] / 0x100); /* data high byte */
        }

        sum = 0;
        for (j = 0; j < 2 * n; j++)
        {
            if ((int)buf[6 + j] < 0)
            {
                sum = sum + 0xff + (int)buf[6 + j] + 1;
            }
            else
            {
                sum = sum + (int)buf[6 + j];
            }
        }
        buf[6 + 2 * n] = (unsigned char)(sum % 0x10000); /* data checksum (low byte) */
        buf[7 + 2 * n] = (unsigned char)(sum / 0x100);   /* data checksum (high byte) */
    }
    if ((res = STV_CheckHeaderSum((unsigned char *)buf)) != 0)
    { /* Check outgoing packet as well */

        fprintf(stderr, "STV_SendPacket: corrupt header\n");

        if (n > 0)
        {
            if ((res = STV_CheckDataSum((unsigned char *)buf)) != 0)
            {
                fprintf(stderr, "STV_SendPacket: corrupt data\n");
            }
        }
    }
    return STV_portWrite(buf, 8 + l * n);
}

/* Returns 0 or -1 in case of an error */

int STV_portWrite(char *buf, int nbytes)
{
    int bytesWritten = 0;

    /*fprintf( stderr,"w") ; */
    while (nbytes > 0)
    {
        if ((bytesWritten = write(fd, buf, nbytes)) == -1)
        {
            fprintf(stderr, "STV_portWrite: Error writing at serial port, %s\n", strerror(errno));
            /* return -1 ; */
        }

        if (bytesWritten < 0)
        {
            fprintf(stderr, "STV_portWrite: Error writing\n");
            return -1;
        }
        else
        {
            buf += bytesWritten;
            nbytes -= bytesWritten;
        }
    }
    return nbytes;
}

int STV_CheckAck(unsigned char *buf)
{
    /* Watch out for an ACK, for debugging purposes only */
    int i;
    unsigned char ackseq[] = { 0xa5, 0x06, 0x00, 0x00, 0xab, 0x00 };

    for (i = 0; i < 6; i++)
    {
        if (buf[i] != ackseq[i])
        {
            return -1;
        }
    }
    return 0;
}

unsigned int STV_RecombineInt(unsigned char low_byte, unsigned char high_byte)
{
    return (unsigned int)high_byte * (unsigned int)0x100 + (unsigned int)low_byte;
}

unsigned int STV_GetBits(unsigned int x, int p, int n)
{ /* from the C book */

    return (x >> (p + 1 - n)) & ~(~0 << n);
}

void STV_PrintBits(unsigned int x, int n)
{
    /* debugging purposes only */
    int i;
    int res;
    fprintf(stderr, "STV_PrintBits:\n");

    if (n > 8)
    {
        fprintf(stderr, "54321098 76543210\n");
    }
    else
    {
        fprintf(stderr, "76543210\n");
    }

    for (i = n; i > 0; i--)
    {
        if ((i == 8) && (n > 8))
        {
            fprintf(stderr, " ");
        }
        if ((res = STV_GetBits(x, i - 1, 1)) == 0)
        {
            fprintf(stderr, "0");
        }
        else
        {
            fprintf(stderr, "1");
        }
    }
    fprintf(stderr, "\n");
}

double STV_SetCCDTemperature(double set_value)
{
    int i;
    int res;
    int delay = 40000;
    unsigned char buf[1024];
    /* 1st step */
    res = STV_Interrupt(); /* Reset Display */
    tcflush(fd, TCIOFLUSH);
    usleep(100000);

    STV_MenueCCDTemperature(delay);

    for (i = 0; i < 100; i++)
    { /* Change to the highest temperature */
        res = STV_LRRotaryIncrease();
        tcflush(fd, TCIOFLUSH);
        usleep(delay);
    }

    di.ccd_temperature = 25.2; /* The actual value is set in STV_ReceivePacket, needed to enter the while() loop */
    i                  = 0;
    while (set_value < di.ccd_temperature)
    {
        /*fprintf( stderr, "STV_SetCCDTemperature %g %g\n", set_value, di.ccd_temperature) ; */
        res = STV_LRRotaryDecrease(); /* Lower CCD temperature */
        /*fprintf(stderr, ":") ; */
        usleep(10000);
        res = STV_TerminateTXDisplay();
        usleep(10000);
        res = STV_TXDisplay();           /* That's the trick, STV sends at least one display */
        res = STV_ReceivePacket(buf, 0); /* discard it */

        /*STV_PrintBufferAsText( buf, res) ; */
        if ((res != 62) || (i++ > 100))
        { /* why 56 + 6?, 6 + 48 + 6 */
            STV_PrintBuffer(buf, res);
            /*STV_PrintBufferAsText( buf, res) ; */
            return 0.0;
        }
    }
    res = STV_Interrupt();

    return di.ccd_temperature;
}

int STV_MenueCCDTemperature(int delay)
{
    int res;

    res = STV_MenueSetup(delay);
    usleep(delay);
    res = STV_UDRotaryIncrease(); /* Change to CCD Temperature */
    /*usleep( delay) ; */
    tcflush(fd, TCIOFLUSH);
    return 0;
}

int STV_SetDateTime(char *times)
{
    int i;
    int res;
    int turn;
    struct ln_date utm;
    int delay = 20000;
    /*fprintf(stderr, "STV_SetTime\n") ; */

    if ((times == NULL) || ((res = strlen(times)) == 0))
        ln_get_date_from_sys(&utm);
    else
    {
        if (extractISOTime(times, &utm) < 0)
        {
            fprintf(stderr, "Bad time string %s\n", times);
            return -1;
        }
    }
    /* Print out the date and time in the standard format. */
    /*fprintf( stderr, "TIME %s\n", asctime (utc)) ; */

    /* 1st step */
    res = STV_Interrupt(); /* Reset Display */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    res = STV_MenueDateTime(delay);
    usleep(delay);
    res = STV_MenueDateTime(delay); /* This not an error */
    usleep(delay);

    for (i = 0; i < 13; i++)
    { /* Reset Month menu to the lef most position */
        res = STV_LRRotaryDecrease();
        usleep(delay);
    }

    for (i = 0; i < utm.months; i++)
    { /* Set Month menu */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    for (i = 0; i < 32; i++)
    { /* Reset Day menu to the lef most position */
        res = STV_LRRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }

    for (i = 0; i < utm.days - 1; i++)
    { /* Set Day menu -1? */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    for (i = 0; i < 128; i++)
    { /* Reset Year menu to the lef most position, ATTENTION */
        res = STV_LRRotaryDecrease();

        usleep(delay); /* sleep a 1/100 second */
        tcflush(fd, TCIOFLUSH);
    }

    /* JM: Is this how you want not? Please verify the code! */
    int ymenu = utm.years % 100;

    for (i = 0; i < ymenu; i++)
    { /* Set Year menu */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    for (i = 0; i < 25; i++)
    { /* Reset Hour menu to the lef most position, ATTENTION */
        res = STV_LRRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    for (i = 0; i < utm.hours; i++)
    { /* Set Hour menu */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    for (i = 0; i < 61; i++)
    { /* Reset Minute menu to the lef most position, ATTENTION */
        res = STV_LRRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    for (i = 0; i < utm.minutes; i++)
    { /* Set Minute menu */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    tcflush(fd, TCIOFLUSH);
    for (i = 0; i < 5; i++)
    { /* Reset Seconds menu to the lef most position, ATTENTION */
        res = STV_LRRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }

    if (utm.seconds < 15)
    {
        turn = 0;
    }
    else if (utm.seconds < 30)
    {
        turn = 1;
    }
    else if (utm.seconds < 45)
    {
        turn = 2;
    }
    else
    {
        turn = 3;
    }

    for (i = 0; i < turn; i++)
    { /* Set Seconds menu, steps of 15 seconds */
        res = STV_LRRotaryIncrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    res = STV_AKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);

    res = STV_BKey(); /* Press the Parameter button */
    usleep(delay);
    tcflush(fd, TCIOFLUSH);
    res = STV_Interrupt();
    return 0;
}

int STV_MenueDateTime(int delay)
{
    int i;
    int res;

    res = STV_MenueSetup(delay);
    usleep(delay);
    res = STV_BKey(); /* Press the Value button */
    usleep(delay);

    for (i = 0; i < 8; i++)
    { /* Reset Date menu to the lef most position */

        res = STV_UDRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    return 0;
}

int STV_MenueSetup(int delay)
{
    int i;
    int res;

    res = STV_Setup(); /* Change to Setup */
    usleep(delay);

    for (i = 0; i < 16; i++)
    { /* Reset Setup menu to the lef most position */
        res = STV_UDRotaryDecrease();
        usleep(delay);
        tcflush(fd, TCIOFLUSH);
    }
    return 0;
}

int STV_Connect(char *device, int baud)
{
    /*fprintf( stderr, "STV_Connect\n") ; */
    if ((fd = init_serial(device, baud, 8, 0, 1)) == -1)
    {
        fprintf(stderr, "Error on port %s, %s\n", device, strerror(errno));
        return -1;
    }

    return fd;
}

/******************************************************************************
 * shutdown_serial(..)
 ******************************************************************************
 * Restores terminal settings of open serial port device and close the file.
 * Arguments:
 *   fd: file descriptor of open serial port device.
 *****************************************************************************/
void shutdown_serial(int fd)
{
    if (fd > 0)
    {
        if (tcsetattr(fd, TCSANOW, &orig_tty_setting) < 0)
        {
            perror("shutdown_serial: can't restore serial device's terminal settings.");
        }
        close(fd);
    }
}
/******************************************************************************
 * init_serial(..)
 ******************************************************************************
 * Opens and initializes a serial device and returns it's file descriptor.
 * Arguments:
 *   device_name : device name string of the device to open (/dev/ttyS0, ...)
 *   bit_rate    : bit rate
 *   word_size   : number of data bits, 7 or 8, USE 8 DATA BITS with modbus
 *   parity      : 0=no parity, 1=parity EVEN, 2=parity ODD
 *   stop_bits   : number of stop bits : 1 or 2
 * Return:
 *   file descriptor  of successfully opened serial device
 *   or -1 in case of error.
 *****************************************************************************/
int init_serial(char *device_name, int bit_rate, int word_size, int parity, int stop_bits)
{
    int fd;
    char *msg;

    /* open serial device */
    fd = open(device_name, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        if (asprintf(&msg, "init_serial: open %s failed", device_name) < 0)
            perror(NULL);
        else
            perror(msg);

        return -1;
    }

    /* save current tty settings */
    if (tcgetattr(fd, &orig_tty_setting) < 0)
    {
        perror("init_serial: can't get terminal parameters.");
        return -1;
    }

    /* Control Modes */
    /* Set bps rate */
    int bps;
    switch (bit_rate)
    {
        case 0:
            bps = B0;
            break;
        case 50:
            bps = B50;
            break;
        case 75:
            bps = B75;
            break;
        case 110:
            bps = B110;
            break;
        case 134:
            bps = B134;
            break;
        case 150:
            bps = B150;
            break;
        case 200:
            bps = B200;
            break;
        case 300:
            bps = B300;
            break;
        case 600:
            bps = B600;
            break;
        case 1200:
            bps = B1200;
            break;
        case 1800:
            bps = B1800;
            break;
        case 2400:
            bps = B2400;
            break;
        case 4800:
            bps = B4800;
            break;
        case 9600:
            bps = B9600;
            break;
        case 19200:
            bps = B19200;
            break;
        case 38400:
            bps = B38400;
            break;
        case 57600:
            bps = B57600;
            break;
        case 115200:
            bps = B115200;
            break;
        case 230400:
            bps = B230400;
            break;
        default:
            if (asprintf(&msg, "init_serial: %d is not a valid bit rate.", bit_rate) < 0)
                perror(NULL);
            else
                perror(msg);
            return -1;
    }
    if ((cfsetispeed(&tty_setting, bps) < 0) || (cfsetospeed(&tty_setting, bps) < 0))
    {
        perror("init_serial: failed setting bit rate.");
        return -1;
    }

    /* Control Modes */
    /* set no flow control word size, parity and stop bits. */
    /* Also don't hangup automatically and ignore modem status. */
    /* Finally enable receiving characters. */
    tty_setting.c_cflag &= ~(CSIZE | CSTOPB | PARENB | PARODD | HUPCL | CRTSCTS);
    /*  #ifdef CBAUDEX */
    /*tty_setting.c_cflag &= ~(CBAUDEX); */
    /*#endif */
    /*#ifdef CBAUDEXT */
    /*tty_setting.c_cflag &= ~(CBAUDEXT); */
    /*#endif */
    tty_setting.c_cflag |= (CLOCAL | CREAD);

    /* word size */
    switch (word_size)
    {
        case 5:
            tty_setting.c_cflag |= CS5;
            break;
        case 6:
            tty_setting.c_cflag |= CS6;
            break;
        case 7:
            tty_setting.c_cflag |= CS7;
            break;
        case 8:
            tty_setting.c_cflag |= CS8;
            break;
        default:

            fprintf(stderr, "Default\n");
            if (asprintf(&msg, "init_serial: %d is not a valid data bit count.", word_size) < 0)
                perror(NULL);
            else
                perror(msg);

            return -1;
    }

    /* parity */
    switch (parity)
    {
        case PARITY_NONE:
            break;
        case PARITY_EVEN:
            tty_setting.c_cflag |= PARENB;
            break;
        case PARITY_ODD:
            tty_setting.c_cflag |= PARENB | PARODD;
            break;
        default:

            fprintf(stderr, "Default1\n");
            if (asprintf(&msg, "init_serial: %d is not a valid parity selection value.", parity) < 0)
                perror(NULL);
            else
                perror(msg);

            return -1;
    }

    /* stop_bits */
    switch (stop_bits)
    {
        case 1:
            break;
        case 2:
            tty_setting.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr, "Default2\n");
            if (asprintf(&msg, "init_serial: %d is not a valid stop bit count.", stop_bits) < 0)
                perror(NULL);
            else
                perror(msg);

            return -1;
    }
    /* Control Modes complete */

    /* Ignore bytes with parity errors and make terminal raw and dumb. */
    tty_setting.c_iflag &= ~(PARMRK | ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | IXANY);
    tty_setting.c_iflag |= INPCK | IGNPAR | IGNBRK;

    /* Raw output. */
    tty_setting.c_oflag &= ~(OPOST | ONLCR);

    /* Local Modes */
    /* Don't echo characters. Don't generate signals. */
    /* Don't process any characters. */
    tty_setting.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN | NOFLSH | TOSTOP);
    tty_setting.c_lflag |= NOFLSH;

    /* blocking read until 1 char arrives */
    tty_setting.c_cc[VMIN]  = 1;
    tty_setting.c_cc[VTIME] = 0;

    /* now clear input and output buffers and activate the new terminal settings */
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty_setting))
    {
        perror("init_serial: failed setting attributes on serial port.");
        shutdown_serial(fd);
        return -1;
    }
    return fd;
}
