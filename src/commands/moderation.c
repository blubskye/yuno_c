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
#include <limits.h>

/* Helper to get an option value from slash command args */
static const char *get_option_value(const struct discord_interaction *interaction, const char *name) {
    if (!interaction->data->options) return NULL;
    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(interaction->data->options->array[i].name, name) == 0) {
            return interaction->data->options->array[i].value;
        }
    }
    return NULL;
}

void cmd_ban(struct discord *client, const struct discord_interaction *interaction) {
    const char *user_val = get_option_value(interaction, "user");
    const char *reason_val = get_option_value(interaction, "reason");
    u64snowflake user_id = user_val ? strtoll(user_val, NULL, 10) : 0;
    const char *reason = reason_val ? reason_val : "No reason provided";

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
        .delete_message_days = 0
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
    struct discord_create_guild_ban params = { .delete_message_days = 0 };
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
    const char *user_val = get_option_value(interaction, "user");
    const char *reason_val = get_option_value(interaction, "reason");
    u64snowflake user_id = user_val ? strtoll(user_val, NULL, 10) : 0;
    const char *reason = reason_val ? reason_val : "No reason provided";

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

    discord_remove_guild_member(client, interaction->guild_id, user_id, NULL, NULL);

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

    discord_remove_guild_member(client, msg->guild_id, user_id, NULL, NULL);

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
    const char *user_val = get_option_value(interaction, "user_id");
    const char *reason_val = get_option_value(interaction, "reason");
    u64snowflake user_id = user_val ? strtoll(user_val, NULL, 10) : 0;
    const char *reason = reason_val ? reason_val : "No reason provided";

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

    discord_remove_guild_ban(client, interaction->guild_id, user_id, NULL, NULL);

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

    discord_remove_guild_ban(client, msg->guild_id, user_id, NULL, NULL);

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
    const char *user_val = get_option_value(interaction, "user");
    const char *minutes_val = get_option_value(interaction, "minutes");
    const char *reason_val = get_option_value(interaction, "reason");
    u64snowflake user_id = user_val ? strtoll(user_val, NULL, 10) : 0;
    int64_t minutes = minutes_val ? strtoll(minutes_val, NULL, 10) : 5;
    const char *reason = reason_val ? reason_val : "No reason provided";

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

    /* Calculate timeout timestamp (milliseconds since epoch) */
    uint64_t timeout_until_ms = ((uint64_t)time(NULL) + (uint64_t)(minutes * 60)) * 1000ULL;

    struct discord_modify_guild_member params = {
        .communication_disabled_until = timeout_until_ms
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

    /* Calculate timeout timestamp (milliseconds since epoch) */
    uint64_t timeout_until_ms = ((uint64_t)time(NULL) + (uint64_t)(minutes * 60)) * 1000ULL;

    struct discord_modify_guild_member params2 = {
        .communication_disabled_until = timeout_until_ms
    };
    discord_modify_guild_member(client, msg->guild_id, user_id, &params2, NULL);

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

/* Clone a channel, delete the old one, restore position/nsfw. Returns new channel ID or 0 on failure. */
u64snowflake clean_channel(struct discord *client, u64snowflake guild_id, u64snowflake channel_id) {
    /* Fetch old channel info */
    struct discord_channel old_ch = { 0 };
    struct discord_ret_channel get_ret = { .sync = &old_ch };
    if (discord_get_channel(client, channel_id, &get_ret) != CCORD_OK) {
        return 0;
    }

    /* Create clone with same properties */
    struct discord_create_guild_channel create_params = {
        .reason = "Cleaning by Yuno.",
        .name = old_ch.name,
        .type = old_ch.type,
        .topic = old_ch.topic,
        .nsfw = old_ch.nsfw,
        .position = old_ch.position,
        .parent_id = old_ch.parent_id,
        .permission_overwrites = old_ch.permission_overwrites,
        .rate_limit_per_user = old_ch.rate_limit_per_user,
    };

    struct discord_channel new_ch = { 0 };
    struct discord_ret_channel create_ret = { .sync = &new_ch };
    if (discord_create_guild_channel(client, guild_id, &create_params, &create_ret) != CCORD_OK) {
        discord_channel_cleanup(&old_ch);
        return 0;
    }

    u64snowflake new_id = new_ch.id;

    /* Delete old channel */
    discord_delete_channel(client, channel_id, NULL, NULL);

    /* Send completion message in new channel */
    struct discord_create_message done_msg = {
        .content = "🧹 **Channel cleaned!**\n*Yuno is done cleaning~* 💕"
    };
    discord_create_message(client, new_id, &done_msg, NULL);

    discord_channel_cleanup(&old_ch);
    discord_channel_cleanup(&new_ch);
    return new_id;
}

void cmd_clean(struct discord *client, const struct discord_interaction *interaction) {
    /* Clean the current channel */
    u64snowflake new_id = clean_channel(client, interaction->guild_id, interaction->channel_id);
    if (new_id == 0) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Failed to clean channel~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
    }
    /* If successful, the old channel (and interaction) is deleted, so no response needed */
}

void cmd_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;

    /* Send warning first */
    struct discord_create_message warn = {
        .content = "🧹 **Cleaning this channel in 5 seconds...** Speak now or forever hold your peace~"
    };
    discord_create_message(client, msg->channel_id, &warn, NULL);

    /* Note: In a real async environment we'd sleep(5) here.
       Concord is single-threaded so we clean immediately for now. */
    clean_channel(client, msg->guild_id, msg->channel_id);
}

