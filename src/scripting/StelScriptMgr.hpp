/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau, 2009 Matthew Gates
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELSCRIPTMGR_HPP
#define STELSCRIPTMGR_HPP

#include <QObject>
#include <QStringList>
#include <QFile>
#include <QTimer>
#include <QEventLoop>
#include <QMap>
#include <QPair>
#include <QSet>

#ifdef ENABLE_SCRIPT_QML
#include <QMutex>
#include <QJSValue>
#include "V3d.hpp"
class QJSEngine;
#else
class StelScriptEngineAgent;
class QScriptEngine;
#endif

#include "StelMainScriptAPI.hpp"

#ifdef ENABLE_SCRIPT_CONSOLE
class ScriptConsole;
#endif

//! Manage scripting in Stellarium
//! Notes on migration from QtScript to QJSEngine
//! - The old engine had isEvaluating(). We must use a mutex for the same idea.
//! - There is no script.pause() function. We can only stop a running script.
class StelScriptMgr : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString runningScriptId READ runningScriptId NOTIFY runningScriptIdChanged)
	// CAVEAT: Do not convert this class to a StelModule: The Qt properties (esp. the following 2) should not be available in the StelProperty system (and the object should not be scriptable)!
	Q_PROPERTY(bool flagAllowExternalScreenshotDir READ getFlagAllowExternalScreenshotDir WRITE setFlagAllowExternalScreenshotDir NOTIFY flagAllowExternalScreenshotDirChanged)
	Q_PROPERTY(bool flagAllowWriteAbsolutePaths READ getFlagAllowWriteAbsolutePaths WRITE setFlagAllowWriteAbsolutePaths NOTIFY flagAllowWriteAbsolutePathsChanged)

#ifdef ENABLE_SCRIPT_CONSOLE
friend class ScriptConsole;
#endif

public:
	StelScriptMgr(QObject *parent=Q_NULLPTR);
	~StelScriptMgr() Q_DECL_OVERRIDE;

	QStringList getScriptList() const;

	//! Find out if a script is running
	//! @return true if a script is running, else false
	#ifdef ENABLE_SCRIPT_QML
	bool scriptIsRunning();
	#else
	bool scriptIsRunning() const;
	#endif
	//! Get the ID (usually filename) of the currently running script
	//! @return Empty string if no script is running, else the 
	//! ID of the script which is running.
	QString runningScriptId() const;

	// Pre-processor functions
	//! Preprocess script, esp. process include instructions.
	//! if the command line option --verbose has been given,
	//! this dumps the preprocessed script with line numbers attached to log.
	//! This helps to understand the line number given by the usual error message.
	bool preprocessScript(const QString fileName, const QString& input, QString& output, const QString& scriptDir, int &errLoc);
	bool preprocessFile(const QString fileName, QFile &input, QString& output, const QString& scriptDir);
	
	//! Add all the StelModules into the script engine
	void addModules();

	//! Add a single QObject as scripting object
	//! The object must have set a name by QObject::setObjectName().
	//! @note use this sparingly and with caution, and only add one object per class!
	void addObject(QObject *obj);

	//! Define JS classes Vec3f, Vec3d
	#ifdef ENABLE_SCRIPT_QML
	static void defVecClasses(QJSEngine *engine);
	#else
	static void defVecClasses(QScriptEngine *engine);
	#endif

	//! Permit access to StelScriptMainAPI's methods
	const QMetaObject * getMetaOfStelMainScriptAPI() const { return mainAPI->metaObject(); }

	//! Accessor to QEventLoop
	QEventLoop* getWaitEventLoop() const { return waitEventLoop; }

