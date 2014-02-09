/**
 * @file handler.hpp
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

#ifndef _DOTA_HANDLER_HPP_
#define _DOTA_HANDLER_HPP_

#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <type_traits>

#include <google/protobuf/message.h>

#include <alice/entity.hpp>
#include <alice/exception.hpp>
#include <alice/delegate.hpp>

/// Registers and object with the handler.
///
/// Invoking it is nessecary when the object should be created by the handler from the data
/// supplied (e.g. create a protobuffer from a string).
/// If the above is the case, the object supplied as the 3rd parameter is constructed and its
/// parseFromString function is invoked on the data.
///
/// Example: handlerRegisterObject( handler, msgTest, 0, MyProtobuf )
///
#define handlerRegisterObject(handler_, type_, id_, object_) \
handler_->registerObject<type_, object_>( (handler_t::type<type_>::id_t) id_ );

/// Register a callback function for specified type.
///
/// The callback is invoked when an object is relayed to the handler with the specified unique
/// and non-unique identifier. The macro can only be called from the class having the callback
/// function as the delegate is constructed explicitly from *this;
///
/// Example: handlerRegisterCallback( handler, msgTest, 0, myObject, myFunction )
///
#define handlerRegisterCallback(handler_, type_, id_, object_, function_) \
handler_->registerCallback<type_>( \
    (handler_t::type<type_>::id_t)id_, \
    handler_t::type<type_>::delegate_t::fromMemberFunc<object_, &object_::function_>(this) \
);

/// Removes a callback function for a specific type.
///
/// This macro removes a callback function which has been registered with handlerRegisterCallback.
/// Parameter order and usage is the same. Removing should only be done in special cases when you
/// realy don't rely on the object anymore as iterating the whole callback list is required.
///
/// Example: handlerRemoveCallback( handler, msgTest, 0, myObject, myFunction )
///
#define handlerRemoveCallback(handler_, type_, id_, object_, function_) \
handler_->removeCallback<type_>( \
    (handler_t::type<type_>::id_t)id_, \
    handler_t::type<type_>::delegate_t::fromMemberFunc<object_, &object_::function_>(this) \
);

/// Register a catch-all callback function for the given prefix.
///
/// This function compares a substring of the non-unique ID of each relayed object to the ID provided.
/// This means that an ID of ABC_ will match all relayed objects starting with it.
/// The macro resolved to an exception being thrown of the type of ID is not a string.
///
/// Example: handlerRegisterPrefixCallback( handler, msgTest, "ABC_", myObject, myCatchAllFunction )
///
#define handlerRegisterPrefixCallback(handler_, type_, id_, object_, function_) \
handler_->registerCallback<type_>( \
    (handler_t::type<type_>::id_t)id_, \
    handler_t::type<type_>::delegate_t::fromMemberFunc<object_, &object_::function_>(this), \
    true \
);

/// Removes a prefix callback function for the given prefix.
///
/// Example: handlerRemovePrefixCallback( handler, msgTest, "ABC_", myObject, myCatchAllFunction )
///
#define handlerRemovePrefixCallback(handler_, type_, id_, object_, function_) \
handler_->removeCallback<type_>( \
    (handler_t::type<type_>::id_t)id_, \
    handler_t::type<type_>::delegate_t::fromMemberFunc<object_, &object_::function_>(this), \
    true \
);

/// Resolves the return type for a specific message type.
///
/// This macro is best used as the function parameter for callback functions.
///
/// Example: void myCallbackFunction( handlerCbType(msgTest) callbacjObject );
///
#define handlerCbType(type_) handler_t::type<type_>::callbackObj_t

// include detail namespace
#include "handler_detail.hpp"

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Gets thrown when an unkown subhandler type is specified.
    ///
    /// An important think to look into when this is thrown is whether the type of said
    /// subhandler is passed as uint32_t.
    CREATE_EXCEPTION( handlerNoConversionAvailable, "Unable to convert base-id to unique message-id." )

    /// Gets thrown when protobuf fails to parse the message
    CREATE_EXCEPTION( handlerParserError, "Error while parsing message with protobuf" )

    /// Gets thrown when the conversion from data to object fails
    ///
    /// This likely happens when you subscribe to a type which is available in the protobuf fails
    /// but is missing acorresponding #handlerRegisterObject call.
    CREATE_EXCEPTION( handlerTypeError, "Type in question has not be registered." )

    /// Thrown when anything but std::string is handled in a prefix context.
    CREATE_EXCEPTION( handlerPrefixError, "Type in question is unabled to be used in the prefix context." )

    /// @}
    /// @defgroup CORE Core
    /// @{

    /** Callback object getting supplied to each function invoked by the handler. */
    template <typename Obj, typename Id>
    struct cbObject {
        /** Current tick */
        uint32_t tick;
        /** The message in question */
        Obj msg;
        /** Id for the message */
        const Id &id;
        /** Whether to take ownership of the memory */
        bool ownsPointer;

        /** Constructor, sets all values */
        cbObject(Obj msg, uint32_t tick, const Id &id, bool ownsPointer = true)
            : tick(tick), msg(std::move(msg)), id(id), ownsPointer(ownsPointer) { }

        /** Destructor, deletes message if it's a pointer */
        ~cbObject() {
            if (ownsPointer)
                detail::destroyCallbackObjectHelper<Obj>::destroy(std::move(msg));
        }

        /** Casts message into the specified type and returns it. */
        template <typename T>
        T* get() {
            return reinterpret_cast<T*>(msg);
        }
    };

    /**
     * Used to transmit string data and length to the handler.
     *
     * Taking substrings of std::string always requires a copy. This is generally pretty bad
     * for the performance because most of the messages are nested. This struct is a compromise
     * between adding a pointer + size as handler argument and using the std::string. It's not as
     * nice but it's still viable and leads to a significant performance increase.
     */
    struct stringWrapper {
        /** string in question */
        const char* str;
        /** size to read */
        std::size_t size;
    };

    /**
     * This is the implementation of each of the handler functions for a specifc set of parameters.
     *
     * @param Obj Parent object for each of the messages that should be parsed
     * @param Id Which type non-unique ID's should be saved at (e.g. uint32_t, string)
     * @param Data Type of the data that should be parsed into Obj
     * @param Own unique ID in the handler context (numeric uint32_t)
    */
    template <typename Obj, typename Id, typename Data, uint32_t IdSelf>
    class handlersub {
        public:
            /** Type for parent object each message of this type inherits from */
            typedef Obj obj_t;
            /** Type of the object id to manage relations */
            typedef Id id_t;

            /** Type for the parameter of the callback function */
            typedef cbObject<Obj, Id>* callbackObj_t;
            /** Type for the callback signature */
            typedef delegate<void, callbackObj_t> delegate_t;

            /** Own unique ID in the handler context */
            static const unsigned id = IdSelf;

            /** Returns true if the specified type has at least one callback function */
            bool hasCallback(const id_t& i) {
                return hasCallback(i, std::is_same<std::string, Id>{});
            }

            /** Registers a new callback handler for the specified ID */
            void registerCallback(const id_t& i, delegate_t&& d, bool prefix = false) {
                if (prefix && std::is_same<std::string, Id>::value) {
                    // register prefixes only for strings
                    cbpref[i].push_back(std::move(d));
                } else {
                    cb[i].push_back(std::move(d));
                }
            }

            /**
             * Removes a callback handler for the specified ID.
             *
             * This function is relativly expensive when a lot of handlers are registered because it
             * has to iterate over the complete handler. Removing callbacks is generally not nessecary
             * so this function should not be overused.
             */
            void removeCallback(const id_t& i, delegate_t&& d, bool prefix = false) {
                if (prefix && std::is_same<std::string, Id>::value) {
                    for (auto it = cbpref[i].begin(); it != cbpref[i].end(); ++it) {
                        if (*it == d) {
                            cbpref[i].erase(it);
                            break;
                        }
                    }
                } else {
                   for (auto it = cb[i].begin(); it != cb[i].end(); ++it) {
                        if (*it == d) {
                            cb[i].erase(it);
                            break;
                        }
                    }
                }
            }

            /** Register a new object type for the specified ID */
            template <typename T>
            void registerObject(const id_t& i) {
                obj[i] = [](stringWrapper&& data) {
                    obj_t msg = new T;
                    if (!msg->ParseFromArray(data.str, data.size))
                            BOOST_THROW_EXCEPTION((handlerParserError()));

                    return msg;
                };
            }

            /** Forwards a message to all handlers, the correct object is inferred from the id. */
            void forward(const id_t& i, Data&& data, uint32_t tick) {
                forward(i, std::move(data), tick, std::is_same<Obj, Data>{});
            }

            /** Deletes all registered callbacks and objects. */
            void clear() {
                cb.clear();
                obj.clear();
                cbpref.clear();
            }
        private:
            /** Type for a list of registered callbacks */
            typedef std::unordered_map<id_t, std::vector<delegate_t>> cbmap_t;
            /** Callback list */
            cbmap_t cb;

            /** Type for a list of registered objects */
            typedef std::unordered_map<id_t, obj_t (*)(Data&&)> objmap_t;
            /** Object list */
            objmap_t obj;
            /** List of prefix searched objects when dealing with string ids */
            cbmap_t cbpref;

            /** Called if prefix serarch is applicable */
            bool hasCallback(const id_t& i, std::true_type);
            /** Called if id needs to be equal to cb */
            bool hasCallback(const id_t& i, std::false_type);

            /** Called if the message needs to be parsed from the data */
            void forward(const id_t& i, Data &&data, uint32_t tick, std::false_type);
            /** Called if the data is already in message format */
            void forward(const id_t& i, Data &&data, uint32_t tick, std::true_type);

            /** Forward a message to all handlers matching the prefix */
            void forwardPrefix(callbackObj_t o) {
                forwardPrefix(o, std::is_same<std::string, Id>{});
            }
            /** Forward a message to all handlers matching the prefix */
            void forwardPrefix(callbackObj_t o, std::true_type);
            /** Forward a message to all handlers matching the prefix, stub, throws */
            void forwardPrefix(callbackObj_t o, std::false_type);
    };

    /**
     * Handler, taking a variable number of subhandlers and handles functions accordingly.
     *
     * This class is implemented as a variadic template. This saves us the time of using a parent-child construct
     * which requires virtual method and can't be optimized well. The handler calls get inlined at compile time which
     * makes this method a lot faster.
     */
    template <typename...>
    class handler;

    template <>
    class handler<> {
        public:
            template <uint32_t Type, typename Id>
            bool hasCallback(const Id& i) {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );

                return false; // just to fix compiler warnings
            }

            template<uint32_t Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, bool prefix = false) {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );
            }

            template<uint32_t Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, bool prefix = false) {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );
            }

            template<uint32_t Type, typename T, typename Id>
            void registerObject(const Id& i) {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );
            }

            template <uint32_t Type, typename Id, typename Data>
            void forward(const Id& i, Data &&data, uint32_t tick) {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );
            }

            template <uint32_t Type>
            void clear() {
                BOOST_THROW_EXCEPTION(handlerTypeError()
                    << (EArgT<1, uint32_t>::info(Type))
                );
            }
    };

    template <typename T1, typename... Rest>
    class handler<T1, Rest...> {
        public:
            /** Helper to get the type for the Nth argument. */
            template <unsigned int N>
            using type = typename detail::GetNthArgument<N, T1, Rest...>::type;

            /** Returns true if a callback function for the specified ID is available. */
            template <uint32_t Type, typename Id>
            bool hasCallback(const Id& i) {
                return hasCallback<Type, Id>(i, std::is_same<typename T1::id_t, Id>{});
            }

            /** Register a callback for a specified type */
            template<uint32_t Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, bool prefix = false) {
                registerCallback<Type, Id, Delegate>(i, std::move(d), prefix, std::is_same<typename T1::id_t, Id>{});
            }

            /** Removes a callback for a specified type */
            template<uint32_t Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, bool prefix = false) {
                removeCallback<Type, Id, Delegate>(i, std::move(d), prefix, std::is_same<typename T1::id_t, Id>{});
            }

            /** Register an object for a specific ID of the selected type */
            template<uint32_t Type, typename T, typename Id>
            void registerObject(const Id& i) {
                registerObject<Type, T, Id>(i, std::is_same<typename T1::id_t, Id>{});
            }

            /** Forwards a message to all registered handlers */
            template <uint32_t Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick) {
                forward<Type, Id, Data>(std::move(i), std::move(data), tick, std::is_same<typename T1::id_t, Id>{});
            }

            /** Removes all registered callbacks */
            template <uint32_t Type>
            void clear() {
                if (T1::id == Type) {
                    subhandler.clear();
                } else {
                    child.template clear<Type>();
                }
            }
        private:
            /**
             * The current subhandler.
             *
             * Each recursion has it's own subhandler as a member variable. This is required if
             * we want the handler state to depend soley on the object refered to as opposed to
             * having global instances.
             */
            T1 subhandler;

            /** The next handler in the recursion process. */
            handler<Rest...> child;

            /** Implementation for hasCallback */
            template <uint32_t Type, typename Id>
            bool hasCallback(const Id& i, std::true_type);
            /** Implementation for hasCallback */
            template <uint32_t Type, typename Id>
            bool hasCallback(const Id& i, std::false_type);

            /** Implementation for registerCallback */
            template<uint32_t Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, bool prefix, std::true_type);
            /** Implementation for registerCallback */
            template<uint32_t Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, bool prefix, std::false_type);

            /** Implementation for removeCallback */
            template<uint32_t Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, bool prefix, std::true_type);
            /** Implementation for removeCallback */
            template<uint32_t Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, bool prefix, std::false_type);

            /** Implementation for registerObject */
            template<uint32_t Type, typename T, typename Id>
            void registerObject(const Id& i, std::true_type);
            /** Implementation for registerObject */
            template<uint32_t Type, typename T, typename Id>
            void registerObject(const Id& i, std::false_type);

            /** Implementation for forward */
            template <uint32_t Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick, std::true_type);
            /** Implementation for forward */
            template <uint32_t Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick, std::false_type);
    };

    // this file includes the actual implementation which is seperated for enhanced readability
    #include "handler_impl.hpp"

    /** Enumeration of different message types for our replay parser */
    enum type {
        msgDem = 0, // messages encoded in the dem-file
        msgUser,    // user messages
        msgNet,     // net messages
        msgEntity   // entities / deltas of those
    };

    /** Type for our default handler */
    typedef handler<
        handlersub< ::google::protobuf::Message*, uint32_t, stringWrapper, msgDem >,
        handlersub< ::google::protobuf::Message*, uint32_t, stringWrapper, msgUser >,
        handlersub< ::google::protobuf::Message*, uint32_t, stringWrapper, msgNet >,
        handlersub< entity*, std::string, entity*, msgEntity >
    > handler_t;

    /// @}
}

#endif /* _DOTA_HANDLER_HPP_ */
