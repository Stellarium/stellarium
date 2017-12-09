/*
    Copyright (C) 2005 by Jasem Mutlaq <mutlaqja@ikarustech.com>

    Based on V4L 2 Example
    http://v4l2spec.bytesex.org/spec-single/v4l2.html#CAPTURE-EXAMPLE

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

*/

#include "v4l2_base.h"

#include "ccvt.h"
#include "eventloop.h"
#include "indidevapi.h"
#include "indilogger.h"
#include "lilxml.h"
// PWC framerate support
#include "pwc-ioctl.h"

#include <iostream>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cerrno>
#include <sys/mman.h>
#include <cstring>
#include <asm/types.h> /* for videodev2.h */
#include <ctime>
#include <cmath>
#include <sys/time.h>

/* Kernel headers version */
#include <linux/version.h>

#define ERRMSGSIZ 1024

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define XIOCTL(fd, ioctl, arg) this->xioctl(fd, ioctl, arg, #ioctl)

#define DBG_STR_PIX "%c%c%c%c"
#define DBG_PIX(pf) ((pf) >> 0) & 0xFF, ((pf) >> 8) & 0xFF, ((pf) >> 16) & 0xFF, ((pf) >> 24) & 0xFF

/* TODO: Before 3.17, the only way to determine a format is compressed is to
 * consolidate a matrix with v4l2_pix_format::pixelformat and v4l2_fourcc
 * values. After 3.17, field 'flags' in v4l2_pix_format is assumed to be
 * properly filled. For now we rely on 'flags', but we could just check the
 * most used pixel formats in a CCD whitelist (YUVx, RGBxxx...).
 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0))
#define DBG_STR_FLAGS "...%c .%c%c%c %c%c.%c .%c%c%c %c%c%c%c"
#define DBG_FLAGS(b)                                                                  \
    /* 0x00010000 */ ((b).flags & V4L2_BUF_FLAG_TSTAMP_SRC_SOE) ? 'S' : 'E',          \
        /* 0x00004000 */ ((b).flags & V4L2_BUF_FLAG_TIMESTAMP_COPY) ? 'c' : '.',      \
        /* 0x00002000 */ ((b).flags & V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC) ? 'm' : '.', \
        /* 0x00001000 */ ((b).flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN) ? 'C' : '.',      \
        /* 0x00000800 */ ((b).flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE) ? 'I' : '.', \
        /* 0x00000400 */ ((b).flags & V4L2_BUF_FLAG_PREPARED) ? 'p' : '.',            \
        /* 0x00000100 */ ((b).flags & V4L2_BUF_FLAG_TIMECODE) ? 'T' : '.',            \
        /* 0x00000040 */ ((b).flags & V4L2_BUF_FLAG_ERROR) ? 'E' : '.',               \
        /* 0x00000020 */ ((b).flags & V4L2_BUF_FLAG_BFRAME) ? 'B' : '.',              \
        /* 0x00000010 */ ((b).flags & V4L2_BUF_FLAG_PFRAME) ? 'P' : '.',              \
        /* 0x00000008 */ ((b).flags & V4L2_BUF_FLAG_KEYFRAME) ? 'K' : '.',            \
        /* 0x00000004 */ ((b).flags & V4L2_BUF_FLAG_DONE) ? 'd' : '.',                \
        /* 0x00000002 */ ((b).flags & V4L2_BUF_FLAG_QUEUED) ? 'q' : '.',              \
        /* 0x00000001 */ ((b).flags & V4L2_BUF_FLAG_MAPPED) ? 'm' : '.'
#else
#define DBG_STR_FLAGS "%s"
#define DBG_FLAGS(b)  ""
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0))
#define DBG_STR_FMT "%ux%u " DBG_STR_PIX " %scompressed (%ssupported)"
#define DBG_FMT(f)                                                           \
    (f).fmt.pix.width, (f).fmt.pix.height, DBG_PIX((f).fmt.pix.pixelformat), \
        ((f).fmt.pix.flags & V4L2_FMT_FLAG_COMPRESSED) ? "" : "un",          \
        (decoder->issupportedformat((f).fmt.pix.pixelformat) ? "" : "un")
#else
#define DBG_STR_FMT "%ux%u " DBG_STR_PIX " (%ssupported)"
#define DBG_FMT(f)                                                           \
    (f).fmt.pix.width, (f).fmt.pix.height, DBG_PIX((f).fmt.pix.pixelformat), \
        (decoder->issupportedformat((f).fmt.pix.pixelformat) ? "" : "un")
#endif

#define DBG_STR_BUF "#%d " DBG_STR_FLAGS " % 7d bytes %4.4s seq %d:%d stamp %ld.%06ld"
#define DBG_BUF(b)                                                                                           \
    (b).index, DBG_FLAGS(b), (b).bytesused,                                                                  \
        ((b).memory == V4L2_MEMORY_MMAP) ?                                                                   \
            "mmap" :                                                                                         \
            ((b).memory == V4L2_MEMORY_USERPTR) ? "uptr" :                                                   \
                                                  ((b).memory == 4 /* kernel 3.8.0: V4L2_MEMORY_DMABUF */) ? \
                                                  "dma" :                                                    \
                                                  ((b).memory == V4L2_MEMORY_OVERLAY) ? "over" : "",         \
        (b).sequence, (b).field, (b).timestamp.tv_sec, (b).timestamp.tv_usec

using namespace std;

/*char *entityXML(char *src) {
  char *s = src;
  while (*s) {
    if ((*s == '&') || (*s == '<') || (*s == '>') || (*s == '\'') || (*s == '"'))
      *s = '_';
    s += 1;
  }
  return src;
}*/

namespace INDI
{

V4L2_Base::V4L2_Base()
{
    frameRate.numerator   = 1;
    frameRate.denominator = 25;

    selectCallBackID = -1;
    //dropFrameCount = 1;
    //dropFrame = 0;

    xmax = xmin = 160;
    ymax = ymin = 120;

    io        = IO_METHOD_MMAP;
    fd        = -1;
    buffers   = nullptr;
    n_buffers = 0;

    callback = nullptr;

    cancrop      = true;
    cansetrate   = true;
    streamedonce = false;

    v4l2_decode = new V4L2_Decode();
    decoder     = v4l2_decode->getDefaultDecoder();
    decoder->init();
    dodecode = true;

    bpp                                           = 8;
    has_ext_pix_format                            = false;
    const std::vector<unsigned int> &vsuppformats = decoder->getsupportedformats();
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                 "Using default decoder '%s'\n  Supported V4L2 formats are:", decoder->getName());
    for (std::vector<unsigned int>::const_iterator it = vsuppformats.begin(); it != vsuppformats.end(); ++it)
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%c%c%c%c ", (*it >> 0), (*it >> 8), (*it >> 16),
                     (*it >> 24));
    //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,INDI::Logger::DBG_SESSION,"Default decoder: %s", decoder->getName());

    getframerate = nullptr;
    setframerate = nullptr;

    reallocate_buffers = false;
    path               = nullptr;
    uptr               = nullptr;

    lxstate      = LX_ACTIVE;
    streamactive = false;
    cropset      = false;
}

V4L2_Base::~V4L2_Base()
{
    delete v4l2_decode;
}

/** @brief Helper indicating whether current pixel format is compressed or not.
 *
 * This function is used in read_frame to check for corrupted frames.
 *
 * @return true if pixel format is considered compressed by the driver, else
 * false.
 *
 * @warning If kernel headers 3.17 or later are available, this function will
 * rely on field 'flags', else will compare the current pixel format against an
 * arbitrary list of known format codes.
 */
bool V4L2_Base::is_compressed() const
{
/* See note at top of this file */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0))
    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_JPEG:
        case V4L2_PIX_FMT_MJPEG:
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: format %c%c%c%c patched to be considered compressed",
                         __FUNCTION__, fmt.fmt.pix.pixelformat, fmt.fmt.pix.pixelformat >> 8,
                         fmt.fmt.pix.pixelformat >> 16, fmt.fmt.pix.pixelformat >> 24);
            return true;

        default:
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: format %c%c%c%c has compressed flag %d",
                         __FUNCTION__, fmt.fmt.pix.pixelformat, fmt.fmt.pix.pixelformat >> 8,
                         fmt.fmt.pix.pixelformat >> 16, fmt.fmt.pix.pixelformat >> 24,
                         fmt.fmt.pix.flags & V4L2_FMT_FLAG_COMPRESSED);
            return fmt.fmt.pix.flags & V4L2_FMT_FLAG_COMPRESSED;
    }
#else
    switch (fmt.fmt.pix.pixelformat)
    {
        case V4L2_PIX_FMT_GREY:
            /* case V4L2_PIX_FMT... add other uncompressed and supported formats here */
            return false;

        default:
            return true;
    }
#endif
}

/** @internal Helper for ioctl calls, with logging facility.
 *
 * This function is called by internal macro XIOCTL.
 *
 * @param fd is the file descriptor against which to run the ioctl.
 * @param request is the name of the ioctl to run.
 * @param arg is the argument structure to pass to the ioctl.
 * @param request_str is the stringified name of the request for debug prints.
 * @return the result of the ioctl.
 * @note This function takes care of EINTR while running the ioctl.
 */
int V4L2_Base::xioctl(int fd, int request, void *arg, char const *const request_str)
{
    int r = -1;

    do
    {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);

    if (-1 == r)
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: ioctl 0x%08X/%s received errno %d (%s)", __FUNCTION__,
                     request, request_str, errno, strerror(errno));

    return r;
}

/* @internal Setting a V4L2 format through ioctl VIDIOC_S_FMT
 *
 * If the format type is non-zero, this function executes ioctl
 * VIDIOC_S_FMT on the argument data format, and updates the instance
 * data format on success. If an error arises, the instance data
 * format is left unmodified.
 *
 * If the format type is zero, this function executes ioctl
 * VIDIOC_G_FMT on a temporary data format, and updates the instance
 * data format on success. If an error arises, the instance data
 * format is left unmodified.
 *
 * @warning If the format type is non-zero and the device streamed
 * at least once before the call, the device is closed and reopened
 * before updating the format.
 *
 * @warning If successful, this function updates the instance data
 * format 'fmt'.
 *
 * @note The frame decoder format is updated with the resulting format,
 * and the instance depth field 'bpp' is updated with the resulting
 * frame decoder depth.
 *
 * @param new_fmt is the v4l2_format to set, eventually with type set
 * to zero to refresh the instance format with the current device format.
 * @return 0 if ioctl is successful, or -1 with error message updated.
 */
int V4L2_Base::ioctl_set_format(struct v4l2_format new_fmt, char *errmsg)
{
    /* Reopen device if it streamed at least once and we want to update the format*/
    if (streamedonce && new_fmt.type)
    {
        close_device();

        if (open_device(path, errmsg))
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: failed reopening device %s (%s)", __FUNCTION__, path,
                         errmsg);
            return -1;
        }
    }

    /* Trying format with VIDIOC_TRY_FMT has no interesting advantage here */
    if (false)
    {
        if (-1 == XIOCTL(fd, VIDIOC_TRY_FMT, &new_fmt))
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: failed VIDIOC_TRY_FMT with " DBG_STR_FMT,
                         __FUNCTION__, DBG_FMT(new_fmt));
            return errno_exit("VIDIOC_TRY_FMT", errmsg);
        }
    }

    if (new_fmt.type)
    {
        /* Set format */
        if (-1 == XIOCTL(fd, VIDIOC_S_FMT, &new_fmt))
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: failed VIDIOC_S_FMT with " DBG_STR_FMT, __FUNCTION__,
                         DBG_FMT(new_fmt));
            return errno_exit("VIDIOC_S_FMT", errmsg);
        }
    }
    else
    {
        /* Retrieve format */
        new_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == XIOCTL(fd, VIDIOC_G_FMT, &new_fmt))
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: failed VIDIOC_G_FMT", __FUNCTION__);
            return errno_exit("VIDIOC_G_FMT", errmsg);
        }
    }

    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: current format " DBG_STR_FMT, __FUNCTION__,
                 DBG_FMT(new_fmt));

    /* Update internals */
    decoder->setformat(new_fmt, has_ext_pix_format);
    this->bpp = decoder->getBpp();

    /* Assign the format as current */
    fmt = new_fmt;

    return 0;
}

