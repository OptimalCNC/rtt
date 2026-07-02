/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:15 CET 2005  Logger.cxx

                        Logger.cxx -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public                   *
 *   License as published by the Free Software Foundation;                 *
 *   version 2 of the License.                                             *
 *                                                                         *
 *   As a special exception, you may use this file as part of a free       *
 *   software library without restriction.  Specifically, if other files   *
 *   instantiate templates or use macros or inline functions from this     *
 *   file, or you compile this file and link it with other files to        *
 *   produce an executable, this file does not by itself cause the         *
 *   resulting executable to be covered by the GNU General Public          *
 *   License.  This exception does not however invalidate any other        *
 *   reasons why the executable file might be covered by the GNU General   *
 *   Public License.                                                       *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

// to retrieve RTAI version, if any.
//#define OROBLD_OS_LXRT_INTERNAL
#include "os/StartStopManager.hpp"
#include "os/MutexLock.hpp"
#include "os/Mutex.hpp"
#include "os/TimeService.hpp"

#include "Logger.hpp"
#include <iomanip>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <sstream>
#include <thread>
#include <utility>

#ifdef OROSEM_PRINTF_LOGGING
#  include <stdio.h>
#else
#  include <iostream>
#  include <ostream>
#  ifdef OROSEM_FILE_LOGGING
#   include <fstream>
#  endif
#  ifdef OROSEM_REMOTE_LOGGING
#   include "base/BufferLockFree.hpp"
#  endif
#endif

#ifndef OROBLD_DISABLE_LOGGING
#include <rtlog/rtlog.h>
#endif

#include <stdlib.h>
#include "rtt-config.h"
#include "rtt-fwd.hpp"

namespace RTT
{
    using namespace std;
    using namespace detail;

    Logger* Logger::_instance = 0;

    Logger* Logger::Instance(std::ostream& str) {
        if (_instance == 0) {
            _instance =  new Logger(str);
        }
        return _instance;
    }

    void Logger::Release() {
      if (_instance) {
        _instance->shutdown();
        delete _instance;
        _instance = 0;
      }
    }

#ifndef OROBLD_DISABLE_LOGGING

    namespace {
        struct RtLogData {
            Logger::LogLevel level;
            bool to_stdout;
            bool to_file;
            os::TimeService::ticks timestamp;
            char module[48];
        };

        constexpr std::size_t RtLogQueueSize = 1024;
        constexpr std::size_t RtLogMessageSize = 256;

        std::atomic<std::size_t> rtlogSequenceNumber(0);

        typedef rtlog::Logger<RtLogData, RtLogQueueSize, RtLogMessageSize, rtlogSequenceNumber,
                              rtlog::MultiRealtimeWriterQueueType> RtLogger;

        void copyBounded(char* destination, std::size_t destination_size, const char* source)
        {
            if (destination_size == 0)
                return;

            if (!source)
                source = "";

            std::size_t i = 0;
            for (; i + 1 < destination_size && source[i] != '\0'; ++i)
                destination[i] = source[i];
            destination[i] = '\0';
        }

        const char* showLevelText(Logger::LogLevel ll)
        {
            switch (ll)
                {
                case Logger::Fatal:
                    return "[ FATAL  ]";
                case Logger::Critical:
                    return "[CRITICAL]";
                case Logger::Error:
                    return "[ ERROR  ]";
                case Logger::Warning:
                    return "[ Warning]";
                case Logger::Info:
                    return "[ Info   ]";
                case Logger::Debug:
                    return "[ Debug  ]";
                case Logger::RealTime:
                    return "[RealTime]";
                case Logger::Never:
                    break;
                }
            return "";
        }
    }

    Logger& Logger::log() {
        return *Instance();
    }

