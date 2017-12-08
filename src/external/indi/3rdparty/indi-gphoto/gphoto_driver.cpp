/*******************************************************************************
  Copyright(c) 2009 Geoffrey Hausheer. All rights reserved.
  Copyright(c) 2012-2016 Jasem Mutlaq. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#include "gphoto_driver.h"

#include "dsusbdriver.h"

#include <indilogger.h>

#include <gphoto2/gphoto2-version.h>

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h> /* ioctl()*/

static GPPortInfoList *portinfolist   = NULL;
static CameraAbilitiesList *abilities = NULL;

struct _gphoto_widget_list
{
    struct _gphoto_widget_list *next;
    gphoto_widget *widget;
};

struct _gphoto_driver
{
    Camera *camera;
    GPContext *context;
    CameraFile *camerafile;
    CameraWidget *config;
    CameraFilePath camerapath;
    int command;
    struct timeval bulb_end;

    char filename[80];
    int width;
    int height;
    gphoto_widget *format_widget;
    gphoto_widget *iso_widget;
    gphoto_widget *exposure_widget;
    gphoto_widget *bulb_widget;
    gphoto_widget *autoexposuremode_widget;
    gphoto_widget *capturetarget_widget;
    gphoto_widget *viewfinder_widget;

    char bulb_port[256];
    int bulb_fd;

    int exposure_cnt;
    double *exposure;
    int bulb_exposure_index;
    double max_exposure, min_exposure;

    int iso;
    int format;
    int upload_settings;

    char *model;
    char *manufacturer;

    char **exposure_presets;
    int exposure_presets_count;

    DSUSBDriver *dsusb;

    gphoto_widget_list *widgets;
    gphoto_widget_list *iter;

    pthread_mutex_t mutex;
    pthread_t thread;
    pthread_cond_t signal;
};

enum
{
    DSLR_CMD_BULB_CAPTURE = 0x01,
    DSLR_CMD_CAPTURE      = 0x02,
    DSLR_CMD_DONE         = 0x04,
    DSLR_CMD_THREAD_EXIT  = 0x08,
};

static char device[64];

int RTS_flag = TIOCM_RTS;

void gphoto_set_debug(const char *name)
{
    strncpy(device, name, 64);
}

static const char *widget_name(CameraWidget *widget)
{
    const char *name = NULL;
    int ret;
    if (!widget)
        return NULL;
    ret = gp_widget_get_name(widget, &name);
    if (ret < GP_OK)
    {
        ret = gp_widget_get_label(widget, &name);
        if (ret < GP_OK)
            return NULL;
    }
    return name;
}
/*
 * This function looks up a label or key entry of
 * a configuration widget.
 * The functions descend recursively, so you can just
 * specify the last component.
 */
static const char *lookup_widget(CameraWidget *config, const char *key, CameraWidget **widget)
{
    int ret;
    /*
     * Grab name from widget instead of from 'key' because it will be invariant
     * as long as the widget is valid
     */
    ret = gp_widget_get_child_by_name(config, key, widget);
    if (ret < GP_OK)
        gp_widget_get_child_by_label(config, key, widget);
    return widget_name(*widget);
}

int gphoto_widget_changed(gphoto_widget *widget)
{
    return gp_widget_changed(widget->widget);
}

int gphoto_read_widget(gphoto_widget *widget)
{
    const char *ptr = NULL;
    int i;
    int ret = GP_OK;

    switch (widget->type)
    {
        case GP_WIDGET_TEXT:
            ret = gp_widget_get_value(widget->widget, &widget->value.text);
            break;
        case GP_WIDGET_RANGE:
            ret = gp_widget_get_value(widget->widget, &widget->value.num);
            gp_widget_get_range(widget->widget, &widget->min, &widget->max, &widget->step);
            break;
        case GP_WIDGET_TOGGLE:
            ret = gp_widget_get_value(widget->widget, &widget->value.toggle);
            break;
        case GP_WIDGET_RADIO:
        case GP_WIDGET_MENU:
            ret = gp_widget_get_value(widget->widget, &ptr);
            if (ret != GP_OK)
                return ret;
            if (!widget->choices)
            {
                widget->choice_cnt = gp_widget_count_choices(widget->widget);
                widget->choices    = (char **)calloc(sizeof(char *), widget->choice_cnt + 1);
            }

            for (i = 0; i < widget->choice_cnt; i++)
            {
                const char *choice = NULL;
                ret                = gp_widget_get_choice(widget->widget, i, &choice);
                if (ret != GP_OK)
                    return ret;
                if (ptr && choice)
                {
                    if (strcmp(choice, ptr) == 0)
                        widget->value.index = i;
                    widget->choices[i] = (char *)choice;
                }
                else
                    return GP_ERROR;
            }
            break;
        case GP_WIDGET_DATE:
            ret = gp_widget_get_value(widget->widget, &widget->value.date);
            break;
        default:
            DEBUGFDEVICE(device, INDI::Logger::DBG_WARNING, "WARNING: Widget type %d is unsupported", widget->type);
    }
    return ret;
}

static gphoto_widget *find_widget(gphoto_driver *gphoto, const char *name)
{
    gphoto_widget *widget = (gphoto_widget *)calloc(sizeof(gphoto_widget), 1);
    int ret;

    widget->name = lookup_widget(gphoto->config, name, &widget->widget);
    if (!widget->name)
    {
        /*fprintf (stderr, "lookup widget failed: %d", ret);*/
        goto out;
    }

    ret = gp_widget_get_type(widget->widget, &widget->type);
    if (ret < GP_OK)
    {
        fprintf(stderr, "widget get type failed: %d", ret);
        goto out;
    }
    ret = gphoto_read_widget(widget);
    if (ret == GP_OK)
        return widget;
out:
    free(widget);
    return NULL;
}

void show_widget(gphoto_widget *widget, const char *prefix)
{
    int i;
    struct tm *tm;
    switch (widget->type)
    {
        case GP_WIDGET_TEXT:
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sValue: %s", prefix, widget->value.text);
            break;
        case GP_WIDGET_RANGE:
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sMin:   %f", prefix, widget->min);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sMax:   %f", prefix, widget->max);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sStep:  %f", prefix, widget->step);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sValue: %f", prefix, widget->value.num);
            break;
        case GP_WIDGET_TOGGLE:
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sValue: %s", prefix, widget->value.toggle ? "On" : "Off");
            break;
        case GP_WIDGET_RADIO:
        case GP_WIDGET_MENU:
            for (i = 0; i < widget->choice_cnt; i++)
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%s%s %3d: %s", prefix,
                             i == widget->value.index ? "*" : " ", i, widget->choices[i]);
            break;
        case GP_WIDGET_DATE:
            tm = gmtime((time_t *)&widget->value.date);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%sValue: %s", prefix, asctime(tm));
            break;
        default:
            break;
    }
}

int gphoto_set_config(Camera *camera, CameraWidget *config, GPContext *context)
{
    int ret;
    int retry;

    for (retry = 0; retry < 5; retry++)
    {
        ret = gp_camera_set_config(camera, config, context);
        switch (ret)
        {
            case GP_OK:
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting new configuration OK.");
                return ret;
                break;
            case GP_ERROR_CAMERA_BUSY:
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG,
                            "Failed to set new configuration value (camera busy), retrying...");
                usleep(500 * 1000);
                break;
            default:
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Failed to set new configuration value (GP result: %d)",
                             ret);
                return ret;
                break;
        }
    }
    return ret;
}

