/*
    IEQ45 Driver
    Copyright (C) 2011 Nacho Mas (mas.ignacio@gmail.com). Only litle changes
    from lx200basic made it by Jasem Mutlaq (mutlaqja@ikarustech.com)

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
    IEQ45_SLEW_MAX,
    IEQ45_SLEW_FIND,
    IEQ45_SLEW_CENTER,
    IEQ45_SLEW_GUIDE
};
/* Alignment modes */
enum TAlign
{
    IEQ45_ALIGN_POLAR,
    IEQ45_ALIGN_ALTAZ,
    IEQ45_ALIGN_LAND
};
/* Directions */
enum TDirection
{
    IEQ45_NORTH,
    IEQ45_WEST,
    IEQ45_EAST,
    IEQ45_SOUTH,
    IEQ45_ALL
};
/* Formats of Right ascention and Declenation */
enum TFormat
{
    IEQ45_SHORT_FORMAT,
    IEQ45_LONG_FORMAT
};
/* Time Format */
enum TTimeFormat
{
    IEQ45_24,
    IEQ45_AM,
    IEQ45_PM
};
/* Focus operation */
enum TFocusMotion
{
    IEQ45_FOCUSIN,
    IEQ45_FOCUSOUT
};
enum TFocusSpeed
{
    IEQ45_HALTFOCUS = 0,
    IEQ45_FOCUSSLOW,
    IEQ45_FOCUSFAST
};
/* Library catalogs */
enum TCatalog
{
    IEQ45_STAR_C,
    IEQ45_DEEPSKY_C
};
/* Frequency mode */
enum StarCatalog
{
    IEQ45_STAR,
    IEQ45_SAO,
    IEQ45_GCVS
};
/* Deep Sky Catalogs */
enum DeepSkyCatalog
{
    IEQ45_NGC,
    IEQ45_IC,
    IEQ45_UGC,
    IEQ45_CALDWELL,
    IEQ45_ARP,
    IEQ45_ABELL,
    IEQ45_MESSIER_C
};
/* Mount tracking frequency, in Hz */
enum TFreq
{
    IEQ45_TRACK_SIDERAL,
    IEQ45_TRACK_LUNAR,
    IEQ45_TRACK_SOLAR,
    IEQ45_TRACK_ZERO
};

#define MaxReticleDutyCycle 15
#define MaxFocuserSpeed     4

/* GET formatted sexagisemal value from device, return as double */
#define getIEQ45RA(fd, x)  getCommandSexa(fd, x, ":GR#") //OK
#define getIEQ45DEC(fd, x) getCommandSexa(fd, x, ":GD#") //OK
//#define getObjectRA(fd, x)				getCommandSexa(fd, x, ":Gr#")		//NO OK
//#define getObjectDEC(fd, x)				getCommandSexa(fd, x, ":Gd#")		//NO OK
//#define getLocalTime12(fd, x)				getCommandSexa(fd, x, ":Ga#")		//NO OK
#define getLocalTime24(fd, x) getCommandSexa(fd, x, ":GL#") //OK
#define getSDTime(fd, x)      getCommandSexa(fd, x, ":GS#") //OK
#define getIEQ45Alt(fd, x)    getCommandSexa(fd, x, ":GA#") //OK
#define getIEQ45Az(fd, x)     getCommandSexa(fd, x, ":GZ#") //OK

/* GET String from device and store in supplied buffer x */
//#define getObjectInfo(fd, x)				getCommandString(fd, x, ":LI#")	//NO OK
//#define getVersionDate(fd, x)				getCommandString(fd, x, ":GVD#")	//NO OK
//#define getVersionTime(fd, x)				getCommandString(fd, x, ":GVT#")	//NO OK
//#define getFullVersion(fd, x)				getCommandString(fd, x, ":GVF#")	//NO OK
//#define getVersionNumber(fd, x)			getCommandString(fd, x, ":GVN#")	//NO OK
//#define getProductName(fd, x)				getCommandString(fd, x, ":GVP#")	//NO OK
//#define turnGPS_StreamOn(fd)				getCommandString(fd, x, ":gps#")	//NO OK

