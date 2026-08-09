/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryConfig.h"
#include "ArenaRatingMemoryFormula.h"
#include "ArenaRatingMemoryStore.h"
#include "ArenaScript.h"
#include "ArenaSeasonMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"
#include <vector>

namespace
{
    struct PendingRestore
    {
        uint32 ArenaTeamId;
        ObjectGuid PlayerGuid;
    };

    // Only ever touched from the world thread: ArenaScript::CanAddMember fills it while handling a
    // packet, WorldScript::OnUpdate drains it at the end of the same tick.
    std::vector<PendingRestore> pendingRestores;

    bool IsPersistentTeam(ArenaTeam const* team)
    {
        // Solo queue and battleground code build throwaway teams above this id; they never call
        // AddMember and have no row in arena_team, so they must not reach the module.
        return team && team->GetId() < MAX_ARENA_TEAM_ID;
    }

    void ApplyRestore(PendingRestore const& pending)
    {
        ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(pending.ArenaTeamId);
        if (!team)
            return;

        // AddMember can still bail out after CanAddMember returned true, e.g. when the player
        // already belongs to another team of the same size.
        ArenaTeamMember* member = team->GetMember(pending.PlayerGuid);
        if (!member)
            return;

        Optional<uint32> const remembered = ArenaRatingMemory::Recall(pending.ArenaTeamId, pending.PlayerGuid);
        if (!remembered)
            return;

        uint32 const startingRating = ArenaRatingMemory::ComputeStartingRating(
            sArenaSeasonMgr->GetCurrentSeason(),
            sWorld->getIntConfig(CONFIG_ARENA_START_PERSONAL_RATING),
            team->GetRating(),
            sArenaRatingMemoryConfig.GetStartingRatingConfig());

        uint32 const restored = ArenaRatingMemory::ComputeJoinRating(*remembered, startingRating);
        if (restored == member->PersonalRating)
            return;

        member->PersonalRating = static_cast<uint16>(restored);

        // AddMember already inserted the row with the default rating, so this is an update.
        CharacterDatabase.Execute(
            "UPDATE arena_team_member SET personalRating = {} WHERE arenaTeamId = {} AND guid = {}",
            restored, pending.ArenaTeamId, pending.PlayerGuid.GetCounter());

        if (Player* player = ObjectAccessor::FindConnectedPlayer(pending.PlayerGuid))
        {
            // mod-arena-3v3-solo-queue uses slot 4, which is out of range for the player field
            // block, so this guard is load-bearing rather than defensive.
            uint8 const slot = team->GetSlot();
            if (slot < MAX_ARENA_SLOT)
                player->SetArenaTeamInfoField(slot, ARENA_TEAM_PERSONAL_RATING, restored);

            // Roster is what serialises member personal ratings; NotifyStatsChanged only sends team
            // stats, so it would leave an open arena frame showing the default value.
            team->Roster(player->GetSession());
        }

        LOG_DEBUG("module.arenaratingmemory", "Restored personal rating {} for {} in arena team {}",
            restored, pending.PlayerGuid.ToString(), pending.ArenaTeamId);
    }
}

class ArenaRatingMemoryArenaScript : public ArenaScript
{
public:
    ArenaRatingMemoryArenaScript() : ArenaScript("ArenaRatingMemoryArenaScript", {
        ARENAHOOK_CAN_ADD_MEMBER,
        ARENAHOOK_CAN_SAVE_TO_DB
    }) { }

    // Fires before the member struct exists, so the rating cannot be written here yet -- queue it
    // and let the world update apply it. Never blocks the join.
    bool CanAddMember(ArenaTeam* team, ObjectGuid playerGuid) override
    {
        if (sArenaRatingMemoryConfig.IsEnabled() && IsPersistentTeam(team))
            pendingRestores.push_back({ team->GetId(), playerGuid });

        return true;
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
        WORLDHOOK_ON_STARTUP,
        WORLDHOOK_ON_UPDATE
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

    void OnUpdate(uint32 /*diff*/) override
    {
        if (pendingRestores.empty())
            return;

        for (PendingRestore const& pending : pendingRestores)
            ApplyRestore(pending);

        pendingRestores.clear();
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
