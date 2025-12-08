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

int spam_filter_init(spam_filter_t *filter) {
    memset(filter, 0, sizeof(spam_filter_t));
    return 0;
}

void spam_filter_cleanup(spam_filter_t *filter) {
    filter->user_count = 0;
}

uint32_t hash_content(const char *content) {
    uint32_t hash = 5381;
    int c;
    while ((c = *content++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static int find_user_entry(spam_filter_t *filter, uint64_t user_id, uint64_t guild_id) {
    for (int i = 0; i < filter->user_count; i++) {
        if (filter->users[i].user_id == user_id &&
            filter->users[i].guild_id == guild_id) {
            return i;
        }
    }
    return -1;
}

static void cleanup_old_messages(user_message_history_t *user, time_t now) {
    /* Remove messages older than 60 seconds */
    int new_count = 0;
    for (int i = 0; i < user->history_count; i++) {
        if (now - user->history[i].timestamp < 60) {
            if (new_count != i) {
                user->history[new_count] = user->history[i];
            }
            new_count++;
        }
    }
    user->history_count = new_count;
}

static int is_rate_spam(user_message_history_t *user, time_t now) {
    int recent_count = 0;
    for (int i = 0; i < user->history_count; i++) {
        if (now - user->history[i].timestamp <= SPAM_INTERVAL_SECONDS) {
            recent_count++;
        }
    }
    return recent_count >= MAX_MESSAGES_PER_INTERVAL;
}

static int is_duplicate_spam(user_message_history_t *user, uint32_t content_hash) {
    int duplicates = 0;
    for (int i = 0; i < user->history_count; i++) {
        if (user->history[i].content_hash == content_hash) {
            duplicates++;
        }
    }
    return duplicates >= DUPLICATE_THRESHOLD;
}

int spam_filter_check(spam_filter_t *filter, uint64_t user_id, uint64_t guild_id, const char *content) {
    time_t now = time(NULL);
    uint32_t content_hash = hash_content(content);

    int idx = find_user_entry(filter, user_id, guild_id);

    if (idx < 0) {
        /* Create new entry */
        if (filter->user_count >= MAX_TRACKED_USERS) {
            /* Remove oldest entry */
            memmove(&filter->users[0], &filter->users[1],
                (MAX_TRACKED_USERS - 1) * sizeof(user_message_history_t));
            idx = MAX_TRACKED_USERS - 1;
        } else {
            idx = filter->user_count++;
        }
        filter->users[idx].user_id = user_id;
        filter->users[idx].guild_id = guild_id;
        filter->users[idx].history_count = 0;
    }

    user_message_history_t *user = &filter->users[idx];

    /* Cleanup old messages */
    cleanup_old_messages(user, now);

    /* Add new message to history */
    if (user->history_count < MAX_MESSAGE_HISTORY) {
        user->history[user->history_count].timestamp = now;
        user->history[user->history_count].content_hash = content_hash;
        user->history_count++;
    } else {
        /* Shift and add */
        memmove(&user->history[0], &user->history[1],
            (MAX_MESSAGE_HISTORY - 1) * sizeof(message_record_t));
        user->history[MAX_MESSAGE_HISTORY - 1].timestamp = now;
        user->history[MAX_MESSAGE_HISTORY - 1].content_hash = content_hash;
    }

    /* Check for spam */
    if (is_rate_spam(user, now) || is_duplicate_spam(user, content_hash)) {
        return 1;
    }

    return 0;
}

void spam_filter_handle(spam_filter_t *filter, struct discord *client, const struct discord_message *msg) {
    (void)filter;

    /* Delete the spam message */
    discord_delete_message(client, msg->channel_id, msg->id, NULL);

    /* Add warning to database */
    db_add_spam_warning(&g_bot->database, msg->author->id, msg->guild_id);
    int warnings = db_get_spam_warnings(&g_bot->database, msg->author->id, msg->guild_id);

    /* Check if user should be punished */
    int max_warnings = g_bot->config.spam_max_warnings;
    if (warnings >= max_warnings) {
        /* Timeout the user for 10 minutes */
        time_t timeout_until = time(NULL) + (10 * 60);
        char iso_timestamp[32];
        struct tm *tm_info = gmtime(&timeout_until);
        strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

        struct discord_modify_guild_member params = {
            .communication_disabled_until = iso_timestamp
        };
        discord_modify_guild_member(client, msg->guild_id, msg->author->id, &params, NULL);

        char warn_msg[256];
        snprintf(warn_msg, sizeof(warn_msg),
            "<@%lu> has been timed out for spamming! 😤",
            (unsigned long)msg->author->id);

        struct discord_create_message response = { .content = warn_msg };
        discord_create_message(client, msg->channel_id, &response, NULL);

        /* Reset warnings */
        db_reset_spam_warnings(&g_bot->database, msg->author->id, msg->guild_id);
    } else {
        /* Warn the user */
        char warn_msg[256];
        snprintf(warn_msg, sizeof(warn_msg),
            "<@%lu> Stop spamming! 😤 Warning %d/%d",
            (unsigned long)msg->author->id, warnings, max_warnings);

        struct discord_create_message response = { .content = warn_msg };
        discord_create_message(client, msg->channel_id, &response, NULL);
    }
}

void spam_filter_clear_user(spam_filter_t *filter, uint64_t user_id, uint64_t guild_id) {
    int idx = find_user_entry(filter, user_id, guild_id);
    if (idx >= 0) {
        /* Remove entry by shifting */
        if (idx < filter->user_count - 1) {
            memmove(&filter->users[idx], &filter->users[idx + 1],
                (filter->user_count - idx - 1) * sizeof(user_message_history_t));
        }
        filter->user_count--;
    }
}
