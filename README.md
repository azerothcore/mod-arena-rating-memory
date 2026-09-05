# mod-arena-rating-memory

An [AzerothCore](http://www.azerothcore.org) module that remembers each player's **personal arena
rating and played games per arena team**. Leave a team and rejoin it later, and you get your old
standing back instead of starting over.

## What it does

Without this module, leaving an arena team throws your personal rating and your played games away.
Rejoin and you start from the server default, exactly like someone who has never played for that
team.

With it:

1. You have 1800 personal rating and 6 games played this week in **Team A**.
2. You leave Team A and join **Team B**, where you have never played — you start at the normal
   default there.
3. You leave Team B and rejoin **Team A** — you are back at 1800, with your 6 games still on the
   board.

The memory is per team, not per bracket. Two different 2v2 teams keep separate records for you.

### The rating you get when joining

```
PR = max(remembered rating, starting rating a brand new member would get)
```

The second term is the value the core computed for that join, so a returning member is never worse
off than a stranger joining the same team, and a server that changed what a new member starts with
is followed automatically. With default settings (arena season 6 or later,
`Arena.ArenaStartPersonalRating = 0`):

| Remembered | Team rating | You get | Why |
|---|---|---|---|
| 500 | 450 | 500 | remembered rating wins |
| 500 | 600 | 500 | remembered rating wins |
| 500 | 1200 | 1000 | a new member of a 1000+ team would start at 1000 |
| 1100 | 900 | 1100 | remembered rating wins |
| 1200 | 1500 | 1200 | remembered rating wins |

A player with no memory for a team is unaffected — the module does nothing and the core's normal
behaviour applies.

The memory is symmetric: a rating you dropped comes back with you as well as one you earned. Without
this module a player can reset a bad personal rating by leaving a team and rejoining it, and that
door is now closed.

### What is restored

The personal rating, plus the week and season games and wins you played for that team. The counters
come back exactly as you left them, whichever way the rating comparison above went.

That matters for arena points: the core only pays them to members who played at least 30% of the
team's games that week. Play your weekly games for Team A, leave to help a friend get Team B to ten
games, rejoin Team A before the payout, and you still get Team A's points — without the module you
would have to play A's games all over again.

Worked example. You are remembered in Team A with 500 rating, 6 games and 3 wins this week, 40 games
and 22 wins this season. Team A is now rated 1200, so a stranger joining it would start at 1000. You
rejoin: you get 1000 rating — the floor for that team — and your 6/3 and 40/22 back untouched. The
floor protects the rating; it says nothing about your history.

### Weekly flush

When the core distributes arena points and resets every team's week statistics, the module clears
the remembered week games and wins of every row — players still in a team and players who left
alike. Nobody can sit out the reset in another team and bring stale week games back. Season counters
are never touched.

### When memory is forgotten

| Event | Memory |
|---|---|
| You leave or are kicked from the team | **kept** — this is the point of the module |
| You rejoin | kept, so it works again next time |
| The team is disbanded | deleted |
| A season reset wipes all teams (`.arena season deleteteams`) | deleted |
| The character is deleted | deleted |

Cleanup is done by `ON DELETE CASCADE` foreign keys rather than by a background task, so it happens
in the same database transaction that removes the team. There is nothing to schedule and nothing
that can drift out of sync.

## Requirements

AzerothCore with the `ArenaScript` hooks `OnAddMember` and `CanSaveToDB`, the `GlobalScript` hook
`OnArenaWeekReset`, and a characters database using InnoDB (the default).

`OnAddMember` and `OnArenaWeekReset` were added in
[azerothcore-wotlk#27512](https://github.com/azerothcore/azerothcore-wotlk/pull/27512), so the module
needs a core at least that recent.

## Installation

```sh
cd path/to/azerothcore/modules
git clone https://github.com/azerothcore/mod-arena-rating-memory.git
cd path/to/azerothcore/build
cmake ../ -DCMAKE_INSTALL_PREFIX=/path/to/azerothcore/env/dist/ -DTOOLS=0
make -j $(nproc)
make install
```

The SQL is applied automatically by the database updater on the next worldserver start.

## Configuration

Copy `conf/mod_arena_rating_memory.conf.dist` to `mod_arena_rating_memory.conf` in your config
directory and edit it there. Defaults work out of the box.

| Option | Default | Meaning |
|---|---|---|
| `ArenaRatingMemory.Enable` | `1` | Master switch |
| `ArenaRatingMemory.Announce` | `0` | Tell players on login that the module is running |

Setting `ArenaRatingMemory.Enable = 0` makes the module completely inert: it stops recording as well
as restoring, and it stops clearing the week counters at the weekly flush. If you leave it off for a
while, players will leave teams unobserved and the stored ratings and counters will go stale. Clear
the table before switching it back on:

```sql
DELETE FROM `mod_arena_rating_memory`;
```

The next server start repopulates it from everyone currently in a team.

## Commands

Both sit under the core's `.arena` command and only touch this module's table. Neither can change a
player's live arena rating.

| Command | Level | What it does |
|---|---|---|
| `.arena ratingmemory show [player]` | Game Master | Lists what is remembered for that character: team id and name, rating, week and season games/wins, last update |
| `.arena ratingmemory clear [player] [teamId]` | Administrator | Deletes what is remembered for that character, or just the entry for a given team |

Both default to your current target or yourself when no player is given, and both work from the
console.

Clearing the memory of a player who is still in the team is temporary: their rating and counters are
recorded again after their next match. Clearing is meant for teams they have already left.

For tracing a restore as it happens, enable the module's debug log in `worldserver.conf`:

```
Logger.module.arenaratingmemory=5,Console Server
```

## Notes

- **1v1 arena.** If you run [mod-1v1-arena](https://github.com/azerothcore/mod-1v1-arena), its teams
  are real persisted arena teams, so they are covered too. In practice the effect is limited: that
  module destroys and recreates a player's 1v1 team whenever they make a new one, which forgets the
  rating along with the team.
- **Solo queue.** [mod-arena-3v3-solo-queue](https://github.com/azerothcore/mod-arena-3v3-solo-queue)
  gives every player a persistent one-man arena team, so those teams do go through this module. It
  has no visible effect there: that module sets each member's personal rating to the team rating
  after every match and overrides personal rating reads with the team rating, so there is no
  independent value left to remember. The throwaway teams it builds for the match itself are
  skipped, like the ones the battleground queue makes.
- **mod-glicko2-mmr.** If that module is enabled it takes over personal rating updates after a
  match, so the values recorded here are the ones it produces. The two have not been tested
  together.
- Because `mod_arena_rating_memory` references `arena_team`, InnoDB will refuse `DROP TABLE` and
  `TRUNCATE TABLE` on it while the module table exists. Drop `mod_arena_rating_memory` first if you
  ever need to do that by hand.

## Uninstalling

```sql
DROP TABLE `mod_arena_rating_memory`;
```

Core tables are left untouched.

## License

Released under [AGPL 3.0](LICENSE).
