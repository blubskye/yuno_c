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

typedef struct {
    time_t timestamp;
    uint32_t content_hash;
} message_record_t;

typedef struct {
    uint64_t user_id;
    uint64_t guild_id;
    message_record_t history[MAX_MESSAGE_HISTORY];
    int history_count;
} user_message_history_t;

typedef struct {
    user_message_history_t users[MAX_TRACKED_USERS];
    int user_count;
} spam_filter_t;

/* Spam filter lifecycle */
int spam_filter_init(spam_filter_t *filter);
void spam_filter_cleanup(spam_filter_t *filter);

/* Check message for spam, returns 1 if spam detected */
int spam_filter_check(spam_filter_t *filter, uint64_t user_id, uint64_t guild_id, const char *content);

/* Handle spam detection */
void spam_filter_handle(spam_filter_t *filter, struct discord *client, const struct discord_message *msg);

/* Clear user history */
void spam_filter_clear_user(spam_filter_t *filter, uint64_t user_id, uint64_t guild_id);

/* Hash function for content */
uint32_t hash_content(const char *content);

#endif /* YUNO_MODULES_SPAM_FILTER_H */
