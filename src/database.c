/*
 * Yuno Gasai 2 (C Edition) - Database
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int db_open(yuno_database_t *database, const char *path) {
    int result = sqlite3_open(path, &database->db);
    if (result != SQLITE_OK) {
        fprintf(stderr, "💔 Failed to open database: %s\n", sqlite3_errmsg(database->db));
        return -1;
    }

#ifdef YUNO_SQLCIPHER
    /* If compiled with SQLCipher support, apply encryption key */
    const char *db_key = getenv("YUNO_DB_KEY");
    if (db_key && db_key[0]) {
        /* Use native sqlite3_key() API to prevent SQL injection */
        int key_result = sqlite3_key(database->db, db_key, (int)strlen(db_key));
        if (key_result != SQLITE_OK) {
            fprintf(stderr, "💔 Failed to set database encryption key: %s\n",
                    sqlite3_errmsg(database->db));
            sqlite3_close(database->db);
            database->db = NULL;
            return -1;
        }
        printf("🔒 Database encryption enabled (SQLCipher)\n");
    }
#endif

    /* Enable WAL mode for better concurrent access */
    sqlite3_exec(database->db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);

    return db_initialize(database);
}

void db_close(yuno_database_t *database) {
    if (database->db) {
        sqlite3_close(database->db);
        database->db = NULL;
    }
}

static int exec_sql(yuno_database_t *database, const char *sql) {
    char *error_msg = NULL;
    int result = sqlite3_exec(database->db, sql, NULL, NULL, &error_msg);
    if (result != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", error_msg);
        sqlite3_free(error_msg);
        return -1;
    }
    return 0;
}

int db_initialize(yuno_database_t *database) {
    /* Guild settings table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS guild_settings ("
        "guild_id TEXT PRIMARY KEY,"
        "prefix TEXT DEFAULT '.',"
        "spam_filter_enabled INTEGER DEFAULT 0,"
        "leveling_enabled INTEGER DEFAULT 1"
        ")");

    /* User XP table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS user_xp ("
        "user_id TEXT NOT NULL,"
        "guild_id TEXT NOT NULL,"
        "xp INTEGER DEFAULT 0,"
        "level INTEGER DEFAULT 0,"
        "PRIMARY KEY (user_id, guild_id)"
        ")");

    /* Mod actions table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS mod_actions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "guild_id TEXT NOT NULL,"
        "moderator_id TEXT NOT NULL,"
        "target_id TEXT NOT NULL,"
        "action_type TEXT NOT NULL,"
        "reason TEXT,"
        "timestamp INTEGER NOT NULL"
        ")");

    /* Auto-clean config table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS auto_clean_config ("
        "guild_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "interval_minutes INTEGER DEFAULT 1,"
        "message_count INTEGER DEFAULT 60,"
        "warning_minutes INTEGER DEFAULT 5,"
        "enabled INTEGER DEFAULT 1,"
        "PRIMARY KEY (guild_id, channel_id)"
        ")");

    /* Migration: add warning_minutes column if missing */
    exec_sql(database, "ALTER TABLE auto_clean_config ADD COLUMN warning_minutes INTEGER DEFAULT 5");

    /* Spam warnings table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS spam_warnings ("
        "user_id TEXT NOT NULL,"
        "guild_id TEXT NOT NULL,"
        "warnings INTEGER DEFAULT 0,"
        "last_warning INTEGER,"
        "PRIMARY KEY (user_id, guild_id)"
        ")");

    /* Create indexes */
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_mod_actions_guild ON mod_actions(guild_id)");
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_mod_actions_moderator ON mod_actions(moderator_id)");
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_user_xp_guild ON user_xp(guild_id)");

    /* Voice XP config table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS voice_xp_config ("
        "guild_id TEXT PRIMARY KEY,"
        "enabled INTEGER DEFAULT 0,"
        "xp_per_minute INTEGER DEFAULT 5,"
        "min_users INTEGER DEFAULT 2,"
        "ignore_afk INTEGER DEFAULT 1"
        ")");

    /* Activity log table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS activity_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "guild_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "channel_id TEXT,"
        "event_type TEXT NOT NULL,"
        "old_content TEXT,"
        "new_content TEXT,"
        "timestamp INTEGER NOT NULL"
        ")");
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_activity_guild ON activity_log(guild_id)");
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_activity_timestamp ON activity_log(timestamp)");

    /* DM inbox table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS dm_inbox ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id TEXT NOT NULL,"
        "username TEXT,"
        "content TEXT,"
        "timestamp INTEGER NOT NULL,"
        "read_status INTEGER DEFAULT 0"
        ")");
    exec_sql(database, "CREATE INDEX IF NOT EXISTS idx_dm_timestamp ON dm_inbox(timestamp)");

    /* Bot-level bans table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS bot_bans ("
        "user_id TEXT PRIMARY KEY,"
        "banned_by TEXT,"
        "reason TEXT,"
        "timestamp INTEGER NOT NULL"
        ")");

    /* Level-role mapping table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS level_roles ("
        "guild_id TEXT NOT NULL,"
        "level INTEGER NOT NULL,"
        "role_id TEXT NOT NULL,"
        "PRIMARY KEY (guild_id, level)"
        ")");

    /* DM forwarding config table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS dm_config ("
        "guild_id TEXT PRIMARY KEY,"
        "channel_id TEXT NOT NULL,"
        "enabled INTEGER DEFAULT 1"
        ")");

    /* Log channels table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS log_channels ("
        "guild_id TEXT NOT NULL,"
        "log_type TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "enabled INTEGER DEFAULT 1,"
        "PRIMARY KEY (guild_id, log_type)"
        ")");

    /* Log settings table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS log_settings ("
        "guild_id TEXT PRIMARY KEY,"
        "flush_interval INTEGER DEFAULT 30,"
        "max_buffer_size INTEGER DEFAULT 50"
        ")");

    /* Add join message columns to guild_settings (if not exist) */
    exec_sql(database, "ALTER TABLE guild_settings ADD COLUMN join_message TEXT");
    exec_sql(database, "ALTER TABLE guild_settings ADD COLUMN join_message_title TEXT");
    exec_sql(database, "ALTER TABLE guild_settings ADD COLUMN invite_filter_enabled INTEGER DEFAULT 0");
    exec_sql(database, "ALTER TABLE guild_settings ADD COLUMN error_channel_id TEXT");

    /* Mention responses table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS mention_responses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "guild_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "trigger_word TEXT NOT NULL,"
        "response TEXT NOT NULL,"
        "image_url TEXT,"
        "UNIQUE(guild_id, trigger_word)"
        ")");

    /* Bot presence table (single row) */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS bot_presence ("
        "id INTEGER PRIMARY KEY DEFAULT 1,"
        "type INTEGER DEFAULT 0,"
        "text TEXT,"
        "status TEXT DEFAULT 'online',"
        "stream_url TEXT"
        ")");

    /* User roles persistence table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS user_roles ("
        "guild_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "role_ids TEXT,"
        "PRIMARY KEY (guild_id, user_id)"
        ")");

    /* Ban images table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS ban_images ("
        "guild_id TEXT NOT NULL,"
        "user_id TEXT NOT NULL,"
        "image_url TEXT,"
        "PRIMARY KEY (guild_id, user_id)"
        ")");

    /* Custom spam rules table */
    exec_sql(database,
        "CREATE TABLE IF NOT EXISTS spam_rules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "guild_id TEXT NOT NULL,"
        "pattern TEXT NOT NULL,"
        "action INTEGER DEFAULT 0,"
        "enabled INTEGER DEFAULT 1"
        ")");

    return 0;
}

