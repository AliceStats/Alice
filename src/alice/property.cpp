/**
 * @file property.cpp
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

#include <cmath>
#include <iostream>

#include <alice/property.hpp>

namespace dota {
    void property::update(bitstream &stream) {
         switch (prop->getType()) {
            case sendprop::T_Int:
                set(readInt(stream, prop));
                break;

            case sendprop::T_Float:
                set(readFloat(stream, prop));
                break;

            case sendprop::T_Vector: {
                std::array<float, 3> vec;
                readVector(vec, stream, prop);
                set(std::move(vec));
            } break;

            case sendprop::T_VectorXY: {
                std::array<float, 2> vec;
                readVectorXY(vec, stream, prop);
                set(std::move(vec));
            } break;

            case sendprop::T_String: {
                char str[PROPERTY_MAX_STRING_LENGTH + 1];
                uint32_t length = readString(str, PROPERTY_MAX_STRING_LENGTH, stream, prop);
                set(std::string(str, length));
            } break;

            case sendprop::T_Array:{
                std::vector<property> vec;
                readArray(vec, stream, prop);
                set(std::move(vec));
            } break;

            case sendprop::T_Int64:
                set(readInt64(stream, prop));
                break;

            default:
                BOOST_THROW_EXCEPTION( propertyInvalidType()
                    << (EArgT<1, uint32_t>::info(prop->getType()))
                );
                break;

        }
    }

    property property::create(bitstream &stream, sendprop* prop) {
        property p(prop); // filled in switch
        p.update(stream);
        return std::move(p);
    }

    int32_t property::readInt(bitstream &stream, sendprop* p) {
        const uint32_t& flags = p->getFlags();

        int32_t ret;

        if (flags & SPROP_ENCODED_AGAINST_TICKCOUNT) {
            ret = stream.readVarUInt();

            if (flags & SPROP_UNSIGNED)
                return ret;
            else
                return (-(ret & 1)) ^ (ret >> 1);
        }

        const uint32_t bits = p->getBits();
        const uint32_t sign = (0x80000000 >> (32 - bits)) * ((flags & SPROP_UNSIGNED) - 1);

        ret = stream.read(bits);
        ret = ret ^ sign;
        return (ret - sign);
    }

    float property::readFloatCoord(bitstream &stream) {
        uint32_t integer = stream.read(1);
        uint32_t fraction = stream.read(1);

        if (integer || fraction) {
            const bool sign = stream.read(1);

            if (integer)
                integer = stream.read(14) + 1;

            if (fraction)
                fraction = stream.read(5);

            double d = 0.03125 * fraction;
            d += integer;

            if (sign)
                d *= -1;

            return static_cast<float>(d);
        } else {
            return 0;
        }
    }

    float property::readFloatCoordMp(bitstream &stream, uint32_t type) {
        uint32_t value;

        if (type == SPROP_COORD_MP_INTEGRAL) {
            uint32_t a = stream.read(1);
            const uint32_t b = stream.read(1);
            a = a + 2 * b;

            if (!b) {
                return 0;
            } else {
                if (a)
                    value = stream.read(12);
                else
                    value = stream.read(15);

                if (value & 1)
                    value = -((value >> 1) + 1);
                else
                    value = (value >> 1) + 1;
            }

            return value;
        } else {
            BOOST_THROW_EXCEPTION( propertyInvalidFloatCoord() );
        }
    }

    float property::readFloatNoScale(bitstream &stream) {
        // reinterprets the memory representation of v as f
        // union nessecary when using strict aliasing semantics
        union {
            float f;
            uint32_t v;
        } u;

        u.v = stream.read(32);
        return u.f;
    }

    float property::readFloatNormal(bitstream &stream) {
        const bool sign = stream.read(1);
        uint32_t val = stream.read(11);
        float f = val;

        if (val >> 31)
            f += 4.2949673e9;

        f *= 4.885197850512946e-4;

        if (sign)
            f = -1 * f;

        return f;
    }

    float property::readFloatCellCord(bitstream &stream, uint32_t type, uint32_t bits) {
        uint32_t val = stream.read(bits);

        if (type == SPROP_CELL_COORD || type == SPROP_CELL_COORD_LOWPRECISION) {
            bool lp = (type == SPROP_CELL_COORD_LOWPRECISION);

            uint32_t fraction;
            if (!lp)
                fraction = stream.read(5);
            else
                fraction = stream.read(3);

            double d = val + (lp ? 0.125 : 0.03125) * fraction;
            return static_cast<float>(d);
        } else {
            double d = val;

            if (val >> 31) {
                d += 4.2949673e9;
            }

            return static_cast<float>(d);
        }
    }

    float property::readFloat(bitstream &stream, sendprop* prop) {
        const uint32_t flags = prop->getFlags();

        if (flags & SPROP_COORD) {
            return readFloatCoord(stream);
        } else if (flags & SPROP_COORD_MP) {
            return readFloatCoordMp(stream, SPROP_COORD_MP);
        } else if (flags & SPROP_COORD_MP_LOWPRECISION) {
            return readFloatCoordMp(stream, SPROP_COORD_MP_LOWPRECISION);
        } else if (flags & SPROP_COORD_MP_INTEGRAL) {
            return readFloatCoordMp(stream, SPROP_COORD_MP_INTEGRAL);
        } else if (flags & SPROP_NOSCALE) {
            return readFloatNoScale(stream);
        } else if (flags & SPROP_CELL_COORD) {
            return readFloatCellCord(stream, SPROP_CELL_COORD, prop->getBits());
        } else if (flags & SPROP_CELL_COORD_LOWPRECISION) {
            return readFloatCellCord(stream, SPROP_CELL_COORD_LOWPRECISION, prop->getBits());
        } else if (flags & SPROP_CELL_COORD_INTEGRAL) {
            return readFloatCellCord(stream, SPROP_CELL_COORD_INTEGRAL, prop->getBits());
        } else {
            const uint32_t dividend = stream.read(prop->getBits());
            const uint32_t divisor = (1 << prop->getBits()) - 1;

            float f = ((float) dividend) / divisor;
            float range = prop->getHighVal() - prop->getLowVal();

            return f * range + prop->getLowVal();
        }
    }

    void property::readVector(std::array<float, 3> &vec, bitstream &stream, sendprop* prop) {
        vec[0] = readFloat(stream, prop);
        vec[1] = readFloat(stream, prop);

        if (prop->getFlags() & SPROP_NORMAL) {
            const bool sign = stream.read(1);

            float f = vec[0] * vec[0] + vec[1] * vec[1];

            if (f < 0) {
                vec[2] = 0;
            } else {
                vec[2] = sqrt(1 - f);
            }

            if (sign)
                vec[2] *= -1;
        } else {
            vec[2] = readFloat(stream, prop);
        }
    }

    void property::readVectorXY(std::array<float, 2> &vec, bitstream &stream, sendprop* prop) {
        vec[0] = readFloat(stream, prop);
        vec[1] = readFloat(stream, prop);
    }

    uint32_t property::readString(char *buf, std::size_t max, bitstream &stream, sendprop* prop) {
        uint32_t length = stream.read(9);

        if (length > PROPERTY_MAX_STRING_LENGTH)
            BOOST_THROW_EXCEPTION( propertyInvalidStringLength()
                << (EArgT<1, uint32_t>::info(length))
            );

        stream.readBits(buf, 8*length);
        return length;
    }

    int64_t property::readInt64(bitstream &stream, sendprop* prop) {
        if (prop->getFlags() & SPROP_ENCODED_AGAINST_TICKCOUNT) {
            // let's hope this doesn't really want a 64 bit int or something
            // will throw an exception anyway so let's see what happens in the long run
            int64_t ret = stream.readVarUInt();
            
            if (prop->getFlags() & SPROP_UNSIGNED)
                return ret;
            else
                return (-(ret & 1)) ^ (ret >> 1);
        } else {
            bool negate = false;
            std::size_t sbits = prop->getBits() - 32; // extra bits above 32

            if (!(SPROP_UNSIGNED & prop->getFlags())) {
                --sbits;
                negate = stream.read(1);
            }

            int64_t a = stream.read(32);
            int64_t b = stream.read(sbits);
            int64_t val = (a << 32) | b;

            return negate ? -val : val;
        }
    }

    void property::readArray(std::vector<property> &props, bitstream &stream, sendprop* prop) {
        uint32_t elements = prop->getElements();
        uint32_t bits = 0;

        // equivalent to std::floor(std::log2(elements) + 1)
        while (elements) {
            ++bits;
            elements >>= 1;
        }

        const uint32_t count = stream.read(bits);

        if (count > PROPERTY_MAX_ELEMENTS)
            BOOST_THROW_EXCEPTION( propertyInvalidNumberOfElements()
                << (EArgT<1, uint32_t>::info(count))
            );

        sendprop* aType = prop->getArrayType();
        for (uint32_t i = 0; i < count; ++i) {
            props.push_back(std::move(property::create(stream, aType)));
        }
    }
}