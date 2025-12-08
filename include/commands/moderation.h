/*
 * Yuno Gasai 2 (C Edition) - Moderation Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_COMMANDS_MODERATION_H
#define YUNO_COMMANDS_MODERATION_H

#include <concord/discord.h>

/* Slash command handlers */
void cmd_ban(struct discord *client, const struct discord_interaction *interaction);
void cmd_kick(struct discord *client, const struct discord_interaction *interaction);
void cmd_unban(struct discord *client, const struct discord_interaction *interaction);
void cmd_timeout(struct discord *client, const struct discord_interaction *interaction);
void cmd_clean(struct discord *client, const struct discord_interaction *interaction);
void cmd_mod_stats(struct discord *client, const struct discord_interaction *interaction);
void cmd_scan_bans(struct discord *client, const struct discord_interaction *interaction);

/* Prefix command handlers */
void cmd_ban_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_kick_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_unban_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_timeout_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_mod_stats_prefix(struct discord *client, const struct discord_message *msg, const char *args);

#endif /* YUNO_COMMANDS_MODERATION_H */