static void build_mod_stats_message(char *buf, size_t buf_size, u64snowflake guild_id, u64snowflake requester_id) {
    int ban_count, kick_count, timeout_count, unban_count;
    db_get_mod_stats_breakdown(&g_bot->database, guild_id, &ban_count, &kick_count, &timeout_count, &unban_count);

    int total = ban_count + kick_count + timeout_count + unban_count;

    /* Get per-moderator stats for the requester */
    int my_bans, my_kicks, my_timeouts;
    db_get_mod_stats(&g_bot->database, guild_id, requester_id, &my_bans, &my_kicks, &my_timeouts);
    int my_total = my_bans + my_kicks + my_timeouts;

    snprintf(buf, buf_size,
        "📊 **Moderation Statistics** 💕\n\n"
        "**Server Totals:**\n"
        "🔪 Bans: **%d**\n"
        "👢 Kicks: **%d**\n"
        "⏰ Timeouts: **%d**\n"
        "💕 Unbans: **%d**\n"
        "📋 Total: **%d**\n\n"
        "**Your Stats:**\n"
        "🔪 Bans: **%d**\n"
        "👢 Kicks: **%d**\n"
        "⏰ Timeouts: **%d**\n"
        "📋 Total: **%d**",
        ban_count, kick_count, timeout_count, unban_count, total,
        my_bans, my_kicks, my_timeouts, my_total);
}

void cmd_mod_stats(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[2048];
    build_mod_stats_message(response_msg, sizeof(response_msg),
                            interaction->guild_id, interaction->member->user->id);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_mod_stats_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    char response_msg[2048];
    build_mod_stats_message(response_msg, sizeof(response_msg),
                            msg->guild_id, msg->author->id);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

static void scan_bans_impl(struct discord *client, u64snowflake guild_id, u64snowflake channel_id) {
    /* Fetch all guild bans */
    struct discord_bans bans = { 0 };
    struct discord_ret_bans ret = { .sync = &bans };

    if (discord_get_guild_bans(client, guild_id, &ret) != CCORD_OK) {
        struct discord_create_message err = { .content = "Failed to fetch guild ban list. Make sure I have Ban Members permission~" };
        discord_create_message(client, channel_id, &err, NULL);
        return;
    }

    int imported = 0, skipped = 0;

    for (int i = 0; i < bans.size; i++) {
        struct discord_ban *ban = &bans.array[i];
        if (!ban->user) continue;

        /* Check if this ban is already in the database */
        if (db_mod_action_exists(&g_bot->database, guild_id, ban->user->id, "ban")) {
            skipped++;
            continue;
        }

        /* Import as mod action */
        mod_action_t action = {
            .guild_id = guild_id,
            .moderator_id = 0, /* Unknown from ban list */
            .target_id = ban->user->id,
            .timestamp = time(NULL)
        };
        strncpy(action.action_type, "ban", sizeof(action.action_type));
        strncpy(action.reason, ban->reason ? ban->reason : "Imported from ban list", sizeof(action.reason) - 1);
        db_log_mod_action(&g_bot->database, &action);
        imported++;
    }

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "📊 **Ban Scan Complete!** 💕\n\n"
        "**Total bans found:** %d\n"
        "**Imported:** %d\n"
        "**Already in database:** %d",
        bans.size, imported, skipped);

    struct discord_create_message done = { .content = response_msg };
    discord_create_message(client, channel_id, &done, NULL);

    discord_bans_cleanup(&bans);
}

