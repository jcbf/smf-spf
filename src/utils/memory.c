/*
 * memory.c - Memory safety utilities for smf-spf
 *
 * This file is part of smf-spf.
 *
 * smf-spf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * smf-spf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "memory.h"
#include "logging.h"
#include <string.h>

void *safe_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);

    if (!ptr && count > 0 && size > 0) {
        log_message(LOG_ERR, "Memory allocation failed: calloc(%zu, %zu)",
                    count, size);
    }

    return ptr;
}

char *safe_strdup(const char *str) {
    char *dup;

    if (!str) {
        return NULL;
    }

    dup = strdup(str);
    if (!dup) {
        log_message(LOG_ERR, "Memory allocation failed: strdup");
    }

    return dup;
}
