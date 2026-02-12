/*
 * Fuzz harness for API response JSON parsing patterns from fun.c
 *
 * Reproduces the json_tokener_parse() -> json_object_object_get_ex() ->
 * json_object_get_string() chains used in anime_lookup(), manga_lookup(),
 * urban_lookup(), neko_lookup(), and hentai_lookup().
 *
 * Also tests truncate_text() and has_banned_tag() logic.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>

/* Reproduced from fun.c:315 */
static void truncate_text(const char *src, char *dst, size_t max_len) {
    if (strlen(src) <= max_len) {
        strcpy(dst, src);
    } else {
        strncpy(dst, src, max_len - 3);
        dst[max_len - 3] = '\0';
        strcat(dst, "...");
    }
}

/* Reproduced from fun.c:883 */
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

/* Test anime/manga Jikan API response parsing pattern */
static void fuzz_jikan_response(const char *json_str) {
    struct json_object *root = json_tokener_parse(json_str);
    if (!root) return;

    struct json_object *data_arr;
    if (!json_object_object_get_ex(root, "data", &data_arr) ||
        !json_object_is_type(data_arr, json_type_array) ||
        json_object_array_length(data_arr) == 0) {
        json_object_put(root);
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

    /* Exercise truncate_text */
    char synopsis_trunc[400];
    truncate_text(synopsis, synopsis_trunc, 350);

    /* Exercise snprintf formatting */
    char result[2048];
    snprintf(result, sizeof(result),
        "**%s**%s%s%s\n**Type:** %s | **Status:** %s\n**Episodes:** %d | **Score:** %.1f\n\n%s\n\n%s",
        title, title_en[0] ? " (" : "", title_en, title_en[0] ? ")" : "",
        type, status, episodes, score, synopsis_trunc,
        image_url[0] ? image_url : "");

    (void)result;
    json_object_put(root);
}

/* Test Urban Dictionary response parsing pattern */
static void fuzz_urban_response(const char *json_str) {
    struct json_object *root = json_tokener_parse(json_str);
    if (!root) return;

    struct json_object *list;
    if (!json_object_object_get_ex(root, "list", &list) ||
        !json_object_is_type(list, json_type_array) ||
        json_object_array_length(list) == 0) {
        json_object_put(root);
        return;
    }

    struct json_object *entry = json_object_array_get_idx(list, 0);
    struct json_object *tmp;

    const char *word = "", *definition = "", *example = "", *author = "Unknown";
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
        "**%s**\n\n**Definition:**\n`%s`\n\n%s%s%s**Upvotes:** %d | **Downvotes:** %d\n*Author: %s*",
        word, def_trunc,
        ex_trunc[0] ? "**Example:**\n`" : "", ex_trunc, ex_trunc[0] ? "`\n\n" : "",
        thumbs_up, thumbs_down, author);

    (void)result;
    json_object_put(root);
}

/* Test neko response parsing */
static void fuzz_neko_response(const char *json_str) {
    struct json_object *root = json_tokener_parse(json_str);
    if (!root) return;

    struct json_object *neko_url;
    if (json_object_object_get_ex(root, "neko", &neko_url)) {
        const char *url = json_object_get_string(neko_url);
        (void)url;
    }

    json_object_put(root);
}

/* Test Rule34 response parsing + banned tag check */
static void fuzz_rule34_response(const char *json_str) {
    struct json_object *root = json_tokener_parse(json_str);
    if (!root) return;

    if (json_object_get_type(root) != json_type_array ||
        json_object_array_length(root) == 0) {
        json_object_put(root);
        return;
    }

    int arr_len = json_object_array_length(root);
    char result[4000];
    int pos = 0;

    int count = arr_len < 5 ? arr_len : 5;
    for (int i = 0; i < count && pos < (int)sizeof(result) - 200; i++) {
        struct json_object *item = json_object_array_get_idx(root, i);
        struct json_object *tmp;

        const char *file_url = NULL;
        if (json_object_object_get_ex(item, "file_url", &tmp) && tmp) {
            file_url = json_object_get_string(tmp);
        }

        if (file_url) {
            pos += snprintf(result + pos, sizeof(result) - pos, "%s\n", file_url);
        }
    }

    (void)result;
    json_object_put(root);
}

#ifdef AFL_MODE

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0) { fclose(f); return 0; }
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return 1; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    /* Test all parsing patterns */
    has_banned_tag(buf);
    fuzz_jikan_response(buf);
    fuzz_urban_response(buf);
    fuzz_neko_response(buf);
    fuzz_rule34_response(buf);

    free(buf);
    return 0;
}

#else /* libFuzzer */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* Null-terminate for json_tokener_parse */
    char *buf = malloc(size + 1);
    if (!buf) return 0;
    memcpy(buf, data, size);
    buf[size] = '\0';

    /* Test banned tag check */
    has_banned_tag(buf);

    /* Test all JSON response parsing patterns */
    fuzz_jikan_response(buf);
    fuzz_urban_response(buf);
    fuzz_neko_response(buf);
    fuzz_rule34_response(buf);

    free(buf);
    return 0;
}

#endif
