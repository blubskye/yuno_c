/*
 * Yuno Gasai 2 (C Edition) - Spam Filter Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/spam_filter.h"
#include "bot.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static spam_filter_t g_filter;
static yuno_bot_t *g_spam_bot = NULL;

/* Hash function for user+guild combination - O(1) lookup */
static inline uint32_t hash_user_guild(uint64_t user_id, uint64_t guild_id) {
    /* FNV-1a inspired hash for two uint64s */
    uint64_t h = 14695981039346656037ULL;
    h ^= user_id;
    h *= 1099511628211ULL;
    h ^= guild_id;
    h *= 1099511628211ULL;
    return (uint32_t)(h % SPAM_HASH_SIZE);
}

void spam_filter_init(yuno_bot_t *bot) {
    memset(&g_filter, 0, sizeof(spam_filter_t));
    g_spam_bot = bot;

    /* Initialize hash table buckets to -1 (empty) */
    for (int i = 0; i < SPAM_HASH_SIZE; i++) {
        g_filter.hash_table[i] = -1;
    }

    /* Initialize free list - chain all entries together */
    for (int i = 0; i < MAX_TRACKED_USERS - 1; i++) {
        g_filter.users[i].hash_next = i + 1;
        g_filter.users[i].in_use = 0;
    }
    g_filter.users[MAX_TRACKED_USERS - 1].hash_next = -1;
    g_filter.free_head = 0;
}

void spam_filter_cleanup(void) {
    g_filter.user_count = 0;
    g_spam_bot = NULL;
}

