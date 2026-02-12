/*
 * Yuno Gasai 2 (C Edition) - LRU Cache Layer
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/lru_cache.h"
#include <string.h>
#include <time.h>

/* DJB2 hash */
static uint32_t cache_hash(const char *key) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % LRU_HASH_SIZE;
}

void lru_cache_init(lru_cache_t *cache) {
    memset(cache, 0, sizeof(lru_cache_t));
    cache->head = -1;
    cache->tail = -1;

    for (int i = 0; i < LRU_HASH_SIZE; i++) {
        cache->hash_table[i] = -1;
    }
    for (int i = 0; i < LRU_MAX_ENTRIES; i++) {
        cache->entries[i].prev = -1;
        cache->entries[i].next = -1;
        cache->entries[i].hash_next = -1;
        cache->entries[i].in_use = 0;
    }
}

/* Remove entry from doubly-linked LRU list */
static void lru_detach(lru_cache_t *cache, int idx) {
    lru_entry_t *e = &cache->entries[idx];
    if (e->prev >= 0) {
        cache->entries[e->prev].next = e->next;
    } else {
        cache->head = e->next;
    }
    if (e->next >= 0) {
        cache->entries[e->next].prev = e->prev;
    } else {
        cache->tail = e->prev;
    }
    e->prev = -1;
    e->next = -1;
}

/* Push entry to front (most recently used) */
static void lru_push_front(lru_cache_t *cache, int idx) {
    lru_entry_t *e = &cache->entries[idx];
    e->prev = -1;
    e->next = cache->head;
    if (cache->head >= 0) {
        cache->entries[cache->head].prev = idx;
    }
    cache->head = idx;
    if (cache->tail < 0) {
        cache->tail = idx;
    }
}

/* Remove entry from hash table */
static void hash_remove(lru_cache_t *cache, int idx) {
    uint32_t bucket = cache_hash(cache->entries[idx].key);
    int prev = -1;
    int cur = cache->hash_table[bucket];

    while (cur >= 0) {
        if (cur == idx) {
            if (prev < 0) {
                cache->hash_table[bucket] = cache->entries[cur].hash_next;
            } else {
                cache->entries[prev].hash_next = cache->entries[cur].hash_next;
            }
            cache->entries[cur].hash_next = -1;
            return;
        }
        prev = cur;
        cur = cache->entries[cur].hash_next;
    }
}

/* Find a free slot or evict LRU */
static int lru_alloc(lru_cache_t *cache) {
    /* Find free slot */
    for (int i = 0; i < LRU_MAX_ENTRIES; i++) {
        if (!cache->entries[i].in_use) {
            return i;
        }
    }

    /* Evict LRU (tail) */
    int victim = cache->tail;
    if (victim >= 0) {
        lru_detach(cache, victim);
        hash_remove(cache, victim);
        cache->entries[victim].in_use = 0;
        cache->count--;
    }
    return victim;
}

/* Find entry by key */
static int lru_find(lru_cache_t *cache, const char *key) {
    uint32_t bucket = cache_hash(key);
    int idx = cache->hash_table[bucket];

    while (idx >= 0) {
        if (cache->entries[idx].in_use && strcmp(cache->entries[idx].key, key) == 0) {
            return idx;
        }
        idx = cache->entries[idx].hash_next;
    }
    return -1;
}

int lru_cache_get(lru_cache_t *cache, const char *key, void *out_value, size_t max_len) {
    int idx = lru_find(cache, key);
    if (idx < 0) {
        cache->misses++;
        return -1;
    }

    lru_entry_t *e = &cache->entries[idx];

    /* Check TTL */
    if (e->expires_at > 0 && time(NULL) > e->expires_at) {
        /* Expired - remove */
        lru_detach(cache, idx);
        hash_remove(cache, idx);
        e->in_use = 0;
        cache->count--;
        cache->misses++;
        return -1;
    }

    /* Move to front (most recently used) */
    lru_detach(cache, idx);
    lru_push_front(cache, idx);

    /* Copy value */
    size_t copy_len = e->value_len < max_len ? e->value_len : max_len;
    memcpy(out_value, e->value, copy_len);

    cache->hits++;
    return (int)copy_len;
}

void lru_cache_put(lru_cache_t *cache, const char *key, const void *value, size_t value_len, int ttl_seconds) {
    if (value_len > LRU_VALUE_SIZE) {
        value_len = LRU_VALUE_SIZE;
    }

    /* Check if key already exists */
    int idx = lru_find(cache, key);
    if (idx >= 0) {
        /* Update existing entry */
        lru_entry_t *e = &cache->entries[idx];
        memcpy(e->value, value, value_len);
        e->value_len = value_len;
        e->expires_at = ttl_seconds > 0 ? time(NULL) + ttl_seconds : 0;

        /* Move to front */
        lru_detach(cache, idx);
        lru_push_front(cache, idx);
        return;
    }

    /* Allocate new entry */
    idx = lru_alloc(cache);
    if (idx < 0) return;

    lru_entry_t *e = &cache->entries[idx];
    strncpy(e->key, key, LRU_KEY_SIZE - 1);
    e->key[LRU_KEY_SIZE - 1] = '\0';
    memcpy(e->value, value, value_len);
    e->value_len = value_len;
    e->expires_at = ttl_seconds > 0 ? time(NULL) + ttl_seconds : 0;
    e->in_use = 1;

    /* Add to hash table */
    uint32_t bucket = cache_hash(key);
    e->hash_next = cache->hash_table[bucket];
    cache->hash_table[bucket] = idx;

    /* Add to front of LRU list */
    lru_push_front(cache, idx);
    cache->count++;
}

void lru_cache_invalidate(lru_cache_t *cache, const char *key) {
    int idx = lru_find(cache, key);
    if (idx < 0) return;

    lru_detach(cache, idx);
    hash_remove(cache, idx);
    cache->entries[idx].in_use = 0;
    cache->count--;
}

void lru_cache_invalidate_prefix(lru_cache_t *cache, const char *prefix) {
    size_t prefix_len = strlen(prefix);

    for (int i = 0; i < LRU_MAX_ENTRIES; i++) {
        if (cache->entries[i].in_use && strncmp(cache->entries[i].key, prefix, prefix_len) == 0) {
            lru_detach(cache, i);
            hash_remove(cache, i);
            cache->entries[i].in_use = 0;
            cache->count--;
        }
    }
}

void lru_cache_clear(lru_cache_t *cache) {
    int64_t hits = cache->hits;
    int64_t misses = cache->misses;
    lru_cache_init(cache);
    cache->hits = hits;
    cache->misses = misses;
}

void lru_cache_stats(const lru_cache_t *cache, int *count, int64_t *hits, int64_t *misses) {
    if (count) *count = cache->count;
    if (hits) *hits = cache->hits;
    if (misses) *misses = cache->misses;
}
