/*
 * Fuzz harness for config_load() — src/config.c
 *
 * Targets: JSON parsing, strncpy with json_object_get_string(),
 * ftell/malloc interaction, type confusion in json-c getters.
 */

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef AFL_MODE

#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    yuno_config_t config;
    config_init_defaults(&config);
    config_load(&config, argv[1]);
    return 0;
}

#else /* libFuzzer */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* Write fuzz input to a temp file since config_load() reads from a path */
    char tmppath[] = "/tmp/fuzz_config_XXXXXX";
    int fd = mkstemp(tmppath);
    if (fd < 0) return 0;

    write(fd, data, size);
    close(fd);

    yuno_config_t config;
    config_init_defaults(&config);
    config_load(&config, tmppath);

    unlink(tmppath);
    return 0;
}

#endif
