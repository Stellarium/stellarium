/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _QTSCRIPTMGR_HPP_
#define _QTSCRIPTMGR_HPP_

#include <QObject>
#include <QtScript>
#include <QStringList>
#include <QFile>
#include "VecMath.hpp"

//! Provide script API for Stellarium global functions.  Public slots in this class
//! may be used in Stellarium scripts, and are accessed as member function to the
//! "core" scripting object.  Module-specific functions, such as setting and clearing
//! of display flags (e.g. LandscapeMgr::setFlagAtmosphere) can be accessed directly
//! via the scripting object with the class name, e.g.  by using the scripting command:
//!  LandscapeMgr.setFlagAtmosphere(true);
class StelMainScriptAPI : public QObject
{
	Q_OBJECT
	Q_PROPERTY(double JDay READ getJDay WRITE setJDay)
	Q_PROPERTY(double timeSpeed READ getTimeRate WRITE setTimeRate)
					
public:
	StelMainScriptAPI(QObject *parent = 0);
	~StelMainScriptAPI();
	
// These functions will be available in scripts
public slots:
	//! Set the current date in Julian Day
	//! @param JD the Julian Date
	void setJDay(double JD);
	//! Get the current date in Julian Day
	//! @return the Julian Date
	double getJDay(void) const;

	//! set the date in ISO format, e.g. "2008-03-24T13:21:01"
	//! @param dt the date string to use.  Formats:
	//! - ISO, e.g. "2008-03-24T13:21:01"
	//! - "now" (set sim time to real time)
	//! - relative, e.g. "+ 4 days", "-2 weeks".  can use these
	//!   units: seconds, minutes, hours, days, weeks, months, years.
	//!   You may also append " sidereal" to use sidereal days and so on.
	//!   You can also use "now" at the start.  For example:
	//!   "now + 3 hours sidereal"
	//!   Note: you must use the plural all the time, even when the number 
	//!   of the unit is 1.  i.e. use "+ 1 days" not "+1 day".
	//! Note: when sidereal time is used, the length of time for
	//! each unit is dependent on the current planet.  By contrast
	//! when sidereal timeis not specified (i.e. solar time is used)
	//! the value is conventional - i.e. 1 day means 1 Earth Solar day.
	//! @param spec "local" or "utc" - only has an effect when 
	//! the ISO date type is used.
	void setDate(const QString& dt, const QString& spec="utc");
		
	//! Set time speed in JDay/sec
	//! @param ts the new rate of passage of time as a multiple of real time.
	//! For example if ts is 1, time will pass at the normal rate.  If ts == 10
	//! then simulation time will pass at 10 times the normal rate.
	//! If ts is negative, simulation time will go backwards.
	void setTimeRate(double ts);
	//! Get simulation time rate.
	//! @return time speed as a multiple of real time.
	double getTimeRate(void) const;
	
	//! Pauses the script for t seconds
	//! @param t the number of seconds to wait
	void wait(double t);

	//! Waits until a specified simulation date/time.  This function
	//! will take into account the rate (and direction) in which simulation
	//! time is passing. e.g. if a future date is specified and the 
	//! time is moving backwards, the function will return immediately.
	//! If the time rate is 0, the function will not wait.  This is to 
	//! prevent infinite wait time.
	//! @param dt the date string to use
	//! @param spec "local" or "utc"
	void waitFor(const QString& dt, const QString& spec="utc");
	
	//! Select an object by name
	//! @param name the name of the object to select (english)
	//! If the name is "", any currently selected objects will be
	//! de-selected.
	//! @param pointer whether or not to have the selection pointer enabled
	void selectObjectByName(const QString& name, bool pointer=false);

	//! Clear the display options, setting a "standard" view.
	//! Preset states:
	//! - natural : azimuthal mount, atmosphere, landscape, 
	//!   no lines, labels or markers
	//! - starchart : equatorial mount, constellation lines,
	//!   no landscape, atmoshere etc.  labels & markers on.
	//! @param state the name of a preset state.
	void clear(const QString& state="natural");

	//! move the current viewing direction to some specified altitude and azimuth
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param alt the altitude angle
	//! @param azi the azimuth angle
	//! @param duration the duration of the movement in seconds
	void moveToAltAzi(const QString& alt, const QString& azi, float duration=1.);

	//! move the current viewing direction to some specified right ascension and declination
	//! angles may be specified in a format recognised by StelUtils::getDecAngle()
	//! @param ra the right ascension angle
	//! @param dec the declination angle
	//! @param duration the duration of the movement in seconds
	void moveToRaDec(const QString& ra, const QString& dec, float duration=1.);

