/*
 * Yuno Gasai 2 (C Edition) - HTTP Client Helper
 * Copyright (C) 2025 blubskye
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "modules/http_client.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUF_SIZE 4096

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    http_response_t *resp = (http_response_t *)userp;

    /* Grow buffer if needed */
    while (resp->size + total + 1 > resp->capacity) {
        size_t new_cap = resp->capacity * 2;
        char *new_data = realloc(resp->data, new_cap);
        if (!new_data) return 0;
        resp->data = new_data;
        resp->capacity = new_cap;
    }

    memcpy(resp->data + resp->size, contents, total);
    resp->size += total;
    resp->data[resp->size] = '\0';
    return total;
}

void http_global_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void http_global_cleanup(void) {
    curl_global_cleanup();
}

int http_get(const char *url, http_response_t *response) {
    CURL *curl;
    CURLcode res;

    response->data = malloc(INITIAL_BUF_SIZE);
    if (!response->data) return -1;
    response->data[0] = '\0';
    response->size = 0;
    response->capacity = INITIAL_BUF_SIZE;

    curl = curl_easy_init();
    if (!curl) {
        free(response->data);
        response->data = NULL;
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "YunoGasai2-CBot/1.0");

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response->data);
        response->data = NULL;
        response->size = 0;
        return -1;
    }

    return 0;
}

void http_response_free(http_response_t *response) {
    if (response->data) {
        free(response->data);
        response->data = NULL;
    }
    response->size = 0;
    response->capacity = 0;
}
