#include <iostream>
#include <exception>

#include <alice/alice.hpp>

using namespace dota;

/** This handler prints all global chat messages to the console. */
class handler_chat {
    private:
        /** Pointer to the parser */
        parser* p;
        /** Pointer to the handler */
        handler_t* h;
    public:
        /** Constructor, takes the handler */
        handler_chat(parser* p) : p(p), h(p->getHandler()) {
            handlerRegisterCallback(h, msgUser, UM_SayText2, handler_chat, handleChat)
        }

        /** Prints chat message to console */
        void handleChat(handlerCbType(msgUser) msg) {
            CUserMsg_SayText2 *m = msg->get<CUserMsg_SayText2>();

            // print chat
            std::cout << m->prefix() << ": " << m->text() << std::endl;
        }
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: alice-chat <file>" << std::endl;
        return 1;
    }

    try {
        // settings for the parser
        settings s{
            false,                   // forward_dem - We don't handle them so we don't need to listen to them
            true,                    // forward_net  - Yes for user messages
            false,                   // forward_net_internal - Not required
            true,                    // forward_user - Yes please.
            false,                   // parse_stringtables - No required, information is available soely in user messages
            std::set<std::string>{}, // blocked stringtables - Names say all
            false,                   // parse_entities - Nope
            false,                   // track_entities - Nope
            false,                   // forward entities - Nope
            true,                    // skip unused - Yes cause we don't request them via the parser
            std::set<uint32_t>{},    // blocked ones - All except the forwarded with skip_unused=true
            false                    // we dont handle events
        };

        // create a parser and open the replay
        parser p(s, new dem_stream_file);
        p.open(argv[1]);

        // create handler and attach parser
        handler_chat h(&p);

        // parse all messages
        p.handle();
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
