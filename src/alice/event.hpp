/**
 * @file event.hpp
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

#ifndef _DOTA_EVENT_HPP_
#define	_DOTA_EVENT_HPP_

#include <vector>
#include <unordered_map>
#include <string>

namespace dota {   
    /// @defgroup EXCEPTIONS Exceptions
    /// @{

    /// Thrown when an unkown event descriptor is being accessed
    CREATE_EXCEPTION( eventUnkownDescriptor, "Trying to access unkown event descriptor." )

    /// @}

    /// @defgroup CORE Core
    /// @{
    
    /** Contains the names / types of event properties*/
    struct event_prop {
        /** Type of the property */
        int32_t type;
        /** Name of the property */
        std::string name;
    };    
    
    /** Contains the specs single event being send */
    struct event_descriptor {
        /** Event ID referenced by events send */
        uint32_t id;
        /** Name of the event */
        std::string name;
        /** Properties send */
        std::vector<event_prop> props;
        /** Property type value counterparts */
        enum prop_type {
            FLOAT = 2, // val_float
            SHORT = 4, // val_short            
            BYTE  = 5, // val_byte
            BOOL  = 6  // val_bool
        };
    };
    
    /** Contains the specs for an event emitted. */
    class event_list {
        public:
            /** Type for stored event information */
            typedef event_descriptor value_type;
            /** Type for a map of event id's and their counterparts */
            typedef std::unordered_map<uint32_t, value_type> map_type;
            /** Size for map_type */
            typedef map_type::size_type size_type;
            /** Iterator for map_type */
            typedef map_type::iterator iterator;
            
            /** Returns iterator pointing to the beginning of the event list */
            inline iterator begin() const {
                return events.begin();
            }

            /** Returns iterator pointing to the end of the event list */
            inline iterator end() const {
                return events.end();
            }

            /** Returns iterator pointing to the element request or one element behind the last if none can be found */
            inline iterator find(const uint32_t& needle) {
                return events.find(needle);
            }
            
            /** Returns current number of events available */
            inline size_type size() const {
                return events.size();
            }

            /** Removes all event descriptors */
            inline void clear() const {
                events.clear();
            }

            /** Inserts event information for given ID */
            inline void set(uint32_t index, value_type c) const {
                events.emplace(std::make_pair(index, std::move(c)));
            }

            /** Returns the event information stored for the given index */
            inline const value_type& get(uint32_t index) const {
                return events[index];
            }
        private:
            /** Contains a list of all possible events being emitted */
            mutable map_type events;
    };
    
    /// @}
}

#endif	/* _DOTA_EVENT_HPP_ */