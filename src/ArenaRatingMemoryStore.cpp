/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryStore.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "QueryResult.h"

void ArenaRatingMemory::SeedFromExistingTeams()
{
    // INSERT IGNORE also skips arena_team_member rows whose team no longer exists. mod-1v1-arena
    // leaves such orphans behind (it deletes from arena_team without touching arena_team_member),
    // and they would otherwise abort the seed on a foreign key violation.
    CharacterDatabase.Execute(
        "INSERT IGNORE INTO mod_arena_rating_memory (guid, arenaTeamId, personalRating) "
        "SELECT guid, arenaTeamId, personalRating FROM arena_team_member");
}

void ArenaRatingMemory::Remember(uint32 arenaTeamId, ObjectGuid playerGuid, uint32 personalRating)
{
    // IGNORE guards against the team being deleted between this statement being queued and executed.
    CharacterDatabase.Execute(
        "INSERT IGNORE INTO mod_arena_rating_memory (guid, arenaTeamId, personalRating) VALUES ({}, {}, {}) "
        "ON DUPLICATE KEY UPDATE personalRating = {}",
        playerGuid.GetCounter(), arenaTeamId, personalRating, personalRating);
}

Optional<uint32> ArenaRatingMemory::Recall(uint32 arenaTeamId, ObjectGuid playerGuid)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT personalRating FROM mod_arena_rating_memory WHERE guid = {} AND arenaTeamId = {}",
        playerGuid.GetCounter(), arenaTeamId);

    if (!result)
        return std::nullopt;

    return result->Fetch()[0].Get<uint32>();
}
