/**
 * @file tree.hpp
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

#ifndef _DOTA_TREE_HPP_
#define _DOTA_TREE_HPP_

#include <memory>
#include <map>
#include <sstream>
#include <utility>
#include <type_traits>

#include <boost/utility.hpp>

#include <alice/exception.hpp>

namespace dota {
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when trying to access an unkown child by it's key
    CREATE_EXCEPTION( treeUnkownKey, "Trying to access child via invalid key." )
    
    /// @}
    /// @defgroup ADDON Addon
    /// @{

    /**
     * Provides a map like nested tree structure, can be converted to json.
     *
     * Nodes and values can be added and modified but not removed at this time.
     * There is also no way to load a tree from it's json representation. This
     * structure was specifically written to be used with keyvalue.
     */
    template <typename Key, typename Value>
    class tree {
        public:
            /** Type for this nodes key */
            typedef Key key_type;
            /** Type for this nodes value */
            typedef typename std::remove_pointer<Value>::type value_type;
            /** Own type to instanciate child nodes */
            typedef tree<Key, Value> node_type;
            /** Type for a list of this node's children, unordered_map wants to re-hash so we have to settle on a standard map */
            typedef std::map<key_type, node_type> childlist_type;
            /** Size type */
            typedef typename childlist_type::size_type size_type;
            /** Iterator type */
            typedef typename childlist_type::iterator iterator;

            /** Constructor */
            tree() : k{}, v{}, children{}, p{nullptr} {}

            /** Constuctor takes key and value */
            tree(key_type k, value_type v) : k{k}, v{v}, children{}, p{nullptr} { }
            
            /** Constuctor takes value */
            tree(value_type v) : k{}, v{v}, children{}, p{nullptr} { }
            
            /** Copy-Constructor */
            tree(const tree& t) : k{t.k}, v{t.v}, children{t.children}, p{t.p} { }
            
            /** Move-Constructor */
            tree (tree&& t) 
                : k{std::move(t.k)}, v{std::move(t.v)}, children{std::move(t.children)}, p(t.p) 
            {
                t.children.clear();
                t.p = nullptr;
            }
            
            /** Destructor */
            ~tree() {
                // set the parent of all children to null
                for (auto &it : children) {
                    it.second.setParent(nullptr);
                }
            }
            
            /** Assignment operator */
            tree& operator= (tree t) {
                swap(t);
                return *this;
            }
            
            /** Swap this tree with a provided one */
            void swap(tree& t) {
                std::swap(k, t.k);
                std::swap(v, t.v);
                std::swap(children, t.children);
                std::swap(p, t.p);
            }
            
            /** Sets key of this node */
            void setKey(key_type k) {
                k = std::move(k);
            }

            /** Return this nodes key */
            const key_type& key() {
                return k;
            }
            
            /** Sets value */
            void set(Value v) {
                this->v = std::move(v);
            }

            /** Returns reference to object */
            template<class T = Value>
            typename std::enable_if<std::is_pointer<T>::value, value_type&>::type
            value() {
                return *v;
            }
            
            /** Returns reference to object */
            template<class T = Value>
            typename std::enable_if<!std::is_pointer<T>::value, value_type&>::type
            value() {
                return v;
            }

            /** Returns number of children */
            size_type size() {
                return children.size();
            }
            
            /** Returns iterator pointing to the beginning of the child list */
            iterator begin() {
                return children.begin();
            }

            /** Returns iterator pointing to the end of the child list */
            iterator end() {
                return children.end();
            }

            /** Looks up a child by key */
            iterator find(key_type k) {
                return children.find(k);
            }

            /** Returns a child by key */
            node_type& child(const key_type& k) {
                auto it = children.find(k);
                if (it == children.end()) {
                    BOOST_THROW_EXCEPTION( treeUnkownKey() << EArg<1>::info(k) );
                }

                return it->second;
            }

            /**
             * Returns pointer to parent
             *
             * If this function returns a nullptr it means you have reached the root node.
             */
            node_type* parent() {
                return p;
            }

            /** Adds a new child */
            void add(key_type k, node_type&& n) {
                n.setParent(this);
                children[k] = std::forward<node_type&&>(n);
            }

            /** Converts this tree to a json string */
            std::string toJson(uint32_t depth = 0) {
                // lambda just returns a string with N tabs
                auto nestTabs = [](uint32_t tabs) {
                    std::string t = "";
                    while (tabs-- > 0) {
                        t += '\t';
                    }
                    return t;
                };

                std::string tabs = nestTabs(depth);

                // this is pretty ugly but on the other hand it's also pretty simple
                // why implement a json object when you don't really need it
                std::stringstream ret("");

                if (children.empty()) {
                    ret << "\"" << v << "\"";
                } else {
                    ret << "{" << std::endl;

                    for (auto it = begin(); it != end(); ++it) {
                        ret << '\t' << tabs << "\"" << it->first << "\":" << it->second.toJson(depth+1);

                        if (boost::next(it) != children.end())
                            ret << ",";
                        ret << std::endl;
                    }

                    ret << tabs << "}";
                }

                return ret.str();
            }
        private:
            /** Key of this node, always filled  */
            key_type k;
            /** Value of this node, maybe empty */
            Value v;
            /** List of children, may also be empty */
            childlist_type children;
            /** Pointer to parrent, may be NULL for the root element */
            node_type* p;

            /** Sets parent of current node, only used from within add */
            void setParent(node_type* pr) {
                p = pr;
            }
    };

    /// @}
}

#endif // _DOTA_TREE_HPP_