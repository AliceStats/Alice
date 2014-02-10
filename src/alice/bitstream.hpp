/**
 * @file bitstream.hpp
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
/// stringtable update can possible pass to a bitstream.
#define DOTA_BITSTREAM_MAX_SIZE 0x10000 // 65536

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
     * The string is converted into a vector of unsigned 32-bit integers. It keeps track of the data read
     * and adjusts its position accordingly. Size values returned are measured in bit. This class is can
     * not be copied.
     *
     * Thanks to edith for providing a reference implementation.
     */
    class bitstream {
        public:
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
            }
            
            /** Copy-Constructor */
            bitstream(const bitstream& b) : data{b.data}, pos{b.pos}, size{b.size} { }
            
            /** Move-Constructor */
            bitstream(bitstream&& b) : data{std::move(b.data)}, pos{b.pos}, size{b.size} {
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

            /** Checked whether there is still data left to be read. */
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

            /** Reads n characters from the steam with a maximum of 32 at once */
            uint32_t read(size_type n);
            /** Reads a variable sized uint32_t from the stream */
            uint32_t readVarUInt();
            /** Reads a null-terminated string into the buffer, stops once it reaches \0 */
            void readString(char *buffer, size_type n);
            /** Reads the exact number of bits into the buffer */
            void readBits(char *buffer, size_type n);
        private:
            /** Data to read from */
            std::vector<uint32_t> data;
            /** Current position in the vector in bits */
            size_type pos;
            /** Overall size of the data in bits */
            size_type size;
    };

    /// @}
}


#endif /* _DOTA_BITSTREAM_HPP_ */