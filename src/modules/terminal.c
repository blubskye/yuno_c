/*
 * Yuno Gasai 2 (C Edition) - Terminal Interface
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/terminal.h"
#include "modules/activity_logger.h"
#include "modules/lru_cache.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <errno.h>

static yuno_bot_t *g_terminal_bot = NULL;
static pthread_t terminal_thread;
static volatile int terminal_running = 0;

/* Watch system: track active channel watches */
#define MAX_WATCHES 16

typedef struct {
    uint64_t channel_id;
    int active;
} watch_entry_t;

static watch_entry_t g_watches[MAX_WATCHES];
static int g_watch_count = 0;
static pthread_mutex_t g_watch_mutex = PTHREAD_MUTEX_INITIALIZER;

void terminal_init(yuno_bot_t *bot) {
    g_terminal_bot = bot;
    memset(g_watches, 0, sizeof(g_watches));
}

void terminal_cleanup(void) {
    terminal_watch_cleanup();
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
    printf("║  help            - Show this help message                 ║\n");
    printf("║  servers         - List all connected servers             ║\n");
    printf("║  inbox           - View DM inbox                          ║\n");
    printf("║  botban <id>     - Ban a user from using the bot          ║\n");
    printf("║  botunban <id>   - Unban a user from the bot              ║\n");
    printf("║  botbanlist      - List all bot-banned users              ║\n");
    printf("║  status <msg>    - Set bot status message                 ║\n");
    printf("║  commands        - List all available commands             ║\n");
    printf("║  watch <channel> - Watch a channel in real-time           ║\n");
    printf("║  texportbans <id>- Export guild bans to file              ║\n");
    printf("║  timportbans     - Import bans from file                  ║\n");
    printf("║  quit/exit       - Shutdown the bot                       ║\n");
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
    char *saveptr;
    char *user_str = strtok_r(args_copy, " ", &saveptr);
    char *reason = strtok_r(NULL, "", &saveptr);

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
        /* Show current presence */
        bot_presence_t presence;
        if (db_get_bot_presence(&g_terminal_bot->database, &presence) == 0 && presence.text[0]) {
            const char *type_names[] = { "Playing", "Streaming", "Listening to", "Watching", "Custom", "Competing in" };
            int type = presence.type < 6 ? presence.type : 0;
            printf("Current status: %s %s [%s]\n", type_names[type], presence.text, presence.status);
        } else {
            printf("No status set. Usage: status [playing|streaming|listening|watching|competing] <message>\n");
        }
        return;
    }

    /* Parse: [type] <message> */
    bot_presence_t presence = { .type = 0 };
    strncpy(presence.status, "online", sizeof(presence.status) - 1);

    char first_word[32] = "";
    const char *message = args;

    sscanf(args, "%31s", first_word);
    if (strcmp(first_word, "playing") == 0) {
        presence.type = 0; message = args + 8;
    } else if (strcmp(first_word, "streaming") == 0) {
        presence.type = 1; message = args + 10;
    } else if (strcmp(first_word, "listening") == 0) {
        presence.type = 2; message = args + 10;
    } else if (strcmp(first_word, "watching") == 0) {
        presence.type = 3; message = args + 9;
    } else if (strcmp(first_word, "competing") == 0) {
        presence.type = 5; message = args + 10;
    }

    while (*message == ' ') message++;
    strncpy(presence.text, message, sizeof(presence.text) - 1);

    bot_update_presence(g_terminal_bot, &presence);
    const char *type_names[] = { "Playing", "Streaming", "Listening to", "Watching", "Custom", "Competing in" };
    int type = presence.type < 6 ? presence.type : 0;
    printf("✅ Status set: %s %s\n", type_names[type], presence.text);
}

/* ===== list-commands: show all available commands ===== */

/* External reference to command table from bot.c */
typedef void (*prefix_cmd_handler_t)(struct discord *, const struct discord_message *, const char *);
typedef void (*slash_cmd_handler_t)(struct discord *, const struct discord_interaction *);

