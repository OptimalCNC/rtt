/***************************************************************************
  tag: Peter Soetens  Mon Jun 26 13:25:56 CEST 2006  RealTimeTypekit.cxx

                        RealTimeTypekit.cxx -  description
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



#include "rtt-typekit-config.h"
#include "RealTimeTypekit.hpp"
#include "../types/Types.hpp"
#include "../FlowStatus.hpp"
#include "../internal/DataSources.hpp"
#include "../typekit/Types.hpp"
#include "../rtt-fwd.hpp"
#include "../internal/mystd.hpp"
#include "../types/TemplateConstructor.hpp"
#include <cstdint>
#include <type_traits>
#ifdef OS_RT_MALLOC
#include "../rt_string.hpp"
#endif

namespace RTT
{
    using namespace std;
    using namespace detail;

    namespace {
#ifndef ORO_EMBEDDED
        // CONSTRUCTORS
        struct array_ctor
        {
            typedef const std::vector<double>& (Signature)( int );
            typedef int argument_type;
            typedef const std::vector<double>& result_type;
            mutable boost::shared_ptr< std::vector<double> > ptr;
            array_ctor()
                : ptr( new std::vector<double>() ) {}
            const std::vector<double>& operator()( int size ) const
            {
                ptr->resize( size );
                return *(ptr);
            }
        };

        /**
         * See NArityDataSource which requires a function object like
         * this one.
         */
        struct array_varargs_ctor
        {
            typedef const std::vector<double>& result_type;
            typedef double argument_type;
            result_type operator()( const std::vector<double>& args ) const
            {
                return args;
            }
        };

        /**
         * Helper DataSource for constructing arrays with a variable number of
         * parameters.
         */
        typedef NArityDataSource<array_varargs_ctor> ArrayDataSource;

        /**
         * Constructs an array with \a n elements, which are given upon
         * construction time.
         */
        struct ArrayBuilder
            : public TypeConstructor
        {
            virtual DataSourceBase::shared_ptr build(const std::vector<DataSourceBase::shared_ptr>& args) const {
                if (args.size() == 0 )
                    return DataSourceBase::shared_ptr();
                ArrayDataSource::shared_ptr vds = new ArrayDataSource();
                for(unsigned int i=0; i != args.size(); ++i) {
                    DataSource<double>::shared_ptr dsd = boost::dynamic_pointer_cast< DataSource<double> >( args[i] );
                    if (dsd)
                        vds->add( dsd );
                    else
                        return DataSourceBase::shared_ptr();
                }
                return vds;
            }

        };

        struct array_ctor2
        {
            typedef const std::vector<double>& (Signature)( int, double );
            typedef int first_argument_type;
            typedef double second_argument_type;
            typedef const std::vector<double>& result_type;
            mutable boost::shared_ptr< std::vector<double> > ptr;
            array_ctor2()
                : ptr( new std::vector<double>() ) {}
            const std::vector<double>& operator()( int size, double value ) const
            {
                ptr->resize( size );
                ptr->assign( size, value );
                return *(ptr);
            }
        };

        template <typename Target, typename Source>
        Target numeric_cast(Source value)
        {
            return static_cast<Target>(value);
        }

        template <typename Signature>
        struct ExactTypeConstructor : types::TemplateConstructor<Signature>
        {
            using Base = types::TemplateConstructor<Signature>;
            using Source = typename Base::arg1_type;

            template <typename Function>
            ExactTypeConstructor(Function function, bool automatic)
                : Base(function, automatic)
            {
            }

            base::DataSourceBase::shared_ptr build(
                const std::vector<base::DataSourceBase::shared_ptr>& args) const override
            {
                if (args.size() != 1 ||
                    args.front()->getTypeInfo() !=
                        internal::DataSourceTypeInfo<Source>::getTypeInfo()) {
                    return base::DataSourceBase::shared_ptr();
                }
                return Base::build(args);
            }
        };

        template <typename Target, typename Source>
        void add_numeric_constructor(TypeInfo* target, bool automatic)
        {
            if constexpr (!std::is_same_v<Target, Source>) {
                target->addConstructor(
                    new ExactTypeConstructor<Target(Source)>(
                        &numeric_cast<Target, Source>, automatic));
            }
        }

        template <typename Target>
        void add_integer_constructors(TypeInfo* target)
        {
            add_numeric_constructor<Target, std::int8_t>(target, false);
            add_numeric_constructor<Target, std::uint8_t>(target, false);
            add_numeric_constructor<Target, std::int16_t>(target, false);
            add_numeric_constructor<Target, std::uint16_t>(target, false);
            add_numeric_constructor<Target, std::int32_t>(target, true);
            add_numeric_constructor<Target, std::uint32_t>(target, true);
            add_numeric_constructor<Target, std::int64_t>(target, false);
            add_numeric_constructor<Target, std::uint64_t>(target, false);
            add_numeric_constructor<Target, float>(target, false);
            add_numeric_constructor<Target, double>(target, false);
            add_numeric_constructor<Target, bool>(target, true);
        }

        template <typename Target>
        void add_floating_point_constructors(TypeInfo* target)
        {
            add_numeric_constructor<Target, std::int8_t>(target, true);
            add_numeric_constructor<Target, std::uint8_t>(target, true);
            add_numeric_constructor<Target, std::int16_t>(target, true);
            add_numeric_constructor<Target, std::uint16_t>(target, true);
            add_numeric_constructor<Target, std::int32_t>(target, true);
            add_numeric_constructor<Target, std::uint32_t>(target, true);
            add_numeric_constructor<Target, std::int64_t>(target, true);
            add_numeric_constructor<Target, std::uint64_t>(target, true);
            add_numeric_constructor<Target, float>(target, true);
            add_numeric_constructor<Target, double>(target, true);
        }