void cmd_scan_bans(struct discord *client, const struct discord_interaction *interaction) {
    /* Acknowledge first since this can take a while */
    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = "📊 Scanning bans..." }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    scan_bans_impl(client, interaction->guild_id, interaction->channel_id);
}

void cmd_scan_bans_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    struct discord_create_message notify = { .content = "📊 Scanning bans... please wait~ 💕" };
    discord_create_message(client, msg->channel_id, &notify, NULL);

    scan_bans_impl(client, msg->guild_id, msg->channel_id);
}

/* --- Export Bans --- */
void cmd_export_bans(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_bans bans = { 0 };
    struct discord_ret_bans ret = { .sync = &bans };

    if (discord_get_guild_bans(client, interaction->guild_id, &ret) != CCORD_OK) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "Failed to fetch ban list~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    /* Write bans to file as JSON array of user IDs */
    char filename[64];
    snprintf(filename, sizeof(filename), "BANS-%lu.txt", (unsigned long)interaction->guild_id);

    FILE *f = fopen(filename, "w");
    if (!f) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "Failed to create export file~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        discord_bans_cleanup(&bans);
        return;
    }

    fprintf(f, "[");
    for (int i = 0; i < bans.size; i++) {
        if (!bans.array[i].user) continue;
        if (i > 0) fprintf(f, ",");
        fprintf(f, "\"%lu\"", (unsigned long)bans.array[i].user->id);
    }
    fprintf(f, "]");
    fclose(f);

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "📤 **Exported %d bans** to `%s` 💕",
        bans.size, filename);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);

    discord_bans_cleanup(&bans);
}

void cmd_export_bans_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;

    struct discord_bans bans = { 0 };
    struct discord_ret_bans ret = { .sync = &bans };

    if (discord_get_guild_bans(client, msg->guild_id, &ret) != CCORD_OK) {
        struct discord_create_message err = { .content = "Failed to fetch ban list~" };
        discord_create_message(client, msg->channel_id, &err, NULL);
        return;
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "BANS-%lu.txt", (unsigned long)msg->guild_id);

    FILE *f = fopen(filename, "w");
    if (!f) {
        struct discord_create_message err = { .content = "Failed to create export file~" };
        discord_create_message(client, msg->channel_id, &err, NULL);
        discord_bans_cleanup(&bans);
        return;
    }

    fprintf(f, "[");
    for (int i = 0; i < bans.size; i++) {
        if (!bans.array[i].user) continue;
        if (i > 0) fprintf(f, ",");
        fprintf(f, "\"%lu\"", (unsigned long)bans.array[i].user->id);
    }
    fprintf(f, "]");
    fclose(f);

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "📤 **Exported %d bans** to `%s` 💕",
        bans.size, filename);

    struct discord_create_message response = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &response, NULL);

    discord_bans_cleanup(&bans);
}

/* --- Import Bans --- */
static int import_bans_from_file(struct discord *client, u64snowflake guild_id, const char *filename,
                                  int *imported, int *failed) {
    *imported = 0;
    *failed = 0;

    FILE *f = fopen(filename, "r");
    if (!f) return -1;

    /* Read entire file */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Check for ftell error (negative) and size limits */
    if (fsize < 0) {
        fclose(f);
        return -1;
    }
    if (fsize <= 2 || fsize > 10 * 1024 * 1024) {
        fclose(f);
        return -1;
    }

    /* Check for overflow in malloc size calculation */
    if (fsize >= LONG_MAX) {
        fclose(f);
        return -1;
    }

    char *buf = malloc((size_t)fsize + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, fsize, f);
    buf[fsize] = '\0';
    fclose(f);

    /* Parse JSON array of user ID strings: ["123","456",...] */
    char *p = buf;
    while (*p && *p != '[') p++;
    if (*p == '[') p++;

    while (*p) {
        /* Skip whitespace and commas */
        while (*p == ' ' || *p == ',' || *p == '\n' || *p == '\r' || *p == '\t') p++;
        if (*p == ']' || *p == '\0') break;

        /* Extract quoted ID */
        if (*p == '"') {
            p++;
            char *end = strchr(p, '"');
            if (!end) break;
            *end = '\0';

            u64snowflake user_id = strtoull(p, NULL, 10);
            if (user_id > 0) {
                struct discord_create_guild_ban params = { .delete_message_days = 0 };
                CCORDcode code = discord_create_guild_ban(client, guild_id, user_id, &params, NULL);
                if (code == CCORD_OK) {
                    (*imported)++;
                } else {
                    (*failed)++;
                }
            }
            p = end + 1;
        } else {
            p++;
        }
    }

    free(buf);
    return 0;
}