int V4L2_Base::errno_exit(const char *s, char *errmsg)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    snprintf(errmsg, ERRMSGSIZ, "%s error %d, %s\n", s, errno, strerror(errno));

    if (streamactive)
        stop_capturing(errmsg);

    return -1;
}

void V4L2_Base::doDecode(bool d)
{
    dodecode = d;
}

int V4L2_Base::connectCam(const char *devpath, char *errmsg, int pixelFormat, int width, int height)
{
    INDI_UNUSED(pixelFormat);
    INDI_UNUSED(width);
    INDI_UNUSED(height);
    selectCallBackID      = -1;
    cancrop               = true;
    cansetrate            = true;
    streamedonce          = false;
    frameRate.numerator   = 1;
    frameRate.denominator = 25;

    if (open_device(devpath, errmsg) < 0)
        return -1;

    path = devpath;

    if (check_device(errmsg) < 0)
        return -1;

    //cerr << "V4L2 Check: All successful, returning\n";
    return fd;
}

void V4L2_Base::disconnectCam(bool stopcapture)
{
    char errmsg[ERRMSGSIZ];

    if (selectCallBackID != -1)
        rmCallback(selectCallBackID);

    if (stopcapture)
        stop_capturing(errmsg);

    //uninit_device (errmsg);

    close_device();

    //fprintf(stderr, "Disconnect cam\n");
}

bool V4L2_Base::isLXmodCapable()
{
    if (!(strcmp((const char *)cap.driver, "pwc")))
        return true;
    else
        return false;
}

/* @internal Calculate epoch time shift
 *
 * The clock CLOCK_MONOTONIC starts counting from an undefined origin (boot time
 * for instance). This function computes the offset between the current time returned
 * by gettimeofday and the monotonic time returned by clock_gettime in milliseconds.
 * This value can then be used to determine the time and date of frames from their
 * timestamp as returned by the kernel.
 *
 * Code provided at:
 * http://stackoverflow.com/questions/10266451/where-does-v4l2-buffer-timestamp-value-starts-counting
 *
 * @return the milliseconds offset to apply to the timestamp returned by gettimeofday for
 * it to have the same reference as clock_gettime.
 */
//static long getEpochTimeShift()
//{
//    struct timeval epochtime = { 0, 0 };
//    struct timespec vsTime   = { 0, 0 };
//
//    gettimeofday(&epochtime, nullptr);
//    clock_gettime(CLOCK_MONOTONIC, &vsTime);
//
//    long const uptime_ms = vsTime.tv_sec * 1000 + (long)round(vsTime.tv_nsec / 1000000.0);
//    long const epoch_ms  = epochtime.tv_sec * 1000 + (long)round(epochtime.tv_usec / 1000.0);
//
//    long const epoch_shift = epoch_ms - uptime_ms;
//    //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"%s: epoch shift is %ld",__FUNCTION__,epoch_shift);
//
//    return epoch_shift;
//}

/* @brief Reading a frame from the V4L2 driver.
 *
 * This function will attempt to read a frame with the adequate
 * method for the device, and forward the frame read to the configured
 * decoder and/or recorder.
 *
 * With the MMAP method, the first available buffer is dequeued to read
 * the embedded frame. If the frame is marked erroneous by the driver, or
 * the frame is known to be uncompressed but its length doesn't match the
 * expected size, the buffer is re-enqueued immediately.
 *
 * Although only the MMAP method is actually supported, two other methods
 * are also implemented:
 * - With the READ method, the frame is read directly from the device
 * descriptor, using the first buffer characteristics are address and
 * length. But no processing is done actually.
 * - With the USERPTR method, the first available buffer is dequeued, then
 * verified against the buffer list. No processing is done actually, and
 * the buffer is immediately requeued.
 *
 * @param errmsg is the error messsage updated in case of error.
 * @return 0 if frame read is processed, or -1 with error message updated.
 */
int V4L2_Base::read_frame(char *errmsg)
{
    unsigned int i;
    //cerr << "in read Frame" << endl;

    switch (io)
    {
        case IO_METHOD_READ:
            cerr << "in read Frame method read" << endl;
            if (-1 == read(fd, buffers[0].start, buffers[0].length))
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;
                    case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                    default:
                        return errno_exit("read", errmsg);
                }
            }
            //process_image (buffers[0].start);
            break;

        case IO_METHOD_MMAP:
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: using MMAP to recover frame buffer", __FUNCTION__);
            CLEAR(buf);

            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            /* For debugging purposes */
            if (false)
            {
                for (i = 0; i < n_buffers; ++i)
                {
                    buf.index = i;
                    if (-1 == XIOCTL(fd, VIDIOC_QUERYBUF, &buf))
                        switch (errno)
                        {
                            case EINVAL:
                                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                             "%s: invalid buffer query, doing as if buffer was in output queue",
                                             __FUNCTION__);
                                break;

                            default:
                                return errno_exit("ReadFrame IO_METHOD_MMAP: VIDIOC_QUERYBUF", errmsg);
                        }

                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: " DBG_STR_BUF, __FUNCTION__, DBG_BUF(buf));
                }
            }

            if (-1 == XIOCTL(fd, VIDIOC_DQBUF, &buf))
                switch (errno)
                {
                    case EAGAIN:
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                     "%s: no buffer found with DQBUF ioctl (EAGAIN) - frame not ready or not requested",
                                     __FUNCTION__);
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */
                        /* Fall through */
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                     "%s: transitory internal error with DQBUF ioctl (EIO)", __FUNCTION__);
                        return 0;

                    case EINVAL:
                    case EPIPE:
                    default:
                        return errno_exit("ReadFrame IO_METHOD_MMAP: VIDIOC_DQBUF", errmsg);
                }

            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: buffer #%d dequeued from fd:%d\n", __FUNCTION__,
                         buf.index, fd);

            if (buf.flags & V4L2_BUF_FLAG_ERROR)
            {
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                             "%s: recoverable error with DQBUF ioctl (BUF_FLAG_ERROR) - frame should be dropped",
                             __FUNCTION__);
                if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                    return errno_exit("ReadFrame IO_METHOD_MMAP: VIDIOC_QBUF", errmsg);
                buf.bytesused = 0;
                return 0;
            }

            if (!is_compressed() && buf.bytesused != fmt.fmt.pix.sizeimage)
            {
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                             "%s: frame is %d-byte long, expected %d - frame should be dropped", __FUNCTION__,
                             buf.bytesused, fmt.fmt.pix.sizeimage);

                if (false)
                {
                    unsigned char const *b   = (unsigned char const *)buffers[buf.index].start;
                    unsigned char const *end = b + buf.bytesused;

                    do
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                     "%s: [%p] %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
                                     __FUNCTION__, b, b[0 * 4 + 0], b[0 * 4 + 1], b[0 * 4 + 2], b[0 * 4 + 3],
                                     b[1 * 4 + 0], b[1 * 4 + 1], b[1 * 4 + 2], b[1 * 4 + 3], b[2 * 4 + 0], b[2 * 4 + 1],
                                     b[2 * 4 + 2], b[2 * 4 + 3], b[3 * 4 + 0], b[3 * 4 + 1], b[3 * 4 + 2],
                                     b[3 * 4 + 3]);
                    while ((b += 16) < end);
                }

                if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                    return errno_exit("ReadFrame IO_METHOD_MMAP: VIDIOC_QBUF", errmsg);
                buf.bytesused = 0;
                return 0;
            }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0))
            /* TODO: the timestamp can be checked against the expected exposure to validate the frame - doesn't work, yet */
            switch (buf.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK)
            {
                case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
                /* FIXME: try monotonic clock when timestamp clock type is unknown */
                case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
                {
                    struct timespec uptime = { 0, 0 };
                    clock_gettime(CLOCK_MONOTONIC, &uptime);

                    struct timeval epochtime = { 0, 0 };
                    /*gettimeofday(&epochtime, nullptr); uncomment this to get the timestamp from epoch start */

                    float const secs =
                        (epochtime.tv_sec - uptime.tv_sec + buf.timestamp.tv_sec) +
                        (epochtime.tv_usec - uptime.tv_nsec / 1000.0f + buf.timestamp.tv_usec) / 1000000.0f;

                    if (V4L2_BUF_FLAG_TSTAMP_SRC_SOE == (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK))
                    {
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                     "%s: frame exposure started %.03f seconds ago", __FUNCTION__, -secs);
                    }
                    else if (V4L2_BUF_FLAG_TSTAMP_SRC_EOF == (buf.flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK))
                    {
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                     "%s: frame finished capturing %.03f seconds ago", __FUNCTION__, -secs);
                    }
                    else
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: unsupported timestamp in frame",
                                     __FUNCTION__);

                    break;
                }

                case V4L2_BUF_FLAG_TIMESTAMP_COPY:
                default:
                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: no usable timestamp found in frame",
                                 __FUNCTION__);
            }
#endif

            /* TODO: there is probably a better error handling than asserting the buffer index */
            assert(buf.index < n_buffers);

            if (dodecode)
            {
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: [%p] decoding %d-byte buffer %p cropset %c",
                             __FUNCTION__, decoder, buf.bytesused, buffers[buf.index].start, cropset ? 'Y' : 'N');
                decoder->decode((unsigned char *)(buffers[buf.index].start), &buf);
            }

            /*
            if (dorecord)
            {
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: [%p] recording %d-byte buffer %p", __FUNCTION__,
                             recorder, buf.bytesused, buffers[buf.index].start);
                recorder->writeFrame((unsigned char *)(buffers[buf.index].start));
            }
            */

            //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"lxstate is %d, dropFrame %c\n", lxstate, (dropFrame?'Y':'N'));

            /* Requeue buffer */
            if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                return errno_exit("ReadFrame IO_METHOD_MMAP: VIDIOC_QBUF", errmsg);

            if (lxstate == LX_ACTIVE)
            {
                /* Call provided callback function if any */
                //if (callback && !dorecord)
                if (callback)
                    (*callback)(uptr);
            }

            if (lxstate == LX_TRIGGERED)
                lxstate = LX_ACTIVE;

            break;

        case IO_METHOD_USERPTR:
            cerr << "in read Frame method userptr" << endl;
            CLEAR(buf);

            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            if (-1 == XIOCTL(fd, VIDIOC_DQBUF, &buf))
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;
                    case EIO:
                    /* Could ignore EIO, see spec. */
                    /* fall through */
                    default:
                        errno_exit("VIDIOC_DQBUF", errmsg);
                }
            }

            for (i = 0; i < n_buffers; ++i)
                if (buf.m.userptr == (unsigned long)buffers[i].start && buf.length == buffers[i].length)
                    break;

            assert(i < n_buffers);

            //process_image ((void *) buf.m.userptr);

            if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                errno_exit("ReadFrame IO_METHOD_USERPTR: VIDIOC_QBUF", errmsg);

            break;
    }

    return 0;
}

int V4L2_Base::stop_capturing(char *errmsg)
{
    enum v4l2_buf_type type;

    switch (io)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
        /* Kernel 3.11 problem with streamoff: vb2_is_busy(queue) remains true so we can not change anything without diconnecting */
        /* It seems that device should be closed/reopened to change any capture format settings. From V4L2 API Spec. (Data Negotiation) */
        /* Switching the logical stream or returning into "panel mode" is possible by closing and reopening the device. */
        /* Drivers may support a switch using VIDIOC_S_FMT. */
        case IO_METHOD_USERPTR:
            // N.B. I used this as a hack to solve a problem with capturing a frame
            // long time ago. I recently tried taking this hack off, and it worked fine!

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (selectCallBackID != -1)
            {
                IERmCallback(selectCallBackID);
                selectCallBackID = -1;
            }
            streamactive = false;
            if (-1 == XIOCTL(fd, VIDIOC_STREAMOFF, &type))
                return errno_exit("VIDIOC_STREAMOFF", errmsg);
            break;
    }
    //uninit_device(errmsg);

    return 0;
}

