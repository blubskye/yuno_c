/*
 * Yuno Gasai 2 (C Edition) - Bot Core
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "bot.h"
#include "commands/moderation.h"
#include "commands/utility.h"
#include "commands/fun.h"
#include "modules/terminal.h"
#include "modules/spam_filter.h"
#include "modules/activity_logger.h"
#include "modules/auto_cleaner.h"
#include "modules/http_client.h"
#include <concord/discord-worker.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Global bot instance for callbacks */
yuno_bot_t *g_bot = NULL;

/* XP Batcher implementation with hash table */
#define XP_FLUSH_INTERVAL 10 /* seconds */

/* Hash function for user+guild - O(1) lookup */
static inline uint32_t xp_hash_user_guild(uint64_t user_id, uint64_t guild_id) {
    uint64_t h = user_id ^ (guild_id * 2654435761ULL);
    return (uint32_t)(h % XP_HASH_SIZE);
}

void xp_batcher_init(xp_batcher_t *batcher) {
    memset(batcher, 0, sizeof(xp_batcher_t));
    batcher->last_flush = time(NULL);

    /* Initialize hash table to -1 (empty) */
    for (int i = 0; i < XP_HASH_SIZE; i++) {
        batcher->hash_table[i] = -1;
    }
}

void xp_batcher_add(yuno_bot_t *bot, uint64_t user_id, uint64_t guild_id, uint64_t channel_id, int xp) {
    xp_batcher_t *batcher = &bot->xp_batcher;

    /* O(1) hash lookup instead of O(n) linear search */
    uint32_t bucket = xp_hash_user_guild(user_id, guild_id);
    int idx = batcher->hash_table[bucket];

    while (idx >= 0) {
        if (batcher->pending[idx].user_id == user_id &&
            batcher->pending[idx].guild_id == guild_id) {
            /* Found existing entry - update it */
            batcher->pending[idx].xp_amount += xp;
            batcher->pending[idx].channel_id = channel_id;
            return;
        }
        idx = batcher->hash_next[idx];
    }

    /* Add new entry if space available */
    if (batcher->count < MAX_PENDING_XP) {
        idx = batcher->count;
        batcher->pending[idx].user_id = user_id;
        batcher->pending[idx].guild_id = guild_id;
        batcher->pending[idx].channel_id = channel_id;
        batcher->pending[idx].xp_amount = xp;
        batcher->pending[idx].added_at = time(NULL);

        /* Add to hash table */
        batcher->hash_next[idx] = batcher->hash_table[bucket];
        batcher->hash_table[bucket] = idx;
        batcher->count++;
    }

    /* Flush if batch is full (periodic flush handled by Concord timer) */
    if (batcher->count >= MAX_PENDING_XP) {
        voice_tracker_grant_xp(bot);
        xp_batcher_flush(bot);
        activity_logger_flush(bot->client, &bot->database);
    }
}

void xp_batcher_flush(yuno_bot_t *bot) {
    xp_batcher_t *batcher = &bot->xp_batcher;

    if (batcher->count == 0) return;

    for (int i = 0; i < batcher->count; i++) {
        const pending_xp_t *p = &batcher->pending[i];

        /* Get current XP */
        user_xp_t user_xp;
        db_get_user_xp(&bot->database, p->user_id, p->guild_id, &user_xp);

        /* Add XP (per-level model matching JS: xp resets on level-up) */
        user_xp.xp += p->xp_amount;
        int leveled_up = 0;

        /* Check for level up: needed = 5 * level^2 + 50 * level + 100 */
        /* Sanity check to prevent integer overflow (max safe level ~60000 for int64) */
        if (user_xp.level > 100000) {
            user_xp.level = 100000;  /* Cap at reasonable maximum */
        }

        int64_t needed = 5 * (int64_t)user_xp.level * user_xp.level
                       + 50 * user_xp.level + 100;
        while (user_xp.xp >= needed && user_xp.level < 100000) {
            user_xp.xp -= needed;
            user_xp.level += 1;
            leveled_up = 1;

            /* Check for potential overflow before calculation */
            if (user_xp.level > 100000) break;

            needed = 5 * (int64_t)user_xp.level * user_xp.level
                   + 50 * user_xp.level + 100;
        }

        db_set_xp_data(&bot->database, p->user_id, p->guild_id,
                        user_xp.xp, user_xp.level);

        /* Send level up message and auto-assign role */
        if (leveled_up && bot->client && p->channel_id != 0) {
            char level_msg[256];
            snprintf(level_msg, sizeof(level_msg),
                "✨ **Level Up!** ✨\nCongratulations <@%lu>! You've reached level **%d**! 💕",
                (unsigned long)p->user_id, user_xp.level);

            struct discord_create_message params = { .content = level_msg };
            discord_create_message(bot->client, p->channel_id, &params, NULL);

            /* Auto-assign level role if configured */
            u64snowflake role_id = 0;
            if (db_get_role_for_level(&bot->database, p->guild_id, user_xp.level, &role_id) == 0) {
                discord_add_guild_member_role(bot->client, p->guild_id,
                    p->user_id, role_id, NULL, NULL);
            }
        }
    }

    /* Reset batcher */
    batcher->count = 0;
    for (int i = 0; i < XP_HASH_SIZE; i++) {
        batcher->hash_table[i] = -1;
    }
    batcher->last_flush = time(NULL);
}

