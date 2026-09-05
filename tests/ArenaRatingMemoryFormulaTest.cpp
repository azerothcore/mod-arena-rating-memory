/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "ArenaRatingMemoryFormula.h"
#include "ArenaTeam.h"
#include "gtest/gtest.h"

using namespace ArenaRatingMemory;

// The second argument is the rating ArenaTeam::AddMember computed for the joining player, handed to
// the module through OnAddMember. On an unmodified core that is 0 for a team rated below 1000, 1000
// for a team rated 1000 or more, 1500 before arena season 6, or Arena.ArenaStartPersonalRating when
// that is set.

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

// ApplyRemembered puts a returning player back into the member the core just built. The rating goes
// through the formula above; the counters are that player's history in this team and are copied
// verbatim, whichever side of the rating comparison wins.
TEST(ArenaRatingMemoryApplyRemembered, RestoresTheCountersWhicheverRatingWins)
{
    struct Case
    {
        uint32 RememberedRating;
        uint32 StartingRating;
        uint32 ExpectedRating;
    };

    // The rows the module was specified against, remembered rating against what the core would give
    // a first-time joiner of a team rated 450, 600, 1200, 900 and 1500.
    Case const cases[] =
    {
        { 500,  0,    500  },
        { 500,  0,    500  },
        { 500,  1000, 1000 },
        { 1100, 0,    1100 },
        { 1200, 1000, 1200 }
    };

    for (Case const& testCase : cases)
    {
        RememberedStats const remembered = { testCase.RememberedRating, 6, 3, 40, 22 };

        ArenaTeamMember member;
        member.PersonalRating = testCase.StartingRating;
        member.WeekGames      = 0;
        member.WeekWins       = 0;
        member.SeasonGames    = 0;
        member.SeasonWins     = 0;

        ApplyRemembered(member, remembered);

        EXPECT_EQ(member.PersonalRating, testCase.ExpectedRating);
        EXPECT_EQ(member.WeekGames, 6u);
        EXPECT_EQ(member.WeekWins, 3u);
        EXPECT_EQ(member.SeasonGames, 40u);
        EXPECT_EQ(member.SeasonWins, 22u);
    }
}
