/*
 * Yuno Gasai 2 (C Edition) - Utility Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _POSIX_C_SOURCE 199309L

#include "commands/utility.h"
#include "bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

/* Forward declarations for helpers used by auto-clean (defined later in file) */
static const char *get_util_option_value(const struct discord_interaction *interaction, const char *name);
static u64snowflake parse_channel_mention(const char *str);
static void send_interaction_reply(struct discord *client,
    const struct discord_interaction *interaction, const char *content);
static void send_prefix_reply(struct discord *client, u64snowflake channel_id, const char *content);

void cmd_ping(struct discord *client, const struct discord_interaction *interaction) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = "💓 Pinging..." }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

    char edit_msg[128];
    snprintf(edit_msg, sizeof(edit_msg),
        "💓 **Pong!** `%ldms`\nI'm always here for you~ 💕", ms);

    struct discord_edit_original_interaction_response edit = { .content = edit_msg };
    discord_edit_original_interaction_response(client, interaction->application_id,
        interaction->token, &edit, NULL);
}

void cmd_ping_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    struct timespec start, end;
    struct discord_message sync_msg = { 0 };

    clock_gettime(CLOCK_MONOTONIC, &start);

    struct discord_create_message params = { .content = "💓 Pinging..." };
    struct discord_ret_message ret = { .sync = &sync_msg };
    discord_create_message(client, msg->channel_id, &params, &ret);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;

    char edit_msg[128];
    snprintf(edit_msg, sizeof(edit_msg),
        "💓 **Pong!** `%ldms`\nI'm always here for you~ 💕", ms);

    struct discord_edit_message edit = { .content = edit_msg };
    discord_edit_message(client, msg->channel_id, sync_msg.id, &edit, NULL);
    discord_message_cleanup(&sync_msg);
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
        "`/stats` - Bot statistics\n"
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
        "`stats` - Bot statistics\n"
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

static void build_stats_message(char *buf, size_t len) {
    int64_t uptime_sec = time(NULL) - g_bot->start_time;
    int days = (int)(uptime_sec / 86400);
    int hours = (int)((uptime_sec % 86400) / 3600);
    int minutes = (int)((uptime_sec % 3600) / 60);
    int seconds = (int)(uptime_sec % 60);

    /* Get memory usage from /proc/self/status */
    long rss_kb = 0;
    FILE *f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, " %ld", &rss_kb);
                break;
            }
        }
        fclose(f);
    }

    /* Get system info */
    struct sysinfo si;
    sysinfo(&si);
    long total_mb = (long)(si.totalram / (1024 * 1024));

    struct utsname uts;
    uname(&uts);

    snprintf(buf, len,
        "📊 **Yuno Stats** (C Edition)\n"
        "*\"I'm always keeping track of everything~\"* 💕\n\n"
        "**Uptime:** %dd %02dh %02dm %02ds\n"
        "**RAM:** %ld MB / %ld MB\n"
        "**System:** %s (%s)\n"
        "**Libraries:** Concord (Discord), SQLite3, json-c",
        days, hours, minutes, seconds,
        rss_kb / 1024, total_mb,
        uts.sysname, uts.machine);
}

void cmd_stats(struct discord *client, const struct discord_interaction *interaction) {
    char response_msg[512];
    build_stats_message(response_msg, sizeof(response_msg));

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_stats_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    char response_msg[512];
    build_stats_message(response_msg, sizeof(response_msg));

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

void cmd_prefix(struct discord *client, const struct discord_interaction *interaction) {
    const char *new_prefix = NULL;

    if (interaction->data->options) {
        for (int i = 0; i < interaction->data->options->size; i++) {
            if (strcmp(interaction->data->options->array[i].name, "prefix") == 0) {
                new_prefix = interaction->data->options->array[i].value;
            }
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

/* --- Auto-Clean: subcommand dispatch --- */

static void auto_clean_add(struct discord *client, u64snowflake guild_id, u64snowflake channel_id,
                            int hours_between, int warning_mins, char *out, size_t out_len) {
    if (hours_between <= 0 || warning_mins <= 0) {
        snprintf(out, out_len, "Between cleans and warning time must be positive numbers~");
        return;
    }
    if (warning_mins >= hours_between * 60) {
        snprintf(out, out_len, "Warning time cannot be equal to or higher than the clean interval~");
        return;
    }

    auto_clean_config_t config = {
        .guild_id = guild_id,
        .channel_id = channel_id,
        .interval_minutes = hours_between,
        .message_count = hours_between * 60,
        .warning_minutes = warning_mins,
        .enabled = 1
    };

    /* Check if config already exists */
    auto_clean_config_t existing;
    int exists = (db_get_auto_clean_config(&g_bot->database, guild_id, channel_id, &existing) == 0);

    db_set_auto_clean_config(&g_bot->database, &config);
    snprintf(out, out_len,
        "🧹 Auto-clean **%s**!\n<#%lu> will be cleaned every **%d hours** with a warning **%d minutes** before~ 💕",
        exists ? "updated" : "created",
        (unsigned long)channel_id, hours_between, warning_mins);
}

static void auto_clean_remove(struct discord *client, u64snowflake guild_id, u64snowflake channel_id,
                               char *out, size_t out_len) {
    (void)client;
    auto_clean_config_t existing;
    if (db_get_auto_clean_config(&g_bot->database, guild_id, channel_id, &existing) != 0) {
        snprintf(out, out_len, "This channel doesn't have any auto-clean set up~");
        return;
    }
    db_remove_auto_clean_config(&g_bot->database, guild_id, channel_id);
    snprintf(out, out_len, "🧹 Auto-clean removed for <#%lu>~ 💕", (unsigned long)channel_id);
}

static void auto_clean_list(struct discord *client, u64snowflake guild_id, u64snowflake channel_id,
                             char *out, size_t out_len) {
    (void)client;
    if (channel_id != 0) {
        /* Show details for specific channel */
        auto_clean_config_t config;
        if (db_get_auto_clean_config(&g_bot->database, guild_id, channel_id, &config) != 0) {
            snprintf(out, out_len, "This channel doesn't have an auto-clean configured~");
            return;
        }
        int remaining_hrs = config.message_count / 60;
        int remaining_mins = config.message_count % 60;
        snprintf(out, out_len,
            "🧹 **<#%lu> auto-clean config**\n"
            "**Clean every:** %d hours\n"
            "**Warning at:** %d minutes before\n"
            "**Remaining:** %dh %dm\n"
            "**Status:** %s",
            (unsigned long)channel_id,
            config.interval_minutes,
            config.warning_minutes,
            remaining_hrs, remaining_mins,
            config.enabled ? "Enabled" : "Disabled");
        return;
    }

    /* List all auto-cleans for the guild */
    auto_clean_config_t configs[MAX_AUTO_CLEAN_CHANNELS];
    int count = 0;
    db_get_guild_auto_clean_configs(&g_bot->database, guild_id, configs, MAX_AUTO_CLEAN_CHANNELS, &count);

    if (count == 0) {
        snprintf(out, out_len, "🧹 No auto-cleans configured for this guild~");
        return;
    }

    int pos = snprintf(out, out_len, "🧹 **Auto-clean channels:**\n");
    for (int i = 0; i < count && pos < (int)out_len - 100; i++) {
        pos += snprintf(out + pos, out_len - pos, "- <#%lu> (every %dh, %dm remaining)\n",
            (unsigned long)configs[i].channel_id, configs[i].interval_minutes,
            configs[i].message_count);
    }
}

static void auto_clean_reset(struct discord *client, u64snowflake guild_id, u64snowflake channel_id,
                              char *out, size_t out_len) {
    (void)client;
    auto_clean_config_t config;
    if (db_get_auto_clean_config(&g_bot->database, guild_id, channel_id, &config) != 0) {
        snprintf(out, out_len, "This channel doesn't have any auto-clean set up~");
        return;
    }
    config.message_count = config.interval_minutes * 60;
    db_set_auto_clean_config(&g_bot->database, &config);
    snprintf(out, out_len, "🧹 Auto-clean timer reset for <#%lu>~ 💕", (unsigned long)channel_id);
}

static void auto_clean_delay_cmd(struct discord *client, u64snowflake guild_id, u64snowflake channel_id,
                                  int minutes, char *out, size_t out_len) {
    (void)client;
    auto_clean_config_t config;
    if (db_get_auto_clean_config(&g_bot->database, guild_id, channel_id, &config) != 0) {
        snprintf(out, out_len, "This channel doesn't have any auto-clean set up~");
        return;
    }
    config.message_count += minutes;
    db_set_auto_clean_config(&g_bot->database, &config);
    snprintf(out, out_len, "⏳ Delayed the clean by **%d minutes** for <#%lu>~ 💕",
        minutes, (unsigned long)channel_id);
}

void cmd_auto_clean(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    /* Parse subcommand and options from first option value */
    const char *sub = get_util_option_value(interaction, "action");
    const char *channel_val = get_util_option_value(interaction, "channel");
    const char *hours_val = get_util_option_value(interaction, "hours");
    const char *warning_val = get_util_option_value(interaction, "warning");
    const char *minutes_val = get_util_option_value(interaction, "minutes");

    if (!sub) {
        send_interaction_reply(client, interaction,
            "Usage: `/auto-clean <add|remove|list|reset|delay> [channel] [hours] [warning]`~");
        return;
    }

    u64snowflake channel_id = channel_val ? parse_channel_mention(channel_val) : interaction->channel_id;
    char response_msg[1024];

    if (strcmp(sub, "add") == 0 || strcmp(sub, "edit") == 0) {
        int hours = hours_val ? atoi(hours_val) : 0;
        int warning = warning_val ? atoi(warning_val) : 5;
        if (hours <= 0) {
            send_interaction_reply(client, interaction,
                "Usage: `/auto-clean add <channel> <hours_between_cleans> [warning_minutes]`~");
            return;
        }
        auto_clean_add(client, interaction->guild_id, channel_id, hours, warning, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "remove") == 0) {
        auto_clean_remove(client, interaction->guild_id, channel_id, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "list") == 0) {
        u64snowflake list_ch = channel_val ? parse_channel_mention(channel_val) : 0;
        auto_clean_list(client, interaction->guild_id, list_ch, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "reset") == 0) {
        auto_clean_reset(client, interaction->guild_id, channel_id, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "delay") == 0) {
        int mins = minutes_val ? atoi(minutes_val) : (hours_val ? atoi(hours_val) : 5);
        if (mins <= 0) mins = 5;
        auto_clean_delay_cmd(client, interaction->guild_id, channel_id, mins, response_msg, sizeof(response_msg));
    } else {
        snprintf(response_msg, sizeof(response_msg),
            "Unknown subcommand `%s`. Use: `add`, `remove`, `list`, `reset`, `delay`~", sub);
    }

    send_interaction_reply(client, interaction, response_msg);
}

void cmd_auto_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `auto-clean <add|remove|list|reset|delay> [#channel] [hours] [warning_mins]`~");
        return;
    }

    char sub[32] = { 0 };
    char channel_str[64] = { 0 };
    char arg3[32] = { 0 };
    char arg4[32] = { 0 };
    int parsed = sscanf(args, "%31s %63s %31s %31s", sub, channel_str, arg3, arg4);

    char response_msg[1024];

    if (strcmp(sub, "add") == 0 || strcmp(sub, "edit") == 0) {
        if (parsed < 3) {
            send_prefix_reply(client, msg->channel_id,
                "Usage: `auto-clean add <#channel> <hours_between_cleans> [warning_minutes]`~");
            return;
        }
        u64snowflake channel_id = parse_channel_mention(channel_str);
        if (channel_id == 0) {
            send_prefix_reply(client, msg->channel_id, "Invalid channel~");
            return;
        }
        int hours = atoi(arg3);
        int warning = parsed >= 4 ? atoi(arg4) : 5;
        auto_clean_add(client, msg->guild_id, channel_id, hours, warning, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "remove") == 0) {
        u64snowflake channel_id = channel_str[0] ? parse_channel_mention(channel_str) : msg->channel_id;
        if (channel_id == 0) {
            send_prefix_reply(client, msg->channel_id, "Invalid channel~");
            return;
        }
        auto_clean_remove(client, msg->guild_id, channel_id, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "list") == 0) {
        u64snowflake channel_id = channel_str[0] ? parse_channel_mention(channel_str) : 0;
        auto_clean_list(client, msg->guild_id, channel_id, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "reset") == 0) {
        u64snowflake channel_id = channel_str[0] ? parse_channel_mention(channel_str) : msg->channel_id;
        if (channel_id == 0) {
            send_prefix_reply(client, msg->channel_id, "Invalid channel~");
            return;
        }
        auto_clean_reset(client, msg->guild_id, channel_id, response_msg, sizeof(response_msg));
    } else if (strcmp(sub, "delay") == 0) {
        u64snowflake channel_id = channel_str[0] ? parse_channel_mention(channel_str) : msg->channel_id;
        if (channel_id == 0) {
            send_prefix_reply(client, msg->channel_id, "Invalid channel~");
            return;
        }
        int mins = arg3[0] ? atoi(arg3) : 5;
        if (mins <= 0) mins = 5;
        auto_clean_delay_cmd(client, msg->guild_id, channel_id, mins, response_msg, sizeof(response_msg));
    } else {
        snprintf(response_msg, sizeof(response_msg),
            "Unknown subcommand `%s`. Use: `add`, `remove`, `list`, `reset`, `delay`~", sub);
    }

    send_prefix_reply(client, msg->channel_id, response_msg);
}

/* Standalone delay command - shortcut for current channel */
void cmd_delay(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *minutes_val = get_util_option_value(interaction, "minutes");
    int minutes = minutes_val ? atoi(minutes_val) : 5;
    if (minutes <= 0) minutes = 5;

    char response_msg[256];
    auto_clean_delay_cmd(client, interaction->guild_id, interaction->channel_id,
        minutes, response_msg, sizeof(response_msg));
    send_interaction_reply(client, interaction, response_msg);
}

void cmd_delay_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    /* Parse: delay [#channel] [minutes] */
    char arg1[64] = { 0 };
    char arg2[32] = { 0 };
    int parsed = 0;
    if (args && strlen(args) > 0) {
        parsed = sscanf(args, "%63s %31s", arg1, arg2);
    }

    u64snowflake channel_id = msg->channel_id;
    int minutes = 5;

    if (parsed >= 1) {
        /* Check if arg1 is a channel mention or a number */
        if (arg1[0] == '<' && arg1[1] == '#') {
            channel_id = parse_channel_mention(arg1);
            if (parsed >= 2) minutes = atoi(arg2);
        } else {
            minutes = atoi(arg1);
        }
        if (minutes <= 0) minutes = 5;
    }

    char response_msg[256];
    auto_clean_delay_cmd(client, msg->guild_id, channel_id, minutes, response_msg, sizeof(response_msg));
    send_prefix_reply(client, msg->channel_id, response_msg);
}

void cmd_xp(struct discord *client, const struct discord_interaction *interaction) {
    u64snowflake user_id = interaction->member->user->id;
    /* Check for user option */

    user_xp_t user_xp;
    db_get_user_xp(&g_bot->database, user_id, interaction->guild_id, &user_xp);

    /* JS formula: needed = 5 * level^2 + 50 * level + 100 */
    int64_t needed = 5 * (int64_t)user_xp.level * user_xp.level
                   + 50 * user_xp.level + 100;
    int64_t remaining = needed - user_xp.xp;

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "✨ **XP Stats**\n<@%lu>'s progress~ 💕\n\n"
        "**Level:** %d\n"
        "**XP:** %ld / %ld\n"
        "**XP until level %d:** %ld",
        (unsigned long)user_id, user_xp.level, (long)user_xp.xp,
        (long)needed, user_xp.level + 1, (long)remaining);

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

    /* JS formula: needed = 5 * level^2 + 50 * level + 100 */
    int64_t needed = 5 * (int64_t)user_xp.level * user_xp.level
                   + 50 * user_xp.level + 100;
    int64_t remaining = needed - user_xp.xp;

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg),
        "✨ **XP Stats**\n<@%lu>'s progress~ 💕\n\n"
        "**Level:** %d\n"
        "**XP:** %ld / %ld\n"
        "**XP until level %d:** %ld",
        (unsigned long)user_id, user_xp.level, (long)user_xp.xp,
        (long)needed, user_xp.level + 1, (long)remaining);

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