int gphoto_set_widget_num(gphoto_driver *gphoto, gphoto_widget *widget, float value)
{
    int ret;
    int ival        = value;
    const char *ptr = 0;

    if (!widget)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Invalid widget specified to set_widget_num");
        return 1;
    }

    switch (widget->type)
    {
        case GP_WIDGET_TOGGLE:
            ret = gp_widget_set_value(widget->widget, &ival);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting toggle widget %s: %d", widget->name, ival);
            break;
        case GP_WIDGET_RADIO:
        case GP_WIDGET_MENU:
            ret = gp_widget_get_choice(widget->widget, ival, &ptr);
            ret = gp_widget_set_value(widget->widget, ptr);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting radio/menu widget %s: %d (%s)", widget->name, ival,
                         widget->choices[ival]);
            break;
        case GP_WIDGET_RANGE:
            ret = gp_widget_set_value(widget->widget, &value);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting range widget %s: %f", widget->name, value);
            break;
        default:
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Widget type: %d is unsupported", widget->type);
            return 1;
    }

    if (ret == GP_OK)
        ret = gphoto_set_config(gphoto->camera, gphoto->config, gphoto->context);

    return ret;
}

int gphoto_set_widget_text(gphoto_driver *gphoto, gphoto_widget *widget, const char *str)
{
    int ret;

    if (!widget || widget->type != GP_WIDGET_TEXT)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Invalid widget specified to set_widget_text");
        return 1;
    }

    ret = gp_widget_set_value(widget->widget, str);
    if (ret == GP_OK)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting text widget %s: %s", widget->name, str);
        ret = gphoto_set_config(gphoto->camera, gphoto->config, gphoto->context);
    }

    return ret;
}

static void widget_free(gphoto_widget *widget)
{
    if (widget->choices)
        free(widget->choices);
    free(widget);
}

static double *parse_shutterspeed(gphoto_driver *gphoto, char **choices, int count)
{
    double *exposure, val;
    int i, num, denom;
    double max_exposure         = gphoto->max_exposure;
    double min_exposure         = 1e6;
    gphoto->bulb_exposure_index = -1;

    if (count <= 0)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_WARNING, "Shutter speed widget does not have any valid data (count=%d)",
                     count);
        return NULL;
    }

    if (count > 4)
    {
        gphoto->exposure_presets       = choices;
        gphoto->exposure_presets_count = count;
    }

    exposure = (double *)calloc(sizeof(double), count);

    for (i = 0; i < count; i++)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Parsing shutter speed #%d: %s", i, choices[i]);

        if ((strncasecmp(choices[i], "bulb", 4) == 0) || (strcmp(choices[i], "65535/65535") == 0))
        {
            exposure[i] = -1;
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "exposure[%d]= BULB", i);
            gphoto->bulb_exposure_index = i;
        }
        else if (sscanf(choices[i], "%d/%d", &num, &denom) == 2)
        {
            exposure[i] = 1.0 * num / (double)denom;
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "exposure[%d]=%g seconds", i, exposure[i]);
        }
        else if ((val = strtod(choices[i], NULL)))
        {
            exposure[i] = val;
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "exposure[%d]=%g seconds", i, exposure[i]);
        }
        else
        {
            // unknown
            exposure[i] = -2;
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "exposure[%d]= UNKNOWN", i);
        }

        if (exposure[i] > max_exposure)
            max_exposure = exposure[i];
        if (exposure[i] > 0 && exposure[i] < min_exposure)
            min_exposure = exposure[i];
    }

    gphoto->max_exposure = max_exposure;
    if (min_exposure != 1e6)
        gphoto->min_exposure = min_exposure;

    return exposure;
}

static void *stop_bulb(void *arg)
{
    int timeout_set       = 0;
    gphoto_driver *gphoto = (gphoto_driver *)arg;
    //CameraEventType event;
    long timeleft;
    struct timespec timeout;
    struct timeval curtime;

    pthread_mutex_lock(&gphoto->mutex);
    pthread_cond_signal(&gphoto->signal);

    while (1)
    {
        if (!timeout_set)
        {
            // 5 second timeout
            gettimeofday(&curtime, NULL);
            timeout.tv_sec  = curtime.tv_sec + 5;
            timeout.tv_nsec = curtime.tv_usec * 1000;
        }
        timeout_set = 0;
        // All camera opertions take place with the mutex held, so we are thread-safe
        pthread_cond_timedwait(&gphoto->signal, &gphoto->mutex, &timeout);
        //DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG,"timeout expired");
        if (!(gphoto->command & DSLR_CMD_DONE) && (gphoto->command & DSLR_CMD_BULB_CAPTURE))
        {
            //result = gp_camera_wait_for_event(gphoto->camera, 1, &event, &data, gphoto->context);
            /*switch (event) {
                //TODO: Handle unexpected events
                default: break;
            }*/
            gettimeofday(&curtime, NULL);
            timeleft = ((gphoto->bulb_end.tv_sec - curtime.tv_sec) * 1000) +
                       ((gphoto->bulb_end.tv_usec - curtime.tv_usec) / 1000);
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Time left: %ld", timeleft);
            if (timeleft <= 0)
            {
                //shut off bulb mode
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Closing shutter");
                if (gphoto->bulb_widget)
                {
                    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Using widget:%s", gphoto->bulb_widget->name);
                    if (strcmp(gphoto->bulb_widget->name, "eosremoterelease") == 0)
                    {
                        gphoto_set_widget_num(gphoto, gphoto->bulb_widget,
                                              EOS_RELEASE_FULL); //600D eosremoterelease RELEASE FULL
                    }
                    else
                    {
                        gphoto_set_widget_num(gphoto, gphoto->bulb_widget, FALSE);
                    }
                }
                else if (gphoto->dsusb)
                {
                    gphoto->dsusb->closeShutter();
                }
                else
                {
                    ioctl(gphoto->bulb_fd, TIOCMBIC, &RTS_flag);
                    close(gphoto->bulb_fd);
                }
                gphoto->command |= DSLR_CMD_DONE;
                pthread_cond_signal(&gphoto->signal);
            }
            else if (timeleft < 5000)
            {
                timeout_set     = 1;
                timeout.tv_sec  = gphoto->bulb_end.tv_sec;
                timeout.tv_nsec = gphoto->bulb_end.tv_usec * 1000;
            }
        }
        if (!(gphoto->command & DSLR_CMD_DONE) && (gphoto->command & DSLR_CMD_CAPTURE))
        {
            //gp_camera_capture(gphoto->camera, GP_CAPTURE_IMAGE, &gphoto->camerapath, gphoto->context);
            gp_camera_capture(gphoto->camera, GP_CAPTURE_IMAGE, &gphoto->camerapath, gphoto->context);
            gphoto->command |= DSLR_CMD_DONE;
            pthread_cond_signal(&gphoto->signal);
        }
        if (gphoto->command & DSLR_CMD_THREAD_EXIT)
        {
            break;
        }
    }
    pthread_mutex_unlock(&gphoto->mutex);
    return NULL;
}

static void reset_settings(gphoto_driver *gphoto)
{
    if (gphoto->iso >= 0)
        gphoto_set_widget_num(gphoto, gphoto->iso_widget, gphoto->iso_widget->value.index);

    if (gphoto->format >= 0)
        gphoto_set_widget_num(gphoto, gphoto->format_widget, gphoto->format_widget->value.index);

    if (gphoto->exposure_widget)
        gphoto_set_widget_num(gphoto, gphoto->exposure_widget, gphoto->exposure_widget->value.index);
}

int find_bulb_exposure(gphoto_driver *gphoto, gphoto_widget *widget)
{
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Looking for bulb exposure in %s exposure widget..", widget->name);
    for (int i = 0; i < widget->choice_cnt; i++)
    {
        if (gphoto->exposure[i] == -1)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "bulb exposure found! index: %d", i);
            return i;
        }
    }
    return -1;
}

