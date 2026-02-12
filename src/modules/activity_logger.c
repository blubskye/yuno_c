/*
 * Yuno Gasai 2 (C Edition) - Activity Logger Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/activity_logger.h"
#include "database.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

activity_logger_t g_activity_logger;

void activity_logger_init(void) {
    memset(&g_activity_logger, 0, sizeof(activity_logger_t));
    pthread_mutex_init(&g_activity_logger.mutex, NULL);
    g_activity_logger.last_flush = time(NULL);
    g_activity_logger.last_stale_check = time(NULL);
    g_activity_logger.max_entries = LOG_BUFFER_NORMAL;
}

void activity_logger_set_low_memory(int enabled) {
    g_activity_logger.low_memory_mode = enabled;
    g_activity_logger.max_entries = enabled ? LOG_BUFFER_LOWMEM : LOG_BUFFER_NORMAL;

    if (enabled) {
        printf("📋 Activity logger: low memory mode enabled (buffer: %d entries)\n",
               LOG_BUFFER_LOWMEM);
    }
}

/* Track guild activity time for stale cleanup */
static void update_guild_activity(uint64_t guild_id) {
    activity_logger_t *logger = &g_activity_logger;
    int64_t now = time(NULL);

    /* Find existing entry */
    for (int i = 0; i < logger->guild_activity_count; i++) {
        if (logger->guild_activity[i].guild_id == guild_id) {
            logger->guild_activity[i].last_activity = now;
            return;
        }
    }

    /* Add new entry */
    if (logger->guild_activity_count < LOWMEM_MAX_TRACKED_GUILDS) {
        int idx = logger->guild_activity_count++;
        logger->guild_activity[idx].guild_id = guild_id;
        logger->guild_activity[idx].last_activity = now;
    }
}

/* Remove buffered entries from a specific guild */
static int remove_guild_entries(uint64_t guild_id) {
    activity_logger_t *logger = &g_activity_logger;
    int removed = 0;

    /* Compact buffer: shift entries to fill gaps */
    int write = 0;
    for (int read = 0; read < logger->count; read++) {
        if (logger->buffer[read].guild_id == guild_id) {
            removed++;
        } else {
            if (write != read) {
                logger->buffer[write] = logger->buffer[read];
            }
            write++;
        }
    }
    logger->count = write;
    return removed;
}

void activity_logger_cleanup_stale(void) {
    activity_logger_t *logger = &g_activity_logger;
    if (!logger->low_memory_mode) return;

    int64_t now = time(NULL);

    /* Only check periodically */
    if ((now - logger->last_stale_check) < LOWMEM_CHECK_INTERVAL) return;
    logger->last_stale_check = now;

    /* Find and remove entries from guilds inactive for > LOWMEM_STALE_TIMEOUT */
    int cleaned = 0;
    int write = 0;
    for (int i = 0; i < logger->guild_activity_count; i++) {
        if ((now - logger->guild_activity[i].last_activity) > LOWMEM_STALE_TIMEOUT) {
            int removed = remove_guild_entries(logger->guild_activity[i].guild_id);
            if (removed > 0) {
                printf("📋 [low-mem] Dropped %d stale entries for guild %lu\n",
                       removed, (unsigned long)logger->guild_activity[i].guild_id);
                cleaned += removed;
            }
            /* Don't keep this guild's activity tracking */
        } else {
            if (write != i) {
                logger->guild_activity[write] = logger->guild_activity[i];
            }
            write++;
        }
    }
    logger->guild_activity_count = write;

    if (cleaned > 0) {
        printf("📋 [low-mem] Stale cleanup: removed %d entries, buffer: %d/%d\n",
               cleaned, logger->count, logger->max_entries);
    }
}

void activity_logger_add(uint64_t guild_id, uint64_t user_id, uint64_t channel_id,
                          const char *event_type, const char *description) {
    activity_logger_t *logger = &g_activity_logger;

    pthread_mutex_lock(&logger->mutex);

    /* Low memory mode: track guild activity and check for stale entries */
    if (logger->low_memory_mode) {
        update_guild_activity(guild_id);
        activity_logger_cleanup_stale();
    }

    if (logger->count >= logger->max_entries) {
        /* Buffer full - in low memory mode, drop oldest entry to make room */
        if (logger->low_memory_mode && logger->count > 0) {
            /* Shift buffer left by 1, dropping oldest entry */
            memmove(&logger->buffer[0], &logger->buffer[1],
                    (logger->count - 1) * sizeof(log_entry_t));
            logger->count--;
            printf("📋 [low-mem] Emergency trim: dropped oldest entry\n");
        } else {
            pthread_mutex_unlock(&logger->mutex);
            return; /* Normal mode: silently drop */
        }
    }

    log_entry_t *entry = &logger->buffer[logger->count++];
    entry->guild_id = guild_id;
    entry->user_id = user_id;
    entry->channel_id = channel_id;
    entry->timestamp = time(NULL);
    strncpy(entry->event_type, event_type, sizeof(entry->event_type) - 1);
    entry->event_type[sizeof(entry->event_type) - 1] = '\0';
    strncpy(entry->description, description, sizeof(entry->description) - 1);
    entry->description[sizeof(entry->description) - 1] = '\0';

    pthread_mutex_unlock(&logger->mutex);
}