int V4L2_Base::start_capturing(char *errmsg)
{
    unsigned int i;
    enum v4l2_buf_type type;

    if (!streamedonce)
        init_device(errmsg);

    switch (io)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i)
            {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index  = i;
                //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"v4l2_start_capturing: enqueuing buffer %d for fd=%d\n", buf.index, fd);
                /*if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                return errno_exit ("StartCapturing IO_METHOD_MMAP: VIDIOC_QBUF", errmsg);*/
                XIOCTL(fd, VIDIOC_QBUF, &buf);
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == XIOCTL(fd, VIDIOC_STREAMON, &type))
                return errno_exit("VIDIOC_STREAMON", errmsg);

            selectCallBackID = IEAddCallback(fd, newFrame, this);
            streamactive     = true;

            break;

        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i)
            {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory    = V4L2_MEMORY_USERPTR;
                buf.m.userptr = (unsigned long)buffers[i].start;
                buf.length    = buffers[i].length;

                if (-1 == XIOCTL(fd, VIDIOC_QBUF, &buf))
                    return errno_exit("StartCapturing IO_METHOD_USERPTR: VIDIOC_QBUF", errmsg);
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == XIOCTL(fd, VIDIOC_STREAMON, &type))
                return errno_exit("VIDIOC_STREAMON", errmsg);

            break;
    }
    //if (dropFrameEnabled)
    //dropFrame = dropFrameCount;
    streamedonce = true;
    return 0;
}

void V4L2_Base::newFrame(int /*fd*/, void *p)
{
    char errmsg[ERRMSGSIZ];

    ((V4L2_Base *)(p))->read_frame(errmsg);
}

int V4L2_Base::uninit_device(char *errmsg)
{
    switch (io)
    {
        case IO_METHOD_READ:
            free(buffers[0].start);
            break;

        case IO_METHOD_MMAP:
            for (unsigned int i = 0; i < n_buffers; ++i)
                if (-1 == munmap(buffers[i].start, buffers[i].length))
                    return errno_exit("munmap", errmsg);
            break;

        case IO_METHOD_USERPTR:
            for (unsigned int i = 0; i < n_buffers; ++i)
                free(buffers[i].start);
            break;
    }

    free(buffers);

    return 0;
}

void V4L2_Base::init_read(unsigned int buffer_size)
{
    buffers = (buffer *)calloc(1, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start  = malloc(buffer_size);

    if (!buffers[0].start)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}

int V4L2_Base::init_mmap(char *errmsg)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    //req.count               = 1;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == XIOCTL(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%.*s does not support memory mapping\n", (int)sizeof(dev_name), dev_name);
            snprintf(errmsg, ERRMSGSIZ, "%.*s does not support memory mapping\n", (int)sizeof(dev_name), dev_name);
            return -1;
        }
        else
        {
            return errno_exit("VIDIOC_REQBUFS", errmsg);
        }
    }

    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %.*s\n", (int)sizeof(dev_name), dev_name);
        snprintf(errmsg, ERRMSGSIZ, "Insufficient buffer memory on %.*s\n", (int)sizeof(dev_name), dev_name);
        return -1;
    }

    buffers = (buffer *)calloc(req.count, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "buffers. Out of memory\n");
        strncpy(errmsg, "buffers. Out of memory\n", ERRMSGSIZ);
        return -1;
    }

    for (n_buffers = 0; n_buffers < req.count; n_buffers++)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers;

        if (-1 == XIOCTL(fd, VIDIOC_QUERYBUF, &buf))
            return errno_exit("VIDIOC_QUERYBUF", errmsg);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(nullptr /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */,
                                        MAP_SHARED /* recommended */, fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            return errno_exit("mmap", errmsg);
    }

    return 0;
}

void V4L2_Base::init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    char errmsg[ERRMSGSIZ];

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == XIOCTL(fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%.*s does not support user pointer i/o\n", (int)sizeof(dev_name), dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS", errmsg);
        }
    }

    buffers = (buffer *)calloc(4, sizeof(*buffers));

    if (!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers)
    {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start  = malloc(buffer_size);

        if (!buffers[n_buffers].start)
        {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
    }
}

int V4L2_Base::check_device(char *errmsg)
{
    struct v4l2_input input_avail;

    if (-1 == XIOCTL(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%.*s is no V4L2 device\n", (int)sizeof(dev_name), dev_name);
            snprintf(errmsg, ERRMSGSIZ, "%.*s is no V4L2 device\n", (int)sizeof(dev_name), dev_name);
            return -1;
        }
        else
        {
            return errno_exit("VIDIOC_QUERYCAP", errmsg);
        }
    }

    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Driver %.*s (version %u.%u.%u)", (int)sizeof(cap.driver),
                 cap.driver, (cap.version >> 16) & 0xFF, (cap.version >> 8) & 0xFF, (cap.version & 0xFF));
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  card: %.*s", (int)sizeof(cap.card), cap.card);
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  bus:  %.*s", (int)sizeof(cap.bus_info), cap.bus_info);

    setframerate = &V4L2_Base::stdsetframerate;
    getframerate = &V4L2_Base::stdgetframerate;

//    if (!(strncmp((const char *)cap.driver, "pwc", sizeof(cap.driver))))
//    {
        //unsigned int qual = 3;
        //pwc driver does not allow to get current fps with VIDIOCPWC
        //frameRate.numerator=1; // using default module load fps
        //frameRate.denominator=10;
        //if (ioctl(fd, VIDIOCPWCSLED, &qual)) {
        //  DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"ioctl: can't set pwc video quality to High (uncompressed).\n");
        //}
        //else
        //  DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"  Setting pwc video quality to High (uncompressed)\n");
        //setframerate=&V4L2_Base::pwcsetframerate;
//    }

    DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Driver capabilities:");
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VIDEO_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VIDEO_OUTPUT");
    if (cap.capabilities & V4L2_CAP_VIDEO_OVERLAY)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VIDEO_OVERLAY");
    if (cap.capabilities & V4L2_CAP_VBI_CAPTURE)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VBI_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VBI_OUTPUT)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VBI_OUTPUT");
    if (cap.capabilities & V4L2_CAP_SLICED_VBI_CAPTURE)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_SLICED_VBI_CAPTURE");
    if (cap.capabilities & V4L2_CAP_SLICED_VBI_OUTPUT)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_SLICED_VBI_OUTPUT");
    if (cap.capabilities & V4L2_CAP_RDS_CAPTURE)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_RDS_CAPTURE");
    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_VIDEO_OUTPUT_OVERLAY");
    if (cap.capabilities & V4L2_CAP_TUNER)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_TUNER");
    if (cap.capabilities & V4L2_CAP_AUDIO)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_AUDIO");
    if (cap.capabilities & V4L2_CAP_RADIO)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_RADIO");
    if (cap.capabilities & V4L2_CAP_READWRITE)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_READWRITE");
    if (cap.capabilities & V4L2_CAP_ASYNCIO)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_ASYNCIO");
    if (cap.capabilities & V4L2_CAP_STREAMING)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "  V4L2_CAP_STREAMING");
    /*if (cap.capabilities & V4L2_CAP_EXT_PIX_FORMAT) {
      has_ext_pix_format=true;
      DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"  V4L2_CAP_EXT_PIX_FORMAT\n");
    }*/
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%.*s is no video capture device\n", (int)sizeof(dev_name), dev_name);
        snprintf(errmsg, ERRMSGSIZ, "%.*s is no video capture device", (int)sizeof(dev_name), dev_name);
        return -1;
    }

    switch (io)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
            {
                fprintf(stderr, "%.*s does not support read i/o", (int)sizeof(dev_name), dev_name);
                snprintf(errmsg, ERRMSGSIZ, "%.*s does not support read i/o", (int)sizeof(dev_name), dev_name);
                return -1;
            }
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                fprintf(stderr, "%.*s does not support streaming i/o", (int)sizeof(dev_name), dev_name);
                snprintf(errmsg, ERRMSGSIZ, "%.*s does not support streaming i/o", (int)sizeof(dev_name), dev_name);
                return -1;
            }

            break;
    }

    /* Select video input, video standard and tune here. */

    if (-1 == ioctl(fd, VIDIOC_G_INPUT, &input.index))
    {
        perror("VIDIOC_G_INPUT");
        exit(EXIT_FAILURE);
    }

    DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Enumerating available Inputs:");
    for (input_avail.index = 0; ioctl(fd, VIDIOC_ENUMINPUT, &input_avail) != -1; input_avail.index++)
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%2d. %.*s (type %s)%s", input_avail.index,
                     (int)sizeof(input_avail.name), input_avail.name,
                     (input_avail.type == V4L2_INPUT_TYPE_TUNER ? "Tuner/RF Demodulator" : "Composite/S-Video"),
                     input.index == input_avail.index ? " current" : "");
    if (errno != EINVAL)
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "(problem enumerating inputs)");
    enumeratedInputs = input_avail.index;

    /* Cropping */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cancrop      = true;
    if (-1 == XIOCTL(fd, VIDIOC_CROPCAP, &cropcap))
    {
        perror("VIDIOC_CROPCAP");
        crop.c.top = -1;
        cancrop    = false;
        /* Errors ignored. */
    }
    if (cancrop)
    {
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                     " Crop capabilities: bounds  = (top=%d, left=%d, width=%d, height=%d)", cropcap.bounds.top,
                     cropcap.bounds.left, cropcap.bounds.width, cropcap.bounds.height);
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                     " Crop capabilities: defrect = (top=%d, left=%d, width=%d, height=%d)", cropcap.defrect.top,
                     cropcap.defrect.left, cropcap.defrect.width, cropcap.defrect.height);
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, " Crop capabilities: pixelaspect = %d / %d",
                     cropcap.pixelaspect.numerator, cropcap.pixelaspect.denominator);
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Explicitely resetting crop area to default...");
        crop.c.top    = cropcap.defrect.top;
        crop.c.left   = cropcap.defrect.left;
        crop.c.width  = cropcap.defrect.width;
        crop.c.height = cropcap.defrect.height;
        if (-1 == XIOCTL(fd, VIDIOC_S_CROP, &crop))
        {
            perror("VIDIOC_S_CROP");
            cancrop = false;
            /* Errors ignored. */
        }
        if (-1 == XIOCTL(fd, VIDIOC_G_CROP, &crop))
        {
            perror("VIDIOC_G_CROP");
            crop.c.top = -1;
            cancrop    = false;
            /* Errors ignored. */
        }
    }
    decoder->usesoftcrop(!cancrop);

    // Enumerating capture format
    {
        struct v4l2_fmtdesc fmt_avail;
        fmt_avail.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //DEBUG(INDI::Logger::DBG_SESSION,"Available Capture Image formats:");
        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Enumerating available Capture Image formats:");
        for (fmt_avail.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmt_avail) != -1; fmt_avail.index++)
        {
            //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,INDI::Logger::DBG_SESSION,"\t%d. %s (%c%c%c%c) %s\n", fmt_avail.index, fmt_avail.description, (fmt_avail.pixelformat)&0xFF, (fmt_avail.pixelformat >> 8)&0xFF,
            //     (fmt_avail.pixelformat >> 16)&0xFF, (fmt_avail.pixelformat >> 24)&0xFF, (decoder->issupportedformat(fmt_avail.pixelformat)?"supported":"UNSUPPORTED"));
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%2d. Format %.*s (%c%c%c%c) is %s", fmt_avail.index,
                         (int)sizeof(fmt_avail.description), fmt_avail.description, (fmt_avail.pixelformat) & 0xFF,
                         (fmt_avail.pixelformat >> 8) & 0xFF, (fmt_avail.pixelformat >> 16) & 0xFF,
                         (fmt_avail.pixelformat >> 24) & 0xFF,
                         (decoder->issupportedformat(fmt_avail.pixelformat) ? "supported" : "UNSUPPORTED"));
            {
                // Enumerating frame sizes available for this pixel format
                struct v4l2_frmsizeenum frm_sizeenum;
                frm_sizeenum.pixel_format = fmt_avail.pixelformat;
                DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                            "    Enumerating available Frame sizes/rates for this format:");
                for (frm_sizeenum.index = 0; XIOCTL(fd, VIDIOC_ENUM_FRAMESIZES, &frm_sizeenum) != -1;
                     frm_sizeenum.index++)
                {
                    switch (frm_sizeenum.type)
                    {
                        case V4L2_FRMSIZE_TYPE_DISCRETE:
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                         "    %2d. (Discrete)  width %d x height %d\n", frm_sizeenum.index,
                                         frm_sizeenum.discrete.width, frm_sizeenum.discrete.height);
                            break;
                        case V4L2_FRMSIZE_TYPE_STEPWISE:
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                         "        (Stepwise)  min. width %d, max. width %d step width %d",
                                         frm_sizeenum.stepwise.min_width, frm_sizeenum.stepwise.max_width,
                                         frm_sizeenum.stepwise.step_width);
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                         "        (Stepwise)  min. height %d, max. height %d step height %d ",
                                         frm_sizeenum.stepwise.min_height, frm_sizeenum.stepwise.max_height,
                                         frm_sizeenum.stepwise.step_height);
                            break;
                        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                         "        (Continuous--step=1)  min. width %d, max. width %d",
                                         frm_sizeenum.stepwise.min_width, frm_sizeenum.stepwise.max_width);
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                         "        (Continuous--step=1)  min. height %d, max. height %d ",
                                         frm_sizeenum.stepwise.min_height, frm_sizeenum.stepwise.max_height);
                            break;
                        default:
                            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "        Unknown Frame size type: %d\n",
                                         frm_sizeenum.type);
                            break;
                    }
                    {
                        // Enumerating frame intervals available for this frame size and  this pixel format
                        struct v4l2_frmivalenum frmi_valenum;
                        frmi_valenum.pixel_format = fmt_avail.pixelformat;
                        if (frm_sizeenum.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                        {
                            frmi_valenum.width  = frm_sizeenum.discrete.width;
                            frmi_valenum.height = frm_sizeenum.discrete.height;
                        }
                        else
                        {
                            frmi_valenum.width  = frm_sizeenum.stepwise.max_width;
                            frmi_valenum.height = frm_sizeenum.stepwise.max_height;
                        }
                        frmi_valenum.type                      = 0;
                        frmi_valenum.stepwise.min.numerator    = 0;
                        frmi_valenum.stepwise.min.denominator  = 0;
                        frmi_valenum.stepwise.max.numerator    = 0;
                        frmi_valenum.stepwise.max.denominator  = 0;
                        frmi_valenum.stepwise.step.numerator   = 0;
                        frmi_valenum.stepwise.step.denominator = 0;
                        DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "        Frame intervals:");
                        for (frmi_valenum.index = 0; XIOCTL(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmi_valenum) != -1;
                             frmi_valenum.index++)
                        {
                            switch (frmi_valenum.type)
                            {
                                case V4L2_FRMIVAL_TYPE_DISCRETE:
                                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "        %d/%d s",
                                                 frmi_valenum.discrete.numerator, frmi_valenum.discrete.denominator);
                                    break;
                                case V4L2_FRMIVAL_TYPE_STEPWISE:
                                    DEBUGFDEVICE(
                                        deviceName, INDI::Logger::DBG_DEBUG,
                                        "        (Stepwise)  min. %d/%ds, max. %d / %d s, step %d / %d s",
                                        frmi_valenum.stepwise.min.numerator, frmi_valenum.stepwise.min.denominator,
                                        frmi_valenum.stepwise.max.numerator, frmi_valenum.stepwise.max.denominator,
                                        frmi_valenum.stepwise.step.numerator, frmi_valenum.stepwise.step.denominator);
                                    break;
                                case V4L2_FRMIVAL_TYPE_CONTINUOUS:
                                    DEBUGFDEVICE(
                                        deviceName, INDI::Logger::DBG_DEBUG,
                                        "        (Continuous)  min. %d / %d s, max. %d / %d s",
                                        frmi_valenum.stepwise.min.numerator, frmi_valenum.stepwise.min.denominator,
                                        frmi_valenum.stepwise.max.numerator, frmi_valenum.stepwise.max.denominator);
                                    break;
                                default:
                                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                                 "        Unknown Frame rate type: %d", frmi_valenum.type);
                                    break;
                            }
                        }
                        if (frmi_valenum.index == 0)
                        {
                            perror("VIDIOC_ENUM_FRAMEINTERVALS");
                            switch (frmi_valenum.type)
                            {
                                case V4L2_FRMIVAL_TYPE_DISCRETE:
                                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "        %d/%d s",
                                                 frmi_valenum.discrete.numerator, frmi_valenum.discrete.denominator);
                                    break;
                                case V4L2_FRMIVAL_TYPE_STEPWISE:
                                    DEBUGFDEVICE(
                                        deviceName, INDI::Logger::DBG_DEBUG,
                                        "        (Stepwise)  min. %d/%ds, max. %d / %d s, step %d / %d s",
                                        frmi_valenum.stepwise.min.numerator, frmi_valenum.stepwise.min.denominator,
                                        frmi_valenum.stepwise.max.numerator, frmi_valenum.stepwise.max.denominator,
                                        frmi_valenum.stepwise.step.numerator, frmi_valenum.stepwise.step.denominator);
                                    break;
                                case V4L2_FRMIVAL_TYPE_CONTINUOUS:
                                    DEBUGFDEVICE(
                                        deviceName, INDI::Logger::DBG_DEBUG,
                                        "        (Continuous)  min. %d / %d s, max. %d / %d s",
                                        frmi_valenum.stepwise.min.numerator, frmi_valenum.stepwise.min.denominator,
                                        frmi_valenum.stepwise.max.numerator, frmi_valenum.stepwise.max.denominator);
                                    break;
                                default:
                                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,
                                                 "        Unknown Frame rate type: %d", frmi_valenum.type);
                                    break;
                            }
                        }
                        //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"error %d, %s\n", errno, strerror (errno));
                    }
                }
            }
        }
        if (errno != EINVAL)
            DEBUGDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Problem enumerating capture formats.");
        enumeratedCaptureFormats = fmt_avail.index;
    }

    // CLEAR (fmt);

    //    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //    if (-1 == XIOCTL(fd, VIDIOC_G_FMT, &fmt))
    //         return errno_exit ("VIDIOC_G_FMT", errmsg);

    //     fmt.fmt.pix.width       = (width == -1)       ? fmt.fmt.pix.width : width;
    //     fmt.fmt.pix.height      = (height == -1)      ? fmt.fmt.pix.height : height;
    //     fmt.fmt.pix.pixelformat = (pixelFormat == -1) ? fmt.fmt.pix.pixelformat : pixelFormat;
    //     //fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    //     if (-1 == XIOCTL(fd, VIDIOC_S_FMT, &fmt))
    //             return errno_exit ("VIDIOC_S_FMT", errmsg);

    //     /* Note VIDIOC_S_FMT may change width and height. */

    // 	/* Buggy driver paranoia. */
    // 	min = fmt.fmt.pix.width * 2;
    // 	if (fmt.fmt.pix.bytesperline < min)
    // 		fmt.fmt.pix.bytesperline = min;
    // 	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    // 	if (fmt.fmt.pix.sizeimage < min)
    // 		fmt.fmt.pix.sizeimage = min;

    /* Refresh the instance format with the current device format */
    CLEAR(fmt);
    return ioctl_set_format(fmt, errmsg);
}

