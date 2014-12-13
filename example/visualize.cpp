#include <iostream>
#include <exception>

#include <alice/alice.hpp>
#include <alice/dota_usermessages.pb.h>

using namespace dota;

#define visualizeDefine( _type, _subtype ) \
void visualize ## _type ## _subtype( handlerCbType( _type ) ) { \
    std::cout << "{\"n\":\"" #_subtype << "\",\"t\":\"" << #_type << "\"},"; \
}

#define visualizePrint( _type, _subtype, _atype ) \
void visualize ## _type ## _subtype( handlerCbType( _type ) t ) { \
    std::string dbg = escapeJsonString(t->get<_atype>()->DebugString()); \
    std::cout << "{\"n\":\"" #_subtype << "\",\"t\":\"" << #_type << "\",\"c\":\"" << dbg <<"\"},"; \
}

#define visualizeRegister( _type, _subtype ) \
handlerRegisterCallback(h, _type, _subtype, handler_visualize, visualize ## _type ## _subtype);

std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (auto &iter : input) {
        switch (iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << iter; break;
        }
    }
    return ss.str();
}

/** This handler prints the json of the relationship / nesting between different message types. */
class handler_visualize {
    private:
        // Callbacks DEM
        visualizePrint(  msgDem, DEM_FileHeader, CDemoFileHeader );
        visualizePrint(  msgDem, DEM_FileInfo,   CDemoFileInfo );
        visualizeDefine( msgDem, DEM_SyncTick );
        visualizeDefine( msgDem, DEM_SendTables );
        visualizeDefine( msgDem, DEM_ClassInfo );
        visualizeDefine( msgDem, DEM_StringTables );
        visualizeDefine( msgDem, DEM_Packet );
        visualizeDefine( msgDem, DEM_SignonPacket );
        visualizePrint(  msgDem, DEM_ConsoleCmd, CDemoConsoleCmd );
        visualizeDefine( msgDem, DEM_CustomData );
        visualizeDefine( msgDem, DEM_CustomDataCallbacks );
        visualizeDefine( msgDem, DEM_UserCmd );
        visualizeDefine( msgDem, DEM_FullPacket );
        visualizeDefine( msgDem, DEM_SaveGame );

        // Callbacks NET
        visualizeDefine( msgNet, net_NOP )
        visualizeDefine( msgNet, net_Disconnect )
        visualizePrint(  msgNet, net_File, CNETMsg_File )
        visualizeDefine( msgNet, net_SplitScreenUser )
        visualizeDefine( msgNet, net_Tick )
        visualizePrint(  msgNet, net_StringCmd, CNETMsg_StringCmd )
        visualizePrint(  msgNet, net_SetConVar, CNETMsg_SetConVar )
        visualizeDefine( msgNet, net_SignonState )

        // Callbacks NET -> svc
        visualizePrint(  msgNet, svc_ServerInfo, CSVCMsg_ServerInfo )
        visualizePrint(  msgNet, svc_SendTable, CSVCMsg_SendTable )
        visualizePrint(  msgNet, svc_ClassInfo, CSVCMsg_ClassInfo )
        visualizeDefine( msgNet, svc_SetPause )
        visualizeDefine( msgNet, svc_CreateStringTable )
        visualizeDefine( msgNet, svc_UpdateStringTable )
        visualizePrint(  msgNet, svc_VoiceInit, CSVCMsg_VoiceInit )
        visualizeDefine( msgNet, svc_VoiceData )
        visualizeDefine( msgNet, svc_Print )
        visualizeDefine( msgNet, svc_Sounds )
        visualizeDefine( msgNet, svc_SetView )
        visualizeDefine( msgNet, svc_FixAngle )
        visualizeDefine( msgNet, svc_CrosshairAngle )
        visualizeDefine( msgNet, svc_BSPDecal )
        visualizeDefine( msgNet, svc_SplitScreen )
        visualizeDefine( msgNet, svc_UserMessage )
        visualizeDefine( msgNet, svc_GameEvent )
        visualizeDefine( msgNet, svc_PacketEntities )
        visualizeDefine( msgNet, svc_TempEntities )
        visualizeDefine( msgNet, svc_Prefetch )
        visualizeDefine( msgNet, svc_Menu )
        visualizeDefine( msgNet, svc_GameEventList )
        visualizePrint(  msgNet, svc_GetCvarValue, CSVCMsg_GetCvarValue )
        visualizeDefine( msgNet, svc_PacketReliable )

