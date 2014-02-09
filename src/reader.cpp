/**
 * @file reader.cpp
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
#include <snappy.h>

#include "demo.pb.h"
#include "netmessages.pb.h"
#include "usermessages.pb.h"
#include "dota_usermessages.pb.h"

#include "reader.hpp"

namespace dota {
    int32_t reader::readVarInt(std::ifstream &stream, const std::string& file) {
        char buffer;
        uint32_t count  = 0;
        uint32_t result = 0;

        do {
            if (count == 5) {
                BOOST_THROW_EXCEPTION(demCorrupted()
                    << EArg<1>::info(file)
                );
            } else if (!stream.good()) {
                BOOST_THROW_EXCEPTION(demUnexpectedEOF()
                    << EArg<1>::info(file)
                );
            } else {
                buffer = stream.get();
                result |= (uint32_t)(buffer & 0x7F) << ( 7 * count );
                ++count;
            }
        } while (buffer & 0x80);

        return result;
    }

    int32_t reader::readVarInt(stringWrapper &stream, const std::string& file) {
        char buffer;
        uint32_t count  = 0;
        uint32_t result = 0;

        do {
            if (count == 5) {
                BOOST_THROW_EXCEPTION(demCorrupted()
                    << EArg<1>::info(file)
                );
            } else if (stream.size == 0) {
                BOOST_THROW_EXCEPTION(demUnexpectedEOF()
                    << EArg<1>::info(file)
                );
            } else {
                buffer = stream.str[0];
                result |= (uint32_t)(buffer & 0x7F) << ( 7 * count );
                --stream.size;
                stream.str += 1;
                ++count;
            }
        } while (buffer & 0x80);

        return result;
    }

    reader::reader(const std::string& file)
        : file(file), fstream(file.c_str(), std::ifstream::in | std::ifstream::binary),
          state(0), cTick(0), h(new handler_t), db(h)
    {
        // check if file can be opened / read
        if (!fstream.is_open())
            BOOST_THROW_EXCEPTION(demFileNotAccessible()
                << EArg<1>::info(file)
            );

        // check filesize
        const std::streampos fstart = fstream.tellg();
        fstream.seekg (0, std::ios::end);
        std::streampos fsize = fstream.tellg() - fstart;
        fstream.seekg(fstart);

        if (fsize < sizeof(demheader_t))
            BOOST_THROW_EXCEPTION((demFileTooSmall()
                << EArg<1>::info(file)
                << EArgT<2, std::streampos>::info(fsize)
                << EArgT<3, std::size_t>::info(sizeof(demheader_t))
            ));

        // read header
        demheader_t head;
        fstream.read((char*) &head, sizeof(demheader_t));
        if(strcmp(head.headerid, DOTA_DEMHEADERID))
            BOOST_THROW_EXCEPTION(demHeaderMismatch()
                << EArg<1>::info(file)
                << EArg<2>::info(std::string(head.headerid, 8))
                << EArg<3>::info(std::string(DOTA_DEMHEADERID))
            );

        // initialize buffer
        buffer = new char[DOTA_BUFSIZE];
        bufferCmp = new char[DOTA_BUFSIZE];

        // register all possible types to handle
        registerTypes();

        // add handlers for events we want to receive
        handlerRegisterCallback(h, msgDem, DEM_Packet,       reader, handlePacket)
        handlerRegisterCallback(h, msgDem, DEM_SignonPacket, reader, handlePacket)
        handlerRegisterCallback(h, msgDem, DEM_SendTables,   reader, handleSendTablesDem)
        handlerRegisterCallback(h, msgNet, svc_UserMessage,  reader, handleUserMessage)
    }

    void reader::readMessage(bool skip) {
        if (!fstream.good())
            return;

        // get type, tick and size
        uint32_t type = readVarInt(fstream, file);
        const bool compressed = !!( type & DEM_IsCompressed );
        type = (type & ~DEM_IsCompressed);

        uint32_t tick = readVarInt(fstream, file);
        uint32_t size = readVarInt(fstream, file);
        cTick = tick;

        // mark for finish if type 0 is reached
        if (state == 1) state = 2;
        if (type == 0)  state = 1;

        // skip messages if skipUntil is set or no handler is available
        if (skip || (!h->hasCallback<msgDem>(type))) {
            fstream.seekg(size, std::ios::cur); // seek forward
            return;
        }

        // read into char array and convert to string
        if (size > DOTA_BUFSIZE)
            BOOST_THROW_EXCEPTION((demMessageToBig()
                << EArgT<1, std::size_t>::info(size)
            ));

        fstream.read(buffer, size);
        stringWrapper s;

        // uncompress
        if (compressed && snappy::IsValidCompressedBuffer(buffer, size)) {
            std::size_t uSize;
            if (!snappy::GetUncompressedLength(buffer, size, &uSize)) {
                BOOST_THROW_EXCEPTION((demInvalidCompression()
                    << EArg<1>::info(file)
                    << EArgT<2, std::streampos>::info(fstream.gcount())
                    << EArgT<3, std::size_t>::info(size)
                    << EArgT<4, uint32_t>::info(type)
                ));
            }

            if (uSize > DOTA_BUFSIZE)
                BOOST_THROW_EXCEPTION((demMessageToBig()
                    << EArgT<1, std::size_t>::info(uSize)
                ));

            if (!snappy::RawUncompress(buffer, size, bufferCmp)) {
                BOOST_THROW_EXCEPTION((demInvalidCompression()
                    << EArg<1>::info(file)
                    << EArgT<2, std::streampos>::info(fstream.gcount())
                    << EArgT<3, std::size_t>::info(size)
                    << EArgT<4, uint32_t>::info(type)
                ));
            }

            s.str = bufferCmp;
            s.size = uSize;
        } else {
            s.str = buffer;
            s.size = size;
        }

        // relay message
        h->forward<msgDem>(type, std::move(s), tick);
    }

    void reader::registerTypes() {
        #define regDem( _type ) handlerRegisterObject(h, msgDem, DEM_ ## _type, CDemo ## _type)
        #define regNet( _type ) handlerRegisterObject(h, msgNet, net_ ## _type, CNETMsg_ ## _type)
        #define regSvc( _type ) handlerRegisterObject(h, msgNet, svc_ ## _type, CSVCMsg_ ## _type)
        #define regUsr( _type ) handlerRegisterObject(h, msgUser, UM_ ## _type, CUserMsg_ ## _type)
        #define regUsrDota( _type ) handlerRegisterObject(h, msgUser, DOTA_UM_ ## _type, CDOTAUserMsg_ ## _type)

        regDem( FileHeader )                                         // 1
        regDem( FileInfo )                                           // 2
        regDem( SyncTick )                                           // 3
        regDem( SendTables )                                         // 4
        regDem( ClassInfo )                                          // 5
        regDem( StringTables )                                       // 6
        regDem( Packet )                                             // 7
        handlerRegisterObject(h, msgDem, DEM_SignonPacket, CDemoPacket) // 8
        regDem( ConsoleCmd )                                         // 9
        regDem( CustomData )                                         // 10
        regDem( CustomDataCallbacks )                                // 11
        regDem( UserCmd )                                            // 12
        regDem( FullPacket )                                         // 13

        regNet( NOP )               // 0
        regNet( Disconnect )        // 1
        regNet( File )              // 2
        regNet( SplitScreenUser )   // 3
        regNet( Tick )              // 4
        regNet( StringCmd )         // 5
        regNet( SetConVar )         // 6
        regNet( SignonState )       // 7
        regSvc( ServerInfo )        // 8
        regSvc( SendTable )         // 9
        regSvc( ClassInfo )         // 10
        regSvc( SetPause )          // 11
        regSvc( CreateStringTable ) // 12
        regSvc( UpdateStringTable ) // 13
        regSvc( VoiceInit )         // 14
        regSvc( VoiceData )         // 15
        regSvc( Print )             // 16
        regSvc( Sounds )            // 17
        regSvc( SetView )           // 18
        regSvc( FixAngle )          // 19
        regSvc( CrosshairAngle )    // 20
        regSvc( BSPDecal )          // 21
        regSvc( SplitScreen )       // 22
        regSvc( UserMessage )       // 23
        regSvc( GameEvent )         // 25
        regSvc( PacketEntities )    // 26
        regSvc( TempEntities )      // 27
        regSvc( Prefetch )          // 28
        regSvc( Menu )              // 29
        regSvc( GameEventList )     // 30
        regSvc( GetCvarValue )      // 31
        regSvc( PacketReliable )    // 32

        regUsr( AchievementEvent )  // 1
        regUsr( CloseCaption )      // 2
        regUsr( CurrentTimescale )  // 4
        regUsr( DesiredTimescale )  // 5
        regUsr( Fade )              // 6
        regUsr( GameTitle )         // 7
        regUsr( Geiger )            // 8
        regUsr( HintText )          // 9
        regUsr( HudMsg )            // 10
        regUsr( HudText )           // 11
        regUsr( KeyHintText )       // 12
        regUsr( MessageText )       // 13
        regUsr( RequestState )      // 14
        regUsr( ResetHUD )          // 15
        regUsr( Rumble )            // 16
        regUsr( SayText )           // 17
        regUsr( SayText2 )          // 18
        regUsr( SayTextChannel )    // 19
        regUsr( Shake )             // 20
        regUsr( ShakeDir )          // 21
        regUsr( StatsCrawlMsg )     // 22
        regUsr( StatsSkipState )    // 23
        regUsr( TextMsg )           // 24
        regUsr( Tilt )              // 25
        regUsr( Train )             // 26
        regUsr( VGUIMenu )          // 27
        regUsr( VoiceMask )         // 28
        regUsr( VoiceSubtitle )     // 29
        regUsr( SendAudio )         // 30

        //regUsrDota( AddUnitToSelection )      // 64
        regUsrDota( AIDebugLine )               // 65
        regUsrDota( ChatEvent )                 // 66
        regUsrDota( CombatHeroPositions )       // 67
        regUsrDota( CombatLogData )             // 68
        regUsrDota( CombatLogShowDeath )        // 70
        regUsrDota( CreateLinearProjectile )    // 71
        regUsrDota( DestroyLinearProjectile )   // 72
        regUsrDota( DodgeTrackingProjectiles )  // 73
        regUsrDota( GlobalLightColor )          // 74
        regUsrDota( GlobalLightDirection )      // 75
        regUsrDota( InvalidCommand )            // 76
        regUsrDota( LocationPing )              // 77
        regUsrDota( MapLine )                   // 78
        regUsrDota( MiniKillCamInfo )           // 79
        regUsrDota( MinimapDebugPoint )         // 80
        regUsrDota( MinimapEvent )              // 81
        regUsrDota( NevermoreRequiem )          // 82
        regUsrDota( OverheadEvent )             // 83
        regUsrDota( SetNextAutobuyItem )        // 84
        regUsrDota( SharedCooldown )            // 85
        regUsrDota( SpectatorPlayerClick )      // 86
        regUsrDota( TutorialTipInfo )           // 87
        regUsrDota( UnitEvent )                 // 88
        regUsrDota( ParticleManager )           // 89
        regUsrDota( BotChat )                   // 90
        regUsrDota( HudError )                  // 91
        regUsrDota( ItemPurchased )             // 92
        regUsrDota( Ping )                      // 93
        regUsrDota( ItemFound )                 // 94
        //regUsrDota( CharacterSpeakConcept )   // 95
        regUsrDota( SwapVerify )                // 96
        regUsrDota( WorldLine )                 // 97
        regUsrDota( TournamentDrop )            // 98
        regUsrDota( ItemAlert )                 // 99
        regUsrDota( HalloweenDrops )            // 100
        regUsrDota( ChatWheel )                 // 101
        regUsrDota( ReceivedXmasGift )          // 102
        regUsrDota( UpdateSharedContent )       // 103
        regUsrDota( TutorialRequestExp )        // 104
        regUsrDota( TutorialPingMinimap )       // 105
        handlerRegisterObject(h, msgUser, DOTA_UM_GamerulesStateChanged, CDOTA_UM_GamerulesStateChanged) // 106
        regUsrDota( ShowSurvey )                // 107
        regUsrDota( TutorialFade )              // 108
        regUsrDota( AddQuestLogEntry )          // 109
        regUsrDota( SendStatPopup )             // 110
        regUsrDota( TutorialFinish )            // 111
        regUsrDota( SendRoshanPopup )           // 112
        regUsrDota( SendGenericToolTip )        // 113
        regUsrDota( SendFinalGold )             // 114

        #undef regDem
        #undef regNet
        #undef regSvc
        #undef regUsr
        #undef regUsrDota
    }
}