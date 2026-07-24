/***************************************************************************
  tag: Peter Soetens  Mon Jun 26 13:25:57 CEST 2006  StatementProcessor.cxx

                        StatementProcessor.cxx -  description
                           -------------------
    begin                : Mon June 26 2006
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
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/



#include "StatementProcessor.hpp"
#include "Parser.hpp"
#include "parse_exception.hpp"

#include "../TaskContext.hpp"
#include "../types/TypeStream.hpp"
#include "../Logger.hpp"

#include <vector>
#include <boost/tuple/tuple.hpp>
#include <iostream>
#include <sstream>


using namespace boost;

namespace RTT
{ namespace scripting {

    using namespace detail;
    struct StatementProcessor::D
    {
	public:
        TaskContext* tc;
        D() {}

        void printResult( DataSourceBase* ds, bool recurse)
        {
            const std::string result = doPrint(ds, recurse);
            Logger::log().logf(Logger::Info, "StatementProcessor", " = %s", result.c_str());
        }

        std::string doPrint( DataSourceBase* ds, bool recurse) {
            // this is needed for ds's that rely on initialision.
            // e.g. eval true once or time measurements.
            // becomes only really handy for 'watches' (todo).
            ds->reset();
            /**
             * If this list of types gets to long, we can still add a virtual
             * printOut( std::ostream& ) = 0 to DataSourceBase.
             */
            // this method can print some primitive DataSource<>'s.
            DataSource<bool>* dsb = DataSource<bool>::narrow(ds);
            if (dsb) {
                std::ostringstream out;
                out << dsb->get();
                return out.str();
            }
            DataSource<int>* dsi = DataSource<int>::narrow(ds);
            if (dsi) {
                std::ostringstream out;
                out << dsi->get();
                return out.str();
            }
#if 0
            // does not work yet with CORBA layer.
            DataSource<long>* dsl = DataSource<long>::narrow(ds);
            if (dsl) {
                std::ostringstream out;
                out << dsl->get();
                return out.str();
            }
#endif
            DataSource<unsigned int>* dsui = DataSource<unsigned int>::narrow(ds);
            if (dsui) {
                std::ostringstream out;
                out << dsui->get();
                return out.str();
            }
            DataSource<std::string>* dss = DataSource<std::string>::narrow(ds);
            if (dss) {
                return "\"" + dss->get() + "\"";
            }
#if 0
            DataSource<std::vector<double> >* dsvval = DataSource< std::vector<double> >::narrow(ds);
            if (dsvval) {
                std::ostringstream out;
                out << dsvval->get();
                return out.str();
            }
            DataSource< Double6D >* ds6d = DataSource<Double6D>::narrow(ds);
            if (ds6d) {
                std::ostringstream out;
                out << ds6d->get();
                return out.str();
            }
