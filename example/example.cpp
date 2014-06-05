#include <iostream>
#include <exception>

#include <alice/alice.hpp>

using namespace dota;

// typedef for the parser we use
typedef parser parser_t;

/** This handler prints the coordinates of a hero dieing to the console. */
class handler_example {
    private:
        /** Hero-to-Player list, maps the player ID to a corresponding hero */
        std::unordered_map<uint32_t, std::string> h2p;
        /** Pointer to the parser */
        parser_t* p;
        /** Pointer to the handler */
        handler_t* h;
    public:
        /** Constructor, takes the handler */
        handler_example(parser_t* p) : p(p), h(p->getHandler()) {
            // Subscribe to REPLAY_FLATTABLES so we can register our entity stuff
            handlerRegisterCallback(h, msgStatus, REPLAY_FLATTABLES, handler_example, handleReady)
        }

        /** Callback when stringtables are available */
        void handleReady(handlerCbType(msgStatus) msg) {
            // Normal Callback, method is only invoked if ressource is equal
            handlerRegisterCallback(h, msgEntity, p->getEntityIdFor("CDOTA_PlayerResource"), handler_example, handlePlayer)

            // Prefix Callback, any entity matching it triggers a callback
            std::vector<uint32_t> units = p->findEntityIdFor("CDOTA_Unit_Hero_");
            for (auto &i : units) {
                handlerRegisterCallback(h, msgEntity, i, handler_example, handleHero)
            }
        }

        /** Callback for a player ressource */
        void handlePlayer(handlerCbType(msgEntity) msg) {
            for (int i = 0; i < 10; ++i) {
                // get player hero
                auto name = msg->msg->find(std::string(".m_iszPlayerNames.000")+std::to_string(i));
                auto hero = msg->msg->find(std::string(".m_hSelectedHero.000")+std::to_string(i));

                //std::cout << 1 << std::endl;
                h2p[(hero->as<UIntProperty>() & 0x7FF)] = name->as<StringProperty>();
            }
        }

        /** Callback for hero */
        void handleHero(handlerCbType(msgEntity) msg) {
            // track a players life so that we only output the coordinates once
            static std::unordered_map<uint32_t, uint32_t> lifeTracker;

            int heroId = msg->msg->getId();
            if (h2p.find(heroId) == h2p.end())
                return; // illusion

            //std::cout << 2 << std::endl;
            int life = msg->msg->find(".m_iHealth")->as<UIntProperty>();
            if (life > 0 || lifeTracker[heroId] == 0) {
                lifeTracker[heroId] = life; // do not double track kills
                return;
            }

            lifeTracker[heroId] = life;
            //std::cout << 3 << std::endl;
            int cell_x = msg->msg->find(".m_cellX")->as<UIntProperty>();
            int cell_y = msg->msg->find(".m_cellY")->as<UIntProperty>();
            int cell_z = msg->msg->find(".m_cellZ")->as<UIntProperty>();
            auto vec = msg->msg->find(".m_vecOrigin")->as<VectorXYProperty>();

            std::cout << heroId << ", " << msg->msg->getClassName() << ", " << h2p[heroId] << ", "
                << life << ", [" << cell_x << "|" << cell_y << "|" << cell_z << "], [" << vec[0]
                << "|" << vec[1] << "]" << std::endl;
        }
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: example <file>" << std::endl;
        return 1;
    }

    try {
        // settings for the parser
        settings s{
            false, // forward_dem - We don't handle them so we don't need to listen to them
            false, // forward_net - Same as above
            false, // forward_net_internal - Same as above. In addition, these are never ever required. Ever. Almost.
            false, // forward_user - We don't use them so we can skip them. Contains stuff like the combat log and chat.
            true,  // parse_stringtables - We need baseline instance
            std::set<std::string>{
                 "ActiveModifiers", "CooldownNames", "ModifierNames", "CombatLogNames" 
            }, // blocked stringtables - Names say all
            true,  // parse_entities - Yes we need them
            false, // track_entities - Nope
            true,  // forward entities - Yes we listen to em
            true,  // skip unused - Yes cause we don't request them via the parser
            std::set<uint32_t>{}, // blocked ones - All except the forwarded with skip_unused=true
            false  // we dont handle events
        };

        // create a parser and open the replay
        parser_t p(s, new dem_stream_file);
        p.open(argv[1]);

        // create handler and attach parser
        handler_example h(&p);

        // parse all messages
        p.handle();
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
