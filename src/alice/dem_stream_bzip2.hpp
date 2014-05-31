/**
 * @file dem_stream_bzip2.hpp
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

#ifndef _DOTA_DEM_STREAM_BZIP2_HPP_
#define _DOTA_DEM_STREAM_BZIP2_HPP_

#include <alice/config.hpp>

#if DOTA_BZIP2

/// Defines a fixed amount of memory to allocate for the internal buffer for snappy
#define DOTA_SNAPPY_BUFSIZE 0x100000 // 1 MB

#include <string>
#include <fstream>
#include <utility>

#include <alice/exception.hpp>
#include <alice/dem.hpp>

namespace dota {
    /// @defgroup CORE Core
    /// @{

    /**
     * Read the contents of a bzip2-compressed demo file (Dota 2 Replay) from the harddrive.
     *
     * This class copies the entire contents of the replay into the memory.
     * It addition it allocates 1MB of fixed space to take care of decompressing
     * messages with snappy.
     */
    class dem_stream_bzip2 : public dem_stream {
        public:
            /** Constructor, allocates memory to buffer file contents */
            dem_stream_bzip2() : buffer(""), bufferSnappy(nullptr), pos(0), size(0), parsingState(0) {
                bufferSnappy = new char[DOTA_SNAPPY_BUFSIZE];
            }

            /** Copy constructor, don't allow copying */
            dem_stream_bzip2(const dem_stream_bzip2 &s) = delete;

            /** Move constructor, don't allow moving */
            dem_stream_bzip2(dem_stream_bzip2 &&stream) = delete;

            /** Destructor, free's allocated memory */
            virtual ~dem_stream_bzip2() {
                delete[] bufferSnappy;
            }

            /** Whether there are still messages left to be parsed */
            virtual bool good() {
                return (pos < size) && (parsingState != 2);
            }

            /** Opens a DEM file from the given path */
            virtual void open(std::string path);

            /** Returns a message */
            virtual demMessage_t read(const bool skip = false);

            /** Move to the desired minute in the replay */
            virtual void move(uint32_t min);
        private:
            /** Internal buffer (message) */
            std::string buffer;
            /** Internal buffer (uncompressed message) */
            char* bufferSnappy;

            /** Path to opened replay */
            std::string file;
            /** Position in the array */
            uint32_t pos;
            /** Maximum size */
            uint32_t size;
            /** Parsing state */
            uint32_t parsingState;
            /** Position cache for fullpackets */
            std::vector<uint32_t> fpackcache;

            /** Reads a varint32 from the array (protobuf serialization format) */
            uint32_t readVarInt();
    };

    /// @}
}

#endif // DOTA_BZIP2
#endif // _DOTA_DEM_STREAM_BZIP2_HPP_