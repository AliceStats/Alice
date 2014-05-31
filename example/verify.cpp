#include <iostream>
#include <exception>

#include <boost/algorithm/string/predicate.hpp>
#include <dirent.h>
#include <alice/config.hpp>
#include <alice/alice.hpp>

using namespace dota;

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: verify <replay folder>" << std::endl;
        return 1;
    }

    try {
        // Settings, we activate everything to verify that all information can be parsed
        settings s {
            true, true, true, true, true, {}, true, true, true, false, {}, true
        };

        // Read all replays into the list
        DIR *dir;
        struct dirent *entry;
        std::vector<std::string> entries;

        if ((dir = opendir(argv[1])) == NULL) {
            std::cerr << "Failed to open directory." << std::endl;
            return 1;
        }

        while ((entry = readdir (dir)) != NULL) {
            std::string e(entry->d_name);

            // don't add the current and prev dir
            if (e.substr(0,1).compare(".") == 0)
                continue;

            if (boost::algorithm::ends_with(e, ".dem"))
                entries.push_back(std::move(e));

            #if DOTA_BZIP2
            if (boost::algorithm::ends_with(e, ".bz2"))
                entries.push_back(std::move(e));
            #endif // DOTA_BZIP2
        }

        closedir(dir);

        // Parse replays
        for (auto &rep : entries) {
            try {
                std::string replay = std::string(argv[1])+"/"+rep;

                parser *p;

                #if DOTA_BZIP2
                if (boost::algorithm::ends_with(replay, ".bz2")) {
                    p = new parser(s, new dem_stream_bzip2);
                } else
                #endif // DOTA_BZIP2
                {
                    p = new parser(s, new dem_stream_file);
                }

                p->open(replay);
                p->handle();

                delete p;
                std::cout << rep << ": OK" << std::endl;
            } catch (boost::exception &e) {
                std::cout << boost::diagnostic_information(e) << std::endl;
            } catch (std::exception &e) {
                std::cout << e.what() << std::endl;
            }
        }

        std::cout << "Done" << std::endl;
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}