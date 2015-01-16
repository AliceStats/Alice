/**
 * @file stringtable.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.1
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
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#ifndef _DOTA_STRINGTABLE_HPP_
#define _DOTA_STRINGTABLE_HPP_

#include <string>
#include <unordered_map>

#include <climits>

#include <alice/netmessages.pb.h>
#include <alice/exception.hpp>
#include <alice/multiindex.hpp>

/// Defines the maximum number of keys to keep a history of
///
/// Stringtables may require you to keep track of a number of recently added keys.
/// A substring of those is prepended to new keys in order to save bandwidth.
#define STRINGTABLE_KEY_HISTORY 32

/// Maximum length of a stringtable Key
///
/// This value is just an estimate.
#define STRINGTABLE_MAX_KEY_SIZE 0x400      // 1024

/// Maximum size of a stringtable Value
#define STRINGTABLE_MAX_VALUE_SIZE 0x4000   // 16384

/// The baselinetable, required for parsing entities
///
/// This is a special table which contains the default values for
/// a number of entities. It's used to generate a default state for said
/// entity in order to save bandwith. The baselineinstance maybe updated in order
/// to reflect state changes in those entities.
#define BASELINETABLE "instancebaseline"

namespace dota {
    namespace detail {
        /** Helper that pops the first element off a vector */
        template<typename T>
        void pop_front(std::vector<T>& vec) {
            assert(!vec.empty());
            vec.erase(vec.begin());
        }
    }

    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when trying to access a stringtable directly via an invalid / non-existent key
    CREATE_EXCEPTION( stringtableUnkownKey, "Trying to access stringtable via invalid key." )
    /// Thrown when trying to access an unkown index
    CREATE_EXCEPTION( stringtableUnkownIndex, "Trying to access stringtable via invalid index." )
    /// Thrown when a stringtable is marked for a full update but doesn't provide a key
    CREATE_EXCEPTION( stringtableKeyMissing, "Stringtable key missing in full update." )
    /// Thrown when using the key history with a malformed string
    CREATE_EXCEPTION( stringtableMalformedSubstring, "Trying to access recent keys with invalid specs." )
    /// Thrown when reading a value would cause an overflow
    CREATE_EXCEPTION( stringtableValueOverflow, "Trying to read large stringtable value." )

    /// @}

    // forward declaration
    class bitstream;

    /// @defgroup CORE Core
    /// @{

    /**
     * Networked stringtable containing a set of keys and values.
     *
     * Stringtables seem to be used for multiple purposes in Source. They are basically values
     * that the client needs which are not directly tied to properties and might not change very often,
     * hence why they can be globaly accessed in the stringtable.
     *
     * These might be implemented in a different way in the source engine to provide facilities for
     * writing and direct table access. We currently only know the meaning of a few tables, though this might
     * change in the future.
     */
    class stringtable {
        public:
            /** Type of multiindex container */
            typedef multiindex<std::string, int32_t, std::string> storage;
            /** Size type of said container */
            typedef storage::size_type size_type;
            /** Value type */
            typedef storage::value_type value_type;
            /** Type for an ordered iterator */
            typedef storage::index_iterator iterator;

            /** Constructor, initializes table from protobuf object */
            stringtable(CSVCMsg_CreateStringTable* table);

            /** Returns iterator pointed at the beginning of the stringtable entries */
            inline iterator begin() const {
                return db.beginIndex();
            }

            /**  Returns iterator pointed at the end of the stringtable entries */
            inline iterator end() const {
                return db.endIndex();
            }

            /** Return number of elements stored in the stringtable */
            inline size_type size() const {
                return db.size();
            }

            /** Set value of key directly */
            inline void set(const std::string& key, std::string value) const {
                if (db.findKey(key) == db.end()) {
                    assert(db.size() < INT_MAX); // check that this does not overflow when casting to int
                    db.insert({key, static_cast<int32_t>(db.size()), value});
                } else {
                    db.set(key, std::move(value));
                }
            }

            /** Get element value by key */
            inline const value_type& get(const std::string& key) const  {
                auto it = db.findKey(key);
                if (it == db.end())
                    BOOST_THROW_EXCEPTION( stringtableUnkownKey()
                        << EArg<1>::info(key)
                    );

                return it->value;
            }

            /** Get element value by index */
            inline const value_type& get(const int32_t& index) const {
                auto it = db.findIndex(index);
                if (it == db.endIndex())
                    BOOST_THROW_EXCEPTION( stringtableUnkownIndex()
                        << (EArgT<1, int32_t>::info(index))
                    );

                return it->value;
            }

            /** Get key by index */
            inline storage::key_type getKey(const int32_t& index) const {
                auto it = db.findIndex(index);
                if (it == db.endIndex())
                    BOOST_THROW_EXCEPTION( stringtableUnkownIndex()
                        << (EArgT<1, int32_t>::info(index))
                    );

                return it->key;
            }

            /** Returns name of this stringtable */
            inline const std::string& getName() const {
                return name;
            }

            /** Returns maximum number of entries for this table */
            inline const uint32_t& getMaxEntries() const {
                return maxEntries;
            }

            /** Returns whether the data read from updates has a fixed size */
            inline const bool isSizeFixed() const {
                return userDataFixed;
            }

            /** Returns data size in bits */
            inline const uint32_t& getDataBits() const {
                return userDataSizeBits;
            }

            /** Returns flags for this table */
            inline const int32_t& getFlags() const {
                return flags;
            }

            /** Update table from protobuf */
            inline void update(CSVCMsg_UpdateStringTable* table) const {
                update(table->num_changed_entries(), table->string_data());
            }
        private:
            /** Name of this stringtable */
            const std::string name;
            /** Maximum number of entries */
            const uint32_t maxEntries;
            /** Whether the data read from updates has a fixed size */
            const bool userDataFixed;
            /** Size of data in bytes */
            const uint32_t userDataSize;
            /** Size of data in bits */
            const uint32_t userDataSizeBits;
            /** Flags for this table */
            const int32_t flags;

            /** List of stringtable entries */
            mutable storage db;

            /** Update table from raw data */
            void update(const uint32_t& entries, const std::string& data) const;
    };


    /// @}
}

#endif /* _DOTA_STRINGTABLES_HPP_ */