        // Callbacks User
        visualizeDefine( msgUser, UM_AchievementEvent )
        visualizeDefine( msgUser, UM_CloseCaption )
        visualizeDefine( msgUser, UM_CurrentTimescale )
        visualizeDefine( msgUser, UM_DesiredTimescale )
        visualizeDefine( msgUser, UM_Fade )
        visualizeDefine( msgUser, UM_GameTitle )
        visualizeDefine( msgUser, UM_Geiger )
        visualizeDefine( msgUser, UM_HintText )
        visualizeDefine( msgUser, UM_HudMsg )
        visualizeDefine( msgUser, UM_HudText )
        visualizeDefine( msgUser, UM_KeyHintText )
        visualizeDefine( msgUser, UM_MessageText )
        visualizeDefine( msgUser, UM_RequestState )
        visualizeDefine( msgUser, UM_ResetHUD )
        visualizeDefine( msgUser, UM_Rumble )
        visualizePrint(  msgUser, UM_SayText, CUserMsg_SayText )
        visualizePrint(  msgUser, UM_SayText2, CUserMsg_SayText2 )
        visualizePrint(  msgUser, UM_SayTextChannel, CUserMsg_SayTextChannel )
        visualizeDefine( msgUser, UM_Shake )
        visualizeDefine( msgUser, UM_ShakeDir )
        visualizeDefine( msgUser, UM_StatsCrawlMsg )
        visualizeDefine( msgUser, UM_StatsSkipState )
        visualizeDefine( msgUser, UM_TextMsg )
        visualizeDefine( msgUser, UM_Tilt )
        visualizeDefine( msgUser, UM_Train )
        visualizeDefine( msgUser, UM_VGUIMenu )
        visualizeDefine( msgUser, UM_VoiceMask )
        visualizeDefine( msgUser, UM_VoiceSubtitle )
        visualizeDefine( msgUser, UM_SendAudio )

