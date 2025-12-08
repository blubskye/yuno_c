/*
 * Yuno Gasai 2 (C Edition) - Configuration
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_CONFIG_H
#define YUNO_CONFIG_H

#include <stddef.h>

#define MAX_PREFIX_LEN 16
#define MAX_TOKEN_LEN 256
#define MAX_PATH_LEN 512
#define MAX_MESSAGE_LEN 2048
#define MAX_MASTER_USERS 16

typedef struct {
    char discord_token[MAX_TOKEN_LEN];
    char default_prefix[MAX_PREFIX_LEN];
    char database_path[MAX_PATH_LEN];
    char master_users[MAX_MASTER_USERS][32];
    int master_user_count;
    int spam_max_warnings;
    char ban_default_image[MAX_PATH_LEN];
    char dm_message[MAX_MESSAGE_LEN];
    char insufficient_permissions_message[MAX_MESSAGE_LEN];
} yuno_config_t;

/* Load configuration from JSON file */
int config_load(yuno_config_t *config, const char *path);

/* Load configuration from environment variables */
int config_load_from_env(yuno_config_t *config);

/* Initialize config with defaults */
void config_init_defaults(yuno_config_t *config);

/* Check if user is a master user */
int config_is_master_user(const yuno_config_t *config, const char *user_id);

#endif /* YUNO_CONFIG_H */
