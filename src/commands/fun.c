/*
 * Yuno Gasai 2 (C Edition) - Fun Commands
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "commands/fun.h"
#include "bot.h"
#include "modules/http_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <concord/discord-worker.h>

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
    const char *question = "...";

    if (interaction->data->options) {
        for (int i = 0; i < interaction->data->options->size; i++) {
            if (strcmp(interaction->data->options->array[i].name, "question") == 0) {
                question = interaction->data->options->array[i].value;
            }
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

/* --- Quote Command --- */

static const char *YUNO_QUOTES[] = {
    "Your future belongs to me",
    "I'm glad Yukkis mother is a good person, I didn't have to use any of the tools I brought",
    "I'm the only friend you need",
    "I was practically dead, but you gave me a future. Yukki is my hope in life, but if it won't come true then I will die for Yukki, and even in death I will chase after Yukki",
    "They are all planning to betray you!!!",
    "What's insane is this world that won't let me and Yukki be together!",
    "A half moon, it has a dark half and a bright half, just like me\xe2\x80\xa6",
    "Everything in this world is just a game and we are merely the pawns.",
    "Breaking curfew is 3 demerits. 3 demerits gets the cage, the cage means no food."
};
static const int QUOTE_COUNT = sizeof(YUNO_QUOTES) / sizeof(YUNO_QUOTES[0]);

void cmd_quote(struct discord *client, const struct discord_interaction *interaction) {
    const char *quote = YUNO_QUOTES[rand() % QUOTE_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "\xe2\x80\x9c*%s*\xe2\x80\x9d\n\n\xe2\x80\x94 Yuno Gasai \xf0\x9f\x92\x95", quote);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_quote_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    (void)args;
    const char *quote = YUNO_QUOTES[rand() % QUOTE_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "\xe2\x80\x9c*%s*\xe2\x80\x9d\n\n\xe2\x80\x94 Yuno Gasai \xf0\x9f\x92\x95", quote);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

/* --- Praise Command --- */

static const char *PRAISE_IMAGES[] = {
    "https://media.giphy.com/media/ny8mlxWio6WBi/giphy.gif"
};
static const int PRAISE_COUNT = sizeof(PRAISE_IMAGES) / sizeof(PRAISE_IMAGES[0]);

void cmd_praise(struct discord *client, const struct discord_interaction *interaction) {
    const char *user_val = NULL;
    if (interaction->data->options) {
        for (int i = 0; i < interaction->data->options->size; i++) {
            if (strcmp(interaction->data->options->array[i].name, "user") == 0) {
                user_val = interaction->data->options->array[i].value;
            }
        }
    }

    if (!user_val) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Who do you want me to praise?" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(user_val);
    const char *image = PRAISE_IMAGES[rand() % PRAISE_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "<@%lu> %s", (unsigned long)user_id, image);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_praise_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "Who do you want me to praise?" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(args);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "Who do you want me to praise?" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    const char *image = PRAISE_IMAGES[rand() % PRAISE_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "<@%lu> %s", (unsigned long)user_id, image);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

/* --- Scold Command --- */

static const char *SCOLD_IMAGES[] = {
    "http://static3.fjcdn.com/thumbnails/comments/2+is+wrong+in+the+food+business+they+_e4a5025baf43b957f18c834d9615f7fe.jpg",
    "https://i.makeagif.com/media/6-29-2015/oQA7fS.gif",
    "https://i.imgur.com/ZLaayKG.gif",
    "http://orig15.deviantart.net/d57e/f/2012/148/1/7/u_mad_bro__by_meme_thickilisious-d51gdaa.png",
    "https://s-media-cache-ak0.pinimg.com/originals/71/42/a6/7142a6d8d7379e89605c853ec46cf80c.gif"
};
static const int SCOLD_COUNT = sizeof(SCOLD_IMAGES) / sizeof(SCOLD_IMAGES[0]);

void cmd_scold(struct discord *client, const struct discord_interaction *interaction) {
    const char *user_val = NULL;
    if (interaction->data->options) {
        for (int i = 0; i < interaction->data->options->size; i++) {
            if (strcmp(interaction->data->options->array[i].name, "user") == 0) {
                user_val = interaction->data->options->array[i].value;
            }
        }
    }

    if (!user_val) {
        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){ .content = "Who do you want me to scold?" }
        };
        discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(user_val);
    const char *image = SCOLD_IMAGES[rand() % SCOLD_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "<@%lu> %s", (unsigned long)user_id, image);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = response_msg }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

void cmd_scold_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || strlen(args) == 0) {
        struct discord_create_message params = { .content = "Who do you want me to scold?" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    uint64_t user_id = parse_user_mention(args);
    if (user_id == 0) {
        struct discord_create_message params = { .content = "Who do you want me to scold?" };
        discord_create_message(client, msg->channel_id, &params, NULL);
        return;
    }

    const char *image = SCOLD_IMAGES[rand() % SCOLD_COUNT];

    char response_msg[512];
    snprintf(response_msg, sizeof(response_msg), "<@%lu> %s", (unsigned long)user_id, image);

    struct discord_create_message params = { .content = response_msg };
    discord_create_message(client, msg->channel_id, &params, NULL);
}

/* --- Helpers for API commands --- */

static void fun_send_reply(struct discord *client, u64snowflake channel_id, const char *content) {
    struct discord_create_message p = { .content = (char *)content };
    discord_create_message(client, channel_id, &p, NULL);
}

static void fun_send_interaction(struct discord *client,
    const struct discord_interaction *interaction, const char *content) {
    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){ .content = (char *)content }
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &response, NULL);
}

