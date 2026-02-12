/*
 * Yuno Gasai 2 (C Edition) - Activity Logger Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_ACTIVITY_LOGGER_H
#define YUNO_MODULES_ACTIVITY_LOGGER_H

#include <stdint.h>
#include <pthread.h>
#include <concord/discord.h>
#include "bot.h"

/* Buffer sizes: normal mode gets 200, low memory mode gets 50 */
#define LOG_BUFFER_NORMAL  200
#define LOG_BUFFER_LOWMEM  50
#define MAX_LOG_BUFFER     LOG_BUFFER_NORMAL

/* Low memory mode constants (matching JS) */
#define LOWMEM_STALE_TIMEOUT   300  /* 5 minutes - remove inactive guild entries */
#define LOWMEM_CHECK_INTERVAL   60  /* Check every 60 seconds */
#define LOWMEM_MAX_TRACKED_GUILDS 64

typedef struct {
    uint64_t guild_id;
    uint64_t user_id;
    uint64_t channel_id;
    char event_type[32];
    char description[512];
    int64_t timestamp;
} log_entry_t;

/* Per-guild activity tracking for low memory mode */
typedef struct {
    uint64_t guild_id;
    int64_t last_activity;
} guild_activity_t;

typedef struct {
    log_entry_t buffer[MAX_LOG_BUFFER];
    int count;
    int max_entries;  /* Effective max: LOG_BUFFER_LOWMEM or LOG_BUFFER_NORMAL */
    int64_t last_flush;
    int low_memory_mode;

    /* Low memory mode: per-guild activity tracking */
    guild_activity_t guild_activity[LOWMEM_MAX_TRACKED_GUILDS];
    int guild_activity_count;
    int64_t last_stale_check;

    pthread_mutex_t mutex;
} activity_logger_t;

/* Global logger instance */
extern activity_logger_t g_activity_logger;

/* Lifecycle */
void activity_logger_init(void);
void activity_logger_set_low_memory(int enabled);
void activity_logger_flush(struct discord *client, yuno_database_t *database);

/* Add a log entry to the buffer */
void activity_logger_add(uint64_t guild_id, uint64_t user_id, uint64_t channel_id,
                          const char *event_type, const char *description);

/* Low memory mode: trim stale guild entries */
void activity_logger_cleanup_stale(void);

/* Event handlers */
void on_message_update(struct discord *client, const struct discord_message *event);
void on_message_delete(struct discord *client, const struct discord_message_delete *event);
void on_guild_ban_add(struct discord *client, const struct discord_guild_ban_add *event);
void on_guild_ban_remove(struct discord *client, const struct discord_guild_ban_remove *event);
void on_guild_member_update(struct discord *client, const struct discord_guild_member_update *event);

#endif /* YUNO_MODULES_ACTIVITY_LOGGER_H */