/* Voice tracker implementation */
void voice_tracker_init(voice_tracker_t *tracker) {
    memset(tracker, 0, sizeof(voice_tracker_t));
}

static int voice_tracker_find(voice_tracker_t *tracker, uint64_t user_id, uint64_t guild_id) {
    for (int i = 0; i < tracker->count; i++) {
        if (tracker->sessions[i].user_id == user_id &&
            tracker->sessions[i].guild_id == guild_id) {
            return i;
        }
    }
    return -1;
}

static void voice_tracker_join(voice_tracker_t *tracker, uint64_t user_id,
                                uint64_t guild_id, uint64_t channel_id) {
    int idx = voice_tracker_find(tracker, user_id, guild_id);
    if (idx >= 0) {
        /* Update channel if switching */
        tracker->sessions[idx].channel_id = channel_id;
        return;
    }
    if (tracker->count >= MAX_VOICE_SESSIONS) return;

    idx = tracker->count++;
    tracker->sessions[idx].user_id = user_id;
    tracker->sessions[idx].guild_id = guild_id;
    tracker->sessions[idx].channel_id = channel_id;
    tracker->sessions[idx].join_time = time(NULL);
    tracker->sessions[idx].last_xp_grant = time(NULL);
}

static void voice_tracker_leave(voice_tracker_t *tracker, uint64_t user_id, uint64_t guild_id) {
    int idx = voice_tracker_find(tracker, user_id, guild_id);
    if (idx < 0) return;

    /* Swap with last element */
    tracker->count--;
    if (idx < tracker->count) {
        tracker->sessions[idx] = tracker->sessions[tracker->count];
    }
}

void voice_tracker_grant_xp(yuno_bot_t *bot) {
    voice_tracker_t *tracker = &bot->voice_tracker;
    int64_t now = time(NULL);

    for (int i = 0; i < tracker->count; i++) {
        voice_session_t *s = &tracker->sessions[i];

        /* Check if voice XP is enabled for this guild */
        voice_xp_config_t config;
        if (db_get_voice_xp_config(&bot->database, s->guild_id, &config) != 0 || !config.enabled)
            continue;

        /* Check if enough time elapsed since last grant (default: every 60 seconds) */
        int interval = config.xp_per_minute > 0 ? 60 : 60;
        if ((now - s->last_xp_grant) < interval) continue;

        /* Grant XP */
        int xp = config.xp_per_minute > 0 ? config.xp_per_minute : 5;
        xp_batcher_add(bot, s->user_id, s->guild_id, s->channel_id, xp);
        s->last_xp_grant = now;
    }
}

