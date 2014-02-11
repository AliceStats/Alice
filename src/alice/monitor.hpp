/**
 * @file monitor.hpp
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
 *
 */

#ifndef _DOTA_MONITOR_HPP_
#define _DOTA_MONITOR_HPP_

#include <functional>
#include <future>
#include <thread>
#include <cstdlib>

#include <alice/queue.hpp>

namespace dota {
    /// @defgroup ADDON Addon
    /// @{

    /** Helper to set value for non-void promises */
    template<typename Fut, typename F, typename T>
    void monitor_set_value( std::promise<Fut>& p, F& f, T& t ) {
        p.set_value( f(t) );
    }

    /** Helper to set value for void promises */
    template<typename F, typename T>
    void monitor_set_value( std::promise<void>& p, F& f, T& t ) {
        f(t);
        p.set_value();
    }

    /** Monitor pattern, providing concurrent access to non-concurrent structures */
    template <typename T>
    class monitor {
        private:
            /** Distinct object we take care of */
            mutable T t;
            /** Queue working for entries */
            mutable queue<std::function<void()>> q;
            /** Worker thread */
            std::thread worker;
            /** Whether the worker is finished */
            bool finish;

        public:
            /** Constructor, takes ownership of T */
            monitor(T _t) : t(std::move(_t)), worker([=]{ while (!finish) { q.pop()(); } }), finish(false) {}

            /** Prevent copying */
            monitor(const monitor&) = delete;

            /** Default move constructor */
            monitor(monitor&&) = default;

            /** Destructor, joins thread, blocks until all operations at the time are finished */
            ~monitor() {
                q.push([=]{finish = true;});
                worker.join();
            }

            /** Add s function to the call queue */
            template <typename F>
            auto operator()(F f) const -> std::future<decltype(f(t))> {
                auto p = std::shared_ptr<std::promise<decltype(f(t))>>( new std::promise<decltype(f(t))> );
                auto ret = p->get_future();

                q.push([=]{
                    try {
                        monitor_set_value(*p, f, t);
                    } catch (...) {
                        p->set_exception(std::current_exception());
                    }
                });

                return ret;
            }
    };

    /// @}
}

#endif /* _DOTA_MONITOR_HPP_ */
