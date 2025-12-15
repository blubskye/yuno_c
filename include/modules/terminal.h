/*
 * Yuno Gasai 2 (C Edition) - Terminal Interface
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_TERMINAL_H
#define YUNO_TERMINAL_H

#include "../bot.h"

/* Terminal interface */
void terminal_init(yuno_bot_t *bot);
void terminal_cleanup(void);
void terminal_start(void);
void terminal_stop(void);

/* Terminal command handlers */
void terminal_cmd_help(void);
void terminal_cmd_servers(void);
void terminal_cmd_inbox(void);
void terminal_cmd_botban(const char *args);
void terminal_cmd_botunban(const char *args);
void terminal_cmd_botbanlist(void);
void terminal_cmd_status(const char *args);

#endif /* YUNO_TERMINAL_H */