int find_exposure_setting(gphoto_driver *gphoto, gphoto_widget *widget, uint32_t exptime_usec)
{
    int i;
    double exptime = exptime_usec / 1e6;
    int best_idx   = 0;
    double delta;
    double best_match = 99999;

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Finding optimal exposure setting for %g seconds in %s (count=%d)...",
                 exptime, widget->name, widget->choice_cnt);

    for (i = 0; i < widget->choice_cnt; i++)
    {
        //DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG,"Exposure[%d] = %g", i, gphoto->exposure[i]);
        if (gphoto->exposure[i] <= 0)
            continue;
        delta = exptime - gphoto->exposure[i];
        if (delta < 0)
            delta = -delta;
        // printf("%d: Comparing %f <> %f (delta: %f)", i, exptime, gphoto->exposure[i], delta);
        if (delta < best_match)
        {
            best_match = delta;
            best_idx   = i;
        }
    }

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Best match: %g seconds Index: %d", gphoto->exposure[best_idx],
                 best_idx);
    return best_idx;
}

void gphoto_set_upload_settings(gphoto_driver *gphoto, int setting)
{
    gphoto->upload_settings = setting;
}

static int download_image(gphoto_driver *gphoto, CameraFilePath *fn, int fd)
{
    int result;
    CameraFileInfo info;

    strncpy(gphoto->filename, fn->name, sizeof(gphoto->filename));

    if (fd < 0)
    {
        result = gp_file_new(&gphoto->camerafile);
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "gp_file_new result: %d", result);
    }
    else
    {
        result = gp_file_new_from_fd(&gphoto->camerafile, fd);
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "gp_file_new_from_fd result: %d", result);
    }

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Downloading %s/%s", fn->folder, fn->name);

    result = gp_camera_file_get(gphoto->camera, fn->folder, fn->name, GP_FILE_TYPE_NORMAL, gphoto->camerafile,
                                gphoto->context);

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Downloading result: %d", result);

    if (result != GP_OK)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "Error downloading image from camera: %s",
                     gp_result_as_string(result));
        gp_file_free(gphoto->camerafile);
        gphoto->camerafile = NULL;
        return result;
    }

    gp_camera_file_get_info(gphoto->camera, fn->folder, fn->name, &info, gphoto->context);
    gphoto->width  = info.file.width;
    gphoto->height = info.file.height;

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, " Downloaded %dx%d (preview %dx%d)", info.file.width,
                 info.file.height, info.preview.width, info.preview.height);

    // For some reason Canon 20D fails when deleting here
    // so this hack is a workaround until a permement fix is found
    // JM 2017-05-17
    //if (gphoto->upload_settings == GP_UPLOAD_CLIENT && !strstr(gphoto->model, "20D"))
    int captureTarget = -1;
    gphoto_get_capture_target(gphoto, &captureTarget);
    // If it was set to RAM
    if (captureTarget == 0 && !strstr(gphoto->model, "20D"))
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Deleting.");
        result = gp_camera_file_delete(gphoto->camera, fn->folder, fn->name, gphoto->context);
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "  Retval: %d", result);
    }

    if (fd >= 0)
    {
        // This will close the file descriptor
        gp_file_free(gphoto->camerafile);
        gphoto->camerafile = NULL;
    }

    return GP_OK;
}

int gphoto_mirrorlock(gphoto_driver *gphoto, int msec)
{
    // If already set to BULB, set eosremoterelease to 2, then 4, then sleep
    //if (gphoto->autoexposuremode_widget && gphoto->autoexposuremode_widget->value.index == 4)
    if (gphoto->bulb_widget && !strcmp(gphoto->bulb_widget->name, "eosremoterelease"))
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "eosremoterelease Mirror Lock for %g secs", msec / 1000.0);

        gphoto_set_widget_num(gphoto, gphoto->bulb_widget, EOS_PRESS_FULL);
        gphoto_set_widget_num(gphoto, gphoto->bulb_widget, EOS_RELEASE_FULL);

        usleep(msec * 1000);

        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "End of mirror lock timer");

        return 0;
    }

    if (gphoto->bulb_port[0])
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Locking mirror by opening remote serial shutter port: %s ...",
                     gphoto->bulb_port);
        gphoto->bulb_fd = open(gphoto->bulb_port, O_RDWR, O_NONBLOCK);
        if (gphoto->bulb_fd < 0)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Failed to open serial port: %s", gphoto->bulb_port);
            //pthread_mutex_unlock(&gphoto->mutex);
            return -1;
        }

        ioctl(gphoto->bulb_fd, TIOCMBIS, &RTS_flag);

        usleep(20000);
        ioctl(gphoto->bulb_fd, TIOCMBIC, &RTS_flag);
        close(gphoto->bulb_fd);
        gphoto->bulb_fd = -1;
        usleep(msec * 1000 - 20000);
        return 0;
    }

    // Otherwise fail gracefully
    DEBUGDEVICE(device, INDI::Logger::DBG_ERROR, "Mirror lock feature is not yet implemented for this camera model.");
    return -1;
}

int gphoto_start_exposure(gphoto_driver *gphoto, uint32_t exptime_usec, int mirror_lock)
{
    if (gphoto->exposure_widget == NULL)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "No exposure widget found. Can not expose!");
        return -1;
    }

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Starting exposure (exptime: %g secs, mirror lock: %d)",
                 exptime_usec / 1e6, mirror_lock);
    pthread_mutex_lock(&gphoto->mutex);
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "  Mutex locked");

    // Set ISO Settings
    if (gphoto->iso >= 0)
        gphoto_set_widget_num(gphoto, gphoto->iso_widget, gphoto->iso);

    // Set Format Settings
    if (gphoto->format >= 0)
        gphoto_set_widget_num(gphoto, gphoto->format_widget, gphoto->format);

// Set Capture Target
// JM: 2017-05-21: Disabled now since user can explicity set capture target via the driver interface
#if 0
    if (gphoto->capturetarget_widget)
    {
        // Use RAM but NOT on Nikon as it can only work when saving to SD Card
        if (gphoto->upload_settings == GP_UPLOAD_CLIENT && (strstr(gphoto->manufacturer, "Nikon") == NULL) )
            gphoto_set_widget_num(gphoto, gphoto->capturetarget_widget, 0);
        else
        // Store in SD Card in Camera
            gphoto_set_widget_num(gphoto, gphoto->capturetarget_widget, 1);
    }
