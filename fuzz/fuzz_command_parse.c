/*
 * Fuzz harness for prefix command parsing from on_message_create() in bot.c
 *
 * Reproduces: prefix check, strtok() splitting, tolower conversion,
 * parse_user_mention() with strtoull().
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_CONTENT_LEN 4096
#define MAX_PREFIX_LEN 16

/* Reproduced from bot.c:790 */
static uint64_t parse_user_mention(const char *mention) {
    const char *start;
    char *end;
    uint64_t id;

    if (mention[0] == '<' && mention[1] == '@') {
        start = mention + 2;
        if (*start == '!') start++;
        id = strtoull(start, &end, 10);
        if (*end == '>') return id;
    }

    id = strtoull(mention, &end, 10);
    if (*end == '\0') return id;

    return 0;
}

/* Reproduced from bot.c on_message_create prefix command dispatch */
static void parse_command(const char *content, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    size_t content_len;
    char content_buf[MAX_CONTENT_LEN];
    char *command;
    char *args;

    if (!content || strlen(content) < prefix_len) return;

    /* Check for prefix */
    if (strncmp(content, prefix, prefix_len) != 0) return;

    /* Copy content to stack buffer */
    content_len = strlen(content + prefix_len);
    if (content_len >= MAX_CONTENT_LEN) {
        content_len = MAX_CONTENT_LEN - 1;
    }
    memcpy(content_buf, content + prefix_len, content_len);
    content_buf[content_len] = '\0';

    /* Parse command */
    command = strtok(content_buf, " \t\n");
    if (!command) return;

    /* Convert command to lowercase */
    for (char *p = command; *p; p++) {
        *p = tolower((unsigned char)*p);
    }

    /* Get remaining args */
    args = strtok(NULL, "");
    if (!args) args = "";

    /* Exercise parse_user_mention on args if present */
    if (strlen(args) > 0) {
        /* Try parsing first arg as a mention */
        char arg_buf[256];
        strncpy(arg_buf, args, sizeof(arg_buf) - 1);
        arg_buf[sizeof(arg_buf) - 1] = '\0';

        char *first_arg = strtok(arg_buf, " ");
        if (first_arg) {
            uint64_t user_id = parse_user_mention(first_arg);
            (void)user_id;
        }
    }

    (void)command;
}

#ifdef AFL_MODE

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    char buf[MAX_CONTENT_LEN];
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    /* Test with common prefixes */
    parse_command(buf, ".");
    parse_command(buf, "!");
    parse_command(buf, "yuno ");

    return 0;
}

#else /* libFuzzer */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char *buf = malloc(size + 1);
    if (!buf) return 0;
    memcpy(buf, data, size);
    buf[size] = '\0';

    /* Test with common prefixes */
    parse_command(buf, ".");
    parse_command(buf, "!");
    parse_command(buf, "yuno ");

    free(buf);
    return 0;
}

#endif