/* --- Master-user admin commands --- */

static int parse_toggle(const char *arg) {
    if (!arg || strlen(arg) == 0) return -1;
    if (strncmp(arg, "enab", 4) == 0 || strncmp(arg, "tru", 3) == 0
        || strcmp(arg, "on") == 0 || strcmp(arg, "1") == 0) return 1;
    if (strncmp(arg, "disab", 5) == 0 || strncmp(arg, "fa", 2) == 0
        || strcmp(arg, "off") == 0 || strcmp(arg, "0") == 0) return 0;
    return -1;
}

static void send_interaction_reply(struct discord *client,
    const struct discord_interaction *interaction, const char *content) {
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = (char *)content }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

static void send_prefix_reply(struct discord *client, u64snowflake channel_id, const char *content) {
    struct discord_create_message p = { .content = (char *)content };
    discord_create_message(client, channel_id, &p, NULL);
}

/* bot-ban */
void cmd_bot_ban_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }
    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `bot-ban <user_id> [reason]`");
        return;
    }

    char args_copy[512];
    strncpy(args_copy, args, sizeof(args_copy) - 1);
    args_copy[sizeof(args_copy) - 1] = '\0';
    char *user_str = strtok(args_copy, " ");
    char *reason = strtok(NULL, "");

    uint64_t user_id = strtoull(user_str, NULL, 10);
    if (user_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid user ID.");
        return;
    }

    if (db_is_bot_banned(&g_bot->database, user_id)) {
        send_prefix_reply(client, msg->channel_id, "That user is already bot-banned.");
        return;
    }

    bot_ban_t ban = {
        .user_id = user_id,
        .banned_by = msg->author->id,
        .timestamp = time(NULL)
    };
    strncpy(ban.reason, reason ? reason : "No reason given", MAX_REASON_LEN - 1);

    if (db_add_bot_ban(&g_bot->database, &ban) == 0) {
        char reply[256];
        snprintf(reply, sizeof(reply),
            "User <@%lu> has been bot-banned~ 💢\n%s%s",
            (unsigned long)user_id,
            reason ? "**Reason:** " : "", reason ? reason : "");
        send_prefix_reply(client, msg->channel_id, reply);
    } else {
        send_prefix_reply(client, msg->channel_id, "Failed to bot-ban user.");
    }
}

void cmd_bot_ban(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }
    send_interaction_reply(client, interaction, "Use the prefix command for bot-ban: `.bot-ban <user_id> [reason]`");
}

/* bot-unban */
void cmd_bot_unban_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }
    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `bot-unban <user_id>`");
        return;
    }

    uint64_t user_id = strtoull(args, NULL, 10);
    if (user_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid user ID.");
        return;
    }

    if (db_remove_bot_ban(&g_bot->database, user_id) == 0) {
        char reply[128];
        snprintf(reply, sizeof(reply),
            "User <@%lu> has been unbanned from the bot~ 💕", (unsigned long)user_id);
        send_prefix_reply(client, msg->channel_id, reply);
    } else {
        send_prefix_reply(client, msg->channel_id, "Failed to unban user.");
    }
}

void cmd_bot_unban(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }
    send_interaction_reply(client, interaction, "Use the prefix command for bot-unban: `.bot-unban <user_id>`");
}

/* bot-banlist */
void cmd_bot_banlist_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    bot_ban_t bans[20];
    int count = 0;
    db_get_bot_bans(&g_bot->database, bans, 20, &count);

    char response_msg[2048];
    char *ptr = response_msg;

    if (count == 0) {
        ptr += sprintf(ptr, "No users are bot-banned~ 💕");
    } else {
        ptr += sprintf(ptr, "🚫 **Bot-Banned Users** (%d)\n\n", count);
        for (int i = 0; i < count; i++) {
            ptr += sprintf(ptr, "- <@%lu> — %s\n",
                (unsigned long)bans[i].user_id, bans[i].reason);
        }
    }

    send_prefix_reply(client, msg->channel_id, response_msg);
}

void cmd_bot_banlist(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    bot_ban_t bans[20];
    int count = 0;
    db_get_bot_bans(&g_bot->database, bans, 20, &count);

    char response_msg[2048];
    char *ptr = response_msg;

    if (count == 0) {
        ptr += sprintf(ptr, "No users are bot-banned~ 💕");
    } else {
        ptr += sprintf(ptr, "🚫 **Bot-Banned Users** (%d)\n\n", count);
        for (int i = 0; i < count; i++) {
            ptr += sprintf(ptr, "- <@%lu> — %s\n",
                (unsigned long)bans[i].user_id, bans[i].reason);
        }
    }

    send_interaction_reply(client, interaction, response_msg);
}