#endif

    // If exposure more than 5 seconds OR if camera already set in bulb mode, try doing a BULB exposure
    //if (exptime_msec > 5000 || (gphoto->autoexposuremode_widget != NULL && gphoto->autoexposuremode_widget->value.index == 4))

    // If exposure time is more than 1 second AND we have BULB widget OR we have bulb in exposure widget then do bulb
    // JM 2017-05-29: Also added if gphoto->exposure == NULL in case where exposure_widget exists but has 0 members
    // In that case, we always capture using either shutter release or bulb widget regardless of time
    if ((gphoto->exposure == NULL || exptime_usec > 1e6) && ((gphoto->bulb_port[0]) || (gphoto->bulb_widget != NULL)))
    {
        //Bulb mode is supported

        // Check if we are in BULB or MANUAL or other mode. Always set it to BULB if needed
        if (gphoto->bulb_widget && gphoto->autoexposuremode_widget)
        {
            // If it settings is not either MANAUL or BULB then warn the user.
            if (gphoto->autoexposuremode_widget->value.index < 3 || gphoto->autoexposuremode_widget->value.index > 4)
            {
                DEBUGFDEVICE(
                    device, INDI::Logger::DBG_WARNING,
                    "Camera auto exposure mode is not set to either BULB or MANUAL modes (%s). Please set mode to BULB "
                    "for long exposures.",
                    gphoto->autoexposuremode_widget->choices[(uint8_t)gphoto->autoexposuremode_widget->value.index]);
                /*
                ISState states = ISS_ON;
                char *names = {"autoexposuremode4"};
                ISNewSwitch(device, "autoexposuremode", &states, &names, 1);

                // Update widget
                gphoto_read_widget(gphoto->autoexposuremode_widget);
                */
            }
        }

        // We set bulb setting for exposure widget if it is defined by the camera
        if (gphoto->exposure && gphoto->bulb_exposure_index != -1)
        {
            // If it's not already set to the bulb exposure index
            if (gphoto->bulb_exposure_index != (uint8_t)gphoto->exposure_widget->value.index)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Setting exposure widget bulb index: %d",
                             gphoto->bulb_exposure_index);
                gphoto_set_widget_num(gphoto, gphoto->exposure_widget, gphoto->bulb_exposure_index);
            }
        }

        // If we have mirror lock enabled, let's lock mirror. Return on failure
        if (mirror_lock)
        {
            if (gphoto->dsusb)
            {
                DEBUGDEVICE(device, INDI::Logger::DBG_ERROR, "Using mirror lock with DSUSB is unsupported!");
                return -1;
            }

            if (gphoto_mirrorlock(gphoto, mirror_lock * 1000))
                return -1;
        }

        // If bulb port is specified, let's open it
        if (gphoto->dsusb)
        {
            DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Using DSUSB to open shutter...");
            gphoto->dsusb->openShutter();
        }
        else if (gphoto->bulb_port[0])
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Opening remote serial shutter port: %s ...",
                         gphoto->bulb_port);
            gphoto->bulb_fd = open(gphoto->bulb_port, O_RDWR, O_NONBLOCK);
            if (gphoto->bulb_fd < 0)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Failed to open serial port: %s", gphoto->bulb_port);
                pthread_mutex_unlock(&gphoto->mutex);
                return -1;
            }
            ioctl(gphoto->bulb_fd, TIOCMBIS, &RTS_flag);
        }
        // if no bulb port (external shutter release) is specified, let's use the internal bulb widget
        else if (gphoto->bulb_widget)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Using internal bulb widget:%s", gphoto->bulb_widget->name);

            // Check if the internal bulb widget is eosremoterelease, and if yes, set it accordingly
            if (strcmp(gphoto->bulb_widget->name, "eosremoterelease") == 0)
            {
                gphoto_set_widget_num(gphoto, gphoto->bulb_widget, EOS_PRESS_FULL); //600D eosremoterelease PRESS FULL
            }
            // Otherwise use the regular bulb widget
            else
            {
                gphoto_set_widget_num(gphoto, gphoto->bulb_widget, TRUE);
            }
        }

        // Preparing exposure
        gettimeofday(&gphoto->bulb_end, NULL);
        //unsigned int usec = gphoto->bulb_end.tv_usec + exptime_msec % 1000 * 1000;
        uint32_t usec = gphoto->bulb_end.tv_usec + exptime_usec;
        //gphoto->bulb_end.tv_sec = gphoto->bulb_end.tv_sec + exptime_msec / 1000 + usec / 1000000;
        gphoto->bulb_end.tv_sec  = gphoto->bulb_end.tv_sec + exptime_usec / 1e6;
        gphoto->bulb_end.tv_usec = usec % 1000000;

        // Start actual exposure
        gphoto->command = DSLR_CMD_BULB_CAPTURE;
        pthread_cond_signal(&gphoto->signal);
        pthread_mutex_unlock(&gphoto->mutex);
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Exposure started");
        return 0;
    }

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Using camera predefined exposure ranges.");

    // NOT using bulb mode so let's find an exposure time that would closely match the requested exposure time
    int idx = find_exposure_setting(gphoto, gphoto->exposure_widget, exptime_usec);

    gphoto_set_widget_num(gphoto, gphoto->exposure_widget, idx);
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Using predefined exposure time: %g seconds", gphoto->exposure[idx]);

    // Lock the mirror if required.
    if (mirror_lock && gphoto_mirrorlock(gphoto, mirror_lock * 1000))
        return -1;

    // If bulb port is specified, a serial shutter control is required to start the exposure. Treat this as a bulb exposure.
    if (gphoto->bulb_port[0])
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Opening remote serial shutter port: %s ...", gphoto->bulb_port);
        gphoto->bulb_fd = open(gphoto->bulb_port, O_RDWR, O_NONBLOCK);
        if (gphoto->bulb_fd < 0)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Failed to open serial port: %s", gphoto->bulb_port);
            pthread_mutex_unlock(&gphoto->mutex);
            return -1;
        }

        ioctl(gphoto->bulb_fd, TIOCMBIS, &RTS_flag);

        // Preparing exposure: we let stop_bulb() close the serial port although this could be done here as well
        // because the camera closes the shutter.
        gettimeofday(&gphoto->bulb_end, NULL);
        uint32_t usec            = gphoto->bulb_end.tv_usec + exptime_usec;
        gphoto->bulb_end.tv_sec  = gphoto->bulb_end.tv_sec + exptime_usec / 1e6;
        gphoto->bulb_end.tv_usec = usec % 1000000;

        // Start actual exposure
        gphoto->command = DSLR_CMD_BULB_CAPTURE;
        pthread_cond_signal(&gphoto->signal);
        pthread_mutex_unlock(&gphoto->mutex);
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Exposure started");
        return 0;
    }
    gphoto->command = DSLR_CMD_CAPTURE;
    pthread_cond_signal(&gphoto->signal);
    pthread_mutex_unlock(&gphoto->mutex);
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Exposure started");
    return 0;
}

int gphoto_read_exposure_fd(gphoto_driver *gphoto, int fd)
{
    CameraFilePath *fn;
    CameraEventType event;
    void *data = NULL;
    int result;

    // Wait for exposure to complete
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Reading exposure...");
    pthread_mutex_lock(&gphoto->mutex);
    if (gphoto->camerafile)
    {
        gp_file_free(gphoto->camerafile);
        gphoto->camerafile = NULL;
    }
    if (!(gphoto->command & DSLR_CMD_DONE))
        pthread_cond_wait(&gphoto->signal, &gphoto->mutex);

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Exposure complete");

    if (gphoto->command & DSLR_CMD_CAPTURE)
    {
        result          = download_image(gphoto, &gphoto->camerapath, fd);
        gphoto->command = 0;
        //Set exposure back to original value
        reset_settings(gphoto);
        pthread_mutex_unlock(&gphoto->mutex);
        return result;
    }

    //Bulb mode
    int timeoutCounter = 0;
    gphoto->command    = 0;

    while (1)
    {
        // Wait for image to be ready to download
        result = gp_camera_wait_for_event(gphoto->camera, 1000, &event, &data, gphoto->context);
        if (result != GP_OK)
        {
            DEBUGDEVICE(device, INDI::Logger::DBG_WARNING, "WARNING: Could not wait for event.");
            timeoutCounter++;
            if (timeoutCounter >= 10)
            {
                pthread_mutex_unlock(&gphoto->mutex);
                return -1;
            }

            continue;
        }

        switch (event)
        {
            case GP_EVENT_CAPTURE_COMPLETE:
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Capture event completed.");
                break;
            case GP_EVENT_FILE_ADDED:
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "File added event completed.");
                fn     = (CameraFilePath *)data;
                result = download_image(gphoto, fn, fd);
                //Set exposure back to original value
                reset_settings(gphoto);

                pthread_mutex_unlock(&gphoto->mutex);
                return result;
                break;
            case GP_EVENT_UNKNOWN:
                DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Unknown event.");
                break;
            case GP_EVENT_TIMEOUT:
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Event timed out #%d, retrying...", ++timeoutCounter);
                // So retry for 5 seconds before giving up
                if (timeoutCounter >= 10)
                {
                    pthread_mutex_unlock(&gphoto->mutex);
                    return -1;
                }
                break;

            default:
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Got unexpected message: %d", event);
        }
        //pthread_mutex_unlock(&gphoto->mutex);
        //usleep(500 * 1000);
        //pthread_mutex_lock(&gphoto->mutex);
    }
    pthread_mutex_unlock(&gphoto->mutex);
    return 0;
}