static const char *fun_get_option(const struct discord_interaction *interaction, const char *name) {
    if (!interaction->data->options) return NULL;
    for (int i = 0; i < interaction->data->options->size; i++) {
        if (strcmp(interaction->data->options->array[i].name, name) == 0)
            return interaction->data->options->array[i].value;
    }
    return NULL;
}

/* URL-encode a string for query parameters */
static void url_encode(const char *src, char *dst, size_t dst_len) {
    CURL *curl = curl_easy_init();
    if (curl) {
        char *encoded = curl_easy_escape(curl, src, 0);
        if (encoded) {
            strncpy(dst, encoded, dst_len - 1);
            dst[dst_len - 1] = '\0';
            curl_free(encoded);
        }
        curl_easy_cleanup(curl);
    } else {
        strncpy(dst, src, dst_len - 1);
        dst[dst_len - 1] = '\0';
    }
}

/* Truncate text to max length, adding "..." if truncated */
static void truncate_text(const char *src, char *dst, size_t max_len) {
    size_t src_len = strlen(src);
    if (src_len <= max_len) {
        /* Safe copy with bounds check */
        strncpy(dst, src, max_len);
        dst[max_len] = '\0';
    } else {
        /* Truncate with ellipsis */
        if (max_len >= 3) {
            strncpy(dst, src, max_len - 3);
            dst[max_len - 3] = '\0';
            /* Append ellipsis — we know exactly 3 bytes of space remain */
            dst[max_len - 3] = '.';
            dst[max_len - 2] = '.';
            dst[max_len - 1] = '.';
            dst[max_len]     = '\0';
        } else {
            /* Not enough space for ellipsis */
            strncpy(dst, src, max_len);
            dst[max_len] = '\0';
        }
    }
}

/* --- Worker thread context for async HTTP commands --- */

typedef struct {
    struct discord *client;
    u64snowflake channel_id;         /* For prefix commands */
    u64snowflake application_id;     /* For slash commands */
    char interaction_token[256];     /* For slash commands */
    int is_slash;                    /* 1 = edit interaction, 0 = send message */
    char query[512];                 /* Search query / term / tags */
    int count;                       /* For hentai count */
    int lewd;                        /* For neko lewd flag */
} fun_worker_ctx_t;

