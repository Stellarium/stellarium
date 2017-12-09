#if 0
    STV Driver
    Copyright (C) 2006 Markus Wildi, markus.wildi@datacomm.ch

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

/* Standard headers */

#include <cstring>
#include <sys/stat.h>
#include <stdio.h>
#include <zlib.h>
#include <unistd.h>

#ifndef _WIN32
#include <termios.h>
#endif

/* INDI Core headers */

#include "indidevapi.h"

/* INDI Eventloop mechanism */

#include "eventloop.h"

/* INDI Common Routines/RS232 */

#include "indicom.h"

/* Config parameters */
#include <config.h>

/* Fits */

#include <fitsio.h>

/* STV's definitions */

#include "stvdriver.h"

/* Definitions */

#define mydev            "STV Guider"        /* Device name */
#define CONNECTION_GROUP "Connection"        /* Group name */
#define SETTINGS_GROUP   "Setings"           /* Group name */
#define BUTTONS_GROUP    "Buttons and Knobs" /* Button Pannel */
#define IMAGE_GROUP      "Download"          /* Button Pannel */

#define currentBuffer BufferN[0].value
#define currentX      WindowingN[0].value
#define currentY      WindowingN[1].value
#define currentLines  WindowingN[2].value
#define currentLength WindowingN[3].value

#define currentCompression CompressionS[0].s

static int compression = OFF;
static int acquiring   = OFF;
static int guiding     = OFF;
static int processing  = OFF;

/* Fits (fli_ccd project) */
enum STVFrames
{
    LIGHT_FRAME = 0,
    BIAS_FRAME,
    DARK_FRAME,
    FLAT_FRAME
};
#define TEMPFILE_LEN 16

/* Image (fli_ccd project)*/

typedef struct
{
    int width;
    int height;
    int frameType;
    int expose;
    unsigned short *img;
} img_t;

static img_t *STVImg;

/* Function adapted from fli_ccd project */

void addFITSKeywords(fitsfile *fptr, IMAGE_INFO *image_info);
int writeFITS(const char *filename, IMAGE_INFO *image_info, char errmsg[]);
void uploadFile(const char *filename);

/* File descriptor and call back id */
int fd;
static int cb = -1;
char tracking_buf[1024];

/* Function prototypes */
int ISTerminateTXDisplay(void);
int ISRestoreTXDisplay(void);
int ISMessageImageInfo(int buffer, IMAGE_INFO *image_info);
int ISRequestImageData(int compression, int buffer, int x_offset, int y_offset, int length, int lines);

int STV_LRRotaryDecrease(void);
int STV_LRRotaryIncrease(void);
int STV_UDRotaryDecrease(void);
int STV_UDRotaryIncrease(void);

int STV_AKey(void);
int STV_BKey(void);
int STV_Setup(void);
int STV_Interrupt(void);
int STV_Focus(void);
int STV_Image(void);
int STV_Monitor(void);
int STV_Calibrate(void);
int STV_Track(void);
int STV_Display(void);
int STV_FileOps(void);
int STV_RequestImageInfo(int imagebuffer, IMAGE_INFO *image_info);
int STV_BufferStatus(int buffer);
int STV_RequestImage(int compression, int buffer, int x_offset, int y_offset, int *length, int *lines, int image[][320],
                     IMAGE_INFO *image_info);
int STV_Download(void);
int STV_TXDisplay(void);
int STV_TerminateTXDisplay(void);
int STV_RequestAck(void);
unsigned int STV_GetBits(unsigned int x, int p, int n);
int STV_PrintBuffer(unsigned char *cmdbuf, int n);
void handleError(ISwitchVectorProperty *svp, int err, const char *msg);
static void ISInit();
void ISCallBack(void);

int init_serial(char *device_name, int bit_rate, int word_size, int parity, int stop_bits);
int STV_ReceivePacket(unsigned char *buf, int mode);
int STV_Connect(char *device, int baud);
int STV_SetDateTime(char *times);
double STV_SetCCDTemperature(double set_value);

static IText StatusT[] = {
    { "STATUS", "This driver", "is experimental, contact markus.wildi@datacomm.ch", 0, 0, 0 },
};

static ITextVectorProperty StatusTP = { mydev,           "STAUS",     "Status", CONNECTION_GROUP,
                                        IP_RO,           ISR_1OFMANY, IPS_IDLE, StatusT,
                                        NARRAY(StatusT), "",          0 };

/* RS 232 Connection */

