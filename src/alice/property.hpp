/**
 * @file property.hpp
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

#ifndef _DOTA_PROPERTY_HPP_
#define _DOTA_PROPERTY_HPP_

#include <array>
#include <memory>
#include <utility>
#include <string>
#include <type_traits>

#include <cassert>

#include <boost/variant.hpp>

#include <alice/exception.hpp>
#include <alice/sendprop.hpp>

/// Specifies the maximum length of a string
#define PROPERTY_MAX_STRING_LENGTH 0x200

/// Specifies the maximum elements an array property can hold
#define PROPERTY_MAX_ELEMENTS 100

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when the property seems to have an unkown type
    CREATE_EXCEPTION( propertyInvalidType, "Property has unkown / invalid type." )
    /// Thrown when the type of float serialized cannot be read
    CREATE_EXCEPTION( propertyInvalidFloatCoord, "Type of float coord is not implemented" )
    /// Thrown when the string length exceeds #PROPERTY_MAX_STRING_LENGTH
    CREATE_EXCEPTION( propertyInvalidStringLength, "Trying to read large string" )
    /// Thrown when the type of int serialized cannot be read
    CREATE_EXCEPTION( propertyInvalidInt64Type, "Type of int64 is not implemented" )
    /// Thrown when the number of array elements exceeds #PROPERTY_MAX_ELEMENTS
    CREATE_EXCEPTION( propertyInvalidNumberOfElements, "Unnaturaly large number of elements" )
    /// Thrown when trying to access property as invalid Type
    CREATE_EXCEPTION( propertyBadCast, "Property requested as wrong type" )

    /// @}

    // forward declaration
    class bitstream;
    class entity;
    class property;

    /// @defgroup CORE Core
    /// @{

    namespace detail {
        /** Visitor to call to_string on a Boost.Variant. */
        class propertyToString : public boost::static_visitor<> {
            private:
                /** String to fill */
                std::string &toFill;
            public:
                /** Sets internal string as a reference to r */
                propertyToString(std::string &r) : toFill(r) {}

                /** Invoking this does the actual string conversion */
                template <typename T>
                void operator()(const T& t) const;
        };

        // Forward declaration
        template <typename T> inline std::string to_string(const T& t);
    }

    /** Underlying type for an IntProperty */
    typedef int32_t IntProperty;

    /** Underlying type for an UIntProperty */
    typedef uint32_t UIntProperty;

    /** Underlying type for a FloatProperty */
    typedef float FloatProperty;

    /** Underlying type for a Vector Property */
    typedef std::array<float, 3> VectorProperty;

    /** Underlying type for a VectorXY Property */
    typedef std::array<float, 2> VectorXYProperty;

    /** Underlying type for a String Property */
    typedef std::string StringProperty;

    /** Underlying type for an Array Property */
    typedef std::vector<property> ArrayProperty;

    /** Underlying type for a 64 bit Int Property */
    typedef int64_t Int64Property;

    /** Underlying type for a 64 bit Int Property */
    typedef uint64_t UInt64Property;

    /**
     * This is the actual property, consisting of it's data and definition.
     *
     * Properties are not created on their own but are part of an entity which can contain an
     * unlimited number of these. These are very likely directly tied to a class in the Source Engine,
     * so that they can be automatically updated once new data arrives on the wire.
     *
     * Instead of implementing this, the property class is a more general approach. It can currently hold
     * all different possible values that are send and provides some convinience functions to get to the
     * data.
     */
    class property {
        friend entity;
        public:
            /** Type for the underlying definition */
            typedef sendprop::type type_t;
            /** Type for the variant used internally */
            typedef boost::variant<
                IntProperty,        // T_Int
                UIntProperty,       // T_Int & SPROP_UNSIGNED
                FloatProperty,      // T_Float
                VectorProperty,     // T_Vector
                VectorXYProperty,   // T_VectorXY
                StringProperty,     // T_String
                ArrayProperty,      // T_Array
                Int64Property,      // T_Int64
                UInt64Property      // T_Int64 & SPROP_UNSIGNED
            > value_type;

            /** Empty constructor */
            property() : name(nullptr), init(false) {}

            /** Returns whether this property has been initialized */
            bool isInitialized() {
                return init;
            }

            /** Returns the type of this property */
            inline const type_t& getType() const {
                assert(init);

                return type;
            }

            /** Returns value as type T, throws if conversion fails */
            template <typename T>
            const T& as() {
                assert(init);

                try {
                    return boost::get<T>(value);
                } catch (boost::bad_get &e) {
                    BOOST_THROW_EXCEPTION( propertyBadCast()
                        << EArg<1>::info(getName())
                        << (EArgT<2, uint32_t>::info(type))
                    );
                }
            }

            /** Sets value as type T with data provided */
            template <typename T>
            void set(T data) {
                value = std::move(data);
            }

            /** Returns name of this property based on it's sendprop */
            std::string getName() {
                assert(init);

                return *name;
            }

            /** Returns unique name from flattened sendtable */
            std::string getFlatName() {
                assert(init);
                return prop->getNetname()+*name;
            }

            /** Return value as string */
            std::string asString() const {
                assert(init);

                // this has gotten a bit ugly using variant, but as this function should only be used to debug
                // it's probably fine

                std::string ret;
                detail::propertyToString v(ret);
                boost::apply_visitor( v, value );

                return ret;
            }

            /** Returns definition for this property */
            sendprop* getSendprop() {
                assert(init);
                return prop;
            }

            /** Updates this property from a bitstream */
            void update(bitstream& stream);

            /** Creates property from bitstream and corresponding sendprop definition */
            static property create(bitstream& stream, sendprop* prop);

            /** Skips property contents in the bitstream */
            static void skip(bitstream& stream, sendprop* prop);
        protected:
            /** Type for this paticular property */
            type_t type;
            /** Value for this property */
            mutable value_type value;
            /** Sendprop definition for the type */
            sendprop* prop;
            /** unique name for this property */
            const std::string* name;
            /** Whether this property has been initialized */
            bool init;

            /** Private constructor, we only want our create function to instantize new properties */
            property(sendprop* p) : type(p->getType()), prop(p), init(true) {

            }

            /** Set hierarchial name */
            void setName(const std::string* name) {
                this->name = name;
            }
    };

    namespace detail {
        template <typename T>
        void propertyToString::operator()(const T& t) const {
            toFill = to_string(t);
        }

        template <typename T>
        inline std::string to_string_helper(const T& t, std::true_type) {
            return std::to_string(t);
        }

        template <typename T>
        inline std::string to_string_helper(const T& t, std::false_type) {
            return "unkown";
        }

        /** Helper converting a type T into a string */
        template <typename T>
        inline std::string to_string(const T& t) {
            return to_string_helper(t, std::is_arithmetic<T>{});
        }

        template <>
        inline std::string to_string(const std::array<float, 3>& t) {
            std::stringstream s("");
            s << "[" << t[0] << "|" << t[1] << "|" << t[2] << "]";
            return s.str();
        }

        template <>
        inline std::string to_string(const std::array<float, 2>& t) {
            std::stringstream s("");
            s << "[" << t[0] << "|" << t[1] << "]";
            return s.str();
        }

        template <>
        inline std::string to_string(const std::string& t) {
            return t;
        }

        template <>
        inline std::string to_string(const std::vector<property>& t) {
            std::stringstream s("");
            s << "[";
            for (std::size_t i = 0; i < t.size(); ++i) {
                s << t[i].asString();
                if (i+1 < t.size())
                    s << "|";
            }
            s << "]";

            return s.str();
        }
    }

    /// @}
}

#endif /* _DOTA_PROPERTY_HPP_ */
