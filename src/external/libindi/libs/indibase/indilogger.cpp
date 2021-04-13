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

#include "indilogger.h"

#include <dirent.h>
#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace INDI
{
char Logger::Tags[Logger::nlevels][MAXINDINAME] = { "ERROR",       "WARNING",     "INFO",        "DEBUG",
                                                    "DBG_EXTRA_1", "DBG_EXTRA_2", "DBG_EXTRA_3", "DBG_EXTRA_4" };

struct Logger::switchinit Logger::DebugLevelSInit[] = { { "DBG_ERROR", "Errors", ISS_ON, DBG_ERROR },
                                                        { "DBG_WARNING", "Warnings", ISS_ON, DBG_WARNING },
                                                        { "DBG_SESSION", "Messages", ISS_ON, DBG_SESSION },
                                                        { "DBG_DEBUG", "Driver Debug", ISS_OFF, DBG_DEBUG },
                                                        { "DBG_EXTRA_1", "Debug Extra 1", ISS_OFF, DBG_EXTRA_1 },
                                                        { "DBG_EXTRA_2", "Debug Extra 2", ISS_OFF, DBG_EXTRA_2 },
                                                        { "DBG_EXTRA_3", "Debug Extra 3", ISS_OFF, DBG_EXTRA_3 },
                                                        { "DBG_EXTRA_4", "Debug Extra 4", ISS_OFF, DBG_EXTRA_4 } };

struct Logger::switchinit Logger::LoggingLevelSInit[] = {
    { "LOG_ERROR", "Errors", ISS_ON, DBG_ERROR },           { "LOG_WARNING", "Warnings", ISS_ON, DBG_WARNING },
    { "LOG_SESSION", "Messages", ISS_ON, DBG_SESSION },     { "LOG_DEBUG", "Driver Debug", ISS_OFF, DBG_DEBUG },
    { "LOG_EXTRA_1", "Log Extra 1", ISS_OFF, DBG_EXTRA_1 }, { "LOG_EXTRA_2", "Log Extra 2", ISS_OFF, DBG_EXTRA_2 },
    { "LOG_EXTRA_3", "Log Extra 3", ISS_OFF, DBG_EXTRA_3 }, { "LOG_EXTRA_4", "Log Extra 4", ISS_OFF, DBG_EXTRA_4 }
};

ISwitch Logger::DebugLevelS[Logger::nlevels];
ISwitchVectorProperty Logger::DebugLevelSP;
ISwitch Logger::LoggingLevelS[Logger::nlevels];
ISwitchVectorProperty Logger::LoggingLevelSP;
ISwitch Logger::ConfigurationS[2];
ISwitchVectorProperty Logger::ConfigurationSP;

INDI::DefaultDevice *Logger::parentDevice  = nullptr;
unsigned int Logger::fileVerbosityLevel_   = Logger::defaultlevel;
unsigned int Logger::screenVerbosityLevel_ = Logger::defaultlevel;
unsigned int Logger::rememberscreenlevel_  = Logger::defaultlevel;
Logger::loggerConf Logger::configuration_  = Logger::screen_on | Logger::file_off;
std::string Logger::logDir_;
std::string Logger::logFile_;
unsigned int Logger::nDevices    = 0;
unsigned int Logger::customLevel = 4;

// Create dir recursively
static int _mkdir(const char *dir, mode_t mode)
{
    char tmp[PATH_MAX];
    char *p = nullptr;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(tmp, mode) == -1 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    if (mkdir(tmp, mode) == -1 && errno != EEXIST)
        return -1;

    return 0;
}

int Logger::addDebugLevel(const char *debugLevelName, const char *loggingLevelName)
{
    // Cannot create any more levels
    if (customLevel == nlevels)
        return -1;

    strncpy(Tags[customLevel], loggingLevelName, MAXINDINAME);
    strncpy(DebugLevelSInit[customLevel].label, debugLevelName, MAXINDINAME);
    strncpy(LoggingLevelSInit[customLevel].label, debugLevelName, MAXINDINAME);

    return DebugLevelSInit[customLevel++].levelmask;
}

bool Logger::initProperties(DefaultDevice *device)
{
    nDevices++;

    for (unsigned int i = 0; i < customLevel; i++)
    {
        IUFillSwitch(&DebugLevelS[i], DebugLevelSInit[i].name, DebugLevelSInit[i].label, DebugLevelSInit[i].state);
        DebugLevelS[i].aux = (void *)&DebugLevelSInit[i].levelmask;
        IUFillSwitch(&LoggingLevelS[i], LoggingLevelSInit[i].name, LoggingLevelSInit[i].label,
                     LoggingLevelSInit[i].state);
        LoggingLevelS[i].aux = (void *)&LoggingLevelSInit[i].levelmask;
    }

    IUFillSwitchVector(&DebugLevelSP, DebugLevelS, customLevel, device->getDeviceName(), "DEBUG_LEVEL", "Debug Levels",
                       OPTIONS_TAB, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);
    IUFillSwitchVector(&LoggingLevelSP, LoggingLevelS, customLevel, device->getDeviceName(), "LOGGING_LEVEL",
                       "Logging Levels", OPTIONS_TAB, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);

    IUFillSwitch(&ConfigurationS[0], "CLIENT_DEBUG", "To Client", ISS_ON);
    IUFillSwitch(&ConfigurationS[1], "FILE_DEBUG", "To Log File", ISS_OFF);
    IUFillSwitchVector(&ConfigurationSP, ConfigurationS, 2, device->getDeviceName(), "LOG_OUTPUT", "Log Output",
                       OPTIONS_TAB, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);

    parentDevice = device;

    return true;
}

bool Logger::updateProperties(bool enable)
{
    if (enable)
    {
        parentDevice->defineSwitch(&DebugLevelSP);
        parentDevice->defineSwitch(&LoggingLevelSP);

        screenVerbosityLevel_ = rememberscreenlevel_;

        parentDevice->defineSwitch(&ConfigurationSP);
    }
    else
    {
        parentDevice->deleteProperty(DebugLevelSP.name);
        parentDevice->deleteProperty(LoggingLevelSP.name);
        parentDevice->deleteProperty(ConfigurationSP.name);
        rememberscreenlevel_  = screenVerbosityLevel_;
        screenVerbosityLevel_ = defaultlevel;
    }

    return true;
}

bool Logger::saveConfigItems(FILE *fp)
{
    IUSaveConfigSwitch(fp, &DebugLevelSP);
    IUSaveConfigSwitch(fp, &LoggingLevelSP);
    IUSaveConfigSwitch(fp, &ConfigurationSP);

    return true;
}

bool Logger::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int debug_level = 0, log_level = 0, bitmask = 0, verbose_level = 0;
    if (strcmp(name, "DEBUG_LEVEL") == 0)
    {
        ISwitch *sw;
        IUUpdateSwitch(&DebugLevelSP, states, names, n);
        sw = IUFindOnSwitch(&DebugLevelSP);
        if (sw == nullptr)
        {
            DebugLevelSP.s = IPS_IDLE;
            IDSetSwitch(&DebugLevelSP, nullptr);
            screenVerbosityLevel_ = 0;
            return true;
        }

        for (int i = 0; i < DebugLevelSP.nsp; i++)
        {
            sw      = &DebugLevelSP.sp[i];
            bitmask = *((unsigned int *)sw->aux);
            if (sw->s == ISS_ON)
            {
                debug_level = i;
                verbose_level |= bitmask;
            }
            else
                verbose_level &= ~bitmask;
        }

        screenVerbosityLevel_ = verbose_level;

        DEBUGFDEVICE(dev, Logger::DBG_DEBUG, "Toggle Debug Level -- %s", DebugLevelSInit[debug_level].label);
        DebugLevelSP.s = IPS_OK;
        IDSetSwitch(&DebugLevelSP, nullptr);
        return true;
    }

    if (strcmp(name, "LOGGING_LEVEL") == 0)
    {
        ISwitch *sw;
        IUUpdateSwitch(&LoggingLevelSP, states, names, n);
        sw = IUFindOnSwitch(&LoggingLevelSP);
        if (sw == nullptr)
        {
            fileVerbosityLevel_ = 0;
            LoggingLevelSP.s    = IPS_IDLE;
            IDSetSwitch(&LoggingLevelSP, nullptr);
            return true;
        }

        for (int i = 0; i < LoggingLevelSP.nsp; i++)
        {
            sw      = &LoggingLevelSP.sp[i];
            bitmask = *((unsigned int *)sw->aux);
            if (sw->s == ISS_ON)
            {
                log_level = i;
                fileVerbosityLevel_ |= bitmask;
            }
            else
                fileVerbosityLevel_ &= ~bitmask;
        }

        DEBUGFDEVICE(dev, Logger::DBG_DEBUG, "Toggle Logging Level -- %s", LoggingLevelSInit[log_level].label);
        LoggingLevelSP.s = IPS_OK;
        IDSetSwitch(&LoggingLevelSP, nullptr);
        return true;
    }

    if (!strcmp(name, "LOG_OUTPUT"))
    {
        ISwitch *sw;
        IUUpdateSwitch(&ConfigurationSP, states, names, n);
        sw = IUFindOnSwitch(&ConfigurationSP);

        if (sw == nullptr)
        {
            configuration_    = screen_off | file_off;
            ConfigurationSP.s = IPS_IDLE;
            IDSetSwitch(&ConfigurationSP, nullptr);
            return true;
        }

        bool wasFileOff = configuration_ & file_off;

        configuration_ = (loggerConf)0;

        if (ConfigurationS[1].s == ISS_ON)
            configuration_ = configuration_ | file_on;
        else
            configuration_ = configuration_ | file_off;

        if (ConfigurationS[0].s == ISS_ON)
            configuration_ = configuration_ | screen_on;
        else
            configuration_ = configuration_ | screen_off;

        // If file was off, then on again
        if (wasFileOff && (configuration_ & file_on))
            Logger::getInstance().configure(logFile_, configuration_, fileVerbosityLevel_, screenVerbosityLevel_);

        ConfigurationSP.s = IPS_OK;
        IDSetSwitch(&ConfigurationSP, nullptr);

        return true;
    }

    return false;
}