int gphoto_read_exposure(gphoto_driver *gphoto)
{
    return gphoto_read_exposure_fd(gphoto, -1);
}

char **gphoto_get_formats(gphoto_driver *gphoto, int *cnt)
{
    if (!gphoto->format_widget)
    {
        if (cnt)
            *cnt = 0;
        return NULL;
    }
    if (cnt)
        *cnt = gphoto->format_widget->choice_cnt;
    return gphoto->format_widget->choices;
}

char **gphoto_get_iso(gphoto_driver *gphoto, int *cnt)
{
    if (!gphoto->iso_widget)
    {
        if (cnt)
            *cnt = 0;
        return NULL;
    }
    if (cnt)
        *cnt = gphoto->iso_widget->choice_cnt;
    return gphoto->iso_widget->choices;
}

void gphoto_set_iso(gphoto_driver *gphoto, int iso)
{
    if (gphoto->iso_widget)
        gphoto->iso = iso;
    else
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "WARNING: Could not set iso");
}

char **gphoto_get_exposure_presets(gphoto_driver *gphoto, int *cnt)
{
    if (!gphoto->exposure_presets)
    {
        if (cnt)
            *cnt = 0;

        return NULL;
    }

    if (cnt)
        *cnt = gphoto->exposure_presets_count;

    return gphoto->exposure_presets;
}

void gphoto_get_minmax_exposure(gphoto_driver *gphoto, double *min, double *max)
{
    *min = gphoto->min_exposure;
    *max = gphoto->max_exposure;
}

void gphoto_set_format(gphoto_driver *gphoto, int format)
{
    if (gphoto->format_widget)
        gphoto->format = format;
    else
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "WARNING: Could not set format");
}

int gphoto_get_format_current(gphoto_driver *gphoto)
{
    if (!gphoto->format_widget)
        return 0;
    return gphoto->format_widget->value.index;
}

int gphoto_get_iso_current(gphoto_driver *gphoto)
{
    if (!gphoto->iso_widget)
        return 0;
    return gphoto->iso_widget->value.index;
}

gphoto_driver *gphoto_open(Camera *camera, GPContext *context, const char *model, const char *port,
                           const char *shutter_release_port)
{
    gphoto_driver *gphoto;
    gphoto_widget *widget;
    CameraAbilities a;
    GPPortInfo pi;
    int result = 0, index = 0;

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "libgphoto2 info:");
    const char **libgphotoVerionInfo = gp_library_version(GP_VERSION_SHORT);
    for (const char **verionInfo = libgphotoVerionInfo; *verionInfo; verionInfo++)
    {
        const char *pInfo = *verionInfo;
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "%s", pInfo);
    }

    //gp_log_add_func(GP_LOG_ERROR, errordumper, NULL);
    gp_camera_new(&camera);

    if (model == NULL || port == NULL)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Camera init. Takes about 10 seconds.");
        result = gp_camera_init(camera, context);
        if (result != GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "Camera open error (%d): %s", result,
                         gp_result_as_string(result));
            return NULL;
        }
    }
    else
    {
        if (!abilities)
        {
            /* Load all the camera drivers we have... */
            result = gp_abilities_list_new(&abilities);
            if (result < GP_OK)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_abilities_list_new failed (%d): %s", result,
                             gp_result_as_string(result));
                return NULL;
            }
            result = gp_abilities_list_load(abilities, context);
            if (result < GP_OK)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_abilities_list_load failed (%d): %s", result,
                             gp_result_as_string(result));
                return NULL;
            }
        }

        /* First lookup the model / driver */
        index = gp_abilities_list_lookup_model(abilities, model);
        if (index < GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_abilities_list_lookup_model failed (%d): %s", index,
                         gp_result_as_string(index));
            return NULL;
        }

        result = gp_abilities_list_get_abilities(abilities, index, &a);
        if (result < GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_abilities_list_get_abilities (%d): %s", result,
                         gp_result_as_string(result));
            return NULL;
        }

        result = gp_camera_set_abilities(camera, a);
        if (result < GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_abilities_list_get_abilities (%d): %s", result,
                         gp_result_as_string(result));
            return NULL;
        }

        if (!portinfolist)
        {
            /* Load all the port drivers we have... */
            result = gp_port_info_list_new(&portinfolist);
            if (result < GP_OK)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_port_info_list_new (%d): %s", result,
                             gp_result_as_string(result));
                return NULL;
            }
            result = gp_port_info_list_load(portinfolist);
            if (result < 0)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_port_info_list_load (%d): %s", result,
                             gp_result_as_string(result));
                return NULL;
            }
            result = gp_port_info_list_count(portinfolist);
            if (result < 0)
            {
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_port_info_list_count (%d): %s", result,
                             gp_result_as_string(result));
                return NULL;
            }
        }

        /* Then associate the camera with the specified port */
        index = gp_port_info_list_lookup_path(portinfolist, port);
        switch (index)
        {
            case GP_ERROR_UNKNOWN_PORT:
                DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR,
                             "The port you specified "
                             "('%s') can not be found. Please "
                             "specify one of the ports found by "
                             "'gphoto2 --list-ports' and make "
                             "sure the spelling is correct "
                             "(i.e. with prefix 'serial:' or 'usb:').",
                             port);
                break;
            default:
                break;
        }

        if (index < GP_OK)
        {
            return NULL;
        }

        result = gp_port_info_list_get_info(portinfolist, index, &pi);
        if (result < GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_port_info_list_get_info (%d): %s", result,
                         gp_result_as_string(result));
            return NULL;
        }

        result = gp_camera_set_port_info(camera, pi);
        if (result < GP_OK)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "gp_port_info_list_get_info (%d): %s", result,
                         gp_result_as_string(result));
            return NULL;
        }
    }

    gphoto                         = (gphoto_driver *)calloc(sizeof(gphoto_driver), 1);
    gphoto->camera                 = camera;
    gphoto->context                = context;
    gphoto->exposure_presets       = nullptr;
    gphoto->exposure_presets_count = 0;
    gphoto->max_exposure           = 3600;
    gphoto->min_exposure           = 0.001;
    gphoto->dsusb                  = nullptr;

    result = gp_camera_get_config(gphoto->camera, &gphoto->config, gphoto->context);
    if (result < GP_OK)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "Camera_get_config failed (%d): %s", result,
                     gp_result_as_string(result));
        free(gphoto);
        return NULL;
    }

    // Set 'capture=1' for Canon DSLRs.  Won't harm other cameras
    if ((widget = find_widget(gphoto, "capture")))
    {
        gphoto_set_widget_num(gphoto, widget, TRUE);
        widget_free(widget);
    }

    // Look for an exposure widget
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Finding exposure widget...");

    if ((gphoto->exposure_widget = find_widget(gphoto, "shutterspeed2")) ||
        (gphoto->exposure_widget = find_widget(gphoto, "shutterspeed")) ||
        (gphoto->exposure_widget = find_widget(gphoto, "eos-shutterspeed")))
    {
        gphoto->exposure =
            parse_shutterspeed(gphoto, gphoto->exposure_widget->choices, gphoto->exposure_widget->choice_cnt);
    }
    else if ((gphoto->exposure_widget = find_widget(gphoto, "capturetarget")))
    {
        const char *choices[2] = { "1/1", "bulb" };
        gphoto->exposure       = parse_shutterspeed(gphoto, (char **)choices, 2);
    }
    else
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_WARNING,
                    "Warning: Didn't find an exposure widget! Are you sure the camera is set to Bulb mode?");
    }

    if (gphoto->exposure_widget != NULL)
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Exposure Widget: %s", gphoto->exposure_widget->name);

    int ret = gphoto_read_widget(gphoto->exposure_widget);
    if (ret == GP_OK)
        show_widget(gphoto->exposure_widget, "\t\t");

    gphoto->format_widget   = find_widget(gphoto, "imageformat");
    gphoto->format          = -1;
    gphoto->manufacturer    = NULL;
    gphoto->model           = NULL;
    gphoto->upload_settings = GP_UPLOAD_CLIENT;

    if (gphoto->format_widget != NULL)
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Image Format Widget: %s", gphoto->format_widget->name);

    gphoto->iso_widget = find_widget(gphoto, "iso");
    if (gphoto->iso_widget == NULL)
    {
        gphoto->iso_widget = find_widget(gphoto, "eos-iso");
    }
    gphoto->iso = -1;

    if (gphoto->iso_widget != NULL)
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "ISO Widget: %s", gphoto->iso_widget->name);

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Finding bulb widget...");

    // Look for eosremoterelease widget first
    if ((gphoto->bulb_widget = find_widget(gphoto, "eosremoterelease")) == NULL)
    {
        // Find Bulb widget if eosremoterelease is not found
        gphoto->bulb_widget = find_widget(gphoto, "bulb");
    }

    if (gphoto->bulb_widget)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Bulb Widget: %s", gphoto->bulb_widget->name);
        int ret = gphoto_read_widget(gphoto->bulb_widget);
        if (ret == GP_OK)
            show_widget(gphoto->bulb_widget, "\t\t");
    }
    else
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "No bulb widget found.");

    // Check for autoexposuremode widget for some cameras
    if ((gphoto->autoexposuremode_widget = find_widget(gphoto, "autoexposuremode")) != NULL)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Autoexposure Widget: %s", gphoto->autoexposuremode_widget->name);
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Current Auto Exposure Mode: %s",
                     gphoto->autoexposuremode_widget->choices[(uint8_t)gphoto->autoexposuremode_widget->value.index]);
    }

    // Check for capture target widget, used to set where image is stored (RAM vs SD Card)
    if ((gphoto->capturetarget_widget = find_widget(gphoto, "capturetarget")) != NULL)
    {
        if (gphoto->capturetarget_widget == gphoto->exposure_widget)
            gphoto->capturetarget_widget = NULL;
        else
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Capture Target Widget: %s",
                         gphoto->capturetarget_widget->name);
    }

    // Check viewfinder widget to force mirror down after live preview if needed
    if ((gphoto->viewfinder_widget = find_widget(gphoto, "viewfinder")) != NULL)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "ViewFinder Widget: %s", gphoto->viewfinder_widget->name);
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Current ViewFinder Value: %s",
                     (gphoto->viewfinder_widget->value.toggle == 0) ? "Off" : "On");
    }

    // Find Manufacturer
    if ((widget = find_widget(gphoto, "manufacturer")) != NULL)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Manufacturer: %s", widget->value.text);
        gphoto->manufacturer = widget->value.text;
    }

    // Find Model
    if ((widget = find_widget(gphoto, "cameramodel")) != NULL || (widget = find_widget(gphoto, "model")) != NULL)
    {
        DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Model: %s", widget->value.text);
        gphoto->model = widget->value.text;
    }
    // Make sure manufacturer is set to something useful
    if (gphoto->manufacturer == NULL)
        gphoto->manufacturer = gphoto->model;

    // Check for user
    if (shutter_release_port)
    {
        strncpy(gphoto->bulb_port, shutter_release_port, sizeof(gphoto->bulb_port));
        /*if (strcmp(gphoto->bulb_port,"/dev/null") != 0 )
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG,"Using external shutter-release cable");
        } else {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG,"Using main usb cable for mirror locking");
        }*/

        if (!strcmp(gphoto->bulb_port, "DSUSB"))
        {
            gphoto->dsusb = new DSUSBDriver(device);

            if (gphoto->dsusb->isConnected() == false)
            {
                DEBUGDEVICE(device, INDI::Logger::DBG_WARNING, "Failed to detect DSUSB!");
                delete (gphoto->dsusb);
                gphoto->dsusb = nullptr;
            }
            else
            {
                DEBUGDEVICE(device, INDI::Logger::DBG_SESSION, "Connected to DSUSB");
            }
        }
        else
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Using external shutter release port: %s", gphoto->bulb_port);
    }

    gphoto->bulb_fd = -1;

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "GPhoto initialized.");

    pthread_mutex_init(&gphoto->mutex, NULL);
    pthread_cond_init(&gphoto->signal, NULL);

    pthread_mutex_lock(&gphoto->mutex);
    pthread_create(&gphoto->thread, NULL, stop_bulb, gphoto);
    pthread_cond_wait(&gphoto->signal, &gphoto->mutex);

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Blub-stop thread enabled.");

    pthread_mutex_unlock(&gphoto->mutex);

    return gphoto;
}

