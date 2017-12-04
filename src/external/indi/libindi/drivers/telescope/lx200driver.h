/*
    LX200 Driver
    Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

/* Slew speeds */
enum TSlew
{
    LX200_SLEW_MAX,
    LX200_SLEW_FIND,
    LX200_SLEW_CENTER,
    LX200_SLEW_GUIDE
};
/* Alignment modes */
enum TAlign
{
    LX200_ALIGN_POLAR,
    LX200_ALIGN_ALTAZ,
    LX200_ALIGN_LAND
};
/* Directions */
enum TDirection
{
    LX200_NORTH,
    LX200_WEST,
    LX200_EAST,
    LX200_SOUTH,
    LX200_ALL
};
/* Formats of Right Ascension and Declination */
enum TFormat
{
    LX200_SHORT_FORMAT,
    LX200_LONG_FORMAT,
    LX200_LONGER_FORMAT
};
/* Time Format */
enum TTimeFormat
{
    LX200_24,
    LX200_AM,
    LX200_PM
};
/* Focus operation */
enum TFocusMotion
{
    LX200_FOCUSIN,
    LX200_FOCUSOUT
};
enum TFocusSpeed
{
    LX200_HALTFOCUS = 0,
    LX200_FOCUSSLOW,
    LX200_FOCUSFAST
};
/* Library catalogs */
enum TCatalog
{
    LX200_STAR_C,
    LX200_DEEPSKY_C
};
/* Frequency mode */
enum StarCatalog
{
    LX200_STAR,
    LX200_SAO,
    LX200_GCVS
};
/* Deep Sky Catalogs */
enum DeepSkyCatalog
{
    LX200_NGC,
    LX200_IC,
    LX200_UGC,
    LX200_CALDWELL,
    LX200_ARP,
    LX200_ABELL,
    LX200_MESSIER_C
};
/* Mount tracking frequency, in Hz */
enum TFreq
{
    LX200_TRACK_SIDEREAL,
    LX200_TRACK_SOLAR,
    LX200_TRACK_LUNAR,
    LX200_TRACK_MANUAL
};

#define MaxReticleDutyCycle 15
#define MaxFocuserSpeed     4

/* GET formatted sexagisemal value from device, return as double */
#define getLX200RA(fd, x)     getCommandSexa(fd, x, ":GR#")
#define getLX200DEC(fd, x)    getCommandSexa(fd, x, ":GD#")
#define getObjectRA(fd, x)    getCommandSexa(fd, x, ":Gr#")
#define getObjectDEC(fd, x)   getCommandSexa(fd, x, ":Gd#")
#define getLocalTime12(fd, x) getCommandSexa(fd, x, ":Ga#")
#define getLocalTime24(fd, x) getCommandSexa(fd, x, ":GL#")
#define getSDTime(fd, x)      getCommandSexa(fd, x, ":GS#")
#define getLX200Alt(fd, x)    getCommandSexa(fd, x, ":GA#")
#define getLX200Az(fd, x)     getCommandSexa(fd, x, ":GZ#")

/* GET String from device and store in supplied buffer x */
#define getObjectInfo(fd, x)    getCommandString(fd, x, ":LI#")
#define getVersionDate(fd, x)   getCommandString(fd, x, ":GVD#")
#define getVersionTime(fd, x)   getCommandString(fd, x, ":GVT#")
#define getFullVersion(fd, x)   getCommandString(fd, x, ":GVF#")
#define getVersionNumber(fd, x) getCommandString(fd, x, ":GVN#")
#define getProductName(fd, x)   getCommandString(fd, x, ":GVP#")
#define turnGPS_StreamOn(fd)    getCommandString(fd, x, ":gps#")

/* GET Int from device and store in supplied pointer to integer x */
#define getUTCOffset(fd, x)         getCommandInt(fd, x, ":GG#")
#define getMaxElevationLimit(fd, x) getCommandInt(fd, x, ":Go#")
#define getMinElevationLimit(fd, x) getCommandInt(fd, x, ":Gh#")

