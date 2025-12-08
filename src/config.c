/*
 * Yuno Gasai 2 (C Edition) - Configuration
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

void config_init_defaults(yuno_config_t *config) {
    memset(config, 0, sizeof(yuno_config_t));
    strcpy(config->default_prefix, ".");
    strcpy(config->database_path, "yuno.db");
    config->spam_max_warnings = 3;
    strcpy(config->dm_message, "I'm just a bot :'(. I can't answer to you.");
    strcpy(config->insufficient_permissions_message, "${author} You don't have permission to do that~");
}

int config_load(yuno_config_t *config, const char *path) {
    FILE *file;
    char *buffer;
    long length;
    struct json_object *root;
    struct json_object *value;

    file = fopen(path, "r");
    if (!file) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return -1;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    root = json_tokener_parse(buffer);
    free(buffer);

    if (!root) {
        return -1;
    }

    /* Parse discord_token */
    if (json_object_object_get_ex(root, "discord_token", &value)) {
        strncpy(config->discord_token, json_object_get_string(value), MAX_TOKEN_LEN - 1);
    }

    /* Parse default_prefix */
    if (json_object_object_get_ex(root, "default_prefix", &value)) {
        strncpy(config->default_prefix, json_object_get_string(value), MAX_PREFIX_LEN - 1);
    }

    /* Parse database_path */
    if (json_object_object_get_ex(root, "database_path", &value)) {
        strncpy(config->database_path, json_object_get_string(value), MAX_PATH_LEN - 1);
    }

    /* Parse master_users array */
    if (json_object_object_get_ex(root, "master_users", &value)) {
        if (json_object_is_type(value, json_type_array)) {
            int len = json_object_array_length(value);
            if (len > MAX_MASTER_USERS) len = MAX_MASTER_USERS;
            config->master_user_count = len;
            for (int i = 0; i < len; i++) {
                struct json_object *item = json_object_array_get_idx(value, i);
                strncpy(config->master_users[i], json_object_get_string(item), 31);
            }
        }
    }

    /* Parse spam_max_warnings */
    if (json_object_object_get_ex(root, "spam_max_warnings", &value)) {
        config->spam_max_warnings = json_object_get_int(value);
    }

    /* Parse ban_default_image */
    if (json_object_object_get_ex(root, "ban_default_image", &value)) {
        if (!json_object_is_type(value, json_type_null)) {
            strncpy(config->ban_default_image, json_object_get_string(value), MAX_PATH_LEN - 1);
        }
    }

    /* Parse dm_message */
    if (json_object_object_get_ex(root, "dm_message", &value)) {
        strncpy(config->dm_message, json_object_get_string(value), MAX_MESSAGE_LEN - 1);
    }

    /* Parse insufficient_permissions_message */
    if (json_object_object_get_ex(root, "insufficient_permissions_message", &value)) {
        strncpy(config->insufficient_permissions_message, json_object_get_string(value), MAX_MESSAGE_LEN - 1);
    }

    json_object_put(root);
    return 0;
}

int config_load_from_env(yuno_config_t *config) {
    const char *token = getenv("DISCORD_TOKEN");
    if (token) {
        strncpy(config->discord_token, token, MAX_TOKEN_LEN - 1);
    }

    const char *prefix = getenv("DEFAULT_PREFIX");
    if (prefix) {
        strncpy(config->default_prefix, prefix, MAX_PREFIX_LEN - 1);
    }

    const char *db_path = getenv("DATABASE_PATH");
    if (db_path) {
        strncpy(config->database_path, db_path, MAX_PATH_LEN - 1);
    }

    const char *spam_warnings = getenv("SPAM_MAX_WARNINGS");
    if (spam_warnings) {
        config->spam_max_warnings = atoi(spam_warnings);
    }

    const char *master = getenv("MASTER_USER");
    if (master) {
        strncpy(config->master_users[0], master, 31);
        config->master_user_count = 1;
    }

    const char *dm_msg = getenv("DM_MESSAGE");
    if (dm_msg) {
        strncpy(config->dm_message, dm_msg, MAX_MESSAGE_LEN - 1);
    }

    return (strlen(config->discord_token) > 0) ? 0 : -1;
}

int config_is_master_user(const yuno_config_t *config, const char *user_id) {
    for (int i = 0; i < config->master_user_count; i++) {
        if (strcmp(config->master_users[i], user_id) == 0) {
            return 1;
        }
    }
    return 0;
}