int V4L2_Base::init_device(char *errmsg)
{
    //findMinMax();

    // decode allocBuffers();

    lxstate = LX_ACTIVE;

    switch (io)
    {
        case IO_METHOD_READ:
            init_read(fmt.fmt.pix.sizeimage);
            break;

        case IO_METHOD_MMAP:
            return init_mmap(errmsg);
            break;

        case IO_METHOD_USERPTR:
            init_userp(fmt.fmt.pix.sizeimage);
            break;
    }
    return 0;
}

void V4L2_Base::close_device()
{
    char errmsg[ERRMSGSIZ];
    uninit_device(errmsg);

    if (-1 == close(fd))
        errno_exit("close", errmsg);

    fd = -1;
}

int V4L2_Base::open_device(const char *devpath, char *errmsg)
{
    struct stat st;

    strncpy(dev_name, devpath, 64);

    if (-1 == stat(dev_name, &st))
    {
        fprintf(stderr, "Cannot identify %.*s: %d, %s\n", (int)sizeof(dev_name), dev_name, errno, strerror(errno));
        snprintf(errmsg, ERRMSGSIZ, "Cannot identify %.*s: %d, %s\n", (int)sizeof(dev_name), dev_name, errno,
                 strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%.*s is no device\n", (int)sizeof(dev_name), dev_name);
        snprintf(errmsg, ERRMSGSIZ, "%.*s is no device\n", (int)sizeof(dev_name), dev_name);
        return -1;
    }

    fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open %.*s: %d, %s\n", (int)sizeof(dev_name), dev_name, errno, strerror(errno));
        snprintf(errmsg, ERRMSGSIZ, "Cannot open %.*s: %d, %s\n", (int)sizeof(dev_name), dev_name, errno,
                 strerror(errno));
        return -1;
    }

    streamedonce = false;
    snprintf(errmsg, ERRMSGSIZ, "%s\n", strerror(0));
    return 0;
}

/* @brief Storing inputs in a switch vector property, marking current as selected.
 *
 * @param inputssp is the property to store to (nothing is stored if null).
 */
void V4L2_Base::getinputs(ISwitchVectorProperty *inputssp)
{
    if (!inputssp)
        return;

    struct v4l2_input input_avail;

    /* Allocate inputs from previously enumerated count */
    size_t const inputsLen = enumeratedInputs * sizeof(ISwitch);
    ISwitch *inputs        = (ISwitch *)malloc(inputsLen);
    if (!inputs)
        exit(EXIT_FAILURE);
    memset(inputs, 0, inputsLen);

    /* Ask device about each input */
    for (input_avail.index = 0; (int)input_avail.index < enumeratedInputs; input_avail.index++)
    {
        /* Enumeration ends with EINVAL */
        if (XIOCTL(fd, VIDIOC_ENUMINPUT, &input_avail))
            break;

        /* Store input description */
        strncpy(inputs[input_avail.index].name, (const char *)input_avail.name, MAXINDINAME);
        strncpy(inputs[input_avail.index].label, (const char *)input_avail.name, MAXINDILABEL);
    }

    /* Free inputs before replacing */
    if (inputssp->sp)
        free(inputssp->sp);

    /* Store inputs */
    inputssp->sp  = inputs;
    inputssp->nsp = input_avail.index;

    IUResetSwitch(inputssp);

    /* And mark current */
    inputs[input.index].s = ISS_ON;
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Current video input is   %d. %.*s", input.index,
                 (int)sizeof(inputs[input.index].name), inputs[input.index].name);
}

int V4L2_Base::setinput(unsigned int inputindex, char *errmsg)
{
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Setting Video input to %d", inputindex);
    if (streamedonce)
    {
        close_device();

        if (open_device(path, errmsg))
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%s: failed reopening device %s (%s)", __FUNCTION__, path,
                         errmsg);
            return -1;
        }
    }
    if (-1 == XIOCTL(fd, VIDIOC_S_INPUT, &inputindex))
    {
        return errno_exit("VIDIOC_S_INPUT", errmsg);
    }
    if (-1 == XIOCTL(fd, VIDIOC_G_INPUT, &input.index))
    {
        return errno_exit("VIDIOC_G_INPUT", errmsg);
    }
    //decode  reallocate_buffers=true;
    return 0;
}

/* @brief Storing capture formats in a switch vector property, marking current as selected.
 *
 * @param captureformatssp is the property to store to (nothing is stored if null).
 */