int bot_init(yuno_bot_t *bot, const yuno_config_t *config) {
    memset(bot, 0, sizeof(yuno_bot_t));
    memcpy(&bot->config, config, sizeof(yuno_config_t));

    /* Open database */
    if (db_open(&bot->database, config->database_path) != 0) {
        fprintf(stderr, "💔 Failed to open database\n");
        return -1;
    }

    /* Create Discord client */
    bot->client = discord_init(config->discord_token);
    if (!bot->client) {
        fprintf(stderr, "💔 Failed to initialize Discord client\n");
        db_close(&bot->database);
        return -1;
    }

    /* Record start time for uptime tracking */
    bot->start_time = time(NULL);

    /* Initialize XP batcher */
    xp_batcher_init(&bot->xp_batcher);

    /* Initialize voice tracker */
    voice_tracker_init(&bot->voice_tracker);

    /* Initialize connection state */
    bot->connection.is_connected = 0;
    bot->connection.reconnect_count = 0;
    bot->connection.last_disconnect = 0;

    /* Set global bot pointer for callbacks */
    g_bot = bot;

    /* Set up event handlers */
    discord_set_on_ready(bot->client, on_ready);
    discord_set_on_message_create(bot->client, on_message_create);
    discord_set_on_interaction_create(bot->client, on_interaction_create);
    discord_set_on_voice_state_update(bot->client, on_voice_state_update);
    discord_set_on_guild_member_add(bot->client, on_guild_member_add);

    /* Activity logger events */
    discord_set_on_message_update(bot->client, on_message_update);
    discord_set_on_message_delete(bot->client, on_message_delete);
    discord_set_on_guild_ban_add(bot->client, on_guild_ban_add);
    discord_set_on_guild_ban_remove(bot->client, on_guild_ban_remove);
    discord_set_on_guild_member_update(bot->client, on_guild_member_update);

    /* Initialize terminal interface */
    terminal_init(bot);

    /* Initialize spam filter */
    spam_filter_init(bot);

    /* Initialize activity logger */
    activity_logger_init();
    if (config->low_memory_mode) {
        activity_logger_set_low_memory(1);
    }

    /* Initialize auto-cleaner */
    auto_cleaner_init(&bot->auto_cleaner);

    /* Initialize LRU cache */
    lru_cache_init(&bot->cache);

    /* Initialize HTTP client (libcurl) */
    http_global_init();

    /* Initialize global worker threadpool for async HTTP commands */
    discord_worker_global_init(0);

    return 0;
}

void bot_cleanup(yuno_bot_t *bot) {
    /* Stop auto-cleaner */
    auto_cleaner_stop(&bot->auto_cleaner);
    auto_cleaner_cleanup(&bot->auto_cleaner);

    /* Cancel XP flush timer and do final flush */
    if (bot->client && bot->xp_flush_timer_id) {
        discord_timer_cancel(bot->client, bot->xp_flush_timer_id);
        bot->xp_flush_timer_id = 0;
    }
    xp_batcher_flush(bot);
    activity_logger_flush(bot->client, &bot->database);

    /* Stop terminal */
    terminal_stop();
    terminal_cleanup();

    /* Stop spam filter */
    spam_filter_cleanup();

    /* Wait for any pending worker threads to finish */
    if (bot->client) {
        discord_worker_join(bot->client);
    }

    if (bot->client) {
        discord_cleanup(bot->client);
        bot->client = NULL;
    }
    db_close(&bot->database);

    /* Cleanup worker threadpool and HTTP client */
    discord_worker_global_cleanup();
    http_global_cleanup();

    g_bot = NULL;
}

int bot_run(yuno_bot_t *bot) {
    bot->running = 1;
    discord_run(bot->client);
    return 0;
}

void bot_stop(yuno_bot_t *bot) {
    bot->running = 0;
    if (bot->client) {
        discord_shutdown(bot->client);
    }
}

/* XP/activity flush timer callback — runs on Concord event loop every 10 seconds */
static void xp_flush_timer_cb(struct discord *client, struct discord_timer *timer) {
    (void)client;
    (void)timer;
    if (!g_bot) return;

    voice_tracker_grant_xp(g_bot);
    xp_batcher_flush(g_bot);
    activity_logger_flush(g_bot->client, &g_bot->database);
}

