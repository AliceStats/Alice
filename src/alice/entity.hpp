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
#include <mutex>

#include <boost/functional/hash.hpp>

#include <alice/netmessages.pb.h>
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
    /// Thrown when trying to access a non-existant property without a default value
    CREATE_EXCEPTION( entityUnkownProperty, "Property specified does not exist" )
    /// Thrown when trying to access a non-existant property (e.g. larger than the size of properties)
    CREATE_EXCEPTION( entityUnkownSendprop, "Property index out of range" )

    /// @}

    // forward declaration for bitstream
    class bitstream;
    class parser;

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
                D_( std::cout << "[entity_list] Reserving memory for " << num << " classes " << D_FILE << " " << __LINE__ << std::endl;, 1 )
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

    /** Contains information about the last updated fields for a specific entity. */
    struct entity_delta {
        /** ID of entity as stored in the gamestate */
        uint32_t entity_id;
        /** List of fields updated as stored in the entities recv- / flattables */
        std::vector<uint32_t> entity_fields;
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
        friend parser;
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
            typedef std::vector<property> map_type;
            /** Iterator type for underlying map */
            typedef map_type::iterator iterator;
            /** Value type for underlying map */
            typedef map_type::value_type value_type;
            /** Typedef for state enum */
            typedef state state_type;

            /** Default constructor, sets initialization status to false */
            entity() : initialized(false) {

            }

            /** Allow copying */
            entity(const entity& e) : initialized(e.initialized), id(e.id), cls(e.cls),
                flat(e.flat), properties(e.properties), currentState(e.currentState),
                stringIndex(e.stringIndex)
            {
                if (cls == nullptr || flat == nullptr)
                    initialized = false;
            }

            /** Allow moving */
            entity(entity&& e) : initialized(e.initialized), id(e.id), cls(e.cls),
                flat(e.flat), properties(std::move(e.properties)),
                currentState(e.currentState), stringIndex(std::move(e.stringIndex))
            {
                if (cls == nullptr || flat == nullptr)
                    initialized = false;

                e.initialized = false;
                e.cls = nullptr;
                e.flat = nullptr;
            }

            /** Default destructor */
            ~entity() = default;

            /** Assignment operator */
            entity& operator= (entity t) {
                swap(t);
                return *this;
            }

            /** Swap this entity with given one */
            void swap(entity& b) {
                std::swap(initialized, b.initialized);
                std::swap(id, b.id);
                std::swap(cls, b.cls);
                std::swap(flat, b.flat);
                std::swap(properties, b.properties);
                std::swap(currentState, b.currentState);
                std::swap(stringIndex, b.stringIndex);
            }

            /** Returns whether this entity has been initialized */
            bool isInitialized() {
                return initialized;
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
                buildIndex();

                auto it = stringIndex.find(needle);
                if (it == stringIndex.end()) {
                    return properties.end();
                } else {
                    return properties.begin() + it->second;
                }
            }

            /** Returns iterator pointing to the element request or one element behind the last if none can be found */
            inline iterator find(uint32_t index) {
                return properties.begin() + index;
            }

            /** Returns property value by name, throws if property doesn't exist */
            template <typename T>
            inline T prop(const std::string& needle) {
                buildIndex();

                auto it = stringIndex.find(needle);
                if (it == stringIndex.end()) {
                    BOOST_THROW_EXCEPTION(entityUnkownProperty()
                        << EArg<1>::info(needle)
                    );
                } else {
                    return properties[it->second].as<T>();
                }
            }

            /** Returns property value by recv table index, throws if property doesn't exist */
            template <typename T>
            inline T prop(uint32_t index) {
                if (properties.size() <= index) {
                    BOOST_THROW_EXCEPTION(entityUnkownProperty()
                        << (EArgT<1, uint32_t>::info(index))
                    );
                } else {
                    return properties[index].as<T>();
                }
            }

            /** Returns property value by name, returns default value if property doesn't exist */
            template <typename T>
            inline T prop(const std::string& needle, T def) {
                buildIndex();

                auto it = stringIndex.find(needle);
                if (it == stringIndex.end()) {
                    return def;
                } else {
                    return properties[it->second].as<T>();
                }
            }

            /** Returns the index of the specified property in the flattable */
            inline int32_t getPropIndex(const std::string& needle) {
                buildIndex();

                auto it = stringIndex.find(needle);
                if (it == stringIndex.end()) {
                    return -1;
                } else {
                    return it->second;
                }
            }

            /** Checks if a property exists by name */
            template <typename T>
            inline T hasProp(const std::string& needle) {
                buildIndex();
                return (stringIndex.find(needle) != stringIndex.end());
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

            /** Returns the flattened sendtable for this entity */
            inline const flatsendtable* getRecvTable() const {
                return flat;
            }

            /** Returns the last entity state. */
            inline state_type getState() const {
                return currentState;
            }

            /** Deletes all properties. */
            inline void clear() {
                properties.clear();
            }

            /** Prints a debug string containing all the properties and their values. */
            std::string DebugString();
        protected:
            /**
             * Constructor filling the initial state.
             *
             * @param id The id of the entity
             * @param cls Entity description according to it's type in the entity_list
             * @param flat Flattened sendtable, containing the correct order of properties
             */
            entity(uint32_t id, const entity_list::value_type &cls, const flatsendtable& flat)
                : initialized(true), id(id), cls(&cls), flat(&flat), currentState(state_created)
            {
                // Reserve memory for each possible property
                D_( std::cout << "[entity] Reserving memory for " << flat.properties.size()+1 << " props " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                properties.resize(flat.properties.size()+1);
            }

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

            /** Sets the entity state */
            inline void setState(state_type s) {
                currentState = s;

                // delete the id of the entity to be able to catch overwrites
                if (s == state_deleted)
                    id = -1;
            }

            /** Updates all the entities from the given bitstream. */
            void updateFromBitstream(bitstream& bstream, entity_delta* = nullptr);
            /** Skips each property in this entity */
            void skip(bitstream& bstream);

            /** Reads the entities header. */
            static void readHeader(uint32_t &id, bitstream &bstream, state_type &type);
        private:
            /** Whether this entity has been initialized */
            bool initialized;
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

            /** Builds the string index */
            void buildIndex() {
                if (stringIndex.empty())
                    for (uint32_t i = 0; i < properties.size(); ++i) {
                        if (properties[i].isInitialized())
                            stringIndex[properties[i].getName()] = i;
                    }
            }
    };

    /// @}
}

#endif /* _DOTA_ENTITY_HPP_ */
