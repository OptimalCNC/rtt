/***************************************************************************
  tag: The SourceWorks  Tue Sep 7 00:54:57 CEST 2010  typekit_test.cpp

                        typekit_test.cpp -  description
                           -------------------
    begin                : Tue September 07 2010
    copyright            : (C) 2010 The SourceWorks
    email                : peter@thesourceworks.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


/**
 * typekit_test.cpp
 *
 *  Created on: May 10, 2010
 *      Author: kaltan
 */

#include "unit.hpp"

#include <cstdint>
#include <memory>
#include <type_traits>

#include <types/TemplateTypeInfo.hpp>
#include <types/TemplateConstructor.hpp>
#include <types/Operators.hpp>
#include <types/OperatorTypes.hpp>

#include <types/SequenceTypeInfo.hpp>
#include <typekit/RealTimeTypekit.hpp>
#include <base/TaskCore.hpp>
#ifdef OS_RT_MALLOC
#include <rtt/rt_string.hpp>
#endif

struct TypekitFixture
{
    TypekitFixture()
    {
        if (!Types()->type("Int32")) {
            RTT::types::RealTimeTypekitPlugin().loadTypes();
        }
    }
};

// Registers the fixture into the 'registry'
BOOST_FIXTURE_TEST_SUITE( TypekitTestSuite, TypekitFixture )

//! Tests the SequenceTypeInfo class.
BOOST_AUTO_TEST_CASE( testVectorTypeInfo )
{
    BOOST_REQUIRE(Types()->type("StringArray"));
    BOOST_CHECK(Types()->type("StringArray") ==
                Types()->getTypeInfo<std::vector<std::string>>());
#if 0 // not supported
    Types()->addType( new types::SequenceTypeInfo<std::vector<bool> >("bools") );
#endif
    BOOST_REQUIRE(Types()->type("Int32Array"));
    BOOST_CHECK(Types()->type("Int32Array") ==
                Types()->getTypeInfo<std::vector<std::int32_t>>());
}

//! This test tries to compose/decompose a default built variable of
//! every known type. So this is not a test covering the nominal case...
BOOST_AUTO_TEST_CASE( testComposeDecompose )
{
    vector<string> names = Types()->getTypes();
    for(vector<string>::iterator it = names.begin(); it != names.end(); ++it) {
        TypeInfo* ti = Types()->type(*it);
        BOOST_REQUIRE(ti);
        // might return null in case of void:
        std::unique_ptr<PropertyBase> input(ti->buildProperty("A", "B"));
        std::unique_ptr<PropertyBase> output(ti->buildProperty("C", "D"));
        // if it's decomposable, compose it as well.
        if ( input && output && ti->decomposeType(input->getDataSource()) ) {
            BOOST_CHECK_MESSAGE( ti->composeType( ti->decomposeType(input->getDataSource()), output->getDataSource()), "Decomposition/Composition of " + *it + " failed!" );
        }
    }
}

BOOST_AUTO_TEST_CASE( testCanonicalBuiltinTypesAreRegistered )
{
    const std::vector<std::string> canonical_names = {
        "Bool", "Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32",
        "Int64", "UInt64", "Float32", "Float64", "Char", "String", "Void",
        "Float64Array", "Int32Array", "StringArray", "TaskState"
#ifdef OS_RT_MALLOC
        , "RtString"
#endif
    };
    for (const std::string& name : canonical_names) {
        BOOST_CHECK_MESSAGE(Types()->type(name), "Missing canonical type " + name);
    }

    const std::vector<std::string> legacy_names = {
        "bool", "int8", "uint8", "short", "ushort", "int16", "uint16",
        "int", "uint", "int32", "uint32", "llong", "ullong", "int64",
        "uint64", "float", "double", "char", "string", "void", "array",
        "ints", "strings", "rt_string"
    };
    for (const std::string& name : legacy_names) {
        BOOST_CHECK_MESSAGE(!Types()->type(name), "Legacy type is still registered: " + name);
    }

#define RTT_CHECK_CANONICAL_TYPE(CPP_TYPE, TYPE_NAME)                         \
    BOOST_REQUIRE(Types()->getTypeInfo<CPP_TYPE>());                          \
    BOOST_CHECK_EQUAL(Types()->getTypeInfo<CPP_TYPE>()->getTypeName(), TYPE_NAME)

    RTT_CHECK_CANONICAL_TYPE(bool, "Bool");
    RTT_CHECK_CANONICAL_TYPE(std::int8_t, "Int8");
    RTT_CHECK_CANONICAL_TYPE(std::uint8_t, "UInt8");
    RTT_CHECK_CANONICAL_TYPE(std::int16_t, "Int16");
    RTT_CHECK_CANONICAL_TYPE(std::uint16_t, "UInt16");
    RTT_CHECK_CANONICAL_TYPE(std::int32_t, "Int32");
    RTT_CHECK_CANONICAL_TYPE(std::uint32_t, "UInt32");
    RTT_CHECK_CANONICAL_TYPE(std::int64_t, "Int64");
    RTT_CHECK_CANONICAL_TYPE(std::uint64_t, "UInt64");
    RTT_CHECK_CANONICAL_TYPE(float, "Float32");
    RTT_CHECK_CANONICAL_TYPE(double, "Float64");
    RTT_CHECK_CANONICAL_TYPE(char, "Char");
    RTT_CHECK_CANONICAL_TYPE(std::string, "String");
    RTT_CHECK_CANONICAL_TYPE(void, "Void");
    RTT_CHECK_CANONICAL_TYPE(std::vector<double>, "Float64Array");
    RTT_CHECK_CANONICAL_TYPE(std::vector<std::int32_t>, "Int32Array");
    RTT_CHECK_CANONICAL_TYPE(std::vector<std::string>, "StringArray");
    RTT_CHECK_CANONICAL_TYPE(RTT::base::TaskCore::TaskState, "TaskState");
#ifdef OS_RT_MALLOC
    RTT_CHECK_CANONICAL_TYPE(RTT::rt_string, "RtString");
#endif

#undef RTT_CHECK_CANONICAL_TYPE

    static_assert(std::is_same_v<short, std::int16_t>);
    static_assert(std::is_same_v<int, std::int32_t>);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::Init), 0);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::PreOperational), 1);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::FatalError), 2);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::Exception), 3);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::Stopped), 4);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::Running), 5);
    BOOST_CHECK_EQUAL(static_cast<int>(RTT::base::TaskCore::RunTimeError), 6);
}

BOOST_AUTO_TEST_SUITE_END()
