/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryConfig.h"

ArenaRatingMemoryConfig sArenaRatingMemoryConfig;

void ArenaRatingMemoryConfig::BuildConfigCache()
{
    SetConfigValue<bool>(ArenaRatingMemoryConfigId::ENABLED, "ArenaRatingMemory.Enable", true);
    SetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_THRESHOLD, "ArenaRatingMemory.FloorThreshold", 1000);
    SetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_AT_OR_ABOVE, "ArenaRatingMemory.FloorAtOrAbove", 1000);
    SetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_BELOW, "ArenaRatingMemory.FloorBelow", 0);
    SetConfigValue<bool>(ArenaRatingMemoryConfigId::ANNOUNCE, "ArenaRatingMemory.Announce", true);
}

ArenaRatingMemory::StartingRatingConfig ArenaRatingMemoryConfig::GetStartingRatingConfig() const
{
    return {
        GetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_THRESHOLD),
        GetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_AT_OR_ABOVE),
        GetConfigValue<uint32>(ArenaRatingMemoryConfigId::FLOOR_BELOW)
    };
}