int db_get_guild_settings(yuno_database_t *database, uint64_t guild_id, guild_settings_t *settings) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    int result;

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT prefix, spam_filter_enabled, leveling_enabled, join_message, join_message_title, invite_filter_enabled, error_channel_id FROM guild_settings WHERE guild_id = ?";
    result = sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    memset(settings, 0, sizeof(guild_settings_t));
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        settings->guild_id = guild_id;
        strncpy(settings->prefix, (const char *)sqlite3_column_text(stmt, 0), MAX_PREFIX_LEN - 1);
        settings->spam_filter_enabled = sqlite3_column_int(stmt, 1);
        settings->leveling_enabled = sqlite3_column_int(stmt, 2);
        const char *jm = (const char *)sqlite3_column_text(stmt, 3);
        if (jm) strncpy(settings->join_message, jm, sizeof(settings->join_message) - 1);
        const char *jmt = (const char *)sqlite3_column_text(stmt, 4);
        if (jmt) strncpy(settings->join_message_title, jmt, sizeof(settings->join_message_title) - 1);
        settings->invite_filter_enabled = sqlite3_column_int(stmt, 5);
        const char *err_ch = (const char *)sqlite3_column_text(stmt, 6);
        if (err_ch) settings->error_channel_id = strtoull(err_ch, NULL, 10);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_set_guild_settings(yuno_database_t *database, const guild_settings_t *settings) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)settings->guild_id);

    const char *sql = "INSERT OR REPLACE INTO guild_settings (guild_id, prefix, spam_filter_enabled, leveling_enabled, join_message, join_message_title, invite_filter_enabled, error_channel_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, settings->prefix, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, settings->spam_filter_enabled);
    sqlite3_bind_int(stmt, 4, settings->leveling_enabled);
    sqlite3_bind_text(stmt, 5, settings->join_message[0] ? settings->join_message : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, settings->join_message_title[0] ? settings->join_message_title : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, settings->invite_filter_enabled);
    char err_ch_str[32] = "";
    if (settings->error_channel_id) snprintf(err_ch_str, sizeof(err_ch_str), "%lu", (unsigned long)settings->error_channel_id);
    sqlite3_bind_text(stmt, 8, settings->error_channel_id ? err_ch_str : NULL, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_prefix(yuno_database_t *database, uint64_t guild_id, const char *default_prefix, char *out_prefix, size_t out_len) {
    guild_settings_t settings;
    if (db_get_guild_settings(database, guild_id, &settings) == 0) {
        strncpy(out_prefix, settings.prefix, out_len - 1);
    } else {
        strncpy(out_prefix, default_prefix, out_len - 1);
    }
    return 0;
}

int db_set_prefix(yuno_database_t *database, uint64_t guild_id, const char *prefix) {
    guild_settings_t settings;
    if (db_get_guild_settings(database, guild_id, &settings) != 0) {
        settings.guild_id = guild_id;
        settings.spam_filter_enabled = 0;
        settings.leveling_enabled = 1;
    }
    strncpy(settings.prefix, prefix, MAX_PREFIX_LEN - 1);
    return db_set_guild_settings(database, &settings);
}

int db_get_user_xp(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, user_xp_t *xp) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    xp->user_id = user_id;
    xp->guild_id = guild_id;
    xp->xp = 0;
    xp->level = 0;

    const char *sql = "SELECT xp, level FROM user_xp WHERE user_id = ? AND guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        xp->xp = sqlite3_column_int64(stmt, 0);
        xp->level = sqlite3_column_int(stmt, 1);
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_add_xp(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int64_t amount) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "INSERT INTO user_xp (user_id, guild_id, xp, level) VALUES (?, ?, ?, 0) "
                      "ON CONFLICT(user_id, guild_id) DO UPDATE SET xp = xp + ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, amount);
    sqlite3_bind_int64(stmt, 4, amount);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_set_xp_data(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int64_t xp, int level) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "INSERT INTO user_xp (user_id, guild_id, xp, level) VALUES (?, ?, ?, ?) "
                      "ON CONFLICT(user_id, guild_id) DO UPDATE SET xp = ?, level = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, xp);
    sqlite3_bind_int(stmt, 4, level);
    sqlite3_bind_int64(stmt, 5, xp);
    sqlite3_bind_int(stmt, 6, level);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_set_level(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int level) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "UPDATE user_xp SET level = ? WHERE user_id = ? AND guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, level);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, guild_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_leaderboard(yuno_database_t *database, uint64_t guild_id, user_xp_t *results, int max_results, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT user_id, xp, level FROM user_xp WHERE guild_id = ? ORDER BY xp DESC LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, max_results);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_results) {
        results[*count].user_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        results[*count].guild_id = guild_id;
        results[*count].xp = sqlite3_column_int64(stmt, 1);
        results[*count].level = sqlite3_column_int(stmt, 2);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_log_mod_action(yuno_database_t *database, const mod_action_t *action) {
    sqlite3_stmt *stmt;
    char guild_str[32], mod_str[32], target_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)action->guild_id);
    snprintf(mod_str, sizeof(mod_str), "%lu", (unsigned long)action->moderator_id);
    snprintf(target_str, sizeof(target_str), "%lu", (unsigned long)action->target_id);

    const char *sql = "INSERT INTO mod_actions (guild_id, moderator_id, target_id, action_type, reason, timestamp) VALUES (?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, mod_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, target_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, action->action_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, action->reason, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, action->timestamp);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_mod_actions(yuno_database_t *database, uint64_t guild_id, mod_action_t *results, int max_results, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT id, moderator_id, target_id, action_type, reason, timestamp FROM mod_actions WHERE guild_id = ? ORDER BY timestamp DESC LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, max_results);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_results) {
        results[*count].id = sqlite3_column_int64(stmt, 0);
        results[*count].guild_id = guild_id;
        results[*count].moderator_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        results[*count].target_id = strtoull((const char *)sqlite3_column_text(stmt, 2), NULL, 10);
        strncpy(results[*count].action_type, (const char *)sqlite3_column_text(stmt, 3), 31);
        const char *reason = (const char *)sqlite3_column_text(stmt, 4);
        strncpy(results[*count].reason, reason ? reason : "", MAX_REASON_LEN - 1);
        results[*count].timestamp = sqlite3_column_int64(stmt, 5);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_get_mod_stats(yuno_database_t *database, uint64_t guild_id, uint64_t moderator_id, int *ban_count, int *kick_count, int *timeout_count) {
    sqlite3_stmt *stmt;
    char guild_str[32], mod_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(mod_str, sizeof(mod_str), "%lu", (unsigned long)moderator_id);

    *ban_count = 0;
    *kick_count = 0;
    *timeout_count = 0;

    const char *sql = "SELECT action_type, COUNT(*) FROM mod_actions WHERE guild_id = ? AND moderator_id = ? GROUP BY action_type";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, mod_str, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *type = (const char *)sqlite3_column_text(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);
        if (strcmp(type, "ban") == 0) *ban_count = count;
        else if (strcmp(type, "kick") == 0) *kick_count = count;
        else if (strcmp(type, "timeout") == 0) *timeout_count = count;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_get_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id, auto_clean_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32], channel_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)channel_id);

    const char *sql = "SELECT interval_minutes, message_count, warning_minutes, enabled FROM auto_clean_config WHERE guild_id = ? AND channel_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, channel_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        config->guild_id = guild_id;
        config->channel_id = channel_id;
        config->interval_minutes = sqlite3_column_int(stmt, 0);
        config->message_count = sqlite3_column_int(stmt, 1);
        config->warning_minutes = sqlite3_column_int(stmt, 2);
        config->enabled = sqlite3_column_int(stmt, 3);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

