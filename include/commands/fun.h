/*
 * Yuno Gasai 2 (C Edition) - Fun Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_COMMANDS_FUN_H
#define YUNO_COMMANDS_FUN_H

#include <concord/discord.h>

/* Slash command handlers */
void cmd_8ball(struct discord *client, const struct discord_interaction *interaction);

/* Prefix command handlers */
void cmd_8ball_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Get a random 8ball response */
const char *get_8ball_response(void);

#endif /* YUNO_COMMANDS_FUN_H */
