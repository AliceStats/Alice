/**
 * @file dem_stream_memory.cpp
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
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License. *
 */

#include <set>
#include <snappy.h>

#include <alice/config.hpp>
#include <alice/demo.pb.h>
#include <alice/dem_stream_memory.hpp>

namespace dota {
    void dem_stream_memory::open(std::string path) {
        // open stream
        std::ifstream stream(path.c_str(), std::ifstream::in | std::ifstream::binary);
        D_( std::cout << "[dem_stream] Opening replay: " << path << " " << D_FILE << " " << __LINE__ << std::endl;, 1 )

        // check if it was successful
        if (!stream.is_open())
            BOOST_THROW_EXCEPTION(demFileNotAccessible()
                << EArg<1>::info(path)
            );

        // check filesize
        const std::streampos fstart = stream.tellg();
        stream.seekg (0, std::ios::end);
        size = stream.tellg() - fstart;
        stream.seekg(fstart);
        D_( std::cout << "[dem_stream] Filesize: " << size << " " << D_FILE << " " << __LINE__ << std::endl;, 1 )

        if (size < sizeof(demHeader_t))
            BOOST_THROW_EXCEPTION((demFileTooSmall()
                << EArg<1>::info(path)
                << EArgT<2, std::streampos>::info(this->size)
                << EArgT<3, std::size_t>::info(sizeof(demHeader_t))
            ));

        // read everything into the buffer
        buffer = new char[size];
        stream.read(buffer, size);
        stream.close();

        // verify the header
        demHeader_t head;
        memcpy((char*) &head, buffer, sizeof(demHeader_t));
        if(strcmp(head.headerid, DOTA_DEMHEADERID))
            BOOST_THROW_EXCEPTION(demHeaderMismatch()
                << EArg<1>::info(path)
                << EArg<2>::info(std::string(head.headerid, 8))
                << EArg<3>::info(std::string(DOTA_DEMHEADERID))
            );

        // save path for later
        file = path;

        // current position
        pos = sizeof(demHeader_t);
    }

    demMessage_t dem_stream_memory::read(const bool skip) {
        // Make sure the buffers has been allocated
        assert(buffer != nullptr);
        assert(bufferSnappy != nullptr);

        // Static default list of packages that are not parsed
        static std::set<uint32_t> skips {
            1, 2, 3, 9, 10, 11, 12, 13, 14
        };

        // Get type / tick / size
        uint32_t type = readVarInt();
        const bool compressed = type & DEM_IsCompressed;
        type = (type & ~DEM_IsCompressed);

        uint32_t tick = readVarInt();
        uint32_t size = readVarInt();

        // Check if this is the last message
        if (parsingState == 1) parsingState = 2;
        if (type == 0)         parsingState = 1; // marks the message before the laste one

        // skip messages if skip is set
        if (skip && skips.count(type)) {
            pos += size; // seek forward
            D_( std::cout << "[dem_stream] Skipping Message: " << " " << type << D_FILE << " " << __LINE__ << std::endl;, 2 )
            return demMessage_t{false, 0, 0, nullptr, 0}; // return empty msg
        }

        D_( std::cout << "[dem_stream] Reading Message: " << type << D_FILE << " " << __LINE__ << std::endl;, 3 )

        // read into char array and convert to string
        if (size > this->size)
            BOOST_THROW_EXCEPTION((demMessageToBig()
                << EArgT<1, std::size_t>::info(size)
            ));

        demMessage_t msg {compressed, tick, type, nullptr, 0}; // return msg
        char* buffer = &this->buffer[pos];
        pos += size;

        // Check if we need to uncompress
        if (compressed && snappy::IsValidCompressedBuffer(buffer, size)) {
            D_( std::cout << "[dem_stream] Uncompressing Message: " << " " << type << D_FILE << " " << __LINE__ << std::endl;, 3 )
            std::size_t uSize;

            // Check if we can get the output length
            if (!snappy::GetUncompressedLength(buffer, size, &uSize)) {
                BOOST_THROW_EXCEPTION((demInvalidCompression()
                    << EArg<1>::info(file)
                    << EArgT<2, uint32_t>::info(this->pos)
                    << EArgT<3, std::size_t>::info(size)
                    << EArgT<4, uint32_t>::info(type)
                ));
            }

            // Check if it fits in the buffer
            if (uSize > DOTA_SNAPPY_BUFSIZE)
                BOOST_THROW_EXCEPTION((demMessageToBig()
                    << EArgT<1, std::size_t>::info(uSize)
                ));

            // Make sure its uncompressed
            if (!snappy::RawUncompress(buffer, size, bufferSnappy))
                BOOST_THROW_EXCEPTION((demInvalidCompression()
                    << EArg<1>::info(file)
                    << EArgT<2, uint32_t>::info(this->pos)
                    << EArgT<3, std::size_t>::info(size)
                    << EArgT<4, uint32_t>::info(type)
                ));

            msg.msg = bufferSnappy;
            msg.size = uSize;
        } else {
            msg.msg = buffer;
            msg.size = size;
        }

        return msg;
    }

    void dem_stream_memory::move(uint32_t min) {
        // generate the cache
        if (fpackcache.empty()) {
            pos = sizeof(demHeader_t);
            fpackcache.push_back(pos); // 0 min start

            // Get type / tick / size
            uint32_t type = 0;

            do {
                uint32_t p = pos;
                type = readVarInt() & ~DEM_IsCompressed;
                uint32_t tick = readVarInt();
                uint32_t size = readVarInt();

                if (type == 13) {
                    fpackcache.push_back(p);
                    D_( std::cout << "[dem_stream] Adding fullpacket at position " << " " << p << D_FILE << " " << __LINE__ << std::endl;, 3 )
                }

                pos += size;
            } while (type != 0);
        }

        // seek to the fullpacket at the desired position
        if (fpackcache.size() <= min)
            min = fpackcache.size() - 1;

        pos = fpackcache[min];
    }

    uint32_t dem_stream_memory::readVarInt() {
        char buf;
        uint32_t count = 0;
        uint32_t result = 0;

        do {
            if (count == 5) {
                BOOST_THROW_EXCEPTION(demCorrupted()
                    << EArg<1>::info(file)
                );
            } else if (!good()) {
                BOOST_THROW_EXCEPTION(demUnexpectedEOF()
                    << EArg<1>::info(file)
                );
            } else {
                buf = buffer[pos];
                result |= (uint32_t)(buf & 0x7F) << ( 7 * count );
                ++count;
                ++pos;
            }
        } while (buf & 0x80);

        return result;
    }
}