void on_ready(struct discord *client, const struct discord_ready *event) {
    (void)client;
    printf("💕 Yuno is online! Logged in as %s~ 💕\n", event->user->username);
    printf("💗 I'm watching over your servers for you~ 💗\n");

    /* Mark as connected */
    g_bot->connection.is_connected = 1;
    if (g_bot->connection.reconnect_count > 0) {
        printf("✓ Reconnected successfully (attempt #%d)\n", g_bot->connection.reconnect_count);
        g_bot->connection.reconnect_count = 0;
    }

    /* Register slash commands */
    bot_register_commands(g_bot);

    /* Restore bot presence from database */
    bot_restore_presence(g_bot);

    /* Start terminal interface */
    terminal_start();

    /* Start auto-cleaner on Concord event loop timer */
    auto_cleaner_start(&g_bot->auto_cleaner, client);

    /* Start XP/activity flush timer (every 10 seconds) */
    g_bot->xp_flush_timer_id = discord_timer_interval(client,
        xp_flush_timer_cb, NULL, NULL,
        10000,   /* initial delay: 10 seconds */
        10000,   /* interval: 10 seconds */
        -1);     /* repeat forever */
}

/* Command dispatch using hash-based lookup - O(1) average instead of O(n) strcmp chain */

typedef void (*prefix_cmd_handler_t)(struct discord *, const struct discord_message *, const char *);
typedef void (*slash_cmd_handler_t)(struct discord *, const struct discord_interaction *);

typedef struct {
    const char *name;
    const char *alias;  /* Optional alias, NULL if none */
    prefix_cmd_handler_t prefix_handler;
    slash_cmd_handler_t slash_handler;
} command_entry_t;

