/*
 * Yuno Gasai 2 (C Edition) - Spam Filter Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_SPAM_FILTER_H
#define YUNO_MODULES_SPAM_FILTER_H

#include <stdint.h>
#include <time.h>
#include <concord/discord.h>

#define MAX_TRACKED_USERS 1024
#define MAX_MESSAGE_HISTORY 10
#define SPAM_INTERVAL_SECONDS 5
#define MAX_MESSAGES_PER_INTERVAL 5
#define DUPLICATE_THRESHOLD 3

/* Hash table size - should be prime and larger than MAX_TRACKED_USERS */
#define SPAM_HASH_SIZE 1543

typedef struct {
    time_t timestamp;
    uint32_t content_hash;
} message_record_t;

typedef struct {
    uint64_t user_id;
    uint64_t guild_id;
    message_record_t history[MAX_MESSAGE_HISTORY];
    int history_head;       /* Circular buffer head (next write position) */
    int history_count;      /* Number of valid entries */
    int hash_next;          /* Next index in hash chain (-1 = end) */
    int in_use;             /* 1 if entry is active */
} user_message_history_t;

typedef struct {
    user_message_history_t users[MAX_TRACKED_USERS];
    int hash_table[SPAM_HASH_SIZE]; /* Hash buckets -> index in users[], -1 = empty */
    int user_count;
    int free_head;          /* Head of free list */
} spam_filter_t;

/* Forward declaration - include bot.h for full definition */
#include "bot.h"

/* Spam filter lifecycle */
void spam_filter_init(yuno_bot_t *bot);
void spam_filter_cleanup(void);

/* Check message for spam, returns 1 if spam detected */
int spam_filter_check(uint64_t user_id, uint64_t guild_id, const char *content);

/* Handle spam detection - returns 1 if message was spam */
int spam_filter_handle(yuno_bot_t *bot, const struct discord_message *msg);

/* Clear user history */
void spam_filter_clear_user(uint64_t user_id, uint64_t guild_id);

/* Hash function for content */
uint32_t hash_content(const char *content);

#endif /* YUNO_MODULES_SPAM_FILTER_H */
