/*
    IEQ Pro driver

    Copyright (C) 2015 Jasem Mutlaq

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

#pragma once

#include <string>

typedef enum { GPS_OFF, GPS_ON, GPS_DATA_OK } IEQ_GPS_STATUS;
typedef enum {
    ST_STOPPED,
    ST_TRACKING_PEC_OFF,
    ST_SLEWING,
    ST_GUIDING,
    ST_MERIDIAN_FLIPPING,
    ST_TRACKING_PEC_ON,
    ST_PARKED,
    ST_HOME
} IEQ_SYSTEM_STATUS;
typedef enum { TR_SIDEREAL, TR_LUNAR, TR_SOLAR, TR_KING, TR_CUSTOM } IEQ_TRACK_RATE;
typedef enum { SR_1, SR_2, SR_3, SR_4, SR_5, SR_6, SR_7, SR_8, SR_MAX } IEQ_SLEW_RATE;
typedef enum { TS_RS232, TS_CONTROLLER, TS_GPS } IEQ_TIME_SOURCE;
typedef enum { HEMI_SOUTH, HEMI_NORTH } IEQ_HEMISPHERE;
typedef enum { FW_MODEL, FW_BOARD, FW_CONTROLLER, FW_RA, FW_DEC } IEQ_FIRMWARE;
typedef enum { RA_AXIS, DEC_AXIS } IEQ_AXIS;
typedef enum { IEQ_N, IEQ_S, IEQ_W, IEQ_E } IEQ_DIRECTION;
typedef enum { IEQ_FIND_HOME, IEQ_SET_HOME, IEQ_GOTO_HOME } IEQ_HOME_OPERATION;

typedef struct
{
    IEQ_GPS_STATUS gpsStatus;
    IEQ_SYSTEM_STATUS systemStatus;
    IEQ_SYSTEM_STATUS rememberSystemStatus;
    IEQ_TRACK_RATE trackRate;
    IEQ_SLEW_RATE slewRate;
    IEQ_TIME_SOURCE timeSource;
    IEQ_HEMISPHERE hemisphere;
} IEQInfo;

typedef struct
{
    std::string Model;
    std::string MainBoardFirmware;
    std::string ControllerFirmware;
    std::string RAFirmware;
    std::string DEFirmware;
} FirmwareInfo;

/**************************************************************************
 Misc.
**************************************************************************/

void set_ieqpro_debug(bool enable);
void set_ieqpro_simulation(bool enable);
void set_ieqpro_device(const char *name);

/**************************************************************************
 Simulation
**************************************************************************/
void set_sim_gps_status(IEQ_GPS_STATUS value);
void set_sim_system_status(IEQ_SYSTEM_STATUS value);
void set_sim_track_rate(IEQ_TRACK_RATE value);
void set_sim_slew_rate(IEQ_SLEW_RATE value);
void set_sim_time_source(IEQ_TIME_SOURCE value);
void set_sim_hemisphere(IEQ_HEMISPHERE value);
void set_sim_ra(double ra);
void set_sim_dec(double dec);
void set_sim_guide_rate(double rate);

/**************************************************************************
 Diagnostics
**************************************************************************/
bool check_ieqpro_connection(int fd);

/**************************************************************************
 Get Info
**************************************************************************/
/** Get iEQ current status info */
bool get_ieqpro_status(int fd, IEQInfo *info);
/** Get All firmware informatin in addition to mount model */
bool get_ieqpro_firmware(int fd, FirmwareInfo *info);
/** Get mainboard and controller firmware only */
bool get_ieqpro_main_firmware(int fd, FirmwareInfo *info);
/** Get RA and DEC firmware info */
bool get_ieqpro_radec_firmware(int fd, FirmwareInfo *info);
/** Get Mount model */
bool get_ieqpro_model(int fd, FirmwareInfo *info);
/** Get RA/DEC */
bool get_ieqpro_coords(int fd, double *ra, double *dec);
/** Get UTC/Date/Time */
bool get_ieqpro_utc_date_time(int fd, double *utc_hours, int *yy, int *mm, int *dd, int *hh, int *minute, int *ss);

/**************************************************************************
 Motion
**************************************************************************/
bool start_ieqpro_motion(int fd, IEQ_DIRECTION dir);
bool stop_ieqpro_motion(int fd, IEQ_DIRECTION dir);
bool set_ieqpro_slew_rate(int fd, IEQ_SLEW_RATE rate);
bool set_ieqpro_custom_ra_track_rate(int fd, double rate);
bool set_ieqpro_custom_de_track_rate(int fd, double rate);
bool set_ieqpro_track_mode(int fd, IEQ_TRACK_RATE rate);
bool set_ieqpro_track_enabled(int fd, bool enabled);
bool abort_ieqpro(int fd);
bool slew_ieqpro(int fd);
bool sync_ieqpro(int fd);
bool set_ieqpro_ra(int fd, double ra);
bool set_ieqpro_dec(int fd, double dec);

/**************************************************************************
 Home
**************************************************************************/
bool find_ieqpro_home(int fd);
bool goto_ieqpro_home(int fd);
bool set_ieqpro_current_home(int fd);

/**************************************************************************
 Park
**************************************************************************/
bool park_ieqpro(int fd);
bool unpark_ieqpro(int fd);

/**************************************************************************
 Guide
**************************************************************************/
bool set_ieqpro_guide_rate(int fd, double rate);
bool get_ieqpro_guide_rate(int fd, double *rate);
bool start_ieqpro_guide(int fd, IEQ_DIRECTION dir, int ms);

/**************************************************************************
 Time & Location
**************************************************************************/
bool set_ieqpro_longitude(int fd, double longitude);
bool set_ieqpro_latitude(int fd, double latitude);
bool get_ieqpro_longitude(int fd, double *longitude);
bool get_ieqpro_latitude(int fd, double *latitude);
bool set_ieqpro_local_date(int fd, int yy, int mm, int dd);
bool set_ieqpro_local_time(int fd, int hh, int mm, int ss);
bool set_ieqpro_utc_offset(int fd, double offset_hours);
bool set_ieqpro_daylight_saving(int fd, bool enabled);
