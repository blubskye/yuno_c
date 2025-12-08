/*
 * Yuno Gasai 2 (C Edition) - Utility Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_COMMANDS_UTILITY_H
#define YUNO_COMMANDS_UTILITY_H

#include <concord/discord.h>

/* Slash command handlers */
void cmd_ping(struct discord *client, const struct discord_interaction *interaction);
void cmd_help(struct discord *client, const struct discord_interaction *interaction);
void cmd_source(struct discord *client, const struct discord_interaction *interaction);
void cmd_prefix(struct discord *client, const struct discord_interaction *interaction);
void cmd_auto_clean(struct discord *client, const struct discord_interaction *interaction);
void cmd_delay(struct discord *client, const struct discord_interaction *interaction);
void cmd_xp(struct discord *client, const struct discord_interaction *interaction);
void cmd_leaderboard(struct discord *client, const struct discord_interaction *interaction);

/* Prefix command handlers */
void cmd_ping_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_help_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_source_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_prefix_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_auto_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_delay_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_leaderboard_prefix(struct discord *client, const struct discord_message *msg, const char *args);

#endif /* YUNO_COMMANDS_UTILITY_H */
