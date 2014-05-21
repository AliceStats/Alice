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
 */

#include <iostream>
#include <alice/alice.hpp>

#include "tinyformat.hpp"

using namespace dota;

/** This handler prints the coordinates of a hero dieing to the console. */
class handler_entities {
    private:
        /** Pointer to the parser */
        parser* p;
        /** Pointer to the handler */
        handler_t* h;
        /** Whether the handler got all the relevant information */
        bool finished;
    public:
        /** Constructor, takes the handler */
        handler_entities(parser* p) : p(p), h(p->getHandler()), finished(false) {
            // Subscribe to REPLAY_FLATTABLES so we can register our entity stuff
            handlerRegisterCallback(h, msgStatus, REPLAY_FLATTABLES, handler_entities, handleReady)
        }

        /** Callback when stringtables are available */
        void handleReady(handlerCbType(msgStatus) msg) {
            // just force the program to exit
            finished = true;
        }

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
        settings s{ false, false, false, false, true, { }, true, false, false, true, {}, false };

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