#endif
        bool flowstatus_to_bool(FlowStatus fs) { return fs != NoData ; }
        bool writestatus_to_bool(WriteStatus fs) { return fs == WriteSuccess ; }
        bool send_to_bool(SendStatus ss) { return ss == SendSuccess; }
        template <typename Source>
        bool numeric_to_bool(Source value)
        {
            return value != Source{};
        }

        template <typename Source>
        void add_bool_constructor(TypeInfo* target)
        {
            target->addConstructor(
                new ExactTypeConstructor<bool(Source)>(
                    &numeric_to_bool<Source>, true));
        }

        struct string_ctor
        {
            typedef std::string (Signature)( int );
            typedef int argument_type;
            typedef std::string result_type;
            std::string operator()( int size ) const
            {
                return std::string( size, std::string::value_type() );
            }
        };

#ifdef OS_RT_MALLOC
        struct rt_string_ctor_int
        {
            typedef rt_string (Signature)( int );
            typedef int argument_type;
            typedef rt_string result_type;
            rt_string operator()( int size ) const
            {
                return rt_string( size, rt_string::value_type() );
            }
        };

        struct rt_string_ctor_string
        {
            typedef rt_string (Signature)( std::string const& );
            typedef const std::string& argument_type;
            typedef rt_string result_type;
            rt_string operator()( std::string const& arg ) const
            {
                return rt_string( arg.c_str() );
            }
        };

        struct string_ctor_rt_string
        {
            typedef std::string (Signature)( rt_string const& );
            typedef const rt_string& argument_type;
            typedef std::string result_type;
            std::string operator()( rt_string const& arg ) const
            {
                return std::string( arg.c_str() );
            }
        };

#endif
    }

    bool RealTimeTypekitPlugin::loadConstructors()
    {
        TypeInfoRepository::shared_ptr ti = TypeInfoRepository::Instance();
#ifndef ORO_EMBEDDED
        add_integer_constructors<std::int8_t>(ti->type("Int8"));
        add_integer_constructors<std::uint8_t>(ti->type("UInt8"));
        add_integer_constructors<std::int16_t>(ti->type("Int16"));
        add_integer_constructors<std::uint16_t>(ti->type("UInt16"));
        add_integer_constructors<std::int32_t>(ti->type("Int32"));
        add_integer_constructors<std::uint32_t>(ti->type("UInt32"));
        add_integer_constructors<std::int64_t>(ti->type("Int64"));
        add_integer_constructors<std::uint64_t>(ti->type("UInt64"));
        add_floating_point_constructors<float>(ti->type("Float32"));
        add_floating_point_constructors<double>(ti->type("Float64"));

        ti->type("String")->addConstructor( newConstructor( string_ctor() ) );
#ifdef OS_RT_MALLOC
        ti->type("rt_string")->addConstructor( newConstructor( rt_string_ctor_int() ) );
        ti->type("rt_string")->addConstructor( newConstructor( rt_string_ctor_string() ) );
        ti->type("String")->addConstructor( newConstructor( string_ctor_rt_string() ) );
#endif
        TypeInfo* bool_type = ti->type("Bool");
        bool_type->addConstructor( newConstructor( &flowstatus_to_bool, true ) );
        bool_type->addConstructor( newConstructor( &writestatus_to_bool, true ) );
        bool_type->addConstructor( newConstructor( &send_to_bool, true ) );
        add_bool_constructor<std::int8_t>(bool_type);
        add_bool_constructor<std::uint8_t>(bool_type);
        add_bool_constructor<std::int16_t>(bool_type);
        add_bool_constructor<std::uint16_t>(bool_type);
        add_bool_constructor<std::int32_t>(bool_type);
        add_bool_constructor<std::uint32_t>(bool_type);
        add_bool_constructor<std::int64_t>(bool_type);
        add_bool_constructor<std::uint64_t>(bool_type);
#endif
        return true;
    }
}
