/**
 * @file sendtable.hpp
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

#ifndef _DOTA_SENDTABLE_HPP_
#define _DOTA_SENDTABLE_HPP_

#include <string>
#include <vector>

#include <alice/multiindex.hpp>
#include <alice/exception.hpp>
#include <alice/sendprop.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when an unkown stringtable is being accessed
    CREATE_EXCEPTION( sendtableUnkownTable, "Trying to access unkown table." )
    /// Thrown when an unkown property is being accesed directly
    CREATE_EXCEPTION( sendtableUnkownProperty, "Trying to access unkown property." )

    /// @}

    /// @defgroup CORE Core
    /// @{

    /** Structure for keeping track of properties when building the hierarchy */
    struct dt_hiera {
        /** Pointer to sendprop */
        sendprop* prop;
        /** Name based on current level */
        std::string name;
    };

    /**
     * Structure for a flattened sendtable, includes it's name an correct property order.
     *
     * A flattable is a representation of an entity as it is send over the network. There might be
     * parts of the entity that are stripped because they are not required or included in it's
     * parent sendtable.
     */
    struct flatsendtable {
        /** Name of the sendtable */
        std::string name;
        /** Correct network property order and their corresponding hierarchy name */
        std::vector<dt_hiera> properties;
    };

    /**
     * Definition for a single sendtable, provides means to generate a network representation (flattable of it).
     *
     * The sendtable includes a number of property definitions. In order to read entities relating to this sendtable,
     * a flat- / recvtable is nessecary.
     */
    class sendtable {
        public:
            /** Type for the underlying map of properties */
            typedef multiindex<std::string, int32_t, sendprop*> propmap;
            /** Size type of underlying map */
            typedef propmap::size_type size_type;
            /** Value type of underlying map */
            typedef propmap::value_type value_type;
            /** Ordered iterator type forwarded from multiindex */
            typedef propmap::index_iterator iterator;

            /** Creates a new sendtable from data provided */
            sendtable(std::string name, bool decodable) : name(std::move(name)), decodable(std::move(decodable)), counter(-1) {

            }

            /** Default copy constructor */
            sendtable(const sendtable&) = default;

            /** Default destructor */
            ~sendtable() = default;

            /** Dealocates memory for all sendproperties */
            inline void free() const {
                // About 1MB of memory get's freed here
                for (auto &p : properties) {
                    delete p.value;
                }
            }

            /** Returns iterator pointed at the beginning of the sendprops stored s */
            inline iterator begin() const {
                return properties.beginIndex();
            }

            /** Returns iterator pointed at the end of the sendprops stored  */
            inline iterator end() const {
                return properties.endIndex();
            }

            /** Return size of map */
            inline size_type size() const {
                return properties.size();
            }

            /** Returns name of sendtable */
            inline const std::string& getName() const {
                return name;
            }

            /** Returns whether this table is decodable or not */
            inline const bool isDecodable() const {
                return decodable;
            }

            /** Adds a new property to the table */
            inline void insert(sendprop *p) const {
                properties.insert(propmap::entry_type{p->getName(), ++counter, p});
            }

             /** Returns property by name */
            inline sendprop* get(const std::string &name) const {
                // check if the key exists
                auto it = properties.findKey(name);
                if (it == properties.end())
                    BOOST_THROW_EXCEPTION( sendtableUnkownProperty()
                        << EArg<1>::info(name)
                    );

                return it->value;
            }
        private:
            /** Name of the table */
            const std::string name;
            /** Whether the table should be decoded or not */
            const bool decodable;
            /** Internal property counter to preserve order of insertion */
            mutable int32_t counter;
            /** Mapping property names to their corresponding objects */
            mutable propmap properties;
    };

    /// @}
}

#endif /* _DOTA_SENDTABLE_HPP_ */