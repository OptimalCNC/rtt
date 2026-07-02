/***************************************************************************
  tag: Peter Soetens Mon Oct 23 14:21:16 2006 +0000 Logger.inl

                        Logger.inl -  description
                           -------------------
    begin                : Mon Oct 23 2006
    copyright            : (C) 2006 Peter Soetens
    email                : peter.soetens@fmtc.be

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
 *   General Public License for more details.                              *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef ORO_CORELIB_LOGGER_INL
#define ORO_CORELIB_LOGGER_INL

/**
 * @file Logger.inl
 * Provides empty inlines when no logging is used, which the
 * compiler can optimize out.
 */



namespace RTT
{
#ifdef OROBLD_DISABLE_LOGGING

    // instance will be actually null
    inline Logger& Logger::log() {
        return *_instance;
    }

    inline bool Logger::mayLog() const {
        return false;
    }

    inline void Logger::mayLogStdOut(bool ) {
    }

    inline void Logger::mayLogFile(bool ) {
    }

    inline void Logger::allowRealTime() {
    }

    inline void Logger::disallowRealTime() {
    }

    inline void Logger::startup() {
    }

    inline void Logger::shutdown() {
    }

    inline std::string Logger::getLogLine() {
        return "";
    }

    inline void Logger::logf(LogLevel, const char*, const char*, ...) {
    }

    inline int Logger::drainLog() {
        return 0;
    }

    inline void Logger::setAutoDrain(bool) {
    }

    inline bool Logger::isAutoDrainEnabled() const {
        return false;
    }

    inline std::size_t Logger::droppedLogCount() const {
        return 0;
    }

    inline void Logger::setStdStream( std::ostream& ) {
    }

    inline void Logger::setLogLevel( LogLevel ) {
    }

    inline Logger::LogLevel Logger::getLogLevel() const {
        return Never;
    }
#endif

}

#endif
