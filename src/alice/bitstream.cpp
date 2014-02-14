/**
 * @file bitstream.cpp
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
 */

#include <cstring>
#include <cstdio>

#include <alice/bitstream.hpp>

namespace dota {
    uint32_t bitstream::read(bitstream::size_type n) {
        // make sure the data read fits the return type
        if (n > size-pos) // cant read more than what is left over
            BOOST_THROW_EXCEPTION(bitstreamOverflow()
                << (EArgT<1, size_type>::info(n))
                << (EArgT<2, size_type>::info(size))
            );

        if (n > 32) // cant read more than sizeof(uint32_t)
            BOOST_THROW_EXCEPTION(bitstreamOverflow()
                << (EArgT<1, size_type>::info(n))
                << (EArgT<2, size_type>::info(32))
            );

        uint32_t start = pos / 32;
        uint32_t end = (pos + n - 1) / 32;
        uint32_t s = (pos % 32);
        uint32_t ret;

        if (start == end) {
            ret = (data[start] >> shift[s]) & masks[n];
        } else { // wrap around
            ret = ((data[start] >> shift[s]) | (data[end] << (32 - shift[s]))) & masks[n];
        }

        pos += n;
        return ret;
    }

    uint32_t bitstream::readVarUInt() {
        std::size_t readCount = 0;
        uint32_t value = 0;

        uint32_t tmpBuf;
        do {
            tmpBuf = read(8);

            uint32_t lower = tmpBuf & 0x7F;

            value |= lower << readCount;
            readCount += 7;
        } while ((tmpBuf >> 7) && readCount < 35);

        return value;
    }

    void bitstream::readString(char *buffer, bitstream::size_type n) {
        // If an overflow happens during this function it's trigger in read()
        // We can't check for size here because reading stops at \0 and the data
        // requested might be arbitrary

        for (std::size_t i = 0; i < n; ++i) {
            buffer[i] = static_cast<char>(read(8));

            if (buffer[i] == '\0')
                return;
        }

        buffer[n - 1] = '\0';
    }

    void bitstream::readBits(char *buffer, bitstream::size_type n) {
        if (n > size)
            BOOST_THROW_EXCEPTION(bitstreamOverflow()
                << (EArgT<1, size_type>::info(n))
                << (EArgT<2, size_type>::info(size))
            );

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