/* set-spamfilter */
void cmd_set_spamfilter_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    int val = parse_toggle(args);
    if (val < 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-spamfilter <on|off>`");
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) != 0) {
        settings.guild_id = msg->guild_id;
        settings.leveling_enabled = 1;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
    }
    settings.spam_filter_enabled = val;
    db_set_guild_settings(&g_bot->database, &settings);

    char reply[128];
    snprintf(reply, sizeof(reply),
        "Spam filter is now **%s** on this guild~ %s",
        val ? "enabled" : "disabled", val ? "🛡️" : "💕");
    send_prefix_reply(client, msg->channel_id, reply);
}

void cmd_set_spamfilter(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }
    send_interaction_reply(client, interaction, "Use the prefix command: `.set-spamfilter <on|off>`");
}

/* set-experiencecounter */
void cmd_set_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    int val = parse_toggle(args);
    if (val < 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-experiencecounter <on|off>`");
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) != 0) {
        settings.guild_id = msg->guild_id;
        settings.spam_filter_enabled = 0;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
    }
    settings.leveling_enabled = val;
    db_set_guild_settings(&g_bot->database, &settings);

    char reply[128];
    snprintf(reply, sizeof(reply),
        "Experience counter is now **%s** on this guild~ %s",
        val ? "enabled" : "disabled", val ? "✨" : "💕");
    send_prefix_reply(client, msg->channel_id, reply);
}

void cmd_set_xp(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }
    send_interaction_reply(client, interaction, "Use the prefix command: `.set-experiencecounter <on|off>`");
}

/* --- Phase 3: Admin XP Commands --- */

/* Helper to get option value from slash command */
static const char *get_util_option_value(const struct discord_interaction *interaction, const char *name) {
    if (!interaction->data->options) return NULL;
    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(interaction->data->options->array[i].name, name) == 0)
            return interaction->data->options->array[i].value;
    }
    return NULL;
}

/* set-level: manually set a user's level */
void cmd_set_level(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *user_val = get_util_option_value(interaction, "user");
    const char *level_val = get_util_option_value(interaction, "level");

    if (!level_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-level <level> [user]`~");
        return;
    }

    int level = atoi(level_val);
    if (level < 0) {
        send_interaction_reply(client, interaction, "Level must be non-negative~");
        return;
    }

    u64snowflake user_id = user_val ? strtoull(user_val, NULL, 10) : interaction->member->user->id;

    db_set_xp_data(&g_bot->database, user_id, interaction->guild_id, 0, level);

    int64_t needed = 5 * (int64_t)level * level + 50 * level + 100;
    char response[256];
    snprintf(response, sizeof(response),
        "✨ Level set for <@%lu>!\n**Level:** %d\n**XP:** 0 / %ld",
        (unsigned long)user_id, level, (long)needed);
    send_interaction_reply(client, interaction, response);
}

void cmd_set_level_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-level <level> [@user]`~");
        return;
    }

    char level_str[16], user_mention[64] = "";
    int parsed = sscanf(args, "%15s %63s", level_str, user_mention);
    if (parsed < 1) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-level <level> [@user]`~");
        return;
    }

    int level = atoi(level_str);
    if (level < 0) {
        send_prefix_reply(client, msg->channel_id, "Level must be non-negative~");
        return;
    }

    u64snowflake user_id = msg->author->id;
    if (user_mention[0]) {
        u64snowflake parsed_id = parse_user_mention(user_mention);
        if (parsed_id != 0) user_id = parsed_id;
    }

    db_set_xp_data(&g_bot->database, user_id, msg->guild_id, 0, level);

    int64_t needed = 5 * (int64_t)level * level + 50 * level + 100;
    char response[256];
    snprintf(response, sizeof(response),
        "✨ Level set for <@%lu>!\n**Level:** %d\n**XP:** 0 / %ld",
        (unsigned long)user_id, level, (long)needed);
    send_prefix_reply(client, msg->channel_id, response);
}

/* fix-xp-data: recalculate/fix corrupted XP data */
void cmd_fix_xp(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    int scanned = 0, fixed = 0;
    db_fix_xp_data(&g_bot->database, interaction->guild_id, &scanned, &fixed);

    char response[256];
    snprintf(response, sizeof(response),
        "🔧 **XP Data Fix Complete!**\n\n"
        "**Scanned:** %d users\n"
        "**Fixed:** %d corrupted records\n"
        "Corrupted users now have 0 XP at their current level~",
        scanned, fixed);
    send_interaction_reply(client, interaction, response);
}

void cmd_fix_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    int scanned = 0, fixed = 0;
    db_fix_xp_data(&g_bot->database, msg->guild_id, &scanned, &fixed);

    char response[256];
    snprintf(response, sizeof(response),
        "🔧 **XP Data Fix Complete!**\n\n"
        "**Scanned:** %d users\n"
        "**Fixed:** %d corrupted records\n"
        "Corrupted users now have 0 XP at their current level~",
        scanned, fixed);
    send_prefix_reply(client, msg->channel_id, response);
}

/* mass-addxp: add XP to all tracked users in guild */
void cmd_mass_addxp(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *amount_val = get_util_option_value(interaction, "amount");
    if (!amount_val) {
        send_interaction_reply(client, interaction, "Usage: `/mass-addxp <amount>`~");
        return;
    }

    int64_t amount = strtoll(amount_val, NULL, 10);
    if (amount <= 0) {
        send_interaction_reply(client, interaction, "Amount must be positive~");
        return;
    }

    int updated = 0;
    db_add_xp_all_guild(&g_bot->database, interaction->guild_id, amount, &updated);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Mass XP Added!**\n\n"
        "**XP given:** +%ld each\n"
        "**Users updated:** %d",
        (long)amount, updated);
    send_interaction_reply(client, interaction, response);
}

void cmd_mass_addxp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `mass-addxp <amount>`~");
        return;
    }

    int64_t amount = strtoll(args, NULL, 10);
    if (amount <= 0) {
        send_prefix_reply(client, msg->channel_id, "Amount must be positive~");
        return;
    }

    int updated = 0;
    db_add_xp_all_guild(&g_bot->database, msg->guild_id, amount, &updated);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Mass XP Added!**\n\n"
        "**XP given:** +%ld each\n"
        "**Users updated:** %d",
        (long)amount, updated);
    send_prefix_reply(client, msg->channel_id, response);
}

/* mass-setxp: set level for all tracked users in guild */
void cmd_mass_setxp(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *level_val = get_util_option_value(interaction, "level");
    if (!level_val) {
        send_interaction_reply(client, interaction, "Usage: `/mass-setxp <level>`~");
        return;
    }

    int level = atoi(level_val);
    if (level < 0) {
        send_interaction_reply(client, interaction, "Level must be non-negative~");
        return;
    }

    int updated = 0;
    db_set_level_all_guild(&g_bot->database, interaction->guild_id, level, &updated);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Mass Level Set!**\n\n"
        "**Level set:** %d\n"
        "**Users updated:** %d",
        level, updated);
    send_interaction_reply(client, interaction, response);
}

void cmd_mass_setxp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `mass-setxp <level>`~");
        return;
    }

    int level = atoi(args);
    if (level < 0) {
        send_prefix_reply(client, msg->channel_id, "Level must be non-negative~");
        return;
    }

    int updated = 0;
    db_set_level_all_guild(&g_bot->database, msg->guild_id, level, &updated);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Mass Level Set!**\n\n"
        "**Level set:** %d\n"
        "**Users updated:** %d",
        level, updated);
    send_prefix_reply(client, msg->channel_id, response);
}

/* --- Phase 3.2: Level-Role Mapping --- */

/* set-levelrolemap: map a level to an auto-assigned role */
void cmd_set_levelrolemap(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *level_val = get_util_option_value(interaction, "level");
    const char *role_val = get_util_option_value(interaction, "role");

    if (!level_val || !role_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-levelrolemap <level> <role>`~");
        return;
    }

    int level = atoi(level_val);
    if (level < 0) {
        send_interaction_reply(client, interaction, "Level must be non-negative~");
        return;
    }

    u64snowflake role_id = strtoull(role_val, NULL, 10);
    if (role_id == 0) {
        send_interaction_reply(client, interaction, "Invalid role~");
        return;
    }

    db_set_level_role(&g_bot->database, interaction->guild_id, level, role_id);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ Level role map updated!\nLevel **%d** -> <@&%lu>",
        level, (unsigned long)role_id);
    send_interaction_reply(client, interaction, response);
}

void cmd_set_levelrolemap_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-levelrolemap <level> <@role>`~");
        return;
    }

    char level_str[16], role_mention[64];
    if (sscanf(args, "%15s %63s", level_str, role_mention) < 2) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-levelrolemap <level> <@role>`~");
        return;
    }

    int level = atoi(level_str);
    if (level < 0) {
        send_prefix_reply(client, msg->channel_id, "Level must be non-negative~");
        return;
    }

    /* Parse role mention: <@&123456> or raw ID */
    u64snowflake role_id = 0;
    if (role_mention[0] == '<' && role_mention[1] == '@' && role_mention[2] == '&') {
        role_id = strtoull(role_mention + 3, NULL, 10);
    } else {
        role_id = strtoull(role_mention, NULL, 10);
    }

    if (role_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid role~");
        return;
    }

    db_set_level_role(&g_bot->database, msg->guild_id, level, role_id);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ Level role map updated!\nLevel **%d** -> <@&%lu>",
        level, (unsigned long)role_id);
    send_prefix_reply(client, msg->channel_id, response);
}

/* sync-levelroles: apply level roles to existing users at a given level */
void cmd_sync_levelroles(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    /* Get all level-role mappings for this guild */
    level_role_t roles[50];
    int role_count = 0;
    db_get_level_roles(&g_bot->database, interaction->guild_id, roles, 50, &role_count);

    if (role_count == 0) {
        send_interaction_reply(client, interaction, "No level role map configured. Use `set-levelrolemap` first~");
        return;
    }

    /* Get all users with XP data and assign appropriate roles */
    user_xp_t users[500];
    int user_count = 0;
    db_get_leaderboard(&g_bot->database, interaction->guild_id, users, 500, &user_count);

    int assigned = 0;
    for (int i = 0; i < user_count; i++) {
        for (int j = 0; j < role_count; j++) {
            if (users[i].level >= roles[j].level) {
                discord_add_guild_member_role(client, interaction->guild_id,
                    users[i].user_id, roles[j].role_id, NULL, NULL);
                assigned++;
            }
        }
    }

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Level Roles Synced!**\n\n"
        "**Users checked:** %d\n"
        "**Role assignments:** %d",
        user_count, assigned);
    send_interaction_reply(client, interaction, response);
}

