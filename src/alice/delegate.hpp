/**
 * @file delegate.hpp
 * @author Robin Dietrich <me (at) invokr (dot) org>
 * @version 1.1
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
 *
 */

#ifndef _DOTA_DELEGATE_HPP_
#define	_DOTA_DELEGATE_HPP_

#include <memory>
#include <cstddef>

namespace dota {
    /// @defgroup CORE Core
    /// @{

    /** A simple and fast delegate implementation, performing magnitudes better than std::function or Callback. */
    template <typename R, typename ... ARGS>
    class delegate {
        public:
            /** Constructor, zero's object and function pointer */
            delegate() : pObj(nullptr), pFunc(nullptr) { }

            /** Default copy constructor. */
            delegate(const delegate&) = default;

            /** Default destructor */
            ~delegate() = default;

            /** Compares two delegates with each other */
            bool operator== (delegate &d1) {
                return (d1.pObj == pObj && d1.pFunc == pFunc);
            }

            /** Redirects all arguments to the function called and returns it's result */
            inline R operator()(ARGS... args) const {
                return (*pFunc)(pObj, std::move(args)...);
            }

            /**
             * Creates a delegate from a member function.
             *
             * An example call might look like this:
             * ::fromMemberFunc<object_, &object_::function_>(obj)
             */
            template <typename T, R (T::*method)(ARGS...)>
            static delegate fromMemberFunc(T* pObj) {
                delegate ret;
                ret.pObj = pObj;
                ret.pFunc = &callFwd<T, method>;
                return ret;
            }
        private:
            /** Function signature of the delegate */
            typedef R (*func)(void* obj, ARGS...);
            /** Pointer to object. */
            void* pObj;
            /** Pointer to member function. */
            func pFunc;

            /** Internally used to forward the function call. */
            template <typename T, R (T::*method)(ARGS...)>
            static R callFwd(void* pObj, ARGS... args) {
                T* p = static_cast<T*>(pObj);
                return (p->*method)(std::forward<ARGS>(args)...);
            }
    };

    /// @}
}

#endif	/* _DOTA_DELEGATE_HPP_ */
