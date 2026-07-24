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
#include "../types/Operators.hpp"
#include "../types/OperatorTypes.hpp"
#include "../internal/mystd.hpp"
#include "../rtt-fwd.hpp"
#include "../FlowStatus.hpp"
#include "../SendStatus.hpp"
#include "../ConnPolicy.hpp"
#include "../typekit/Types.hpp"
#include <cstdint>
#include <ostream>
#include <sstream>
#include <type_traits>
#ifdef OS_RT_MALLOC
#include "../rt_string.hpp"
#endif

namespace RTT
{
    using namespace std;
    using namespace detail;

    template <typename T>
    void add_signed_integer_operators(OperatorRepository::shared_ptr const& oreg)
    {
        oreg->add( newUnaryOperator( "-", std::negate<T>() ) );
        oreg->add( newUnaryOperator( "+", identity<T>() ) );
        oreg->add( newBinaryOperator( "*", std::multiplies<T>() ) );
        oreg->add( newBinaryOperator( "/", divides3<T, T, T>() ) );
        oreg->add( newBinaryOperator( "%", std::modulus<T>() ) );
        oreg->add( newBinaryOperator( "+", std::plus<T>() ) );
        oreg->add( newBinaryOperator( "-", std::minus<T>() ) );
        oreg->add( newBinaryOperator( "<", std::less<T>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<T>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<T>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<T>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<T>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to<T>() ) );
    }

    template <typename T>
    void add_unsigned_integer_operators(OperatorRepository::shared_ptr const& oreg)
    {
        oreg->add( newUnaryOperator( "+", identity<T>() ) );
        oreg->add( newBinaryOperator( "*", std::multiplies<T>() ) );
        oreg->add( newBinaryOperator( "/", divides3<T, T, T>() ) );
        oreg->add( newBinaryOperator( "%", std::modulus<T>() ) );
        oreg->add( newBinaryOperator( "+", std::plus<T>() ) );
        oreg->add( newBinaryOperator( "-", std::minus<T>() ) );
        oreg->add( newBinaryOperator( "<", std::less<T>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<T>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<T>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<T>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<T>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to<T>() ) );
    }

    template <typename T>
    void add_floating_point_operators(OperatorRepository::shared_ptr const& oreg)
    {
        oreg->add( newUnaryOperator( "-", std::negate<T>() ) );
        oreg->add( newUnaryOperator( "+", identity<T>() ) );
        oreg->add( newBinaryOperator( "*", std::multiplies<T>() ) );
        oreg->add( newBinaryOperator( "/", std::divides<T>() ) );
        oreg->add( newBinaryOperator( "+", std::plus<T>() ) );
        oreg->add( newBinaryOperator( "-", std::minus<T>() ) );
        oreg->add( newBinaryOperator( "<", std::less<T>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<T>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<T>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<T>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<T>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to<T>() ) );
    }

#ifndef RTT_NO_STD_TYPES
    template<class T>
    struct get_capacity
    {
        typedef T argument_type;
        typedef int result_type;

        int operator()(T cont ) const
        {
            return cont.capacity();
        }
    };

    template<class T>
    struct get_size
    {
        typedef T argument_type;
        typedef int result_type;

        int operator()(T cont ) const
        {
            return cont.size();
        }
    };

    /** @cond */
    /** Strings concatenation
     */
    template <class T>
    struct string_concatenation {
        typedef const std::string& first_argument_type;
        typedef T second_argument_type;
        typedef std::string result_type;

        std::string operator()(const std::string& s, T t) const {
            std::ostringstream oss(s, std::ios_base::ate);
            oss << std::boolalpha;
            if constexpr (std::is_same_v<T, std::int8_t> ||
                          std::is_same_v<T, std::uint8_t>)
                oss << static_cast<int>(t);
            else
                oss << t;
            return oss.str();
        }
    };
#ifdef OS_RT_MALLOC
    template <class T>
    struct rt_string_concatenation {
        typedef const rt_string& first_argument_type;
        typedef T second_argument_type;
        typedef rt_string result_type;

        rt_string operator()(const rt_string& s, T t) const {
            rt_ostringstream oss(s, std::ios_base::ate);
            oss << std::boolalpha;
            if constexpr (std::is_same_v<T, std::int8_t> ||
                          std::is_same_v<T, std::uint8_t>)
                oss << static_cast<int>(t);
            else
                oss << t;
            return oss.str();
        }
    };
#endif
    /** @endcond */
#endif