void V4L2_Base::getcaptureformats(ISwitchVectorProperty *captureformatssp)
{
    if (!captureformatssp)
        return;

    struct v4l2_fmtdesc fmt_avail;

    /* Allocate capture formats from preliminary enumerated count */
    size_t const formatsLen = enumeratedCaptureFormats * sizeof(ISwitch);
    ISwitch *formats        = (ISwitch *)malloc(formatsLen);
    if (!formats)
        exit(EXIT_FAILURE);
    memset(formats, 0, formatsLen);

    /* Ask device about each format */
    fmt_avail.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    for (fmt_avail.index = 0; (int)fmt_avail.index < enumeratedCaptureFormats; fmt_avail.index++)
    {
        /* Enumeration ends with EINVAL */
        if (XIOCTL(fd, VIDIOC_ENUM_FMT, &fmt_avail))
            break;

        /* Store format description */
        strncpy(formats[fmt_avail.index].name, (const char *)fmt_avail.description, MAXINDINAME);
        strncpy(formats[fmt_avail.index].label, (const char *)fmt_avail.description, MAXINDILABEL);

        /* And store pixel format for reference */
        /* FIXME: store pixel format as void pointer to avoid that malloc */
        formats[fmt_avail.index].aux = (int *)malloc(sizeof(int));
        if (!formats[fmt_avail.index].aux)
            exit(EXIT_FAILURE);
        *(int *)formats[fmt_avail.index].aux = fmt_avail.pixelformat;
    }

    /* Free formats before replacing */
    if (captureformatssp->sp)
        free(captureformatssp->sp);

    /* Store formats */
    captureformatssp->sp  = formats;
    captureformatssp->nsp = fmt_avail.index;
    IUResetSwitch(captureformatssp);

    /* And mark current */
    for (unsigned int i = 0; i < fmt_avail.index; i++)
    {
        if ((int)fmt.fmt.pix.pixelformat == *(int *)formats[i].aux)
        {
            formats[i].s = ISS_ON;
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Current capture format is %d. %c%c%c%c.", i,
                         (fmt.fmt.pix.pixelformat) & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
                         (fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
        }
        else
            formats[i].s = ISS_OFF;
    }
}

/* @brief Setting the pixel format of the capture.
 *
 * @param captureformat is the identifier of the pixel format to set.
 * @param errmsg is the error message to return in case of failure.
 * @return 0 if successful, else -1 with error message updated.
 */
int V4L2_Base::setcaptureformat(unsigned int captureformat, char *errmsg)
{
    struct v4l2_format new_fmt;
    CLEAR(new_fmt);

    new_fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    new_fmt.fmt.pix.pixelformat = captureformat;

    return ioctl_set_format(new_fmt, errmsg);
}

void V4L2_Base::getcapturesizes(ISwitchVectorProperty *capturesizessp, INumberVectorProperty *capturesizenp)
{
    struct v4l2_frmsizeenum frm_sizeenum;
    ISwitch *sizes     = nullptr;
    INumber *sizevalue = nullptr;
    bool sizefound     = false;
    if (capturesizessp->sp)
        free(capturesizessp->sp);
    if (capturesizenp->np)
        free(capturesizenp->np);

    frm_sizeenum.pixel_format = fmt.fmt.pix.pixelformat;
    //DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"\t  Available Frame sizes/rates for this format:\n");
    for (frm_sizeenum.index = 0; XIOCTL(fd, VIDIOC_ENUM_FRAMESIZES, &frm_sizeenum) != -1; frm_sizeenum.index++)
    {
        switch (frm_sizeenum.type)
        {
            case V4L2_FRMSIZE_TYPE_DISCRETE:
                sizes = (sizes == nullptr) ? (ISwitch *)malloc(sizeof(ISwitch)) :
                                             (ISwitch *)realloc(sizes, (frm_sizeenum.index + 1) * sizeof(ISwitch));

                snprintf(sizes[frm_sizeenum.index].name, MAXINDINAME, "%dx%d", frm_sizeenum.discrete.width,
                         frm_sizeenum.discrete.height);
                snprintf(sizes[frm_sizeenum.index].label, MAXINDINAME, "%dx%d", frm_sizeenum.discrete.width,
                         frm_sizeenum.discrete.height);
                sizes[frm_sizeenum.index].s = ISS_OFF;
                if (!sizefound)
                {
                    if ((fmt.fmt.pix.width == frm_sizeenum.discrete.width) &&
                        (fmt.fmt.pix.height == frm_sizeenum.discrete.height))
                    {
                        sizes[frm_sizeenum.index].s = ISS_ON;
                        sizefound                   = true;
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Current capture size is (%d.)  %dx%d",
                                     frm_sizeenum.index, frm_sizeenum.discrete.width, frm_sizeenum.discrete.height);
                    }
                }
                break;
            case V4L2_FRMSIZE_TYPE_STEPWISE:
            case V4L2_FRMSIZE_TYPE_CONTINUOUS:
                sizevalue = (INumber *)malloc(2 * sizeof(INumber));
                IUFillNumber(sizevalue, "Width", "Width", "%.0f", frm_sizeenum.stepwise.min_width,
                             frm_sizeenum.stepwise.max_width, frm_sizeenum.stepwise.step_width, fmt.fmt.pix.width);
                IUFillNumber(sizevalue + 1, "Height", "Height", "%.0f", frm_sizeenum.stepwise.min_height,
                             frm_sizeenum.stepwise.max_height, frm_sizeenum.stepwise.step_height, fmt.fmt.pix.height);
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Current capture size is %dx%d", fmt.fmt.pix.width,
                             fmt.fmt.pix.height);
                break;
            default:
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Unknown Frame size type: %d", frm_sizeenum.type);
                break;
        }
    }

    if (sizes != nullptr)
    {
        capturesizessp->sp  = sizes;
        capturesizessp->nsp = frm_sizeenum.index;
        capturesizenp->np   = nullptr;
    }
    else
    {
        capturesizenp->np  = sizevalue;
        capturesizenp->nnp = 2;
        capturesizessp->sp = nullptr;
    }
}

/* @brief Updating the capture dimensions.
 *
 * @param w is the updated width of the capture.
 * @param h is the update height of the capture.
 * @param errmsg is the returned error message in case of failure.
 * @return 0 if successful, else -1 with error message updated.
 */
int V4L2_Base::setcapturesize(unsigned int w, unsigned int h, char *errmsg)
{
    struct v4l2_format new_fmt = fmt;

    new_fmt.fmt.pix.width  = w;
    new_fmt.fmt.pix.height = h;

    return ioctl_set_format(new_fmt, errmsg);
}

void V4L2_Base::getframerates(ISwitchVectorProperty *frameratessp, INumberVectorProperty *frameratenp)
{
    struct v4l2_frmivalenum frmi_valenum;
    ISwitch *rates     = nullptr;
    INumber *ratevalue = nullptr;
    struct v4l2_fract frate;

    if (frameratessp->sp)
        free(frameratessp->sp);
    if (frameratenp->np)
        free(frameratenp->np);
    frate = (this->*getframerate)();

    bzero(&frmi_valenum, sizeof(frmi_valenum));
    frmi_valenum.pixel_format = fmt.fmt.pix.pixelformat;
    frmi_valenum.width        = fmt.fmt.pix.width;
    frmi_valenum.height       = fmt.fmt.pix.height;

    for (frmi_valenum.index = 0; XIOCTL(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmi_valenum) != -1; frmi_valenum.index++)
    {
        switch (frmi_valenum.type)
        {
            case V4L2_FRMIVAL_TYPE_DISCRETE:
                rates = (rates == nullptr) ? (ISwitch *)malloc(sizeof(ISwitch)) :
                                             (ISwitch *)realloc(rates, (frmi_valenum.index + 1) * sizeof(ISwitch));
                snprintf(rates[frmi_valenum.index].name, MAXINDINAME, "%d/%d", frmi_valenum.discrete.numerator,
                         frmi_valenum.discrete.denominator);
                snprintf(rates[frmi_valenum.index].label, MAXINDINAME, "%d/%d", frmi_valenum.discrete.numerator,
                         frmi_valenum.discrete.denominator);
                if ((frate.numerator == frmi_valenum.discrete.numerator) &&
                    (frate.denominator == frmi_valenum.discrete.denominator))
                {
                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Current frame interval is %d/%d",
                                 frmi_valenum.discrete.numerator, frmi_valenum.discrete.denominator);
                    rates[frmi_valenum.index].s = ISS_ON;
                }
                else
                    rates[frmi_valenum.index].s = ISS_OFF;
                break;
            case V4L2_FRMIVAL_TYPE_STEPWISE:
            case V4L2_FRMIVAL_TYPE_CONTINUOUS:
                ratevalue = (INumber *)malloc(sizeof(INumber));
                IUFillNumber(ratevalue, "V4L2_FRAME_INTERVAL", "Frame Interval", "%.0f",
                             frmi_valenum.stepwise.min.numerator / (double)frmi_valenum.stepwise.min.denominator,
                             frmi_valenum.stepwise.max.numerator / (double)frmi_valenum.stepwise.max.denominator,
                             frmi_valenum.stepwise.step.numerator / (double)frmi_valenum.stepwise.step.denominator,
                             frate.numerator / (double)frate.denominator);
                break;
            default:
                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Unknown Frame rate type: %d", frmi_valenum.type);
                break;
        }
    }
    frameratessp->sp  = nullptr;
    frameratessp->nsp = 0;
    frameratenp->np   = nullptr;
    frameratenp->nnp  = 0;
    if (frmi_valenum.index != 0)
    {
        if (rates != nullptr)
        {
            frameratessp->sp  = rates;
            frameratessp->nsp = frmi_valenum.index;
        }
        else
        {
            frameratenp->np  = ratevalue;
            frameratenp->nnp = 1;
        }
    }
}

int V4L2_Base::setcroprect(int x, int y, int w, int h, char *errmsg)
{
    bool softcrop = false;

    crop.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.left   = x;
    crop.c.top    = y;
    crop.c.width  = w;
    crop.c.height = h;
    if ((int)(crop.c.left + crop.c.width) > (int)fmt.fmt.pix.width)
    {
        strncpy(errmsg, "crop width exceeds image width", ERRMSGSIZ);
        return -1;
    }
    if ((int)(crop.c.top + crop.c.height) > (int)fmt.fmt.pix.height)
    {
        strncpy(errmsg, "crop height exceeds image height", ERRMSGSIZ);
        return -1;
    }
    // we only support pair sizes
    if ((crop.c.width % 2 != 0) || (crop.c.height % 2 != 0))
    {
        strncpy(errmsg, "crop width/height must be pair", ERRMSGSIZ);
        return -1;
    }
    if ((crop.c.left == 0) && (crop.c.top == 0) && ((int)crop.c.width == (int)fmt.fmt.pix.width) &&
        ((int)crop.c.height == (int)fmt.fmt.pix.height))
    {
        cropset = false;
        decoder->resetcrop();
    }
    else
    {
        if (cancrop)
        {
            if (-1 == XIOCTL(fd, VIDIOC_S_CROP, &crop))
            {
                return errno_exit("VIDIOC_S_CROP", errmsg);
            }
            if (-1 == XIOCTL(fd, VIDIOC_G_CROP, &crop))
            {
                return errno_exit("VIDIOC_G_CROP", errmsg);
            }
        }
        softcrop = decoder->setcrop(crop);
        cropset  = true;
        if ((!cancrop) && (!softcrop))
        {
            cropset = false;
            strncpy(errmsg, "No hardware and software cropping for this format.", ERRMSGSIZ);
            return -1;
        }
    }
    //decode allocBuffers();
    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "V4L2 base setcroprect %dx%d at (%d, %d)", crop.c.width,
                 crop.c.height, crop.c.left, crop.c.top);
    return 0;
}

int V4L2_Base::getWidth()
{
    if (cropset)
        return crop.c.width;
    else
        return fmt.fmt.pix.width;
}

int V4L2_Base::getHeight()
{
    if (cropset)
        return crop.c.height;
    else
        return fmt.fmt.pix.height;
}

int V4L2_Base::getBpp()
{
    return bpp;
}

int V4L2_Base::getFormat()
{
    return fmt.fmt.pix.pixelformat;
}

struct v4l2_rect V4L2_Base::getcroprect()
{
    return crop.c;
}

int V4L2_Base::stdsetframerate(struct v4l2_fract frate, char *errmsg)
{
    struct v4l2_streamparm sparm;
    //if (!cansetrate) {sprintf(errmsg, "Can not set rate"); return -1;}
    bzero(&sparm, sizeof(struct v4l2_streamparm));
    sparm.type                      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    sparm.parm.capture.timeperframe = frate;
    if (-1 == XIOCTL(fd, VIDIOC_S_PARM, &sparm))
    {
        //cansetrate=false;
        return errno_exit("VIDIOC_S_PARM", errmsg);
    }
    return 0;
}

/* @brief Setting the framerate for Philips-based PWC devices.
 *
 * @param frate is the v4l2_fract structure defining framerate.
 * @param errmsg is the returned error message in case of error.
 * @return 0 if successful, else -1 with error message updated.
 */
int V4L2_Base::pwcsetframerate(struct v4l2_fract frate, char *errmsg)
{
    int const fps = frate.denominator / frate.numerator;

    struct v4l2_format new_fmt = fmt;
    new_fmt.fmt.pix.priv |= (fps << PWC_FPS_SHIFT);

    if (-1 == ioctl_set_format(new_fmt, errmsg))
        return errno_exit("pwcsetframerate", errmsg);

    frameRate = frate;

    return 0;
}

