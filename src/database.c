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
        "interval_minutes INTEGER DEFAULT 60,"
        "message_count INTEGER DEFAULT 100,"
        "enabled INTEGER DEFAULT 1,"
        "PRIMARY KEY (guild_id, channel_id)"
        ")");

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

    return 0;
}

int db_get_guild_settings(yuno_database_t *database, uint64_t guild_id, guild_settings_t *settings) {
    sqlite3_stmt *stmt;
    char guild_str[32];
    int result;

    snprintf(guild_str, sizeof(guild_str), "%lu", (unsigned long)guild_id);

    const char *sql = "SELECT prefix, spam_filter_enabled, leveling_enabled FROM guild_settings WHERE guild_id = ?";
    result = sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        settings->guild_id = guild_id;
        strncpy(settings->prefix, (const char *)sqlite3_column_text(stmt, 0), MAX_PREFIX_LEN - 1);
        settings->spam_filter_enabled = sqlite3_column_int(stmt, 1);
        settings->leveling_enabled = sqlite3_column_int(stmt, 2);
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

    const char *sql = "INSERT OR REPLACE INTO guild_settings (guild_id, prefix, spam_filter_enabled, leveling_enabled) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, settings->prefix, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, settings->spam_filter_enabled);
    sqlite3_bind_int(stmt, 4, settings->leveling_enabled);

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

    const char *sql = "SELECT interval_minutes, message_count, enabled FROM auto_clean_config WHERE guild_id = ? AND channel_id = ?";
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
        config->enabled = sqlite3_column_int(stmt, 2);
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

    const char *sql = "INSERT OR REPLACE INTO auto_clean_config (guild_id, channel_id, interval_minutes, message_count, enabled) VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, guild_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, channel_str, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, config->interval_minutes);
    sqlite3_bind_int(stmt, 4, config->message_count);
    sqlite3_bind_int(stmt, 5, config->enabled);

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

    const char *sql = "SELECT guild_id, channel_id, interval_minutes, message_count, enabled FROM auto_clean_config WHERE enabled = 1";
    if (sqlite3_prepare_v2(database->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    *count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && *count < max_configs) {
        configs[*count].guild_id = strtoull((const char *)sqlite3_column_text(stmt, 0), NULL, 10);
        configs[*count].channel_id = strtoull((const char *)sqlite3_column_text(stmt, 1), NULL, 10);
        configs[*count].interval_minutes = sqlite3_column_int(stmt, 2);
        configs[*count].message_count = sqlite3_column_int(stmt, 3);
        configs[*count].enabled = sqlite3_column_int(stmt, 4);
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