/* Generic set, x is an integer */
#define setReticleDutyFlashCycle(fd, x) setCommandInt(fd, x, ":BD")
#define setReticleFlashRate(fd, x)      setCommandInt(fd, x, ":B")
#define setFocuserSpeed(fd, x)          setCommandInt(fd, x, ":F")
#define setSlewSpeed(fd, x)             setCommandInt(fd, x, ":Sw")

/* Set X:Y:Z */
#define setLocalTime(fd, x, y, z) setCommandXYZ(fd, x, y, z, ":SL")
#define setSDTime(fd, x, y, z)    setCommandXYZ(fd, x, y, z, ":SS")

/* GPS Specefic */
#define turnGPSOn(fd)                   write(fd, ":g+#", 4)
#define turnGPSOff(fd)                  write(fd, ":g-#", 4)
#define alignGPSScope(fd)               write(fd, ":Aa#", 4)
#define gpsSleep(fd)                    write(fd, ":hN#", 4)
#define gpsWakeUp(fd)                   write(fd, ":hW#", 4);
#define gpsRestart(fd)                  write(fd, ":I#", 3);
#define updateGPS_System(fd)            setStandardProcedure(fd, ":gT#")
#define enableDecAltPec(fd)             write(fd, ":QA+#", 4)
#define disableDecAltPec(fd)            write(fd, ":QA-#", 4)
#define enableRaAzPec(fd)               write(fd, ":QZ+#", 4)
#define disableRaAzPec(fd)              write(fd, ":QZ-#", 4)
#define activateAltDecAntiBackSlash(fd) write(fd, ":$BAdd#", 7)
#define activateAzRaAntiBackSlash(fd)   write(fd, ":$BZdd#", 7)
#define SelenographicSync(fd)           write(fd, ":CL#", 4);

#define slewToAltAz(fd)               setStandardProcedure(fd, ":MA#")
#define toggleTimeFormat(fd)          write(fd, ":H#", 3)
#define increaseReticleBrightness(fd) write(fd, ":B+#", 4)
#define decreaseReticleBrightness(fd) write(fd, ":B-#", 4)
#define turnFanOn(fd)                 write(fd, ":f+#", 4)
#define turnFanOff(fd)                write(fd, ":f-#", 4)
#define seekHomeAndSave(fd)           write(fd, ":hS#", 4)
#define seekHomeAndSet(fd)            write(fd, ":hF#", 4)
#define turnFieldDeRotatorOn(fd)      write(fd, ":r+#", 4)
#define turnFieldDeRotatorOff(fd)     write(fd, ":r-#", 4)
#define slewToPark(fd)                write(fd, ":hP#", 4)
#define initTelescope(fd)             write(fd, ":I#", 3)

/**************************************************************************
 Basic I/O - OBSELETE
**************************************************************************/
/*int openPort(const char *portID);
int portRead(char *buf, int nbytes, int timeout);
int portWrite(const char * buf);
int LX200readOut(int timeout);
int Connect(const char* device);
void Disconnect();*/

/**************************************************************************
 Diagnostics
 **************************************************************************/
char ACK(int fd);
/*int testTelescope();
int testAP();*/
int check_lx200_connection(int fd);

/**************************************************************************
 Get Commands: store data in the supplied buffer. Return 0 on success or -1 on failure
 **************************************************************************/

