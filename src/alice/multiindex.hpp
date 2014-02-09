/**
 * @file multiindex.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.0
 *
 * @par License
 *    Alice Replay Parser
 *    Copyright 2014 Robin Dietrich
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#ifndef _DOTA_MULTIINDEX_HPP_
#define _DOTA_MULTIINDEX_HPP_

#include <utility>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <alice/exception.hpp>

namespace dota {
    // omit unnessecary typing
    namespace bm = boost::multi_index;

    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown we an unkown property is accessed
    CREATE_EXCEPTION( multiindexUnkownProperty, "Trying to set value on unkown property." )

    /// @}
    /// @defgroup CORE Core
    /// @{

    /** Ordered multi-index container, specific proxy for boost's multi_index template */
    template <typename Key, typename Index, typename Value>
    class multiindex {
        public:
            /** Struct for a single entry */
            struct entry {
                /** Key, unordered, non-unique */
                Key key;
                /** Index, ordered, unique */
                Index index;
                /** Value */
                Value value;
            };

            /** Type for a single entry */
            typedef entry entry_type;

            /** Type for the underlying container */
            typedef bm::multi_index_container <
                entry_type, // element to store
                bm::indexed_by <
                    bm::hashed_non_unique <
                        bm::member < entry_type, Key, &entry_type::key > // key
                    >,
                    bm::ordered_unique <
                        bm::member < entry_type, Index, &entry_type::index >  // index
                    >
                >
            > multiindexdb;

            /** Size type of the container */
            typedef typename multiindexdb::size_type size_type;
            /** Type of Key */
            typedef Key key_type;
            /** Type of Index */
            typedef Index index_type;
            /** Tpe of Value */
            typedef Value value_type;

            /** List of keys */
            typedef typename multiindexdb::template nth_index<0>::type key_set;
            /** Iterator for access by key */
            typedef typename key_set::iterator key_iterator;
            /** List of indexes, ordered */
            typedef typename multiindexdb::template nth_index<1>::type index_set;
            /** Iterator for access by index */
            typedef typename index_set::iterator index_iterator;

            /** Returns iterator pointing at the beginning of the key set */
            inline key_iterator begin() const {
                return db.template get<0>().begin();
            }

            /** Returns iterator pointing at the end of the key set */
            inline key_iterator end() const {
                return db.template get<0>().end();
            }

            /** Whether a key exists */
            inline bool hasKey(const Key &k) const {
                return db.template get<0>().find(k) != db.template get<0>().end();
            }

            /** Searches for a value by Key */
            inline key_iterator findKey(const Key &k) const {
                return db.template get<0>().find(k);
            }

            /**
             * Sets the value for a specific key
             *
             * This function only sets the value for the first key found.
             * Because the iteration is unordered, using this functions for
             * non-unique keys may lead to inconsisten behavior.
             */
            inline void set(const Key &k, Value &&v) const {
                key_set &index = db.template get<0>();
                auto it = index.find(k);

                if (it == index.end())
                    BOOST_THROW_EXCEPTION( multiindexUnkownProperty()
                        << (typename EArgT<1, Key>::info(k))
                    );

                index.modify(it, change_value(std::move(v)));
            }

            /** Returns iterator pointing at the beginning of the index set */
            inline index_iterator beginIndex() const {
                return db.template get<1>().begin();
            }

            /** Returns iterator pointing at the end of the index set */
            inline index_iterator endIndex() const {
                return db.template get<1>().end();
            }

            /** Whether an index exists */
            inline bool hasIndex(const Index &k) const {
                return db.template get<1>().find(k) != db.template get<1>().end();
            }

            /** Searches for a value by Index */
            inline index_iterator findIndex(const Index &k) const {
                return db.template get<1>().find(k);
            }

            /** Sets the value for a specific Index */
            inline void set(const Index &i, Value &&v) const {
                index_set &index = db.template get<1>();
                auto it = index.find(i);

                if (it == index.end())
                    BOOST_THROW_EXCEPTION( multiindexUnkownProperty()
                        << (typename EArgT<1, Index>::info(i))
                    );

                index.modify(it, change_value(std::move(v)));
            }

            /** Insert a new entry into the container */
            inline void insert(entry_type &&e) const {
                db.insert(std::move(e));
            }

            /** Return number of entries */
            inline size_type size() const {
                return db.size();
            }

            /** Removes any values stored, does not free memory if Value is a pointer */
            inline void clear() const {
                db.clear();
            }
        private:
            /** Internal container */
            mutable multiindexdb db;

            /** Helper struct to change the value efficiently */
            struct change_value {
                public:
                    // move if possible
                    change_value(Value &&v) : v(std::move(v)) {}
                    // perform the change
                    void operator()(entry_type& e) { e.value = std::move(v); }
                private:
                    Value v;
            };
    };

    /// @}
}

#endif /* _DOTA_MULTIINDEX_HPP_ */