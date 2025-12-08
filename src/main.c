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
#include "bot.h"
#include "config.h"

static yuno_bot_t bot;
static volatile int running = 1;

static void signal_handler(int signum) {
    (void)signum;
    printf("\n💔 Yuno is shutting down... goodbye, my love~ 💔\n");
    running = 0;
    bot_stop(&bot);
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
