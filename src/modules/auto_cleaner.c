/*
 * Yuno Gasai 2 (C Edition) - Auto Cleaner Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/auto_cleaner.h"
#include "bot.h"
#include <stdio.h>
#include <string.h>

/* Hash function for guild+channel - O(1) lookup */
static inline uint32_t hash_guild_channel(uint64_t guild_id, uint64_t channel_id) {
    uint64_t h = guild_id ^ (channel_id * 2654435761ULL);
    return (uint32_t)(h % CLEANER_HASH_SIZE);
}

int auto_cleaner_init(auto_cleaner_t *cleaner) {
    memset(cleaner, 0, sizeof(auto_cleaner_t));

    /* Initialize hash table buckets to -1 (empty) */
    for (int i = 0; i < CLEANER_HASH_SIZE; i++) {
        cleaner->hash_table[i] = -1;
    }

    /* Initialize free list */
    for (int i = 0; i < MAX_AUTO_CLEAN_CHANNELS - 1; i++) {
        cleaner->delays[i].hash_next = i + 1;
        cleaner->delays[i].in_use = 0;
    }
    cleaner->delays[MAX_AUTO_CLEAN_CHANNELS - 1].hash_next = -1;
    cleaner->free_head = 0;

    return 0;
}

void auto_cleaner_cleanup(auto_cleaner_t *cleaner) {
    cleaner->running = 0;
}

int auto_cleaner_start(auto_cleaner_t *cleaner) {
    cleaner->running = 1;
    printf("🧹 Auto-cleaner started~\n");
    return 0;
}

void auto_cleaner_stop(auto_cleaner_t *cleaner) {
    cleaner->running = 0;
    printf("🧹 Auto-cleaner stopped~\n");
}

/* O(1) lookup using hash table */
static channel_delay_t *find_delay_entry(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    uint32_t bucket = hash_guild_channel(guild_id, channel_id);
    int idx = cleaner->hash_table[bucket];

    while (idx >= 0) {
        channel_delay_t *entry = &cleaner->delays[idx];
        if (entry->in_use && entry->guild_id == guild_id && entry->channel_id == channel_id) {
            return entry;
        }
        idx = entry->hash_next;
    }
    return NULL;
}

/* Allocate new entry from free list */
static channel_delay_t *alloc_delay_entry(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    if (cleaner->free_head < 0) {
        return NULL; /* No space */
    }

    int idx = cleaner->free_head;
    channel_delay_t *entry = &cleaner->delays[idx];
    cleaner->free_head = entry->hash_next;

    /* Initialize entry */
    entry->guild_id = guild_id;
    entry->channel_id = channel_id;
    entry->delay_count = 0;
    entry->delayed_until = 0;
    entry->in_use = 1;

    /* Add to hash table */
    uint32_t bucket = hash_guild_channel(guild_id, channel_id);
    entry->hash_next = cleaner->hash_table[bucket];
    cleaner->hash_table[bucket] = idx;
    cleaner->delay_count++;

    return entry;
}

int auto_cleaner_delay(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id, int minutes) {
    channel_delay_t *entry = find_delay_entry(cleaner, guild_id, channel_id);

    if (entry) {
        /* Check if max delays reached */
        if (entry->delay_count >= MAX_DELAYS_PER_CYCLE) {
            return -1; /* Max delays reached */
        }
        entry->delay_count++;
        entry->delayed_until = time(NULL) + (minutes * 60);
    } else {
        /* Create new entry */
        entry = alloc_delay_entry(cleaner, guild_id, channel_id);
        if (!entry) {
            return -1; /* No space */
        }
        entry->delay_count = 1;
        entry->delayed_until = time(NULL) + (minutes * 60);
    }

    return 0;
}

int auto_cleaner_get_remaining_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    const channel_delay_t *entry = find_delay_entry(cleaner, guild_id, channel_id);
    if (!entry) {
        return MAX_DELAYS_PER_CYCLE;
    }
    return MAX_DELAYS_PER_CYCLE - entry->delay_count;
}

void auto_cleaner_reset_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    channel_delay_t *entry = find_delay_entry(cleaner, guild_id, channel_id);
    if (entry) {
        entry->delay_count = 0;
        entry->delayed_until = 0;
    }
}

void auto_cleaner_check(auto_cleaner_t *cleaner) {
    if (!cleaner->running) return;

    time_t now = time(NULL);

    /* Check all auto-clean configs from database */
    auto_clean_config_t configs[MAX_AUTO_CLEAN_CHANNELS];
    int count;

    if (db_get_all_auto_clean_configs(&g_bot->database, configs, MAX_AUTO_CLEAN_CHANNELS, &count) != 0) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (!configs[i].enabled) continue;

        /* O(1) lookup for delay check */
        const channel_delay_t *entry = find_delay_entry(cleaner, configs[i].guild_id, configs[i].channel_id);
        if (entry && entry->delayed_until > now) {
            continue; /* Still delayed */
        }

        /* Would perform cleaning here */
        /* Reset delays after clean */
        auto_cleaner_reset_delays(cleaner, configs[i].guild_id, configs[i].channel_id);
    }
}