void cmd_sync_levelroles_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    level_role_t roles[50];
    int role_count = 0;
    db_get_level_roles(&g_bot->database, msg->guild_id, roles, 50, &role_count);

    if (role_count == 0) {
        send_prefix_reply(client, msg->channel_id, "No level role map configured. Use `set-levelrolemap` first~");
        return;
    }

    user_xp_t users[500];
    int user_count = 0;
    db_get_leaderboard(&g_bot->database, msg->guild_id, users, 500, &user_count);

    int assigned = 0;
    for (int i = 0; i < user_count; i++) {
        for (int j = 0; j < role_count; j++) {
            if (users[i].level >= roles[j].level) {
                discord_add_guild_member_role(client, msg->guild_id,
                    users[i].user_id, roles[j].role_id, NULL, NULL);
                assigned++;
            }
        }
    }

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **Level Roles Synced!**\n\n"
        "**Users checked:** %d\n"
        "**Role assignments:** %d",
        user_count, assigned);
    send_prefix_reply(client, msg->channel_id, response);
}

/* sync-xp-from-roles: assign XP/levels based on users' current roles */
void cmd_sync_xp_from_roles(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    level_role_t roles[50];
    int role_count = 0;
    db_get_level_roles(&g_bot->database, interaction->guild_id, roles, 50, &role_count);

    if (role_count == 0) {
        send_interaction_reply(client, interaction, "No level role map configured. Use `set-levelrolemap` first~");
        return;
    }

    /* Fetch guild members in batches and check roles */
    struct discord_guild_members members = { 0 };
    struct discord_ret_guild_members ret = { .sync = &members };
    struct discord_list_guild_members params = { .limit = 1000, .after = 0 };

    if (discord_list_guild_members(client, interaction->guild_id, &params, &ret) != CCORD_OK) {
        send_interaction_reply(client, interaction, "Failed to fetch guild members~");
        return;
    }

    int synced = 0, skipped = 0;
    for (int i = 0; i < members.size; i++) {
        struct discord_guild_member *m = &members.array[i];
        if (!m->user || m->user->bot) continue;
        if (!m->roles) continue;

        /* Find highest level role this user has */
        int highest_level = -1;
        for (int j = 0; j < role_count; j++) {
            for (int k = 0; k < m->roles->size; k++) {
                if (m->roles->array[k] == roles[j].role_id && roles[j].level > highest_level) {
                    highest_level = roles[j].level;
                }
            }
        }

        if (highest_level < 0) continue;

        /* Check current level */
        user_xp_t xp;
        db_get_user_xp(&g_bot->database, m->user->id, interaction->guild_id, &xp);
        if (xp.level >= highest_level) {
            skipped++;
            continue;
        }

        db_set_xp_data(&g_bot->database, m->user->id, interaction->guild_id, 0, highest_level);
        synced++;
    }

    discord_guild_members_cleanup(&members);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **XP Synced from Roles!**\n\n"
        "**Members checked:** %d\n"
        "**Synced:** %d\n"
        "**Already at/above level:** %d",
        members.size, synced, skipped);
    send_interaction_reply(client, interaction, response);
}

void cmd_sync_xp_from_roles_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    level_role_t roles[50];
    int role_count = 0;
    db_get_level_roles(&g_bot->database, msg->guild_id, roles, 50, &role_count);

    if (role_count == 0) {
        send_prefix_reply(client, msg->channel_id, "No level role map configured. Use `set-levelrolemap` first~");
        return;
    }

    struct discord_guild_members members = { 0 };
    struct discord_ret_guild_members ret = { .sync = &members };
    struct discord_list_guild_members params = { .limit = 1000, .after = 0 };

    if (discord_list_guild_members(client, msg->guild_id, &params, &ret) != CCORD_OK) {
        send_prefix_reply(client, msg->channel_id, "Failed to fetch guild members~");
        return;
    }

    int synced = 0, skipped = 0;
    for (int i = 0; i < members.size; i++) {
        struct discord_guild_member *m = &members.array[i];
        if (!m->user || m->user->bot) continue;
        if (!m->roles) continue;

        int highest_level = -1;
        for (int j = 0; j < role_count; j++) {
            for (int k = 0; k < m->roles->size; k++) {
                if (m->roles->array[k] == roles[j].role_id && roles[j].level > highest_level) {
                    highest_level = roles[j].level;
                }
            }
        }

        if (highest_level < 0) continue;

        user_xp_t xp;
        db_get_user_xp(&g_bot->database, m->user->id, msg->guild_id, &xp);
        if (xp.level >= highest_level) {
            skipped++;
            continue;
        }

        db_set_xp_data(&g_bot->database, m->user->id, msg->guild_id, 0, highest_level);
        synced++;
    }

    int total = members.size;
    discord_guild_members_cleanup(&members);

    char response[256];
    snprintf(response, sizeof(response),
        "✨ **XP Synced from Roles!**\n\n"
        "**Members checked:** %d\n"
        "**Synced:** %d\n"
        "**Already at/above level:** %d",
        total, synced, skipped);
    send_prefix_reply(client, msg->channel_id, response);
}

/* --- Phase 3.3: Voice Chat XP Commands --- */

/* set-vcxp: toggle voice XP and configure settings */
void cmd_set_vcxp(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *toggle_val = get_util_option_value(interaction, "enabled");
    const char *xp_val = get_util_option_value(interaction, "xp_per_minute");
    const char *min_users_val = get_util_option_value(interaction, "min_users");

    voice_xp_config_t config;
    db_get_voice_xp_config(&g_bot->database, interaction->guild_id, &config);

    if (toggle_val) {
        config.enabled = parse_toggle(toggle_val);
    }
    if (xp_val) {
        config.xp_per_minute = atoi(xp_val);
        if (config.xp_per_minute <= 0) config.xp_per_minute = 5;
    }
    if (min_users_val) {
        config.min_users = atoi(min_users_val);
        if (config.min_users < 1) config.min_users = 1;
    }

    config.guild_id = interaction->guild_id;
    db_set_voice_xp_config(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "🎤 **Voice XP Updated!**\n\n"
        "**Enabled:** %s\n"
        "**XP per minute:** %d\n"
        "**Min users in channel:** %d\n"
        "**Ignore AFK:** %s",
        config.enabled ? "Yes" : "No",
        config.xp_per_minute, config.min_users,
        config.ignore_afk ? "Yes" : "No");
    send_interaction_reply(client, interaction, response);
}

void cmd_set_vcxp_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `set-vcxp <on|off> [xp_per_minute] [min_users]`~");
        return;
    }

    char toggle_str[8] = "";
    char xp_str[8] = "";
    char min_str[8] = "";
    sscanf(args, "%7s %7s %7s", toggle_str, xp_str, min_str);

    voice_xp_config_t config;
    db_get_voice_xp_config(&g_bot->database, msg->guild_id, &config);

    if (toggle_str[0]) config.enabled = parse_toggle(toggle_str);
    if (xp_str[0]) {
        config.xp_per_minute = atoi(xp_str);
        if (config.xp_per_minute <= 0) config.xp_per_minute = 5;
    }
    if (min_str[0]) {
        config.min_users = atoi(min_str);
        if (config.min_users < 1) config.min_users = 1;
    }

    config.guild_id = msg->guild_id;
    db_set_voice_xp_config(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "🎤 **Voice XP Updated!**\n\n"
        "**Enabled:** %s\n"
        "**XP per minute:** %d\n"
        "**Min users in channel:** %d",
        config.enabled ? "Yes" : "No",
        config.xp_per_minute, config.min_users);
    send_prefix_reply(client, msg->channel_id, response);
}

/* vcxp-status: show current voice XP config */
void cmd_vcxp_status(struct discord *client, const struct discord_interaction *interaction) {
    voice_xp_config_t config;
    db_get_voice_xp_config(&g_bot->database, interaction->guild_id, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "🎤 **Voice XP Status**\n\n"
        "**Enabled:** %s\n"
        "**XP per minute:** %d\n"
        "**Min users in channel:** %d\n"
        "**Ignore AFK:** %s\n"
        "**Active voice sessions:** %d",
        config.enabled ? "Yes" : "No",
        config.xp_per_minute, config.min_users,
        config.ignore_afk ? "Yes" : "No",
        g_bot->voice_tracker.count);
    send_interaction_reply(client, interaction, response);
}

void cmd_vcxp_status_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    voice_xp_config_t config;
    db_get_voice_xp_config(&g_bot->database, msg->guild_id, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "🎤 **Voice XP Status**\n\n"
        "**Enabled:** %s\n"
        "**XP per minute:** %d\n"
        "**Min users in channel:** %d\n"
        "**Ignore AFK:** %s\n"
        "**Active voice sessions:** %d",
        config.enabled ? "Yes" : "No",
        config.xp_per_minute, config.min_users,
        config.ignore_afk ? "Yes" : "No",
        g_bot->voice_tracker.count);
    send_prefix_reply(client, msg->channel_id, response);
}

/* --- Phase 4.1: Config get/set Command --- */

static void config_get(yuno_config_t *cfg, const char *key, char *out, size_t out_len) {
    if (strcmp(key, "prefix") == 0 || strcmp(key, "default_prefix") == 0) {
        snprintf(out, out_len, "`%s` = `%s`", key, cfg->default_prefix);
    } else if (strcmp(key, "xp_per_msg") == 0 || strcmp(key, "chat.exppermsg") == 0) {
        snprintf(out, out_len, "`%s` = `%d`", key, cfg->xp_per_msg);
    } else if (strcmp(key, "spam_max_warnings") == 0) {
        snprintf(out, out_len, "`%s` = `%d`", key, cfg->spam_max_warnings);
    } else if (strcmp(key, "dm_message") == 0) {
        snprintf(out, out_len, "`%s` = `%.200s`", key, cfg->dm_message);
    } else if (strcmp(key, "insufficient_permissions_message") == 0 || strcmp(key, "noperm_msg") == 0) {
        snprintf(out, out_len, "`%s` = `%.200s`", key, cfg->insufficient_permissions_message);
    } else if (strcmp(key, "database_path") == 0) {
        snprintf(out, out_len, "`%s` = `%s`", key, cfg->database_path);
    } else if (strcmp(key, "discord.token") == 0 || strcmp(key, "token") == 0) {
        snprintf(out, out_len, "`%s` = `[redacted]`", key);
    } else if (strcmp(key, "master_users") == 0) {
        int pos = snprintf(out, out_len, "`master_users` = [");
        for (int i = 0; i < cfg->master_user_count && pos < (int)out_len - 40; i++) {
            pos += snprintf(out + pos, out_len - pos, "%s`%s`", i > 0 ? ", " : "", cfg->master_users[i]);
        }
        snprintf(out + pos, out_len - pos, "]");
    } else {
        snprintf(out, out_len, "Unknown config key `%s`. Keys: `prefix`, `xp_per_msg`, `spam_max_warnings`, `dm_message`, `noperm_msg`, `master_users`", key);
    }
}

