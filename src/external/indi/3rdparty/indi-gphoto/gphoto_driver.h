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

#pragma once

#include <gphoto2/gphoto2.h>

#define GP_UPLOAD_CLIENT 0
#define GP_UPLOAD_SDCARD 1
#define GP_UPLOAD_ALL    2

typedef struct
{
    CameraWidget *widget;
    CameraWidgetType type;
    const char *name;
    const char *parent;
    int readonly;
    union {
        int toggle;
        char index;
        char *text;
        float num;
        int date;
    } value;
    int choice_cnt;
    char **choices;
    float min;
    float max;
    float step;
} gphoto_widget;

// EOS Release buttons settings
enum
{
    EOS_NONE,
    EOS_PRESS_HALF,
    EOS_PRESS_FULL,
    EOS_RELEASE_HALF,
    EOS_RELEASE_FULL,
    EOS_IMMEDIATE,
    EOS_PRESS_1,
    EOS_PRESS_2,
    EOS_PRESS_3,
    EOS_RELEASE_1,
    EOS_RELEASE_2,
    EOS_RELEASE_3
};

struct _gphoto_driver;
typedef struct _gphoto_driver gphoto_driver;

struct _gphoto_widget_list;
typedef struct _gphoto_widget_list gphoto_widget_list;

int gphoto_start_exposure(gphoto_driver *gphoto, uint32_t exptime_usec, int mirror_lock);
int gphoto_read_exposure(gphoto_driver *gphoto);
int gphoto_read_exposure_fd(gphoto_driver *gphoto, int fd);
void gphoto_set_upload_settings(gphoto_driver *gphoto, int setting);
void gphoto_get_minmax_exposure(gphoto_driver *gphoto, double *min, double *max);
char **gphoto_get_formats(gphoto_driver *gphoto, int *cnt);
char **gphoto_get_iso(gphoto_driver *gphoto, int *cnt);
char **gphoto_get_exposure_presets(gphoto_driver *gphoto, int *cnt);
void gphoto_set_iso(gphoto_driver *gphoto, int iso);
void gphoto_set_format(gphoto_driver *gphoto, int format);
int gphoto_get_format_current(gphoto_driver *gphoto);
int gphoto_get_iso_current(gphoto_driver *gphoto);
gphoto_driver *gphoto_open(Camera *camera, GPContext *context, const char *model, const char *port,
                           const char *shutter_release_port);
int gphoto_close(gphoto_driver *gphoto);
void gphoto_get_buffer(gphoto_driver *gphoto, const char **buffer, size_t *size);
void gphoto_free_buffer(gphoto_driver *gphoto);
const char *gphoto_get_file_extension(gphoto_driver *gphoto);
void gphoto_show_options(gphoto_driver *gphoto);
gphoto_widget_list *gphoto_find_all_widgets(gphoto_driver *gphoto);
gphoto_widget *gphoto_get_widget_info(gphoto_driver *gphoto, gphoto_widget_list **iter);
int gphoto_set_widget_num(gphoto_driver *gphoto, gphoto_widget *widget, float value);
int gphoto_set_widget_text(gphoto_driver *gphoto, gphoto_widget *widget, const char *str);
int gphoto_read_widget(gphoto_widget *widget);
int gphoto_widget_changed(gphoto_widget *widget);
int gphoto_get_dimensions(gphoto_driver *gphoto, int *width, int *height);
int gphoto_auto_focus(gphoto_driver *gphoto, char *errMsg);
int gphoto_manual_focus(gphoto_driver *gphoto, int xx, char *errMsg);
int gphoto_capture_preview(gphoto_driver *gphoto, CameraFile *previewFile, char *errMsg);
int gphoto_stop_preview(gphoto_driver *gphoto);
int gphoto_get_capture_target(gphoto_driver *gphoto, int *capture_target);
int gphoto_set_capture_target(gphoto_driver *gphoto, int capture_target);
void gphoto_set_debug(const char *name);
int gphoto_mirrorlock(gphoto_driver *gphoto, int msec);
const char *gphoto_get_manufacturer(gphoto_driver *gphoto);
const char *gphoto_get_model(gphoto_driver *gphoto);
