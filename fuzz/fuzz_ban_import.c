/*
 * Fuzz harness for ban file import parsing from terminal_cmd_timportbans()
 *
 * Targets: fread + ftell for file size, json_tokener_parse() on file contents,
 * json_object_array iteration, json_object_get_string() per element.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>

/* Reproduced from terminal.c timportbans parsing logic */
static int parse_ban_file(const char *data, size_t len) {
    char *buf = malloc(len + 1);
    if (!buf) return -1;
    memcpy(buf, data, len);
    buf[len] = '\0';

    struct json_object *root = json_tokener_parse(buf);
    free(buf);

    if (!root) return -1;

    if (json_object_get_type(root) != json_type_array) {
        json_object_put(root);
        return -1;
    }

    int count = json_object_array_length(root);
    int imported = 0;

    for (int i = 0; i < count; i++) {
        struct json_object *item = json_object_array_get_idx(root, i);
        const char *id_str = json_object_get_string(item);
        if (!id_str) continue;

        /* Simulate strtoull parsing like the real code does */
        char *end;
        uint64_t user_id = strtoull(id_str, &end, 10);
        if (user_id > 0) {
            imported++;
        }
        (void)user_id;
    }

    json_object_put(root);
    return imported;
}

#ifdef AFL_MODE

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0) { fclose(f); return 0; }
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len);
    if (!buf) { fclose(f); return 1; }
    fread(buf, 1, len, f);
    fclose(f);

    parse_ban_file(buf, len);
    free(buf);
    return 0;
}

#else /* libFuzzer */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    parse_ban_file((const char *)data, size);
    return 0;
}

#endif
