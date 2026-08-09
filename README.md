# mod-arena-rating-memory

An [AzerothCore](http://www.azerothcore.org) module that remembers each player's **personal arena
rating per arena team**. Leave a team and rejoin it later, and you get your old rating back instead
of starting over.

## What it does

Without this module, leaving an arena team throws your personal rating away. Rejoin and you start
from the server default, exactly like someone who has never played for that team.

With it:

1. You have 1800 personal rating in **Team A**.
2. You leave Team A and join **Team B**, where you have never played — you start at the normal
   default there.
3. You leave Team B and rejoin **Team A** — you are back at 1800.

The memory is per team, not per bracket. Two different 2v2 teams keep separate ratings for you.

### The rating you get when joining

```
PR = max(remembered rating, starting rating a brand new member would get)
```

The second term is the core's own rule, so a returning member is never worse off than a stranger
joining the same team. With default settings (arena season 6 or later,
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

AzerothCore with the `ArenaScript` hooks `CanAddMember` and `CanSaveToDB`, and a characters database
using InnoDB (the default).

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
| `ArenaRatingMemory.FloorThreshold` | `1000` | Team rating at or above which the higher starting rating applies |
| `ArenaRatingMemory.FloorAtOrAbove` | `1000` | Starting rating when the team rating is at or above the threshold |
| `ArenaRatingMemory.FloorBelow` | `0` | Starting rating when the team rating is below the threshold |
| `ArenaRatingMemory.Announce` | `1` | Tell players on login that the module is running |

The three `Floor*` options exist so servers that changed what a new arena team member starts with can
keep the module in sync. Leave them alone unless you did.

## Notes

- **1v1 arena.** If you run [mod-1v1-arena](https://github.com/azerothcore/mod-1v1-arena), its teams
  are real persisted arena teams, so they are covered too. In practice the effect is limited: that
  module destroys and recreates a player's 1v1 team whenever they make a new one, which forgets the
  rating along with the team.
- **Solo queue.** Temporary teams built by the battleground queue and by
  [mod-arena-3v3-solo-queue](https://github.com/azerothcore/mod-arena-3v3-solo-queue) are ignored.
- **mod-glicko2-mmr.** If that module is enabled it takes over personal rating updates, and the
  "starting rating" logic here no longer describes what a new member receives. The two have not been
  tested together.
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