void activity_logger_flush(struct discord *client, yuno_database_t *database) {
    activity_logger_t *logger = &g_activity_logger;

    pthread_mutex_lock(&logger->mutex);
    if (logger->count == 0) {
        pthread_mutex_unlock(&logger->mutex);
        return;
    }

    /* Snapshot the buffer under lock, then release for processing */
    int flush_count = logger->count;
    log_entry_t flush_buffer[MAX_LOG_BUFFER];
    memcpy(flush_buffer, logger->buffer, flush_count * sizeof(log_entry_t));
    logger->count = 0;
    logger->last_flush = time(NULL);
    pthread_mutex_unlock(&logger->mutex);

    /* Process entries without holding the lock */
    for (int i = 0; i < flush_count; i++) {
        log_entry_t *entry = &flush_buffer[i];

        /* Save to database */
        activity_log_t db_log = {
            .guild_id = entry->guild_id,
            .user_id = entry->user_id,
            .channel_id = entry->channel_id,
            .timestamp = entry->timestamp
        };
        strncpy(db_log.event_type, entry->event_type, sizeof(db_log.event_type) - 1);
        strncpy(db_log.new_content, entry->description, sizeof(db_log.new_content) - 1);
        db_log_activity(database, &db_log);

        /* Find the appropriate log channel for this guild */
        log_channel_t log_ch;
        const char *log_type = "unified";

        if (db_get_log_channel(database, entry->guild_id, log_type, &log_ch) != 0 || !log_ch.enabled) {
            continue;
        }

        /* Format and send log message */
        char log_msg[768];
        snprintf(log_msg, sizeof(log_msg),
            "📋 **[%s]** <@%lu>\n%s",
            entry->event_type,
            (unsigned long)entry->user_id,
            entry->description);

        struct discord_create_message params = { .content = log_msg };
        discord_create_message(client, log_ch.channel_id, &params, NULL);
    }
}

/* Event: Message edited */
void on_message_update(struct discord *client, const struct discord_message *event) {
    (void)client;
    if (!event->guild_id || !event->author || event->author->bot) return;

    char desc[512];
    snprintf(desc, sizeof(desc),
        "Message edited in <#%lu>\n**New content:** %.300s%s",
        (unsigned long)event->channel_id,
        event->content ? event->content : "(empty)",
        (event->content && strlen(event->content) > 300) ? "..." : "");

    activity_logger_add(event->guild_id, event->author->id, event->channel_id,
                         "message_edit", desc);
}

/* Event: Message deleted */
void on_message_delete(struct discord *client, const struct discord_message_delete *event) {
    (void)client;
    if (!event->guild_id) return;

    char desc[256];
    snprintf(desc, sizeof(desc),
        "Message deleted in <#%lu> (ID: %lu)",
        (unsigned long)event->channel_id,
        (unsigned long)event->id);

    activity_logger_add(event->guild_id, 0, event->channel_id,
                         "message_delete", desc);
}

/* Event: User banned */
void on_guild_ban_add(struct discord *client, const struct discord_guild_ban_add *event) {
    (void)client;
    if (!event->user) return;

    char desc[256];
    snprintf(desc, sizeof(desc),
        "**%s** (`%lu`) was banned from the server",
        event->user->username ? event->user->username : "Unknown",
        (unsigned long)event->user->id);

    activity_logger_add(event->guild_id, event->user->id, 0,
                         "ban", desc);
}

/* Event: User unbanned */
void on_guild_ban_remove(struct discord *client, const struct discord_guild_ban_remove *event) {
    (void)client;
    if (!event->user) return;

    char desc[256];
    snprintf(desc, sizeof(desc),
        "**%s** (`%lu`) was unbanned from the server",
        event->user->username ? event->user->username : "Unknown",
        (unsigned long)event->user->id);

    activity_logger_add(event->guild_id, event->user->id, 0,
                         "unban", desc);
}

/* Event: Guild member updated (role/nick changes) */
void on_guild_member_update(struct discord *client, const struct discord_guild_member_update *event) {
    (void)client;
    if (!event->user || event->user->bot) return;

    /* Log nick changes if nick is present */
    if (event->nick) {
        char desc[256];
        snprintf(desc, sizeof(desc),
            "Nickname changed to **%s**",
            event->nick);

        activity_logger_add(event->guild_id, event->user->id, 0,
                             "nickname_change", desc);
    }

    /* Log and persist role changes if roles are present */
    if (event->roles && event->roles->size > 0) {
        char desc[512];
        char *ptr = desc;
        ptr += sprintf(ptr, "Roles updated: ");
        for (int i = 0; i < event->roles->size && i < 10; i++) {
            if (i > 0) ptr += sprintf(ptr, ", ");
            ptr += sprintf(ptr, "<@&%lu>", (unsigned long)event->roles->array[i]);
        }

        activity_logger_add(event->guild_id, event->user->id, 0,
                             "role_change", desc);

        /* Persist roles for auto-role restoration */
        uint64_t role_ids[50];
        int role_count = event->roles->size < 50 ? event->roles->size : 50;
        for (int i = 0; i < role_count; i++) {
            role_ids[i] = event->roles->array[i];
        }
        db_save_user_roles(&g_bot->database, event->guild_id, event->user->id, role_ids, role_count);
    }
}
