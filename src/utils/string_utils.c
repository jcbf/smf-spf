/*
 * string_utils.c - String manipulation utilities for smf-spf
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

#include "string_utils.h"
#include <ctype.h>
#include <string.h>

void strscpy(char *dst, const char *src, size_t size) {
    size_t i;

    if (!dst || !src || size == 0) {
        if (dst && size > 0) {
            dst[0] = '\0';
        }
        return;
    }

    for (i = 0; i < size - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

void strtolower(char *str) {
    if (!str) {
        return;
    }

    while (*str) {
        if (isascii(*str) && isupper(*str)) {
            *str = tolower(*str);
        }
        str++;
    }
}

char *trim_space(char *str) {
    char *end;

    if (!str) {
        return NULL;
    }

    /* Skip leading whitespace */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    /* All spaces? */
    if (*str == '\0') {
        return str;
    }

    /* Trim trailing whitespace */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    /* Write null terminator */
    *(end + 1) = '\0';

    return str;
}