    /**
     * This hidden struct stores all data structures required for logging.
     */
    struct Logger::D
    {
    public:
        D(std::ostream& str, char const* logfile_name) :
#ifndef OROSEM_PRINTF_LOGGING
              stdoutput( &str ),
#endif
#ifdef OROSEM_REMOTE_LOGGING
              remotestring(ORONUM_LOGGING_BUFSIZE,std::string(), true),
#endif
#if     defined(OROSEM_FILE_LOGGING)
#if     !defined(OROSEM_PRINTF_LOGGING)
              logfile(logfile_name ? logfile_name : "orocos.log"),
#endif
#endif
              outloglevel(Warning),
              timestamp(0),
              droppedLogMessages(0),
              nextRtLogSequence(rtlogSequenceNumber.load(std::memory_order_relaxed)),
              drainThreadRunning(false),
              started(false), showtime(true), allowRT(false),
              mlogStdOut(true), mlogFile(true)
        {
#if defined(OROSEM_FILE_LOGGING) && defined(OROSEM_PRINTF_LOGGING)
            logfile = fopen(logfile_name ? logfile_name : "orocos.log","w");
#endif
        }

        ~D()
        {
            stopDrainThread();
        }

        bool maylog() const {
            if (!started || (outloglevel == RealTime && allowRT == false))
                return false;
            return true;
        }

        void queueHistory(const std::string& line)
        {
#ifdef OROSEM_REMOTE_LOGGING
            if (!remotestring.Push(line))
                droppedLogMessages.fetch_add(1, std::memory_order_relaxed);
#else
            (void)line;
#endif
        }

        void enqueueRtLine(LogLevel level, bool to_stdout, bool to_file, const char* module, const char* message)
        {
            RtLogData data;
            data.level = level;
            data.to_stdout = to_stdout;
            data.to_file = to_file;
            data.timestamp = TimeService::Instance()->getTicks();
            copyBounded(data.module, sizeof(data.module), module);

            rtlog::Status status = rtlogger.Log(std::move(data), "%s", message ? message : "");
            if (status == rtlog::Status::Error_QueueFull)
                droppedLogMessages.fetch_add(1, std::memory_order_relaxed);
        }

        int drainRtLog()
        {
            os::MutexLock lock(drainGuard);
            int processed = rtlogger.PrintAndClearLogQueue([this](
                const RtLogData& data, std::size_t sequence, const char* format, ...) {
                if (sequence > nextRtLogSequence)
                    droppedLogMessages.fetch_add(sequence - nextRtLogSequence, std::memory_order_relaxed);
                if (sequence >= nextRtLogSequence)
                    nextRtLogSequence = sequence + 1;

                char message[RtLogMessageSize];
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                va_end(args);

                std::stringstream line;
                if (showtime)
                    line << fixed << showpoint << setprecision(3)
                         << Seconds(TimeService::ticks2nsecs(data.timestamp - timestamp)) / NSECS_IN_SECS;
                line << " " << showLevelText(data.level) << "[" << data.module << "] "
                     << message;
                writeLine(data, line.str());
            });
            return processed;
        }