struct v4l2_fract V4L2_Base::stdgetframerate()
{
    struct v4l2_streamparm sparm;
    //if (!cansetrate) return frameRate;
    bzero(&sparm, sizeof(struct v4l2_streamparm));
    sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == XIOCTL(fd, VIDIOC_G_PARM, &sparm))
    {
        perror("VIDIOC_G_PARM");
    }
    else
    {
        frameRate = sparm.parm.capture.timeperframe;
    }
    //if (!(sparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) cansetrate=false;

    return frameRate;
}

char *V4L2_Base::getDeviceName()
{
    return ((char *)cap.card);
}

void V4L2_Base::getMaxMinSize(int &x_max, int &y_max, int &x_min, int &y_min)
{
    x_max = xmax;
    y_max = ymax;
    x_min = xmin;
    y_min = ymin;
}

/* @brief Setting the dimensions of the capture frame.
 *
 * @param x is the width of the capture frame.
 * @param y is the height of the capture frame.
 * @return 0 if successful, else -1.
 */
int V4L2_Base::setSize(int x, int y)
{
    char errmsg[ERRMSGSIZ];
    struct v4l2_format new_fmt = fmt;

    new_fmt.fmt.pix.width  = x;
    new_fmt.fmt.pix.height = y;

    if (-1 == ioctl_set_format(new_fmt, errmsg))
        return -1;

    /* PWC bug? It seems that setting the "wrong" width and height will mess something in the driver.
      Only 160x120, 320x280, and 640x480 are accepted. If I try to set it for example to 300x200, it wii
      get set to 320x280, which is fine, but then the video information is messed up for some reason. */
    //   XIOCTL(fd, VIDIOC_S_FMT, &fmt);

    //allocBuffers();

    return 0;
}

void V4L2_Base::setColorProcessing(bool quantization, bool colorconvert, bool linearization)
{
    INDI_UNUSED(colorconvert);
    decoder->setQuantization(quantization);
    decoder->setLinearization(linearization);
    bpp = decoder->getBpp();
}

unsigned char *V4L2_Base::getY()
{
    return decoder->getY();
}

unsigned char *V4L2_Base::getU()
{
    return decoder->getU();
}

unsigned char *V4L2_Base::getV()
{
    return decoder->getV();
}

/*unsigned char * V4L2_Base::getColorBuffer()
{
  return decoder->geColorBuffer();
}*/

unsigned char *V4L2_Base::getRGBBuffer()
{
    return decoder->getRGBBuffer();
}

float *V4L2_Base::getLinearY()
{
    return decoder->getLinearY();
}

void V4L2_Base::registerCallback(WPF *fp, void *ud)
{
    callback = fp;
    uptr     = ud;
}

void V4L2_Base::findMinMax()
{
    char errmsg[ERRMSGSIZ];
    struct v4l2_format tryfmt;
    CLEAR(tryfmt);

    xmin = xmax = fmt.fmt.pix.width;
    ymin = ymax = fmt.fmt.pix.height;

    tryfmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tryfmt.fmt.pix.width       = 10;
    tryfmt.fmt.pix.height      = 10;
    tryfmt.fmt.pix.pixelformat = fmt.fmt.pix.pixelformat;
    tryfmt.fmt.pix.field       = fmt.fmt.pix.field;

    if (-1 == XIOCTL(fd, VIDIOC_TRY_FMT, &tryfmt))
    {
        errno_exit("VIDIOC_TRY_FMT 1", errmsg);
        return;
    }

    xmin = tryfmt.fmt.pix.width;
    ymin = tryfmt.fmt.pix.height;

    tryfmt.fmt.pix.width  = 1600;
    tryfmt.fmt.pix.height = 1200;

    if (-1 == XIOCTL(fd, VIDIOC_TRY_FMT, &tryfmt))
    {
        errno_exit("VIDIOC_TRY_FMT 2", errmsg);
        return;
    }

    xmax = tryfmt.fmt.pix.width;
    ymax = tryfmt.fmt.pix.height;

    cerr << "Min X: " << xmin << " - Max X: " << xmax << " - Min Y: " << ymin << " - Max Y: " << ymax << endl;
}

void V4L2_Base::enumerate_ctrl()
{
    char errmsg[ERRMSGSIZ];
    CLEAR(queryctrl);

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++)
    {
        if (0 == XIOCTL(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << "DISABLED--Control " << queryctrl.name << endl;
                continue;
            }
            cerr << "Control " << queryctrl.name << endl;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
            if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
                enumerate_menu();
            if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
                cerr << "  boolean" << endl;
            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
                cerr << "  integer" << endl;
            //if (queryctrl.type == V4L2_CTRL_TYPE_BITMASK) //not in < 3.1
            //  cerr << "  bitmask" <<endl;
            if (queryctrl.type == V4L2_CTRL_TYPE_BUTTON)
                cerr << "  button" << endl;
        }
        else
        {
            if (errno == EINVAL)
                continue;

            errno_exit("VIDIOC_QUERYCTRL", errmsg);
            return;
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;; queryctrl.id++)
    {
        if (0 == XIOCTL(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << "DISABLED--Private Control " << queryctrl.name << endl;
                continue;
            }

            cerr << "Private Control " << queryctrl.name << endl;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
            if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
                enumerate_menu();
            if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
                cerr << "  boolean" << endl;
            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
                cerr << "  integer" << endl;
            //if (queryctrl.type == V4L2_CTRL_TYPE_BITMASK)
            //  cerr << "  bitmask" <<endl;
            if (queryctrl.type == V4L2_CTRL_TYPE_BUTTON)
                cerr << "  button" << endl;
        }
        else
        {
            if (errno == EINVAL)
                break;

            errno_exit("VIDIOC_QUERYCTRL", errmsg);
            return;
        }
    }
}

void V4L2_Base::enumerate_menu()
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
    if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
        cerr << "  Menu items:" << endl;
    if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
        cerr << "  Integer Menu items:" << endl;
#else
    cerr << "  Menu items:" << endl;
#endif

    CLEAR(querymenu);
    querymenu.id = queryctrl.id;

    for (querymenu.index = queryctrl.minimum; (int)querymenu.index <= queryctrl.maximum; querymenu.index++)
    {
        if (0 == XIOCTL(fd, VIDIOC_QUERYMENU, &querymenu))
        {
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                cerr << "  " << querymenu.name << endl;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
            {
                char menuname[19];
                menuname[18] = '\0';
                snprintf(menuname, 19, "0x%016llX", querymenu.value);
                cerr << "  " << menuname << endl;
            }
#endif
        }
        //else
        //{
        //  errno_exit("VIDIOC_QUERYMENU", errmsg);
        //  return;
        //}
    }
}

int V4L2_Base::query_ctrl(unsigned int ctrl_id, double &ctrl_min, double &ctrl_max, double &ctrl_step,
                          double &ctrl_value, char *errmsg)
{
    struct v4l2_control control;

    CLEAR(queryctrl);

    queryctrl.id = ctrl_id;

    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (errno != EINVAL)
            return errno_exit("VIDIOC_QUERYCTRL", errmsg);

        else
        {
            cerr << "#" << ctrl_id << " is not supported" << endl;
            snprintf(errmsg, ERRMSGSIZ, "# %d is not supported", ctrl_id);
            return -1;
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
        cerr << "#" << ctrl_id << " is disabled" << endl;
        snprintf(errmsg, ERRMSGSIZ, "# %d is disabled", ctrl_id);
        return -1;
    }

    ctrl_min   = queryctrl.minimum;
    ctrl_max   = queryctrl.maximum;
    ctrl_step  = queryctrl.step;
    ctrl_value = queryctrl.default_value;

    /* Get current value */
    CLEAR(control);
    control.id = ctrl_id;

    if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
        ctrl_value = control.value;

    cerr << queryctrl.name << " -- min: " << ctrl_min << " max: " << ctrl_max << " step: " << ctrl_step
         << " value: " << ctrl_value << endl;

    return 0;
}

