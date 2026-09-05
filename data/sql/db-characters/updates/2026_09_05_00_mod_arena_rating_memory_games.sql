-- Adds the remembered week/season games and wins to `mod_arena_rating_memory`.
--
-- Guarded twice, because the updater applies every file it finds in filename order and re-applies
-- any file whose hash changes:
--   * the table may not exist yet -- `2026_...` sorts before `mod_arena_rating_memory.sql`, so on a
--     fresh install this file runs before the base file creates the table (already in its final
--     shape, which is why doing nothing here is correct);
--   * the columns may already be there, on an install that has run this file before.

DROP PROCEDURE IF EXISTS `mod_arena_rating_memory_add_games`;

DELIMITER $$
CREATE PROCEDURE `mod_arena_rating_memory_add_games`()
BEGIN
    IF EXISTS (SELECT 1 FROM `INFORMATION_SCHEMA`.`TABLES`
               WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'mod_arena_rating_memory')
       AND NOT EXISTS (SELECT 1 FROM `INFORMATION_SCHEMA`.`COLUMNS`
                       WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'mod_arena_rating_memory'
                         AND `COLUMN_NAME` = 'weekGames') THEN
        ALTER TABLE `mod_arena_rating_memory`
            ADD COLUMN `weekGames`   SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `personalRating`,
            ADD COLUMN `weekWins`    SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `weekGames`,
            ADD COLUMN `seasonGames` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `weekWins`,
            ADD COLUMN `seasonWins`  SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `seasonGames`;
    END IF;
END$$
DELIMITER ;

CALL `mod_arena_rating_memory_add_games`();

DROP PROCEDURE IF EXISTS `mod_arena_rating_memory_add_games`;
