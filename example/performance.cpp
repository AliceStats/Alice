#include <iostream>
#include <exception>

#include <dirent.h>

#include <alice/alice.hpp>

using namespace dota;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: performance <replay folder>" << std::endl;
        return 1;
    }

    try {
        // This is the least performant configuration.
        // It should be used when you are developing an application that needs all possible information available.
        settings s_full {
            true, true, true, true, true, std::set<std::string>{}, true, true, true, false, std::set<uint32_t>{}, true
        };

        // This configuration skips stuff which you most likely don't care about.
        settings s_minimal {
            false, true, false, true, true,
            std::set<std::string>{"ParticleEffectNames", "EconItems", "lightstyles", "ResponseKeys", "downloadables", "InfoPanel",
             "userinfo", "server_query_info", "Scenes", "dynamicmodel", "VguiScreen", "ExtraParticleFilesTable",
             "EffectDispatch", "GameRulesCreation", "Materials"
            }, true, false, true, true, std::set<uint32_t>{}, false
        };

        // Verify file integrity
        settings s_verify {
            false, false, false, false, true, std::set<std::string>{}, true, false, false, true, std::set<uint32_t>{}, false
        };

        // Read all replays into our vector
        DIR *dir;
        struct dirent *entry;
        std::vector<std::string> entries;

        if ((dir = opendir(argv[1])) == NULL) {
            std::cout << "Failed to read directory." << std::endl;
            return 1;
        }

        while ((entry = readdir (dir)) != NULL) {
            std::string e(entry->d_name);

            // don't add the current and prev dir
            if (e.substr(0,1).compare(".") == 0)
                continue;

            if (e.substr(e.size()-3) == "dem")
                entries.push_back(std::move(e));
        }

        closedir(dir);

        std::cout << "Found the following replays: " << std::endl;
        for (auto &e : entries) {
            std::cout << " - " << e << std::endl;
        }

        // this contains the result set
        struct presult {
            presult() : full(0), minimal(0), verify(0) {}
            uint32_t full;
            uint32_t minimal;
            uint32_t verify;
            uint32_t ticks;
            uint32_t msgs;
            std::string name;
            std::string time_;
        };

        std::vector<presult> results;

        // Parse replays and measure the time
        for (auto &rep : entries) {
            std::string replay = std::string(argv[1])+"/"+rep;
            std::cout << "Parsing " << replay << std::endl;
            presult pr;
            pr.name = replay;

            uint64_t c = getZTime();
            for (int i = 0; i < 10; ++i) {
                parser p(s_full, new dem_stream_file);
                p.open(replay);
                p.handle();
                pr.ticks = p.getTick();
                pr.msgs  = p.getMsgCount();
            }
            uint64_t c2 = getZTime();

            pr.full += ((c2 - c) / 10000);

            c = getZTime();
            for (int i = 0; i < 10; ++i) {
                parser p(s_minimal, new dem_stream_file);
                p.open(replay);
                p.handle();
            }
            c2 = getZTime();

            pr.minimal += ((c2 - c) / 10000);

            c = getZTime();
            for (int i = 0; i < 10; ++i) {
                parser p(s_verify, new dem_stream_file);
                p.open(replay);
                p.handle();
            }
            c2 = getZTime();

            pr.verify += ((c2 - c) / 10000);

            results.push_back(pr);
        }

        std::cout << "[";
        for (auto &i : results) {
            std::cout << "{";
                std::cout << "\"name\":\"" << i.name << "\",";
                std::cout << "\"full\":" << i.full << ",";
                std::cout << "\"minimal\":" << i.minimal << ",";
                std::cout << "\"verify\":" << i.verify << ",";
                std::cout << "\"ticks\":" << i.ticks << ",";
                std::cout << "\"msgs\":" << i.msgs << ",";
                std::cout << "\"time\":\"" << i.time_ << "\"";
            std::cout << "},";
        }
        std::cout << "]" << std::endl;
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