void V4L2_Base::queryControls(INumberVectorProperty *nvp, unsigned int *nnumber, ISwitchVectorProperty **options,
                              unsigned int *noptions, const char *dev, const char *group)
{
    struct v4l2_control control;

    INumber *numbers           = nullptr;
    unsigned int *num_ctrls    = nullptr;
    int nnum                   = 0;
    ISwitchVectorProperty *opt = nullptr;
    unsigned int nopt          = 0;
    char optname[]             = "OPT000";
    char swonname[]            = "SET_OPT000";
    char swoffname[]           = "UNSET_OPT000";
    char menuname[]            = "MENU000";
    char menuoptname[]         = "MENU000_OPT000";
    *noptions                  = 0;
    *nnumber                   = 0;

    CLEAR(queryctrl);

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++)
    {
        if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << queryctrl.name << " is disabled." << endl;
                continue;
            }

            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
            {
                numbers = (numbers == nullptr) ? (INumber *)malloc(sizeof(INumber)) :
                                                 (INumber *)realloc(numbers, (nnum + 1) * sizeof(INumber));

                num_ctrls = (num_ctrls == nullptr) ?
                                (unsigned int *)malloc(sizeof(unsigned int)) :
                                (unsigned int *)realloc(num_ctrls, (nnum + 1) * sizeof(unsigned int));

                strncpy(numbers[nnum].name, (const char *)entityXML((char *)queryctrl.name), MAXINDINAME);
                strncpy(numbers[nnum].label, (const char *)entityXML((char *)queryctrl.name), MAXINDILABEL);
                strncpy(numbers[nnum].format, "%0.f", MAXINDIFORMAT);
                numbers[nnum].min   = queryctrl.minimum;
                numbers[nnum].max   = queryctrl.maximum;
                numbers[nnum].step  = queryctrl.step;
                numbers[nnum].value = queryctrl.default_value;

                /* Get current value if possible */
                CLEAR(control);
                control.id = queryctrl.id;
                if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
                    numbers[nnum].value = control.value;

                /* Store ID info in INumber. This is the first time ever I make use of aux0!! */
                num_ctrls[nnum] = queryctrl.id;

                cerr << "Adding " << queryctrl.name << " -- min: " << queryctrl.minimum << " max: " << queryctrl.maximum
                     << " step: " << queryctrl.step << " value: " << numbers[nnum].value << endl;

                nnum++;
            }
            if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
            {
                ISwitch *sw = (ISwitch *)malloc(2 * sizeof(ISwitch));
                snprintf(optname + 3, 4, "%03d", nopt);
                snprintf(swonname + 7, 4, "%03d", nopt);
                snprintf(swoffname + 9, 4, "%03d", nopt);

                opt = (opt == nullptr) ?
                          (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                          (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

                CLEAR(control);
                control.id = queryctrl.id;
                XIOCTL(fd, VIDIOC_G_CTRL, &control);

                IUFillSwitch(sw, swonname, "Off", (control.value ? ISS_OFF : ISS_ON));
                IUFillSwitch(sw + 1, swoffname, "On", (control.value ? ISS_ON : ISS_OFF));
                queryctrl.name[31] = '\0';
                IUFillSwitchVector(&opt[nopt], sw, 2, dev, optname, (const char *)entityXML((char *)queryctrl.name),
                                   group, IP_RW, ISR_1OFMANY, 0.0, IPS_IDLE);
                opt[nopt].aux                    = malloc(sizeof(unsigned int));
                *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding switch  %.*s (%s)\n",
                             (int)sizeof(queryctrl.name), queryctrl.name, (control.value ? "On" : "Off"));
                nopt += 1;
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
            if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
            {
                ISwitch *sw           = nullptr;
                unsigned int nmenuopt = 0;
                char sname[32];
                snprintf(menuname + 4, 4, "%03d", nopt);
                snprintf(menuoptname + 4, 4, "%03d", nopt);
                menuoptname[7] = '_';

                opt = (opt == nullptr) ?
                          (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                          (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

                CLEAR(control);
                control.id = queryctrl.id;
                XIOCTL(fd, VIDIOC_G_CTRL, &control);

                CLEAR(querymenu);
                querymenu.id = queryctrl.id;

                for (querymenu.index = queryctrl.minimum; (int)querymenu.index <= queryctrl.maximum; querymenu.index++)
                {
                    if (0 == XIOCTL(fd, VIDIOC_QUERYMENU, &querymenu))
                    {
                        sw = (sw == nullptr) ? (ISwitch *)malloc(sizeof(ISwitch)) :
                                               (ISwitch *)realloc(sw, (nmenuopt + 1) * sizeof(ISwitch));
                        snprintf(menuoptname + 11, 4, "%03d", nmenuopt);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
                        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        {
                            snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                            sname[31] = '\0';
                        }
                        if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
                        {
                            snprintf(sname, 19, "0x%016llX", querymenu.value);
                            sname[31] = '\0';
                        }
#else
                        snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                        sname[31] = '\0';
#endif
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding menu item %.*s %.*s item %d",
                                     (int)sizeof(sname), sname, (int)sizeof(menuoptname), menuoptname, nmenuopt);
                        //IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)sname, (control.value==nmenuopt?ISS_ON:ISS_OFF));
                        IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)entityXML((char *)sname),
                                     (control.value == (int)nmenuopt ? ISS_ON : ISS_OFF));
                        nmenuopt += 1;
                    }
                    else
                    {
                        //errno_exit("VIDIOC_QUERYMENU", errmsg);
                        //exit(3);
                    }
                }

                queryctrl.name[31] = '\0';
                IUFillSwitchVector(&opt[nopt], sw, nmenuopt, dev, menuname,
                                   (const char *)entityXML((char *)queryctrl.name), group, IP_RW, ISR_1OFMANY, 0.0,
                                   IPS_IDLE);
                opt[nopt].aux                    = malloc(sizeof(unsigned int));
                *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding menu  %.*s (item %d set)",
                             (int)sizeof(queryctrl.name), queryctrl.name, control.value);
                nopt += 1;
            }
        }
        else
        {
            if (errno != EINVAL)
            {
                if (numbers)
                    free(numbers);
                if (opt)
                    free(opt);
                perror("VIDIOC_QUERYCTRL");
                return;
            }
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;; queryctrl.id++)
    {
        if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << queryctrl.name << " is disabled." << endl;
                continue;
            }

            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
            {
                numbers = (numbers == nullptr) ? (INumber *)malloc(sizeof(INumber)) :
                                                 (INumber *)realloc(numbers, (nnum + 1) * sizeof(INumber));

                num_ctrls = (num_ctrls == nullptr) ?
                                (unsigned int *)malloc(sizeof(unsigned int)) :
                                (unsigned int *)realloc(num_ctrls, (nnum + 1) * sizeof(unsigned int));

                strncpy(numbers[nnum].name, (const char *)entityXML((char *)queryctrl.name), MAXINDINAME);
                strncpy(numbers[nnum].label, (const char *)entityXML((char *)queryctrl.name), MAXINDILABEL);
                strncpy(numbers[nnum].format, "%0.f", MAXINDIFORMAT);
                numbers[nnum].min   = queryctrl.minimum;
                numbers[nnum].max   = queryctrl.maximum;
                numbers[nnum].step  = queryctrl.step;
                numbers[nnum].value = queryctrl.default_value;

                /* Get current value if possible */
                CLEAR(control);
                control.id = queryctrl.id;
                if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
                    numbers[nnum].value = control.value;

                /* Store ID info in INumber. This is the first time ever I make use of aux0!! */
                num_ctrls[nnum] = queryctrl.id;
                cerr << "Adding ext. " << queryctrl.name << " -- min: " << queryctrl.minimum
                     << " max: " << queryctrl.maximum << " step: " << queryctrl.step
                     << " value: " << numbers[nnum].value << endl;

                nnum++;
            }
            if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
            {
                ISwitch *sw = (ISwitch *)malloc(2 * sizeof(ISwitch));
                snprintf(optname + 3, 4, "%03d", nopt);
                snprintf(swonname + 7, 4, "%03d", nopt);
                snprintf(swoffname + 9, 4, "%03d", nopt);

                opt = (opt == nullptr) ?
                          (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                          (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

                CLEAR(control);
                control.id = queryctrl.id;
                XIOCTL(fd, VIDIOC_G_CTRL, &control);

                IUFillSwitch(sw, swonname, "On", (control.value ? ISS_ON : ISS_OFF));
                IUFillSwitch(sw + 1, swoffname, "Off", (control.value ? ISS_OFF : ISS_ON));
                queryctrl.name[31] = '\0';
                IUFillSwitchVector(&opt[nopt], sw, 2, dev, optname, (const char *)entityXML((char *)queryctrl.name),
                                   group, IP_RW, ISR_1OFMANY, 0.0, IPS_IDLE);

                opt[nopt].aux                    = malloc(sizeof(unsigned int));
                *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding ext. switch  %.*s (%s)\n",
                             (int)sizeof(queryctrl.name), queryctrl.name, (control.value ? "On" : "Off"));
                nopt += 1;
            }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
            if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
            {
                ISwitch *sw           = nullptr;
                unsigned int nmenuopt = 0;
                char sname[32];
                snprintf(menuname + 4, 4, "%03d", nopt);
                snprintf(menuoptname + 4, 4, "%03d", nopt);
                menuoptname[7] = '_';

                opt = (opt == nullptr) ?
                          (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                          (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

                CLEAR(control);
                control.id = queryctrl.id;
                XIOCTL(fd, VIDIOC_G_CTRL, &control);

                CLEAR(querymenu);
                querymenu.id = queryctrl.id;

                for (querymenu.index = queryctrl.minimum; (int)querymenu.index <= queryctrl.maximum; querymenu.index++)
                {
                    if (0 == XIOCTL(fd, VIDIOC_QUERYMENU, &querymenu))
                    {
                        sw = (sw == nullptr) ? (ISwitch *)malloc(sizeof(ISwitch)) :
                                               (ISwitch *)realloc(sw, (nmenuopt + 1) * sizeof(ISwitch));
                        snprintf(menuoptname + 11, 4, "%03d", nmenuopt);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
                        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        {
                            snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                            sname[31] = '\0';
                        }
                        if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
                        {
                            snprintf(sname, 19, "0x%016llX", querymenu.value);
                            sname[31] = '\0';
                        }
#else
                        snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                        sname[31] = '\0';
#endif
                        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding menu item %.*s %.*s item %d",
                                     (int)sizeof(sname), sname, (int)sizeof(menuoptname), menuoptname, nmenuopt);
                        //IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)sname, (control.value==nmenuopt?ISS_ON:ISS_OFF));
                        IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)entityXML((char *)querymenu.name),
                                     (control.value == (int)nmenuopt ? ISS_ON : ISS_OFF));
                        nmenuopt += 1;
                    }
                    else
                    {
                        //errno_exit("VIDIOC_QUERYMENU", errmsg);
                        //exit(3);
                    }
                }

                queryctrl.name[31] = '\0';
                IUFillSwitchVector(&opt[nopt], sw, nmenuopt, dev, menuname,
                                   (const char *)entityXML((char *)queryctrl.name), group, IP_RW, ISR_1OFMANY, 0.0,
                                   IPS_IDLE);

                opt[nopt].aux                    = malloc(sizeof(unsigned int));
                *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding ext. menu  %.*s (item %d set)",
                             (int)sizeof(queryctrl.name), queryctrl.name, control.value);
                nopt += 1;
            }
        }
        else
            break;
    }

    /* Store numbers in aux0 */
    for (int i = 0; i < nnum; i++)
        numbers[i].aux0 = &num_ctrls[i];

    nvp->np  = numbers;
    nvp->nnp = nnum;
    *nnumber = nnum;

    *options  = opt;
    *noptions = nopt;
}

int V4L2_Base::queryINTControls(INumberVectorProperty *nvp)
{
    struct v4l2_control control;
    char errmsg[ERRMSGSIZ];
    CLEAR(queryctrl);
    INumber *numbers        = nullptr;
    unsigned int *num_ctrls = nullptr;
    int nnum                = 0;

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++)
    {
        if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << queryctrl.name << " is disabled." << endl;
                continue;
            }

            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
            {
                numbers = (numbers == nullptr) ? (INumber *)malloc(sizeof(INumber)) :
                                                 (INumber *)realloc(numbers, (nnum + 1) * sizeof(INumber));

                num_ctrls = (num_ctrls == nullptr) ?
                                (unsigned int *)malloc(sizeof(unsigned int)) :
                                (unsigned int *)realloc(num_ctrls, (nnum + 1) * sizeof(unsigned int));

                strncpy(numbers[nnum].name, ((char *)queryctrl.name), MAXINDINAME);
                strncpy(numbers[nnum].label, ((char *)queryctrl.name), MAXINDILABEL);
                strncpy(numbers[nnum].format, "%0.f", MAXINDIFORMAT);
                numbers[nnum].min   = queryctrl.minimum;
                numbers[nnum].max   = queryctrl.maximum;
                numbers[nnum].step  = queryctrl.step;
                numbers[nnum].value = queryctrl.default_value;

                /* Get current value if possible */
                CLEAR(control);
                control.id = queryctrl.id;
                if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
                    numbers[nnum].value = control.value;

                /* Store ID info in INumber. This is the first time ever I make use of aux0!! */
                num_ctrls[nnum] = queryctrl.id;

                DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%.*s -- min: %d max: %d step: %d value: %d",
                             (int)sizeof(queryctrl.name), queryctrl.name, queryctrl.minimum, queryctrl.maximum,
                             queryctrl.step, numbers[nnum].value);

                nnum++;
            }
        }
        else if (errno != EINVAL)
        {
            if (numbers)
                free(numbers);
            return errno_exit("VIDIOC_QUERYCTRL", errmsg);
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE;; queryctrl.id++)
    {
        if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                cerr << queryctrl.name << " is disabled." << endl;
                continue;
            }

            if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
            {
                numbers = (numbers == nullptr) ? (INumber *)malloc(sizeof(INumber)) :
                                                 (INumber *)realloc(numbers, (nnum + 1) * sizeof(INumber));

                num_ctrls = (num_ctrls == nullptr) ?
                                (unsigned int *)malloc(sizeof(unsigned int)) :
                                (unsigned int *)realloc(num_ctrls, (nnum + 1) * sizeof(unsigned int));

                strncpy(numbers[nnum].name, ((char *)queryctrl.name), MAXINDINAME);
                strncpy(numbers[nnum].label, ((char *)queryctrl.name), MAXINDILABEL);
                strncpy(numbers[nnum].format, "%0.f", MAXINDIFORMAT);
                numbers[nnum].min   = queryctrl.minimum;
                numbers[nnum].max   = queryctrl.maximum;
                numbers[nnum].step  = queryctrl.step;
                numbers[nnum].value = queryctrl.default_value;

                /* Get current value if possible */
                CLEAR(control);
                control.id = queryctrl.id;
                if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
                    numbers[nnum].value = control.value;

                /* Store ID info in INumber. This is the first time ever I make use of aux0!! */
                num_ctrls[nnum] = queryctrl.id;

                nnum++;
            }
        }
        else
            break;
    }

    /* Store numbers in aux0 */
    for (int i = 0; i < nnum; i++)
        numbers[i].aux0 = &num_ctrls[i];

    nvp->np  = numbers;
    nvp->nnp = nnum;

    return nnum;
}

int V4L2_Base::getControl(unsigned int ctrl_id, double *value, char *errmsg)
{
    struct v4l2_control control;

    CLEAR(control);
    control.id = ctrl_id;

    if (-1 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
        return errno_exit("VIDIOC_G_CTRL", errmsg);
    *value = (double)control.value;
    return 0;
}

int V4L2_Base::setINTControl(unsigned int ctrl_id, double new_value, char *errmsg)
{
    struct v4l2_control control;

    CLEAR(queryctrl);

    queryctrl.id = ctrl_id;
    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        return 0;
    if ((queryctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) || (queryctrl.flags & V4L2_CTRL_FLAG_GRABBED) ||
        (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE) || (queryctrl.flags & V4L2_CTRL_FLAG_VOLATILE))
    {
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_WARNING, "Setting INT control %.*s will fail, currently %s%s%s%s",
                     (int)sizeof(queryctrl.name), queryctrl.name,
                     queryctrl.flags&V4L2_CTRL_FLAG_READ_ONLY?"read only ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_GRABBED?"grabbed ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_INACTIVE?"inactive ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_VOLATILE?"volatile":"");
        return 0;
    }

    CLEAR(control);

    //cerr << "The id is " << ctrl_id << " new value is " << new_value << endl;

    control.id    = ctrl_id;
    control.value = (int)new_value;
    if (-1 == XIOCTL(fd, VIDIOC_S_CTRL, &control))
    {
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_ERROR, "Setting INT control %.*s failed (%s)",
               (int)sizeof(queryctrl.name), queryctrl.name, errmsg);
        return errno_exit("VIDIOC_S_CTRL", errmsg);
    }
    return 0;
}

