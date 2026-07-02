/***************************************************************************
  tag: The SourceWorks  Tue Sep 7 00:55:18 CEST 2010  TypeInfoRepository.cpp

                        TypeInfoRepository.cpp -  description
                           -------------------
    begin                : Tue September 07 2010
    copyright            : (C) 2010 The SourceWorks
    email                : peter@thesourceworks.com

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


#include "TypeInfoRepository.hpp"

#include "rtt-config.h"

#include "../Logger.hpp"
#include "TypeTransporter.hpp"
#include "TransportPlugin.hpp"
#include "../internal/mystd.hpp"
#include "../internal/DataSourceTypeInfo.hpp"
#include <boost/algorithm/string.hpp>
#include <cstdio>

namespace RTT
{
    using namespace std;
    using namespace detail;

    namespace {
        boost::shared_ptr<TypeInfoRepository> typerepos;
    }

    TypeInfoRepository::TypeInfoRepository()
    {
    }

    boost::shared_ptr<TypeInfoRepository> TypeInfoRepository::Instance()
    {
        if ( typerepos )
            return typerepos;
        typerepos.reset( new TypeInfoRepository() );

        return typerepos;
    }

    void TypeInfoRepository::Release() {
        typerepos.reset();
    }
    
    void TypeInfoRepository::setAutoLoader(const boost::function<bool (const std::string &)> &loader)
    {
        loadTypeKitForName = loader;
    }
    
    TypeInfo* TypeInfoRepository::typeInternal( const std::string& name ) const
    {
        MutexLock lock(type_lock);
        map_t::const_iterator i = data.find( name );
        if ( i != data.end() ) {
            // found
            return i->second;
        }

        // try alternate name replace / with dots:
        string tkname = "/" + boost::replace_all_copy(boost::replace_all_copy(name, string("."), "/"), "<","</");
        i = data.find( tkname );
        if ( i != data.end() ) {
            // found
            return i->second;
        }

        // try alias name
        for (i = data.begin(); i != data.end(); ++i) {
            std::vector< std::string > names = i->second->getTypeNames();
            vector<std::string>::iterator j = names.begin();
            for (; j != names.end(); ++j) {
                if(((*j) == name) || ((*j) == tkname)) {
                    return i->second;
                }
            }
        }

        // not found
        return 0;
    }

    TypeInfo* TypeInfoRepository::type( const std::string& name ) const
    {
        TypeInfo *ret = typeInternal(name);
        
        if(!ret && loadTypeKitForName)
        {
            if(loadTypeKitForName(name))
                ret = typeInternal(name);
        }
            
        return ret;
    }

    TypeInfoRepository::~TypeInfoRepository()
    {
        // because of aliases, we only want unique pointers:
        vector<TypeInfo*> todelete = values(data);
        sort(todelete.begin(), todelete.end());
        vector<TypeInfo*>::iterator begin, last = unique( todelete.begin(), todelete.end() );
        begin = todelete.begin();
        for( ; begin != last; ++begin )
            delete *begin;
        delete DataSourceTypeInfo<UnknownType>::TypeInfoObject;
        DataSourceTypeInfo<UnknownType>::TypeInfoObject = 0;
    }

    TypeInfo* TypeInfoRepository::getTypeById(TypeInfo::TypeId type_id) const {
      if (!type_id)
          return 0;
      MutexLock lock(type_lock);
      // Ask each type for its type id name.
      map_t::const_iterator i = data.begin();
      for (; i != data.end(); ++i){
        if (i->second->getTypeId() && *(i->second->getTypeId()) == *type_id)
          return i->second;
      }
      return 0;
    }

    TypeInfo* TypeInfoRepository::getTypeById(const char * type_id_name) const {
      // Ask each type for its type id name.
      MutexLock lock(type_lock);
      map_t::const_iterator i = data.begin();
      for (; i != data.end(); ++i){
        if (i->second->getTypeId() && i->second->getTypeId()->name() == type_id_name)
          return i->second;
      }
      return 0;
    }

    bool TypeInfoRepository::addType(TypeInfo* t)
    {
        if (!t)
            return false;
        MutexLock lock(type_lock);
        if (data.count(t->getTypeName() ) ) {
            Logger::log().logf(Logger::Error, "TypeInfoRepository",
                               "Can't register a new TypeInfo object for '%s': one already exists.",
                               t->getTypeName().c_str());
            return false;
        }

        data[t->getTypeName()] = t;
        return true;
    }

    bool TypeInfoRepository::addType(TypeInfoGenerator* t)
    {
        if (!t)
            return false;
        std::string tname = t->getTypeName();
        TypeInfo* ti = t->getTypeInfoObject();

        {
            MutexLock lock(type_lock);
            if (ti && data.count(tname) && data[tname] != ti ) {
                Logger::log().logf(Logger::Error, "TypeInfoRepository",
                                   "Refusing to add type information for '%s': the name is already in use by another type.",
                                   tname.c_str());
                return false;
            }
        }
        // Check for first registration, or alias:
        if ( ti == 0 )
            ti = new TypeInfo(tname);
        else
            ti->addAlias(tname);

        if ( t->installTypeInfoObject( ti ) ) {
            delete t;
        }
        MutexLock lock(type_lock);
        // keep track of this type:
        data[ tname ] = ti;

        Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                           "Registered Type '%s' to the Orocos Type System.",
                           tname.c_str());
        for(Transports::iterator it = transports.begin(); it != transports.end(); ++it)
            if ( (*it)->registerTransport( tname, ti) )
                Logger::log().logf(Logger::Info, "TypeInfoRepository",
                                   "Registered new '%s' transport for %s",
                                   (*it)->getTransportName().c_str(),
                                   tname.c_str());
        return true;
    }

    std::vector<std::string> TypeInfoRepository::getTypes() const
    {
        MutexLock lock(type_lock);
        return keys( data );
    }

    string TypeInfoRepository::toDot( const string& type ) const
    {
        if (type.empty())
            return type;
        // try alternate name replace / with dots:
        string dotname = boost::replace_all_copy(boost::replace_all_copy(type, string("/"), "."), "<.","<");
        if ( dotname[0] == '.')
            dotname = dotname.substr(1);
        return dotname;
    }

    std::vector<std::string> TypeInfoRepository::getDottedTypes() const
    {
        MutexLock lock(type_lock);
        vector<string> result = keys( data );
        for( vector<string>::iterator it = result.begin(); it != result.end(); ++it)
            *it = toDot(*it);
        return result;
    }

    void TypeInfoRepository::registerTransport( TransportPlugin* tr )
    {
        MutexLock lock(type_lock);
        transports.reserve( transports.size() + 1 );
        transports.push_back( tr );
        // inform transport of existing types.
        map_t::const_iterator i = data.begin();
        for( ; i != data.end(); ++i )
            if ( tr->registerTransport( i->first , i->second ) )
                Logger::log().logf(Logger::Info, "TypeInfoRepository",
                                   "Registered new '%s' transport for %s",
                                   tr->getTransportName().c_str(),
                                   i->first.c_str());
        // give chance to register fallback protocol:
        if ( tr->registerTransport("unknown_t", DataSourceTypeInfo<UnknownType>::getTypeInfo() ) == false )
            Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                               "Transport %s did not install a fallback handler for 'unknown_t'.",
                               tr->getTransportName().c_str());
    }

    void TypeInfoRepository::logTypeInfo() const
    {
        // dump the names of all known types
        Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                           "Types known to the Orocos Type System.");
        MutexLock lock(type_lock);
        for(map_t::const_iterator it = data.begin(); it != data.end(); ++it)
        {
            std::vector<int>    transports;
            transports = it->second->getTransportNames();
            char protocols[256];
            int offset = std::snprintf(protocols, sizeof(protocols), "[");
            for (std::vector<int>::const_iterator   iter=transports.begin();
                 iter != transports.end();
                 ++iter)
            {
                if (offset < 0 || offset >= int(sizeof(protocols)))
                    break;
                offset += std::snprintf(protocols + offset, sizeof(protocols) - offset,
                                        "%s%d",
                                        iter == transports.begin() ? "" : ",",
                                        *iter);
            }
            if (offset >= 0 && offset < int(sizeof(protocols)))
                std::snprintf(protocols + offset, sizeof(protocols) - offset, "]");
            else
                protocols[sizeof(protocols) - 1] = 0;
            Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                               "-- %s (%s) protocols %s",
                               it->first.c_str(),
                               (*it).second->getTypeName().c_str(),
                               protocols);
        }
        // dump the names of all known transports
        Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                           "Transports known to the Orocos Type System.");
        for(Transports::const_iterator it = transports.begin(); it != transports.end(); ++it)
        {
            Logger::log().logf(Logger::Debug, "TypeInfoRepository",
                               "-- %s",
                               (*it)->getTransportName().c_str());
        }
    }


}
