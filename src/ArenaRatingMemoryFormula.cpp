/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryFormula.h"
#include <algorithm>

namespace
{
    // Seasons before 6 predate the "start at 0 / 1000" rule; see ArenaTeam::AddMember.
    constexpr uint8 FIRST_MODERN_ARENA_SEASON = 6;
    constexpr uint32 LEGACY_SEASON_PERSONAL_RATING = 1500;
}

uint32 ArenaRatingMemory::ComputeStartingRating(uint8 currentSeason, uint32 startPersonalRatingConfig,
    uint32 teamRating, StartingRatingConfig const& config)
{
    if (startPersonalRatingConfig > 0)
        return startPersonalRatingConfig;

    if (currentSeason < FIRST_MODERN_ARENA_SEASON)
        return LEGACY_SEASON_PERSONAL_RATING;

    return teamRating >= config.floorThreshold ? config.floorAtOrAbove : config.floorBelow;
}

uint32 ArenaRatingMemory::ComputeJoinRating(uint32 rememberedRating, uint32 startingRating)
{
    return std::max(rememberedRating, startingRating);
}
