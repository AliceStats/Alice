/**
 * @file defs.hpp
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
 * @par Thanks
 *    Some of the values were taken from Tarrasque. Thanks!
 */

#ifndef _DOTA_DEFS_HPP_
#define _DOTA_DEFS_HPP_

// Values taken from https://raw2.github.com/skadistats/Tarrasque/master/tarrasque/consts.py
// Thanks.

namespace dota {
    /// @defgroup ADDON Addon
    /// @{

    /** The team a player is on, including spectators. */
    enum team {
        TEAM_RADIANT   = 2,
        TEAM_DIRE      = 3,
        TEAM_SPECTATOR = 5
    };

    /** Describes the life state of a player / npc entity. */
    enum life_state {
        STATE_ALIVE = 0,
        STATE_DYING,
        STATE_DEAD,
        STATE_RESPAWNABLE,
        STATE_DISCARDBODY
    };

    /** State / progress of the current game. */
    enum game_state {
        GAME_LOADING = 1,   // loading screen
        GAME_DRAFT,         // pick screen
        GAME_STRATEGY,      // not used yet
        GAME_PREGAME,       // before the 00:00 mark
        GAME_GAME,          // after the 00:00 mark
        GAME_POST,          // one ancient died
        GAME_DISCONNECT     // not used yet, problably forces disconnects in clients
    };

    /** Gamemode being played. */
    enum game_mode {
        MODE_NONE = 0,      // invalid
        MODE_AP,
        MODE_CM,
        MODE_RD,
        MODE_SD,
        MODE_AR,
        MODE_INTRO,         // intro movie?
        MODE_DIRETIED,
        MODE_RCM,           // reverse captains mode
        MODE_GREEVILING,
        MODE_TUTORIAL,
        MODE_MIDONLY,
        MODE_LP,            // least played
        MODE_NEWPLAYER,     // 20 hero pool
        MODE_COMPENDIUM     // compendium matchmaking
    };

    /** The type of a combat log entry. */
    enum combatlog_enty_type {
        CLE_DAMAGE = 0, // damage dealt
        CLE_HEAL,       // healed
        CLE_MODADD,     // modifier added
        CLE_MODDEL,     // modifier removed
        CLE_DEATH       // death
    };

    /// @}
}

#endif // _DOTA_DEFS_HPP_