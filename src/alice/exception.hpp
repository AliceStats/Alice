/**
 * @file exception.hpp
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

#ifndef _DOTA_EXCEPTION_HPP_
#define	_DOTA_EXCEPTION_HPP_

#include <string>
#include <exception>

#include <boost/exception/all.hpp>

/**
 * This file provides convinience templates / macros for boost.exception.
 *
 * Example:
 * CREATE_EXCEPTION (myException, "Something went wrong")
 *
 * BOOST_THROW_EXCEPTION(myException()
 *     << EArg<1>::info("String")
 *     << EArgT<2, uint32_t>::info(12345)
 * );
 */

/// Creates a exception from the class and message provided
/// with an arbitrary set of parameters.
#define CREATE_EXCEPTION(__name, __msg)                             \
struct __name : virtual boost::exception, virtual std::exception {  \
    virtual const char* what() const throw() {                      \
        return __msg;                                               \
    }                                                               \
};

namespace dota {
    /// @defgroup CORE Core
    /// @{

    /** Baseclass doing nothing but providing an ordered type identifier for exception data */
    template <unsigned N>
    struct exception_info_c {};

    /** Used to transfere a string value within an exception */
    template <unsigned N>
    struct EArg {
        typedef boost::error_info<exception_info_c<N>, std::string> info;
    };

    /** Used to transfere a type T and it's value within an exception */
    template <unsigned N, typename T>
    struct EArgT {
        typedef typename boost::error_info<exception_info_c<N>, T> info;
    };

    /// @}
}


#endif	/* _DOTA_EXCEPTION_HPP_ */