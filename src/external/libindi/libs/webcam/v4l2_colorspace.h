
#pragma once

#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif
/* for kernel < 3.19 */
/* ITU-R 601 -- SDTV */
#define YCBCR_ENC_601 1
/* Rec. 709 -- HDTV */
#define YCBCR_ENC_709 2
/* sYCC (Y'CbCr encoding of sRGB) */
#define YCBCR_ENC_SYCC 5
/* SMPTE 240M -- Obsolete HDTV */
#define YCBCR_ENC_SMPTE240M     8
#define QUANTIZATION_FULL_RANGE 1
#define QUANTIZATION_LIM_RANGE  2

void initColorSpace();
const char *getColorSpaceName(struct v4l2_format *fmt);
unsigned int getYCbCrEncoding(struct v4l2_format *fmt);
const char *getYCbCrEncodingName(struct v4l2_format *fmt);
unsigned int getQuantization(struct v4l2_format *fmt);
const char *getQuantizationName(struct v4l2_format *fmt);

void rangeY8(unsigned char *buf, unsigned int len);
void linearize(float *buf, unsigned int len, struct v4l2_format *fmt);

#ifdef __cplusplus
}
#endif
