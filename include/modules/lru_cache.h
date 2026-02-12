/*
 * Yuno Gasai 2 (C Edition) - LRU Cache Layer
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_LRU_CACHE_H
#define YUNO_MODULES_LRU_CACHE_H

#include <stdint.h>
#include <stddef.h>

/* Cache configuration */
#define LRU_MAX_ENTRIES  256
#define LRU_KEY_SIZE      64
#define LRU_VALUE_SIZE  1024

/* Cache entry */
typedef struct lru_entry {
    char key[LRU_KEY_SIZE];
    char value[LRU_VALUE_SIZE];
    size_t value_len;
    int64_t expires_at;  /* Unix timestamp, 0 = no expiry */
    int prev;            /* Index of previous entry in LRU order (-1 = none) */
    int next;            /* Index of next entry in LRU order (-1 = none) */
    int hash_next;       /* Hash chain for collision resolution */
    int in_use;
} lru_entry_t;

/* Hash table for O(1) key lookup */
#define LRU_HASH_SIZE 389

typedef struct {
    lru_entry_t entries[LRU_MAX_ENTRIES];
    int hash_table[LRU_HASH_SIZE];
    int head;            /* Most recently used */
    int tail;            /* Least recently used */
    int count;
    int64_t hits;
    int64_t misses;
} lru_cache_t;

/* Lifecycle */
void lru_cache_init(lru_cache_t *cache);

/* Operations */
int lru_cache_get(lru_cache_t *cache, const char *key, void *out_value, size_t max_len);
void lru_cache_put(lru_cache_t *cache, const char *key, const void *value, size_t value_len, int ttl_seconds);
void lru_cache_invalidate(lru_cache_t *cache, const char *key);
void lru_cache_invalidate_prefix(lru_cache_t *cache, const char *prefix);
void lru_cache_clear(lru_cache_t *cache);

/* Stats */
void lru_cache_stats(const lru_cache_t *cache, int *count, int64_t *hits, int64_t *misses);

#endif /* YUNO_MODULES_LRU_CACHE_H */
