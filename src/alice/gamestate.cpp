/**
 * @file gamestate.cpp
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

#include <utility>

#include <alice/gamestate.hpp>

namespace dota {
    void gamestate::handleServerInfo(handlerCbType(msgNet) msg) {
        CSVCMsg_ServerInfo* m = msg->get<CSVCMsg_ServerInfo>();
        clist.reserve(m->max_classes());
        setMaxClasses(m->max_classes());
    }

    void gamestate::handleClassInfo(handlerCbType(msgDem) msg) {
        CDemoClassInfo* m = msg->get<CDemoClassInfo>();

        for (int32_t i = 0; i < m->classes_size(); ++i) {
            const CDemoClassInfo::class_t &c = m->classes(i);
            clist.set(c.class_id(), entity_description{c.class_id(), c.table_name(), c.network_name()});
        }

        flattenSendtables();
        h->forward<msgStatus>(1, 1, msg->tick); // "1" corresponds to REPLAY_FLATTABLES
    }

    void gamestate::handleSendTable(handlerCbType(msgNet) msg) {
        int32_t stableId = ++sendtableId;
        CSVCMsg_SendTable* m = msg->get<CSVCMsg_SendTable>();

        sendtable tbl(m->net_table_name(), m->needs_decoder());
        for (int32_t i = 0; i < m->props_size(); ++i) {
            tbl.insert(new sendprop(m->props(i), m->net_table_name()));
        }

        sendtables.insert(sendtableMap::entry_type{m->net_table_name(), ++stableId, std::move(tbl)});
    }

    void gamestate::handleCreateStringtable(handlerCbType(msgNet) msg) {
        CSVCMsg_CreateStringTable* m = msg->get<CSVCMsg_CreateStringTable>();

        // global unique id for ordered indexes
        int32_t tableid = ++stringtableId;

        if (m->user_data_size_bits() & 2)
            return;

        // add table to table list
        stringtables.insert(stringtableMap::entry_type{m->name(), tableid, stringtable(m)});
    }

    void gamestate::handleUpdateStringtable(handlerCbType(msgNet) msg) {
        CSVCMsg_UpdateStringTable* m = msg->get<CSVCMsg_UpdateStringTable>();

        // only update tables which we handle
        auto it = stringtables.findIndex(m->table_id());
        if (it == stringtables.endIndex())
            return;

        it->value.update(m);
    }

    void gamestate::handleEntity(handlerCbType(msgNet) msg) {
        CSVCMsg_PacketEntities* e = msg->get<CSVCMsg_PacketEntities>(); // get entity
        bitstream stream(e->entity_data()); // bistream from the entity data

        uint32_t eId = -1;          // Entity ID
        entity::state_type eType;   // Entity update Type (create, update, delete)

        // baseline table containing default entity values
        stringtableMap::key_iterator it = stringtables.findKey(BASELINETABLE);
        if (it == stringtables.end())
            BOOST_THROW_EXCEPTION( gamestateBaselineNotFound() );

        const stringtable &baseline = it->value;

        for (int32_t i = 0; i < e->updated_entries(); ++i) {
            // read update type and id from header
            entity::readHeader(eId, stream, eType);

            switch(eType) {
                // entity is being created
                case entity::state_created: {
                    if (eId > DOTA_MAX_ENTITIES)
                        BOOST_THROW_EXCEPTION( entityIdToLarge()
                            << (EArgT<1, uint32_t>::info(eId))
                        );

                    uint32_t classId = stream.read(entityClassBits); // <- points to id in the entity list
                    uint32_t serial = stream.read(10);

                    const entity_list::value_type &eClass = clist.get(classId);
                    const flatsendtable &f = getFlattable(eClass.name);

                    entity* ent;

                    auto it = entities.find(eId);
                    if (it != entities.end()) {
                        // entity already exists, update it as overwritten
                        ent = it->second;
                        ent->update(eId, eClass, f);
                        ent->setState(entity::state_overwritten);
                    } else {
                        // create the entity
                        ent = new entity(eId, eClass, f);
                        entities[eId] = ent;
                    }

                    // read updates from baseline and current data
                    bitstream baselineStream(baseline.get(std::to_string(classId)));
                    ent->updateFromBitstream(baselineStream);
                    ent->updateFromBitstream(stream);

                    // forward to handler
                    h->forward<msgEntity>(ent->getClassId(), ent, 0);
                } break;
                // entity is being updated
                case entity::state_updated: {
                    if (eId > DOTA_MAX_ENTITIES)
                        BOOST_THROW_EXCEPTION( entityIdToLarge()
                            << (EArgT<1, uint32_t>::info(eId))
                        );

                    auto it = entities.find(eId);
                    if (it != entities.end()) {
                        entity* ent = it->second;
                        ent->updateFromBitstream(stream);
                        ent->setState(entity::state_updated);
                        h->forward<msgEntity>(ent->getClassId(), ent, 0);
                    } else {
                        BOOST_THROW_EXCEPTION( gamestateInvalidId()
                            << (EArgT<1, uint32_t>::info(eId))
                        );
                    }
                } break;
                // entity is being deleted
                case entity::state_deleted: {
                    if (eId > DOTA_MAX_ENTITIES)
                        BOOST_THROW_EXCEPTION( entityIdToLarge()
                            << (EArgT<1, uint32_t>::info(eId))
                        );

                    auto it = entities.find(eId);
                    if (it != entities.end()) {
                        entity* ent = it->second;
                        ent->setState(entity::state_deleted);
                        h->forward<msgEntity>(ent->getClassId(), ent, 0);
                        delete it->second;
                        entities.erase(it);
                    } else {
                        BOOST_THROW_EXCEPTION( gamestateInvalidId()
                            << (EArgT<1, uint32_t>::info(eId))
                        );
                    }
                } break;
                default:
                    // ignore
                    break;
            }
        }

        // all entities in list are marked as removed
        if (e->is_delta()) {
            while (stream.read(1)) {
                eId = stream.read(11);

                auto it = entities.find(eId);
                if (it != entities.end()) {
                    entity* ent = it->second;
                    ent->setState(entity::state_deleted);
                    h->forward<msgEntity>(ent->getClassId(), ent, 0);
                    delete it->second;
                    entities.erase(it);
                }
            }
        }
    }

    void gamestate::flattenSendtables() {
        // Tieing dependend properties
        for (auto it = sendtables.beginIndex(); it != sendtables.endIndex(); ++it) {
            sendprop* last = nullptr;
            for (auto &prop : it->value) {
                if (prop.value->getType() == sendprop::T_Array) {
                    if (last)
                        prop.value->setArrayType(last);
                    else
                        BOOST_THROW_EXCEPTION( gamestateInvalidArrayProp() );
                }
                last = prop.value;
            }
        }

        // Building flattables
        for (auto it = sendtables.beginIndex(); it != sendtables.endIndex(); ++it) {
            auto table = *it;

            std::set<std::string> excludes;  // names of excluded properties
            std::vector<sendprop*> props;    // list of property classes

            // Building excludes
            buildExcludeList(table.value, excludes);

            // Building hierarchy
            buildHierarchy(table.value, excludes, props);

            // Sorting tables
            std::set<uint32_t> priorities({64}); // list of all possible priorities
            for (auto it : props) {
                priorities.insert(it->getPriority());
            }

            std::size_t offset = 0;
            for (auto &prio : priorities) {
                std::size_t cursor = offset;

                while (cursor < props.size()) {
                    auto prop = props[cursor];

                    if ((prop->getPriority() == prio) || ((SPROP_CHANGES_OFTEN & prop->getFlags()) && (prio == 64))) {
                        std::swap(props[cursor], props[offset]);
                        ++offset;
                    }
                    ++cursor;
                }
            }

            // insert stuff into flat table
            flattables.emplace(table.key, flatsendtable{table.key, std::move(props)});
        }
    }

    void gamestate::buildExcludeList(const sendtable &tbl, std::set<std::string> &excludes) {
        for (auto &p : tbl) {
            sendprop* pr = p.value;

            if (SPROP_EXCLUDE & pr->getFlags()) {
                // adding an exclude
                excludes.insert(std::string(pr->getClassname() + pr->getName()));
            } else if (pr->getType() == sendprop::T_DataTable) { // sub table can also point to excluded data
                sendtableMap::key_iterator it = sendtables.findKey(pr->getClassname());
                if (it == sendtables.end())
                    BOOST_THROW_EXCEPTION( sendtableUnkownTable()
                        << EArg<1>::info(pr->getClassname())
                    );

                buildExcludeList(it->value, excludes);
            }
        }
    }

    void gamestate::buildHierarchy(const sendtable &tbl, std::set<std::string> &excludes, std::vector<sendprop*> &props) {
        // Building hierarchy for the table
        std::vector<sendprop*> p;
        gatherProperties(tbl, p, excludes, props);

        for (auto ExcludedProps : p) {
            props.push_back(ExcludedProps);
        }
    }

    void gamestate::gatherProperties(const sendtable &tbl, std::vector<sendprop*> &dt_prop, std::set<std::string> &excludes, std::vector<sendprop*> &props) {
        for (auto p : tbl) {
            sendprop* pr = p.value;

            // skip excluded properties
            if ((SPROP_EXCLUDE | SPROP_INSIDEARRAY) & pr->getFlags()) {
                continue;
            } else if (excludes.count(std::string(tbl.getName() + pr->getName()))) {
                continue;
            }

            // check for new data
            if (pr->getType() == sendprop::T_DataTable) {
                auto it = sendtables.findKey(pr->getClassname());
                if (it == sendtables.end())
                    BOOST_THROW_EXCEPTION( sendtableUnkownTable()
                        << EArg<1>::info(pr->getClassname())
                    );
                const sendtable &dtTbl = it->value;

                if (SPROP_COLLAPSIBLE & pr->getFlags()) {
                    gatherProperties(dtTbl, dt_prop, excludes, props);
                } else {
                    buildHierarchy(dtTbl, excludes, props);
                }
            } else {
                dt_prop.push_back(p.value);
            }
        }
    }
}