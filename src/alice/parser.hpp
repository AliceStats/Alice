/**
 * @file parser.hpp
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
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef _ALICE_PARSER_HPP_
#define _ALICE_PARSER_HPP_

#include <string>

#include <alice/dem.hpp>
#include <alice/entity.hpp>
#include <alice/event.hpp>
#include <alice/handler.hpp>
#include <alice/multiindex.hpp>
#include <alice/sendtable.hpp>
#include <alice/stringtable.hpp>
#include <alice/settings.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when a property is defined as an Array but it's subtype hasn't been set
    CREATE_EXCEPTION( aliceInvalidArrayProp, "Array property has no previous member to define state." )

    /// Thrown when the baselinceinstance table is not available
    CREATE_EXCEPTION( aliceBaselineNotFound, "Unable to find baseline instance." )

    /// Thrown when an invalid entity is specified in an update or delete
    CREATE_EXCEPTION( aliceInvalidId, "Invalid entity id specified in update or delete." )

    /// Thrown when the id for an invalid definition is requested
    CREATE_EXCEPTION( aliceInvalidDefinition, "Invalid definition specified." )

    /// @}
    /// @defgroup CORE Core
    /// @{

    class parser {
        public:
             /** Type for a map of stringtables. */
            typedef multiindex<std::string, int32_t, stringtable> stringtableMap;
            /** Type for a map of sendtables. */
            typedef multiindex<std::string, int32_t, sendtable> sendtableMap;
            /** Type for a map of flattables. */
            typedef std::vector<flatsendtable> flatMap;
            /** Type for a list of entities. */
            typedef std::vector<entity> entityMap;

            /** Construct a parser with given settings */
            parser(const settings s, dem_stream *stream);

            /** Destructor */
            ~parser();

            /** Open a replay for parsing */
            void open(std::string path);

            /** Returns whether there are still messages left to read */
            bool good();

            /** Parse and read a single message */
            void read();

            /** Parse and handle all messages in the replay */
            void handle();

            /** Skip to the desired second in the replay */
            void skipTo(uint32_t second);

            /** Returns pointer to handler, pointer is tied to lifetime of this object */
            handler_t* getHandler();

            /** Returns an event descriptor for the specified event id */
            event_descriptor* getEventDescriptor(uint32_t id);

            /** Returns the flattable for the specified sendtable based on it's name. */
            const flatsendtable& getFlattable(const uint32_t);

            /** Returns entity class id for a specific definition */
            uint32_t getEntityIdFor(std::string name);

            /** Returns all entities */
            entityMap& getEntities();

            /** Returns all stringtables */
            stringtableMap& getStringtables();

            /** Returns all flattables */
            flatMap& getFlattables();

            /** Returns all sendtables */
            sendtableMap& getSendtables();

            /** Returns all entity class id's for a specific definition substring */
            std::vector<uint32_t> findEntityIdFor(std::string name);

            /** Get the current tick count */
            uint32_t getTick() {
                return tick;
            }

            /** Get the current message count */
            uint32_t getMsgCount() {
                return msgs;
            }
        private:
            /** Settings for this parser, cannot be changed */
            settings set;
            /** Stream to read the dem contents from */
            dem_stream* stream;
            /** Handler */
            handler_t handler;
            /** Current tick */
            uint32_t tick;
            /** Number of messages parsed */
            uint32_t msgs;

            /** File opened */
            std::string file;
            /** Number of bits to read for the class id */
            uint32_t classBits;
            /** ID of the next sendtable to add */
            int32_t sendtableId;
            /** ID of the next stringtable to add */
            int32_t stringtableId;

            /** List which includes the name's and id's of all possible entities. */
            entity_list clist;
            /** List which contains the specifications for all events emitted */
            event_list elist;
            /** Contains all active stringtables. */
            stringtableMap stringtables;
            /** Contains the sendtables. */
            sendtableMap sendtables;
            /** A map of flattened sendtables accessible by the sendtable name. */
            flatMap flattables;
            /** List of active entities. */
            entityMap entities;
            /** Contains last entity delta */
            entity_delta* delta;

            /** Reads a varint from a string */
            uint32_t readVarInt(const char* data, uint32_t& count);

            /** Treats message as a container of given Type, handles / forwards messages accordingly */
            template <typename Type>
            void forwardMessageContainer(const char* data, uint32_t size, uint32_t tick) {
                while (size) {
                    // increase message count
                    ++msgs;

                    // 0ed in readVarInt, used to increase data pointer offset
                    uint32_t read;

                    // read type and increase offset
                    uint32_t mType = readVarInt(data, read);
                    data = data+read;
                    size -= read;

                    // read size and increase offset
                    uint32_t mSize = readVarInt(data, read);
                    data = data+read;
                    size -= read;

                    // If this happens there has been an error reading from the container
                    if (mSize > size)
                        BOOST_THROW_EXCEPTION((demUnexpectedEOF()
                            << EArg<1>::info(file)
                            << EArgT<2, uint32_t>::info(mSize)
                            << EArgT<3, uint32_t>::info(size)
                            << EArgT<4, uint32_t>::info(mType)
                        ));

                    // Let the buffer point at the next segment of data
                    const char* mMsg = data;
                    data = data+mSize;
                    size -= mSize;

                    // Take care of Net messages
                    if (std::is_same<Type, msgNet>{}) {
                        // Forward all net messages without exception, implies forward_net
                        if (set.forward_net_internal) {
                            handler.forward<msgNet>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                            continue;
                        }

                        // Parse internal messages
                        #ifndef _MSC_VER
                        switch (mType) {
                            case svc_PacketEntities: {
                                if (set.parse_entities) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleEntity(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                            case svc_ServerInfo: {
                                if (set.parse_entities) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleServerInfo(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                            case svc_SendTable: {
                                if (set.parse_entities) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleSendTable(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                            case svc_CreateStringTable: {
                                if (set.parse_stringtables) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleCreateStringtable(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                            case svc_UpdateStringTable: {
                                if (set.parse_stringtables) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleUpdateStringtable(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                            case svc_GameEventList: {
                                if (set.parse_events) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleEventList(&e);
                                    e.free();
                                }
                            } break;
                            case svc_UserMessage: {
                                if (set.forward_user) {
                                    auto e = handler.retrieve<msgNet::id>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                    handleUserMessage(&e);
                                    e.free();
                                }
                                continue;
                            } break;
                        }
                        #else // _MSC_VER
                        switch (mType) {
                            case svc_PacketEntities:
                            case svc_ServerInfo:
                            case svc_SendTable:
                            case svc_CreateStringTable:
                            case svc_UpdateStringTable:
                            case svc_UserMessage:
                            case svc_GameEventList:
                                handler.forward<msgNet>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                                continue;
                            default:
                                break;
                        }
                        #endif // _MSC_VER

                        // Forward remaining?
                        if (set.forward_net)
                            handler.forward<msgNet>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                    }

                    // Take care of User messages
                    if (std::is_same<Type, msgUser>{} && set.forward_user)
                        handler.forward<msgUser>(mType, demMessage_t{0, tick, mType, mMsg, mSize}, tick);
                }
            }

            /** Callback function, creates the entity list */
            void handleClasses(handlerCbType(msgDem) msg);

            /** Handles a single packet */
            void handlePacket(handlerCbType(msgDem) msg);

            /** Takes care of the creation of the send tables */
            void handleSendTables(handlerCbType(msgDem) msg);

            /** Handles a user message */
            void handleUserMessage(handlerCbType(msgDem) msg);

            /** Handle server info which reserves memory for the maximum number of entities */
            void handleServerInfo(handlerCbType(msgNet) msg);

            /** Handles creation of a send table */
            void handleSendTable(handlerCbType(msgNet) msg);

            /** Handles creation of a stringtable */
            void handleCreateStringtable(handlerCbType(msgNet) msg);

            /** Handles updates to stringtables */
            void handleUpdateStringtable(handlerCbType(msgNet) msg);

            /** Handles the creation of the event list */
            void handleEventList(handlerCbType(msgNet) msg);

            /** Check if an entity is skipped */
            bool isSkipped(entity &e);

            /** Handles entity updates */
            void handleEntity(handlerCbType(msgNet) msg);

            /** Walk through each sendprop table's hierarchy and flatten it */
            void flattenSendtables();

            /** Generates a list of excluded properties */
            void buildExcludeList(const sendtable &tbl, std::set<std::string> &excludes);

            /** Build hierarchy for one table */
            void buildHierarchy(const sendtable &tbl, std::set<std::string> &excludes, std::vector<dt_hiera> &props, std::string base);

            /** Gather all properties, exclude those marked from the list */
            void gatherProperties(const sendtable &tbl, std::vector<dt_hiera> &dt_prop, std::set<std::string> &excludes, std::vector<dt_hiera> &props, std::string base);

            /** Registers all currently known types */
            void registerTypes();
    };

    /// @}
}

#endif /* _ALICE_PARSER_HPP_ */