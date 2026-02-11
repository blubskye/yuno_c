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
void cmd_stats(struct discord *client, const struct discord_interaction *interaction);
void cmd_bot_ban(struct discord *client, const struct discord_interaction *interaction);
void cmd_bot_unban(struct discord *client, const struct discord_interaction *interaction);
void cmd_bot_banlist(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_spamfilter(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_xp(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_level(struct discord *client, const struct discord_interaction *interaction);
void cmd_fix_xp(struct discord *client, const struct discord_interaction *interaction);
void cmd_mass_addxp(struct discord *client, const struct discord_interaction *interaction);
void cmd_mass_setxp(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_levelrolemap(struct discord *client, const struct discord_interaction *interaction);
void cmd_sync_levelroles(struct discord *client, const struct discord_interaction *interaction);
void cmd_sync_xp_from_roles(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_vcxp(struct discord *client, const struct discord_interaction *interaction);
void cmd_vcxp_status(struct discord *client, const struct discord_interaction *interaction);

/* Phase 4: Configuration commands */
void cmd_set_dm_channel(struct discord *client, const struct discord_interaction *interaction);
void cmd_dm_status(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_joinmessage(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_logchannel(struct discord *client, const struct discord_interaction *interaction);
void cmd_log_status(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_logsettings(struct discord *client, const struct discord_interaction *interaction);
void cmd_set_invitefilter(struct discord *client, const struct discord_interaction *interaction);

/* Phase 7: Admin/Master commands */
void cmd_send(struct discord *client, const struct discord_interaction *interaction);
void cmd_reply_dm(struct discord *client, const struct discord_interaction *interaction);
void cmd_inbox(struct discord *client, const struct discord_interaction *interaction);
void cmd_add_masteruser(struct discord *client, const struct discord_interaction *interaction);
void cmd_debug_error(struct discord *client, const struct discord_interaction *interaction);
void cmd_init_guild(struct discord *client, const struct discord_interaction *interaction);

/* Phase 5.7: Error Channel Logging */
void cmd_drop_errors_on(struct discord *client, const struct discord_interaction *interaction);

/* Phase 5.2: Mention Response commands */
void cmd_add_mentionresponse(struct discord *client, const struct discord_interaction *interaction);
void cmd_del_mentionresponse(struct discord *client, const struct discord_interaction *interaction);
void cmd_mentionresponses(struct discord *client, const struct discord_interaction *interaction);

/* Phase 4.1: Config command */
void cmd_config(struct discord *client, const struct discord_interaction *interaction);

/* Phase 7: List command */
void cmd_list_command(struct discord *client, const struct discord_interaction *interaction);

/* Prefix command handlers */
void cmd_ping_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_help_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_source_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_prefix_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_auto_clean_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_delay_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_leaderboard_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_stats_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_bot_ban_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_bot_unban_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_bot_banlist_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_spamfilter_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_level_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_fix_xp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_mass_addxp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_mass_setxp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_levelrolemap_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_sync_levelroles_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_sync_xp_from_roles_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_vcxp_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_vcxp_status_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 4.1: Config prefix command */
void cmd_config_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 7: List command prefix */
void cmd_list_command_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 4: Configuration prefix commands */
void cmd_set_dm_channel_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_dm_status_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_joinmessage_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_logchannel_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_log_status_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_logsettings_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_set_invitefilter_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 7: Admin/Master prefix commands */
void cmd_send_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_reply_dm_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_inbox_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_add_masteruser_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_debug_error_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_init_guild_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 5.7: Error Channel Logging prefix */
void cmd_drop_errors_on_prefix(struct discord *client, const struct discord_message *msg, const char *args);

/* Phase 5.2: Mention Response prefix commands */
void cmd_add_mentionresponse_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_del_mentionresponse_prefix(struct discord *client, const struct discord_message *msg, const char *args);
void cmd_mentionresponses_prefix(struct discord *client, const struct discord_message *msg, const char *args);

#endif /* YUNO_COMMANDS_UTILITY_H */