/* GET Int from device and store in supplied pointer to integer x */
#define getUTCOffset(fd, x) getCommandInt(fd, x, ":GG#") //OK
//#define getMaxElevationLimit(fd, x)			getCommandInt(fd, x, ":Go#")  		//NO OK
//#define getMinElevationLimit(fd, x)			getCommandInt(fd, x, ":Gh#")  		//NO OK

/* Generic set, x is an integer */
//#define setReticleDutyFlashCycle(fd, x)			setCommandInt(fd, x, ":BD")
#define setReticleFlashRate(fd, x) setCommandInt(fd, x, ":B")
#define setFocuserSpeed(fd, x)     setCommandInt(fd, x, ":F")
#define setSlewSpeed(fd, x)        setCommandInt(fd, x, ":Sw")

/* Set X:Y:Z */
#define setLocalTime(fd, x, y, z) setCommandXYZ(fd, x, y, z, ":SL")
#define setSDTime(fd, x, y, z)    setCommandXYZ(fd, x, y, z, ":SS")

/* GPS Specefic */
#define turnGPSOn(fd)                   write(fd, ":g+#", 5)
#define turnGPSOff(fd)                  write(fd, ":g-#", 5)
#define alignGPSScope(fd)               write(fd, ":Aa#", 5)
#define gpsSleep(fd)                    write(fd, ":hN#", 5)
#define gpsWakeUp(fd)                   write(fd, ":hW#", 5);
#define gpsRestart(fd)                  write(fd, ":I#", 4);
#define updateGPS_System(fd)            setStandardProcedure(fd, ":gT#")
#define enableDecAltPec(fd)             write(fd, ":QA+#", 6)
#define disableDecAltPec(fd)            write(fd, ":QA-#", 6)
#define enableRaAzPec(fd)               write(fd, ":QZ+#", 6)
#define disableRaAzPec(fd)              write(fd, ":QZ-#", 6)
#define activateAltDecAntiBackSlash(fd) write(fd, "$BAdd#", 7)
#define activateAzRaAntiBackSlash(fd)   write(fd, "$BZdd#", 7)
#define SelenographicSync(fd)           write(fd, ":CL#", 5);

#define slewToAltAz(fd)               setStandardProcedure(fd, ":MA#")
#define toggleTimeFormat(fd)          write(fd, ":H#", 4)
#define increaseReticleBrightness(fd) write(fd, ":B+#", 5)
#define decreaseReticleBrightness(fd) write(fd, ":B-#", 5)
#define turnFanOn(fd)                 write(fd, ":f+#", 5)
#define turnFanOff(fd)                write(fd, ":f-#", 5)
#define seekHomeAndSave(fd)           write(fd, ":hS#", 5)
#define seekHomeAndSet(fd)            write(fd, ":hF#", 5)
#define turnFieldDeRotatorOn(fd)      write(fd, ":r+#", 5)
#define turnFieldDeRotatorOff(fd)     write(fd, ":r-#", 5)
#define slewToPark(fd)                write(fd, ":hP#", 5)

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 Basic I/O - OBSELETE
**************************************************************************/
/*int openPort(const char *portID);
int portRead(char *buf, int nbytes, int timeout);
int portWrite(const char * buf);
int IEQ45readOut(int timeout);
int Connect(const char* device);
void Disconnect();*/

/**************************************************************************
 Diagnostics
 **************************************************************************/
char ACK(int fd);
/*int testTelescope();
int testAP();*/
int check_IEQ45_connection(int fd);

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
/* Get Number of Bars */
int getNumberOfBars(int fd, int *value);
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
int setStandardProcedure(int fd, char *writeData);
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
/* Select Astro-Physics tracking mode */
int selectAPTrackingMode(int fd, int trackMode);
/* Send Pulse-Guide command (timed guide move), two valid directions can be stacked */
int SendPulseCmd(int fd, int direction, int duration_msec);

/**************************************************************************
 Other Commands
 **************************************************************************/
/* Ensures IEQ45 RA/DEC format is long */
int checkIEQ45Format(int fd);
/* Select a site from the IEQ45 controller */
int selectSite(int fd, int siteNum);
/* Select a catalog object */
int selectCatalogObject(int fd, int catalog, int NNNN);
/* Select a sub catalog */
int selectSubCatalog(int fd, int catalog, int subCatalog);

#ifdef __cplusplus
}
#endif
