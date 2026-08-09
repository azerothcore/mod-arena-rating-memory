-- Remembered personal arena rating, one row per (character, arena team).
--
-- Both foreign keys are ON DELETE CASCADE on purpose: they are the only cleanup mechanism the
-- module has. Every way an arena team dies ends in a DELETE on `arena_team` -- ArenaTeam::Disband()
-- for a single team, ArenaTeamMgr::DeleteAllArenaTeams() for a season reset, and mod-1v1-arena's
-- raw DELETE when a player recreates their 1v1 team -- and none of those is hookable from a module.
-- Cascading also keeps the removal inside the same transaction as the team deletion, which matters
-- because DeleteAllArenaTeams() resets NextArenaTeamId to 1: a row that outlived its team could
-- otherwise be inherited by an unrelated future team that reuses the id.
--
-- There is deliberately NO foreign key to `arena_team_member`: the row must outlive that one, which
-- is the entire point of the module.
CREATE TABLE IF NOT EXISTS `mod_arena_rating_memory` (
  `guid`           INT UNSIGNED      NOT NULL,
  `arenaTeamId`    INT UNSIGNED      NOT NULL,
  `personalRating` SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `updatedAt`      TIMESTAMP         NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`, `arenaTeamId`),
  KEY `fk_armem_team` (`arenaTeamId`),
  CONSTRAINT `fk_armem_team`      FOREIGN KEY (`arenaTeamId`) REFERENCES `arena_team` (`arenaTeamId`) ON DELETE CASCADE,
  CONSTRAINT `fk_armem_character` FOREIGN KEY (`guid`)        REFERENCES `characters` (`guid`)        ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
