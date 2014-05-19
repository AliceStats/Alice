/**
 * @file sendprop.hpp
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

#ifndef _DOTA_SENDPROP_HPP_
#define _DOTA_SENDPROP_HPP_

/// Flag for an unsigned integer
#define SPROP_UNSIGNED                  (1<<0)
/// If this is set, the float/vector is treated like a world coordinate
#define SPROP_COORD                     (1<<1)
/// For floating point, don't scale into range, just take value as is.
#define SPROP_NOSCALE                   (1<<2)
 /// For floating point, limit high value to range minus one bit unit
#define SPROP_ROUNDDOWN                 (1<<3)
/// For floating point, limit low value to range minus one bit unit
#define SPROP_ROUNDUP                   (1<<4)
/// If this is set, the vector is treated like a normal (only valid for vectors)
#define SPROP_NORMAL                    (1<<5)
/// This is an exclude prop (not excluded, but it points at another prop to be excluded).
#define SPROP_EXCLUDE                   (1<<6)
/// Use XYZ/Exponent encoding for vectors.
#define SPROP_XYZE                      (1<<7)
/// This tells us that the property is inside an array, so it shouldn't be put into the
/// flattened property list - its array will point at it when it needs to.
#define SPROP_INSIDEARRAY               (1<<8)
/// Set automatically if it's a datatable with an offset of 0 that doesn't change the pointer
/// (ie: for all automatically-chained base classes).
/// In this case, it can get rid of this SendPropDataTable altogether and spare the
/// trouble of walking the hierarchy more than necessary.
#define SPROP_COLLAPSIBLE               (1<<11)
/// Like SPROP_COORD, but special handling for multiplayer games
#define SPROP_COORD_MP                  (1<<12)
/// Like SPROP_COORD, but special handling for multiplayer games where the fractional
/// component only gets a 3 bits instead of 5
#define SPROP_COORD_MP_LOWPRECISION     (1<<13)
/// SPROP_COORD_MP, but coordinates are rounded to integral boundaries
#define SPROP_COORD_MP_INTEGRAL         (1<<14)
/// SPROP_CELL_COORD
#define SPROP_CELL_COORD                (1<<15)
/// SPROP_CELL_COORD_LOWPRECISION
#define SPROP_CELL_COORD_LOWPRECISION   (1<<16)
/// SPROP_CELL_COORD_INTEGRAL
#define SPROP_CELL_COORD_INTEGRAL       (1<<17)
/// If this is set the sendtable flattening will set the propertie's priority to 64
#define SPROP_CHANGES_OFTEN             (1<<18)
/// SPROP_ENCODED_AGAINST_TICKCOUNT
#define SPROP_ENCODED_AGAINST_TICKCOUNT (1<<19)

#include <string>

#include <alice/netmessages.pb.h>
#include <alice/exception.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when a property is accessed like an arry but actualy has another type
    CREATE_EXCEPTION( sendpropInvalidArrayAccess, "Trying to access non-array property as array." )

    /// @}

    /// @defgroup CORE Core
    /// @{

    /**
     * Definition of a networked property.
     *
     * A sendprop is not an actual property but merly a definition / guide on how to read
     * said property. It contains things like the type, flags and networked bits. In short,
     * everything nessecary to read the property from a bitstream.
     *
     * See the sendtable.hpp for additional documentation.
     */
    class sendprop {
        public:
            /** Type of property */
            enum type {
                T_Int = 0,
                T_Float,
                T_Vector,
                T_VectorXY,
                T_String,
                T_Array,
                T_DataTable,
                T_Int64
            };

            /** Constructor, intializes property from corresponding protobuf object */
            sendprop(const CSVCMsg_SendTable::sendprop_t& p, const std::string& netname) : tself(static_cast<type>(p.type())),
                name(p.var_name()), netname(netname), flags(p.flags()), priority(p.priority()),
                classname(p.dt_name()), elements(p.num_elements()),
                lowVal(p.low_value()), highVal(p.high_value()), bits(p.num_bits())
            {

            }

            /** Default copy constructor */
            sendprop(const sendprop&) = default;

            /** Default destructor */
            ~sendprop() = default;

            /** Returns own type */
            inline const type& getType() const {
                return tself;
            }

            /** Returns name */
            inline const std::string& getName() const {
                return name;
            }

            /** Returns network name */
            inline const std::string& getNetname() const {
                return netname;
            }

            /** Returns flags */
            inline const uint32_t& getFlags() const {
                return flags;
            }

            /**
             * Returns priority.
             *
             * This value is related to the properties position in the flattable.
             */
            inline const uint32_t& getPriority() const {
                return priority;
            }

            /** Returns classname */
            inline const std::string& getClassname() const {
                return classname;
            }

            /** Returns number of elements this property has */
            inline const uint32_t& getElements() const {
                return elements;
            }

            /** Returns minimum value if applicable */
            inline const uint32_t& getLowVal() const {
                return lowVal;
            }

            /** Returns max value if applicable */
            inline const uint32_t& getHighVal() const {
                return highVal;
            }

            /** Returns size as bits */
            inline const uint32_t& getBits() const {
                return bits;
            }

            /** Sets type of array elements this property holds */
            inline void setArrayType(sendprop* e) const {
                elemType = e;
            }

            /** Returns type of array element if applicable */
            inline sendprop* getArrayType() const {
                if (!elemType)
                    BOOST_THROW_EXCEPTION( sendpropInvalidArrayAccess()
                        << EArg<1>::info(netname)
                        << EArg<2>::info(name)
                        << (EArgT<3, uint32_t>::info(tself))
                    );

                return elemType;
            }
        private:
            /** Type of property */
            const type tself;
            /** Variable name */
            const std::string name;
            /** Network table name */
            const std::string netname;
            /** Flags */
            const uint32_t flags;
            /** Priority for Flattables */
            const uint32_t priority;
            /** Name of class this is refering to */
            const std::string classname;
            /** Number of elements (for array etc) */
            const uint32_t elements;
            /** Min value */
            const uint32_t lowVal;
            /** Max value */
            const uint32_t highVal;
            /** Number of bits send */
            const uint32_t bits;

            /** If this property is an array, this is the type of each element stored in it */
            mutable sendprop* elemType;
    };

    /// @}
}

#endif /* _DOTA_SENDPROP_HPP_ */