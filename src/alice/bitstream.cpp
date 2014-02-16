/**
 * @file bitstream.cpp
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
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <alice/bitstream.hpp>

namespace dota {
    uint32_t bitstream::read(bitstream::size_type n) {
        // make sure the data read fits the return type
        if (n > size-pos) // cant read more than what is left over
            BOOST_THROW_EXCEPTION(bitstreamOverflow()
                << (EArgT<1, size_type>::info(n))
                << (EArgT<2, size_type>::info(size))
            );

        if (n > 32)
            BOOST_THROW_EXCEPTION(bitstreamOverflow()
                << (EArgT<1, size_type>::info(n))
                << (EArgT<2, size_type>::info(32))
            );

        uint32_t bitSize = sizeof(word_t)*8;        // size of chunk in bits
        uint32_t start   = pos / bitSize;           // current active chunk
        uint32_t end     = (pos + n - 1) / bitSize; // next chunk if data needs to be wrapped
        uint32_t s       = (pos % bitSize);         // shift amount
        uint32_t ret;                               // return value

        if (start == end) {
            ret = (data[start] >> shift[s]) & masks[n];
        } else { // wrap around
            ret = ((data[start] >> shift[s]) | (data[end] << (bitSize - shift[s]))) & masks[n];
        }

        pos += n;
        return ret;
    }

    uint32_t bitstream::nReadVarUInt32() {
        uint32_t readCount = 0;
        uint32_t value = 0;

        uint32_t tmpBuf;
        do {
            if (readCount == VARINT32_MAX) // return when maximum number of iterations is reached
                return value;

            tmpBuf = read(8);
            value |= (tmpBuf & 0x7F) << (7*readCount);
            ++readCount;
        } while (tmpBuf & 0x80);

        return value;
    }

    uint64_t bitstream::nReadVarUInt64() {
        uint32_t readCount = 0;
        uint64_t value = 0;

        uint32_t tmpBuf;
        do {
            if (readCount == VARINT64_MAX)
                return value;

            tmpBuf = read(8);
            value |= static_cast<uint64_t>(tmpBuf & 0x7F) << (7*readCount);
            ++readCount;
        } while (tmpBuf & 0x80);

        return value;
    }

    float bitstream::nReadCoord() {
        uint32_t intval   = read(1); // integer part
        uint32_t fractval = read(1); // fraction part

        if ( intval || fractval ) {
            float    ret  = 0.0f;    // return value
            uint32_t sign = read(1); // signed / unsigned flag

            if (intval)
                intval = read( COORD_INTEGER_BITS ) + 1;

            if (fractval)
                // shift from [0..MAX-1] to (1..MAX)
                fractval = read( COORD_FRACTION_BITS );

            ret = intval + (static_cast<float>(fractval) * COORD_RESOLUTION);

            if (sign)
                return -ret;
            else
                return ret;
        }

        return 0.0f;
    }
    float bitstream::nReadCoordMp(bool integral, bool lowPrecision) {
        // Read flags depending on type
        uint32_t flags = integral ? read(3) : read(2);
        uint32_t flag_inbound = 1;
        uint32_t flag_intval  = 2;
        uint32_t flag_sign    = 4;

        if (integral) {
            if (flags & flag_intval) {
                uint32_t toRead = (flags & flag_inbound) ? COORD_INTEGER_BITS_MP+1 : COORD_INTEGER_BITS+1;
                uint32_t bits = read(toRead);

                // shift from [0..MAX-1] to [1..MAX], fist part of bits is the sign type
                int intval = (bits >> 1) + 1;
                return (bits & 1) ? -intval : intval;
            }

            return 0.0f;
        }

        // multiplication table
        static const float mult[4] = {
             1.f / (1 << COORD_FRACTION_BITS),
            -1.f / (1 << COORD_FRACTION_BITS),
             1.f / (1 << COORD_FRACTION_BITS_MP_LOWPRECISION),
            -1.f / (1 << COORD_FRACTION_BITS_MP_LOWPRECISION)
        };

        // multiplication val
        float multiply = mult[ (flags & flag_sign ? 1 : 0) + lowPrecision*2];

        // bits to read depending on type
        static const unsigned char bits[8] = {
            COORD_FRACTION_BITS,
            COORD_FRACTION_BITS,
            COORD_FRACTION_BITS + COORD_INTEGER_BITS,
            COORD_FRACTION_BITS + COORD_INTEGER_BITS_MP,
            COORD_FRACTION_BITS_MP_LOWPRECISION,
            COORD_FRACTION_BITS_MP_LOWPRECISION,
            COORD_FRACTION_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS,
            COORD_FRACTION_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS_MP
        };

        uint32_t val = read( bits[ (flags & (flag_inbound|flag_intval)) + lowPrecision*4] );

        if (flags & flag_intval) {
            // Remap the integer part from [0,N] to [1,N+1] and paste
            //  it in front of the fractional parts.

            uint32_t fracMp  = val >> COORD_INTEGER_BITS_MP;
            uint32_t frac    = val >> COORD_INTEGER_BITS;

            uint32_t maskMp  = ((1<<COORD_INTEGER_BITS_MP)-1);
            uint32_t mask    = ((1<<COORD_INTEGER_BITS)-1);

            uint32_t selectNotMp = (flags & flag_inbound) - 1;

            frac -= fracMp;
            frac &= selectNotMp;
            frac += fracMp;

            mask -= maskMp;
            mask &= selectNotMp;
            mask += maskMp;

            uint32_t intpart      = (val & mask) + 1;
            uint32_t intbitsLow   = intpart << COORD_FRACTION_BITS_MP_LOWPRECISION;
            uint32_t intbits      = intpart << COORD_FRACTION_BITS;
            uint32_t selectNotLow = static_cast<uint32_t>(lowPrecision) - 1;

            intbits -= intbitsLow;
            intbits &= selectNotLow;
            intbits += intbitsLow;

            val = frac | intbits;
        }

        return static_cast<int32_t>(val) * multiply;
    }

    void bitstream::nSkipCoordMp(bool integral, bool lowPrecision) {
        // Read flags depending on type
        uint32_t flags = integral ? read(3) : read(2);
        uint32_t flag_inbound = 1;
        uint32_t flag_intval  = 2;

        if (integral && (flags & flag_intval)) {
            uint32_t toRead = (flags & flag_inbound) ? COORD_INTEGER_BITS_MP+1 : COORD_INTEGER_BITS+1;
            seekForward(toRead);
        }

        // bits to skip depending on type
        static const unsigned char bits[8] = {
            COORD_FRACTION_BITS,
            COORD_FRACTION_BITS,
            COORD_FRACTION_BITS + COORD_INTEGER_BITS,
            COORD_FRACTION_BITS + COORD_INTEGER_BITS_MP,
            COORD_FRACTION_BITS_MP_LOWPRECISION,
            COORD_FRACTION_BITS_MP_LOWPRECISION,
            COORD_FRACTION_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS,
            COORD_FRACTION_BITS_MP_LOWPRECISION + COORD_INTEGER_BITS_MP
        };

        seekForward( bits[ (flags & (flag_inbound|flag_intval)) + lowPrecision*4] );
    }

    float bitstream::nReadCellCoord(size_type n, bool integral, bool lowPrecision) {
        uint32_t val = read(n);

        if (integral) {
            if (val & 0x80)
                val += 4.2949673e9;

            return (float)val;
        } else {
            uint32_t fraction = lowPrecision ? read(CELL_COORD_FRACTION_BITS_LOWPRECISION) : read(CELL_COORD_FRACTION_BITS);
            float f = val + (lowPrecision ? 0.125 : 0.03125) * fraction;

            return f;
        }
    }

    void bitstream::readString(char *buffer, bitstream::size_type n) {
        for (std::size_t i = 0; i < n; ++i) {
            buffer[i] = static_cast<char>(read(8));

            if (buffer[i] == '\0')
                return;
        }

        buffer[n - 1] = '\0';
    }

    void bitstream::readBits(char *buffer, bitstream::size_type n) {
        size_type remaining = n;
        size_type i = 0;

        while (remaining >= 8) {
            buffer[i++] = read(8);
            remaining -= 8;
        }

        if (remaining > 0) {
            buffer[i++] = read(remaining);
        }
    }
}