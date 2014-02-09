/**
 * @file handler_detail.hpp
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
 */

namespace dota {
    /// @defgroup CORE Core
    /// @{

    namespace detail {
        /** This helper destroys the data it holds if its type is a pointer */
        template <
            typename Object,
            bool = std::is_pointer<Object>::value
        >
        struct destroyCallbackObjectHelper { };

        template <typename Object>
        struct destroyCallbackObjectHelper<Object, true> {
            static void destroy( Object &&o ) {
                delete o;
            }
        };

        template <typename Object>
        struct destroyCallbackObjectHelper<Object, false> {
            static void destroy( Object &&o ) {
                // do nothing for non-pointers
            }
        };

        /** Helper template to get the n-th argument of a variadic template. */
        template <unsigned int Index, typename... T>
        struct GetNthArgument;

        template <typename Head, typename... Tail>
        struct GetNthArgument<0, Head, Tail...> {
            typedef Head type;
        };

        template <unsigned int Index, typename Head, typename... Tail>
        struct GetNthArgument<Index, Head, Tail...> {
            typedef typename GetNthArgument<Index-1, Tail...>::type type;
        };
    }

    /// @}
}