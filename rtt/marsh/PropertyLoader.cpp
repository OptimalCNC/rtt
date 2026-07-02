/***************************************************************************
  tag: Peter Soetens  Sat May 7 12:56:52 CEST 2005  PropertyLoader.cxx

                        PropertyLoader.cxx -  description
                           -------------------
    begin                : Sat May 07 2005
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



#include "PropertyLoader.hpp"
#include "rtt-config.h"
#ifdef OROPKG_CORELIB_PROPERTIES_MARSHALLING
#include ORODAT_CORELIB_PROPERTIES_MARSHALLING_INCLUDE
#include ORODAT_CORELIB_PROPERTIES_DEMARSHALLING_INCLUDE
#endif
#include "../Logger.hpp"
#include "../TaskContext.hpp"
#include "PropertyBagIntrospector.hpp"
#include "../types/PropertyComposition.hpp"
#include <fstream>

using namespace std;
using namespace RTT;
using namespace RTT::detail;

namespace {
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
    void logNoDemarshaller(const char* module)
    {
        Logger::log().logf(Logger::Error, module, "No Property DemarshallInterface configured !");
    }

    void logNoMarshaller(const char* module, const char* interface_name)
    {
        Logger::log().logf(Logger::Error, module, "No Property %s configured !", interface_name);
    }
#endif

#ifdef OROPKG_CORELIB_PROPERTIES_MARSHALLING
    void logNoPropertiesToConfigure(const char* module, Service* target)
    {
        Logger::log().logf(Logger::Error, module,
                           "Service %s has no Properties to configure.",
                           target->getName().c_str());
    }

    void logCannotOpenFile(const char* module, const std::string& filename)
    {
        Logger::log().logf(Logger::Error, module, "Could not open file %s", filename.c_str());
    }

    void logCannotOpenFileForWriting(const char* module, const std::string& filename)
    {
        Logger::log().logf(Logger::Error, module,
                           "Could not open file %s for writing.",
                           filename.c_str());
    }

    void logParseError(const char* module, const std::string& filename)
    {
        Logger::log().logf(Logger::Error, module,
                           "Some error occured while parsing %s",
                           filename.c_str());
    }

    void logUncaughtDeserialiseException(const char* module)
    {
        Logger::log().logf(Logger::Error, module, "Uncaught exception in deserialise !");
    }
#endif
}

PropertyLoader::PropertyLoader(TaskContext *task)
  : target(task->provides().get())
{}

PropertyLoader::PropertyLoader(Service *service)
  : target(service)
{}

bool PropertyLoader::load(const std::string& filename) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
        logNoDemarshaller("PropertyLoader:load");
        return false;

#else
    if ( target->properties() == 0) {
        logNoPropertiesToConfigure("PropertyLoader:load", target);
        return false;
    }

    Logger::log().logf(Logger::Info, "PropertyLoader:load",
                       "Loading properties into Service '%s' with '%s'.",
                       target->getName().c_str(), filename.c_str());
    bool failure = false;
    OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER* demarshaller = 0;
    try
    {
        demarshaller = new OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER (filename);
    } catch (...) {
        logCannotOpenFile("PropertyLoader:load", filename);
        return false;
    }
    try {
        PropertyBag propbag;
        vector<ActionInterface*> assignComs;

        if ( demarshaller->deserialize( propbag ) )
        {
            // compose propbag:
            PropertyBag composed_props;
            if ( composePropertyBag(propbag, composed_props) == false) {
                delete demarshaller;
                return false;
            }
            // take restore-copy;
            PropertyBag backup;
            copyProperties( backup, *target->properties() );
            // First test if the updateProperties will succeed:
            if ( refreshProperties(  *target->properties(), composed_props, false) ) { // not strict
                // this just adds the new properties, *should* never fail, but
                // let's record failure to be sure.
                failure = !updateProperties( *target->properties(), composed_props );
            } else {
                // restore backup in case of failure:
                refreshProperties( *target->properties(), backup, false ); // not strict
                failure = true;
            }
            // cleanup
            deletePropertyBag( backup );
        }
        else
            {
                logParseError("PropertyLoader:load", filename);
                failure = true;
            }
    } catch (...)
    {
        logUncaughtDeserialiseException("PropertyLoader:load");
        failure = true;
    }
    delete demarshaller;
    return !failure;
#endif // OROPKG_CORELIB_PROPERTIES_MARSHALLING

}

bool PropertyLoader::configure(const std::string& filename, bool all ) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
        logNoDemarshaller("PropertyLoader:configure");
        return false;

#else
    if ( target->properties() == 0) {
        logNoPropertiesToConfigure("PropertyLoader:configure", target);
        return false;
    }

    Logger::log().logf(Logger::Info, "PropertyLoader:configure",
                       "Configuring Service '%s' with '%s'.",
                       target->getName().c_str(), filename.c_str());
    bool failure = false;
    OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER* demarshaller = 0;
    try
    {
        demarshaller = new OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER (filename);
    } catch (...) {
        logCannotOpenFile("PropertyLoader:configure", filename);
        return false;
    }
    try {
        PropertyBag propbag;

        if ( demarshaller->deserialize( propbag ) )
        {
            // compose propbag:
            PropertyBag composed_props;
            if ( composePropertyBag(propbag, composed_props) == false) {
                delete demarshaller;
                return false;
            }
            // take restore-copy;
            PropertyBag backup;
            copyProperties( backup, *target->properties() );
            if ( refreshProperties( *target->properties(), composed_props, all ) == false ) {
                // restore backup:
                refreshProperties( *target->properties(), backup );
                failure = true;
                }
            // cleanup
            deletePropertyBag( backup );
        }
        else
            {
                logParseError("PropertyLoader:configure", filename);
                failure = true;
            }
        deletePropertyBag( propbag );
    } catch (...)
    {
        logUncaughtDeserialiseException("PropertyLoader:configure");
        failure = true;
    }
    delete demarshaller;
    return !failure;
#endif // OROPKG_CORELIB_PROPERTIES_MARSHALLING

}

bool PropertyLoader::store(const std::string& filename) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
    logNoMarshaller("PropertyLoader::store", "Marshaller");
    return false;
#else
    std::ofstream file( filename.c_str() );
    if ( file )
    {
        // Write results
        PropertyBag* compProps = target->properties();
        PropertyBag allProps;

        // decompose repos into primitive property types.
        PropertyBagIntrospector pbi( allProps );
        pbi.introspect( *compProps );

        OROCLS_CORELIB_PROPERTIES_MARSHALLING_DRIVER<std::ostream> marshaller( file );
        marshaller.serialize( allProps );
        deletePropertyBag( allProps );
        Logger::log().logf(Logger::Info, "PropertyLoader::store",
                           "Wrote %s", filename.c_str());
    }
    else {
        logCannotOpenFileForWriting("PropertyLoader::store", filename);
        return false;
    }
    return true;
#endif
}

bool PropertyLoader::save(const std::string& filename, bool all) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
        logNoMarshaller("PropertyLoader::save", "MarshallInterface");
        return false;

#else
    if ( target->properties() == 0 ) {
        Logger::log().logf(Logger::Error, "PropertyLoader::save",
                           "Service %s does not have Properties to save.",
                           target->getName().c_str());
        return false;
    }
    PropertyBag allProps;
	PropertyBag  decompProps;

    // first check if the target file exists.
    std::ifstream ifile( filename.c_str() );
    // if target file does not exist, skip this step.
    if ( ifile ) {
        ifile.close();
        Logger::log().logf(Logger::Info, "PropertyLoader::save",
                           "%s updating of file %s",
                           target->getName().c_str(), filename.c_str());
        // The demarshaller itself will open the file.
        OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER demarshaller( filename );
        if ( demarshaller.deserialize( allProps ) == false ) {
            // Parse error, abort writing of this file.
            Logger::log().logf(Logger::Error, "PropertyLoader::save",
                               "While updating %s : Failed to read %s",
                               target->getName().c_str(), filename.c_str());
            return false;
        }
    }
    else {
        Logger::log().logf(Logger::Info, "PropertyLoader::save",
                           "Creating %s", filename.c_str());
        return store(filename);
    }

    // Write results
    PropertyBag* compProps = target->properties();

    // decompose repos into primitive property types.
    PropertyBagIntrospector pbi( decompProps );
    pbi.introspect( *compProps );

    //Add target properties to existing properties
    bool updater = false;
    if (all) {
        Logger::log().logf(Logger::Info, "PropertyLoader::save",
                           "Writing all properties of %s to file %s",
                           target->getName().c_str(), filename.c_str());
        updater = updateProperties( allProps, decompProps ); // add new.
    }
    else {
        Logger::log().logf(Logger::Info, "PropertyLoader::save",
                           "Refreshing properties in file %s with values of properties of %s",
                           filename.c_str(), target->getName().c_str());
        updater = refreshProperties( allProps, decompProps ); // only refresh existing.
    }
    if (updater == false) {
        Logger::log().logf(Logger::Error, "PropertyLoader::save",
                           "Could not update properties of file %s.",
                           filename.c_str());
        deletePropertyBag( allProps );
        deletePropertyBag( decompProps );
        return false;
    }
    // ok, finish.
    // serialize and cleanup
    std::ofstream file( filename.c_str() );
    if ( file )
        {
            OROCLS_CORELIB_PROPERTIES_MARSHALLING_DRIVER<std::ostream> marshaller( file );
            marshaller.serialize( allProps );
            Logger::log().logf(Logger::Info, "PropertyLoader::save",
                               "Wrote %s", filename.c_str());
        }
    else {
        logCannotOpenFileForWriting("PropertyLoader::save", filename);
        deletePropertyBag( allProps );
        return false;
    }
    // allProps contains copies (clone()), thus may be safely deleted :
    deletePropertyBag( allProps );
    deletePropertyBag( decompProps );
    return true;
#endif
}

bool PropertyLoader::configure(const std::string& filename, const std::string& name ) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
    logNoDemarshaller("PropertyLoader:configure");
    return false;

#else
    Logger::log().logf(Logger::Info, "PropertyLoader:configure",
                       "Reading Property '%s' from file '%s'.",
                       name.c_str(), filename.c_str());
    bool failure = false;
    OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER* demarshaller = 0;
    try
    {
        demarshaller = new OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER (filename);
    } catch (...) {
        logCannotOpenFile("PropertyLoader:configure", filename);
        return false;
    }
    try {
        PropertyBag propbag;
        if ( demarshaller->deserialize( propbag ) )
        {
            // compose propbag:
            PropertyBag composed_props;
            if ( composePropertyBag(propbag, composed_props) == false) {
                deletePropertyBag( propbag );
                delete demarshaller;
                return false;
            }
            failure = !refreshProperty( *(target->properties()), composed_props, name );
        }
        else
            {
                logParseError("PropertyLoader:configure", filename);
                failure = true;
            }
        deletePropertyBag( propbag );
    } catch (...)
    {
        logUncaughtDeserialiseException("PropertyLoader:configure");
        failure = true;
    }
    delete demarshaller;
    return !failure;
#endif // OROPKG_CORELIB_PROPERTIES_MARSHALLING
}

bool PropertyLoader::save(const std::string& filename, const std::string& name) const
{
#ifndef OROPKG_CORELIB_PROPERTIES_MARSHALLING
        logNoMarshaller("PropertyLoader::save", "MarshallInterface");
        return false;

#else
    PropertyBag fileProps;
    // Update exising file ?
    {
        // first check if the target file exists.
        std::ifstream ifile( filename.c_str() );
        // if target file does not exist, skip this step.
        if ( ifile ) {
            ifile.close();
            Logger::log().logf(Logger::Info, "PropertyLoader::save",
                               "Updating file %s with properties of %s",
                               filename.c_str(), target->getName().c_str());
            // The demarshaller itself will open the file.
            OROCLS_CORELIB_PROPERTIES_DEMARSHALLING_DRIVER demarshaller( filename );
            if ( demarshaller.deserialize( fileProps ) == false ) {
                // Parse error, abort writing of this file.
                Logger::log().logf(Logger::Error, "PropertyLoader::save",
                                   "Failed to read %s", filename.c_str());
                return false;
            }
        }
        else
            Logger::log().logf(Logger::Info, "PropertyLoader::save",
                               "Creating %s", filename.c_str());
    }

    // decompose service properties into primitive property types.
    PropertyBag  serviceProps;
    PropertyBagIntrospector pbi( serviceProps );
    pbi.introspect( *(target->properties()) );

    bool failure;
    failure = ! updateProperty( fileProps, serviceProps, name );

    deletePropertyBag( serviceProps );

    if ( failure ) {
        Logger::log().logf(Logger::Error, "PropertyLoader::save",
                           "Could not update properties of file %s.",
                           filename.c_str());
        deletePropertyBag( fileProps );
        return false;
    }
    // serialize and cleanup
    std::ofstream file( filename.c_str() );
    if ( file )
        {
            OROCLS_CORELIB_PROPERTIES_MARSHALLING_DRIVER<std::ostream> marshaller( file );
            marshaller.serialize( fileProps );
            Logger::log().logf(Logger::Info, "PropertyLoader::save",
                               "Wrote Property %s to %s",
                               name.c_str(), filename.c_str());
        }
    else {
        logCannotOpenFileForWriting("PropertyLoader::save", filename);
        deletePropertyBag( fileProps );
        return false;
    }
    // fileProps contains copies (clone()), thus may be safely deleted :
    deletePropertyBag( fileProps );
    return true;
#endif
}