int db_set_auto_clean_config(yuno_database_t *database, const auto_clean_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32], channel_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)config->guild_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)config->channel_id);

    const char *sql = "INSERT OR REPLACE INTO auto_clean_config (guild_id, channel_id, interval_minutes, message_count, warning_minutes, enabled) VALUES (?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, channel_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, config->interval_minutes);
    sqlite3_bind_int(stmt, 4, config->message_count);
    sqlite3_bind_int(stmt, 5, config->warning_minutes);
    sqlite3_bind_int(stmt, 6, config->enabled);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id) {
    sqlite3_stmt *stmt;
    char guild_str[32], channel_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)channel_id);

    const char *sql = "DELETE FROM auto_clean_config WHERE guild_id = ? AND channel_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, channel_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_all_auto_clean_configs(yuno_database_t *database, auto_clean_config_t *configs, int max_configs, int *count) {
    sqlite3_stmt *stmt;

    const char *sql = "SELECT guild_id, channel_id, interval_minutes, message_count, warning_minutes, enabled FROM auto_clean_config WHERE enabled = 1";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_configs) {
        configs[*count].guild_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        configs[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        configs[*count].interval_minutes = sqlite3_column_int(stmt, 2);
        configs[*count].message_count = sqlite3_column_int(stmt, 3);
        configs[*count].warning_minutes = sqlite3_column_int(stmt, 4);
        configs[*count].enabled = sqlite3_column_int(stmt, 5);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_get_guild_auto_clean_configs(yuno_database_t *database, uint64_t guild_id, auto_clean_config_t *configs, int max_configs, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT guild_id, channel_id, interval_minutes, message_count, warning_minutes, enabled FROM auto_clean_config WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_configs) {
        configs[*count].guild_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        configs[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        configs[*count].interval_minutes = sqlite3_column_int(stmt, 2);
        configs[*count].message_count = sqlite3_column_int(stmt, 3);
        configs[*count].warning_minutes = sqlite3_column_int(stmt, 4);
        configs[*count].enabled = sqlite3_column_int(stmt, 5);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_add_spam_warning(yuno_database_t *database, uint64_t user_id, uint64_t guild_id) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];
    time_t now = time(NULL);

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "INSERT INTO spam_warnings (user_id, guild_id, warnings, last_warning) VALUES (?, ?, 1, ?) "
                      "ON CONFLICT(user_id, guild_id) DO UPDATE SET warnings = warnings + 1, last_warning = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_int64(stmt, 4, now);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_spam_warnings(yuno_database_t *database, uint64_t user_id, uint64_t guild_id) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];
    int warnings = 0;

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT warnings FROM spam_warnings WHERE user_id = ? AND guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        warnings = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return warnings;
}

int db_reset_spam_warnings(yuno_database_t *database, uint64_t user_id, uint64_t guild_id) {
    sqlite3_stmt *stmt;
    char user_str[32], guild_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM spam_warnings WHERE user_id = ? AND guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

/* Voice XP config */
int db_get_voice_xp_config(yuno_database_t *database, uint64_t guild_id, voice_xp_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    config->guild_id = guild_id;
    config->enabled = 0;
    config->xp_per_minute = 5;
    config->min_users = 2;
    config->ignore_afk = 1;

    const char *sql = "SELECT enabled, xp_per_minute, min_users, ignore_afk FROM voice_xp_config WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        config->enabled = sqlite3_column_int(stmt, 0);
        config->xp_per_minute = sqlite3_column_int(stmt, 1);
        config->min_users = sqlite3_column_int(stmt, 2);
        config->ignore_afk = sqlite3_column_int(stmt, 3);
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_set_voice_xp_config(yuno_database_t *database, const voice_xp_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)config->guild_id);

    const char *sql = "INSERT OR REPLACE INTO voice_xp_config (guild_id, enabled, xp_per_minute, min_users, ignore_afk) VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, config->enabled);
    sqlite3_bind_int(stmt, 3, config->xp_per_minute);
    sqlite3_bind_int(stmt, 4, config->min_users);
    sqlite3_bind_int(stmt, 5, config->ignore_afk);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

/* Activity logging */
int db_log_activity(yuno_database_t *database, const activity_log_t *log) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32], channel_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)log->guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)log->user_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)log->channel_id);

    const char *sql = "INSERT INTO activity_log (guild_id, user_id, channel_id, event_type, old_content, new_content, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, channel_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, log->event_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, log->old_content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, log->new_content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 7, log->timestamp);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_activity_logs(yuno_database_t *database, uint64_t guild_id, activity_log_t *logs, int max_logs, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT id, user_id, channel_id, event_type, old_content, new_content, timestamp FROM activity_log WHERE guild_id = ? ORDER BY timestamp DESC LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, max_logs);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_logs) {
        logs[*count].id = sqlite3_column_int64(stmt, 0);
        logs[*count].guild_id = guild_id;
        logs[*count].user_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        logs[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 2), NULL, 10);
        strncpy(logs[*count].event_type, (const char *)sqlite3_column_text(stmt, 3), 31);
        const char *old = (const char *)sqlite3_column_text(stmt, 4);
        const char *new = (const char *)sqlite3_column_text(stmt, 5);
        strncpy(logs[*count].old_content, old ? old : "", 1023);
        strncpy(logs[*count].new_content, new ? new : "", 1023);
        logs[*count].timestamp = sqlite3_column_int64(stmt, 6);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

