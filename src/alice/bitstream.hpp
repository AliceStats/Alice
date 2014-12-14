/**
 * @file bitstream.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.3
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
 * @par Source
 *    You can find most of the things implemented in this bitstream in the public
 *    Source Engine SDK released by Valve.
 */

#ifndef _DOTA_BITSTREAM_HPP_
#define _DOTA_BITSTREAM_HPP_

#include <string>
#include <vector>

#include <alice/exception.hpp>
#include <alice/config.hpp>

/// Defines the maximum possible size of a bitstream
///
/// This requirement is verly likely not defined in the source engine. In our case however this is helpful
/// towards mitigating problems from malicious replays by limiting the amount of data a single entity or
/// stringtable update can pass to a bitstream.
#define DOTA_BITSTREAM_MAX_SIZE 65536

/// Defines the underlying type used to represent the data
#if DOTA_64BIT
    #define DOTA_BITSTREAM_WORD_TYPE uint64_t
#else
    #define DOTA_BITSTREAM_WORD_TYPE uint32_t // slightly faster on 32 bit systems
#endif

/// Number of fraction bits in a normalized float
#define NORMAL_FRACTION_BITS 11
/// Normal denominator
#define NORMAL_DENOMINATOR ( (1<<(NORMAL_FRACTION_BITS)) - 1 )
/// Normal resolution
#define NORMAL_RESOLUTION (1.0/(NORMAL_DENOMINATOR))
/// Maximum number of bytes to read for a 32 bit varint (32/8 + 1)
#define VARINT32_MAX 5
/// Maximum number of bytes to read for a 64 bit varint (64/8 + 2)
#define VARINT64_MAX 10
/// Number of bits to read for the integer part of a coord
#define COORD_INTEGER_BITS 14
/// Number of bits to read for the fraction part of a coord
#define COORD_FRACTION_BITS 5
/// Coord demoniator
#define COORD_DENOMINATOR (1<<(COORD_FRACTION_BITS))
/// Coord resolution
#define COORD_RESOLUTION (1.0/(COORD_DENOMINATOR))
/// Bits to read for multiplayer optimized coordinates
#define COORD_INTEGER_BITS_MP 11
/// Fractional part of low-precision coords
#define COORD_FRACTION_BITS_MP_LOWPRECISION 3
/// Denominator for low-precision coords
#define COORD_DENOMINATOR_LOWPRECISION (1<<(COORD_FRACTION_BITS_MP_LOWPRECISION))
/// Resolution for low-precision coords
#define COORD_RESOLUTION_LOWPRECISION (1.0/(COORD_DENOMINATOR_LOWPRECISION))
/// Number of bits to read for the fraction part of a cell coord
#define CELL_COORD_FRACTION_BITS 5
/// Number of bits to read for a low-precision cell coord's fraction
#define CELL_COORD_FRACTION_BITS_LOWPRECISION 3

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// This exception is thrown when the read size exceeds the size of the remaining data.
    CREATE_EXCEPTION( bitstreamOverflow, "More bits requested than available" )

    /// This exception is thrown when an unlikely large chunk of data is submitted.
    /// The exception is tied to the value #DOTA_BITSTREAM_MAX_SIZE
    CREATE_EXCEPTION( bitstreamDataSize, "Unlikely large chunk of data submitted" )

    /// @}
    /// @defgroup CORE Core
    /// @{

    /**
     * Converts a string into a read-only bitstream.
     *
     * The string is converted into a vector of word_t. It keeps track of the data read
     * and adjusts its position accordingly. Size values are mostly measured in bits.
     *
     * Methods starting with "n" provide facilities to read network encoded data from the
     * bitstream.
     *
     * Thanks to edith for providing an initial implementation.
     */
    class bitstream {
        public:
            /** Underlying type used to represent a chunk of data */
            typedef DOTA_BITSTREAM_WORD_TYPE word_t;
            /** Type used to keep track of the stream position */
            typedef std::size_t size_type;

            /** Creates an empty bitstream */
            bitstream() : data{}, pos{0}, size{0} { }

            /** Creates a bitstream from a std::string */
            bitstream(const std::string &str) : data{}, pos{0}, size{str.size()*8} {
                if (str.size() > DOTA_BITSTREAM_MAX_SIZE)
                    BOOST_THROW_EXCEPTION( bitstreamDataSize() << (EArgT<1, std::size_t>::info(str.size())) );

                // Reserve the memory in beforehand so we can just memcpy everything
                data.resize((str.size() + 3) / 4 + 1);
                memcpy(&data[0], str.c_str(), str.size());

                D_( std::cout << "[bitstream] Reserving " << (str.size() + 3) / 4 + 1 << " bytes of memory " << D_FILE << " " << __LINE__ << std::endl;, 2 )
            }

            /** Copy-Constructor */
            bitstream(const bitstream& b) : data(b.data), pos(b.pos), size(b.size) {}

            /** Move-Constructor */
            bitstream(bitstream&& b) : data(std::move(b.data)), pos(b.pos), size(b.size) {
                b.data.clear();
                b.pos = 0;
                b.size = 0;
            }

            /** Destructor */
            ~bitstream() = default;

            /** Assignment operator */
            bitstream& operator= (bitstream t) {
                swap(t);
                return *this;
            }

            /** Swap this bitstream with given one */
            void swap(bitstream& b) {
                std::swap(data, b.data);
                std::swap(pos, b.pos);
                std::swap(size, b.size);
            }

            /** Checkes whether there is still data left to be read. */
            inline bool good() const {
                return pos < size;
            }

            /** Returns the size of the stream in bits. */
            inline size_type end() const {
                return size;
            }

            /** Returns the current position of the stream in bits. */
            inline size_type position() const {
                return pos;
            }

            /**
             * Returns result of reading n bits into an uint32_t.
             *
             * This function can read a maximum of 32 bits at once. If the amount of data requested
             * exceeds the remaining size of the current chunk it wraps around to the next one.
             */
            uint32_t read(const size_type n);

            /**
             * Seek n bits forward.
             *
             * If the resulting position would overflow, it is set to the maximum one possible.
             */
            void seekForward(const size_type n) {
                pos += n;

                if (pos > size) {
                    D_( std::cout << "[bitstream] Overflowing by " << pos-size << " bytes " << D_FILE << " " << __LINE__ << std::endl;, 1 )
                    pos = size;
                }
            }

            /**
             * Seek n bits backward.
             *
             * If the resulting position would underflow, it is set to 0.
             */
            void seekBackward(const size_type n) {
                if ((pos - n) > pos) {
                    D_( std::cout << "[bitstream] Underflowing " << D_FILE << " " << __LINE__ << std::endl;, 1 )
                    pos = 0;
                } else {
                    pos -= n;
                }
            }

            /**
             * Read an unsigned integer from the stream.
             *
             * n corresponds to the number of bits in the sendprop.
             */
            uint32_t nReadUInt(const size_type n) {
                return read(n);
            }

            /**
             * Read a signed integer from the stream.
             *
             * n corresponds to the number of bits in the sendprop.
             */
            int32_t nReadSInt(const size_type n) {
                int32_t ret = read(n);
                const uint32_t sign = 1 << (n - 1);

                if (ret >= sign)
                    ret = ret - sign - sign;

                return ret;
            }

            /**
             * Read a normalized float from the stream.
             *
             * Reads one bit for the sign and NORMAL_FRACTION_BITS for the float.
             */
            float nReadNormal() {
                const bool sign = read(1);
                const float fraction = read( NORMAL_FRACTION_BITS );
                const float ret = fraction * NORMAL_RESOLUTION;

                if (sign)
                    return -ret;
                else
                    return ret;
            }

            /** Skips a normalized float in the stream. */
            void nSkipNormal() {
                // sign bit + fraction bits
                seekForward(NORMAL_FRACTION_BITS + 1);
            }

            /**
             * Reads a variable sized uint32_t from the stream.
             *
             * A variable int is read in chunks of 8. The first 7 bits are added to return value
             * while the last bit is the indicator whether to continue reading.
             */
            uint32_t nReadVarUInt32();

            /** Reads a variable sized uint64_t from the stream. */
            uint64_t nReadVarUInt64();

            /**
             * Reads a variable sized int32_t from the stream.
             *
             * The signed version of the varint uses protobuf's zigzag encoding for
             * the sign part.
             */
            int32_t nReadVarSInt32() {
                const uint32_t value = nReadVarUInt32();
                return (value >> 1) ^ -static_cast<int32_t>(value & 1);
            }

            /** Reads a variable sized int64_t from the stream. */
            int64_t nReadVarSInt64() {
                const uint64_t value = nReadVarUInt64();
                return (value >> 1) ^ -static_cast<int64_t>(value & 1);
            }

            /**
             * Skips over a variable sized int.
             *
             * This function will skip 7 bits and read 1.
             */
            void nSkipVarInt() {
                do { seekForward(7); } while (read(1));
            }

            /** Reads networked coordinates from the bitstream. */
            float nReadCoord();

            /**
             * Skips coordinates in bitstream.
             *
             * This function reads 2 bits to determine the amount of skipped bits
             */
            void nSkipCoord() {
                switch( read(2) ) {
                    case 1:
                        seekForward( COORD_INTEGER_BITS + 1 );
                        break;
                    case 2:
                        seekForward( COORD_FRACTION_BITS + 1 );
                        break;
                    case 3:
                        seekForward( COORD_INTEGER_BITS + COORD_FRACTION_BITS + 1 );
                        break;
                }
            }

            /**
             * Reads coordinates, version optimized for multi player games.
             *
             * Float Encoding:   [ Inbound | IsInteger |  IsSigned  | [Int Part] | Float Part ]
             * Integer Encoding: [ Inbound | IsInteger | [IsSigned] | [Int part] ]
             */
            float nReadCoordMp(const bool integral, const bool lowPrecision);

            /**
             * Skips coordinates optimized towards multiplayer games.
             *
             * This function needs to read 2 - 3 bits in order to determine the amount required.
             */
            void nSkipCoordMp(const bool integral, const bool lowPrecision);

            /** Reads cell coordinate from their network representation. */
            float nReadCellCoord(size_type n, const bool integral, const bool lowPrecision);

            /**
             * Skips cell coordinate.
             *
             * Can fully skip the coord based on the sendprop flags.
             */
            void nSkipCellCoord(size_type n, const bool integral, const bool lowPrecision) {
                if (!integral)
                    lowPrecision ? seekForward( n + 3 ) : seekForward( n + 5 );
                else
                    seekForward(n);
            }

            /**
             * Reads a null-terminated string into the buffer, stops once it reaches \0 or n chars.
             *
             * n is treated as the number of bytes to be read.
             * n can be arbitrarily large in this context. The underlying read method throws in case an overflow
             * happens.
             */
            void nReadString(char *buffer, const size_type n);

            /**
             * Skips over a 0 terminated string.
             *
             * This function will always read the full 8 bits per character until the null
             * terminator occours.
             */
            void nSkipString(const size_type n) {
                for (std::size_t i = 0; i < n; ++i) {
                    if (static_cast<char>(read(8)) == '\0')
                        return;
                }
            }

            /**
             * Reads the exact number of bits into the buffer.
             *
             * The function reads in chunks of 8 bit until n is smaller than that
             * and appends the left over bits
             */
            void readBits(char *buffer, const size_type n);
        private:
            /** Data to read from */
            std::vector<word_t> data;
            /** Current position in the vector in bits */
            size_type pos;
            /** Overall size of the data in bits */
            size_type size;

            /** Bitmask for reading */
            static const uint64_t masks[64];
            /** Shift amount for reading */
            static const uint64_t shift[64];
    };

    /// @}
}


#endif /* _DOTA_BITSTREAM_HPP_ */