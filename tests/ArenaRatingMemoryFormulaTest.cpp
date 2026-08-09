/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryFormula.h"
#include "gtest/gtest.h"

using namespace ArenaRatingMemory;

namespace
{
    constexpr uint8 MODERN_SEASON = 8;
    constexpr uint8 LEGACY_SEASON = 5;
    constexpr uint32 NO_START_RATING_CONFIG = 0;

    // Defaults from the .conf.dist, which reproduce ArenaTeam::AddMember.
    constexpr StartingRatingConfig DEFAULTS = { 1000, 1000, 0 };

    uint32 JoinRating(uint32 remembered, uint32 teamRating, StartingRatingConfig const& config = DEFAULTS,
        uint8 season = MODERN_SEASON, uint32 startRatingConfig = NO_START_RATING_CONFIG)
    {
        return ComputeJoinRating(remembered, ComputeStartingRating(season, startRatingConfig, teamRating, config));
    }
}

// The five cases the module was specified against.
TEST(ArenaRatingMemoryFormula, RestoresRememberedRatingBelowTheTeamRating)
{
    EXPECT_EQ(JoinRating(500, 450), 500u);
    EXPECT_EQ(JoinRating(500, 600), 500u);
}

TEST(ArenaRatingMemoryFormula, RaisesToTheStartingRatingWhenTheTeamQualifiesForIt)
{
    EXPECT_EQ(JoinRating(500, 1200), 1000u);
}

TEST(ArenaRatingMemoryFormula, DoesNotCapTheRememberedRatingAtTheTeamRating)
{
    EXPECT_EQ(JoinRating(1100, 900), 1100u);
    EXPECT_EQ(JoinRating(1200, 1500), 1200u);
}

// A player with no memory must be left exactly where the core would have put them.
TEST(ArenaRatingMemoryFormula, WithoutMemoryMatchesTheCoreStartingRating)
{
    EXPECT_EQ(JoinRating(0, 900), 0u);
    EXPECT_EQ(JoinRating(0, 1000), 1000u);
    EXPECT_EQ(JoinRating(0, 2400), 1000u);
}

TEST(ArenaRatingMemoryFormula, ArenaStartPersonalRatingConfigOverridesEverythingElse)
{
    EXPECT_EQ(JoinRating(900, 800, DEFAULTS, MODERN_SEASON, 1500), 1500u);
    EXPECT_EQ(JoinRating(1800, 800, DEFAULTS, MODERN_SEASON, 1500), 1800u);
}

TEST(ArenaRatingMemoryFormula, SeasonsBeforeSixStartAtFifteenHundred)
{
    EXPECT_EQ(JoinRating(900, 0, DEFAULTS, LEGACY_SEASON), 1500u);
    EXPECT_EQ(JoinRating(1800, 0, DEFAULTS, LEGACY_SEASON), 1800u);
}

// The three floor numbers are config, not constants.
TEST(ArenaRatingMemoryFormula, HonoursANonDefaultFloorConfig)
{
    constexpr StartingRatingConfig custom = { 1500, 1200, 500 };

    EXPECT_EQ(JoinRating(0, 1499, custom), 500u);
    EXPECT_EQ(JoinRating(0, 1500, custom), 1200u);
    EXPECT_EQ(JoinRating(800, 1400, custom), 800u);
    EXPECT_EQ(JoinRating(400, 1400, custom), 500u);
}

TEST(ArenaRatingMemoryFormula, ThresholdIsInclusive)
{
    EXPECT_EQ(ComputeStartingRating(MODERN_SEASON, NO_START_RATING_CONFIG, 999, DEFAULTS), 0u);
    EXPECT_EQ(ComputeStartingRating(MODERN_SEASON, NO_START_RATING_CONFIG, 1000, DEFAULTS), 1000u);
}