/* Send result from worker thread — handles both slash and prefix */
static void worker_send(fun_worker_ctx_t *ctx, const char *content) {
    if (ctx->is_slash) {
        struct discord_edit_original_interaction_response edit = {
            .content = (char *)content
        };
        discord_edit_original_interaction_response(ctx->client, ctx->application_id,
            ctx->interaction_token, &edit, NULL);
    } else {
        fun_send_reply(ctx->client, ctx->channel_id, content);
    }
}

/* --- Anime Command (Jikan API v4) --- */

static void anime_worker(void *data) {
    fun_worker_ctx_t *ctx = data;

    char encoded[512];
    url_encode(ctx->query, encoded, sizeof(encoded));

    char url[1024];
    snprintf(url, sizeof(url), "https://api.jikan.moe/v4/anime?q=%s&limit=1", encoded);

    http_response_t resp;
    if (http_get(url, &resp) != 0) {
        worker_send(ctx, "Failed to reach the anime API~ Try again later.");
        free(ctx);
        return;
    }

    struct json_object *root = json_tokener_parse(resp.data);
    http_response_free(&resp);

    if (!root) {
        worker_send(ctx, "Failed to parse anime response~");
        free(ctx);
        return;
    }

    struct json_object *data_arr;
    if (!json_object_object_get_ex(root, "data", &data_arr) ||
        json_object_array_length(data_arr) == 0) {
        json_object_put(root);
        char msg_buf[256];
        snprintf(msg_buf, sizeof(msg_buf), "No anime results found for `%s`~", ctx->query);
        worker_send(ctx, msg_buf);
        free(ctx);
        return;
    }

    struct json_object *item = json_object_array_get_idx(data_arr, 0);
    struct json_object *tmp;

    const char *title = "", *title_en = "", *type = "Unknown", *status = "Unknown";
    const char *synopsis = "", *image_url = "";
    int episodes = 0;
    double score = 0.0;

    if (json_object_object_get_ex(item, "title", &tmp)) title = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "title_english", &tmp) && tmp) title_en = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "type", &tmp) && tmp) type = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "status", &tmp) && tmp) status = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "synopsis", &tmp) && tmp) synopsis = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "episodes", &tmp) && tmp) episodes = json_object_get_int(tmp);
    if (json_object_object_get_ex(item, "score", &tmp) && tmp) score = json_object_get_double(tmp);

    struct json_object *images, *jpg;
    if (json_object_object_get_ex(item, "images", &images) &&
        json_object_object_get_ex(images, "jpg", &jpg) &&
        json_object_object_get_ex(jpg, "image_url", &tmp) && tmp) {
        image_url = json_object_get_string(tmp);
    }

    char synopsis_trunc[400];
    truncate_text(synopsis, synopsis_trunc, 350);
    char ep_str[16];
    if (episodes > 0) snprintf(ep_str, sizeof(ep_str), "%d", episodes);
    else strncpy(ep_str, "TBD", sizeof(ep_str) - 1), ep_str[sizeof(ep_str) - 1] = '\0';

    char result[2048];
    snprintf(result, sizeof(result),
        "**%s**%s%s%s\n"
        "**Type:** %s | **Status:** %s\n"
        "**Episodes:** %s | **Score:** %.1f\n\n"
        "%s\n\n%s",
        title, title_en[0] ? " (" : "", title_en, title_en[0] ? ")" : "",
        type, status, ep_str, score, synopsis_trunc,
        image_url[0] ? image_url : "");

    worker_send(ctx, result);
    json_object_put(root);
    free(ctx);
}

