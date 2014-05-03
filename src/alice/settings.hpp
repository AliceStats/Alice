/**
 * @file settings.hpp
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
 */

#ifndef _DOTA_SETTINGS_HPP_
#define _DOTA_SETTINGS_HPP_

#include <set>

namespace dota {
    /** Settings for the replay parser. Immutable for the duration of a full-parse once set. */
    struct settings {
        /** Whether to forward dem messages */
        const bool forward_dem;

        /** Whether to forward net messages */
        const bool forward_net;

        /** Whether to forward internal net messages */
        const bool forward_net_internal;

        /** Whether to forward user messages */
        const bool forward_user;

        /** Whether to parse stringtables */
        const bool parse_stringtables;

        /** Stringtables to skip */
        const std::set<std::string> skip_stringtables;

        /** Whether to parse entities */
        const bool parse_entities;

        /** Whether to send information about updated fields */
        const bool track_entities;

        /**
         * Whether to forward entities.
         *
         * Even if no entities are forwarded, access is still possible directly though
         * the entity list.
         */
        const bool forward_entities;

        /**
         * Skips parsing entities which have no subscriber.
         *
         * You will be unable to access entities that fall into this categories
         * even when using the entity list directly.
         */
        const bool skip_unsubscribed_entities;

        /** List of entities to always skip. Not affected by subscription status. */
        const std::set<uint32_t> skip_entities;
        
        /** Whether to parse / handle event information. */
        const bool parse_events;
    };
}

#endif // _DOTA_SETTINGS_HPP_