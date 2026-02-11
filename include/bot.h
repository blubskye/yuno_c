/*
 * Yuno Gasai 2 (C Edition) - Bot Core
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_BOT_H
#define YUNO_BOT_H

#include <concord/discord.h>
#include "config.h"
#include "database.h"
#include "modules/auto_cleaner.h"
#include "modules/lru_cache.h"

/* XP Batcher for batching XP updates with hash table */
#define MAX_PENDING_XP 256
#define XP_HASH_SIZE 389  /* Prime number larger than MAX_PENDING_XP */

typedef struct {
    pending_xp_t pending[MAX_PENDING_XP];
    int hash_table[XP_HASH_SIZE];  /* Hash buckets -> index in pending[], -1 = empty */
    int hash_next[MAX_PENDING_XP]; /* Chain for hash collisions */
    int count;
    int64_t last_flush;
} xp_batcher_t;

/* Connection state for auto-reconnection */
typedef struct {
    int is_connected;
    int reconnect_count;
    int64_t last_disconnect;
} connection_state_t;

/* Voice session tracking for voice XP */
#define MAX_VOICE_SESSIONS 256

typedef struct {
    uint64_t user_id;
    uint64_t guild_id;
    uint64_t channel_id;
    int64_t join_time;
    int64_t last_xp_grant; /* Last time XP was granted */
} voice_session_t;

typedef struct {
    voice_session_t sessions[MAX_VOICE_SESSIONS];
    int count;
} voice_tracker_t;

typedef struct {
    struct discord *client;
    yuno_config_t config;
    yuno_database_t database;
    int running;
    int64_t start_time;
    xp_batcher_t xp_batcher;
    connection_state_t connection;
    voice_tracker_t voice_tracker;
    auto_cleaner_t auto_cleaner;
    lru_cache_t cache;
} yuno_bot_t;

/* Global bot instance (needed for callbacks) */
extern yuno_bot_t *g_bot;

/* Bot lifecycle */
int bot_init(yuno_bot_t *bot, const yuno_config_t *config);
void bot_cleanup(yuno_bot_t *bot);
int bot_run(yuno_bot_t *bot);
void bot_stop(yuno_bot_t *bot);

/* Event handlers */
void on_ready(struct discord *client, const struct discord_ready *event);
void on_message_create(struct discord *client, const struct discord_message *message);
void on_interaction_create(struct discord *client, const struct discord_interaction *interaction);
void on_voice_state_update(struct discord *client, const struct discord_voice_state *event);
void on_guild_member_add(struct discord *client, const struct discord_guild_member *member);

/* Slash command registration */
int bot_register_commands(yuno_bot_t *bot);

/* Utility functions */
int bot_is_master_user(yuno_bot_t *bot, uint64_t user_id);
uint64_t parse_user_mention(const char *mention);
void format_duration(int64_t seconds, char *buffer, size_t len);

/* XP batching */
void xp_batcher_init(xp_batcher_t *batcher);
void xp_batcher_add(yuno_bot_t *bot, uint64_t user_id, uint64_t guild_id, uint64_t channel_id, int xp);
void xp_batcher_flush(yuno_bot_t *bot);

/* Voice XP tracking */
void voice_tracker_init(voice_tracker_t *tracker);
void voice_tracker_grant_xp(yuno_bot_t *bot);

/* Presence management */
void bot_update_presence(yuno_bot_t *bot, const bot_presence_t *presence);
void bot_restore_presence(yuno_bot_t *bot);

#endif /* YUNO_BOT_H */
