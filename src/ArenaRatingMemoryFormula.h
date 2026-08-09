/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef MOD_ARENA_RATING_MEMORY_FORMULA_H
#define MOD_ARENA_RATING_MEMORY_FORMULA_H

#include "Define.h"

namespace ArenaRatingMemory
{
    // What a returning member gets: their remembered rating, but never below the rating the core
    // computed for them, which is what a first-time joiner of that team would have received.
    [[nodiscard]] uint32 ComputeJoinRating(uint32 rememberedRating, uint32 startingRating);
}

#endif // MOD_ARENA_RATING_MEMORY_FORMULA_H
