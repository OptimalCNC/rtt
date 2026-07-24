/***************************************************************************
  tag: Peter Soetens Fri Nov 26 16:10:18 2010 +0100 Types.hpp

                        Types.hpp -  description
                           -------------------
    begin                : Fri Nov 26 2010
    copyright            : (C) 2010 Peter Soetens
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
 *   General Public License for more details.                              *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


/**
 * @file Types.hpp
 * This file contains a series of 'extern template' definitions of the
 * Orocos RTT pre-defined types. You should include this header in your components in case you use
 * any of these types. You then also need to link with the rtt-typekit library.
 */

#ifndef RTT_TYPEKIT_TYPES
//#define RTT_TYPEKIT_TYPES

#include "rtt-typekit-config.h"
#include "RTTTypes.hpp"
#include <rtt/rt_string.hpp>
#include <cstdint>
#include <string>
#include <vector>

// Disable extern template warning on MSVC
#if !defined( __MINGW__ ) && defined( WIN32 )
# pragma warning( disable : 4231 )
#endif

#ifdef CORELIB_DATASOURCE_HPP
# define RTT_TYPEKIT_EXTERN_DATASOURCES(TYPE)                               \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API                    \
        RTT::internal::DataSource<TYPE>;                                   \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API                    \
        RTT::internal::AssignableDataSource<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_DATASOURCES(TYPE)
#endif

#ifdef ORO_CORELIB_DATASOURCES_HPP
# define RTT_TYPEKIT_EXTERN_VALUE_DATASOURCES(TYPE)                         \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API                    \
        RTT::internal::ValueDataSource<TYPE>;                              \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API                    \
        RTT::internal::ConstantDataSource<TYPE>;                           \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API                    \
        RTT::internal::ReferenceDataSource<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_VALUE_DATASOURCES(TYPE)
#endif

#ifdef ORO_OUTPUT_PORT_HPP
# define RTT_TYPEKIT_EXTERN_OUTPUT_PORT(TYPE)                               \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API RTT::OutputPort<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_OUTPUT_PORT(TYPE)
#endif

#ifdef ORO_INPUT_PORT_HPP
# define RTT_TYPEKIT_EXTERN_INPUT_PORT(TYPE)                                \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API RTT::InputPort<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_INPUT_PORT(TYPE)
#endif

#ifdef ORO_PROPERTY_HPP
# define RTT_TYPEKIT_EXTERN_PROPERTY(TYPE)                                  \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API RTT::Property<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_PROPERTY(TYPE)
#endif

#ifdef ORO_CORELIB_ATTRIBUTE_HPP
# define RTT_TYPEKIT_EXTERN_ATTRIBUTE(TYPE)                                 \
    RTT_TYPEKIT_EXT_TMPL template class RTT_TYPEKIT_API RTT::Attribute<TYPE>;
#else
# define RTT_TYPEKIT_EXTERN_ATTRIBUTE(TYPE)
#endif

#define RTT_TYPEKIT_EXTERN_INTERFACE_TYPE(TYPE)                             \
    RTT_TYPEKIT_EXTERN_OUTPUT_PORT(TYPE)                                    \
    RTT_TYPEKIT_EXTERN_INPUT_PORT(TYPE)                                     \
    RTT_TYPEKIT_EXTERN_PROPERTY(TYPE)                                       \
    RTT_TYPEKIT_EXTERN_ATTRIBUTE(TYPE)

#define RTT_TYPEKIT_EXTERN_TYPE(TYPE)                                       \
    RTT_TYPEKIT_EXTERN_DATASOURCES(TYPE)                                    \
    RTT_TYPEKIT_EXTERN_VALUE_DATASOURCES(TYPE)                              \
    RTT_TYPEKIT_EXTERN_INTERFACE_TYPE(TYPE)

RTT_TYPEKIT_EXTERN_TYPE(std::int8_t)
RTT_TYPEKIT_EXTERN_TYPE(std::uint8_t)
RTT_TYPEKIT_EXTERN_TYPE(std::int16_t)
RTT_TYPEKIT_EXTERN_TYPE(std::uint16_t)
RTT_TYPEKIT_EXTERN_TYPE(std::int32_t)
RTT_TYPEKIT_EXTERN_TYPE(std::uint32_t)
RTT_TYPEKIT_EXTERN_TYPE(std::int64_t)
RTT_TYPEKIT_EXTERN_TYPE(std::uint64_t)
RTT_TYPEKIT_EXTERN_TYPE(float)
RTT_TYPEKIT_EXTERN_TYPE(double)
RTT_TYPEKIT_EXTERN_TYPE(char)
RTT_TYPEKIT_EXTERN_INTERFACE_TYPE(bool)
RTT_TYPEKIT_EXTERN_INTERFACE_TYPE(std::string)
RTT_TYPEKIT_EXTERN_TYPE(std::vector<double>)
RTT_TYPEKIT_EXTERN_TYPE(RTT::rt_string)

#undef RTT_TYPEKIT_EXTERN_TYPE
#undef RTT_TYPEKIT_EXTERN_INTERFACE_TYPE
#undef RTT_TYPEKIT_EXTERN_ATTRIBUTE
#undef RTT_TYPEKIT_EXTERN_PROPERTY
#undef RTT_TYPEKIT_EXTERN_INPUT_PORT
#undef RTT_TYPEKIT_EXTERN_OUTPUT_PORT
#undef RTT_TYPEKIT_EXTERN_VALUE_DATASOURCES
#undef RTT_TYPEKIT_EXTERN_DATASOURCES


#endif