/* DM inbox */
int db_save_dm(yuno_database_t *database, const dm_inbox_t *dm) {
    sqlite3_stmt *stmt;
    char user_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)dm->user_id);

    const char *sql = "INSERT INTO dm_inbox (user_id, username, content, timestamp, read_status) VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, dm->username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, dm->content, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, dm->timestamp);
    sqlite3_bind_int(stmt, 5, dm->read_status);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_dms(yuno_database_t *database, dm_inbox_t *dms, int max_dms, int *count) {
    sqlite3_stmt *stmt;

    const char *sql = "SELECT id, user_id, username, content, timestamp, read_status FROM dm_inbox ORDER BY timestamp DESC LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, max_dms);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_dms) {
        dms[*count].id = sqlite3_column_int64(stmt, 0);
        dms[*count].user_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        const char *username = (const char *)sqlite3_column_text(stmt, 2);
        strncpy(dms[*count].username, username ? username : "", 63);
        const char *content = (const char *)sqlite3_column_text(stmt, 3);
        strncpy(dms[*count].content, content ? content : "", 1999);
        dms[*count].timestamp = sqlite3_column_int64(stmt, 4);
        dms[*count].read_status = sqlite3_column_int(stmt, 5);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_mark_dm_read(yuno_database_t *database, int64_t dm_id) {
    sqlite3_stmt *stmt;

    const char *sql = "UPDATE dm_inbox SET read_status = 1 WHERE id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, dm_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_unread_dm_count(yuno_database_t *database) {
    sqlite3_stmt *stmt;
    int count = 0;

    const char *sql = "SELECT COUNT(*) FROM dm_inbox WHERE read_status = 0";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

/* Bot-level bans */
int db_add_bot_ban(yuno_database_t *database, const bot_ban_t *ban) {
    sqlite3_stmt *stmt;
    char user_str[32], banned_by_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)ban->user_id);
    snprintf(banned_by_str, sizeof(banned_by_str), "%lu", (unsigned long)ban->banned_by);

    const char *sql = "INSERT OR REPLACE INTO bot_bans (user_id, banned_by, reason, timestamp) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, banned_by_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ban->reason, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, ban->timestamp);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_bot_ban(yuno_database_t *database, uint64_t user_id) {
    sqlite3_stmt *stmt;
    char user_str[32];

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "DELETE FROM bot_bans WHERE user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_is_bot_banned(yuno_database_t *database, uint64_t user_id) {
    sqlite3_stmt *stmt;
    char user_str[32];
    int banned = 0;

    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "SELECT 1 FROM bot_bans WHERE user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, user_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        banned = 1;
    }

    sqlite3_finalize(stmt);
    return banned;
}

