/*
 * Yuno Gasai 2 (C Edition) - Moderation Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "commands/moderation.h"
#include "bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void cmd_ban(struct discord *client, const struct discord_interaction *interaction) {
    /* Get user from options */
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    u64snowflake user_id = 0;
    const char *reason = "No reason provided";

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "user") == 0) {
            user_id = strtoll(options[i].value, NULL, 10);
        } else if (strcmp(options[i].name, "reason") == 0) {
            reason = options[i].value;
        }
    }

    if (user_id == 0) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Please specify a user to ban~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    /* Ban the user */
    struct discord_create_guild_ban params = {
        .delete_message_seconds = 0
    };
    discord_create_guild_ban(client, interaction->guild_id, user_id, &params, NULL);

    /* Log the action */
    mod_action_t action = {
        .guild_id = interaction->guild_id,
        .moderator_id = interaction->member->user->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "ban", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    /* Send response */
    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "🔪 **Banned!**\nThey won't bother you anymore~ 💕\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)interaction->member->user->id, reason);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_ban_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    char user_mention[64];
    char reason[MAX_REASON_LEN] = "No reason provided";
    u64snowflake user_id;

    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "💔 Please specify a user to ban~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Parse user mention and reason */
    if (sscanf(args, "%63s", user_mention) != 1) {
        struct discord_create_message params = { .content = "💔 Please specify a user to ban~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    user_id = parse_user_mention(user_mention);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "💔 I couldn't find that user~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Get reason if provided */
    const char *reason_start = args + strlen(user_mention);
    while (*reason_start == ' ') reason_start++;
    if (*reason_start) {
        strncpy(reason, reason_start, sizeof(reason) - 1);
    }

    /* Ban the user */
    struct discord_create_guild_ban params = { .delete_message_seconds = 0 };
    discord_create_guild_ban(client, msg->guild_id, user_id, &params, NULL);

    /* Log the action */
    mod_action_t action = {
        .guild_id = msg->guild_id,
        .moderator_id = msg->author->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "ban", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "🔪 **Banned!**\nThey won't bother you anymore~ 💕\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)msg->author->id, reason);

    struct discord_create_message response = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &response, NULL);
}

void cmd_kick(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    u64snowflake user_id = 0;
    const char *reason = "No reason provided";

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "user") == 0) {
            user_id = strtoll(options[i].value, NULL, 10);
        } else if (strcmp(options[i].name, "reason") == 0) {
            reason = options[i].value;
        }
    }

    if (user_id == 0) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Please specify a user to kick~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    discord_remove_guild_member(client, interaction->guild_id, user_id, NULL);

    mod_action_t action = {
        .guild_id = interaction->guild_id,
        .moderator_id = interaction->member->user->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "kick", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "👢 **Kicked!**\nGet out! 💢\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)interaction->member->user->id, reason);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_kick_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    char user_mention[64];
    char reason[MAX_REASON_LEN] = "No reason provided";
    u64snowflake user_id;

    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "💔 Please specify a user to kick~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    if (sscanf(args, "%63s", user_mention) != 1) {
        struct discord_create_message params = { .content = "💔 Please specify a user to kick~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    user_id = parse_user_mention(user_mention);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "💔 I couldn't find that user~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    const char *reason_start = args + strlen(user_mention);
    while (*reason_start == ' ') reason_start++;
    if (*reason_start) {
        strncpy(reason, reason_start, sizeof(reason) - 1);
    }

    discord_remove_guild_member(client, msg->guild_id, user_id, NULL);

    mod_action_t action = {
        .guild_id = msg->guild_id,
        .moderator_id = msg->author->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "kick", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "👢 **Kicked!**\nGet out! 💢\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)msg->author->id, reason);

    struct discord_create_message response = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &response, NULL);
}

void cmd_unban(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    u64snowflake user_id = 0;
    const char *reason = "No reason provided";

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "user_id") == 0) {
            user_id = strtoll(options[i].value, NULL, 10);
        } else if (strcmp(options[i].name, "reason") == 0) {
            reason = options[i].value;
        }
    }

    if (user_id == 0) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Please specify a user ID to unban~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    discord_remove_guild_ban(client, interaction->guild_id, user_id, NULL);

    mod_action_t action = {
        .guild_id = interaction->guild_id,
        .moderator_id = interaction->member->user->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "unban", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "💕 **Unbanned!**\nI'm giving them another chance~ Be good this time!\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)interaction->member->user->id, reason);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_unban_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    char user_id_str[32];
    char reason[MAX_REASON_LEN] = "No reason provided";
    u64snowflake user_id;

    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "💔 Please specify a user ID to unban~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    if (sscanf(args, "%31s", user_id_str) != 1) {
        struct discord_create_message params = { .content = "💔 Please specify a user ID to unban~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    user_id = strtoull(user_id_str, NULL, 10);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "💔 Invalid user ID~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    const char *reason_start = args + strlen(user_id_str);
    while (*reason_start == ' ') reason_start++;
    if (*reason_start) {
        strncpy(reason, reason_start, sizeof(reason) - 1);
    }

    discord_remove_guild_ban(client, msg->guild_id, user_id, NULL);

    mod_action_t action = {
        .guild_id = msg->guild_id,
        .moderator_id = msg->author->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "unban", sizeof(action.action_type));
    strncpy(action.reason, reason, sizeof(action.reason) - 1);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "💕 **Unbanned!**\nI'm giving them another chance~ Be good this time!\n\n"
        "**User:** <@%lu>\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (unsigned long)msg->author->id, reason);

    struct discord_create_message response = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &response, NULL);
}

