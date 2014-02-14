/**
 * @file entity.hpp
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

#ifndef _DOTA_ENTITY_HPP_
#define _DOTA_ENTITY_HPP_

#include <unordered_map>
#include <string>
#include <utility>
#include <sstream>

#include <boost/functional/hash.hpp>

#include <alice/netmessages.pb.h>
#include <alice/bitstream.hpp>
#include <alice/exception.hpp>
#include <alice/sendtable.hpp>
#include <alice/property.hpp>

/// Defines the maximum number of concurrent entities
#define DOTA_MAX_ENTITIES 0x3FFF // 16383

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when trying to access an unkown entity class definition
    CREATE_EXCEPTION( entityUnkownClassIndex, "Can't set / get specified index (out of range)" )
    /// Thrown when the given entity id exceedes entity_list::reserve
    CREATE_EXCEPTION( entityIdToLarge, "Entity ID supplied is to large" )

    /// @}

    /// @defgroup CORE Core
    /// @{

    /** This struct contains the id, name and networkname of a single entity. */
    struct entity_description {
        /** Id */
        int32_t id;
        /** Name */
        std::string name;
        /** Network Name */
        std::string networkName;
    };

    /** This class stores a list of entity descriptions */
    class entity_list {
        public:
            /** Type for stored entity information */
            typedef entity_description value_type;
            /** Type for a map of entity id's and their counterparts */
            typedef std::unordered_map<uint32_t, value_type, boost::hash<uint32_t>> map_type;
            /** Size for map_type */
            typedef map_type::size_type size_type;
            /** Iterator for map_type */
            typedef map_type::iterator iterator;

            /** Returns iterator pointing to the beginning of the class list */
            inline iterator begin() const {
                return classes.begin();
            }

            /** Returns iterator pointing to the end of the class list */
            inline iterator end() const {
                return classes.end();
            }

            /** Returns iterator pointing to the element request or one element behind the last if none can be found */
            inline iterator find(const uint32_t& needle) {
                return classes.find(needle);
            }

            /**
             * Preallocate memory for a fixed number of entities.
             *
             * Calling this function sets the maximum number of entities to the value provided.
             */
            inline void reserve(size_type num) const {
                classes.reserve(num);
                maxEntities = num;
            }

            /** Returns current number of class entries */
            inline size_type size() const {
                return classes.size();
            }

            /** Removes all entries stored */
            inline void clear() const {
                classes.clear();
            }

            /** Inserts entity information for given ID */
            inline void set(uint32_t index, value_type c) const {
                if (index >= maxEntities)
                    BOOST_THROW_EXCEPTION(entityUnkownClassIndex()
                        << (EArgT<1, size_type>::info(index))
                        << (EArgT<2, size_type>::info(classes.size()))
                    );

                classes.emplace(std::make_pair(index, std::move(c)));
            }

            /** Returns the entity information stored for the given index */
            inline const value_type& get(uint32_t index) const {
                if (index >= maxEntities)
                    BOOST_THROW_EXCEPTION(entityUnkownClassIndex()
                        << (EArgT<1, size_type>::info(index))
                        << (EArgT<2, size_type>::info(classes.size()))
                    );

                return classes[index];
            }
        private:
            /** Contains a list of all possible entity classes */
            mutable map_type classes;

            /**
             * The maximum number of entities stored.
             *
             * This variable is set by calling the reserve method.
             */
            mutable size_type maxEntities;
    };

    /**
     * Represents a single entity send over the network including it's properties.
     *
     * Entities are not unique, each one can appear an unlimited number of times based on it's usage.
     * Some of these may related to each other. An example of this would be a player who is tied to the
     * hero he picks. Because different parts of the engine may need linked entities in different contexts,
     * these do not nessecary inherit from each other. Other entities however don't appear on their own but
     * are only used to provide properties required by all entities of a kind.
     *
     * Everytime an entity is updated, the handler is being invoked with the last known state of the entity to
     * provide means to save the last properties. Changed properties are currently not marked as such.
     */
    class entity {
        public:
            /** Different possible entity states. */
            enum state {
                state_default = 0,  // can happen but doesnt say anything in paticular about what happened
                state_created,      // entity has just be created
                state_overwritten,  // entity has been overwritten
                state_updated,      // entity has been updated
                state_deleted       // entity has been deleted
            };

            /** Type for a list of properties accessed by their field id. */
            typedef std::unordered_map<uint32_t, property, boost::hash<uint32_t>> map_type;
            /** Iterator type for underlying map */
            typedef map_type::iterator iterator;
            /** Value type for underlying map */
            typedef map_type::value_type value_type;
            /** Typedef for state enum */
            typedef state state_type;

            /**
             * Constructor filling the initial state.
             *
             * @param id The id of the entity
             * @param cls Entity description according to it's type in the entity_list
             * @param flat Flattened sendtable, containing the correct order of properties
             */
            entity(uint32_t id, const entity_list::value_type &cls, const flatsendtable& flat) : id(id),
                cls(&cls), flat(&flat), currentState(state_created) {}

            /** Default constructor */
            ~entity() = default;

            /**
             * Updates the entity with given values.
             *
             * @param id The id of the entity
             * @param cls Entity description according to it's type in the entity_list
             * @param flat Flattened sendtable, containing the correct order of properties
             */
            inline void update(uint32_t id, const entity_list::value_type &cls, const flatsendtable &flat) {
                this->id = id;
                this->cls = &cls;
                this->flat = &flat;
            }

            /** Returns iterator pointing to the beginning of the property list, access is unordered */
            inline iterator begin() {
                return properties.begin();
            }

            /** Returns iterator pointing to the end of the property list, access is unordered */
            inline iterator end() {
                return properties.end();
            }

            /**
             * Returns iterator pointing to the element request or one element behind the last if none can be found
             *
             * The first time this function is called a search index is build to cache the ID -> Name relation.
             * This is only nessecary ones and happens automatically. Subsequent access will be a lot faster.
             */
            inline iterator find(const std::string& needle) {
                if (stringIndex.empty())
                    for (auto &it : properties) {
                        stringIndex[it.second.getName()] = it.first;
                    }

                auto it = stringIndex.find(needle);
                if (it == stringIndex.end()) {
                    return properties.end();
                } else {
                    return properties.find(it->second);
                }
            }

            /** Returns iterator pointing to the element request or one element behind the last if none can be found */
            inline iterator find(uint32_t index) {
                return properties.find(index);
            }

            /** Returns this entities ID */
            inline uint32_t getId() const {
                return id;
            }

            /** Returns the numeric ID for the definition of this entity */
            inline uint32_t getClassId() const {
                return cls->id;
            }

            /** Returns the network name referenced in the entity description. */
            inline const std::string& getClassName() const {
                return cls->networkName;
            }

            /** Returns the last entity state. */
            inline state_type getState() const {
                return currentState;
            }

            /** Sets the entity state */
            inline void setState(state_type s) {
                currentState = s;

                // delete the id of the entity to be able to catch overwrites
                if (s == state_deleted)
                    id = -1;
            }

            /** Deletes all properties. */
            inline void clear() {
                properties.clear();
            }

            /** Updates all the entities from the given bitstream. */
            void updateFromBitstream(bitstream& bstream);

            /** Prints a debug string containing all the properties and their values. */
            std::string DebugString();

            /** Reads the entities header. */
            static void readHeader(uint32_t &id, bitstream &bstream, state_type &type);
        private:
            /** Entity Id */
            uint32_t id;
            /** Information / Name for this entity. */
            const entity_list::value_type *cls;
            /** Flattable containing the properties for this entity. */
            const flatsendtable *flat;
            /** A list of properties with their values. */
            map_type properties;
            /** Last set entity state. */
            state_type currentState;
            /** Search index to allow getting properties by their respective name. */
            std::unordered_map<std::string, uint32_t, boost::hash<std::string>> stringIndex;
    };

    /// @}
}

#endif /* _DOTA_ENTITY_HPP_ */