int db_get_bot_bans(yuno_database_t *database, bot_ban_t *bans, int max_bans, int *count) {
    sqlite3_stmt *stmt;

    const char *sql = "SELECT user_id, banned_by, reason, timestamp FROM bot_bans ORDER BY timestamp DESC LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, max_bans);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_bans) {
        bans[*count].user_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        bans[*count].banned_by = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        const char *reason = (const char *)sqlite3_column_text(stmt, 2);
        strncpy(bans[*count].reason, reason ? reason : "", MAX_REASON_LEN - 1);
        bans[*count].timestamp = sqlite3_column_int64(stmt, 3);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_mod_action_exists(yuno_database_t *database, uint64_t guild_id, uint64_t target_id, const char *action_type) {
    sqlite3_stmt *stmt;
    char guild_str[32], target_str[32];
    int exists = 0;

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(target_str, sizeof(target_str), "%lu", (unsigned long)target_id);

    const char *sql = "SELECT 1 FROM mod_actions WHERE guild_id = ? AND target_id = ? AND action_type = ? LIMIT 1";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, target_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, action_type, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = 1;
    }

    sqlite3_finalize(stmt);
    return exists;
}

int db_get_mod_stats_breakdown(yuno_database_t *database, uint64_t guild_id, int *ban_count, int *kick_count, int *timeout_count, int *unban_count) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    *ban_count = 0;
    *kick_count = 0;
    *timeout_count = 0;
    *unban_count = 0;

    const char *sql = "SELECT action_type, COUNT(*) FROM mod_actions WHERE guild_id = ? GROUP BY action_type";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *type = (const char *)sqlite3_column_text(stmt, 0);
        int count = sqlite3_column_int(stmt, 1);
        if (strcmp(type, "ban") == 0) *ban_count = count;
        else if (strcmp(type, "kick") == 0) *kick_count = count;
        else if (strcmp(type, "timeout") == 0) *timeout_count = count;
        else if (strcmp(type, "unban") == 0) *unban_count = count;
    }

    sqlite3_finalize(stmt);
    return 0;
}