static void config_set(yuno_config_t *cfg, const char *key, const char *value, char *out, size_t out_len) {
    if (strcmp(key, "prefix") == 0 || strcmp(key, "default_prefix") == 0) {
        strncpy(cfg->default_prefix, value, MAX_PREFIX_LEN - 1);
        cfg->default_prefix[MAX_PREFIX_LEN - 1] = '\0';
        snprintf(out, out_len, "Set `%s` to `%s`~ 💕", key, cfg->default_prefix);
    } else if (strcmp(key, "xp_per_msg") == 0 || strcmp(key, "chat.exppermsg") == 0) {
        cfg->xp_per_msg = atoi(value);
        snprintf(out, out_len, "Set `%s` to `%d`~ 💕", key, cfg->xp_per_msg);
    } else if (strcmp(key, "spam_max_warnings") == 0) {
        cfg->spam_max_warnings = atoi(value);
        snprintf(out, out_len, "Set `%s` to `%d`~ 💕", key, cfg->spam_max_warnings);
    } else if (strcmp(key, "dm_message") == 0) {
        strncpy(cfg->dm_message, value, MAX_MESSAGE_LEN - 1);
        cfg->dm_message[MAX_MESSAGE_LEN - 1] = '\0';
        snprintf(out, out_len, "Set `%s`~ 💕", key);
    } else if (strcmp(key, "insufficient_permissions_message") == 0 || strcmp(key, "noperm_msg") == 0) {
        strncpy(cfg->insufficient_permissions_message, value, MAX_MESSAGE_LEN - 1);
        cfg->insufficient_permissions_message[MAX_MESSAGE_LEN - 1] = '\0';
        snprintf(out, out_len, "Set `%s`~ 💕", key);
    } else if (strcmp(key, "discord.token") == 0 || strcmp(key, "token") == 0) {
        snprintf(out, out_len, "Cannot set token at runtime~");
    } else {
        snprintf(out, out_len, "Unknown or read-only config key `%s`~", key);
    }
}

void cmd_config(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *action = get_util_option_value(interaction, "action");
    const char *key = get_util_option_value(interaction, "key");
    const char *value = get_util_option_value(interaction, "value");

    if (!action || !key) {
        send_interaction_reply(client, interaction,
            "Usage: `/config <get|set> <key> [value]`~\n"
            "Keys: `prefix`, `xp_per_msg`, `spam_max_warnings`, `dm_message`, `noperm_msg`, `master_users`");
        return;
    }

    char result[1024];
    if (strcmp(action, "get") == 0) {
        config_get(&g_bot->config, key, result, sizeof(result));
    } else if (strcmp(action, "set") == 0) {
        if (!value) {
            snprintf(result, sizeof(result), "Usage: `/config set <key> <value>`~");
        } else {
            config_set(&g_bot->config, key, value, result, sizeof(result));
        }
    } else {
        snprintf(result, sizeof(result), "Unknown action `%s`. Use `get` or `set`~", action);
    }

    send_interaction_reply(client, interaction, result);
}

void cmd_config_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `config <get|set> <key> [value]` or `config <key> [value]`~\n"
            "Keys: `prefix`, `xp_per_msg`, `spam_max_warnings`, `dm_message`, `noperm_msg`, `master_users`");
        return;
    }

    char action[16] = { 0 };
    char key[64] = { 0 };
    char value[512] = { 0 };

    int parsed = sscanf(args, "%15s %63s %511[^\n]", action, key, value);

    char result[1024];

    if (strcmp(action, "get") == 0 && parsed >= 2) {
        config_get(&g_bot->config, key, result, sizeof(result));
    } else if (strcmp(action, "set") == 0 && parsed >= 3) {
        config_set(&g_bot->config, key, value, result, sizeof(result));
    } else if (parsed >= 2) {
        /* Implicit: config <key> <value> = set */
        config_set(&g_bot->config, action, key, result, sizeof(result));
    } else {
        /* Implicit: config <key> = get */
        config_get(&g_bot->config, action, result, sizeof(result));
    }

    send_prefix_reply(client, msg->channel_id, result);
}

/* --- Phase 4: Configuration Commands --- */

/* Parse channel mention: <#123456> or raw ID */
static u64snowflake parse_channel_mention(const char *str) {
    if (!str) return 0;
    if (str[0] == '<' && str[1] == '#') {
        return strtoull(str + 2, NULL, 10);
    }
    return strtoull(str, NULL, 10);
}

/* set-dm-channel: configure DM forwarding channel */
void cmd_set_dm_channel(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *channel_val = get_util_option_value(interaction, "channel");
    if (!channel_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-dm-channel <channel>`~");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_val);
    if (channel_id == 0) {
        send_interaction_reply(client, interaction, "Invalid channel~");
        return;
    }

    dm_config_t config = {
        .guild_id = interaction->guild_id,
        .channel_id = channel_id,
        .enabled = 1
    };
    db_set_dm_config(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "📬 **DM Forwarding Updated!**\n\n"
        "DMs will now be forwarded to <#%lu>~ 💕",
        (unsigned long)channel_id);
    send_interaction_reply(client, interaction, response);
}

void cmd_set_dm_channel_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        /* If no args, disable DM forwarding */
        db_remove_dm_config(&g_bot->database, msg->guild_id);
        send_prefix_reply(client, msg->channel_id, "📬 DM forwarding disabled for this guild~ 💕");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(args);
    if (channel_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid channel. Use `set-dm-channel <#channel>` or `set-dm-channel` to disable~");
        return;
    }

    dm_config_t config = {
        .guild_id = msg->guild_id,
        .channel_id = channel_id,
        .enabled = 1
    };
    db_set_dm_config(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "📬 **DM Forwarding Updated!**\n\n"
        "DMs will now be forwarded to <#%lu>~ 💕",
        (unsigned long)channel_id);
    send_prefix_reply(client, msg->channel_id, response);
}

/* dm-status: show DM forwarding config */
void cmd_dm_status(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    dm_config_t config;
    if (db_get_dm_config(&g_bot->database, interaction->guild_id, &config) != 0 || !config.enabled) {
        send_interaction_reply(client, interaction, "📬 DM forwarding is **disabled** for this guild~");
        return;
    }

    char response[256];
    snprintf(response, sizeof(response),
        "📬 **DM Forwarding Status**\n\n"
        "**Enabled:** Yes\n"
        "**Channel:** <#%lu>",
        (unsigned long)config.channel_id);
    send_interaction_reply(client, interaction, response);
}

void cmd_dm_status_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    dm_config_t config;
    if (db_get_dm_config(&g_bot->database, msg->guild_id, &config) != 0 || !config.enabled) {
        send_prefix_reply(client, msg->channel_id, "📬 DM forwarding is **disabled** for this guild~");
        return;
    }

    char response[256];
    snprintf(response, sizeof(response),
        "📬 **DM Forwarding Status**\n\n"
        "**Enabled:** Yes\n"
        "**Channel:** <#%lu>",
        (unsigned long)config.channel_id);
    send_prefix_reply(client, msg->channel_id, response);
}

/* set-joinmessage: set welcome DM for new members */
void cmd_set_joinmessage(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *title_val = get_util_option_value(interaction, "title");
    const char *message_val = get_util_option_value(interaction, "message");

    if (!message_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-joinmessage <title> <message>`~");
        return;
    }

    const char *title = title_val ? title_val : "Welcome!";
    db_set_join_message(&g_bot->database, interaction->guild_id, title, message_val);

    char response[512];
    snprintf(response, sizeof(response),
        "👋 **Join Message Updated!**\n\n"
        "**Title:** %s\n"
        "**Message:** %.200s%s",
        title, message_val,
        strlen(message_val) > 200 ? "..." : "");
    send_interaction_reply(client, interaction, response);
}

void cmd_set_joinmessage_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        /* Show current join message */
        char title[256], message[1024];
        if (db_get_join_message(&g_bot->database, msg->guild_id, title, sizeof(title), message, sizeof(message)) == 0 && message[0]) {
            char response[512];
            snprintf(response, sizeof(response),
                "👋 **Current Join Message**\n\n"
                "**Title:** %s\n"
                "**Message:** %.200s%s",
                title, message, strlen(message) > 200 ? "..." : "");
            send_prefix_reply(client, msg->channel_id, response);
        } else {
            send_prefix_reply(client, msg->channel_id, "No join message configured. Usage: `set-joinmessage <title> | <message>`~");
        }
        return;
    }

    /* Parse: title | message (pipe-separated) */
    const char *pipe = strchr(args, '|');
    char title[256] = "Welcome!";
    const char *message;

    if (pipe) {
        size_t title_len = (size_t)(pipe - args);
        if (title_len > sizeof(title) - 1) title_len = sizeof(title) - 1;
        memcpy(title, args, title_len);
        title[title_len] = '\0';
        /* Trim trailing spaces from title */
        while (title_len > 0 && title[title_len - 1] == ' ') title[--title_len] = '\0';
        /* Skip pipe and leading spaces */
        message = pipe + 1;
        while (*message == ' ') message++;
    } else {
        message = args;
    }

    db_set_join_message(&g_bot->database, msg->guild_id, title, message);

    char response[512];
    snprintf(response, sizeof(response),
        "👋 **Join Message Updated!**\n\n"
        "**Title:** %s\n"
        "**Message:** %.200s%s",
        title, message, strlen(message) > 200 ? "..." : "");
    send_prefix_reply(client, msg->channel_id, response);
}

/* set-logchannel: configure activity log channel */
void cmd_set_logchannel(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *channel_val = get_util_option_value(interaction, "channel");
    const char *type_val = get_util_option_value(interaction, "type");
    const char *toggle_val = get_util_option_value(interaction, "enabled");

    if (!channel_val) {
        send_interaction_reply(client, interaction,
            "Usage: `/set-logchannel <channel> [type] [on|off]`\n"
            "Types: `unified`, `voice`, `nickname`, `avatar`, `presence`~");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_val);
    if (channel_id == 0) {
        send_interaction_reply(client, interaction, "Invalid channel~");
        return;
    }

    const char *log_type = type_val ? type_val : "unified";
    int enabled = toggle_val ? parse_toggle(toggle_val) : 1;
    if (enabled < 0) enabled = 1;

    log_channel_t config = {
        .guild_id = interaction->guild_id,
        .channel_id = channel_id,
        .enabled = enabled
    };
    strncpy(config.log_type, log_type, MAX_LOG_TYPE_LEN - 1);
    db_set_log_channel(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "📋 **Log Channel Updated!**\n\n"
        "**Type:** %s\n"
        "**Channel:** <#%lu>\n"
        "**Enabled:** %s",
        log_type, (unsigned long)channel_id,
        enabled ? "Yes" : "No");
    send_interaction_reply(client, interaction, response);
}

void cmd_set_logchannel_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `set-logchannel <#channel> [type] [on|off]`\n"
            "Types: `unified`, `voice`, `nickname`, `avatar`, `presence`~");
        return;
    }

    char channel_str[64], type_str[32] = "unified", toggle_str[8] = "";
    int parsed = sscanf(args, "%63s %31s %7s", channel_str, type_str, toggle_str);
    if (parsed < 1) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-logchannel <#channel> [type] [on|off]`~");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_str);
    if (channel_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid channel~");
        return;
    }

    int enabled = toggle_str[0] ? parse_toggle(toggle_str) : 1;
    if (enabled < 0) enabled = 1;

    log_channel_t config = {
        .guild_id = msg->guild_id,
        .channel_id = channel_id,
        .enabled = enabled
    };
    strncpy(config.log_type, type_str, MAX_LOG_TYPE_LEN - 1);
    db_set_log_channel(&g_bot->database, &config);

    char response[256];
    snprintf(response, sizeof(response),
        "📋 **Log Channel Updated!**\n\n"
        "**Type:** %s\n"
        "**Channel:** <#%lu>\n"
        "**Enabled:** %s",
        type_str, (unsigned long)channel_id,
        enabled ? "Yes" : "No");
    send_prefix_reply(client, msg->channel_id, response);
}