// Definition (and initialization) of static attributes
Logger *Logger::m_ = nullptr;

#ifdef LOGGER_MULTITHREAD
pthread_mutex_t Logger::lock_ = PTHREAD_MUTEX_INITIALIZER;
inline void Logger::lock()
{
    pthread_mutex_lock(&lock_);
}

inline void Logger::unlock()
{
    pthread_mutex_unlock(&lock_);
}
#else
void Logger::lock()
{
}
void Logger::unlock()
{
}
#endif

Logger::Logger() : configured_(false)
{
    gettimeofday(&initialTime_, nullptr);
}

void Logger::configure(const std::string &outputFile, const loggerConf configuration, const int fileVerbosityLevel,
                       const int screenVerbosityLevel)
{
    Logger::lock();

    fileVerbosityLevel_   = fileVerbosityLevel;
    screenVerbosityLevel_ = screenVerbosityLevel;
    rememberscreenlevel_  = screenVerbosityLevel_;
    // Close the old stream, if needed
    if (configuration_ & file_on)
        out_.close();

    // Compute a new file name, if needed
    if (outputFile != logFile_)
    {
        char ts_date[32], ts_time[32];
        struct tm *tp;
        time_t t;

        time(&t);
        tp = gmtime(&t);
        strftime(ts_date, sizeof(ts_date), "%Y-%m-%d", tp);
        strftime(ts_time, sizeof(ts_time), "%H:%M:%S", tp);

        char dir[MAXRBUF];
        snprintf(dir, MAXRBUF, "%s/.indi/logs/%s/%s", getenv("HOME"), ts_date, outputFile.c_str());
        logDir_ = dir;

        char logFileBuf[MAXRBUF];
        snprintf(logFileBuf, MAXRBUF, "%s/%s_%s.log", dir, outputFile.c_str(), ts_time);
        logFile_ = logFileBuf;
    }

    // Open a new stream, if needed
    if (configuration & file_on)
    {
        _mkdir(logDir_.c_str(), 0775);
        out_.open(logFile_.c_str(), std::ios::app);
    }

    configuration_ = configuration;
    configured_    = true;

    Logger::unlock();
}