int gphoto_close(gphoto_driver *gphoto)
{
    int result;
    if (!gphoto)
        return 0;
    if (gphoto->thread)
    {
        pthread_mutex_lock(&gphoto->mutex);
        gphoto->command |= DSLR_CMD_THREAD_EXIT;
        pthread_cond_signal(&gphoto->signal);
        pthread_mutex_unlock(&gphoto->mutex);
        pthread_join(gphoto->thread, NULL);
    }

    if (gphoto->exposure)
        free(gphoto->exposure);
    if (gphoto->exposure_widget)
        widget_free(gphoto->exposure_widget);
    if (gphoto->format_widget)
        widget_free(gphoto->format_widget);
    if (gphoto->iso_widget)
        widget_free(gphoto->iso_widget);
    if (gphoto->bulb_widget)
        widget_free(gphoto->bulb_widget);
    if (gphoto->autoexposuremode_widget)
        widget_free(gphoto->autoexposuremode_widget);
    if (gphoto->viewfinder_widget)
        widget_free(gphoto->viewfinder_widget);

    while (gphoto->widgets)
    {
        gphoto_widget_list *next = gphoto->widgets->next;
        widget_free(gphoto->widgets->widget);
        free(gphoto->widgets);
        gphoto->widgets = next;
    }

    result = gp_camera_exit(gphoto->camera, gphoto->context);
    if (result != GP_OK)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "WARNING: Could not close camera connection.");
    }
    // We seem to leak the context here.  libgphoto supplies no way to free it?
    free(gphoto);
    return 0;
}

gphoto_widget *gphoto_get_widget_info(gphoto_driver *gphoto, gphoto_widget_list **iter)
{
    INDI_UNUSED(gphoto);
    gphoto_widget *widget;

    if (!*iter)
        return NULL;
    widget  = (*iter)->widget;
    int ret = gphoto_read_widget(widget);
    // Read next iterator regrardless of return value.
    *iter = (*iter)->next;
    if (ret == GP_OK)
    {
        return widget;
    }
    else
        return NULL;
}

