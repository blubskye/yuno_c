/*
 * Yuno Gasai 2 (C Edition)
 * "I'll protect this server forever... just for you~" <3
 *
 * Copyright (C) 2025 blubskye
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <execinfo.h>
#include "bot.h"
#include "config.h"

#define CRASH_BACKTRACE_MAX 64

static yuno_bot_t bot;
static volatile int shutdown_count = 0;

static const char *signal_name(int signum) {
    switch (signum) {
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGABRT: return "SIGABRT (Aborted)";
        case SIGFPE:  return "SIGFPE (Floating point exception)";
        case SIGBUS:  return "SIGBUS (Bus error)";
        case SIGILL:  return "SIGILL (Illegal instruction)";
        default:      return "Unknown signal";
    }
}

static void crash_handler(int signum) {
    /* Prevent recursive crashes */
    signal(signum, SIG_DFL);

    void *frames[CRASH_BACKTRACE_MAX];
    int frame_count = backtrace(frames, CRASH_BACKTRACE_MAX);
    char **symbols = backtrace_symbols(frames, frame_count);

    /* Build timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", tm_info);

    /* Print to stderr */
    fprintf(stderr, "\n💀 FATAL CRASH: %s (signal %d)\n", signal_name(signum), signum);
    fprintf(stderr, "Timestamp: %s\n", timestamp);
    fprintf(stderr, "Stack trace (%d frames):\n", frame_count);
    for (int i = 0; i < frame_count; i++) {
        fprintf(stderr, "  #%d %s\n", i, symbols ? symbols[i] : "???");
    }
    fprintf(stderr, "\n");

    /* Write crash dump to file */
    char crash_path[256];
    snprintf(crash_path, sizeof(crash_path), "crash_%s.log", timestamp);
    FILE *crash_file = fopen(crash_path, "w");
    if (crash_file) {
        fprintf(crash_file, "Yuno Gasai 2 (C Edition) - Crash Report\n");
        fprintf(crash_file, "========================================\n\n");
        fprintf(crash_file, "Signal:    %s (signal %d)\n", signal_name(signum), signum);
        fprintf(crash_file, "Timestamp: %s\n", timestamp);
        fprintf(crash_file, "PID:       %d\n\n", (int)getpid());
        fprintf(crash_file, "Stack trace (%d frames):\n", frame_count);
        for (int i = 0; i < frame_count; i++) {
            fprintf(crash_file, "  #%d %s\n", i, symbols ? symbols[i] : "???");
        }
        fprintf(crash_file, "\nNote: Compile with -g and run through addr2line for source locations.\n");
        fprintf(crash_file, "  addr2line -e yuno_gasai -f -C <address>\n");
        fclose(crash_file);
        fprintf(stderr, "💀 Crash dump written to: %s\n", crash_path);
    }

    free(symbols);

    /* Re-raise to get core dump from default handler */
    raise(signum);
}

static void signal_handler(int signum) {
    (void)signum;
    shutdown_count++;

    if (shutdown_count == 1) {
        printf("\n💔 Yuno is shutting down... goodbye, my love~ 💔\n");
        bot_stop(&bot);
    } else if (shutdown_count == 2) {
        printf("\n💔 Forcing shutdown... Yuno didn't want to leave~ 💔\n");
        _exit(1);
    }

    /* Third Ctrl+C: reset to default handler so kernel kills us */
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
}

static void print_banner(void) {
    printf("\n");
    printf("    💕 ╔═══════════════════════════════════════════╗ 💕\n");
    printf("       ║     Yuno Gasai 2 (C Edition)              ║\n");
    printf("       ║     \"I'll protect you forever~\" 💗        ║\n");
    printf("       ╚═══════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    yuno_config_t config;
    const char *config_path = "config.json";
    int result;

    print_banner();

    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Set up crash handlers for stack tracing */
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGILL, crash_handler);

    /* Initialize config with defaults */
    config_init_defaults(&config);

    /* Determine config path */
    if (argc > 1) {
        config_path = argv[1];
    } else {
        const char *env_path = getenv("CONFIG_PATH");
        if (env_path) {
            config_path = env_path;
        }
    }

    /* Try to load config from file */
    result = config_load(&config, config_path);
    if (result != 0) {
        printf("📝 Config file not found, checking environment...\n");
        result = config_load_from_env(&config);
        if (result != 0) {
            fprintf(stderr, "❌ Failed to load configuration\n");
            return 1;
        }
    } else {
        printf("💖 Loaded config from %s~\n", config_path);
    }

    /* Validate token */
    if (strlen(config.discord_token) == 0 ||
        strcmp(config.discord_token, "YOUR_DISCORD_BOT_TOKEN_HERE") == 0) {
        fprintf(stderr, "❌ Error: No valid Discord token provided!\n");
        fprintf(stderr, "Set DISCORD_TOKEN environment variable or add it to config.json\n");
        return 1;
    }

    /* Initialize bot */
    printf("💕 Yuno is waking up... please wait~\n");
    result = bot_init(&bot, &config);
    if (result != 0) {
        fprintf(stderr, "💔 Failed to initialize bot\n");
        return 1;
    }

    /* Run bot */
    result = bot_run(&bot);

    /* Cleanup */
    bot_cleanup(&bot);

    printf("💔 Yuno has gone to sleep... see you next time~ 💔\n");
    return result;
}
