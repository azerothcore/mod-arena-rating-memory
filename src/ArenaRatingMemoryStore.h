/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef MOD_ARENA_RATING_MEMORY_STORE_H
#define MOD_ARENA_RATING_MEMORY_STORE_H

#include "ObjectGuid.h"
#include "Optional.h"
#include <string>
#include <vector>

// Nothing here may keep state in memory without locking: RememberTeam is reached from
// ArenaTeam::SaveToDB, which Arena::EndBattleground calls during a battleground map update, i.e. on
// a worker thread when MapUpdate.Threads is set. Only the database calls are thread safe.
namespace ArenaRatingMemory
{
    // What a player is remembered with in one team: everything the core throws away when they leave.
    struct RememberedStats
    {
        uint32 PersonalRating;
        uint32 WeekGames;
        uint32 WeekWins;
        uint32 SeasonGames;
        uint32 SeasonWins;
    };

    struct MemberSnapshot
    {
        ObjectGuid Guid;
        uint32 PersonalRating;
        uint32 WeekGames;
        uint32 WeekWins;
        uint32 SeasonGames;
        uint32 SeasonWins;
    };

    struct RememberedTeam
    {
        uint32 ArenaTeamId;
        std::string TeamName;
        uint32 PersonalRating;
        uint32 WeekGames;
        uint32 WeekWins;
        uint32 SeasonGames;
        uint32 SeasonWins;
        std::string UpdatedAt;
    };

    // Re-syncs everyone currently in a team from arena_team_member, which is authoritative for them.
    // Departed players are left alone. Safe on every startup, and it heals the drift left behind by
    // running with the module disabled for a while.
    void SeedFromExistingTeams();

    void RememberTeam(uint32 arenaTeamId, std::vector<MemberSnapshot> const& members);

    [[nodiscard]] Optional<RememberedStats> Recall(uint32 arenaTeamId, ObjectGuid playerGuid);

    [[nodiscard]] std::vector<RememberedTeam> RecallAll(ObjectGuid playerGuid);

    void Forget(ObjectGuid playerGuid, Optional<uint32> arenaTeamId);

    // Clears the remembered week counters of every row on the realm, current and departed members
    // alike, so week games cannot survive a weekly reset by parking in another team.
    void ForgetWeek();
}

#endif // MOD_ARENA_RATING_MEMORY_STORE_H