int V4L2_Base::setOPTControl(unsigned int ctrl_id, unsigned int new_value, char *errmsg)
{
    struct v4l2_control control;

    CLEAR(queryctrl);

    queryctrl.id = ctrl_id;
    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        return 0;
    if ((queryctrl.flags & V4L2_CTRL_FLAG_READ_ONLY) || (queryctrl.flags & V4L2_CTRL_FLAG_GRABBED) ||
        (queryctrl.flags & V4L2_CTRL_FLAG_INACTIVE) || (queryctrl.flags & V4L2_CTRL_FLAG_VOLATILE))
    {
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Setting OPT control %.*s will fail, currently %s%s%s%s",
                     (int)sizeof(queryctrl.name), queryctrl.name,
                     queryctrl.flags&V4L2_CTRL_FLAG_READ_ONLY?"read only ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_GRABBED?"grabbed ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_INACTIVE?"inactive ":"",
                     queryctrl.flags&V4L2_CTRL_FLAG_VOLATILE?"volatile":"");
        return 0;
    }

    CLEAR(control);

    //cerr << "The id is " << ctrl_id << " new value is " << new_value << endl;

    control.id    = ctrl_id;
    control.value = new_value;
    if (-1 == XIOCTL(fd, VIDIOC_S_CTRL, &control))
    {
        DEBUGFDEVICE(deviceName, INDI::Logger::DBG_ERROR, "Setting INT control %.*s failed (%s)",
               (int)sizeof(queryctrl.name), queryctrl.name, errmsg);
        return errno_exit("VIDIOC_S_CTRL", errmsg);
    }
    return 0;
}

bool V4L2_Base::enumerate_ext_ctrl()
{
    //struct v4l2_queryctrl queryctrl;

    CLEAR(queryctrl);

    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        return false;

    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == XIOCTL(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        {
            cerr << "DISABLED--Control " << queryctrl.name << endl;
            queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            continue;
        }

        if (queryctrl.type == V4L2_CTRL_TYPE_CTRL_CLASS)
        {
            cerr << "Control Class " << queryctrl.name << endl;
            queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            continue;
        }

        cerr << "Control " << queryctrl.name << endl;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
        if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
            enumerate_menu();
        if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
            cerr << "  boolean" << endl;
        if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
            cerr << "  integer" << endl;
        //if (queryctrl.type == V4L2_CTRL_TYPE_BITMASK) //not in < 3.1
        //  cerr << "  bitmask" <<endl;
        if (queryctrl.type == V4L2_CTRL_TYPE_BUTTON)
            cerr << "  button" << endl;
        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }
    return true;
}

// TODO free opt/menu aux placeholders

bool V4L2_Base::queryExtControls(INumberVectorProperty *nvp, unsigned int *nnumber, ISwitchVectorProperty **options,
                                 unsigned int *noptions, const char *dev, const char *group)
{
    struct v4l2_control control;

    INumber *numbers           = nullptr;
    unsigned int *num_ctrls    = nullptr;
    int nnum                   = 0;
    ISwitchVectorProperty *opt = nullptr;
    unsigned int nopt          = 0;
    char optname[]             = "OPT000";
    char swonname[]            = "SET_OPT000";
    char swoffname[]           = "UNSET_OPT000";
    char menuname[]            = "MENU000";
    char menuoptname[]         = "MENU000_OPT000";
    *noptions                  = 0;
    *nnumber                   = 0;

    CLEAR(queryctrl);

    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl))
        return false;

    CLEAR(queryctrl);
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
    while (0 == XIOCTL(fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (queryctrl.type == V4L2_CTRL_TYPE_CTRL_CLASS)
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Control Class %.*s", (int)sizeof(queryctrl.name),
                         queryctrl.name);
            queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            continue;
        }

        if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
        {
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "%.*s is disabled.", (int)sizeof(queryctrl.name),
                         queryctrl.name);
            queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
            continue;
        }

        if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER)
        {
            numbers = (numbers == nullptr) ? (INumber *)malloc(sizeof(INumber)) :
                                             (INumber *)realloc(numbers, (nnum + 1) * sizeof(INumber));

            num_ctrls = (num_ctrls == nullptr) ? (unsigned int *)malloc(sizeof(unsigned int)) :
                                                 (unsigned int *)realloc(num_ctrls, (nnum + 1) * sizeof(unsigned int));

            strncpy(numbers[nnum].name, (const char *)entityXML((char *)queryctrl.name), MAXINDINAME);
            strncpy(numbers[nnum].label, (const char *)entityXML((char *)queryctrl.name), MAXINDILABEL);
            strncpy(numbers[nnum].format, "%0.f", MAXINDIFORMAT);
            numbers[nnum].min   = queryctrl.minimum;
            numbers[nnum].max   = queryctrl.maximum;
            numbers[nnum].step  = queryctrl.step;
            numbers[nnum].value = queryctrl.default_value;

            /* Get current value if possible */
            CLEAR(control);
            control.id = queryctrl.id;
            if (0 == XIOCTL(fd, VIDIOC_G_CTRL, &control))
                numbers[nnum].value = control.value;

            /* Store ID info in INumber. This is the first time ever I make use of aux0!! */
            num_ctrls[nnum] = queryctrl.id;

            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding %.*s -- min: %d max: %d step: %d value: %d",
                         (int)sizeof(queryctrl.name), queryctrl.name, queryctrl.minimum, queryctrl.maximum,
                         queryctrl.step, numbers[nnum].value);

            nnum++;
        }
        if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
        {
            ISwitch *sw = (ISwitch *)malloc(2 * sizeof(ISwitch));
            snprintf(optname + 3, 4, "%03d", nopt);
            snprintf(swonname + 7, 4, "%03d", nopt);
            snprintf(swoffname + 9, 4, "%03d", nopt);

            opt = (opt == nullptr) ? (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                                     (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

            CLEAR(control);
            control.id = queryctrl.id;
            XIOCTL(fd, VIDIOC_G_CTRL, &control);

            IUFillSwitch(sw, swonname, "Off", (control.value ? ISS_OFF : ISS_ON));
            sw->aux = nullptr;
            IUFillSwitch(sw + 1, swoffname, "On", (control.value ? ISS_ON : ISS_OFF));
            (sw + 1)->aux      = nullptr;
            queryctrl.name[31] = '\0';
            IUFillSwitchVector(&opt[nopt], sw, 2, dev, optname, (const char *)entityXML((char *)queryctrl.name), group,
                               IP_RW, ISR_1OFMANY, 0.0, IPS_IDLE);
            opt[nopt].aux                    = malloc(sizeof(unsigned int));
            *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding switch  %.*s (%s)", (int)sizeof(queryctrl.name),
                         queryctrl.name, (control.value ? "On" : "Off"));
            nopt += 1;
        }
        if (queryctrl.type == V4L2_CTRL_TYPE_BUTTON)
        {
            ISwitch *sw = (ISwitch *)malloc(sizeof(ISwitch));
            snprintf(optname + 3, 4, "%03d", nopt);
            snprintf(swonname + 7, 4, "%03d", nopt);
            //snprintf(swoffname+9, 4, "%03d", nopt);

            opt = (opt == nullptr) ? (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                                     (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

            queryctrl.name[31] = '\0';
            IUFillSwitch(sw, swonname, (const char *)entityXML((char *)queryctrl.name), ISS_OFF);
            sw->aux = nullptr;
            IUFillSwitchVector(&opt[nopt], sw, 1, dev, optname, (const char *)entityXML((char *)queryctrl.name), group,
                               IP_RW, ISR_NOFMANY, 0.0, IPS_IDLE);
            opt[nopt].aux                    = malloc(sizeof(unsigned int));
            *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);
            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding Button %.*s", (int)sizeof(queryctrl.name),
                         queryctrl.name);
            nopt += 1;
        }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
        if ((queryctrl.type == V4L2_CTRL_TYPE_MENU) || (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU))
#else
        if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
#endif
        {
            ISwitch *sw           = nullptr;
            unsigned int nmenuopt = 0;
            char sname[32];
            snprintf(menuname + 4, 4, "%03d", nopt);
            snprintf(menuoptname + 4, 4, "%03d", nopt);
            menuoptname[7] = '_';

            opt = (opt == nullptr) ? (ISwitchVectorProperty *)malloc(sizeof(ISwitchVectorProperty)) :
                                     (ISwitchVectorProperty *)realloc(opt, (nopt + 1) * sizeof(ISwitchVectorProperty));

            CLEAR(control);
            control.id = queryctrl.id;
            XIOCTL(fd, VIDIOC_G_CTRL, &control);

            CLEAR(querymenu);
            querymenu.id = queryctrl.id;

            for (querymenu.index = queryctrl.minimum; (int)querymenu.index <= queryctrl.maximum; querymenu.index++)
            {
                if (0 == XIOCTL(fd, VIDIOC_QUERYMENU, &querymenu))
                {
                    sw = (sw == nullptr) ? (ISwitch *)malloc(sizeof(ISwitch)) :
                                           (ISwitch *)realloc(sw, (nmenuopt + 1) * sizeof(ISwitch));
                    snprintf(menuoptname + 11, 4, "%03d", nmenuopt);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0))
                    if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                    {
                        snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                        sname[31] = '\0';
                    }
                    if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
                    {
                        snprintf(sname, 19, "0x%016llX", querymenu.value);
                        sname[31] = '\0';
                    }
#else
                    snprintf(sname, 31, "%.*s", (int)sizeof(querymenu.name), querymenu.name);
                    sname[31] = '\0';
#endif
                    DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding menu item %.*s %.*s item %d index %d",
                                 (int)sizeof(sname), sname, (int)sizeof(menuoptname), menuoptname, nmenuopt,
                                 querymenu.index);
                    //IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)sname, (control.value==nmenuopt?ISS_ON:ISS_OFF));
                    IUFillSwitch(&sw[nmenuopt], menuoptname, (const char *)entityXML((char *)querymenu.name),
                                 (control.value == (int)nmenuopt ? ISS_ON : ISS_OFF));
                    sw[nmenuopt].aux                    = malloc(sizeof(unsigned int));
                    *(unsigned int *)(sw[nmenuopt].aux) = (querymenu.index);
                    nmenuopt += 1;
                }
                else
                {
                    //errno_exit("VIDIOC_QUERYMENU", errmsg);
                    //exit(3);
                }
            }

            queryctrl.name[31] = '\0';
            IUFillSwitchVector(&opt[nopt], sw, nmenuopt, dev, menuname, (const char *)entityXML((char *)queryctrl.name),
                               group, IP_RW, ISR_1OFMANY, 0.0, IPS_IDLE);
            opt[nopt].aux                    = malloc(sizeof(unsigned int));
            *(unsigned int *)(opt[nopt].aux) = (queryctrl.id);

            DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG, "Adding menu  %.*s (item %d set)",
                         (int)sizeof(queryctrl.name), queryctrl.name, control.value);
            nopt += 1;
        }

        //if (queryctrl.type == V4L2_CTRL_TYPE_INTEGER_MENU)
        //	{
        //  DEBUGFDEVICE(deviceName, INDI::Logger::DBG_DEBUG,"Control type not implemented\n");
        //}

        queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
    }

    /* Store numbers in aux0 */
    for (int i = 0; i < nnum; i++)
        numbers[i].aux0 = &num_ctrls[i];

    nvp->np  = numbers;
    nvp->nnp = nnum;
    *nnumber = nnum;

    *options  = opt;
    *noptions = nopt;

    return true;
}

void V4L2_Base::setDeviceName(const char *name)
{
    strncpy(deviceName, name, MAXINDIDEVICE);
}
}