typedef struct {
    const char *name;
    const char *alias;
    prefix_cmd_handler_t prefix_handler;
    slash_cmd_handler_t slash_handler;
} command_entry_t;

extern const command_entry_t g_commands[];
extern const int g_num_commands;

void terminal_cmd_list_commands(void) {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║            💕 All Available Commands 💕                   ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");

    printf("\n=== DISCORD COMMANDS (Slash + Prefix) ===\n");
    int discord_count = 0;
    for (int i = 0; i < g_num_commands; i++) {
        if (g_commands[i].slash_handler || g_commands[i].prefix_handler) {
            printf("  %-24s", g_commands[i].name);
            if (g_commands[i].alias) {
                printf(" (alias: %s)", g_commands[i].alias);
            }
            if (g_commands[i].slash_handler) {
                printf(" [slash]");
            }
            if (g_commands[i].prefix_handler) {
                printf(" [prefix]");
            }
            printf("\n");
            discord_count++;
        }
    }

    printf("\n=== TERMINAL ONLY COMMANDS ===\n");
    printf("  %-24s  Show this help message\n", "help");
    printf("  %-24s  List connected servers\n", "servers");
    printf("  %-24s  View DM inbox\n", "inbox");
    printf("  %-24s  Ban user from bot\n", "botban <id>");
    printf("  %-24s  Unban user from bot\n", "botunban <id>");
    printf("  %-24s  List bot-banned users\n", "botbanlist");
    printf("  %-24s  Set bot status\n", "status <msg>");
    printf("  %-24s  List all commands\n", "commands");
    printf("  %-24s  Watch channel messages\n", "watch <channel>");
    printf("  %-24s  Export guild bans\n", "texportbans <guild>");
    printf("  %-24s  Import guild bans\n", "timportbans <guild> <file>");
    printf("  %-24s  Shutdown the bot\n", "quit/exit");

    printf("\n── Summary: %d Discord commands, 12 terminal commands ──\n", discord_count);
}

/* ===== watch: real-time channel message monitoring ===== */

/* Check if a channel is being watched (caller must hold g_watch_mutex or use terminal_notify_watch) */
static int is_watching_unlocked(uint64_t channel_id) {
    for (int i = 0; i < g_watch_count; i++) {
        if (g_watches[i].channel_id == channel_id && g_watches[i].active) {
            return 1;
        }
    }
    return 0;
}

int terminal_is_watching(uint64_t channel_id) {
    pthread_mutex_lock(&g_watch_mutex);
    int result = is_watching_unlocked(channel_id);
    pthread_mutex_unlock(&g_watch_mutex);
    return result;
}

/* Called from on_message_create to display watched messages */
void terminal_notify_watch(uint64_t channel_id, const char *author, const char *content,
                            int attachment_count, int has_embed) {
    pthread_mutex_lock(&g_watch_mutex);
    int watching = is_watching_unlocked(channel_id);
    pthread_mutex_unlock(&g_watch_mutex);

    if (!watching) return;

    char time_buf[16];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm);

    /* Truncate to 150 chars */
    printf("[%s] %s: %.150s%s",
           time_buf, author,
           content ? content : "",
           (content && strlen(content) > 150) ? "..." : "");
    if (attachment_count > 0) {
        printf(" [%d file(s)]", attachment_count);
    }
    if (has_embed) {
        printf(" [embed]");
    }
    printf("\n");
    fflush(stdout);
}

