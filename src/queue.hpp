/**
 * @file queue.hpp
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
 *    for people building applications with it. It's also required by the monitor
 *    pattern, providing a thread-safe building block for the worker queue
 *
 */

#ifndef _DOTA_QUEUE_HPP_
#define _DOTA_QUEUE_HPP_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dota {
    /// @defgroup ADDON Addon
    /// @{

    /** Thread safe blocking queue */
    template <typename T>
    class queue {
        public:
            /** Pop entry from the queue, blocks until one is available */
            T pop() {
                std::unique_lock<std::mutex> mlock(mutex);
                while (q.empty()) {
                    cond.wait(mlock);
                }

                auto item = q.front();
                q.pop();
                return item;
            }

            /** Push entry into the queue */
            void push(T&& item) {
                std::unique_lock<std::mutex> mlock(mutex);
                q.push(std::move(item));
                mlock.unlock();
                cond.notify_one();
            }

            /** Returns the current number of entries */
            typename std::queue<T>::size_type size() {
                return q.size();
            }
        private:
            /** Underlying non-thread safe queue */
            std::queue<T> q;
            /** Mutex to provide thread safety */
            std::mutex mutex;
            /** Condition variable for blocking pop */
            std::condition_variable cond;
    };

    /// @}
}
#endif /* _DOTA_QUEUE_HPP_ */