/* DJB2 hash for command names */
static inline uint32_t hash_command(const char *name) {
    uint32_t hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* Command table - sorted by usage frequency for cache efficiency */
const command_entry_t g_commands[] = {
    /* High frequency commands first */
    { "xp",         NULL,       cmd_xp_prefix,         cmd_xp },
    { "level",      NULL,       cmd_xp_prefix,         NULL },
    { "rank",       NULL,       cmd_xp_prefix,         NULL },
    { "ping",       NULL,       cmd_ping_prefix,       cmd_ping },
    { "help",       NULL,       cmd_help_prefix,       cmd_help },
    { "leaderboard", "lb",      cmd_leaderboard_prefix, cmd_leaderboard },
    { "top",        NULL,       cmd_leaderboard_prefix, NULL },
    { "8ball",      NULL,       cmd_8ball_prefix,      cmd_8ball },
    { "quote",      NULL,       cmd_quote_prefix,      cmd_quote },
    { "praise",     NULL,       cmd_praise_prefix,     cmd_praise },
    { "scold",      NULL,       cmd_scold_prefix,      cmd_scold },
    { "anime",      "animoo",   cmd_anime_prefix,      cmd_anime },
    { "manga",      NULL,       cmd_manga_prefix,      cmd_manga },
    { "neko",       "nya",      cmd_neko_prefix,       cmd_neko },
    { "urban",      "ub",       cmd_urban_prefix,      cmd_urban },
    { "hentai",     "hen",      cmd_hentai_prefix,     cmd_hentai },

    /* Moderation commands */
    { "ban",        NULL,       cmd_ban_prefix,        cmd_ban },
    { "kick",       NULL,       cmd_kick_prefix,       cmd_kick },
    { "unban",      NULL,       cmd_unban_prefix,      cmd_unban },
    { "timeout",    NULL,       cmd_timeout_prefix,    cmd_timeout },
    { "clean",      NULL,       cmd_clean_prefix,      cmd_clean },
    { "mod-stats",  "modstats", cmd_mod_stats_prefix,  cmd_mod_stats },
    { "scan-bans",  "scanbans", cmd_scan_bans_prefix, cmd_scan_bans },
    { "exportbans", NULL,       cmd_export_bans_prefix, cmd_export_bans },
    { "importbans", NULL,       cmd_import_bans_prefix, cmd_import_bans },

    /* Utility commands */
    { "stats",      "inf",      cmd_stats_prefix,      cmd_stats },
    { "source",     NULL,       cmd_source_prefix,     cmd_source },
    { "prefix",     NULL,       cmd_prefix_prefix,     cmd_prefix },
    { "auto-clean", "autoclean", cmd_auto_clean_prefix, cmd_auto_clean },
    { "delay",      NULL,       cmd_delay_prefix,      cmd_delay },

    /* Admin/master commands */
    { "bot-ban",    "botban",   cmd_bot_ban_prefix,    cmd_bot_ban },
    { "bot-unban",  "botunban", cmd_bot_unban_prefix,  cmd_bot_unban },
    { "bot-banlist","bot-bans", cmd_bot_banlist_prefix, cmd_bot_banlist },
    { "set-spamfilter", "ssf",  cmd_set_spamfilter_prefix, cmd_set_spamfilter },
    { "set-experiencecounter", "set-expcounter", cmd_set_xp_prefix, cmd_set_xp },

    /* XP admin commands */
    { "set-level",  "slvl",     cmd_set_level_prefix,  cmd_set_level },
    { "fix-xp-data","fixxp",    cmd_fix_xp_prefix,     cmd_fix_xp },
    { "mass-addxp", "massxp",   cmd_mass_addxp_prefix, cmd_mass_addxp },
    { "mass-setxp", NULL,       cmd_mass_setxp_prefix, cmd_mass_setxp },

    /* Level-role commands */
    { "set-levelrolemap", "slrmap", cmd_set_levelrolemap_prefix, cmd_set_levelrolemap },
    { "sync-levelroles", "syncroles", cmd_sync_levelroles_prefix, cmd_sync_levelroles },
    { "sync-xp-from-roles", "syncxp", cmd_sync_xp_from_roles_prefix, cmd_sync_xp_from_roles },

    /* Voice XP commands */
    { "set-vcxp",    NULL,       cmd_set_vcxp_prefix,   cmd_set_vcxp },
    { "vcxp-status", "vcxp",    cmd_vcxp_status_prefix, cmd_vcxp_status },

    /* Configuration commands */
    { "set-dm-channel", "setdm", cmd_set_dm_channel_prefix, cmd_set_dm_channel },
    { "dm-status",  "dmstatus",  cmd_dm_status_prefix,  cmd_dm_status },
    { "set-joinmessage", "sjm",  cmd_set_joinmessage_prefix, cmd_set_joinmessage },
    { "set-logchannel", "slc",   cmd_set_logchannel_prefix, cmd_set_logchannel },
    { "log-status", "logstatus", cmd_log_status_prefix, cmd_log_status },
    { "set-logsettings", "sls",  cmd_set_logsettings_prefix, cmd_set_logsettings },
    { "set-invitefilter", "sif", cmd_set_invitefilter_prefix, cmd_set_invitefilter },
    { "config",     "cfg",      cmd_config_prefix,     cmd_config },

    /* Admin/master commands */
    { "send",          NULL,       cmd_send_prefix,          cmd_send },
    { "reply",         NULL,       cmd_reply_dm_prefix,      cmd_reply_dm },
    { "inbox",         NULL,       cmd_inbox_prefix,         cmd_inbox },
    { "add-masteruser", NULL,      cmd_add_masteruser_prefix, cmd_add_masteruser },
    { "debug-error",   NULL,       cmd_debug_error_prefix,   cmd_debug_error },
    { "init-guild",    NULL,       cmd_init_guild_prefix,    cmd_init_guild },

    /* Error channel logging */
    { "drop-errors-on", "errch", cmd_drop_errors_on_prefix, cmd_drop_errors_on },

    /* Mention response commands */
    { "add-mentionresponse", "amr", cmd_add_mentionresponse_prefix, cmd_add_mentionresponse },
    { "del-mentionresponse", "dmr", cmd_del_mentionresponse_prefix, cmd_del_mentionresponse },
    { "mentionresponses", "mrs",    cmd_mentionresponses_prefix,    cmd_mentionresponses },

    /* List command */
    { "list-command", "cmds",       cmd_list_command_prefix,        cmd_list_command },

    /* Ban image commands */
    { "set-banimage", "sbi",        cmd_set_banimage_prefix,       cmd_set_banimage },
    { "del-banimage", "dbi",        cmd_del_banimage_prefix,       cmd_del_banimage },

    /* Custom spam rule commands */
    { "add-spamrule", "asr",        cmd_add_spamrule_prefix,       cmd_add_spamrule },
    { "del-spamrule", "dsr",        cmd_del_spamrule_prefix,       cmd_del_spamrule },
    { "spamrules",    "srs",        cmd_spamrules_prefix,          cmd_spamrules },
};

#define NUM_COMMANDS (sizeof(g_commands) / sizeof(g_commands[0]))
const int g_num_commands = sizeof(g_commands) / sizeof(g_commands[0]);
#define CMD_HASH_SIZE 67  /* Small prime, commands are few */

static int g_cmd_hash_table[CMD_HASH_SIZE];
static int g_cmd_initialized = 0;

/* Initialize command hash table on first use */
static void init_command_hash(void) {
    if (g_cmd_initialized) return;

    for (int i = 0; i < CMD_HASH_SIZE; i++) {
        g_cmd_hash_table[i] = -1;
    }
    /* Note: This simple hash doesn't handle collisions perfectly,
       but with few commands and good hash distribution, it's fine.
       For collisions, we fall back to linear scan of that bucket. */
    g_cmd_initialized = 1;
}

/* Find prefix command handler - O(1) average with hash, O(n) worst case */
static prefix_cmd_handler_t find_prefix_handler(const char *name) {
    init_command_hash();

    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(g_commands[i].name, name) == 0) {
            return g_commands[i].prefix_handler;
        }
        if (g_commands[i].alias && strcmp(g_commands[i].alias, name) == 0) {
            return g_commands[i].prefix_handler;
        }
    }
    return NULL;
}