public slots:
	//! Returns a HTML description of the specified script.
	//! Includes name, author, description...
	//! @param s the file name of the script whose HTML description is to be returned.
	//! @param generateDocumentTags if true, the main wrapping document tags (\<html\>\<body\>...\</body\>\</html\>) are also generated
	QString getHtmlDescription(const QString& s, bool generateDocumentTags=true);

	//! Gets a single line name of the script. 
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Name: at the start.  If no 
	//! such comment is found, the file name will be returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	QString getName(const QString& s);

	//! Gets the name of the script Author
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Author: at the start.  If no 
	//! such comment is found, "" is returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	QString getAuthor(const QString& s);

	//! Gets the licensing terms for the script
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with License: at the start.  If no 
	//! such comment is found, "" is returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	QString getLicense(const QString& s);

	//! Gets the version of the script
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Version: at the start.  If no
	//! such comment is found, "" is returned.  If the file
	//! is not found or cannot be opened for some reason, an Empty string
	//! will be returned.
	QString getVersion(const QString& s);

	//! Gets a description of the script.
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Description: at the start.
	//! The description is considered to be over when a line with no comment 
	//! is found.  If no such comment is found, QString("") is returned.
	//! If the file is not found or cannot be opened for some reason, an 
	//! Empty string will be returned.
	QString getDescription(const QString& s);

	//! Gets the default shortcut of the script.
	//! @param s the file name of the script whose name is to be returned.
	//! @return text following a comment with Shortcut: at the start.
	//! If no such comment is found, QString("") is returned.
	//! If the file is not found or cannot be opened for some reason, an
	//! Empty string will be returned.
	QString getShortcut(const QString& s);

	//! Run the script located in the given file. In essence, this calls prepareScript and runPreprocessedScript.
	//! @note This is a blocking call! The event queue is held up by calls of QCoreApplication::processEvents().
	//! @param fileName the location of the file containing the script.
	//! @param includePath the directory to use when searching for include files
	//! in the SSC preprocessor. If empty, this will be the same as the
	//! directory where the script file resides. If you're running a generated script from
	//! a temp directory, but want to include a file from elsewhere, it 
	//! can be usetul to set it to something else (e.g. in ScriptConsole).
	//! @return false if the named script could not be prepared or run, true otherwise
	bool runScript(const QString& fileName, const QString& includePath="");

	//! Runs the script code given. This can be used for quick script executions, without having to create a
	//! temporary file first.
	//! @note This is a blocking call! The event queue is held up by calls of QCoreApplication::processEvents().
	//! @param scriptId path name, if available, or something helpful
	//! @param scriptCode The script to execute
	//! @param errLoc offset of erroneous include line, or -1
	//! @param includePath If a null string (the default), no pre-processing is done. If an empty string, the default
	//! script directories are used (script/ in both user and install directory). Otherwise, the given directory is used.
	//! @return false if the named script code could not be prepared or run, true otherwise
	bool runScriptDirect(const QString scriptId, const QString& scriptCode, int &errLoc, const QString &includePath = QString());

	//! Convenience method similar to runScriptDirect(const QString scriptId, const QString& scriptCode, int &errLoc, const QString &includePath = QString());
	//! when scriptId and errLoc are not relevant. (Required e.g. in RemoteControl)
	bool runScriptDirect(const QString& scriptCode, const QString &includePath = QString());

	//! Runs preprocessed script code which has been generated using runPreprocessedScript().
	//! In general, you do not want to use this method, use runScript() or runScriptDirect() instead.
	//! @note This is a blocking call! The event queue is held up by calls of QCoreApplication::processEvents().
	//! @param preprocessedScript the string containing the preprocessed script.
	//! @param scriptId The name of the script. Usually should correspond to the file name.
	//! @return false if the given script code could not be run, true otherwise
	bool runPreprocessedScript(const QString& preprocessedScript, const QString &scriptId);

	//! Loads a script file and does all preparatory steps except for actually executing the script in the engine.
	//! Use runPreprocessedScript to execute the script.
	//! It should be safe to call this method from another thread.
	//! @param script returns the preprocessed script text
	//! @param fileName the location of the file containing the script.
	//! @param includePath the directory to use when searching for include files
	//! in the SSC preprocessor.  Usually this will be the same as the
	//! script file itself, but if you're running a generated script from
	//! a temp directory, but want to include a file from elsewhere, it
	//! can be usetul to set it to something else (e.g. in ScriptConsole).
	//! @return false if the named script could not be prepared, true otherwise
	bool prepareScript(QString& script, const QString& fileName, const QString& includePath="");

	//! Stops any running script.
	//! @return false if no script was running, true otherwise.
	void stopScript();

	//! Changes the rate at which the script executes as a multiple of real time.
	//! Note that this is not the same as the rate at which simulation time passes
	//! because the script running at normal rate might set the simulation time rate
	//! to be non-real time.
	//! @param r rate, e.g. 2.0 = script runs at twice the normal rate
	void setScriptRate(double r);
	
	//! Get the rate at which the script is running as a multiple of the normal
	//! execution rate.
	double getScriptRate() const;

	//! cause the emission of the scriptDebug signal. This is so that functions in
	//! StelMainScriptAPI can explicitly send information to the ScriptConsole
	void debug(const QString& msg);

	//! cause the emission of the scriptOutput signal. This is so that functions in
	//! StelMainScriptAPI can explicitly send information to the ScriptConsole
	void output(const QString& msg);

	//! Reset output file and cause the emission of an (empty) scriptOutput signal.
	void resetOutput(void);

	//! Save output file to new file.
	//! This is required to allow reading with other program on Windows while output.txt is still open.
	//! @param filename new filename. If this is not an absolute path, it will be created in the same directory as output.txt
	//! @note For storing to absolute path names, set [scripts]/flag_script_allow_write_absolute_path=true.
	void saveOutputAs(const QString &filename);

	//! Pause a running script.
	//! @note This method only works with the Qt5-based scripting engine.
	void pauseScript();

	//! Resume a paused script.
	//! @note This method only works with the Qt5-based scripting engine.
	void resumeScript();

	//! Ensure that users must actively enable storing screenshots to directories configured in scripts.
	//! If this is false, a directory dir given in StelMainScriptAPI::screenshot(prefix, invert, dir, ...) is ignored.
	bool getFlagAllowExternalScreenshotDir() const {return flagAllowExternalScreenshotDir;}
	void setFlagAllowExternalScreenshotDir(bool flag);

	//! Ensure that users must actively enable writing to absolute paths configured in scripts.
	//! If this is false, the output file is just stored to the user data directory
	bool getFlagAllowWriteAbsolutePaths() const {return flagAllowWriteAbsolutePaths;}
	void setFlagAllowWriteAbsolutePaths(bool flag);