void terminal_cmd_watch(const char *args) {
    if (!g_terminal_bot || !g_terminal_bot->client) {
        printf("❌ Bot not connected\n");
        return;
    }

    /* No args: show status */
    if (!args || strlen(args) == 0) {
        pthread_mutex_lock(&g_watch_mutex);
        if (g_watch_count == 0) {
            pthread_mutex_unlock(&g_watch_mutex);
            printf("📺 No active watches\n");
            printf("Usage: watch <channel-id> | watch stop <channel-id|all>\n");
            return;
        }
        printf("\n=== Active Watches ===\n");
        for (int i = 0; i < g_watch_count; i++) {
            if (g_watches[i].active) {
                printf("  #%lu\n", (unsigned long)g_watches[i].channel_id);
            }
        }
        pthread_mutex_unlock(&g_watch_mutex);
        return;
    }

    /* Parse args */
    char args_buf[256];
    strncpy(args_buf, args, sizeof(args_buf) - 1);
    args_buf[sizeof(args_buf) - 1] = '\0';

    char *saveptr;
    char *first = strtok_r(args_buf, " ", &saveptr);
    char *second = strtok_r(NULL, " ", &saveptr);

    /* Handle "stop" subcommand */
    if (strcmp(first, "stop") == 0) {
        if (!second) {
            printf("❌ Usage: watch stop <channel-id|all>\n");
            return;
        }
        pthread_mutex_lock(&g_watch_mutex);
        if (strcmp(second, "all") == 0) {
            for (int i = 0; i < g_watch_count; i++) {
                g_watches[i].active = 0;
            }
            g_watch_count = 0;
            pthread_mutex_unlock(&g_watch_mutex);
            printf("✅ Stopped all watches\n");
            return;
        }
        uint64_t ch_id = strtoull(second, NULL, 10);
        if (ch_id == 0) {
            pthread_mutex_unlock(&g_watch_mutex);
            printf("❌ Invalid channel ID\n");
            return;
        }
        for (int i = 0; i < g_watch_count; i++) {
            if (g_watches[i].channel_id == ch_id) {
                g_watches[i].active = 0;
                /* Compact */
                g_watch_count--;
                if (i < g_watch_count) {
                    g_watches[i] = g_watches[g_watch_count];
                }
                pthread_mutex_unlock(&g_watch_mutex);
                printf("✅ Stopped watching channel %lu\n", (unsigned long)ch_id);
                return;
            }
        }
        pthread_mutex_unlock(&g_watch_mutex);
        printf("❌ Channel %lu is not being watched\n", (unsigned long)ch_id);
        return;
    }

    /* Start watching a channel */
    uint64_t channel_id = strtoull(first, NULL, 10);
    if (channel_id == 0) {
        printf("❌ Invalid channel ID. Usage: watch <channel-id>\n");
        return;
    }

    pthread_mutex_lock(&g_watch_mutex);

    /* Check if already watching */
    if (is_watching_unlocked(channel_id)) {
        pthread_mutex_unlock(&g_watch_mutex);
        printf("📺 Already watching channel %lu\n", (unsigned long)channel_id);
        return;
    }

    if (g_watch_count >= MAX_WATCHES) {
        pthread_mutex_unlock(&g_watch_mutex);
        printf("❌ Maximum %d watches reached. Stop some first.\n", MAX_WATCHES);
        return;
    }

    g_watches[g_watch_count].channel_id = channel_id;
    g_watches[g_watch_count].active = 1;
    g_watch_count++;
    pthread_mutex_unlock(&g_watch_mutex);

    printf("=== Now watching channel %lu ===\n", (unsigned long)channel_id);
    printf("Messages will appear in real-time.\n");
    printf("Use 'watch stop %lu' to stop watching.\n", (unsigned long)channel_id);
}

void terminal_watch_cleanup(void) {
    pthread_mutex_lock(&g_watch_mutex);
    memset(g_watches, 0, sizeof(g_watches));
    g_watch_count = 0;
    pthread_mutex_unlock(&g_watch_mutex);
}

/* ===== texportbans: export guild bans to JSON file ===== */