#define GPHOTO_MATCH_WIDGET(widget1, widget2) (widget2 && widget1 == widget2->widget)
static void find_all_widgets(gphoto_driver *gphoto, CameraWidget *widget, char *prefix)
{
    int ret, n, i;
    char *newprefix = NULL;
    const char *uselabel;
    CameraWidgetType type;

    uselabel = widget_name(widget);
    gp_widget_get_type(widget, &type);

    n = gp_widget_count_children(widget);

    if (prefix)
    {
        newprefix = (char *)malloc(strlen(prefix) + 1 + strlen(uselabel) + 1);
        if (!newprefix)
            return;
        sprintf(newprefix, "%s/%s", prefix, uselabel);
    }

    if ((type != GP_WIDGET_WINDOW) && (type != GP_WIDGET_SECTION))
    {
        gphoto_widget_list *list;
        CameraWidget *parent = NULL;

        if (newprefix)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "\t%s", newprefix);
            free(newprefix);
        }
        if (GPHOTO_MATCH_WIDGET(widget, gphoto->iso_widget) || GPHOTO_MATCH_WIDGET(widget, gphoto->format_widget) ||
            GPHOTO_MATCH_WIDGET(widget, gphoto->exposure_widget) || GPHOTO_MATCH_WIDGET(widget, gphoto->bulb_widget))
        {
            return;
        }
        list                 = (gphoto_widget_list *)calloc(1, sizeof(gphoto_widget_list));
        list->widget         = (gphoto_widget *)calloc(sizeof(gphoto_widget), 1);
        list->widget->widget = widget;
        list->widget->type   = type;
        list->widget->name   = uselabel;
        gp_widget_get_parent(widget, &parent);
        if (parent)
            list->widget->parent = widget_name(parent);
        if (!gphoto->widgets)
        {
            gphoto->widgets = list;
        }
        else
        {
            gphoto->iter->next = list;
        }
        gp_widget_get_readonly(widget, &list->widget->readonly);
        gphoto->iter = list;
        return;
    }

    for (i = 0; i < n; i++)
    {
        CameraWidget *child;

        ret = gp_widget_get_child(widget, i, &child);
        if (ret != GP_OK)
            continue;
        find_all_widgets(gphoto, child, newprefix);
    }
    if (newprefix)
        free(newprefix);
}

gphoto_widget_list *gphoto_find_all_widgets(gphoto_driver *gphoto)
{
    find_all_widgets(gphoto, gphoto->config, NULL);
    return gphoto->widgets;
}

void gphoto_show_options(gphoto_driver *gphoto)
{
    find_all_widgets(gphoto, gphoto->config, NULL);
    if (gphoto->widgets)
    {
        gphoto_widget_list *list = gphoto->widgets;
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Available options");
        while (list)
        {
            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "\t%s:", list->widget->name);
            int ret = gphoto_read_widget(list->widget);
            if (ret == GP_OK)
                show_widget(list->widget, "\t\t");
            list = list->next;
        }
    }
    if (gphoto->format_widget)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Available image formats:");
        show_widget(gphoto->format_widget, "\t");
    }
    if (gphoto->iso_widget)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Available ISO:");
        show_widget(gphoto->iso_widget, "\t");
    }
}

void gphoto_get_buffer(gphoto_driver *gphoto, const char **buffer, size_t *size)
{
    gp_file_get_data_and_size(gphoto->camerafile, buffer, (unsigned long *)size);
}

void gphoto_free_buffer(gphoto_driver *gphoto)
{
    if (gphoto->camerafile)
    {
        gp_file_free(gphoto->camerafile);
        gphoto->camerafile = NULL;
    }
}

const char *gphoto_get_file_extension(gphoto_driver *gphoto)
{
    if (gphoto->filename[0])
        return strchr(gphoto->filename, '.') + 1;
    else
        return "unknown";
}

int gphoto_get_dimensions(gphoto_driver *gphoto, int *width, int *height)
{
    *width  = gphoto->width;
    *height = gphoto->height;
    return 0;
}

/*
 * This function looks up a label or key entry of
 * a configuration widget.
 * The functions descend recursively, so you can just
 * specify the last component.
 */

static int _lookup_widget(CameraWidget *widget, const char *key, CameraWidget **child)
{
    int ret;
    ret = gp_widget_get_child_by_name(widget, key, child);
    if (ret < GP_OK)
        ret = gp_widget_get_child_by_label(widget, key, child);
    return ret;
}

/* calls the Nikon DSLR or Canon DSLR autofocus method. */
int gphoto_auto_focus(gphoto_driver *gphoto, char *errMsg)
{
    CameraWidget *widget = NULL, *child = NULL;
    CameraWidgetType type;
    int ret, val;

    ret = gp_camera_get_config(gphoto->camera, &widget, gphoto->context);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "camera_get_config failed: %d", ret);
        return ret;
    }
    ret = _lookup_widget(widget, "autofocusdrive", &child);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "lookup 'autofocusdrive' failed: %d", ret);
        goto out;
    }

    /* check that this is a toggle */
    ret = gp_widget_get_type(child, &type);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "widget get type failed: %d", ret);
        goto out;
    }
    switch (type)
    {
        case GP_WIDGET_TOGGLE:
            break;
        default:
            snprintf(errMsg, MAXRBUF, "widget has bad type %d", type);
            ret = GP_ERROR_BAD_PARAMETERS;
            goto out;
    }

    ret = gp_widget_get_value(child, &val);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "could not get widget value: %d", ret);
        goto out;
    }
    val++;
    ret = gp_widget_set_value(child, &val);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "could not set widget value to 1: %d", ret);
        goto out;
    }

    ret = gp_camera_set_config(gphoto->camera, widget, gphoto->context);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "could not set config tree to autofocus: %d", ret);
        goto out;
    }
out:
    gp_widget_free(widget);
    return ret;
}

int gphoto_capture_preview(gphoto_driver *gphoto, CameraFile *previewFile, char *errMsg)
{
    int rc = GP_OK;

    rc = gp_camera_capture_preview(gphoto->camera, previewFile, gphoto->context);
    if (rc != GP_OK)
        snprintf(errMsg, MAXRBUF, "Error capturing preview: %s", gp_result_as_string(rc));

    return rc;
}

int gphoto_stop_preview(gphoto_driver *gphoto)
{
    int rc = GP_OK;

    // If viewfinder not found, nothing to do
    if (gphoto->viewfinder_widget == NULL)
        return rc;

    rc = gphoto_set_widget_num(gphoto, gphoto->viewfinder_widget, 0);

    return rc;
}

/* Manual focusing a camera...
 * xx is -3 / -2 / -1 / 0 / 1 / 2 / 3
 * Choice: 0 (-3) Near 1
 * Choice: 1 (-2) Near 2
 * Choice: 2 (-1) Near 3
 * Choice: 3 ( 0) None
 * Choice: 4 (+1) Far 1
 * Choice: 5 (+2) Far 2
 * Choice: 6 (+3) Far 3
 */
