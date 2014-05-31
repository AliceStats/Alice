/**
 * @file alice.hpp
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

#ifndef _DOTA_ALICE_HPP_
#define _DOTA_ALICE_HPP_

// Core
#include <alice/config.hpp>
#include <alice/bitstream.hpp>
#include <alice/delegate.hpp>
#include <alice/dem.hpp>
#include <alice/dem_stream_bzip2.hpp>
#include <alice/dem_stream_file.hpp>
#include <alice/dem_stream_memory.hpp>
#include <alice/entity.hpp>
#include <alice/exception.hpp>
#include <alice/handler.hpp>
#include <alice/multiindex.hpp>
#include <alice/parser.hpp>
#include <alice/property.hpp>
#include <alice/sendprop.hpp>
#include <alice/sendtable.hpp>
#include <alice/settings.hpp>
#include <alice/stringtable.hpp>

// Protobuf objects
#include <alice/ai_activity.pb.h>
#include <alice/demo.pb.h>
#include <alice/dota_commonmessages.pb.h>
#include <alice/dota_modifiers.pb.h>
#include <alice/usermessages.pb.h>
#include <alice/netmessages.pb.h>
#include <alice/networkbasetypes.pb.h>
#include <alice/usermessages.pb.h>

// Include addons ?
#if DOTA_EXTRA
#include <alice/defs.hpp>
#include <alice/keyvalue.hpp>
#include <alice/monitor.hpp>
#include <alice/timer.hpp>
#include <alice/tree.hpp>
#endif

#endif // _DOTA_ALICE_HPP_