	//! Set the observer location
	//! @param longitude the longitude in degrees. E is +ve.  
	//!        values out of the range -180 .. 180 mean that
	//!        the longitude will not be set
	//! @param latitude the longitude in degrees. N is +ve.  
	//!        values out of the range -180 .. 180 mean that
	//!        the latitude will not be set
	//! @param altitude the new altitude in meters.
	//!        values less than -1000 mean the altitude will not
	//!        be set.
	//! @param duration the time for the transition from the 
	//!        old to the new location.
	//! @param name A name for the location (which will appear
	//!        in the status bar.
	//! @param planet the English name of the new planet.
	//!        If the planet name is not known (e.g. ""), the
	//!        planet will not be set.
	void setObserverLocation(double longitude, double latitude, double altitude, double duration=1., const QString& name="", const QString& planet="");

	//! Set the location by the name of the location.
	//! @param id the location ID as it would be found in the database
	//! of locations - do a search in the Location window to see what
	//! where is.  e.g. "York, UnitedKingdom".
	//! @param duration the number of seconds to take to move location.
	void setObserverLocation(const QString id, double duration=1.);

	//! Save a screenshot.
	//! @param prefix the prefix for the file name to use
	//! @param dir the path of the directory to save the screenshot in.  If
	//! none is specified, the default screenshot directory will be used.
	//! @param invert whether colors have to be inverted in the output image
	void screenshot(const QString& prefix, bool invert=false, const QString& dir="");

	//! Hide or show the GUI (toolbars)
	//! @param b the new hidden status of the toolbars, i.e. if b==true, the 
	//! toolbars will be hidden, else they will be made visible.
	void setHideGui(bool b);

        //! Set the minimum frames per second.  Usually this minimum will
        //! be switched to after there are no user events for some seconds
        //! to save power.  However, if can be useful to set this to a high
        //! value to improve playing smoothness in scripts.
        //! @param m the new minimum fps setting.
        void setMinFps(float m);

        //! Get the current minimum frames per second.
        float getMinFps();

        //! Set the maximum frames per second.
        //! @param m the new maximum fps setting.
        void setMaxFps(float m);

        //! Get the current maximum frames per second.
        float getMaxFps();

	//! Load an image which will have sky coordinates.
	//! @param id a string ID to be used when referring to this
	//! image (e.g. when changing the displayed status or deleting
	//! it.
	//! @param filename the file name of the image.  If a relative
	//! path is specified, "scripts/" will be prefixed before the
	//! image is searched for using StelFileMgr.
	//! @param ra0 The right ascension of the first corner of the image in degrees
	//! @param dec0 The declenation of the first corner of the image in degrees
	//! @param ra1 The right ascension of the second corner of the image in degrees
	//! @param dec1 The declenation of the second corner of the image in degrees
	//! @param ra2 The right ascension of the third corner of the image in degrees
	//! @param dec2 The declenation of the third corner of the image in degrees
	//! @param ra3 The right ascension of the fourth corner of the image in degrees
	//! @param dec3 The declenation of the fourth corner of the image in degrees
	//! @param minRes The minimum resolution setting for the image
	//! @param maxBright The maximum brightness setting for the image
	//! @param visible The initial visibility of the image
	void loadSkyImage(const QString& id, const QString& filename,
	                  double ra0, double dec0,
	                  double ra1, double dec1,
	                  double ra2, double dec2,
	                  double ra3, double dec3,
	                  double minRes=2.5, double maxBright=14, bool visible=true);

	//! Convenience function which allows the user to provide RA and DEC angles
	//! as strings (e.g. "12d 14m 8s" or "5h 26m 8s" - formats accepted by
	//! StelUtils::getDecAngle()).
	void loadSkyImage(const QString& id, const QString& filename,
	                  const QString& ra0, const QString& dec0,
	                  const QString& ra1, const QString& dec1,
	                  const QString& ra2, const QString& dec2,
	                  const QString& ra3, const QString& dec3,
	                  double minRes=2.5, double maxBright=14, bool visible=true);

	//! Convenience function which allows loading of a sky image based on a
	//! central coordinate, angular size and rotation.
	void loadSkyImage(const QString& id, const QString& filename,
	                  double ra, double dec, double angSize, double rotation,
	                  double minRes=2.5, double maxBright=14, bool visible=true);

	//! Convenience function which allows loading of a sky image based on a
	//! central coordinate, angular size and rotation.
	void loadSkyImage(const QString& id, const QString& filename,
	                  const QString& ra, const QString& dec, double angSize, double rotation,
	                  double minRes=2.5, double maxBright=14, bool visible=true);

