/**
 * @file keyvalue.cpp
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
 * @par Info
 *    This file is not activly used in Alice. It is provided as a convinience
 *    for people building applications with it.
 */

#include <set>
#include <fstream>

#include <alice/keyvalue.hpp>

namespace dota {
    keyvalue::keyvalue(std::string s, bool isPath) : src(""), path(""), col(0), row(0), kv("", "") {
        if (isPath) {
            path = std::move(s);

            // read the file
            std::ifstream in(path.c_str(), std::ios::in);
            if (in) {
                in.seekg(0, std::ios::end);
                src.resize(in.tellg());
                in.seekg(0, std::ios::beg);
                in.read(&src[0], src.size());
                in.close();
            } else {
                BOOST_THROW_EXCEPTION(kvFileError()
                    << EArg<1>::info(s)
                );
            }
        } else {
            src = std::move(s);
        }
    }

    keyvalue::value_type keyvalue::parse() {
        std::string key   = "";             // key
        std::string value = "";             // value
        bool waitNl = false;                // whether we are waiting for the next line to continue parsing
        state s = state::EXPECT_KEY_START;  // current parsing state
        value_type* t = &kv;                // tree where we will store our results

        // allowed characters when between keys / values / objects
        static std::set<char> allowed = {'\r', '\n', ' ', '\t'};

        for (std::string::size_type i = 0; i < src.size(); ++i) {
            // skip the rest of lines that contain a comment
            if (waitNl) {
                if (src[i] == '\n') {
                    waitNl = false;
                    ++row;
                    col = 0;
                }

                continue;
            }

            ++col; // increment column

            // do stuff depending on the parser state and current character
            switch ( src[i] ) {
                case '\n':
                    ++row; // increment row
                    col = 0;
                    break;
                case '"':
                    switch (s) {
                        case state::EXPECT_TYPE:
                            s = state::EXPECT_VALUE_VALUE; // next type is a string value
                            break;
                        case state::EXPECT_KEY_VALUE:
                            s = state::EXPECT_TYPE; // wait for the next type
                            break;
                        case state::EXPECT_KEY_START:
                            s = state::EXPECT_KEY_VALUE; // wait for characters
                            break;
                        case state::EXPECT_VALUE_VALUE:
                            s = state::EXPECT_KEY_START;
                            t->add(key, std::move(value_type::node_type(key, value)));
                            key = "";
                            value = "";
                            break;
                        default:
                            BOOST_THROW_EXCEPTION((kvUnexpectedQuote()
                                << EArg<1>::info(path)
                                << EArgT<2, uint32_t>::info(row)
                                << EArgT<3, uint32_t>::info(col)
                            ));
                            break;
                    } break;
                case '{':
                    switch (s) {
                        case state::EXPECT_TYPE:
                            s = state::EXPECT_KEY_START;
                            t->add(key, value_type::node_type(key, ""));
                            t = &t->child(key);
                            key = "";
                            value = "";
                            break;
                        case state::EXPECT_VALUE_VALUE:
                        case state::EXPECT_KEY_START:
                        case state::EXPECT_KEY_VALUE:
                            break;
                        default:
                            BOOST_THROW_EXCEPTION((kvStartOfObject()
                                << EArg<1>::info(path)
                                << EArgT<2, uint32_t>::info(row)
                                << EArgT<3, uint32_t>::info(col)
                            ));
                            break;
                    } break;
                case '}':
                    switch (s) {
                        case state::EXPECT_KEY_START:
                            t = t->parent();
                            break;
                        case state::EXPECT_VALUE_VALUE:
                        case state::EXPECT_KEY_VALUE:
                            break;
                        default:
                            BOOST_THROW_EXCEPTION((kvEndOfObject()
                                << EArg<1>::info(path)
                                << EArgT<2, uint32_t>::info(row)
                                << EArgT<3, uint32_t>::info(col)
                            ));
                            break;
                    } break;

                case '/':
                    // check if we are between steps
                    if (s != state::EXPECT_TYPE && s != state::EXPECT_KEY_START)
                        continue;

                    if ((i+1) < src.size() && src[i+1] == '/')
                        waitNl = true; // found a comment

                    break;
                default: {
                    if (s == state::EXPECT_KEY_VALUE) {
                        key += src[i];
                        continue;
                    }

                    if (s == state::EXPECT_VALUE_VALUE) {
                        value += src[i];
                        continue;
                    }

                    if (!allowed.count(src[i]))
                        BOOST_THROW_EXCEPTION((kvUnexpectedCharacter()
                            << EArg<1>::info(path)
                            << EArgT<2, char>::info(src[i])
                            << EArgT<3, uint32_t>::info(row)
                            << EArgT<4, uint32_t>::info(col)
                        ));
                } break;
            }
        }

        // delete stuff
        src.clear();
        return kv;
    }
}