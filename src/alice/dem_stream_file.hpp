/**
 * @file dem_stream_file.hpp
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
 */

#ifndef _DOTA_DEM_STREAM_FILE_HPP_
#define _DOTA_DEM_STREAM_FILE_HPP_

/// Defines a fixed amount of memory to allocate for the internal buffer of the stream
#define DOTA_DEM_BUFSIZE 0x100000 // 1 MB

#include <string>
#include <fstream>
#include <utility>

#include <alice/exception.hpp>
#include <alice/dem.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when the dem_stream_file object has been accessed after being moved
    CREATE_EXCEPTION( streamInvalidState, "Accessing stream in invalid state." )

    /// @}
    /// @defgroup CORE Core
    /// @{

    /**
     * Read the contents of a demo file (Dota 2 Replay) from the harddrive.
     *
     * This class allocates a fixed buffer of 2 MB to be used for copy-less
     * message return as well as uncompressing the data.
     */
    class dem_stream_file : public dem_stream {
        public:
            /** Constructor, allocates memory to buffer file contents */
            dem_stream_file() : buffer(nullptr), bufferSnappy(nullptr), parsingState(0) {
                buffer = new char[DOTA_DEM_BUFSIZE];
                bufferSnappy = new char[DOTA_DEM_BUFSIZE];
            }

            /** Copy constructor, don't allow copying */
            dem_stream_file(const dem_stream_file &s) = delete;

            /** Move constructor, don't allow moving */
            dem_stream_file(dem_stream_file &&stream) = delete;

            /** Destructor, free's allocated memory */
            virtual ~dem_stream_file() {
                delete[] buffer;
                delete[] bufferSnappy;

                if (stream.is_open())
                    stream.close();
            }

            /** Whether there are still messages left to be parsed */
            virtual bool good() {
                return stream.good() && (parsingState != 2);
            }

            /** Opens a DEM file from the given path */
            virtual void open(std::string path);

            /** Returns next message from current position */
            virtual demMessage_t read(const bool skip = false);

            /** Move to the desired minute in the replay */
            virtual void move(uint32_t min);
        private:
            /** Internal buffer (message) */
            char* buffer;
            /** Internal buffer (uncompressed message) */
            char* bufferSnappy;

            /** Path to opened replay */
            std::string file;
            /** Underlying ifstream */
            std::ifstream stream;
            /** Parsing state */
            uint32_t parsingState;
            /** Position cache for fullpackets */
            std::vector<uint32_t> fpackcache;

            /** Reads a varint32 from the stream (protobuf serialization format) */
            uint32_t readVarInt();
    };

    /// @}
}

#endif // _DOTA_DEM_STREAM_FILE_HPP_