void terminal_cmd_texportbans(const char *args) {
    if (!g_terminal_bot || !g_terminal_bot->client) {
        printf("❌ Bot not connected\n");
        return;
    }

    if (!args || strlen(args) == 0) {
        printf("❌ Usage: texportbans <guild-id> [output-file]\n");
        return;
    }

    char args_buf[512];
    strncpy(args_buf, args, sizeof(args_buf) - 1);
    args_buf[sizeof(args_buf) - 1] = '\0';

    char *saveptr;
    char *guild_str = strtok_r(args_buf, " ", &saveptr);
    char *output_file = strtok_r(NULL, " ", &saveptr);

    uint64_t guild_id = strtoull(guild_str, NULL, 10);
    if (guild_id == 0) {
        printf("❌ Invalid guild ID\n");
        return;
    }

    /* Default output file: BANS-<guild_id>.txt */
    char default_path[128];
    if (!output_file) {
        snprintf(default_path, sizeof(default_path), "./data/BANS-%lu.txt",
                 (unsigned long)guild_id);
        output_file = default_path;
    }

    /* Security: prevent path traversal with realpath validation */
    char resolved_path[PATH_MAX];
    char allowed_dir[PATH_MAX];

    /* Get the absolute path of the allowed data directory */
    if (realpath("./data", allowed_dir) == NULL) {
        /* If ./data doesn't exist, try to create it first */
        if (mkdir("./data", 0755) != 0 && errno != EEXIST) {
            printf("❌ Failed to access data directory: %s\n", strerror(errno));
            return;
        }
        /* Try realpath again after creation */
        if (realpath("./data", allowed_dir) == NULL) {
            printf("❌ Failed to resolve data directory path: %s\n", strerror(errno));
            return;
        }
    }

    /* Resolve the output file path (may not exist yet, so check parent dir) */
    char output_dir[PATH_MAX];
    strncpy(output_dir, output_file, sizeof(output_dir) - 1);
    output_dir[sizeof(output_dir) - 1] = '\0';

    /* Get directory part of the path */
    char *last_slash = strrchr(output_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        /* Resolve parent directory */
        if (realpath(output_dir, resolved_path) == NULL) {
            printf("❌ Invalid file path (directory doesn't exist or not accessible)\n");
            return;
        }
    } else {
        /* No directory specified, use current directory */
        if (realpath(".", resolved_path) == NULL) {
            printf("❌ Failed to resolve current directory\n");
            return;
        }
    }

    /* Verify resolved path is within allowed directory */
    size_t allowed_len = strlen(allowed_dir);
    if (strncmp(resolved_path, allowed_dir, allowed_len) != 0) {
        printf("❌ Invalid file path (must be within ./data directory)\n");
        return;
    }

    printf("Exporting bans from guild %lu...\n", (unsigned long)guild_id);

    /* Fetch bans via Discord API using Concord */
    struct discord_bans ban_result = { 0 };
    struct discord_ret_bans ret = { .sync = &ban_result };

    CCORDcode code = discord_get_guild_bans(g_terminal_bot->client, guild_id, &ret);
    if (code != CCORD_OK) {
        printf("❌ Failed to fetch bans (error: %d). Check bot permissions.\n", code);
        return;
    }

    /* Build JSON array of user IDs */
    struct json_object *arr = json_object_new_array();
    for (int i = 0; i < ban_result.size; i++) {
        char id_str[32];
        snprintf(id_str, sizeof(id_str), "%lu", (unsigned long)ban_result.array[i].user->id);
        json_object_array_add(arr, json_object_new_string(id_str));
    }

    int total = ban_result.size;

    /* Write to file */
    FILE *f = fopen(output_file, "w");
    if (!f) {
        printf("❌ Failed to open %s: %s\n", output_file, strerror(errno));
        json_object_put(arr);
        discord_bans_cleanup(&ban_result);
        return;
    }

    const char *json_str = json_object_to_json_string_ext(arr, JSON_C_TO_STRING_PRETTY);
    fputs(json_str, f);
    fclose(f);

    json_object_put(arr);
    discord_bans_cleanup(&ban_result);

    printf("✅ Successfully exported %d bans to %s\n", total, output_file);
}

/* ===== timportbans: import bans from JSON file ===== */