#endif
            DataSource<double>* dsd = DataSource<double>::narrow(ds);
            if (dsd) {
                std::ostringstream out;
                out << dsd->get();
                return out.str();
            }
            DataSource<char>* dsc = DataSource<char>::narrow(ds);
            if (dsc) {
                std::string out("'");
                out += dsc->get();
                out += "'";
                return out;
            }

            DataSource<PropertyBag>* dspbag = DataSource<PropertyBag>::narrow(ds);
            if (dspbag) {
                PropertyBag bag( dspbag->get() );
                if (!recurse) {
                    int siz = bag.getProperties().size();
                    std::ostringstream out;
                    out << siz << " Properties";
                    return out.str();
                } else {
                    if ( ! bag.empty() ) {
                        std::string out("\n");
                        for( PropertyBag::iterator it= bag.getProperties().begin(); it!=bag.getProperties().end(); ++it) {
                            if (out.size() != 1)
                                out += "\n";
                            out += (*it)->getType() + " " + (*it)->getName();
                            DataSourceBase::shared_ptr propds = (*it)->getDataSource();
                            out += " = " + this->doPrint(propds.get(), false);
                            out += " (" + (*it)->getDescription() + ")";
                        }
                        return out;
                    } else {
                        return "(empty PropertyBag)";
                    }
                }
            }

            // Leave void  as last since any DS is convertible to void !
            DataSource<void>* dsvd = DataSource<void>::narrow(ds);
            if (dsvd) {
                dsvd->get();
                return "(void)";
            }

            if (ds) {
                ds->evaluate();
                return "( result type '" + ds->getType() + "' not known to TaskBrowser )";
            }
            return "(null)";

        }

    };


    StatementProcessor::StatementProcessor(TaskContext* tc)
        : d ( new D() )
    {
        d->tc = tc;
    }

    StatementProcessor::~StatementProcessor() {
        delete d;
    }

    int StatementProcessor::execute(const std::string& comm)
    {
        TaskContext* taskcontext = d->tc;

        // Minor hack : also check if it was an attribute of current TC, for example,
        // if both the object and attribute with that name exist. the if
        // statement after this one would return and not give the expr parser
        // time to evaluate 'comm'.
        if ( taskcontext->provides()->getValue( comm ) ) {
                d->printResult( taskcontext->provides()->getValue( comm )->getDataSource().get(), true );
                return 0;
        }

        Parser _parser;

        Logger::log().logf(Logger::Debug, "StatementProcessor", "Trying ValueChange...");
        try {
            // Check if it was a method or datasource :
            DataSourceBase::shared_ptr ds = _parser.parseValueChange( comm, taskcontext );
            // methods and DS'es are processed immediately.
            if ( ds.get() != 0 ) {
                Logger::log().logf(Logger::Debug, "StatementProcessor", "ok");
                d->printResult( ds.get(), false );
                return 0; // done here
            } else
                Logger::log().logf(Logger::Debug, "StatementProcessor", "no");
        } catch ( fatal_semantic_parse_exception& pe ) { // incorr args, ...
            // way to fatal,  must be reported immediately
            Logger::log().logf(Logger::Error, "StatementProcessor",
                                "fatal_semantic_parse_exception: %s", pe.what().c_str());
            return -1;
        } catch ( syntactic_parse_exception& pe ) { // wrong content after = sign etc..
            // syntactic errors must be reported immediately
            Logger::log().logf(Logger::Error, "StatementProcessor",
                                "syntactic_parse_exception: %s", pe.what().c_str());
            return -1;
        } catch ( parse_exception_parser_fail &pe )
            {
                // ignore, try next parser
                Logger::log().logf(Logger::Debug, "StatementProcessor",
                                    "Ignoring ValueChange exception :\n%s", pe.what().c_str());
        } catch ( parse_exception& pe ) {
            // syntactic errors must be reported immediately
            Logger::log().logf(Logger::Error, "StatementProcessor",
                                "parse_exception :%s", pe.what().c_str());
            return -1;
        }
        Logger::log().logf(Logger::Debug, "StatementProcessor", "Trying Expression...");
        try {
            // Check if it was a method or datasource :
            DataSourceBase::shared_ptr ds = _parser.parseExpression( comm, taskcontext );
            // methods and DS'es are processed immediately.
            if ( ds.get() != 0 ) {
                d->printResult( ds.get(), true );
                return 0; // done here
            } else
                Logger::log().logf(Logger::Error, "StatementProcessor", "returned zero !");
        } catch ( syntactic_parse_exception& pe ) { // missing brace etc
            // syntactic errors must be reported immediately
            Logger::log().logf(Logger::Error, "StatementProcessor",
                                "syntactic_parse_exception :%s", pe.what().c_str());
            return -1;
        } catch ( fatal_semantic_parse_exception& pe ) { // incorr args, ...
            // way to fatal,  must be reported immediately
            Logger::log().logf(Logger::Error, "StatementProcessor",
                                "fatal_semantic_parse_exception :%s", pe.what().c_str());
            return -1;
        } catch ( parse_exception_parser_fail &pe ) {
                // ignore, try next parser
                Logger::log().logf(Logger::Debug, "StatementProcessor",
                                    "Ignoring Expression exception :\n%s", pe.what().c_str());
        } catch ( parse_exception& pe ) {
            // ignore, try next parser
            Logger::log().logf(Logger::Debug, "StatementProcessor",
                                "Ignoring Expression parse_exception :\n%s", pe.what().c_str());
        }
        return -1;
    }

}}
