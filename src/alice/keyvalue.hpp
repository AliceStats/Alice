/**
 * @file keyvalue.hpp
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
 * @par Info
 *    This file is not activly used in Alice. It is provided as a convinience
 *    for people building applications with it.
 */

#ifndef _DOTA_KEYVALUE_HPP_
#define _DOTA_KEYVALUE_HPP_

#include <string>
#include <alice/exception.hpp>
#include <alice/tree.hpp>

/// Maximum size of a packed kv key
#define PKV_KEY_SIZE 1024

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when the parser can't open the file specified
    CREATE_EXCEPTION( kvFileError,            "Unable to load specified file" )

    /// Thrown when the parser finds an unexpected character
    CREATE_EXCEPTION( kvUnexpectedCharacter,  "Unexpected character" )

    /// Thrown when the parser finds an unexpected quote
    CREATE_EXCEPTION( kvUnexpectedQuote,      "Unexpected quote" )

    /// Thrown when the parser reaches the end of an object but expected a value
    CREATE_EXCEPTION( kvEndOfObject,          "Unexpected end of object" )

    /// Thrown when the parser expects a key but find an object
    CREATE_EXCEPTION( kvStartOfObject,        "Unexpected start of object" )

    /// Thrown when the parser expects a key but find an object
    CREATE_EXCEPTION( kvBinaryError,          "Start of binary kv does not point to node" )

    /// @}
    /// @defgroup ADDON Addon
    /// @{

    /** Parser for Valves KeyValue format, see https://developer.valvesoftware.com/wiki/KeyValues */
    class keyvalue {
        public:
            /** Type definition for the parsing result */
            typedef tree<std::string, std::string> value_type;

            /**
             * Constructor, takes data to be parsed as string or path
             *
             * @param s String or path depending on isPath
             * @param isPath Treats first argument as path if true
             * @param isBinary Treats data as binary
             */
            keyvalue(std::string s, bool isPath = false, bool isBinary = false);

            /**
             * Parses contents and returns them as value_type.
             *
             * value_type is moved from the internal kv object.
             * Multiple calls to this function will lead to the parser generating the object
             * again. Save the retun value for subsequent access.
             */
            value_type parse();
        private:
            /** Contains source of the file in question */
            std::string src;
            /** Contains path to file if isPath is true */
            std::string path;
            /** Current column, updated during parsing */
            uint32_t col;
            /** Current row, updated during parsing */
            uint32_t row;
            /** Contains parsed structure */
            value_type kv;
            /** Whether the structure has been packed */
            bool packed;

            /** Parse a normal KV */
            value_type parse_text();
            /** Parse a binary KV */
            value_type parse_binary(value_type *n = nullptr);

            /** The different possible parser states */
            enum class state {
                // for the sequence " something "
                EXPECT_KEY_START   = 1,
                EXPECT_KEY_VALUE   = 2,

                // switches for the next type, e.g. object or key
                EXPECT_TYPE        = 3,

                // same as EXPECT_KEY but for value content
                EXPECT_VALUE_VALUE = 4
            };

            /** Different types of values packed */
        	enum pkv {
                PKV_NODE = 0,
                PKV_STRING,
                PKV_INT,
                PKV_FLOAT,
                PKV_PTR,
                PKV_WSTRING,
                PKV_COLOR,
                PKV_UINT64,
                PKV_MAX = 11      // Marks the end of the structure
            };
    };

    /// @}
}

#endif // _DOTA_KEYVALUE_HPP_