/*
    Celestron driver

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

/*
    Version with experimental pulse guide support. GC 04.12.2015
*/

#pragma once

#include <string>

/* Starsense specific constants */
#define ISNEXSTAR       0x11
#define ISSTARSENSE     0x13
#define MINSTSENSVER    float(1.18)

typedef enum { GPS_OFF, GPS_ON } CELESTRON_GPS_STATUS;
typedef enum { SR_1, SR_2, SR_3, SR_4, SR_5, SR_6, SR_7, SR_8, SR_9 } CELESTRON_SLEW_RATE;
typedef enum { TRACKING_OFF, TRACK_ALTAZ, TRACK_EQN, TRACK_EQS } CELESTRON_TRACK_MODE;
typedef enum { RA_AXIS, DEC_AXIS } CELESTRON_AXIS;
typedef enum { CELESTRON_N, CELESTRON_S, CELESTRON_W, CELESTRON_E } CELESTRON_DIRECTION;
typedef enum { FW_MODEL, FW_VERSION, FW_GPS, FW_RA, FW_DEC } CELESTRON_FIRMWARE;

typedef struct
{
    std::string Model;
    std::string Version;
    std::string GPSFirmware;
    std::string RAFirmware;
    std::string DEFirmware;

    float controllerVersion;
    char controllerVariant;

} FirmwareInfo;

/**************************************************************************
 Misc.
**************************************************************************/
void set_celestron_debug(bool enable);
void set_celestron_simulation(bool enable);
void set_celestron_device(const char *name);

/**************************************************************************
 Simulation
**************************************************************************/
void set_sim_gps_status(CELESTRON_GPS_STATUS value);
void set_sim_slew_rate(CELESTRON_SLEW_RATE value);
void set_sim_track_mode(CELESTRON_TRACK_MODE value);
void set_sim_slewing(bool isSlewing);
void set_sim_ra(double ra);
double get_sim_ra();
void set_sim_dec(double dec);
double get_sim_dec();
void set_sim_az(double az);
void set_sim_alt(double alt);

/**************************************************************************
 Diagnostics
**************************************************************************/
bool check_celestron_connection(int fd);

/**************************************************************************
 Get Info
**************************************************************************/
/** Get All firmware information in addition to model and version */
bool get_celestron_firmware(int fd, FirmwareInfo *info);
/** Get version */
bool get_celestron_version(int fd, FirmwareInfo *info);
/** Get hand controller variant */
bool get_celestron_variant(int fd, FirmwareInfo * info);
/** Get Mount model */
bool get_celestron_model(int fd, FirmwareInfo *info);
/** Get GPS Firmware version */
bool get_celestron_gps_firmware(int fd, FirmwareInfo *info);
/** Get RA Firmware version */
bool get_celestron_ra_firmware(int fd, FirmwareInfo *info);
/** Get DEC Firmware version */
bool get_celestron_dec_firmware(int fd, FirmwareInfo *info);
/** Get RA/DEC */
bool get_celestron_coords(int fd, double *ra, double *dec);
/** Get Az/Alt */
bool get_celestron_coords_azalt(int fd, double latitude, double *az, double *alt);
/** Get UTC/Date/Time */
bool get_celestron_utc_date_time(int fd, double *utc_hours, int *yy, int *mm, int *dd, int *hh, int *minute, int *ss);

/**************************************************************************
 Motion
**************************************************************************/
bool start_celestron_motion(int fd, CELESTRON_DIRECTION dir, CELESTRON_SLEW_RATE rate);
bool stop_celestron_motion(int fd, CELESTRON_DIRECTION dir);
bool abort_celestron(int fd);
bool slew_celestron(int fd, double ra, double dec);
bool slew_celestron_azalt(int fd, double latitude, double az, double alt);
bool sync_celestron(int fd, double ra, double dec);

/**************************************************************************
 Time & Location
**************************************************************************/
bool set_celestron_location(int fd, double longitude, double latitude);
bool set_celestron_datetime(int fd, struct ln_date *utc, double utc_offset);

/**************************************************************************
 Track Mode
**************************************************************************/
bool get_celestron_track_mode(int fd, CELESTRON_TRACK_MODE *mode);
bool set_celestron_track_mode(int fd, CELESTRON_TRACK_MODE mode);

/**************************************************************************
 Utility functions
**************************************************************************/
uint16_t get_ra_fraction(double ra);
uint16_t get_de_fraction(double de);
uint16_t get_az_fraction(double az);
uint16_t get_alt_fraction(double lat, double alt, double az);
uint16_t get_angle_fraction(double angle);

bool is_scope_slewing(int fd);

/**************************************************************************
 Hibernate/Wakup
 *************************************************************************/
bool hibernate(int fd);
bool wakeup(int fd);

/**************************************************************************
 Pulse Guide (experimental)
 *************************************************************************/
int SendPulseCmd(int fd, CELESTRON_DIRECTION direction, signed char rate, unsigned char duration_msec);
int SendPulseStatusCmd(int fd, CELESTRON_DIRECTION direction, bool &pulse_state);
