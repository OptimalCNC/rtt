/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:16 CET 2005  Logger.hpp

                        Logger.hpp -  description
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


#ifndef ORO_CORELIB_lOGGER_HPP
#define ORO_CORELIB_lOGGER_HPP

#include "rtt-config.h"
#ifndef OROBLD_DISABLE_LOGGING
#include <ostream>
#else
#include <iosfwd>
#endif
#include <string>
#include <cstddef>
#ifndef OROSEM_PRINTF_LOGGING
#include <iostream>            // for std::cerr
#endif

#include "os/TimeService.hpp"

namespace RTT
{
    /**
     * A simple logging class to debug/ analyse what is
     * going on in the Orocos system.
     *
     * You can disable all logging at compile time by
     * defining \a OROBLD_DISABLE_LOGGING (not advised for normal usage).
     * This class can log to a console, and/or to a file and/or to an
     * internal buffer which may be emptied by another class. This
     * is decided upon compile time and can not be changed during runtime.
     * Both printf/iostream drains are supported.
     *
     * Example Usage :
     * @verbatim
     Logger::log().logf(Logger::Error, "MyModule", "An error occurred: %d", 333);
     Logger::log().logf(Logger::Debug, "MyModule", "All debug info ...");
     * @endverbatim
     *
     * When the application is started, set the displayed loglevel with
     * setLogLevel() with a LogLevel parameter. The default is Warning. Set the desired log streams
     * ( a file or std output ) with logToStream() and setStdStream(). Additionally,
     * an \a orocos.log  which is always logs
     * at log level 'Info'.
     *
     * If you set an environment variable \a ORO_LOGLEVEL=0..6, this value will be used
     * to determine the output level until overriden by the application (if so).
     * The \a ORO_LOGLEVEL has the same effect on the 'orocos.log' file, but can not lower it below "Info".
     *
     * @warning
     * logf() is the bounded real-time logging entry point. Draining,
     * configuration, and output stream/file work remain non-real-time work.
     * @ingroup CoreLib
     */
    class RTT_API Logger
    {
        struct D;
        D* d;
    public:

        /**
         * Function to get the loggers starting timestamp
         */
        os::TimeService::ticks getReferenceTime()const;

        /**
         * Enumerate all log-levels from absolute silence to
         * everything.
         * @warning If you enable 'RealTime' logging, this may break realtime performance. Use With Care and NOT
         * on production systems.
         * @see allowRealTime()
         */
        enum LogLevel { Never = 0, Fatal, Critical, Error, Warning, Info, Debug, RealTime };

        /**
         * Allow messages of the LogLevel 'RealTime' to appear on the console.
         */
        void allowRealTime();

        /**
         * Disallow messages of the LogLevel 'RealTime' to appear on the console.
         */
        void disallowRealTime();

        /**
         * Toggles the flag if the logger may log to the
         * standard output stream.
         */
        void mayLogStdOut(bool tf);

        /**
         * Toggles the flag if the logger may log to the
         * file stream.
         */
        void mayLogFile(bool tf);

        /**
         * Get the singleton logger.
         * \post If the singleton did not already exist then it is created
         * and associated with the given ostream. If the singleton already
         * existed then no change occurs (and the singleton remains associated
         * with its existing ostream).
         */
        static Logger* Instance(std::ostream& str=std::cerr);

        /**
         * Delete the singleton logger.
         */
        static void Release();

        /**
         * As Instance(), but more userfriendly.
         */
        static Logger& log();

        /**
         * Print a 'welcome' string in Info  and reset log timestamp.
         */
        void startup();

        /**
         * Print a 'goodbye' string in Info, Flush all streams and stop Logging.
         */
        void shutdown();

        /**
         * This method gets all messages upto level Info
         * and can be freely read by the user, removing the line
         * from an internal buffer of Logger.
         */
        std::string getLogLine();

        /**
         * Enqueue one bounded printf-style log message for non-real-time
         * draining. This is the real-time-safe logging surface for code that
         * can format through fixed-size printf arguments.
         */
        void logf(LogLevel ll, const char* module, const char* format, ...);

        /**
         * Drain queued log messages to configured sinks and history.
         */
        int drainLog();

        /**
         * Enable or disable the background drain thread.
         *
         * Interactive frontends may disable automatic draining while they own
         * terminal input and call drainLog() at prompt-safe points instead.
         */
        void setAutoDrain(bool enabled);

        /**
         * Returns true when the background drain thread is enabled.
         */
        bool isAutoDrainEnabled() const;

        /**
         * Return the number of log messages dropped by the bounded queue.
         */
        std::size_t droppedLogCount() const;

        /**
         * Set the standard output stream. (default is cerr).
         */
        void setStdStream( std::ostream& stdos  );

        /**
         * Set the loglevel of the outgoing (streamed) messages.
         * All messages with this level or higher importance will be displayed.
         * For example, setting to \a Logger::Debug will print everyting,
         * setting to \a Logger::Critical will only print critical or fatal
         * errors.
         */
        void setLogLevel( LogLevel ll );

        /**
         * Return the current output loglevel.
         */
        LogLevel getLogLevel() const;

    private:
        /**
         * Returns true if the next message will be logged.
         * Returns false if the LogLevel is RealTime and
         * allowRealTime() was not called or if the logger
         * was not started.
         */
        bool mayLog() const;

        Logger(std::ostream& str=std::cerr);
        ~Logger();

        static Logger* _instance;
    };
}

#include "Logger.inl"

#endif