/* log-status: show all configured log channels */
void cmd_log_status(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    log_channel_t channels[10];
    int count = 0;
    db_get_all_log_channels(&g_bot->database, interaction->guild_id, channels, 10, &count);

    log_settings_t settings;
    db_get_log_settings(&g_bot->database, interaction->guild_id, &settings);

    char response[1024];
    char *ptr = response;

    ptr += sprintf(ptr, "📋 **Activity Log Status**\n\n");

    if (count == 0) {
        ptr += sprintf(ptr, "No log channels configured~\n");
    } else {
        for (int i = 0; i < count; i++) {
            ptr += sprintf(ptr, "**%s:** <#%lu> (%s)\n",
                channels[i].log_type,
                (unsigned long)channels[i].channel_id,
                channels[i].enabled ? "enabled" : "disabled");
        }
    }

    ptr += sprintf(ptr, "\n**Flush interval:** %ds\n**Max buffer:** %d",
        settings.flush_interval > 0 ? settings.flush_interval : 30,
        settings.max_buffer_size > 0 ? settings.max_buffer_size : 50);

    send_interaction_reply(client, interaction, response);
}

void cmd_log_status_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    log_channel_t channels[10];
    int count = 0;
    db_get_all_log_channels(&g_bot->database, msg->guild_id, channels, 10, &count);

    log_settings_t settings;
    db_get_log_settings(&g_bot->database, msg->guild_id, &settings);

    char response[1024];
    char *ptr = response;

    ptr += sprintf(ptr, "📋 **Activity Log Status**\n\n");

    if (count == 0) {
        ptr += sprintf(ptr, "No log channels configured~\n");
    } else {
        for (int i = 0; i < count; i++) {
            ptr += sprintf(ptr, "**%s:** <#%lu> (%s)\n",
                channels[i].log_type,
                (unsigned long)channels[i].channel_id,
                channels[i].enabled ? "enabled" : "disabled");
        }
    }

    ptr += sprintf(ptr, "\n**Flush interval:** %ds\n**Max buffer:** %d",
        settings.flush_interval > 0 ? settings.flush_interval : 30,
        settings.max_buffer_size > 0 ? settings.max_buffer_size : 50);

    send_prefix_reply(client, msg->channel_id, response);
}

/* set-logsettings: configure log flush interval and buffer size */
void cmd_set_logsettings(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *interval_val = get_util_option_value(interaction, "interval");
    const char *buffer_val = get_util_option_value(interaction, "buffer_size");

    if (!interval_val && !buffer_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-logsettings <interval> <buffer_size>`~");
        return;
    }

    log_settings_t settings;
    db_get_log_settings(&g_bot->database, interaction->guild_id, &settings);
    settings.guild_id = interaction->guild_id;

    if (interval_val) {
        settings.flush_interval = atoi(interval_val);
        if (settings.flush_interval < 5) settings.flush_interval = 5;
    }
    if (buffer_val) {
        settings.max_buffer_size = atoi(buffer_val);
        if (settings.max_buffer_size < 10) settings.max_buffer_size = 10;
    }

    db_set_log_settings(&g_bot->database, &settings);

    char response[256];
    snprintf(response, sizeof(response),
        "📋 **Log Settings Updated!**\n\n"
        "**Flush interval:** %d seconds\n"
        "**Max buffer size:** %d",
        settings.flush_interval, settings.max_buffer_size);
    send_interaction_reply(client, interaction, response);
}

void cmd_set_logsettings_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-logsettings <interval_secs> <max_buffer_size>`~");
        return;
    }

    char interval_str[16] = "", buffer_str[16] = "";
    sscanf(args, "%15s %15s", interval_str, buffer_str);

    log_settings_t settings;
    db_get_log_settings(&g_bot->database, msg->guild_id, &settings);
    settings.guild_id = msg->guild_id;

    if (interval_str[0]) {
        settings.flush_interval = atoi(interval_str);
        if (settings.flush_interval < 5) settings.flush_interval = 5;
    }
    if (buffer_str[0]) {
        settings.max_buffer_size = atoi(buffer_str);
        if (settings.max_buffer_size < 10) settings.max_buffer_size = 10;
    }

    db_set_log_settings(&g_bot->database, &settings);

    char response[256];
    snprintf(response, sizeof(response),
        "📋 **Log Settings Updated!**\n\n"
        "**Flush interval:** %d seconds\n"
        "**Max buffer size:** %d",
        settings.flush_interval, settings.max_buffer_size);
    send_prefix_reply(client, msg->channel_id, response);
}

/* --- Phase 5.5: Invite Link Filter --- */

void cmd_set_invitefilter(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *toggle_val = get_util_option_value(interaction, "enabled");
    if (!toggle_val) {
        send_interaction_reply(client, interaction, "Usage: `/set-invitefilter <on|off>`~");
        return;
    }

    int val = parse_toggle(toggle_val);
    if (val < 0) {
        send_interaction_reply(client, interaction, "Usage: `/set-invitefilter <on|off>`~");
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, interaction->guild_id, &settings) != 0) {
        settings.guild_id = interaction->guild_id;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
        settings.leveling_enabled = 1;
    }
    settings.invite_filter_enabled = val;
    db_set_guild_settings(&g_bot->database, &settings);

    char response[128];
    snprintf(response, sizeof(response),
        "Invite filter is now **%s** on this guild~ %s",
        val ? "enabled" : "disabled", val ? "🛡️" : "💕");
    send_interaction_reply(client, interaction, response);
}

void cmd_set_invitefilter_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    int val = parse_toggle(args);
    if (val < 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `set-invitefilter <on|off>`~");
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) != 0) {
        settings.guild_id = msg->guild_id;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
        settings.leveling_enabled = 1;
    }
    settings.invite_filter_enabled = val;
    db_set_guild_settings(&g_bot->database, &settings);

    char response[128];
    snprintf(response, sizeof(response),
        "Invite filter is now **%s** on this guild~ %s",
        val ? "enabled" : "disabled", val ? "🛡️" : "💕");
    send_prefix_reply(client, msg->channel_id, response);
}

/* --- Phase 7: Admin/Master Commands --- */

/* send: send a message through the bot to a channel */
void cmd_send(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *channel_val = get_util_option_value(interaction, "channel");
    const char *message_val = get_util_option_value(interaction, "message");

    if (!channel_val || !message_val) {
        send_interaction_reply(client, interaction, "Usage: `/send <channel> <message>`~");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_val);
    if (channel_id == 0) {
        send_interaction_reply(client, interaction, "Invalid channel~");
        return;
    }

    struct discord_create_message params = { .content = (char *)message_val };
    discord_create_message(client, channel_id, &params, NULL);

    char resp[256];
    snprintf(resp, sizeof(resp), "Message sent to <#%lu>~ 💕", (unsigned long)channel_id);
    send_interaction_reply(client, interaction, resp);
}

void cmd_send_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `send <#channel> <message>`~");
        return;
    }

    char channel_str[64] = "";
    char message[2000] = "";
    if (sscanf(args, "%63s %[^\n]", channel_str, message) < 2) {
        send_prefix_reply(client, msg->channel_id, "Usage: `send <#channel> <message>`~");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_str);
    if (channel_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid channel~");
        return;
    }

    struct discord_create_message params = { .content = message };
    discord_create_message(client, channel_id, &params, NULL);

    char resp[256];
    snprintf(resp, sizeof(resp), "Message sent to <#%lu>~ 💕", (unsigned long)channel_id);
    send_prefix_reply(client, msg->channel_id, resp);
}

/* reply: reply to a DM inbox message */
void cmd_reply_dm(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *user_val = get_util_option_value(interaction, "user");
    const char *message_val = get_util_option_value(interaction, "message");

    if (!user_val || !message_val) {
        send_interaction_reply(client, interaction, "Usage: `/reply <user> <message>`~");
        return;
    }

    uint64_t user_id = parse_user_mention(user_val);
    if (user_id == 0) {
        send_interaction_reply(client, interaction, "Invalid user~");
        return;
    }

    /* Create DM channel and send */
    struct discord_create_dm dm_params = { .recipient_id = user_id };
    struct discord_channel dm_channel = { 0 };
    struct discord_ret_channel ret = { .sync = &dm_channel };

    if (discord_create_dm(client, &dm_params, &ret) != CCORD_OK) {
        send_interaction_reply(client, interaction, "Failed to create DM channel~");
        return;
    }

    struct discord_create_message msg_params = { .content = (char *)message_val };
    discord_create_message(client, dm_channel.id, &msg_params, NULL);
    discord_channel_cleanup(&dm_channel);

    char resp[256];
    snprintf(resp, sizeof(resp), "Reply sent to <@%lu>~ 💕", (unsigned long)user_id);
    send_interaction_reply(client, interaction, resp);
}

