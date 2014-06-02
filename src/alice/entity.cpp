/**
 * @file entity.cpp
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

#include <alice/bitstream.hpp>
#include <alice/entity.hpp>

namespace dota {
    /** Read the id of a field from a bitstream */
    inline bool readFieldId(bitstream& bstream, uint32_t& fieldId) {
        const bool incremental = bstream.read(1);
        if (incremental) {
            ++fieldId;
        } else {
            const uint32_t value = bstream.nReadVarUInt32();

            if (value == 0x3FFF)
                return false;
            else
                fieldId += value + 1;
        }

        return true;
    }

    void entity::updateFromBitstream(bitstream& bstream, entity_delta* delta) {
        // use this static vector so we don't realocate memory all the time
        static std::mutex fieldLock;
        static std::vector<uint32_t> fields(1000, 0);

        std::unique_lock<std::mutex> lock(fieldLock);
        fields.clear();

        uint32_t fieldId = -1;
        while (readFieldId(bstream, fieldId)) {
            D_( std::cout << "[entity] Read field: " << fieldId << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
            fields.push_back(fieldId);
        }

        for (auto &it : fields) {
            if (it >= properties.size())
                BOOST_THROW_EXCEPTION(entityUnkownSendprop()
                    << (EArgT<1, std::size_t>::info(properties.size()))
                    << (EArgT<2, std::size_t>::info(id))
                );

            property &p = properties[it];
            if (p.isInitialized()) {
                D_( std::cout << "[entity] Updating property at " << it << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                p.update(bstream);
            } else {
                D_( std::cout << "[entity] Creating property at " << it << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
                properties[it] = property::create(bstream, flat->properties[it].prop);
                properties[it].setName(&flat->properties[it].name); // set hierarchial name of property
            }
        }

        // check if we should keep track of changes
        if (delta != nullptr) {
            delta->entity_fields.clear();
            delta->entity_fields.resize(fields.size());
            memcpy( &delta->entity_fields[0], &fields[0], fields.size()*sizeof(uint32_t) );
        }
    }

    void entity::skip(bitstream& bstream) {
        // use this static vector so we don't realocate memory all the time
        static std::vector<uint32_t> fields(1000, 0);
        fields.clear();

        uint32_t fieldId = -1;
        while (readFieldId(bstream, fieldId)) {
            D_( std::cout << "[entity] Read field: " << fieldId << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
            fields.push_back(fieldId);
        }

        for (auto &it : fields) {
            if (it >= properties.size())
                BOOST_THROW_EXCEPTION(entityUnkownSendprop()
                    << (EArgT<1, std::size_t>::info(properties.size()))
                    << (EArgT<2, std::size_t>::info(id))
                );

            D_( std::cout << "[entity] Skipping property at " << it << " " << D_FILE << " " << __LINE__ << std::endl;, 4 )
            property::skip(bstream, flat->properties[it].prop);
        }
    }

    std::string entity::DebugString() {
        std::stringstream str("");

        str << "Name: " << cls->networkName << " / Id: " << id << " / State: " << currentState << std::endl;

        for (auto &p : properties) {
            if (p.isInitialized()) {
                str << "Property: " << p.getName() << " / Type: ";

                switch (p.getType()) {
                    case sendprop::T_Int:
                        str << " Int / Value: ";
                        break;
                    case sendprop::T_Float:
                        str << " Float / Value: ";
                        break;
                    case sendprop::T_Vector: {
                        str << " Vector / Value: ";
                    } break;
                    case sendprop::T_VectorXY: {
                        str << " VectorXY / Value: ";
                    } break;
                    case sendprop::T_String:
                        str << " String / Value: ";
                        break;
                    case sendprop::T_Array:
                        str << " Array / Value: ";
                        break;
                    case sendprop::T_DataTable:
                        str << " DataTable / Value: ";
                        break;
                    default:
                        str << " Unkown / Value: " << std::endl;
                }

                str  << p.asString() << std::endl;
            }
        }

        return str.str();
    }

    void entity::readHeader(uint32_t &id, bitstream &bstream, state_type &type) {
        // The header looks like this: [XY00001111222233333333333333333333] where everything > 0 is optional.
        // The first 2 bits (X and Y) tell us how much (if any) to read other than the 6 initial bits:
        // Y set     -> read 4
        // X set     -> read 8
        // X + Y set -> read 28

        uint32_t nId = bstream.read(6);
        switch (nId & 0x30) {
            case 16:
                nId = (nId & 15) | ( bstream.read(4) << 4);
                break;
            case 32:
                nId = (nId & 15) | ( bstream.read(8) << 4);
                break;
            case 48:
                nId = (nId & 15) | ( bstream.read(28) << 4);
                break;
        }

        id += nId + 1;

        // In Source the entity state is referred to as PVS (Potential Visible State) which
        // can have a number of different configurations.
        //
        // We only care about what we should do with the entity so we only represent those three
        // states: Create, Update and Delete.
        //
        // Cases in which entities are marked to leave the PVS but are not explicitly deleted are not
        // handled as they only represent a state change which we do not need to take into account being
        // not a client. In Source this might trigger behavior like dropping the entity after a certain
        // time which is something we don't need to take care of.
        //
        // If you want to get a more accurate representation of these states you should have a look at
        // edith or skadi.

        if (!bstream.read(1)) {
            if (bstream.read(1)) {
                D_( std::cout << "[entity] Creating " << id << " " << D_FILE << " " << __LINE__ << std::endl;, 3 )
                type = state_created;
            } else {
                D_( std::cout << "[entity] Updating " << id << " " << D_FILE << " " << __LINE__ << std::endl;, 3 )
                type = state_updated;
            }
        } else if (bstream.read(1)) {
            D_( std::cout << "[entity] Deleting " << id << " " << D_FILE << " " << __LINE__ << std::endl;, 3 )
            type = state_deleted;
        } else {
            type = state_default;
        }
    }
}
