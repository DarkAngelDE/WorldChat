/*
 * WorldChat Script — Legion 7.x / MoPCore
 * ==========================================
 * Players can send global messages with .w <message>,
 * visible to all players who have WorldChat enabled.
 *
 * Supported commands:
 *   .w <message>   — Send a message to all players
 *   .wc on         — Enable WorldChat (default)
 *   .wc off        — Disable WorldChat (you won't see any messages)
 *   .wc status     — Show your current WorldChat status
 *
 * Requirements:
 *   - AzerothCore / MaNGOS / TrinityCore (Legion 7.x / MoPCore fork)
 *   - SQL setup (see sql/worldchat_setup.sql)
 *
 * Credits:
 *   Script by cybermist2 (cybermist2@gmail.com)
 *   Published under the WoWEmu.de community — https://wowemu.de
 *   Emulator resources: https://emervoid.de
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "WorldSession.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "ChatPackets.h"
#include <set>

// ============================================================
// Configuration
// ============================================================
#define WC_PLAYED_TIME  1500   // Minimum played time (in seconds) before a player may send messages
#define WC_MUTE_SPAM    5      // Cooldown (in seconds) applied after each WorldChat message (anti-spam)
#define WC_MUTE_CENSOR  300    // Mute duration (in seconds) for players who attempt to post forbidden links

// ============================================================
// Database helper functions
// ============================================================

bool WorldChat_IsEnabled(uint32 guid)
{
    QueryResult result = CharacterDatabase.PQuery(
        "SELECT `enabled` FROM `worldchat_settings` WHERE `guid` = %u", guid);
    if (!result)
        return true; // default: enabled
    return result->Fetch()[0].GetBool();
}

void WorldChat_SetEnabled(uint32 guid, bool enabled)
{
    CharacterDatabase.PExecute(
        "REPLACE INTO `worldchat_settings` (`guid`, `enabled`) VALUES (%u, %u)",
        guid, enabled ? 1 : 0);
}

// ============================================================
// Name link builder — faction icon + class color
// ============================================================

std::string WorldChat_GetNameLink(Player* player)
{
    std::string name = player->GetName();
    std::string classColor = "|cffFFFFFF";

    switch (player->getClass())
    {
        case CLASS_DEATH_KNIGHT: classColor = "|cffC41F3B"; break;
        case CLASS_DEMON_HUNTER: classColor = "|cffA330C9"; break;
        case CLASS_DRUID:        classColor = "|cffFF7D0A"; break;
        case CLASS_HUNTER:       classColor = "|cffABD473"; break;
        case CLASS_MAGE:         classColor = "|cff69CCF0"; break;
        case CLASS_PALADIN:      classColor = "|cffF58CBA"; break;
        case CLASS_PRIEST:       classColor = "|cffFFFFFF"; break;
        case CLASS_ROGUE:        classColor = "|cffFFF569"; break;
        case CLASS_SHAMAN:       classColor = "|cff0070DE"; break;
        case CLASS_WARLOCK:      classColor = "|cff9482C9"; break;
        case CLASS_WARRIOR:      classColor = "|cffC79C6E"; break;
        default:                 classColor = "|cffFFFFFF"; break;
    }

    return "|Hplayer:" + name + "|h" +
           "|cffFFFFFF[" + classColor + name + "|cffFFFFFF]|h|r";
}

// ============================================================
// Broadcast — sends a WorldChat message to all online players
// ============================================================

void WorldChat_Send(Player* player, const std::string& message)
{
    // ---- Forbidden content (links, hyperlinks, item links) ----
    static const std::vector<std::string> forbiddenPatterns =
    {
        "http://", "https://", "ftp://",
        ".com", ".net", ".org", ".biz", ".ir", ".zapto", ".de",
        ".ru", ".eu", ".info", ".tv", ".gg", ".io",
        "www.", "wow-", "-wow", "no-ip", ".servegame",
        "|TInterface", "\n",
        "|Hquest:", "|Htrade:", "|Htalent:", "|Henchant:",
        "|Hachievement:", "|Hglyph:", "|Hspell:", "Hitem:",
    };

    for (const auto& pattern : forbiddenPatterns)
    {
        if (message.find(pattern) != std::string::npos)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(
                "|cffFF0000[WorldChat]|r Links are not allowed! "
                "You have been muted for %u seconds.", WC_MUTE_CENSOR);

            PreparedStatement* mt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME);
            int64 muteTime = time(nullptr) + WC_MUTE_CENSOR;
            player->GetSession()->m_muteTime = muteTime;
            mt->setInt64(0, muteTime);
            mt->setUInt32(1, player->GetSession()->GetAccountId());
            LoginDatabase.Execute(mt);
            return;
        }
    }

    // ---- Minimum played time check ----
    if (player->GetTotalPlayedTime() <= WC_PLAYED_TIME)
    {
        uint32 remaining = WC_PLAYED_TIME - player->GetTotalPlayedTime();
        ChatHandler(player->GetSession()).PSendSysMessage(
            "|cffFF9900[WorldChat]|r You need |cffFFFF00%s|r more playtime to use WorldChat.",
            secsToTimeString(remaining).c_str());
        return;
    }

    // ---- Build the message string ----
    std::string msg;

    // Prefix
    msg += "|cff00FF00[WorldChat]|r ";

    // Faction icon
    msg += (player->GetTeam() == ALLIANCE)
        ? "|TInterface/pvpframe/pvp-currency-alliance:16|t "
        : "|TInterface/pvpframe/pvp-currency-horde:16|t ";

    // GM/Moderator badge
    if (player->GetSession()->GetSecurity() >= SEC_MODERATOR)
        msg += "|TInterface/CHATFRAME/UI-CHATICON-BLIZZ:14|t ";

    msg += WorldChat_GetNameLink(player);
    msg += "|cffFFFFFF: |cffF5DEB3";
    msg += message;

    char c_msg[1024];
    snprintf(c_msg, sizeof(c_msg), "%s", msg.c_str());

    // ---- Deliver to all players with WorldChat enabled ----
    SessionMap const& sessions = sWorld->GetAllSessions();
    for (auto const& itr : sessions)
    {
        if (!itr.second)
            continue;

        Player* plr = itr.second->GetPlayer();
        if (!plr || !plr->IsInWorld())
            continue;

        if (!WorldChat_IsEnabled(plr->GetGUIDLow()))
            continue;

        sWorld->SendServerMessage(SERVER_MSG_STRING, c_msg, plr);
    }
}

// ============================================================
// Command script registration
// ============================================================

class WorldChatCommandScript : public CommandScript
{
public:
    WorldChatCommandScript() : CommandScript("WorldChatCommandScript") { }

    // .w <message> — broadcast to all players
    static bool HandleWorldChatSay(ChatHandler* handler, const char* args)
    {
        if (!args || !*args)
        {
            handler->PSendSysMessage("|cffFF9900[WorldChat]|r Usage: .w <message>");
            return true;
        }

        Player* player = handler->GetSession()->GetPlayer();

        if (!WorldChat_IsEnabled(player->GetGUIDLow()))
        {
            handler->PSendSysMessage(
                "|cffFF0000[WorldChat]|r Your WorldChat is disabled. "
                "Enable it with |cffFFFFFF.wc on|r");
            return true;
        }

        WorldChat_Send(player, args);

        // Apply anti-spam cooldown
        PreparedStatement* mt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_MUTE_TIME);
        int64 muteTime = time(nullptr) + WC_MUTE_SPAM;
        player->GetSession()->m_muteTime = muteTime;
        mt->setInt64(0, muteTime);
        mt->setUInt32(1, player->GetSession()->GetAccountId());
        LoginDatabase.Execute(mt);

        return true;
    }

    // .wc on
    static bool HandleWcOn(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        WorldChat_SetEnabled(player->GetGUIDLow(), true);
        handler->PSendSysMessage(
            "|cff00FF00[WorldChat]|r WorldChat has been |cff00FF00enabled|r.");
        return true;
    }

    // .wc off
    static bool HandleWcOff(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        WorldChat_SetEnabled(player->GetGUIDLow(), false);
        handler->PSendSysMessage(
            "|cff00FF00[WorldChat]|r WorldChat has been |cffFF0000disabled|r.");
        return true;
    }

    // .wc status
    static bool HandleWcStatus(ChatHandler* handler, const char* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        bool enabled = WorldChat_IsEnabled(player->GetGUIDLow());
        handler->PSendSysMessage(
            "|cff00FF00[WorldChat]|r Status: %s",
            enabled
                ? "|cff00FF00Enabled|r"
                : "|cffFF0000Disabled|r");
        return true;
    }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> wcSub =
        {
            { "on",     SEC_PLAYER, false, &HandleWcOn,     "" },
            { "off",    SEC_PLAYER, false, &HandleWcOff,    "" },
            { "status", SEC_PLAYER, false, &HandleWcStatus, "" },
        };

        static std::vector<ChatCommand> root =
        {
            { "w",  SEC_PLAYER, false, &HandleWorldChatSay, ""     },
            { "wc", SEC_PLAYER, false, nullptr,             "", wcSub },
        };

        return root;
    }
};

void AddSC_WorldChat()
{
    new WorldChatCommandScript();
}
