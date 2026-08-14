/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryFormula.h"
#include <algorithm>

uint32 ArenaRatingMemory::ComputeJoinRating(uint32 rememberedRating, uint32 startingRating)
{
    return std::max(rememberedRating, startingRating);
}