/* Level-role mapping */
int db_set_level_role(yuno_database_t *database, uint64_t guild_id, int level, uint64_t role_id) {
    sqlite3_stmt *stmt;
    char guild_str[32], role_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(role_str, sizeof(role_str), "%lu", (unsigned long)role_id);

    const char *sql = "INSERT OR REPLACE INTO level_roles (guild_id, level, role_id) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, level);
    sqlite3_bind_text(stmt, 3, role_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_level_role(yuno_database_t *database, uint64_t guild_id, int level) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM level_roles WHERE guild_id = ? AND level = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, level);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_level_roles(yuno_database_t *database, uint64_t guild_id, level_role_t *results, int max_results, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT level, role_id FROM level_roles WHERE guild_id = ? ORDER BY level ASC";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_results) {
        results[*count].level = sqlite3_column_int(stmt, 0);
        results[*count].role_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_get_role_for_level(yuno_database_t *database, uint64_t guild_id, int level, uint64_t *role_id) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT role_id FROM level_roles WHERE guild_id = ? AND level = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, level);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *role_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1; /* No mapping found */
}

/* Bulk XP operations */
int db_fix_xp_data(yuno_database_t *database, uint64_t guild_id, int *scanned, int *fixed) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    *scanned = 0;
    *fixed = 0;

    /* Find all users in guild */
    const char *sql = "SELECT user_id, xp, level FROM user_xp WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    /* Collect corrupted records to fix after iteration */
    typedef struct { uint64_t user_id; int level; } fix_entry_t;
    fix_entry_t to_fix[1000];
    int fix_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        (*scanned)++;
        uint64_t user_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        int64_t xp = sqlite3_column_int64(stmt, 1);
        int level = sqlite3_column_int(stmt, 2);

        int64_t needed = 5 * (int64_t)level * level + 50 * level + 100;
        if (xp >= needed && fix_count < 1000) {
            to_fix[fix_count].user_id = user_id;
            to_fix[fix_count].level = level;
            fix_count++;
        }
    }
    sqlite3_finalize(stmt);

    /* Fix corrupted records: set XP to 0 at current level */
    for (int i = 0; i < fix_count; i++) {
        db_set_xp_data(database, to_fix[i].user_id, guild_id, 0, to_fix[i].level);
        (*fixed)++;
    }

    return 0;
}

int db_add_xp_all_guild(yuno_database_t *database, uint64_t guild_id, int64_t amount, int *updated) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    *updated = 0;

    const char *sql = "UPDATE user_xp SET xp = xp + ? WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int64(stmt, 1, amount);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    *updated = sqlite3_changes(database->db);
    sqlite3_finalize(stmt);
    return 0;
}

int db_set_level_all_guild(yuno_database_t *database, uint64_t guild_id, int level, int *updated) {
    sqlite3_stmt *stmt;
    char guild_str[32];

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    *updated = 0;

    const char *sql = "UPDATE user_xp SET level = ?, xp = 0 WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, level);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    *updated = sqlite3_changes(database->db);
    sqlite3_finalize(stmt);
    return 0;
}

/* DM config */
int db_get_dm_config(yuno_database_t *database, uint64_t guild_id, dm_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT channel_id, enabled FROM dm_config WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        config->guild_id = guild_id;
        config->channel_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        config->enabled = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return -1;
}

int db_set_dm_config(yuno_database_t *database, const dm_config_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32], channel_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)config->guild_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)config->channel_id);

    const char *sql = "INSERT OR REPLACE INTO dm_config (guild_id, channel_id, enabled) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, channel_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, config->enabled);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_dm_config(yuno_database_t *database, uint64_t guild_id) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM dm_config WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_all_dm_configs(yuno_database_t *database, dm_config_t *configs, int max_configs, int *count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT guild_id, channel_id, enabled FROM dm_config WHERE enabled = 1";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_configs) {
        configs[*count].guild_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        configs[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        configs[*count].enabled = sqlite3_column_int(stmt, 2);
        (*count)++;
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* Log channels */
int db_get_log_channel(yuno_database_t *database, uint64_t guild_id, const char *log_type, log_channel_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT channel_id, enabled FROM log_channels WHERE guild_id = ? AND log_type = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, log_type, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        config->guild_id = guild_id;
        strncpy(config->log_type, log_type, MAX_LOG_TYPE_LEN - 1);
        config->channel_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        config->enabled = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return -1;
}

int db_set_log_channel(yuno_database_t *database, const log_channel_t *config) {
    sqlite3_stmt *stmt;
    char guild_str[32], channel_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)config->guild_id);
    snprintf(channel_str, sizeof(channel_str), "%lu", (unsigned long)config->channel_id);

    const char *sql = "INSERT OR REPLACE INTO log_channels (guild_id, log_type, channel_id, enabled) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, config->log_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, channel_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, config->enabled);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_log_channel(yuno_database_t *database, uint64_t guild_id, const char *log_type) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM log_channels WHERE guild_id = ? AND log_type = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, log_type, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_all_log_channels(yuno_database_t *database, uint64_t guild_id, log_channel_t *channels, int max_channels, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT log_type, channel_id, enabled FROM log_channels WHERE guild_id = ? ORDER BY log_type";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_channels) {
        channels[*count].guild_id = guild_id;
        strncpy(channels[*count].log_type, (const char *)sqlite3_column_text(stmt, 0), MAX_LOG_TYPE_LEN - 1);
        channels[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        channels[*count].enabled = sqlite3_column_int(stmt, 2);
        (*count)++;
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* Log settings */
int db_get_log_settings(yuno_database_t *database, uint64_t guild_id, log_settings_t *settings) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    settings->guild_id = guild_id;
    settings->flush_interval = 30;
    settings->max_buffer_size = 50;

    const char *sql = "SELECT flush_interval, max_buffer_size FROM log_settings WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        settings->flush_interval = sqlite3_column_int(stmt, 0);
        settings->max_buffer_size = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return 0;
}

int db_set_log_settings(yuno_database_t *database, const log_settings_t *settings) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)settings->guild_id);

    const char *sql = "INSERT OR REPLACE INTO log_settings (guild_id, flush_interval, max_buffer_size) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, settings->flush_interval);
    sqlite3_bind_int(stmt, 3, settings->max_buffer_size);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

