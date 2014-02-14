/**
 * @file reader.hpp
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

#ifndef _DOTA_READER_HPP_
#define _DOTA_READER_HPP_

#include <fstream>
#include <string>
#include <utility>
#include <type_traits>

#include <alice/exception.hpp>
#include <alice/handler.hpp>
#include <alice/gamestate.hpp>
#include <alice/config.hpp>

/// Defines the first 7 bytes to check as the header ID
#define DOTA_DEMHEADERID "PBUFDEM"
/// Defines a fixed amount of memory to allocate for the internal buffer
#define DOTA_BUFSIZE 0x60000

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when the file in inaccesible (e.g. wrong permissions or wrong path)
    CREATE_EXCEPTION( demFileNotAccessible,  "Unable to open file." )
    /// Thrown when the file has an unlikely small file size
    CREATE_EXCEPTION( demFileTooSmall,       "Filesize is too small." )
    /// Thrown when the header of the file is not matching #DOTA_DEMHEADERID
    CREATE_EXCEPTION( demHeaderMismatch,     "Header ID is not matching." )
    /// Thrown if the end of the file is reached before an operation is completed
    CREATE_EXCEPTION( demUnexpectedEOF,      "Unexpected end of file." )
    /// Thrown when the file is likely corrupted
    CREATE_EXCEPTION( demCorrupted,          "Demo file appears to be corruped." )
    /// Thrown when snappy fails to decompress content
    CREATE_EXCEPTION( demInvalidCompression, "Data uncompression failed." )
    /// Thrown when protobuf fails to parse a message
    CREATE_EXCEPTION( demParsingError, "Parsing protouf message failed." )
    /// Thrown when the size of a single messages would cause a buffer overflow
    CREATE_EXCEPTION( demMessageToBig,       "Messagesize is to big." )

    /// @}

    /// @defgroup CORE Core
    /// @{

    /** This class reads all messages for a given replay and reports them to the registered handlers and the gamestate. */
    class reader {
        public:
            /** Reading status, announces when certain parts of the replay become available */
            enum status {
                REPLAY_START = 0,   // Parsing started
                REPLAY_FLATTABLES,  // Flattables are available
                REPLAY_FINISH       // Parsing finished
            };

            /** Constructor, takes path to the replay as a first argument. */
            reader(const std::string& file);

            /** Destructor, closed file and resets handler */
            ~reader() {
                if (fstream.is_open())
                    fstream.close();

                delete h;
                delete[] buffer;
                delete[] bufferCmp;
            }

            /** Returns pointer to handler */
            handler_t* getHandler() {
                return h;
            }

            /** Returns reference to gamestate */
            gamestate& getState() {
                return db;
            }

            /** Reads a single message */
            void readMessage(bool skip = false);

            /** Returns true if there are still messages left to read */
            inline bool good() {
                return (fstream.good() && (state != 2));
            }

            /** Reads until the next tick and returns */
            inline void readUntilNextTick() {
                const uint32_t tTick = cTick;
                while (cTick == tTick && good()) {
                    readMessage();
                }
            }

            /** Reads all messages and returns */
            inline void readAll() {
                while (good()) {
                    readMessage();
                }
            }

            /** Returns current tick */
            inline uint32_t getTick() {
                return cTick;
            }
        private:
            /** File header for verification purposes */
            struct demheader_t {
                char headerid[ 8 ]; // needs to equal DOTA_HEADERID
                int32_t offset;
            };

            /** Path to file */
            std::string file;
            /** Filestream for the demo file in question */
            std::ifstream fstream;
            /** Parsing state: 0 = normal, 1 = STOP reached, 2 = finished */
            uint8_t state;
            /** Equals the last tick read */
            uint32_t cTick;
            /** Buffer for messages, reused to omit calling new for every message */
            char* buffer;
            /** Buffer the uncompressed messages, reused to omit calling new for every message */
            char* bufferCmp;
            /** The handler object for this reader */
            handler_t* h;
            /** Initialized in the reader to make sure a valid gamestate is available for all accessors */
            gamestate db;

            /** Private copy constructor, dont allow copying */
            reader(const reader&) = delete;

            /** Reads single 32 bit integer from the file stream */
            int32_t readVarInt(std::ifstream &stream, const std::string& file);
            /** Reads single 32 bit integer from a string */
            int32_t readVarInt(stringWrapper &s, const std::string& file);
            /** Registers all known types with their object counterparts */
            void registerTypes();

            /**
             * Directly returns a callback object from the data provided.
             *
             * MSVC chokes on the resolution of the return type if we specify it correctly.
             * To fix this, we provide if with the fixed return type of msgDem. This works
             * because currently all protobuf objects forwarded to the handler have the same
             * callback signature.
             *
             * This function is definitly broken for entities though.
             */
            template <typename HandlerType, typename Object>
            typename std::remove_pointer<handlerCbType(msgDem)>::type getCallbackObject(stringWrapper str, uint32_t tick) {
                Object* msg = new Object;
                if (!msg->ParseFromArray(str.str, str.size))
                    BOOST_THROW_EXCEPTION((handlerParserError()));

                typedef typename std::remove_pointer<typename handler_t::type<HandlerType::id>::callbackObj_t>::type cbObj;

                return cbObj(msg, tick, 0);
            }

            /** Forwards all entries in the message as specified type */
            template <typename Type>
            void forwardMessageContainer(stringWrapper str, uint32_t tick) {
                while (str.size) {
                    uint32_t type = readVarInt(str, file);
                    uint32_t size = readVarInt(str, file);

                    if (size > str.size) {
                        BOOST_THROW_EXCEPTION((demUnexpectedEOF()
                            << EArg<1>::info(file)
                            << EArgT<2, uint32_t>::info(str.size)
                            << EArgT<3, uint32_t>::info(size)
                            << EArgT<4, uint32_t>::info(type)
                        ));
                    }

                    // remove data from the original buffer
                    stringWrapper message{str.str, size};
                    str.str = str.str+size;
                    str.size = (str.size - size);

                    // directly parse entities to omit calling the handler to often
                    if (std::is_same<Type, msgNet>{}) {
                        switch (type) {
                        case svc_PacketEntities: {
                            auto e = getCallbackObject<msgNet, CSVCMsg_PacketEntities>(std::move(message), tick);
                            db.handleEntity(&e);
                        } break;
                        case svc_ServerInfo: {
                            auto e = getCallbackObject<msgNet, CSVCMsg_ServerInfo>(std::move(message), tick);
                            db.handleServerInfo(&e);
                        } break;
                        case svc_SendTable: {
                            auto e = getCallbackObject<msgNet, CSVCMsg_SendTable>(std::move(message), tick);
                            db.handleSendTable(&e);
                        } break;
                        case svc_CreateStringTable: {
                            auto e = getCallbackObject<msgNet, CSVCMsg_CreateStringTable>(std::move(message), tick);
                            db.handleCreateStringtable(&e);
                        } break;
                        case svc_UpdateStringTable: {
                            auto e = getCallbackObject<msgNet, CSVCMsg_UpdateStringTable>(std::move(message), tick);
                            db.handleUpdateStringtable(&e);
                        } break;
                        default:
                            h->forward<Type>(type, std::move(message), tick);
                        }
                    } else {
                        h->forward<Type>(type, std::move(message), tick);
                    }
                }
            }

            /** Handle packets */
            inline void handlePacket(handlerCbType(msgDem) msg) {
                CDemoPacket* m = msg->get<CDemoPacket>();
                const std::string &data = m->data();
                forwardMessageContainer<msgNet>(stringWrapper{data.c_str(), data.size()}, msg->tick); // forward as Net Message
            }

            /** Handle user message */
            inline void handleUserMessage(handlerCbType(msgNet) msg) {
                CSVCMsg_UserMessage* m = msg->get<CSVCMsg_UserMessage>();
                const std::string &data = m->msg_data();
                h->forward<msgUser>(static_cast<uint32_t>(m->msg_type()), stringWrapper{data.c_str(), data.size()}, msg->tick);
            }

            /** Handle send tables forwarded from DEM */
            inline void handleSendTablesDem(handlerCbType(msgDem) msg) {
                CDemoSendTables* m = msg->get<CDemoSendTables>();
                const std::string &data = m->data();
                forwardMessageContainer<msgNet>(stringWrapper{data.c_str(), data.size()}, msg->tick); // forward as Net Message
            }
    };

    /// @}
}

#endif /* _DOTA_READER_HPP_ */