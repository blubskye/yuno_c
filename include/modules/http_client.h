/*
 * Yuno Gasai 2 (C Edition) - HTTP Client Helper
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef YUNO_MODULES_HTTP_CLIENT_H
#define YUNO_MODULES_HTTP_CLIENT_H

#include <stddef.h>

/* Dynamic response buffer */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} http_response_t;

/* Initialize/cleanup */
void http_global_init(void);
void http_global_cleanup(void);

/* Perform HTTP GET, returns 0 on success. Caller must call http_response_free(). */
int http_get(const char *url, http_response_t *response);

/* Free response buffer */
void http_response_free(http_response_t *response);

#endif /* YUNO_MODULES_HTTP_CLIENT_H */
