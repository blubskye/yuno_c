/*
 * Yuno Gasai 2 (C Edition) - Utility Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "commands/utility.h"
#include "bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

void cmd_ping(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[] = "💓 **Pong!**\nI'm always here for you~ 💕";
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_ping_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    struct discord_create_message params = { .content = "💓 **Pong!**\nI'm always here for you~ 💕" };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_help(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[] =
        "💕 **Yuno's Commands** 💕\n"
        "*\"Let me show you everything I can do for you~\"* 💗\n\n"
        "**🔪 Moderation**\n"
        "`/ban` - Ban a user\n"
        "`/kick` - Kick a user\n"
        "`/unban` - Unban a user\n"
        "`/timeout` - Timeout a user\n"
        "`/clean` - Delete messages\n"
        "`/mod-stats` - View moderation stats\n\n"
        "**⚙️ Utility**\n"
        "`/ping` - Check latency\n"
        "`/prefix` - Set server prefix\n"
        "`/auto-clean` - Configure auto-clean\n"
        "`/delay` - Delay auto-clean\n"
        "`/source` - View source code\n"
        "`/help` - This menu\n\n"
        "**✨ Leveling**\n"
        "`/xp` - Check XP and level\n"
        "`/leaderboard` - Server rankings\n\n"
        "**🎱 Fun**\n"
        "`/8ball` - Ask the magic 8-ball\n\n"
        "💕 *Yuno is always watching over you~* 💕";

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_help_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    char prefix[MAX_PREFIX_LEN];
    db_get_prefix(&g_bot->database, msg->guild_id, g_bot->config.default_prefix, prefix, sizeof(prefix));

    char response_msg[2048];
    snprintf(response_msg, sizeof(response_msg),
        "💕 **Yuno's Commands** 💕\n"
        "*\"Let me show you everything I can do for you~\"* 💗\n"
        "Prefix: `%s`\n\n"
        "**🔪 Moderation**\n"
        "`ban` - Ban a user\n"
        "`kick` - Kick a user\n"
        "`unban` - Unban a user\n"
        "`timeout` - Timeout a user\n"
        "`clean` - Delete messages\n"
        "`mod-stats` - View moderation stats\n\n"
        "**⚙️ Utility**\n"
        "`ping` - Check latency\n"
        "`prefix` - Set server prefix\n"
        "`delay` - Delay auto-clean\n"
        "`source` - View source code\n"
        "`help` - This menu\n\n"
        "**✨ Leveling**\n"
        "`xp` - Check XP and level\n"
        "`leaderboard` - Server rankings\n\n"
        "**🎱 Fun**\n"
        "`8ball` - Ask the magic 8-ball\n\n"
        "💕 *Yuno is always watching over you~* 💕", prefix);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_source(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[] =
        "📜 **Source Code**\n"
        "*\"I have nothing to hide from you~\"* 💕\n\n"
        "**C Version**: https://github.com/blubskye/yuno_c\n"
        "**C++ Version**: https://github.com/blubskye/yuno_cpp\n"
        "**Rust Version**: https://github.com/blubskye/yuno_rust\n"
        "**Original JS**: https://github.com/japaneseenrichmentorganization/Yuno-Gasai-2\n\n"
        "Licensed under **AGPL-3.0** 💗";

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_source_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    char response_msg[] =
        "📜 **Source Code**\n"
        "*\"I have nothing to hide from you~\"* 💕\n\n"
        "**C Version**: https://github.com/blubskye/yuno_c\n"
        "**C++ Version**: https://github.com/blubskye/yuno_cpp\n"
        "**Rust Version**: https://github.com/blubskye/yuno_rust\n"
        "**Original JS**: https://github.com/japaneseenrichmentorganization/Yuno-Gasai-2\n\n"
        "Licensed under **AGPL-3.0** 💗";

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_prefix(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    const char *new_prefix = NULL;

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "prefix") == 0) {
            new_prefix = options[i].value;
        }
    }

    if (!new_prefix || strlen(new_prefix) > 5) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = "💔 Prefix too long! Max 5 characters~",
                .flags = DISCORD_MESSAGE_EPHEMERAL
            }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    db_set_prefix(&g_bot->database, interaction->guild_id, new_prefix);

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "🔧 **Prefix Updated!**\nNew prefix is now: `%s` 💕", new_prefix);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_prefix_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        char prefix[MAX_PREFIX_LEN];
        db_get_prefix(&g_bot->database, msg->guild_id, g_bot->config.default_prefix, prefix, sizeof(prefix));

        char response_msg[128];
        snprintf(response_msg, sizeof(response_msg), "💕 Current prefix: `%s`", prefix);

        struct discord_create_message params = { .content = response_msg };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    if (strlen(args) > 5) {
        struct discord_create_message params = { .content = "💔 Prefix too long! Max 5 characters~" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    db_set_prefix(&g_bot->database, msg->guild_id, args);

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "🔧 **Prefix Updated!**\nNew prefix is now: `%s` 💕", args);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_auto_clean(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[] = "🧹 Auto-clean configuration~ 💕";
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_auto_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    struct discord_create_message params = { .content = "🧹 Auto-clean configuration~ 💕" };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_delay(struct discord *client, const struct discord_interaction *interaction) {
    int minutes = 5;
    /* Parse minutes from options if provided */

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "⏳ **Delay Requested**\nI'll wait %d more minutes before cleaning~ 💕", minutes);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_delay_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    int minutes = 5;
    if (args && strlen(args) > 0) {
        minutes = atoi(args);
        if (minutes <= 0) minutes = 5;
    }

    char response_msg[256];
    snprintf(response_msg, sizeof(response_msg),
        "⏳ **Delay Requested**\nI'll wait %d more minutes before cleaning~ 💕", minutes);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_xp(struct discord *client, const struct discord_interaction *interaction) {
    u64snowflake user_id = interaction->member->user->id;
    /* Check for user option */

    user_xp_t user_xp;
    db_get_user_xp(&g_bot->database, user_id, interaction->guild_id, &user_xp);

    int next_level = user_xp.level + 1;
    int64_t xp_for_next = next_level * next_level * 100;
    int progress = (user_xp.xp * 100) / (xp_for_next > 0 ? xp_for_next : 1);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "✨ **XP Stats**\n<@%lu>'s progress~ 💕\n\n"
        "**Level:** %d\n"
        "**XP:** %ld\n"
        "**Progress to Next:** %d%%",
        (unsigned long)user_id, user_xp.level, (long)user_xp.xp, progress);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    u64snowflake user_id = msg->author->id;

    user_xp_t user_xp;
    db_get_user_xp(&g_bot->database, user_id, msg->guild_id, &user_xp);

    int next_level = user_xp.level + 1;
    int64_t xp_for_next = next_level * next_level * 100;
    int progress = (user_xp.xp * 100) / (xp_for_next > 0 ? xp_for_next : 1);

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "✨ **XP Stats**\n<@%lu>'s progress~ 💕\n\n"
        "**Level:** %d\n"
        "**XP:** %ld\n"
        "**Progress to Next:** %d%%",
        (unsigned long)user_id, user_xp.level, (long)user_xp.xp, progress);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_leaderboard(struct discord *client, const struct discord_interaction *interaction) {
    user_xp_t top_users[10];
    int count;
    db_get_leaderboard(&g_bot->database, interaction->guild_id, top_users, 10, &count);

    char response_msg[2048];
    char *ptr = response_msg;
    ptr += sprintf(ptr, "🏆 **Server Leaderboard**\n*\"Look who's been the most active~\"* 💕\n\n");

    for (int i = 0; i < count; i++) {
        const char *medal;
        if (i == 0) medal = "🥇";
        else if (i == 1) medal = "🥈";
        else if (i == 2) medal = "🥉";
        else medal = "";

        ptr += sprintf(ptr, "%s %d. <@%lu> - Level %d (%ld XP)\n",
            medal, i + 1,
            (unsigned long)top_users[i].user_id,
            top_users[i].level,
            (long)top_users[i].xp);
    }

    if (count == 0) {
        ptr += sprintf(ptr, "No one has earned XP yet~");
    }

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_leaderboard_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    user_xp_t top_users[10];
    int count;
    db_get_leaderboard(&g_bot->database, msg->guild_id, top_users, 10, &count);

    char response_msg[2048];
    char *ptr = response_msg;
    ptr += sprintf(ptr, "🏆 **Server Leaderboard**\n*\"Look who's been the most active~\"* 💕\n\n");

    for (int i = 0; i < count; i++) {
        const char *medal;
        if (i == 0) medal = "🥇";
        else if (i == 1) medal = "🥈";
        else if (i == 2) medal = "🥉";
        else medal = "";

        ptr += sprintf(ptr, "%s %d. <@%lu> - Level %d (%ld XP)\n",
            medal, i + 1,
            (unsigned long)top_users[i].user_id,
            top_users[i].level,
            (long)top_users[i].xp);
    }

    if (count == 0) {
        ptr += sprintf(ptr, "No one has earned XP yet~");
    }

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}
