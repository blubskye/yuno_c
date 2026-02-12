#!/usr/bin/env python3
"""
Migrate a Yuno Gasai JS database to the C version schema.

Usage:
    python3 migrate_js_db.py <input_js.db> <output_c.db>
    python3 migrate_js_db.py <input_js.db>              # overwrites in-place (backs up first)

The script will:
  1. Back up the original database
  2. Create all C-schema tables
  3. Migrate data from JS tables, renaming columns as needed
  4. Drop old JS-only tables
  5. Create indexes
"""

import sqlite3
import sys
import shutil
import os
import json
import time


def migrate(src_path, dst_path):
    # If in-place, back up first
    if src_path == dst_path:
        backup = src_path + f".backup-{int(time.time())}"
        shutil.copy2(src_path, backup)
        print(f"Backed up original to: {backup}")

    if src_path != dst_path:
        shutil.copy2(src_path, dst_path)

    db = sqlite3.connect(dst_path)
    db.execute("PRAGMA journal_mode=WAL")
    db.execute("PRAGMA foreign_keys=OFF")

    # ──────────────────────────────────────────────
    # 1. guild_settings  (from: guilds)
    # ──────────────────────────────────────────────
    if table_exists(db, "guilds"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS guild_settings (
                guild_id TEXT PRIMARY KEY,
                prefix TEXT DEFAULT '.',
                spam_filter_enabled INTEGER DEFAULT 0,
                leveling_enabled INTEGER DEFAULT 1,
                join_message TEXT,
                join_message_title TEXT,
                invite_filter_enabled INTEGER DEFAULT 0,
                error_channel_id TEXT
            )
        """)
        rows = db.execute("SELECT id, prefix, onJoinDMMsg, onJoinDMMsgTitle, spamFilter, measureXP FROM guilds").fetchall()
        for r in rows:
            guild_id, prefix, join_msg, join_title, spam, xp = r
            spam_int = 1 if spam and str(spam).lower() in ("1", "true") else 0
            xp_int = 1 if xp is None or str(xp).lower() in ("1", "true") else 0
            db.execute("""
                INSERT OR IGNORE INTO guild_settings
                    (guild_id, prefix, spam_filter_enabled, leveling_enabled, join_message, join_message_title)
                VALUES (?, ?, ?, ?, ?, ?)
            """, (str(guild_id), prefix or ".", spam_int, xp_int, join_msg, join_title))

        # Migrate levelRoleMap from guilds into level_roles table
        db.execute("""
            CREATE TABLE IF NOT EXISTS level_roles (
                guild_id TEXT NOT NULL,
                level INTEGER NOT NULL,
                role_id TEXT NOT NULL,
                PRIMARY KEY (guild_id, level)
            )
        """)
        rows = db.execute("SELECT id, levelRoleMap FROM guilds WHERE levelRoleMap IS NOT NULL").fetchall()
        for guild_id, lrm in rows:
            try:
                mapping = json.loads(lrm) if isinstance(lrm, str) else {}
                for level_str, role_id in mapping.items():
                    db.execute("INSERT OR IGNORE INTO level_roles (guild_id, level, role_id) VALUES (?, ?, ?)",
                               (str(guild_id), int(level_str), str(role_id)))
            except (json.JSONDecodeError, ValueError):
                pass

        db.execute("DROP TABLE IF EXISTS guilds")
        print("  Migrated: guilds -> guild_settings + level_roles")

    # ──────────────────────────────────────────────
    # 2. user_xp  (from: experiences)
    # ──────────────────────────────────────────────
    if table_exists(db, "experiences"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS user_xp (
                user_id TEXT NOT NULL,
                guild_id TEXT NOT NULL,
                xp INTEGER DEFAULT 0,
                level INTEGER DEFAULT 0,
                PRIMARY KEY (user_id, guild_id)
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO user_xp (user_id, guild_id, xp, level)
            SELECT userID, guildID, COALESCE(exp, 0), COALESCE(level, 0) FROM experiences
        """)
        count = db.execute("SELECT COUNT(*) FROM user_xp").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS experiences")
        print(f"  Migrated: experiences -> user_xp ({count} rows)")

    # ──────────────────────────────────────────────
    # 3. mod_actions  (from: modActions)
    # ──────────────────────────────────────────────
    if table_exists(db, "modActions"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS mod_actions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                guild_id TEXT NOT NULL,
                moderator_id TEXT NOT NULL,
                target_id TEXT NOT NULL,
                action_type TEXT NOT NULL,
                reason TEXT,
                timestamp INTEGER NOT NULL
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO mod_actions (id, guild_id, moderator_id, target_id, action_type, reason, timestamp)
            SELECT id, gid, moderatorId, targetId, action, reason, COALESCE(timestamp, 0) FROM modActions
        """)
        count = db.execute("SELECT COUNT(*) FROM mod_actions").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS modActions")
        print(f"  Migrated: modActions -> mod_actions ({count} rows)")

    # ──────────────────────────────────────────────
    # 4. auto_clean_config  (from: channelcleans)
    # ──────────────────────────────────────────────
    if table_exists(db, "channelcleans"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS auto_clean_config (
                guild_id TEXT NOT NULL,
                channel_id TEXT NOT NULL,
                interval_minutes INTEGER DEFAULT 1,
                message_count INTEGER DEFAULT 60,
                warning_minutes INTEGER DEFAULT 5,
                enabled INTEGER DEFAULT 1,
                PRIMARY KEY (guild_id, channel_id)
            )
        """)
        rows = db.execute("SELECT gid, cname, cleantime, warningtime, remainingtime FROM channelcleans").fetchall()
        for gid, cname, cleantime, warningtime, remaining in rows:
            interval = int(cleantime) if cleantime else 1
            warning = int(warningtime) if warningtime else 5
            msg_count = int(remaining) if remaining and str(remaining).isdigit() else interval * 60
            db.execute("""
                INSERT OR IGNORE INTO auto_clean_config
                    (guild_id, channel_id, interval_minutes, message_count, warning_minutes, enabled)
                VALUES (?, ?, ?, ?, ?, 1)
            """, (str(gid), str(cname), interval, msg_count, warning))
        db.execute("DROP TABLE IF EXISTS channelcleans")
        print(f"  Migrated: channelcleans -> auto_clean_config ({len(rows)} rows)")

    # ──────────────────────────────────────────────
    # 5. voice_xp_config  (from: vcXpConfig)
    #    JS: xpPerInterval (per intervalSeconds)
    #    C:  xp_per_minute
    # ──────────────────────────────────────────────
    if table_exists(db, "vcXpConfig"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS voice_xp_config (
                guild_id TEXT PRIMARY KEY,
                enabled INTEGER DEFAULT 0,
                xp_per_minute INTEGER DEFAULT 5,
                min_users INTEGER DEFAULT 2,
                ignore_afk INTEGER DEFAULT 1
            )
        """)
        rows = db.execute("SELECT gid, enabled, xpPerInterval, intervalSeconds, ignoreAfkChannel FROM vcXpConfig").fetchall()
        for gid, enabled, xp_per_int, interval_sec, ignore_afk in rows:
            # Convert xp/interval to xp/minute
            interval_sec = int(interval_sec) if interval_sec else 300
            xp_per_int = int(xp_per_int) if xp_per_int else 10
            xp_per_min = max(1, round(xp_per_int * 60 / interval_sec)) if interval_sec > 0 else 5
            db.execute("""
                INSERT OR IGNORE INTO voice_xp_config
                    (guild_id, enabled, xp_per_minute, min_users, ignore_afk)
                VALUES (?, ?, ?, 2, ?)
            """, (str(gid), int(enabled or 0), xp_per_min, int(ignore_afk or 1)))
        db.execute("DROP TABLE IF EXISTS vcXpConfig")
        print(f"  Migrated: vcXpConfig -> voice_xp_config ({len(rows)} rows)")

    # ──────────────────────────────────────────────
    # 6. dm_inbox  (from: dmInbox)
    # ──────────────────────────────────────────────
    if table_exists(db, "dmInbox"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS dm_inbox (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id TEXT NOT NULL,
                username TEXT,
                content TEXT,
                timestamp INTEGER NOT NULL,
                read_status INTEGER DEFAULT 0
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO dm_inbox (id, user_id, username, content, timestamp, read_status)
            SELECT id, usrId, userTag, content, COALESCE(timestamp, 0), COALESCE(replied, 0) FROM dmInbox
        """)
        count = db.execute("SELECT COUNT(*) FROM dm_inbox").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS dmInbox")
        print(f"  Migrated: dmInbox -> dm_inbox ({count} rows)")

    # ──────────────────────────────────────────────
    # 7. bot_bans  (from: botBans)
    # ──────────────────────────────────────────────
    if table_exists(db, "botBans"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS bot_bans (
                user_id TEXT PRIMARY KEY,
                banned_by TEXT,
                reason TEXT,
                timestamp INTEGER NOT NULL
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO bot_bans (user_id, banned_by, reason, timestamp)
            SELECT id, bannedBy, reason, COALESCE(bannedAt, 0) FROM botBans
        """)
        count = db.execute("SELECT COUNT(*) FROM bot_bans").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS botBans")
        print(f"  Migrated: botBans -> bot_bans ({count} rows)")

    # ──────────────────────────────────────────────
    # 8. dm_config  (from: dmConfig)
    # ──────────────────────────────────────────────
    if table_exists(db, "dmConfig"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS dm_config (
                guild_id TEXT PRIMARY KEY,
                channel_id TEXT NOT NULL,
                enabled INTEGER DEFAULT 1
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO dm_config (guild_id, channel_id, enabled)
            SELECT gid, channelId, COALESCE(enabled, 1) FROM dmConfig
        """)
        count = db.execute("SELECT COUNT(*) FROM dm_config").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS dmConfig")
        print(f"  Migrated: dmConfig -> dm_config ({count} rows)")

    # ──────────────────────────────────────────────
    # 9. log_channels  (from: logChannels)
    # ──────────────────────────────────────────────
    if table_exists(db, "logChannels"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS log_channels (
                guild_id TEXT NOT NULL,
                log_type TEXT NOT NULL,
                channel_id TEXT NOT NULL,
                enabled INTEGER DEFAULT 1,
                PRIMARY KEY (guild_id, log_type)
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO log_channels (guild_id, log_type, channel_id, enabled)
            SELECT gid, logType, channelId, COALESCE(enabled, 1) FROM logChannels
        """)
        count = db.execute("SELECT COUNT(*) FROM log_channels").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS logChannels")
        print(f"  Migrated: logChannels -> log_channels ({count} rows)")

    # ──────────────────────────────────────────────
    # 10. log_settings  (from: logSettings)
    # ──────────────────────────────────────────────
    if table_exists(db, "logSettings"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS log_settings (
                guild_id TEXT PRIMARY KEY,
                flush_interval INTEGER DEFAULT 30,
                max_buffer_size INTEGER DEFAULT 50
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO log_settings (guild_id, flush_interval, max_buffer_size)
            SELECT gid, COALESCE(flushInterval, 30), COALESCE(maxBufferSize, 50) FROM logSettings
        """)
        count = db.execute("SELECT COUNT(*) FROM log_settings").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS logSettings")
        print(f"  Migrated: logSettings -> log_settings ({count} rows)")

    # ──────────────────────────────────────────────
    # 11. mention_responses  (from: mentionResponses)
    # ──────────────────────────────────────────────
    if table_exists(db, "mentionResponses"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS mention_responses (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                guild_id TEXT NOT NULL,
                user_id TEXT NOT NULL,
                trigger_word TEXT NOT NULL,
                response TEXT NOT NULL,
                image_url TEXT,
                UNIQUE(guild_id, trigger_word)
            )
        """)
        # JS version has no user_id column — set to empty string
        db.execute("""
            INSERT OR IGNORE INTO mention_responses (id, guild_id, user_id, trigger_word, response, image_url)
            SELECT id, gid, '', trigger, response, image FROM mentionResponses
        """)
        count = db.execute("SELECT COUNT(*) FROM mention_responses").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS mentionResponses")
        print(f"  Migrated: mentionResponses -> mention_responses ({count} rows)")

    # ──────────────────────────────────────────────
    # 12. bot_presence  (from: botPresence)
    #     JS: type is TEXT ("PLAYING", "WATCHING", etc.)
    #     C:  type is INTEGER (0=Playing, 1=Streaming, 2=Listening, 3=Watching, 5=Competing)
    # ──────────────────────────────────────────────
    if table_exists(db, "botPresence"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS bot_presence (
                id INTEGER PRIMARY KEY DEFAULT 1,
                type INTEGER DEFAULT 0,
                text TEXT,
                status TEXT DEFAULT 'online',
                stream_url TEXT
            )
        """)
        rows = db.execute("SELECT type, text, status, streamUrl FROM botPresence").fetchall()
        type_map = {
            "PLAYING": 0, "STREAMING": 1, "LISTENING": 2,
            "WATCHING": 3, "COMPETING": 5
        }
        for typ, text, status, stream_url in rows:
            type_int = type_map.get(str(typ).upper(), 0) if typ and not str(typ).isdigit() else int(typ or 0)
            db.execute("""
                INSERT OR REPLACE INTO bot_presence (id, type, text, status, stream_url)
                VALUES (1, ?, ?, ?, ?)
            """, (type_int, text, status or "online", stream_url))
        db.execute("DROP TABLE IF EXISTS botPresence")
        print(f"  Migrated: botPresence -> bot_presence")

    # ──────────────────────────────────────────────
    # 13. ban_images  (from: banImages)
    #     JS: banner = user who set it; C: user_id = target user
    # ──────────────────────────────────────────────
    if table_exists(db, "banImages"):
        db.execute("""
            CREATE TABLE IF NOT EXISTS ban_images (
                guild_id TEXT NOT NULL,
                user_id TEXT NOT NULL,
                image_url TEXT,
                PRIMARY KEY (guild_id, user_id)
            )
        """)
        db.execute("""
            INSERT OR IGNORE INTO ban_images (guild_id, user_id, image_url)
            SELECT gid, banner, image FROM banImages
        """)
        count = db.execute("SELECT COUNT(*) FROM ban_images").fetchone()[0]
        db.execute("DROP TABLE IF EXISTS banImages")
        print(f"  Migrated: banImages -> ban_images ({count} rows)")

    # ──────────────────────────────────────────────
    # 14. Drop JS-only tables
    # ──────────────────────────────────────────────
    if table_exists(db, "vcSessions"):
        db.execute("DROP TABLE IF EXISTS vcSessions")
        print("  Dropped: vcSessions (voice sessions are tracked in-memory in C)")

    # ──────────────────────────────────────────────
    # 15. Create C-only tables (empty)
    # ──────────────────────────────────────────────
    db.execute("""
        CREATE TABLE IF NOT EXISTS spam_warnings (
            user_id TEXT NOT NULL,
            guild_id TEXT NOT NULL,
            warnings INTEGER DEFAULT 0,
            last_warning INTEGER,
            PRIMARY KEY (user_id, guild_id)
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS activity_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            guild_id TEXT NOT NULL,
            user_id TEXT NOT NULL,
            channel_id TEXT,
            event_type TEXT NOT NULL,
            old_content TEXT,
            new_content TEXT,
            timestamp INTEGER NOT NULL
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS user_roles (
            guild_id TEXT NOT NULL,
            user_id TEXT NOT NULL,
            role_ids TEXT,
            PRIMARY KEY (guild_id, user_id)
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS spam_rules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            guild_id TEXT NOT NULL,
            pattern TEXT NOT NULL,
            action INTEGER DEFAULT 0,
            enabled INTEGER DEFAULT 1
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS level_roles (
            guild_id TEXT NOT NULL,
            level INTEGER NOT NULL,
            role_id TEXT NOT NULL,
            PRIMARY KEY (guild_id, level)
        )
    """)
    print("  Created: spam_warnings, activity_log, user_roles, spam_rules, level_roles (empty)")

    # ──────────────────────────────────────────────
    # 16. Create indexes
    # ──────────────────────────────────────────────
    db.execute("CREATE INDEX IF NOT EXISTS idx_user_xp_guild ON user_xp(guild_id)")
    db.execute("CREATE INDEX IF NOT EXISTS idx_mod_actions_guild ON mod_actions(guild_id)")
    db.execute("CREATE INDEX IF NOT EXISTS idx_dm_timestamp ON dm_inbox(timestamp)")
    db.execute("CREATE INDEX IF NOT EXISTS idx_activity_guild ON activity_log(guild_id)")

    db.commit()
    db.execute("VACUUM")
    db.close()

    print(f"\nMigration complete: {dst_path}")


def table_exists(db, name):
    r = db.execute("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?", (name,)).fetchone()
    return r[0] > 0


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <js_database.db> [output.db]")
        print(f"       {sys.argv[0]} <js_database.db>              # in-place (backs up first)")
        sys.exit(1)

    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else src

    if not os.path.exists(src):
        print(f"Error: {src} not found")
        sys.exit(1)

    print(f"Migrating JS database: {src}")
    print(f"Output: {dst}\n")
    migrate(src, dst)


if __name__ == "__main__":
    main()