void terminal_cmd_timportbans(const char *args) {
    if (!g_terminal_bot || !g_terminal_bot->client) {
        printf("❌ Bot not connected\n");
        return;
    }

    if (!args || strlen(args) == 0) {
        printf("❌ Usage: timportbans <guild-id> <file-path>\n");
        return;
    }

    char args_buf[512];
    strncpy(args_buf, args, sizeof(args_buf) - 1);
    args_buf[sizeof(args_buf) - 1] = '\0';

    char *saveptr;
    char *guild_str = strtok_r(args_buf, " ", &saveptr);
    char *file_path = strtok_r(NULL, " ", &saveptr);

    if (!guild_str || !file_path) {
        printf("❌ Usage: timportbans <guild-id> <file-path>\n");
        return;
    }

    uint64_t guild_id = strtoull(guild_str, NULL, 10);
    if (guild_id == 0) {
        printf("❌ Invalid guild ID\n");
        return;
    }

    /* Security: prevent path traversal */
    if (strstr(file_path, "..")) {
        printf("❌ Invalid file path (path traversal not allowed)\n");
        return;
    }

    /* Read file */
    FILE *f = fopen(file_path, "r");
    if (!f) {
        printf("❌ Failed to open %s: %s\n", file_path, strerror(errno));
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *file_data = malloc(file_len + 1);
    if (!file_data) {
        printf("❌ Out of memory\n");
        fclose(f);
        return;
    }

    fread(file_data, 1, file_len, f);
    file_data[file_len] = '\0';
    fclose(f);

    /* Parse JSON array */
    struct json_object *arr = json_tokener_parse(file_data);
    free(file_data);

    if (!arr || !json_object_is_type(arr, json_type_array)) {
        printf("❌ Invalid JSON format. Expected array of user IDs.\n");
        if (arr) json_object_put(arr);
        return;
    }

    int total = json_object_array_length(arr);
    printf("Importing %d bans to guild %lu...\n", total, (unsigned long)guild_id);
    printf("This may take a while due to rate limits.\n\n");

    int banned = 0;
    int already_banned = 0;
    int failed = 0;

    for (int i = 0; i < total; i++) {
        struct json_object *item = json_object_array_get_idx(arr, i);
        const char *id_str = json_object_get_string(item);
        uint64_t user_id = strtoull(id_str, NULL, 10);
        if (user_id == 0) {
            failed++;
            continue;
        }

        /* Progress update every 50 bans */
        if (i > 0 && i % 50 == 0) {
            printf("\rProcessing %d/%d (%d%%)...", i, total, (i * 100) / total);
            fflush(stdout);
        }

        struct discord_create_guild_ban params = {
            .delete_message_days = 0
        };

        CCORDcode code = discord_create_guild_ban(g_terminal_bot->client, guild_id,
                                                    user_id, &params, NULL);
        if (code == CCORD_OK) {
            banned++;
        } else {
            /* Could be already banned or other error */
            already_banned++;
        }
    }

    json_object_put(arr);

    printf("\r\n=== Import Complete ===\n");
    printf("Successfully banned: %d\n", banned);
    printf("Already banned/failed: %d\n", already_banned);
    printf("Invalid IDs: %d\n", failed);
    printf("Total processed: %d\n", total);
}

/* ===== reload: hot-reload configuration ===== */

void terminal_cmd_reload(void) {
    if (!g_terminal_bot) {
        printf("❌ Bot not initialized\n");
        return;
    }

    printf("🔄 Reloading configuration...\n");

    /* Save old config for comparison */
    yuno_config_t old_config;
    memcpy(&old_config, &g_terminal_bot->config, sizeof(yuno_config_t));

    /* Reload config from file */
    yuno_config_t new_config;
    config_init_defaults(&new_config);

    if (config_load(&new_config, "config.json") != 0) {
        printf("❌ Failed to load config.json\n");
        return;
    }

    /* Apply env overrides */
    config_load_from_env(&new_config);

    /* Preserve token (can't change at runtime) */
    strncpy(new_config.discord_token, old_config.discord_token, MAX_TOKEN_LEN - 1);

    /* Apply new config */
    memcpy(&g_terminal_bot->config, &new_config, sizeof(yuno_config_t));

    /* Update low memory mode if changed */
    if (new_config.low_memory_mode != old_config.low_memory_mode) {
        activity_logger_set_low_memory(new_config.low_memory_mode);
    }

    /* Clear cache on config reload */
    lru_cache_clear(&g_terminal_bot->cache);

    printf("✅ Configuration reloaded!\n");

    /* Show what changed */
    if (strcmp(old_config.default_prefix, new_config.default_prefix) != 0) {
        printf("  prefix: %s → %s\n", old_config.default_prefix, new_config.default_prefix);
    }
    if (old_config.xp_per_msg != new_config.xp_per_msg) {
        printf("  xp_per_msg: %d → %d\n", old_config.xp_per_msg, new_config.xp_per_msg);
    }
    if (old_config.spam_max_warnings != new_config.spam_max_warnings) {
        printf("  spam_max_warnings: %d → %d\n", old_config.spam_max_warnings, new_config.spam_max_warnings);
    }
    if (old_config.master_user_count != new_config.master_user_count) {
        printf("  master_users: %d → %d\n", old_config.master_user_count, new_config.master_user_count);
    }
    if (old_config.low_memory_mode != new_config.low_memory_mode) {
        printf("  low_memory_mode: %d → %d\n", old_config.low_memory_mode, new_config.low_memory_mode);
    }
}

/* ===== auto-update: check for git updates ===== */

void terminal_cmd_autoupdate(void) {
    printf("🔄 Checking for updates...\n");

    /* Run git fetch to check remote */
    FILE *fp = popen("git fetch --dry-run 2>&1", "r");
    if (!fp) {
        printf("❌ Failed to run git. Is this a git repository?\n");
        return;
    }

    char output[1024] = "";
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        size_t current_len = strlen(output);
        size_t remaining = sizeof(output) - current_len - 1;
        if (remaining > 0) {
            strncat(output, line, remaining);
        }
    }
    int status = pclose(fp);

    if (status != 0) {
        printf("❌ git fetch failed: %s\n", output);
        return;
    }

    /* Check if there are differences between local and remote */
    fp = popen("git log HEAD..@{u} --oneline 2>/dev/null", "r");
    if (!fp) {
        printf("❌ Failed to check for updates\n");
        return;
    }

    int has_updates = 0;
    char updates[2048] = "";
    while (fgets(line, sizeof(line), fp)) {
        has_updates++;
        size_t current_len = strlen(updates);
        size_t remaining = sizeof(updates) - current_len - 1;
        if (remaining > 0) {
            strncat(updates, line, remaining);
        }
    }
    pclose(fp);

    if (has_updates == 0) {
        printf("✅ Already up to date!\n");
    } else {
        printf("📦 %d update(s) available:\n%s\n", has_updates, updates);
        printf("Run 'git pull && cmake --build build' to update.\n");
    }
}