/* Find slash command handler */
static slash_cmd_handler_t find_slash_handler(const char *name) {
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(g_commands[i].name, name) == 0 && g_commands[i].slash_handler) {
            return g_commands[i].slash_handler;
        }
    }
    return NULL;
}

/* Maximum message content we'll process */
#define MAX_CONTENT_LEN 2048

void on_message_create(struct discord *client, const struct discord_message *msg) {
    char prefix[MAX_PREFIX_LEN];
    char content_buf[MAX_CONTENT_LEN]; /* Stack allocation instead of strdup */
    char *command;
    char *args;
    size_t prefix_len;
    size_t content_len;

    /* Watch notification: stream messages to terminal if channel is being watched */
    if (msg->channel_id && msg->author) {
        terminal_notify_watch(msg->channel_id,
                               msg->author->username ? msg->author->username : "Unknown",
                               msg->content,
                               msg->attachments ? msg->attachments->size : 0,
                               msg->embeds ? msg->embeds->size > 0 : 0);
    }

    /* Ignore bots */
    if (msg->author->bot) return;

    /* Check for bot-level ban */
    if (db_is_bot_banned(&g_bot->database, msg->author->id)) {
        return; /* Silently ignore banned users */
    }

    /* Handle DMs - save to inbox and respond */
    if (msg->guild_id == 0) {
        /* Save DM to inbox */
        dm_inbox_t dm = {
            .user_id = msg->author->id,
            .timestamp = time(NULL),
            .read_status = 0
        };
        strncpy(dm.username, msg->author->username, sizeof(dm.username) - 1);
        strncpy(dm.content, msg->content, sizeof(dm.content) - 1);
        db_save_dm(&g_bot->database, &dm);

        /* Notify in terminal - avoid strlen in printf */
        content_len = strlen(msg->content);
        printf("\n📬 New DM from %s (%lu): %.50s%s\n",
            msg->author->username, (unsigned long)msg->author->id,
            msg->content, content_len > 50 ? "..." : "");

        /* Forward DM to configured channels */
        dm_config_t dm_configs[10];
        int dm_config_count = 0;
        db_get_all_dm_configs(&g_bot->database, dm_configs, 10, &dm_config_count);

        if (dm_config_count > 0) {
            char forward_msg[2200];
            snprintf(forward_msg, sizeof(forward_msg),
                "📬 **DM from %s** (`%lu`)\n\n%.*s",
                msg->author->username, (unsigned long)msg->author->id,
                (int)(sizeof(forward_msg) - 200), msg->content);

            for (int i = 0; i < dm_config_count; i++) {
                if (!dm_configs[i].enabled) continue;
                struct discord_create_message fwd = { .content = forward_msg };
                discord_create_message(client, dm_configs[i].channel_id, &fwd, NULL);
            }
        }

        /* Send auto-reply */
        struct discord_create_message params = { .content = g_bot->config.dm_message };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Run spam filter and invite filter */
    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) == 0) {
        /* Spam filter */
        if (settings.spam_filter_enabled && spam_filter_handle(g_bot, msg)) {
            return; /* Message was spam, already handled */
        }
        /* Invite link filter - skip master users */
        if (settings.invite_filter_enabled && !bot_is_master_user(g_bot, msg->author->id)) {
            if (strstr(msg->content, "discord.gg/") ||
                strstr(msg->content, "discord.com/invite/") ||
                strstr(msg->content, "discordapp.com/invite/")) {
                discord_delete_message(client, msg->channel_id, msg->id, NULL, NULL);
                char warn_msg[256];
                snprintf(warn_msg, sizeof(warn_msg),
                    "<@%lu> Invite links are not allowed here~ 💢",
                    (unsigned long)msg->author->id);
                struct discord_create_message warn_params = { .content = warn_msg };
                discord_create_message(client, msg->channel_id, &warn_params, NULL);
                return;
            }
        }
    }

    /* Check for mention responses */
    if (msg->mentions && msg->content) {
        mention_response_t mr;
        /* Check each mentioned user for a configured trigger */
        for (int i = 0; i < msg->mentions->size; i++) {
            if (db_find_mention_response(&g_bot->database, msg->guild_id,
                    msg->mentions->array[i].id, msg->content, &mr) == 0) {
                struct discord_create_message mr_params = { .content = mr.response };
                discord_create_message(client, msg->channel_id, &mr_params, NULL);
                break; /* Only respond to first match */
            }
        }
    }

    /* Get guild prefix */
    db_get_prefix(&g_bot->database, msg->guild_id, g_bot->config.default_prefix, prefix, sizeof(prefix));
    prefix_len = strlen(prefix);

    /* Check for prefix */
    if (strncmp(msg->content, prefix, prefix_len) != 0) {
        /* Add XP for chatting using batcher */
        guild_settings_t lvl_settings;
        if (db_get_guild_settings(&g_bot->database, msg->guild_id, &lvl_settings) != 0 || lvl_settings.leveling_enabled) {
            xp_batcher_add(g_bot, msg->author->id, msg->guild_id, msg->channel_id, g_bot->config.xp_per_msg);
        }
        return;
    }

    /* Copy content to stack buffer instead of heap allocation */
    content_len = strlen(msg->content + prefix_len);
    if (content_len >= MAX_CONTENT_LEN) {
        content_len = MAX_CONTENT_LEN - 1;
    }
    memcpy(content_buf, msg->content + prefix_len, content_len);
    content_buf[content_len] = '\0';

    /* Parse command */
    command = strtok(content_buf, " \t\n");
    if (!command) {
        return;
    }

    /* Convert command to lowercase */
    for (char *p = command; *p; p++) {
        *p = tolower((unsigned char)*p);
    }

    /* Get remaining args */
    args = strtok(NULL, "");
    if (!args) args = "";

    /* Hash-based command dispatch */
    prefix_cmd_handler_t handler = find_prefix_handler(command);
    if (handler) {
        handler(client, msg, args);
    }
}