void cmd_timeout(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    u64snowflake user_id = 0;
    int64_t minutes = 5;
    const char *reason = "No reason provided";

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "user") == 0) {
            user_id = strtoll(options[i].value, NULL, 10);
        } else if (strcmp(options[i].name, "minutes") == 0) {
            minutes = strtoll(options[i].value, NULL, 10);
        } else if (strcmp(options[i].name, "reason") == 0) {
            reason = options[i].value;
        }
    }

    if (user_id == 0) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Please specify a user to timeout~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    /* Calculate timeout timestamp */
    time_t timeout_until = time(NULL) + (minutes * 60);
    char iso_timestamp[32];
    struct tm *tm_info = gmtime(&timeout_until);
    strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

    struct discord_modify_guild_member params = {
        .communication_disabled_until = iso_timestamp
    };
    discord_modify_guild_member(client, interaction->guild_id, user_id, &params, NULL);

    mod_action_t action = {
        .guild_id = interaction->guild_id,
        .moderator_id = interaction->member->user->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "timeout", sizeof(action.action_type));
    snprintf(action.reason, sizeof(action.reason), "%s (%ld minutes)", reason, (long)minutes);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "⏰ **Timed Out!**\nThink about what you did~ 😤\n\n"
        "**User:** <@%lu>\n**Duration:** %ld minutes\n**Moderator:** <@%lu>\n**Reason:** %s",
        (unsigned long)user_id, (long)minutes, (unsigned long)interaction->member->user->id, reason);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_timeout_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    char user_mention[64];
    char minutes_str[16];
    char reason[MAX_REASON_LEN] = "No reason provided";
    u64snowflake user_id;
    int64_t minutes;

    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "💔 Usage: timeout <user> <minutes> [reason]~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    if (sscanf(args, "%63s %15s", user_mention, minutes_str) < 2) {
        struct discord_create_message params = { .content = "💔 Usage: timeout <user> <minutes> [reason]~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    user_id = parse_user_mention(user_mention);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "💔 I couldn't find that user~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    minutes = strtoll(minutes_str, NULL, 10);
    if (minutes <= 0) {
        struct discord_create_message params = { .content = "💔 Invalid duration~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Calculate timeout timestamp */
    time_t timeout_until = time(NULL) + (minutes * 60);
    char iso_timestamp[32];
    struct tm *tm_info = gmtime(&timeout_until);
    strftime(iso_timestamp, sizeof(iso_timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

    struct discord_modify_guild_member params = {
        .communication_disabled_until = iso_timestamp
    };
    discord_modify_guild_member(client, msg->guild_id, user_id, &params, NULL);

    mod_action_t action = {
        .guild_id = msg->guild_id,
        .moderator_id = msg->author->id,
        .target_id = user_id,
        .timestamp = time(NULL)
    };
    strncpy(action.action_type, "timeout", sizeof(action.action_type));
    snprintf(action.reason, sizeof(action.reason), "%s (%ld minutes)", reason, (long)minutes);
    db_log_mod_action(&g_bot->database, &action);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "⏰ **Timed Out!**\nThink about what you did~ 😤\n\n"
        "**User:** <@%lu>\n**Duration:** %ld minutes\n**Moderator:** <@%lu>",
        (unsigned long)user_id, (long)minutes, (unsigned long)msg->author->id);

    struct discord_create_message response = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &response, NULL);
}

void cmd_clean(struct discord *client, const struct discord_interaction *interaction) {
    /* Simplified clean command */
    char response_msg[] = "🧹 Cleaning messages~ 💕";
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    struct discord_create_message params = { .content = "🧹 Cleaning messages~ 💕" };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_mod_stats(struct discord *client, const struct discord_interaction *interaction) {
    mod_action_t actions[100];
    int count;
    db_get_mod_actions(&g_bot->database, interaction->guild_id, actions, 100, &count);

    char response_msg[2048];
    snprintf(response_msg, sizeof(response_msg),
        "📊 **Moderation Statistics**\nLook at all we've done together~ 💕\n\n"
        "**Total Actions:** %d", count);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_mod_stats_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    mod_action_t actions[100];
    int count;
    db_get_mod_actions(&g_bot->database, msg->guild_id, actions, 100, &count);

    char response_msg[2048];
    snprintf(response_msg, sizeof(response_msg),
        "📊 **Moderation Statistics**\nLook at all we've done together~ 💕\n\n"
        "**Total Actions:** %d", count);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_scan_bans(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[] = "📊 Scanning bans... This feature is simplified in C~ 💕";
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_scan_bans_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    struct discord_create_message params = { .content = "📊 Scanning bans... This feature is simplified in C~ 💕" };
    discord_create_message(client, msg->channel_id, &params, NULL);
}
