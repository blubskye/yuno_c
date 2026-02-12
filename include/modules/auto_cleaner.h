/*
 * Yuno Gasai 2 (C Edition) - Auto Cleaner Module
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_AUTO_CLEANER_H
#define YUNO_MODULES_AUTO_CLEANER_H

#include <stdint.h>
#include <time.h>

/* Forward declaration */
struct discord;

#define MAX_AUTO_CLEAN_CHANNELS 256
#define MAX_DELAYS_PER_CYCLE 3
#define CLEANER_HASH_SIZE 389  /* Prime number > MAX_AUTO_CLEAN_CHANNELS */

typedef struct {
    uint64_t guild_id;
    uint64_t channel_id;
    int delay_count;
    time_t delayed_until;
    int hash_next;  /* Chain for hash collisions */
    int in_use;     /* 1 if entry is active */
} channel_delay_t;

typedef struct {
    channel_delay_t delays[MAX_AUTO_CLEAN_CHANNELS];
    int hash_table[CLEANER_HASH_SIZE]; /* Hash buckets -> index, -1 = empty */
    int delay_count;
    int free_head;  /* Head of free list */
    int running;
    struct discord *client;   /* Concord client for timer */
    unsigned timer_id;        /* Concord timer handle */
    int64_t last_check;       /* Last time we decremented remaining_time */
} auto_cleaner_t;

/* Auto cleaner lifecycle */
int auto_cleaner_init(auto_cleaner_t *cleaner);
void auto_cleaner_cleanup(auto_cleaner_t *cleaner);
int auto_cleaner_start(auto_cleaner_t *cleaner, struct discord *client);
void auto_cleaner_stop(auto_cleaner_t *cleaner);

/* Delay operations */
int auto_cleaner_delay(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id, int minutes);
int auto_cleaner_get_remaining_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id);
void auto_cleaner_reset_delays(auto_cleaner_t *cleaner, uint64_t guild_id, uint64_t channel_id);

/* Check and perform cleaning */
void auto_cleaner_check(auto_cleaner_t *cleaner);

#endif /* YUNO_MODULES_AUTO_CLEANER_H */