/* Join messages */
int db_set_join_message(yuno_database_t *database, uint64_t guild_id, const char *title, const char *message) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "UPDATE guild_settings SET join_message = ?, join_message_title = ? WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, message, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(database->db) == 0) {
        const char *isql = "INSERT INTO guild_settings (guild_id, prefix, spam_filter_enabled, leveling_enabled, join_message, join_message_title) VALUES (?, '.', 0, 1, ?, ?)";
        if (sqlite3_prepare_v2(database->db, isql, -1, &stmt, NULL) != SQLITE_OK) return -1;
        sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, message, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, title, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return 0;
}

int db_get_join_message(yuno_database_t *database, uint64_t guild_id, char *title, size_t title_len, char *message, size_t msg_len) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    title[0] = '\0';
    message[0] = '\0';

    const char *sql = "SELECT join_message_title, join_message FROM guild_settings WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *t = (const char *)sqlite3_column_text(stmt, 0);
        const char *m = (const char *)sqlite3_column_text(stmt, 1);
        if (t) strncpy(title, t, title_len - 1);
        if (m) strncpy(message, m, msg_len - 1);
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* User role persistence */
int db_save_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id,
                        const uint64_t *role_ids, int role_count) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    /* Build comma-separated role ID string */
    char roles_str[2048] = "";
    char *ptr = roles_str;
    for (int i = 0; i < role_count; i++) {
        if (i > 0) *ptr++ = ',';
        ptr += snprintf(ptr, (size_t)(roles_str + sizeof(roles_str) - ptr),
                          "%lu", (unsigned long)role_ids[i]);
    }

    const char *sql = "INSERT OR REPLACE INTO user_roles (guild_id, user_id, role_ids) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, roles_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id,
                       uint64_t *role_ids, int max_roles, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    *count = 0;

    const char *sql = "SELECT role_ids FROM user_roles WHERE guild_id = ? AND user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *roles_str = (const char *)sqlite3_column_text(stmt, 0);
        if (roles_str && roles_str[0]) {
            /* Parse comma-separated role IDs */
            char buf[2048];
            strncpy(buf, roles_str, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            char *tok = strtok(buf, ",");
            while (tok && *count < max_roles) {
                role_ids[*count] = strtoull(tok, NULL, 10);
                if (role_ids[*count] != 0) (*count)++;
                tok = strtok(NULL, ",");
            }
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

int db_delete_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "DELETE FROM user_roles WHERE guild_id = ? AND user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

/* Mention responses */
int db_add_mention_response(yuno_database_t *database, const mention_response_t *mr) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)mr->guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)mr->user_id);

    const char *sql = "INSERT OR REPLACE INTO mention_responses (guild_id, user_id, trigger_word, response, image_url) VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, mr->trigger, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, mr->response, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, mr->image_url[0] ? mr->image_url : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_mention_response(yuno_database_t *database, uint64_t guild_id, const char *trigger) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM mention_responses WHERE guild_id = ? AND trigger_word = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, trigger, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    int deleted = sqlite3_changes(database->db);
    sqlite3_finalize(stmt);
    return deleted > 0 ? 0 : -1;
}

int db_get_mention_responses(yuno_database_t *database, uint64_t guild_id, mention_response_t *results, int max_results, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    *count = 0;

    const char *sql = "SELECT id, user_id, trigger_word, response, image_url FROM mention_responses WHERE guild_id = ? LIMIT ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, max_results);

    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_results) {
        mention_response_t *r = &results[*count];
        memset(r, 0, sizeof(mention_response_t));
        r->id = sqlite3_column_int64(stmt, 0);
        r->guild_id = guild_id;
        r->user_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        const char *t = (const char *)sqlite3_column_text(stmt, 2);
        if (t) strncpy(r->trigger, t, sizeof(r->trigger) - 1);
        const char *resp = (const char *)sqlite3_column_text(stmt, 3);
        if (resp) strncpy(r->response, resp, sizeof(r->response) - 1);
        const char *img = (const char *)sqlite3_column_text(stmt, 4);
        if (img) strncpy(r->image_url, img, sizeof(r->image_url) - 1);
        (*count)++;
    }
    sqlite3_finalize(stmt);
    return 0;
}

