/**
 * @file gen-entities.cpp
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
 *    limitations under the License.
 *
 * This program generates a static representation of the entities in a single
 * replay. It makes it possible to conditionally skip generating a flattable by
 * hashing the sendtable.
 */

#include <iostream>
#include <boost/crc.hpp>
#include <alice/alice.hpp>

#include "tinyformat.hpp"

using namespace dota;

/** Generates header files from the send-/recv tables. */
class handler_entities {
    private:
        /** Pointer to the parser */
        parser* p;
        /** Pointer to the handler */
        handler_t* h;
        /** Whether the handler got all the relevant information */
        bool finished;
        /** Sendtables checksum for version identification */
        uint32_t ver_crc;
    public:
        /** Constructor, takes the handler */
        handler_entities(parser* p) : p(p), h(p->getHandler()), finished(false), ver_crc(0) {
            // Subscribe to the sendtables
            handlerRegisterCallback(h, msgDem, DEM_SendTables, handler_entities, handleSendTable)

            // Subscribe to REPLAY_FLATTABLES so we can register our entity stuff
            handlerRegisterCallback(h, msgStatus, REPLAY_FLATTABLES, handler_entities, handleReady)
        }

        /** handle send table to compute crc32 of it */
        void handleSendTable(handlerCbType(msgNet) msg) {
            CSVCMsg_SendTable* m = msg->get<CSVCMsg_SendTable>();

            // Serialize it back into a string to compute the crc
            std::string m2;
            m->SerializeToString(&m2);

            // save our crc
            boost::crc_32_type crc32;
            crc32.process_bytes(m2.data(), m2.length());
            ver_crc = crc32.checksum();
        }

        /** Callback when stringtables are available */
        void handleReady(handlerCbType(msgStatus) msg) {
            // Macros:
            // - NET_ClassBegin(classname)
            // - NET_Property()
            // - NET_ClassEnd()

            // header
            std::stringstream out("");
            out << "#ifndef _ALICE_ENT_" << ver_crc << "_HPP_" << std::endl;
            out << "#define _ALICE_ENT_" << ver_crc << "_HPP_" << std::endl;

            // iterate and dump all entities
            auto ft = p->getFlattables();
            for (auto &tbl : ft) {
                out << std::endl;
                out << "NET_ClassBegin(" << tbl.name << ")" << std::endl;
                for (auto &p : tbl.properties) {
                    out << "\tNET_Property(" << p.name << ")" << std::endl;
                }
                out << "NET_ClassEnd()" << std::endl;
            }

            out << "#endif" << std::endl;

            // just force the program to exit
            finished = true;

            std::cout << out.str() << std::endl;
        }

        /** Returns true once we are done parsing */
        bool isFinished() {
            return finished;
        }
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: alice-gen-entities <demo file>" << std::endl;
        return 1;
    }

    try {
        // settings for the parser
        // see the examples for a description of the indivdual fields
        settings s{ true, false, false, false, true, { }, true, false, false, true, {}, false };

        // create a parser and open the replay
        parser p(s, new dem_stream_file);
        p.open(argv[1]);

        // create handler and attach parser
        handler_entities h(&p);

        // parse all messages
        while (!h.isFinished() && p.good()) {
            p.read();
        }
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