void cmd_import_bans(struct discord *client, const struct discord_interaction *interaction) {
    const char *guild_id_val = get_option_value(interaction, "guild_id");

    if (!guild_id_val) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "Usage: `/importbans <guild_id>` — imports bans from BANS-<guild_id>.txt~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    /* Validate guild_id is numeric only (prevent path traversal) */
    for (const char *c = guild_id_val; *c; c++) {
        if (*c < '0' || *c > '9') {
            struct discord_interaction_response response = {
                .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
                .data = &(struct discord_interaction_callback_data){
                    .content = "Invalid guild ID~",
                    .flags = DISCORD_MESSAGE_EPHEMERAL
                }
            };
            discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
            return;
        }
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "BANS-%s.txt", guild_id_val);

    /* Acknowledge - this may take a while */
    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = "📥 Importing bans..." }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    int imported = 0, failed = 0;
    if (import_bans_from_file(client, interaction->guild_id, filename, &imported, &failed) != 0) {
        struct discord_create_message err = {
            .content = "Failed to read ban file. Make sure the file exists~"
        };
        discord_create_message(client, interaction->channel_id, &err, NULL);
        return;
    }

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "📥 **Import Complete!** 💕\n\n"
        "**Imported:** %d\n"
        "**Failed:** %d",
        imported, failed);

    struct discord_create_message done = { .content = response_msg };
    discord_create_message(client, interaction->channel_id, &done, NULL);
}

void cmd_import_bans_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message params = {
            .content = "Usage: `importbans <guild_id>` — imports bans from BANS-<guild_id>.txt~"
        };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    char guild_id_str[32];
    if (sscanf(args, "%31s", guild_id_str) != 1) {
        struct discord_create_message params = { .content = "Invalid guild ID~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Validate numeric */
    for (const char *c = guild_id_str; *c; c++) {
        if (*c < '0' || *c > '9') {
            struct discord_create_message params = { .content = "Invalid guild ID~" };
            discord_create_message(client, msg->channel_id, &params, NULL);
            return;
        }
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "BANS-%s.txt", guild_id_str);

    struct discord_create_message notify = { .content = "📥 Importing bans... please wait~ 💕" };
    discord_create_message(client, msg->channel_id, &notify, NULL);

    int imported = 0, failed = 0;
    if (import_bans_from_file(client, msg->guild_id, filename, &imported, &failed) != 0) {
        struct discord_create_message err = { .content = "Failed to read ban file. Make sure the file exists~" };
        discord_create_message(client, msg->channel_id, &err, NULL);
        return;
    }

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "📥 **Import Complete!** 💕\n\n"
        "**Imported:** %d\n"
        "**Failed:** %d",
        imported, failed);

    struct discord_create_message done = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &done, NULL);
}

/* ===== Phase 9.3: Ban Image Commands ===== */

void cmd_set_banimage(struct discord *client, const struct discord_interaction *interaction) {
    const char *user_val = get_option_value(interaction, "user");
    const char *url_val = get_option_value(interaction, "url");

    if (!user_val || !url_val) {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Usage: /set-banimage <user> <url>" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
        return;
    }

    uint64_t user_id = strtoull(user_val, NULL, 10);
    db_set_ban_image(&g_bot->database, interaction->guild_id, user_id, url_val);

    char msg[256];
    snprintf(msg, sizeof(msg), "🖼️ Ban image set for <@%lu>~ 💕", (unsigned long)user_id);
    struct discord_interaction_response resp = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
}

