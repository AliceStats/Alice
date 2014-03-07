/**
 * @file test/tree.cpp
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
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */


#ifndef _DOTA_TEST_TREE_HPP_
#define _DOTA_TEST_TREE_HPP_

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Tree

#include <boost/test/unit_test.hpp>
#include <alice/tree.hpp>
#include <alice/exception.hpp>

using namespace dota;

BOOST_AUTO_TEST_CASE( Constructor )
{
    // standard constructor
    tree<std::string, std::string> t1;

    // standard constructor setting Key and Value
    tree<std::string, std::string> t2("Key", "Value");

    BOOST_REQUIRE( t1.parent() == nullptr && t2.parent() == nullptr );
    BOOST_REQUIRE( t2.value() == "Value" );
    BOOST_REQUIRE( t2.key() == "Key");

    // test move constructor
    t1 = std::move(t2);
    BOOST_REQUIRE( t1.value() == "Value" );
    BOOST_REQUIRE( t1.key() == "Key");
    BOOST_REQUIRE( t2.value() != "Value" );
    BOOST_REQUIRE( t2.key() != "Key");

    // test copy constructor
    t2 = t1;
    BOOST_REQUIRE( t2.value() == "Value" );
    BOOST_REQUIRE( t2.key() == "Key");
}

BOOST_AUTO_TEST_CASE( Children )
{
    tree<std::string, std::string> t1("Key", "Value");

    {
        // standard constructor setting Key and Value
        tree<std::string, std::string> t2;

        // append t1 as a child of t2
        t2.setKey("Parent");
        t2.add("t1", std::move(t2));

        BOOST_REQUIRE( t2.size() == 1);
        BOOST_REQUIRE( t2.find("t1") != t2.end() );
        BOOST_REQUIRE( t2.child("t1").parent() != nullptr );
    }

    // t2 is destructed, check that t1 has nullptr as it's parent
    BOOST_REQUIRE( t1.parent() == nullptr );
}

BOOST_AUTO_TEST_CASE( Json )
{
    std::string j1 = "\"Value\"";
    tree<std::string, std::string> t1("Value");
    BOOST_REQUIRE( t1.toJson() == j1 );

    // testing for other cases is a bit tedious to write
    // because we would also have to take different line
    // endings on different platforms into account.
}

BOOST_AUTO_TEST_CASE( Exception )
{
    tree<std::string, std::string> t1;
    try {
        t1.child("should-not-exist");
        BOOST_FAIL( "Exception not thrown" );
    } catch (boost::exception &e) {
        // ignore, we want this to end up here
    }
}

BOOST_AUTO_TEST_CASE( Pointer )
{
    struct t { uint32_t v; };
    t* test = new t{99};

    tree<std::string, t*> tree;
    tree.set(test);

    BOOST_REQUIRE( tree.value().v == 99 );
    delete test;
}

#endif // _DOTA_TEST_TREE_HPP_