void cmd_anime(struct discord *client, const struct discord_interaction *interaction) {
    const char *query = fun_get_option(interaction, "name");
    if (!query || !*query) {
        fun_send_interaction(client, interaction, "Usage: `/anime <name>`~");
        return;
    }

    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_interaction(client, interaction, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->application_id = interaction->application_id;
    strncpy(ctx->interaction_token, interaction->token, sizeof(ctx->interaction_token) - 1);
    ctx->is_slash = 1;
    strncpy(ctx->query, query, sizeof(ctx->query) - 1);
    discord_worker_add(client, anime_worker, ctx);
}

void cmd_anime_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || !*args) {
        fun_send_reply(client, msg->channel_id, "Usage: `anime <name>`~");
        return;
    }

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_reply(client, msg->channel_id, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->channel_id = msg->channel_id;
    strncpy(ctx->query, args, sizeof(ctx->query) - 1);
    discord_worker_add(client, anime_worker, ctx);
}

/* --- Manga Command (Jikan API v4) --- */

static void manga_worker(void *data) {
    fun_worker_ctx_t *ctx = data;

    char encoded[512];
    url_encode(ctx->query, encoded, sizeof(encoded));

    char url[1024];
    snprintf(url, sizeof(url), "https://api.jikan.moe/v4/manga?q=%s&limit=1", encoded);

    http_response_t resp;
    if (http_get(url, &resp) != 0) {
        worker_send(ctx, "Failed to reach the manga API~ Try again later.");
        free(ctx);
        return;
    }

    struct json_object *root = json_tokener_parse(resp.data);
    http_response_free(&resp);

    if (!root) {
        worker_send(ctx, "Failed to parse manga response~");
        free(ctx);
        return;
    }

    struct json_object *data_arr;
    if (!json_object_object_get_ex(root, "data", &data_arr) ||
        json_object_array_length(data_arr) == 0) {
        json_object_put(root);
        char msg_buf[256];
        snprintf(msg_buf, sizeof(msg_buf), "No manga results found for `%s`~", ctx->query);
        worker_send(ctx, msg_buf);
        free(ctx);
        return;
    }

    struct json_object *item = json_object_array_get_idx(data_arr, 0);
    struct json_object *tmp;

    const char *title = "", *title_en = "", *type = "Unknown", *status = "Unknown";
    const char *synopsis = "", *image_url = "";
    int chapters = 0, volumes = 0;
    double score = 0.0;

    if (json_object_object_get_ex(item, "title", &tmp)) title = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "title_english", &tmp) && tmp) title_en = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "type", &tmp) && tmp) type = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "status", &tmp) && tmp) status = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "synopsis", &tmp) && tmp) synopsis = json_object_get_string(tmp);
    if (json_object_object_get_ex(item, "chapters", &tmp) && tmp) chapters = json_object_get_int(tmp);
    if (json_object_object_get_ex(item, "volumes", &tmp) && tmp) volumes = json_object_get_int(tmp);
    if (json_object_object_get_ex(item, "score", &tmp) && tmp) score = json_object_get_double(tmp);

    struct json_object *images, *jpg;
    if (json_object_object_get_ex(item, "images", &images) &&
        json_object_object_get_ex(images, "jpg", &jpg) &&
        json_object_object_get_ex(jpg, "image_url", &tmp) && tmp) {
        image_url = json_object_get_string(tmp);
    }

    char synopsis_trunc[400];
    truncate_text(synopsis, synopsis_trunc, 350);
    char ch_str[16], vol_str[16];
    if (chapters > 0) snprintf(ch_str, sizeof(ch_str), "%d", chapters);
    else strncpy(ch_str, "TBD", sizeof(ch_str) - 1), ch_str[sizeof(ch_str) - 1] = '\0';
    if (volumes > 0) snprintf(vol_str, sizeof(vol_str), "%d", volumes);
    else strncpy(vol_str, "TBD", sizeof(vol_str) - 1), vol_str[sizeof(vol_str) - 1] = '\0';

    char result[2048];
    snprintf(result, sizeof(result),
        "**%s**%s%s%s\n"
        "**Type:** %s | **Status:** %s\n"
        "**Chapters:** %s | **Volumes:** %s | **Score:** %.1f\n\n"
        "%s\n\n%s",
        title, title_en[0] ? " (" : "", title_en, title_en[0] ? ")" : "",
        type, status, ch_str, vol_str, score, synopsis_trunc,
        image_url[0] ? image_url : "");

    worker_send(ctx, result);
    json_object_put(root);
    free(ctx);
}

