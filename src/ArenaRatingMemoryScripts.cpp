/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryConfig.h"
#include "ArenaRatingMemoryFormula.h"
#include "ArenaRatingMemoryStore.h"
#include "ArenaScript.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Chat.h"
#include "GlobalScript.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <vector>

namespace
{
    bool IsPersistentTeam(ArenaTeam const* team)
    {
        // Solo queue and battleground code build throwaway teams above this id; they have no row in
        // arena_team, so they must not reach the module.
        return team && team->GetId() < MAX_ARENA_TEAM_ID;
    }
}

class ArenaRatingMemoryArenaScript : public ArenaScript
{
public:
    ArenaRatingMemoryArenaScript() : ArenaScript("ArenaRatingMemoryArenaScript", {
        ARENAHOOK_ON_ADD_MEMBER,
        ARENAHOOK_CAN_SAVE_TO_DB
    }) { }

    // The member arrives holding what the core built for a first-time joiner of this team: the
    // starting rating a remembered one only has to beat, and zeroed counters. Whatever is left here
    // is what ArenaTeam::AddMember keeps on the member and writes to arena_team_member.
    void OnAddMember(ArenaTeam* team, ArenaTeamMember& member) override
    {
        if (!sArenaRatingMemoryConfig.IsEnabled() || !IsPersistentTeam(team))
            return;

        Optional<ArenaRatingMemory::RememberedStats> const remembered =
            ArenaRatingMemory::Recall(team->GetId(), member.Guid);

        if (!remembered)
            return;

        ArenaRatingMemory::ApplyRemembered(member, *remembered);

        // AddMember never refills the player fields itself: DelMember zeroed the whole slot on the
        // way out, and they are only written again at login or after the next rated match. Left
        // alone, a restored rating would not count towards rating-gated vendor items until then, and
        // the PvP frame would show none of the restored games.
        if (Player* player = ObjectAccessor::FindConnectedPlayer(member.Guid))
        {
            // mod-arena-3v3-solo-queue uses slot 4, which is out of range for the player field
            // block, so this guard is load-bearing rather than defensive.
            uint8 const slot = team->GetSlot();
            if (slot < MAX_ARENA_SLOT)
            {
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_PERSONAL_RATING, member.PersonalRating);
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_GAMES_WEEK, member.WeekGames);
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_GAMES_SEASON, member.SeasonGames);
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_WINS_SEASON, member.SeasonWins);
            }
        }

        LOG_DEBUG("module.arenaratingmemory",
            "Restored rating {}, week {}/{}, season {}/{} (games/wins) for {} in arena team {}",
            member.PersonalRating, member.WeekGames, member.WeekWins, member.SeasonGames,
            member.SeasonWins, member.Guid.ToString(), team->GetId());
    }

    // A member's rating and counters only ever change right before a SaveToDB, so mirroring here
    // captures every value a player could later leave the team with. Never blocks the save.
    //
    // Reached from a battleground map thread via Arena::EndBattleground, so it must not touch any
    // module state beyond the database.
    bool CanSaveToDB(ArenaTeam* team) override
    {
        if (!sArenaRatingMemoryConfig.IsEnabled() || !IsPersistentTeam(team))
            return true;

        std::vector<ArenaRatingMemory::MemberSnapshot> members;
        members.reserve(team->GetMembersSize());

        for (ArenaTeamMember const& member : team->GetMembers())
            members.push_back({ member.Guid, member.PersonalRating, member.WeekGames, member.WeekWins,
                member.SeasonGames, member.SeasonWins });

        ArenaRatingMemory::RememberTeam(team->GetId(), members);
        return true;
    }
};

// The core resets the week statistics of every arena team in one sweep, so the remembered week
// counters go with them, departed members included: a player must not be able to sit out the reset
// in another team and bring their old week games back.
class ArenaRatingMemoryGlobalScript : public GlobalScript
{
public:
    ArenaRatingMemoryGlobalScript() : GlobalScript("ArenaRatingMemoryGlobalScript", {
        GLOBALHOOK_ON_ARENA_WEEK_RESET
    }) { }

    void OnArenaWeekReset() override
    {
        if (sArenaRatingMemoryConfig.IsEnabled())
            ArenaRatingMemory::ForgetWeek();
    }
};

class ArenaRatingMemoryWorldScript : public WorldScript
{
public:
    ArenaRatingMemoryWorldScript() : WorldScript("ArenaRatingMemoryWorldScript", {
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD,
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnBeforeConfigLoad(bool reload) override
    {
        sArenaRatingMemoryConfig.Initialize(reload);
    }

    void OnStartup() override
    {
        if (sArenaRatingMemoryConfig.IsEnabled())
            ArenaRatingMemory::SeedFromExistingTeams();
    }
};

class ArenaRatingMemoryPlayerScript : public PlayerScript
{
public:
    ArenaRatingMemoryPlayerScript() : PlayerScript("ArenaRatingMemoryPlayerScript", {
        PLAYERHOOK_ON_LOGIN
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (sArenaRatingMemoryConfig.IsEnabled() && sArenaRatingMemoryConfig.ShouldAnnounce())
            ChatHandler(player->GetSession()).SendSysMessage(
                "This server is running the |cff4CFF00mod-arena-rating-memory|r module.");
    }
};

void AddArenaRatingMemoryScripts()
{
    new ArenaRatingMemoryArenaScript();
    new ArenaRatingMemoryGlobalScript();
    new ArenaRatingMemoryWorldScript();
    new ArenaRatingMemoryPlayerScript();
}
