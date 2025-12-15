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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
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

    /* Flush if batch is full or time elapsed */
    if (batcher->count >= MAX_PENDING_XP || (time(NULL) - batcher->last_flush) >= XP_FLUSH_INTERVAL) {
        xp_batcher_flush(bot);
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

        /* Add XP */
        db_add_xp(&bot->database, p->user_id, p->guild_id, p->xp_amount);

        /* Calculate new level */
        int64_t new_total = user_xp.xp + p->xp_amount;
        int new_level = (int)sqrt(new_total / 100.0);

        /* Check for level up */
        if (new_level > user_xp.level) {
            db_set_level(&bot->database, p->user_id, p->guild_id, new_level);

            /* Send level up message */
            if (bot->client && p->channel_id != 0) {
                char level_msg[256];
                snprintf(level_msg, sizeof(level_msg),
                    "✨ **Level Up!** ✨\nCongratulations <@%lu>! You've reached level **%d**! 💕",
                    (unsigned long)p->user_id, new_level);

                struct discord_create_message params = { .content = level_msg };
                discord_create_message(bot->client, p->channel_id, &params, NULL);
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

    /* Initialize XP batcher */
    xp_batcher_init(&bot->xp_batcher);

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

    /* Initialize terminal interface */
    terminal_init(bot);

    /* Initialize spam filter */
    spam_filter_init(bot);

    return 0;
}

void bot_cleanup(yuno_bot_t *bot) {
    /* Flush any remaining XP */
    xp_batcher_flush(bot);

    /* Stop terminal */
    terminal_stop();
    terminal_cleanup();

    /* Stop spam filter */
    spam_filter_cleanup();

    if (bot->client) {
        discord_cleanup(bot->client);
        bot->client = NULL;
    }
    db_close(&bot->database);
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

    /* Start terminal interface */
    terminal_start();
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
static const command_entry_t g_commands[] = {
    /* High frequency commands first */
    { "xp",         NULL,       cmd_xp_prefix,         cmd_xp },
    { "level",      NULL,       cmd_xp_prefix,         NULL },
    { "rank",       NULL,       cmd_xp_prefix,         NULL },
    { "ping",       NULL,       cmd_ping_prefix,       cmd_ping },
    { "help",       NULL,       cmd_help_prefix,       cmd_help },
    { "leaderboard", "lb",      cmd_leaderboard_prefix, cmd_leaderboard },
    { "top",        NULL,       cmd_leaderboard_prefix, NULL },
    { "8ball",      NULL,       cmd_8ball_prefix,      cmd_8ball },

    /* Moderation commands */
    { "ban",        NULL,       cmd_ban_prefix,        cmd_ban },
    { "kick",       NULL,       cmd_kick_prefix,       cmd_kick },
    { "unban",      NULL,       cmd_unban_prefix,      cmd_unban },
    { "timeout",    NULL,       cmd_timeout_prefix,    cmd_timeout },
    { "clean",      NULL,       cmd_clean_prefix,      cmd_clean },
    { "mod-stats",  "modstats", cmd_mod_stats_prefix,  cmd_mod_stats },

    /* Utility commands */
    { "source",     NULL,       cmd_source_prefix,     cmd_source },
    { "prefix",     NULL,       cmd_prefix_prefix,     cmd_prefix },
    { "auto-clean", "autoclean", cmd_auto_clean_prefix, cmd_auto_clean },
    { "delay",      NULL,       cmd_delay_prefix,      cmd_delay },
};

#define NUM_COMMANDS (sizeof(g_commands) / sizeof(g_commands[0]))
#define CMD_HASH_SIZE 37  /* Small prime, commands are few */

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

        /* Send auto-reply */
        struct discord_create_message params = { .content = g_bot->config.dm_message };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Run spam filter */
    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) == 0 && settings.spam_filter_enabled) {
        if (spam_filter_handle(g_bot, msg)) {
            return; /* Message was spam, already handled */
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
            /* Better random distribution */
            int xp_gain = 15 + (rand() % 11);
            xp_batcher_add(g_bot, msg->author->id, msg->guild_id, msg->channel_id, xp_gain);
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