Logger::~Logger()
{
    Logger::lock();
    if (configuration_ & file_on)
        out_.close();

    m_ = nullptr;
    Logger::unlock();
}

Logger &Logger::getInstance()
{
    Logger::lock();
    if (m_ == nullptr)
        m_ = new Logger;
    Logger::unlock();
    return *m_;
}

unsigned int Logger::rank(unsigned int l)
{
    switch (l)
    {
        case DBG_ERROR:
            return 0;
        case DBG_WARNING:
            return 1;
        case DBG_SESSION:
            return 2;
        case DBG_EXTRA_1:
            return 4;
        case DBG_EXTRA_2:
            return 5;
        case DBG_EXTRA_3:
            return 6;
        case DBG_EXTRA_4:
            return 7;
        case DBG_DEBUG:
        default:
            return 3;
    }
}

void Logger::print(const char *devicename, const unsigned int verbosityLevel, const std::string &file, const int line,
                   //const std::string& message,
                   const char *message, ...)
{
    // 0 is ignored
    if (verbosityLevel == 0)
        return;

    INDI_UNUSED(file);
    INDI_UNUSED(line);
    bool filelog   = (verbosityLevel & fileVerbosityLevel_) != 0;
    bool screenlog = (verbosityLevel & screenVerbosityLevel_) != 0;

    va_list ap;
    char msg[257];
    char usec[7];

    msg[256] = '\0';
    va_start(ap, message);
    vsnprintf(msg, 257, message, ap);
    va_end(ap);

    if (!configured_)
    {
        //std::cerr << "Warning! Logger not configured!" << std::endl;
        std::cerr << msg << std::endl;
        return;
    }
    struct timeval currentTime, resTime;
    usec[6] = '\0';
    gettimeofday(&currentTime, nullptr);
    timersub(&currentTime, &initialTime_, &resTime);
#if defined(__APPLE__)
    snprintf(usec, 7, "%06d", resTime.tv_usec);
#else
    snprintf(usec, 7, "%06ld", resTime.tv_usec);
#endif
    Logger::lock();

    if ((configuration_ & file_on) && filelog)
    {
        if (nDevices == 1)
            out_ << Tags[rank(verbosityLevel)] << "\t" << (resTime.tv_sec) << "." << (usec) << " sec"
                 << "\t: " << msg << std::endl;
        else
            out_ << Tags[rank(verbosityLevel)] << "\t" << (resTime.tv_sec) << "." << (usec) << " sec"
                 << "\t: [" << devicename << "] " << msg << std::endl;
    }

    if ((configuration_ & screen_on) && screenlog)
        IDMessage(devicename, "[%s] %s", Tags[rank(verbosityLevel)], msg);

    Logger::unlock();
}
}