void cmd_manga(struct discord *client, const struct discord_interaction *interaction) {
    const char *query = fun_get_option(interaction, "name");
    if (!query || !*query) {
        fun_send_interaction(client, interaction, "Usage: `/manga <name>`~");
        return;
    }

    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_interaction(client, interaction, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->application_id = interaction->application_id;
    strncpy(ctx->interaction_token, interaction->token, sizeof(ctx->interaction_token) - 1);
    ctx->is_slash = 1;
    strncpy(ctx->query, query, sizeof(ctx->query) - 1);
    discord_worker_add(client, manga_worker, ctx);
}

void cmd_manga_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || !*args) {
        fun_send_reply(client, msg->channel_id, "Usage: `manga <name>`~");
        return;
    }

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_reply(client, msg->channel_id, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->channel_id = msg->channel_id;
    strncpy(ctx->query, args, sizeof(ctx->query) - 1);
    discord_worker_add(client, manga_worker, ctx);
}

/* --- Neko Command (nekos.life API) --- */

static void neko_worker(void *data) {
    fun_worker_ctx_t *ctx = data;

    const char *url = ctx->lewd
        ? "https://nekos.life/api/lewd/neko"
        : "https://nekos.life/api/neko";

    http_response_t resp;
    if (http_get(url, &resp) != 0) {
        worker_send(ctx, "Failed to reach nekos.life~ Try again later.");
        free(ctx);
        return;
    }

    struct json_object *root = json_tokener_parse(resp.data);
    http_response_free(&resp);

    if (!root) {
        worker_send(ctx, "Failed to parse neko response~");
        free(ctx);
        return;
    }

    struct json_object *neko_url;
    if (json_object_object_get_ex(root, "neko", &neko_url)) {
        worker_send(ctx, json_object_get_string(neko_url));
    } else {
        worker_send(ctx, "No neko found~");
    }

    json_object_put(root);
    free(ctx);
}

