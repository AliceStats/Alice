#include <iostream>
#include <exception>

#include <dirent.h>
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

            if (e.substr(e.size()-3) == "dem")
                entries.push_back(std::move(e));
        }

        closedir(dir);

        std::cout << "Found the following replays: " << std::endl;
        for (auto &e : entries) {
            std::cout << " - " << e << std::endl;
        }

        // Parse replays
        for (auto &rep : entries) {
            std::string replay = std::string(argv[1])+"/"+rep;
            std::cout << "Parsing " << replay << ": ";


            uint64_t c = getZTime();
            parser p(s, new dem_stream_file);
            p.open(replay);
            p.handle();
            uint64_t c2 = getZTime();

            std::cout << ((c2 - c) / 1000) << " ms" << std::endl;
        }

        std::cout << "Done" << std::endl;
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}