        // Callabcks User -> dota
        visualizeDefine( msgUser, DOTA_UM_AIDebugLine )
        visualizePrint(  msgUser, DOTA_UM_ChatEvent, CDOTAUserMsg_ChatEvent )
        visualizeDefine( msgUser, DOTA_UM_CombatHeroPositions )
        visualizeDefine( msgUser, DOTA_UM_CombatLogData )
        visualizeDefine( msgUser, DOTA_UM_CombatLogShowDeath )
        visualizeDefine( msgUser, DOTA_UM_CreateLinearProjectile )
        visualizeDefine( msgUser, DOTA_UM_DestroyLinearProjectile )
        visualizeDefine( msgUser, DOTA_UM_DodgeTrackingProjectiles )
        visualizeDefine( msgUser, DOTA_UM_GlobalLightColor )
        visualizeDefine( msgUser, DOTA_UM_GlobalLightDirection )
        visualizeDefine( msgUser, DOTA_UM_InvalidCommand )
        visualizePrint( msgUser, DOTA_UM_LocationPing, CDOTAUserMsg_LocationPing )
        visualizeDefine( msgUser, DOTA_UM_MapLine )
        visualizeDefine( msgUser, DOTA_UM_MiniKillCamInfo )
        visualizeDefine( msgUser, DOTA_UM_MinimapDebugPoint )
        visualizeDefine( msgUser, DOTA_UM_MinimapEvent )
        visualizeDefine( msgUser, DOTA_UM_NevermoreRequiem )
        visualizeDefine( msgUser, DOTA_UM_OverheadEvent )
        visualizeDefine( msgUser, DOTA_UM_SetNextAutobuyItem )
        visualizeDefine( msgUser, DOTA_UM_SharedCooldown )
        visualizeDefine( msgUser, DOTA_UM_SpectatorPlayerClick )
        visualizeDefine( msgUser, DOTA_UM_TutorialTipInfo )
        visualizeDefine( msgUser, DOTA_UM_UnitEvent )
        visualizeDefine( msgUser, DOTA_UM_ParticleManager )
        visualizePrint( msgUser, DOTA_UM_BotChat, CDOTAUserMsg_BotChat )
        visualizeDefine( msgUser, DOTA_UM_HudError )
        visualizeDefine( msgUser, DOTA_UM_ItemPurchased )
        visualizeDefine( msgUser, DOTA_UM_Ping )
        visualizePrint( msgUser, DOTA_UM_ItemFound, CDOTAUserMsg_ItemFound )
        visualizeDefine( msgUser, DOTA_UM_SwapVerify )
        visualizeDefine( msgUser, DOTA_UM_WorldLine )
        visualizePrint( msgUser, DOTA_UM_ItemAlert, CDOTAUserMsg_ItemAlert )
        visualizeDefine( msgUser, DOTA_UM_HalloweenDrops )
        visualizeDefine( msgUser, DOTA_UM_ChatWheel )
        visualizeDefine( msgUser, DOTA_UM_ReceivedXmasGift )
        visualizeDefine( msgUser, DOTA_UM_UpdateSharedContent )
        visualizeDefine( msgUser, DOTA_UM_TutorialRequestExp )
        visualizeDefine( msgUser, DOTA_UM_TutorialPingMinimap )
        visualizePrint( msgUser, DOTA_UM_GamerulesStateChanged, CDOTA_UM_GamerulesStateChanged )
        visualizeDefine( msgUser, DOTA_UM_ShowSurvey )
        visualizeDefine( msgUser, DOTA_UM_TutorialFade )
        visualizeDefine( msgUser, DOTA_UM_AddQuestLogEntry )
        visualizeDefine( msgUser, DOTA_UM_SendStatPopup )
        visualizeDefine( msgUser, DOTA_UM_TutorialFinish )
        visualizeDefine( msgUser, DOTA_UM_SendRoshanPopup )
        visualizeDefine( msgUser, DOTA_UM_SendGenericToolTip )
        visualizePrint( msgUser, DOTA_UM_SendFinalGold, CDOTAUserMsg_SendFinalGold )
    public:
        handler_visualize(parser* p) : h(p->getHandler()) {
            // register DEM
            visualizeRegister( msgDem, DEM_FileHeader);
            visualizeRegister( msgDem, DEM_FileInfo);
            visualizeRegister( msgDem, DEM_SyncTick );
            visualizeRegister( msgDem, DEM_SendTables )
            visualizeRegister( msgDem, DEM_ClassInfo )
            visualizeRegister( msgDem, DEM_StringTables )
            visualizeRegister( msgDem, DEM_Packet )
            visualizeRegister( msgDem, DEM_SignonPacket )
            visualizeRegister( msgDem, DEM_ConsoleCmd )
            visualizeRegister( msgDem, DEM_CustomData )
            visualizeRegister( msgDem, DEM_CustomDataCallbacks )
            visualizeRegister( msgDem, DEM_UserCmd )
            visualizeRegister( msgDem, DEM_FullPacket )
            visualizeRegister( msgDem, DEM_SaveGame )

            // register NET
            visualizeRegister( msgNet, net_NOP )
            visualizeRegister( msgNet, net_Disconnect )
            visualizeRegister( msgNet, net_File )
            visualizeRegister( msgNet, net_SplitScreenUser )
            visualizeRegister( msgNet, net_Tick )
            visualizeRegister( msgNet, net_StringCmd )
            visualizeRegister( msgNet, net_SetConVar )
            visualizeRegister( msgNet, net_SignonState )

            // register NET -> svc
            visualizeRegister( msgNet, svc_ServerInfo )
            visualizeRegister( msgNet, svc_SendTable )
            visualizeRegister( msgNet, svc_ClassInfo )
            visualizeRegister( msgNet, svc_SetPause )
            visualizeRegister( msgNet, svc_CreateStringTable )
            visualizeRegister( msgNet, svc_UpdateStringTable )
            visualizeRegister( msgNet, svc_VoiceInit )
            visualizeRegister( msgNet, svc_VoiceData )
            visualizeRegister( msgNet, svc_Print )
            visualizeRegister( msgNet, svc_Sounds )
            visualizeRegister( msgNet, svc_SetView )
            visualizeRegister( msgNet, svc_FixAngle )
            visualizeRegister( msgNet, svc_CrosshairAngle )
            visualizeRegister( msgNet, svc_BSPDecal )
            visualizeRegister( msgNet, svc_SplitScreen )
            visualizeRegister( msgNet, svc_UserMessage )
            visualizeRegister( msgNet, svc_GameEvent )
            visualizeRegister( msgNet, svc_PacketEntities )
            visualizeRegister( msgNet, svc_TempEntities )
            visualizeRegister( msgNet, svc_Prefetch )
            visualizeRegister( msgNet, svc_Menu )
            visualizeRegister( msgNet, svc_GameEventList )
            visualizeRegister( msgNet, svc_GetCvarValue )
            visualizeRegister( msgNet, svc_PacketReliable )

            // register user
            visualizeRegister( msgUser, UM_AchievementEvent )
            visualizeRegister( msgUser, UM_CloseCaption )
            visualizeRegister( msgUser, UM_CurrentTimescale )
            visualizeRegister( msgUser, UM_DesiredTimescale )
            visualizeRegister( msgUser, UM_Fade )
            visualizeRegister( msgUser, UM_GameTitle )
            visualizeRegister( msgUser, UM_Geiger )
            visualizeRegister( msgUser, UM_HintText )
            visualizeRegister( msgUser, UM_HudMsg )
            visualizeRegister( msgUser, UM_HudText )
            visualizeRegister( msgUser, UM_KeyHintText )
            visualizeRegister( msgUser, UM_MessageText )
            visualizeRegister( msgUser, UM_RequestState )
            visualizeRegister( msgUser, UM_ResetHUD )
            visualizeRegister( msgUser, UM_Rumble )
            visualizeRegister( msgUser, UM_SayText )
            visualizeRegister( msgUser, UM_SayText2 )
            visualizeRegister( msgUser, UM_SayTextChannel )
            visualizeRegister( msgUser, UM_Shake )
            visualizeRegister( msgUser, UM_ShakeDir )
            visualizeRegister( msgUser, UM_StatsCrawlMsg )
            visualizeRegister( msgUser, UM_StatsSkipState )
            visualizeRegister( msgUser, UM_TextMsg )
            visualizeRegister( msgUser, UM_Tilt )
            visualizeRegister( msgUser, UM_Train )
            visualizeRegister( msgUser, UM_VGUIMenu )
            visualizeRegister( msgUser, UM_VoiceMask )
            visualizeRegister( msgUser, UM_VoiceSubtitle )
            visualizeRegister( msgUser, UM_SendAudio )

            // register user -> dota
            visualizeRegister( msgUser, DOTA_UM_AIDebugLine )
            visualizeRegister( msgUser, DOTA_UM_ChatEvent )
            visualizeRegister( msgUser, DOTA_UM_CombatHeroPositions )
            visualizeRegister( msgUser, DOTA_UM_CombatLogData )
            visualizeRegister( msgUser, DOTA_UM_CombatLogShowDeath )
            visualizeRegister( msgUser, DOTA_UM_CreateLinearProjectile )
            visualizeRegister( msgUser, DOTA_UM_DestroyLinearProjectile )
            visualizeRegister( msgUser, DOTA_UM_DodgeTrackingProjectiles )
            visualizeRegister( msgUser, DOTA_UM_GlobalLightColor )
            visualizeRegister( msgUser, DOTA_UM_GlobalLightDirection )
            visualizeRegister( msgUser, DOTA_UM_InvalidCommand )
            visualizeRegister( msgUser, DOTA_UM_LocationPing )
            visualizeRegister( msgUser, DOTA_UM_MapLine )
            visualizeRegister( msgUser, DOTA_UM_MiniKillCamInfo )
            visualizeRegister( msgUser, DOTA_UM_MinimapDebugPoint )
            visualizeRegister( msgUser, DOTA_UM_MinimapEvent )
            visualizeRegister( msgUser, DOTA_UM_NevermoreRequiem )
            visualizeRegister( msgUser, DOTA_UM_OverheadEvent )
            visualizeRegister( msgUser, DOTA_UM_SetNextAutobuyItem )
            visualizeRegister( msgUser, DOTA_UM_SharedCooldown )
            visualizeRegister( msgUser, DOTA_UM_SpectatorPlayerClick )
            visualizeRegister( msgUser, DOTA_UM_TutorialTipInfo )
            visualizeRegister( msgUser, DOTA_UM_UnitEvent )
            visualizeRegister( msgUser, DOTA_UM_ParticleManager )
            visualizeRegister( msgUser, DOTA_UM_BotChat )
            visualizeRegister( msgUser, DOTA_UM_HudError )
            visualizeRegister( msgUser, DOTA_UM_ItemPurchased )
            visualizeRegister( msgUser, DOTA_UM_Ping )
            visualizeRegister( msgUser, DOTA_UM_ItemFound )
            visualizeRegister( msgUser, DOTA_UM_SwapVerify )
            visualizeRegister( msgUser, DOTA_UM_WorldLine )
            visualizeRegister( msgUser, DOTA_UM_ItemAlert )
            visualizeRegister( msgUser, DOTA_UM_HalloweenDrops )
            visualizeRegister( msgUser, DOTA_UM_ChatWheel )
            visualizeRegister( msgUser, DOTA_UM_ReceivedXmasGift )
            visualizeRegister( msgUser, DOTA_UM_UpdateSharedContent )
            visualizeRegister( msgUser, DOTA_UM_TutorialRequestExp )
            visualizeRegister( msgUser, DOTA_UM_TutorialPingMinimap )
            visualizeRegister( msgUser, DOTA_UM_GamerulesStateChanged)
            visualizeRegister( msgUser, DOTA_UM_ShowSurvey )
            visualizeRegister( msgUser, DOTA_UM_TutorialFade )
            visualizeRegister( msgUser, DOTA_UM_AddQuestLogEntry )
            visualizeRegister( msgUser, DOTA_UM_SendStatPopup )
            visualizeRegister( msgUser, DOTA_UM_TutorialFinish )
            visualizeRegister( msgUser, DOTA_UM_SendRoshanPopup )
            visualizeRegister( msgUser, DOTA_UM_SendGenericToolTip )
            visualizeRegister( msgUser, DOTA_UM_SendFinalGold )
        }
    private:
        /** Pointer to callback handler */
        handler_t* h;
};

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Recommended usage: visualize (file) > output.json" << std::endl;
        return 1;
    }

    try {
        // settings for the parser
        settings s{
            true,  // forward_dem            - Yes, we want to forward all messages in order to show their relationship
            true,  // forward_net            - Same as above
            true,  // forward_net_internal   - Same as above.
            true,  // forward_user           - Yes
            true,  // parse_stringtables     - Yes
            std::set<std::string>{},
            true,  // parse_entities         - We don't need the content but the changes
            true,  // track_entities         - Yes, that's what we'll print out
            false, // forward entities       - Printing this would be way to much data
            false, // skip unused entities   - No, would be a bad visualization if all the entities were to be missing
            std::set<uint32_t>{},
            false  // parse_events           - Nope, not implemented yet
        };

        // create a parser and open the replay.

        // We'll use the memory stream because parsing / forwarding each and every
        // message takes some time and we don't want the hard drive to skip back to the replay
        // in case other I/O is going on in the background.
        parser p(s, new dem_stream_file);
        p.open(argv[1]);

        // create handler and attach parser
        handler_visualize h(&p);

        // parse all messages
        std::cout << "[";
        p.handle();
        std::cout << "{}]";
    } catch (boost::exception &e) {
        std::cout << boost::diagnostic_information(e) << std::endl;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
