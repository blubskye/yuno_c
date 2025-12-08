/*
 * Yuno Gasai 2 (C Edition) - Fun Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "commands/fun.h"
#include "bot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *EIGHTBALL_RESPONSES[] = {
    /* Positive */
    "It is certain~ 💕",
    "It is decidedly so~ 💗",
    "Without a doubt~ 💖",
    "Yes, definitely~ 💕",
    "You may rely on it~ 💗",
    "As I see it, yes~ ✨",
    "Most likely~ 💕",
    "Outlook good~ 💖",
    "Yes~ 💗",
    "Signs point to yes~ ✨",

    /* Neutral */
    "Reply hazy, try again~ 🤔",
    "Ask again later~ 💭",
    "Better not tell you now~ 😏",
    "Cannot predict now~ 🔮",
    "Concentrate and ask again~ 💫",

    /* Negative */
    "Don't count on it~ 💔",
    "My reply is no~ 😤",
    "My sources say no~ 💢",
    "Outlook not so good~ 😞",
    "Very doubtful~ 💔"
};

static const int RESPONSE_COUNT = sizeof(EIGHTBALL_RESPONSES) / sizeof(EIGHTBALL_RESPONSES[0]);

const char *get_8ball_response(void) {
    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }
    return EIGHTBALL_RESPONSES[rand() % RESPONSE_COUNT];
}

void cmd_8ball(struct discord *client, const struct discord_interaction *interaction) {
    struct discord_application_command_interaction_data_option *options = interaction->data->options;
    const char *question = "...";

    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(options[i].name, "question") == 0) {
            question = options[i].value;
        }
    }

    const char *response_text = get_8ball_response();

    char response_msg[1024];
    snprintf(response_msg, sizeof(response_msg),
        "🎱 **Magic 8-Ball**\n\n"
        "**Question:** %s\n\n"
        "**Answer:** %s\n\n"
        "*shakes the 8-ball mysteriously*",
        question, response_text);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_8ball_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "💔 You need to ask a question~ 🎱" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    const char *response_text = get_8ball_response();

    char response_msg[1024];
    snprintf(response_msg, sizeof(response_msg),
        "🎱 **Magic 8-Ball**\n\n"
        "**Question:** %s\n\n"
        "**Answer:** %s\n\n"
        "*shakes the 8-ball mysteriously*",
        args, response_text);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}