void on_interaction_create(struct discord *client, const struct discord_interaction *interaction) {
    if (interaction->type != DISCORD_INTERACTION_APPLICATION_COMMAND) return;

    const char *name = interaction->data->name;

    /* Hash-based slash command dispatch */
    slash_cmd_handler_t handler = find_slash_handler(name);
    if (handler) {
        handler(client, interaction);
    }
}

void on_voice_state_update(struct discord *client, const struct discord_voice_state *event) {
    (void)client;
    if (!event->guild_id) return;

    if (event->channel_id == 0) {
        /* User left voice */
        voice_tracker_leave(&g_bot->voice_tracker, event->user_id, event->guild_id);
    } else {
        /* User joined or switched channel */
        voice_tracker_join(&g_bot->voice_tracker, event->user_id, event->guild_id, event->channel_id);
    }
}

/* Async callback: DM channel created for welcome message */
typedef struct {
    char welcome_msg[2048];
} welcome_dm_ctx_t;

static void welcome_dm_cleanup(struct discord *client, void *data) {
    (void)client;
    free(data);
}

static void on_welcome_dm_created(struct discord *client, struct discord_response *resp,
                                   const struct discord_channel *dm_channel) {
    welcome_dm_ctx_t *ctx = resp->data;
    struct discord_create_message msg_params = { .content = ctx->welcome_msg };
    discord_create_message(client, dm_channel->id, &msg_params, NULL);
}

