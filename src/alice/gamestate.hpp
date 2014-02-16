/**
 * @file gamestate.hpp
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

#ifndef _DOTA_GAMESTATE_HPP_
#define _DOTA_GAMESTATE_HPP_

#include <string>
#include <memory>
#include <vector>
#include <set>

#include <alice/demo.pb.h>
#include <alice/netmessages.pb.h>
#include <alice/multiindex.hpp>
#include <alice/stringtable.hpp>
#include <alice/sendtable.hpp>
#include <alice/entity.hpp>
#include <alice/handler.hpp>
#include <alice/exception.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when a property is defined as an Array but it's subtype hasn't been set
    CREATE_EXCEPTION( gamestateInvalidArrayProp, "Array property has no previous member to define state." )

    /// Thrown when the baselinceinstance table is not available
    CREATE_EXCEPTION( gamestateBaselineNotFound, "Unable to find baseline instance." )

    /// Thrown when an invalid entity is specified in an update or delete
    CREATE_EXCEPTION( gamestateInvalidId, "Invalid entity id specified in update or delete." )

    /// Thrown when the id for an invalid definition is requested
    CREATE_EXCEPTION( gamestateInvalidDefinition, "Invalid definition specified." )

    /// @}
    /// @defgroup CORE Core
    /// @{

    /**
     * Contains the current state of the game regarding stringtables, sendtables and entities.
     *
     * In addition the class contains the handlers for all callbacks related to entity parsing and handeling.
     * It forwards it's entities to the default handler for them to be relayed to subscribers. Generating the
     * flattables and keeping track of entity updates and deletes also happens here.
     */
    class gamestate {
        public:
            /** Type for a map of stringtables. */
            typedef multiindex<std::string, int32_t, stringtable> stringtableMap;
            /** Type for a map of sendtables. */
            typedef multiindex<std::string, int32_t, sendtable> sendtableMap;
            /** Type for a map of flattables. */
            typedef std::unordered_map<std::string, flatsendtable> flatMap;
            /** Type for a list of entities. */
            typedef std::vector<entity*> entityMap;

            /** Constructor, sets default values and registers the callbacks nessecary. */
            gamestate(handler_t* h)
                : h(h), clist{}, stringtables{}, sendtables{}, flattables{}, entities{}, entityClassBits(0),
                  sendtableId(-1), stringtableId(-1), skipUnsubscribed(false)
            {
                // handle messages nessecary to update gamestate
                handlerRegisterCallback(h, msgDem, DEM_ClassInfo, gamestate, handleClassInfo)

                // allocate memory for entities
                entities.resize(DOTA_MAX_ENTITIES, nullptr);
            }

            /** Destructor, frees all entities that have not been explicitly deleted. */
            ~gamestate() {
                for (auto &e : entities) {
                    delete e;
                }

                for (auto &s : sendtables) {
                    for (auto &ss : s.value) {
                        delete ss.value;
                    }
                }

                stringtables.clear();
                sendtables.clear();
                flattables.clear();
                entities.clear();
            }

            /** Set whether we should skip all unsubscribed entities */
            void skipByDefault(bool skip) {
                skipUnsubscribed = skip;
            }

            /** Ignores said entity when updating the gamestate */
            void ignoreEntity(uint32_t id) {
                skipent.insert(id);
            }

            /** Check if an entity is skipped */
            bool isSkipped(entity *e) {
                // get classid
                uint32_t eId = e->getClassId();

                // check if the entity is skipped because there is no handler
                bool skipU = (skipUnsubscribed && !h->hasCallback<msgEntity>(eId));
                // check to skip if entity is in the ignore set
                bool skipE = skipent.empty() ? false :  (skipent.find(eId) != skipent.end());

                return skipU || skipE;
            }

            /** Returns how many bits we need to read for the class size. */
            inline uint32_t getEntityClassSize() {
                return entityClassBits;
            }

            /** Returns a list of all registered entities. */
            inline entity_list& getEntityClasses() {
                return clist;
            }

            /** Returns a list of all registered stringtables. */
            inline stringtableMap& getStringtables() {
                return stringtables;
            }

            /** Returns a list of all registered sendtables. */
            inline sendtableMap& getSendtables() {
                return sendtables;
            }

            /**
             * Returns a list of all flattables.
             *
             * This function returns an empty map until they have been generated.
             */
            inline flatMap& getFlattables() {
                return flattables;
            }

            /** Returns a list of all entities. */
            inline entityMap& getEntities() {
                return entities;
            }

            /** Returns the flattable for the specified sendtable based on it's name. */
            inline const flatsendtable& getFlattable(const std::string &tbl) {
                auto it = flattables.find(tbl);
                if (it == flattables.end())
                    BOOST_THROW_EXCEPTION( sendtableUnkownTable()
                        << EArg<1>::info(tbl)
                    );

                return it->second;
            }

            /** Returns entity class id for a specific definition */
            inline uint32_t getEntityIdFor(std::string name) {
                for (auto &e : clist) {
                    if (e.second.networkName == name)
                        return e.second.id;
                }

                BOOST_THROW_EXCEPTION( gamestateInvalidDefinition()
                    << EArg<1>::info(name)
                );

                return 0;
            }

            /** Returns all entity class id's for a specific definition substring */
            inline std::vector<uint32_t> findEntityIdFor(std::string name) {
                std::vector<uint32_t> ret;
                uint32_t length = name.size();

                for (auto &e : clist) {
                    if (e.second.networkName.substr(0, length) == name)
                        ret.push_back(e.second.id);
                }

                return ret;
            }

            /** Handles the entity class information and creates the entity_list. */
            void handleClassInfo(handlerCbType(msgDem) msg);

            /** Handles the server information message. Sets the maximum number of possible entities. */
            void handleServerInfo(handlerCbType(msgNet) msg);

            /** Handles creation of the sendtable's and their properties. */
            void handleSendTable(handlerCbType(msgNet) msg);

            /** Creates the stringtable in question. */
            void handleCreateStringtable(handlerCbType(msgNet) msg);

            /** Handles updates to a given stringtable. */
            void handleUpdateStringtable(handlerCbType(msgNet) msg);

            /**
             * Handles an incoming entities and creates / update / deletes it in the gamestate.
             *
             * This function will also forward all the resulting entities to the handler where they are relayed
             * to any subscribers.
             */
            void handleEntity(handlerCbType(msgNet) msg);
        private:
            /** Handler object for this gamestate */
            handler_t* h;
            /** List which includes the name's of all possible entities. */
            entity_list clist;
            /** Contains all active stringtables. */
            stringtableMap stringtables;
            /** Contains the sendtables. */
            sendtableMap sendtables;
            /** A map of flattened sendtables accessible by the sendtable name. */
            flatMap flattables;
            /** List of active entities. */
            entityMap entities;

            /** The number of bits to read for each entitie's class. */
            uint32_t entityClassBits;
            /** ID of the next sendtable to add */
            int32_t sendtableId;
            /** ID of the next stringtable to add */
            int32_t stringtableId;

            /** Whether to skip unsubscribed entities  */
            bool skipUnsubscribed;
            /** List of entity IDs to ignore */
            std::set<uint32_t> skipent;

            /** Prevent copying of this class. */
            gamestate(const gamestate&) = delete;
            /** Prevent moving of this class. */
            gamestate(gamestate&&) = delete;

            /** Sets maximum number of different entity classes, derives entity class size in bits. */
            inline void setMaxClasses(std::size_t classes) {
                entityClassBits = std::ceil(log2(classes));
            }

            /** Flattens the sendtables, generating network representations of the data received in the right order. */
            void flattenSendtables();

            /** Builds a list of properties excluded (included with parents). */
            void buildExcludeList(const sendtable &tbl, std::set<std::string> &excludes);

            /** Build the table hierarchy. */
            void buildHierarchy(const sendtable &tbl, std::set<std::string> &excludes, std::vector<sendprop*> &props);

            /** Gathers properties for a table. */
            void gatherProperties(const sendtable &tbl, std::vector<sendprop*> &dt_prop,
                std::set<std::string> &excludes, std::vector<sendprop*> &props);
    };

    /// @}
}

#endif /* _DOTA_GAMESTATE_HPP_ */