/**
 * @file handler.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.2
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

#ifndef _DOTA_HANDLER_HPP_
#define _DOTA_HANDLER_HPP_

#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <type_traits>

#include <alice/exception.hpp>
#include <alice/delegate.hpp>
#include <alice/dem.hpp>

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
handler_->registerObject<type_, object_>( (handler_t::type<type_::id>::id_t) id_ );

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
    (handler_t::type<type_::id>::id_t)id_, \
    handler_t::type<type_::id>::delegate_t::fromMemberFunc<object_, &object_::function_>(this) \
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
    (handler_t::type<type_::id>::id_t)id_, \
    handler_t::type<type_::id>::delegate_t::fromMemberFunc<object_, &object_::function_>(this) \
);

/// Resolves the return type for a specific message type.
///
/// This macro is best used as the function parameter for callback functions.
///
/// Example: void myCallbackFunction( handlerCbType(msgTest) callbacjObject );
///
#define handlerCbType(type_) handler_t::type<type_::id>::callbackObj_t

// include detail namespace
#include "handler_detail.hpp"

namespace google {
    namespace protobuf {
        // forward declaration
        class Message;
    }
}
namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Gets thrown when an unkown subhandler type is specified.
    CREATE_EXCEPTION( handlerNoConversionAvailable, "Unable to convert base-id to unique message-id." )

    /// Gets thrown when protobuf fails to parse the message.
    CREATE_EXCEPTION( handlerParserError, "Error while parsing message with protobuf" )

    /// Gets thrown when the conversion from data to object fails
    ///
    /// This likely happens when you subscribe to a type which is available in the protobuf files
    /// but is missing a corresponding #handlerRegisterObject call.
    CREATE_EXCEPTION( handlerTypeError, "Type in question has not be registered." )

    /// @}

    // forward declaration
    class entity;
    struct entity_delta;

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

        /** Delete memory held by this object, does never lead to a double free, only works if Obj is a pointer */
        void free() {
            ownsPointer = false;
            detail::destroyCallbackObjectHelper<Obj>::destroy(std::move(msg));
        }
    };

    /**
     * This is the implementation of each of the handler functions for a specifc set of parameters.
     *
     * @param Obj Parent object for each of the messages that should be parsed
     * @param Id Which type non-unique ID's should be saved at (e.g. uint32_t, string)
     * @param Data Type of the data that should be parsed into Obj
     * @param Own unique ID in the handler context (numeric uint32_t)
     *
     * If Obj and Data have the same type, Data will be treated as if it was already parsed.
    */
    template <typename Obj, typename Data, typename IdSelf>
    class handlersub {
        public:
            /** Type for parent object each message of this type inherits from */
            typedef Obj obj_t;
            /** Type of the object id to manage relations */
            typedef uint32_t id_t;

            /** Type for the parameter of the callback function */
            typedef cbObject<Obj, uint32_t>* callbackObj_t;
            /** Pointer-less type for the callback object */
            typedef cbObject<Obj, uint32_t> callbackObjS_t;
            /** Type for the callback signature */
            typedef delegate<void, callbackObj_t> delegate_t;

            /** Own unique ID in the handler context */
            typedef IdSelf id;

            /** Returns true if the specified type has at least one callback function */
            bool hasCallback(const id_t& i) {
                if (cb.size() <= i)
                    return false;

                return !cb[i].empty();
            }

            /** Registers a new callback handler for the specified ID */
            void registerCallback(const id_t& i, delegate_t&& d) {
                if (cb.size() <= i)
                    cb.resize(i+1);

                cb[i].push_back(std::move(d));
            }

            /**
             * Removes a callback handler for the specified ID.
             *
             * This function is relativly expensive when a lot of handlers are registered because it
             * has to iterate over the complete handler. Removing callbacks is generally not nessecary
             * so this function should not be overused.
             */
            void removeCallback(const id_t& i, delegate_t&& d) {
                for (auto it = cb[i].begin(); it != cb[i].end(); ++it) {
                    if (*it == d) {
                        cb[i].erase(it);
                        break;
                    }
                }
            }

            /** Register a new object type for the specified ID */
            template <typename T>
            void registerObject(const id_t& i) {
                if (obj.size() <= i)
                    obj.resize(i+1, nullptr);

                obj[i] = [](demMessage_t&& data) {
                    obj_t msg = new T;
                    if (!msg->ParseFromArray(data.msg, data.size))
                            BOOST_THROW_EXCEPTION((handlerParserError()));

                    return msg;
                };
            }

            /** Forwards a message to all handlers, the correct object is inferred from the id. */
            void forward(const id_t& i, Data&& data, uint32_t tick) {
                forward(i, std::move(data), tick, std::is_same<Obj, Data>{});
            }

            /** Retrieve a parsed callback object without forwarding it. */
            callbackObjS_t retrieve(const id_t& i, Data&& data, uint32_t tick) {
                return retrieve(i, std::move(data), tick, std::is_same<Obj, Data>{});
            }

            /** Deletes all registered callbacks and objects. */
            void clear() {
                cb.clear();
                obj.clear();
            }
        private:
            /** Type for a list of registered callbacks */
            typedef std::vector<std::vector<delegate_t>> cblist_t;
            /** Callback list */
            cblist_t cb;

            /** Type for a list of registered objects */
            typedef std::vector<obj_t (*)(Data&&)> objlist_t;
            /** Object list */
            objlist_t obj;

            /** Called if the message needs to be parsed from the data */
            void forward(const id_t& i, Data &&data, uint32_t tick, std::false_type);
            /** Called if the data is already in message format */
            void forward(const id_t& i, Data &&data, uint32_t tick, std::true_type);

            /** Called if the message needs to be parsed from the data */
            callbackObjS_t retrieve(const id_t& i, Data &&data, uint32_t tick, std::false_type);
            /** Called if the data is already in message format */
            callbackObjS_t retrieve(const id_t& i, Data &&data, uint32_t tick, std::true_type);
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
            template <typename Type, typename Id>
            bool hasCallback(const Id& i) {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
                return false; // just to fix compiler warnings
            }

            template<typename Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, bool prefix = false) {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
            }

            template<typename Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, bool prefix = false) {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
            }

            template<typename Type, typename T, typename Id>
            void registerObject(const Id& i) {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
            }

            template <typename Type, typename Id, typename Data>
            void forward(const Id& i, Data &&data, uint32_t tick) {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
            }

            template <typename Type>
            void clear() {
                BOOST_THROW_EXCEPTION( handlerNoConversionAvailable() );
            }
    };

    template <typename T1, typename... Rest>
    class handler<T1, Rest...> {
        public:
            /** Helper to get the type for the Nth argument. */
            template <unsigned int N>
            using type = typename detail::GetNthArgument<N, T1, Rest...>::type;

            /** Returns true if a callback function for the specified ID is available. */
            template <typename Type, typename Id>
            bool hasCallback(const Id& i) {
                return hasCallback<Type, Id>(i, std::is_same<typename T1::id, Type>{});
            }

            /** Register a callback for a specified type */
            template<typename Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d) {
                registerCallback<Type, Id, Delegate>(i, std::move(d), std::is_same<typename T1::id, Type>{});
            }

            /** Removes a callback for a specified type */
            template<typename Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d) {
                removeCallback<Type, Id, Delegate>(i, std::move(d), std::is_same<typename T1::id, Type>{});
            }

            /** Register an object for a specific ID of the selected type */
            template<typename Type, typename T, typename Id>
            void registerObject(const Id& i) {
                registerObject<Type, T, Id>(i, std::is_same<typename T1::id, Type>{});
            }

            /** Forwards a message to all registered handlers */
            template <typename Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick) {
                forward<Type, Id, Data>(std::move(i), std::move(data), tick, std::is_same<typename T1::id, Type>{});
            }

            #ifndef _MSC_VER
            /** Retrieve a parsed callback object without forwarding it. Not available when compiling with visual studio. */
            template <unsigned Type, typename Id, typename Data>
            typename type<Type>::callbackObjS_t retrieve(Id i, Data data, uint32_t tick) {
                return retrieve<Type, Id, Data>(
                    std::move(i), std::move(data), tick,
                    std::is_same<typename type<Type>::id, typename T1::id>{}
                );
            }
            #endif // _MSC_VER

            /** Removes all registered callbacks */
            template <typename Type>
            void clear() {
                subhandler.clear();
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
            template <typename Type, typename Id>
            bool hasCallback(const Id& i, std::true_type);
            /** Implementation for hasCallback */
            template <typename Type, typename Id>
            bool hasCallback(const Id& i, std::false_type);

            /** Implementation for registerCallback */
            template<typename Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, std::true_type);
            /** Implementation for registerCallback */
            template<typename Type, typename Id, typename Delegate>
            void registerCallback(const Id& i, Delegate&& d, std::false_type);

            /** Implementation for removeCallback */
            template<typename Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, std::true_type);
            /** Implementation for removeCallback */
            template<typename Type, typename Id, typename Delegate>
            void removeCallback(const Id& i, Delegate&& d, std::false_type);

            /** Implementation for registerObject */
            template<typename Type, typename T, typename Id>
            void registerObject(const Id& i, std::true_type);
            /** Implementation for registerObject */
            template<typename Type, typename T, typename Id>
            void registerObject(const Id& i, std::false_type);

            /** Implementation for forward */
            template <typename Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick, std::true_type);
            /** Implementation for forward */
            template <typename Type, typename Id, typename Data>
            void forward(Id i, Data data, uint32_t tick, std::false_type);

            #ifndef _MSC_VER
            /** Implementation for retrieve */
            template <unsigned Type, typename Id, typename Data>
            typename type<Type>::callbackObjS_t
            retrieve(Id i, Data data, uint32_t tick, std::true_type);
            /** Implementation for retrieve */
            template <unsigned Type, typename Id, typename Data>
            typename type<Type>::callbackObjS_t
            retrieve(Id i, Data data, uint32_t tick, std::false_type);
            #endif // _MSC_VER
    };

    // this file has the actual implementation which is seperated for enhanced readability
    #include "handler_impl.hpp"

    /// Different message types for our replay parser.

    /** Struct for parser status messages */
    struct msgStatus { static const uint32_t id = 0; };
    /** Struct for a top-level demo messages */
    struct msgDem    { static const uint32_t id = 1; };
    /** Struct for a user message */
    struct msgUser   { static const uint32_t id = 2; };
    /** Struct for a net message */
    struct msgNet    { static const uint32_t id = 3; };
    /** Struct for an entity message */
    struct msgEntity { static const uint32_t id = 4; };

    /**
     * Struct for an entity delta message.
     *
     * The entity_delta object passed to each subscriber will always reuse the previous
     * pointer.
     */
    struct msgEntityDelta { static const uint32_t id = 5; };

    /** Type for our default handler */
    typedef handler<
        handlersub< uint32_t, uint32_t, msgStatus >,
        handlersub< ::google::protobuf::Message*, demMessage_t, msgDem >,
        handlersub< ::google::protobuf::Message*, demMessage_t, msgUser >,
        handlersub< ::google::protobuf::Message*, demMessage_t, msgNet >,
        handlersub< entity*, entity*, msgEntity >,
        handlersub< entity_delta*, entity_delta*, msgEntityDelta >
    > handler_t;

    /// @}
}

#endif /* _DOTA_HANDLER_HPP_ */
