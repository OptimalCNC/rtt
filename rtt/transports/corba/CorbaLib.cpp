/***************************************************************************
  tag: FMTC  Tue Mar 11 21:49:22 CET 2008  CorbaLib.cpp

                        CorbaLib.cpp -  description
                           -------------------
    begin                : Tue March 11 2008
    copyright            : (C) 2008 FMTC
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


#include "corba.h"
#ifdef CORBA_IS_TAO
#include "corba.h"
#include <tao/PortableServer/PortableServer.h>
#else
#include <omniORB4/CORBA.h>
#include <omniORB4/poa.h>
#endif

#include "TransportPlugin.hpp"
#include "CorbaTemplateProtocol.hpp"
#include "RTTCorbaConversion.hpp"
#include "../../types/TransportPlugin.hpp"
#include "../../types/TypekitPlugin.hpp"
#include <cstdint>
#ifdef OS_RT_MALLOC
#include "../../rt_string.hpp"
#endif

using namespace std;
using namespace RTT::detail;

namespace RTT {
    namespace corba {

        /**
         * This protocol is used for all types which did not get a protocol.
         * Specifically, if the type is UnknownType.
         */
        class CorbaFallBackProtocol
            : public CorbaTypeTransporter
        {
            bool warn;
        public:
            CorbaFallBackProtocol(bool do_warn = true) : warn(do_warn) {}
            virtual CORBA::Any* createAny(DataSourceBase::shared_ptr source) const
            {
                if (warn) {
                    Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                       "Could not send data of type '%s' : data type not known to CORBA Transport.",
                                       source->getTypeName().c_str());
                }
                source->evaluate();
                return new CORBA::Any();
            }

            virtual bool updateAny( base::DataSourceBase::shared_ptr source, CORBA::Any& any) const
            {
                if (warn) {
                    Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                       "Could not send data of type '%s' : data type not known to CORBA Transport.",
                                       source->getTypeName().c_str());
                }
                source->evaluate();
                return false;
            }

            virtual base::DataSourceBase::shared_ptr createDataSource(const CORBA::Any* any) const
            {
                return base::DataSourceBase::shared_ptr();
            }

            /**
             * Update \a target with the contents of \a blob which is an object of a \a protocol.
             */
            virtual bool updateFromAny(const CORBA::Any* blob, DataSourceBase::shared_ptr target) const
            {
                if (warn) {
                    Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                       "Could not update type '%s' with received data : data type not known to CORBA Transport.",
                                       target->getTypeName().c_str());
                }
                return false;
            }

            virtual ChannelElementBase::shared_ptr createStream(base::PortInterface* port, const ConnPolicy& policy, bool is_sender) const {
                Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                   "Could create Stream for port '%s' : data type not known to CORBA Transport.",
                                   port->getName().c_str());
                return ChannelElementBase::shared_ptr();
            }

            virtual base::ChannelElementBase* buildDataStorage(ConnPolicy const& policy) const { return 0; }

            virtual CRemoteChannelElement_i* createChannelElement_i(DataFlowInterface*, ::PortableServer::POA* poa, const ConnPolicy &) const {
                Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                   "Could create Channel : data type not known to CORBA Transport.");
                return 0;
            }

            virtual base::ChannelElementBase* buildChannelOutput(base::InputPortInterface& port,
                ConnPolicy const& policy) const {
                Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                   "Could create outputHalf for port %s: data type not known to CORBA Transport.",
                                   port.getName().c_str());
                return 0;
            }

            virtual base::ChannelElementBase* buildChannelInput(base::OutputPortInterface& port,
                ConnPolicy const& policy) const {
                Logger::log().logf(Logger::Error, "CorbaFallBackProtocol",
                                   "Could create outputHalf for port %s: data type not known to CORBA Transport.",
                                   port.getName().c_str());
                return 0;
            }
          virtual base::DataSourceBase::shared_ptr createPropertyDataSource(CService_ptr serv, const std::string& vname) {
              CORBA::String_var tname = serv->getPropertyTypeName( CORBA::string_dup(vname.c_str()));
              Logger::log().logf(Logger::Warning, "CorbaFallBackProtocol",
                                 "Corba: Remote property '%s' has unknown type %s",
                                 vname.c_str(), tname.in());
              return base::DataSourceBase::shared_ptr( );
          }

          virtual base::DataSourceBase::shared_ptr createAttributeDataSource(CService_ptr serv, const std::string& vname, bool) {
              CORBA::String_var tname = serv->getAttributeTypeName( CORBA::string_dup( vname.c_str()));
              Logger::log().logf(Logger::Warning, "CorbaFallBackProtocol",
                                 "Corba: Remote attribute '%s' has unknown type %s",
                                 vname.c_str(), tname.in());
              return base::DataSourceBase::shared_ptr( );
          }
        };

        bool CorbaLibPlugin::registerTransport(std::string name, TypeInfo* ti)
        {
            if ( name == "unknown_t") // register fallback also.
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaFallBackProtocol());
            if ( name == "Int8" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::int8_t>() );
            if ( name == "UInt8" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::uint8_t>() );
            if ( name == "Int16" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::int16_t>() );
            if ( name == "UInt16" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::uint16_t>() );
            if ( name == "Int32" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::int32_t>() );
            if ( name == "UInt32" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::uint32_t>() );
            if ( name == "Int64" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::int64_t>() );
            if ( name == "UInt64" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::uint64_t>() );
            if ( name == "Float32" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<float>() );
            if ( name == "Float64" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<double>() );
            if ( name == "Char" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<char>() );
//                if ( name == "PropertyBag" )
//                    return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<PropertyBag>() );
            if ( name == "Bool" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<bool>() );
#ifndef RTT_NO_STD_TYPES
            if ( name == "String" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<std::string>() );
            if ( name == "Float64Array" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol< std::vector<double> >() );
#endif
#ifdef OS_RT_MALLOC
            if ( name == "RtString")
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<rt_string>() );
#endif
            if ( name == "Void" )
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaFallBackProtocol(false)); // warn=false
            if ( name == "ConnPolicy")
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<ConnPolicy>() );
            if ( name == "TaskContext")
                return ti->addProtocol(ORO_CORBA_PROTOCOL_ID, new CorbaTemplateProtocol<TaskContext*>() );

            return false;
        }

        std::string CorbaLibPlugin::getTransportName() const {
            return "CORBA";
        }

        std::string CorbaLibPlugin::getTypekitName() const {
            return "rtt-types";
        }

        std::string CorbaLibPlugin::getName() const {
            return "rtt-corba-types";
        }
    }
}

ORO_TYPEKIT_PLUGIN( RTT::corba::CorbaLibPlugin )