/* ===== Terminal main loop ===== */

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
        char *saveptr;
        char *cmd = strtok_r(line, " ", &saveptr);
        char *args = strtok_r(NULL, "", &saveptr);

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
        } else if (strcmp(cmd, "commands") == 0 || strcmp(cmd, "cmds") == 0 ||
                   strcmp(cmd, "list-commands") == 0 || strcmp(cmd, "help-all") == 0) {
            terminal_cmd_list_commands();
        } else if (strcmp(cmd, "watch") == 0 || strcmp(cmd, "stream") == 0 ||
                   strcmp(cmd, "listen") == 0) {
            terminal_cmd_watch(args);
        } else if (strcmp(cmd, "texportbans") == 0 || strcmp(cmd, "tebans") == 0) {
            terminal_cmd_texportbans(args);
        } else if (strcmp(cmd, "timportbans") == 0 || strcmp(cmd, "tibans") == 0) {
            terminal_cmd_timportbans(args);
        } else if (strcmp(cmd, "auto-update") == 0 || strcmp(cmd, "update") == 0) {
            terminal_cmd_autoupdate();
        } else if (strcmp(cmd, "reload") == 0) {
            terminal_cmd_reload();
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
    /* Cancel the thread since it's likely blocked on fgets(stdin) */
    pthread_cancel(terminal_thread);
    pthread_join(terminal_thread, NULL);
}
