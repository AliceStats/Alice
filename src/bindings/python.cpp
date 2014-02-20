// Boost python
#include <boost/python.hpp>
using namespace boost::python;

// Alice
#include <alice/alice.hpp>
using namespace dota;

// protobuf for the repeated field def
#include <google/protobuf/repeated_field.h>


  // repeated .CGameInfo.CDotaGameInfo.CPlayerInfo player_info = 4;
/*
      inline const ::CGameInfo_CDotaGameInfo_CPlayerInfo& player_info(int index) const;
      inline ::CGameInfo_CDotaGameInfo_CPlayerInfo* mutable_player_info(int index);
      inline ::CGameInfo_CDotaGameInfo_CPlayerInfo* add_player_info();
      inline const ::google::protobuf::RepeatedPtrField< ::CGameInfo_CDotaGameInfo_CPlayerInfo >& player_info() const; mutable_player_info();
    */

#define CLASS_EXCEPTION(name) class_<name, boost::noncopyable>("name").def("what", &name::what);

BOOST_PYTHON_MODULE(alice)
{
    // protobufs
    //#include <bindings/python/ai_activity.python.hpp>
    //#include <bindings/python/demo.python.hpp>
    //#include <bindings/python/dota_commonmessages.python.hpp>
    //#include <bindings/python/dota_modifiers.python.hpp>
    //#include <bindings/python/dota_usermessages.python.hpp>
    //#include <bindings/python/netmessages.python.hpp>
    //#include <bindings/python/networkbasetypes.python.hpp>
    //#include <bindings/python/usermessages.python.hpp>

    // alice.bitstream
    CLASS_EXCEPTION(bitstreamOverflow)
    CLASS_EXCEPTION(bitstreamDataSize)

    class_<bitstream>("bitstream", init<const std::string&>())
        .def("swap", &bitstream::swap)
        .def("good", &bitstream::good)
        .def("end", &bitstream::end)
        .def("position", &bitstream::position)
        .def("read", &bitstream::read)
        .def("readBits", &bitstream::readBits)
        .def("seekForward", &bitstream::seekForward)
        .def("seekBackward", &bitstream::seekBackward)
        .def("nReadUInt", &bitstream::nReadUInt)
        .def("nReadSInt", &bitstream::nReadSInt)
        .def("nReadNormal", &bitstream::nReadNormal)
        .def("nSkipNormal", &bitstream::nSkipNormal)
        .def("nReadVarUInt32", &bitstream::nReadVarUInt32)
        .def("nReadVarSInt32", &bitstream::nReadVarSInt32)
        .def("nSkipVarInt", &bitstream::nSkipVarInt)
        .def("nReadCoord", &bitstream::nReadCoord)
        .def("nSkipCoord", &bitstream::nSkipCoord)
        .def("nReadCoordMp", &bitstream::nReadCoordMp)
        .def("nSkipCoordMp", &bitstream::nSkipCoordMp)
        .def("nReadCellCoord", &bitstream::nReadCellCoord)
        .def("nSkipCellCoord", &bitstream::nSkipCellCoord)
        .def("nReadString", &bitstream::nReadString)
        .def("nSkipString", &bitstream::nSkipString)
    ;

    // alice.entity_description
    class_<entity_description>("entity_description")
        .def_readonly("id", &entity_description::id)
        .def_readonly("id", &entity_description::name)
        .def_readonly("id", &entity_description::networkName)
    ;

    // alice.entity_list
    class_<entity_list>("entity_list")
        .def("__iter__", iterator<entity_list>())
        .def("size", &entity_list::size)
        .def("set", &entity_list::set)
        .def("get", &entity_list::get, return_value_policy<reference_existing_object>()) // might segfault
}