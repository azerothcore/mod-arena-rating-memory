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
    struct MemberRating
    {
        ObjectGuid Guid;
        uint32 PersonalRating;
    };

    struct RememberedTeam
    {
        uint32 ArenaTeamId;
        std::string TeamName;
        uint32 PersonalRating;
        std::string UpdatedAt;
    };

    // Re-syncs the ratings of everyone currently in a team from arena_team_member, which is
    // authoritative for them. Departed players are left alone. Safe on every startup, and it heals
    // the drift left behind by running with the module disabled for a while.
    void SeedFromExistingTeams();

    void RememberTeam(uint32 arenaTeamId, std::vector<MemberRating> const& members);

    [[nodiscard]] Optional<uint32> Recall(uint32 arenaTeamId, ObjectGuid playerGuid);

    [[nodiscard]] std::vector<RememberedTeam> RecallAll(ObjectGuid playerGuid);

    void Forget(ObjectGuid playerGuid, Optional<uint32> arenaTeamId);
}

#endif // MOD_ARENA_RATING_MEMORY_STORE_H