void cmd_del_banimage(struct discord *client, const struct discord_interaction *interaction) {
    const char *user_val = get_option_value(interaction, "user");
    if (!user_val) {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Usage: /del-banimage <user>" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
        return;
    }

    uint64_t user_id = strtoull(user_val, NULL, 10);
    if (db_remove_ban_image(&g_bot->database, interaction->guild_id, user_id) == 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "🗑️ Ban image removed for <@%lu>~ 💕", (unsigned long)user_id);
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = msg }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
    } else {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "No ban image found for that user~" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
    }
}

void cmd_set_banimage_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message p = { .content = "Usage: set-banimage <user> <url>" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    char args_buf[512];
    strncpy(args_buf, args, sizeof(args_buf) - 1);
    args_buf[sizeof(args_buf) - 1] = '\0';

    char *saveptr;
    char *user_str = strtok_r(args_buf, " ", &saveptr);
    char *url = strtok_r(NULL, " ", &saveptr);

    if (!user_str || !url) {
        struct discord_create_message p = { .content = "Usage: set-banimage <user> <url>" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(user_str);
    if (user_id == 0) {
        struct discord_create_message p = { .content = "Invalid user~" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    db_set_ban_image(&g_bot->database, msg->guild_id, user_id, url);

    char resp[256];
    snprintf(resp, sizeof(resp), "🖼️ Ban image set for <@%lu>~ 💕", (unsigned long)user_id);
    struct discord_create_message p = { .content = resp };
    discord_create_message(client, msg->channel_id, &p, NULL);
}

void cmd_del_banimage_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message p = { .content = "Usage: del-banimage <user>" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(args);
    if (user_id == 0) {
        struct discord_create_message p = { .content = "Invalid user~" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    if (db_remove_ban_image(&g_bot->database, msg->guild_id, user_id) == 0) {
        char resp[256];
        snprintf(resp, sizeof(resp), "🗑️ Ban image removed for <@%lu>~ 💕", (unsigned long)user_id);
        struct discord_create_message p = { .content = resp };
        discord_create_message(client, msg->channel_id, &p, NULL);
    } else {
        struct discord_create_message p = { .content = "No ban image found for that user~" };
        discord_create_message(client, msg->channel_id, &p, NULL);
    }
}

/* ===== Phase 9.4: Custom Spam Rule Commands ===== */

static const char *action_names[] = { "warn", "delete", "timeout", "ban" };

void cmd_add_spamrule(struct discord *client, const struct discord_interaction *interaction) {
    const char *pattern = get_option_value(interaction, "pattern");
    const char *action_val = get_option_value(interaction, "action");

    if (!pattern) {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Usage: /add-spamrule <pattern> [action]" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
        return;
    }

    int action = 0; /* default: warn */
    if (action_val) {
        for (int i = 0; i < 4; i++) {
            if (strcmp(action_val, action_names[i]) == 0) { action = i; break; }
        }
    }

    spam_rule_t rule = { .guild_id = interaction->guild_id, .action = action, .enabled = 1 };
    strncpy(rule.pattern, pattern, sizeof(rule.pattern) - 1);
    db_add_spam_rule(&g_bot->database, &rule);

    char msg[256];
    snprintf(msg, sizeof(msg), "✅ Spam rule added: `%s` → **%s** 💕", pattern, action_names[action]);
    struct discord_interaction_response resp = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
}

void cmd_del_spamrule(struct discord *client, const struct discord_interaction *interaction) {
    const char *id_val = get_option_value(interaction, "id");
    if (!id_val) {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Usage: /del-spamrule <id>" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
        return;
    }

    int64_t rule_id = strtoll(id_val, NULL, 10);
    if (db_remove_spam_rule(&g_bot->database, interaction->guild_id, rule_id) == 0) {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "✅ Spam rule removed~ 💕" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
    } else {
        struct discord_interaction_response resp = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Rule not found~" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &resp, NULL);
    }
}

void cmd_spamrules(struct discord *client, const struct discord_interaction *interaction) {
    spam_rule_t rules[20];
    int count = 0;
    db_get_spam_rules(&g_bot->database, interaction->guild_id, rules, 20, &count);

    char resp[2048];
    char *ptr = resp;
    size_t remaining = sizeof(resp);
    int written;

    written = snprintf(ptr, remaining, "🛡️ **Custom Spam Rules** (%d total)\n\n", count);
    if (written < 0 || (size_t)written >= remaining) goto buffer_full;
    ptr += written;
    remaining -= written;

    if (count == 0) {
        written = snprintf(ptr, remaining, "No custom spam rules configured~");
        if (written < 0 || (size_t)written >= remaining) goto buffer_full;
        ptr += written;
        remaining -= written;
    } else {
        for (int i = 0; i < count; i++) {
            const char *act = (rules[i].action >= 0 && rules[i].action < 4) ? action_names[rules[i].action] : "warn";
            written = snprintf(ptr, remaining, "**#%ld** `%s` → **%s** %s\n",
                (long)rules[i].id, rules[i].pattern, act,
                rules[i].enabled ? "✅" : "❌");
            if (written < 0 || (size_t)written >= remaining) {
                /* Buffer full - truncate gracefully */
                snprintf(ptr, remaining, "\n... (%d more)", count - i);
                break;
            }
            ptr += written;
            remaining -= written;
        }
    }

buffer_full:

    struct discord_interaction_response r = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = resp }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &r, NULL);
}

void cmd_add_spamrule_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message p = { .content = "Usage: add-spamrule <pattern> [action: warn|delete|timeout|ban]" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    if (!bot_is_master_user(g_bot, msg->author->id)) {
        struct discord_create_message p = { .content = g_bot->config.insufficient_permissions_message };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    char args_buf[512];
    strncpy(args_buf, args, sizeof(args_buf) - 1);
    args_buf[sizeof(args_buf) - 1] = '\0';

    char *saveptr;
    char *pattern = strtok_r(args_buf, " ", &saveptr);
    char *action_str = strtok_r(NULL, " ", &saveptr);

    int action = 0;
    if (action_str) {
        for (int i = 0; i < 4; i++) {
            if (strcmp(action_str, action_names[i]) == 0) { action = i; break; }
        }
    }

    spam_rule_t rule = { .guild_id = msg->guild_id, .action = action, .enabled = 1 };
    strncpy(rule.pattern, pattern, sizeof(rule.pattern) - 1);
    db_add_spam_rule(&g_bot->database, &rule);

    char resp[256];
    snprintf(resp, sizeof(resp), "✅ Spam rule added: `%s` → **%s** 💕", pattern, action_names[action]);
    struct discord_create_message p = { .content = resp };
    discord_create_message(client, msg->channel_id, &p, NULL);
}

void cmd_del_spamrule_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message p = { .content = "Usage: del-spamrule <id>" };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    if (!bot_is_master_user(g_bot, msg->author->id)) {
        struct discord_create_message p = { .content = g_bot->config.insufficient_permissions_message };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    int64_t rule_id = strtoll(args, NULL, 10);
    if (db_remove_spam_rule(&g_bot->database, msg->guild_id, rule_id) == 0) {
        struct discord_create_message p = { .content = "✅ Spam rule removed~ 💕" };
        discord_create_message(client, msg->channel_id, &p, NULL);
    } else {
        struct discord_create_message p = { .content = "Rule not found~" };
        discord_create_message(client, msg->channel_id, &p, NULL);
    }
}

void cmd_spamrules_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        struct discord_create_message p = { .content = g_bot->config.insufficient_permissions_message };
        discord_create_message(client, msg->channel_id, &p, NULL);
        return;
    }

    spam_rule_t rules[20];
    int count = 0;
    db_get_spam_rules(&g_bot->database, msg->guild_id, rules, 20, &count);

    char resp[2048];
    char *ptr = resp;
    ptr += sprintf(ptr, "🛡️ **Custom Spam Rules** (%d total)\n\n", count);

    if (count == 0) {
        ptr += sprintf(ptr, "No custom spam rules configured~");
    } else {
        for (int i = 0; i < count && (ptr - resp) < 1800; i++) {
            const char *act = (rules[i].action >= 0 && rules[i].action < 4) ? action_names[rules[i].action] : "warn";
            ptr += sprintf(ptr, "**#%ld** `%s` → **%s** %s\n",
                (long)rules[i].id, rules[i].pattern, act,
                rules[i].enabled ? "✅" : "❌");
        }
    }

    struct discord_create_message p = { .content = resp };
    discord_create_message(client, msg->channel_id, &p, NULL);
}
