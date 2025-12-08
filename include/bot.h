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

typedef struct {
    struct discord *client;
    yuno_config_t config;
    yuno_database_t database;
    int running;
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

#endif /* YUNO_BOT_H */
