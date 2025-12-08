/*
 * Yuno Gasai 2 (C Edition) - Bot Core
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "bot.h"
#include "commands/moderation.h"
#include "commands/utility.h"
#include "commands/fun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

/* Global bot instance for callbacks */
yuno_bot_t *g_bot = NULL;

int bot_init(yuno_bot_t *bot, const yuno_config_t *config) {
    memset(bot, 0, sizeof(yuno_bot_t));
    memcpy(&bot->config, config, sizeof(yuno_config_t));

    /* Open database */
    if (db_open(&bot->database, config->database_path) != 0) {
        fprintf(stderr, "💔 Failed to open database\n");
        return -1;
    }

    /* Create Discord client */
    bot->client = discord_init(config->discord_token);
    if (!bot->client) {
        fprintf(stderr, "💔 Failed to initialize Discord client\n");
        db_close(&bot->database);
        return -1;
    }

    /* Set global bot pointer for callbacks */
    g_bot = bot;

    /* Set up event handlers */
    discord_set_on_ready(bot->client, on_ready);
    discord_set_on_message_create(bot->client, on_message_create);
    discord_set_on_interaction_create(bot->client, on_interaction_create);

    return 0;
}

void bot_cleanup(yuno_bot_t *bot) {
    if (bot->client) {
        discord_cleanup(bot->client);
        bot->client = NULL;
    }
    db_close(&bot->database);
    g_bot = NULL;
}

int bot_run(yuno_bot_t *bot) {
    bot->running = 1;
    discord_run(bot->client);
    return 0;
}

void bot_stop(yuno_bot_t *bot) {
    bot->running = 0;
    if (bot->client) {
        discord_shutdown(bot->client);
    }
}

void on_ready(struct discord *client, const struct discord_ready *event) {
    (void)client;
    printf("💕 Yuno is online! Logged in as %s~ 💕\n", event->user->username);
    printf("💗 I'm watching over your servers for you~ 💗\n");

    /* Register slash commands */
    bot_register_commands(g_bot);
}

