/*
 * Yuno Gasai 2 (C Edition) - Terminal Interface
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

static yuno_bot_t *g_terminal_bot = NULL;
static pthread_t terminal_thread;
static volatile int terminal_running = 0;

void terminal_init(yuno_bot_t *bot) {
    g_terminal_bot = bot;
}

void terminal_cleanup(void) {
    g_terminal_bot = NULL;
}

static void print_prompt(void) {
    int unread = db_get_unread_dm_count(&g_terminal_bot->database);
    if (unread > 0) {
        printf("\n💕 Yuno [%d unread DMs] > ", unread);
    } else {
        printf("\n💕 Yuno > ");
    }
    fflush(stdout);
}

void terminal_cmd_help(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║             💕 Yuno Terminal Commands 💕                  ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║  help          - Show this help message                   ║\n");
    printf("║  servers       - List all connected servers               ║\n");
    printf("║  inbox         - View DM inbox                            ║\n");
    printf("║  botban <id>   - Ban a user from using the bot            ║\n");
    printf("║  botunban <id> - Unban a user from the bot                ║\n");
    printf("║  botbanlist    - List all bot-banned users                ║\n");
    printf("║  status <msg>  - Set bot status message                   ║\n");
    printf("║  quit/exit     - Shutdown the bot                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
}

void terminal_cmd_servers(void) {
    if (!g_terminal_bot || !g_terminal_bot->client) {
        printf("❌ Bot not connected\n");
        return;
    }

    /* Note: In Concord, getting guild list requires caching or API call */
    /* This is a simplified placeholder - actual implementation depends on Concord's API */
    printf("\n📊 Connected Servers:\n");
    printf("─────────────────────────────────────────\n");
    printf("(Server listing requires Concord cache implementation)\n");
    printf("─────────────────────────────────────────\n");
}

void terminal_cmd_inbox(void) {
    dm_inbox_t dms[20];
    int count = 0;

    if (db_get_dms(&g_terminal_bot->database, dms, 20, &count) != 0) {
        printf("❌ Failed to retrieve DM inbox\n");
        return;
    }

    if (count == 0) {
        printf("\n📭 No DMs in inbox\n");
        return;
    }

    printf("\n📬 DM Inbox (%d messages):\n", count);
    printf("─────────────────────────────────────────\n");

    for (int i = 0; i < count; i++) {
        char time_buf[32];
        time_t ts = (time_t)dms[i].timestamp;
        struct tm *tm = localtime(&ts);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm);

        char status = dms[i].read_status ? ' ' : '*';
        printf("%c [%s] %s (%lu):\n", status, time_buf, dms[i].username, (unsigned long)dms[i].user_id);
        printf("  %.100s%s\n", dms[i].content, strlen(dms[i].content) > 100 ? "..." : "");
        printf("─────────────────────────────────────────\n");

        /* Mark as read */
        db_mark_dm_read(&g_terminal_bot->database, dms[i].id);
    }
}

void terminal_cmd_botban(const char *args) {
    if (!args || strlen(args) == 0) {
        printf("❌ Usage: botban <user_id> [reason]\n");
        return;
    }

    char *args_copy = strdup(args);
    char *user_str = strtok(args_copy, " ");
    char *reason = strtok(NULL, "");

    uint64_t user_id = strtoull(user_str, NULL, 10);
    if (user_id == 0) {
        printf("❌ Invalid user ID\n");
        free(args_copy);
        return;
    }

    bot_ban_t ban = {
        .user_id = user_id,
        .banned_by = 0, /* Console ban */
        .timestamp = time(NULL)
    };
    strncpy(ban.reason, reason ? reason : "Banned via console", MAX_REASON_LEN - 1);

    if (db_add_bot_ban(&g_terminal_bot->database, &ban) == 0) {
        printf("✅ User %lu has been banned from using the bot\n", (unsigned long)user_id);
    } else {
        printf("❌ Failed to ban user\n");
    }

    free(args_copy);
}

void terminal_cmd_botunban(const char *args) {
    if (!args || strlen(args) == 0) {
        printf("❌ Usage: botunban <user_id>\n");
        return;
    }

    uint64_t user_id = strtoull(args, NULL, 10);
    if (user_id == 0) {
        printf("❌ Invalid user ID\n");
        return;
    }

    if (db_remove_bot_ban(&g_terminal_bot->database, user_id) == 0) {
        printf("✅ User %lu has been unbanned from the bot\n", (unsigned long)user_id);
    } else {
        printf("❌ Failed to unban user\n");
    }
}

void terminal_cmd_botbanlist(void) {
    bot_ban_t bans[50];
    int count = 0;

    if (db_get_bot_bans(&g_terminal_bot->database, bans, 50, &count) != 0) {
        printf("❌ Failed to retrieve bot bans\n");
        return;
    }

    if (count == 0) {
        printf("\n📋 No bot-level bans\n");
        return;
    }

    printf("\n🚫 Bot-Level Bans (%d total):\n", count);
    printf("─────────────────────────────────────────\n");

    for (int i = 0; i < count; i++) {
        char time_buf[32];
        time_t ts = (time_t)bans[i].timestamp;
        struct tm *tm = localtime(&ts);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", tm);

        printf("• User: %lu\n", (unsigned long)bans[i].user_id);
        printf("  Reason: %s\n", bans[i].reason);
        printf("  Banned: %s\n", time_buf);
        printf("─────────────────────────────────────────\n");
    }
}

void terminal_cmd_status(const char *args) {
    if (!args || strlen(args) == 0) {
        printf("❌ Usage: status <message>\n");
        return;
    }

    /* Note: Setting status in Concord requires specific API calls */
    printf("✅ Status update requested: %s\n", args);
    printf("(Actual status update depends on Concord API implementation)\n");
}

static void *terminal_loop(void *arg) {
    (void)arg;
    char line[1024];

    printf("\n💕 Terminal interface ready! Type 'help' for commands.\n");

    while (terminal_running && g_terminal_bot) {
        print_prompt();

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strlen(line) == 0) continue;

        /* Parse command */
        char *cmd = strtok(line, " ");
        char *args = strtok(NULL, "");

        if (strcmp(cmd, "help") == 0) {
            terminal_cmd_help();
        } else if (strcmp(cmd, "servers") == 0) {
            terminal_cmd_servers();
        } else if (strcmp(cmd, "inbox") == 0) {
            terminal_cmd_inbox();
        } else if (strcmp(cmd, "botban") == 0) {
            terminal_cmd_botban(args);
        } else if (strcmp(cmd, "botunban") == 0) {
            terminal_cmd_botunban(args);
        } else if (strcmp(cmd, "botbanlist") == 0) {
            terminal_cmd_botbanlist();
        } else if (strcmp(cmd, "status") == 0) {
            terminal_cmd_status(args);
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            printf("💔 Shutting down...\n");
            bot_stop(g_terminal_bot);
            break;
        } else {
            printf("❌ Unknown command: %s (type 'help' for commands)\n", cmd);
        }
    }

    return NULL;
}

void terminal_start(void) {
    terminal_running = 1;
    pthread_create(&terminal_thread, NULL, terminal_loop, NULL);
}

void terminal_stop(void) {
    terminal_running = 0;
    /* Note: Properly stopping the terminal thread requires signaling stdin */
}
