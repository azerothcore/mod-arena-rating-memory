/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#ifndef MOD_ARENA_RATING_MEMORY_CONFIG_H
#define MOD_ARENA_RATING_MEMORY_CONFIG_H

#include "ArenaRatingMemoryFormula.h"
#include "ConfigValueCache.h"

enum class ArenaRatingMemoryConfigId
{
    ENABLED,
    FLOOR_THRESHOLD,
    FLOOR_AT_OR_ABOVE,
    FLOOR_BELOW,
    ANNOUNCE,

    NUM_CONFIGS
};

class ArenaRatingMemoryConfig : public ConfigValueCache<ArenaRatingMemoryConfigId>
{
public:
    ArenaRatingMemoryConfig() : ConfigValueCache(ArenaRatingMemoryConfigId::NUM_CONFIGS) { }

    void BuildConfigCache() override;

    [[nodiscard]] bool IsEnabled() const { return GetConfigValue<bool>(ArenaRatingMemoryConfigId::ENABLED); }
    [[nodiscard]] bool ShouldAnnounce() const { return GetConfigValue<bool>(ArenaRatingMemoryConfigId::ANNOUNCE); }
    [[nodiscard]] ArenaRatingMemory::StartingRatingConfig GetStartingRatingConfig() const;
};

extern ArenaRatingMemoryConfig sArenaRatingMemoryConfig;

#endif // MOD_ARENA_RATING_MEMORY_CONFIG_H