	//! Remove a SkyImage.
	//! @param id the ID of the image to remove.
	void removeSkyImage(const QString& id);

	//! Load a sound from a file.
	//! @param filename the name of the file to load.
	//! @param id the identifier which will be used to refer to the sound
	//! when calling playSound, pauseSound, stopSound and dropSound.
	void loadSound(const QString& filename, const QString& id);

	//! Play a sound which has previously been loaded with loadSound
	//! @param id the identifier used when loadSound was called
	void playSound(const QString& id);

	//! Pause a sound which is playing.  Subsequent playSound calls will
	//! resume playing from the position in the file when it was paused.
	//! @param id the identifier used when loadSound was called
	void pauseSound(const QString& id);

	//! Stop a sound from playing.  This resets the position in the 
	//! sound to the start so that subsequent playSound calls will
	//! start from the beginning.
	//! @param id the identifier used when loadSound was called
	void stopSound(const QString& id);

	//! Drop a sound from memory.  You should do this before the end
	//! of your script.
	//! @param id the identifier used when loadSound was called
	void dropSound(const QString& id);

	//! print a debugging message to the console
	//! @param s the message to be displayed on the console.
	void debug(const QString& s);

signals:
	void requestLoadSkyImage(const QString& id, const QString& filename,
	                         double c1, double c2,
	                         double c3, double c4,
	                         double c5, double c6,
	                         double c7, double c8,
	                         double minRes, double maxBright, bool visible);

	void requestRemoveSkyImage(const QString& id);

	void requestLoadSound(const QString& filename, const QString& id);
	void requestPlaySound(const QString& id);
	void requestPauseSound(const QString& id);
	void requestStopSound(const QString& id);
	void requestDropSound(const QString& id);

private:
	//! For use in setDate and waitFor
	//! For parameter descriptions see setDate().
	//! @returns Julian day.
	double jdFromDateString(const QString& dt, const QString& spec);

};
		
//! Manage scripting in Stellarium
class StelScriptMgr : public QObject
{
Q_OBJECT
		
public:
	StelScriptMgr(QObject *parent=0);
	~StelScriptMgr();

	QStringList getScriptList(void);

	//! Find out if a script is running
	//! @return true if a script is running, else false
	bool scriptIsRunning(void);
	//! Get the ID (filename) of the currently running script
	//! @return Empty string if no script is running, else the 
	//! ID of the script which is running.
	QString runningScriptId(void);
	
public slots:
	//! Gets a single line name of the script. 
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Name: at the start.  If no 
	//! such comment is found, the file name will be returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	const QString getName(const QString& s);

	//! Gets the name of the script Author
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Author: at the start.  If no 
	//! such comment is found, "" is returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	const QString getAuthor(const QString& s);

	//! Gets the licensing terms for the script
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with License: at the start.  If no 
	//! such comment is found, "" is returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	const QString getLicense(const QString& s);

	//! Gets a description of the script.
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Description: at the start.
	//! The description is considered to be over when a line with no comment 
	//! is found.  If no such comment is found, QString("") is returned.
	//! If the file is not found or cannot be opened for some reason, an 
	//! Empty string will be returned.
	const QString getDescription(const QString& s);

	//! Run the script located at the given location
	//! @param fileName the location of the file containing the script.
	//! @return false if the named script could not be run, true otherwise
	bool runScript(const QString& fileName);

	//! Stops any running script.
	//! @return false if no script was running, true otherwise.
	bool stopScript(void);

private slots:
	//! Called at the end of the running thread
	void scriptEnded();

signals:
	//! Notification when a script starts running
	void scriptRunning();
	//! Notification when a script has stopped running 
	void scriptStopped();

private:
	// Utility functions for preprocessor
	QMap<QString, QString> mappify(const QStringList& args, bool lowerKey=false);
	bool strToBool(const QString& str);
	// Pre-processor functions
	bool preprocessScript(QFile& input, QFile& output, const QString& scriptDir);
	bool preprocessStratoScript(QFile& input, QFile& output, const QString& scriptDir);

	//! This function is for use with getName, getAuthor and getLicense.
	//! @param s the script id
	//! @param id the command line id, e.g. "Name"
	//! @param notFoundText the text to be returned if the key is not found
	//! @return the text following the id and : on a comment line near the top of 
	//! the script file (i.e. before there is a non-comment line).
	const QString getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText="");	
	QScriptEngine engine;
	
	//! The thread in which scripts are run
	class StelScriptThread* thread;
};

#endif // _QTSCRIPTMGR_HPP_