int db_find_mention_response(yuno_database_t *database, uint64_t guild_id, uint64_t user_id,
                              const char *content, mention_response_t *result) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    /* Find responses where the trigger matches and the user is mentioned */
    const char *sql = "SELECT id, user_id, trigger_word, response, image_url FROM mention_responses WHERE guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *trigger = (const char *)sqlite3_column_text(stmt, 2);
        uint64_t mr_user = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);

        /* Check if the message mentions the configured user and contains the trigger */
        char mention_str[32];
        snprintf(mention_str, sizeof(mention_str), "<@%lu>", (unsigned long)mr_user);
        char mention_str2[32];
        snprintf(mention_str2, sizeof(mention_str2), "<@!%lu>", (unsigned long)mr_user);

        if ((strstr(content, mention_str) || strstr(content, mention_str2)) &&
            strstr(content, trigger)) {
            memset(result, 0, sizeof(mention_response_t));
            result->id = sqlite3_column_int64(stmt, 0);
            result->guild_id = guild_id;
            result->user_id = mr_user;
            strncpy(result->trigger, trigger, sizeof(result->trigger) - 1);
            const char *resp = (const char *)sqlite3_column_text(stmt, 3);
            if (resp) strncpy(result->response, resp, sizeof(result->response) - 1);
            const char *img = (const char *)sqlite3_column_text(stmt, 4);
            if (img) strncpy(result->image_url, img, sizeof(result->image_url) - 1);
            sqlite3_finalize(stmt);
            return 0;
        }
    }
    sqlite3_finalize(stmt);
    return -1;
}

/* Bot presence */
int db_get_bot_presence(yuno_database_t *database, bot_presence_t *presence) {
    sqlite3_stmt *stmt;
    memset(presence, 0, sizeof(bot_presence_t));
    strncpy(presence->status, "online", sizeof(presence->status) - 1);

    const char *sql = "SELECT type, text, status, stream_url FROM bot_presence WHERE id = 1";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        presence->type = sqlite3_column_int(stmt, 0);
        const char *text = (const char *)sqlite3_column_text(stmt, 1);
        if (text) strncpy(presence->text, text, sizeof(presence->text) - 1);
        const char *status = (const char *)sqlite3_column_text(stmt, 2);
        if (status) strncpy(presence->status, status, sizeof(presence->status) - 1);
        const char *url = (const char *)sqlite3_column_text(stmt, 3);
        if (url) strncpy(presence->stream_url, url, sizeof(presence->stream_url) - 1);
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return -1;
}

int db_set_bot_presence(yuno_database_t *database, const bot_presence_t *presence) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR REPLACE INTO bot_presence (id, type, text, status, stream_url) VALUES (1, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, presence->type);
    sqlite3_bind_text(stmt, 2, presence->text[0] ? presence->text : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, presence->status, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, presence->stream_url[0] ? presence->stream_url : NULL, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

/* ===== Ban Images ===== */

int db_set_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, const char *url) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "INSERT OR REPLACE INTO ban_images (guild_id, user_id, image_url) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, url, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, char *url, size_t url_len) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "SELECT image_url FROM ban_images WHERE guild_id = ? AND user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *img = (const char *)sqlite3_column_text(stmt, 0);
        if (img) strncpy(url, img, url_len - 1);
        url[url_len - 1] = '\0';
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return -1;
}

int db_remove_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id) {
    sqlite3_stmt *stmt;
    char guild_str[32], user_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);

    const char *sql = "DELETE FROM ban_images WHERE guild_id = ? AND user_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(database->db) > 0 ? 0 : -1;
}

/* ===== Custom Spam Rules ===== */

int db_add_spam_rule(yuno_database_t *database, const spam_rule_t *rule) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)rule->guild_id);

    const char *sql = "INSERT INTO spam_rules (guild_id, pattern, action, enabled) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, rule->pattern, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, rule->action);
    sqlite3_bind_int(stmt, 4, rule->enabled);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_remove_spam_rule(yuno_database_t *database, uint64_t guild_id, int64_t rule_id) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "DELETE FROM spam_rules WHERE id = ? AND guild_id = ?";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int64(stmt, 1, rule_id);
    sqlite3_bind_text(stmt, 2, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_changes(database->db) > 0 ? 0 : -1;
}

int db_get_spam_rules(yuno_database_t *database, uint64_t guild_id, spam_rule_t *rules, int max_rules, int *count) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);
    *count = 0;

    const char *sql = "SELECT id, guild_id, pattern, action, enabled FROM spam_rules WHERE guild_id = ? ORDER BY id";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_rules) {
        spam_rule_t *r = &rules[*count];
        r->id = sqlite3_column_int64(stmt, 0);
        r->guild_id = guild_id;
        const char *pat = (const char *)sqlite3_column_text(stmt, 2);
        if (pat) strncpy(r->pattern, pat, sizeof(r->pattern) - 1);
        r->action = sqlite3_column_int(stmt, 3);
        r->enabled = sqlite3_column_int(stmt, 4);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return 0;
}
