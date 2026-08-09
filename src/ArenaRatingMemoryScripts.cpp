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
        ARENAHOOK_ON_GET_START_PERSONAL_RATING,
        ARENAHOOK_CAN_SAVE_TO_DB
    }) { }

    // personalRating arrives holding what the core would give a first-time joiner of this team, so
    // a remembered rating only has to beat that. Whatever is left here is what ArenaTeam::AddMember
    // stores on the member and writes to arena_team_member.
    void OnGetStartPersonalRating(ArenaTeam* team, ObjectGuid playerGuid, uint32& personalRating) override
    {
        if (!sArenaRatingMemoryConfig.IsEnabled() || !IsPersistentTeam(team))
            return;

        Optional<uint32> const remembered = ArenaRatingMemory::Recall(team->GetId(), playerGuid);
        if (!remembered)
            return;

        uint32 const restored = ArenaRatingMemory::ComputeJoinRating(*remembered, personalRating);
        if (restored == personalRating)
            return;

        personalRating = restored;

        // AddMember never sets ARENA_TEAM_PERSONAL_RATING itself: DelMember zeroed the whole slot on
        // the way out, and the field is only refilled at login or after the next rated match. Left
        // alone, a restored rating would not count towards rating-gated vendor items until then.
        if (Player* player = ObjectAccessor::FindConnectedPlayer(playerGuid))
        {
            // mod-arena-3v3-solo-queue uses slot 4, which is out of range for the player field
            // block, so this guard is load-bearing rather than defensive.
            uint8 const slot = team->GetSlot();
            if (slot < MAX_ARENA_SLOT)
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_PERSONAL_RATING, restored);
        }

        LOG_DEBUG("module.arenaratingmemory", "Restored personal rating {} for {} in arena team {}",
            restored, playerGuid.ToString(), team->GetId());
    }

    // A personal rating only ever changes right before a SaveToDB, so mirroring here captures every
    // value a player could later leave the team with. Never blocks the save.
    //
    // Reached from a battleground map thread via Arena::EndBattleground, so it must not touch any
    // module state beyond the database.
    bool CanSaveToDB(ArenaTeam* team) override
    {
        if (!sArenaRatingMemoryConfig.IsEnabled() || !IsPersistentTeam(team))
            return true;

        std::vector<ArenaRatingMemory::MemberRating> members;
        members.reserve(team->GetMembersSize());

        for (ArenaTeamMember const& member : team->GetMembers())
            members.push_back({ member.Guid, member.PersonalRating });

        ArenaRatingMemory::RememberTeam(team->GetId(), members);
        return true;
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
    new ArenaRatingMemoryWorldScript();
    new ArenaRatingMemoryPlayerScript();
}
