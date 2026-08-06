/***************************************************************************
  tag: The SourceWorks  Tue Sep 7 00:55:18 CEST 2010  PartDataSource.hpp

                        PartDataSource.hpp -  description
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


#ifndef ORO_PARTDATASOURCE_HPP_
#define ORO_PARTDATASOURCE_HPP_

#include <cstddef>
#include <memory>

#include "DataSource.hpp"
#include "../types/carray.hpp"

namespace RTT
{
    namespace internal
    {
        /**
         * A DataSource which is used to manipulate a reference to a
         * part of a data source.
         *
         * It recalculates the reference of the part in case of copy/clone semantics.
         *
         * @param T The data type of an element.
         */
        template<typename T>
        class PartDataSource
        : public AssignableDataSource<T>
        {
            // a reference to a value_t
            typename AssignableDataSource<T>::reference_t mref;
            // parent data source, for updating after set().
            base::DataSourceBase::shared_ptr mparent;
        public:
            ~PartDataSource() {}

            typedef boost::intrusive_ptr<PartDataSource<T> > shared_ptr;

            /**
             * The part is constructed from a reference to a member variable of an existing struct.
             *
             * @param ref    Reference to the struct's variable.
             * @param parent A data source holding the struct.
             */
            PartDataSource( typename AssignableDataSource<T>::reference_t ref,
                            base::DataSourceBase::shared_ptr parent )
                : mref(ref), mparent(parent)
            {
            }

            typename DataSource<T>::result_t get() const
            {
                return mref;
            }

            typename DataSource<T>::result_t value() const
            {
                return mref;
            }

            void set( typename AssignableDataSource<T>::param_t t )
            {
                mref = t;
                updated();
            }

            typename AssignableDataSource<T>::reference_t set()
            {
                return mref;
            }

            typename AssignableDataSource<T>::const_reference_t rvalue() const
            {
                return mref;
            }

            void updated() {
                mparent->updated();
            }

            virtual PartDataSource<T>* clone() const {
                return new PartDataSource<T>(mref, mparent);
            }

            virtual PartDataSource<T>* copy( std::map<const base::DataSourceBase*, base::DataSourceBase*>& replace ) const {
                // if somehow a copy exists, return the copy, otherwise return this (see Attribute copy)
                if ( replace[this] != 0 ) {
                    assert ( dynamic_cast<PartDataSource<T>*>( replace[this] ) == static_cast<PartDataSource<T>*>( replace[this] ) );
                    return static_cast<PartDataSource<T>*>( replace[this] );
                }
                // Both this and mparent are copied, but the part must also copy reference mref from the new parent !
                assert( mparent->getRawPointer() != 0 && "Can't copy part of rvalue datasource.");
                if ( mparent->getRawPointer() == 0 )
                    throw std::runtime_error("PartDataSource.hpp: Can't copy part of rvalue datasource.");
                base::DataSourceBase::shared_ptr mparent_copy =  mparent->copy(replace);
                // calculate the pointer offset in parent:
                int offset = reinterpret_cast<unsigned char*>( &mref ) - reinterpret_cast<unsigned char*>(mparent->getRawPointer());
                // get the pointer to the new parent member:
                typename AssignableDataSource<T>::value_t* mref_copy = reinterpret_cast<typename AssignableDataSource<T>::value_t*>( reinterpret_cast<unsigned char*>(mparent_copy->getRawPointer()) + offset );
                replace[this] = new PartDataSource<T>( *mref_copy, mparent_copy );
                // returns copy
                return static_cast<PartDataSource<T>*>(replace[this]);

            }
        };

        template <typename T>
        class ReadOnlyPartDataSource : public DataSource<T>
        {
            typename DataSource<T>::value_t mvalue;
            base::DataSourceBase::shared_ptr mparent;
            std::ptrdiff_t moffset;

            static std::ptrdiff_t memberOffset(
                const void* member, base::DataSourceBase::shared_ptr parent)
            {
                const void* parent_value = parent->getRawConstPointer();
                assert(parent_value != 0 && "Read-only part requires a readable parent.");
                if (parent_value == 0) {
                    throw std::runtime_error(
                        "PartDataSource.hpp: Read-only part has no readable parent.");
                }
                return reinterpret_cast<const unsigned char*>(member) -
                       reinterpret_cast<const unsigned char*>(parent_value);
            }

            ReadOnlyPartDataSource(
                typename DataSource<T>::const_reference_t value,
                base::DataSourceBase::shared_ptr parent,
                std::ptrdiff_t offset)
                : mvalue(value), mparent(parent), moffset(offset) {}

        public:
            typedef boost::intrusive_ptr<ReadOnlyPartDataSource<T> > shared_ptr;

            ReadOnlyPartDataSource(
                typename DataSource<T>::const_reference_t value,
                base::DataSourceBase::shared_ptr parent)
                : mvalue(value), mparent(parent),
                  moffset(memberOffset(&value, parent)) {}

            typename DataSource<T>::result_t get() const { return mvalue; }
            typename DataSource<T>::result_t value() const { return mvalue; }
            typename DataSource<T>::const_reference_t rvalue() const { return mvalue; }

            ReadOnlyPartDataSource<T>* clone() const override
            {
                return new ReadOnlyPartDataSource<T>(mvalue, mparent, moffset);
            }

            ReadOnlyPartDataSource<T>* copy(
                std::map<const base::DataSourceBase*, base::DataSourceBase*>& replace)
                const override
            {
                if (replace[this] != 0) {
                    return static_cast<ReadOnlyPartDataSource<T>*>(replace[this]);
                }
                base::DataSourceBase::shared_ptr parent_copy =
                    mparent->copy(replace);
                const void* parent_value = parent_copy->getRawConstPointer();
                assert(parent_value != 0 && "Copied read-only part requires a readable parent.");
                if (parent_value == 0) {
                    throw std::runtime_error(
                        "PartDataSource.hpp: Copied read-only part has no readable parent.");
                }
                const typename DataSource<T>::value_t* value_copy =
                    reinterpret_cast<const typename DataSource<T>::value_t*>(
                        reinterpret_cast<const unsigned char*>(parent_value) + moffset);
                replace[this] =
                    new ReadOnlyPartDataSource<T>(*value_copy, parent_copy, moffset);
                return static_cast<ReadOnlyPartDataSource<T>*>(replace[this]);
            }
        };

        template <typename T>
        class ReadOnlyPartDataSource< types::carray<T> >
            : public DataSource< types::carray<T> >
        {
            std::size_t mcount;
            std::unique_ptr<T[]> mvalue;
            mutable std::unique_ptr<T[]> mpresentation;
            mutable types::carray<T> mview;
            base::DataSourceBase::shared_ptr mparent;
            std::ptrdiff_t moffset;

            static std::ptrdiff_t memberOffset(
                types::carray<T> value,
                base::DataSourceBase::shared_ptr parent)
            {
                if (value.count() == 0) {
                    return 0;
                }
                const void* parent_value = parent->getRawConstPointer();
                assert(parent_value != 0 && "Read-only array part requires a readable parent.");
                if (parent_value == 0) {
                    throw std::runtime_error(
                        "PartDataSource.hpp: Read-only array part has no readable parent.");
                }
                return reinterpret_cast<const unsigned char*>(value.address()) -
                       reinterpret_cast<const unsigned char*>(parent_value);
            }

            ReadOnlyPartDataSource(
                const T* value, std::size_t count,
                base::DataSourceBase::shared_ptr parent,
                std::ptrdiff_t offset)
                : mcount(count), mvalue(count ? new T[count] : 0),
                  mpresentation(count ? new T[count] : 0), mview(),
                  mparent(parent), moffset(offset)
            {
                for (std::size_t i = 0; i != count; ++i) {
                    mvalue[i] = value[i];
                }
            }

            void refreshView() const
            {
                for (std::size_t i = 0; i != mcount; ++i) {
                    mpresentation[i] = mvalue[i];
                }
                mview.init(mpresentation.get(), mcount);
            }

        public:
            typedef boost::intrusive_ptr<
                ReadOnlyPartDataSource< types::carray<T> > > shared_ptr;

            ReadOnlyPartDataSource(
                typename DataSource< types::carray<T> >::const_reference_t value,
                base::DataSourceBase::shared_ptr parent)
                : mcount(value.count()),
                  mvalue(mcount ? new T[mcount] : 0),
                  mpresentation(mcount ? new T[mcount] : 0), mview(),
                  mparent(parent), moffset(memberOffset(value, parent))
            {
                for (std::size_t i = 0; i != value.count(); ++i) {
                    mvalue[i] = value.address()[i];
                }
            }

            types::carray<T> get() const
            {
                refreshView();
                return mview;
            }

            types::carray<T> value() const
            {
                refreshView();
                return mview;
            }

            types::carray<T> const& rvalue() const
            {
                refreshView();
                return mview;
            }

            ReadOnlyPartDataSource* clone() const override
            {
                return new ReadOnlyPartDataSource(
                    mvalue.get(), mcount, mparent, moffset);
            }

            ReadOnlyPartDataSource* copy(
                std::map<const base::DataSourceBase*, base::DataSourceBase*>& replace)
                const override
            {
                if (replace[this] != 0) {
                    return static_cast<ReadOnlyPartDataSource*>(replace[this]);
                }
                base::DataSourceBase::shared_ptr parent_copy =
                    mparent->copy(replace);
                const void* parent_value = parent_copy->getRawConstPointer();
                assert(parent_value != 0 && "Copied read-only array part requires a readable parent.");
                if (parent_value == 0) {
                    throw std::runtime_error(
                        "PartDataSource.hpp: Copied read-only array part has no readable parent.");
                }
                const T* value_copy = reinterpret_cast<const T*>(
                    reinterpret_cast<const unsigned char*>(parent_value) + moffset);
                replace[this] = new ReadOnlyPartDataSource(
                    value_copy, mcount, parent_copy, moffset);
                return static_cast<ReadOnlyPartDataSource*>(replace[this]);
            }
        };

        /**
         * Partial specialisation of PartDataSource for carray<T> types.
         */
        template<typename T>
        class PartDataSource< types::carray<T> >
        : public AssignableDataSource< types::carray<T> >
        {
            // keeps ref to real array.
            types::carray<T> mref;
            // parent data source, for updating after set().
            base::DataSourceBase::shared_ptr mparent;
        public:
            ~PartDataSource() {}

            typedef boost::intrusive_ptr<PartDataSource<T> > shared_ptr;

            /**
             * The part is constructed from a reference to a member variable of an existing struct.
             *
             * @param ref    Reference to the struct's variable.
             * @param parent A data source holding the struct.
             */
            PartDataSource( types::carray<T> ref,
                            base::DataSourceBase::shared_ptr parent )
                : mref(ref), mparent(parent)
            {
            }

            types::carray<T> get() const
            {
                return mref;
            }

            types::carray<T> value() const
            {
                return mref;
            }

            void set( typename AssignableDataSource< types::carray<T> >::param_t t )
            {
                mref = t;
                updated();
            }

            types::carray<T>& set()
            {
                return mref;
            }

            types::carray<T> const& rvalue() const
            {
                return mref;
            }

            void updated() {
                mparent->updated();
            }

            virtual PartDataSource* clone() const {
                return new PartDataSource(mref, mparent);
            }

            virtual PartDataSource* copy( std::map<const base::DataSourceBase*, base::DataSourceBase*>& replace ) const {
                // if somehow a copy exists, return the copy, otherwise return this (see Attribute copy)
                if ( replace[this] != 0 ) {
                    assert ( dynamic_cast<PartDataSource*>( replace[this] ) == static_cast<PartDataSource*>( replace[this] ) );
                    return static_cast<PartDataSource*>( replace[this] );
                }
                // Both this and mparent are copied, but the part must also copy reference mref from the new parent !
                assert( mparent->getRawPointer() != 0 && "Can't copy part of rvalue datasource.");
                if ( mparent->getRawPointer() == 0 )
                    throw std::runtime_error("PartDataSource.hpp: Can't copy part of rvalue datasource.");
                base::DataSourceBase::shared_ptr mparent_copy =  mparent->copy(replace);
                // calculate the pointer offset in parent:
                int offset = reinterpret_cast<unsigned char*>( mref.address() ) - reinterpret_cast<unsigned char*>(mparent->getRawPointer());
                // get the pointer to the new parent member:
                types::carray<T> mref_copy;
                mref_copy.init(reinterpret_cast<T*>( reinterpret_cast<unsigned char*>(mparent_copy->getRawPointer()) + offset ), mref.count() );
                replace[this] = new PartDataSource(mref_copy, mparent_copy);
                // return copy.
                return static_cast<PartDataSource*>(replace[this]);

            }
        };
    }
}


#endif /* ORO_PARTDATASOURCE_HPP_ */