static ISwitch PowerS[] = {
    { "CONNECT", "Connect", ISS_OFF, 0, 0 },
    { "DISCONNECT", "Disconnect", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty PowerSP = { mydev, "CONNECTION", "Connection", CONNECTION_GROUP, IP_RW, ISR_1OFMANY,
                                         0,     IPS_IDLE,     PowerS,       NARRAY(PowerS),   "",    0 };

/* Serial Port */

static IText PortT[] = { { "PORT", "Port", NULL, 0, 0, 0 }, { "SPEED", "Speed", NULL, 0, 0, 0 } };

static ITextVectorProperty PortTP = {
    mydev, "DEVICE_PORT", "Port", CONNECTION_GROUP, IP_RW, ISR_1OFMANY, IPS_IDLE, PortT, NARRAY(PortT), "", 0
};

static ISwitch TXDisplayS[] = {

    { "1", "On", ISS_ON, 0, 0 },
    { "2", "Off", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty TXDisplaySP = {
    mydev, "Update Display", "Update Display", CONNECTION_GROUP,   IP_RW, ISR_1OFMANY,
    0,     IPS_IDLE,         TXDisplayS,       NARRAY(TXDisplayS), "",    0
};

static IText DisplayCT[] = {

    { "DISPLAYC1", "Line 1", NULL, 0, 0, 0 },
    { "DISPLAYC2", "Line 2", NULL, 0, 0, 0 }
};

static ITextVectorProperty DisplayCTP = {
    mydev, "DISPLAYC", "Display", CONNECTION_GROUP, IP_RO, ISR_1OFMANY, IPS_IDLE, DisplayCT, NARRAY(DisplayCT), "", 0
};

static IText DisplayBT[] = {

    { "DISPLAYB1", "Line 1", NULL, 0, 0, 0 },
    { "DISPLAYB2", "Line 2", NULL, 0, 0, 0 }
};

static ITextVectorProperty DisplayBTP = {
    mydev, "DISPLAYB", "Display", BUTTONS_GROUP, IP_RO, ISR_1OFMANY, IPS_IDLE, DisplayBT, NARRAY(DisplayBT), "", 0
};

static IText DisplayDT[] = {

    { "DISPLAYD1", "Line 1", NULL, 0, 0, 0 },
    { "DISPLAYD2", "Line 2", NULL, 0, 0, 0 }
};

static ITextVectorProperty DisplayDTP = { mydev,    "DISPLAYD", "Display",         IMAGE_GROUP, IP_RO, ISR_1OFMANY,
                                          IPS_IDLE, DisplayDT,  NARRAY(DisplayDT), "",          0 };

/* Setings */

static IText UTCT[]       = { { "UTC", "UTC", NULL, 0, 0, 0 } };
ITextVectorProperty UTCTP = { mydev,        "TIME_UTC", "UTC Time", SETTINGS_GROUP, IP_RW, 0, IPS_IDLE, UTCT,
                              NARRAY(UTCT), "",         0 };

static INumber SetCCDTemperatureN[] = {
    { "TEMPERATURE", "Cel. -55.1, +25.2", "%6.1f", -55.8, 25.2, 0., 16., 0, 0, 0 },

};

static INumberVectorProperty SetCCDTemperatureNP = { mydev,
                                                     "CCD_TEMPERATURE",
                                                     "CCD Temperature",
                                                     SETTINGS_GROUP,
                                                     IP_RW,
                                                     ISR_1OFMANY,
                                                     IPS_IDLE,
                                                     SetCCDTemperatureN,
                                                     NARRAY(SetCCDTemperatureN),
                                                     "",
                                                     0 };

/* Buttons */
static ISwitch ControlS[] = {

    { "1", "Parameter", ISS_OFF, 0, 0 },
    { "2", "Increase", ISS_OFF, 0, 0 },
    { "3", "Decrease", ISS_OFF, 0, 0 },

};

static ISwitchVectorProperty ControlSP = { mydev, "ParaButtons", "Control", BUTTONS_GROUP,    IP_RW, ISR_1OFMANY,
                                           0,     IPS_IDLE,      ControlS,  NARRAY(ControlS), "",    0 };

static ISwitch ValueS[] = {

    { "1", "Value", ISS_OFF, 0, 0 },
    { "2", "Increase", ISS_OFF, 0, 0 },
    { "3", "Decrease", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty ValueSP = { mydev, "ValueButtons", "Control", BUTTONS_GROUP,  IP_RW, ISR_1OFMANY,
                                         0,     IPS_IDLE,       ValueS,    NARRAY(ValueS), "",    0 };

static ISwitch AuxiliaryS[] = {

    { "1", "Setup", ISS_OFF, 0, 0 },
    { "2", "Interrupt", ISS_OFF, 0, 0 },

};

static ISwitchVectorProperty AuxiliarySP = { mydev, "Auxilliary", "",         BUTTONS_GROUP,      IP_RW, ISR_1OFMANY,
                                             0,     IPS_IDLE,     AuxiliaryS, NARRAY(AuxiliaryS), "",    0 };

static ISwitch AcquireS[] = {

    { "1", "Focus", ISS_OFF, 0, 0 },
    { "2", "Image", ISS_OFF, 0, 0 },
    { "3", "Monitor", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty AcquireSP = { mydev, "Acquire", "",       BUTTONS_GROUP,    IP_RW, ISR_1OFMANY,
                                           0,     IPS_IDLE,  AcquireS, NARRAY(AcquireS), "",    0 };

static ISwitch GuideS[] = {

    { "1", "Calibrate", ISS_OFF, 0, 0 },
    { "2", "Track", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty GuideSP = { mydev, "Guide",  "",     BUTTONS_GROUP,  IP_RW, ISR_1OFMANY,
                                         0,     IPS_IDLE, GuideS, NARRAY(GuideS), "",    0 };

static ISwitch ProcessS[] = {

    { "1", "Display/Crosshairs", ISS_OFF, 0, 0 },
    { "2", "File Ops", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty ProcessSP = { mydev, "Process", "",       BUTTONS_GROUP,    IP_RW, ISR_1OFMANY,
                                           0,     IPS_IDLE,  ProcessS, NARRAY(ProcessS), "",    0 };

static ISwitch CompressionS[] = {

    { "1", "On", ISS_OFF, 0, 0 },
    { "2", "Off", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty CompressionSP = { mydev,        "Compression",        "", IMAGE_GROUP,
                                               IP_RW,        ISR_1OFMANY,          0,  IPS_IDLE,
                                               CompressionS, NARRAY(CompressionS), "", 0 };

static ISwitch BufferStatusS[] = {

    { "1", "Status", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty BufferStatusSP = { mydev,       "Buffers", "",       IMAGE_GROUP,   IP_RW,
                                                ISR_1OFMANY, 0,         IPS_IDLE, BufferStatusS, NARRAY(BufferStatusS),
                                                "",          0 };

static INumber BufferN[] = {
    { "A0", "Number 1 - 32", "%6.0f", 1., 32., 0., 32., 0, 0, 0 },
};

static INumberVectorProperty BufferNP = { mydev,    "BUFFER_Number", "Buffer",        IMAGE_GROUP, IP_RW, ISR_1OFMANY,
                                          IPS_IDLE, BufferN,         NARRAY(BufferN), "",          0 };

static INumber WindowingN[] = {

    { "X", "Offset x", "%6.0f", 0., 199., 0., 0., 0, 0, 0 },
    { "Y", "Offset y", "%6.0f", 0., 319., 0., 0., 0, 0, 0 },
    { "HEIGHT", "Lines", "%6.0f", 1., 200., 0., 200., 0, 0, 0 },
    { "WIDTH", "Length", "%6.0f", 1., 320., 0., 320., 0, 0, 0 },
};

static INumberVectorProperty WindowingNP = { mydev,    "CCD_FRAME", "Windowing",        IMAGE_GROUP, IP_RW, ISR_1OFMANY,
                                             IPS_IDLE, WindowingN,  NARRAY(WindowingN), "",          0 };

static ISwitch ImageInfoS[] = {

    { "1", "One Image", ISS_OFF, 0, 0 },
    { "2", "All Images", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty ImageInfoSP = { mydev, "Information", "",         IMAGE_GROUP,        IP_RW, ISR_1OFMANY,
                                             0,     IPS_IDLE,      ImageInfoS, NARRAY(ImageInfoS), "",    0 };

static ISwitch DownloadS[] = {

    { "1", "One Image", ISS_OFF, 0, 0 },
    { "2", "All Images", ISS_OFF, 0, 0 },
};

static ISwitchVectorProperty DownloadSP = { mydev, "Download", "",        IMAGE_GROUP,       IP_RW, ISR_1OFMANY,
                                            0,     IPS_IDLE,   DownloadS, NARRAY(DownloadS), "",    0 };

/* BLOB for sending image */

static IBLOB imageB = { "CCD1", "Image", "", 0, 0, 0, 0, 0, 0, 0 };

static IBLOBVectorProperty imageBP = { mydev, "Image", "Image", IMAGE_GROUP, IP_RO, 0, IPS_IDLE, &imageB, 1, "", 0 };

/* Initlization routine */

static void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;

    IUSaveText(&PortT[0], "/dev/ttyUSB0");
    IUSaveText(&PortT[1], "115200");

    if ((DisplayCT[0].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "3:Memory allocation error");
        return;
    }
    if ((DisplayCT[1].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "4:Memory allocation error");
        return;
    }
    if ((DisplayBT[0].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "5:Memory allocation error");
        return;
    }
    if ((DisplayBT[1].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "5:Memory allocation error");
        return;
    }
    if ((DisplayDT[0].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "7:Memory allocation error");
        return;
    }
    if ((DisplayDT[1].text = malloc(1024)) == NULL)
    {
        fprintf(stderr, "8:Memory allocation error");
        return;
    }

    if ((STVImg = malloc(sizeof(img_t))) == NULL)
    {
        fprintf(stderr, "9:Memory allocation error");
        return;
    }

    isInit = 1;
}

void ISResetButtons(char *message)
{
    ControlSP.s = IPS_IDLE;
    IUResetSwitch(&ControlSP);
    IDSetSwitch(&ControlSP, NULL);

    ValueSP.s = IPS_IDLE;
    IUResetSwitch(&ValueSP);
    IDSetSwitch(&ValueSP, NULL);

    AuxiliarySP.s = IPS_IDLE;
    IUResetSwitch(&AuxiliarySP);
    IDSetSwitch(&AuxiliarySP, NULL);

    AcquireSP.s = IPS_IDLE;
    IUResetSwitch(&AcquireSP);
    IDSetSwitch(&AcquireSP, NULL);

    GuideSP.s = IPS_IDLE;
    IUResetSwitch(&GuideSP);
    IDSetSwitch(&GuideSP, NULL);

    ProcessSP.s = IPS_IDLE;
    IUResetSwitch(&ProcessSP);
    IDSetSwitch(&ProcessSP, NULL);

    ImageInfoSP.s = IPS_IDLE;
    IUResetSwitch(&ImageInfoSP);
    IDSetSwitch(&ImageInfoSP, NULL);

    BufferStatusSP.s = IPS_IDLE;
    IUResetSwitch(&BufferStatusSP);
    IDSetSwitch(&BufferStatusSP, NULL);

    /*   SP.s= IPS_IDLE ; */
    /*   IUResetSwitch(&SP); */
    /*   IDSetSwitch(&SP, NULL); */

    DownloadSP.s = IPS_IDLE;
    IUResetSwitch(&DownloadSP);

    IDSetSwitch(&DownloadSP, "%s", message);

    return;
}

/* This function is called when ever the file handle fd provides data */
void ISCallBack()
{
    int res;
    int k, l, m;

    unsigned char buf[1024];

    IERmCallback(cb);
    cb = -1;

    /*   fprintf( stderr, "ISCallBack\n") ; */
    /*   if(( counter++ % 4) ==0){ */
    /*   fprintf( stderr, ".") ; */
    /*   } */

    if (PowerS[0].s == ISS_ON)
    {
        res = STV_ReceivePacket(buf, guiding);

        /*     res= STV_PrintBuffer(buf,res) ; */

        DisplayCTP.s = IPS_IDLE;
        IDSetText(&DisplayCTP, NULL);
        DisplayBTP.s = IPS_IDLE;
        IDSetText(&DisplayBTP, NULL);
        DisplayDTP.s = IPS_IDLE;
        IDSetText(&DisplayDTP, NULL);

        switch ((int)buf[1])
        { /* STV cmd byte */

            case DISPLAY_ECHO:

                if (res < 0)
                {
                    DisplayCTP.s = IPS_ALERT;
                    IDSetText(&DisplayCTP, NULL);
                    DisplayBTP.s = IPS_ALERT;
                    IDSetText(&DisplayBTP, NULL);
                    DisplayDTP.s = IPS_ALERT;
                    IDSetText(&DisplayDTP, NULL);
                    IDMessage(mydev, "Error while reading, continuing\n");
                }
                else
                {
                    l = 0;
                    m = 0;
                    /* replace unprintable characters and format the string */
                    for (k = 0; k < 24; k++)
                    {
                        if (buf[k + 6] == 0x5e)
                        { /* first line */

                            DisplayCT[0].text[l - 1] = 0x50; /* P */
                            DisplayCT[0].text[l++]   = 0x6b; /* k */
                        }
                        else if (buf[k + 6] == 0xd7)
                        {
                            DisplayCT[0].text[l++] = 0x28; /* "(x,y) " */
                            DisplayCT[0].text[l++] = 0x78;
                            DisplayCT[0].text[l++] = 0x2c;
                            DisplayCT[0].text[l++] = 0x79;
                            DisplayCT[0].text[l++] = 0x29;
                            DisplayCT[0].text[l++] = 0x20;
                        }
                        else if (buf[k + 6] > 29 && buf[k + 6] < 127)
                        {
                            DisplayCT[0].text[l++] = buf[k + 6];
                        }
                        else
                        {
                            /* fprintf(stderr, "LINE 1%2x, %2x, %2x, %c %c %c\n", buf[k+ 5], buf[k+ 6], buf[k+ 7], buf[k+ 5], buf[k+ 6], buf[k+ 7]) ; */
                            DisplayCT[0].text[l++] = 0x20;
                        }
                        if (buf[k + 30] == 0xb0)
                        { /* second line */

                            DisplayCT[1].text[m++] = 0x43; /* Celsius */
                        }
                        else if (buf[k + 30] > 29 && buf[k + 30] < 127)
                        {
                            DisplayCT[1].text[m++] = buf[k + 30];
                        }
                        else
                        {
                            /* fprintf(stderr, "LINE 2 %2x, %2x, %2x, %c %c %c\n", buf[k+ 29], buf[k+ 30], buf[k+ 31], buf[k+ 29], buf[k+ 30], buf[k+ 31]) ; */
                            DisplayCT[1].text[m++] = 0x20;
                        }
                    }
                    DisplayCT[0].text[l] = 0;
                    DisplayCT[1].text[m] = 0;

                    strcpy(DisplayBT[0].text, DisplayCT[0].text);
                    strcpy(DisplayBT[1].text, DisplayCT[1].text);
                    strcpy(DisplayDT[0].text, DisplayCT[0].text);
                    strcpy(DisplayDT[1].text, DisplayCT[1].text);

                    DisplayCTP.s = IPS_OK;
                    IDSetText(&DisplayCTP, NULL);
                    DisplayBTP.s = IPS_OK;
                    IDSetText(&DisplayBTP, NULL);
                    DisplayDTP.s = IPS_OK;
                    IDSetText(&DisplayDTP, NULL);
                }
                break;
            case REQUEST_DOWNLOAD:

                /* fprintf(stderr, "STV says REQUEST_DOWNLOAD\n") ; */

                if (TXDisplayS[0].s == ISS_ON)
                {
                    res = STV_Download();

                    imageB.blob    = NULL;
                    imageB.bloblen = 0;
                    imageB.size    = 0;

                    imageBP.s = IPS_ALERT;
                    IDSetBLOB(&imageBP, NULL);

                    IDMessage(mydev, "Switch off display read out manually first (Update Display: Off\n)");
                }
                else
                {
                    tcflush(fd, TCIOFLUSH);
                    usleep(100000);

                    res = ISRequestImageData(1, 31, 0, 0, 320, 200); /* Download the on screen image (buffer 32 -1) */
                }
                /*fprintf(stderr, "STV END REQUEST_DOWNLOAD\n") ;  */
                break;
            case REQUEST_DOWNLOAD_ALL:

                IDMessage(mydev, "REQUEST_DOWNLOAD_ALL initiated at the STV not implemented");
                break;
            case ACK:

                if (cb == -1)
                {
                    strcpy(DisplayCT[0].text, "Key press acknowledged");
                    strcpy(DisplayBT[0].text, DisplayCT[0].text);
                    strcpy(DisplayDT[0].text, DisplayCT[0].text);

                    DisplayCTP.s = IPS_OK;
                    IDSetText(&DisplayCTP, NULL);
                    DisplayBTP.s = IPS_OK;
                    IDSetText(&DisplayBTP, NULL);
                    DisplayDTP.s = IPS_OK;
                    IDSetText(&DisplayDTP, NULL);
                }
                break;
            case NACK:

                /*fprintf(stderr, "STV says NACK!!") ;   */
                IDMessage(mydev, "STV says NACK!");
                break;
            case REQUEST_BUFFER_STATUS:

                IDMessage(mydev, "Request Buffer status seen, ignoring\n");
                break;
            default:

                if (guiding == ON)
                { /* While STV is tracking, it send time, brightnes, centroid x,y */

                    IDMessage(mydev, "Tracking: %s", tracking_buf);
                }
                else
                {
                    /*fprintf(stderr, "STV ISCallBack: Unknown response 0x%2x\n",  buf[1]) ;  */
                    IDLog("ISCallBack: Unknown response 0x%2x\n", buf[1]);
                }
                break;
        }
    }
    cb = IEAddCallback(fd, (IE_CBF *)ISCallBack, NULL);
}

/* Client retrieves properties */
void ISGetProperties(const char *dev)
{
    /* #1 Let's make sure everything has been initialized properly */
    ISInit();
    /* #2 Let's make sure that the client is asking for the properties of our device, otherwise ignore */
    if (dev != nullptr && strcmp(mydev, dev))
        return;

    /* #3 Tell the client to create new properties */

    /* Connection tab */

    IDDefText(&DisplayCTP, NULL);
    IDDefSwitch(&PowerSP, NULL);
    IDDefText(&PortTP, NULL);
    IDDefSwitch(&TXDisplaySP, NULL);
    IDDefText(&StatusTP, NULL);

    /* Settings Tab */
    IDDefText(&UTCTP, NULL);
    IDDefNumber(&SetCCDTemperatureNP, NULL);

    /* Buttons tab */
    IDDefText(&DisplayBTP, NULL);
    IDDefSwitch(&ControlSP, NULL);
    IDDefSwitch(&ValueSP, NULL);
    IDDefSwitch(&AuxiliarySP, NULL);
    IDDefSwitch(&AcquireSP, NULL);
    IDDefSwitch(&GuideSP, NULL);
    IDDefSwitch(&ProcessSP, NULL);

    /* Image tab */
    IDDefText(&DisplayDTP, NULL);
    /* "direct" read out does not work well IDDefSwitch(&BufferStatusSP, NULL) ; */
    IDDefSwitch(&BufferStatusSP, NULL);
    IDDefSwitch(&ImageInfoSP, NULL);
    IDDefNumber(&BufferNP, NULL);

    IDDefSwitch(&DownloadSP, NULL);
    IDDefSwitch(&CompressionSP, NULL);
    IDDefNumber(&WindowingNP, NULL);
    IDDefBLOB(&imageBP, NULL);
}

/* Client sets new switch */
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int i, j;
    int res = 0;
    int baud;
    IMAGE_INFO image_info;
    ISwitch *sp;
    int lower_buffer = 0;
    int upper_buffer = 0;

    /*fprintf(stderr, "ISNewSwitch\n") ; */

    /* #1 Let's make sure everything has been initialized properly */

    ISInit();

    /* #2 Let's make sure that the client is asking to update the properties of our device, otherwise ignore */

    if (dev != nullptr && strcmp(dev, mydev))
        return;

    /* #3 Now let's check if the property the client wants to change is the PowerSP (name: CONNECTION) property*/

    if (!strcmp(name, PowerSP.name))
    {
        /* A. We reset all switches (in this case CONNECT and DISCONNECT) to ISS_OFF */

        IUResetSwitch(&PowerSP);

        /* B. We update the switches by sending their names and updated states IUUpdateSwitch function */

        IUUpdateSwitch(&PowerSP, states, names, n);

        /* C. We try to establish a connection to our device or terminate it*/

        int res = 0;
        switch (PowerS[0].s)
        {
            case ISS_ON:

                if ((res = strcmp("9600", PortT[1].text)) == 0)
                {
                    baud = 9600;
                }
                else if ((res = strcmp("19200", PortT[1].text)) == 0)
                {
                    baud = 19200;
                }
                else if ((res = strcmp("38400", PortT[1].text)) == 0)
                {
                    baud = 38400;
                }
                else if ((res = strcmp("57600", PortT[1].text)) == 0)
                {
                    baud = 57600;
                }
                else if ((res = strcmp("115200", PortT[1].text)) == 0)
                {
                    baud = 115200;
                }
                else
                {
                    IUSaveText(&PortT[1], "9600");
                    IDSetText(&PortTP, "Wrong RS 232 value: %s, defaulting to 9600 baud", PortT[1].text);
                    return;
                }

                if ((fd = STV_Connect(PortT[0].text, baud)) == -1)
                {
                    PowerSP.s = IPS_ALERT;
                    IUResetSwitch(&PowerSP);
                    IDSetSwitch(&PowerSP, "Error connecting to port %s", PortT[0].text);

                    return;
                }
                else
                {
                    cb = IEAddCallback(fd, (IE_CBF *)ISCallBack, NULL);

                    /* The SBIG manual says one can request an ACK, never saw it, even not on a RS 232 tester */
                    if ((res = STV_RequestAck()) != 0)
                    {
                        fprintf(stderr, "COULD not write an ACK\n");
                    }
                    /* Second trial: start reading out the display  */
                    if ((res = STV_TXDisplay()) != 0)
                    {
                        fprintf(stderr, "STV: Could not write  %d\n", res);
                        return;
                    }
                }

                PowerSP.s = IPS_OK;
                IDSetSwitch(&PowerSP, "STV is online, port: %s, baud rate: %s", PortT[0].text, PortT[1].text);

                PortTP.s = IPS_OK;
                IDSetText(&PortTP, NULL);

                break;

            case ISS_OFF:

                IERmCallback(cb);
                cb = -1;

                /* Close the serial port */

                tty_disconnect(fd);

                ISResetButtons(NULL);

                GuideSP.s = IPS_IDLE;
                IUResetSwitch(&GuideSP);
                IDSetSwitch(&GuideSP, NULL);

                TXDisplaySP.s = IPS_IDLE;
                IUResetSwitch(&TXDisplaySP);
                IDSetSwitch(&TXDisplaySP, NULL);

                DisplayCTP.s = IPS_IDLE;
                IDSetText(&DisplayCTP, NULL);

                DisplayBTP.s = IPS_IDLE;
                IDSetText(&DisplayBTP, NULL);

                DisplayDTP.s = IPS_IDLE;
                IDSetText(&DisplayDTP, NULL);

                PortTP.s = IPS_IDLE;
                IDSetText(&PortTP, NULL);

                imageB.blob    = NULL;
                imageB.bloblen = 0;
                imageB.size    = 0;

                imageBP.s = IPS_IDLE;
                IDSetBLOB(&imageBP, NULL);

                PowerSP.s = IPS_IDLE;
                IUResetSwitch(&PowerSP);
                IDSetSwitch(&PowerSP, "STV is offline");

                break;
        }
        return;
    }
    else if (!strcmp(name, AuxiliarySP.name))
    {
        /* Setup und interrupt buttons */

        ISResetButtons(NULL);
        IUResetSwitch(&AuxiliarySP);
        IUUpdateSwitch(&AuxiliarySP, states, names, n);

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&AuxiliarySP, names[i]);

            if (sp == &AuxiliaryS[0])
            {
                res = STV_Setup();
            }
            else if (sp == &AuxiliaryS[1])
            {
                res = STV_Interrupt();
            }
        }
        if (res == 0)
        {
            AuxiliarySP.s = IPS_OK;
            IUResetSwitch(&AuxiliarySP);
            IDSetSwitch(&AuxiliarySP, NULL);
        }
        else
        {
            AuxiliarySP.s = IPS_ALERT;
            IUResetSwitch(&AuxiliarySP);
            IDSetSwitch(&AuxiliarySP, "Check connection");
        }
    }
    else if (!strcmp(name, ControlSP.name))
    {
        /* Parameter, value and the rotary knobs */
        ISResetButtons(NULL);
        IUResetSwitch(&ControlSP);
        IUUpdateSwitch(&ControlSP, states, names, n);

        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&ControlSP, names[i]);

            /* If the state found is ControlS[0] then process it */

            if (sp == &ControlS[0])
            {
                res = STV_AKey();
            }
            else if (sp == &ControlS[1])
            {
                res = STV_UDRotaryIncrease();
            }
            else if (sp == &ControlS[2])
            {
                res = STV_UDRotaryDecrease();
            }
        }
        if (res == 0)
        {
            ControlSP.s = IPS_OK;
            IUResetSwitch(&ControlSP);
            IDSetSwitch(&ControlSP, NULL);
        }
        else
        {
            ControlSP.s = IPS_ALERT;
            IUResetSwitch(&ControlSP);
            IDSetSwitch(&ControlSP, "Check connection");
        }
    }
    else if (!strcmp(name, ValueSP.name))
    {
        /* Button Value, left/right knob */
        ISResetButtons(NULL);
        IUResetSwitch(&ValueSP);
        IUUpdateSwitch(&ValueSP, states, names, n);

        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&ValueSP, names[i]);

            if (sp == &ValueS[0])
            {
                res = STV_BKey();
            }
            else if (sp == &ValueS[1])
            {
                res = STV_LRRotaryIncrease();
            }
            else if (sp == &ValueS[2])
            {
                res = STV_LRRotaryDecrease();
            }
        }

        if (res == 0)
        {
            ValueSP.s = IPS_OK;
            IUResetSwitch(&ValueSP);
            IDSetSwitch(&ValueSP, NULL);
        }
        else
        {
            ValueSP.s = IPS_ALERT;
            IUResetSwitch(&ValueSP);
            IDSetSwitch(&ValueSP, "Check connection");
        }
    }
    else if (!strcmp(name, AcquireSP.name))
    {
        /* Focus, Image Monitor buttons */
        ISResetButtons(NULL);
        IUResetSwitch(&AcquireSP);
        IUUpdateSwitch(&AcquireSP, states, names, n);

        acquiring  = ON;
        guiding    = OFF;
        processing = OFF;

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&AcquireSP, names[i]);

            if (sp == &AcquireS[0])
            {
                res = STV_Focus();
            }
            else if (sp == &AcquireS[1])
            {
                res = STV_Image();
            }
            else if (sp == &AcquireS[2])
            {
                res = STV_Monitor();
            }
        }
        if (res == 0)
        {
            AcquireSP.s = IPS_OK;
            IUResetSwitch(&AcquireSP);
            IDSetSwitch(&AcquireSP, NULL);
        }
        else
        {
            AcquireSP.s = IPS_ALERT;
            IUResetSwitch(&AcquireSP);
            IDSetSwitch(&AcquireSP, "Check connection");
        }
    }
    else if (!strcmp(name, GuideSP.name))
    {
        /* Calibrate, Track buttons */
        ISResetButtons(NULL);
        IUResetSwitch(&GuideSP);
        IUUpdateSwitch(&GuideSP, states, names, n);

        acquiring  = OFF;
        guiding    = ON;
        processing = OFF;

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&GuideSP, names[i]);

            if (sp == &GuideS[0])
            {
                res = STV_Calibrate();
            }
            else if (sp == &GuideS[1])
            {
                res = STV_Track();
            }
        }
        if (res == 0)
        {
            GuideSP.s = IPS_OK;
            IUResetSwitch(&GuideSP);
            IDSetSwitch(&GuideSP, NULL);
        }
        else
        {
            GuideSP.s = IPS_ALERT;
            IUResetSwitch(&GuideSP);
            IDSetSwitch(&GuideSP, "Check connection");
        }
    }
    else if (!strcmp(name, ProcessSP.name))
    {
        ISResetButtons(NULL);
        IUResetSwitch(&ProcessSP);
        IUUpdateSwitch(&ProcessSP, states, names, n);

        acquiring  = OFF;
        guiding    = OFF;
        processing = ON;

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&ProcessSP, names[i]);

            if (sp == &ProcessS[0])
            {
                res = STV_Display();
            }
            else if (sp == &ProcessS[1])
            {
                res = STV_FileOps();
            }
        }
        if (res == 0)
        {
            ProcessSP.s = IPS_OK;
            IUResetSwitch(&ProcessSP);
            IDSetSwitch(&ProcessSP, NULL);
        }
        else
        {
            ProcessSP.s = IPS_ALERT;
            IUResetSwitch(&ProcessSP);
            IDSetSwitch(&ProcessSP, "Check connection");
        }
    }
    else if (!strcmp(name, ImageInfoSP.name))
    {
        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        /* Read out the image buffer and display a short message if it is empty or not */
        res = ISTerminateTXDisplay();

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&ImageInfoSP, names[i]);

            if (sp == &ImageInfoS[0])
            {
                if ((res = STV_RequestImageInfo(currentBuffer - 1, &image_info)) == 0)
                {
                    ISMessageImageInfo((int)currentBuffer - 1, &image_info);
                }
                else
                {
                    IDMessage(mydev, "Buffer %2d is empty", (int)currentBuffer);
                }
                break;
            }
            else if (sp == &ImageInfoS[1])
            {
                for (i = 0; i < 32; i++)
                {
                    if ((res = STV_RequestImageInfo(i, &image_info)) == 0)
                    {
                        ISMessageImageInfo(i, &image_info);
                    }
                    else
                    {
                        IDMessage(mydev, "Buffer %2d is empty", i + 1);
                    }
                }
                break;
            }
        }
        if (res == 0)
        {
            ImageInfoSP.s = IPS_OK;
            IUResetSwitch(&ImageInfoSP);
            IDSetSwitch(&ImageInfoSP, NULL);
        }
        else
        {
            ImageInfoSP.s = IPS_ALERT;
            IUResetSwitch(&ImageInfoSP);
            /*IDSetSwitch( &ImageInfoSP, "Check connection") ; */
            IDSetSwitch(&ImageInfoSP, NULL);
        }
        res = STV_Interrupt(); /* STV initiates a download that we do not want */
        res = ISRestoreTXDisplay();
    }
    else if (!strcmp(name, CompressionSP.name))
    {
        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        /* Enable or disable compression for image download */
        ISResetButtons(NULL);
        IUResetSwitch(&CompressionSP);
        IUUpdateSwitch(&CompressionSP, states, names, n);

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&CompressionSP, names[i]);

            if (sp == &CompressionS[0])
            {
                CompressionS[0].s = ISS_ON;
            }
            else if (sp == &CompressionS[1])
            {
                CompressionS[1].s = ISS_ON;
            }
        }

        CompressionSP.s = IPS_OK;
        IDSetSwitch(&CompressionSP, NULL);
    }
    else if (!strcmp(name, BufferStatusSP.name))
    {
        ISResetButtons(NULL);

        BufferStatusSP.s = IPS_ALERT;
        IUResetSwitch(&BufferStatusSP);
        IDSetSwitch(&BufferStatusSP, "Wait...");

        if ((AcquireSP.s != OFF) || (GuideSP.s != OFF) || (ProcessSP.s != OFF))
        {
            acquiring  = OFF;
            guiding    = OFF;
            processing = OFF;

            ISResetButtons("Interrupting ongoing image acquisition, calibration or tracking\n");

            AcquireSP.s = IPS_IDLE;
            IUResetSwitch(&AcquireSP);
            IDSetSwitch(&AcquireSP, NULL);

            GuideSP.s = IPS_IDLE;
            IUResetSwitch(&GuideSP);
            IDSetSwitch(&GuideSP, NULL);

            ProcessSP.s = IPS_IDLE;
            IUResetSwitch(&ProcessSP);
            IDSetSwitch(&ProcessSP, NULL);

            ImageInfoSP.s = IPS_IDLE;
            IUResetSwitch(&ImageInfoSP);
            IDSetSwitch(&ImageInfoSP, NULL);

            res = STV_Interrupt();
            usleep(100000);
            res = STV_Interrupt();
        }
        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        sp = IUFindSwitch(&BufferStatusSP, names[0]);

        if ((res = ISTerminateTXDisplay()) != 0)
        {
            fprintf(stderr, "STV Buffer can not terminate TX %d\n", res);
        }

        if (sp == &BufferStatusS[0])
        {
            for (i = 31; i > -1; i--)
            {
                usleep(50000);
                if ((res = STV_BufferStatus(i)) == 0)
                {
                    IDMessage(mydev, "Buffer %2d: image present", i + 1);
                }
                else
                {
                    IDMessage(mydev, "Buffer %2d: empty", i + 1);
                }
            }
        }

        BufferStatusS[0].s = ISS_OFF;

        if (0 <= res)
        {
            BufferStatusSP.s = IPS_OK;
            IUResetSwitch(&BufferStatusSP);
            IDSetSwitch(&BufferStatusSP, NULL);
        }
        else
        {
            BufferStatusSP.s = IPS_ALERT;
            IUResetSwitch(&BufferStatusSP);
            IDSetSwitch(&BufferStatusSP, "Check connection");
        }

        res = ISRestoreTXDisplay();
        res = STV_Interrupt();
    }
    else if (!strcmp(name, DownloadSP.name))
    {
        /* Download images */
        /* Downloading while the STV is occupied is not working */
        if ((AcquireSP.s != OFF) || (GuideSP.s != OFF) || (ProcessSP.s != OFF))
        {
            ISResetButtons("Interrupting ongoing image acquisition, calibration or tracking\n");

            AcquireSP.s = IPS_IDLE;
            IUResetSwitch(&AcquireSP);
            IDSetSwitch(&AcquireSP, NULL);

            GuideSP.s = IPS_IDLE;
            IUResetSwitch(&GuideSP);
            IDSetSwitch(&GuideSP, NULL);

            ProcessSP.s = IPS_IDLE;
            IUResetSwitch(&ProcessSP);
            IDSetSwitch(&ProcessSP, NULL);

            ImageInfoSP.s = IPS_IDLE;
            IUResetSwitch(&ImageInfoSP);
            IDSetSwitch(&ImageInfoSP, NULL);

            res = STV_Interrupt();
            usleep(100000);
            res = STV_Interrupt();
        }
        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        if ((res = ISTerminateTXDisplay()) != 0)
        {
            fprintf(stderr, "STV Buffer can not terminate TX %d\n", res);
        }

        DownloadSP.s = IPS_ALERT;
        IUResetSwitch(&DownloadSP);
        IDSetSwitch(&DownloadSP, NULL);

        compression = OFF;
        if (CompressionS[0].s == ISS_ON)
        {
            compression = ON;
        }
        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&DownloadSP, names[i]);

            if (sp == &DownloadS[0])
            {
                lower_buffer = currentBuffer - 2;
                upper_buffer = currentBuffer - 1;
            }
            else if (sp == &DownloadS[1])
            {
                lower_buffer = -1;
                upper_buffer = 31;
            }
        }
        for (j = upper_buffer; j > lower_buffer; j--)
        {
            if ((res = ISRequestImageData(compression, j, currentX, currentY, currentLength, currentLines)) != 0)
            {
                if (res == 1)
                {
                    IDMessage(mydev, "Buffer %2.0f: empty", (double)(j + 1));
                }
                else
                {
                    break;
                }
            }
        }
        if (res == 0)
        {
            IDMessage(mydev, "STV waits for SYNC TIME Do it! Setting time, PLEASE WAIT!");

            if ((res = STV_SetDateTime(NULL)) == 0)
            {
                UTCTP.s = IPS_OK;
                IDSetText(&UTCTP, "Time set to UTC now");
            }
            else
            {
                UTCTP.s = IPS_ALERT;
                IDSetText(&UTCTP, "Error setting time, check connection");
            }

            DownloadSP.s = IPS_OK;
            IUResetSwitch(&DownloadSP);
            IDSetSwitch(&DownloadSP, NULL);
        }
        else
        { /* res could be -1 (STV_RequestImageData) */

            DownloadSP.s = IPS_ALERT;
            IUResetSwitch(&DownloadSP);
            IDSetSwitch(&DownloadSP, "Check connection");
            IDSetSwitch(&DownloadSP, NULL);
        }

        res = ISRestoreTXDisplay();
        res = STV_Interrupt();
        IDMessage(mydev, "You may continue NOW");
    }
    else if (!strcmp(name, TXDisplaySP.name))
    {
        acquiring  = OFF;
        guiding    = OFF;
        processing = OFF;

        ISResetButtons(NULL);
        IUResetSwitch(&TXDisplaySP);
        IUUpdateSwitch(&TXDisplaySP, states, names, n);

        for (i = 0; i < n; i++)
        {
            sp = IUFindSwitch(&TXDisplaySP, names[i]);

            if (sp == &TXDisplayS[0])
            {
                if ((res = STV_TXDisplay()) == 0)
                {
                    TXDisplaySP.s = IPS_OK;
                    IDSetSwitch(&TXDisplaySP, "Reading out display");

                    DisplayCTP.s = IPS_OK;
                    IDSetText(&DisplayCTP, NULL);

                    DisplayBTP.s = IPS_OK;
                    IDSetText(&DisplayBTP, NULL);

                    DisplayDTP.s = IPS_OK;
                    IDSetText(&DisplayDTP, NULL);
                }
            }
            else if (sp == &TXDisplayS[1])
            {
                DisplayCTP.s = IPS_IDLE;
                DisplayBTP.s = IPS_IDLE;
                DisplayDTP.s = IPS_IDLE;

                if ((res = STV_TerminateTXDisplay()) == 0)
                {
                    TXDisplaySP.s = IPS_OK;
                    IDSetSwitch(&TXDisplaySP, "Stopping display read out");

                    DisplayCTP.s = IPS_IDLE;
                    IUSaveText(&DisplayCT[0], "                        "); /* reset client's display */
                    IUSaveText(&DisplayCT[1], "                        ");
                    IDSetText(&DisplayCTP, NULL);

                    DisplayBTP.s = IPS_IDLE;
                    IUSaveText(&DisplayBT[0], "                        "); /* reset client's display */
                    IUSaveText(&DisplayBT[1], "                        ");
                    IDSetText(&DisplayBTP, NULL);

                    DisplayDTP.s = IPS_IDLE;
                    IUSaveText(&DisplayDT[0], "                        "); /* reset client's display */
                    IUSaveText(&DisplayDT[1], "                        ");
                    IDSetText(&DisplayDTP, NULL);
                }
            }
        }
        if (res != 0)
        {
            TXDisplaySP.s = IPS_ALERT;
            IUResetSwitch(&TXDisplaySP);
            IDSetSwitch(&TXDisplaySP, "Check connection");
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    int res;
    IText *tp;

    /*fprintf(stderr, "ISNewText\n") ; */

    /* #1 Let's make sure everything has been initialized properly */
    ISInit();

    /* #2 Let's make sure that the client is asking to update the properties of our device, otherwise ignore */
    if (dev != nullptr && strcmp(dev, mydev))
        return;

    if (!strcmp(name, PortTP.name))
    {
        if (IUUpdateText(&PortTP, texts, names, n) < 0)
            return;

        PortTP.s = IPS_OK;

        if (PowerS[0].s == ISS_ON)
        {
            PortTP.s = IPS_ALERT;
            IDSetText(&PortTP, "STV is already online");
        }

        /* JM: Don't forget to send acknowledgment */
        IDSetText(&PortTP, NULL);
    }
    else if (!strcmp(name, UTCTP.name))
    {
        ISResetButtons(NULL);

        tp = IUFindText(&UTCTP, names[0]);

        if ((res = ISTerminateTXDisplay()) != 0)
        {
            fprintf(stderr, "STV Buffer can not terminate TX %d\n", res);
        }

        if (tp == &UTCT[0])
        {
            if ((res = STV_SetDateTime(NULL)) == 0)
            {
                UTCTP.s = IPS_OK;
                IDSetText(&UTCTP, "Time set to UTC");
            }
            else
            {
                UTCTP.s = IPS_ALERT;
                IDSetText(&UTCTP, "Error setting time, check connection");
            }
        }
        res = ISRestoreTXDisplay();
    }
}
/* Client sets new number */
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    int res;
    double ccd_temperature;
    /*fprintf(stderr, "ISNewNumber\n") ; */

    /* #1 Let's make sure everything has been initialized properly */
    ISInit();

    /* #2 Let's make sure that the client is asking to update the properties of our device, otherwise ignore */
    if (dev != nullptr && strcmp(dev, mydev))
        return;

    if (PowerS[0].s != ISS_ON)
    {
        PowerSP.s = IPS_ALERT;
        IDSetSwitch(&PowerSP, NULL);
        IDMessage("STV is offline", NULL);
        return;
    }

    if (!strcmp(name, BufferNP.name))
    {
        INumber *buffer = IUFindNumber(&BufferNP, names[0]);

        if (buffer == &BufferN[0])
        {
            currentBuffer = values[0];

            /* Check the boundaries, this is incomplete at the moment */
            BufferNP.s = IPS_OK;
            IDSetNumber(&BufferNP, NULL);
        }
    }
    else if (!strcmp(name, WindowingNP.name))
    {
        INumber *buffer = IUFindNumber(&WindowingNP, names[0]);

        if (buffer == &WindowingN[0])
        {
            currentX      = values[0];
            currentY      = values[1];
            currentLines  = values[2];
            currentLength = values[3];

            WindowingNP.s = IPS_OK;
            IDSetNumber(&WindowingNP, NULL);
        }
    }
    else if (!strcmp(name, SetCCDTemperatureNP.name))
    {
        if ((res = ISTerminateTXDisplay()) != 0)
        {
            fprintf(stderr, "STV Buffer can not terminate TX %d\n", res);
        }

        INumber *np = IUFindNumber(&SetCCDTemperatureNP, names[0]);

        if (np == &SetCCDTemperatureN[0])
        {
            if ((ccd_temperature = STV_SetCCDTemperature(values[0])) != 0)
            { /* STV has no 0 C setting */

                SetCCDTemperatureNP.s       = IPS_OK;
                SetCCDTemperatureN[0].value = ccd_temperature;
                IDSetNumber(&SetCCDTemperatureNP, "CCD Temperature set to %g", SetCCDTemperatureN[0].value);
            }
            else
            {
                SetCCDTemperatureNP.s = IPS_ALERT;
                IDSetNumber(&SetCCDTemperatureNP, "Error setting CCD temperature, check connection");
            }
        }
        res = ISRestoreTXDisplay();
    }
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

int writeFITS(const char *filename, IMAGE_INFO *image_info, char errmsg[])
{
    fitsfile *fptr; /* pointer to the FITS file; defined in fitsio.h */
    int status;
    long fpixel = 1, naxis = 2, nelements;
    long naxes[2];
    char filename_rw[TEMPFILE_LEN + 1];

    naxes[0] = STVImg->width;
    naxes[1] = STVImg->height;

    /* Append ! to file name to over write it.*/
    snprintf(filename_rw, TEMPFILE_LEN + 1, "!%s", filename);

    status = 0;                                    /* initialize status before calling fitsio routines */
    fits_create_file(&fptr, filename_rw, &status); /* create new file */

    /* Create the primary array image (16-bit short integer pixels */
    fits_create_img(fptr, USHORT_IMG, naxis, naxes, &status);

    addFITSKeywords(fptr, image_info);

    nelements = naxes[0] * naxes[1]; /* number of pixels to write */

    /* Write the array of integers to the image */
    fits_write_img(fptr, TUSHORT, fpixel, nelements, STVImg->img, &status);

    fits_close_file(fptr, &status); /* close the file */

    fits_report_error(stderr, status); /* print out any error messages */

    /* Success */
    /*ExposeTimeNP.s = IPS_OK; */
    /*IDSetNumber(&ExposeTimeNP, NULL); */
    uploadFile(filename);

    return status;
}

void addFITSKeywords(fitsfile *fptr, IMAGE_INFO *image_info)
{
    int status = 0;
    char binning_s[32];
    char frame_s[32];
    char date_obs_s[64];
    char tmp[32];

    image_info->pixelSize = 7.4; /* microns */

    if (image_info->binning == 1)
    {
        snprintf(binning_s, 32, "(%1.0f x %1.0f)", 1., 1.);
    }
    else if (image_info->binning == 2)
    {
        snprintf(binning_s, 32, "(%1.0f x %1.0f)", 2., 2.);
    }
    else if (image_info->binning == 3)
    {
        snprintf(binning_s, 32, "(%1.0f x %1.0f)", 3., 3.);
    }
    else
    {
        fprintf(stderr, "Error in binning information: %d\n", image_info->binning);
    }

    strcpy(frame_s, "Light");

    /* ToDo: assign the frame type */
    /*   switch (STVImg->frameType) */
    /*   { */
    /*     case LIGHT_FRAME: */
    /*       	strcpy(frame_s, "Light"); */
    /* 	break; */
    /*     case BIAS_FRAME: */
    /*         strcpy(frame_s, "Bias"); */
    /* 	break; */
    /*     case FLAT_FRAME: */
    /*         strcpy(frame_s, "Flat Field"); */
    /* 	break; */
    /*     case DARK_FRAME: */
    /*         strcpy(frame_s, "Dark"); */
    /* 	break; */
    /*   } */

    fits_update_key(fptr, TDOUBLE, "CCD-TEMP", &(image_info->ccdTemp), "CCD Temperature (Celcius)", &status);
    fits_update_key(fptr, TDOUBLE, "EXPOSURE", &(image_info->exposure), "Total Exposure Time (ms)", &status);
    fits_update_key(fptr, TDOUBLE, "PIX-SIZ", &(image_info->pixelSize), "Pixel Size (microns)", &status);
    fits_update_key(fptr, TSTRING, "BINNING", binning_s, "Binning HOR x VER", &status);
    fits_update_key(fptr, TSTRING, "FRAME", frame_s, "Frame Type", &status);
    fits_update_key(fptr, TDOUBLE, "DATAMIN", &(image_info->minValue), "Minimum value", &status);
    fits_update_key(fptr, TDOUBLE, "DATAMAX", &(image_info->maxValue), "Maximum value", &status);
    fits_update_key(fptr, TSTRING, "INSTRUME", "SBIG STV", "CCD Name", &status);

    sprintf(tmp, "%4d-", image_info->year);
    strcpy(date_obs_s, tmp);

    if (image_info->month < 10)
    {
        sprintf(tmp, "0%1d-", image_info->month);
    }
    else
    {
        sprintf(tmp, "%2d-", image_info->month);
    }
    strcat(date_obs_s, tmp);

    if (image_info->day < 10)
    {
        sprintf(tmp, "0%1dT", image_info->day);
    }
    else
    {
        sprintf(tmp, "%2dT", image_info->day);
    }
    strcat(date_obs_s, tmp);

    if (image_info->hours < 10)
    {
        sprintf(tmp, "0%1d:", image_info->hours);
    }
    else
    {
        sprintf(tmp, "%2d:", image_info->hours);
    }
    strcat(date_obs_s, tmp);

    if (image_info->minutes < 10)
    {
        sprintf(tmp, "0%1d:", image_info->minutes);
    }
    else
    {
        sprintf(tmp, "%2d:", image_info->minutes);
    }
    strcat(date_obs_s, tmp);

    if (image_info->seconds < 10)
    {
        sprintf(tmp, "0%1d:", image_info->seconds);
    }
    else
    {
        sprintf(tmp, "%2d:", image_info->seconds);
    }
    strcat(date_obs_s, tmp);

    fits_update_key(fptr, TSTRING, "DATE-OBS", date_obs_s, "Observing date (YYYY-MM-DDThh:mm:ss UT", &status);

    fits_write_date(fptr, &status);
}

void uploadFile(const char *filename)
{
    FILE *fitsFile;
    unsigned char *fitsData, *compressedData;
    int r          = 0;
    unsigned int i = 0, nr = 0;
    uLongf compressedBytes = 0;
    uLong totalBytes;
    struct stat stat_p;

    if (-1 == stat(filename, &stat_p))
    {
        IDLog("Error occurred attempting to stat file.\n");
        return;
    }

    totalBytes = stat_p.st_size;

    fitsData       = (unsigned char *)malloc(sizeof(unsigned char) * totalBytes);
    compressedData = (unsigned char *)malloc(sizeof(unsigned char) * totalBytes + totalBytes / 64 + 16 + 3);

    if (fitsData == NULL || compressedData == NULL)
    {
        if (fitsData)
            free(fitsData);
        if (compressedData)
            free(compressedData);
        IDLog("Error! low memory. Unable to initialize fits buffers.\n");
        return;
    }

    fitsFile = fopen(filename, "r");

    if (fitsFile == NULL)
    {
        free(fitsData);
        free(compressedData);
        return;
    }

    /* #1 Read file from disk */
    for (i = 0; i < totalBytes; i += nr)
    {
        nr = fread(fitsData + i, 1, totalBytes - i, fitsFile);

        if (nr <= 0)
        {
            IDLog("Error reading temporary FITS file.\n");
            free(fitsData);
            fclose(fitsFile);
            return;
        }
    }
    fclose(fitsFile);

    compressedBytes = sizeof(char) * totalBytes + totalBytes / 64 + 16 + 3;

    /* #2 Compress it */
    r = compress2(compressedData, &compressedBytes, fitsData, totalBytes, 9);
    if (r != Z_OK)
    {
        /* this should NEVER happen */
        IDLog("internal error - compression failed: %d\n", r);
        return;
    }

    /* #3 Send it */
    imageB.blob    = compressedData;
    imageB.bloblen = compressedBytes;
    imageB.size    = totalBytes;
    strcpy(imageB.format, ".fits.z");

    imageBP.s = IPS_OK;
    IDSetBLOB(&imageBP, NULL);

    free(fitsData);
    free(compressedData);
}

int ISTerminateTXDisplay(void)
{
    int res = 0;
    res     = STV_Interrupt(); /* with out it hangs */
    usleep(100000);

    IERmCallback(cb);
    cb = -1;

    if (TXDisplayS[0].s == ISS_ON)
    {
        TXDisplaySP.s = IPS_BUSY;
        IDSetSwitch(&TXDisplaySP, "Stopping display read out");

        DisplayCTP.s = IPS_IDLE;
        IUSaveText(&DisplayCT[0], "                        "); /* reset client's display */
        IUSaveText(&DisplayCT[1], "                        ");
        IDSetText(&DisplayCTP, NULL);

        DisplayBTP.s = IPS_IDLE;
        IUSaveText(&DisplayBT[0], "                        "); /* reset client's display */
        IUSaveText(&DisplayBT[1], "                        ");
        IDSetText(&DisplayBTP, NULL);

        DisplayDTP.s = IPS_IDLE;
        IUSaveText(&DisplayDT[0], "                        "); /* reset client's display */
        IUSaveText(&DisplayDT[1], "                        ");
        IDSetText(&DisplayDTP, NULL);

        if ((res = STV_TerminateTXDisplay()) != 0)
        {
            fprintf(stderr, "STV: error writing TTXD %d\n", res);
        }
    }
    else
    {
        res = 0;
    }
    usleep(500000); /* make sure that everything is discarded */
    tcflush(fd, TCIOFLUSH);

    return res;
}

int ISRestoreTXDisplay(void)
{
    int res = 0;

    cb = IEAddCallback(fd, (IE_CBF *)ISCallBack, NULL);

    if (TXDisplayS[0].s == ISS_ON)
    {
        usleep(500000); /* STV need a little rest */
        res = STV_TXDisplay();

        TXDisplaySP.s = IPS_OK;
        IDSetSwitch(&TXDisplaySP, "Starting Display read out");

        DisplayCTP.s = IPS_OK;
        IDSetText(&DisplayCTP, NULL);

        DisplayBTP.s = IPS_OK;
        IDSetText(&DisplayBTP, NULL);

        DisplayDTP.s = IPS_OK;
        IDSetText(&DisplayDTP, NULL);
    }
    return res;
}

int ISMessageImageInfo(int buffer, IMAGE_INFO *image_info)
{
    buffer++;

    /*     IDMessage( mydev, "B%2d: descriptor:%d\n", buffer, image_info->descriptor) ; */
    /*     IDMessage( mydev, "B%2d: height:%d\n", buffer, image_info->height) ; */
    /*     IDMessage( mydev, "B%2d: width:%d\n", buffer, image_info->width) ; */
    /*     IDMessage( mydev, "B%2d: top:%d\n", buffer, image_info->top) ; */
    /*     IDMessage( mydev, "B%2d: left:%d\n", buffer, image_info->left) ; */
    IDMessage(mydev, "B%2d: Exposure:%6.3f, Height:%2d, Width:%2d, CCD Temperature:%3.1f\n", buffer,
              image_info->exposure, image_info->height, image_info->width, image_info->ccdTemp);
    /*     IDMessage( mydev, "B%2d: noExposure:%d\n", buffer, image_info->noExposure) ; */
    /*     IDMessage( mydev, "B%2d: analogGain:%d\n", buffer, image_info->analogGain) ; */
    /*     IDMessage( mydev, "B%2d: digitalGain:%d\n", buffer, image_info->digitalGain) ; */
    /*     IDMessage( mydev, "B%2d: focalLength:%d\n", buffer, image_info->focalLength) ; */
    /*     IDMessage( mydev, "B%2d: aperture:%d\n", buffer, image_info->aperture) ; */
    /*     IDMessage( mydev, "B%2d: packedDate:%d\n", buffer, image_info->packedDate) ; */
    IDMessage(mydev, "B%2d: Year:%4d, Month: %2d, Day:%2d\n", buffer, image_info->year, image_info->month,
              image_info->day);
    /*     IDMessage( mydev, "B%2d: Day:%d\n", buffer, image_info->day) ; */
    /*     IDMessage( mydev, "B%2d: Month:%d\n", buffer, image_info->month) ; */
    /*     IDMessage( mydev, "B%2d: packedTime:%d\n", buffer, image_info->packedTime) ; */
    /*     IDMessage( mydev, "B%2d: Seconds:%d\n", buffer, image_info->seconds) ; */
    /*     IDMessage( mydev, "B%2d: minutes:%d\n", buffer, image_info->minutes) ; */
    IDMessage(mydev, "B%2d: Hours:%2d, Minutes:%2d, Seconds:%d\n", buffer, image_info->hours, image_info->minutes,
              image_info->seconds);

    /*     IDMessage( mydev, "B%2d: ccdTemp:%f\n", buffer, image_info->ccdTemp) ; */
    /*     IDMessage( mydev, "B%2d: siteID:%d\n", buffer, image_info->siteID) ; */
    /*     IDMessage( mydev, "B%2d: eGain:%d\n", buffer, image_info->eGain) ; */
    /*     IDMessage( mydev, "B%2d: background:%d\n", buffer, image_info->background) ; */
    /*     IDMessage( mydev, "B%2d: range :%d\n", buffer, image_info->range ) ;   */
    /*     IDMessage( mydev, "B%2d: pedestal:%d\n", buffer, image_info->pedestal) ; */
    /*     IDMessage( mydev, "B%2d: ccdTop  :%d\n", buffer, image_info->ccdTop) ; */
    /*     IDMessage( mydev, "B%2d: ccdLeft:%d\n", buffer, image_info->ccdLeft) ; */
    return 0;
}

int ISRequestImageData(int compression, int buffer, int x_offset, int y_offset, int length, int lines)
{
    int res;
    int i, k;
    int img_size;
    char errmsg[1024];
    int image[320][320];
    IMAGE_INFO image_info;

    for (i = 0; i < 320; i++)
    {
        for (k = 0; k < 320; k++)
        {
            image[i][k] = -1;
        }
    }

    res = STV_RequestImage(compression, buffer, x_offset, y_offset, &length, &lines, image, &image_info);

    if (res == 0)
    {
        STVImg->width  = length;
        STVImg->height = lines;

        img_size = STVImg->width * STVImg->height * sizeof(unsigned short);

        STVImg->img = malloc(img_size);

        for (i = 0; i < STVImg->height; i++)
        { /* x */
            for (k = 0; k < STVImg->width; k++)
            { /* y */

                STVImg->img[STVImg->width * i + k] = (unsigned short)image[i][k];
                /* Uncomment this line in case of doubts about decompressed values and compare */
                /* both sets. */
                /*fprintf( stderr, "Line: %d %d %d %d\n", i, k, image[i][k], STVImg->img[ STVImg->width* i + k]) ; */

                if (STVImg->img[STVImg->width * i + k] < image_info.minValue)
                {
                    image_info.minValue = STVImg->img[STVImg->width * i + k];
                }
                if (STVImg->img[STVImg->width * i + k] > image_info.maxValue)
                {
                    image_info.maxValue = STVImg->img[STVImg->width * i + k];
                }
            }
        }
        writeFITS("FITS.fits", &image_info, errmsg);
        /*fprintf( stderr, "Fits writing message: %s\n", errmsg) ;	 */
        free(STVImg->img);
    }
    return res;
}

void ISUpdateDisplay(int buffer, int line)
{
    if (!((line + 1) % 10))
    {
        sprintf(DisplayCT[0].text, "Buffer %2d line: %3d", buffer + 1, line + 1);
        strcpy(DisplayBT[0].text, DisplayCT[0].text);
        strcpy(DisplayDT[0].text, DisplayCT[0].text);

        DisplayCTP.s = IPS_OK;
        IDSetText(&DisplayCTP, NULL);
        DisplayBTP.s = IPS_OK;
        IDSetText(&DisplayBTP, NULL);
        DisplayDTP.s = IPS_OK;
        IDSetText(&DisplayDTP, NULL);
    }
    else if ((line + 1) == 1)
    { /* first time  */

        IDMessage(mydev, "Image download started");
    }
    else if (line < 0)
    { /* last line */

        line = -line;

        sprintf(DisplayCT[0].text, "Buffer %2d line: %3d", buffer + 1, line + 1);
        strcpy(DisplayBT[0].text, DisplayCT[0].text);
        strcpy(DisplayDT[0].text, DisplayCT[0].text);

        DisplayCTP.s = IPS_OK;
        IDSetText(&DisplayCTP, NULL);
        DisplayBTP.s = IPS_OK;
        IDSetText(&DisplayBTP, NULL);
        DisplayDTP.s = IPS_OK;
        IDSetText(&DisplayDTP, NULL);
        IDMessage(mydev, "Image download ended, buffer %2d line: %3d", buffer + 1, line);
    }
}