        void startDrainThread()
        {
            if (drainThreadRunning.exchange(true, std::memory_order_acq_rel))
                return;

            drainThread = std::thread([this] {
                while (drainThreadRunning.load(std::memory_order_acquire)) {
                    if (drainRtLog() == 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                drainRtLog();
            });
        }

        void stopDrainThread()
        {
            if (drainThreadRunning.exchange(false, std::memory_order_acq_rel)) {
                if (drainThread.joinable())
                    drainThread.join();
            }
            drainRtLog();
        }

        bool autoDrainEnabled() const
        {
            return drainThreadRunning.load(std::memory_order_acquire);
        }

        void writeLine(const RtLogData& data, const std::string& line)
        {
            if (!started)
                return;

            if (data.to_stdout) {
#ifndef OROSEM_PRINTF_LOGGING
                *stdoutput << line << std::endl;
#else
                printf("%s\n", line.c_str());
#endif
            }

            if (data.to_file) {
#ifdef OROSEM_FILE_LOGGING
#if     !defined(OROSEM_PRINTF_LOGGING)
                logfile << line << std::endl;
#else
                fprintf(logfile, "%s\n", line.c_str());
#endif
#endif
                queueHistory(line);
            }
        }

#ifndef OROSEM_PRINTF_LOGGING
        std::ostream* stdoutput;
#endif
        RtLogger rtlogger;
#if defined(OROSEM_REMOTE_LOGGING)
        base::BufferLockFree<std::string> remotestring;
#endif
#if defined(OROSEM_FILE_LOGGING)
# ifndef OROSEM_PRINTF_LOGGING
        std::ofstream logfile;
# else
        FILE* logfile;
# endif
#endif
        LogLevel outloglevel;

        TimeService::ticks timestamp;
        std::atomic<std::size_t> droppedLogMessages;
        std::size_t nextRtLogSequence;
        std::atomic<bool> drainThreadRunning;
        std::thread drainThread;

        Logger::LogLevel intToLogLevel(int ll) {
            switch (ll)
                {
                case -1:
                case 0:
                    return Never;
                case 1:
                    return Fatal;
                case 2:
                    return Critical;
                case 3:
                    return Error;
                case 4:
                    return Warning;
                case 5:
                    return Info;
                case 6:
                    return Debug;
                }
            return Debug; // > 6
        }


        std::string showTime() const
        {
            std::stringstream time;
            if ( showtime )
                time <<fixed<< showpoint << setprecision(3) << TimeService::Instance()->secondsSince(timestamp);
            return time.str();
        }

        /**
         * Convert a loglevel to a string representation.
         */
        std::string showLevel( LogLevel ll) const {
            std::string prefix;
            switch (ll)
                {
                case Fatal:
                    prefix="[ FATAL  ]";
                    break;
                case Critical:
                    prefix="[CRITICAL]";
                    break;
                case Error:
                    prefix="[ ERROR  ]";
                    break;
                case Warning:
                    prefix="[ Warning]";
                    break;
                case Info:
                    prefix="[ Info   ]";
                    break;
                case Debug:
                    prefix="[ Debug  ]";
                    break;
                case RealTime:
                    prefix="[RealTime]";
                    break;
                case Never:
                    break;
                }
            return prefix;
        }



        bool started;

        bool showtime;

        bool allowRT;

        bool mlogStdOut, mlogFile;

        os::Mutex inpguard;
        os::Mutex drainGuard;
    };

    Logger::Logger(std::ostream& str)
        :d ( new Logger::D(str, getenv("ORO_LOGFILE")) )
    {
      this->startup();
    }

    Logger::~Logger()
    {
        delete d;
    }

    bool Logger::mayLog() const {
        return d->maylog();
    }

    void Logger::mayLogStdOut(bool tf) {
        d->mlogStdOut = tf;
    }

    void Logger::mayLogFile(bool tf) {
        d->mlogFile = tf;
    }

    void Logger::allowRealTime() {
        // re-enable and then log, otherwise you might not get the log event!
        d->allowRT = true;
        this->logf(Logger::Warning, "Logger", "Enabling Real-Time Logging !");
    }
    void Logger::disallowRealTime() {
        this->logf(Logger::Warning, "Logger", "Disabling Real-Time Logging !");
        d->allowRT = false;
    }

    TimeService::ticks Logger::getReferenceTime()const
    {
        return d->timestamp;
    }

#define ORO_xstr(s) ORO_str(s)
#define ORO_str(s) #s

    void Logger::startup() {
        if (d->started)
            return;
#ifndef OROBLD_DISABLE_LOGGING
        std::string xtramsg = "No ORO_LOGLEVEL environment variable set.";
        LogLevel xtramsg_level = Logger::Info;

        int wantedlevel=4; // default log level is 4.

        if ( getenv( "ORO_LOGLEVEL" ) != 0 ) {
            std::stringstream conv;
            conv.str( std::string( getenv( "ORO_LOGLEVEL" ) ) );
            conv >> wantedlevel;
            if ( conv.fail() ) {
                xtramsg = std::string( "Failed to extract loglevel from environment variable ORO_LOGLEVEL.")
                    + " It contained the string '"+conv.str()+"', while it should contain an integer value.";
                xtramsg_level = Logger::Error;
            }
            else {
                d->outloglevel = d->intToLogLevel(wantedlevel);
                xtramsg = "Successfully extracted environment variable ORO_LOGLEVEL";
            }
        }

        // Completely disable logging on negative values.
        if ( wantedlevel < 0 )
            return;
        d->started = true;

        d->timestamp = TimeService::Instance()->getTicks();
        d->startDrainThread();
        this->logf(xtramsg_level, "Logger", "%s", xtramsg.c_str());
        std::string version = "OROCOS version '" ORO_xstr(RTT_VERSION) "'";
#ifdef __GNUC__
        version += " compiled with GCC " ORO_xstr(__GNUC__) "." ORO_xstr(__GNUC_MINOR__) "." ORO_xstr(__GNUC_PATCHLEVEL__) ".";
#endif
        this->logf(Logger::Info, "Logger", "%s", version.c_str());
#ifdef OROPKG_OS_LXRT
        this->logf(Logger::Info, "Logger", "Running in LXRT/RTAI.");
#endif
#ifdef OROPKG_OS_GNULINUX
        this->logf(Logger::Info, "Logger", "Running in GNU/Linux.");
#endif
#ifdef OROPKG_OS_XENOMAI
        this->logf(Logger::Info, "Logger", "Running in Xenomai.");
#endif
        this->logf(Logger::Info, "Logger", "Orocos Logging Activated at level : %s ( %d )",
                   d->showLevel( d->outloglevel ).c_str(), int(d->outloglevel));
        std::stringstream reference_time;
        reference_time << "Reference System Time is : " << d->timestamp << " ticks ( "
                       << Seconds(TimeService::ticks2nsecs(d->timestamp))/NSECS_IN_SECS << " seconds ).";
        this->logf(Logger::Info, "Logger", "%s", reference_time.str().c_str());
        this->logf(Logger::Info, "Logger", "Logging is relative to this time.");
#endif
    }

    void Logger::shutdown() {
        if (!d->started)
            return;
        this->logf(Logger::Info, "Logger", "Orocos Logging Deactivated.");
        d->stopDrainThread();
        d->started = false;
    }

    std::string Logger::getLogLine() {
#ifdef OROSEM_REMOTE_LOGGING
        if (!d->started)
            return "";

        d->drainRtLog();

        os::MutexLock lock( d->inpguard );
        std::string line;
        if(d->remotestring.Pop(line))
            return line;
        else
            return "";
#else
        return "";
#endif
    }

    void Logger::logf(LogLevel ll, const char* module, const char* format, ...)
    {
        if (!d->maylog())
            return;

        RtLogData data;
        data.level = ll;
        data.to_stdout = (ll <= d->outloglevel && d->outloglevel != Never && ll != Never && d->mlogStdOut);
        data.to_file = ((ll <= Logger::Info || ll <= d->outloglevel) && d->mlogFile);
        data.timestamp = TimeService::Instance()->getTicks();
        copyBounded(data.module, sizeof(data.module), module ? module : "Logger");

        if (!data.to_stdout && !data.to_file)
            return;

        va_list args;
        va_start(args, format);
        rtlog::Status status = d->rtlogger.Logv(std::move(data), format ? format : "", args);
        va_end(args);

        if (status == rtlog::Status::Error_QueueFull)
            d->droppedLogMessages.fetch_add(1, std::memory_order_relaxed);
    }

    int Logger::drainLog()
    {
        return d->drainRtLog();
    }

    void Logger::setAutoDrain(bool enabled)
    {
        if (enabled) {
            if (d->started)
                d->startDrainThread();
        } else {
            d->stopDrainThread();
        }
    }

    bool Logger::isAutoDrainEnabled() const
    {
        return d->autoDrainEnabled();
    }

    std::size_t Logger::droppedLogCount() const
    {
        return d->droppedLogMessages.load(std::memory_order_relaxed);
    }

    void Logger::setStdStream( std::ostream& stdos ) {
#ifndef OROSEM_PRINTF_LOGGING
        os::MutexLock lock( d->drainGuard );
        d->stdoutput = &stdos;
#endif
    }

    void Logger::setLogLevel( LogLevel ll ) {
        d->outloglevel = ll;
    }

    Logger::LogLevel Logger::getLogLevel() const {
        return d->outloglevel ;
    }


#else // OROBLD_DISABLE_LOGGING

    Logger::Logger(std::ostream& )
        : d(0)
    {
    }

    Logger::~Logger()
    {
    }

#endif
}
