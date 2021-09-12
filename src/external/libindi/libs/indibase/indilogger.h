/*******************************************************************************
  Copyright (C) 2012 Evidence Srl - www.evidence.eu.com

 Adapted to INDI Library by Jasem Mutlaq & Geehalel.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indiapi.h"
#include "defaultdevice.h"

#include <stdarg.h>
#include <fstream>
#include <ostream>
#include <string>
#include <sstream>
#ifdef Q_OS_WIN
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

/**
 * @brief Macro to configure the logger.
 * Example of configuration of the Logger:
 * 	DEBUG_CONF("outputfile", Logger::file_on|Logger::screen_on, DBG_DEBUG, DBG_ERROR);
 */
#define DEBUG_CONF(outputFile, configuration, fileVerbosityLevel, screenVerbosityLevel)                       \
    {                                                                                                         \
        Logger::getInstance().configure(outputFile, configuration, fileVerbosityLevel, screenVerbosityLevel); \
    }

/**
 * @brief Macro to print log messages.
 * Example of usage of the Logger:
 *	    DEBUG(DBG_DEBUG, "hello " << "world");
 */
/*
#define DEBUG(priority, msg) { \
	std::ostringstream __debug_stream__; \
	__debug_stream__ << msg; \
	Logger::getInstance().print(priority, __FILE__, __LINE__, \
			__debug_stream__.str()); \
	}
*/
#define DEBUG(priority, msg) INDI::Logger::getInstance().print(getDeviceName(), priority, __FILE__, __LINE__, msg)
#define DEBUGF(priority, msg, ...) \
    INDI::Logger::getInstance().print(getDeviceName(), priority, __FILE__, __LINE__, msg, __VA_ARGS__)

#define DEBUGDEVICE(device, priority, msg) INDI::Logger::getInstance().print(device, priority, __FILE__, __LINE__, msg)
#define DEBUGFDEVICE(device, priority, msg, ...) \
    INDI::Logger::getInstance().print(device, priority, __FILE__, __LINE__, msg, __VA_ARGS__)

/**
 * @brief Shorter logging macros. In order to use these macros, the function
 * (or method) "getDeviceName()" must be defined in the calling scope.
 *
 * Usage examples:
 *	    LOG_DEBUG("hello " << "world");
 *	    LOGF_WARN("hello %s", "world");
 */
#define LOG_ERROR(txt)  DEBUG(INDI::Logger::DBG_ERROR, (txt))
#define LOG_WARN(txt)   DEBUG(INDI::Logger::DBG_WARNING, (txt))
#define LOG_INFO(txt)   DEBUG(INDI::Logger::DBG_SESSION, (txt))
#define LOG_DEBUG(txt)  DEBUG(INDI::Logger::DBG_DEBUG, (txt))
#define LOG_EXTRA1(txt)  DEBUG(INDI::Logger::DBG_EXTRA_1, (txt))
#define LOG_EXTRA2(txt)  DEBUG(INDI::Logger::DBG_EXTRA_2, (txt))
#define LOG_EXTRA3(txt)  DEBUG(INDI::Logger::DBG_EXTRA_3, (txt))

#define LOGF_ERROR(fmt, ...) DEBUGF(INDI::Logger::DBG_ERROR, (fmt), __VA_ARGS__)
#define LOGF_WARN(fmt, ...)  DEBUGF(INDI::Logger::DBG_WARNING, (fmt), __VA_ARGS__)
#define LOGF_INFO(fmt, ...)  DEBUGF(INDI::Logger::DBG_SESSION, (fmt), __VA_ARGS__)
#define LOGF_DEBUG(fmt, ...) DEBUGF(INDI::Logger::DBG_DEBUG, (fmt), __VA_ARGS__)
#define LOGF_EXTRA1(fmt, ...) DEBUGF(INDI::Logger::DBG_EXTRA_1, (fmt), __VA_ARGS__)
#define LOGF_EXTRA2(fmt, ...) DEBUGF(INDI::Logger::DBG_EXTRA_2, (fmt), __VA_ARGS__)
#define LOGF_EXTRA3(fmt, ...) DEBUGF(INDI::Logger::DBG_EXTRA_3, (fmt), __VA_ARGS__)


namespace INDI
{
/**
 * @class INDI::Logger
 * @brief The Logger class is a simple logger to log messages to file and INDI clients. This is the implementation of a simple
 *  logger in C++. It is implemented as a Singleton, so it can be easily called through two DEBUG macros.
 * It is Pthread-safe. It allows to log on both file and screen, and to specify a verbosity threshold for both of them.
 *
 * - By default, the class defines 4 levels of debugging/logging levels:
 *      -# Errors: Use macro DEBUG(INDI::Logger::DBG_ERROR, "My Error Message)
 *
 *      -# Warnings: Use macro DEBUG(INDI::Logger::DBG_WARNING, "My Warning Message)
 *
 *      -# Session: Use macro DEBUG(INDI::Logger::DBG_SESSION, "My Message) Session messages are the regular status messages from the driver.
 *
 *      -# Driver Debug: Use macro DEBUG(INDI::Logger::DBG_DEBUG, "My Driver Debug Message)
 *
 * @note Use DEBUGF macro if you have a variable list message. e.g. DEBUGF(INDI::Logger::DBG_SESSION, "Hello %s!", "There")
 *
 * The default \e active debug levels are Error, Warning, and Session. Driver Debug can be enabled by the client.
 *
 * To add a new debug level, call addDebugLevel(). You can add an additional 4 custom debug/logging levels.
 *
 * Check INDI Tutorial two for an example simple implementation.
 */
class Logger
{
    /** Type used for the configuration */
    enum loggerConf_
    {
        L_nofile_   = 1 << 0,
        L_file_     = 1 << 1,
        L_noscreen_ = 1 << 2,
        L_screen_   = 1 << 3
    };

#ifdef LOGGER_MULTITHREAD
    /// Lock for mutual exclusion between different threads
    static pthread_mutex_t lock_;
#endif