/* DJB2 hash for content strings */
uint32_t hash_content(const char *content) {
    uint32_t hash = 5381;
    int c;
    while ((c = *content++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* O(1) average case lookup using hash table */
static user_message_history_t *find_user_entry(uint64_t user_id, uint64_t guild_id) {
    uint32_t bucket = hash_user_guild(user_id, guild_id);
    int idx = g_filter.hash_table[bucket];

    while (idx >= 0) {
        user_message_history_t *entry = &g_filter.users[idx];
        if (entry->in_use && entry->user_id == user_id && entry->guild_id == guild_id) {
            return entry;
        }
        idx = entry->hash_next;
    }
    return NULL;
}

/* Allocate a new user entry from free list */
static user_message_history_t *alloc_user_entry(uint64_t user_id, uint64_t guild_id) {
    int idx;

    if (g_filter.free_head >= 0) {
        /* Get from free list */
        idx = g_filter.free_head;
        g_filter.free_head = g_filter.users[idx].hash_next;
    } else {
        /* No free entries - evict oldest (LRU would be better but this is simpler) */
        /* Find first in-use entry and evict it */
        for (idx = 0; idx < MAX_TRACKED_USERS; idx++) {
            if (g_filter.users[idx].in_use) {
                /* Remove from hash table */
                uint32_t old_bucket = hash_user_guild(
                    g_filter.users[idx].user_id,
                    g_filter.users[idx].guild_id
                );

                int *prev = &g_filter.hash_table[old_bucket];
                while (*prev >= 0) {
                    if (*prev == idx) {
                        *prev = g_filter.users[idx].hash_next;
                        break;
                    }
                    prev = &g_filter.users[*prev].hash_next;
                }
                g_filter.user_count--;
                break;
            }
        }
        if (idx >= MAX_TRACKED_USERS) {
            return NULL; /* Should never happen */
        }
    }

    /* Initialize the entry */
    user_message_history_t *entry = &g_filter.users[idx];
    entry->user_id = user_id;
    entry->guild_id = guild_id;
    entry->history_head = 0;
    entry->history_count = 0;
    entry->in_use = 1;

    /* Add to hash table */
    uint32_t bucket = hash_user_guild(user_id, guild_id);
    entry->hash_next = g_filter.hash_table[bucket];
    g_filter.hash_table[bucket] = idx;
    g_filter.user_count++;

    return entry;
}

/* Check rate spam using circular buffer - no cleanup needed */
static int is_rate_spam(const user_message_history_t *user, time_t now) {
    int recent_count = 0;
    int count = user->history_count;
    int idx = user->history_head;

    for (int i = 0; i < count; i++) {
        /* Go backwards in circular buffer */
        idx = (idx - 1 + MAX_MESSAGE_HISTORY) % MAX_MESSAGE_HISTORY;
        if (now - user->history[idx].timestamp <= SPAM_INTERVAL_SECONDS) {
            recent_count++;
        }
    }
    return recent_count >= MAX_MESSAGES_PER_INTERVAL;
}

/* Check duplicate spam using circular buffer */
static int is_duplicate_spam(const user_message_history_t *user, uint32_t content_hash) {
    int duplicates = 0;
    int count = user->history_count;
    int idx = user->history_head;

    for (int i = 0; i < count; i++) {
        idx = (idx - 1 + MAX_MESSAGE_HISTORY) % MAX_MESSAGE_HISTORY;
        if (user->history[idx].content_hash == content_hash) {
            duplicates++;
        }
    }
    return duplicates >= DUPLICATE_THRESHOLD;
}

/* Add message to circular buffer - O(1) operation, no memmove! */
static inline void add_message_to_history(user_message_history_t *user, time_t now, uint32_t content_hash) {
    user->history[user->history_head].timestamp = now;
    user->history[user->history_head].content_hash = content_hash;
    user->history_head = (user->history_head + 1) % MAX_MESSAGE_HISTORY;
    if (user->history_count < MAX_MESSAGE_HISTORY) {
        user->history_count++;
    }
}

int spam_filter_check(uint64_t user_id, uint64_t guild_id, const char *content) {
    time_t now = time(NULL);
    uint32_t content_hash = hash_content(content);

    /* O(1) lookup instead of O(n) */
    user_message_history_t *user = find_user_entry(user_id, guild_id);

    if (!user) {
        user = alloc_user_entry(user_id, guild_id);
        if (!user) return 0; /* Allocation failed, don't flag as spam */
    }

    /* Add message to circular buffer - O(1) instead of O(n) memmove */
    add_message_to_history(user, now, content_hash);

    /* Check for spam */
    if (is_rate_spam(user, now) || is_duplicate_spam(user, content_hash)) {
        return 1;
    }

    return 0;
}

int spam_filter_handle(yuno_bot_t *bot, const struct discord_message *msg) {
    /* Check if message is spam */
    if (!spam_filter_check(msg->author->id, msg->guild_id, msg->content)) {
        return 0; /* Not spam */
    }

    /* Delete the spam message */
    discord_delete_message(bot->client, msg->channel_id, msg->id, NULL);

    /* Add warning to database */
    db_add_spam_warning(&bot->database, msg->author->id, msg->guild_id);
    int warnings = db_get_spam_warnings(&bot->database, msg->author->id, msg->guild_id);

    /* Check if user should be punished */
    int max_warnings = bot->config.spam_max_warnings;
    if (warnings >= max_warnings) {
        /* Timeout the user for 10 minutes */
        time_t timeout_until = time(NULL) + (10 * 60);
        char iso_timestamp[32];
        struct tm *tm_info = gmtime(&timeout_until);
        strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

        struct discord_modify_guild_member params = {
            .communication_disabled_until = iso_timestamp
        };
        discord_modify_guild_member(bot->client, msg->guild_id, msg->author->id, &params, NULL);

        char warn_msg[256];
        snprintf(warn_msg, sizeof(warn_msg),
            "<@%lu> has been timed out for spamming! 😤",
            (unsigned long)msg->author->id);

        struct discord_create_message response = { .content = warn_msg };
        discord_create_message(bot->client, msg->channel_id, &response, NULL);

        /* Reset warnings */
        db_reset_spam_warnings(&bot->database, msg->author->id, msg->guild_id);
    } else {
        /* Warn the user */
        char warn_msg[256];
        snprintf(warn_msg, sizeof(warn_msg),
            "<@%lu> Stop spamming! 😤 Warning %d/%d",
            (unsigned long)msg->author->id, warnings, max_warnings);

        struct discord_create_message response = { .content = warn_msg };
        discord_create_message(bot->client, msg->channel_id, &response, NULL);
    }

    return 1; /* Was spam */
}

void spam_filter_clear_user(uint64_t user_id, uint64_t guild_id) {
    uint32_t bucket = hash_user_guild(user_id, guild_id);
    int *prev = &g_filter.hash_table[bucket];

    while (*prev >= 0) {
        int idx = *prev;
        user_message_history_t *entry = &g_filter.users[idx];

        if (entry->in_use && entry->user_id == user_id && entry->guild_id == guild_id) {
            /* Remove from hash chain */
            *prev = entry->hash_next;

            /* Add to free list */
            entry->in_use = 0;
            entry->hash_next = g_filter.free_head;
            g_filter.free_head = idx;
            g_filter.user_count--;
            return;
        }
        prev = &entry->hash_next;
    }
}
