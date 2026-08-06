/***************************************************************************
  tag: The SourceWorks  Tue Sep 7 00:54:57 CEST 2010  type_discovery_struct_test.cpp

                        type_discovery_struct_test.cpp -  description
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


#include "unit.hpp"

#include <boost/serialization/vector.hpp>
#include <boost/array.hpp>

#include <rtt-fwd.hpp>
#include <internal/DataSources.hpp>
#include <types/type_discovery.hpp>
#include <os/fosi.h>

#include "datasource_fixture.hpp"
#include "types/StructTypeInfo.hpp"
#include "types/CArrayTypeInfo.hpp"
#include "types/SequenceTypeInfo.hpp"
#include "types/BoostArrayTypeInfo.hpp"

using namespace boost::archive;
using namespace boost::serialization;

struct CopyConstructibleArrayElement
{
    explicit CopyConstructibleArrayElement(int value) noexcept
        : value(value) {}
    CopyConstructibleArrayElement(
        const CopyConstructibleArrayElement& other) noexcept
        : value(other.value) {}
    CopyConstructibleArrayElement& operator=(
        const CopyConstructibleArrayElement&) = delete;

    int value;
};

struct CopyConstructibleArrayType
{
    CopyConstructibleArrayType()
        : values{CopyConstructibleArrayElement(11),
                 CopyConstructibleArrayElement(22)} {}
    CopyConstructibleArrayType(const CopyConstructibleArrayType& other)
        : values{CopyConstructibleArrayElement(other.values[0]),
                 CopyConstructibleArrayElement(other.values[1])} {}

    CopyConstructibleArrayElement values[2];
};

struct BoolArrayType
{
    BoolArrayType()
        : values{true, false} {}

    bool values[2];
};

struct EmptyArrayType
{
    boost::array<int, 0> values;
};

struct OpaqueArrayElement
{
    explicit OpaqueArrayElement(int value)
        : value(value) {}
    OpaqueArrayElement(const OpaqueArrayElement&) = delete;
    OpaqueArrayElement& operator=(const OpaqueArrayElement&) = delete;

    int value;
};

struct OpaqueArrayType
{
    OpaqueArrayType()
        : values{OpaqueArrayElement(11), OpaqueArrayElement(22)} {}
    OpaqueArrayType(const OpaqueArrayType& other)
        : values{OpaqueArrayElement(other.values[0].value),
                 OpaqueArrayElement(other.values[1].value)} {}

    OpaqueArrayElement values[2];
};

struct ThrowingCopyOnlyArrayElement
{
    explicit ThrowingCopyOnlyArrayElement(int value)
        : value(value) {}
    ThrowingCopyOnlyArrayElement(
        const ThrowingCopyOnlyArrayElement& other) noexcept(false)
        : value(other.value) {}
    ThrowingCopyOnlyArrayElement& operator=(
        const ThrowingCopyOnlyArrayElement&) = delete;

    int value;
};

struct ThrowingCopyOnlyArrayType
{
    ThrowingCopyOnlyArrayType()
        : values{ThrowingCopyOnlyArrayElement(11),
                 ThrowingCopyOnlyArrayElement(22)} {}
    ThrowingCopyOnlyArrayType(const ThrowingCopyOnlyArrayType& other)
        : values{ThrowingCopyOnlyArrayElement(other.values[0]),
                 ThrowingCopyOnlyArrayElement(other.values[1])} {}

    ThrowingCopyOnlyArrayElement values[2];
};

namespace boost
{
    namespace serialization
    {
        template <class Archive>
        void serialize(Archive& archive, CopyConstructibleArrayType& value,
                       const unsigned int)
        {
            archive & make_nvp("values", make_array(value.values, 2));
        }

        template <class Archive>
        void serialize(Archive& archive, BoolArrayType& value,
                       const unsigned int)
        {
            archive & make_nvp("values", make_array(value.values, 2));
        }

        template <class Archive>
        void serialize(Archive& archive, EmptyArrayType& value,
                       const unsigned int)
        {
            archive & make_nvp("values", value.values);
        }

        template <class Archive>
        void serialize(Archive& archive, OpaqueArrayType& value,
                       const unsigned int)
        {
            archive & make_nvp("values", make_array(value.values, 2));
        }

        template <class Archive>
        void serialize(Archive& archive, ThrowingCopyOnlyArrayType& value,
                       const unsigned int)
        {
            archive & make_nvp("values", make_array(value.values, 2));
        }
    }
}

class StructTypeTest
{
public:
    StructTypeTest() {  }
    ~StructTypeTest() {  }
};

// Registers the fixture into the 'registry'
BOOST_FIXTURE_TEST_SUITE(  TypeArchiveTestSuite,  StructTypeTest )

// Test the StructTypeInfo for AType
// Similar as the above tests, but now through the TypeInfo system.
BOOST_AUTO_TEST_CASE( testATypeStruct )
{
    Types()->addType( new StructTypeInfo<AType>("AType") );

    AssignableDataSource<AType>::shared_ptr atype = new ValueDataSource<AType>( AType(true) );

    BOOST_REQUIRE( Types()->type("AType") );

    // check the part names lookup:
    vector<string> names = atype->getMemberNames();
    BOOST_CHECK_EQUAL( atype->getMemberNames().size(), 5 );

    BOOST_REQUIRE_EQUAL( names.size(), 5);
    BOOST_REQUIRE( atype->getMember("a") );

    // Check individual part lookup by name:
    AssignableDataSource<int>::shared_ptr a = AssignableDataSource<int>::narrow( atype->getMember("a").get() );
    AssignableDataSource<double>::shared_ptr b = AssignableDataSource<double>::narrow( atype->getMember("b").get() );
    AssignableDataSource<string>::shared_ptr c = AssignableDataSource<string>::narrow( atype->getMember("c").get());
    AssignableDataSource<carray<int> >::shared_ptr ai = AssignableDataSource<carray<int> >::narrow( atype->getMember("ai").get());
    AssignableDataSource<vector<double> >::shared_ptr vd = AssignableDataSource<vector<double> >::narrow( atype->getMember("vd").get());

    BOOST_REQUIRE( a );
    BOOST_REQUIRE( b );
    BOOST_REQUIRE( c );
    BOOST_REQUIRE( ai );
    BOOST_REQUIRE( vd );

    BOOST_CHECK( !atype->getMember("zort") );

    // Check reading parts (must equal parent)
    BOOST_CHECK_EQUAL( a->get(), atype->get().a );
    BOOST_CHECK_EQUAL( b->get(), atype->get().b );
    BOOST_CHECK_EQUAL( c->get(), atype->get().c );
    BOOST_CHECK_EQUAL( ai->get().address()[3], atype->get().ai[3] );
    BOOST_CHECK_EQUAL( vd->get()[3], atype->get().vd[3] );

    // Check writing a part (must change in parent too).
    a->set(10);
    BOOST_CHECK_EQUAL( a->get(), 10 );
    BOOST_CHECK_EQUAL( a->get(), atype->get().a );

    DataSource<AType>::shared_ptr constant =
        new ConstantDataSource<AType>(AType(true));
    DataSourceBase::shared_ptr constant_a = constant->getMember("a");

    BOOST_REQUIRE(constant_a);
    DataSource<int>::shared_ptr readable_a =
        DataSource<int>::narrow(constant_a.get());
    BOOST_REQUIRE(readable_a);
    BOOST_CHECK_EQUAL(readable_a->get(), constant->get().a);
    BOOST_CHECK(!constant_a->isAssignable());
    BOOST_CHECK(!AssignableDataSource<int>::narrow(constant_a.get()));
}

//! Tests complex type introspection (sequence of structs)
BOOST_AUTO_TEST_CASE( testCTypeStruct )
{
    Types()->addType( new StructTypeInfo< AType >("AType") );
    Types()->addType( new StructTypeInfo< BType >("BType") );
    Types()->addType( new StructTypeInfo< CType >("CType") );
    Types()->addType( new SequenceTypeInfo< vector<AType> >("as") );
    Types()->addType( new SequenceTypeInfo< vector<BType> >("bs") );
    Types()->addType( new CArrayTypeInfo< carray<int> >("cints") );
    Types()->addType( new BoostArrayTypeInfo< boost::array<int,5> >("int5") );
    AssignableDataSource<CType>::shared_ptr atype = new ValueDataSource<CType>( CType(true) );

    // decompose a complex type
    AssignableDataSource<AType>::shared_ptr a = AssignableDataSource<AType>::narrow( atype->getMember("a").get() );
    AssignableDataSource<BType>::shared_ptr b = AssignableDataSource<BType>::narrow( atype->getMember("b").get() );
    AssignableDataSource< vector<AType> >::shared_ptr av = AssignableDataSource< vector<AType> >::narrow( atype->getMember("av").get());
    AssignableDataSource< vector<BType> >::shared_ptr bv = AssignableDataSource< vector<BType> >::narrow( atype->getMember("bv").get());

    BOOST_REQUIRE( a );
    BOOST_REQUIRE( b );
    BOOST_REQUIRE( av );
    BOOST_REQUIRE( bv );

    // Access top level elements
    BOOST_REQUIRE( a->getMember("ai") );
    AssignableDataSource<int>::shared_ptr ai3 = dynamic_pointer_cast< AssignableDataSource<int> >( a->getMember("ai")->getMember("3") );
    BOOST_REQUIRE( b->getMember("ai") );
    AssignableDataSource<int>::shared_ptr bi3 = dynamic_pointer_cast< AssignableDataSource<int> >( b->getMember("ai")->getMember("3") );

    // Access elements in sequences:
    AssignableDataSource<int>::shared_ptr avi3 = dynamic_pointer_cast< AssignableDataSource<int> >( av->getMember("3")->getMember("ai")->getMember("3") );
    AssignableDataSource<int>::shared_ptr bvi3 = dynamic_pointer_cast< AssignableDataSource<int> >( bv->getMember("3")->getMember("ai")->getMember("3") );

    BOOST_REQUIRE( ai3 );
    BOOST_REQUIRE( bi3 );
    BOOST_REQUIRE( avi3 );
    BOOST_REQUIRE( bvi3 );

    // Check reading parts (must equal parent)
    BOOST_CHECK_EQUAL( a->get(), atype->get().a );
    BOOST_CHECK_EQUAL( b->get(), atype->get().b );
    BOOST_CHECK( std::equal(av->set().begin(), av->set().end(), atype->set().av.begin() ) );
    BOOST_CHECK( std::equal(bv->set().begin(), bv->set().end(), atype->set().bv.begin() ) );
    BOOST_CHECK_EQUAL( avi3->get(), atype->get().av[3].ai[3] );
    BOOST_CHECK_EQUAL( bvi3->get(), atype->get().bv[3].ai[3] );

    // Check writing a part (must change in parent too).
    avi3->set(10);
    bvi3->set(20);
    BOOST_CHECK_EQUAL( avi3->get(), 10 );
    BOOST_CHECK_EQUAL( avi3->get(), atype->get().av[3].ai[3] );
    BOOST_CHECK_EQUAL( bvi3->get(), 20 );
    BOOST_CHECK_EQUAL( bvi3->get(), atype->get().bv[3].ai[3] );

    DataSource<CType>::shared_ptr constant =
        new ConstantDataSource<CType>(CType(true));
    DataSourceBase::shared_ptr constant_a = constant->getMember("a");
    BOOST_REQUIRE(constant_a);
    DataSourceBase::shared_ptr nested = constant_a->getMember("a");

    BOOST_REQUIRE(nested);
    DataSource<int>::shared_ptr readable = DataSource<int>::narrow(nested.get());
    BOOST_REQUIRE(readable);
    BOOST_CHECK_EQUAL(readable->get(), constant->get().a.a);
    BOOST_CHECK(!nested->isAssignable());
}

BOOST_AUTO_TEST_CASE( testReadOnlyArrayViewsCannotMutateSnapshots )
{
    Types()->addType( new StructTypeInfo<AType>("AType") );
    Types()->addType( new StructTypeInfo<BType>("BType") );

    DataSourceBase::shared_ptr boost_array_member;
    DataSourceBase::shared_ptr c_array_member;
    {
        DataSource<AType>::shared_ptr boost_array_parent =
            new ConstantDataSource<AType>(AType(true));
        DataSource<BType>::shared_ptr c_array_parent =
            new ConstantDataSource<BType>(BType(true));
        boost_array_member = boost_array_parent->getMember("ai");
        c_array_member = c_array_parent->getMember("ai");
    }

    DataSource<carray<int> >::shared_ptr readable_boost_array =
        DataSource<carray<int> >::narrow(boost_array_member.get());
    DataSource<carray<int> >::shared_ptr readable_c_array =
        DataSource<carray<int> >::narrow(c_array_member.get());
    BOOST_REQUIRE(readable_boost_array);
    BOOST_REQUIRE(readable_c_array);

    carray<int> boost_array_view = readable_boost_array->get();
    carray<int> c_array_view = readable_c_array->get();
    BOOST_REQUIRE(boost_array_view.address());
    BOOST_REQUIRE(c_array_view.address());
    boost_array_view.address()[3] = 7;
    c_array_view.address()[3] = 8;

    BOOST_CHECK_EQUAL(readable_boost_array->get().address()[3], 99);
    BOOST_CHECK_EQUAL(readable_c_array->get().address()[3], 99);
}

BOOST_AUTO_TEST_CASE( testReadOnlyArrayCloneKeepsCanonicalValue )
{
    Types()->addType( new StructTypeInfo<AType>("AType") );

    DataSource<AType>::shared_ptr parent =
        new ConstantDataSource<AType>(AType(true));
    DataSourceBase::shared_ptr member = parent->getMember("ai");
    DataSource<carray<int> >::shared_ptr readable =
        DataSource<carray<int> >::narrow(member.get());
    BOOST_REQUIRE(readable);

    carray<int> escaped = readable->get();
    BOOST_REQUIRE(escaped.address());
    escaped.address()[3] = 7;

    DataSourceBase::shared_ptr cloned_base(member->clone());
    DataSource<carray<int> >::shared_ptr cloned =
        DataSource<carray<int> >::narrow(cloned_base.get());
    BOOST_REQUIRE(cloned);
    parent.reset();
    member.reset();

    BOOST_CHECK_EQUAL(cloned->get().address()[3], 99);
    BOOST_CHECK(!cloned->isAssignable());
}

BOOST_AUTO_TEST_CASE( testReadOnlyMemberCopiesUseReplacementParent )
{
    AssignableDataSource<AType>::shared_ptr source =
        new ValueDataSource<AType>(AType(true));
    type_discovery discovery(source, false);
    DataSourceBase::shared_ptr source_scalar =
        discovery.discoverMember(source->set(), "a");

    type_discovery array_discovery(source, false);
    DataSourceBase::shared_ptr source_array =
        array_discovery.discoverMember(source->set(), "ai");
    BOOST_REQUIRE(source_scalar);
    BOOST_REQUIRE(source_array);

    AssignableDataSource<AType>::shared_ptr replacement =
        new ValueDataSource<AType>(AType(true));
    replacement->set().a = 42;
    replacement->set().ai[3] = 123;

    std::map<const DataSourceBase*, DataSourceBase*> replacements;
    replacements[source.get()] = replacement.get();
    DataSourceBase::shared_ptr copied_scalar(source_scalar->copy(replacements));
    DataSourceBase::shared_ptr copied_array(source_array->copy(replacements));

    DataSource<int>::shared_ptr readable_scalar =
        DataSource<int>::narrow(copied_scalar.get());
    DataSource<carray<int> >::shared_ptr readable_array =
        DataSource<carray<int> >::narrow(copied_array.get());
    BOOST_REQUIRE(readable_scalar);
    BOOST_REQUIRE(readable_array);
    BOOST_CHECK_EQUAL(readable_scalar->get(), 42);
    BOOST_CHECK_EQUAL(readable_array->get().address()[3], 123);

    carray<int> escaped = readable_array->get();
    BOOST_REQUIRE(escaped.address());
    escaped.address()[3] = 7;
    BOOST_CHECK_EQUAL(readable_array->get().address()[3], 123);
    BOOST_CHECK_EQUAL(replacement->get().ai[3], 123);
}

BOOST_AUTO_TEST_CASE( testReadOnlyArraySupportsCopyConstructionOnlyElements )
{
    CopyConstructibleArrayType source;
    DataSource<CopyConstructibleArrayType>::shared_ptr parent =
        new ConstReferenceDataSource<CopyConstructibleArrayType>(source);
    type_discovery discovery(parent, false);
    DataSourceBase::shared_ptr member =
        discovery.discoverMember(source, "values");

    DataSource<carray<CopyConstructibleArrayElement> >::shared_ptr readable =
        DataSource<carray<CopyConstructibleArrayElement> >::narrow(member.get());
    BOOST_REQUIRE(readable);
    BOOST_REQUIRE_EQUAL(readable->get().count(), 2);
    BOOST_CHECK_EQUAL(readable->get().address()[0].value, 11);

    carray<CopyConstructibleArrayElement> escaped = readable->get();
    BOOST_REQUIRE(escaped.address());
    escaped.address()[0].value = 99;

    BOOST_CHECK_EQUAL(readable->get().address()[0].value, 11);
    BOOST_CHECK_EQUAL(source.values[0].value, 11);
}

BOOST_AUTO_TEST_CASE( testReadOnlyArrayViewsKeepStableAddresses )
{
    CopyConstructibleArrayType source;
    DataSource<CopyConstructibleArrayType>::shared_ptr parent =
        new ConstReferenceDataSource<CopyConstructibleArrayType>(source);
    type_discovery discovery(parent, false);
    DataSourceBase::shared_ptr member =
        discovery.discoverMember(source, "values");
    DataSource<carray<CopyConstructibleArrayElement> >::shared_ptr readable =
        DataSource<carray<CopyConstructibleArrayElement> >::narrow(member.get());
    BOOST_REQUIRE(readable);

    carray<CopyConstructibleArrayElement> first = readable->get();
    BOOST_REQUIRE(first.address());
    CopyConstructibleArrayElement* stable_address = first.address();
    carray<CopyConstructibleArrayElement> second = readable->get();
    carray<CopyConstructibleArrayElement> from_value = readable->value();
    const carray<CopyConstructibleArrayElement>& from_rvalue =
        readable->rvalue();

    BOOST_REQUIRE_EQUAL(second.address(), stable_address);
    BOOST_REQUIRE_EQUAL(from_value.address(), stable_address);
    BOOST_REQUIRE_EQUAL(from_rvalue.address(), stable_address);
    BOOST_CHECK_EQUAL(first.address()[0].value, 11);

    first.address()[0].value = 99;
    carray<CopyConstructibleArrayElement> refreshed = readable->get();
    BOOST_REQUIRE_EQUAL(refreshed.address(), stable_address);
    BOOST_CHECK_EQUAL(first.address()[0].value, 11);
}

BOOST_AUTO_TEST_CASE( testReadOnlyBoolAndEmptyArraysRemainReadable )
{
    BoolArrayType bool_source;
    DataSource<BoolArrayType>::shared_ptr bool_parent =
        new ConstReferenceDataSource<BoolArrayType>(bool_source);
    type_discovery bool_discovery(bool_parent, false);
    DataSourceBase::shared_ptr bool_member =
        bool_discovery.discoverMember(bool_source, "values");
    DataSource<carray<bool> >::shared_ptr readable_bool =
        DataSource<carray<bool> >::narrow(bool_member.get());
    BOOST_REQUIRE(readable_bool);
    BOOST_REQUIRE_EQUAL(readable_bool->get().count(), 2);

    carray<bool> escaped_bool = readable_bool->get();
    BOOST_REQUIRE(escaped_bool.address());
    escaped_bool.address()[0] = false;
    BOOST_CHECK(readable_bool->get().address()[0]);
    BOOST_CHECK(bool_source.values[0]);

    EmptyArrayType empty_source;
    DataSource<EmptyArrayType>::shared_ptr empty_parent =
        new ConstReferenceDataSource<EmptyArrayType>(empty_source);
    type_discovery empty_discovery(empty_parent, false);
    DataSourceBase::shared_ptr empty_member =
        empty_discovery.discoverMember(empty_source, "values");
    DataSource<carray<int> >::shared_ptr readable_empty =
        DataSource<carray<int> >::narrow(empty_member.get());
    BOOST_REQUIRE(readable_empty);
    BOOST_CHECK_EQUAL(readable_empty->get().count(), 0);
    BOOST_CHECK(!readable_empty->get().address());
}

BOOST_AUTO_TEST_CASE( testNonCopyConstructibleArrayElementsRemainOpaque )
{
    OpaqueArrayType source;
    DataSource<OpaqueArrayType>::shared_ptr parent =
        new ConstReferenceDataSource<OpaqueArrayType>(source);
    type_discovery discovery(parent, false);

    BOOST_CHECK(!discovery.discoverMember(source, "values"));
}

BOOST_AUTO_TEST_CASE( testThrowingCopyOnlyArrayElementsRemainOpaque )
{
    ThrowingCopyOnlyArrayType source;
    DataSource<ThrowingCopyOnlyArrayType>::shared_ptr parent =
        new ConstReferenceDataSource<ThrowingCopyOnlyArrayType>(source);
    type_discovery discovery(parent, false);

    BOOST_CHECK(!discovery.discoverMember(source, "values"));
}

BOOST_AUTO_TEST_SUITE_END()
