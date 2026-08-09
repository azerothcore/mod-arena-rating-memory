/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryConfig.h"

ArenaRatingMemoryConfig sArenaRatingMemoryConfig;

void ArenaRatingMemoryConfig::BuildConfigCache()
{
    SetConfigValue<bool>(ArenaRatingMemoryConfigId::ENABLED, "ArenaRatingMemory.Enable", true);
    SetConfigValue<bool>(ArenaRatingMemoryConfigId::ANNOUNCE, "ArenaRatingMemory.Announce", false);
}
