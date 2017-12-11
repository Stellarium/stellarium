
#include "v4l2_colorspace.h"

#include <math.h>

unsigned char lutrangey8[256];
unsigned short lutrangey10[1024];
unsigned short lutrangey12[4096];
unsigned short lutrangey16[65536];
unsigned char lutrangecbcr8[256];
unsigned short lutrangecbcr10[1024];
unsigned short lutrangecbcr12[4096];
unsigned short lutrangecbcr16[65536];

void initColorSpace()
{
    unsigned int i;

    for (i = 0; i < 256; i++)
    {
        lutrangey8[i] = (unsigned char)((255.0 / 219.0) * (i - 16));
        if (i > 235)
            lutrangey8[i] = 255;
        lutrangecbcr8[i] = (unsigned char)((255.0 / 224.0) * i);
    }
}

void rangeY8(unsigned char *buf, unsigned int len)
{
    unsigned char *s = buf;

    for (unsigned int i = 0; i < len; i++)
    {
        *s = lutrangey8[*s];
        s++;
    }
}

void linearize(float *buf, unsigned int len, struct v4l2_format *fmt)
{
    unsigned int i;
    float *src = buf;
    switch (fmt->fmt.pix.colorspace)
    {
        case V4L2_COLORSPACE_SMPTE240M:
            // Old obsolete HDTV standard. Replaced by REC 709.
            // This is the transfer function for SMPTE 240M
            for (i = 0; i < len; i++)
            {
                *src = (*src < 0.0913) ? *src / 4.0 : pow((*src + 0.1115) / 1.1115, 1.0 / 0.45);
                src++;
            }
            break;
        case V4L2_COLORSPACE_SRGB:
            // This is used for sRGB as specified by the IEC FDIS 61966-2-1 standard
            for (i = 0; i < len; i++)
            {
                *src = (*src < -0.04045) ? -pow((-*src + 0.055) / 1.055, 2.4) :
                       ((*src <= 0.04045) ? *src / 12.92 : pow((*src + 0.055) / 1.055, 2.4));
                src++;
            }
            break;
        //case V4L2_COLORSPACE_ADOBERGB:
        //r = pow(r, 2.19921875);
        //break;
        case V4L2_COLORSPACE_REC709:
        //case V4L2_COLORSPACE_BT2020:
        default:
            // All others use the transfer function specified by REC 709
            for (i = 0; i < len; i++)
            {
                *src = (*src <= -0.081) ? -pow((*src - 0.099) / -1.099, 1.0 / 0.45) :
                       ((*src < 0.081) ? *src / 4.5 : pow((*src + 0.099) / 1.099, 1.0 / 0.45));
                src++;
            }
    }
}

const char *getColorSpaceName(struct v4l2_format *fmt)
{
    switch (fmt->fmt.pix.colorspace)
    {
        case V4L2_COLORSPACE_SMPTE170M:
            return "SMPTE170M (SDTV)";
        case V4L2_COLORSPACE_SMPTE240M:
            return "SMPTE240M (early HDTV)";
        case V4L2_COLORSPACE_REC709:
            return "REC709 (HDTV)";
        case V4L2_COLORSPACE_BT878:
            return "BT878";
        case V4L2_COLORSPACE_470_SYSTEM_M:
            return "470 SYSTEM M (old NTSC)";
        case V4L2_COLORSPACE_470_SYSTEM_BG:
            return "470 SYSTEM BG (old PAL/SECAM)";
        case V4L2_COLORSPACE_JPEG:
            return "JPEG";
        case V4L2_COLORSPACE_SRGB:
            return "SRGB";
        /* since Kernel 3.19
        case V4L2_COLORSPACE_ADOBERGB:
          return "Adobe RGB";
        case V4L2_COLORSPACE_BT2020:
          return "BT2020 (UHDTV)";
        */
        default:
            return "Unknown";
    }
}

unsigned int getYCbCrEncoding(struct v4l2_format *fmt)
{
    switch (fmt->fmt.pix.colorspace)
    {
        case V4L2_COLORSPACE_SMPTE170M:
        case V4L2_COLORSPACE_BT878:
        case V4L2_COLORSPACE_470_SYSTEM_M:
        case V4L2_COLORSPACE_470_SYSTEM_BG:
        case V4L2_COLORSPACE_JPEG:
            return YCBCR_ENC_601;
        case V4L2_COLORSPACE_REC709:
            return YCBCR_ENC_709;
        case V4L2_COLORSPACE_SRGB:
            return YCBCR_ENC_SYCC;
        case V4L2_COLORSPACE_SMPTE240M:
            return YCBCR_ENC_SMPTE240M;
        /* since Kernel 3.19
        case V4L2_COLORSPACE_ADOBERGB:
          return return V4L2_YCBCR_ENC_601;
        case V4L2_COLORSPACE_BT2020:
          return return V4L2_YCBCR_ENC_BT2020;
        */
        default:
            return YCBCR_ENC_601;
    }
}

const char *getYCbCrEncodingName(struct v4l2_format *fmt)
{
    switch (getYCbCrEncoding(fmt))
    {
        case YCBCR_ENC_601:
            return "ITU-R 601 -- SDTV";
        case YCBCR_ENC_709:
            return "Rec. 709 -- HDTV";
        case YCBCR_ENC_SYCC:
            return "sYCC (Y'CbCr encoding of sRGB)";
        case YCBCR_ENC_SMPTE240M:
            return "SMPTE 240M -- Obsolete HDTV";
        default:
            return "Unknown";
    }
}

unsigned int getQuantization(struct v4l2_format *fmt)
{
    switch (fmt->fmt.pix.colorspace)
    {
        /* since Kernel 3.19
        case V4L2_COLORSPACE_ADOBERGB:
        case V4L2_COLORSPACE_BT2020:
        */
        case V4L2_COLORSPACE_SMPTE170M:
        case V4L2_COLORSPACE_BT878:
        case V4L2_COLORSPACE_470_SYSTEM_M:
        case V4L2_COLORSPACE_470_SYSTEM_BG:
        case V4L2_COLORSPACE_JPEG:
        case V4L2_COLORSPACE_REC709:
        case V4L2_COLORSPACE_SMPTE240M:
            return QUANTIZATION_LIM_RANGE;
        case V4L2_COLORSPACE_SRGB:
            return QUANTIZATION_FULL_RANGE;
        default:
            return QUANTIZATION_LIM_RANGE;
    }
}

const char *getQuantizationName(struct v4l2_format *fmt)
{
    switch (getQuantization(fmt))
    {
        case QUANTIZATION_FULL_RANGE:
            return "Full Range";
        case QUANTIZATION_LIM_RANGE:
            return "Limited Range";
        default:
            return "Unknown";
    }
}
