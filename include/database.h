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
    char join_message[1024];
    char join_message_title[256];
    int invite_filter_enabled;
    uint64_t error_channel_id;
} guild_settings_t;

/* DM forwarding config */
typedef struct {
    uint64_t guild_id;
    uint64_t channel_id;
    int enabled;
} dm_config_t;

/* Activity log channel config */
#define MAX_LOG_TYPE_LEN 32
typedef struct {
    uint64_t guild_id;
    char log_type[MAX_LOG_TYPE_LEN]; /* unified, voice, nickname, avatar, presence */
    uint64_t channel_id;
    int enabled;
} log_channel_t;

/* Activity log settings */
typedef struct {
    uint64_t guild_id;
    int flush_interval; /* seconds between log batches */
    int max_buffer_size;
} log_settings_t;

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
    int interval_minutes;   /* Hours between cleans (matches JS timeFEachClean) */
    int message_count;      /* Remaining time in minutes (matches JS remainingTime) */
    int warning_minutes;    /* Minutes before clean to send warning (matches JS timeBeforeClean) */
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
int db_set_xp_data(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int64_t xp, int level);
int db_set_level(yuno_database_t *database, uint64_t user_id, uint64_t guild_id, int level);
int db_get_leaderboard(yuno_database_t *database, uint64_t guild_id, user_xp_t *results, int max_results, int *count);

/* Mod actions */
int db_log_mod_action(yuno_database_t *database, const mod_action_t *action);
int db_get_mod_actions(yuno_database_t *database, uint64_t guild_id, mod_action_t *results, int max_results, int *count);
int db_get_mod_stats(yuno_database_t *database, uint64_t guild_id, uint64_t moderator_id, int *ban_count, int *kick_count, int *timeout_count);
int db_mod_action_exists(yuno_database_t *database, uint64_t guild_id, uint64_t target_id, const char *action_type);
int db_get_mod_stats_breakdown(yuno_database_t *database, uint64_t guild_id, int *ban_count, int *kick_count, int *timeout_count, int *unban_count);

/* Auto-clean */
int db_get_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id, auto_clean_config_t *config);
int db_set_auto_clean_config(yuno_database_t *database, const auto_clean_config_t *config);
int db_remove_auto_clean_config(yuno_database_t *database, uint64_t guild_id, uint64_t channel_id);
int db_get_all_auto_clean_configs(yuno_database_t *database, auto_clean_config_t *configs, int max_configs, int *count);
int db_get_guild_auto_clean_configs(yuno_database_t *database, uint64_t guild_id, auto_clean_config_t *configs, int max_configs, int *count);

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

/* Level-role mapping */
typedef struct {
    int level;
    uint64_t role_id;
} level_role_t;

int db_set_level_role(yuno_database_t *database, uint64_t guild_id, int level, uint64_t role_id);
int db_remove_level_role(yuno_database_t *database, uint64_t guild_id, int level);
int db_get_level_roles(yuno_database_t *database, uint64_t guild_id, level_role_t *results, int max_results, int *count);
int db_get_role_for_level(yuno_database_t *database, uint64_t guild_id, int level, uint64_t *role_id);

/* Bulk XP operations */
int db_fix_xp_data(yuno_database_t *database, uint64_t guild_id, int *scanned, int *fixed);
int db_add_xp_all_guild(yuno_database_t *database, uint64_t guild_id, int64_t amount, int *updated);
int db_set_level_all_guild(yuno_database_t *database, uint64_t guild_id, int level, int *updated);

/* DM config */
int db_get_dm_config(yuno_database_t *database, uint64_t guild_id, dm_config_t *config);
int db_set_dm_config(yuno_database_t *database, const dm_config_t *config);
int db_remove_dm_config(yuno_database_t *database, uint64_t guild_id);
int db_get_all_dm_configs(yuno_database_t *database, dm_config_t *configs, int max_configs, int *count);

/* Log channels */
int db_get_log_channel(yuno_database_t *database, uint64_t guild_id, const char *log_type, log_channel_t *config);
int db_set_log_channel(yuno_database_t *database, const log_channel_t *config);
int db_remove_log_channel(yuno_database_t *database, uint64_t guild_id, const char *log_type);
int db_get_all_log_channels(yuno_database_t *database, uint64_t guild_id, log_channel_t *channels, int max_channels, int *count);

/* Log settings */
int db_get_log_settings(yuno_database_t *database, uint64_t guild_id, log_settings_t *settings);
int db_set_log_settings(yuno_database_t *database, const log_settings_t *settings);

/* Bot presence */
typedef struct {
    int type;            /* 0=game, 1=streaming, 2=listening, 3=watching, 4=custom, 5=competing */
    char text[128];
    char status[16];     /* online, idle, dnd, invisible */
    char stream_url[256];
} bot_presence_t;

int db_get_bot_presence(yuno_database_t *database, bot_presence_t *presence);
int db_set_bot_presence(yuno_database_t *database, const bot_presence_t *presence);

/* Mention response */
typedef struct {
    int64_t id;
    uint64_t guild_id;
    uint64_t user_id;    /* User whose mention triggers the response */
    char trigger[64];    /* Trigger keyword */
    char response[512];
    char image_url[256];
} mention_response_t;

int db_add_mention_response(yuno_database_t *database, const mention_response_t *mr);
int db_remove_mention_response(yuno_database_t *database, uint64_t guild_id, const char *trigger);
int db_get_mention_responses(yuno_database_t *database, uint64_t guild_id, mention_response_t *results, int max_results, int *count);
int db_find_mention_response(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, const char *content, mention_response_t *result);

/* User role persistence (for auto-role restoration) */
int db_save_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, const uint64_t *role_ids, int role_count);
int db_get_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, uint64_t *role_ids, int max_roles, int *count);
int db_delete_user_roles(yuno_database_t *database, uint64_t guild_id, uint64_t user_id);

/* Join messages (stored in guild_settings) */
int db_set_join_message(yuno_database_t *database, uint64_t guild_id, const char *title, const char *message);
int db_get_join_message(yuno_database_t *database, uint64_t guild_id, char *title, size_t title_len, char *message, size_t msg_len);

/* Ban images */
typedef struct {
    uint64_t guild_id;
    uint64_t user_id;
    char image_url[512];
} ban_image_t;

int db_set_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, const char *url);
int db_get_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id, char *url, size_t url_len);
int db_remove_ban_image(yuno_database_t *database, uint64_t guild_id, uint64_t user_id);

/* Custom spam rules */
typedef struct {
    int64_t id;
    uint64_t guild_id;
    char pattern[256];    /* Regex or keyword pattern */
    int action;           /* 0=warn, 1=delete, 2=timeout, 3=ban */
    int enabled;
} spam_rule_t;

int db_add_spam_rule(yuno_database_t *database, const spam_rule_t *rule);
int db_remove_spam_rule(yuno_database_t *database, uint64_t guild_id, int64_t rule_id);
int db_get_spam_rules(yuno_database_t *database, uint64_t guild_id, spam_rule_t *rules, int max_rules, int *count);

#endif /* YUNO_DATABASE_H */