    bool RealTimeTypekitPlugin::loadOperators()
    {
        OperatorRepository::shared_ptr oreg = OperatorRepository::Instance();

        // boolean stuff:
        oreg->add( newUnaryOperator( "!", std::logical_not<bool>() ) );
        oreg->add( newBinaryOperator( "&&", std::logical_and<bool>() ) );
        oreg->add( newBinaryOperator( "||", std::logical_or<bool>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<bool>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to<bool>() ) );

        add_signed_integer_operators<std::int8_t>(oreg);
        add_unsigned_integer_operators<std::uint8_t>(oreg);
        add_signed_integer_operators<std::int16_t>(oreg);
        add_unsigned_integer_operators<std::uint16_t>(oreg);
        add_signed_integer_operators<std::int32_t>(oreg);
        add_unsigned_integer_operators<std::uint32_t>(oreg);
#ifndef ORO_EMBEDDED
        add_signed_integer_operators<std::int64_t>(oreg);
        add_unsigned_integer_operators<std::uint64_t>(oreg);
#endif
        add_floating_point_operators<double>(oreg);
#ifndef ORO_EMBEDDED
        add_floating_point_operators<float>(oreg);
#endif
#ifndef RTT_NO_STD_TYPES
        // strings
        // causes memory allocation....
        oreg->add( newBinaryOperator( "+", std::plus<std::string>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::int8_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::uint8_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::int16_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::uint16_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::int32_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::uint32_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::int64_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<std::uint64_t>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<float>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<double>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<bool>() ) );
        oreg->add( newBinaryOperator( "+", string_concatenation<char>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<const std::string&>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to< const std::string&>() ) );
        oreg->add( newBinaryOperator( "<", std::less<const std::string&>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<const std::string&>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<std::string>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<std::string>() ) );
#endif

#ifdef OS_RT_MALLOC
        oreg->add( newBinaryOperator( "+", std::plus<rt_string>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::int8_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::uint8_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::int16_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::uint16_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::int32_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::uint32_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::int64_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<std::uint64_t>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<float>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<double>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<bool>() ) );
        oreg->add( newBinaryOperator( "+", rt_string_concatenation<char>() ) );
        oreg->add( newBinaryOperator( "==", std::equal_to<const rt_string&>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to< const rt_string&>() ) );
        oreg->add( newBinaryOperator( "<", std::less<const rt_string&>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<const rt_string&>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<rt_string>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<rt_string>() ) );
#endif

#ifndef ORO_EMBEDDED
        // chars
        oreg->add( newBinaryOperator( "==", std::equal_to<char>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to<char>() ) );
        oreg->add( newBinaryOperator( "<", std::less<char>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<char>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<char>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<char>() ) );
#if 0
        // causes memory allocation....
        oreg->add( newUnaryOperator( "-", std::negate<const std::vector<double>&>() ) );
        oreg->add( newBinaryOperator( "*", std::multiplies<const std::vector<double>&>() ) );
        oreg->add( newBinaryOperator( "+", std::plus<const std::vector<double>&>() ) );
        oreg->add( newBinaryOperator( "-", std::minus<const std::vector<double>&>() ) );
        oreg->add( newBinaryOperator( "*", multiplies3<const std::vector<double>&, double, const std::vector<double>&>() ) );
        oreg->add( newBinaryOperator( "*", multiplies3<const std::vector<double>&, const std::vector<double>&, double>() ) );
        oreg->add( newBinaryOperator( "/", divides3<const std::vector<double>&, const std::vector<double>&, double>() ) );
#endif
#endif

        // FlowStatus
        oreg->add( newBinaryOperator( "==", std::equal_to<FlowStatus>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to< FlowStatus>() ) );
        oreg->add( newBinaryOperator( "<", std::less<FlowStatus>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<FlowStatus>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<FlowStatus>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<FlowStatus>() ) );

        // WriteStatus
        oreg->add( newBinaryOperator( "==", std::equal_to<WriteStatus>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to< WriteStatus>() ) );
        oreg->add( newBinaryOperator( "<", std::less<WriteStatus>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<WriteStatus>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<WriteStatus>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<WriteStatus>() ) );

        // SendStatus
        oreg->add( newBinaryOperator( "==", std::equal_to<SendStatus>() ) );
        oreg->add( newBinaryOperator( "!=", std::not_equal_to< SendStatus>() ) );
        oreg->add( newBinaryOperator( "<", std::less<SendStatus>() ) );
        oreg->add( newBinaryOperator( ">", std::greater<SendStatus>() ) );
        oreg->add( newBinaryOperator( "<=", std::less_equal<SendStatus>() ) );
        oreg->add( newBinaryOperator( ">=", std::greater_equal<SendStatus>() ) );

        return true;
    }
}