void cmd_neko(struct discord *client, const struct discord_interaction *interaction) {
    const char *type = fun_get_option(interaction, "type");
    int lewd = (type && strcmp(type, "lewd") == 0);

    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_interaction(client, interaction, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->application_id = interaction->application_id;
    strncpy(ctx->interaction_token, interaction->token, sizeof(ctx->interaction_token) - 1);
    ctx->is_slash = 1;
    ctx->lewd = lewd;
    discord_worker_add(client, neko_worker, ctx);
}

void cmd_neko_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    int lewd = (args && (strcmp(args, "lewd") == 0 || strcmp(args, "nsfw") == 0));

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_reply(client, msg->channel_id, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->channel_id = msg->channel_id;
    ctx->lewd = lewd;
    discord_worker_add(client, neko_worker, ctx);
}

/* --- Urban Dictionary Command --- */

static void urban_worker(void *data) {
    fun_worker_ctx_t *ctx = data;

    char encoded[512];
    url_encode(ctx->query, encoded, sizeof(encoded));

    char url[1024];
    snprintf(url, sizeof(url), "https://api.urbandictionary.com/v0/define?term=%s", encoded);

    http_response_t resp;
    if (http_get(url, &resp) != 0) {
        worker_send(ctx, "Failed to reach Urban Dictionary~ Try again later.");
        free(ctx);
        return;
    }

    struct json_object *root = json_tokener_parse(resp.data);
    http_response_free(&resp);

    if (!root) {
        worker_send(ctx, "Failed to parse Urban Dictionary response~");
        free(ctx);
        return;
    }

    struct json_object *list;
    if (!json_object_object_get_ex(root, "list", &list) ||
        json_object_array_length(list) == 0) {
        json_object_put(root);
        char msg_buf[256];
        snprintf(msg_buf, sizeof(msg_buf), "No results found for `%s`~", ctx->query);
        worker_send(ctx, msg_buf);
        free(ctx);
        return;
    }

    struct json_object *entry = json_object_array_get_idx(list, 0);
    struct json_object *tmp;

    const char *word = ctx->query, *definition = "", *example = "", *author = "Unknown";
    int thumbs_up = 0, thumbs_down = 0;

    if (json_object_object_get_ex(entry, "word", &tmp)) word = json_object_get_string(tmp);
    if (json_object_object_get_ex(entry, "definition", &tmp)) definition = json_object_get_string(tmp);
    if (json_object_object_get_ex(entry, "example", &tmp)) example = json_object_get_string(tmp);
    if (json_object_object_get_ex(entry, "author", &tmp)) author = json_object_get_string(tmp);
    if (json_object_object_get_ex(entry, "thumbs_up", &tmp)) thumbs_up = json_object_get_int(tmp);
    if (json_object_object_get_ex(entry, "thumbs_down", &tmp)) thumbs_down = json_object_get_int(tmp);

    char def_trunc[800], ex_trunc[400];
    truncate_text(definition, def_trunc, 700);
    truncate_text(example, ex_trunc, 350);

    char result[2048];
    snprintf(result, sizeof(result),
        "**%s**\n\n"
        "**Definition:**\n`%s`\n\n"
        "%s%s%s"
        "**Upvotes:** %d | **Downvotes:** %d\n"
        "*Author: %s*",
        word, def_trunc,
        ex_trunc[0] ? "**Example:**\n`" : "", ex_trunc, ex_trunc[0] ? "`\n\n" : "",
        thumbs_up, thumbs_down, author);

    worker_send(ctx, result);
    json_object_put(root);
    free(ctx);
}

void cmd_urban(struct discord *client, const struct discord_interaction *interaction) {
    const char *term = fun_get_option(interaction, "term");
    if (!term || !*term) {
        fun_send_interaction(client, interaction, "Usage: `/urban <term>`~");
        return;
    }

    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_interaction(client, interaction, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->application_id = interaction->application_id;
    strncpy(ctx->interaction_token, interaction->token, sizeof(ctx->interaction_token) - 1);
    ctx->is_slash = 1;
    strncpy(ctx->query, term, sizeof(ctx->query) - 1);
    discord_worker_add(client, urban_worker, ctx);
}

void cmd_urban_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    if (!args || !*args) {
        fun_send_reply(client, msg->channel_id, "Usage: `urban <term>`~");
        return;
    }

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_reply(client, msg->channel_id, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->channel_id = msg->channel_id;
    strncpy(ctx->query, args, sizeof(ctx->query) - 1);
    discord_worker_add(client, urban_worker, ctx);
}

/* --- Hentai Command (Rule34 API) --- */

static const char *BANNED_TAGS[] = {
    "loli", "gore", "guro", "scat", "vore", "underage", "shota"
};
static const int BANNED_TAG_COUNT = sizeof(BANNED_TAGS) / sizeof(BANNED_TAGS[0]);

static int has_banned_tag(const char *input) {
    char lower[512];
    size_t len = strlen(input);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (input[i] >= 'A' && input[i] <= 'Z') ? input[i] + 32 : input[i];
    }
    lower[len] = '\0';

    for (int i = 0; i < BANNED_TAG_COUNT; i++) {
        if (strstr(lower, BANNED_TAGS[i])) return 1;
    }
    return 0;
}

static void hentai_worker(void *data) {
    fun_worker_ctx_t *ctx = data;

    int count = ctx->count;
    if (count < 1) count = 2;
    if (count > 25) count = 25;

    char url[1024];
    if (ctx->query[0]) {
        char encoded[512];
        url_encode(ctx->query, encoded, sizeof(encoded));
        snprintf(url, sizeof(url),
            "https://api.rule34.xxx/index.php?page=dapi&s=post&q=index&json=1&limit=100&tags=%s", encoded);
    } else {
        int page = rand() % 2000;
        snprintf(url, sizeof(url),
            "https://api.rule34.xxx/index.php?page=dapi&s=post&q=index&json=1&limit=100&pid=%d", page);
    }

    http_response_t resp;
    if (http_get(url, &resp) != 0) {
        worker_send(ctx, "Failed to reach the API~ Try again later.");
        free(ctx);
        return;
    }

    struct json_object *root = json_tokener_parse(resp.data);
    http_response_free(&resp);

    if (!root || json_object_get_type(root) != json_type_array ||
        json_object_array_length(root) == 0) {
        if (root) json_object_put(root);
        char msg_buf[256];
        snprintf(msg_buf, sizeof(msg_buf), "No results found%s%s%s~",
            ctx->query[0] ? " for `" : "", ctx->query[0] ? ctx->query : "", ctx->query[0] ? "`" : "");
        worker_send(ctx, msg_buf);
        free(ctx);
        return;
    }

    int arr_len = json_object_array_length(root);
    char result[4000];
    int pos = 0;

    for (int i = 0; i < count && pos < (int)sizeof(result) - 200; i++) {
        int idx = rand() % arr_len;
        struct json_object *item = json_object_array_get_idx(root, idx);
        struct json_object *tmp;

        const char *file_url = NULL;
        if (json_object_object_get_ex(item, "file_url", &tmp) && tmp) {
            file_url = json_object_get_string(tmp);
        }

        if (file_url) {
            pos += snprintf(result + pos, sizeof(result) - pos, "%s\n", file_url);
        }
    }

    worker_send(ctx, pos > 0 ? result : "No images found~");
    json_object_put(root);
    free(ctx);
}

void cmd_hentai(struct discord *client, const struct discord_interaction *interaction) {
    const char *tags = fun_get_option(interaction, "tags");
    const char *count_val = fun_get_option(interaction, "count");
    int count = count_val ? atoi(count_val) : 2;

    struct discord_interaction_response ack = {
        .type = DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE
    };
    discord_create_interaction_response(client, interaction->id, interaction->token, &ack, NULL);

    if (tags && has_banned_tag(tags)) {
        struct discord_edit_original_interaction_response edit = {
            .content = "That is against Discord ToS. I will not search for that~ 💢"
        };
        discord_edit_original_interaction_response(client, interaction->application_id, interaction->token, &edit, NULL);
        return;
    }

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_interaction(client, interaction, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->application_id = interaction->application_id;
    strncpy(ctx->interaction_token, interaction->token, sizeof(ctx->interaction_token) - 1);
    ctx->is_slash = 1;
    ctx->count = count;
    if (tags) strncpy(ctx->query, tags, sizeof(ctx->query) - 1);
    discord_worker_add(client, hentai_worker, ctx);
}

void cmd_hentai_prefix(struct discord *client, const struct discord_message *msg, const char *args) {
    int count = 2;
    const char *tags = NULL;

    if (args && strlen(args) > 0) {
        char first[32] = { 0 };
        sscanf(args, "%31s", first);
        int n = atoi(first);
        if (n > 0) {
            count = n;
            const char *rest = args + strlen(first);
            while (*rest == ' ') rest++;
            if (*rest) tags = rest;
        } else {
            tags = args;
        }
    }

    if (tags && has_banned_tag(tags)) {
        fun_send_reply(client, msg->channel_id, "That is against Discord ToS. I will not search for that~ 💢");
        return;
    }

    fun_worker_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fun_send_reply(client, msg->channel_id, "Memory allocation failed~");
        return;
    }
    ctx->client = client;
    ctx->channel_id = msg->channel_id;
    ctx->count = count;
    if (tags) strncpy(ctx->query, tags, sizeof(ctx->query) - 1);
    discord_worker_add(client, hentai_worker, ctx);
}
