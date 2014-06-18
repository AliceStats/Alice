/**
 * @file stringtable.cpp
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

#include <cmath>

#include <alice/config.hpp>
#include <alice/bitstream.hpp>
#include <alice/stringtable.hpp>

namespace dota {
    stringtable::stringtable(CSVCMsg_CreateStringTable* table) :
        name(table->name()), maxEntries(table->max_entries()),
        userDataFixed(table->user_data_fixed_size()),
        userDataSize(table->user_data_size()),
        userDataSizeBits(table->user_data_size_bits()),
        flags(table->flags())
    {
        update(table->num_entries(), table->string_data());
    }

    void stringtable::update(const uint32_t& entries, const std::string& data) const {
        // create bitstream for data field
        bitstream bstream(data);

        // if true, list contains no names only updates
        const uint32_t full = bstream.read(1);

        // index for consecutive incrementing
        int32_t index = -1;

        // key history for key deltas
        std::vector<std::string> keys;

        D_( std::cout << "[stringtable] Updating " << name << " " << D_FILE << " " << __LINE__ << std::endl;, 2 )

        // read all the entries in the string table
        for (uint32_t i = 0; i < entries; ++i) {
            char key[STRINGTABLE_MAX_KEY_SIZE] = {'\0'};
            char value[STRINGTABLE_MAX_VALUE_SIZE] = {'\0'};

            const bool increment = bstream.read(1);
            if (increment) {
                ++index;
            } else {
                D_( std::cout << "[stringtable] Read stringtable index " << index << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                index = bstream.read(std::ceil(log2(maxEntries)));
            }

            // read name
            const bool hasName = bstream.read(1);
            if (hasName) {
                if (full && bstream.read(1)) {
                    // this should never happen because we cant reference the entry from this point onwards
                    BOOST_THROW_EXCEPTION( stringtableKeyMissing() );
                    return;
                }

                // check for key delta
                const bool substring = bstream.read(1);
                if (substring) {
                    // read substring
                    const uint32_t sIndex = bstream.read(5);  // index of substr in keyhistory
                    const uint32_t sLength = bstream.read(5); // prefix length to new key

                    if (sIndex >= STRINGTABLE_KEY_HISTORY || sLength >= STRINGTABLE_MAX_KEY_SIZE)
                        BOOST_THROW_EXCEPTION( stringtableMalformedSubstring()
                            << (EArgT<1, uint32_t>::info(sIndex))
                            << (EArgT<1, uint32_t>::info(sLength))
                        );

                    if (keys.size() <= sIndex) {
                        D_( std::cout << "[stringtable] Ignoring invalid history index. " << " " << D_FILE << " " << __LINE__ << std::endl;, 1 )
                        bstream.nReadString(reinterpret_cast<char*>(&key), STRINGTABLE_MAX_KEY_SIZE);
                    } else {
                        keys[sIndex].copy(key, sLength, 0);
                        bstream.nReadString(key + sLength, STRINGTABLE_MAX_KEY_SIZE - sLength);
                    }
                } else {
                    bstream.nReadString(reinterpret_cast<char*>(&key), STRINGTABLE_MAX_KEY_SIZE);
                }

                // check the key history
                if (keys.size() >= STRINGTABLE_KEY_HISTORY)
                    detail::pop_front(keys);

                D_( std::cout << "[stringtable] Read stringtable key " << key << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                keys.push_back(std::string(key));
            }

            // read value
            const bool hasValue = bstream.read(1);
            uint32_t length = 0;
            if (hasValue) {
                uint32_t valsize = 0;
                if (userDataFixed) {
                    length = userDataSize;
                    valsize = userDataSizeBits;
                } else {
                    length = bstream.read(14);
                    valsize = length * 8;
                }

                if (length > STRINGTABLE_MAX_VALUE_SIZE)
                    BOOST_THROW_EXCEPTION( stringtableValueOverflow()
                        << (EArgT<1, uint32_t>::info(length))
                    );

                D_( std::cout << "[stringtable] Read stringtable value " << value << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                bstream.readBits((char*)&value, valsize);
            }

            // insert entry
            std::string k(key);
            std::string v(value, length);

            if (hasName && db.hasKey(k)) {
                db.set(std::move(k), std::move(v));
            } else if (hasName) {
                db.insert(storage::entry_type{std::move(k), index, std::move(v)});
            } else if (hasValue && db.hasIndex(index)) {
                db.set(index, std::move(v));
            } else {
                D_( std::cout << "[stringtable] inserting anonymous stringtable value " << D_FILE << " " << __LINE__ << std::endl;, 2 )
                db.insert(storage::entry_type{"anonymous", index, std::move(v)});
            }
        }
    }
}
