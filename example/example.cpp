#include <iostream>
#include <exception>

#include <alice/exception.hpp>
#include <alice/handler.hpp>
#include <alice/gamestate.hpp>
#include <alice/reader.hpp>

using namespace dota;

/** This handler prints the coordinates of a hero dieing to the console. */
class handler_example {
    private:
        /** Hero-to-Player list, maps the player ID to a corresponding hero */
        std::unordered_map<uint32_t, std::string> h2p;
        /** Pointer to the handler */
        handler_t *h;
        /** Pointer to gamestate */
        gamestate& g;
    public:
        /** Constructor, takes the handler */
        handler_example(handler_t *h, gamestate& g) : h(h), g(g) {
           handlerRegisterCallback(h, msgStatus, reader::REPLAY_FLATTABLES, handler_example, handleReady)
        }

        /** Callback when stringtables are available */
        void handleReady(handlerCbType(msgStatus) msg) {
            // Normal Callback, method is only invoked if ressource is equal
            handlerRegisterCallback(h, msgEntity, g.getEntityIdFor("CDOTA_PlayerResource"), handler_example, handlePlayer)

            // Prefix Callback, any entity matching it triggers a callback
            std::vector<uint32_t> units = g.findEntityIdFor("CDOTA_Unit_Hero_");
            for (auto &i : units) {
                handlerRegisterCallback(h, msgEntity, i, handler_example, handleHero)
            }
        }

        /** Callback for a player ressource */
        void handlePlayer(handlerCbType(msgEntity) msg) {
            for (int i = 0; i < 10; ++i) {
                // get player hero
                auto name = msg->msg->find(std::string("m_iszPlayerNames.000")+std::to_string(i));
                auto hero = msg->msg->find(std::string("m_hSelectedHero.000")+std::to_string(i));

                h2p[(hero->as<IntProperty>() & 0x7FF)] = name->as<StringProperty>();
            }
        }

        /** Callback for hero */
        void handleHero(handlerCbType(msgEntity) msg) {
            // track a players life so that we only output the coordinates once
            static std::unordered_map<uint32_t, uint32_t> lifeTracker;

            int heroId = msg->msg->getId();
            if (h2p.find(heroId) == h2p.end())
                return; // illusion

            int life = msg->msg->find("DT_DOTA_BaseNPC.m_iHealth")->as<IntProperty>();
            if (life > 0 || lifeTracker[heroId] == 0) {
                lifeTracker[heroId] = life; // do not double track kills
                return;
            }

            lifeTracker[heroId] = life;

            int cell_x = msg->msg->find("DT_DOTA_BaseNPC.m_cellX")->as<IntProperty>();
            int cell_y = msg->msg->find("DT_DOTA_BaseNPC.m_cellY")->as<IntProperty>();
            int cell_z = msg->msg->find("DT_DOTA_BaseNPC.m_cellZ")->as<IntProperty>();
            auto vec = msg->msg->find("DT_DOTA_BaseNPC.m_vecOrigin")->as<VectorXYProperty>();

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
        reader r(argv[1]);
        handler_example h(r.getHandler(), r.getState());
        r.readAll();
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