void cmd_reply_dm_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `reply <@user> <message>`~");
        return;
    }

    char user_str[64] = "";
    char message[2000] = "";
    if (sscanf(args, "%63s %[^\n]", user_str, message) < 2) {
        send_prefix_reply(client, msg->channel_id, "Usage: `reply <@user> <message>`~");
        return;
    }

    uint64_t user_id = parse_user_mention(user_str);
    if (user_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid user~");
        return;
    }

    struct discord_create_dm dm_params = { .recipient_id = user_id };
    struct discord_channel dm_channel = { 0 };
    struct discord_ret_channel ret = { .sync = &dm_channel };

    if (discord_create_dm(client, &dm_params, &ret) != CCORD_OK) {
        send_prefix_reply(client, msg->channel_id, "Failed to create DM channel~");
        return;
    }

    struct discord_create_message msg_params = { .content = message };
    discord_create_message(client, dm_channel.id, &msg_params, NULL);
    discord_channel_cleanup(&dm_channel);

    char resp[256];
    snprintf(resp, sizeof(resp), "Reply sent to <@%lu>~ 💕", (unsigned long)user_id);
    send_prefix_reply(client, msg->channel_id, resp);
}

/* inbox: view DM inbox as Discord command */
void cmd_inbox(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    dm_inbox_t dms[10];
    int count = 0;
    db_get_dms(&g_bot->database, dms, 10, &count);

    char resp[2048];
    char *ptr = resp;
    int unread = 0;
    db_get_unread_dm_count(&g_bot->database);

    ptr += sprintf(ptr, "📬 **DM Inbox** (%d messages)\n\n", count);

    if (count == 0) {
        ptr += sprintf(ptr, "No DMs in inbox~");
    } else {
        for (int i = 0; i < count && (ptr - resp) < 1800; i++) {
            char time_buf[32];
            struct tm *tm = localtime(&dms[i].timestamp);
            strftime(time_buf, sizeof(time_buf), "%m/%d %H:%M", tm);
            ptr += sprintf(ptr, "%s **%s** (`%lu`): %.100s%s\n",
                dms[i].read_status ? "" : "**[NEW]** ",
                dms[i].username, (unsigned long)dms[i].user_id,
                dms[i].content,
                strlen(dms[i].content) > 100 ? "..." : "");
            if (!dms[i].read_status) {
                unread++;
                db_mark_dm_read(&g_bot->database, dms[i].id);
            }
        }
        if (unread > 0) {
            ptr += sprintf(ptr, "\n*Marked %d as read*", unread);
        }
    }

    send_interaction_reply(client, interaction, resp);
}

void cmd_inbox_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    dm_inbox_t dms[10];
    int count = 0;
    db_get_dms(&g_bot->database, dms, 10, &count);

    char resp[2048];
    char *ptr = resp;
    int unread = 0;

    ptr += sprintf(ptr, "📬 **DM Inbox** (%d messages)\n\n", count);

    if (count == 0) {
        ptr += sprintf(ptr, "No DMs in inbox~");
    } else {
        for (int i = 0; i < count && (ptr - resp) < 1800; i++) {
            char time_buf[32];
            struct tm *tm = localtime(&dms[i].timestamp);
            strftime(time_buf, sizeof(time_buf), "%m/%d %H:%M", tm);
            ptr += sprintf(ptr, "%s **%s** (`%lu`): %.100s%s\n",
                dms[i].read_status ? "" : "**[NEW]** ",
                dms[i].username, (unsigned long)dms[i].user_id,
                dms[i].content,
                strlen(dms[i].content) > 100 ? "..." : "");
            if (!dms[i].read_status) {
                unread++;
                db_mark_dm_read(&g_bot->database, dms[i].id);
            }
        }
        if (unread > 0) {
            ptr += sprintf(ptr, "\n*Marked %d as read*", unread);
        }
    }

    send_prefix_reply(client, msg->channel_id, resp);
}

/* debug-error: trigger a test error message */
void cmd_debug_error(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    send_interaction_reply(client, interaction, "Test error triggered~ Check error channel (if configured).");

    /* Send to error channels in all guilds that have one configured */
    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, interaction->guild_id, &settings) == 0 &&
        settings.error_channel_id != 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg),
            "⚠️ **Test Error**\nTriggered by <@%lu> via `/debug-error`",
            (unsigned long)interaction->member->user->id);
        struct discord_create_message params = { .content = err_msg };
        discord_create_message(client, settings.error_channel_id, &params, NULL);
    }
}

void cmd_debug_error_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    send_prefix_reply(client, msg->channel_id, "Test error triggered~ Check error channel (if configured).");

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) == 0 &&
        settings.error_channel_id != 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg),
            "⚠️ **Test Error**\nTriggered by <@%lu> via `debug-error`",
            (unsigned long)msg->author->id);
        struct discord_create_message params = { .content = err_msg };
        discord_create_message(client, settings.error_channel_id, &params, NULL);
    }
}

/* init-guild: manually initialize guild settings */
void cmd_init_guild(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, interaction->guild_id, &settings) == 0) {
        send_interaction_reply(client, interaction, "Guild already initialized~ Use config commands to modify settings.");
        return;
    }

    memset(&settings, 0, sizeof(guild_settings_t));
    settings.guild_id = interaction->guild_id;
    strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
    settings.leveling_enabled = 1;
    db_set_guild_settings(&g_bot->database, &settings);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Guild initialized~ 💕\n"
        "**Prefix:** `%s`\n"
        "**Leveling:** enabled\n"
        "**Spam filter:** disabled",
        settings.prefix);
    send_interaction_reply(client, interaction, resp);
}

void cmd_init_guild_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) == 0) {
        send_prefix_reply(client, msg->channel_id, "Guild already initialized~ Use config commands to modify settings.");
        return;
    }

    memset(&settings, 0, sizeof(guild_settings_t));
    settings.guild_id = msg->guild_id;
    strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
    settings.leveling_enabled = 1;
    db_set_guild_settings(&g_bot->database, &settings);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Guild initialized~ 💕\n"
        "**Prefix:** `%s`\n"
        "**Leveling:** enabled\n"
        "**Spam filter:** disabled",
        settings.prefix);
    send_prefix_reply(client, msg->channel_id, resp);
}

/* add-masteruser: dynamically add master users (runtime only, not persisted to config file) */
void cmd_add_masteruser(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *user_val = get_util_option_value(interaction, "user");
    if (!user_val) {
        send_interaction_reply(client, interaction, "Usage: `/add-masteruser <user>`~");
        return;
    }

    uint64_t user_id = parse_user_mention(user_val);
    if (user_id == 0) {
        send_interaction_reply(client, interaction, "Invalid user~");
        return;
    }

    /* Check if already master */
    if (bot_is_master_user(g_bot, user_id)) {
        send_interaction_reply(client, interaction, "User is already a master user~");
        return;
    }

    /* Add to runtime config */
    if (g_bot->config.master_user_count >= MAX_MASTER_USERS) {
        send_interaction_reply(client, interaction, "Master user limit reached~");
        return;
    }

    snprintf(g_bot->config.master_users[g_bot->config.master_user_count],
        sizeof(g_bot->config.master_users[0]), "%lu", (unsigned long)user_id);
    g_bot->config.master_user_count++;

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Added <@%lu> as a master user~ 💕\n*Note: This is runtime only — add to config.json to persist across restarts.*",
        (unsigned long)user_id);
    send_interaction_reply(client, interaction, resp);
}

void cmd_add_masteruser_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `add-masteruser <@user>`~");
        return;
    }

    uint64_t user_id = parse_user_mention(args);
    if (user_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid user~");
        return;
    }

    if (bot_is_master_user(g_bot, user_id)) {
        send_prefix_reply(client, msg->channel_id, "User is already a master user~");
        return;
    }

    if (g_bot->config.master_user_count >= MAX_MASTER_USERS) {
        send_prefix_reply(client, msg->channel_id, "Master user limit reached~");
        return;
    }

    snprintf(g_bot->config.master_users[g_bot->config.master_user_count],
        sizeof(g_bot->config.master_users[0]), "%lu", (unsigned long)user_id);
    g_bot->config.master_user_count++;

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Added <@%lu> as a master user~ 💕\n*Note: This is runtime only — add to config.json to persist across restarts.*",
        (unsigned long)user_id);
    send_prefix_reply(client, msg->channel_id, resp);
}

/* --- Phase 5.7: Error Channel Logging --- */

void cmd_drop_errors_on(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *channel_val = get_util_option_value(interaction, "channel");

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, interaction->guild_id, &settings) != 0) {
        settings.guild_id = interaction->guild_id;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
        settings.leveling_enabled = 1;
    }

    if (!channel_val) {
        /* Disable error logging */
        settings.error_channel_id = 0;
        db_set_guild_settings(&g_bot->database, &settings);
        send_interaction_reply(client, interaction, "Error channel logging **disabled**~ 💕");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(channel_val);
    if (channel_id == 0) {
        send_interaction_reply(client, interaction, "Invalid channel~");
        return;
    }

    settings.error_channel_id = channel_id;
    db_set_guild_settings(&g_bot->database, &settings);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Errors will now be logged to <#%lu>~ 💕",
        (unsigned long)channel_id);
    send_interaction_reply(client, interaction, resp);
}

void cmd_drop_errors_on_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    guild_settings_t settings;
    if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) != 0) {
        settings.guild_id = msg->guild_id;
        strncpy(settings.prefix, g_bot->config.default_prefix, MAX_PREFIX_LEN - 1);
        settings.leveling_enabled = 1;
    }

    if (!args || strlen(args) == 0) {
        /* Disable error logging */
        settings.error_channel_id = 0;
        db_set_guild_settings(&g_bot->database, &settings);
        send_prefix_reply(client, msg->channel_id, "Error channel logging **disabled**~ 💕");
        return;
    }

    u64snowflake channel_id = parse_channel_mention(args);
    if (channel_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid channel. Usage: `drop-errors-on <#channel>` or `drop-errors-on` to disable~");
        return;
    }

    settings.error_channel_id = channel_id;
    db_set_guild_settings(&g_bot->database, &settings);

    char resp[256];
    snprintf(resp, sizeof(resp),
        "Errors will now be logged to <#%lu>~ 💕",
        (unsigned long)channel_id);
    send_prefix_reply(client, msg->channel_id, resp);
}