void on_message_create(struct discord *client, const struct discord_message *msg) {
    char prefix[MAX_PREFIX_LEN];
    char *content;
    char *command;
    char *args;
    size_t prefix_len;

    /* Ignore bots */
    if (msg->author->bot) return;

    /* Handle DMs */
    if (msg->guild_id == 0) {
        struct discord_create_message params = { .content = g_bot->config.dm_message };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    /* Get guild prefix */
    db_get_prefix(&g_bot->database, msg->guild_id, g_bot->config.default_prefix, prefix, sizeof(prefix));
    prefix_len = strlen(prefix);

    /* Check for prefix */
    if (strncmp(msg->content, prefix, prefix_len) != 0) {
        /* Add XP for chatting */
        guild_settings_t settings;
        if (db_get_guild_settings(&g_bot->database, msg->guild_id, &settings) != 0 || settings.leveling_enabled) {
            int xp_gain = 15 + (rand() % 11);
            db_add_xp(&g_bot->database, msg->author->id, msg->guild_id, xp_gain);

            /* Check for level up */
            user_xp_t user_xp;
            db_get_user_xp(&g_bot->database, msg->author->id, msg->guild_id, &user_xp);
            int new_level = (int)sqrt(user_xp.xp / 100.0);

            if (new_level > user_xp.level) {
                db_set_level(&g_bot->database, msg->author->id, msg->guild_id, new_level);

                char level_msg[256];
                snprintf(level_msg, sizeof(level_msg),
                    "✨ **Level Up!** ✨\nCongratulations <@%lu>! You've reached level **%d**! 💕",
                    (unsigned long)msg->author->id, new_level);

                struct discord_create_message params = { .content = level_msg };
                discord_create_message(client, msg->channel_id, &params, NULL);
            }
        }
        return;
    }

    /* Parse command */
    content = strdup(msg->content + prefix_len);
    command = strtok(content, " \t\n");
    if (!command) {
        free(content);
        return;
    }

    /* Convert command to lowercase */
    for (char *p = command; *p; p++) {
        *p = tolower(*p);
    }

    /* Get remaining args */
    args = strtok(NULL, "");
    if (!args) args = "";

    /* Route to command handlers */
    if (strcmp(command, "ping") == 0) {
        cmd_ping_prefix(client, msg, args);
    } else if (strcmp(command, "help") == 0) {
        cmd_help_prefix(client, msg, args);
    } else if (strcmp(command, "source") == 0) {
        cmd_source_prefix(client, msg, args);
    } else if (strcmp(command, "prefix") == 0) {
        cmd_prefix_prefix(client, msg, args);
    } else if (strcmp(command, "ban") == 0) {
        cmd_ban_prefix(client, msg, args);
    } else if (strcmp(command, "kick") == 0) {
        cmd_kick_prefix(client, msg, args);
    } else if (strcmp(command, "unban") == 0) {
        cmd_unban_prefix(client, msg, args);
    } else if (strcmp(command, "timeout") == 0) {
        cmd_timeout_prefix(client, msg, args);
    } else if (strcmp(command, "clean") == 0) {
        cmd_clean_prefix(client, msg, args);
    } else if (strcmp(command, "mod-stats") == 0 || strcmp(command, "modstats") == 0) {
        cmd_mod_stats_prefix(client, msg, args);
    } else if (strcmp(command, "auto-clean") == 0 || strcmp(command, "autoclean") == 0) {
        cmd_auto_clean_prefix(client, msg, args);
    } else if (strcmp(command, "delay") == 0) {
        cmd_delay_prefix(client, msg, args);
    } else if (strcmp(command, "xp") == 0 || strcmp(command, "level") == 0 || strcmp(command, "rank") == 0) {
        cmd_xp_prefix(client, msg, args);
    } else if (strcmp(command, "leaderboard") == 0 || strcmp(command, "lb") == 0 || strcmp(command, "top") == 0) {
        cmd_leaderboard_prefix(client, msg, args);
    } else if (strcmp(command, "8ball") == 0) {
        cmd_8ball_prefix(client, msg, args);
    }

    free(content);
}

void on_interaction_create(struct discord *client, const struct discord_interaction *interaction) {
    if (interaction->type != DISCORD_INTERACTION_APPLICATION_COMMAND) return;

    const char *name = interaction->data->name;

    if (strcmp(name, "ping") == 0) {
        cmd_ping(client, interaction);
    } else if (strcmp(name, "help") == 0) {
        cmd_help(client, interaction);
    } else if (strcmp(name, "source") == 0) {
        cmd_source(client, interaction);
    } else if (strcmp(name, "prefix") == 0) {
        cmd_prefix(client, interaction);
    } else if (strcmp(name, "ban") == 0) {
        cmd_ban(client, interaction);
    } else if (strcmp(name, "kick") == 0) {
        cmd_kick(client, interaction);
    } else if (strcmp(name, "unban") == 0) {
        cmd_unban(client, interaction);
    } else if (strcmp(name, "timeout") == 0) {
        cmd_timeout(client, interaction);
    } else if (strcmp(name, "clean") == 0) {
        cmd_clean(client, interaction);
    } else if (strcmp(name, "mod-stats") == 0) {
        cmd_mod_stats(client, interaction);
    } else if (strcmp(name, "auto-clean") == 0) {
        cmd_auto_clean(client, interaction);
    } else if (strcmp(name, "delay") == 0) {
        cmd_delay(client, interaction);
    } else if (strcmp(name, "xp") == 0) {
        cmd_xp(client, interaction);
    } else if (strcmp(name, "leaderboard") == 0) {
        cmd_leaderboard(client, interaction);
    } else if (strcmp(name, "8ball") == 0) {
        cmd_8ball(client, interaction);
    }
}

int bot_register_commands(yuno_bot_t *bot) {
    struct discord_application_command commands[] = {
        /* Utility commands */
        { .name = "ping", .description = "Check if Yuno is awake~ 💓", .type = DISCORD_APPLICATION_CHAT_INPUT },
        { .name = "help", .description = "See what Yuno can do for you~ 💕", .type = DISCORD_APPLICATION_CHAT_INPUT },
        { .name = "source", .description = "See Yuno's source code~ 📜", .type = DISCORD_APPLICATION_CHAT_INPUT },
    };

    /* Note: Full command registration with options would be more complex */
    /* This is a simplified version - Concord has its own command registration API */

    printf("💕 Registering slash commands~\n");
    return 0;
}

int bot_is_master_user(yuno_bot_t *bot, uint64_t user_id) {
    char user_str[32];
    snprintf(user_str, sizeof(user_str), "%lu", (unsigned long)user_id);
    return config_is_master_user(&bot->config, user_str);
}

uint64_t parse_user_mention(const char *mention) {
    const char *start;
    char *end;
    uint64_t id;

    /* Check for <@123> or <@!123> format */
    if (mention[0] == '<' && mention[1] == '@') {
        start = mention + 2;
        if (*start == '!') start++;
        id = strtoull(start, &end, 10);
        if (*end == '>') return id;
    }

    /* Try parsing as raw ID */
    id = strtoull(mention, &end, 10);
    if (*end == '\0') return id;

    return 0;
}

void format_duration(int64_t seconds, char *buffer, size_t len) {
    if (seconds < 60) {
        snprintf(buffer, len, "%ld seconds", (long)seconds);
    } else if (seconds < 3600) {
        snprintf(buffer, len, "%ld minutes", (long)(seconds / 60));
    } else if (seconds < 86400) {
        snprintf(buffer, len, "%ld hours", (long)(seconds / 3600));
    } else {
        snprintf(buffer, len, "%ld days", (long)(seconds / 86400));
    }
}
