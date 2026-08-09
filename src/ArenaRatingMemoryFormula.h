/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef MOD_ARENA_RATING_MEMORY_FORMULA_H
#define MOD_ARENA_RATING_MEMORY_FORMULA_H

#include "Define.h"

namespace ArenaRatingMemory
{
    // Mirrors the hardcoded numbers in ArenaTeam::AddMember, exposed so servers that changed the
    // starting rating of a new arena team member can keep this module in sync with it.
    struct StartingRatingConfig
    {
        uint32 floorThreshold;
        uint32 floorAtOrAbove;
        uint32 floorBelow;
    };

    // What ArenaTeam::AddMember would assign to a player joining this team for the first time.
    [[nodiscard]] uint32 ComputeStartingRating(uint8 currentSeason, uint32 startPersonalRatingConfig,
        uint32 teamRating, StartingRatingConfig const& config);

    // What a returning member gets: their remembered rating, but never below what a first-time
    // joiner would have received.
    [[nodiscard]] uint32 ComputeJoinRating(uint32 rememberedRating, uint32 startingRating);
}

#endif // MOD_ARENA_RATING_MEMORY_FORMULA_H