/* --- Phase 5.2: Mention Response Commands --- */

void cmd_add_mentionresponse(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *user_val = get_util_option_value(interaction, "user");
    const char *trigger_val = get_util_option_value(interaction, "trigger");
    const char *response_val = get_util_option_value(interaction, "response");
    const char *image_val = get_util_option_value(interaction, "image_url");

    if (!user_val || !trigger_val || !response_val) {
        send_interaction_reply(client, interaction,
            "Usage: `/add-mentionresponse <user> <trigger> <response> [image_url]`~");
        return;
    }

    uint64_t user_id = parse_user_mention(user_val);
    if (user_id == 0) {
        send_interaction_reply(client, interaction, "Invalid user~");
        return;
    }

    mention_response_t mr = {
        .guild_id = interaction->guild_id,
        .user_id = user_id
    };
    strncpy(mr.trigger, trigger_val, sizeof(mr.trigger) - 1);
    strncpy(mr.response, response_val, sizeof(mr.response) - 1);
    if (image_val) strncpy(mr.image_url, image_val, sizeof(mr.image_url) - 1);

    db_add_mention_response(&g_bot->database, &mr);

    char resp[512];
    snprintf(resp, sizeof(resp),
        "💬 **Mention Response Added!**\n\n"
        "**User:** <@%lu>\n"
        "**Trigger:** %s\n"
        "**Response:** %.200s%s",
        (unsigned long)user_id, trigger_val, response_val,
        strlen(response_val) > 200 ? "..." : "");
    send_interaction_reply(client, interaction, resp);
}

void cmd_add_mentionresponse_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `add-mentionresponse <@user> <trigger> | <response> [| image_url]`~");
        return;
    }

    /* Parse: <@user> <trigger> | <response> [| image_url] */
    char user_str[64] = "";
    char rest[1024] = "";
    if (sscanf(args, "%63s %[^\n]", user_str, rest) < 2) {
        send_prefix_reply(client, msg->channel_id,
            "Usage: `add-mentionresponse <@user> <trigger> | <response> [| image_url]`~");
        return;
    }

    uint64_t user_id = parse_user_mention(user_str);
    if (user_id == 0) {
        send_prefix_reply(client, msg->channel_id, "Invalid user~");
        return;
    }

    /* Split rest by pipe: trigger | response [| image_url] */
    char *first_pipe = strchr(rest, '|');
    if (!first_pipe) {
        send_prefix_reply(client, msg->channel_id,
            "Use `|` to separate trigger from response: `add-mentionresponse <@user> <trigger> | <response>`~");
        return;
    }

    *first_pipe = '\0';
    char *trigger = rest;
    char *response_text = first_pipe + 1;

    /* Trim whitespace */
    while (*trigger == ' ') trigger++;
    char *end = trigger + strlen(trigger) - 1;
    while (end > trigger && *end == ' ') *end-- = '\0';
    while (*response_text == ' ') response_text++;

    /* Check for image_url after second pipe */
    char *image_url = NULL;
    char *second_pipe = strchr(response_text, '|');
    if (second_pipe) {
        *second_pipe = '\0';
        image_url = second_pipe + 1;
        while (*image_url == ' ') image_url++;
        /* Trim trailing whitespace from response */
        end = response_text + strlen(response_text) - 1;
        while (end > response_text && *end == ' ') *end-- = '\0';
    }

    mention_response_t mr = {
        .guild_id = msg->guild_id,
        .user_id = user_id
    };
    strncpy(mr.trigger, trigger, sizeof(mr.trigger) - 1);
    strncpy(mr.response, response_text, sizeof(mr.response) - 1);
    if (image_url && image_url[0]) strncpy(mr.image_url, image_url, sizeof(mr.image_url) - 1);

    db_add_mention_response(&g_bot->database, &mr);

    char resp[512];
    snprintf(resp, sizeof(resp),
        "💬 **Mention Response Added!**\n\n"
        "**User:** <@%lu>\n"
        "**Trigger:** %s\n"
        "**Response:** %.200s%s",
        (unsigned long)user_id, trigger, response_text,
        strlen(response_text) > 200 ? "..." : "");
    send_prefix_reply(client, msg->channel_id, resp);
}

void cmd_del_mentionresponse(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    const char *trigger_val = get_util_option_value(interaction, "trigger");
    if (!trigger_val) {
        send_interaction_reply(client, interaction, "Usage: `/del-mentionresponse <trigger>`~");
        return;
    }

    if (db_remove_mention_response(&g_bot->database, interaction->guild_id, trigger_val) == 0) {
        char resp[256];
        snprintf(resp, sizeof(resp), "💬 Removed mention response for trigger **%s**~ 💕", trigger_val);
        send_interaction_reply(client, interaction, resp);
    } else {
        send_interaction_reply(client, interaction, "No mention response found with that trigger~");
    }
}

void cmd_del_mentionresponse_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    if (!args || strlen(args) == 0) {
        send_prefix_reply(client, msg->channel_id, "Usage: `del-mentionresponse <trigger>`~");
        return;
    }

    if (db_remove_mention_response(&g_bot->database, msg->guild_id, args) == 0) {
        char resp[256];
        snprintf(resp, sizeof(resp), "💬 Removed mention response for trigger **%s**~ 💕", args);
        send_prefix_reply(client, msg->channel_id, resp);
    } else {
        send_prefix_reply(client, msg->channel_id, "No mention response found with that trigger~");
    }
}

void cmd_mentionresponses(struct discord *client, const struct discord_interaction *interaction) {
    if (!bot_is_master_user(g_bot, interaction->member->user->id)) {
        send_interaction_reply(client, interaction, g_bot->config.insufficient_permissions_message);
        return;
    }

    mention_response_t results[20];
    int count = 0;
    db_get_mention_responses(&g_bot->database, interaction->guild_id, results, 20, &count);

    char resp[2048];
    char *ptr = resp;
    ptr += sprintf(ptr, "💬 **Mention Responses** (%d total)\n\n", count);

    if (count == 0) {
        ptr += sprintf(ptr, "No mention responses configured~");
    } else {
        for (int i = 0; i < count && (ptr - resp) < 1800; i++) {
            ptr += sprintf(ptr, "**%d.** <@%lu> + `%s` → %.100s%s%s\n",
                i + 1,
                (unsigned long)results[i].user_id,
                results[i].trigger,
                results[i].response,
                strlen(results[i].response) > 100 ? "..." : "",
                results[i].image_url[0] ? " 🖼️" : "");
        }
    }

    send_interaction_reply(client, interaction, resp);
}

void cmd_mentionresponses_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    if (!bot_is_master_user(g_bot, msg->author->id)) {
        send_prefix_reply(client, msg->channel_id, g_bot->config.insufficient_permissions_message);
        return;
    }

    mention_response_t results[20];
    int count = 0;
    db_get_mention_responses(&g_bot->database, msg->guild_id, results, 20, &count);

    char resp[2048];
    char *ptr = resp;
    ptr += sprintf(ptr, "💬 **Mention Responses** (%d total)\n\n", count);

    if (count == 0) {
        ptr += sprintf(ptr, "No mention responses configured~");
    } else {
        for (int i = 0; i < count && (ptr - resp) < 1800; i++) {
            ptr += sprintf(ptr, "**%d.** <@%lu> + `%s` → %.100s%s%s\n",
                i + 1,
                (unsigned long)results[i].user_id,
                results[i].trigger,
                results[i].response,
                strlen(results[i].response) > 100 ? "..." : "",
                results[i].image_url[0] ? " 🖼️" : "");
        }
    }

    send_prefix_reply(client, msg->channel_id, resp);
}

/* ===== Phase 7: /list-command ===== */

/* External reference to command table from bot.c */
typedef void (*_lc_prefix_handler_t)(struct discord *, const struct discord_message *, const char *);
typedef void (*_lc_slash_handler_t)(struct discord *, const struct discord_interaction *);

typedef struct {
    const char *name;
    const char *alias;
    _lc_prefix_handler_t prefix_handler;
    _lc_slash_handler_t slash_handler;
} _lc_command_entry_t;

extern const _lc_command_entry_t g_commands[];
extern const int g_num_commands;

void cmd_list_command(struct discord *client, const struct discord_interaction *interaction) {
    char resp[3800];
    char *ptr = resp;
    ptr += sprintf(ptr, "💕 **All Available Commands** 💕\n\n");

    ptr += sprintf(ptr, "**Discord Commands (%d):**\n", g_num_commands);
    for (int i = 0; i < g_num_commands && (ptr - resp) < 3500; i++) {
        ptr += sprintf(ptr, "`%s`", g_commands[i].name);
        if (g_commands[i].alias) {
            ptr += sprintf(ptr, " (`%s`)", g_commands[i].alias);
        }
        if (i < g_num_commands - 1) ptr += sprintf(ptr, ", ");
    }

    ptr += sprintf(ptr, "\n\n**Terminal Commands:** ");
    ptr += sprintf(ptr, "`help`, `servers`, `inbox`, `botban`, `botunban`, `botbanlist`, "
                         "`status`, `commands`, `watch`, `texportbans`, `timportbans`, `quit`");

    ptr += sprintf(ptr, "\n\nTotal: **%d** unique commands", g_num_commands + 12);

    send_interaction_reply(client, interaction, resp);
}

void cmd_list_command_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;

    char resp[3800];
    char *ptr = resp;
    ptr += sprintf(ptr, "💕 **All Available Commands** 💕\n\n");

    ptr += sprintf(ptr, "**Discord Commands (%d):**\n", g_num_commands);
    for (int i = 0; i < g_num_commands && (ptr - resp) < 3500; i++) {
        ptr += sprintf(ptr, "`%s`", g_commands[i].name);
        if (g_commands[i].alias) {
            ptr += sprintf(ptr, " (`%s`)", g_commands[i].alias);
        }
        if (i < g_num_commands - 1) ptr += sprintf(ptr, ", ");
    }

    ptr += sprintf(ptr, "\n\n**Terminal Commands:** ");
    ptr += sprintf(ptr, "`help`, `servers`, `inbox`, `botban`, `botunban`, `botbanlist`, "
                         "`status`, `commands`, `watch`, `texportbans`, `timportbans`, `quit`");

    ptr += sprintf(ptr, "\n\nTotal: **%d** unique commands", g_num_commands + 12);

    send_prefix_reply(client, msg->channel_id, resp);
}
