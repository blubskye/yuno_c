/*
 * Yuno Gasai 2 (C Edition) - Auto Cleaner Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_AUTO_CLEANER_H
#define YUNO_MODULES_AUTO_CLEANER_H

#include <stdint.h>
#include <time.h>

#define MAX_AUTO_CLEAN_CHANNELS 256
#define MAX_DELAYS_PER_CYCLE 3

typedef struct {
    uint64_t guild_id;
    uint64_t channel_id;
    int delay_count;
    time_t delayed_until;
} channel_delay_t;

typedef struct {
    channel_delay_t delays[MAX_AUTO_CLEAN_CHANNELS];
    int delay_count;
    int running;
} auto_cleaner_t;

/* Auto cleaner lifecycle */
int auto_cleaner_init(auto_cleaner_t *cleaner);
void auto_cleaner_cleanup(auto_cleaner_t *cleaner);
int auto_cleaner_start(auto_cleaner_t *cleaner);
void auto_cleaner_stop(auto_cleaner_t *cleaner);

/* Delay operations */
int auto_cleaner_delay(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id, int minutes);
int auto_cleaner_get_remaining_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id);
void auto_cleaner_reset_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id);

/* Check and perform cleaning */
void auto_cleaner_check(auto_cleaner_t *cleaner);

#endif /* YUNO_MODULES_AUTO_CLEANER_H */
