/**
 * @file property.cpp
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
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

/// Define this to test if skip length equals read length
//#define TEST_SKIPPING

#include <cmath>
#include <iostream>

#include <alice/bitstream.hpp>
#include <alice/property.hpp>

namespace dota {
    /** Reads an integer from the bitstream into the given property */
    void readInt(bitstream &stream, property* p) {
        sendprop* pr = p->getSendprop();
        const uint32_t flags = pr->getFlags();

        // Read a variable size integer
        if (flags & SPROP_ENCODED_AGAINST_TICKCOUNT) {
            if (flags & SPROP_ENCODED_AGAINST_TICKCOUNT)
                p->set(stream.nReadVarUInt32());
            else
                p->set(stream.nReadVarSInt32());

            return;
        }

        // Read a 32 bit unsigned integer
        if (flags & SPROP_UNSIGNED)
            p->set(stream.nReadUInt(pr->getBits()));
        else
            p->set(stream.nReadSInt(pr->getBits()));
    }

    /** Skips an integer in the bitstream */
    void skipInt(bitstream &stream, sendprop* p) {
        const uint32_t flags = p->getFlags();

        if (flags & SPROP_ENCODED_AGAINST_TICKCOUNT) {
            stream.nSkipVarInt();
        } else {
            stream.seekForward(p->getBits());
        }
    }

    /** Reads a float from the bitstream and returns it */
    float readFloat(bitstream &stream, property* p) {
        sendprop* pr = p->getSendprop();
        const uint32_t flags = pr->getFlags();

        // Read float coordinate
        if (flags & SPROP_COORD)
            return stream.nReadCoord();

        // Read float coordinate optimized for multiplayer games
        if (flags & SPROP_COORD_MP) {
            bool integral     = flags & SPROP_COORD_MP_INTEGRAL;
            bool lowprecision = flags & SPROP_COORD_MP_LOWPRECISION;

            return stream.nReadCoordMp(integral, lowprecision);
        }

        // Read noscale float
        if (flags & SPROP_NOSCALE) {
            // The solution below is also possible but breaks strict aliasing
            // float t = *( ( float * ) &u.v );

            // Also possible but undefined behavior
            // union { float f; uint32_t v; } u;
            // u.v = stream.read(32);
            // return u,f

            uint32_t v = stream.read(32);
            float f;
            memcpy(&f, &v, 4);
            return f;
        }

        // Read normalized float
        if (flags & SPROP_NORMAL)
            return stream.nReadNormal();

        // Read cell coordinate
        if (flags & SPROP_CELL_COORD || flags & SPROP_CELL_COORD_INTEGRAL || flags & SPROP_CELL_COORD_LOWPRECISION) {
            const bool lowprecision = flags & SPROP_CELL_COORD_LOWPRECISION;
            const bool integral     = flags & SPROP_CELL_COORD_INTEGRAL;

            return stream.nReadCellCoord(pr->getBits(), integral, lowprecision);
        }

        // Read a standard float
        const uint32_t dividend = stream.read(pr->getBits());
        const uint32_t divisor = (1 << pr->getBits()) - 1;

        const float f = ((float) dividend) / divisor;
        const float range = pr->getHighVal() - pr->getLowVal();
        return f * range + pr->getLowVal();
    }

    /** Skips a float in the bitstream */
    void skipFloat(bitstream &stream, sendprop* pr) {
        const uint32_t flags = pr->getFlags();

        // Skip float coordinate
        if (flags & SPROP_COORD) {
            stream.nSkipCoord();
            return;
        }

        // Skip float coordinate optimized for multiplayer games
        if (flags & SPROP_COORD_MP) {
            const bool integral     = flags & SPROP_COORD_MP_INTEGRAL;
            const bool lowprecision = flags & SPROP_COORD_MP_LOWPRECISION;
            stream.nSkipCoordMp(integral, lowprecision);
            return;
        }

        // Skip noscale float
        if (flags & SPROP_NOSCALE) {
            stream.seekForward(32);
            return;
        }

        // Skip normalized float
        if (flags & SPROP_NORMAL) {
            stream.nSkipNormal();
            return;
        }

        // Skip cell coordinate
        if (flags & SPROP_CELL_COORD || flags & SPROP_CELL_COORD_INTEGRAL || flags & SPROP_CELL_COORD_LOWPRECISION) {
            const bool lowprecision = flags & SPROP_CELL_COORD_LOWPRECISION;
            const bool integral     = flags & SPROP_CELL_COORD_INTEGRAL;

            stream.nSkipCellCoord(pr->getBits(), integral, lowprecision);
            return;
        }

        // Skip a standard float
        stream.seekForward(pr->getBits());
    }

    /** Reads a 3D vector from the bitstream */
    void readVector(std::array<float, 3> &vec, bitstream &stream, property* prop) {
        vec[0] = readFloat(stream, prop);
        vec[1] = readFloat(stream, prop);

        sendprop* pr = prop->getSendprop();

        if (pr->getFlags() & SPROP_NORMAL) {
            const bool sign = stream.read(1);
            const float f = vec[0] * vec[0] + vec[1] * vec[1];

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

    /** Skips a 3D vector */
    void skipVector(bitstream &stream, sendprop* pr) {
        skipFloat(stream, pr);
        skipFloat(stream, pr);

        if (pr->getFlags() & SPROP_NORMAL) {
            stream.seekForward(1);
        } else {
            skipFloat(stream, pr);
        }
    }

    /** Reads a 2D vector from the bitstream */
    void readVectorXY(std::array<float, 2> &vec, bitstream &stream, property* prop) {
        vec[0] = readFloat(stream, prop);
        vec[1] = readFloat(stream, prop);
    }

    /** Skips a 2D vector */
    void skipVectorXY(bitstream &stream, sendprop* pr) {
        skipFloat(stream, pr);
        skipFloat(stream, pr);
    }

    /** Reads a string from the stream, returns it's length */
    uint32_t readString(char *buf, bitstream &stream) {
        const uint32_t length = stream.read(9);

        if (length > PROPERTY_MAX_STRING_LENGTH)
            BOOST_THROW_EXCEPTION( propertyInvalidStringLength()
                << (EArgT<1, uint32_t>::info(length))
            );

        stream.readBits(buf, 8*length);
        return length;
    }

    /** Skips a string from the stream */
    void skipString(bitstream &stream) {
        const uint32_t length = stream.read(9);
        stream.nSkipString(length);
    }

    /** Reads a 64-bit integer from the stream */
    void readInt64(bitstream &stream, property* prop) {
        sendprop* pr = prop->getSendprop();

        if (pr->getFlags() & SPROP_ENCODED_AGAINST_TICKCOUNT) {
            if (pr->getFlags() & SPROP_UNSIGNED)
                return prop->set(stream.nReadVarUInt64());
            else
                return prop->set(stream.nReadVarSInt64());
        } else {
            bool negate = false;
            std::size_t sbits = pr->getBits() - 32; // extra bits above 32

            if (!(SPROP_UNSIGNED & pr->getFlags())) {
                --sbits;
                negate = stream.read(1);
            }

            const int64_t a = stream.read(32);
            const int64_t b = stream.read(sbits);
            const int64_t val = (b << 32) | a;

            if (negate)
                prop->set(-val);
            else
                prop->set(val);
        }
    }

    /** Skips a 64 bit integer */
    void skipInt64(bitstream &stream, sendprop* pr) {
        if (pr->getFlags() & SPROP_ENCODED_AGAINST_TICKCOUNT)
            stream.nSkipVarInt();
        else
            stream.seekForward(pr->getBits());
    }

    /** Read an array from the stream */
    void readArray(std::vector<property> &props, bitstream &stream, sendprop* prop) {
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

    /** Skips an array */
    void skipArray(bitstream &stream, sendprop* prop) {
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
            property::skip(stream, aType);
        }
    }

    void property::update(bitstream &stream) {
        #ifdef TEST_SKIPPING
            uint32_t cur = stream.position();
            property::skip(stream, prop);
            uint32_t diff1 = stream.position() - cur;
            stream.seekBackward(diff1);

            // assert that seeking backwards worked and we have a clean state
            assert ( cur == stream.position() );
        #endif // TEST_SKIPPING

        switch (prop->getType()) {
            // Read Integer
            case sendprop::T_Int:
                readInt(stream, this);
                break;

            // Read Float
            case sendprop::T_Float:
                set(readFloat(stream, this));
                break;

            // Read 3D Vector
            case sendprop::T_Vector: {
                std::array<float, 3> vec;
                readVector(vec, stream, this);
                set(std::move(vec));
            } break;

            // Read 2D
            case sendprop::T_VectorXY: {
                std::array<float, 2> vec;
                readVectorXY(vec, stream, this);
                set(std::move(vec));
            } break;

            // Read String
            case sendprop::T_String: {
                char str[PROPERTY_MAX_STRING_LENGTH + 1];
                uint32_t length = readString(str, stream);
                set(std::string(str, length));
            } break;

            // Read Array
            case sendprop::T_Array:{
                std::vector<property> vec;
                readArray(vec, stream, prop);
                set(std::move(vec));
            } break;

            // Read 64 bit Integer
            case sendprop::T_Int64:
                readInt64(stream, this);
                break;

            default:
                BOOST_THROW_EXCEPTION( propertyInvalidType()
                    << (EArgT<1, uint32_t>::info(prop->getType()))
                );
                break;
        }

        #ifdef TEST_SKIPPING
            uint32_t diff2 = stream.position() - cur;
            if (diff2 != diff1) {
                std::cout << "Skip failed: " << diff1 << " (skip) " << diff2 << " (read) " << std::endl;
                std::cout << "Type ID: " << prop->getType() << std::endl;
                exit(0);
            }
        #endif // TEST_SKIPPING
    }

    property property::create(bitstream &stream, sendprop* prop) {
        property p(prop); // filled in switch
        p.update(stream);
        p.init = true;
        return std::move(p);
    }

    void property::skip(bitstream& stream, sendprop* prop) {
        switch (prop->getType()) {
            // Skip over Integer
            case sendprop::T_Int:
                skipInt(stream, prop);
                break;

            // Skip over Float
            case sendprop::T_Float:
                skipFloat(stream, prop);
                break;

            // Skip over 3D Vector
            case sendprop::T_Vector:
                skipVector(stream, prop);
                break;

            // Skip 2D Vector
            case sendprop::T_VectorXY:
                skipVectorXY(stream, prop);
                break;

            // Skip String
            case sendprop::T_String:
                skipString(stream);
                break;

            // Skip Array
            case sendprop::T_Array:
                skipArray(stream, prop);
                break;

            // Skip 64 bit Integer
            case sendprop::T_Int64:
                skipInt64(stream, prop);
                break;

            default:
                BOOST_THROW_EXCEPTION( propertyInvalidType()
                    << (EArgT<1, uint32_t>::info(prop->getType()))
                );
                break;
        }
    }
}