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

typedef struct {
    struct discord *client;
    yuno_config_t config;
    yuno_database_t database;
    int running;
    xp_batcher_t xp_batcher;
    connection_state_t connection;
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

#endif /* YUNO_BOT_H */
