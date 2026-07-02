/***************************************************************************
  tag: Peter Soetens  Mon Jan 10 15:59:51 CET 2005  logger_test.cpp

                        logger_test.cpp -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include "unit.hpp"
#include "logger_test.hpp"

#include <iostream>
#include <sstream>
#include <boost/scoped_ptr.hpp>
#include <Activity.hpp>
#include <base/RunnableInterface.hpp>

using namespace boost;
using namespace std;
using namespace base;
using namespace RTT;

class Dummy {};

#define QS 10

void
LoggerTest::setUp()
{
    logger = Logger::Instance();
}


void
LoggerTest::tearDown()
{
}

struct TestLog
  : public RunnableInterface
{
  bool fini;
  bool initialize() { fini = false; return true; }

  void step() {
      Logger::log().logf(Logger::Info, "TLOG",
                         "Hello this is the world speaking elaborately and lengthy...!");
  }

  void finalize() {
    fini = true;
  }
};


BOOST_FIXTURE_TEST_SUITE( LoggerTestSuite, LoggerTest )

BOOST_AUTO_TEST_CASE( testStartStop )
{
    BOOST_CHECK( logger != 0 );
    BOOST_CHECK( &Logger::log() != 0 );
}

BOOST_AUTO_TEST_CASE( testLogEnv )
{
    Logger::log().logf(Logger::Debug, "LoggerTest",
                       "Debug Level set + text");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : Single line");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : Two lines on one line.");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : Two");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "lines on two lines.");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : nl");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : flush and std::endl.");
}

BOOST_AUTO_TEST_CASE( testNewLog )
{
    Logger::log().logf(Logger::Debug, "LoggerTest",
                       "Debug Level set + text");
    Logger::log().logf(Logger::Debug, "LoggerTest",
                       "Test Log Environment variable : Single line");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : Two lines on one line.");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : Two");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "lines on two lines.");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : nl");
    Logger::log().logf(Logger::Info, "LoggerTest",
                       "Test Log Environment variable : flush and std::endl.");
}

BOOST_AUTO_TEST_CASE( testRealtimeFormatLogDrainsToHistory )
{
    Logger::LogLevel old_level = logger->getLogLevel();
    logger->setLogLevel(Logger::Debug);
    logger->mayLogStdOut(false);
    logger->mayLogFile(true);

    logger->logf(Logger::Info, "RTLOG_TEST", "bounded format message %d", 42);
    logger->drainLog();

    bool found = false;
    for (int i = 0; i != 200; ++i) {
        std::string line = logger->getLogLine();
        if (line.empty()) {
            break;
        }
        if (line.find("RTLOG_TEST") != std::string::npos &&
            line.find("bounded format message 42") != std::string::npos) {
            found = true;
            break;
        }
    }

    logger->mayLogStdOut(true);
    logger->setLogLevel(old_level);
    BOOST_CHECK(found);
}

BOOST_AUTO_TEST_CASE( testRealtimeFormatLogCountsDroppedMessages )
{
    Logger::LogLevel old_level = logger->getLogLevel();
    logger->setLogLevel(Logger::Debug);
    logger->mayLogStdOut(false);
    logger->mayLogFile(true);

    const std::size_t before = logger->droppedLogCount();
    for (int i = 0; i != 2000; ++i) {
        logger->logf(Logger::Info, "RTLOG_DROP_TEST", "message %d", i);
    }
    logger->drainLog();
    const std::size_t after = logger->droppedLogCount();

    logger->mayLogStdOut(true);
    logger->setLogLevel(old_level);
    BOOST_CHECK(after > before);
}

BOOST_AUTO_TEST_CASE( testRealtimeFormatLogHonorsNeverAtEnqueue )
{
    Logger::LogLevel old_level = logger->getLogLevel();
    logger->setLogLevel(Logger::Debug);
    logger->mayLogStdOut(true);
    logger->mayLogFile(false);

    std::ostringstream output;
    logger->setStdStream(output);
    const std::string marker = "RTLOG_NEVER_FILTER_TEST";
    logger->setLogLevel(Logger::Never);
    logger->logf(Logger::Critical, "RTLOG_TEST", "%s", marker.c_str());
    logger->setLogLevel(Logger::Debug);
    logger->drainLog();

    logger->setStdStream(std::cerr);
    logger->mayLogFile(true);
    logger->mayLogStdOut(true);
    logger->setLogLevel(old_level);
    BOOST_CHECK(output.str().find(marker) == std::string::npos);
}

BOOST_AUTO_TEST_CASE( testLegacyStreamLogUsesBoundedBackend )
{
    Logger::LogLevel old_level = logger->getLogLevel();
    logger->setLogLevel(Logger::Debug);
    logger->mayLogStdOut(false);
    logger->mayLogFile(true);

    for (int i = 0; i != 2500; ++i) {
        if (logger->getLogLine().empty()) {
            break;
        }
    }

    const std::string marker = "RTLOG_STREAM_BOUND_TEST";
    const std::string tail_marker = "RTLOG_STREAM_TAIL_MARKER";
    const std::string payload = marker + " " + std::string(400, 'x') + tail_marker;

    Logger::log(Logger::Info) << payload << Logger::endl;

    bool found = false;
    bool tail_found = false;
    for (int i = 0; i != 2500; ++i) {
        std::string line = logger->getLogLine();
        if (line.empty()) {
            break;
        }
        if (line.find(marker) != std::string::npos) {
            found = true;
            tail_found = line.find(tail_marker) != std::string::npos;
            break;
        }
    }

    logger->mayLogFile(true);
    logger->mayLogStdOut(true);
    logger->setLogLevel(old_level);
    BOOST_CHECK(found);
    BOOST_CHECK(!tail_found);
}

BOOST_AUTO_TEST_CASE( testThreadLog )
{
  boost::scoped_ptr<TestLog> run( new TestLog() );
  boost::scoped_ptr<ActivityInterface> t( new Activity(25, 0.001, 0, "ORActivity1") );
  boost::scoped_ptr<TestLog> run2( new TestLog() );
  boost::scoped_ptr<ActivityInterface> t2( new Activity(25, 0.001, 0, "ORActivity2") );

  t->run( run.get() );
  t2->run( run2.get() );

  t->start();
  t2->start();
  sleep(1);
  t->stop();
  t2->stop();

}

BOOST_AUTO_TEST_SUITE_END()