/* Get Double from Sexagisemal */
int getCommandSexa(int fd, double *value, const char *cmd);
/* Get String */
int getCommandString(int fd, char *data, const char *cmd);
/* Get Int */
int getCommandInt(int fd, int *value, const char *cmd);
/* Get tracking frequency */
int getTrackFreq(int fd, double *value);
/* Get site Latitude */
int getSiteLatitude(int fd, int *dd, int *mm);
/* Get site Longitude */
int getSiteLongitude(int fd, int *ddd, int *mm);
/* Get Calender data */
int getCalendarDate(int fd, char *date);
/* Get site Name */
int getSiteName(int fd, char *siteName, int siteNum);
/* Get Home Search Status */
int getHomeSearchStatus(int fd, int *status);
/* Get OTA Temperature */
int getOTATemp(int fd, double *value);
/* Get time format: 12 or 24 */
int getTimeFormat(int fd, int *format);
/* Get RA, DEC from Sky Commander controller */
int updateSkyCommanderCoord(int fd, double *ra, double *dec);
/* Get RA, DEC from Intelliscope/SkyWizard controllers */
int updateIntelliscopeCoord(int fd, double *ra, double *dec);

/**************************************************************************
 Set Commands
 **************************************************************************/

/* Set Int */
int setCommandInt(int fd, int data, const char *cmd);
/* Set Sexigesimal */
int setCommandXYZ(int fd, int x, int y, int z, const char *cmd);
/* Common routine for Set commands */
int setStandardProcedure(int fd, const char *writeData);
/* Set Slew Mode */
int setSlewMode(int fd, int slewMode);
/* Set Alignment mode */
int setAlignmentMode(int fd, unsigned int alignMode);
/* Set Object RA */
int setObjectRA(int fd, double ra);
/* set Object DEC */
int setObjectDEC(int fd, double dec);
/* Set Calender date */
int setCalenderDate(int fd, int dd, int mm, int yy);
/* Set UTC offset */
int setUTCOffset(int fd, double hours);
/* Set Track Freq */
int setTrackFreq(int fd, double trackF);
/* Set current site longitude */
int setSiteLongitude(int fd, double Long);
/* Set current site latitude */
int setSiteLatitude(int fd, double Lat);
/* Set Object Azimuth */
int setObjAz(int fd, double az);
/* Set Object Altitude */
int setObjAlt(int fd, double alt);
/* Set site name */
int setSiteName(int fd, char *siteName, int siteNum);
/* Set maximum slew rate */
int setMaxSlewRate(int fd, int slewRate);
/* Set focuser motion */
int setFocuserMotion(int fd, int motionType);
/* SET GPS Focuser raneg (1 to 4) */
int setGPSFocuserSpeed(int fd, int speed);
/* Set focuser speed mode */
int setFocuserSpeedMode(int fd, int speedMode);
/* Set minimum elevation limit */
int setMinElevationLimit(int fd, int min);
/* Set maximum elevation limit */
int setMaxElevationLimit(int fd, int max);

/**************************************************************************
 Motion Commands
 **************************************************************************/
/* Slew to the selected coordinates */
int Slew(int fd);
/* Synchronize to the selected coordinates and return the matching object if any */
int Sync(int fd, char *matchedObject);
/* Abort slew in all axes */
int abortSlew(int fd);
/* Move into one direction, two valid directions can be stacked */
int MoveTo(int fd, int direction);
/* Halt movement in a particular direction */
int HaltMovement(int fd, int direction);
/* Select the tracking mode */
int selectTrackingMode(int fd, int trackMode);
/* Is Slew complete? 0 if complete, 1 if in progress, otherwise return an error */
int isSlewComplete(int fd);
/* Send Pulse-Guide command (timed guide move), two valid directions can be stacked */
int SendPulseCmd(int fd, int direction, int duration_msec);

/**************************************************************************
 Other Commands
 **************************************************************************/
/* Determines LX200 RA/DEC format, tries to set to long if found short */
int checkLX200Format(int fd);
/* return the controller_format enum value */
int getLX200Format();
/* Select a site from the LX200 controller */
int selectSite(int fd, int siteNum);
/* Select a catalog object */
int selectCatalogObject(int fd, int catalog, int NNNN);
/* Select a sub catalog */
int selectSubCatalog(int fd, int catalog, int subCatalog);
/* Set Debug */
void setLX200Debug(const char *deviceName, unsigned int debug_level);
