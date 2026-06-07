-- ============================================================
-- WorldChat Script — SQL Setup
-- ============================================================
-- Run this on your CHARACTER database.
-- ============================================================

CREATE TABLE IF NOT EXISTS `worldchat_settings` (
  `guid`    INT(11) UNSIGNED NOT NULL COMMENT 'Player GUID',
  `enabled` TINYINT(1) NOT NULL DEFAULT 1 COMMENT '1 = WorldChat visible, 0 = hidden',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Stores per-player WorldChat opt-in/out preference';

-- That's it! No data needs to be inserted manually.
-- The script uses REPLACE INTO, so rows are created automatically
-- when a player first toggles their setting via .wc on / .wc off.
