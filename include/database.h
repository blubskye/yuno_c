/*
 * Yuno Gasai 2 (C Edition) - Database
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_DATABASE_H
#define YUNO_DATABASE_H

#include <stdint.h>
#include <stddef.h>
#include <sqlite3.h>

#define MAX_REASON_LEN 512
#define MAX_PREFIX_LEN 16

typedef struct {
    uint64_t guild_id;
    char prefix[MAX_PREFIX_LEN];
    int spam_filter_enabled;
    int leveling_enabled;
} guild_settings_t;

typedef struct {
    uint64_t user_id;
    uint64_t guild_id;
    int64_t xp;
    int level;
} user_xp_t;

typedef struct {
    int64_t id;
    uint64_t guild_id;
    uint64_t moderator_id;
    uint64_t target_id;
    char action_type[32];
    char reason[MAX_REASON_LEN];
    int64_t timestamp;
} mod_action_t;

typedef struct {
    uint64_t guild_id;
    uint64_t channel_id;
    int interval_minutes;
    int message_count;
    int enabled;
} auto_clean_config_t;

/* Voice XP configuration */
typedef struct {
    uint64_t guild_id;
    int enabled;
    int xp_per_minute;
    int min_users;
    int ignore_afk;
} voice_xp_config_t;

/* Activity log entry */
typedef struct {
    int64_t id;
    uint64_t guild_id;
    uint64_t user_id;
    uint64_t channel_id;
    char event_type[32];      /* message_edit, message_delete, voice_join, voice_leave, etc. */
    char old_content[1024];
    char new_content[1024];
    int64_t timestamp;
} activity_log_t;

/* DM inbox entry */
typedef struct {
    int64_t id;
    uint64_t user_id;
    char username[64];
    char content[2000];
    int64_t timestamp;
    int read_status;
} dm_inbox_t;

/* Bot-level ban */
typedef struct {
    uint64_t user_id;
    uint64_t banned_by;
    char reason[MAX_REASON_LEN];
    int64_t timestamp;
} bot_ban_t;

/* XP batch entry for batching */
typedef struct {
    uint64_t user_id;
    uint64_t guild_id;
    uint64_t channel_id;
    int64_t xp_amount;
    int64_t added_at;
} pending_xp_t;

typedef struct {
    sqlite3 *db;
} yuno_database_t;

/* Database lifecycle */
int db_open(yuno_database_t *database, const char *path);
void db_close(yuno_database_t *database);
int db_initialize(yuno_database_t *database);

/* Guild settings */
int db_get_guild_settings(yuno_database_t *database, uint64_t guild_id, guild_settings_t *settings);
int db_set_guild_settings(yuno_database_t *database, const guild_settings_t *settings);
int db_get_prefix(yuno_database_t *database, uint64_t guild_id, const char *default_prefix, char *out_prefix, size_t out_len);
int db_set_prefix(yuno_database_t *database, uint64_t guild_id, const char *prefix);

/* XP/Leveling */
int db_get_user_xp(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, user_xp_t *xp);
int db_add_xp(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int64_t amount);
int db_set_level(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int level);
int db_get_leaderboard(yuno_database_t *database, uint64_t guild_id, user_xp_t *results, int max_results, int *count);

/* Mod actions */
int db_log_mod_action(yuno_database_t *database, const mod_action_t *action);
int db_get_mod_actions(yuno_database_t *database, uint64_t guild_id, mod_action_t *results, int max_results, int *count);
int db_get_mod_stats(yuno_database_t *database, uint64_t guild_id, uint64_t moderator_id, int *ban_count, int *kick_count, int *timeout_count);

/* Auto-clean */
int db_get_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id, auto_clean_config_t *config);
int db_set_auto_clean_config(yuno_database_t *database, const auto_clean_config_t *config);
int db_remove_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id);
int db_get_all_auto_clean_configs(yuno_database_t *database, auto_clean_config_t *configs, int max_configs, int *count);

/* Spam filter */
int db_add_spam_warning(yuno_database_t *database, uint64_t user_id, uint64_t guild_id);
int db_get_spam_warnings(yuno_database_t *database, uint64_t user_id, uint64_t guild_id);
int db_reset_spam_warnings(yuno_database_t *database, uint64_t user_id, uint64_t guild_id);

/* Voice XP */
int db_get_voice_xp_config(yuno_database_t *database, uint64_t guild_id, voice_xp_config_t *config);
int db_set_voice_xp_config(yuno_database_t *database, const voice_xp_config_t *config);

/* Activity logging */
int db_log_activity(yuno_database_t *database, const activity_log_t *log);
int db_get_activity_logs(yuno_database_t *database, uint64_t guild_id, activity_log_t *logs, int max_logs, int *count);

/* DM inbox */
int db_save_dm(yuno_database_t *database, const dm_inbox_t *dm);
int db_get_dms(yuno_database_t *database, dm_inbox_t *dms, int max_dms, int *count);
int db_mark_dm_read(yuno_database_t *database, int64_t dm_id);
int db_get_unread_dm_count(yuno_database_t *database);

/* Bot-level bans */
int db_add_bot_ban(yuno_database_t *database, const bot_ban_t *ban);
int db_remove_bot_ban(yuno_database_t *database, uint64_t user_id);
int db_is_bot_banned(yuno_database_t *database, uint64_t user_id);
int db_get_bot_bans(yuno_database_t *database, bot_ban_t *bans, int max_bans, int *count);

#endif /* YUNO_DATABASE_H */
