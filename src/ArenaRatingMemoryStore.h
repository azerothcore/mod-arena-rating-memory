/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef MOD_ARENA_RATING_MEMORY_STORE_H
#define MOD_ARENA_RATING_MEMORY_STORE_H

#include "ObjectGuid.h"
#include "Optional.h"

namespace ArenaRatingMemory
{
    // Copies the personal ratings of teams that already existed before the module was installed.
    // Safe to run on every startup: existing rows win.
    void SeedFromExistingTeams();

    void Remember(uint32 arenaTeamId, ObjectGuid playerGuid, uint32 personalRating);

    [[nodiscard]] Optional<uint32> Recall(uint32 arenaTeamId, ObjectGuid playerGuid);
}

#endif // MOD_ARENA_RATING_MEMORY_STORE_H