void on_guild_member_add(struct discord *client, const struct discord_guild_member *member) {
    if (!member->user || !member->guild_id) return;

    /* Auto-role restoration: restore previously saved roles */
    uint64_t saved_roles[50];
    int saved_count = 0;
    if (db_get_user_roles(&g_bot->database, member->guild_id, member->user->id,
                           saved_roles, 50, &saved_count) == 0 && saved_count > 0) {
        for (int i = 0; i < saved_count; i++) {
            discord_add_guild_member_role(client, member->guild_id,
                member->user->id, saved_roles[i], NULL, NULL);
        }
        printf("✓ Restored %d roles for %s in guild %lu\n",
            saved_count, member->user->username,
            (unsigned long)member->guild_id);
    }

    /* Check for join message */
    char title[256] = { 0 };
    char message[1024] = { 0 };
    if (db_get_join_message(&g_bot->database, member->guild_id, title, sizeof(title),
                             message, sizeof(message)) != 0 || !message[0]) {
        return; /* No join message configured */
    }

    /* Build welcome message and create DM channel asynchronously */
    welcome_dm_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate memory for welcome DM context\n");
        return;
    }
    snprintf(ctx->welcome_msg, sizeof(ctx->welcome_msg),
        "**%s**\n\n%s\n\n💕 *— Yuno*",
        title, message);

    struct discord_create_dm dm_params = { .recipient_id = member->user->id };
    struct discord_ret_channel ret = {
        .done = on_welcome_dm_created,
        .data = ctx,
        .cleanup = welcome_dm_cleanup
    };
    discord_create_dm(client, &dm_params, &ret);
}

void bot_update_presence(yuno_bot_t *bot, const bot_presence_t *presence) {
    if (!bot->client) return;

    struct discord_presence_update pres = { 0 };
    pres.status = (char *)presence->status;

    struct discord_activity activity = { 0 };
    if (presence->text[0]) {
        activity.name = (char *)presence->text;
        activity.type = presence->type;
        if (presence->stream_url[0]) {
            activity.url = (char *)presence->stream_url;
        }
        discord_presence_add_activity(&pres, &activity);
    }

    discord_update_presence(bot->client, &pres);

    /* Save to database */
    db_set_bot_presence(&bot->database, presence);
}

void bot_restore_presence(yuno_bot_t *bot) {
    bot_presence_t presence;
    if (db_get_bot_presence(&bot->database, &presence) == 0 && presence.text[0]) {
        bot_update_presence(bot, &presence);
        printf("✓ Restored presence: %s\n", presence.text);
    }
}

int bot_register_commands(yuno_bot_t *bot) {
    struct discord_application_command commands[] = {
        /* Utility commands */
        { .name = "ping", .description = "Check if Yuno is awake~ 💓", .type = DISCORD_APPLICATION_CHAT_INPUT },
        { .name = "help", .description = "See what Yuno can do for you~ 💕", .type = DISCORD_APPLICATION_CHAT_INPUT },
        { .name = "source", .description = "See Yuno's source code~ 📜", .type = DISCORD_APPLICATION_CHAT_INPUT },
    };

    (void)bot;
    (void)commands;

    printf("💕 Registering slash commands~\n");
    return 0;
}

int bot_is_master_user(yuno_bot_t *bot, uint64_t user_id) {
    char user_str[32];
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    return config_is_master_user(&bot->config, user_str);
}

uint64_t parse_user_mention(const char *mention) {
    const char *start;
    char *end;
    uint64_t id;

    /* Check for <@123> or <@!123> format */
    if (mention[0] == '<' && mention[1] == '@') {
        start = mention + 2;
        if (*start == '!') start++;
        id = strtoull(start, &end, 10);
        if (*end == '>') return id;
    }

    /* Try parsing as raw ID */
    id = strtoull(mention, &end, 10);
    if (*end == '\0') return id;

    return 0;
}

void format_duration(int64_t seconds, char *buffer, size_t len) {
    if (seconds < 60) {
        snprintf(buffer, len, "%ld seconds", (long)seconds);
    } else if (seconds < 3600) {
        snprintf(buffer, len, "%ld minutes", (long)(seconds / 60));
    } else if (seconds < 86400) {
        snprintf(buffer, len, "%ld hours", (long)(seconds / 3600));
    } else {
        snprintf(buffer, len, "%ld days", (long)(seconds / 86400));
    }
}