int gphoto_manual_focus(gphoto_driver *gphoto, int xx, char *errMsg)
{
    CameraWidget *widget = NULL, *child = NULL;
    CameraWidgetType type;
    int ret;
    float rval;
    char *mval;

    // Hack. -3 should be -1 and -1 should be -3
    switch (xx)
    {
        case -3:
            xx = -1;
            break;
        case -1:
            xx = -3;
            break;
    }

    ret = gp_camera_get_config(gphoto->camera, &widget, gphoto->context);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "camera_get_config failed: %d", ret);
        return ret;
    }
    ret = _lookup_widget(widget, "manualfocusdrive", &child);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "lookup manualfocusdrive failed: %d", ret);
        goto out;
    }

    /* check that this is a toggle */
    ret = gp_widget_get_type(child, &type);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "widget get type failed: %d", ret);
        goto out;
    }
    switch (type)
    {
        case GP_WIDGET_RADIO:
        {
            int choices = gp_widget_count_choices(child);

            ret = gp_widget_get_value(child, &mval);
            if (ret < GP_OK)
            {
                snprintf(errMsg, MAXRBUF, "could not get widget value: %d", ret);
                goto out;
            }
            if (choices == 7)
            { /* see what Canon has in EOS_MFDrive */
                ret = gp_widget_get_choice(child, xx + 3, (const char **)&mval);
                if (ret < GP_OK)
                {
                    snprintf(errMsg, MAXRBUF, "could not get widget choice %d: %d", xx + 3, ret);
                    goto out;
                }
                DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "manual focus %d -> %s", xx, mval);
            }
            ret = gp_widget_set_value(child, mval);
            if (ret < GP_OK)
            {
                snprintf(errMsg, MAXRBUF, "could not set widget value to 1: %d", ret);
                goto out;
            }
            break;
        }
        case GP_WIDGET_RANGE:
            ret = gp_widget_get_value(child, &rval);
            if (ret < GP_OK)
            {
                snprintf(errMsg, MAXRBUF, "could not get widget value: %d", ret);
                goto out;
            }

            switch (xx)
            { /* Range is on Nikon from -32768 <-> 32768 */
                case -3:
                    rval = -1024;
                    break;
                case -2:
                    rval = -512;
                    break;
                case -1:
                    rval = -128;
                    break;
                case 0:
                    rval = 0;
                    break;
                case 1:
                    rval = 128;
                    break;
                case 2:
                    rval = 512;
                    break;
                case 3:
                    rval = 1024;
                    break;

                default:
                    rval = xx;
                    break; /* hack */
            }

            DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "manual focus %d -> %f", xx, rval);

            ret = gp_widget_set_value(child, &rval);
            if (ret < GP_OK)
            {
                snprintf(errMsg, MAXRBUF, "could not set widget value to 1: %d", ret);
                goto out;
            }
            break;
        default:
            snprintf(errMsg, MAXRBUF, "widget has bad type %d", type);
            ret = GP_ERROR_BAD_PARAMETERS;
            goto out;
    }

    ret = gp_camera_set_config(gphoto->camera, widget, gphoto->context);
    if (ret < GP_OK)
    {
        snprintf(errMsg, MAXRBUF, "could not set config tree to autofocus: %d", ret);
        goto out;
    }
out:
    gp_widget_free(widget);
    return ret;
}

const char *gphoto_get_manufacturer(gphoto_driver *gphoto)
{
    return gphoto->manufacturer;
}

const char *gphoto_get_model(gphoto_driver *gphoto)
{
    return gphoto->model;
}

int gphoto_get_capture_target(gphoto_driver *gphoto, int *capture_target)
{
    if (gphoto->capturetarget_widget == NULL)
        return GP_ERROR_NOT_SUPPORTED;

    gphoto_read_widget(gphoto->capturetarget_widget);

    *capture_target = gphoto->capturetarget_widget->value.toggle;

    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "Capture target is %s.",
                 (*capture_target == 0) ? "INTERNAL RAM" : "SD Card");

    return GP_OK;
}

int gphoto_set_capture_target(gphoto_driver *gphoto, int capture_target)
{
    if (gphoto->capturetarget_widget == NULL)
        return GP_ERROR_NOT_SUPPORTED;

    gphoto_set_widget_num(gphoto, gphoto->capturetarget_widget, capture_target);

    return GP_OK;
}

#ifdef GPHOTO_TEST
#include <getopt.h>

void write_image(gphoto_driver *gphoto, const char *basename)
{
    FILE *fh;
    const char *buffer;
    size_t size;
    const char *ext;
    char filename[256];
    gphoto_get_buffer(gphoto, &buffer, &size);
    ext = gphoto_get_file_extension(gphoto);
    sprintf(filename, "%s.%s", basename, ext);
    fh = fopen(filename, "w+");
    fwrite(buffer, size, 1, fh);
    fclose(fh);
}

void show_help()
{
    printf("gphoto_driver [options]");
    printf("\t\t-e/--exposure <exposure>          specify exposure in msec (default: 100)");
    printf("\t\t-f/--file <filename>              specify filename to write to");
    printf("\t\t-c/--count <count>                specify how many sequential images to take");
    printf("\t\t-i/--iso <iso>                    choose iso (use --list to query values)");
    printf("\t\t-m/--format <format #>            choose format (use --list to query values)");
    printf("\t\t-p/--port <path to serial port>   choose a serial port to use for shutter release control");
    printf("\t\t-l/--list                         show available iso and format values");
    printf("\t\t-d/--debug                        enable debugging");
    printf("\t\t-h//-help                         show this message");
    exit(0);
}

int main(int argc, char **argv)
{
    gphoto_driver *gphoto;
    char image_name[80];
    int i;
    int count             = 0;
    int list              = 0;
    char *iso             = NULL;
    char *port            = NULL;
    int format            = -1;
    unsigned int exposure = 100;
    unsigned int mlock    = 0;
    char basename[256]    = "image";

    struct option long_options[] = { { "count", required_argument, NULL, 'c' },
                                     { "debug", required_argument, NULL, 'd' },
                                     { "exposure", required_argument, NULL, 'e' },
                                     { "file", required_argument, NULL, 'f' },
                                     { "help", no_argument, NULL, 'h' },
                                     { "iso", required_argument, NULL, 'i' },
                                     { "mlock", required_argument, NULL, 'k' },
                                     { "list", no_argument, NULL, 'l' },
                                     { "format", required_argument, NULL, 'm' },
                                     { "port", required_argument, NULL, 'p' },
                                     { 0, 0, 0, 0 } };

    while (1)
    {
        char c;
        c = getopt_long(argc, argv, "c:de:f:hi:lm:p:", long_options, NULL);
        if (c == EOF)
            break;
        switch (c)
        {
            case 'c':
                count = strtol(optarg, NULL, 0);
                break;
            case 'd':
                debug = 1;
                break;
            case 'e':
                exposure = strtol(optarg, NULL, 0);
                break;
            case 'f':
                strncpy(basename, optarg, 255);
                break;
            case 'h':
                show_help();
                break;
            case 'i':
                iso = optarg;
                break;
            case 'k':
                mlock = strtol(optarg, NULL, 0);
                break;
            case 'l':
                list = 1;
                break;
            case 'm':
                format = strtol(optarg, NULL, 0);
                break;
            case 'p':
                port = optarg;
                break;
        }
    }

    if (!(gphoto = gphoto_open(port)))
    {
        printf("Could not open the DSLR device");
        return -1;
    }
    if (list)
    {
        gphoto_show_options(gphoto);
        gphoto_close(gphoto);
        return 0;
    }

    if (iso)
    {
        int max;
        const char **values = gphoto_get_iso(gphoto, &max);
        for (i = 0; i < max; i++)
        {
            if (strcmp(values[i], iso) == 0)
            {
                gphoto_set_iso(gphoto, i);
                break;
            }
        }
    }
    if (format != -1)
    {
        gphoto_set_format(gphoto, format);
    }
    if (!gphoto->exposure_widget)
    {
        printf("No exposure widget.  Aborting...");
        gphoto_close(gphoto);
        return 1;
    }
    printf("Exposing for %f sec", exposure / 1000.0);
    if (count == 0)
    {
        if (gphoto_start_exposure(gphoto, exposure, mlock))
        {
            printf("Exposure failed!");
            gphoto_close(gphoto);
            return 1;
        }
        usleep(exposure * 1000);
        gphoto_read_exposure(gphoto);
        write_image(gphoto, basename);
    }
    for (i = 0; i < count; i++)
    {
        sprintf(image_name, "%s%d", basename, i);
        if (gphoto_start_exposure(gphoto, exposure, mlock))
        {
            printf("Exposure failed!");
            gphoto_close(gphoto);
            return 1;
        }
        usleep(exposure * 1000);
        gphoto_read_exposure(gphoto);
        write_image(gphoto, image_name);
    }
    gphoto_close(gphoto);
    return 0;
}
#endif
