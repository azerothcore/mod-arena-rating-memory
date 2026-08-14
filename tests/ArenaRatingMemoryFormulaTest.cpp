/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryFormula.h"
#include "gtest/gtest.h"

using namespace ArenaRatingMemory;

// The second argument is the rating ArenaTeam::AddMember computed for the joining player, handed to
// the module through OnGetStartPersonalRating. On an unmodified core that is 0 for a team rated
// below 1000, 1000 for a team rated 1000 or more, 1500 before arena season 6, or
// Arena.ArenaStartPersonalRating when that is set.

// The cases the module was specified against, with the core defaults of season 6+ and
// Arena.ArenaStartPersonalRating = 0.
TEST(ArenaRatingMemoryFormula, RestoresARememberedRatingAboveTheCoreValue)
{
    EXPECT_EQ(ComputeJoinRating(500, 0), 500u);
    EXPECT_EQ(ComputeJoinRating(1100, 0), 1100u);
    EXPECT_EQ(ComputeJoinRating(1200, 1000), 1200u);
}

// Joining a team rated 1000 or more still starts at 1000, even for a player remembered below it.
TEST(ArenaRatingMemoryFormula, KeepsTheCoreValueWhenItIsHigher)
{
    EXPECT_EQ(ComputeJoinRating(500, 1000), 1000u);
    EXPECT_EQ(ComputeJoinRating(900, 1500), 1500u);
}

TEST(ArenaRatingMemoryFormula, LeavesTheCoreValueAloneWhenNothingIsRemembered)
{
    EXPECT_EQ(ComputeJoinRating(0, 0), 0u);
    EXPECT_EQ(ComputeJoinRating(0, 1000), 1000u);
}
