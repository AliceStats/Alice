#include <iostream>
#include <exception>
#include <thread>
#include <queue>
#include <mutex>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <alice/config.hpp>
#include <alice/alice.hpp>

using namespace dota;

/** Simple thread pool, handle's a fixed-size list of tasks */
class threadpool {
    public:
        /** Add a new task to the pool */
        void addTask(std::function<void()>&& t) {
            tasks.push(std::move(t));
        }

        /** Handle all tasks */
        void work(uint32_t nThreads) {
            // add work to each thread
            for (uint32_t i = 0; i < nThreads; ++i) {
                workers.push_back(std::thread([&](){
                    while (true) {
                        std::unique_lock<std::mutex> lock(task_lock);
                        if (tasks.empty())
                            break;

                        std::function<void()> job = tasks.front();
                        tasks.pop();
                        lock.unlock();
                        job();
                    }

                }));
            }

            // join all threads
            for (uint32_t i = 0; i < nThreads; ++i) {
                workers[i].join();
            }
        }
    private:
        /** List of workers */
        std::vector<std::thread> workers;
        /** Task queue */
        std::queue<std::function<void()>> tasks;
        /** Synchronisation for queue */
        std::mutex task_lock;
};

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: verify <replay folder> <threads>" << std::endl;
        return 1;
    }

    try {
        // Settings, we activate everything to verify that all information can be parsed
        settings s {
            true, true, true, true, true, std::set<std::string>{}, true, true, true, false, std::set<uint32_t>{}, true
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
        threadpool pool;
        uint32_t nThreads = boost::lexical_cast<uint32_t>(argv[2]);
        for (auto &rep : entries) {
            std::string replay = std::string(argv[1])+"/"+rep;
            pool.addTask([=](){
                try {
                    // check file size for 0 sized downloads
                    struct stat fstat;
                    stat( replay.c_str(), &fstat );
                    
                    if (fstat.st_size < 200) {
                        std::cout << rep << ": Unavailable" << std::endl;
                        return;
                    }
                    
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
            });
        }

        pool.work(nThreads);

        std::cout << "Done" << std::endl;
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