private slots:
	//! Called at the end of the running threa
	void scriptEnded();
signals:
	//! Emitted when the running script id changes (also on start/stop)
	void runningScriptIdChanged(const QString& id);
	//! Notification when a script starts running
	void scriptRunning();
	//! Notification when a script has paused running
	void scriptPaused();
	//! Notification when a script has stopped running 
	void scriptStopped();
	//! Notification of a script event - warnings, current execution line etc.
	void scriptDebug(const QString&);
	//! Notification of a script event - output line.
	void scriptOutput(const QString&);
	//! Notification that screenshots to script-defined directories are allowed
	void flagAllowExternalScreenshotDirChanged(bool flag);
	//! Notification that flag to allow scripts to write to absolute paths changed
	void flagAllowWriteAbsolutePathsChanged(bool flag);

private:
	// Utility functions for preprocessor. DEAD CODE!
	//QMap<QString, QString> mappify(const QStringList& args, bool lowerKey=false);
	static bool strToBool(const QString& str);
	// The recursive preprocessing workhorse.
	void expand(const QString fileName, const QString &input, QString &output, const QString &scriptDir, int &errLoc);

	//! Generate one StelAction per script.
	//! The name of the action is of the form: "actionScript/<script-path>"
	void initActions();

	//! This function is for use with getName, getAuthor and getLicense.
	//! @param s the script id
	//! @param id the command line id, e.g. "Name"
	//! @param notFoundText the text to be returned if the key is not found
	//! @return the text following the id and : on a comment line near the top of 
	//! the script file (i.e. before there is a non-comment line).
	QString getHeaderSingleLineCommentText(const QString& s, const QString& id, const QString& notFoundText="");

#ifdef ENABLE_SCRIPT_QML
	QJSEngine *engine;
	QMutex mutex; // we need to lock this while a script is running.
	QJSValue result;
#else
	QScriptEngine* engine;
	//Script engine agent
	StelScriptEngineAgent *agent;
#endif
	//! The thread in which scripts are run
	StelMainScriptAPI *mainAPI;

	//! The QEventLoop for wait and waitFor
	QEventLoop* waitEventLoop;
	
	QString scriptFileName;
	

	// Map line numbers of output to <path>:<line>
	int outline;
	QMap<int,QPair<QString,int>> num2loc;
	QString lookup( int outline );

	// Registry for include files
	QSet<QString> includeSet;

	//! Ensure that users must actively allow storing screenshots to directories configured in scripts.
	//! If this is false, a directory dir given in StelMainScriptAPI::screenshot(prefix, invert, dir, ...) is ignored.
	bool flagAllowExternalScreenshotDir;
	//! Ensure that users must actively allow storing output data to absolute paths.
	//! If this is false, the output file is stored to the user data directory.
	bool flagAllowWriteAbsolutePaths;
};

#endif // STELSCRIPTMGR_HPP

