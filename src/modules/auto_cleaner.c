/*
 * Yuno Gasai 2 (C Edition) - Auto Cleaner Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/auto_cleaner.h"
#include "bot.h"
#include <stdio.h>
#include <string.h>

int auto_cleaner_init(auto_cleaner_t *cleaner) {
    memset(cleaner, 0, sizeof(auto_cleaner_t));
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

static int find_delay_entry(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    for (int i = 0; i < cleaner->delay_count; i++) {
        if (cleaner->delays[i].guild_id == guild_id &&
            cleaner->delays[i].channel_id == channel_id) {
            return i;
        }
    }
    return -1;
}

int auto_cleaner_delay(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id, int minutes) {
    int idx = find_delay_entry(cleaner, guild_id, channel_id);

    if (idx >= 0) {
        /* Check if max delays reached */
        if (cleaner->delays[idx].delay_count >= MAX_DELAYS_PER_CYCLE) {
            return -1; /* Max delays reached */
        }
        cleaner->delays[idx].delay_count++;
        cleaner->delays[idx].delayed_until = time(NULL) + (minutes * 60);
    } else {
        /* Create new entry */
        if (cleaner->delay_count >= MAX_AUTO_CLEAN_CHANNELS) {
            return -1; /* No space */
        }
        idx = cleaner->delay_count++;
        cleaner->delays[idx].guild_id = guild_id;
        cleaner->delays[idx].channel_id = channel_id;
        cleaner->delays[idx].delay_count = 1;
        cleaner->delays[idx].delayed_until = time(NULL) + (minutes * 60);
    }

    return 0;
}

int auto_cleaner_get_remaining_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    int idx = find_delay_entry(cleaner, guild_id, channel_id);
    if (idx < 0) {
        return MAX_DELAYS_PER_CYCLE;
    }
    return MAX_DELAYS_PER_CYCLE - cleaner->delays[idx].delay_count;
}

void auto_cleaner_reset_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id) {
    int idx = find_delay_entry(cleaner, guild_id, channel_id);
    if (idx >= 0) {
        cleaner->delays[idx].delay_count = 0;
        cleaner->delays[idx].delayed_until = 0;
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

        /* Check if delayed */
        int idx = find_delay_entry(cleaner, configs[i].guild_id, configs[i].channel_id);
        if (idx >= 0 && cleaner->delays[idx].delayed_until > now) {
            continue; /* Still delayed */
        }

        /* Would perform cleaning here */
        /* Reset delays after clean */
        auto_cleaner_reset_delays(cleaner, configs[i].guild_id, configs[i].channel_id);
    }
}