    bool configured_ { false };

    /** Pointer to the unique Logger (i.e., Singleton) */
    static Logger *m_;

    /**
     * @brief Initial part of the name of the file used for Logging.
     * Date and time are automatically appended.
     */
    static std::string logFile_;

    /**
     * @brief Directory where log file is stored. it is created under ~/.indi/logs/[DATE]/[DRIVER_EXEC]
     */
    static std::string logDir_;

    /**
     * @brief Current configuration of the logger.
     * Variable to know if logging on file and on screen are enabled. Note that if the log on
     * file is enabled, it means that the logger has been already configured, therefore the
     * stream is already open.
     */
    static loggerConf_ configuration_;

    /// Stream used when logging on a file
    std::ofstream out_;
    /// Initial time (used to print relative times)
    struct timeval initialTime_;
    /// Verbosity threshold for files
    static unsigned int fileVerbosityLevel_;
    /// Verbosity threshold for screen
    static unsigned int screenVerbosityLevel_;
    static unsigned int rememberscreenlevel_;

    /**
     * @brief Constructor.
     * It is a private constructor, called only by getInstance() and only the
     * first time. It is called inside a lock, so lock inside this method
     * is not required.
     * It only initializes the initial time. All configuration is done inside the
     * configure() method.
     */
    Logger();

    /**
     * @brief Destructor.
     * It only closes the file, if open, and cleans memory.
     */
    ~Logger();

    /** Method to lock in case of multithreading */
    inline static void lock();

    /** Method to unlock in case of multithreading */
    inline static void unlock();

    static INDI::DefaultDevice *parentDevice;

  public:
    enum VerbosityLevel
    {
        DBG_IGNORE  = 0x0,
        DBG_ERROR   = 0x1,
        DBG_WARNING = 0x2,
        DBG_SESSION = 0x4,
        DBG_DEBUG   = 0x8,
        DBG_EXTRA_1 = 0x10,
        DBG_EXTRA_2 = 0X20,
        DBG_EXTRA_3 = 0x40,
        DBG_EXTRA_4 = 0x80
    };

    struct switchinit
    {
        char name[MAXINDINAME];
        char label[MAXINDILABEL];
        ISState state;
        unsigned int levelmask;
    };

    static const unsigned int defaultlevel = DBG_ERROR | DBG_WARNING | DBG_SESSION;
    static const unsigned int nlevels      = 8;
    static struct switchinit LoggingLevelSInit[nlevels];
    static ISwitch LoggingLevelS[nlevels];
    static ISwitchVectorProperty LoggingLevelSP;
    static ISwitch ConfigurationS[2];
    static ISwitchVectorProperty ConfigurationSP;
    typedef loggerConf_ loggerConf;
    static const loggerConf file_on    = L_nofile_;
    static const loggerConf file_off   = L_file_;
    static const loggerConf screen_on  = L_noscreen_;
    static const loggerConf screen_off = L_screen_;
    static unsigned int customLevel;
    static unsigned int nDevices;

    static std::string getLogFile() { return logFile_; }
    static loggerConf_ getConfiguration() { return configuration_; }

    /**
     * @brief Method to get a reference to the object (i.e., Singleton)
     * It is a static method.
     * @return Reference to the object.
     */
    static Logger &getInstance();

    static bool saveConfigItems(FILE *fp);

    /**
     * @brief Adds a new debugging level to the driver.
     *
     * @param debugLevelName The descriptive debug level defined to the client. e.g. Scope Status
     * @param LoggingLevelName the short logging level recorded in the logfile. e.g. SCOPE
     * @return bitmask of the new debugging level to be used for any subsequent calls to DEBUG and DEBUGF to
     * record events to this debug level.
     */
    int addDebugLevel(const char *debugLevelName, const char *LoggingLevelName);

    void print(const char *devicename, const unsigned int verbosityLevel, const std::string &sourceFile,
               const int codeLine,
               //const std::string& 	message,
               const char *message, ...);

    /**
     * @brief Method to configure the logger. Called by the DEBUG_CONF() macro. To make implementation
     * easier, the old stream is always closed.
     * Then, in case, it is open again in append mode.
     * @param outputFile of the file used for logging
     * @param configuration (i.e., log on file and on screen on or off)
     * @param fileVerbosityLevel threshold for file
     * @param screenVerbosityLevel threshold for screen
     */
    void configure(const std::string &outputFile, const loggerConf configuration, const int fileVerbosityLevel,
                   const int screenVerbosityLevel);

    static struct switchinit DebugLevelSInit[nlevels];
    static ISwitch DebugLevelS[nlevels];
    static ISwitchVectorProperty DebugLevelSP;
    static bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    static bool initProperties(INDI::DefaultDevice *device);
    static bool updateProperties(bool enable);
    static char Tags[nlevels][MAXINDINAME];

    /**
     * @brief Method used to print message called by the DEBUG() macro.
     * @param i which debugging to query its rank. The lower the rank, the more priority it is.
     * @return rank of debugging level requested.
     */
    static unsigned int rank(unsigned int l);
};

inline Logger::loggerConf operator|(Logger::loggerConf __a, Logger::loggerConf __b)
{
    return Logger::loggerConf(static_cast<int>(__a) | static_cast<int>(__b));
}

inline Logger::loggerConf operator&(Logger::loggerConf __a, Logger::loggerConf __b)
{
    return Logger::loggerConf(static_cast<int>(__a) & static_cast<int>(__b));
}
}
